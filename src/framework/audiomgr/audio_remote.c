/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "os_mem.h"
#include "os_sync.h"
#include "trace.h"
#include "audio_remote.h"
#include "jitter_buffer.h"
#include "media_buffer.h"

/* TODO Remove Start */
#include "audio_path.h"
#include "bt_mgr.h"
extern bool bt_clk_compare(uint32_t bt_clk_a, uint32_t bt_clk_b);
/* TODO Remove End */

T_AUDIO_REMOTE_DB *audio_remote_db;

const uint8_t sync_play_margin_table[] =
{
    5,      /* AUDIO_FORMAT_TYPE_PCM: 0 */
    5,      /* AUDIO_FORMAT_TYPE_CVSD: 1 */
    5,      /* AUDIO_FORMAT_TYPE_MSBC: 2 */
    10,      /* AUDIO_FORMAT_TYPE_SBC: 3 */
    14,      /* AUDIO_FORMAT_TYPE_AAC: 4 */
    10,      /* AUDIO_FORMAT_TYPE_OPUS: 5 */
    14,      /* AUDIO_FORMAT_TYPE_FLAC: 6 */
    14,      /* AUDIO_FORMAT_TYPE_MP3: 7 */
    10,      /* AUDIO_FORMAT_TYPE_LC3: 8 */
    12,      /* AUDIO_FORMAT_TYPE_LDAC: 9 */
    12,      /* AUDIO_FORMAT_TYPE_LHDC: 10 */
};

typedef struct
{
    uint16_t msg_id;
    uint16_t padding;
    uint8_t msg[0];
} T_AUDIO_REMOTE_BUFFER_MSG_RELAY;

typedef struct
{
    uint32_t remote_endpoint;
    uint8_t  msg[0];
} T_AUDIO_REMOTE_INTERNAL_MSG;

typedef enum
{
    AUDIO_REMOTE_BUFFER_SYNC_REQ                    = 0x0000,
    AUDIO_REMOTE_BUFFER_ALIGN_REQ                   = 0x0001,
    AUDIO_REMOTE_BUFFER_ALIGN_RSP                   = 0x0002,
    AUDIO_REMOTE_BUFFER_SYNC_PLAY                   = 0x0003,
    AUDIO_REMOTE_BUFFER_SEAMLESS_JOIN_INFO          = 0x0004,
    AUDIO_REMOTE_BUFFER_SEAMLESS_JOIN_PLAY          = 0x0005,
    AUDIO_REMOTE_BUFFER_NOT_SEAMLESS_JOIN           = 0x0006,
    AUDIO_REMOTE_BUFFER_SEC_ALIGNING                = 0x0007,
    AUDIO_REMOTE_BUFFER_LOW_LATENCY_SYNC_PLAY       = 0x0008,
    AUDIO_REMOTE_BUFFER_ULTRA_LOW_LATENCY_SYNC_PLAY = 0x0009,

    AUDIO_REMOTE_BUFFER_MSG_RELAY                   = 0xFFFF,
} T_AUDIO_REMOTE_BUFFER_RELAY_MSG;

typedef enum
{
    AUDIO_TIMER_SEAMLESS_JOIN_FAILED,
    AUDIO_TIMER_SEAMLESS_JOIN_SEQUENCE_CHECK,
} T_AUDIO_REMOTE_TIMER;

typedef struct t_audio_remote_instance
{
    struct t_audio_remote_instance *p_next;
    void *buffer_ent;
    void *latest_rec;
    void *recs;
    void *seamless_join_abort_timer_handle;
    void *seamless_join_seq_contiuity_check_timer;
    T_AUDIO_REMOTE_EVT_CBACK cback;
    T_AUDIO_REMOTE_RELAY_CBACK relay_cback;
    T_AUDIO_STREAM_TYPE sync_type;
    T_AUDIO_REMOTE_STATE state;
    uint16_t buffer_sync_trigger;
    uint32_t join_clk;
    uint32_t join_packet_id;
    uint16_t join_local_seq;
    uint16_t join_frame_counter;
    uint16_t delta_frame;
    uint16_t base_count;
    uint8_t seamless_join_retry_cnt;
    int     frame_diff;
    int     seq_diff;
    uint8_t endpoint;
    uint8_t seamless_join_lost_packet_cnt;
    uint8_t seamless_join_lost_packet_cnt_last;
    uint8_t slave_audio_sync;
    uint8_t sync_peroid_trigger_cnt;
    uint8_t sync_packet_cnt;
    int32_t asrc_ppm;
} T_AUDIO_REMOTE_INSTANCE;

void audio_remote_relay_cback(uint16_t event, T_REMOTE_RELAY_STATUS status,
                              void *buf, uint16_t len);

T_AUDIO_REMOTE_HANDLE media_buffer_packet_record_init(uint16_t record_count)
{
    T_MEDIA_PACKET_RECORD *p_rec;
    uint8_t i;

    p_rec = os_mem_zalloc2(sizeof(T_MEDIA_PACKET_RECORD) * record_count);

    if (p_rec == NULL)
    {
        return NULL;
    }

    for (i = 0; i < record_count - 1; ++i)
    {
        p_rec[i].used = 0;
        p_rec[i].p_next = &p_rec[i + 1];
        p_rec[i + 1].p_prev = &p_rec[i];
    }
    p_rec[i].used = 0;
    p_rec[i].p_next = &p_rec[0];
    p_rec[0].p_prev = &p_rec[i];

    return p_rec;
}


static bool audio_remote_handle_check(T_AUDIO_REMOTE_INSTANCE *handle)
{
    if (handle != NULL)
    {
        return os_queue_search(&audio_remote_db->remote_db_list, handle);
    }

    return false;
}

static T_AUDIO_REMOTE_INSTANCE *audio_remote_handle_find(uint8_t endpoint)
{
    T_AUDIO_REMOTE_INSTANCE *handle;

    handle = os_queue_peek(&audio_remote_db->remote_db_list, 0);
    while (handle != NULL)
    {
        if (handle->endpoint == endpoint)
        {
            break;
        }

        handle = handle->p_next;
    }

    return handle;
}

static bool audio_remote_handle_delete(T_AUDIO_REMOTE_INSTANCE *handle)
{
    if (handle != NULL)
    {
        if (handle->seamless_join_abort_timer_handle != NULL)
        {
            sys_timer_delete(handle->seamless_join_abort_timer_handle);
        }

        if (handle->seamless_join_seq_contiuity_check_timer != NULL)
        {
            sys_timer_delete(handle->seamless_join_seq_contiuity_check_timer);
        }
        os_queue_delete(&audio_remote_db->remote_db_list, handle);
        os_mem_free(handle);
        return true;
    }

    return false;
}

bool audio_remote_init(void)
{
    int32_t ret = 0;

    audio_remote_db = os_mem_alloc2(sizeof(T_AUDIO_REMOTE_DB));
    if (audio_remote_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    audio_remote_db->relay_handle = remote_relay_register(audio_remote_relay_cback);
    if (audio_remote_db->relay_handle == NULL)
    {
        ret = 2;
        goto fail_register_cback;
    }

    audio_remote_db->seamless_join = true;
    audio_remote_db->force_join = false;
    audio_remote_db->endpoint_count = 0;
    os_queue_init(&audio_remote_db->remote_db_list);
    return true;

fail_register_cback:
    os_mem_free(audio_remote_db);
    audio_remote_db = NULL;
fail_alloc_db:
    AUDIO_PRINT_ERROR1("audio_remote_init: failed %d", -ret);
    return false;
}

bool audio_remote_deinit(void)
{
    T_AUDIO_REMOTE_INSTANCE *handle;
    T_AUDIO_REMOTE_INSTANCE *handle_next;

    handle = os_queue_peek(&audio_remote_db->remote_db_list, 0);
    while (handle != NULL)
    {
        if (handle->seamless_join_abort_timer_handle != NULL)
        {
            sys_timer_delete(handle->seamless_join_abort_timer_handle);
        }

        if (handle->seamless_join_seq_contiuity_check_timer != NULL)
        {
            sys_timer_delete(handle->seamless_join_seq_contiuity_check_timer);
        }
        os_queue_delete(&audio_remote_db->remote_db_list, handle);
        handle_next = handle->p_next;
        os_mem_free(handle);
        handle = handle_next;
    }
    remote_relay_unregister(audio_remote_db->relay_handle);
    os_mem_free(audio_remote_db);
    return true;
}

void audio_remote_media_buffer_unregister(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    if (handle == NULL)
    {
        return;
    }
    if (remote_instance->recs)
    {
        os_mem_free(remote_instance->recs);
    }

    if (audio_remote_handle_check(remote_instance) == true)
    {
        audio_remote_handle_delete(remote_instance);
    }
}

T_AUDIO_REMOTE_HANDLE audio_remote_media_buffer_register(T_AUDIO_STREAM_TYPE type,
                                                         void *buffer_ent,
                                                         T_AUDIO_REMOTE_EVT_CBACK event_cback,
                                                         T_AUDIO_REMOTE_RELAY_CBACK relay_cback)
{
    T_AUDIO_REMOTE_INSTANCE *handle = os_mem_zalloc2(sizeof(T_AUDIO_REMOTE_INSTANCE));

    if (handle == NULL)
    {
        return NULL;
    }

    handle->buffer_ent = buffer_ent;

    if (type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        handle->sync_type = AUDIO_STREAM_TYPE_PLAYBACK;
        handle->recs = media_buffer_packet_record_init(MEDIA_SYNC_RECORD_CNT);
        handle->latest_rec = handle->recs;
    }
    else
    {
        handle->sync_type = AUDIO_STREAM_TYPE_VOICE;
    }
    handle->cback = event_cback;
    handle->relay_cback = relay_cback;
    handle->endpoint = audio_remote_db->endpoint_count++;

    os_queue_in(&audio_remote_db->remote_db_list, handle);

    AUDIO_PRINT_TRACE4("audio_remote_media_buffer_register: buffer %p id %d remote %p type %d ep %d",
                       buffer_ent, handle, handle->sync_type, handle->endpoint);

    return handle;
}

void audio_remote_timeout_cback(T_SYS_TIMER_HANDLE handle)
{
    uint32_t timer_id;
    uint16_t event;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_REMOTE_INSTANCE *remote_instance;

    timer_id = sys_timer_id_get(handle);
    event = timer_id >> 16;

    remote_instance = audio_remote_handle_find(timer_id & 0x0000ffff);

    if (remote_instance == NULL)
    {
        return;
    }

    buffer_ent = remote_instance->buffer_ent;

    AUDIO_PRINT_TRACE3("audio_remote_timeout_cback: timer_id 0x%02X handle %p buffer_id %d", timer_id,
                       handle, buffer_ent->buffer_id);

    switch (event)
    {
    case AUDIO_TIMER_SEAMLESS_JOIN_SEQUENCE_CHECK:
        {
            remote_instance->seamless_join_lost_packet_cnt_last =
                remote_instance->seamless_join_lost_packet_cnt;
            remote_instance->seamless_join_lost_packet_cnt = 0;
            sys_timer_start(remote_instance->seamless_join_seq_contiuity_check_timer);
        }
        break;


    case AUDIO_TIMER_SEAMLESS_JOIN_FAILED:
        {
            audio_remote_seamless_join_retry(remote_instance, MEDIA_BUFFER_SEAMLESS_TIMEOUT);
        }
        break;

    default:
        break;
    }

}

static bool audio_remote_internal_async_msg_relay(T_AUDIO_REMOTE_INSTANCE *remote_instance,
                                                  uint16_t               msg_id,
                                                  void                  *msg_buf,
                                                  uint16_t               msg_len,
                                                  bool                   loopback)
{

    bool ret;
    T_AUDIO_REMOTE_INTERNAL_MSG *async_msg;
    uint16_t malloc_len = msg_len + sizeof(T_AUDIO_REMOTE_INTERNAL_MSG);

    if (audio_remote_handle_check(remote_instance) == false)
    {
        return false;
    }
    async_msg = (T_AUDIO_REMOTE_INTERNAL_MSG *)os_mem_zalloc2(malloc_len);
    if (async_msg == NULL)
    {
        return false;
    }

    async_msg->remote_endpoint = remote_instance->endpoint;
    memcpy(async_msg->msg, msg_buf, msg_len);

    ret = remote_async_msg_relay(audio_remote_db->relay_handle,
                                 msg_id,
                                 async_msg,
                                 malloc_len,
                                 loopback);
    os_mem_free(async_msg);
    return ret;

}

static bool audio_remote_internal_sync_msg_relay(T_AUDIO_REMOTE_INSTANCE *remote_instance,
                                                 uint16_t               msg_id,
                                                 void                  *msg_buf,
                                                 uint16_t               msg_len,
                                                 T_REMOTE_TIMER_TYPE    timer_type,
                                                 uint32_t               timer_period,
                                                 bool                   loopback)
{
    bool ret;
    T_AUDIO_REMOTE_INTERNAL_MSG *sync_msg;
    uint16_t malloc_len = msg_len + sizeof(T_AUDIO_REMOTE_INTERNAL_MSG);

    if (audio_remote_handle_check(remote_instance) == false)
    {
        return false;
    }
    sync_msg = (T_AUDIO_REMOTE_INTERNAL_MSG *)os_mem_zalloc2(malloc_len);
    if (sync_msg == NULL)
    {
        return false;
    }

    sync_msg->remote_endpoint = remote_instance->endpoint;
    memcpy(sync_msg->msg, msg_buf, msg_len);

    ret = remote_sync_msg_relay(audio_remote_db->relay_handle,
                                msg_id,
                                sync_msg,
                                malloc_len,
                                timer_type,
                                timer_period,
                                loopback);
    os_mem_free(sync_msg);
    return ret;

}

T_MEDIA_PACKET_RECORD *get_latest_rec(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    T_MEDIA_PACKET_RECORD *p_rec;

    p_rec = remote_instance->latest_rec;

    if (p_rec == NULL)
    {
        return NULL;
    }
    if (p_rec->used == 0)
    {
        return NULL;
    }
    else
    {
        return p_rec;
    }
}

T_MEDIA_PACKET_RECORD *get_oldest_rec(T_AUDIO_REMOTE_HANDLE handle)
{
    T_MEDIA_PACKET_RECORD *p_rec;
    p_rec = get_latest_rec(handle);
    if (p_rec == NULL)
    {
        return NULL;
    }

    while (p_rec->p_next->used == 0)
    {
        p_rec = p_rec->p_next;
    }
    return p_rec->p_next;
}

int cacu_diff_u16(uint16_t x, uint16_t y)
{
    int seq_diff;
    if (x > y)
    {
        if (x - y > 0x7fff) //0xff-0x00
        {
            seq_diff = -((y + 0x10000) - x);
        }
        else
        {
            seq_diff = x - y;
        }
    }
    else
    {
        if (y - x > 0x7fff) //0xff-0x00
        {
            seq_diff = x + 0x10000 - y ;
        }
        else
        {
            seq_diff = x - y;
        }
    }
    return seq_diff;
}

void audio_remote_cancel_seamless_join(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (handle == NULL)
    {
        return;
    }

    audio_remote_internal_async_msg_relay(remote_instance,
                                          AUDIO_REMOTE_BUFFER_NOT_SEAMLESS_JOIN,
                                          &remote_instance->sync_type,
                                          1,
                                          false);
    audio_remote_reset(handle);
}

void media_buffer_packet_record_erase(T_AUDIO_REMOTE_HANDLE handle)
{
    T_MEDIA_PACKET_RECORD *p_rec;
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    p_rec = get_latest_rec(handle);
    if (p_rec == NULL)
    {
        return;
    }
    p_rec->used = 0;
    p_rec = p_rec->p_next;
    while (p_rec != remote_instance->latest_rec)
    {
        p_rec->used = 0;
        p_rec = p_rec->p_next;
    }
}


void media_buffer_packet_record_add(T_AUDIO_REMOTE_HANDLE handle, void *p_packet)
{
    T_MEDIA_PACKET_RECORD *p_rec;
    T_MEDIA_DATA_HDR *pmedia_packet = p_packet;
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (handle == NULL)
    {
        return;
    }

    if (pmedia_packet == NULL || remote_instance->latest_rec == NULL)
    {
        return;
    }
    p_rec = get_latest_rec(handle);
    if (p_rec == NULL)
    {
        p_rec = remote_instance->latest_rec;
    }
    else
    {
        p_rec = p_rec->p_next;
    }
    p_rec->frame_counter = pmedia_packet->frame_counter;
    p_rec->local_seq_num = pmedia_packet->local_seq_number;
    p_rec->avdtp_seq_num = pmedia_packet->avdtp_seq_number;
    p_rec->frame_number = pmedia_packet->frame_number;
    p_rec->timestamp = pmedia_packet->bt_clk;
    p_rec->used = 1;
    p_rec->seq_offset = 0;
    remote_instance->latest_rec = p_rec;
    /*
    AUDIO_PRINT_TRACE4("record_add: local %u avdtp %u fn %u fcounter %u", p_rec->local_seq_num,
                       p_rec->avdtp_seq_num, p_rec->frame_number, p_rec->frame_counter);
    */
}

T_MEDIA_PACKET_RECORD *media_buffer_packet_record_search_by_avdtp_seq(T_AUDIO_REMOTE_HANDLE handle,
                                                                      uint16_t avdtp_seq)
{
    T_MEDIA_PACKET_RECORD *p_latest_rec;
    T_MEDIA_PACKET_RECORD *p_oldest_rec;
    p_latest_rec = get_latest_rec(handle);
    p_oldest_rec = get_oldest_rec(handle);
    if (p_oldest_rec == NULL || p_latest_rec == NULL)
    {
        return NULL;
    }
    if (p_oldest_rec == p_latest_rec)
    {
        //not handle is fine
        return NULL;
    }
    while (p_oldest_rec != p_latest_rec)
    {
        if (p_latest_rec->avdtp_seq_num == avdtp_seq)
        {
            return p_latest_rec;
        }
        p_latest_rec = p_latest_rec->p_prev;
    }
    if (p_oldest_rec->avdtp_seq_num == avdtp_seq)
    {
        return p_oldest_rec;
    }
    else
    {
        return NULL;
    }
}

int media_buffer_packet_record_fix_by_local_seq(T_AUDIO_REMOTE_HANDLE handle, uint16_t local_seq,
                                                uint16_t frame_counter,
                                                uint8_t frame_number)
{
    T_MEDIA_PACKET_RECORD *p_rec;
    T_MEDIA_PACKET_RECORD *p_rec_oldest;
    T_MEDIA_PACKET_RECORD *p_rec_latest;
    int diff;

    if (handle == NULL)
    {
        return 0;
    }

    p_rec_latest = get_latest_rec(handle);
    p_rec_oldest = get_oldest_rec(handle);
    p_rec = p_rec_latest;

    if (p_rec == NULL)
    {
        return 0;
    }

    while (p_rec->local_seq_num != local_seq)
    {
        p_rec = p_rec->p_prev;
        if (p_rec == p_rec_oldest)
        {
            break;
        }
    }

    if (p_rec->local_seq_num != local_seq)
    {
        AUDIO_PRINT_ERROR0("record_update_by_local_seq: failed");
        return 0;
    }

    diff = frame_number - p_rec->frame_number;

    if (diff == 0)
    {
        AUDIO_PRINT_TRACE0("record_update_by_local_seq: frames equals");
        return 0;
    }

    AUDIO_PRINT_TRACE7("record_update_by_local_seq: frame_diff %d local seq %u(%d) frame_num %u->%u frameounter %u->%u",
                       diff, p_rec->local_seq_num, p_rec->seq_offset, p_rec->frame_number, frame_number,
                       p_rec->frame_counter, p_rec->frame_counter + diff);
    p_rec->frame_counter += diff;
    p_rec->frame_number = frame_number;

    while (p_rec != p_rec_latest)
    {
        p_rec->p_next->frame_counter += diff;
        p_rec = p_rec->p_next;
        AUDIO_PRINT_TRACE5("record_update_by_local_seq: local seq %u(%d) frame_num %u frameounter %u + %d",
                           p_rec->local_seq_num, p_rec->seq_offset, p_rec->frame_number, p_rec->frame_counter, diff);
    }

    return diff;
}

void media_buffer_packet_record_update_all(T_AUDIO_REMOTE_HANDLE handle,
                                           T_MEDIA_PACKET_RECORD *p_rec, T_MEDIA_BUFFER_SYNC_UNIT *p_unit)
{
    T_MEDIA_PACKET_RECORD *p_latest_rec;
    int diff;
    int frame_diff;

    p_latest_rec = get_latest_rec(handle);
    if (p_latest_rec == NULL || p_rec == NULL)
    {
        return;
    }

    diff = cacu_diff_u16(p_unit->local_seq_num, p_rec->local_seq_num + p_rec->seq_offset);
    frame_diff = cacu_diff_u16(p_unit->frame_counter, p_rec->frame_counter);

    p_rec->seq_offset += diff;
    p_rec->frame_counter += frame_diff;

    AUDIO_PRINT_TRACE5("record_update: seqdiff %d local %u(%d)->%d framecounter %d", diff,
                       p_rec->local_seq_num, p_rec->seq_offset, p_rec->local_seq_num + p_rec->seq_offset,
                       p_rec->frame_counter);
    while (p_rec != p_latest_rec)
    {
        p_rec->p_next->frame_counter += frame_diff;
        p_rec->p_next->seq_offset += diff;
        p_rec = p_rec->p_next;
        AUDIO_PRINT_TRACE5("record_update: seqdiff %d local %u(%d)->%d framecounter %d", diff,
                           p_rec->local_seq_num, p_rec->seq_offset, p_rec->local_seq_num + p_rec->seq_offset,
                           p_rec->frame_counter);
    }
}

uint8_t media_buffer_get_rec_cnt(T_AUDIO_REMOTE_HANDLE handle)
{
    uint8_t cnt_in_queue = 0;
    T_MEDIA_PACKET_RECORD *p_rec;
    T_MEDIA_PACKET_RECORD *p_latest_rec;
    p_rec = get_oldest_rec(handle);
    p_latest_rec = get_latest_rec(handle);
    if (p_rec == NULL || p_latest_rec == NULL)
    {
        return 0;
    }

    cnt_in_queue++;
    while (p_rec != p_latest_rec)
    {
        cnt_in_queue++;
        p_rec = p_rec->p_next;
    }
    return cnt_in_queue;
}

uint8_t media_buffer_get_rec_data(T_AUDIO_REMOTE_HANDLE handle, T_MEDIA_BUFFER_SYNC_UNIT *p_data,
                                  uint8_t cnt)
{
    uint8_t cnt_in_queue = 0;
    uint8_t i;
    T_MEDIA_PACKET_RECORD *p_latest_rec;

    p_latest_rec = get_latest_rec(handle);
    if (p_latest_rec == NULL)
    {
        return 0;
    }
    cnt_in_queue = media_buffer_get_rec_cnt(handle);
    cnt = cnt_in_queue < cnt ? cnt_in_queue : cnt;
    for (i = 0; i < cnt; i++)
    {
        p_data[i].frame_counter = p_latest_rec->frame_counter;
        p_data[i].local_seq_num = p_latest_rec->local_seq_num;
        p_data[i].l2cap_seq_num = p_latest_rec->avdtp_seq_num;
        p_latest_rec = p_latest_rec->p_prev;
    }
    return cnt;
}

uint8_t media_buffer_packet_record_get_timestamps(T_AUDIO_REMOTE_HANDLE handle,
                                                  uint32_t *p_timestamp, uint8_t cnt)
{
    uint8_t cnt_in_queue = 0;
    uint8_t i;
    T_MEDIA_PACKET_RECORD *p_latest_rec;

    if (handle == NULL)
    {
        return 0;
    }

    p_latest_rec = get_latest_rec(handle);
    if (p_latest_rec == NULL)
    {
        return 0;
    }
    cnt_in_queue = media_buffer_get_rec_cnt(handle);
    cnt = cnt_in_queue < cnt ? cnt_in_queue : cnt;
    for (i = 0; i < cnt; i++)
    {
        p_timestamp[i] = p_latest_rec->timestamp;
        p_latest_rec = p_latest_rec->p_prev;
    }
    return cnt;
}

uint8_t audio_mgr_check_seq_continuity_in_record(T_AUDIO_REMOTE_HANDLE handle)
{
    T_MEDIA_PACKET_RECORD *p_rec;
    T_MEDIA_PACKET_RECORD *p_rec_next;
    uint8_t miss_count = 0;
    uint8_t repeat_count = 0;
    uint8_t rec_count = media_buffer_get_rec_cnt(handle);

    p_rec = get_oldest_rec(handle);

    if (p_rec == NULL)
    {
        return 0;
    }
    p_rec_next = p_rec->p_next;

    for (uint8_t i = 0; i < (rec_count - 1); i++)
    {
        if (p_rec_next->avdtp_seq_num == p_rec->avdtp_seq_num)
        {
            repeat_count++;
        }
        else
        {
            repeat_count = 0;
            miss_count += (uint8_t)(p_rec_next->avdtp_seq_num - (uint16_t)(p_rec->avdtp_seq_num + 1));
        }
        p_rec = p_rec_next;
        p_rec_next = p_rec_next->p_next;
    }
    AUDIO_PRINT_TRACE2("audio_mgr_check_seq_continuity_in_record: check count %d, miss count %d",
                       media_buffer_get_rec_cnt(handle), miss_count + repeat_count);
    return miss_count + repeat_count;
}

void audio_remote_seamless_join_info_req(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
    {
        audio_path_synchronization_join_set(buffer_ent->p_cfg->attached_path_handle, 1);
    }
}

void audio_remote_dsp_seamless_join_handle(T_MEDIA_BUFFER_ENTITY *buffer_ent, void *param)
{
    T_AUDIO_MSG_DSP_JOIN_INFO *dsp_join_info = (T_AUDIO_MSG_DSP_JOIN_INFO *)param;
    uint16_t framecounter_last;
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)
                                               buffer_ent->audio_remote_handle;

    if (remote_instance == NULL)
    {
        return;
    }

    media_buffer_get_downstream_info(buffer_ent, &framecounter_last, NULL, NULL);
    remote_instance->join_clk = dsp_join_info->join_clk;

    if (dsp_join_info->join_frame - framecounter_last < dsp_join_info->delta_frame)
    {
        remote_instance->delta_frame = dsp_join_info->join_frame - framecounter_last;
    }
    else
    {
        remote_instance->delta_frame = dsp_join_info->join_frame - (framecounter_last +
                                                                    0x10000);
    }
    remote_instance->base_count = framecounter_last;
    AUDIO_PRINT_TRACE4("audio_remote_dsp_seamless_join_handle: join_clk 0x%x join_frame %u, delta_frame %u->%u",
                       remote_instance->join_clk, dsp_join_info->join_frame,
                       dsp_join_info->delta_frame, remote_instance->delta_frame);
    audio_remote_set_state(remote_instance, AUDIO_REMOTE_STATE_SEAMLESS_JOIN);
}

bool audio_remote_buffer_sync_req_handle(T_AUDIO_REMOTE_INSTANCE *remote_instance, uint8_t *pdata,
                                         uint8_t len)
{
    T_MEDIA_BUFFER_SYNC_UNIT *p_sync_units = (T_MEDIA_BUFFER_SYNC_UNIT *)pdata;
    uint8_t sync_pkt_cnt = len / sizeof(T_MEDIA_BUFFER_SYNC_UNIT);
    T_MEDIA_PACKET_RECORD *p_rec = NULL;
    uint8_t i;
    bool rec_found = false;

    for (i = 0; i < sync_pkt_cnt; i++)
    {
        p_rec = media_buffer_packet_record_search_by_avdtp_seq(remote_instance,
                                                               p_sync_units[i].l2cap_seq_num);
        if (p_rec != NULL)
        {
            if ((uint16_t)(p_rec->local_seq_num + p_rec->seq_offset) != p_sync_units[i].local_seq_num ||
                p_rec->frame_counter != p_sync_units[i].frame_counter)
            {
                remote_instance->frame_diff += cacu_diff_u16(p_sync_units[i].frame_counter, p_rec->frame_counter);
                remote_instance->seq_diff += cacu_diff_u16(p_sync_units[i].local_seq_num,
                                                           p_rec->local_seq_num + p_rec->seq_offset);
                media_buffer_packet_record_update_all(remote_instance, p_rec, &p_sync_units[i]);
                AUDIO_PRINT_INFO3("media_buffer_sync_req_handle: mismatch seq diff %d frame diff %d avdtp %u",
                                  remote_instance->seq_diff, remote_instance->frame_diff, p_rec->avdtp_seq_num);
                rec_found = true;
                break;
            }
            else
            {
                remote_instance->frame_diff = 0;
                remote_instance->seq_diff = 0;
                AUDIO_PRINT_TRACE4("media_buffer_sync_req_handle: match local %u(%d) avdtp %u freamcount %u",
                                   p_rec->local_seq_num, p_rec->seq_offset, p_rec->avdtp_seq_num, p_rec->frame_counter);
                rec_found = true;
                break;
            }
        }
    }
    if (rec_found == false)
    {
        T_MEDIA_PACKET_RECORD *p_latest_rec = NULL;
        AUDIO_PRINT_TRACE0("media_buffer_sync_req_handle: sec can't find match record");
        p_latest_rec = get_latest_rec(remote_instance);
        if (p_latest_rec == NULL)
        {
            return false;
        }
        if (cacu_diff_u16(p_sync_units[sync_pkt_cnt - 1].l2cap_seq_num, p_latest_rec->avdtp_seq_num) > 0)
        {
            AUDIO_PRINT_TRACE0("media_buffer_sync_req_handle: sec may dup too many packets");
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return true;
    }
}

bool audio_remote_peroid_sync_check(T_AUDIO_REMOTE_HANDLE handle, uint16_t buffer_sync_trigger_cnt)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (audio_remote_handle_check(remote_instance) == false)
    {
        return false;
    }

    if (buffer_sync_trigger_cnt >= remote_instance->sync_peroid_trigger_cnt)
    {
        return true;
    }
    return false;
}

void audio_remote_buffer_sync_req(T_AUDIO_REMOTE_HANDLE handle)
{
    T_MEDIA_BUFFER_SYNC_UNIT *p_sync_units;
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (handle == NULL)
    {
        return;
    }

    if (audio_remote_get_state(handle) != AUDIO_REMOTE_STATE_SYNCED)
    {
        return;
    }

    uint8_t pkt_cnt;

    pkt_cnt = media_buffer_get_rec_cnt(handle);
    if (pkt_cnt == 0)
    {
        AUDIO_PRINT_TRACE0("media_buffer_sync_req: no data in history");
        return;
    }
    pkt_cnt = pkt_cnt > remote_instance->sync_packet_cnt ? remote_instance->sync_packet_cnt : pkt_cnt;
    p_sync_units = os_mem_zalloc2(pkt_cnt * sizeof(T_MEDIA_BUFFER_SYNC_UNIT));
    if (p_sync_units != NULL)
    {
        media_buffer_get_rec_data(handle, p_sync_units, pkt_cnt);
        audio_remote_internal_async_msg_relay(remote_instance,
                                              AUDIO_REMOTE_BUFFER_SYNC_REQ,
                                              p_sync_units,
                                              sizeof(T_MEDIA_BUFFER_SYNC_UNIT)*pkt_cnt,
                                              false);
        AUDIO_PRINT_TRACE3("media_buffer_sync_req: cnt %u avdtp %u--%u", pkt_cnt,
                           p_sync_units->l2cap_seq_num,
                           p_sync_units[pkt_cnt - 1].l2cap_seq_num);
        os_mem_free(p_sync_units);
    }
}

bool audio_remote_find_join_pkt(T_AUDIO_REMOTE_HANDLE handle)
{
    if (handle == NULL)
    {
        return false;
    }

    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
    {
        T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
        T_MEDIA_BUFFER_ENTITY *buffer_ent = remote_instance->buffer_ent;
        uint16_t local_seq;
        T_MEDIA_DATA_HDR *p_packet;
        uint16_t frame_a = 0;
        uint16_t frame_b = 0;
        uint16_t framecounter_last;
        bool found_packet = false;

        media_buffer_get_downstream_info(buffer_ent, &framecounter_last, &local_seq, NULL);

        p_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);

        AUDIO_PRINT_TRACE3("base %u delta %u last %u", remote_instance->base_count,
                           remote_instance->delta_frame, framecounter_last);

        remote_instance->delta_frame -= (uint16_t)(framecounter_last -
                                                   remote_instance->base_count);
        remote_instance->base_count = framecounter_last;

        while (p_packet != NULL)
        {
            frame_a = frame_b;
            frame_b += p_packet->frame_number;
            local_seq++;
            if (remote_instance->delta_frame > frame_a &&
                remote_instance->delta_frame <= frame_b)
            {
                p_packet->local_seq_number = local_seq;
                p_packet->frame_counter = frame_b + remote_instance->base_count;
                AUDIO_PRINT_TRACE5("audio_remote_find_join_pkt: avdtp %u local %u counter %u base %u, delta %u",
                                   p_packet->avdtp_seq_number, p_packet->local_seq_number, p_packet->frame_counter,
                                   remote_instance->base_count, remote_instance->delta_frame);
                found_packet = true;
                break;
            }
            else
            {
                p_packet = p_packet->p_next;
            }
        }

        if (found_packet)
        {
            T_AUDIO_MSG_JOIN_INFO cmd;

            cmd.join_clk = remote_instance->join_clk;
            cmd.packet_local_seq = p_packet->local_seq_number;
            cmd.packet_frame_counter = p_packet->frame_counter;
            cmd.sync_type = remote_instance->sync_type;
            cmd.tid = buffer_ent->media_buffer_fst_sync_tid;
            cmd.asrc_ppm = jitter_buffer_asrc_ppm_get();
            AUDIO_PRINT_INFO1("AUDIO_REMOTE_BUFFER_SEAMLESS_JOIN_INFO: asrc_ppm %d", cmd.asrc_ppm);
            buffer_ent->ignore_latency_report = 2;
            if (remote_instance->sync_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                cmd.packet_id = p_packet->avdtp_seq_number;
            }
            else
            {
                cmd.packet_id = p_packet->bt_clk;
            }

            audio_remote_internal_async_msg_relay(remote_instance,
                                                  AUDIO_REMOTE_BUFFER_SEAMLESS_JOIN_INFO,
                                                  &cmd,
                                                  sizeof(T_AUDIO_MSG_JOIN_INFO),
                                                  false);
            audio_remote_set_state(remote_instance, AUDIO_REMOTE_STATE_UNSYNC);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
        T_MEDIA_BUFFER_ENTITY *buffer_ent = remote_instance->buffer_ent;
        T_MEDIA_DATA_HDR *p_packet = NULL;
        T_BT_CLK_REF clk_ref;
        uint32_t bb_clock_slot;
        uint16_t bb_clock_us;
        uint32_t time_us;
        T_AUDIO_MSG_JOIN_INFO cmd;

        clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);

        if (clk_ref != BT_CLK_NONE
            &&
            bt_clk_compare(remote_instance->join_clk, (bb_clock_slot + 20 * 2 * 1000 / 625) &
                           0x0fffffff)
           )
        {
            p_packet = buffer_ent->ops.peek_fun(buffer_ent, -1, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
            if (remote_instance->sync_type == AUDIO_STREAM_TYPE_VOICE)
            {
                if (p_packet->bt_clk != remote_instance->join_packet_id)
                {
                    if (bt_clk_compare(p_packet->bt_clk, remote_instance->join_packet_id))
                    {
                        audio_remote_seamless_join_retry(remote_instance, MEDIA_BUFFER_SEAMLESS_MISS_PKT);
                    }
                    return false;
                }
                else
                {
                    cmd.packet_id = p_packet->bt_clk;
                }
            }
            else
            {
                if (p_packet->avdtp_seq_number != (uint16_t)remote_instance->join_packet_id)
                {
                    if (cacu_diff_u16(p_packet->avdtp_seq_number, ((uint16_t)remote_instance->join_packet_id)) > 0)
                    {
                        audio_remote_seamless_join_retry(remote_instance, MEDIA_BUFFER_SEAMLESS_MISS_PKT);
                    }
                    return false;
                }
                else
                {
                    cmd.packet_id = p_packet->avdtp_seq_number;
                }
            }
            buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent) - 1,
                                      BUFFER_DIR_DOWNSTREAM);
            time_us = (((remote_instance->join_clk << 4) - ((bb_clock_slot + 20 * 2 * 1000 / 625) << 4)) >> 4)
                      *
                      625 / 2;
            time_us = (time_us > 50000) ? 50000 : time_us;
            cmd.join_clk = remote_instance->join_clk;
            cmd.packet_local_seq = remote_instance->join_local_seq;
            cmd.packet_frame_counter = remote_instance->join_frame_counter;
            cmd.sync_type = remote_instance->sync_type;
            cmd.tid = buffer_ent->media_buffer_fst_sync_tid;
            cmd.asrc_ppm = remote_instance->asrc_ppm;
            audio_remote_internal_sync_msg_relay(remote_instance,
                                                 AUDIO_REMOTE_BUFFER_SEAMLESS_JOIN_PLAY,
                                                 &cmd,
                                                 sizeof(T_AUDIO_MSG_JOIN_INFO),
                                                 REMOTE_TIMER_HIGH_PRECISION,
                                                 time_us / 1000,
                                                 false);
            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
            return true;

        }
        else
        {
            audio_remote_seamless_join_retry(remote_instance, MEDIA_BUFFER_ERR_CLK);
            return false;
        }
    }
}

T_AUDIO_REMOTE_STATE audio_remote_get_state(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    return remote_instance->state;
}

void audio_remote_set_state(T_AUDIO_REMOTE_HANDLE handle, T_AUDIO_REMOTE_STATE state)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    if (handle == NULL)
    {
        return;
    }
    AUDIO_PRINT_TRACE1("audio_remote_set_state: %u", state);
    remote_instance->state = state;
}

void audio_remote_seamless_join_retry(T_AUDIO_REMOTE_HANDLE handle, T_AUDIO_MGR_EXCEPTION exc)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (handle == NULL)
    {
        return;
    }

    remote_instance->seamless_join_retry_cnt++;
    if (remote_instance->seamless_join_retry_cnt >= SEAMLESS_JOIN_MAX_REY_CNT &&
        audio_remote_db->force_join == false)
    {
        audio_mgr_exception(remote_instance->buffer_ent, exc,
                            0, 0);
    }
    else
    {
        audio_remote_reset(remote_instance);
    }
}

void audio_remote_reset(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    T_AUDIO_STREAM_MODE mode;

    if (handle == NULL)
    {
        return;
    }
    AUDIO_PRINT_TRACE0("audio_remote_reset");
    remote_instance->state = AUDIO_REMOTE_STATE_UNSYNC;
    remote_instance->seq_diff = 0;
    remote_instance->frame_diff = 0;
    remote_instance->buffer_sync_trigger = 0;
    remote_instance->seamless_join_retry_cnt = 0;
    remote_instance->slave_audio_sync = 0;
    media_buffer_packet_record_erase(handle);

    media_buffer_get_mode(remote_instance->buffer_ent, &mode);
    if (mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
    {
        remote_instance->sync_peroid_trigger_cnt = MEDIA_SYNC_PEROID_TRIGGER_CNT_RTK;
        remote_instance->sync_packet_cnt = MEDIA_SYNC_PAKCET_CNT_RTK;
    }
    else
    {
        remote_instance->sync_peroid_trigger_cnt = MEDIA_SYNC_PEROID_TRIGGER_CNT;
        remote_instance->sync_packet_cnt = MEDIA_SYNC_PAKCET_CNT;
    }
}

bool audio_remote_align_buffer(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_BUFFER_FIRST_SYNC_CMD *psync_cmd;
    uint16_t malloc_len;
    bool ret = true;
    T_MEDIA_DATA_HDR *p_local_media_head;

    if (handle == NULL)
    {
        return false;
    }
    buffer_ent = remote_instance->buffer_ent;
    //uint16_t media_pkt_cnt = playback_buffer_ds_pkt_cnt(buffer_ent);

    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
    {
        uint16_t media_pkt_cnt = MEDIA_SYNC_TRIGGER_CNT < media_buffer_ds_pkt_cnt(
                                     buffer_ent) ? MEDIA_SYNC_TRIGGER_CNT : media_buffer_ds_pkt_cnt(buffer_ent);
        malloc_len = sizeof(uint32_t) * media_pkt_cnt + sizeof(T_BUFFER_FIRST_SYNC_CMD);

        if (media_pkt_cnt == 0 ||
            media_buffer_ds_frame_cnt(buffer_ent) == 0)
        {
            return false;
        }

        psync_cmd = (T_BUFFER_FIRST_SYNC_CMD *)os_mem_zalloc2(malloc_len);
        if (psync_cmd == NULL)
        {
            return false;
        }

        buffer_ent->media_buffer_fst_sync_tid++;
        psync_cmd->tid = buffer_ent->media_buffer_fst_sync_tid;
        psync_cmd->count = media_pkt_cnt;
        p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);

        if (remote_instance->sync_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            for (uint16_t i = 0; i < media_pkt_cnt; i++)
            {
                psync_cmd->IDs[i] = p_local_media_head->avdtp_seq_number;
                p_local_media_head = p_local_media_head->p_next;
            }
            psync_cmd->buffer_type = AUDIO_STREAM_TYPE_PLAYBACK;

            AUDIO_PRINT_TRACE4("audio_mgr_align_buffer: media_buffer_sync fst_pkt_seq_req pkt cnt %d tid %d ID: %u->%u",
                               media_pkt_cnt, psync_cmd->tid, psync_cmd->IDs[0], psync_cmd->IDs[media_pkt_cnt - 1]);
        }
        else if (remote_instance->sync_type == AUDIO_STREAM_TYPE_VOICE)
        {
            for (uint16_t i = 0; i < media_pkt_cnt; i++)
            {
                psync_cmd->IDs[i] = p_local_media_head->bt_clk;
                p_local_media_head = p_local_media_head->p_next;
            }
            psync_cmd->buffer_type = AUDIO_STREAM_TYPE_VOICE;

            AUDIO_PRINT_TRACE4("audio_mgr_align_buffer: media_buffer_sync fst_pkt_seq_req pkt cnt %d tid %d ID: 0x%x->0x%x",
                               media_pkt_cnt, psync_cmd->tid, psync_cmd->IDs[0], psync_cmd->IDs[media_pkt_cnt - 1]);
        }

        if (audio_remote_internal_async_msg_relay(remote_instance,
                                                  AUDIO_REMOTE_BUFFER_ALIGN_REQ,
                                                  psync_cmd,
                                                  malloc_len,
                                                  false) == false)
        {
            ret = false;
        }
        os_mem_free(psync_cmd);
        if (ret)
        {
            audio_remote_set_state(handle, AUDIO_REMOTE_STATE_ALIGNING);
        }
    }
    else
    {
        T_AUDIO_STREAM_MODE mode;
        audio_remote_set_state(handle, AUDIO_REMOTE_STATE_ALIGNING);

        media_buffer_get_mode(buffer_ent, &mode);

        if (mode != AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY && mode != AUDIO_STREAM_MODE_LOW_LATENCY)
        {
            audio_remote_internal_async_msg_relay(remote_instance,
                                                  AUDIO_REMOTE_BUFFER_SEC_ALIGNING,
                                                  &remote_instance->sync_type,
                                                  1,
                                                  false);
            if (remote_instance->seamless_join_abort_timer_handle != NULL)
            {
                sys_timer_delete(remote_instance->seamless_join_abort_timer_handle);
            }
            remote_instance->seamless_join_abort_timer_handle = sys_timer_create("seamless_join_start_protect",
                                                                                 SYS_TIMER_TYPE_LOW_PRECISION,
                                                                                 (AUDIO_TIMER_SEAMLESS_JOIN_FAILED << 16) | remote_instance->endpoint,
                                                                                 RWS_TIME_TO_ABORT_SEAMLESS_JOIN,
                                                                                 false,
                                                                                 audio_remote_timeout_cback);
            if (remote_instance->seamless_join_abort_timer_handle != NULL)
            {
                sys_timer_start(remote_instance->seamless_join_abort_timer_handle);
            }
        }
        ret = true;
    }
    return ret;

}


void audio_remote_align_buffer_req_handle(T_AUDIO_REMOTE_HANDLE handle, uint8_t *pdata, uint8_t len)
{
    T_BUFFER_FIRST_SYNC_CMD *psync_cmd = (T_BUFFER_FIRST_SYNC_CMD *)pdata;
    uint16_t media_pkt_cnt = psync_cmd->count;
    T_AUDIO_REMOTE_INSTANCE *remote_instance;
    uint32_t *IDs = psync_cmd->IDs;
    T_BUFFER_FIRST_SYNC_RSP rsp;
    T_MEDIA_DATA_HDR *p_local_media_head;
    uint8_t pkt_cnt;
    uint8_t istart = 0;
    uint8_t i = 0;
    T_MEDIA_BUFFER_ENTITY *buffer_ent = NULL;
    uint32_t id = 0;

    rsp.found = false;
    rsp.tid = psync_cmd->tid;
    rsp.buffer_type = psync_cmd->buffer_type;
    rsp.cause = 0;

    remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    buffer_ent = remote_instance->buffer_ent;

    if (buffer_ent == NULL)
    {
        return;
    }

    if (buffer_ent->p_cfg == NULL || buffer_ent->status == MEDIA_BUFFER_UNINIT)
    {
        rsp.cause = FST_SYNC_BUFFET_INACTIVE;
        audio_remote_internal_async_msg_relay(remote_instance,
                                              AUDIO_REMOTE_BUFFER_ALIGN_RSP,
                                              &rsp,
                                              sizeof(T_BUFFER_FIRST_SYNC_RSP),
                                              false);
        return;
    }

    buffer_ent->media_buffer_fst_sync_tid = psync_cmd->tid;

    if (remote_instance->state != AUDIO_REMOTE_STATE_ALIGNING)
    {
        AUDIO_PRINT_WARN1("audio_remote_align_buffer_req_handle: error audio_remote state %u",
                          remote_instance->state);
        uint32_t cause = FST_SYNC_STATE_ERROR;
        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_FST_SYNC_ERR, 0,
                            (cause << 16) | buffer_ent->media_buffer_fst_sync_tid);
        return;
    }

    pkt_cnt = media_buffer_ds_pkt_cnt(buffer_ent);
    AUDIO_PRINT_TRACE2("audio_remote_align_buffer_req_handle: media_buffer_sync rev cnt %d, local cnt %d",
                       media_pkt_cnt, pkt_cnt);
    if (pkt_cnt == 0)
    {
        rsp.found = false;
        rsp.cause = FST_SYNC_DATA_NOT_FOUND;
        audio_remote_internal_async_msg_relay(remote_instance,
                                              AUDIO_REMOTE_BUFFER_ALIGN_RSP,
                                              &rsp,
                                              sizeof(T_BUFFER_FIRST_SYNC_RSP),
                                              false);
        return;
    }

    p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
    if (psync_cmd->buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        id = p_local_media_head->avdtp_seq_number;
        if (cacu_diff_u16(id, IDs[media_pkt_cnt - 1]) > 0)
        {
            rsp.found = false;
            rsp.cause = FST_SYNC_DATA_EXPIRED;
            rsp.match_id = id;
            audio_remote_internal_async_msg_relay(remote_instance,
                                                  AUDIO_REMOTE_BUFFER_ALIGN_RSP,
                                                  &rsp,
                                                  sizeof(T_BUFFER_FIRST_SYNC_RSP),
                                                  false);
            AUDIO_PRINT_TRACE2("audio_remote_align_buffer_req_handle: media_buffer_sync rev lastest id %d < local oldest id %d",
                               IDs[media_pkt_cnt - 1], id);
            return;
        }
    }
    else if (psync_cmd->buffer_type == AUDIO_STREAM_TYPE_VOICE)
    {
        id = p_local_media_head->bt_clk;
        if (bt_clk_compare(id, IDs[media_pkt_cnt - 1]) > 0)
        {
            rsp.found = false;
            rsp.cause = FST_SYNC_DATA_EXPIRED;
            rsp.match_id = id;
            audio_remote_internal_async_msg_relay(remote_instance,
                                                  AUDIO_REMOTE_BUFFER_ALIGN_RSP,
                                                  &rsp,
                                                  sizeof(T_BUFFER_FIRST_SYNC_RSP),
                                                  false);
            AUDIO_PRINT_TRACE2("audio_remote_align_buffer_req_handle: media_buffer_sync rev lastest id 0x%x < local oldest id 0x%x",
                               IDs[media_pkt_cnt - 1], id);
            return;
        }
    }

    p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, -1, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
    if (psync_cmd->buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        id = p_local_media_head->avdtp_seq_number;
        if (cacu_diff_u16(IDs[0], id) > 0)
        {
            rsp.found = false;
            rsp.cause = 0;
            audio_remote_internal_async_msg_relay(remote_instance,
                                                  AUDIO_REMOTE_BUFFER_ALIGN_RSP,
                                                  &rsp,
                                                  sizeof(T_BUFFER_FIRST_SYNC_RSP),
                                                  false);
            AUDIO_PRINT_TRACE2("audio_remote_align_buffer_req_handle: media_buffer_sync rev lastest id %d > local oldest id %d",
                               IDs[0], id);
            buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                      BUFFER_DIR_DOWNSTREAM);

        }
    }
    else if (psync_cmd->buffer_type == AUDIO_STREAM_TYPE_VOICE)
    {
        id = p_local_media_head->bt_clk;
        if (bt_clk_compare(IDs[0], id) > 0)
        {
            rsp.found = false;
            rsp.cause = 0;
            audio_remote_internal_async_msg_relay(remote_instance,
                                                  AUDIO_REMOTE_BUFFER_ALIGN_RSP,
                                                  &rsp,
                                                  sizeof(T_BUFFER_FIRST_SYNC_RSP),
                                                  false);
            AUDIO_PRINT_TRACE2("audio_remote_align_buffer_req_handle: media_buffer_sync rev lastest id 0x%x > local oldest id 0x%x",
                               IDs[0], id);
            buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                      BUFFER_DIR_DOWNSTREAM);
            return;
        }
    }

    p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);

    while (p_local_media_head != NULL && rsp.found == false)
    {
        if (psync_cmd->buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            id = p_local_media_head->avdtp_seq_number;
        }
        else if (psync_cmd->buffer_type == AUDIO_STREAM_TYPE_VOICE)
        {
            id = p_local_media_head->bt_clk;
        }

        for (i = istart; i < media_pkt_cnt; i++)
        {
            if (IDs[i] == id)
            {
                rsp.match_id = id;
                rsp.found = true;
                break;
            }
        }
        p_local_media_head = p_local_media_head->p_next;
    }

    if (rsp.found == true)
    {
        if (psync_cmd->buffer_type == AUDIO_STREAM_TYPE_VOICE)
        {
            buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent) - 1,
                                      BUFFER_DIR_DOWNSTREAM);
            p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
            rsp.match_id = p_local_media_head->bt_clk;
        }
        else
        {
            p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
            while (p_local_media_head != NULL)
            {
                if (p_local_media_head->avdtp_seq_number != rsp.match_id)
                {
                    buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                    p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
                }
                else
                {
                    break;
                }
            }
        }
    }

    audio_remote_internal_async_msg_relay(remote_instance,
                                          AUDIO_REMOTE_BUFFER_ALIGN_RSP,
                                          &rsp,
                                          sizeof(T_BUFFER_FIRST_SYNC_RSP),
                                          false);

    if (rsp.found == true)
    {
        AUDIO_PRINT_TRACE1("audio_remote_align_buffer_req_handle: spk2 has find a match pkt. 0x%x",
                           rsp.match_id);
        audio_remote_set_state(remote_instance, AUDIO_REMOTE_STATE_SYNCED);
        remote_instance->cback(AUDIO_REMOTE_EVENT_ALIGNED, NULL, remote_instance->buffer_ent);
    }
    else
    {
        AUDIO_PRINT_TRACE0("audio_remote_align_buffer_req_handle: spk2 can't find a match pkt");
    }

}

void audio_remote_align_buffer_rsp_handle(T_AUDIO_REMOTE_HANDLE handle, uint8_t *pdata, uint8_t len)
{
    T_BUFFER_FIRST_SYNC_RSP *p_rsp = (T_BUFFER_FIRST_SYNC_RSP *)pdata;
    uint32_t match_id = p_rsp->match_id;
    T_MEDIA_DATA_HDR *p_local_media_head;
    T_MEDIA_BUFFER_ENTITY *buffer_ent = NULL;
    T_AUDIO_REMOTE_INSTANCE *remote_instance;

    remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    buffer_ent = remote_instance->buffer_ent;

    if (buffer_ent == NULL)
    {
        return;
    }

    if (buffer_ent->media_buffer_fst_sync_tid != p_rsp->tid)
    {
        AUDIO_PRINT_TRACE2("audio_remote_align_buffer_rsp_handle: tid not match local %d peer %d",
                           buffer_ent->media_buffer_fst_sync_tid, p_rsp->tid);
        return;
    }

    if ((p_rsp->found) == true)
    {
        p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);

        if (p_local_media_head == NULL)
        {
            AUDIO_PRINT_WARN0("audio_remote_align_buffer_rsp_handle: media_buffer no data in media buffer");
            return;
        }
        if (p_rsp->buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            AUDIO_PRINT_TRACE2("audio_remote_align_buffer_rsp_handle: Sec rsp l2cap_seq %d, pri l2cap_seq = %d"
                               , match_id, p_local_media_head->avdtp_seq_number);
            while (p_local_media_head != NULL)
            {
                if (p_local_media_head->avdtp_seq_number == match_id)
                {
                    break;
                }
                else
                {
                    p_local_media_head = p_local_media_head->p_next;
                    buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                }
            }
        }
        else if (p_rsp->buffer_type == AUDIO_STREAM_TYPE_VOICE)
        {
            AUDIO_PRINT_TRACE2("audio_remote_align_buffer_rsp_handle: Sec rsp 0x%x, pri local 0x%x "
                               , match_id, p_local_media_head->bt_clk);
            while (p_local_media_head != NULL)
            {
                if (p_local_media_head->bt_clk == match_id)
                {
                    break;
                }
                else
                {
                    p_local_media_head = p_local_media_head->p_next;
                    buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                }
            }
        }

        if (p_local_media_head == NULL)
        {
            uint32_t cause = FST_SYNC_DATA_FLUSHED;
            AUDIO_PRINT_WARN0("audio_remote_align_buffer_rsp_handle: media_buffer_sync Spk1 flushed fst seq");
            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_FST_SYNC_ERR, 0, cause << 16);
        }
        else
        {
            audio_remote_set_state(remote_instance, AUDIO_REMOTE_STATE_SYNCED);
            remote_instance->cback(AUDIO_REMOTE_EVENT_ALIGNED, NULL, remote_instance->buffer_ent);
        }

    }
    else
    {
        AUDIO_PRINT_TRACE1("audio_remote_align_buffer_rsp_handle: media_buffer_sync spk2 can't find a match seq cause %u",
                           p_rsp->cause);
        if (remote_instance->state != AUDIO_REMOTE_STATE_UNSYNC)
        {
            uint32_t cause;
            if (p_rsp->cause == FST_SYNC_DATA_EXPIRED)
            {
                cause = FST_SYNC_DATA_EXPIRED;
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_FST_SYNC_ERR, 0, cause << 16);
            }
            else if (p_rsp->cause == FST_SYNC_BUFFET_INACTIVE)
            {
                if (buffer_ent->media_buffer_fst_sync_tid == p_rsp->tid &&
                    media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PREQUEUE)
                {
                    buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent) - MEDIA_SYNC_TRIGGER_CNT,
                                              BUFFER_DIR_DOWNSTREAM);
                    if (media_buffer_ds_pkt_cnt(buffer_ent) >= MEDIA_SYNC_TRIGGER_CNT)
                    {
                        audio_remote_align_buffer(remote_instance);
                    }
                }
            }
            else
            {
                cause = FST_SYNC_DATA_NOT_FOUND;
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_FST_SYNC_ERR, 0, cause << 16);
            }

        }
        else
        {
            buffer_ent->ops.flush_fun(buffer_ent,
                                      media_buffer_ds_pkt_cnt(buffer_ent) - (MEDIA_SYNC_TRIGGER_CNT - 1), BUFFER_DIR_DOWNSTREAM);
        }
    }

}


void audio_mgr_appoint_playtime(T_AUDIO_REMOTE_HANDLE handle, uint32_t set_timing_ms,
                                bool dynamic_latency_on)
{
    T_MEDIA_BUFFER_CFG *p_cfg;
    T_SYNC_PLAY_MSG msg;
    uint16_t timeout_duration;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (handle == NULL)
    {
        return;
    }

    buffer_ent = remote_instance->buffer_ent;
    if (buffer_ent == NULL)
    {
        return;
    }

    p_cfg = buffer_ent->p_cfg;

    if (p_cfg == NULL)
    {
        return;
    }

    if (dynamic_latency_on)
    {
        if (set_timing_ms > 30 + buffer_ent->p_cfg->latency_back_up * NORMAL_MODE_DYNAMIC_LATENCY_RATIO)
        {
            timeout_duration = set_timing_ms - 30 - buffer_ent->p_cfg->latency_back_up *
                               NORMAL_MODE_DYNAMIC_LATENCY_RATIO;
        }
        else
        {
            if (set_timing_ms > 50)
            {
                timeout_duration = set_timing_ms - 30;
            }
            else
            {
                timeout_duration = 20;
            }
        }
    }
    else
    {
        if (set_timing_ms > 50)
        {
            timeout_duration = set_timing_ms - 30;
        }
        else
        {
            timeout_duration = 20;
        }
    }

    msg.time_period = set_timing_ms;
    msg.sync_type = remote_instance->sync_type;
    msg.tid = buffer_ent->media_buffer_fst_sync_tid;

    T_BT_CLK_REF clk_ref;
    uint32_t bb_clock_slot;
    uint16_t bb_clock_us;
    clk_ref = bt_piconet_clk_get(BT_CLK_NONE, &bb_clock_slot, &bb_clock_us);

    if (clk_ref == BT_CLK_NONE)
    {
        bb_clock_slot = 0xffffffff;
    }
    else
    {
        bb_clock_slot += (set_timing_ms * 2 * 1000 / 625); // ms to bt clk
        bb_clock_slot &= 0x0fffffff;
    }
    msg.sync_clk = bb_clock_slot;
    msg.clk_ref = clk_ref;
    AUDIO_PRINT_TRACE3("audio_mgr_appoint_playtime: buffer_sync clk %u time ms %u, buffer_type %d",
                       bb_clock_slot, set_timing_ms, msg.sync_type);
    audio_remote_internal_sync_msg_relay(remote_instance,
                                         AUDIO_REMOTE_BUFFER_SYNC_PLAY,
                                         &msg,
                                         sizeof(T_SYNC_PLAY_MSG),
                                         REMOTE_TIMER_HIGH_PRECISION,
                                         timeout_duration,
                                         false);

}

void audio_remote_relay_cback(uint16_t event, T_REMOTE_RELAY_STATUS status,
                              void *buf, uint16_t len)
{
    T_AUDIO_REMOTE_INTERNAL_MSG *inter_msg;
    T_AUDIO_REMOTE_INSTANCE *remote_instance;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;

    inter_msg = (T_AUDIO_REMOTE_INTERNAL_MSG *)buf;
    remote_instance = audio_remote_handle_find(inter_msg->remote_endpoint);

    if (remote_instance == NULL)
    {
        return;
    }

    if (event != AUDIO_REMOTE_BUFFER_SYNC_REQ &&
        event != AUDIO_REMOTE_BUFFER_MSG_RELAY)
    {
        AUDIO_PRINT_TRACE3("audio_remote_relay_cback: event 0x%04x, status %u ep %d", event, status,
                           remote_instance->endpoint);
    }

    buf = inter_msg->msg;
    len -= sizeof(T_AUDIO_REMOTE_INTERNAL_MSG);
    buffer_ent = remote_instance->buffer_ent;

    switch (event)
    {
    case AUDIO_REMOTE_BUFFER_MSG_RELAY:
        {
            T_AUDIO_REMOTE_BUFFER_MSG_RELAY *msg = (T_AUDIO_REMOTE_BUFFER_MSG_RELAY *)buf;
            remote_instance->relay_cback(msg->msg_id, msg->msg, status, buffer_ent);
        }
        break;

    case AUDIO_REMOTE_BUFFER_SEAMLESS_JOIN_INFO:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                T_BT_CLK_REF clk_ref;
                uint32_t bb_clock_slot;
                uint16_t bb_clock_us;
                T_AUDIO_MSG_JOIN_INFO *p_cmd = (T_AUDIO_MSG_JOIN_INFO *)buf;
                T_MEDIA_DATA_HDR *p_packet;

                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    break;
                }

                sys_timer_delete(remote_instance->seamless_join_abort_timer_handle);
                remote_instance->seamless_join_abort_timer_handle = NULL;
                buffer_ent->media_buffer_fst_sync_tid = p_cmd->tid;

                p_packet = buffer_ent->ops.peek_fun(buffer_ent, -1, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
                if (p_packet != NULL)
                {
                    if (p_cmd->sync_type == AUDIO_STREAM_TYPE_VOICE)
                    {
                        if (bt_clk_compare(p_cmd->packet_id, p_packet->bt_clk))
                        {
                            AUDIO_PRINT_TRACE1("AUDIO_REMOTE_EVT_SEAMLESS_JOIN_INFO: sec has not receives 0x%x",
                                               p_cmd->packet_id);
                            audio_remote_set_state(remote_instance, AUDIO_REMOTE_STATE_SEAMLESS_JOIN);
                            jitter_buffer_asrc_pid_block(buffer_ent->jitter_buffer_handle);
                            remote_instance->join_clk = p_cmd->join_clk;
                            remote_instance->join_packet_id = p_cmd->packet_id;
                            remote_instance->join_local_seq = p_cmd->packet_local_seq;
                            remote_instance->join_frame_counter = p_cmd->packet_frame_counter;
                            break;
                        }
                    }
                    else
                    {
                        if (cacu_diff_u16((uint16_t)p_cmd->packet_id, p_packet->avdtp_seq_number) > 0)
                        {
                            AUDIO_PRINT_TRACE1("AUDIO_REMOTE_EVT_SEAMLESS_JOIN_INFO: sec has not receives 0x%x",
                                               p_cmd->packet_id);
                            audio_remote_set_state(remote_instance, AUDIO_REMOTE_STATE_SEAMLESS_JOIN);
                            jitter_buffer_asrc_pid_block(buffer_ent->jitter_buffer_handle);
                            remote_instance->join_clk = p_cmd->join_clk;
                            remote_instance->join_packet_id = p_cmd->packet_id;
                            remote_instance->join_local_seq = p_cmd->packet_local_seq;
                            remote_instance->join_frame_counter = p_cmd->packet_frame_counter;
                            remote_instance->asrc_ppm = p_cmd->asrc_ppm;
                            break;
                        }
                    }
                }

                p_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);

                while (p_packet != NULL)
                {
                    if (p_cmd->sync_type == AUDIO_STREAM_TYPE_VOICE)
                    {
                        if (p_cmd->packet_id == p_packet->bt_clk)
                        {
                            break;
                        }
                    }
                    else
                    {
                        if (p_cmd->packet_id == p_packet->avdtp_seq_number)
                        {
                            break;
                        }
                    }

                    buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                    p_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
                }

                if (p_packet == NULL)
                {
                    audio_remote_seamless_join_retry(remote_instance, MEDIA_BUFFER_SEAMLESS_MISS_PKT);
                    break;
                }

                clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                AUDIO_PRINT_TRACE4("AUDIO_REMOTE_EVT_SEAMLESS_JOIN_INFO: seamless_join_info pkt_id %u, join_clk 0x%x, asrc_ppm %d, type %u",
                                   p_cmd->packet_id, p_cmd->join_clk, p_cmd->asrc_ppm, p_cmd->sync_type);

                if (clk_ref != BT_CLK_NONE
                    &&
                    bt_clk_compare(p_cmd->join_clk, (bb_clock_slot + 20 * 2 * 1000 / 625) &
                                   0x0fffffff)
                   )
                {
                    uint32_t time_us;

                    time_us = (((p_cmd->join_clk << 4) - ((bb_clock_slot + 20 * 2 * 1000 / 625) << 4)) >> 4) * 625 / 2;
                    time_us = (time_us > 50000) ? 50000 : time_us;
                    audio_remote_internal_sync_msg_relay(remote_instance,
                                                         AUDIO_REMOTE_BUFFER_SEAMLESS_JOIN_PLAY,
                                                         p_cmd,
                                                         sizeof(T_AUDIO_MSG_JOIN_INFO),
                                                         REMOTE_TIMER_HIGH_PRECISION,
                                                         time_us / 1000,
                                                         false);
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                    audio_remote_set_state(remote_instance, AUDIO_REMOTE_STATE_SEAMLESS_JOIN);
                    jitter_buffer_asrc_pid_block(buffer_ent->jitter_buffer_handle);
                }
                else
                {
                    audio_remote_seamless_join_retry(remote_instance, MEDIA_BUFFER_ERR_CLK);
                    break;
                }

            }
        }
        break;

    case AUDIO_REMOTE_BUFFER_SYNC_REQ:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (audio_remote_get_state(remote_instance) == AUDIO_REMOTE_STATE_SYNCED)
                {
                    bool rec_found;
                    rec_found = audio_remote_buffer_sync_req_handle(remote_instance, (uint8_t *)buf, len);
                    remote_instance->cback(AUDIO_REMOTE_EVENT_BUFFER_SYNC, &rec_found, remote_instance->buffer_ent);
                }
            }
        }
        break;

    case AUDIO_REMOTE_BUFFER_NOT_SEAMLESS_JOIN:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                sys_timer_delete(remote_instance->seamless_join_abort_timer_handle);
                remote_instance->seamless_join_abort_timer_handle = NULL;
            }
        }
        break;

    case AUDIO_REMOTE_BUFFER_SEC_ALIGNING:
        {
            T_AUDIO_STREAM_TYPE sync_type = *(T_AUDIO_STREAM_TYPE *)buf;
            T_AUDIO_STREAM_MODE mode;
            T_AUDIO_FORMAT_TYPE type;

            if (buffer_ent->p_cfg == NULL)
            {
                audio_remote_internal_async_msg_relay(remote_instance,
                                                      AUDIO_REMOTE_BUFFER_NOT_SEAMLESS_JOIN,
                                                      &sync_type,
                                                      1,
                                                      false);
                break;
            }

            media_buffer_get_mode(buffer_ent, &mode);
            media_buffer_get_format_type(buffer_ent, &type);

            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
                {
                    if (mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
                        mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                    {
                        audio_mgr_dispatch(AUDIO_MSG_BUFFER_RESET, buffer_ent);
                    }
                    else
                    {
                        if (sync_type == AUDIO_STREAM_TYPE_PLAYBACK)
                        {
                            if (audio_remote_db->seamless_join &&
                                media_buffer_ds_frame_cnt(buffer_ent) > 2)
                            {
                                if (buffer_ent->local_audio_sync)
                                {
                                    buffer_ent->ignore_latency_report = 5;
                                    audio_remote_seamless_join_info_req(buffer_ent);
                                }
                                else
                                {
                                    if (audio_remote_get_state(remote_instance) == AUDIO_REMOTE_STATE_SYNCED)
                                    {
                                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_SEAMLESS_BFE_SYNC, 0, 0);
                                        audio_remote_internal_async_msg_relay(remote_instance,
                                                                              AUDIO_REMOTE_BUFFER_NOT_SEAMLESS_JOIN,
                                                                              &(buffer_ent->p_cfg->stream_type),
                                                                              1,
                                                                              false);
                                    }
                                    else
                                    {
                                        AUDIO_PRINT_TRACE1("ignore seamless join request before local audio sync %d",
                                                           remote_instance->seamless_join_retry_cnt);
                                        remote_instance->seamless_join_retry_cnt++;
                                        if (remote_instance->seamless_join_retry_cnt > SEAMLESS_JOIN_MAX_REY_CNT * 5)
                                        {
                                            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_SEAMLESS_BFE_SYNC, 0, 0);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (audio_remote_db->force_join == false)
                                {
                                    remote_instance->seamless_join_retry_cnt++;
                                    if (remote_instance->seamless_join_retry_cnt > SEAMLESS_JOIN_MAX_REY_CNT)
                                    {
                                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_SEAMLESS_REFUSE, 0, 0);
                                        audio_remote_internal_async_msg_relay(remote_instance,
                                                                              AUDIO_REMOTE_BUFFER_NOT_SEAMLESS_JOIN,
                                                                              &sync_type,
                                                                              1,
                                                                              false);
                                    }
                                }
                            }
                        }
                        else
                        {
                            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_SEAMLESS_REFUSE, 0, 0);
                            audio_remote_internal_async_msg_relay(remote_instance,
                                                                  AUDIO_REMOTE_BUFFER_NOT_SEAMLESS_JOIN,
                                                                  &sync_type,
                                                                  1,
                                                                  false);
                        }
                    }
                }
                else if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_WAIT_TO_PLAY)
                {
                    audio_mgr_dispatch(AUDIO_MSG_BUFFER_RESET, buffer_ent);
                    audio_remote_internal_async_msg_relay(remote_instance,
                                                          AUDIO_REMOTE_BUFFER_NOT_SEAMLESS_JOIN,
                                                          &sync_type,
                                                          1,
                                                          false);
                }
                else
                {
                    audio_remote_internal_async_msg_relay(remote_instance,
                                                          AUDIO_REMOTE_BUFFER_NOT_SEAMLESS_JOIN,
                                                          &sync_type,
                                                          1,
                                                          false);
                }
            }
        }
        break;

    case AUDIO_REMOTE_BUFFER_SEAMLESS_JOIN_PLAY:
        {
            T_AUDIO_MSG_JOIN_INFO *p_cmd = (T_AUDIO_MSG_JOIN_INFO *)buf;

            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT || status == REMOTE_RELAY_STATUS_SYNC_EXPIRED)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (audio_remote_get_state(remote_instance) ==  AUDIO_REMOTE_STATE_SEAMLESS_JOIN)
                    {
                        audio_remote_reset(remote_instance);
                    }
                    if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING &&
                        audio_remote_get_state(remote_instance) ==  AUDIO_REMOTE_STATE_UNSYNC)
                    {
                        audio_remote_set_state(remote_instance, AUDIO_REMOTE_STATE_SYNCED);
                    }
                    else
                    {
                        AUDIO_PRINT_WARN2("seamless play pri state err %u %u", media_buffer_get_status(buffer_ent),
                                          remote_instance->state);
                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_REMOTE_STATE_ERR, 0, 0);
                    }

                }
                else if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    T_BT_CLK_REF clk_ref;
                    uint32_t bb_clock_slot;
                    uint16_t bb_clock_us;
                    T_MEDIA_DATA_HDR *p_packet;

                    if (audio_remote_get_state(remote_instance) != AUDIO_REMOTE_STATE_SEAMLESS_JOIN)
                    {
                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_REMOTE_STATE_ERR, 0, 0);
                        return;
                    }

                    p_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);

                    if (p_cmd->sync_type == AUDIO_STREAM_TYPE_VOICE)
                    {
                        if (p_cmd->packet_id != p_packet->bt_clk)
                        {
                            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_SEAMLESS_MISS_PKT, 0, 0);
                            return;
                        }
                    }
                    else
                    {
                        if (p_cmd->packet_id != p_packet->avdtp_seq_number)
                        {
                            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_SEAMLESS_MISS_PKT, 0, 0);
                            return;
                        }
                    }

                    clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);

                    if (clk_ref != BT_CLK_NONE
                        &&
                        bt_clk_compare(p_cmd->join_clk,
                                       (bb_clock_slot + sync_play_margin_table[buffer_ent->p_cfg->format_info.type] * 2 * 1000 / 625) &
                                       0x0fffffff)
                       )
                    {
                        audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, BT_CLK_SNIFFING, p_cmd->join_clk,
                                                 true);
                        jitter_buffer_asrc_ratio_adjust(p_cmd->join_clk, p_cmd->asrc_ppm);
                        audio_path_synchronization_join_set(buffer_ent->p_cfg->attached_path_handle, 2);
                        media_buffer_set_downstream_info(buffer_ent, p_cmd->packet_frame_counter - p_packet->frame_number,
                                                         p_cmd->packet_local_seq - 1, INVALID_BT_CLOCK);
                        audio_remote_set_state(remote_instance, AUDIO_REMOTE_STATE_SYNCED);

                        remote_instance->seamless_join_retry_cnt = 0;
                        remote_instance->cback(AUDIO_REMOTE_EVENT_SYNC_PLAY_START, NULL, remote_instance->buffer_ent);
                    }
                    else
                    {
                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_MISS_PLAYTIME, 0, 0);
                    }
                }
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED)
            {
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_MISS_PLAYTIME, 0, 0);
            }
        }
        break;

    case AUDIO_REMOTE_BUFFER_ALIGN_REQ:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_remote_align_buffer_req_handle(remote_instance, (uint8_t *)buf, len);
            }
        }
        break;

    case AUDIO_REMOTE_BUFFER_ALIGN_RSP:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_remote_align_buffer_rsp_handle(remote_instance, (uint8_t *)buf, len);
            }
        }
        break;

    case AUDIO_REMOTE_BUFFER_SYNC_PLAY:
        {
            T_SYNC_PLAY_MSG *msg = (T_SYNC_PLAY_MSG *)buf;

            if (buffer_ent == NULL || buffer_ent->p_cfg == NULL)
            {
                return;
            }


            if (status == REMOTE_RELAY_STATUS_SYNC_RCVD)
            {
                if (remote_instance->state != AUDIO_REMOTE_STATE_SYNCED)
                {
                    audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PLAY_WO_SYNC, 0, 0);
                }
                else
                {
                    remote_instance->cback(AUDIO_REMOTE_EVENT_SYNC_PLAY_RSV, &msg->tid, remote_instance->buffer_ent);
                }
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_TOUT || status == REMOTE_RELAY_STATUS_SYNC_EXPIRED)
            {
                T_BT_CLK_REF clk_ref;
                uint32_t bb_clock_slot;
                uint16_t bb_clock_us;
                uint32_t bb_clock_rcv;

                if (status == REMOTE_RELAY_STATUS_SYNC_EXPIRED)
                {
                    remote_instance->cback(AUDIO_REMOTE_EVENT_SYNC_PLAY_RSV, &msg->tid, remote_instance->buffer_ent);
                }

                bb_clock_rcv = msg->sync_clk;
                clk_ref = bt_piconet_clk_get((T_BT_CLK_REF)(msg->clk_ref), &bb_clock_slot, &bb_clock_us);

                if (clk_ref != BT_CLK_NONE
                    &&
                    bt_clk_compare(bb_clock_rcv, (bb_clock_slot +
                                                  sync_play_margin_table[buffer_ent->p_cfg->format_info.type] * 2 * 1000 / 625) &
                                   0x0fffffff)
                   )
                {
                    REMOTE_PRINT_TRACE1("audio_remote_relay_cback: media_buffer_sync play tout state %d",
                                        media_buffer_get_status(buffer_ent));
                    if (audio_path_is_running(buffer_ent->p_cfg->attached_path_handle))
                    {
                        if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_WAIT_TO_PLAY)
                        {
                            if (msg->tid == buffer_ent->media_buffer_fst_sync_tid)
                            {
                                media_buffer_clear_downstream_info(buffer_ent);
                                audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, clk_ref, bb_clock_rcv, true);
                                remote_instance->cback(AUDIO_REMOTE_EVENT_SYNC_PLAY_START, NULL, remote_instance->buffer_ent);
                            }
                            else
                            {
                                AUDIO_PRINT_WARN2("tid is error when wait to play %u %u", msg->tid,
                                                  buffer_ent->media_buffer_fst_sync_tid);
                                //audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PLAY_ERR_TID, 0, 0);
                            }
                        }
                    }
                    else
                    {
                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PATH_NOT_READY, 0, 0);
                    }
                }
                else
                {
                    AUDIO_PRINT_WARN1("audio_remote_relay_cback: rcv clk %u < local clk", bb_clock_rcv);
                    audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PLAY_OVERTIME, 0, 0);
                }
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED)
            {
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PLAY_OVERTIME, 0, 0);
            }
        }
        break;

    case AUDIO_REMOTE_BUFFER_LOW_LATENCY_SYNC_PLAY:
        {
            T_AUDIO_MSG_LOW_LATENCY_SYNC_PLAY *p_cmd = (T_AUDIO_MSG_LOW_LATENCY_SYNC_PLAY *)buf;
            T_AUDIO_REMOTE_LATENCY_INFO msg;

            if (buffer_ent == NULL || buffer_ent->p_cfg == NULL)
            {
                return;
            }

            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                AUDIO_PRINT_TRACE4("AUDIO_REMOTE_LOW_LATENCY_SYNC_PLAY: tid %u avdtp_seq %u sync_clk 0x%0x dynamic_latency_on %u",
                                   p_cmd->tid, p_cmd->avdtp_seq, p_cmd->sync_clk, p_cmd->dynamic_latency_on);
                T_MEDIA_DATA_HDR *p_local_media_head;

                if (audio_path_is_running(buffer_ent->p_cfg->attached_path_handle) == false)
                {
                    audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PATH_NOT_READY, 0, 0);
                    break;
                }

                if (p_cmd->dynamic_latency_on == false)
                {
                    msg.dynamic_latency_on = false;
                    msg.plc_count = 0;
                    msg.latency = p_cmd->latency;
                    msg.latency_override = p_cmd->latency_override;
                    msg.latency_back_up = p_cmd->latency_backup;
                }
                else
                {
                    msg.dynamic_latency_on = true;
                    msg.plc_count = p_cmd->plc_count;
                    msg.latency = p_cmd->latency;
                    msg.latency_override = p_cmd->latency_override;
                    msg.latency_back_up = p_cmd->latency_backup;
                }

                p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);

                while (p_local_media_head != NULL)
                {
                    if (p_local_media_head->avdtp_seq_number == p_cmd->avdtp_seq)
                    {
                        break;
                    }
                    p_local_media_head = p_local_media_head->p_next;
                }
                if (p_local_media_head == NULL)
                {
                    p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
                    if ((uint16_t)(p_cmd->avdtp_seq + 1) == p_local_media_head->avdtp_seq_number)
                    {
                        media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                        buffer_ent->media_buffer_fst_sync_tid = p_cmd->tid;
                        audio_remote_set_state(buffer_ent->audio_remote_handle, AUDIO_REMOTE_STATE_SYNCED);
                    }
                    else
                    {
                        p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, -1, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
                        if (p_cmd->avdtp_seq == (uint16_t)(p_local_media_head->avdtp_seq_number + 1))
                        {
                            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                            buffer_ent->media_buffer_fst_sync_tid = p_cmd->tid;
                            audio_remote_set_state(buffer_ent->audio_remote_handle, AUDIO_REMOTE_STATE_SYNCED);
                            buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                                      BUFFER_DIR_DOWNSTREAM);
                        }
                        else
                        {
                            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_MISS_SPEC_PKT, 0, 0);
                            break;
                        }

                    }
                }
                else
                {
                    p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
                    while (p_local_media_head != NULL)
                    {
                        if (p_local_media_head->avdtp_seq_number == p_cmd->avdtp_seq)
                        {
                            break;
                        }
                        buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                        p_local_media_head = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
                    }
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                    buffer_ent->media_buffer_fst_sync_tid = p_cmd->tid;
                    audio_remote_set_state(buffer_ent->audio_remote_handle, AUDIO_REMOTE_STATE_SYNCED);
                }

                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_WAIT_TO_PLAY)
                {
                    T_BT_CLK_REF clk_ref;
                    uint32_t bb_clock_slot;
                    uint16_t bb_clock_us;
                    uint32_t bb_clock_rcv;
                    uint8_t  protect_ms;

                    bb_clock_rcv = p_cmd->sync_clk;
                    clk_ref = bt_piconet_clk_get((T_BT_CLK_REF)(p_cmd->clk_ref), &bb_clock_slot, &bb_clock_us);

                    if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_SBC)
                    {
                        protect_ms = 3;
                    }
                    else
                    {
                        protect_ms = 10;
                    }

                    if (clk_ref != BT_CLK_NONE
                        &&
                        bt_clk_compare(bb_clock_rcv, (bb_clock_slot + protect_ms * 2 * 1000 / 625) &
                                       0x0fffffff)
                       )
                    {
                        REMOTE_PRINT_TRACE1("audio_remote_relay_cback: gaming_mode media_buffer_sync play tout state %d",
                                            media_buffer_get_status(buffer_ent));

                        audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, clk_ref, bb_clock_rcv, true);
                        media_buffer_clear_downstream_info(buffer_ent);
                        remote_instance->seq_diff = 0;
                        remote_instance->frame_diff = 0;
                        remote_instance->cback(AUDIO_REMOTE_EVENT_SYNC_PLAY_START, &msg, remote_instance->buffer_ent);
                    }
                    else
                    {
                        AUDIO_PRINT_WARN1("audio_remote_relay_cback: rcv clk %u < local clk", bb_clock_rcv);
                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PLAY_OVERTIME, 0, 0);
                    }

                }

            }
        }
        break;

    case AUDIO_REMOTE_BUFFER_ULTRA_LOW_LATENCY_SYNC_PLAY:
        {
            T_AUDIO_MSG_LOW_LATENCY_SYNC_PLAY *p_cmd = (T_AUDIO_MSG_LOW_LATENCY_SYNC_PLAY *)buf;
            T_AUDIO_REMOTE_LATENCY_INFO msg;

            if (status == REMOTE_RELAY_STATUS_SYNC_RCVD)
            {
                buffer_ent->media_buffer_fst_sync_tid = p_cmd->tid;

                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PREQUEUE)
                {
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                }
                else
                {
                    audio_mgr_exception(buffer_ent, MEDIA_BUFFER_STATE_ERR, 0, 0);
                }
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_TOUT || status == REMOTE_RELAY_STATUS_SYNC_EXPIRED)
            {
                if (audio_path_is_running(buffer_ent->p_cfg->attached_path_handle) == false)
                {
                    audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PATH_NOT_READY, 0, 0);
                    break;
                }

                if (p_cmd->dynamic_latency_on == false)
                {
                    msg.dynamic_latency_on = false;
                    msg.plc_count = 0;
                    msg.latency = p_cmd->latency;
                    msg.latency_override = p_cmd->latency_override;
                    msg.latency_back_up = p_cmd->latency_backup;
                }
                else
                {
                    msg.dynamic_latency_on = true;
                    msg.plc_count = p_cmd->plc_count;
                    msg.latency = p_cmd->latency;
                    msg.latency_override = p_cmd->latency_override;
                    msg.latency_back_up = p_cmd->latency_backup;
                }

                buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent) - 1,
                                          BUFFER_DIR_DOWNSTREAM);

                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_WAIT_TO_PLAY)
                {
                    T_BT_CLK_REF clk_ref;
                    uint32_t bb_clock_slot;
                    uint16_t bb_clock_us;
                    uint32_t bb_clock_rcv;
                    uint8_t protect_ms;

                    bb_clock_rcv = p_cmd->sync_clk;
                    clk_ref = bt_piconet_clk_get((T_BT_CLK_REF)(p_cmd->clk_ref), &bb_clock_slot, &bb_clock_us);

                    if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_SBC)
                    {
                        protect_ms = 3;
                    }
                    else
                    {
                        protect_ms = 10;
                    }

                    if (clk_ref != BT_CLK_NONE
                        &&
                        bt_clk_compare(bb_clock_rcv, (bb_clock_slot + protect_ms * 2 * 1000 / 625) &
                                       0x0fffffff)
                       )
                    {
                        audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, p_cmd->clk_ref, bb_clock_rcv,
                                                 true);
                        media_buffer_clear_downstream_info(buffer_ent);
                        remote_instance->seq_diff = 0;
                        remote_instance->frame_diff = 0;
                        audio_remote_set_state(buffer_ent->audio_remote_handle, AUDIO_REMOTE_STATE_SYNCED);
                        remote_instance->cback(AUDIO_REMOTE_EVENT_SYNC_PLAY_START, &msg, remote_instance->buffer_ent);
                    }
                    else
                    {
                        AUDIO_PRINT_WARN1("audio_remote_relay_cback: rcv clk %u < local clk", bb_clock_rcv);
                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PLAY_OVERTIME, 0, 0);
                    }
                }
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED)
            {
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_MISS_PLAYTIME, 0, 0);
            }
        }
        break;

    default:
        break;
    }

}

void audio_remote_ultra_low_latency_sync_play(T_AUDIO_REMOTE_HANDLE handle, bool dynamic_latency_on)
{
    T_BT_CLK_REF clk_ref;
    uint32_t bb_clock_slot;
    uint32_t bb_clock_slot_sync;
    uint16_t bb_clock_us;
    T_AUDIO_MSG_LOW_LATENCY_SYNC_PLAY cmd;
    int latency_offset_ms;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_REMOTE_INSTANCE *remote_instance;

    if (handle == NULL)
    {
        return;
    }

    remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    buffer_ent = remote_instance->buffer_ent;

    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }
    latency_offset_ms = buffer_ent->p_cfg->latency_back_up + media_buffer_cacu_frames_duration(
                            buffer_ent->p_cfg->plc_count,
                            &buffer_ent->p_cfg->format_info);

    clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);

    if (clk_ref == BT_CLK_NONE)
    {
        buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                  BUFFER_DIR_DOWNSTREAM);
        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_ERR_SNIFF_CLK, 0, 0);
        return;
    }
    bb_clock_slot_sync = (bb_clock_slot + (latency_offset_ms + 100) * 1000 * 2 / 625) & 0x0fffffff;

    AUDIO_PRINT_TRACE4("audio_remote_ultra_low_latency_sync_play: sync_clk %u local_clk %u latency offset ms %d dynamic lat %u",
                       bb_clock_slot_sync,
                       bb_clock_slot, latency_offset_ms, dynamic_latency_on);
    cmd.sync_clk = bb_clock_slot_sync;

    buffer_ent->media_buffer_fst_sync_tid++;
    cmd.tid = buffer_ent->media_buffer_fst_sync_tid;
    cmd.clk_ref = BT_CLK_SNIFFING;
    cmd.dynamic_latency_on = dynamic_latency_on;
    cmd.plc_count = buffer_ent->p_cfg->plc_count;
    cmd.latency = buffer_ent->p_cfg->latency;
    cmd.latency_override = buffer_ent->p_cfg->latency_override;
    cmd.latency_backup  = buffer_ent->p_cfg->latency_back_up;

    audio_remote_internal_sync_msg_relay(remote_instance,
                                         AUDIO_REMOTE_BUFFER_ULTRA_LOW_LATENCY_SYNC_PLAY,
                                         &cmd,
                                         sizeof(T_AUDIO_MSG_LOW_LATENCY_SYNC_PLAY),
                                         REMOTE_TIMER_HIGH_PRECISION,
                                         100,
                                         false);
}

void audio_remote_low_latency_sync_play(T_AUDIO_REMOTE_HANDLE handle, bool dynamic_latency_on)
{
    T_BT_CLK_REF clk_ref;
    uint32_t bb_clock_slot;
    uint32_t bb_clock_slot_sync;
    uint16_t bb_clock_us;
    T_AUDIO_MSG_LOW_LATENCY_SYNC_PLAY cmd;
    int latency_offset_ms;
    T_MEDIA_DATA_HDR *p_media_packet;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_REMOTE_INSTANCE *remote_instance;

    if (handle == NULL)
    {
        return;
    }

    remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    buffer_ent = remote_instance->buffer_ent;

    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }
    latency_offset_ms = buffer_ent->p_cfg->latency;
    clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);

    if (clk_ref == BT_CLK_NONE)
    {
        buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                  BUFFER_DIR_DOWNSTREAM);
        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_ERR_SNIFF_CLK, 0, 0);
        return;
    }

    bb_clock_slot_sync = (buffer_ent->last_ts + latency_offset_ms * 1000 * 2 / 625) & 0x0fffffff;

    AUDIO_PRINT_TRACE5("audio_remote_low_latency_sync_play: sync_clk %u local_clk %u latency offset ms %d dynamic lat %u latency_override %u",
                       bb_clock_slot_sync,
                       bb_clock_slot, latency_offset_ms, dynamic_latency_on,
                       buffer_ent->p_cfg->latency_override);
    cmd.sync_clk = bb_clock_slot_sync;

    buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent) - 1,
                              BUFFER_DIR_DOWNSTREAM);

    p_media_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
    if (p_media_packet == NULL)
    {
        AUDIO_PRINT_WARN0("audio_remote_low_latency_sync_play: no more data");
        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_STATE_ERR, 0, 0);
        return;
    }
    /*
        if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_AAC)
        {
            cmd.avdtp_seq = p_media_packet->avdtp_seq_number + 1;
            buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
        }
        else
    */
    {
        cmd.avdtp_seq = p_media_packet->avdtp_seq_number;
    }

    buffer_ent->media_buffer_fst_sync_tid++;
    cmd.tid = buffer_ent->media_buffer_fst_sync_tid;
    cmd.clk_ref = BT_CLK_SNIFFING;
    cmd.dynamic_latency_on = dynamic_latency_on;
    cmd.latency_override = buffer_ent->p_cfg->latency_override;
    cmd.latency_backup  = buffer_ent->p_cfg->latency_back_up;
    cmd.plc_count = buffer_ent->p_cfg->plc_count;
    cmd.latency = buffer_ent->p_cfg->latency;

    if (audio_remote_internal_async_msg_relay(remote_instance,
                                              AUDIO_REMOTE_BUFFER_LOW_LATENCY_SYNC_PLAY,
                                              &cmd,
                                              sizeof(T_AUDIO_MSG_LOW_LATENCY_SYNC_PLAY),
                                              false))
    {
        audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, cmd.clk_ref, bb_clock_slot_sync,
                                 true);
        media_buffer_clear_downstream_info(buffer_ent);
        remote_instance->seq_diff = 0;
        remote_instance->frame_diff = 0;
        audio_remote_set_state(buffer_ent->audio_remote_handle, AUDIO_REMOTE_STATE_SYNCED);
        remote_instance->cback(AUDIO_REMOTE_EVENT_SYNC_PLAY_START, NULL, remote_instance->buffer_ent);
    }
}

bool audio_remote_is_buffer_unsync(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (handle == NULL)
    {
        return false;
    }

    if (remote_instance->seq_diff != 0 || remote_instance->frame_diff != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool audio_remote_buffer_PLC_policy_1(T_AUDIO_REMOTE_INSTANCE *remote_instance)
{
    T_MEDIA_DATA_HDR *p_media_packet;
    T_H2D_STREAM_HEADER2 *p_stream_head;
    uint16_t download_frames = 0;
    uint16_t download_seq = 0;
    uint16_t download_frames_counter = 0;
    uint16_t download_frames_length = 0;
    bool ret = true;
    T_MEDIA_BUFFER_ENTITY *buffer_ent = remote_instance->buffer_ent;

    {
        //seq >0
        if (buffer_ent->last_packet_ptr == NULL)
        {
            p_media_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
            if (p_media_packet == NULL)
            {
                AUDIO_PRINT_ERROR0("media_buffer_plc: policy 1, no more packets");
                return false;
            }
        }
        else
        {
            p_media_packet = buffer_ent->last_packet_ptr;
        }

        if (p_media_packet->pkt_status != AUDIO_STREAM_STATUS_CORRECT)
        {
            return false;
        }

        p_stream_head = (T_H2D_STREAM_HEADER2 *)(p_media_packet->p_data);

        download_frames =    remote_instance->frame_diff / remote_instance->seq_diff;
        download_seq = buffer_ent->ds_local_seq_last + 1;
        download_frames_counter = buffer_ent->ds_frame_counter_last + download_frames;
        download_frames_length = buffer_ent->frame_len * download_frames;

        if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_SBC)
        {
            T_MEDIA_DATA_HDR *p_make_up ;
            T_H2D_STREAM_HEADER2 *p_stream_head_fab;

            uint16_t size = sizeof(T_MEDIA_DATA_HDR) + sizeof(T_H2D_STREAM_HEADER2) + download_frames_length;
            uint16_t media_data_len = p_stream_head->payload_length;
            uint8_t *media_data_ptr = p_stream_head->payload;

            p_make_up = media_buffer_get(buffer_ent->pool_handle, size);
            if (p_make_up == NULL)
            {
                return false;
            }
            p_make_up->payload_length = download_frames_length + sizeof(T_H2D_STREAM_HEADER2);
            p_make_up->frame_counter = download_frames_counter;
            p_make_up->frame_number = download_frames ;
            p_make_up->local_seq_number = download_seq;
            p_make_up->avdtp_seq_number = p_media_packet->avdtp_seq_number;

            p_stream_head_fab = (T_H2D_STREAM_HEADER2 *)(p_make_up->p_data);

            p_stream_head_fab->timestamp = p_media_packet->bt_clk;
            p_stream_head_fab->status = AUDIO_STREAM_STATUS_LOST;
            p_stream_head_fab->local_seq = download_seq;
            p_stream_head_fab->frame_number = download_frames;
            p_stream_head_fab->frame_counter = download_frames_counter;
            p_stream_head_fab->payload_length = download_frames_length;

            if (download_frames > p_media_packet->frame_number)
            {
                //Make up more frames
                uint8_t make_up_frame_num = download_frames - p_media_packet->frame_number;
                uint8_t *p_more_payload;
                uint8_t temp_frame_num = 0;
                uint16_t temp_frame_len = 0;

                memcpy(p_stream_head_fab->payload, media_data_ptr, media_data_len);
                p_more_payload = &p_stream_head_fab->payload[media_data_len];
                while (make_up_frame_num)
                {
                    temp_frame_num = (make_up_frame_num < p_media_packet->frame_number) ? make_up_frame_num :
                                     p_media_packet->frame_number; // min(p_media_packet->frame_number,make_up_frame_num)
                    temp_frame_len = buffer_ent->frame_len * temp_frame_num;
                    memcpy((void *)p_more_payload, media_data_ptr, temp_frame_len); // get it from first frame

                    p_more_payload += temp_frame_len;
                    make_up_frame_num -= temp_frame_num;
                }

                if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle, AUDIO_PATH_DATA_TYPE_LOST,
                                         download_frames_counter,
                                         p_make_up->p_data, p_make_up->payload_length, true))
                {
                    AUDIO_PRINT_WARN5("media_buffer_plc: download_frames %d, h2d_seq %u, packet_len %d, frames_counter %d | %d",
                                      download_frames, p_stream_head_fab->local_seq, p_make_up->payload_length,
                                      buffer_ent->ds_frame_counter_last, download_frames_counter);
                    buffer_ent->ds_local_seq_last++;
                    buffer_ent->ds_frame_counter_last = download_frames_counter;
                    media_buffer_packet_record_add(buffer_ent->audio_remote_handle, p_make_up);
                    remote_instance->frame_diff -= download_frames;
                    remote_instance->seq_diff -= 1;

                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                memcpy(p_stream_head_fab->payload, media_data_ptr, download_frames_length);

                if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle, AUDIO_PATH_DATA_TYPE_LOST,
                                         download_frames_counter,
                                         p_make_up->p_data, p_make_up->payload_length, true))
                {
                    AUDIO_PRINT_WARN5("media_buffer_plc: download_length %d, h2d_seq %d, packet_len %d, frames_counter %d | %d",
                                      p_make_up->payload_length, p_stream_head_fab->local_seq, p_media_packet->payload_length,
                                      buffer_ent->ds_frame_counter_last, download_frames_counter);
                    buffer_ent->ds_local_seq_last++;
                    buffer_ent->ds_frame_counter_last = download_frames_counter;
                    media_buffer_packet_record_add(buffer_ent->audio_remote_handle, p_make_up);
                    remote_instance->frame_diff -= download_frames;
                    remote_instance->seq_diff -= 1;
                }
                else
                {
                    ret = false;
                }
            }
            media_buffer_put(buffer_ent->pool_handle, p_make_up);
        }
        else if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_AAC)
        {
            T_MEDIA_DATA_HDR packet_head;
            T_H2D_STREAM_HEADER2 *p_stream_head_fab;
            T_H2D_STREAM_HEADER2 *p_stream_head = (T_H2D_STREAM_HEADER2 *)p_media_packet->p_data;
            uint8_t             *p_data;
            uint16_t media_data_len;
            uint8_t *media_data_ptr;

            media_data_len = p_stream_head->payload_length;
            media_data_ptr = p_stream_head->payload;

            packet_head.avdtp_seq_number = p_media_packet->avdtp_seq_number;
            packet_head.local_seq_number = download_seq;
            packet_head.frame_counter = download_frames_counter;
            packet_head.frame_number = 1;
            p_data = media_buffer_get(buffer_ent->pool_handle,
                                      p_stream_head->payload_length + sizeof(T_H2D_STREAM_HEADER2));

            if (p_data != NULL)
            {
                p_stream_head_fab = (T_H2D_STREAM_HEADER2 *)p_data;

                p_stream_head_fab->timestamp = p_media_packet->bt_clk;
                p_stream_head_fab->status = AUDIO_STREAM_STATUS_LOST;
                p_stream_head_fab->local_seq = download_seq;
                p_stream_head_fab->frame_number = 1;
                p_stream_head_fab->frame_counter = download_frames_counter;
                p_stream_head_fab->payload_length = media_data_len;

                memcpy(p_stream_head_fab->payload, media_data_ptr, media_data_len);

                if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle, AUDIO_PATH_DATA_TYPE_LOST,
                                         download_frames_counter,
                                         p_data, p_media_packet->payload_length, true))
                {
                    AUDIO_PRINT_WARN3("media_buffer_plc: AAC, h2d_seq %u, frames_counter %d | %d",
                                      p_stream_head_fab->local_seq, p_media_packet->frame_counter, download_frames_counter);
                    buffer_ent->ds_local_seq_last++;
                    buffer_ent->ds_frame_counter_last = download_frames_counter;
                    media_buffer_packet_record_add(buffer_ent->audio_remote_handle, &packet_head);
                    remote_instance->frame_diff -= download_frames;
                    remote_instance->seq_diff -= 1;
                }
                else
                {
                    ret = false;
                }
                media_buffer_put(buffer_ent->pool_handle, p_data);
            }
        }
        else
        {
            AUDIO_PRINT_ERROR0("audio_remote_buffer_PLC_policy_1: codec err");
            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PACKET_LOSS, 0, remote_instance->frame_diff);
            return false;
        }
    }

    AUDIO_PRINT_TRACE2("media_buffer_plc: remain seq %d frames %d",
                       remote_instance->seq_diff,
                       remote_instance->frame_diff);
    return ret;
}

static bool audio_remote_buffer_PLC_policy_2(T_AUDIO_REMOTE_INSTANCE *remote_instance)
{
    uint16_t size;
    uint16_t download_frames_length;
    T_MEDIA_BUFFER_ENTITY *buffer_ent = remote_instance->buffer_ent;
    T_MEDIA_DATA_HDR *p_media_packet;
    T_MEDIA_DATA_HDR *p_make_up ;
    T_H2D_STREAM_HEADER2 *p_head;
    bool ret = true;

    AUDIO_PRINT_TRACE1("audio_remote_buffer_PLC_policy_2: seq diff 0 frame diff %u",
                       remote_instance->frame_diff);

    p_media_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
    if (p_media_packet == NULL)
    {
        AUDIO_PRINT_ERROR0("audio_remote_buffer_PLC_policy_2: no more packets");
        return false;
    }

    download_frames_length = buffer_ent->frame_len * (p_media_packet->frame_number + 1);
    size = sizeof(T_MEDIA_DATA_HDR) + sizeof(T_H2D_STREAM_HEADER2) + download_frames_length;
    p_make_up = media_buffer_get(buffer_ent->pool_handle, size);
    if (p_make_up == NULL)
    {
        return false;
    }

    memcpy(p_make_up,
           p_media_packet,
           sizeof(T_MEDIA_DATA_HDR) + p_media_packet->payload_length); //remain 1 frame

    memcpy(p_make_up->p_data + p_media_packet->payload_length,
           p_media_packet->p_data + p_media_packet->payload_length - buffer_ent->frame_len,
           buffer_ent->frame_len);//copy 1 frame

    p_head = (T_H2D_STREAM_HEADER2 *)p_make_up->p_data;

    p_make_up->payload_length = sizeof(T_H2D_STREAM_HEADER2) + download_frames_length;
    p_make_up->frame_number = p_make_up->frame_number + 1;
    p_make_up->frame_counter = p_make_up->frame_number + buffer_ent->ds_frame_counter_last;
    p_make_up->local_seq_number = buffer_ent->ds_local_seq_last + 1;

    p_head->timestamp = p_media_packet->bt_clk;
    p_head->status = AUDIO_STREAM_STATUS_CORRECT;
    p_head->local_seq = buffer_ent->ds_local_seq_last + 1;
    p_head->frame_number = p_make_up->frame_number;
    p_head->frame_counter = p_make_up->frame_counter;
    p_head->payload_length = download_frames_length;

    if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle, AUDIO_PATH_DATA_TYPE_AUDIO,
                             p_make_up->frame_counter,
                             p_make_up->p_data, p_make_up->payload_length, true))
    {
        AUDIO_PRINT_WARN5("audio_remote_buffer_PLC_policy_2: download_frames %d, h2d_seq %u, packet_len %d, frames_counter %d | %d",
                          p_make_up->frame_number, p_make_up->local_seq_number, p_make_up->payload_length,
                          buffer_ent->ds_frame_counter_last, p_make_up->frame_counter);
        buffer_ent->ds_local_seq_last++;
        buffer_ent->ds_frame_counter_last = p_make_up->frame_counter;
        remote_instance->frame_diff -= 1;
        media_buffer_packet_record_add(buffer_ent->audio_remote_handle, p_make_up);
        buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
    }
    else
    {
        ret = false;
    }
    media_buffer_put(buffer_ent->pool_handle, p_make_up);

    return ret;

}

static bool audio_remote_buffer_PLC_policy_3(T_AUDIO_REMOTE_INSTANCE *remote_instance)
{
    uint16_t download_frames;
    uint16_t download_counter;
    uint16_t download_length;
    T_MEDIA_BUFFER_ENTITY *buffer_ent = remote_instance->buffer_ent;
    T_MEDIA_DATA_HDR *p_media_packet;
    T_H2D_STREAM_HEADER2 *p_head;
    bool ret = true;

    AUDIO_PRINT_TRACE1("audio_remote_buffer_PLC_policy_3: seq diff 0 frame diff %u",
                       remote_instance->frame_diff);
    p_media_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
    if (p_media_packet == NULL)
    {
        AUDIO_PRINT_ERROR0("audio_remote_buffer_PLC_policy_3: no more packets");
        return false;
    }

    while (p_media_packet->frame_number + remote_instance->frame_diff <= 0)
    {
        remote_instance->frame_diff += p_media_packet->frame_number;
        buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
        p_media_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
        if (p_media_packet == NULL)
        {
            AUDIO_PRINT_TRACE1("audio_remote_buffer_PLC_policy_3: no more data remain diff frame %d",
                               remote_instance->frame_diff);
            return false;
        }
    }

    if (remote_instance->frame_diff == 0)
    {
        AUDIO_PRINT_TRACE2("audio_remote_buffer_PLC_policy_3: remain seq %d frames %d",
                           remote_instance->seq_diff,
                           remote_instance->frame_diff);
        return media_buffer_downstream_dsp(buffer_ent, true);
    }
    else
    {
        download_frames = p_media_packet->frame_number + remote_instance->frame_diff;
        download_counter = download_frames + buffer_ent->ds_frame_counter_last;
        download_length = sizeof(T_H2D_STREAM_HEADER2) + download_frames * buffer_ent->frame_len;

        p_head = (T_H2D_STREAM_HEADER2 *)p_media_packet->p_data;
        p_media_packet->local_seq_number = buffer_ent->ds_local_seq_last + 1;

        p_head->timestamp = p_media_packet->bt_clk;
        p_head->status = AUDIO_STREAM_STATUS_CORRECT;
        p_head->local_seq = buffer_ent->ds_local_seq_last + 1;
        p_head->frame_number = download_frames;
        p_head->frame_counter = download_counter;
        p_head->payload_length = download_frames * buffer_ent->frame_len;

        if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle, AUDIO_PATH_DATA_TYPE_AUDIO,
                                 download_counter,
                                 p_head, download_length, true))
        {
            p_media_packet->frame_number = download_frames;
            p_media_packet->frame_counter = download_counter;
            AUDIO_PRINT_WARN5("audio_remote_buffer_PLC_policy_3: download_frames %d, h2d_seq %u, packet_len %d, frames_counter %d | %d",
                              p_media_packet->frame_number, p_media_packet->local_seq_number, download_length,
                              buffer_ent->ds_frame_counter_last, p_media_packet->frame_counter);
            buffer_ent->ds_local_seq_last++;
            buffer_ent->ds_frame_counter_last = download_counter;
            remote_instance->frame_diff = 0;
            media_buffer_packet_record_add(buffer_ent->audio_remote_handle, p_media_packet);
            buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
        }
        else
        {
            ret = false;
        }
        return ret;
    }
}

bool audio_remote_buffer_PLC(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_MEDIA_DATA_HDR *p_media_packet;
    bool ret = false;

    if (audio_remote_handle_check(remote_instance) == false)
    {
        return false;
    }

    buffer_ent = remote_instance->buffer_ent;

    if (buffer_ent->p_cfg == NULL)
    {
        return false;
    }

    if (remote_instance->seq_diff > 0 && remote_instance->frame_diff > 0)
    {
        ret = audio_remote_buffer_PLC_policy_1(handle);
    }
    else if (remote_instance->seq_diff > 0 && remote_instance->frame_diff <= 0)
    {

        while (remote_instance->frame_diff <= 0)
        {
            p_media_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
            if (p_media_packet == NULL)
            {
                AUDIO_PRINT_ERROR0("media_buffer_plc: no more packets");
                return false;
            }
            remote_instance->frame_diff += p_media_packet->frame_number;
            buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
            remote_instance->seq_diff = remote_instance->seq_diff + 1;
        }

        ret = audio_remote_buffer_PLC_policy_1(remote_instance);
    }
    else
    {
        while (remote_instance->seq_diff < 0)
        {
            p_media_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);
            if (p_media_packet == NULL)
            {
                AUDIO_PRINT_ERROR0("media_buffer_plc: no more packets");
                return false;
            }
            remote_instance->frame_diff += p_media_packet->frame_number;
            buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
            remote_instance->seq_diff = remote_instance->seq_diff + 1;
        }

        if (remote_instance->frame_diff == 0)
        {
            AUDIO_PRINT_TRACE2("media_buffer_plc: remain seq %d frames %d",
                               remote_instance->seq_diff,
                               remote_instance->frame_diff);
            ret = media_buffer_downstream_dsp(buffer_ent, true);
        }
        else
        {
            if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_SBC)
            {
                if (remote_instance->frame_diff > 0)
                {
                    ret = audio_remote_buffer_PLC_policy_2(remote_instance);
                }
                else
                {
                    ret = audio_remote_buffer_PLC_policy_3(remote_instance);
                }

            }
            else if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_AAC)
            {
                if (remote_instance->frame_diff < 0)
                {
                    if (media_buffer_ds_pkt_cnt(buffer_ent) != 0)
                    {
                        remote_instance->frame_diff += 1;
                        buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                        ret = true;
                    }
                    else
                    {
                        ret = false;
                    }

                }
                else
                {
                    AUDIO_PRINT_WARN0("media_buffer_plc: err");
                    audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PACKET_LOSS, 0, remote_instance->frame_diff);
                    ret = false;
                }

            }
            else
            {
                AUDIO_PRINT_WARN0("media_buffer_plc: codec has not implented");
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PACKET_LOSS, 0, remote_instance->frame_diff);
                ret = false;
            }
        }

    }
    return ret;
}

int audio_remote_get_frame_diff(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    if (audio_remote_handle_check(remote_instance))
    {
        return remote_instance->frame_diff;
    }
    else
    {
        return false;
    }
}

void  audio_remote_set_frame_diff(T_AUDIO_REMOTE_HANDLE handle, int frame_diff)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    if (audio_remote_handle_check(remote_instance))
    {
        remote_instance->frame_diff = frame_diff;
    }
}


bool audio_remote_seamless_join_seq_contiuity_check(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (audio_remote_handle_check(remote_instance) == false)
    {
        return false;
    }

    if (remote_instance->sync_type != AUDIO_STREAM_TYPE_PLAYBACK)
    {
        return false;
    }

    if (audio_remote_db->force_join)
    {
        if (remote_instance->seamless_join_seq_contiuity_check_timer == NULL)
        {
            remote_instance->seamless_join_seq_contiuity_check_timer =
                sys_timer_create("seamless_join_seq_contiuity_check_timer",
                                 SYS_TIMER_TYPE_LOW_PRECISION,
                                 (AUDIO_TIMER_SEAMLESS_JOIN_SEQUENCE_CHECK << 16) | remote_instance->endpoint,
                                 SEQUENCE_CHECK_TIME,
                                 false,
                                 audio_remote_timeout_cback);
            if (remote_instance->seamless_join_seq_contiuity_check_timer != NULL)
            {
                sys_timer_start(remote_instance->seamless_join_seq_contiuity_check_timer);
            }
            remote_instance->seamless_join_lost_packet_cnt = 0;
            remote_instance->seamless_join_lost_packet_cnt_last = MAX_SEAMLESS_JOIN_CHECK_LOST_COUNT;
        }

        if (remote_instance->seamless_join_lost_packet_cnt_last < MAX_SEAMLESS_JOIN_CHECK_LOST_COUNT)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (remote_instance->seamless_join_seq_contiuity_check_timer)
        {
            sys_timer_delete(remote_instance->seamless_join_seq_contiuity_check_timer);
            remote_instance->seamless_join_seq_contiuity_check_timer = NULL;
            remote_instance->seamless_join_lost_packet_cnt = 0;
            remote_instance->seamless_join_lost_packet_cnt_last = 0;
        }
        return true;
    }
}

void audio_remote_seamless_join_set(bool enable, bool force_join)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance;

    AUDIO_PRINT_TRACE2("audio_remote_seamless_join_set: enable %d force %d", enable, force_join);
    audio_remote_db->force_join = force_join;
    audio_remote_db->seamless_join = enable;
    if (force_join)
    {
        remote_instance = os_queue_peek(&audio_remote_db->remote_db_list, 0);
        while (remote_instance != NULL)
        {
            if (remote_instance->sync_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                if (remote_instance->seamless_join_seq_contiuity_check_timer == NULL)
                {
                    remote_instance->seamless_join_seq_contiuity_check_timer =
                        sys_timer_create("seamless_join_seq_contiuity_check_timer",
                                         SYS_TIMER_TYPE_LOW_PRECISION,
                                         (AUDIO_TIMER_SEAMLESS_JOIN_SEQUENCE_CHECK << 16) | remote_instance->endpoint,
                                         SEQUENCE_CHECK_TIME,
                                         false,
                                         audio_remote_timeout_cback);
                    if (remote_instance->seamless_join_seq_contiuity_check_timer != NULL)
                    {
                        sys_timer_start(remote_instance->seamless_join_seq_contiuity_check_timer);
                    }
                    remote_instance->seamless_join_lost_packet_cnt = 0;
                    remote_instance->seamless_join_lost_packet_cnt_last = 0;
                }
            }

            remote_instance = remote_instance->p_next;
        }

    }
    else
    {
        remote_instance = os_queue_peek(&audio_remote_db->remote_db_list, 0);
        while (remote_instance != NULL)
        {
            if (remote_instance->sync_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                sys_timer_delete(remote_instance->seamless_join_seq_contiuity_check_timer);
                remote_instance->seamless_join_seq_contiuity_check_timer = NULL;
                remote_instance->seamless_join_lost_packet_cnt = 0;
                remote_instance->seamless_join_lost_packet_cnt_last = 0;
            }
            remote_instance = remote_instance->p_next;
        }
    }
}

void audio_remote_handle_lost_packet(T_AUDIO_REMOTE_HANDLE handle, uint8_t count)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (audio_remote_handle_check(remote_instance) == false)
    {
        return;
    }

    if (remote_instance->seamless_join_seq_contiuity_check_timer)
    {
        remote_instance->seamless_join_lost_packet_cnt += count;
    }
}

bool audio_remote_async_msg_relay(T_AUDIO_REMOTE_HANDLE handle,
                                  uint16_t               msg_id,
                                  void                  *msg_buf,
                                  uint16_t               msg_len,
                                  bool                   loopback)
{
    bool ret;
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    T_AUDIO_REMOTE_INTERNAL_MSG *async_msg;
    T_AUDIO_REMOTE_BUFFER_MSG_RELAY *relay_msg;
    uint16_t malloc_len = msg_len + sizeof(T_AUDIO_REMOTE_BUFFER_MSG_RELAY) + sizeof(
                              T_AUDIO_REMOTE_INTERNAL_MSG);

    if (audio_remote_handle_check(remote_instance) == false)
    {
        return false;
    }
    async_msg = (T_AUDIO_REMOTE_INTERNAL_MSG *)os_mem_zalloc2(malloc_len);
    if (async_msg == NULL)
    {
        return false;
    }
    relay_msg = (T_AUDIO_REMOTE_BUFFER_MSG_RELAY *)async_msg->msg;

    async_msg->remote_endpoint = remote_instance->endpoint;
    relay_msg->msg_id = msg_id;
    memcpy(relay_msg->msg, msg_buf, msg_len);

    ret = remote_async_msg_relay(audio_remote_db->relay_handle,
                                 AUDIO_REMOTE_BUFFER_MSG_RELAY,
                                 async_msg,
                                 malloc_len,
                                 loopback);

    os_mem_free(async_msg);
    return ret;
}

bool audio_remote_sync_msg_relay(T_AUDIO_REMOTE_HANDLE handle,
                                 uint16_t               msg_id,
                                 void                  *msg_buf,
                                 uint16_t               msg_len,
                                 T_REMOTE_TIMER_TYPE    timer_type,
                                 uint32_t               timer_period,
                                 bool                   loopback)
{
    bool ret;
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;
    T_AUDIO_REMOTE_INTERNAL_MSG *sync_msg;
    T_AUDIO_REMOTE_BUFFER_MSG_RELAY *relay_msg;
    uint16_t malloc_len = msg_len + sizeof(T_AUDIO_REMOTE_BUFFER_MSG_RELAY) + sizeof(
                              T_AUDIO_REMOTE_INTERNAL_MSG);

    if (audio_remote_handle_check(remote_instance) == false)
    {
        return false;
    }
    sync_msg = (T_AUDIO_REMOTE_INTERNAL_MSG *)os_mem_zalloc2(malloc_len);
    if (sync_msg == NULL)
    {
        return false;
    }
    relay_msg = (T_AUDIO_REMOTE_BUFFER_MSG_RELAY *)sync_msg->msg;

    sync_msg->remote_endpoint = remote_instance->endpoint;
    relay_msg->msg_id = msg_id;
    memcpy(relay_msg->msg, msg_buf, msg_len);

    ret = remote_sync_msg_relay(audio_remote_db->relay_handle,
                                AUDIO_REMOTE_BUFFER_MSG_RELAY,
                                sync_msg,
                                malloc_len,
                                timer_type,
                                timer_period,
                                loopback);
    os_mem_free(sync_msg);
    return ret;
}

void audio_remote_sync_lock(T_AUDIO_REMOTE_HANDLE handle)
{
    T_AUDIO_REMOTE_INSTANCE *remote_instance = (T_AUDIO_REMOTE_INSTANCE *)handle;

    if (audio_remote_handle_check(handle))
    {
        remote_instance->slave_audio_sync = true;
    }
}
