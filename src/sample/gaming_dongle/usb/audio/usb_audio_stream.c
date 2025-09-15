#if F_APP_USB_UAC_SUPPORT && F_APP_USB_SUPPORT
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "ring_buffer.h"
#include "app_timer.h"
#include "section.h"
#include "usb_audio.h"
#include "usb_audio_stream.h"
#include "os_queue.h"
#include "app_usb_audio_wrapper.h"
#include "usb_msg.h"
#include "app_io_msg.h"
#include "app_src_policy.h"
#include "os_sync.h"
#include "app_timer.h"
#include "app_ipc.h"
#include "app_usb.h"

#if F_APP_CFU_FEATURE_SUPPORT
#include "app_common_cfu.h"
#endif

#if F_APP_USB_GIP_SUPPORT
#include "usb_gip.h"
#include "app_gip.h"
#endif

#include "app_downstream_encode.h"

#include "app_upstream_decode.h"

#if F_APP_PCM_SPLIT_0_5MS
#include "hw_tim.h"
#endif

#define MIN_PCM_NUM_ASK_DECODE  3

#define UAS_DBG_LOG_EN      0

#define UAS_OUT_POOL_SIZE       (64)
#define UAS_IN_POOL_SIZE        (64)

#if ENABLE_UAC2
#define EVT_PARAM_ARRAY_NUM     0x40
#else
#define EVT_PARAM_ARRAY_NUM     0x20
#endif

#define USB_ISO_PRINT_LOG_CNT   0x001F

#if F_APP_PCM_SPLIT_0_5MS
#define PCM_SPLIT_BUF_SIZE          1024
#define PCM_SIZE_PER_0_5MS          (UAC1_SPK_SAM_RATE_MAX / 1000 * (UAC1_SPK_BIT_RES / 8) * UAC1_SPK_CHAN_NUM / 2)
#define PCM_SPLIT_INTERVAL          500 //us
#endif

#define DRVTYPE2STREAMTYPE(drv_type) (((drv_type) == 1)?USB_AUDIO_STREAM_TYPE_OUT: \
                                      USB_AUDIO_STREAM_TYPE_IN)
typedef enum
{
    USB_AUDIO_STREAM_EVT_DEACTIVE,
    USB_AUDIO_STREAM_EVT_ACTIVE,
    USB_AUDIO_STREAM_EVT_DATA_XMIT,
    USB_AUDIO_STREAM_EVT_VOL_GET,
    USB_AUDIO_STREAM_EVT_VOL_SET,
    USB_AUDIO_STREAM_EVT_MUTE_SET,
    USB_AUDIO_STREAM_EVT_MUTE_GET,
    USB_AUDIO_STREAM_EVT_RES_SET,
    USB_AUDIO_STREAM_EVT_MAX,
} T_USB_AUDIO_STREAM_EVT;

typedef enum
{
    STREAM_STATE_IDLE,
    STREAM_STATE_INITED,
    STREAM_STATE_ACTIVE,
    STREAM_STATE_XMITING,
} T_AUDIO_STREAM_STATE;

typedef enum
{
    UAC1_DOWNSTREAM_DEBOUNCE,
    UAC1_UPSTREAM_DEBOUNCE,
    UAC2_DOWNSTREAM_DEBOUNCE,
    UAC2_UPSTREAM_DEBOUNCE,
} T_USB_AUDIO_TIME_ID;

typedef struct
{
    void *pipe;
    uint32_t opt;
} T_UAS_EVT_PARAM;

typedef union
{
    uint32_t d32;
    struct
    {
        uint32_t dir: 1;
        uint32_t evt_type: 7;
        uint32_t pipe: 8;
        uint32_t rsv: 16;
    } u;
} T_USB_AUDIO_STREAM_EVT_INFO;

typedef union
{
    uint32_t d32;
    struct
    {
        uint32_t dir: 1;
        uint32_t ual_id: 7;
        uint32_t pipe: 8;
        uint32_t rsv: 16;
    } u;
} T_USB_AUDIO_STREAM_ID;

typedef struct _ual
{
    struct _ual *p_next;
    uint32_t id;
    void *owner;
    void *handle;
//    T_UAS_FUNC *func;
} T_UAL;

typedef struct _usb_audio_stream
{
    struct _usb_audio_stream *p_next;
    void *handle;
    uint32_t pipe;
    T_STREAM_ATTR attr;
    T_AUDIO_STREAM_STATE state;

    T_UAS_CTRL ctrl;
    T_RING_BUFFER pool;
    uint16_t event_cnt;

    uint8_t ual_total;
    void *active_ual;
    T_OS_QUEUE ual_list;
} T_USB_AUDIO_STREAM;

typedef struct _usb_audio_stream_db
{
    T_OS_QUEUE streams[USB_AUDIO_STREAM_TYPE_MAX];
} T_USB_AUDIO_STREAM_DB;

typedef struct
{
    uint8_t label;
    uint8_t type;
    uint8_t active;
    uint8_t timer_id;
    uint8_t timer_idx;
    const char *name;
} T_UAC_DEBOUNCE_INFO;

static T_UAC_DEBOUNCE_INFO uac_debounce_info[] =
{
    {USB_AUDIO_STREAM_LABEL_1, USB_AUDIO_STREAM_TYPE_IN,  false, UAC1_UPSTREAM_DEBOUNCE,   0, "uac1_us_debounce"},
    {USB_AUDIO_STREAM_LABEL_1, USB_AUDIO_STREAM_TYPE_OUT, false, UAC1_DOWNSTREAM_DEBOUNCE, 0, "uac1_ds_debounce"},
#if ENABLE_UAC2
    {USB_AUDIO_STREAM_LABEL_2, USB_AUDIO_STREAM_TYPE_IN,  false, UAC2_UPSTREAM_DEBOUNCE,   0, "uac2_us_debounce"},
    {USB_AUDIO_STREAM_LABEL_2, USB_AUDIO_STREAM_TYPE_OUT, false, UAC2_DOWNSTREAM_DEBOUNCE, 0, "uac2_ds_debounce"},
#endif
};

static uint8_t usb_audio_timer_queue_id = 0;

T_USB_AUDIO_STREAM_DB uas_db;
const uint32_t sample_rate_table[8] = {8000, 16000, 32000, 44100, 48000, 88000, 96000, 192000};

static T_UAS_EVT_PARAM evt_param[EVT_PARAM_ARRAY_NUM];

#if F_APP_PCM_SPLIT_0_5MS
static T_HW_TIMER_HANDLE uac1_pcm_split_timer_handle = NULL;
static uint8_t *uac1_pcm_split_buf = NULL;
static uint16_t uac1_pcm_split_reamin_size = 0;
#endif

RAM_TEXT_SECTION
static void *usb_audio_stream_search(T_USB_AUDIO_STREAM_TYPE type, uint32_t label)
{

    T_USB_AUDIO_STREAM *stream = (T_USB_AUDIO_STREAM *)uas_db.streams[type].p_first;

    while (stream)
    {
        if (stream->pipe == label)
        {
            break;
        }
        stream = stream->p_next;
    }

    return stream;
}

RAM_TEXT_SECTION
static void *usb_audio_stream_get_by_id(uint32_t id)
{
    T_USB_AUDIO_STREAM_ID stream_id = {.d32 = id};
#if UAS_DBG_LOG_EN
    USB_PRINT_INFO2("stream_id.u.dir = %d, stream_id.u.pipe = %d", stream_id.u.dir, stream_id.u.pipe);
#endif
    return usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)stream_id.u.dir, stream_id.u.pipe);
}

RAM_TEXT_SECTION
uint32_t usb_audio_stream_get_data_len(uint32_t id)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_get_data_count(&(stream->pool));
}

uint32_t usb_audio_stream_data_peek(uint32_t id, void *buf, uint32_t len)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_peek(&(stream->pool), len, buf);
}

uint32_t usb_audio_stream_data_flush(uint32_t id, uint32_t len)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_remove(&(stream->pool), len);
}

uint32_t usb_audio_stream_data_write(uint32_t id, void *buf, uint32_t len)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_write(&(stream->pool), buf, len);
}

RAM_TEXT_SECTION
uint32_t usb_audio_stream_get_remaining_pool_size(uint32_t id)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);
    return ring_buffer_get_remaining_space(&(stream->pool)) ;
}

uint32_t usb_audio_stream_data_read(uint32_t id, void *buf, uint32_t len)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_read(&(stream->pool), len, buf);
}

void usb_audio_stream_buf_clear(uint32_t id)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);;
    ring_buffer_clear(&(stream->pool));
}

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
RAM_TEXT_SECTION
static int usb_audio_stream_pipe_xmit_out_feedback(T_USB_AUDIO_PIPES *pipe, void *buf, uint32_t len)
{
//    APP_PRINT_INFO2("usb_audio_stream_pipe_xmit_out_feedback, 0x%x-0x%x", buf, len);

    return app_usb_audio_feedback_ds(buf, len, pipe->label);
}
#endif

#if F_APP_PCM_SPLIT_0_5MS
static void usb_audio_uac1_pcm_split_timer_isr_cb(T_HW_TIMER_HANDLE handle)
{
    if (uac1_pcm_split_reamin_size > 0)
    {
        app_usb_audio_data_xmit_out(uac1_pcm_split_buf, uac1_pcm_split_reamin_size,
                                    USB_AUDIO_STREAM_LABEL_1);

        uac1_pcm_split_reamin_size = 0;
    }
}
#endif

RAM_TEXT_SECTION
static int usb_audio_stream_pipe_xmit_out(T_USB_AUDIO_PIPES *pipe, void *buf, uint32_t len)
{
#if UAS_DBG_LOG_EN
    T_USB_AUDIO_STREAM_TYPE stream_type = USB_AUDIO_STREAM_TYPE_OUT;
    T_USB_AUDIO_STREAM *stream =  usb_audio_stream_search(stream_type, pipe->label);

    if ((stream->event_cnt & USB_ISO_PRINT_LOG_CNT) == 0)
    {
        APP_PRINT_INFO3("usb_audio_stream_pipe_xmit_out, label 0x%x, len 0x%x, cnt:%d", pipe->label, len,
                        stream->event_cnt);
    }
    stream->event_cnt++;
#endif

#if F_APP_CFU_FEATURE_SUPPORT
    if (app_cfu_is_in_process())
    {
        return false;
    }
#endif

#if F_APP_PCM_SPLIT_0_5MS
    if (pipe->label == USB_AUDIO_STREAM_LABEL_1)
    {
        uint16_t read_len = len / 2;

        if (read_len < PCM_SPLIT_BUF_SIZE)
        {
            app_usb_audio_data_xmit_out(buf, read_len, USB_AUDIO_STREAM_LABEL_1);

            hw_timer_start(uac1_pcm_split_timer_handle);

            uac1_pcm_split_reamin_size = len - read_len;

            memcpy(uac1_pcm_split_buf, (uint8_t *)buf + read_len, uac1_pcm_split_reamin_size);
        }

        return true;
    }
#endif

    return app_usb_audio_data_xmit_out(buf, len, pipe->label);
}

RAM_TEXT_SECTION
static bool usb_audio_stream_pipe_xmit_in(T_USB_AUDIO_PIPES *pipe, void *buf, uint32_t len)
{
    bool ret = false;
    uint8_t cause = 0;
    uint16_t pcm_size = ring_buffer_get_data_count(&usb_us_pcm_ring_buf);
    uint16_t remain_pcm_size = pcm_size;

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (us_pkt_start_decode() && remain_pcm_size < PCM_PREQUEUE_SIZE)
    {
        T_IO_MSG io_msg;

        io_msg.type = IO_MSG_TYPE_US_PKT_DECODE;
        app_io_msg_send(&io_msg);
    }
#endif

    if (us_pcm_prequeue_ready() == false)
    {
        cause = 1;
        goto exit;
    }

    if (pcm_size >= len)
    {
        ring_buffer_read(&usb_us_pcm_ring_buf, len, buf);

        ret = true;
    }

    remain_pcm_size = ring_buffer_get_data_count(&usb_us_pcm_ring_buf);

exit:
    if (ret == false)
    {
        memset(buf, 0, len);

        APP_PRINT_TRACE1("usb_audio_stream_pipe_xmit_in empty: cause %d", cause);
    }
    else
    {
        APP_PRINT_TRACE2("usb_audio_stream_pipe_xmit_in: (%d -> %d)", pcm_size, remain_pcm_size);
    }

    return ret;
}

RAM_TEXT_SECTION
static bool trigger_evt(T_USB_AUDIO_PIPES *pipe, uint16_t msg_type, uint32_t param)
{
    static uint8_t toggle_idx = 0;
    T_IO_MSG gpio_msg;

    evt_param[toggle_idx].pipe = pipe;
    evt_param[toggle_idx].opt = param;

    gpio_msg.type = IO_MSG_TYPE_USB;
    gpio_msg.subtype = USB_MSG(USB_MSG_GROUP_IF_AUDIO, msg_type);
    gpio_msg.u.param = (uint32_t)&evt_param[toggle_idx];
    toggle_idx = (toggle_idx + 1) % EVT_PARAM_ARRAY_NUM;
    return app_io_msg_send(&gpio_msg);
}

static int trigger_evt_ctrl(T_USB_AUDIO_PIPES *pipe, T_USB_AUDIO_CTRL_EVT evt, uint8_t dir,
                            uint32_t param)
{
    T_USB_AUDIO_STREAM_EVT_INFO stream_evt;
    uint8_t evt_type = USB_AUDIO_STREAM_EVT_MAX;

    stream_evt.u.dir = DRVTYPE2STREAMTYPE(dir);
    stream_evt.u.pipe = pipe->label;

    switch (evt)
    {
    case USB_AUDIO_CTRL_EVT_ACTIVATE:
        {
            uint32_t s = os_lock();
            T_USB_AUDIO_STREAM *stream =  usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)stream_evt.u.dir,
                                                                  stream_evt.u.pipe);
            ring_buffer_clear(&stream->pool);
            os_unlock(s);

            evt_type = USB_AUDIO_STREAM_EVT_ACTIVE;
        }
        break;

    case USB_AUDIO_CTRL_EVT_DEACTIVATE:
        {
            evt_type = USB_AUDIO_STREAM_EVT_DEACTIVE;
        }
        break;

    case USB_AUDIO_CTRL_EVT_VOL_SET:
        {
            evt_type = USB_AUDIO_STREAM_EVT_VOL_SET;
        }
        break;

    case USB_AUDIO_CTRL_EVT_VOL_GET:
        {
            evt_type = USB_AUDIO_STREAM_EVT_VOL_GET;
        }
        break;

    case USB_AUDIO_CTRL_EVT_MUTE_GET:
        {
            evt_type = USB_AUDIO_STREAM_EVT_MUTE_GET;
        }
        break;

    case USB_AUDIO_CTRL_EVT_MUTE_SET:
        {
            evt_type = USB_AUDIO_STREAM_EVT_MUTE_SET;
        }
        break;

    default:
        break;
    }
    stream_evt.u.evt_type = evt_type;

    return trigger_evt(pipe, stream_evt.d32, param);
}

static T_UAC_DEBOUNCE_INFO *usb_audio_get_debounce_info(uint32_t label, uint8_t stream_type)
{
    uint8_t i = 0;
    T_UAC_DEBOUNCE_INFO *info = NULL;

    for (i = 0; i < sizeof(uac_debounce_info) / sizeof(T_UAC_DEBOUNCE_INFO); i++)
    {
        if (uac_debounce_info[i].label == label &&
            uac_debounce_info[i].type  == stream_type)
        {
            info = &uac_debounce_info[i];
            break;
        }
    }

    return info;
}

static bool usb_audio_handle_evt_debounce(uint32_t label, uint8_t evt_type, uint8_t stream_type)
{
    T_UAC_DEBOUNCE_INFO *debounce_info = usb_audio_get_debounce_info(label, stream_type);
    bool ret = false;
    bool debounce = false;

    APP_PRINT_TRACE3("usb_audio_handle_evt_debounce: label %d evt %d type %d", label, evt_type,
                     stream_type);

    if (debounce_info != NULL)
    {
        if (evt_type == USB_AUDIO_STREAM_EVT_ACTIVE)
        {
            debounce_info->active = true;

            if (debounce_info->timer_idx != 0)
            {
                app_stop_timer(&debounce_info->timer_idx);
            }
        }
        else // USB_AUDIO_STREAM_EVT_DEACTIVE
        {
            debounce = true;
            debounce_info->active = false;
        }

        if (debounce)
        {
            app_start_timer(&debounce_info->timer_idx, debounce_info->name,
                            usb_audio_timer_queue_id, debounce_info->timer_id,
                            0, false, 800);

            ret = true;
        }
    }
    else
    {
        APP_PRINT_ERROR0("debounce info not found!");
    }

    return ret;
}

static void usb_audio_timer_cback(uint8_t timer_id, uint16_t timer_chann)
{
    switch (timer_id)
    {
    case UAC1_DOWNSTREAM_DEBOUNCE:
    case UAC1_UPSTREAM_DEBOUNCE:
#if ENABLE_UAC2
    case UAC2_DOWNSTREAM_DEBOUNCE:
    case UAC2_UPSTREAM_DEBOUNCE:
#endif
        {
            uint8_t i = 0;
            T_UAC_DEBOUNCE_INFO *info = NULL;

            for (i = 0; i < sizeof(uac_debounce_info) / sizeof(T_UAC_DEBOUNCE_INFO); i++)
            {
                if (uac_debounce_info[i].timer_id == timer_id)
                {
                    info = &uac_debounce_info[i];
                    break;
                }
            }

            if (info != NULL)
            {
                app_stop_timer(&info->timer_idx);

                if (info->active)
                {
                    T_USB_AUDIO_STREAM *stream = usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)info->type,
                                                                         info->label);

                    app_usb_audio_active(info->label, info->type);
                    stream->event_cnt = 0;
                }
                else
                {
                    app_usb_audio_deactive(info->label, info->type);
                }
            }
        }
        break;

    default:
        break;
    }
}

void usb_audio_stream_evt_handle(uint8_t evt, uint32_t param)
{
    T_UAS_EVT_PARAM *evt_param = (T_UAS_EVT_PARAM *)param;
    uint32_t pipe_label = ((T_USB_AUDIO_PIPES *)evt_param->pipe)->label;
    T_USB_AUDIO_STREAM_EVT_INFO stream_evt = {.d32 = evt};
    uint8_t evt_type = stream_evt.u.evt_type;
    uint8_t stream_type = stream_evt.u.dir;
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)stream_type,
                                                         pipe_label);
    T_USB_AUDIO_PIPE_ATTR attr = {.content.d32 = evt_param->opt};
    bool handle = true;

    if (evt_type == USB_AUDIO_STREAM_EVT_ACTIVE ||
        evt_type == USB_AUDIO_STREAM_EVT_DEACTIVE)
    {
        if (usb_audio_handle_evt_debounce(pipe_label, evt_type, stream_type))
        {
            return;
        }
    }

    switch (evt_type)
    {
    case USB_AUDIO_STREAM_EVT_ACTIVE:
        {
            app_usb_audio_active(pipe_label, stream_type);
            stream->event_cnt = 0;
        }
        break;

    case USB_AUDIO_STREAM_EVT_DEACTIVE:
        {
            app_usb_audio_deactive(pipe_label, stream_type);
        }
        break;

    case USB_AUDIO_STREAM_EVT_DATA_XMIT:
        {
            handle = false;
            app_usb_audio_data_xmit_out_handle(pipe_label);
        }
        break;

    case USB_AUDIO_STREAM_EVT_MUTE_GET:
        {
        }
        break;

    case USB_AUDIO_STREAM_EVT_VOL_GET:
        {
            uint16_t type = attr.content.vol.type;
            if (type == USB_AUDIO_VOL_TYPE_CUR)
            {
                stream->ctrl.vol.cur = attr.content.vol.value;
            }
            else if (type == USB_AUDIO_VOL_TYPE_RANGE)
            {
                stream->ctrl.vol.range = attr.content.vol.value;
            }
            APP_PRINT_INFO2("USB_AUDIO_STREAM_EVT_VOL_GET, cur:0x%x, range:0x%x", stream->ctrl.vol.cur,
                            stream->ctrl.vol.range);
        }
        break;

    case USB_AUDIO_STREAM_EVT_VOL_SET:
        {
            uint16_t vol;
            vol = attr.content.vol.value;

#if F_APP_USB_GIP_SUPPORT
            if (app_gip_get_switch_mode())
            {
                // GIP send volume range 0~100; change to dac gain in dB
                vol = app_usb_get_dac_gain_by_level(vol);
            }
#endif

            APP_PRINT_INFO2("USB_AUDIO_STREAM_EVT_VOL_SET: type %d vol 0x%04x",
                            stream_type, vol);

            if (stream_type == USB_AUDIO_STREAM_TYPE_OUT)
            {
                app_usb_audio_set_vol(UAC_SPK_VOL, pipe_label, vol);
            }
            else
            {
                app_usb_audio_set_vol(UAC_MIC_VOL, pipe_label, vol);
            }
            if (attr.content.vol.type == USB_AUDIO_VOL_TYPE_CUR)
            {
                if (stream_type == USB_AUDIO_STREAM_TYPE_OUT)
                {
                    uint8_t is_max_volume = (vol - stream->ctrl.vol.range == 0) ? 1 : 0;
                    app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_DS_VOL_SET, &is_max_volume);
                }
            }
            else if (attr.content.vol.type == USB_AUDIO_VOL_TYPE_RES)
            {
                app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_SET_RES, NULL);
            }
        }
        break;

    case USB_AUDIO_STREAM_EVT_MUTE_SET:
        {
            uint32_t mute = attr.content.mute.value;
            stream->ctrl.mute = attr.content.mute.value;

            APP_PRINT_INFO2("USB_AUDIO_STREAM_EVT_MUTE_SET: type %d mute %d",
                            stream_type, stream->ctrl.mute);

            if (stream_type == USB_AUDIO_STREAM_TYPE_OUT)
            {
                app_usb_ds_mute_set((T_UAC_LABEL)pipe_label, mute);
            }
            else if (stream_type == USB_AUDIO_STREAM_TYPE_IN)
            {
                us_pipe_mute_set(mute);
            }
            app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_MUTE_CTRL, &mute);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle)
    {
        APP_PRINT_INFO4("usb_audio_stream_evt_handle, evt:0x%x, param:0x%x, label:%d, stream:0x%x",
                        evt_type, param, pipe_label, stream);
    }
}

static int usb_audio_stream_pipe_ctrl(struct _usb_audio_pipes *pipe, T_USB_AUDIO_CTRL_EVT evt,
                                      T_USB_AUDIO_PIPE_ATTR ctrl)
{
    return trigger_evt_ctrl(pipe, evt, ctrl.dir, ctrl.content.d32);
}

static T_USB_AUDIO_PIPES usb_audio_pipe =
{
    .label = USB_AUDIO_STREAM_LABEL_1,
    .ctrl = (USB_AUDIO_PIPE_CB_CTRL)usb_audio_stream_pipe_ctrl,
    .downstream = (USB_AUDIO_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_out,
    .upstream = (USB_AUDIO_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_in,
#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
    .feedback_d = (USB_AUDIO_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_out_feedback,
#else
    .feedback_d = NULL,
#endif
};

#if ENABLE_UAC2
static T_USB_AUDIO_PIPES usb_audio_pipe_2 =
{
    .label = USB_AUDIO_STREAM_LABEL_2,
    .ctrl = (USB_AUDIO_PIPE_CB_CTRL)usb_audio_stream_pipe_ctrl,
    .downstream = (USB_AUDIO_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_out,
    .upstream = (USB_AUDIO_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_in,
};
#endif

#if F_APP_USB_GIP_SUPPORT
static T_USB_GIP_PIPES usb_gip_pipe =
{
    .label = 1,
    .ctrl = (USB_GIP_PIPE_CB_CTRL)usb_audio_stream_pipe_ctrl,
    .downstream = (USB_GIP_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_out,
    .upstream = (USB_GIP_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_in,
};
#endif

RAM_TEXT_SECTION
void usb_audio_stream_data_trans_msg(uint32_t label)
{
    T_USB_AUDIO_STREAM_EVT_INFO stream_evt = {.u = {.dir = USB_AUDIO_STREAM_TYPE_OUT}};
    stream_evt.u.evt_type = USB_AUDIO_STREAM_EVT_DATA_XMIT;
    if (label == USB_AUDIO_STREAM_LABEL_1)
    {
        trigger_evt(&usb_audio_pipe, stream_evt.d32, (uint32_t)label);
    }
#if ENABLE_UAC2
    else if (label == USB_AUDIO_STREAM_LABEL_2)
    {
        trigger_evt(&usb_audio_pipe_2, stream_evt.d32, (uint32_t)label);
    }
#endif
}

uint16_t usb_audio_get_us_pcm_buf_data_size(void)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_search(USB_AUDIO_STREAM_TYPE_IN,
                                                         USB_AUDIO_STREAM_LABEL_1);

    return ring_buffer_get_data_count(&(stream->pool));
}

uint32_t usb_audio_stream_ual_bind(uint8_t stream_type, uint8_t label)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)stream_type, label);
    T_UAL *ual_node = NULL;
    T_USB_AUDIO_STREAM_ID id = {.u = {.dir = stream_type, .pipe = label}};
    uint8_t *buf = NULL;
    uint32_t pool_size = (stream_type == USB_AUDIO_STREAM_TYPE_OUT) ? UAS_OUT_POOL_SIZE :
                         UAS_IN_POOL_SIZE;

    if (stream == NULL)
    {
        stream = malloc(sizeof(T_USB_AUDIO_STREAM));
        memset(stream, 0, sizeof(T_USB_AUDIO_STREAM));
        stream->pipe = label;
        buf = (uint8_t *)malloc(pool_size);
        ring_buffer_init(&(stream->pool), buf, pool_size);
        os_queue_in(&uas_db.streams[stream_type], stream);
        os_queue_init(&(stream->ual_list));
    }
    stream->ual_total += 1;
    id.u.ual_id = stream->ual_total;

    ual_node = malloc(sizeof(T_UAL));
    ual_node->p_next = NULL;
    ual_node->handle = NULL;
    ual_node->owner = stream;
    ual_node->id = id.d32;
    os_queue_in(&(stream->ual_list), ual_node);

    APP_PRINT_INFO2("usb_audio_stream_ual_bind, type:0x%x, id:0x%x", stream_type, id.d32);

    return (id.d32);
}

void usb_audio_stream_init(void)
{
    memset(&uas_db, 0, sizeof(uas_db));
    usb_audio_init(&usb_audio_pipe);
#if ENABLE_UAC2
    usb_audio_sec_init(&usb_audio_pipe_2);
#endif

#if F_APP_USB_GIP_SUPPORT
    if (app_gip_get_switch_mode())
    {
        usb_gip_init(&usb_gip_pipe);
    }
    else
#endif
    {
        extern int usb_audio_driver_init(void);
        usb_audio_driver_init();
    }

    app_timer_reg_cb(usb_audio_timer_cback, &usb_audio_timer_queue_id);

#if F_APP_PCM_SPLIT_0_5MS
    uac1_pcm_split_timer_handle = hw_timer_create("uac1_pcm_split_timer_handle", PCM_SPLIT_INTERVAL,
                                                  false,
                                                  usb_audio_uac1_pcm_split_timer_isr_cb);

    uac1_pcm_split_buf = malloc(PCM_SPLIT_BUF_SIZE);

#endif
}
#endif
