#if F_APP_USB_AUDIO_SUPPORT
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hw_tim.h"
#include "trace.h"
#include "ring_buffer.h"
#include "app_timer.h"
#include "section.h"
#include "usb_audio.h"
#include "usb_audio_stream.h"
#include "usb_audio_config.h"
#include "usb_msg.h"
#include "app_io_msg.h"
#include "app_ipc.h"
#include "os_queue.h"
#include "os_sync.h"

#define USB_AUDIO_ACTIVE_DEBOUNCE    1
#if F_APP_GAMING_DONGLE_SUPPORT
#define USB_AUDIO_SILENCE_STREAM     1
#endif

#if USB_AUDIO_SILENCE_STREAM
#define USB_AUDIO_SILENCE_STREAM_TIME       300                                     // ms
#define USB_AUDIO_ACTIVE_DEBOUNCE_TIME      (USB_AUDIO_SILENCE_STREAM_TIME + 1000)  // ms
#define USB_AUDIO_STREAM_DETECT_VARIATION   800                                     // us
#define USB_AUDIO_DOWNSTREAM_INTERVAL       (USB_AUDIO_DS_INTERVAL * 1000)          // us
#define USB_AUDIO_STREAM_DETECT_TIMEOUT     (USB_AUDIO_DOWNSTREAM_INTERVAL + USB_AUDIO_STREAM_DETECT_VARIATION)
#else
#define USB_AUDIO_ACTIVE_DEBOUNCE_TIME      800                                     // ms
#endif

#define MONITOR_TIMER_INTERVAL_MS    100

#if TARGET_RTL8763EWM
#define UAS_OUT_POOL_SIZE       (2 * 1024)
#define UAS_IN_POOL_SIZE        (1 * 1024)
#else
#define UAS_OUT_POOL_SIZE       (3 * 1024)
#define UAS_IN_POOL_SIZE        (2 * 1024)
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

typedef enum {PENDING_ACTION_START, PENDING_ACTION_STOP} T_PENDING_ACTION;

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

typedef struct _pending
{
    void *owner;
    bool sync;
    bool (*action)(void *);
    T_AUDIO_STREAM_STATE target_state;
    T_AUDIO_STREAM_STATE next_state;
    void *opt;
} T_PENDING;

typedef struct _ual
{
    struct _ual *p_next;
    uint32_t id;
    void *owner;
    void *handle;
    T_UAS_FUNC *func;
} T_UAL;

typedef struct _usb_audio_stream
{
    struct _usb_audio_stream *p_next;
    void *handle;
    uint32_t pipe;
    T_STREAM_ATTR attr;
    T_AUDIO_STREAM_STATE state;

    uint8_t monitor_timer_idx;
    uint8_t stream_idle_id;

    T_UAS_CTRL ctrl;
    T_RING_BUFFER pool;

    // void *pkt;
    T_PENDING *pending;
    uint8_t ual_total;
    void *active_ual;
    T_OS_QUEUE ual_list;
} T_USB_AUDIO_STREAM;

typedef enum
{
    UAS_TIMER_MONITOR_EMPTY,
} T_USB_AUDIO_TIMER;

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

#if USB_AUDIO_ACTIVE_DEBOUNCE
static T_UAC_DEBOUNCE_INFO uac_debounce_info[] =
{
    {USB_AUDIO_STREAM_LABEL_1, USB_AUDIO_STREAM_TYPE_IN,  false, UAC1_UPSTREAM_DEBOUNCE,   0, "uac1_us_debounce"},
    {USB_AUDIO_STREAM_LABEL_1, USB_AUDIO_STREAM_TYPE_OUT, false, UAC1_DOWNSTREAM_DEBOUNCE, 0, "uac1_ds_debounce"},
};
#endif

static uint8_t usb_audio_timer_queue_id = 0;

T_USB_AUDIO_STREAM_DB uas_db;
const uint32_t sample_rate_table[8] = {8000, 16000, 32000, 44100, 48000, 88000, 96000, 192000};

static bool usb_audio_stream_ctrl(T_USB_AUDIO_STREAM *stream, uint8_t cmd, void *param);

#if USB_AUDIO_SILENCE_STREAM
static T_HW_TIMER_HANDLE usb_audio_stream_timer_handle = NULL;
static uint16_t uac_silence_cnt;
static T_USB_AUDIO_PIPES *uac_audio_pipe;
static uint8_t *uac_silence_buf;
static uint16_t uac_pcm_pkt_size;

static bool usb_audio_stream_xmit_handle(T_USB_AUDIO_PIPES *pipe, void *buf, uint32_t len);
static void usb_audio_stream_timer_isr_cb(T_HW_TIMER_HANDLE handle)
{
    uint32_t period = USB_AUDIO_DOWNSTREAM_INTERVAL;

    if (uac_silence_cnt < USB_AUDIO_SILENCE_STREAM_TIME * 1000 / period)
    {
        uac_silence_cnt++;

        if (uac_silence_cnt < 5)
        {
            period -= USB_AUDIO_STREAM_DETECT_VARIATION;
        }

        APP_PRINT_TRACE1("send silence pcm start: cnt %d", uac_silence_cnt);
        hw_timer_restart(usb_audio_stream_timer_handle, period);
    }
    else
    {
        APP_PRINT_TRACE0("send silence pcm stop");
        hw_timer_stop(usb_audio_stream_timer_handle);
    }

    if (uac_silence_buf != NULL)
    {
        usb_audio_stream_xmit_handle(uac_audio_pipe, uac_silence_buf, uac_pcm_pkt_size);
    }
}

static void usb_audio_stream_alloc_silence_stream(T_STREAM_ATTR attr)
{
    uint16_t sample_rate = attr.sample_rate;
    uint8_t  chan_num = attr.chann_num;
    uint8_t  bit_width = attr.bit_width;
    uint16_t pcm_pkt_size = USB_AUDIO_DOWNSTREAM_INTERVAL * (sample_rate / 1000) *
                            chan_num * (bit_width / 8) / 1000;

    if (usb_audio_stream_timer_handle == NULL)
    {
        usb_audio_stream_timer_handle = hw_timer_create("uac_stream_timer_handle",
                                                        USB_AUDIO_DS_INTERVAL * 1000, false,
                                                        usb_audio_stream_timer_isr_cb);
    }

    if (pcm_pkt_size != uac_pcm_pkt_size)
    {
        uac_pcm_pkt_size = pcm_pkt_size;

        if (uac_silence_buf)
        {
            free(uac_silence_buf);
        }
        uac_silence_buf = calloc(1, uac_pcm_pkt_size);
    }
}
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

static void *usb_audio_stream_get_by_id(uint32_t id)
{
    T_USB_AUDIO_STREAM_ID stream_id = {.d32 = id};
    return usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)stream_id.u.dir, stream_id.u.pipe);
}

static void pend_action(T_USB_AUDIO_STREAM *stream,  T_UAL *ual, T_PENDING_ACTION action,
                        bool sync)
{
    if (stream->pending == NULL)
    {
        stream->pending = malloc(sizeof(T_PENDING));
    }

    stream->pending->owner = ual;
    stream->pending->sync = sync;

    if (action == PENDING_ACTION_START)
    {
        stream->pending->action = ual->func->start;
        stream->pending->opt = ual;
        stream->pending->target_state = STREAM_STATE_INITED;
        stream->pending->next_state = STREAM_STATE_ACTIVE;
    }
    else if (action == PENDING_ACTION_STOP)
    {
        stream->pending->action = ual->func->stop;
        stream->pending->opt = NULL;
        stream->pending->target_state = STREAM_STATE_ACTIVE;
        stream->pending->next_state = STREAM_STATE_INITED;
    }
}

static void try_resume_action(T_USB_AUDIO_STREAM *stream)
{
    T_PENDING *pending = stream->pending;
    T_UAL *ual = NULL;

    if (pending && pending->owner)
    {
        if (stream->state == pending->target_state)
        {
            ual = (T_UAL *)pending->owner;

            if (pending->action)
            {
                if (pending->action(ual->handle) && pending->sync)
                {
                    stream->state = pending->next_state;
                    stream->active_ual = pending->opt;
                }
            }

            free(pending);
            stream->pending = NULL;
        }
    }
}

static void clear_pending(T_USB_AUDIO_STREAM *stream)
{
    T_PENDING *pending = stream->pending;

    if (pending)
    {
        free(pending);
        stream->pending = NULL;
    }
}

static void usb_audio_stream_state_set(T_USB_AUDIO_STREAM *stream, T_AUDIO_STREAM_STATE state)
{
    if (stream->state != state)
    {
        APP_PRINT_INFO2("usb_audio_stream_state_set, old:0x%x, new:0x%x", stream->state, state);
    }
    stream->state = state;

    try_resume_action(stream);
}

static T_AUDIO_STREAM_STATE usb_audio_stream_state_get(T_USB_AUDIO_STREAM *stream)
{
    return (stream->state);
}

static void *find_ual(T_USB_AUDIO_STREAM *stream, uint32_t id)
{
    T_UAL *ual = (T_UAL *)stream->ual_list.p_first;

    while (ual)
    {
        if (ual->id == id)
        {
            break;
        }
        ual = ual->p_next;
    }

    return ual;
}

static bool stream_create_all(T_USB_AUDIO_STREAM *stream, T_STREAM_ATTR attr)
{
    bool ret = true;
    T_UAL *ual = NULL;
    T_UAS_CTRL ctrl = stream->ctrl;

    ual = (T_UAL *)stream->ual_list.p_first;
    while (ual)
    {
        if (ual->func->create)
        {
            ual->handle = ual->func->create(attr, (void *)&ctrl);
            if (ual->handle == NULL)
            {
                ret = false;
            }
        }
        ual = ual->p_next;
    }

    usb_audio_stream_state_set(stream, STREAM_STATE_INITED);

    return ret;
}

static bool usb_audio_stream_create(T_USB_AUDIO_STREAM *stream, T_STREAM_ATTR attr)
{
    bool ret = true;

    if (stream == NULL)
    {
        APP_PRINT_ERROR0("usb_audio_stream_create failed ,stream NULL");
        return NULL;
    }

    if (usb_audio_stream_state_get(stream) != STREAM_STATE_IDLE)
    {
        APP_PRINT_ERROR1("usb_audio_stream_create failed ,state:0x%x", stream->state);
        return NULL;
    }

#if USB_AUDIO_SILENCE_STREAM
    usb_audio_stream_alloc_silence_stream(attr);
#endif

    ret = stream_create_all(stream, attr);

    return ret;
}

static bool stream_release_all(T_USB_AUDIO_STREAM *stream)
{
    bool ret = true;
    T_UAL *ual = NULL;

    ual = (T_UAL *)stream->ual_list.p_first;
    while (ual)
    {
        if (ual->func->release)
        {
            ret = ual->func->release(ual->handle);
            if (ret)
            {
                ual->handle = NULL;
            }
        }
        ual = ual->p_next;
    }

    usb_audio_stream_state_set(stream, STREAM_STATE_IDLE);

    return ret;
}

static bool usb_audio_stream_release(T_USB_AUDIO_STREAM *stream)
{
    bool ret = true;
    T_UAL *ual = NULL;
    T_AUDIO_STREAM_STATE state = STREAM_STATE_IDLE;

    if (stream == NULL)
    {
        APP_PRINT_ERROR0("usb_audio_stream_release failed ,stream NULL");
        return NULL;
    }

    state = usb_audio_stream_state_get(stream);
    if (state == STREAM_STATE_IDLE)
    {
        APP_PRINT_ERROR1("usb_audio_stream_release failed ,state:0x%x", stream->state);
        return NULL;
    }

    clear_pending(stream);

    ret = stream_release_all(stream);

    return ret;
}

bool usb_audio_stream_start(uint32_t id, bool sync)
{
    bool ret = true;
    T_UAL *ual = NULL;
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);
    T_AUDIO_STREAM_STATE state = STREAM_STATE_IDLE;

    if (stream == NULL)
    {
        APP_PRINT_ERROR0("usb_audio_stream_start failed ,stream NULL");
        return false;
    }

    ual = find_ual(stream, id);
    if (ual == NULL)
    {
        APP_PRINT_ERROR0("usb_audio_stream_start failed: ual NULL");
        return false;
    }

    state = usb_audio_stream_state_get(stream);
    if (state != STREAM_STATE_INITED)
    {
        APP_PRINT_ERROR1("usb_audio_stream_start failed, state: 0x%x", state);
        if (state == STREAM_STATE_IDLE)
        {
            pend_action(stream, ual, PENDING_ACTION_START, sync);
        }
        return false;
    }
    else
    {
        clear_pending(stream);
    }

    if (ual->handle && ual->func->start)
    {
        ret = ual->func->start(ual->handle);
        if (ret && sync)
        {
            usb_audio_stream_state_set(stream, STREAM_STATE_ACTIVE);
            stream->active_ual = ual;
        }
    }

    return ret;
}

void usb_audio_stream_start_complete(uint32_t id)
{
    T_UAL *ual = NULL;
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    if (usb_audio_stream_state_get(stream) == STREAM_STATE_INITED)
    {
        ual = find_ual(stream, id);

        usb_audio_stream_state_set(stream, STREAM_STATE_ACTIVE);
        stream->active_ual = ual;
        usb_audio_stream_ctrl(stream, CTRL_CMD_VOL_CHG, &stream->ctrl.vol);
        usb_audio_stream_ctrl(stream, CTRL_CMD_MUTE, &stream->ctrl.mute);
    }
}

bool usb_audio_stream_stop(uint32_t id, bool sync)
{
    bool ret = true;
    T_UAL *ual = NULL;
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);
    T_AUDIO_STREAM_STATE state = STREAM_STATE_IDLE;

    if (stream == NULL)
    {
        APP_PRINT_ERROR0("usb_audio_stream_stop failed ,stream NULL");
        return false;
    }

    ual = find_ual(stream, id);
    if (ual == NULL || ual != stream->active_ual)
    {
        return false;
    }

    state = usb_audio_stream_state_get(stream);
    if (state < STREAM_STATE_ACTIVE)
    {
        pend_action(stream, ual, PENDING_ACTION_STOP, sync);
        APP_PRINT_ERROR1("usb_audio_stream_stop failed ,state:0x%x", state);
        return false;
    }

    if (ual && ual->handle && ual->func->stop)
    {
        ret = ual->func->stop(ual->handle);
        if (ret)
        {
            usb_audio_stream_state_set(stream, STREAM_STATE_INITED);
            stream->active_ual = NULL;
        }
    }

    return ret;
}

void usb_audio_stream_stop_complete(uint32_t id)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    if (usb_audio_stream_state_get(stream) >= STREAM_STATE_ACTIVE)
    {
        usb_audio_stream_state_set(stream, STREAM_STATE_INITED);
        stream->active_ual = NULL;
    }
}

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

static bool usb_audio_stream_xmit(T_USB_AUDIO_STREAM *stream)
{
    bool ret = true;
    T_UAL *ual = NULL;
    T_AUDIO_STREAM_STATE state = STREAM_STATE_IDLE;

    if (stream == NULL)
    {
        APP_PRINT_ERROR0("usb_audio_stream_xmit fail, stream NULL");
        return false;
    }

    state = usb_audio_stream_state_get(stream);
    if (usb_audio_stream_state_get(stream) < STREAM_STATE_ACTIVE)
    {
        APP_PRINT_ERROR1("usb_audio_stream_xmit, fail, state:0x%x", state);
        return false;
    }

    usb_audio_stream_state_set(stream, STREAM_STATE_XMITING);

    ual = (T_UAL *)(stream->active_ual);
    if (ual && ual->handle && ual->func->xmit)
    {
        ret = ual->func->xmit(ual->handle);
    }
    return ret;
}

#if (F_APP_REDUCE_HEAP_USAGE == 0)
RAM_TEXT_SECTION
#endif
static bool usb_audio_stream_pipe_xmit_in(T_USB_AUDIO_PIPES *pipe, void *buf, uint32_t len)
{
    T_USB_AUDIO_STREAM_TYPE stream_type = USB_AUDIO_STREAM_TYPE_IN;
    T_USB_AUDIO_STREAM *stream =  usb_audio_stream_search(stream_type, pipe->label);
    uint32_t data_len = ring_buffer_get_data_count(&(stream->pool));

    APP_PRINT_INFO4("usb_audio_stream_pipe_xmit_in, 0x%x-0x%x-0x%x-0x%x", buf, len, stream, data_len);

    if (data_len >= len)
    {
        ring_buffer_read(&(stream->pool), len, buf);
    }
    else
    {
        memset(buf, 0, len);
        APP_PRINT_ERROR0("usb_audio_stream_pipe_xmit_in, us ring buf empty!");
        return false;
    }

    return true;
}

static bool usb_audio_stream_ctrl(T_USB_AUDIO_STREAM *stream, uint8_t cmd, void *param)
{
    bool ret = true;
    T_UAL *ual = NULL;

    if (stream == NULL)
    {
        APP_PRINT_ERROR0("usb_audio_stream_ctrl, stream NULL");
        return false;
    }

    ual = (T_UAL *)stream->active_ual;
    if (ual && ual->handle && ual->func->ctrl)
    {
        ret = ual->func->ctrl(ual->handle, cmd, param);
    }

    return ret;
}

static void usb_audio_stream_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    uint32_t pipe_label = (uint32_t)param;
    T_USB_AUDIO_STREAM *stream = (T_USB_AUDIO_STREAM *)
                                 uas_db.streams[USB_AUDIO_STREAM_TYPE_OUT].p_first;

    while (stream)
    {
        if (stream->pipe == pipe_label)
        {
            break;
        }
        stream = stream->p_next;
    }

    switch (timer_evt)
    {

    case UAS_TIMER_MONITOR_EMPTY:
        {
            app_stop_timer(&stream->monitor_timer_idx);

            app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_DS_STOP, NULL);

            usb_audio_stream_ctrl(usb_audio_stream_search(USB_AUDIO_STREAM_TYPE_OUT, param), CTRL_CMD_STOP, 0);

        }
        break;

    default:
        break;
    }
}

#define EVT_PARAM_ARRAY_NUM     0x20
static T_UAS_EVT_PARAM evt_param[EVT_PARAM_ARRAY_NUM];
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

RAM_TEXT_SECTION
static bool usb_audio_stream_xmit_handle(T_USB_AUDIO_PIPES *pipe, void *buf, uint32_t len)
{
    uint32_t label = pipe->label;
    T_USB_AUDIO_STREAM_TYPE stream_type = USB_AUDIO_STREAM_TYPE_OUT;
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_search(stream_type, label);
    T_USB_AUDIO_STREAM_EVT_INFO stream_evt = {.u = {.dir = stream_type}};
    stream_evt.u.evt_type = USB_AUDIO_STREAM_EVT_DATA_XMIT;
    if (len % (stream->attr.bit_width / 8 * stream->attr.chann_num) != 0)
    {
        return false;
    }

    if (ring_buffer_get_remaining_space(&(stream->pool)) >= len)
    {
        ring_buffer_write(&(stream->pool), buf, len);
    }
    else
    {
        APP_PRINT_ERROR0("usb_audio_stream_pipe_xmit_out, ds ring buf full!");
        ring_buffer_remove(&(stream->pool), len);
        ring_buffer_write(&(stream->pool), buf, len);
    }

    return trigger_evt(pipe, stream_evt.d32, (uint32_t)label);
}

#if (F_APP_REDUCE_HEAP_USAGE == 0)
RAM_TEXT_SECTION
#endif
static int usb_audio_stream_pipe_xmit_out(T_USB_AUDIO_PIPES *pipe, void *buf, uint32_t len)
{
    APP_PRINT_INFO2("usb_audio_stream_pipe_xmit_out: label %d len %d", pipe->label, len);

#if USB_AUDIO_SILENCE_STREAM
    uac_silence_cnt = 0;
    uac_audio_pipe = pipe;
    hw_timer_restart(usb_audio_stream_timer_handle, USB_AUDIO_STREAM_DETECT_TIMEOUT);
#endif

    return usb_audio_stream_xmit_handle(pipe, buf, len);
}

#if USB_AUDIO_ACTIVE_DEBOUNCE
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
                            0, false, USB_AUDIO_ACTIVE_DEBOUNCE_TIME);

            ret = true;
        }
    }
    else
    {
        APP_PRINT_ERROR0("debounce info not found!");
    }

    return ret;
}
#endif

static void usb_audio_stream_deactive_handle(T_USB_AUDIO_STREAM *stream, uint8_t stream_type)
{
    usb_audio_stream_ctrl(stream, CTRL_CMD_STOP, 0);
    T_USB_IPC_EVT usb_ipc_evt = (stream_type == USB_AUDIO_STREAM_TYPE_OUT) ? USB_IPC_EVT_AUDIO_DS_STOP :
                                \
                                USB_IPC_EVT_AUDIO_US_STOP;
    usb_audio_stream_release(stream);
    app_ipc_publish(USB_IPC_TOPIC, usb_ipc_evt, NULL);
}

static void usb_audio_timer_cback(uint8_t timer_id, uint16_t timer_chann)
{
    switch (timer_id)
    {
#if USB_AUDIO_ACTIVE_DEBOUNCE
    case UAC1_DOWNSTREAM_DEBOUNCE:
    case UAC1_UPSTREAM_DEBOUNCE:
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

                if (info->active == false)
                {
                    T_USB_AUDIO_STREAM *stream = usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)info->type,
                                                                         info->label);

                    usb_audio_stream_deactive_handle(stream, info->type);
                }
            }
        }
        break;
#endif

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

#if USB_AUDIO_ACTIVE_DEBOUNCE
    if (evt_type == USB_AUDIO_STREAM_EVT_ACTIVE ||
        evt_type == USB_AUDIO_STREAM_EVT_DEACTIVE)
    {
        if (usb_audio_handle_evt_debounce(pipe_label, evt_type, stream_type))
        {
            return;
        }
    }

    if (evt_type == USB_AUDIO_STREAM_EVT_ACTIVE &&
        (stream->attr.sample_rate != attr.content.audio.sample_rate ||
         stream->attr.bit_width != attr.content.audio.bit_width ||
         stream->attr.chann_num != attr.content.audio.channels))
    {
        APP_PRINT_TRACE0("usb audio attr change, deactive first");
        usb_audio_stream_deactive_handle(stream, stream_type);
    }
#endif

    switch (evt_type)
    {
    case USB_AUDIO_STREAM_EVT_ACTIVE:
        {
            uint8_t bit_width = attr.content.audio.bit_width;
            T_STREAM_ATTR stream_attr = {.sample_rate = attr.content.audio.sample_rate,
                                         .chann_num = attr.content.audio.channels,
                                         .bit_width = attr.content.audio.bit_width
                                        };
            APP_PRINT_INFO3("USB_AUDIO_STREAM_EVT_ACTIVE, sample rate:%d, channel num:%d, bit width %d",
                            stream_attr.sample_rate, stream_attr.chann_num, stream_attr.bit_width);
            memcpy(&stream->attr, &stream_attr, sizeof(T_STREAM_ATTR));
            T_USB_IPC_EVT usb_ipc_evt = (stream_type == USB_AUDIO_STREAM_TYPE_OUT) ? USB_IPC_EVT_AUDIO_DS_START
                                        : \
                                        USB_IPC_EVT_AUDIO_US_START;
            usb_audio_stream_create(stream, stream_attr);
            app_ipc_publish(USB_IPC_TOPIC, usb_ipc_evt, NULL);
        }
        break;

    case USB_AUDIO_STREAM_EVT_DEACTIVE:
        {
            usb_audio_stream_deactive_handle(stream, stream_type);
        }
        break;

    case USB_AUDIO_STREAM_EVT_DATA_XMIT:
        {
            static uint8_t cnt = 0;
            usb_audio_stream_xmit(stream);
            if (cnt == 0)
            {
                cnt++;

                app_start_timer(&stream->monitor_timer_idx, "monitor_empty",
                                stream->stream_idle_id, UAS_TIMER_MONITOR_EMPTY, stream->pipe, false,
                                MONITOR_TIMER_INTERVAL_MS);
            }
            else
            {
                cnt = (cnt + 1) % 10;
            }

            app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_DS_XMIT, NULL);

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
            T_UAS_VOL vol;
            vol.cur = stream->ctrl.vol.cur = attr.content.vol.value;
            vol.range = stream->ctrl.vol.range;
            APP_PRINT_INFO2("USB_AUDIO_STREAM_EVT_VOL_SET, cur:0x%x, range:0x%x", stream->ctrl.vol.cur,
                            stream->ctrl.vol.range);
            usb_audio_stream_ctrl(stream, CTRL_CMD_VOL_CHG, &vol);
            if (attr.content.vol.type == USB_AUDIO_VOL_TYPE_CUR)
            {
                if (stream_type == USB_AUDIO_STREAM_TYPE_OUT)
                {
                    uint8_t is_max_volume = (vol.cur - vol.range == 0) ? 1 : 0;
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
            usb_audio_stream_ctrl(stream, CTRL_CMD_MUTE, &mute);
            app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_MUTE_CTRL, &mute);
        }
        break;

    default:
        break;
    }
}

static int usb_audio_stream_pipe_ctrl(struct _usb_audio_pipes *pipe, T_USB_AUDIO_CTRL_EVT evt,
                                      T_USB_AUDIO_PIPE_ATTR ctrl)
{
    return trigger_evt_ctrl(pipe, evt, ctrl.dir, ctrl.content.d32);
}

static T_USB_AUDIO_PIPES usb_audio_pipe =
{
    .label = 1,
    .ctrl = (USB_AUDIO_PIPE_CB_CTRL)usb_audio_stream_pipe_ctrl,
    .downstream = (USB_AUDIO_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_out,
    .upstream = (USB_AUDIO_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_in,
};

uint32_t usb_audio_stream_ual_bind(uint8_t stream_type, uint8_t label, T_UAS_FUNC *func)
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
        app_timer_reg_cb(usb_audio_stream_timeout_cb, &stream->stream_idle_id);
        os_queue_in(&uas_db.streams[stream_type], stream);
        os_queue_init(&(stream->ual_list));
    }
    stream->ual_total += 1;
    id.u.ual_id = stream->ual_total;

    ual_node = malloc(sizeof(T_UAL));
    ual_node->p_next = NULL;
    ual_node->handle = NULL;
    ual_node->owner = stream;
    ual_node->func = func;
    ual_node->id = id.d32;
    os_queue_in(&(stream->ual_list), ual_node);

    APP_PRINT_INFO3("usb_audio_stream_ual_bind, funcs:0x%x, type:0x%x, id:0x%x", func, stream_type,
                    id.d32);

    return (id.d32);
}


void usb_audio_stream_init(void)
{
    memset(&uas_db, 0, sizeof(uas_db));
    usb_audio_init(&usb_audio_pipe);

    app_timer_reg_cb(usb_audio_timer_cback, &usb_audio_timer_queue_id);
}
#endif
