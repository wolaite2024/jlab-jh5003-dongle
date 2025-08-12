/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "fmc_api.h"
#include "sys_cfg.h"
#include "audio_path.h"
#include "audio_mgr.h"
#include "ringtone.h"
#include "voice_prompt.h"
#include "tts.h"
#include "notification.h"
#include "notification_buffer.h"
#include "patch_header_check.h"
#include "storage.h"
#include "remote.h"
#include "bt_mgr.h"

#define NOTIFICATION_VOLUME_MAX         (15)
#define NOTIFICATION_VOLUME_MIN         (0)
#define NOTIFICATION_VOLUME_DEFAULT     (10)

#define NOTIFICATION_STREAM_STATE_IDLE          0x00
#define NOTIFICATION_STREAM_STATE_STARTED       0x01
#define NOTIFICATION_STREAM_STATE_CONTINUED     0x02
#define NOTIFICATION_STREAM_STATE_STOPPED       0x03

#define NOTIFICATION_QUEUE_NUM      0x08
#define NOTIFICATION_TTS_NUM        (2)

#define VP_FILE_HDR_LEN         (10)
#define VP_FILE_CFG_OFFSET      (6)
#define VP_FILE_CFG_SIZE        (4)
#define VP_FRAME_HDR_LEN        (2)

#define NOTIFICATION_REMOTE_HANDSHAKE_TIMEOUT   2000000
#define NOTIFICATION_REMOTE_POLL_TIMEOUT        5000000
#define NOTIFICATION_REMOTE_ACK_TIMEOUT         5000000


typedef enum t_notification_remote_msg
{
    NOTIFICATION_REMOTE_START           = 0x0101,
    NOTIFICATION_REMOTE_POLL            = 0x0102,
    NOTIFICATION_REMOTE_ACK             = 0x0103,
    NOTIFICATION_REMOTE_CANCEL          = 0x0104,
    NOTIFICATION_REMOTE_FLUSH           = 0x0105,
    NOTIFICATION_REMOTE_CLK_REQ         = 0x0108,
    NOTIFICATION_REMOTE_SYNC_ACTION     = 0x0109,
    NOTIFICATION_REMOTE_PUSH_RELAY      = 0x010a,
    NOTIFICATION_REMOTE_ACK_ABT         = 0x001b,
    NOTIFICATION_REMOTE_POLL_ABT        = 0x001c,
} T_NOTIFICATION_REMOTE_MSG;

typedef enum t_notification_timer
{
    NOTIFICATION_TIMER_ACK_EXPIRED        = 0x01,
    NOTIFICATION_TIMER_POLL_EXPIRED       = 0x02,
    NOTIFICATION_TIMER_ONE_WAY_PLAY       = 0x03,
} T_NOTIFICATION_TIMER;

typedef struct t_notification_msg_push_relay
{
    uint8_t type;               /* notification type: ringtone, tts, or voice prompt */
    uint8_t index;              /* notification index for ringtone, voice prompt and tts */
    uint8_t language;           /* voice prompt language id */
    uint32_t address;
    uint32_t len;
} T_NOTIFICATION_MSG_PUSH_RELAY;

typedef struct t_notification_msg_relay_start
{
    uint8_t     notification_type;
    uint8_t     index;
    uint8_t     tid;
    union
    {
        struct
        {
            uint8_t language;   /* voice prompt language id */
        } vp;

        struct
        {
            void *handle;
            uint32_t cfg[2];
        } tts;
    } params;
    uint32_t address;
    uint32_t len;
} T_NOTIFICATION_MSG_RELAY_START;

typedef struct t_notification_msg_sync_action
{
    uint32_t    sync_clk;
    uint32_t    init_clk;
    uint8_t     clk_ref;
    uint8_t     ref_role;
    uint8_t     tid;
} T_NOTIFICATION_MSG_SYNC_ACTION;

typedef struct t_notification_msg_flush
{
    uint8_t     type;
    uint8_t     relay;
} T_NOTIFICATION_MSG_FLUSH;

typedef struct t_notification_msg_cancel
{
    uint8_t     type;
    uint8_t     index;
    uint8_t     relay;
} T_NOTIFICATION_MSG_CANCEL;

typedef struct t_vp_data_hdr
{
    uint32_t    offset;
    uint32_t    len;
} T_VP_DATA_HDR;

typedef struct t_tone_data_hdr
{
    uint16_t    offset;
    uint16_t    len;
} T_TONE_DATA_HDR;

typedef struct t_tts_params
{
    T_AUDIO_PATH_HANDLE         handle;
    uint32_t                    length;
    uint32_t                    cfg[2];
    uint16_t                    frame_cnt;
} T_TTS_PARAM;

typedef struct t_notification_elem
{
    struct t_notification_elem *next;

    uint8_t type;               /* notification type: ringtone, tts, or voice prompt */
    uint8_t index;              /* notification index for ringtone, voice prompt and tts */
    bool    relay;              /* relay notification to other remote identical devices/or from remote */
    union
    {
        struct
        {
            uint8_t language;   /* voice prompt language id */
        } vp;

        struct
        {
            T_TTS_HANDLE handle;
            uint32_t cfg[2];
        } tts;
    } params;
    uint32_t address;
    uint32_t len;
} T_NOTIFICATION_ELEM;

typedef struct t_notification_db
{
    void                     *relay_handle;
    void                     *handshake_timer_handle;
    void                     *poll_time_handle;
    void                     *ack_time_handle;
    T_OS_QUEUE                idle_queue;
    T_OS_QUEUE                pending_queue;
    T_OS_QUEUE                play_queue;
    T_NOTIFICATION_ELEM       elements[NOTIFICATION_QUEUE_NUM];
    T_NOTIFICATION_STATE      state;
    T_RINGTONE_MODE           tone_mode;
    T_VOICE_PROMPT_MODE       vp_mode;
    T_NOTIFICATION_BUFFER_TYPE vp_buffer_type;
    T_NOTIFICATION_BUFFER_TYPE tts_buffer_type;
    uint8_t                   sync_action_flag;
    uint8_t                   sync_action_index;
    uint8_t                   sync_action_type;
    uint8_t                   sync_tid;
    uint8_t                   sync_play_index;
    uint8_t                   relay_type;
    bool                      lock;
    uint8_t                   stream_state;
    uint8_t                   tone_idx;
    uint8_t                   vp_idx;
    T_TTS_HANDLE              tts_handle;
    T_TTS_PARAM               tts_tbl[NOTIFICATION_TTS_NUM];
    T_AUDIO_PATH_HANDLE       tone_path_handle;
    T_AUDIO_PATH_HANDLE       vp_path_handle;
    uint32_t                  vp_cfg[3];
    uint16_t                  frame_num;
    uint32_t                  vp_flash_offset;
    uint32_t                  vp_flash_len;
    uint8_t                   tone_volume_max;
    uint8_t                   tone_volume_min;
    bool                      tone_volume_muted;
    uint8_t                   tone_volume;
    float                     tone_volume_scale;
    uint8_t                   vp_volume_max;
    uint8_t                   vp_volume_min;
    bool                      vp_volume_muted;
    uint8_t                   vp_volume;
    float                     vp_volume_scale;
    uint8_t                   tts_volume_max;
    uint8_t                   tts_volume_min;
    bool                      tts_volume_muted;
    uint8_t                   tts_volume;
    float                     tts_volume_scale;
    bool                      flow_go;
} T_NOTIFICATION_DB;

T_NOTIFICATION_DB *notification_db = NULL;

/* TODO Remove Start */
extern bool bt_clk_compare(uint32_t bt_clk_a, uint32_t bt_clk_b);
bool notification_clk_req(void);
void notification_start(bool sync_flag, uint8_t clk_ref, uint32_t sync_clk);
void notification_timeout_cback(T_SYS_TIMER_HANDLE handle);
/* TODO Remove End */

void notification_set_relay_type(uint8_t type)
{
    notification_db->relay_type = type;
}

void notification_set_state(T_NOTIFICATION_STATE state)
{
    notification_db->state = state;
}

T_NOTIFICATION_STATE notification_get_state(void)
{
    return notification_db->state;
}

void notification_lock(void)
{
    notification_db->lock = true;
}

void notification_unlock(void)
{
    notification_db->lock = false;
}

uint8_t notification_mode_get(uint8_t type)
{
    uint8_t mode;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        mode = 0;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        mode = (uint8_t)notification_db->tone_mode;
        break;

    case NOTIFICATION_TYPE_VP:
        mode = (uint8_t)notification_db->vp_mode;
        break;

    case NOTIFICATION_TYPE_TTS:
        mode = 0;
        break;

    default:
        mode = 0;
        break;
    }

    return mode;
}

bool notification_mode_set(uint8_t type, uint8_t mode)
{
    bool ret = true;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        ret = false;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        notification_db->tone_mode = (T_RINGTONE_MODE)mode;
        break;

    case NOTIFICATION_TYPE_VP:
        notification_db->vp_mode = (T_VOICE_PROMPT_MODE)mode;
        break;

    case NOTIFICATION_TYPE_TTS:
        ret = false;
        break;

    default:
        ret = false;
        break;
    }

    return ret;
}

void notification_set_mix_dsp_para(uint32_t bt_clk)
{
    T_TTS_PARAM *tts_param;
    uint8_t i;

    notification_db->vp_cfg[1] = bt_clk;

    for (i = 0; i < NOTIFICATION_TTS_NUM; ++i)
    {
        if (notification_db->tts_tbl[i].handle != NULL)
        {
            tts_param = &notification_db->tts_tbl[i];
            tts_param->cfg[1] = bt_clk;
        }
    }
}

void notification_set_sync_dsp_para(bool sycn_type)
{
    T_TTS_PARAM *tts_param;
    uint8_t i;

    if (sycn_type)
    {
        notification_db->vp_cfg[0] |= 0x1000000;

        for (i = 0; i < NOTIFICATION_TTS_NUM; ++i)
        {
            if (notification_db->tts_tbl[i].handle != NULL)
            {
                tts_param = &notification_db->tts_tbl[i];
                tts_param->cfg[0] |= 0x1000000;
            }
        }
    }
    else
    {
        notification_db->vp_cfg[0] &= ~0x1000000;

        for (i = 0; i < NOTIFICATION_TTS_NUM; ++i)
        {
            if (notification_db->tts_tbl[i].handle == NULL)
            {
                tts_param = &notification_db->tts_tbl[i];
                tts_param->cfg[0] &= ~0x1000000;
            }
        }
    }
}

static T_NOTIFICATION_ELEM *notification_find_tts_element(T_TTS_HANDLE handle)
{
    T_NOTIFICATION_ELEM *elem;

    elem = (T_NOTIFICATION_ELEM *)notification_db->play_queue.p_first;
    while (elem != NULL)
    {
        if (elem->type == NOTIFICATION_TYPE_TTS &&
            elem->params.tts.handle == handle)
        {
            break;
        }

        elem = elem->next;
    }

    return elem;
}

static T_TTS_PARAM *notification_find_tts_param(T_TTS_HANDLE handle)
{
    uint8_t i;

    for (i = 0; i < NOTIFICATION_TTS_NUM; ++i)
    {
        if (notification_db->tts_tbl[i].handle == handle)
        {
            return &(notification_db->tts_tbl[i]);
        }
    }

    return NULL;
}

static bool notification_tts_data_req(T_TTS_HANDLE tts_handle)
{
    T_TTS_PARAM *tts_param;
    uint8_t     *p_frame;
    uint32_t     frame_size;

    tts_param = notification_find_tts_param(tts_handle);
    if (tts_param != NULL)
    {
        if (notification_db->stream_state == NOTIFICATION_STREAM_STATE_STARTED)
        {
            if (audio_path_data_send(tts_param->handle,
                                     AUDIO_PATH_DATA_TYPE_VP,
                                     0,
                                     tts_param->cfg,
                                     8,
                                     true) == false)
            {
                return false;
            }
        }

        notification_db->stream_state = NOTIFICATION_STREAM_STATE_CONTINUED;

        if (notification_db->flow_go == true)
        {
            p_frame = notification_buffer_peek(notification_db->tts_buffer_type, &frame_size);
            if (p_frame != NULL)
            {
                if (audio_path_data_send(tts_param->handle,
                                         AUDIO_PATH_DATA_TYPE_VP,
                                         0,
                                         p_frame,
                                         frame_size,
                                         true) == true)
                {
                    notification_db->flow_go = false;
                    return notification_buffer_flush(notification_db->tts_buffer_type);
                }
            }
        }
    }

    return false;
}

static bool notification_buffer_tts_cback(void                        *owner,
                                          T_NOTIFICATION_BUFFER_EVENT  event,
                                          uint32_t                     param)
{
    T_TTS_HANDLE handle;
    bool         ret = false;

    AUDIO_PRINT_TRACE3("notification_buffer_tts_cback: owner %p, event 0x%02x, param 0x%08x",
                       owner, event, param);

    handle = (T_TTS_HANDLE)owner;

    switch (event)
    {
    case NOTIFICATION_BUFFER_EVENT_PLAY:
        ret = notification_tts_data_req(handle);
        break;

    case NOTIFICATION_BUFFER_EVENT_EMPTY:
        ret = audio_mgr_dispatch(AUDIO_MSG_TTS_BUFFER_EMPTY, handle);
        break;

    case NOTIFICATION_BUFFER_EVENT_LOW:
        ret = audio_mgr_dispatch(AUDIO_MSG_TTS_BUFFER_LOW, handle);
        break;

    case NOTIFICATION_BUFFER_EVENT_E_PLAY:
        ret = notification_tts_data_req(handle);
        break;

    case NOTIFICATION_BUFFER_EVENT_HIGH:
        ret = audio_mgr_dispatch(AUDIO_MSG_TTS_BUFFER_HIGH, handle);
        break;

    case NOTIFICATION_BUFFER_EVENT_F_PLAY:
        ret = audio_mgr_dispatch(AUDIO_MSG_TTS_BUFFER_NORMAL, handle);
        break;

    case NOTIFICATION_BUFFER_EVENT_FULL:
        ret = audio_mgr_dispatch(AUDIO_MSG_TTS_BUFFER_FULL, handle);
        break;

    default:
        break;
    }

    return ret;
}

static bool notification_buffer_vp_cback(void                        *owner,
                                         T_NOTIFICATION_BUFFER_EVENT  event,
                                         uint32_t                     param)
{
    bool ret = true;

    AUDIO_PRINT_TRACE3("notification_buffer_vp_cback: owner %p, event 0x%02x, param 0x%08x",
                       owner, event, param);

    switch (event)
    {
    case NOTIFICATION_BUFFER_EVENT_PREQUEUE:
        {
            if (param == 0xFFFF)
            {
                notification_db->vp_flash_offset = 0;
                notification_db->vp_flash_len    = 0;
            }
            else
            {
                notification_db->vp_flash_offset += param;
                notification_db->vp_flash_len    -= param;
            }
        }
        break;

    case NOTIFICATION_BUFFER_EVENT_PLAY:
        if (notification_clk_req() == false)
        {
            notification_start(false, 0, 0xffffffff);
        }
        else
        {
            if (notification_db->handshake_timer_handle != NULL)
            {
                sys_timer_delete(notification_db->handshake_timer_handle);
            }

            notification_db->handshake_timer_handle = sys_timer_create("notification_handshake",
                                                                       SYS_TIMER_TYPE_LOW_PRECISION,
                                                                       (NOTIFICATION_TIMER_ONE_WAY_PLAY << 16) | 0,
                                                                       NOTIFICATION_REMOTE_HANDSHAKE_TIMEOUT,
                                                                       false,
                                                                       notification_timeout_cback);
            if (notification_db->handshake_timer_handle != NULL)
            {
                sys_timer_start(notification_db->handshake_timer_handle);
            }
        }
        break;

    case NOTIFICATION_BUFFER_EVENT_EMPTY:
        {
            uint32_t load_offset;

            notification_db->vp_flash_offset += param;
            notification_db->vp_flash_len    -= param;

            if (notification_db->vp_flash_len != 0)
            {
                /* make the loading address 4-byte alignment */
                load_offset = notification_db->vp_flash_offset & 0x03;
                notification_db->vp_flash_offset -= load_offset;
                notification_db->vp_flash_len    += load_offset;

                ret = notification_buffer_prequeue(notification_db->vp_buffer_type,
                                                   notification_db->vp_flash_offset,
                                                   load_offset,
                                                   notification_db->vp_flash_len);
            }
        }
        break;

    case NOTIFICATION_BUFFER_EVENT_LOW:
        {
            if (notification_db->vp_flash_len != 0)
            {
                ret = notification_buffer_prequeue(notification_db->vp_buffer_type,
                                                   notification_db->vp_flash_offset,
                                                   0,
                                                   notification_db->vp_flash_len);
            }
        }
        break;

    default:
        ret = false;
        break;
    }

    return ret;
}
#if (defined CONFIG_SOC_SERIES_RTL8763E) || (defined TARGET_RTL87X3G)
static void notification_start_vp(uint8_t index, uint8_t language, uint32_t addr, uint32_t len)
{
    T_VP_DATA_HDR vp_hdr;
    uint32_t      hdr_offset;
    uint32_t      table_size;
    uint8_t       cfg_buf[VP_FILE_HDR_LEN];
    const T_STORAGE_PARTITION_INFO *info;
    bool          use_customized_vp = false;
    bool          read_vp_cfg_ret = false;

    if ((addr > 0x0) && (len > 0x0))
    {
        use_customized_vp = true;
    }

    table_size = sizeof(T_VP_DATA_HDR) * sys_cfg_const.voice_prompt_index_num;
    hdr_offset = APP_DATA_SYNC_WORD_LEN + (table_size * language) + (sizeof(T_VP_DATA_HDR) * index);

    AUDIO_PRINT_TRACE6("notification_start_vp: index 0x%02x, index num %u, language %u, customized_vp %d, addr 0x%x, len 0x%x",
                       index, sys_cfg_const.voice_prompt_index_num, language, use_customized_vp, addr, len);

    info = storage_partition_get(VP_PARTITION_NAME);
    fmc_flash_nor_read(get_img_header_format_size() + info->address +  hdr_offset,
                       &vp_hdr,
                       sizeof(T_VP_DATA_HDR));

    notification_buffer_reset(notification_db->vp_path_handle, notification_buffer_vp_cback);
    notification_buffer_config(vp_hdr.len, 75, 24, 24);

    notification_db->vp_flash_offset = get_img_header_format_size() + info->address + vp_hdr.offset;
    notification_db->vp_flash_len    = vp_hdr.len;


    //Get header (10bytes) to parsing vp info.
    if (use_customized_vp)
    {
        read_vp_cfg_ret = fmc_flash_nor_read(addr, cfg_buf, VP_FILE_HDR_LEN);
    }
    else
    {
        read_vp_cfg_ret = fmc_flash_nor_read(notification_db->vp_flash_offset,
                                             cfg_buf,
                                             VP_FILE_HDR_LEN);
    }

    if (read_vp_cfg_ret)

    {
        /* big endian */
        notification_db->frame_num = (uint16_t)((cfg_buf[4] << 8) | cfg_buf[5]);
        notification_db->vp_cfg[0] = (uint32_t)((cfg_buf[6] << 24) | (cfg_buf[7] << 16) |
                                                (cfg_buf[8] << 8) | cfg_buf[9]);

        if (use_customized_vp)
        {
            notification_db->vp_flash_offset = addr;
        }

        /* calculate the first frame address offset */
        notification_db->vp_flash_offset += VP_FILE_HDR_LEN;
        notification_db->vp_flash_len    -= VP_FILE_HDR_LEN;

        (void)audio_path_start(notification_db->vp_path_handle);
    }
}
#else
static void notification_start_vp(uint8_t index, uint8_t language)
{
    T_VP_DATA_HDR vp_hdr;
    uint32_t      hdr_offset;
    uint32_t      table_size;
    uint8_t       cfg_buf[VP_FILE_HDR_LEN];

    table_size = sizeof(T_VP_DATA_HDR) * sys_cfg_const.voice_prompt_index_num;
    hdr_offset = VOICE_PROMPT_OFFSET + APP_DATA_SYNC_WORD_LEN +
                 (table_size * language) + (sizeof(T_VP_DATA_HDR) * index);

    AUDIO_PRINT_TRACE3("notification_start_vp: index 0x%02x, index num %u, language %u",
                       index, sys_cfg_const.voice_prompt_index_num, language);

    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) + hdr_offset,
                       &vp_hdr,
                       sizeof(T_VP_DATA_HDR));

    notification_buffer_reset(notification_db->vp_path_handle, notification_buffer_vp_cback);
    notification_buffer_config(vp_hdr.len, 75, 24, 24);

    notification_db->vp_flash_offset = flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) +
                                       VOICE_PROMPT_OFFSET + vp_hdr.offset;
    notification_db->vp_flash_len    = vp_hdr.len;

    if (fmc_flash_nor_read(notification_db->vp_flash_offset,
                           cfg_buf,
                           VP_FILE_HDR_LEN))
    {
        /* big endian */
        notification_db->frame_num = (uint16_t)((cfg_buf[4] << 8) | cfg_buf[5]);
        notification_db->vp_cfg[0] = (uint32_t)((cfg_buf[6] << 24) | (cfg_buf[7] << 16) |
                                                (cfg_buf[8] << 8) | cfg_buf[9]);

        /* calculate the first frame address offset */
        notification_db->vp_flash_offset += VP_FILE_HDR_LEN;
        notification_db->vp_flash_len    -= VP_FILE_HDR_LEN;

        (void)audio_path_start(notification_db->vp_path_handle);
    }
}
#endif
static void notification_start_tts(T_TTS_HANDLE tts_handle)
{
    T_TTS_PARAM *tts_param;

    tts_param = notification_find_tts_param(tts_handle);

    AUDIO_PRINT_TRACE2("notification_start_tts: tts_handle %p, tts_param %p", tts_handle, tts_param);

    if (tts_param != NULL)
    {
        notification_buffer_reset(tts_handle, notification_buffer_tts_cback);
        notification_buffer_config(tts_param->length, 87, 25, 75);

        (void)audio_path_start(tts_param->handle);
    }
}

static bool notification_vp_data_req(void)
{
    uint8_t  *p_frame;
    uint32_t  frame_size;

    if (notification_db->stream_state == NOTIFICATION_STREAM_STATE_STARTED)
    {
        if (audio_path_data_send(notification_db->vp_path_handle,
                                 AUDIO_PATH_DATA_TYPE_VP,
                                 0,
                                 notification_db->vp_cfg,
                                 8,
                                 true) == false)
        {
            return false;
        }
    }

    notification_db->stream_state = NOTIFICATION_STREAM_STATE_CONTINUED;

    p_frame = notification_buffer_peek(notification_db->vp_buffer_type, &frame_size);
    if (p_frame != NULL)
    {
        if (audio_path_data_send(notification_db->vp_path_handle,
                                 AUDIO_PATH_DATA_TYPE_VP,
                                 0,
                                 p_frame,
                                 frame_size,
                                 true) == true)
        {
            return notification_buffer_flush(notification_db->vp_buffer_type);
        }
    }

    return false;
}

void notification_path_start(T_NOTIFICATION_ELEM *elem)
{
    APP_PRINT_TRACE1("notification_path_start: %x", elem->type);

    if (elem->type == NOTIFICATION_TYPE_RINGTONE)
    {
        notification_set_state(NOTIFICATION_STATE_RINGTONE);
        notification_set_relay_type(NOTIFICATION_TYPE_NONE);
        notification_db->sync_play_index = 0xff;
        notification_db->tone_idx = elem->index;

        (void)audio_path_start(notification_db->tone_path_handle);
    }
    else if (elem->type == NOTIFICATION_TYPE_VP)
    {
        notification_set_state(NOTIFICATION_STATE_VP);
        notification_set_relay_type(NOTIFICATION_TYPE_NONE);
        notification_db->sync_play_index = 0xff;
        notification_db->vp_idx = elem->index;

        notification_start_vp(elem->index, elem->params.vp.language, elem->address, elem->len);
    }
    else if (elem->type == NOTIFICATION_TYPE_TTS)
    {
        notification_set_state(NOTIFICATION_STATE_TTS);
        notification_set_relay_type(NOTIFICATION_TYPE_NONE);
        notification_db->sync_play_index = 0xff;
        notification_db->tts_handle = elem->params.tts.handle;

        notification_start_tts(elem->params.tts.handle);
    }
}

void notification_ack_abort(void)
{
    if (notification_db->state != NOTIFICATION_STATE_RELAY)
    {
        AUDIO_PRINT_WARN0("notification_poll_abort: state error");
        return;
    }
    notification_set_state(NOTIFICATION_STATE_IDLE);
    notification_set_relay_type(NOTIFICATION_TYPE_NONE);
    notification_db->sync_play_index = 0xff;
}

void notification_poll_abort(void)
{
    T_NOTIFICATION_ELEM *elem;

    if (notification_db->state != NOTIFICATION_STATE_RELAY)
    {
        AUDIO_PRINT_WARN0("notification_poll_abort: state error");
        return;
    }
    notification_set_state(NOTIFICATION_STATE_IDLE);
    notification_set_relay_type(NOTIFICATION_TYPE_NONE);
    notification_db->sync_play_index = 0xff;
    elem = os_queue_out(&notification_db->pending_queue);
    while (elem != NULL)
    {
        os_queue_in(&notification_db->play_queue, elem);
        elem = os_queue_out(&notification_db->pending_queue);
    }

    elem = os_queue_peek(&notification_db->play_queue, 0);

    if (elem == NULL)
    {
        AUDIO_PRINT_WARN0("notification_poll_abort: queue empty");
        return;
    }
    if (elem->relay)
    {
        elem->relay = false;
        AUDIO_PRINT_TRACE4("notification_poll_abort: state %u, type %u, index %u, relay %u",
                           notification_db->state, elem->type, elem->index, elem->relay);
        elem = os_queue_out(&notification_db->play_queue);
        notification_path_start(elem);
        os_queue_in(&notification_db->idle_queue, elem);
    }
    else
    {
        AUDIO_PRINT_WARN0("notification_poll_abort: error");
    }
}

void notification_start(bool sync_flag, uint8_t clk_ref, uint32_t sync_clk)
{
    if (notification_db->handshake_timer_handle != NULL)
    {
        sys_timer_delete(notification_db->handshake_timer_handle);
        notification_db->handshake_timer_handle = NULL;
    }

    if (notification_db->state == NOTIFICATION_STATE_VP)
    {
        notification_set_sync_dsp_para(sync_flag);
        notification_db->vp_cfg[1] = sync_clk;
        notification_db->vp_cfg[2] = clk_ref;
        notification_vp_data_req();
    }
    else if (notification_db->state == NOTIFICATION_STATE_RINGTONE)
    {
        T_TONE_DATA_HDR tone_hdr;
        uint16_t hdr_offset;
        void *p_param;

        hdr_offset = TONE_DATA_OFFSET + APP_DATA_SYNC_WORD_LEN + APP_DATA_TONE_NUM_LEN +
                     (notification_db->tone_idx * sizeof(T_TONE_DATA_HDR));
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) +
                           hdr_offset,
                           (uint8_t *)&tone_hdr, sizeof(T_TONE_DATA_HDR));

        p_param = os_mem_zalloc2(tone_hdr.len);
        if (p_param != NULL)
        {
            notification_db->stream_state = NOTIFICATION_STREAM_STATE_CONTINUED;
            fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) +
                               TONE_DATA_OFFSET + tone_hdr.offset,
                               p_param, tone_hdr.len);
            audio_path_timestamp_set(notification_db->tone_path_handle, clk_ref, sync_clk, sync_flag);
            (void)audio_path_data_send(notification_db->tone_path_handle, AUDIO_PATH_DATA_TYPE_COMPOSITE, 0,
                                       p_param, tone_hdr.len, true);
            os_mem_free(p_param);
        }
    }
    notification_db->sync_action_index = 0xff;
    notification_db->sync_action_flag = 0;
}

void notification_timeout_cback(T_SYS_TIMER_HANDLE handle)
{
    uint32_t timer_id;
    uint16_t event;

    timer_id = sys_timer_id_get(handle);
    AUDIO_PRINT_TRACE2("notification_timeout_cback: timer_id 0x%02X handle %p",
                       timer_id, handle);
    event = timer_id >> 16;

    switch (event)
    {
    case NOTIFICATION_TIMER_POLL_EXPIRED:
        {
            sys_timer_stop(notification_db->poll_time_handle);
            notification_poll_abort();
        }
        break;

    case NOTIFICATION_TIMER_ACK_EXPIRED:
        {
            sys_timer_stop(notification_db->ack_time_handle);
            notification_ack_abort();
        }
        break;

    case NOTIFICATION_TIMER_ONE_WAY_PLAY:
        {
            if (notification_db->sync_action_index != 0xff)
            {
                notification_start(false, BT_CLK_NONE, 0xffffffff);
            }
        }
        break;

    default:
        break;
    }
}


bool notification_clk_req(void)
{
    bool ret = false;

    if (notification_db->sync_action_index == 0xff)
    {
        ret = false;
    }
    else
    {
        if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
        {
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                notification_db->sync_action_flag |= 0xf0;
                ret = true;
                if (notification_db->sync_action_flag == 0xff)
                {
                    T_NOTIFICATION_MSG_SYNC_ACTION cmd;
                    T_BT_CLK_REF clk_ref;
                    uint32_t bb_clock_slot;
                    uint16_t bb_clock_us;

                    clk_ref = bt_piconet_clk_get(BT_CLK_NONE, &bb_clock_slot, &bb_clock_us);
                    cmd.init_clk = bb_clock_slot;
                    if (clk_ref == BT_CLK_NONE)
                    {
                        bb_clock_slot = 0xffffffff;
                    }
                    else
                    {
                        bb_clock_slot += (90 * 2 * 1000 / 625); // ms to bt clk
                        bb_clock_slot &= 0x0fffffff;
                    }
                    cmd.sync_clk = bb_clock_slot;
                    cmd.clk_ref = clk_ref;
                    cmd.tid = notification_db->sync_tid;

                    if (clk_ref == BT_CLK_CTRL)
                    {
                        uint8_t index;
                        bt_piconet_id_get(BT_CLK_CTRL, &index, &cmd.ref_role);
                    }

                    if (remote_sync_msg_relay(notification_db->relay_handle,
                                              NOTIFICATION_REMOTE_SYNC_ACTION,
                                              &cmd,
                                              sizeof(T_NOTIFICATION_MSG_SYNC_ACTION),
                                              REMOTE_TIMER_HIGH_PRECISION,
                                              70,
                                              false) == false)

                    {
                        AUDIO_PRINT_WARN0("audio_mgr_dispatch: sync action send fail");
                        notification_db->sync_action_index = 0xff;
                        notification_db->sync_action_flag = 0;
                        ret = false;
                    }
                }
            }
            else
            {
                ret = remote_async_msg_relay(notification_db->relay_handle,
                                             NOTIFICATION_REMOTE_CLK_REQ,
                                             &notification_db->sync_tid,
                                             1,
                                             false);
            }
        }
        else
        {
            ret = false;
        }
    }

    return ret;
}

void notification_play(void)
{
    T_NOTIFICATION_ELEM *elem;

    if (notification_db->state == NOTIFICATION_STATE_IDLE)
    {
        //notification_play is assumed to be invoked when state is idle
        //Anyway check it with caution
        elem = os_queue_out(&notification_db->pending_queue);
        while (elem != NULL)
        {
            os_queue_in(&notification_db->play_queue, elem);
            elem = os_queue_out(&notification_db->pending_queue);
        }

        if (notification_db->lock)
        {
            notification_db->lock = false;
            if (remote_async_msg_relay(notification_db->relay_handle,
                                       NOTIFICATION_REMOTE_ACK,
                                       &notification_db->sync_tid,
                                       1,
                                       false))
            {
                notification_set_state(NOTIFICATION_STATE_RELAY);
                sys_timer_start(notification_db->ack_time_handle);
            }

            return;
        }

        elem = os_queue_peek(&notification_db->play_queue, 0);

        if (elem == NULL)
        {
            AUDIO_PRINT_INFO0("notification_play: queue empty");
            return;
        }

        if (elem->type == NOTIFICATION_TYPE_VP || elem->type == NOTIFICATION_TYPE_TTS)
        {
            notification_set_mix_dsp_para(0xffffffff);
            notification_set_sync_dsp_para(0);
        }

        if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY &&
            remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED &&
            elem->relay == true &&
            elem->params.tts.handle == NULL)
        {
            /*
                sec may have relay type tone after roleswap.
                relay those tones to primary.
            */
            T_NOTIFICATION_MSG_PUSH_RELAY elem_msg;

            elem_msg.index = elem->index;
            elem_msg.type  = elem->type;
            elem_msg.language = elem->params.vp.language;
            elem_msg.address = elem->address;
            elem_msg.len = elem->len;

            if (remote_async_msg_relay(notification_db->relay_handle,
                                       NOTIFICATION_REMOTE_PUSH_RELAY,
                                       &elem_msg,
                                       sizeof(T_NOTIFICATION_MSG_PUSH_RELAY),
                                       false))
            {
                if (notification_get_state() != NOTIFICATION_STATE_RELAY &&
                    notification_get_state() != NOTIFICATION_STATE_IDLE)
                {
                    notification_lock();
                }
                else
                {
                    if (notification_get_state() == NOTIFICATION_STATE_IDLE)
                    {
                        sys_timer_start(notification_db->ack_time_handle);
                        notification_set_state(NOTIFICATION_STATE_RELAY);
                    }
                }

                elem = os_queue_out(&notification_db->play_queue);
                os_queue_in(&notification_db->idle_queue, elem);
                return;
            }
            else
            {
                elem->relay = false;
            }
        }

        if (elem->relay)
        {
            notification_db->sync_tid++;
            if (remote_async_msg_relay(notification_db->relay_handle,
                                       NOTIFICATION_REMOTE_POLL,
                                       &notification_db->sync_tid,
                                       1,
                                       false))
            {
                sys_timer_start(notification_db->poll_time_handle);
                notification_set_state(NOTIFICATION_STATE_RELAY);
                return;
            }
            else
            {
                AUDIO_PRINT_TRACE0("notification_play: sync notification send failed. play alone.");
            }
        }

        AUDIO_PRINT_TRACE6("notification_play: state %u, type %u, index %u, relay %u, addr 0x%x, len 0x%x",
                           notification_db->state, elem->type, elem->index, elem->relay, elem->address, elem->len);

        elem = os_queue_out(&notification_db->play_queue);
        notification_db->sync_action_index = 0xff;
        notification_db->sync_action_flag = 0;
        notification_path_start(elem);
        os_queue_in(&notification_db->idle_queue, elem);
    }
    else if (notification_db->state == NOTIFICATION_STATE_RELAY)
    {
        elem = os_queue_out(&notification_db->pending_queue);
        while (elem != NULL)
        {
            os_queue_in(&notification_db->play_queue, elem);
            elem = os_queue_out(&notification_db->pending_queue);
        }

        elem = os_queue_out(&notification_db->play_queue);

        if (elem == NULL)
        {
            AUDIO_PRINT_INFO0("notification_play: queue empty");
            notification_set_state(NOTIFICATION_STATE_IDLE);
            notification_play();
            return;
        }

        if (elem->relay)
        {
            T_NOTIFICATION_MSG_RELAY_START payload;

            payload.notification_type = elem->type;
            payload.index = elem->index;
            payload.address = elem->address;
            payload.len = elem->len;

            if (elem->type == NOTIFICATION_TYPE_VP)
            {
                payload.params.vp.language = elem->params.vp.language;
            }
            else if (elem->type == NOTIFICATION_TYPE_TTS)
            {
                payload.params.tts.handle = elem->params.tts.handle;
                payload.params.tts.cfg[0] = elem->params.tts.cfg[0];
                payload.params.tts.cfg[1] = elem->params.tts.cfg[1];
            }

            payload.tid = notification_db->sync_tid;
            if (remote_sync_msg_relay(notification_db->relay_handle,
                                      NOTIFICATION_REMOTE_START,
                                      &payload,
                                      sizeof(T_NOTIFICATION_MSG_RELAY_START),
                                      REMOTE_TIMER_HIGH_PRECISION,
                                      70,
                                      false))
            {
                notification_db->sync_play_index = payload.index;
                notification_set_relay_type(elem->type);
                os_queue_in(&notification_db->idle_queue, elem);
                return;
            }
            else
            {
                AUDIO_PRINT_TRACE0("notification_play: sync notification send failed. play alone.");
            }
        }
        AUDIO_PRINT_TRACE6("notification_play: state %u, type %u, index %u, relay %u, addr 0x%x, len 0x%x",
                           notification_db->state, elem->type, elem->index, elem->relay, elem->address, elem->len);
        notification_path_start(elem);
        os_queue_in(&notification_db->idle_queue, elem);

    }
    else
    {
        AUDIO_PRINT_TRACE0("notification_play: error state");
    }

}

static bool audio_path_voice_prompt_cback(T_AUDIO_PATH_HANDLE handle,
                                          T_AUDIO_PATH_EVENT  event,
                                          uint32_t            param)
{
    AUDIO_PRINT_TRACE3("audio_path_voice_prompt_cback: handle %p, event 0x%02x, param 0x%08x",
                       handle, event, param);

    switch (event)
    {
    case AUDIO_PATH_EVT_RUNNING:
        {
            T_AUDIO_MSG_PAYLOAD_NOTIFICATION_START payload;

            payload.type  = NOTIFICATION_TYPE_VP;
            payload.index = notification_db->vp_idx;

            notification_db->stream_state = NOTIFICATION_STREAM_STATE_STARTED;

            if (notification_db->vp_volume_muted == false)
            {
                audio_path_dac_level_set(handle, notification_db->vp_volume, notification_db->vp_volume_scale);
            }
            else
            {
                audio_path_dac_mute(handle);
            }

            notification_buffer_prequeue(notification_db->vp_buffer_type,
                                         notification_db->vp_flash_offset,
                                         0,
                                         notification_db->vp_flash_len);

            audio_mgr_dispatch(AUDIO_MSG_NOTIFICATION_START, &payload);
        }
        break;

    case AUDIO_PATH_EVT_REQ_DATA:
        {
            notification_vp_data_req();
        }
        break;

    case AUDIO_PATH_EVT_IDLE:
        {
            T_AUDIO_MSG_PAYLOAD_NOTIFICATION_STOP payload;

            payload.type  = NOTIFICATION_TYPE_VP;
            payload.index = notification_db->vp_idx;

            notification_db->stream_state = NOTIFICATION_STREAM_STATE_IDLE;
            notification_db->state        = NOTIFICATION_STATE_IDLE;
            notification_db->vp_idx       = NOTIFICATION_INVALID_INDEX;
            audio_mgr_dispatch(AUDIO_MSG_NOTIFICATION_STOP, &payload);

            notification_play();
        }
        break;

    default:
        break;
    }

    return true;
}

static bool audio_path_tts_cback(T_AUDIO_PATH_HANDLE handle,
                                 T_AUDIO_PATH_EVENT  event,
                                 uint32_t            param)
{
    AUDIO_PRINT_TRACE3("audio_path_tts_cback: handle %p, event 0x%02x, param 0x%08x",
                       handle, event, param);

    switch (event)
    {
    case AUDIO_PATH_EVT_RUNNING:
        {
            T_AUDIO_MSG_PAYLOAD_NOTIFICATION_START payload;

            payload.type   = NOTIFICATION_TYPE_TTS;
            payload.handle = notification_db->tts_handle;

            notification_db->stream_state = NOTIFICATION_STREAM_STATE_STARTED;

            if (notification_db->tts_volume_muted == false)
            {
                audio_path_dac_level_set(handle, notification_db->tts_volume, notification_db->tts_volume_scale);
            }
            else
            {
                audio_path_dac_mute(handle);
            }

            notification_tts_data_req(handle);

            audio_mgr_dispatch(AUDIO_MSG_NOTIFICATION_START, &payload);
        }
        break;

    case AUDIO_PATH_EVT_REQ_DATA:
        {
            notification_db->flow_go = true;
            notification_tts_data_req(handle);
        }
        break;

    case AUDIO_PATH_EVT_IDLE:
        {
            T_AUDIO_MSG_PAYLOAD_NOTIFICATION_STOP payload;

            payload.type   = NOTIFICATION_TYPE_TTS;
            payload.handle = notification_db->tts_handle;
            notification_db->flow_go = true;

            notification_db->stream_state = NOTIFICATION_STREAM_STATE_IDLE;
            notification_db->state        = NOTIFICATION_STATE_IDLE;
            notification_db->tts_handle   = NULL;
            audio_mgr_dispatch(AUDIO_MSG_NOTIFICATION_STOP, &payload);
            notification_play();
        }
        break;

    default:
        break;
    }

    return true;
}

static bool audio_path_composite_cback(T_AUDIO_PATH_HANDLE handle,
                                       T_AUDIO_PATH_EVENT  event,
                                       uint32_t            param)
{
    AUDIO_PRINT_TRACE3("audio_path_composite_cback: handle %p, event 0x%02x, param 0x%08x",
                       handle, event, param);

    switch (event)
    {
    case AUDIO_PATH_EVT_RUNNING:
        {
            T_AUDIO_MSG_PAYLOAD_NOTIFICATION_START payload;

            payload.type  = NOTIFICATION_TYPE_RINGTONE;
            payload.index = notification_db->tone_idx;

            notification_db->stream_state = NOTIFICATION_STREAM_STATE_STARTED;

            if (notification_db->tone_volume_muted == false)
            {
                audio_path_dac_level_set(handle, notification_db->tone_volume, notification_db->tone_volume_scale);
            }
            else
            {
                audio_path_dac_mute(handle);
            }

            if (notification_clk_req() == false)
            {
                notification_start(false, BT_CLK_NONE, 0xffffffff);
            }
            else
            {
                if (notification_db->handshake_timer_handle != NULL)
                {
                    sys_timer_delete(notification_db->handshake_timer_handle);
                }
                notification_db->handshake_timer_handle = sys_timer_create("notification_handshake",
                                                                           SYS_TIMER_TYPE_LOW_PRECISION,
                                                                           (NOTIFICATION_TIMER_ONE_WAY_PLAY << 16) | 0,
                                                                           NOTIFICATION_REMOTE_HANDSHAKE_TIMEOUT,
                                                                           false,
                                                                           notification_timeout_cback);
                if (notification_db->handshake_timer_handle != NULL)
                {
                    sys_timer_start(notification_db->handshake_timer_handle);
                }
            }

            audio_mgr_dispatch(AUDIO_MSG_NOTIFICATION_START, &payload);
        }
        break;

    case AUDIO_PATH_EVT_REQ_DATA:
        break;

    case AUDIO_PATH_EVT_IDLE:
        {
            T_AUDIO_MSG_PAYLOAD_NOTIFICATION_STOP payload;

            payload.type  = NOTIFICATION_TYPE_RINGTONE;
            payload.index = notification_db->tone_idx;

            notification_db->stream_state = NOTIFICATION_STREAM_STATE_IDLE;
            notification_db->state        = NOTIFICATION_STATE_IDLE;
            notification_db->tone_idx     = NOTIFICATION_INVALID_INDEX;
            audio_mgr_dispatch(AUDIO_MSG_NOTIFICATION_STOP, &payload);
            notification_play();
        }
        break;

    default:
        break;
    }

    return true;
}

uint8_t notification_volume_max_get(uint8_t type)
{
    uint8_t volume;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        volume = 0;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        volume = notification_db->tone_volume_max;
        break;

    case NOTIFICATION_TYPE_VP:
        volume = notification_db->vp_volume_max;
        break;

    case NOTIFICATION_TYPE_TTS:
        volume = notification_db->tts_volume_max;
        break;

    default:
        volume = 0;
        break;
    }

    return volume;
}

bool notification_volume_max_set(uint8_t type, uint8_t volume)
{
    bool ret = true;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        ret = false;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        notification_db->tone_volume_max = volume;
        break;

    case NOTIFICATION_TYPE_VP:
        notification_db->vp_volume_max = volume;
        break;

    case NOTIFICATION_TYPE_TTS:
        notification_db->tts_volume_max = volume;
        break;
    }

    return ret;
}

uint8_t notification_volume_min_get(uint8_t type)
{
    uint8_t volume;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        volume = 0;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        volume = notification_db->tone_volume_min;
        break;

    case NOTIFICATION_TYPE_VP:
        volume = notification_db->vp_volume_min;
        break;

    case NOTIFICATION_TYPE_TTS:
        volume = notification_db->tts_volume_min;
        break;

    default:
        volume = 0;
        break;
    }

    return volume;
}

bool notification_volume_min_set(uint8_t type, uint8_t volume)
{
    bool ret = true;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        ret = false;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        notification_db->tone_volume_min = volume;
        break;

    case NOTIFICATION_TYPE_VP:
        notification_db->vp_volume_min = volume;
        break;

    case NOTIFICATION_TYPE_TTS:
        notification_db->tts_volume_min = volume;
        break;
    }

    return ret;
}

uint8_t notification_volume_get(uint8_t type)
{
    uint8_t volume;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        volume = 0;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        volume = notification_db->tone_volume;
        break;

    case NOTIFICATION_TYPE_VP:
        volume = notification_db->vp_volume;
        break;

    case NOTIFICATION_TYPE_TTS:
        volume = notification_db->tts_volume;
        break;

    default:
        volume = 0;
        break;
    }

    return volume;
}

bool notification_volume_set(uint8_t type, uint8_t volume)
{
    bool ret = true;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        ret = false;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        notification_db->tone_volume = volume;

        if (notification_db->tone_volume_muted == false)
        {
            audio_path_dac_level_set(notification_db->tone_path_handle,
                                     notification_db->tone_volume,
                                     notification_db->tone_volume_scale);
        }
        break;

    case NOTIFICATION_TYPE_VP:
        notification_db->vp_volume = volume;

        if (notification_db->vp_volume_muted == false)
        {
            audio_path_dac_level_set(notification_db->vp_path_handle,
                                     notification_db->vp_volume,
                                     notification_db->vp_volume_scale);
        }
        break;

    case NOTIFICATION_TYPE_TTS:
        notification_db->tts_volume = volume;

        if (notification_db->tts_handle != NULL)
        {
            T_TTS_PARAM *tts_param;

            tts_param = notification_find_tts_param(notification_db->tts_handle);
            if (tts_param != NULL)
            {
                if (notification_db->tts_volume_muted == false)
                {
                    audio_path_dac_level_set(tts_param->handle,
                                             notification_db->tts_volume,
                                             notification_db->tts_volume_scale);
                }
            }
        }
        break;
    }

    return ret;
}

bool notification_volume_mute(uint8_t type)
{
    bool ret = true;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        ret = false;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        if (notification_db->tone_volume_muted == false)
        {
            notification_db->tone_volume_muted  = true;
            audio_path_dac_mute(notification_db->tone_path_handle);
        }
        else
        {
            ret = false;
        }
        break;

    case NOTIFICATION_TYPE_VP:
        if (notification_db->vp_volume_muted == false)
        {
            notification_db->vp_volume_muted = true;
            audio_path_dac_mute(notification_db->vp_path_handle);
        }
        else
        {
            ret = false;
        }
        break;

    case NOTIFICATION_TYPE_TTS:
        if (notification_db->tts_volume_muted == false)
        {
            notification_db->tts_volume_muted = true;

            if (notification_db->tts_handle != NULL)
            {
                T_TTS_PARAM *tts_param;

                tts_param = notification_find_tts_param(notification_db->tts_handle);
                if (tts_param != NULL)
                {
                    audio_path_dac_mute(tts_param->handle);
                }
            }
        }
        else
        {
            ret = false;
        }
        break;
    }

    return ret;
}

bool notification_volume_unmute(uint8_t type)
{
    bool ret = true;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        ret = false;
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        if (notification_db->tone_volume_muted == true)
        {
            notification_db->tone_volume_muted = false;
            audio_path_dac_level_set(notification_db->tone_path_handle,
                                     notification_db->tone_volume,
                                     notification_db->tone_volume_scale);
        }
        else
        {
            ret = false;
        }
        break;

    case NOTIFICATION_TYPE_VP:
        if (notification_db->vp_volume_muted == true)
        {
            notification_db->vp_volume_muted = false;
            audio_path_dac_level_set(notification_db->vp_path_handle,
                                     notification_db->vp_volume,
                                     notification_db->vp_volume_scale);
        }
        else
        {
            ret = false;
        }
        break;

    case NOTIFICATION_TYPE_TTS:
        if (notification_db->tts_volume_muted == true)
        {
            notification_db->tts_volume_muted = false;
            if (notification_db->tts_handle != NULL)
            {
                T_TTS_PARAM *tts_param;

                tts_param = notification_find_tts_param(notification_db->tts_handle);
                if (tts_param != NULL)
                {
                    audio_path_dac_level_set(tts_param->handle,
                                             notification_db->tts_volume,
                                             notification_db->tts_volume_scale);
                }
            }
        }
        else
        {
            ret = false;
        }
        break;
    }

    return ret;
}

float notification_volume_balance_get(uint8_t type)
{
    float scale = 0.0f;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        scale = notification_db->tone_volume_scale;
        break;

    case NOTIFICATION_TYPE_VP:
        scale = notification_db->vp_volume_scale;
        break;

    case NOTIFICATION_TYPE_TTS:
        scale = notification_db->tts_volume_scale;
        break;
    }

    return scale;
}

bool notification_volume_balance_set(uint8_t type, float scale)
{
    bool ret = false;

    switch (type)
    {
    case NOTIFICATION_TYPE_NONE:
        break;

    case NOTIFICATION_TYPE_RINGTONE:
        if (scale >= -1.0f && scale <= 1.0f)
        {
            notification_db->tone_volume_scale  = scale;
            audio_path_dac_level_set(notification_db->tone_path_handle,
                                     notification_db->tone_volume,
                                     notification_db->tone_volume_scale);
            ret = true;
        }
        break;

    case NOTIFICATION_TYPE_VP:
        if (scale >= -1.0f && scale <= 1.0f)
        {
            notification_db->vp_volume_scale    = scale;
            audio_path_dac_level_set(notification_db->vp_path_handle,
                                     notification_db->vp_volume,
                                     notification_db->vp_volume_scale);
            ret = true;
        }
        break;

    case NOTIFICATION_TYPE_TTS:
        if (scale >= -1.0f && scale <= 1.0f)
        {
            notification_db->tts_volume_scale   = scale;

            if (notification_db->tts_handle != NULL)
            {
                T_TTS_PARAM *tts_param;

                tts_param = notification_find_tts_param(notification_db->tts_handle);
                if (tts_param != NULL)
                {
                    audio_path_dac_level_set(tts_param->handle,
                                             notification_db->tts_volume,
                                             notification_db->tts_volume_scale);
                }
            }

            ret = true;
        }
        break;
    }

    return ret;
}

bool notification_sync_play(void *p_data)
{
    T_NOTIFICATION_ELEM elem;
    int32_t ret = 0;
    T_NOTIFICATION_MSG_RELAY_START *p_buf;
    p_buf = (T_NOTIFICATION_MSG_RELAY_START *)p_data;

    if (notification_db->state != NOTIFICATION_STATE_RELAY)
    {
        ret = 1;
        goto fail_error_state;
    }

    elem.index = p_buf->index;
    elem.type  = p_buf->notification_type;
    elem.relay = false;
    elem.address = p_buf->address;
    elem.len = p_buf->len;

    if (elem.type == NOTIFICATION_TYPE_VP)
    {
        elem.params.vp.language = p_buf->params.vp.language;
    }
    else if (elem.type == NOTIFICATION_TYPE_TTS)
    {
        elem.params.tts.handle = notification_db->tts_tbl[0].handle;
        elem.params.tts.cfg[0] = p_buf->params.tts.cfg[0];
        elem.params.tts.cfg[1] = p_buf->params.tts.cfg[1];
    }

    if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (elem.type == NOTIFICATION_TYPE_VP)
        {
            //add VOICE_PROMPT_INDEX
            notification_db->sync_action_type = NOTIFICATION_TYPE_VP;
            notification_db->sync_action_index = elem.index;
        }
        else if (elem.type == NOTIFICATION_TYPE_TTS)
        {
            notification_db->sync_action_type = NOTIFICATION_TYPE_TTS;
            notification_db->sync_action_index = elem.index;
        }
        else
        {
            notification_db->sync_action_type = NOTIFICATION_TYPE_RINGTONE;
            notification_db->sync_action_index = elem.index;
        }
    }
    else
    {
        notification_db->sync_action_index = 0xff;
    }

    AUDIO_PRINT_TRACE6("notification_sync_play: state %u, type %u, index %u, relay %u, addr 0x%x, len 0x%x",
                       notification_db->state, elem.type, elem.index, elem.relay, elem.address, elem.len);
    notification_path_start(&elem);

    return true;
fail_error_state:
    AUDIO_PRINT_ERROR1("notification_sync_play: ret %d", -ret);
    return false;
}

bool notification_flush(uint8_t type, bool relay)
{
    T_NOTIFICATION_ELEM        *curr_elem;
    T_NOTIFICATION_ELEM        *next_elem;
    T_NOTIFICATION_MSG_FLUSH    payload;

    AUDIO_PRINT_TRACE2("notification_flush: type %u, relay %u",
                       type, relay);

    curr_elem = os_queue_peek(&notification_db->play_queue, 0);
    while (curr_elem != NULL)
    {
        if (curr_elem->type == type)
        {
            next_elem = curr_elem->next;
            os_queue_delete(&notification_db->play_queue, curr_elem);
            os_queue_in(&notification_db->idle_queue, curr_elem);
            curr_elem = next_elem;
        }
        else
        {
            curr_elem = curr_elem->next;
        }
    }

    curr_elem = os_queue_peek(&notification_db->pending_queue, 0);
    while (curr_elem != NULL)
    {
        if (curr_elem->type == type)
        {
            next_elem = curr_elem->next;
            os_queue_delete(&notification_db->pending_queue, curr_elem);
            os_queue_in(&notification_db->idle_queue, curr_elem);
            curr_elem = next_elem;
        }
        else
        {
            curr_elem = curr_elem->next;
        }
    }

    payload.type  = type;
    payload.relay = relay;

    if (relay)
    {
        remote_async_msg_relay(notification_db->relay_handle,
                               NOTIFICATION_REMOTE_FLUSH,
                               &payload,
                               sizeof(T_NOTIFICATION_MSG_FLUSH),
                               false);
    }

    return true;
}

bool notification_cancel(uint8_t type, uint8_t index, bool relay)
{
    T_NOTIFICATION_ELEM       *curr_elem;
    T_NOTIFICATION_ELEM       *next_elem;
    T_NOTIFICATION_MSG_CANCEL  payload;

    AUDIO_PRINT_TRACE3("notification_cancel: type %u, index %u, relay %u",
                       type, index, relay);

    curr_elem = os_queue_peek(&notification_db->play_queue, 0);
    while (curr_elem != NULL)
    {
        if (curr_elem->type == type && curr_elem->index == index)
        {
            next_elem = curr_elem->next;
            os_queue_delete(&notification_db->play_queue, curr_elem);
            os_queue_in(&notification_db->idle_queue, curr_elem);
            curr_elem = next_elem;
        }
        else
        {
            curr_elem = curr_elem->next;
        }
    }

    curr_elem = os_queue_peek(&notification_db->pending_queue, 0);
    while (curr_elem != NULL)
    {
        if (curr_elem->type == type && curr_elem->index == index)
        {
            next_elem = curr_elem->next;
            os_queue_delete(&notification_db->pending_queue, curr_elem);
            os_queue_in(&notification_db->idle_queue, curr_elem);
            curr_elem = next_elem;
        }
        else
        {
            curr_elem = curr_elem->next;
        }
    }

    if (type == NOTIFICATION_TYPE_RINGTONE)
    {
        if (notification_db->state == NOTIFICATION_STATE_RINGTONE)
        {
            if (notification_db->tone_idx == index)
            {
                notification_stop(NOTIFICATION_TYPE_RINGTONE, NULL);
            }
        }
        else if (notification_db->state == NOTIFICATION_STATE_RELAY)
        {
            if (notification_db->relay_type == NOTIFICATION_TYPE_RINGTONE)
            {
                if (notification_db->sync_play_index == index)
                {
                    notification_stop(NOTIFICATION_TYPE_RINGTONE, NULL);
                }
            }
        }
    }
    else if (type == NOTIFICATION_TYPE_VP)
    {
        if (notification_db->state == NOTIFICATION_STATE_VP)
        {
            if (notification_db->vp_idx == index)
            {
                notification_stop(NOTIFICATION_TYPE_VP, NULL);
            }
        }
        else if (notification_db->state == NOTIFICATION_STATE_RELAY)
        {
            if (notification_db->relay_type == NOTIFICATION_TYPE_VP)
            {
                if (notification_db->sync_play_index == index)
                {
                    notification_stop(NOTIFICATION_TYPE_VP, NULL);
                }
            }
        }
    }

    payload.index = index;
    payload.type  = type;
    payload.relay = relay;

    if (relay)
    {
        remote_async_msg_relay(notification_db->relay_handle,
                               NOTIFICATION_REMOTE_CANCEL,
                               &payload,
                               sizeof(T_NOTIFICATION_MSG_CANCEL),
                               false);
    }

    return true;
}

bool notification_push(uint8_t type, uint8_t index, uint8_t language,
                       void *tts_handle, bool relay, uint32_t addr, uint32_t len)
{
    int32_t ret = 0;

    T_NOTIFICATION_ELEM *elem;

    elem = os_queue_out(&notification_db->idle_queue);
    if (elem == NULL)
    {
        ret = 1;
        goto fail_no_elem;
    }

    elem->index = index;
    elem->type  = type;
    elem->relay = relay;
    elem->address = addr;
    elem->len = len;

    if (type == NOTIFICATION_TYPE_VP)
    {
        elem->params.vp.language = language;
    }
    else if (type == NOTIFICATION_TYPE_TTS)
    {
        elem->params.tts.handle = tts_handle;
    }

    if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY &&
        remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED &&
        elem->relay == true &&
        elem->params.tts.handle == NULL)
    {
        /*
            sec may have relay type tone after roleswap.
            relay those tones to primary.
        */
        T_NOTIFICATION_MSG_PUSH_RELAY elem_msg;

        elem_msg.index = elem->index;
        elem_msg.type  = elem->type;
        elem_msg.language = elem->params.vp.language;
        elem_msg.address = elem->address;
        elem_msg.len = elem->len;

        if (remote_async_msg_relay(notification_db->relay_handle,
                                   NOTIFICATION_REMOTE_PUSH_RELAY,
                                   &elem_msg,
                                   sizeof(T_NOTIFICATION_MSG_PUSH_RELAY),
                                   false))
        {
            if (notification_get_state() != NOTIFICATION_STATE_RELAY &&
                notification_get_state() != NOTIFICATION_STATE_IDLE)
            {
                notification_lock();
            }
            else
            {
                if (notification_get_state() == NOTIFICATION_STATE_IDLE)
                {
                    sys_timer_start(notification_db->ack_time_handle);
                    notification_set_state(NOTIFICATION_STATE_RELAY);
                }
            }
            os_queue_in(&notification_db->idle_queue, elem);
            return true;
        }
    }

    os_queue_in(&notification_db->pending_queue, elem);
    AUDIO_PRINT_TRACE3("notification_push: pending queue num %u, play queue num %u, state %u",
                       notification_db->pending_queue.count, notification_db->play_queue.count, notification_db->state);
    if (notification_db->state == NOTIFICATION_STATE_IDLE)
    {
        notification_play();
    }
    else
    {
        elem = os_queue_out(&notification_db->pending_queue);
        os_queue_in(&notification_db->play_queue, elem);
    }

    return true;

fail_no_elem:
    AUDIO_PRINT_ERROR1("notification_push: ret %d", -ret);
    return false;
}

bool notification_stop(uint8_t type, void *handle)
{
    bool ret = false;

    if (type == NOTIFICATION_TYPE_RINGTONE)
    {
        if (notification_db->state == NOTIFICATION_STATE_RINGTONE)
        {
            notification_db->sync_action_type = NOTIFICATION_TYPE_NONE;
            notification_db->sync_action_index = 0xff;
            notification_db->sync_action_flag = 0;
            ret = audio_path_stop(notification_db->tone_path_handle);
        }
        else if (notification_db->state == NOTIFICATION_STATE_RELAY)
        {
            if (notification_db->relay_type == NOTIFICATION_TYPE_RINGTONE)
            {
                uint8_t tid = notification_db->sync_tid;
                notification_set_state(NOTIFICATION_STATE_IDLE);
                notification_set_relay_type(NOTIFICATION_TYPE_NONE);
                notification_db->sync_play_index = 0xff;
                notification_db->sync_tid++;

                if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
                {
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        ret = remote_async_msg_relay(notification_db->relay_handle,
                                                     NOTIFICATION_REMOTE_POLL_ABT,
                                                     &tid,
                                                     1,
                                                     false);
                        if (notification_get_pending_elem_num() != 0)
                        {
                            notification_play();
                        }
                    }
                }

                ret = true;
            }
        }
    }
    else if (type == NOTIFICATION_TYPE_VP)
    {
        if (notification_db->state == NOTIFICATION_STATE_VP)
        {
            notification_db->sync_action_type = NOTIFICATION_TYPE_NONE;
            notification_db->sync_action_index = 0xff;
            notification_db->sync_action_flag = 0;
            ret = audio_path_stop(notification_db->vp_path_handle);
        }
        else if (notification_db->state == NOTIFICATION_STATE_RELAY)
        {
            if (notification_db->relay_type == NOTIFICATION_TYPE_VP)
            {
                uint8_t tid = notification_db->sync_tid;
                notification_set_state(NOTIFICATION_STATE_IDLE);
                notification_set_relay_type(NOTIFICATION_TYPE_NONE);
                notification_db->sync_play_index = 0xff;
                notification_db->sync_tid++;
                if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
                {
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        ret = remote_async_msg_relay(notification_db->relay_handle,
                                                     NOTIFICATION_REMOTE_POLL_ABT,
                                                     &tid,
                                                     1,
                                                     false);
                        if (notification_get_pending_elem_num() != 0)
                        {
                            notification_play();
                        }
                    }
                }
                ret = true;
            }
        }
    }
    else if (type == NOTIFICATION_TYPE_TTS)
    {
        T_NOTIFICATION_ELEM *elem;

        elem = notification_find_tts_element(handle);
        if (elem != NULL)
        {
            if (os_queue_delete(&notification_db->play_queue, elem))
            {
                T_AUDIO_MSG_PAYLOAD_NOTIFICATION_STOP payload;

                payload.type   = NOTIFICATION_TYPE_TTS;
                payload.handle = handle;

                os_queue_in(&notification_db->idle_queue, elem);

                ret = audio_mgr_dispatch(AUDIO_MSG_NOTIFICATION_STOP, &payload);
            }
        }
        else
        {
            if (notification_db->state == NOTIFICATION_STATE_TTS)
            {
                ret = audio_path_stop(handle);
            }
        }
        notification_db->sync_tid++;
    }

    AUDIO_PRINT_TRACE5("notification_stop: state %d, type %u, handle %p, tid %d, ret %d",
                       notification_db->state, type, handle, notification_db->sync_tid, ret);

    return ret;
}

bool notification_write(uint8_t type, void *tts_handle, uint8_t *buf, uint16_t len)
{
    if (type == NOTIFICATION_TYPE_TTS)
    {
        if (notification_find_tts_param(tts_handle) != NULL)
        {
            return notification_buffer_push(notification_db->tts_buffer_type,
                                            (uint32_t)buf,
                                            0,
                                            len);
        }
    }

    return false;
}

void notification_relay_cback(T_NOTIFICATION_REMOTE_MSG event, T_REMOTE_RELAY_STATUS status,
                              void *buf, uint16_t len)
{
    AUDIO_PRINT_TRACE2("notification_relay_cback: event 0x%04x, status %u", event, status);

    switch (event)
    {
    case NOTIFICATION_REMOTE_ACK:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t tid = *(uint8_t *)buf;
                if (notification_db->sync_tid == tid)
                {
                    sys_timer_stop(notification_db->poll_time_handle);
                    if (notification_get_pending_elem_num() != 0)
                    {
                        if (notification_get_state() != NOTIFICATION_STATE_RELAY &&
                            notification_get_state() != NOTIFICATION_STATE_IDLE)
                        {
                            AUDIO_PRINT_WARN0("Notification state error");
                            notification_set_state(NOTIFICATION_STATE_IDLE);
                            remote_async_msg_relay(notification_db->relay_handle,
                                                   NOTIFICATION_REMOTE_ACK_ABT,
                                                   NULL,
                                                   0,
                                                   false);
                        }
                        else
                        {
                            notification_play();
                        }
                    }
                    else
                    {
                        AUDIO_PRINT_WARN0("Notification queue is empty");
                        notification_set_state(NOTIFICATION_STATE_IDLE);
                        notification_set_relay_type(NOTIFICATION_TYPE_NONE);
                        notification_db->sync_play_index = 0xff;
                        remote_async_msg_relay(notification_db->relay_handle,
                                               NOTIFICATION_REMOTE_ACK_ABT,
                                               NULL,
                                               0,
                                               false);
                    }
                }
                else
                {
                    AUDIO_PRINT_WARN2("Notification error tid local %d remote %d",
                                      notification_db->sync_tid,
                                      tid);
                }
            }
            else if (status == REMOTE_RELAY_STATUS_SEND_FAILED)
            {
                sys_timer_stop(notification_db->ack_time_handle);
                notification_set_state(NOTIFICATION_STATE_IDLE);
                notification_set_relay_type(NOTIFICATION_TYPE_NONE);
                notification_db->sync_play_index = 0xff;
                notification_play();
            }
        }
        break;

    case NOTIFICATION_REMOTE_POLL:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                notification_db->sync_tid = *(uint8_t *)buf;
                if (notification_get_state() == NOTIFICATION_STATE_RELAY)
                {
                    notification_unlock();
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
                    {
                        sys_timer_stop(notification_db->poll_time_handle);
                        if (remote_async_msg_relay(notification_db->relay_handle,
                                                   NOTIFICATION_REMOTE_ACK,
                                                   &notification_db->sync_tid,
                                                   1,
                                                   false))
                        {
                            sys_timer_start(notification_db->ack_time_handle);
                        }
                    }
                }
                else if (notification_get_state() == NOTIFICATION_STATE_IDLE)
                {
                    notification_unlock();
                    if (remote_async_msg_relay(notification_db->relay_handle,
                                               NOTIFICATION_REMOTE_ACK,
                                               &notification_db->sync_tid,
                                               1,
                                               false))
                    {
                        notification_set_state(NOTIFICATION_STATE_RELAY);
                        sys_timer_start(notification_db->ack_time_handle);
                    }
                }
                else
                {
                    notification_lock();
                }
            }
            else if (status == REMOTE_RELAY_STATUS_SEND_FAILED)
            {
                sys_timer_stop(notification_db->poll_time_handle);
                notification_set_state(NOTIFICATION_STATE_IDLE);
                notification_set_relay_type(NOTIFICATION_TYPE_NONE);
                notification_db->sync_play_index = 0xff;
                notification_play();
            }
        }
        break;

    case NOTIFICATION_REMOTE_FLUSH:
        {
            T_NOTIFICATION_MSG_FLUSH *payload = (T_NOTIFICATION_MSG_FLUSH *)buf;

            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                notification_flush(payload->type, false);
            }
        }
        break;

    case NOTIFICATION_REMOTE_CANCEL:
        {
            T_NOTIFICATION_MSG_CANCEL *payload = (T_NOTIFICATION_MSG_CANCEL *)buf;

            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                notification_cancel(payload->type, payload->index, false);
            }
        }
        break;

    case NOTIFICATION_REMOTE_POLL_ABT:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t state = notification_get_state();
                notification_unlock();
                if (state != NOTIFICATION_STATE_IDLE)
                {
                    if (state == NOTIFICATION_STATE_RELAY)
                    {
                        notification_set_state(NOTIFICATION_STATE_IDLE);
                        notification_set_relay_type(NOTIFICATION_TYPE_NONE);
                        notification_db->sync_play_index = 0xff;
                        if (notification_get_pending_elem_num() != 0)
                        {
                            notification_play();
                        }
                    }
                    else if (state == NOTIFICATION_STATE_VP)
                    {
                        uint8_t tid = *(uint8_t *)buf;
                        if (notification_db->sync_tid == tid)
                        {
                            notification_db->sync_action_type = NOTIFICATION_TYPE_NONE;
                            notification_db->sync_action_index = 0xff;
                            notification_db->sync_action_flag = 0;
                            audio_path_stop(notification_db->vp_path_handle);
                        }
                    }
                    else if (state == NOTIFICATION_STATE_RINGTONE)
                    {
                        uint8_t tid = *(uint8_t *)buf;
                        if (notification_db->sync_tid == tid)
                        {
                            notification_db->sync_action_type = NOTIFICATION_TYPE_NONE;
                            notification_db->sync_action_index = 0xff;
                            notification_db->sync_action_flag = 0;
                            audio_path_stop(notification_db->tone_path_handle);
                        }
                    }
                }
            }
        }
        break;

    case NOTIFICATION_REMOTE_ACK_ABT:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                sys_timer_stop(notification_db->ack_time_handle);
                if (notification_get_state() != NOTIFICATION_STATE_IDLE)
                {
                    if (notification_get_state() == NOTIFICATION_STATE_RELAY)
                    {
                        notification_set_state(NOTIFICATION_STATE_IDLE);
                        notification_set_relay_type(NOTIFICATION_TYPE_NONE);
                        notification_db->sync_play_index = 0xff;
                    }
                    if (notification_get_pending_elem_num() != 0)
                    {
                        notification_play();
                    }
                }
            }
        }
        break;

    case NOTIFICATION_REMOTE_PUSH_RELAY:
        {
            T_NOTIFICATION_MSG_PUSH_RELAY *p_cmd = (T_NOTIFICATION_MSG_PUSH_RELAY *)buf;
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                notification_push(p_cmd->type, p_cmd->index, p_cmd->language, NULL, true, p_cmd->address,
                                  p_cmd->len);
            }
        }
        break;

    case NOTIFICATION_REMOTE_START:
        {
            T_NOTIFICATION_MSG_RELAY_START *payload = (T_NOTIFICATION_MSG_RELAY_START *)buf;

            if (status == REMOTE_RELAY_STATUS_SYNC_RCVD)
            {
                notification_db->sync_play_index = payload->index;
                notification_set_relay_type(payload->notification_type);
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_SENT_OUT)
            {
                notification_db->sync_action_index = payload->index;
            }
            else if (status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                     status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                     status == REMOTE_RELAY_STATUS_SYNC_TOUT)
            {
                sys_timer_stop(notification_db->poll_time_handle);
                sys_timer_stop(notification_db->ack_time_handle);

                if (payload->tid == notification_db->sync_tid)
                {
                    notification_sync_play(buf);
                }
                else
                {
                    AUDIO_PRINT_WARN2("Notification error tid local %d remote %d",
                                      notification_db->sync_tid,
                                      payload->tid);
                }
            }
        }
        break;

    case NOTIFICATION_REMOTE_CLK_REQ:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t tid = *(uint8_t *)buf;

                AUDIO_PRINT_TRACE2("audio_mgr_relay_cback: report dsp ready, ck fg = %x, keep index %x",
                                   notification_db->sync_action_flag,
                                   notification_db->sync_action_index);

                if (tid != notification_db->sync_tid)
                {
                    AUDIO_PRINT_WARN2("Notification error tid local %d remote %d",
                                      notification_db->sync_tid,
                                      tid);
                    break;
                }
                if (notification_db->sync_action_index != 0xff)
                {

                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
                        remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        notification_db->sync_action_flag |= 0x0f;
                        if (notification_db->sync_action_flag == 0xff)
                        {
                            T_NOTIFICATION_MSG_SYNC_ACTION cmd;
                            T_BT_CLK_REF clk_ref;
                            uint32_t bb_clock_slot;
                            uint16_t bb_clock_us;

                            clk_ref = bt_piconet_clk_get(BT_CLK_NONE, &bb_clock_slot, &bb_clock_us);
                            cmd.init_clk = bb_clock_slot;
                            if (clk_ref == BT_CLK_NONE)
                            {
                                bb_clock_slot = 0xffffffff;
                            }
                            else
                            {
                                bb_clock_slot += (90 * 2 * 1000 / 625); // ms to bt clk
                                bb_clock_slot &= 0x0fffffff;
                            }
                            cmd.sync_clk = bb_clock_slot;
                            cmd.clk_ref = clk_ref;
                            cmd.tid = notification_db->sync_tid;

                            if (clk_ref == BT_CLK_CTRL)
                            {
                                uint8_t index;
                                bt_piconet_id_get(BT_CLK_CTRL, &index, &cmd.ref_role);
                            }

                            if (remote_sync_msg_relay(notification_db->relay_handle,
                                                      NOTIFICATION_REMOTE_SYNC_ACTION,
                                                      &cmd,
                                                      sizeof(T_NOTIFICATION_MSG_SYNC_ACTION),
                                                      REMOTE_TIMER_HIGH_PRECISION,
                                                      70,
                                                      false) == false)

                            {
                                notification_start(false, BT_CLK_NONE, 0xffffffff);
                                notification_db->sync_action_index = 0xff;
                                notification_db->sync_action_flag = 0;
                                AUDIO_PRINT_WARN0("audio_mgr_dispatch: sync action send fail");
                            }
                        }
                    }
                }
            }
        }
        break;

    case NOTIFICATION_REMOTE_SYNC_ACTION:
        {
            AUDIO_PRINT_TRACE2("audio_mgr_relay_cback: sync vp action status %d, vp idx %d", status,
                               notification_db->sync_action_index);
            if (notification_db->sync_action_index != 0xff)
            {
                T_NOTIFICATION_MSG_SYNC_ACTION *p_msg = (T_NOTIFICATION_MSG_SYNC_ACTION *)buf;
                if (p_msg->tid != notification_db->sync_tid)
                {
                    AUDIO_PRINT_WARN2("Notification error tid local %d remote %d",
                                      notification_db->sync_tid,
                                      p_msg->tid);
                    break;
                }
                if (status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED)
                {
                    T_BT_CLK_REF clk_ref;
                    uint32_t bb_clock_slot;
                    uint32_t bb_clock_rcv;
                    uint16_t bb_clock_us;

                    bb_clock_rcv = p_msg->sync_clk;
                    clk_ref = bt_piconet_clk_get(BT_CLK_CTRL, &bb_clock_slot, &bb_clock_us);

                    if (clk_ref == BT_CLK_CTRL
                        &&
                        bt_clk_compare(bb_clock_rcv, (bb_clock_slot + SYNC_PLAY_NOTI_PROTECT_MARGIN * 2 * 1000 / 625) &
                                       0x0fffffff)
                        &&
                        bt_clk_compare(bb_clock_slot, p_msg->init_clk)
                       )
                    {
                        uint8_t role;
                        uint8_t index;

                        bt_piconet_id_get(BT_CLK_CTRL, &index, &role);
                        if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
                        {
                            if (p_msg->ref_role == role)
                            {
                                AUDIO_PRINT_WARN0("Sec b2b link role has been changed while playing notification");
                                notification_start(false, BT_CLK_NONE, 0xffffffff);
                            }
                            else
                            {
                                notification_start(true, clk_ref, bb_clock_rcv);
                            }
                        }
                        else
                        {
                            if (p_msg->ref_role != role)
                            {
                                AUDIO_PRINT_WARN0("Pri b2b link role has been changed while playing notification");
                                notification_start(false, BT_CLK_NONE, 0xffffffff);
                            }
                            else
                            {
                                notification_start(true, clk_ref, bb_clock_rcv);
                            }
                        }
                    }
                    else
                    {
                        notification_start(false, BT_CLK_NONE, 0xffffffff);
                    }
                    notification_db->sync_action_index = 0xff;
                    notification_db->sync_action_flag = 0;
                }
                else if (status == REMOTE_RELAY_STATUS_SYNC_EXPIRED || status == REMOTE_RELAY_STATUS_SYNC_TOUT)
                {
                    T_BT_CLK_REF clk_ref;
                    uint32_t bb_clock_slot;
                    uint32_t bb_clock_rcv;
                    uint16_t bb_clock_us;

                    bb_clock_rcv = p_msg->sync_clk;
                    clk_ref = bt_piconet_clk_get((T_BT_CLK_REF)(p_msg->clk_ref), &bb_clock_slot, &bb_clock_us);

                    if (clk_ref != BT_CLK_NONE
                        &&
                        bt_clk_compare(bb_clock_rcv, (bb_clock_slot + SYNC_PLAY_NOTI_PROTECT_MARGIN * 2 * 1000 / 625) &
                                       0x0fffffff)
                       )
                    {
                        if (clk_ref == BT_CLK_CTRL)
                        {
                            uint8_t role;
                            uint8_t index;

                            bt_piconet_id_get(BT_CLK_CTRL, &index, &role);
                            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
                            {
                                if (p_msg->ref_role == role)
                                {
                                    AUDIO_PRINT_WARN0("Sec b2b link role has been changed while playing notification");
                                    notification_start(false, BT_CLK_NONE, 0xffffffff);
                                }
                                else
                                {
                                    notification_start(true, clk_ref, bb_clock_rcv);
                                }
                            }
                            else
                            {
                                if (p_msg->ref_role != role)
                                {
                                    AUDIO_PRINT_WARN0("Pri b2b link role has been changed while playing notification");
                                    notification_start(false, BT_CLK_NONE, 0xffffffff);
                                }
                                else
                                {
                                    notification_start(true, clk_ref, bb_clock_rcv);
                                }
                            }
                        }
                        else
                        {
                            notification_start(true, clk_ref, bb_clock_rcv);
                        }
                    }
                    else
                    {
                        clk_ref = bt_piconet_clk_get(BT_CLK_CTRL, &bb_clock_slot, &bb_clock_us);

                        if (clk_ref == BT_CLK_CTRL
                            &&
                            bt_clk_compare(bb_clock_rcv, (bb_clock_slot + SYNC_PLAY_NOTI_PROTECT_MARGIN * 2 * 1000 / 625) &
                                           0x0fffffff)
                            &&
                            bt_clk_compare(bb_clock_slot, p_msg->init_clk)
                           )
                        {
                            AUDIO_PRINT_WARN4("VP_clk_err_acceptable: clk ref %u sync clk %u local %u + margin %u", clk_ref,
                                              bb_clock_rcv,
                                              bb_clock_slot, SYNC_PLAY_NOTI_PROTECT_MARGIN * 2 * 1000 / 625);
                            notification_start(false, clk_ref, bb_clock_rcv);
                        }
                        else
                        {
                            notification_start(false, BT_CLK_NONE, 0xffffffff);
                        }
                    }
                    notification_db->sync_action_index = 0xff;
                    notification_db->sync_action_flag = 0;
                }
            }
        }
        break;

    default:
        break;
    }
}

bool notification_init(uint16_t pool_size)
{
    uint8_t i;
    int32_t ret = 0;

    notification_db = os_mem_zalloc2(sizeof(T_NOTIFICATION_DB));
    if (notification_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    notification_db->tone_path_handle = audio_path_create(AUDIO_CATEGORY_TONE,
                                                          AUDIO_DEVICE_OUT_DEFAULT,
                                                          NULL,
                                                          AUDIO_STREAM_MODE_NORMAL,
                                                          NOTIFICATION_VOLUME_DEFAULT,
                                                          0,
                                                          audio_path_composite_cback);
    if (notification_db->tone_path_handle == NULL)
    {
        ret = 2;
        goto fail_create_composite_path;
    }

    if (pool_size != 0)
    {
        if (notification_buffer_init(pool_size) == false)
        {
            ret = 3;
            goto fail_init_vp_buffer;
        }
    }


    notification_db->vp_path_handle = audio_path_create(AUDIO_CATEGORY_VP,
                                                        AUDIO_DEVICE_OUT_DEFAULT,
                                                        NULL,
                                                        AUDIO_STREAM_MODE_NORMAL,
                                                        NOTIFICATION_VOLUME_DEFAULT,
                                                        0,
                                                        audio_path_voice_prompt_cback);
    if (notification_db->vp_path_handle == NULL)
    {
        ret = 4;
        goto fail_create_voice_prompt;
    }

    notification_db->relay_handle = remote_relay_register(notification_relay_cback);
    if (notification_db->relay_handle == NULL)
    {
        ret = 5;
        goto fail_register_remote_relay;
    }


    notification_db->poll_time_handle = sys_timer_create("notification_poll",
                                                         SYS_TIMER_TYPE_LOW_PRECISION,
                                                         NOTIFICATION_TIMER_POLL_EXPIRED << 16,
                                                         NOTIFICATION_REMOTE_POLL_TIMEOUT,
                                                         false,
                                                         notification_timeout_cback);
    if (notification_db->poll_time_handle == NULL)
    {
        ret = 6;
        goto fail_init_poll_timer;
    }

    notification_db->ack_time_handle = sys_timer_create("notification_ack",
                                                        SYS_TIMER_TYPE_LOW_PRECISION,
                                                        NOTIFICATION_TIMER_ACK_EXPIRED << 16,
                                                        NOTIFICATION_REMOTE_ACK_TIMEOUT,
                                                        false,
                                                        notification_timeout_cback);
    if (notification_db->ack_time_handle == NULL)
    {
        ret = 7;
        goto fail_init_ack_timer;
    }

    os_queue_init(&notification_db->idle_queue);
    os_queue_init(&notification_db->pending_queue);
    os_queue_init(&notification_db->play_queue);

    for (i = 0; i < NOTIFICATION_QUEUE_NUM; ++i)
    {
        os_queue_in(&notification_db->idle_queue, &notification_db->elements[i]);
    }

    notification_db->tone_mode          = RINGTONE_MODE_AUDIBLE;
    notification_db->vp_mode            = VOICE_PROMPT_MODE_AUDIBLE;

    notification_db->vp_buffer_type     = NOTIFICATION_BUFFER_TYPE_STREAM;
    notification_db->tts_buffer_type    = NOTIFICATION_BUFFER_TYPE_FRAME;

    notification_db->tone_idx           = 0xFF;
    notification_db->vp_idx             = 0xFF;
    notification_db->tts_handle         = NULL;

    notification_db->tone_volume_max    = NOTIFICATION_VOLUME_MAX;
    notification_db->tone_volume_min    = NOTIFICATION_VOLUME_MIN;
    notification_db->tone_volume_muted  = false;
    notification_db->tone_volume        = NOTIFICATION_VOLUME_DEFAULT;
    notification_db->tone_volume_scale  = 0.0f;

    notification_db->vp_volume_max      = NOTIFICATION_VOLUME_MAX;
    notification_db->vp_volume_min      = NOTIFICATION_VOLUME_MIN;
    notification_db->vp_volume_muted    = false;
    notification_db->vp_volume          = NOTIFICATION_VOLUME_DEFAULT;
    notification_db->vp_volume_scale    = 0.0f;

    notification_db->tts_volume_max     = NOTIFICATION_VOLUME_MAX;
    notification_db->tts_volume_min     = NOTIFICATION_VOLUME_MIN;
    notification_db->tts_volume_muted   = false;
    notification_db->tts_volume         = NOTIFICATION_VOLUME_DEFAULT;
    notification_db->tts_volume_scale   = 0.0f;

    notification_db->sync_action_index  = 0xFF;
    notification_db->sync_action_type   = 0;
    notification_db->sync_action_flag   = 0;
    notification_db->flow_go            = true;

    return true;

fail_init_ack_timer:
fail_init_poll_timer:
fail_register_remote_relay:
fail_create_voice_prompt:
    notification_buffer_deinit();
fail_init_vp_buffer:
    audio_path_destory(notification_db->tone_path_handle);
fail_create_composite_path:
    os_mem_free(notification_db);
    notification_db = NULL;
fail_alloc_db:
    AUDIO_PRINT_ERROR1("notification_init: failed %d", -ret);
    return false;
}

void notification_deinit(void)
{
    T_NOTIFICATION_ELEM *elem;

    if (notification_db != NULL)
    {
        elem = os_queue_out(&notification_db->idle_queue);
        while (elem != NULL)
        {
            elem = os_queue_out(&notification_db->idle_queue);
        }

        elem = os_queue_out(&notification_db->pending_queue);
        while (elem != NULL)
        {
            elem = os_queue_out(&notification_db->pending_queue);
        }

        elem = os_queue_out(&notification_db->play_queue);
        while (elem != NULL)
        {
            elem = os_queue_out(&notification_db->play_queue);
        }

        if (notification_db->vp_path_handle != NULL)
        {
            audio_path_destory(notification_db->vp_path_handle);
        }

        if (notification_db->tone_path_handle != NULL)
        {
            audio_path_destory(notification_db->tone_path_handle);
        }

        notification_buffer_deinit();

        os_mem_free(notification_db);
        notification_db = NULL;
    }
}

T_TTS_HANDLE notification_tts_alloc(uint32_t length, uint16_t frame_cnt, uint32_t cfg)
{
    T_TTS_PARAM *tts_param;
    T_TTS_HANDLE handle = NULL;
    uint8_t i;

    for (i = 0; i < NOTIFICATION_TTS_NUM; ++i)
    {
        if (notification_db->tts_tbl[i].handle == NULL)
        {
            tts_param = &notification_db->tts_tbl[i];

            tts_param->length      = length;
            tts_param->frame_cnt   = frame_cnt;
            tts_param->cfg[0]      = cfg;
            tts_param->cfg[1]      = 0xffffffff;

            tts_param->handle = audio_path_create(AUDIO_CATEGORY_VP,
                                                  AUDIO_DEVICE_OUT_DEFAULT,
                                                  NULL,
                                                  AUDIO_STREAM_MODE_NORMAL,
                                                  notification_db->tts_volume,
                                                  0,
                                                  audio_path_tts_cback);
            handle = tts_param->handle;

            break;
        }
    }

    AUDIO_PRINT_TRACE4("notification_tts_alloc: handle %p, length %u, frame_cnt %u, cfg 0x%08x",
                       handle, length, frame_cnt, cfg);

    return handle;
}

bool notification_tts_free(T_TTS_HANDLE handle)
{
    if (handle != NULL)
    {
        T_TTS_PARAM *tts_param;

        tts_param = notification_find_tts_param(handle);
        if (tts_param != NULL)
        {
            audio_path_destory(tts_param->handle);
            tts_param->handle = NULL;
            return true;
        }
    }

    return false;
}

uint8_t notification_get_pending_elem_num(void)
{
    uint8_t num = 0;
    num = notification_db->pending_queue.count + notification_db->play_queue.count;
    return num;
}

uint8_t notification_remaining_count_get(uint8_t type)
{

    T_NOTIFICATION_ELEM                     *elem;
    uint8_t count = 0;

    elem = os_queue_peek(&notification_db->play_queue, 0);
    while (elem != NULL)
    {
        if (elem->type == type)
        {
            count++;
        }
        elem = elem->next;
    }

    elem = os_queue_peek(&notification_db->pending_queue, 0);
    while (elem != NULL)
    {
        if (elem->type == type)
        {
            count++;
        }
        elem = elem->next;
    }

    if (notification_db->state == NOTIFICATION_STATE_RINGTONE)
    {
        if (type == NOTIFICATION_TYPE_RINGTONE)
        {
            count++;
        }
    }
    else if (notification_db->state == NOTIFICATION_STATE_VP)
    {
        if (type == NOTIFICATION_TYPE_VP)
        {
            count++;
        }
    }
    else if (notification_db->state == NOTIFICATION_STATE_TTS)
    {
        if (type == NOTIFICATION_TYPE_TTS)
        {
            count++;
        }
    }
    else if (notification_db->state == NOTIFICATION_STATE_RELAY)
    {
        if (type == notification_db->relay_type)
        {
            count++;
        }
    }

    return count;
}

void notification_sync_lost(void)
{
    notification_unlock();
    if (notification_get_state() == NOTIFICATION_STATE_RELAY)
    {
        sys_timer_stop(notification_db->poll_time_handle);
        sys_timer_stop(notification_db->ack_time_handle);
        notification_set_state(NOTIFICATION_STATE_IDLE);
        notification_set_relay_type(NOTIFICATION_TYPE_NONE);
        notification_db->sync_play_index = 0xff;
        notification_play();
    }
}
