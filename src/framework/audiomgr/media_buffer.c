/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "trace.h"
#include "jitter_buffer.h"
#include "media_pool.h"
#include "media_buffer.h"
#include "audio_codec.h"
#include "audio_remote.h"
#include "audio_latency.h"
#include "audio_interval_eval.h"

/* TODO Remove Start */
#include "audio_mgr.h"
#include "dsp_mgr.h"
#include "audio_path.h"
#include "bt_mgr.h"
#include "remote.h"
#include "notification.h"

extern void *hEventQueueHandleAu;
extern bool bt_clk_compare(uint32_t bt_clk_a, uint32_t bt_clk_b);

#define LOW_LATENCY_ASRC_ADJ_SPEED_PPM_LIM  500
#define LOW_LATENCY_ASRC_ADJ_DURATION_LIM   0x0fffffff/1000*(625/2)

/* TODO Remove End */

typedef struct
{
    uint16_t event;
    int final_asrc;
    uint32_t sync_clk;
    uint8_t clk_ref;
    uint8_t tid;
} T_MEDIA_BUFFER_ASRC_SYNC_ADJUST;

typedef enum
{
    MEDIA_BUFFER_REMOTE_LOW_LAT_ADJUST,
    MEDIA_BUFFER_REMOTE_LOW_LAT_ADJUST_RESTORE,
    MEDIA_BUFFER_REMOTE_ASRC_SYNC_ADJUST,
    MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_INC,
    MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_DEC,
    MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_INC_REQ,
    MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_SET,
    MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_RESUME,
    MEDIA_BUFFER_REMOTE_ULTRA_LOW_LATENCY_PLC_REQ,
    MEDIA_BUFFER_REMOTE_LOW_LATENCY_OVERRIDE,
    MEDIA_BUFFER_REMOTE_AUDIO_SYNCED,
    MEDIA_BUFFER_REMOTE_AUDIO_SYNCED_SUCC,
    MEDIA_BUFFER_PRI_ONE_WAY_PLAY,
    MEDIA_BUFFER_REMOTE_RPT_EXCEPTION,
    MEDIA_BUFFER_REMOTE_QUERY_PATH_STATE,
    MEDIA_BUFFER_REMOTE_QUERY_PATH_STATE_RSP,
    MEDIA_BUFFER_REMOTE_PRI_RESET,
} T_MEDIA_BUFFER_REMOTE_MSG;

typedef struct t_media_bffer_db
{
    T_OS_QUEUE  media_buffer_list;
} T_MEDIA_BFFER_DB;

static T_MEDIA_BFFER_DB media_buffer_db = {0};

void voice_buffer_audio_remote_evt_cback(T_AUDIO_REMOTE_EVT event, void *param, void *owner);
void voice_buffer_audio_remote_relay_cback(uint16_t event, void *param,
                                           T_REMOTE_RELAY_STATUS status, void *owner);
void playback_buffer_audio_remote_evt_cback(T_AUDIO_REMOTE_EVT event, void *param, void *owner);
void playback_buffer_audio_remote_relay_cback(uint16_t event, void *param,
                                              T_REMOTE_RELAY_STATUS status, void *owner);
void playback_buffer_jb_evt_cback(T_JITTER_BUFFER_EVT event, void *param, void *owner);
void playback_buffer_lat_evt_cback(T_AUDIO_LATENCY_EVT event, void *param, void *owner);
void playback_buffer_interval_eval_evt_cback(T_AUDIO_INTERVAL_EVAL_EVT event, void *param,
                                             void *owner);

void media_buffer_backup_update(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_MEDIA_DATA_HDR *mediapacket)
{
    if (buffer_ent->last_packet_ptr != NULL)
    {
        media_buffer_put(audio_db->playback_pool_handle, buffer_ent->last_packet_ptr);
        buffer_ent->last_packet_ptr = mediapacket;
    }
    else
    {
        buffer_ent->last_packet_ptr = mediapacket;
    }
}

static bool __buffer_flush(void *buffer_self, uint16_t cnt, T_MEDIA_BUFFER_DIRECTION direction)
{
    uint16_t i;
    T_BUFFER_QUEUE *p_queue;
    T_MEDIA_DATA_HDR *p_hdr;
    T_H2D_STREAM_HEADER2 *p_h2d_hdr;
    T_MEDIA_BUFFER_ENTITY *media_buffer = buffer_self;

    if (buffer_self == NULL)
    {
        return false;
    }
    if (media_buffer->p_cfg == NULL)
    {
        AUDIO_PRINT_ERROR0("__buffer_flush: p_cfg is null!!");
        return false;
    }

    if (cnt == 0)
    {
        return true;
    }
    if (direction == BUFFER_DIR_DOWNSTREAM)
    {
        p_queue = &media_buffer->downstream_queue;
    }
    else
    {
        p_queue = &media_buffer->upstream_queue;
    }

    if (cnt > p_queue->queue.count)
    {
        cnt = p_queue->queue.count;
    }
    for (i = 0; i < cnt; i++)
    {
        p_hdr = os_queue_out(&p_queue->queue);
        if (p_hdr == NULL)
        {
            return false;
        }

        p_h2d_hdr = (T_H2D_STREAM_HEADER2 *)p_hdr->p_data;

        p_queue->total_frames_in_buffer -= p_hdr->frame_number;
        p_queue->total_payload_bytes_in_buffer -= p_h2d_hdr->payload_length;

        p_queue->total_block_bytes_in_buffer -= p_hdr->used_block_size;
        if (media_buffer->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
            media_buffer->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
        {
            media_buffer_backup_update(media_buffer, p_hdr);
        }
        else
        {
            media_buffer_put(media_buffer->pool_handle, p_hdr);
        }
    }

    AUDIO_PRINT_TRACE4("media_buffer_flush: cnt %d direction %d remain bytes %u remain frames %u", cnt,
                       direction, p_queue->total_payload_bytes_in_buffer, p_queue->total_frames_in_buffer);
    return true;
}

static void *__buffer_peek(void *buffer_self, int index, uint16_t *len, uint32_t *timestamp,
                           T_MEDIA_BUFFER_DIRECTION direction)
{
    T_MEDIA_DATA_HDR *p_hdr;
    T_MEDIA_BUFFER_ENTITY *media_buffer = buffer_self;

    if (buffer_self == NULL)
    {
        return NULL;
    }

    if (direction == BUFFER_DIR_DOWNSTREAM)
    {
        p_hdr = os_queue_peek(&media_buffer->downstream_queue.queue, index);
    }
    else
    {
        p_hdr = os_queue_peek(&media_buffer->upstream_queue.queue, index);
    }
    if (p_hdr != NULL)
    {
        if (len != NULL)
        {
            *len = p_hdr->payload_length;
        }
        if (timestamp != NULL)
        {
            *timestamp = p_hdr->bt_clk;
        }
    }
    return p_hdr;
}

static uint16_t __buffer_shrink(void *buffer_self, void *p_packet, uint16_t len,
                                T_MEDIA_BUFFER_DIRECTION direction)
{
    uint16_t remain_len;
    uint16_t shrink_len;
    T_MEDIA_BUFFER_ENTITY *media_buffer = buffer_self;
    T_MEDIA_DATA_HDR *p_hdr = p_packet;
    T_H2D_STREAM_HEADER2 *p_h2d_hdr;
    T_OS_QUEUE *p_queue;

    if (buffer_self == NULL || p_hdr == NULL)
    {
        return 0;
    }

    if (direction == BUFFER_DIR_DOWNSTREAM)
    {
        p_queue = &media_buffer->downstream_queue.queue;
    }
    else
    {
        p_queue = &media_buffer->upstream_queue.queue;
    }
    if (os_queue_search(p_queue, p_hdr) == false) //get upstream queue packet
    {
        return 0;
    }
    p_h2d_hdr = (T_H2D_STREAM_HEADER2 *)p_hdr->p_data;
    remain_len = p_h2d_hdr->payload_length - p_hdr->read_offset;
    shrink_len = len <= remain_len ? len : remain_len;

    p_hdr->read_offset += shrink_len;
    if (shrink_len == remain_len)
    {
        os_queue_delete(p_queue, p_hdr);
        media_buffer_put(media_buffer->pool_handle, p_hdr);
    }

    return shrink_len;
}

static bool __buffer_write(void                     *buffer_self,
                           void                     *data,
                           uint16_t                  len,
                           uint16_t                 *written_len,
                           void                     *param,
                           T_MEDIA_BUFFER_DIRECTION  write_direction)
{
    T_MEDIA_DATA_HDR *p_hdr = (T_MEDIA_DATA_HDR *)param;
    T_MEDIA_DATA_HDR *p_packet;
    uint16_t malloc_len;
    T_MEDIA_BUFFER_ENTITY *media_buffer = buffer_self;
    T_H2D_STREAM_HEADER2 *h2d_hdr;

    *written_len = 0;

    if (media_buffer == NULL || data == NULL)
    {
        return false;
    }

    if (p_hdr == NULL)
    {
        return false;
    }
    p_hdr->payload_length = len + sizeof(T_H2D_STREAM_HEADER2); //fix length
    malloc_len = sizeof(T_MEDIA_DATA_HDR) + sizeof(T_H2D_STREAM_HEADER2) + len;
    p_packet = (T_MEDIA_DATA_HDR *)media_buffer_get(media_buffer->pool_handle, malloc_len);
    if (p_packet == NULL)
    {
        return false;
    }
    memcpy(p_packet, p_hdr, sizeof(T_MEDIA_DATA_HDR));
    p_packet->used_block_size = malloc_len;
    h2d_hdr = (T_H2D_STREAM_HEADER2 *)p_packet->p_data;
    h2d_hdr->timestamp = p_packet->bt_clk;
    h2d_hdr->local_seq = p_packet->local_seq_number;
    h2d_hdr->frame_number = p_packet->frame_number;
    h2d_hdr->payload_length = len;
    h2d_hdr->status = p_packet->pkt_status;

    memcpy(p_packet->p_data + sizeof(T_H2D_STREAM_HEADER2), data, len);
    p_packet->read_offset = 0;

    if (write_direction == BUFFER_DIR_DOWNSTREAM)
    {

        os_queue_in(&media_buffer->downstream_queue.queue, p_packet);
        media_buffer->downstream_queue.total_frames_in_buffer += p_packet->frame_number;
        media_buffer->downstream_queue.total_payload_bytes_in_buffer += len;
        media_buffer->downstream_queue.total_block_bytes_in_buffer += p_packet->used_block_size;
        *written_len = len;
    }
    else
    {
        os_queue_in(&media_buffer->upstream_queue.queue, p_packet);
        media_buffer->upstream_queue.total_frames_in_buffer += p_packet->frame_number;
        media_buffer->upstream_queue.total_payload_bytes_in_buffer += len;
        media_buffer->upstream_queue.total_block_bytes_in_buffer += p_packet->used_block_size;
        *written_len = len;
    }

    return true;
}

static void __buffer_reset(void *buffer_self)
{
    T_MEDIA_BUFFER_ENTITY *media_buffer = buffer_self;
    if (media_buffer == NULL)
    {
        return;
    }

    if (!media_buffer->reset_wo_flush)
    {
        media_buffer->ops.flush_fun(media_buffer, media_buffer_ds_pkt_cnt(media_buffer),
                                    BUFFER_DIR_DOWNSTREAM);
        media_buffer->ops.flush_fun(media_buffer, media_buffer_us_pkt_cnt(media_buffer),
                                    BUFFER_DIR_UPSTREAM);
        media_buffer->last_seq = 0;
        media_buffer->last_seq_updated = false;
        media_buffer->downstream_queue.total_frames_in_buffer = 0;
        media_buffer->downstream_queue.total_payload_bytes_in_buffer = 0;
        media_buffer->downstream_queue.total_block_bytes_in_buffer = 0;
        media_buffer->upstream_queue.total_frames_in_buffer = 0;
        media_buffer->upstream_queue.total_payload_bytes_in_buffer = 0;
        media_buffer->upstream_queue.total_block_bytes_in_buffer = 0;
    }

    media_buffer->status = MEDIA_BUFFER_UNINIT;
    media_buffer->frame_len = 0;
    media_buffer->start_decode_frame_cnt = 0;
    media_buffer->miss_ack_cnt = 0;
    media_buffer_clear_downstream_info(media_buffer);
    media_buffer->voice_plc_check_interval = 0xff;//invalid BT_CLK
    media_buffer->reset_wo_flush = 0;
    media_buffer->paused = 0;
    media_buffer->ignore_latency_report = 0;
    media_buffer->local_audio_sync = false;

    if (media_buffer->p_cfg == NULL)
    {
        AUDIO_PRINT_ERROR0("__buffer_reset: p_cfg is null!!");
        return;
    }

    media_buffer_backup_update(media_buffer, NULL);
    audio_latency_buffer_reset(media_buffer->audio_latency_handle);
    jitter_buffer_buffer_reset(media_buffer->jitter_buffer_handle);
    audio_remote_reset(media_buffer->audio_remote_handle);

    AUDIO_PRINT_TRACE1("media_buffer_reset: stream_type %u media_buffer_set uninit",
                       media_buffer->p_cfg->stream_type);
}

static void __buffer_dump(void *buffer_self)
{

}

const T_MEDIA_BUFFER_OP media_buffer_ops =
{
    .peek_fun       = __buffer_peek,
    .shrink_fun     = __buffer_shrink,
    .write_fun      = __buffer_write,
    .flush_fun      = __buffer_flush,
    .dump_fun       = __buffer_dump,
    .reset_fun      = __buffer_reset,
};

T_MEDIA_BUFFER_HANDLE media_buffer_construct(T_AUDIO_STREAM_TYPE buffer_type)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    int ret;

    buffer_ent = os_mem_zalloc2(sizeof(T_MEDIA_BUFFER_ENTITY));
    if (buffer_ent == NULL)
    {
        ret = 1;
        goto fail_alloc_mem;
    }

    memcpy(&buffer_ent->ops, &media_buffer_ops, sizeof(T_MEDIA_BUFFER_OP));
    buffer_ent->status = MEDIA_BUFFER_UNINIT;
    os_queue_init(&buffer_ent->downstream_queue.queue);
    os_queue_init(&buffer_ent->upstream_queue.queue);

    os_queue_in(&media_buffer_db.media_buffer_list, buffer_ent);

    if (buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        buffer_ent->pool_handle = audio_db->playback_pool_handle;
        buffer_ent->audio_remote_handle = audio_remote_media_buffer_register(AUDIO_STREAM_TYPE_PLAYBACK,
                                                                             buffer_ent,
                                                                             playback_buffer_audio_remote_evt_cback,
                                                                             playback_buffer_audio_remote_relay_cback);

        buffer_ent->buffer_type = AUDIO_STREAM_TYPE_PLAYBACK;
        buffer_ent->jitter_buffer_handle = jitter_buffer_register(buffer_ent, playback_buffer_jb_evt_cback);
        buffer_ent->audio_latency_handle = audio_latency_mgr_register(buffer_ent,
                                                                      playback_buffer_lat_evt_cback);

    }
    else if (buffer_type == AUDIO_STREAM_TYPE_VOICE)
    {
        buffer_ent->pool_handle = audio_db->voice_pool_handle;
        buffer_ent->buffer_type = AUDIO_STREAM_TYPE_VOICE;
        buffer_ent->audio_remote_handle = audio_remote_media_buffer_register(AUDIO_STREAM_TYPE_VOICE,
                                                                             buffer_ent,
                                                                             voice_buffer_audio_remote_evt_cback,
                                                                             voice_buffer_audio_remote_relay_cback);
    }
    else
    {
        buffer_ent->pool_handle = audio_db->record_pool_handle;
        buffer_ent->buffer_type = AUDIO_STREAM_TYPE_RECORD;
    }

    return (T_MEDIA_BUFFER_HANDLE)buffer_ent;

fail_alloc_mem:
    AUDIO_PRINT_ERROR1("media_buffer_construct: failed %d", -ret);
    return NULL;
}

void media_buffer_destruct(T_MEDIA_BUFFER_HANDLE buffer_handle)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent = buffer_handle;
    if (buffer_ent != NULL)
    {
        buffer_ent->ops.reset_fun(buffer_ent);
        if (buffer_ent->oneway_play_timer)
        {
            sys_timer_delete(buffer_ent->oneway_play_timer);
        }
        if (buffer_ent->audio_remote_handle)
        {
            audio_remote_media_buffer_unregister(buffer_ent->audio_remote_handle);
        }
        if (buffer_ent->jitter_buffer_handle)
        {
            jitter_buffer_unregister(buffer_ent->jitter_buffer_handle);
        }
        if (buffer_ent->audio_latency_handle)
        {
            audio_latency_mgr_unregister(buffer_ent->audio_latency_handle);
        }
        if (buffer_ent->audio_interval_eval_handle)
        {
            audio_interval_evaluator_unregister(buffer_ent->audio_interval_eval_handle);
        }

        os_queue_delete(&media_buffer_db.media_buffer_list, buffer_ent);
        os_mem_free(buffer_ent);
    }
}

bool media_buffer_init(void)
{
    int32_t ret = 0;

    os_queue_init(&media_buffer_db.media_buffer_list);

    if (audio_db->playback_pool_size != 0)
    {
        audio_db->playback_pool_handle = media_pool_create(audio_db->playback_pool_size);
        if (audio_db->playback_pool_handle == NULL)
        {
            ret = 1;
            goto fail_create_audio_pool;
        }
    }

    if (audio_db->voice_pool_size != 0)
    {
        audio_db->voice_pool_handle = media_pool_create(audio_db->voice_pool_size);
        if (audio_db->voice_pool_handle == NULL)
        {
            ret = 2;
            goto fail_create_voice_pool;
        }
    }

    if (audio_db->record_pool_size != 0)
    {
        audio_db->record_pool_handle = media_pool_create(audio_db->record_pool_size);
        if (audio_db->record_pool_handle == NULL)
        {
            ret = 3;
            goto fail_create_record_pool;
        }
    }

    audio_db->playback_buffer_handle = media_buffer_construct(AUDIO_STREAM_TYPE_PLAYBACK);
    if (audio_db->playback_buffer_handle == NULL)
    {
        ret = 4;
        goto fail_create_playback_buf;
    }

    audio_db->voice_buffer_handle = media_buffer_construct(AUDIO_STREAM_TYPE_VOICE);
    if (audio_db->voice_buffer_handle == NULL)
    {
        ret = 5;
        goto fail_create_voice_buf;
    }

    audio_db->record_buffer_handle = media_buffer_construct(AUDIO_STREAM_TYPE_RECORD);
    if (audio_db->record_buffer_handle == NULL)
    {
        ret = 6;
        goto fail_create_record_buf;
    }

    return true;

fail_create_record_buf:
    media_buffer_destruct(audio_db->voice_buffer_handle);
fail_create_voice_buf:
    media_buffer_destruct(audio_db->playback_buffer_handle);
fail_create_playback_buf:
    media_pool_destory(audio_db->record_pool_handle);
fail_create_record_pool:
    media_pool_destory(audio_db->voice_pool_handle);
fail_create_voice_pool:
    media_pool_destory(audio_db->playback_pool_handle);
fail_create_audio_pool:
    AUDIO_PRINT_ERROR1("media_buffer_init: ret %d", -ret);
    return false;
}

void media_buffer_deinit(void)
{
    if (audio_db->playback_buffer_handle != NULL)
    {
        media_buffer_destruct(audio_db->playback_buffer_handle);
        media_pool_destory(audio_db->playback_pool_handle);
    }

    if (audio_db->voice_buffer_handle != NULL)
    {
        media_buffer_destruct(audio_db->voice_buffer_handle);
        media_pool_destory(audio_db->voice_pool_handle);
    }

    if (audio_db->record_buffer_handle != NULL)
    {
        media_buffer_destruct(audio_db->record_buffer_handle);
        media_pool_destory(audio_db->record_pool_handle);
    }
}

uint16_t media_buffer_cacu_frame_len(T_MEDIA_BUFFER_ENTITY *buffer_ent,
                                     T_AUDIO_FORMAT_INFO   *format_info,
                                     void                  *param)
{
    uint16_t frame_len = 0;

    switch (format_info->type)
    {
    case AUDIO_FORMAT_TYPE_AAC:
        {
            if (buffer_ent->downstream_queue.total_frames_in_buffer == 0)
            {
                frame_len = 0;
            }
            else
            {
                frame_len = buffer_ent->downstream_queue.total_payload_bytes_in_buffer /
                            buffer_ent->downstream_queue.total_frames_in_buffer;
            }
        }
        break;

    case AUDIO_FORMAT_TYPE_MP3:
        {
            if (buffer_ent->downstream_queue.total_frames_in_buffer == 0)
            {
                frame_len = 0;
            }
            else
            {
                frame_len = buffer_ent->downstream_queue.total_payload_bytes_in_buffer /
                            buffer_ent->downstream_queue.total_frames_in_buffer;
            }
        }
        break;

    case AUDIO_FORMAT_TYPE_SBC:
        {
            T_AUDIO_SBC_ATTR *sbc_attr = &format_info->attr.sbc;

            sbc_attr->bitpool = *((uint8_t *)param + 2);

            switch (sbc_attr->chann_mode)
            {
            case AUDIO_SBC_CHANNEL_MODE_MONO: /* MONO */
                frame_len = 4 + ((4 * sbc_attr->subband_num * 1) / 8) + ((sbc_attr->block_length * sbc_attr->bitpool
                                                                          + 7) /
                                                                         8);
                break;

            case AUDIO_SBC_CHANNEL_MODE_DUAL: /* DUAL_CHANNEL */
                frame_len = 4 + ((4 * sbc_attr->subband_num * 2) / 8) + ((sbc_attr->block_length * sbc_attr->bitpool
                                                                          + 7) /
                                                                         8);
                break;

            case AUDIO_SBC_CHANNEL_MODE_STEREO: /* STEREO */
                frame_len = 4 + ((4 * sbc_attr->subband_num * 2) / 8) + ((sbc_attr->block_length * sbc_attr->bitpool
                                                                          + 7) /
                                                                         8);
                break;

            case AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO: /* JOINT_STEREO */
                frame_len = 4 + ((4 * sbc_attr->subband_num * 2) / 8) + (((1 * sbc_attr->subband_num) +
                                                                          (sbc_attr->block_length * sbc_attr->bitpool) + 7) / 8);
                break;

            default:
                break;
            }
        }
        break;

    case AUDIO_FORMAT_TYPE_CVSD:
        {
            if (format_info->attr.cvsd.frame_duration == AUDIO_CVSD_FRAME_DURATION_3_75_MS)
            {
                frame_len = (16 / 2 / 8) * format_info->attr.cvsd.sample_rate * (15 / 4) / 1000;
            }
            else if (format_info->attr.cvsd.frame_duration == AUDIO_CVSD_FRAME_DURATION_7_5_MS)
            {
                frame_len = (16 / 2 / 8) * format_info->attr.cvsd.sample_rate * (15 / 2) / 1000;
            }
        }
        break;

    case AUDIO_FORMAT_TYPE_MSBC:
        {
            T_AUDIO_MSBC_ATTR *msbc_attr = &format_info->attr.msbc;

            msbc_attr->bitpool = *((uint8_t *)param + 2);
            frame_len = 4 + ((4 * msbc_attr->subband_num * 1) / 8) + ((msbc_attr->block_length *
                                                                       msbc_attr->bitpool + 7) / 8);
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            frame_len = format_info->attr.lc3.frame_length;
        }
        break;

    case AUDIO_FORMAT_TYPE_PCM:
        {
            frame_len = format_info->attr.pcm.frame_length;
        }
        break;

    case AUDIO_FORMAT_TYPE_LDAC:
        {
            frame_len = buffer_ent->downstream_queue.total_payload_bytes_in_buffer /
                        buffer_ent->downstream_queue.total_frames_in_buffer;
        }
        break;

    case AUDIO_FORMAT_TYPE_LHDC:
        {
            frame_len = buffer_ent->downstream_queue.total_payload_bytes_in_buffer /
                        buffer_ent->downstream_queue.total_frames_in_buffer;
        }
        break;

    default:
        break;
    }

    return frame_len;
}

uint32_t media_buffer_cacu_frame_duration(T_AUDIO_FORMAT_INFO *format_info)
{
    uint32_t frame_duration = 0;

    switch (format_info->type)
    {
    case AUDIO_FORMAT_TYPE_MSBC:
        {
            frame_duration = 15000 / 2;
        }
        break;

    case AUDIO_FORMAT_TYPE_CVSD:
        {
            T_AUDIO_CVSD_ATTR *cvsd_attr;

            cvsd_attr  = &format_info->attr.cvsd;
            if (cvsd_attr->frame_duration == AUDIO_CVSD_FRAME_DURATION_3_75_MS)
            {
                frame_duration = 15000 / 4;
            }
            else
            {
                frame_duration = 15000 / 2;
            }
        }
        break;

    case AUDIO_FORMAT_TYPE_SBC:
        {
            T_AUDIO_SBC_ATTR *sbc_attr;

            sbc_attr       = &format_info->attr.sbc;
            frame_duration = (sbc_attr->block_length * sbc_attr->subband_num) *
                             1000 * 1000 / sbc_attr->sample_rate;
        }
        break;

    case AUDIO_FORMAT_TYPE_AAC:
        {
            T_AUDIO_AAC_ATTR *aac_attr;

            aac_attr       = &format_info->attr.aac;
            frame_duration = 1024 * 1000 * 1000 / aac_attr->sample_rate;
        }
        break;

    case AUDIO_FORMAT_TYPE_MP3:
        {
            T_AUDIO_MP3_ATTR *mp3_attr;

            mp3_attr       = &format_info->attr.mp3;

            frame_duration = audio_codec_frame_size_get(AUDIO_FORMAT_TYPE_MP3, mp3_attr) *
                             1000 * 1000 / mp3_attr->sample_rate;
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            T_AUDIO_LC3_ATTR *lc3_attr;

            lc3_attr  = &format_info->attr.lc3;
            if (lc3_attr->frame_duration == AUDIO_LC3_FRAME_DURATION_10_MS)
            {
                frame_duration = 10000;
            }
            else
            {
                frame_duration = 15000 / 2;
            }
        }
        break;

    case AUDIO_FORMAT_TYPE_PCM:
        {
            T_AUDIO_PCM_ATTR *pcm_attr;

            pcm_attr       = &format_info->attr.pcm;
            frame_duration = pcm_attr->frame_length * 1000 * 1000 / (pcm_attr->sample_rate *
                                                                     pcm_attr->bit_width / 8);
        }
        break;

    case AUDIO_FORMAT_TYPE_LDAC:
        {
            T_AUDIO_LDAC_ATTR *ldac_attr;
            uint16_t sample_count = 0;

            ldac_attr  = &format_info->attr.ldac;

            switch (ldac_attr->sample_rate)
            {
            case 44100:
            case 48000:
                {
                    sample_count = 128;
                }
                break;

            case 88200:
            case 96000:
                {
                    sample_count = 256;
                }
                break;

            case 176400:
            case 192000:
                {
                    sample_count = 512;
                }
                break;
            }

            frame_duration = sample_count * 1000 * 1000 / ldac_attr->sample_rate;
        }
        break;

    case AUDIO_FORMAT_TYPE_LHDC:
        {
            T_AUDIO_LHDC_ATTR *lhdc_attr;

            lhdc_attr = &format_info->attr.lhdc;
            if (lhdc_attr->frame_duration == AUDIO_LHDC_FRAME_DURATION_5_MS)
            {
                frame_duration = 5000;
            }
        }
        break;

    default:
        break;
    }

    return frame_duration;
}

uint16_t media_buffer_cacu_frame_num(uint16_t time_ms, T_AUDIO_FORMAT_INFO *format_info)
{
    uint16_t frame_num = 0;
    uint32_t frame_duration;

    frame_duration = media_buffer_cacu_frame_duration(format_info);
    if (frame_duration != 0)
    {
        frame_num = (uint16_t)((uint32_t)time_ms * 1000 / frame_duration);
    }

    return frame_num;
}

uint16_t media_buffer_cacu_frames_duration(uint16_t frame_num, T_AUDIO_FORMAT_INFO *format_info)
{
    uint32_t frame_duration;
    uint16_t total_duration;

    frame_duration = media_buffer_cacu_frame_duration(format_info);
    total_duration = (uint16_t)(frame_duration * frame_num / 1000);
    return total_duration;
}

uint16_t media_buffer_cacu_frames_length(T_AUDIO_FORMAT_INFO *format_info)
{
    uint16_t frames_length;
    uint8_t  frame_num = 1;

    if (format_info->frame_num > frame_num)
    {
        frame_num = format_info->frame_num;
    }

    switch (format_info->type)
    {
    case AUDIO_FORMAT_TYPE_MSBC:
        {
            frames_length = 60;
        }
        break;

    case AUDIO_FORMAT_TYPE_CVSD:
        {
            T_AUDIO_CVSD_ATTR *cvsd_attr;

            cvsd_attr  = &format_info->attr.cvsd;
            if (cvsd_attr->frame_duration == AUDIO_CVSD_FRAME_DURATION_3_75_MS)
            {
                frames_length = 30;
            }
            else
            {
                frames_length = 60;
            }
        }
        break;

    case AUDIO_FORMAT_TYPE_AAC:
        {
            T_AUDIO_AAC_ATTR *aac_attr;

            aac_attr      = &format_info->attr.aac;
            frames_length = aac_attr->bitrate * media_buffer_cacu_frame_duration(format_info) / 1000 / 8
                            * frame_num;
        }
        break;

    case AUDIO_FORMAT_TYPE_MP3:
        {
            T_AUDIO_MP3_ATTR *mp3_attr;

            mp3_attr      = &format_info->attr.mp3;
            frames_length = mp3_attr->bitrate * media_buffer_cacu_frame_duration(format_info) / 1000 / 8
                            * frame_num;
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            T_AUDIO_LC3_ATTR *lc3_attr;
            uint8_t chan_num = 1;

            lc3_attr      = &format_info->attr.lc3;
            if (lc3_attr->chann_location > AUDIO_CHANNEL_LOCATION_MONO)
            {
                chan_num = __builtin_popcount(lc3_attr->chann_location);
            }

            frames_length = lc3_attr->frame_length * frame_num * chan_num;
        }
        break;

    case AUDIO_FORMAT_TYPE_PCM:
        {
            T_AUDIO_PCM_ATTR *pcm_attr;

            pcm_attr      = &format_info->attr.pcm;
            frames_length = pcm_attr->frame_length * frame_num;
        }
        break;

    case AUDIO_FORMAT_TYPE_OPUS:
        {
            T_AUDIO_OPUS_ATTR *opus_attr;
            uint32_t max_frame_duration;

            opus_attr          = &format_info->attr.opus;
            max_frame_duration = 120;
            frames_length = opus_attr->bitrate * max_frame_duration / 1000 / 8 * frame_num;
        }
        break;

    case AUDIO_FORMAT_TYPE_SBC:
    case AUDIO_FORMAT_TYPE_LDAC:
    case AUDIO_FORMAT_TYPE_LHDC:
        {
            frames_length = audio_db->record_pool_size;
        }
        break;

    default:
        {
            frames_length = 0;
        }
        break;
    }

    return frames_length;
}

bool media_buffer_get_downstream_info(T_MEDIA_BUFFER_ENTITY *buffer_ent,
                                      uint16_t *ds_frame_counter_last, uint16_t *ds_local_seq_last, uint32_t *ds_bt_clk_last)
{
    if (buffer_ent != NULL)
    {
        if (ds_frame_counter_last != NULL)
        {
            *ds_frame_counter_last = buffer_ent->ds_frame_counter_last;
        }

        if (ds_local_seq_last != NULL)
        {
            *ds_local_seq_last = buffer_ent->ds_local_seq_last;
        }

        if (ds_bt_clk_last != NULL)
        {
            *ds_bt_clk_last = buffer_ent->ds_bt_clk_last;
        }

        return true;
    }

    return false;
}

bool media_buffer_clear_downstream_info(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent != NULL)
    {
        buffer_ent->ds_frame_counter_last = 0;
        buffer_ent->ds_local_seq_last = 0;
        buffer_ent->ds_bt_clk_last = INVALID_BT_CLOCK;
        return true;
    }

    return false;
}

bool media_buffer_set_downstream_info(T_MEDIA_BUFFER_ENTITY *buffer_ent,
                                      uint16_t ds_frame_counter_last, uint16_t ds_local_seq_last, uint32_t ds_bt_clk_last)
{
    if (buffer_ent != NULL)
    {
        buffer_ent->ds_frame_counter_last = ds_frame_counter_last;
        buffer_ent->ds_local_seq_last = ds_local_seq_last;
        buffer_ent->ds_bt_clk_last = ds_bt_clk_last;
        return true;
    }

    return false;
}

T_MEDIA_BUFFER_STATUS media_buffer_get_status(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    T_MEDIA_BUFFER_STATUS status = MEDIA_BUFFER_UNINIT;
    if (buffer_ent != NULL)
    {
        status = buffer_ent->status;
    }
    return status;
}

void media_buffer_set_status(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_MEDIA_BUFFER_STATUS status)
{
    if (buffer_ent != NULL && buffer_ent->p_cfg != NULL)
    {
        AUDIO_PRINT_INFO2("media_buffer_set_state: %u buffer %u", status, buffer_ent->p_cfg->stream_type);
        buffer_ent->status = status;
    }
}

uint16_t media_buffer_ds_frame_cnt(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent != NULL)
    {
        return buffer_ent->downstream_queue.total_frames_in_buffer;
    }

    return 0;
}

uint16_t media_buffer_ds_pkt_cnt(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent != NULL)
    {
        return buffer_ent->downstream_queue.queue.count;
    }

    return 0;
}

uint16_t media_buffer_us_frame_cnt(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent != NULL)
    {
        return buffer_ent->upstream_queue.total_frames_in_buffer;
    }

    return 0;
}

uint16_t media_buffer_us_pkt_cnt(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent != NULL)
    {
        return buffer_ent->upstream_queue.queue.count;
    }

    return 0;
}

void media_buffer_latency_get(T_MEDIA_BUFFER_PROXY proxy, uint16_t *latency)
{
    T_MEDIA_BUFFER_CFG *p_cfg;

    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    if (p_cfg != NULL)
    {
        *latency = p_cfg->latency;
    }
}

void media_buffer_latency_set(T_MEDIA_BUFFER_PROXY proxy, uint16_t latency, bool fixed)
{
    T_MEDIA_BUFFER_CFG *p_cfg;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;

    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    if (p_cfg != NULL)
    {
        buffer_ent = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;

        p_cfg->latency_fixed = fixed;
        p_cfg->latency = latency;
        p_cfg->latency_back_up = latency;
        audio_latency_set_dynamic_latency_on(buffer_ent->audio_latency_handle, false);
        p_cfg->plc_count = 0;
        p_cfg->latency_override = false;
    }
}

void media_buffer_latency_set2(T_MEDIA_BUFFER_PROXY proxy, uint16_t latency, bool fixed, float var)
{
    T_MEDIA_BUFFER_CFG *p_cfg;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    bool dynamic_latency_on = false;

    if (proxy == NULL)
    {
        return;
    }

    if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    buffer_ent = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;
    audio_latency_get_dynamic_latency_on(buffer_ent->audio_latency_handle,
                                         &dynamic_latency_on);

    if (p_cfg->latency_override == false)
    {
        if (var > (latency / 2) * (latency / 2))
        {
            T_AUDIO_MSG_LOW_LATENCY_OVERRIDE cmd;
            AUDIO_PRINT_WARN4("the latency %ums is not suitable for this device, inc latency to %u current latency %u current plc cnt %u",
                              latency, latency * 2, p_cfg->latency, p_cfg->plc_count);

            p_cfg->latency_fixed = fixed;
            p_cfg->latency_back_up = latency * 2;
            p_cfg->latency_override = true;

            if (!dynamic_latency_on)
            {
                p_cfg->latency = latency * 2;
                p_cfg->plc_count = 0;
            }

            cmd.dynamic_latency_on = dynamic_latency_on;
            cmd.plc_count = p_cfg->plc_count;
            cmd.latency = p_cfg->latency;
            cmd.latency_override = p_cfg->latency_override;
            cmd.latency_backup = p_cfg->latency_back_up;

            audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                         MEDIA_BUFFER_REMOTE_LOW_LATENCY_OVERRIDE,
                                         &cmd,
                                         sizeof(T_AUDIO_MSG_LOW_LATENCY_OVERRIDE),
                                         false);
        }
    }
    else
    {
        if (var < ((latency - 4) / 2) * ((latency - 4) / 2))
        {
            T_AUDIO_MSG_LOW_LATENCY_OVERRIDE cmd;

            AUDIO_PRINT_WARN4("restore the latency %ums to %ums current latency %u current plc cnt %u",
                              p_cfg->latency_back_up, latency, p_cfg->latency, p_cfg->plc_count);

            p_cfg->latency_fixed = fixed;
            p_cfg->latency_back_up = latency;
            p_cfg->latency_override = false;

            if (!dynamic_latency_on)
            {
                p_cfg->latency = latency;
                p_cfg->plc_count = 0;
            }

            cmd.dynamic_latency_on = dynamic_latency_on;
            cmd.plc_count = p_cfg->plc_count;
            cmd.latency = p_cfg->latency;
            cmd.latency_override = p_cfg->latency_override;
            cmd.latency_backup = p_cfg->latency_back_up;

            audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                         MEDIA_BUFFER_REMOTE_LOW_LATENCY_OVERRIDE,
                                         &cmd,
                                         sizeof(T_AUDIO_MSG_LOW_LATENCY_OVERRIDE),
                                         false);
        }
    }
}

bool media_buffer_level_get(T_MEDIA_BUFFER_PROXY proxy, uint16_t *level)
{
    T_MEDIA_BUFFER_CFG *p_cfg;
    int32_t        ret = 0;

    if (proxy == NULL)
    {
        ret = 1;
        goto fail_invalid_proxy;
    }

    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;

    if (p_cfg->stream_type != AUDIO_STREAM_TYPE_PLAYBACK)
    {
        ret = 2;
        goto fail_stream_type;
    }

    if (media_buffer_get_status(p_cfg->buffer_handle) != MEDIA_BUFFER_PLAYING)
    {
        ret = 3;
        goto fail_invalid_level;
    }

    *level = media_buffer_cacu_frames_duration(media_buffer_ds_frame_cnt(p_cfg->buffer_handle),
                                               &(p_cfg->format_info));
    return true;

fail_invalid_level:
fail_stream_type:
fail_invalid_proxy:
    AUDIO_PRINT_ERROR1("media_buffer_level_get: failed %d", -ret);
    return false;
}

void media_buffer_threshold_set(T_MEDIA_BUFFER_PROXY proxy, uint16_t upper_threshold_ms,
                                uint16_t lower_threshold_ms)
{
    T_MEDIA_BUFFER_CFG *p_cfg;

    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    if (p_cfg != NULL)
    {
        p_cfg->lower_threshold_ms = lower_threshold_ms;
        p_cfg->upper_threshold_ms = upper_threshold_ms;
    }
}

T_MEDIA_BUFFER_PROXY media_buffer_attach(T_AUDIO_STREAM_TYPE   stream_type,
                                         T_AUDIO_STREAM_USAGE  usage,
                                         T_AUDIO_STREAM_MODE   mode,
                                         T_AUDIO_FORMAT_INFO  *format_info_ref,
                                         void                 *track_handle,
                                         void                 *path_handle)
{
    T_MEDIA_BUFFER_CFG *p_cfg;

    if (format_info_ref == NULL)
    {
        return NULL;
    }

    p_cfg = os_mem_alloc2(sizeof(T_MEDIA_BUFFER_CFG));
    if (p_cfg == NULL)
    {
        return NULL;
    }

    p_cfg->track_handle = track_handle;
    p_cfg->stream_type = stream_type;
    p_cfg->usage = usage;
    p_cfg->mode = mode;
    memcpy(&p_cfg->format_info, format_info_ref, sizeof(T_AUDIO_FORMAT_INFO));

    switch (stream_type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        p_cfg->buffer_handle = audio_db->playback_buffer_handle;
        p_cfg->attached_path_handle = path_handle;
        p_cfg->lower_threshold_ms = 0;
        p_cfg->upper_threshold_ms = 0;
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        p_cfg->buffer_handle = audio_db->voice_buffer_handle;
        p_cfg->attached_path_handle = path_handle;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        p_cfg->buffer_handle = audio_db->record_buffer_handle;
        p_cfg->attached_path_handle = path_handle;
        break;

    default:
        break;
    }

    return (T_MEDIA_BUFFER_PROXY)p_cfg;
}

void media_buffer_detach(T_MEDIA_BUFFER_PROXY proxy)
{
    if (proxy != NULL)
    {
        os_mem_free(proxy);
    }
}

uint16_t media_buffer_us_write(T_MEDIA_BUFFER_PROXY proxy,
                               void                 *buf,
                               uint16_t              len,
                               uint32_t              timestamp,
                               uint16_t             seq,
                               T_AUDIO_STREAM_STATUS  status,
                               uint8_t              frame_num)
{
    T_MEDIA_BUFFER_CFG *p_cfg;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_MEDIA_DATA_HDR hdr;
    uint16_t written_len;

    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;

    if (p_cfg == NULL || buf == NULL)
    {
        return 0;
    }
    buffer_ent = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;
    if (buffer_ent == NULL)
    {
        return 0;
    }

    if (buffer_ent->p_cfg == NULL)
    {
        AUDIO_PRINT_ERROR0("media_buffer_us_write: p_cfg is null!!");
        return 0;
    }

    hdr.avdtp_seq_number = seq;
    hdr.frame_number = frame_num;
    hdr.payload_length = len;
    hdr.pkt_format = p_cfg->format_info.type;
    hdr.bt_clk = timestamp;
    hdr.pkt_status = status;
    if (p_cfg->stream_type == AUDIO_STREAM_TYPE_VOICE)
    {
        buffer_ent->ops.flush_fun(buffer_ent, media_buffer_us_pkt_cnt(buffer_ent),
                                  BUFFER_DIR_UPSTREAM);
    }

    if (buffer_ent->ops.write_fun(buffer_ent, (uint8_t *)buf, len,
                                  &written_len, &hdr, BUFFER_DIR_UPSTREAM) == false)
    {
        AUDIO_PRINT_TRACE0("media_buffer_write: write failed");
        return 0;
    }

    return written_len;
}

bool media_buffer_ds_write(T_MEDIA_BUFFER_PROXY   proxy,
                           uint32_t               timestamp,
                           uint16_t               seq,
                           T_AUDIO_STREAM_STATUS  status,
                           uint8_t                frame_num,
                           void                  *buf,
                           uint16_t               len,
                           uint16_t              *written_len)
{
    T_MEDIA_BUFFER_CFG *p_cfg;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_MEDIA_DATA_HDR hdr;
    uint8_t dup_flag = 0;
    uint16_t buffer_level = 0;

    *written_len = 0;

    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    if (p_cfg == NULL || buf == NULL)
    {
        AUDIO_PRINT_ERROR0("media_buffer_ds_write:p_cfg or buf is null!!");
        return false;
    }
    buffer_ent = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;
    if (buffer_ent == NULL)
    {
        return false;
    }

    if (buffer_ent->p_cfg == NULL)
    {
        AUDIO_PRINT_ERROR0("media_buffer_ds_write: p_cfg is null!!");
        return false;
    }

    if (buffer_ent->delay_reset_path)
    {
        buffer_ent->delay_reset_path = false;
        audio_track_restart(buffer_ent->p_cfg->track_handle);
        return false;
    }

#if (TARGET_RTL8753GFE == 1)
    if (
        p_cfg->stream_type == AUDIO_STREAM_TYPE_PLAYBACK
        &&
        media_buffer_get_status(buffer_ent) != MEDIA_BUFFER_PLAYING
        &&
        notification_get_state() != NOTIFICATION_STATE_IDLE
    )
    {
        AUDIO_PRINT_WARN1("playback buffer doesn't write when notification is %u",
                          notification_get_state());
        if (media_buffer_get_status(buffer_ent) != MEDIA_BUFFER_UNINIT)
        {
            buffer_ent->ops.reset_fun(buffer_ent);
        }
        return false;
    }
#endif

    if (buffer_ent->last_seq_updated == false)
    {
        buffer_ent->last_seq = seq;
        buffer_ent->last_ts = timestamp;
        buffer_ent->last_seq_updated = true;
    }
    else
    {
        if (seq == buffer_ent->last_seq)
        {
            AUDIO_PRINT_WARN0("media buffer abort dup seq packet");
            return false;
        }
        else
        {
            if (p_cfg->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                if ((uint16_t)(buffer_ent->last_seq + 1) != seq)
                {
                    dup_flag = seq - (uint16_t)(buffer_ent->last_seq + 1);
                    AUDIO_PRINT_INFO3("lost packet last %u, seq %u dup %u", buffer_ent->last_seq, seq, dup_flag);
                    if (dup_flag >= audio_db->max_plc_count + MAX_PLC_EXC_CNT &&
                        p_cfg->mode != AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY &&
                        p_cfg->mode != AUDIO_STREAM_MODE_DIRECT)
                    {
                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PACKET_LOST, 0, 0);
                        return false;
                    }
                    else
                    {
                        audio_remote_handle_lost_packet(buffer_ent->audio_remote_handle, dup_flag);
                        if (dup_flag > audio_db->max_plc_count)
                        {
                            dup_flag = audio_db->max_plc_count;
                        }
                    }
                }
            }
            else
            {
                if (buffer_ent->last_ts == timestamp && timestamp != 0)
                {
                    AUDIO_PRINT_WARN1("Receive same timestamp sco data 0x%x", timestamp);
                    //audio_mgr_exception(buffer_ent, MEDIA_BUFFER_VOICE_SAME_TS, 0, 0);
                    return 0;
                }
            }
            buffer_ent->last_seq = seq;
            buffer_ent->last_ts = timestamp;
        }
    }

    hdr.avdtp_seq_number = seq;
    hdr.frame_number = frame_num;
    hdr.payload_length = len;
    hdr.pkt_format = p_cfg->format_info.type;
    hdr.bt_clk = timestamp;
    hdr.pkt_status = status;

    if (p_cfg->stream_type == AUDIO_STREAM_TYPE_PLAYBACK &&
        media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
    {
        uint16_t len1 = 0;

        media_buffer_level_get((T_MEDIA_BUFFER_PROXY)buffer_ent->p_cfg, &buffer_level);
        if (p_cfg->mode != AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY &&
            p_cfg->mode != AUDIO_STREAM_MODE_DIRECT)
        {
            while (dup_flag)
            {
                dup_flag--;
                hdr.pkt_status = AUDIO_STREAM_STATUS_LOST;
                buffer_ent->ops.write_fun(buffer_ent, buf, len, &len1, &hdr, BUFFER_DIR_DOWNSTREAM);
            }
        }
    }

    if (buffer_ent->ops.write_fun(buffer_ent, (uint8_t *)buf, len,
                                  written_len, &hdr, BUFFER_DIR_DOWNSTREAM) == false)
    {
        AUDIO_PRINT_TRACE0("media_buffer_write: write failed");
        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_FULL, 0, 0);
        return false;
    }

    if (p_cfg->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        buffer_ent->frame_len = media_buffer_cacu_frame_len(buffer_ent, &p_cfg->format_info, buf);

        AUDIO_PRINT_INFO8("media_buffer_write: role %u mode|state 0x%x frame %u total frames %u seq %u ts 0x%x lat %u plc_cnt %u",
                          remote_session_role_get(), (p_cfg->mode << 8) | buffer_ent->status, frame_num,
                          media_buffer_ds_frame_cnt(buffer_ent),
                          seq, timestamp, p_cfg->latency, p_cfg->plc_count);

        if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING &&
            p_cfg->mode == AUDIO_STREAM_MODE_NORMAL &&
            buffer_level < buffer_ent->p_cfg->upper_threshold_ms)
        {
            media_buffer_level_get((T_MEDIA_BUFFER_PROXY)buffer_ent->p_cfg, &buffer_level);
            if (buffer_level >= buffer_ent->p_cfg->upper_threshold_ms)
            {
                audio_mgr_dispatch(AUDIO_MSG_BUFFER_LEVEL_HIGH, buffer_ent);
            }
        }
    }
    else if (p_cfg->stream_type == AUDIO_STREAM_TYPE_VOICE)
    {
        buffer_ent->frame_len = media_buffer_cacu_frame_len(buffer_ent, &p_cfg->format_info, buf);
        AUDIO_PRINT_INFO5("media_buffer_write: voice role %u state %u total frames %u seq %u ts 0x%x",
                          remote_session_role_get(), buffer_ent->status,
                          buffer_ent->downstream_queue.total_frames_in_buffer, seq, timestamp);
    }

    audio_mgr_dispatch(AUDIO_MSG_BUFFER_WRITE, buffer_ent);

    return true;
}


uint16_t media_buffer_us_read(T_MEDIA_BUFFER_PROXY   proxy,
                              void                  *buf,
                              uint16_t               len,
                              uint32_t              *timestamp,
                              uint16_t              *seq_num,
                              T_AUDIO_STREAM_STATUS *status,
                              uint8_t               *frame_num)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_MEDIA_BUFFER_CFG *p_cfg;
    T_MEDIA_DATA_HDR *p_packet;
    T_H2D_STREAM_HEADER2 *p_h2d_hdr;
    uint16_t read_len;
    uint8_t *p_data;
    uint16_t data_len;

    if (proxy == NULL || buf == NULL)
    {
        return 0;
    }

    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    buffer_ent = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;

    if (buffer_ent->p_cfg == NULL)
    {
        AUDIO_PRINT_ERROR0("media_buffer_us_write: p_cfg is null!!");
        return 0;
    }

    p_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, timestamp, BUFFER_DIR_UPSTREAM);
    if (p_packet == NULL)
    {
        return 0;
    }
    p_h2d_hdr = (T_H2D_STREAM_HEADER2 *)p_packet->p_data;
    p_data = p_h2d_hdr->payload;
    data_len = p_h2d_hdr->payload_length;

    if (len > (data_len - p_packet->read_offset))
    {
        read_len = data_len - p_packet->read_offset;
    }
    else
    {
        read_len = len;
    }
    memcpy(buf, p_data + p_packet->read_offset, read_len);
    *seq_num = p_packet->avdtp_seq_number;
    *timestamp = p_packet->bt_clk;
    *frame_num = p_packet->frame_number;

    buffer_ent->ops.shrink_fun(buffer_ent, p_packet, read_len, BUFFER_DIR_UPSTREAM);
    return read_len;

}

void media_buffer_deactive_proxy(T_MEDIA_BUFFER_PROXY proxy)
{
    T_MEDIA_BUFFER_ENTITY *media_buffer;
    T_MEDIA_BUFFER_CFG *p_cfg;

    if (proxy == NULL)
    {
        return;
    }
    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    media_buffer = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;
    if (media_buffer->p_cfg == p_cfg)
    {
        media_buffer->delay_reset_path = false;
        audio_latency_set_dynamic_latency_on(media_buffer->audio_latency_handle, false);
        media_buffer->p_cfg->plc_count = 0;
        media_buffer->p_cfg->latency = media_buffer->p_cfg->latency_back_up;
        audio_latency_reset(media_buffer->audio_latency_handle);
        media_buffer->ops.reset_fun(media_buffer);
        media_buffer->p_cfg = NULL;
    }
}

void media_buffer_active_proxy(T_MEDIA_BUFFER_PROXY proxy)
{
    T_MEDIA_BUFFER_ENTITY *media_buffer;
    T_MEDIA_BUFFER_CFG *p_cfg;

    if (proxy == NULL)
    {
        return;
    }
    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    media_buffer = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;
    media_buffer->p_cfg = p_cfg;
    media_buffer->ops.reset_fun(media_buffer);
    if (p_cfg->latency_fixed == false &&
        (p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY || p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
       )
    {
        if (media_buffer->audio_interval_eval_handle == NULL)
        {
            media_buffer->audio_interval_eval_handle = audio_interval_evaluator_register(media_buffer,
                                                                                         playback_buffer_interval_eval_evt_cback);
        }
    }
    else
    {
        if (media_buffer->audio_interval_eval_handle)
        {
            audio_interval_evaluator_unregister(media_buffer->audio_interval_eval_handle);
        }
    }
}

bool media_buffer_backup_downstream(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    T_MEDIA_DATA_HDR *p_packet;
    T_H2D_STREAM_HEADER2 *p_h2d_hdr;
    int32_t ret;

    if (buffer_ent->p_cfg == NULL)
    {
        ret = 1;
        goto null_cfg_fail;
    }

    p_packet = (T_MEDIA_DATA_HDR *)buffer_ent->last_packet_ptr;

    if (p_packet == NULL)
    {
        ret = 2;
        goto null_packet_fail;
    }

    p_h2d_hdr = (T_H2D_STREAM_HEADER2 *)(p_packet->p_data);

    p_packet->frame_counter = buffer_ent->ds_frame_counter_last + p_packet->frame_number;
    p_packet->local_seq_number = buffer_ent->ds_local_seq_last + 1;
    p_h2d_hdr->local_seq = buffer_ent->ds_local_seq_last + 1;
    p_h2d_hdr->frame_counter = p_packet->frame_counter;

    if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle, AUDIO_PATH_DATA_TYPE_AUDIO,
                             p_packet->frame_counter
                             , p_packet->p_data, p_packet->payload_length, true))
    {
        buffer_ent->ds_local_seq_last++;
        buffer_ent->ds_frame_counter_last = buffer_ent->ds_frame_counter_last + p_packet->frame_number;
        media_buffer_packet_record_add(buffer_ent->audio_remote_handle, p_packet);

        AUDIO_PRINT_INFO7("media_buffer_backup_downstream: seq %u, local_seq %u, packet_cnt %u, bt_clock 0x%x, frame_couter %u, frame_num %d, codec %u, miss ack %u",
                          p_packet->avdtp_seq_number, p_packet->local_seq_number,
                          media_buffer_ds_pkt_cnt(buffer_ent), p_packet->bt_clk, p_packet->frame_counter,
                          p_packet->frame_number, buffer_ent->miss_ack_cnt);

        return true;
    }
    else
    {
        ret = 3;
        goto path_data_send_fail;
    }

path_data_send_fail:
null_packet_fail:
null_cfg_fail:
    AUDIO_PRINT_WARN1("media_buffer_backup_downstream: warning %d", -ret);
    return false;
}

bool media_buffer_downstream_dsp(T_MEDIA_BUFFER_ENTITY *buffer_ent, bool flush)
{
    T_MEDIA_DATA_HDR *p_packet;
    T_H2D_STREAM_HEADER2 *p_h2d_hdr;
    int32_t ret = 0;

    if (buffer_ent == NULL)
    {
        ret = 1;
        goto null_buffer_fail;
    }

    if (buffer_ent->p_cfg == NULL)
    {
        ret = 2;
        goto null_cfg_fail;
    }

    p_packet = buffer_ent->ops.peek_fun(buffer_ent, 0, NULL, NULL, BUFFER_DIR_DOWNSTREAM);

    if (p_packet == NULL)
    {
        return false;
    }
    p_h2d_hdr = (T_H2D_STREAM_HEADER2 *)(p_packet->p_data);

    if (buffer_ent->buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        T_AUDIO_PATH_DATA_TYPE packet_type = AUDIO_PATH_DATA_TYPE_AUDIO;

        p_packet->frame_counter = buffer_ent->ds_frame_counter_last + p_packet->frame_number;
        p_packet->local_seq_number = buffer_ent->ds_local_seq_last + 1;
        if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY ||
            buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_DIRECT)
        {
            p_h2d_hdr->local_seq = p_packet->avdtp_seq_number;
        }
        else
        {
            p_h2d_hdr->local_seq = buffer_ent->ds_local_seq_last + 1;
        }
        p_h2d_hdr->frame_counter = p_packet->frame_counter;

        if (p_packet->pkt_status == AUDIO_STREAM_STATUS_LOST)
        {
            packet_type = AUDIO_PATH_DATA_TYPE_LOST;
        }
        if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle, packet_type,
                                 p_packet->frame_counter
                                 , p_packet->p_data, p_packet->payload_length, flush))
        {
            uint16_t buffer_level = 0;
            media_buffer_level_get((T_MEDIA_BUFFER_PROXY)buffer_ent->p_cfg, &buffer_level);

            buffer_ent->ds_local_seq_last++;
            buffer_ent->ds_frame_counter_last = buffer_ent->ds_frame_counter_last + p_packet->frame_number;
            media_buffer_packet_record_add(buffer_ent->audio_remote_handle, p_packet);

            AUDIO_PRINT_TRACE7("media_buffer_downstream: seq %u, local_seq %u, packet_cnt %u, bt_clock 0x%x, frame_couter %u, frame_num %d, miss ack %u",
                               p_packet->avdtp_seq_number, p_packet->local_seq_number,
                               media_buffer_ds_pkt_cnt(buffer_ent), p_packet->bt_clk, p_packet->frame_counter,
                               p_packet->frame_number, buffer_ent->miss_ack_cnt);
            buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);

            if (buffer_ent->p_cfg->lower_threshold_ms != 0 &&
                buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_NORMAL &&
                buffer_level > buffer_ent->p_cfg->lower_threshold_ms)
            {
                media_buffer_level_get((T_MEDIA_BUFFER_PROXY)buffer_ent->p_cfg, &buffer_level);
                if (buffer_level <= buffer_ent->p_cfg->lower_threshold_ms)
                {
                    audio_mgr_dispatch(AUDIO_MSG_BUFFER_LEVEL_LOW, buffer_ent);
                }
            }
            return true;
        }
        else
        {
            ret = 4;
            goto a2dp_data_send_fail;
        }
    }
    else if (buffer_ent->buffer_type == AUDIO_STREAM_TYPE_VOICE)
    {
        if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_DIRECT)
        {
            T_AUDIO_PATH_DATA_TYPE data_type;
            p_packet->frame_counter = buffer_ent->ds_frame_counter_last + p_packet->frame_number;
            p_packet->local_seq_number = buffer_ent->ds_local_seq_last + 1;
            p_h2d_hdr->local_seq = p_packet->local_seq_number;
            p_h2d_hdr->frame_counter = p_packet->frame_counter;

            if (p_packet->pkt_status != AUDIO_STREAM_STATUS_CORRECT)
            {
                p_h2d_hdr->status = AUDIO_STREAM_STATUS_LOST;
                data_type = AUDIO_PATH_DATA_TYPE_LOST;
            }
            else
            {
                data_type = AUDIO_PATH_DATA_TYPE_VOICE;
            }

            if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle,
                                     data_type,
                                     p_packet->frame_counter,
                                     p_packet->p_data,
                                     p_packet->payload_length,
                                     flush))
            {
                buffer_ent->ds_local_seq_last++;
                buffer_ent->ds_frame_counter_last = buffer_ent->ds_frame_counter_last + p_packet->frame_number;
                buffer_ent->ds_bt_clk_last = p_packet->bt_clk;
                buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                return true;
            }
            else
            {
                ret = 5;
                goto direct_data_send_fail;
            }
        }
        else
        {
            if (buffer_ent->ds_bt_clk_last != INVALID_BT_CLOCK &&
                buffer_ent->voice_plc_check_interval != 0xff &&
                ((buffer_ent->ds_bt_clk_last + buffer_ent->voice_plc_check_interval) & 0x0fffffff) !=
                p_packet->bt_clk)
            {
                T_MEDIA_DATA_HDR *p_makeup;
                T_H2D_STREAM_HEADER2 *p_h2d_header;

                p_makeup = os_mem_zalloc2(p_packet->payload_length + sizeof(T_MEDIA_DATA_HDR));
                if (p_makeup == NULL)
                {
                    ret = 6;
                    goto makeup_alloc_fail;
                }
                memcpy(p_makeup, p_packet, p_packet->payload_length + sizeof(T_MEDIA_DATA_HDR));
                p_h2d_header = (T_H2D_STREAM_HEADER2 *)p_makeup->p_data;
                p_makeup->frame_counter = buffer_ent->ds_frame_counter_last + p_makeup->frame_number;
                p_makeup->local_seq_number = buffer_ent->ds_local_seq_last + 1;
                p_makeup->bt_clk = (buffer_ent->ds_bt_clk_last + buffer_ent->voice_plc_check_interval) & 0x0fffffff;

                p_h2d_header->local_seq = buffer_ent->ds_local_seq_last + 1;
                p_h2d_header->timestamp = p_makeup->bt_clk;
                p_h2d_header->frame_counter = p_makeup->frame_counter;
                p_h2d_header->status = AUDIO_STREAM_STATUS_LOST;


                if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle, AUDIO_PATH_DATA_TYPE_LOST,
                                         p_makeup->frame_counter
                                         , p_makeup->p_data, p_makeup->payload_length, flush))
                {
                    buffer_ent->ds_local_seq_last++;
                    buffer_ent->ds_frame_counter_last = buffer_ent->ds_frame_counter_last + p_makeup->frame_number;
                    buffer_ent->ds_bt_clk_last = p_makeup->bt_clk;
                    AUDIO_PRINT_WARN6("media_buffer_downstream: voice_PLC clk 0x%x current pkt clk 0x%x local_seq %u, remain cnt %u, frame_couter %u, codec %u",
                                      buffer_ent->ds_bt_clk_last, p_makeup->bt_clk, p_makeup->local_seq_number,
                                      media_buffer_ds_pkt_cnt(buffer_ent),
                                      p_makeup->frame_counter, p_makeup->pkt_format);
                    os_mem_free(p_makeup);
                    return true;
                }
                else
                {
                    os_mem_free(p_makeup);
                    ret = 7;
                    goto voice_data_send_fail;
                }

            }
            else
            {
                T_AUDIO_PATH_DATA_TYPE data_type;

                p_packet->frame_counter = buffer_ent->ds_frame_counter_last + p_packet->frame_number;
                p_packet->local_seq_number = buffer_ent->ds_local_seq_last + 1;

                p_h2d_hdr->local_seq = p_packet->local_seq_number;
                p_h2d_hdr->frame_counter = p_packet->frame_counter;

                if (p_packet->pkt_status != AUDIO_STREAM_STATUS_CORRECT)
                {
                    p_h2d_hdr->status = AUDIO_STREAM_STATUS_LOST;
                    data_type = AUDIO_PATH_DATA_TYPE_LOST;
                }
                else
                {
                    data_type = AUDIO_PATH_DATA_TYPE_VOICE;
                }

                if (audio_path_data_send(buffer_ent->p_cfg->attached_path_handle, data_type,
                                         p_packet->frame_counter
                                         , p_packet->p_data, p_packet->payload_length, flush))
                {
                    buffer_ent->ds_local_seq_last++;
                    buffer_ent->ds_frame_counter_last = buffer_ent->ds_frame_counter_last + p_packet->frame_number;

                    if (buffer_ent->ds_bt_clk_last != INVALID_BT_CLOCK)
                    {
                        if (buffer_ent->voice_plc_check_interval == 0xff)
                        {
                            buffer_ent->voice_plc_check_interval = ((p_packet->bt_clk << 4) - (buffer_ent->ds_bt_clk_last << 4))
                                                                   >> 4;
                            AUDIO_PRINT_TRACE1("media_buffer_downstream: voice check interval updated 0x%x",
                                               buffer_ent->voice_plc_check_interval);
                        }
                    }

                    buffer_ent->ds_bt_clk_last = p_packet->bt_clk;

                    AUDIO_PRINT_TRACE5("media_buffer_downstream: voice local_seq %u, type %u, remain cnt %u, bt_clock 0x%x, frame_couter %u, codec %u",
                                       p_packet->local_seq_number, data_type, media_buffer_ds_pkt_cnt(buffer_ent), p_packet->bt_clk,
                                       p_packet->frame_counter);
                    buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);

                    return true;
                }
                else
                {
                    ret = 8;
                    goto voice_data_send_fail;
                }
            }
        }
    }

voice_data_send_fail:
makeup_alloc_fail:
direct_data_send_fail:
a2dp_data_send_fail:
null_cfg_fail:
null_buffer_fail:
    AUDIO_PRINT_WARN1("media_buffer_downstream: warning %d", -ret);
    return false;
}

bool media_buffer_get_stream_type(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_AUDIO_STREAM_TYPE *type)
{

    if (buffer_ent == NULL || buffer_ent->p_cfg == NULL)
    {
        return false;
    }

    *type = buffer_ent->p_cfg->stream_type;
    return true;
}

bool media_buffer_get_mode(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_AUDIO_STREAM_MODE *mode)
{

    if (buffer_ent == NULL || buffer_ent->p_cfg == NULL)
    {
        return false;
    }

    *mode = buffer_ent->p_cfg->mode;
    return true;
}

bool media_buffer_get_format_type(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_AUDIO_FORMAT_TYPE *type)
{
    if (buffer_ent == NULL || buffer_ent->p_cfg == NULL)
    {
        return false;
    }

    *type = buffer_ent->p_cfg->format_info.type;
    return true;
}

void media_buffer_leading_downstream(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    uint8_t preamble_count;
    if (buffer_ent->buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_SBC)
        {
            preamble_count = SBC_PREAMBL_COUNT;
        }
        else if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_AAC)
        {
            preamble_count = AAC_PREAMBL_COUNT;
        }
        else if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_PCM)
        {
            preamble_count = PCM_PREAMBL_COUNT;
        }
        else if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_LDAC)
        {
            preamble_count = LDAC_PREAMBL_COUNT;
        }
        else if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_LHDC)
        {
            preamble_count = LHDC_PREAMBL_COUNT;
        }
        else if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_MP3)
        {
            preamble_count = MP3_PREAMBL_COUNT;
        }
        else
        {
            preamble_count = DEFAULT_PREAMBL_COUNT;
        }

        for (uint8_t i = preamble_count; i > 0; i--)
        {
            if (media_buffer_downstream_dsp(buffer_ent, true) == false)
            {
                buffer_ent->miss_ack_cnt++;
            }
        }

    }
    else if (buffer_ent->buffer_type == AUDIO_STREAM_TYPE_VOICE)
    {
        uint8_t ds_num;
        ds_num = (media_buffer_ds_pkt_cnt(buffer_ent) - 1) < 3 ? (media_buffer_ds_pkt_cnt(
                                                                      buffer_ent) - 1) : 3;
        for (uint8_t i = 0; i < ds_num; i++)
        {
            media_buffer_downstream_dsp(buffer_ent, false);
        }
        media_buffer_downstream_dsp(buffer_ent, true);
    }
}

void media_buffer_timeout_cback(T_SYS_TIMER_HANDLE handle)
{
    uint32_t timer_id;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;

    timer_id = sys_timer_id_get(handle);
    AUDIO_PRINT_TRACE2("media_buffer_timeout_cback: timer_id 0x%02X handle %p",
                       timer_id, handle);

    buffer_ent = (T_MEDIA_BUFFER_ENTITY *)timer_id;
    if (buffer_ent != NULL)
    {
        sys_timer_delete(buffer_ent->oneway_play_timer);
        buffer_ent->oneway_play_timer = NULL;

        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            if (buffer_ent->p_cfg == NULL)
            {
                return;
            }

            if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PREQUEUE)
            {
                bool instant_play = false;
                uint16_t play_time;

                audio_remote_reset(buffer_ent->audio_remote_handle);

                while (media_buffer_ds_frame_cnt(buffer_ent) > buffer_ent->start_decode_frame_cnt)
                {
                    instant_play = true;
                    buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                }

                if (media_buffer_cacu_playtime(buffer_ent, &play_time))
                {
                    uint32_t bb_clock_slot;
                    uint32_t bb_clock_slot_sync;
                    uint16_t bb_clock_us;
                    T_BT_CLK_REF clk_ref;
                    uint16_t latency = buffer_ent->p_cfg->latency;

                    audio_path_synchronization_join_set(buffer_ent->p_cfg->attached_path_handle, 0);
                    if (instant_play)
                    {
                        media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                        audio_remote_set_state(buffer_ent->audio_remote_handle, AUDIO_REMOTE_STATE_UNSYNC);
                        clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                        audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, clk_ref, 0xffffffff, false);
                    }
                    else
                    {
                        clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                        if (clk_ref != BT_CLK_NONE)
                        {
                            bb_clock_slot_sync = (bb_clock_slot + play_time * 1000 * 2 / 625) & 0x0fffffff;
                            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                            audio_remote_set_state(buffer_ent->audio_remote_handle, AUDIO_REMOTE_STATE_UNSYNC);
                            audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, clk_ref, bb_clock_slot_sync,
                                                     false);
                        }
                        else
                        {
                            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                            audio_remote_set_state(buffer_ent->audio_remote_handle, AUDIO_REMOTE_STATE_UNSYNC);
                            audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, clk_ref, 0xffffffff, false);
                        }
                    }
                    AUDIO_PRINT_TRACE2("media_buffer_set_playing: one way play latency %d %d", latency,
                                       media_buffer_ds_frame_cnt(buffer_ent));

                    media_buffer_leading_downstream(buffer_ent);

                    if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                                     MEDIA_BUFFER_PRI_ONE_WAY_PLAY,
                                                     NULL,
                                                     0,
                                                     false);
                    }
                }
            }
        }
    }
}

void playback_buffer_audio_remote_relay_cback(uint16_t event, void *param,
                                              T_REMOTE_RELAY_STATUS status, void *owner)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)owner;

    AUDIO_PRINT_INFO2("playback_buffer_audio_remote_relay_cback: event 0x%04x status %d", event,
                      status);

    switch (event)
    {
    case MEDIA_BUFFER_REMOTE_LOW_LAT_ADJUST_RESTORE:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT || status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                T_BT_CLK_REF clk_ref;
                uint32_t bb_clock_slot;
                uint32_t bb_clock_rcv = *(uint32_t *)param;
                uint16_t bb_clock_us;

                clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                if (clk_ref != BT_CLK_NONE &&
                    bt_clk_compare(bb_clock_rcv, (bb_clock_slot + 5 * 2 * 1000 / 625) & 0x0fffffff)
                   )
                {
                    jitter_buffer_low_latency_asrc_restore(buffer_ent->jitter_buffer_handle, bb_clock_rcv);
                }
                else
                {
                    audio_mgr_exception(buffer_ent, MEDIA_BUFFER_ASRC_ADJ_OVERTIME, 0, 0);
                }
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_PRI_RESET:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_PRI_RESET_BUFFER, 1, 0);
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_QUERY_PATH_STATE:
        {
            if (buffer_ent->p_cfg == NULL)
            {
                break;
            }

            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (audio_path_is_running(buffer_ent->p_cfg->attached_path_handle))
                {
                    audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                                 MEDIA_BUFFER_REMOTE_QUERY_PATH_STATE_RSP,
                                                 NULL,
                                                 0,
                                                 false);
                }
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_QUERY_PATH_STATE_RSP:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                buffer_ent->sec_path_ready = 1;
                buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                          BUFFER_DIR_DOWNSTREAM);
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_RPT_EXCEPTION:
        {
            T_AUDIO_EXC_CMD *p_cmd;
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                p_cmd = (T_AUDIO_EXC_CMD *)param;
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
                    remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
                {
                    audio_mgr_exception(buffer_ent, p_cmd->exc, 1, p_cmd->param);
                }
            }
        }
        break;

    case MEDIA_BUFFER_PRI_ONE_WAY_PLAY:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
                {
                    audio_track_restart(buffer_ent->p_cfg->track_handle);
                }
                buffer_ent->ops.reset_fun(buffer_ent);
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_AUDIO_SYNCED:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_remote_sync_lock(buffer_ent->audio_remote_handle);
                jitter_buffer_asrc_pid_unblock(buffer_ent->jitter_buffer_handle);
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_AUDIO_SYNCED_SUCC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_remote_sync_lock(buffer_ent->audio_remote_handle);
                jitter_buffer_asrc_pid_unblock(buffer_ent->jitter_buffer_handle);
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_TOUT || status == REMOTE_RELAY_STATUS_SYNC_EXPIRED)
            {
                buffer_ent->ignore_latency_report = 0;
                audio_path_synchronization_join_stop();
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_LOW_LATENCY_OVERRIDE:
        {
            T_AUDIO_MSG_LOW_LATENCY_OVERRIDE *p_cmd = (T_AUDIO_MSG_LOW_LATENCY_OVERRIDE *)param;

            if (buffer_ent->p_cfg == NULL)
            {
                break;
            }
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_latency_set_dynamic_latency_on(buffer_ent->audio_latency_handle,
                                                     p_cmd->dynamic_latency_on);
                buffer_ent->p_cfg->plc_count = p_cmd->plc_count;
                buffer_ent->p_cfg->latency = p_cmd->latency;
                buffer_ent->p_cfg->latency_override = p_cmd->latency_override;
                buffer_ent->p_cfg->latency_back_up = p_cmd->latency_backup;
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_RESUME:
        {
            if (buffer_ent->p_cfg == NULL)
            {
                break;
            }

            audio_latency_resume(buffer_ent->audio_latency_handle);
        }
        break;

    case MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_SET:
        {
            T_DYNAMIC_LAT_SET *p_cmd = (T_DYNAMIC_LAT_SET *)param;

            if (buffer_ent->p_cfg == NULL)
            {
                return;
            }

            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_latency_dynamic_set(buffer_ent->audio_latency_handle, p_cmd->target_lat_ms,
                                          p_cmd->auto_resume);
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_INC_REQ:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    T_DYNAMIC_LAT_INC_DEC *p_cmd = (T_DYNAMIC_LAT_INC_DEC *)param;

                    if (buffer_ent->p_cfg == NULL)
                    {
                        return;
                    }
                    if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
                        buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                    {
                        if (buffer_ent->p_cfg->latency_fixed == false &&
                            media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
                        {
                            if (buffer_ent->p_cfg->plc_count < p_cmd->previous_total_plc_count + p_cmd->plc_count)
                            {
                                T_DYNAMIC_LAT_INC_DEC cmd;
                                cmd.previous_total_plc_count = buffer_ent->p_cfg->plc_count;
                                cmd.plc_count = p_cmd->previous_total_plc_count + p_cmd->plc_count - buffer_ent->p_cfg->plc_count;
                                cmd.auto_dec = true;
                                if (audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                                                 MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_INC,
                                                                 &cmd,
                                                                 sizeof(T_DYNAMIC_LAT_INC_DEC),
                                                                 false) == false)
                                {
                                    audio_latency_low_lat_dynamic_increase(buffer_ent->audio_latency_handle, cmd.plc_count,
                                                                           cmd.auto_dec);
                                }
                            }
                        }
                    }
                }
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_INC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                T_DYNAMIC_LAT_INC_DEC *p_cmd = (T_DYNAMIC_LAT_INC_DEC *)param;

                if (buffer_ent->p_cfg == NULL)
                {
                    return;
                }
                if (buffer_ent->p_cfg->latency_fixed == false)
                {
                    buffer_ent->p_cfg->plc_count = p_cmd->previous_total_plc_count;
                    audio_latency_low_lat_dynamic_increase(buffer_ent->audio_latency_handle, p_cmd->plc_count,
                                                           p_cmd->auto_dec);
                }
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_DEC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {

                T_DYNAMIC_LAT_INC_DEC *p_cmd = (T_DYNAMIC_LAT_INC_DEC *)param;

                if (buffer_ent->p_cfg == NULL)
                {
                    return;
                }
                if (buffer_ent->p_cfg->latency_fixed == false)
                {
                    buffer_ent->p_cfg->plc_count = p_cmd->previous_total_plc_count;
                    audio_latency_low_lat_dynamic_decrease(buffer_ent->audio_latency_handle, p_cmd->plc_count);
                }
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_ASRC_SYNC_ADJUST:
        {
            T_MEDIA_BUFFER_ASRC_SYNC_ADJUST *p_msg = (T_MEDIA_BUFFER_ASRC_SYNC_ADJUST *)param;
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT || status == REMOTE_RELAY_STATUS_SYNC_EXPIRED)
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
                {
                    T_BT_CLK_REF clk_ref;
                    uint32_t bb_clock_slot;
                    uint32_t bb_clock_rcv;
                    uint16_t bb_clock_us;

                    bb_clock_rcv = p_msg->sync_clk;
                    clk_ref = bt_piconet_clk_get((T_BT_CLK_REF)(p_msg->clk_ref), &bb_clock_slot, &bb_clock_us);

                    if (p_msg->event != JITTER_BUFFER_EVT_ASRC_PID_ADJ_CANCEL)
                    {
                        if (jitter_buffer_asrc_pid_is_block(buffer_ent->jitter_buffer_handle))
                        {
                            AUDIO_PRINT_WARN0("jitter buffer adjust blocks");
                            break;
                        }
                    }

                    if (p_msg->tid != buffer_ent->media_buffer_fst_sync_tid)
                    {
                        AUDIO_PRINT_WARN2("jitter buffer adjust error tid %d %d", p_msg->tid,
                                          buffer_ent->media_buffer_fst_sync_tid);
                        break;
                    }

                    if (clk_ref != BT_CLK_NONE
                        &&
                        bt_clk_compare(bb_clock_rcv, (bb_clock_slot + 2 * 2 * 1000 / 625) &
                                       0x0fffffff)
                       )
                    {
                        jitter_buffer_asrc_ratio_adjust(bb_clock_rcv, p_msg->final_asrc);
                    }
                    else
                    {
                        AUDIO_PRINT_WARN4("JB_clk_err: clk ref %u sync clk %u local %u + margin %u", clk_ref, bb_clock_rcv,
                                          bb_clock_slot, 2 * 2 * 1000 / 625);
                        if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
                            remote_session_state_get() != REMOTE_SESSION_STATE_DISCONNECTED)
                        {
                            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_JBC_CLK_ERR, 0, 0);
                        }

                    }
                }
                else if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_WAIT_TO_PLAY ||
                         audio_remote_get_state(buffer_ent->audio_remote_handle) == AUDIO_REMOTE_STATE_SEAMLESS_JOIN)
                {
                    audio_mgr_exception(buffer_ent, MEDIA_BUFFER_JBC_CLK_ERR, 0, 0);
                }
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED)
            {
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_ASRC_ADJ_OVERTIME, 0, 0);
            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_LOW_LAT_ADJUST:
        {
            T_JITTER_BUFFER_EVT_LOW_LAT_ADJ *p_msg = (T_JITTER_BUFFER_EVT_LOW_LAT_ADJ *)param;

            if (status == REMOTE_RELAY_STATUS_SYNC_RCVD)
            {
                AUDIO_PRINT_TRACE5("gaming mode asrc: type %d restore clk 0x%x sync clk 0x%x acc %d dec_num %d",
                                   p_msg->policy, p_msg->restore_clk,
                                   p_msg->sync_adj_clk, p_msg->accelerate, p_msg->dec_frame_num);
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_TOUT || status == REMOTE_RELAY_STATUS_SYNC_EXPIRED)
            {
                T_BT_CLK_REF clk_ref;
                uint32_t bb_clock_slot;
                uint32_t bb_clock_rcv;
                uint16_t bb_clock_us;

                if (
                    media_buffer_get_status(buffer_ent) != MEDIA_BUFFER_PLAYING ||
                    (buffer_ent->p_cfg->mode != AUDIO_STREAM_MODE_LOW_LATENCY &&
                     buffer_ent->p_cfg->mode != AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                )
                {
                    break;
                }

                if (p_msg->policy == LATENCY_KEEPER_POLICY_BY_ASRC ||
                    p_msg->policy == LATENCY_KEEPER_POLICY_BY_BOTH)
                {
                    bb_clock_rcv = p_msg->sync_adj_clk;
                    clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);

                    if (clk_ref != BT_CLK_NONE
                        &&
                        bt_clk_compare(bb_clock_rcv, (bb_clock_slot + 5 * 2 * 1000 / 625) &
                                       0x0fffffff)
                       )
                    {
                        uint32_t restore_us;

                        restore_us = (((p_msg->restore_clk << 4) - (bb_clock_slot << 4)) >> 4) * 625 / 2;

                        if (restore_us < 100000)
                        {
                            AUDIO_PRINT_WARN4("JB_clk_err2: gaming mode asrc clk ref %u sync clk %u local %u + margin %u",
                                              clk_ref, bb_clock_rcv,
                                              bb_clock_slot, 5 * 2 * 1000 / 625);
                            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_ASRC_ADJ_OVERTIME, 0, 0);
                            break;
                        }
                        jitter_buffer_low_latency_adjust_latency(buffer_ent->jitter_buffer_handle, p_msg);
                    }
                    else
                    {
                        AUDIO_PRINT_WARN4("JB_clk_err: gaming mode asrc clk ref %u sync clk %u local %u + margin %u",
                                          clk_ref, bb_clock_rcv,
                                          bb_clock_slot, 5 * 2 * 1000 / 625);
                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_ASRC_ADJ_OVERTIME, 0, 0);
                    }
                }
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED)
            {
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_JBC_CLK_ERR, 0, 0);

            }
        }
        break;

    case MEDIA_BUFFER_REMOTE_ULTRA_LOW_LATENCY_PLC_REQ:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                T_MEDIA_DATA_HDR *p_media_packet;

                if (media_buffer_backup_downstream(buffer_ent))
                {
                    p_media_packet = buffer_ent->last_packet_ptr;
                    audio_latency_ultra_low_latency_plc_req(buffer_ent->audio_latency_handle,
                                                            p_media_packet->frame_number);
                    buffer_ent->buffer_sync_trigger_cnt = 0;
                    audio_remote_buffer_sync_req(buffer_ent->audio_remote_handle);
                }
            }
        }
        break;

    default:
        break;
    }

}

void playback_buffer_audio_remote_evt_cback(T_AUDIO_REMOTE_EVT event, void *param, void *owner)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)owner;

    if (event != AUDIO_REMOTE_EVENT_BUFFER_SYNC)
    {
        AUDIO_PRINT_INFO1("playback_buffer_audio_remote_evt_cback: event 0x%04x", event);
    }

    switch (event)
    {
    case AUDIO_REMOTE_EVENT_BUFFER_SYNC:
        {
            bool rec_found = *(bool *)param;
            if (rec_found)
            {
                if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
                    buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                {
                    if (audio_remote_is_buffer_unsync(buffer_ent->audio_remote_handle))
                    {
                        audio_remote_buffer_PLC(buffer_ent->audio_remote_handle);
                    }
                    media_buffer_downstream_dsp(buffer_ent, true);
                }
                else
                {
                    media_buffer_check_miss_ack(buffer_ent);
                }
            }
            else
            {
                if (audio_remote_db->force_join == false)
                {
                    audio_mgr_exception(buffer_ent, MEDIA_BUFFER_SYNC_ERR, 0, 0);
                }
            }
        }
        break;

    case AUDIO_REMOTE_EVENT_ALIGNED:
        {
            if (media_buffer_ds_pkt_cnt(buffer_ent) == 0)
            {
                AUDIO_PRINT_ERROR0("audio_buffer_prequeue_state: no data, unknow fatal error");
                break;
            }
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                uint16_t play_time;
                if (media_buffer_cacu_playtime(buffer_ent, &play_time))
                {
                    bool dynamic_latency_on = false;
                    audio_latency_get_dynamic_latency_on(buffer_ent->audio_latency_handle,
                                                         &dynamic_latency_on);
                    audio_mgr_appoint_playtime(buffer_ent->audio_remote_handle, play_time, dynamic_latency_on);
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                }
            }
            else if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                //sys_timer_stop(&audio_db->one_way_play_a2dp_timer_handle);
            }
        }
        break;

    case AUDIO_REMOTE_EVENT_SYNC_PLAY_START:
        {
            if (param != NULL)
            {
                T_AUDIO_REMOTE_LATENCY_INFO *msg = (T_AUDIO_REMOTE_LATENCY_INFO *) param;

                audio_latency_set_dynamic_latency_on(buffer_ent->audio_latency_handle,
                                                     msg->dynamic_latency_on);
                buffer_ent->p_cfg->plc_count = msg->plc_count;
                buffer_ent->p_cfg->latency = msg->latency;
                buffer_ent->p_cfg->latency_override = msg->latency_override;
                buffer_ent->p_cfg->latency_back_up = msg->latency_back_up;
            }

            if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY)
            {
                for (uint8_t i = media_buffer_ds_pkt_cnt(buffer_ent) ; i > 0; i--)
                {
                    media_buffer_downstream_dsp(buffer_ent, true);
                }
            }
            else if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
            {
                media_buffer_downstream_dsp(buffer_ent, true);
            }
            else
            {
                media_buffer_leading_downstream(buffer_ent);
            }

            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
            jitter_buffer_asrc_pid_block(buffer_ent->jitter_buffer_handle);//wait for slave_sync flag
            audio_mgr_dispatch(AUDIO_MSG_BUFFER_STATE_PLAYING, NULL);

        }
        break;

    case AUDIO_REMOTE_EVENT_SYNC_PLAY_RSV:
        {
            uint8_t tid = *(uint8_t *)param;

            if (buffer_ent->media_buffer_fst_sync_tid == tid)
            {
                if (buffer_ent->oneway_play_timer != NULL)
                {
                    sys_timer_delete(buffer_ent->oneway_play_timer);
                    buffer_ent->oneway_play_timer = NULL;
                }
                media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
            }
        }
        break;

    default:
        break;
    }
}

void playback_buffer_lat_evt_cback(T_AUDIO_LATENCY_EVT event, void *param, void *owner)
{
    AUDIO_PRINT_TRACE1("playback_buffer_lat_evt_cback: event 0x%04x", event);
    T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)owner;

    switch (event)
    {
    case AUDIO_LATENCY_EVT_ULTRA_LOW_LAT_PLC_REQ:
        {
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                             MEDIA_BUFFER_REMOTE_ULTRA_LOW_LATENCY_PLC_REQ,
                                             NULL,
                                             0,
                                             false);
            }
            else
            {
                T_MEDIA_DATA_HDR *p_media_packet;

                if (media_buffer_backup_downstream(buffer_ent))
                {
                    p_media_packet = buffer_ent->last_packet_ptr;
                    audio_latency_ultra_low_latency_plc_req(buffer_ent->audio_latency_handle,
                                                            p_media_packet->frame_number);
                    buffer_ent->buffer_sync_trigger_cnt = 0;
                    audio_remote_buffer_sync_req(buffer_ent->audio_remote_handle);
                }
            }
        }
        break;

    case AUDIO_LATENCY_EVT_PLC_FIX:
        {
            uint8_t plc_frame_num = *(uint8_t *)param;
            if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
            {
                audio_remote_set_frame_diff(buffer_ent->audio_remote_handle,
                                            audio_remote_get_frame_diff(buffer_ent->audio_remote_handle) - plc_frame_num);
                audio_remote_buffer_PLC(buffer_ent->audio_remote_handle);
            }
        }
        break;

    case AUDIO_LATENCY_EVT_MAX_LAT_PLC:
        {
            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_DSP_PLC_TOO_MUCH, 0, 0);
        }
        break;

    case AUDIO_LATENCY_EVT_LOW_LAT_DECREASE_REQ:
        {
            T_DYNAMIC_LAT_INC_DEC cmd;
            uint8_t plc_count = *(int8_t *)param;

            cmd.previous_total_plc_count = buffer_ent->p_cfg->plc_count;
            cmd.plc_count = plc_count;
            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                                 MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_DEC,
                                                 &cmd,
                                                 sizeof(T_DYNAMIC_LAT_INC_DEC),
                                                 false) == false)
                {
                    audio_latency_low_lat_dynamic_decrease(buffer_ent->audio_latency_handle, plc_count);
                }
            }
        }
        break;

    case AUDIO_LATENCY_EVT_LOW_LAT_INCREASE_REQ:
        {
            T_DYNAMIC_LAT_INC_DEC cmd;
            uint8_t plc_count = *(int8_t *)param;

            cmd.previous_total_plc_count = buffer_ent->p_cfg->plc_count;
            cmd.plc_count = plc_count;
            cmd.auto_dec = false;
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                                 MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_INC,
                                                 &cmd,
                                                 sizeof(T_DYNAMIC_LAT_INC_DEC),
                                                 false) == false)
                {
                    audio_latency_low_lat_dynamic_increase(buffer_ent->audio_latency_handle, plc_count, cmd.auto_dec);
                }
            }
            else
            {
                audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                             MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_INC_REQ,
                                             &cmd,
                                             sizeof(T_DYNAMIC_LAT_INC_DEC),
                                             false);
            }
        }
        break;

    case AUDIO_LATENCY_EVT_LAT_DECREASED:
        {
            uint8_t plc_frame_num = *(uint8_t *)param;
            if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    buffer_ent->ignore_latency_report++;
                }
                audio_remote_set_frame_diff(buffer_ent->audio_remote_handle,
                                            audio_remote_get_frame_diff(buffer_ent->audio_remote_handle) - plc_frame_num);
                audio_remote_buffer_PLC(buffer_ent->audio_remote_handle);
            }
        }

    case AUDIO_LATENCY_EVT_LAT_INCREASED:
        {
            if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    buffer_ent->ignore_latency_report++;
                    buffer_ent->buffer_sync_trigger_cnt = 0;
                    audio_remote_buffer_sync_req(buffer_ent->audio_remote_handle);
                }
            }
            else
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    buffer_ent->start_decode_frame_cnt = media_buffer_cacu_frame_num(buffer_ent->p_cfg->latency,
                                                                                     &(buffer_ent->p_cfg->format_info));
                }
                else
                {
                    buffer_ent->ops.reset_fun(buffer_ent);
                }
            }
        }
        break;

    default:
        break;
    }
}

void playback_buffer_interval_eval_evt_cback(T_AUDIO_INTERVAL_EVAL_EVT event, void *param,
                                             void *owner)
{
    AUDIO_PRINT_TRACE1("playback_buffer_interval_eval_evt_cback: event 0x%04x", event);
    T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)owner;

    switch (event)
    {
    case AUDIO_INTERVAL_EVAL_EVT_UNSTABLE:
        {
            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                audio_latency_handle_inverval_unstable_evt(buffer_ent->audio_latency_handle);
            }
        }
        break;

    case AUDIO_INTERVAL_EVAL_EVT_STABLE:
        {
            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (buffer_ent->p_cfg != NULL)
                {
                    audio_latency_handle_inverval_stable_evt(buffer_ent->audio_latency_handle,
                                                             buffer_ent->p_cfg->format_info.type);
                }
            }
        }
        break;

    default:
        break;
    }
}

void playback_buffer_jb_evt_cback(T_JITTER_BUFFER_EVT event, void *param, void *owner)
{
    AUDIO_PRINT_TRACE1("playback_buffer_jb_evt_cback: event 0x%04x", event);
    T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)owner;

    switch (event)
    {
    case JITTER_BUFFER_EVT_LOW_LAT_ASRC_RESTORE_CANCEL:
        {
            T_BT_CLK_REF clk_ref;
            uint32_t bb_clock_slot;
            uint16_t bb_clock_us;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                break;
            }

            if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
                buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
                {
                    clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);

                    if (clk_ref == BT_CLK_NONE)
                    {
                        break;
                    }

                    bb_clock_slot += (250 * 2 * 1000 / 625); // ms to bt clk
                    bb_clock_slot &= 0x0fffffff;

                    if (audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                                     MEDIA_BUFFER_REMOTE_LOW_LAT_ADJUST_RESTORE,
                                                     &bb_clock_slot,
                                                     sizeof(uint32_t),
                                                     false) == false)
                    {
                        jitter_buffer_low_latency_asrc_restore(buffer_ent->jitter_buffer_handle, 0xffffffff);
                    }
                }
            }
        }
        break;

    case JITTER_BUFFER_EVT_LOW_LAT_ASRC_RESTORE_REQ:
        {
            uint32_t restore_clk = *(uint32_t *)param;
            T_BT_CLK_REF clk_ref;
            uint32_t bb_clock_slot;
            uint16_t bb_clock_us;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                break;
            }

            if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
                buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
                {
                    clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                    if (clk_ref != BT_CLK_NONE
                        &&
                        bt_clk_compare(restore_clk, (bb_clock_slot + 5 * 2 * 1000 / 625) &
                                       0x0fffffff)
                       )
                    {
                        if (audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                                         MEDIA_BUFFER_REMOTE_LOW_LAT_ADJUST_RESTORE,
                                                         &restore_clk,
                                                         sizeof(uint32_t),
                                                         false) == false)
                        {
                            jitter_buffer_low_latency_asrc_restore(buffer_ent->jitter_buffer_handle, 0xffffffff);
                        }
                    }
                    else
                    {
                        AUDIO_PRINT_WARN3("asrc_restore_error: gaming mode asrc clk ref %u sync clk %u local %u",
                                          clk_ref, restore_clk, bb_clock_slot);
                        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_ASRC_ADJ_OVERTIME, 0, 0);
                    }
                }
            }
        }
        break;

    case JITTER_BUFFER_EVT_LAT_ADJ_PLC_REQ:
        {
            uint8_t dec_frame_num = *(uint8_t *)param;
            audio_remote_set_frame_diff(buffer_ent->audio_remote_handle,
                                        audio_remote_get_frame_diff(buffer_ent->audio_remote_handle) - dec_frame_num);
            audio_remote_buffer_PLC(buffer_ent->audio_remote_handle);
        }
        break;

    case JITTER_BUFFER_EVT_ASRC_ADJ_REQ:
        {
            T_JITTER_BUFFER_EVT_LOW_LAT_ADJ_REQ *msg = (T_JITTER_BUFFER_EVT_LOW_LAT_ADJ_REQ *)param;
            T_JITTER_BUFFER_EVT_LOW_LAT_ADJ cmd;
            T_BT_CLK_REF clk_ref;
            uint32_t bb_clock_slot;
            uint16_t bb_clock_us;
            uint32_t duration = 0;

            cmd.policy = audio_db->latency_keeper_policy;
            cmd.dec_frame_num = 0;
            cmd.restore_clk = 0;
            cmd.sync_adj_clk = 0;
            cmd.duration = 0;
            cmd.accelerate = msg->accelerate;

            if (audio_db->latency_keeper_policy == LATENCY_KEEPER_POLICY_BY_ASRC)
            {
                clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);

                if (clk_ref == BT_CLK_NONE)
                {
                    break;
                }

                bb_clock_slot += (250 * 2 * 1000 / 625); // ms to bt clk
                bb_clock_slot &= 0x0fffffff;

                cmd.sync_adj_clk = bb_clock_slot;

                cmd.ratio = msg->diff_ms * 50;
                if (cmd.ratio > LOW_LATENCY_ASRC_ADJ_SPEED_PPM_LIM)
                {
                    cmd.ratio = LOW_LATENCY_ASRC_ADJ_SPEED_PPM_LIM;
                }

                duration = msg->diff_ms * 1000 / cmd.ratio * 1000;
                if (duration > LOW_LATENCY_ASRC_ADJ_DURATION_LIM)
                {
                    duration = LOW_LATENCY_ASRC_ADJ_DURATION_LIM;
                }
                bb_clock_slot += (duration * 2 * 1000 / 625);
                bb_clock_slot &= 0x0fffffff;
                cmd.restore_clk = bb_clock_slot;
                cmd.duration = duration;

            }
            else if (audio_db->latency_keeper_policy == LATENCY_KEEPER_POLICY_BY_PLC)
            {
                if (msg->accelerate)
                {
                    uint16_t frame_duration = media_buffer_cacu_frames_duration(1, &buffer_ent->p_cfg->format_info);
                    if (frame_duration != 0)
                    {
                        cmd.dec_frame_num = msg->diff_ms / frame_duration;
                    }
                    if (cmd.dec_frame_num == 0)
                    {
                        break;
                    }
                }
            }
            else
            {
                uint16_t frame_duration = media_buffer_cacu_frames_duration(1, &buffer_ent->p_cfg->format_info);
                uint16_t diff_ms;

                if (frame_duration != 0)
                {
                    if (msg->accelerate)
                    {
                        if (msg->diff_ms > 20)
                        {
                            cmd.dec_frame_num = (msg->diff_ms - 20) / frame_duration;
                        }
                    }
                }

                diff_ms = msg->diff_ms - cmd.dec_frame_num * frame_duration;

                clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);

                if (clk_ref == BT_CLK_NONE)
                {
                    return;
                }

                bb_clock_slot += (250 * 2 * 1000 / 625); // ms to bt clk
                bb_clock_slot &= 0x0fffffff;

                cmd.sync_adj_clk = bb_clock_slot;

                cmd.ratio = diff_ms * 50;
                if (cmd.ratio > LOW_LATENCY_ASRC_ADJ_SPEED_PPM_LIM)
                {
                    cmd.ratio = LOW_LATENCY_ASRC_ADJ_SPEED_PPM_LIM;
                }

                duration = diff_ms * 1000 / cmd.ratio * 1000;
                if (duration > LOW_LATENCY_ASRC_ADJ_DURATION_LIM)
                {
                    duration = LOW_LATENCY_ASRC_ADJ_DURATION_LIM;
                }
                bb_clock_slot += (duration * 2 * 1000 / 625);
                bb_clock_slot &= 0x0fffffff;
                cmd.restore_clk = bb_clock_slot;
                cmd.duration = duration;

            }
            AUDIO_PRINT_TRACE7("gaming mode asrc: policy %d diff %d restore clk 0x%x sync clk 0x%x duration %d acc %d dec_num %d",
                               audio_db->latency_keeper_policy, msg->diff_ms, cmd.restore_clk, cmd.sync_adj_clk,
                               duration, msg->accelerate, cmd.dec_frame_num);


            if (audio_remote_sync_msg_relay(buffer_ent->audio_remote_handle,
                                            MEDIA_BUFFER_REMOTE_LOW_LAT_ADJUST,
                                            &cmd,
                                            sizeof(T_JITTER_BUFFER_EVT_LOW_LAT_ADJ),
                                            REMOTE_TIMER_HIGH_PRECISION,
                                            200,
                                            false) == false)
            {
                jitter_buffer_low_latency_adjust_latency(buffer_ent->jitter_buffer_handle, &cmd);
            }
        }
        break;

    case JITTER_BUFFER_EVT_ASRC_PID_ADJ_OPEN:
    case JITTER_BUFFER_EVT_ASRC_PID_ADJ_CLOSE:
    case JITTER_BUFFER_EVT_ASRC_PID_ADJ_CANCEL:
    case JITTER_BUFFER_EVT_ASRC_PID_ADJ:
    case JITTER_BUFFER_EVT_ASRC_LONGTERM_ADJ_REQ:
        {
            T_JITTER_BUFFER_ASRC_ADJ *msg = (T_JITTER_BUFFER_ASRC_ADJ *)param;
            T_MEDIA_BUFFER_ASRC_SYNC_ADJUST cmd;
            T_BT_CLK_REF clk_ref;
            uint32_t bb_clock_slot;
            uint16_t bb_clock_us;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                break;
            }

            buffer_ent->ignore_latency_report++;

            if (remote_session_state_get() != REMOTE_SESSION_STATE_CONNECTED)
            {
                jitter_buffer_asrc_ratio_adjust(0xffffffff, msg->asrc_ppm);
                break;
            }
            if (audio_remote_get_state(buffer_ent->audio_remote_handle) != AUDIO_REMOTE_STATE_SYNCED)
            {
                jitter_buffer_asrc_ratio_adjust(0xffffffff, msg->asrc_ppm);
                break;
            }

            clk_ref = bt_piconet_clk_get(BT_CLK_NONE, &bb_clock_slot, &bb_clock_us);

            if (clk_ref == BT_CLK_NONE)
            {
                bb_clock_slot = 0xffffffff;
            }
            else
            {
                bb_clock_slot += (250 * 2 * 1000 / 625); // ms to bt clk
                bb_clock_slot &= 0x0fffffff;
            }

            cmd.sync_clk = bb_clock_slot;
            cmd.clk_ref = clk_ref;
            cmd.final_asrc = msg->asrc_ppm;
            cmd.tid = buffer_ent->media_buffer_fst_sync_tid;
            cmd.event = event;
            if (audio_remote_sync_msg_relay(buffer_ent->audio_remote_handle,
                                            MEDIA_BUFFER_REMOTE_ASRC_SYNC_ADJUST,
                                            &cmd,
                                            sizeof(T_MEDIA_BUFFER_ASRC_SYNC_ADJUST),
                                            REMOTE_TIMER_HIGH_PRECISION,
                                            150,
                                            false) == false)
            {
                jitter_buffer_asrc_ratio_adjust(0xffffffff, msg->asrc_ppm);
            }
        }
        break;

    default:
        break;
    }
}

void voice_buffer_audio_remote_relay_cback(uint16_t event, void *param,
                                           T_REMOTE_RELAY_STATUS status, void *owner)
{
    AUDIO_PRINT_TRACE2("voice_buffer_audio_remote_relay_cback: event 0x%04x status %d", event, status);
    T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)owner;

    switch (event)
    {
    case MEDIA_BUFFER_REMOTE_RPT_EXCEPTION:
        {
            T_AUDIO_EXC_CMD *p_cmd;
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                p_cmd = (T_AUDIO_EXC_CMD *)param;
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
                    remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
                {
                    audio_mgr_exception(buffer_ent, p_cmd->exc, 1, p_cmd->param);
                }
            }
        }
        break;

    case MEDIA_BUFFER_PRI_ONE_WAY_PLAY:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
                {
                    audio_track_restart(buffer_ent->p_cfg->track_handle);
                }
                buffer_ent->ops.reset_fun(buffer_ent);
            }
        }
        break;

    default:
        break;
    }

}

void voice_buffer_audio_remote_evt_cback(T_AUDIO_REMOTE_EVT event, void *param, void *owner)
{
    AUDIO_PRINT_TRACE1("voice_buffer_audio_remote_evt_cback: event 0x%04x", event);
    T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)owner;

    switch (event)
    {
    case AUDIO_REMOTE_EVENT_ALIGNED:
        {
            if (media_buffer_ds_pkt_cnt(buffer_ent) == 0)
            {
                AUDIO_PRINT_ERROR0("audio_buffer_prequeue_state: no data, unknow fatal error");
                break;
            }
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                uint16_t play_time;
                if (media_buffer_cacu_playtime(buffer_ent, &play_time))
                {
                    bool dynamic_latency_on = false;
                    audio_latency_get_dynamic_latency_on(buffer_ent->audio_latency_handle,
                                                         &dynamic_latency_on);
                    audio_mgr_appoint_playtime(buffer_ent->audio_remote_handle, play_time, dynamic_latency_on);
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                }
            }
            else if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                //sys_timer_stop(&audio_db->one_way_play_a2dp_timer_handle);
            }
        }
        break;

    case AUDIO_REMOTE_EVENT_SYNC_PLAY_START:
        {
            media_buffer_leading_downstream(buffer_ent);
            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
        }
        break;

    case AUDIO_REMOTE_EVENT_SYNC_PLAY_RSV:
        {
            uint8_t tid = *(uint8_t *)param;

            if (buffer_ent->media_buffer_fst_sync_tid == tid)
            {
                if (buffer_ent->oneway_play_timer != NULL)
                {
                    sys_timer_delete(buffer_ent->oneway_play_timer);
                    buffer_ent->oneway_play_timer = NULL;
                }
                media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
            }
        }
        break;

    default:
        break;
    }
}

uint16_t media_buffer_get_ds_watermark(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    return buffer_ent->downstream_queue.total_block_bytes_in_buffer;
}

uint16_t media_buffer_get_us_watermark(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    return buffer_ent->upstream_queue.total_block_bytes_in_buffer;
}

void media_buffer_latency_report_set_peroid(T_MEDIA_BUFFER_PROXY proxy, bool enable,
                                            uint16_t peroid_ms)
{
    T_MEDIA_BUFFER_CFG *p_cfg;
    uint16_t count;

    if (proxy == NULL)
    {
        return;
    }
    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;

    count = media_buffer_cacu_frame_num(peroid_ms, &(p_cfg->format_info));
    if (p_cfg->format_info.type == AUDIO_FORMAT_TYPE_SBC)
    {
        if (p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
        {
            count = count / 2;
        }
        else
        {
            count = count / 5;
        }
    }

    audio_path_latency_rpt_set(enable, count);
}

void media_buffer_low_latency_config(T_MEDIA_BUFFER_PROXY proxy)
{
    T_MEDIA_BUFFER_CFG *p_cfg;

    if (proxy == NULL)
    {
        return;
    }
    p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;

    if (p_cfg->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        if (p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY)
        {
            if (p_cfg->format_info.type == AUDIO_FORMAT_TYPE_PCM)
            {
                audio_path_low_latency_set(false, true);
                media_buffer_latency_report_set_peroid(proxy, true, LATENCY_REPORT_PERIOD_MS);
            }
            else
            {
                audio_path_low_latency_set(true, true);
                if (audio_db->latency_keeper_enable)
                {
                    media_buffer_latency_report_set_peroid(proxy, true, LATENCY_REPORT_PERIOD_MS);
                }
            }
        }
        else if (p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
        {
            if (p_cfg->format_info.type == AUDIO_FORMAT_TYPE_PCM)
            {
                audio_path_low_latency_set(false, true);
            }
            else
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_SINGLE)
                {
                    audio_path_low_latency_set(false, true);
                }
                else
                {
                    audio_path_low_latency_set(true, true);
                }
                media_buffer_latency_report_set_peroid(proxy, true, LATENCY_REPORT_PERIOD_MS);
            }
        }
        else
        {
            media_buffer_latency_report_set_peroid(proxy, true, LATENCY_REPORT_PERIOD_MS);
        }
    }

}

bool media_buffer_cacu_playtime(T_MEDIA_BUFFER_ENTITY *buffer_ent, uint16_t *playtime_ms)
{
    uint16_t play_time_ms;
    uint16_t total_frames;

    if (buffer_ent->p_cfg == NULL)
    {
        return false;
    }

    if (buffer_ent->p_cfg->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        total_frames = media_buffer_ds_frame_cnt(buffer_ent);
        if (total_frames < buffer_ent->start_decode_frame_cnt)
        {
            uint16_t diff = buffer_ent->start_decode_frame_cnt - total_frames;
            play_time_ms = diff * buffer_ent->p_cfg->latency / buffer_ent->start_decode_frame_cnt ;
            if (play_time_ms <= 69)
            {
                AUDIO_PRINT_WARN0("media_buffer_cacu_playtime: total_frames > start_decode_frame_count");
                *playtime_ms = 69;
                return true;
            }
            else
            {
                AUDIO_PRINT_INFO4("media_buffer_cacu_playtime: start frames %u, total frames %u, lantency %u, play time %u ms",
                                  buffer_ent->start_decode_frame_cnt, total_frames, buffer_ent->p_cfg->latency, play_time_ms);
                *playtime_ms = play_time_ms;
                return true;
            }
        }
        else
        {
            AUDIO_PRINT_WARN0("audio_mgr_cacu_playtime2: total_frames > start_decode_frame_count");
            *playtime_ms = 69;
            return true;
        }
    }
    else if (buffer_ent->p_cfg->stream_type == AUDIO_STREAM_TYPE_VOICE)
    {
        total_frames = media_buffer_ds_frame_cnt(buffer_ent);
        if (total_frames < buffer_ent->start_decode_frame_cnt)
        {
            uint16_t diff = buffer_ent->start_decode_frame_cnt - total_frames;
            play_time_ms = diff * 15 / 2;
            if (play_time_ms <= 30)
            {
                AUDIO_PRINT_WARN0("media_buffer_cacu_playtime: total_frames > start_decode_frame_count");
                *playtime_ms = 30;
                return true;
            }
            else
            {
                AUDIO_PRINT_INFO3("media_buffer_cacu_playtime: voice start frames %u, total frames %u, play time %u ms",
                                  buffer_ent->start_decode_frame_cnt, total_frames, play_time_ms);
                *playtime_ms = play_time_ms;
                return true;
            }
        }
        else
        {
            AUDIO_PRINT_WARN0("media_buffer_cacu_playtime: total_frames > start_decode_frame_count");
            *playtime_ms = 25;
            return true;
        }
    }
    return false;
}

void media_buffer_dynamic_latency_set(T_MEDIA_BUFFER_ENTITY *buffer_ent, uint16_t latency,
                                      bool auto_resume)
{
    T_DYNAMIC_LAT_SET cmd;

    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }

    cmd.target_lat_ms = latency;
    cmd.previous_lat_ms = buffer_ent->p_cfg->latency_back_up;
    cmd.auto_resume = auto_resume;

    audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                 MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_SET,
                                 &cmd,
                                 sizeof(T_DYNAMIC_LAT_SET),
                                 false);

    audio_latency_dynamic_set(buffer_ent->audio_latency_handle, latency, auto_resume);
}

void media_buffer_dynamic_latency_resume(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }

    audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                 MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_RESUME,
                                 NULL,
                                 0,
                                 false);
    audio_latency_resume(buffer_ent->audio_latency_handle);
}

void media_buffer_handle_plc_event(T_MEDIA_BUFFER_ENTITY *buffer_ent, uint16_t plc_total_frames,
                                   uint16_t plc_frame_counter, uint16_t plc_local_seq, uint8_t plc_frame_num)
{
    uint16_t current_plc_count = buffer_ent->p_cfg->plc_count;
    uint16_t ds_local_seq_last;

    ds_local_seq_last = buffer_ent->ds_local_seq_last;

    if (media_buffer_cacu_frames_duration(plc_total_frames, &buffer_ent->p_cfg->format_info) > 90)
    {
        AUDIO_PRINT_INFO0("dsp_plc_event: plc too much, stop dsp");
        if (!buffer_ent->p_cfg->latency_fixed)
        {
            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY &&
                current_plc_count <= buffer_ent->max_latency_plc_count)
            {
                T_DYNAMIC_LAT_INC_DEC cmd;
                cmd.previous_total_plc_count = current_plc_count;
                cmd.plc_count = plc_frame_num;
                cmd.auto_dec = true;
                if (audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                                 MEDIA_BUFFER_REMOTE_DYNAMIC_LATENCY_INC,
                                                 &cmd,
                                                 sizeof(T_DYNAMIC_LAT_INC_DEC),
                                                 false) == false)
                {
                    audio_latency_low_lat_dynamic_increase(buffer_ent->audio_latency_handle, plc_frame_num,
                                                           cmd.auto_dec);
                }
            }
        }
        audio_mgr_exception(buffer_ent, MEDIA_BUFFER_DSP_PLC_TOO_MUCH, 0, 0);
        return;
    }

    if ((uint16_t)(buffer_ent->ds_local_seq_last + 1) == plc_local_seq)
    {

        T_MEDIA_DATA_HDR media_packet;
        T_MEDIA_DATA_HDR *p_media_packet;

        media_packet.frame_counter = plc_frame_counter;
        media_packet.local_seq_number = plc_local_seq;
        media_packet.frame_number = plc_frame_num;

        if (buffer_ent->last_packet_ptr == NULL)
        {
            return;
        }
        else
        {
            p_media_packet = buffer_ent->last_packet_ptr;
        }

        media_packet.avdtp_seq_number = p_media_packet->avdtp_seq_number;
        buffer_ent->ds_local_seq_last++;
        buffer_ent->ds_frame_counter_last = media_packet.frame_counter;
        media_buffer_packet_record_add(buffer_ent->audio_remote_handle, &media_packet);

    }
    else if (buffer_ent->ds_local_seq_last == plc_local_seq)
    {
        int diff;

        diff = media_buffer_packet_record_fix_by_local_seq(buffer_ent->audio_remote_handle, plc_local_seq,
                                                           plc_frame_counter,
                                                           plc_frame_num);
        AUDIO_PRINT_INFO1("dsp_plc_event: plc seq smaller than last seq frame diff %d ", diff);

        buffer_ent->ds_frame_counter_last += diff;
        if (buffer_ent->p_cfg->latency_fixed)
        {
            audio_remote_set_frame_diff(buffer_ent->audio_remote_handle,
                                        audio_remote_get_frame_diff(buffer_ent->audio_remote_handle) - diff);
        }
    }
    else
    {
        int diff;

        diff = media_buffer_packet_record_fix_by_local_seq(buffer_ent->audio_remote_handle, plc_local_seq,
                                                           plc_frame_counter,
                                                           plc_frame_num);
        AUDIO_PRINT_INFO1("dsp_plc_event: plc seq smaller than last seq frame diff %d ", diff);
        if (diff != 0)
        {
            audio_mgr_exception(buffer_ent, MEDIA_BUFFER_DSP_PLC_TOO_LATE, 0, 0);
            return;
        }
    }
    audio_latency_handle_plc_event(buffer_ent->audio_latency_handle, ds_local_seq_last,
                                   plc_total_frames,
                                   plc_frame_counter, plc_local_seq, plc_frame_num);
}

void media_buffer_handle_sync_lock(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
    {
        audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                     MEDIA_BUFFER_REMOTE_AUDIO_SYNCED,
                                     NULL,
                                     0,
                                     false);
        audio_remote_sync_lock(buffer_ent->audio_remote_handle);
        jitter_buffer_asrc_pid_unblock(buffer_ent->jitter_buffer_handle);
    }

    buffer_ent->local_audio_sync = true;
}

void media_buffer_handle_sync_v2_succ(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
    {
        audio_remote_sync_msg_relay(buffer_ent->audio_remote_handle,
                                    MEDIA_BUFFER_REMOTE_AUDIO_SYNCED_SUCC,
                                    NULL,
                                    0,
                                    REMOTE_TIMER_HIGH_PRECISION,
                                    60,
                                    false);
        audio_remote_sync_lock(buffer_ent->audio_remote_handle);
        jitter_buffer_asrc_pid_unblock(buffer_ent->jitter_buffer_handle);
    }

    buffer_ent->local_audio_sync = true;
}

void media_buffer_set_oneway_play_timer(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent->oneway_play_timer != NULL)
    {
        sys_timer_delete(buffer_ent->oneway_play_timer);
        buffer_ent->oneway_play_timer = NULL;
    }

    buffer_ent->oneway_play_timer = sys_timer_create("oneway_play_timer",
                                                     SYS_TIMER_TYPE_LOW_PRECISION,
                                                     (uint32_t)buffer_ent,
                                                     RWS_TIME_TO_ONE_WAY_PLAY,
                                                     false,
                                                     media_buffer_timeout_cback);
    if (buffer_ent->oneway_play_timer != NULL)
    {
        sys_timer_start(buffer_ent->oneway_play_timer);
    }
}

void media_buffer_relay_exception(T_MEDIA_BUFFER_ENTITY *buffer_ent, uint8_t *cmd)
{
    audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                 MEDIA_BUFFER_REMOTE_RPT_EXCEPTION,
                                 cmd,
                                 sizeof(T_AUDIO_EXC_CMD),
                                 false);
}

void media_buffer_query_sec_path_state(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
    {
        audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                     MEDIA_BUFFER_REMOTE_QUERY_PATH_STATE,
                                     NULL,
                                     0,
                                     false);
    }
}

void media_buffer_reset_relay(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    audio_remote_async_msg_relay(buffer_ent->audio_remote_handle,
                                 MEDIA_BUFFER_REMOTE_PRI_RESET,
                                 NULL,
                                 0,
                                 false);
}

void media_buffer_inteval_evaluator_update(T_MEDIA_BUFFER_PROXY proxy, uint16_t seq, uint32_t clk)
{
    T_MEDIA_BUFFER_CFG *p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    T_MEDIA_BUFFER_ENTITY *buffer_ent = p_cfg->buffer_handle;
    if (buffer_ent->audio_interval_eval_handle)
    {
        audio_inteval_evaluator_update(buffer_ent->audio_interval_eval_handle, seq, clk);
    }
}

bool media_buffer_inteval_evaluator_get_var(T_MEDIA_BUFFER_PROXY proxy, float *var)
{
    T_MEDIA_BUFFER_CFG *p_cfg = (T_MEDIA_BUFFER_CFG *)proxy;
    T_MEDIA_BUFFER_ENTITY *buffer_ent = p_cfg->buffer_handle;
    if (buffer_ent->audio_interval_eval_handle)
    {
        return audio_inteval_evaluator_get_var(buffer_ent->audio_interval_eval_handle, var);
    }
    else
    {
        return false;
    }
}
