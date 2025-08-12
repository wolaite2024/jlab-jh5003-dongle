/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "trace.h"
#include "compiler_abstraction.h"
#include "os_queue.h"
#include "os_msg.h"
#include "audio.h"
#include "trace.h"
#include "audio_type.h"
#include "mem_types.h"
#include "ual_types.h"
#include "ual_list.h"
#include "app_audio_pipe.h"
#include "app_audio_track.h"
#include "app_cyclic_buffer.h"
#include "app_timer.h"
#if APP_DEBUG_REPORT
#include "app_status_report.h"
#endif
#include "section.h"
#include "app_audio_path.h"

#define MSG_TYPE_PCM_RECV   0x0F00
#define MSG_TYPE_CODE_RECV  0x0F01

#define AUDIO_PATH_ID_MAX   ((1 << 4) - 1)

/* Store the received encoded frames. */
#define IT_CODE_BUF_SIZE    1024
/* Store the received PCM frames. */
#define IT_PCM_BUF_SIZE     (1024 * 4)

#define VOLUME_CTL_UPDOWN       0
#define VOLUME_CTL_MUTE         1

#define STREAM_TIMER_ID         1
#define STREAM_IDLE_TIMEDOUT    200 /* 200ms */
#define STREAM_IDLE_MAX         10

#define MIX_PATHS_MAX           0x08
#define MIX_PATHS_MASK          0x07

#define ARRAY_SIZE(a)           (sizeof((a)) / sizeof((a[0])))

enum app_audio_path_type
{
    AUDIO_PATH_TYPE_PIPE = 0,
    AUDIO_PATH_TYPE_TRACK,
    AUDIO_PATH_TYPE_MAX,
};

enum app_audio_path_state
{
    APP_AUDIO_PATH_IDLE,
    APP_AUDIO_PATH_CREATING,
    APP_AUDIO_PATH_CREATED,
    APP_AUDIO_PATH_STARTED,
    APP_AUDIO_PATH_STOPPING,
    APP_AUDIO_PATH_STOPPED,
    /* path would be released, but a renewed path has been added. */
    APP_AUDIO_PATH_PENDING_REL,
    APP_AUDIO_PATH_RELEASING,

    /* There is no released state, because if it's released, the path would be
     * removed from list and freed. */
};

typedef struct t_audio_path
{
    T_UALIST_HEAD           list;
    T_UALIST_HEAD           mix_action;
    uint8_t                 id;         /* 0 - 15 */
    uint8_t                 type;       /* pipe or track ... */
    uint8_t                 codec_type; /* none, enc, dec, ... */
    uint8_t                 it;         /* input terminal, IT_* */
    uint8_t                 ot;         /* output_terminal, OT_* */
    uint8_t                 state;      /* idle, creating, created, ... */
    uint8_t                 priority;
    uint8_t                 renewed;    /* path is renewed. */
    uint8_t                 pre_state;  /* Save the state before renew */
    uint8_t                 streaming;
    uint8_t                 stream_idle_count;
    uint8_t                 mix;
    uint16_t                media_packets;
    uint16_t                frame_len;
    char                   *ident;
    T_AUDIO_FORMAT_INFO     ifmt;
    T_AUDIO_FORMAT_INFO     ofmt;
    t_audio_uapi_usr_cback  uapi_usr_cback;
    void (*mgr_cback)(uint8_t id, uint8_t event, uint8_t *buf, uint32_t len);
    /* One path has at most one operation at a time. */
    void                   *op;

    T_CYCLIC_BUF           *cyclic;
    uint8_t                timer_idx_stream;
} T_AUDIO_PATH;

/* Operation code for releasing or creating a path */
enum audio_path_operation
{
    AUDIO_PATH_OP_REL = 0,
    AUDIO_PATH_OP_CREATE,
    AUDIO_PATH_OP_MAX,
};

typedef struct t_audio_path_op
{
    T_UALIST_HEAD           list;
    T_AUDIO_PATH           *path;
    uint8_t                      opcode;
} T_AUDIO_PATH_OP;

struct audio_path_type
{
    T_UALIST_HEAD           path_list;

    /* The list contains the audio path operations to do in the future.
     * The operation would be removed from the list when it is in process.
     * */
    T_UALIST_HEAD           operation_list;
    uint8_t                      busy;
};

struct audio_path_type audio_path_types[AUDIO_PATH_TYPE_MAX];
/* Pay attention to the array size, it must be max + 1. */
static uint8_t path_id_state[AUDIO_PATH_ID_MAX + 1];
static uint8_t next_path_id;

/* Report audio events to upper layer. */
struct audio_path_monitor
{
    T_UALIST_HEAD list;
    t_audio_monitor_cback cb;
};
static T_UALIST_HEAD audio_monitor_list =
{
    &audio_monitor_list, &audio_monitor_list
};

/* The item header in cyclic buf */
struct cyclic_ihdr
{
    uint16_t len;
    uint8_t  it;
    uint8_t  flag;
    uint32_t timestamp;
};

/* All input terminals with code share a common buf because the data is
 * packet-based. */
static T_CYCLIC_BUF     it_code_buf;
/* static T_CYCLIC_BUF     it_code_bt_spp; */
/* static T_CYCLIC_BUF     it_code_bt_lc3frm; */
/* Every PCM input terminal has a dedicated buf. */
static T_CYCLIC_BUF     it_pcm_buf1;
static T_CYCLIC_BUF     it_pcm_buf2;

static void *audio_path_evt_q;
static void *audio_path_msg_q;

static uint8_t it_statistics[IT_MAX];
static uint8_t audio_path_stream_timer_id = 0;
static T_UALIST_HEAD mix_paths[MIX_PATHS_MAX];

uint8_t ipc_version = 1;

static void audio_path_op_dequeue_type(uint8_t path_type);
static uint32_t app_audio_path_track_dev(T_AUDIO_PATH *path);
static void app_audio_path_process_code(T_AUDIO_PATH *path);
static uint8_t app_audio_path_pipe_codec(T_AUDIO_PATH *path);

/****** TODO: Implement the following functions for LE Audio ******/

bool app_audio_path_track_in_volume_mute(uint32_t device, bool enable)
{
    app_audio_track_in_volume_mute(device, enable);
    return true;
}

bool app_audio_path_track_in_volume_up_down(uint32_t device, bool vol_up)
{
    app_audio_track_in_volume_up_down(device, vol_up);
    return true;
}

bool app_audio_path_control_track_volume(uint8_t type, uint8_t val)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;
    T_UALIST_HEAD *track_list;
    uint32_t dev;
    T_AUDIO_PATH *tmp;

    APP_PRINT_INFO2("app_audio_path_control_track_volume: type %u, val %u",
                    type, val);

    track_list = &audio_path_types[AUDIO_PATH_TYPE_TRACK].path_list;

    if (!track_list)
    {
        APP_PRINT_ERROR0("app_audio_path_control_track_volume: track_list is null!");
        return false;
    }

    ualist_for_each_safe(pos, next, track_list)
    {

        tmp = ualist_entry(pos, T_AUDIO_PATH, list);
        if (!tmp)
        {
            APP_PRINT_ERROR0("app_audio_path_control_track_volume: tmp is null!");
            return false;
        }
        dev = app_audio_path_track_dev(tmp);

        APP_PRINT_INFO3("app_audio_path_control_track_volume: it 0x%x, ot 0x%x, dev %u",
                        tmp->it, tmp->ot, dev);
        if (dev == 0xFFFFFFFF)
        {
            continue;
        }

        /* TODO: Currently we only set the volume of the first aux out dev. */
        if (dev == AUDIO_DEVICE_OUT_SPK || dev == AUDIO_DEVICE_OUT_AUX)
        {
            if (type == VOLUME_CTL_UPDOWN)
            {
                return app_audio_track_out_volume_change(dev, val);
            }
            else
            {
                return app_audio_track_out_volume_mute(dev, !!val);
            }
        }
    }

    return false;
}

bool app_audio_path_set_aux_track_volume(uint8_t volume)
{
    return app_audio_path_control_track_volume(VOLUME_CTL_UPDOWN, volume);
}

bool app_audio_path_set_aux_mute(bool mute)
{
    return app_audio_path_control_track_volume(VOLUME_CTL_MUTE, (uint8_t)mute);
}

bool app_audio_path_set_input_mute(uint8_t iterminal, bool mute)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;
    T_UALIST_HEAD *track_list;
    uint32_t dev;
    T_AUDIO_PATH *tmp;

    /* FIXME: usb input terminal needs to be considered. */

    track_list = &audio_path_types[AUDIO_PATH_TYPE_TRACK].path_list;

    ualist_for_each_safe(pos, next, track_list)
    {
        tmp = ualist_entry(pos, T_AUDIO_PATH, list);
        if (tmp->it != iterminal)
        {
            continue;
        }
        if (tmp->state == APP_AUDIO_PATH_IDLE ||
            tmp->state == APP_AUDIO_PATH_PENDING_REL ||
            tmp->state == APP_AUDIO_PATH_RELEASING)
        {
            continue;
        }

        dev = app_audio_path_track_dev(tmp);
        return app_audio_track_in_volume_mute(dev, mute);
    }

    APP_PRINT_ERROR1("app_audio_path_set_input_mute: No active path with iterminal %u",
                     iterminal);
    return false;
}
/****** END ******/

static uint8_t count_1_bit_num(uint32_t value)
{
    uint8_t count = 0;

    while (value)
    {
        if (value & 0x01)
        {
            count++;
        }
        value >>= 1;
    }
    return count;
}

static uint8_t app_audio_path_type(uint8_t it, uint8_t ot)
{
    switch (it)
    {
    case IT_MIC:
    case IT_AUX:
        return AUDIO_PATH_TYPE_TRACK;
    case IT_UDEV_IN1:
    case IT_UDEV_IN2:
        switch (ot)
        {
        case OT_SBC:
        case OT_SBC2:
        case OT_LC3FRM:
        case OT_LC3FRM2:
        case OT_MSBC:
            return AUDIO_PATH_TYPE_PIPE;
        default:
            return 0xff;
        }
    }

    switch (ot)
    {
    case OT_SPK:
    case OT_AUX:
        return AUDIO_PATH_TYPE_TRACK;
    case OT_UDEV_OUT1:
        if (it != IT_SBC && it != IT_LC3FRM && it != IT_MSBC)
        {
            return 0xff;
        }
        return AUDIO_PATH_TYPE_PIPE;
    }

    return 0xff;
}

static uint8_t app_audio_path_codec_type(uint8_t it, uint8_t ot)
{
    switch (it)
    {
    case IT_MIC:
    case IT_AUX:
        return AUDIO_PATH_CODEC_ENC;
    case IT_UDEV_IN1:
    case IT_UDEV_IN2:
        switch (ot)
        {
        case OT_SBC:
        case OT_SBC2:
        case OT_LC3FRM:
        case OT_LC3FRM2:
        case OT_MSBC:
            return AUDIO_PATH_CODEC_ENC;
        }
        break;
    default:
        break;
    }

    switch (ot)
    {
    case OT_SPK:
    case OT_AUX:
        return AUDIO_PATH_CODEC_DEC;
    case OT_UDEV_OUT1:
        if (it == IT_SBC || it == IT_LC3FRM || it == IT_MSBC)
        {
            return AUDIO_PATH_CODEC_DEC;
        }
        break;
    default:
        break;
    }

    return AUDIO_PATH_CODEC_NONE;
}

/* static T_UALIST_HEAD *app_audio_path_type_list(uint8_t it, uint8_t ot)
 * {
 *     uint8_t type;
 *
 *     type = app_audio_path_type(it, ot);
 *     if (type >= AUDIO_PATH_TYPE_MAX)
 *     {
 *         return NULL;
 *     }
 *     return &audio_path_types[type].path_list;
 * }
 */

static inline T_AUDIO_PATH *app_audio_path_get_by_id(T_UALIST_HEAD *head, uint8_t id)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;
    T_AUDIO_PATH *path;

    ualist_for_each_safe(pos, next, head)
    {
        path = ualist_entry(pos, T_AUDIO_PATH, list);

        if (path->id == id)
        {
            return path;
        }
    }

    return NULL;
}

static inline void compose_monitor_msghdr(struct audio_monitor_msghdr *hdr,
                                          T_AUDIO_PATH *path)
{
    hdr->path_id = path->id;
    hdr->it = path->it;
    hdr->ot = path->ot;
    hdr->codec_type = path->codec_type;
}

static void app_audio_path_notify_monitors(uint8_t event, struct audio_monitor_msghdr *msghdr,
                                           uint8_t *buf, uint16_t len)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;
    struct audio_path_monitor *monitor;
    /* TODO: Use dynamically memory allocation. */
    uint8_t tmp_buf[8];

    if (!msghdr)
    {
        return;
    }

    if (ualist_empty(&audio_monitor_list))
    {
        return;
    }

    if (sizeof(tmp_buf) < sizeof(*msghdr) + len)
    {
        APP_PRINT_ERROR1("app_audio_path_notify_monitors: msg too big %u",
                         sizeof(*msghdr) + len);
        return;
    }

    memcpy(tmp_buf, msghdr, sizeof(*msghdr));
    if (buf && len > 0)
    {
        memcpy(tmp_buf + sizeof(*msghdr), buf, len);
    }

    ualist_for_each_safe(pos, next, &audio_monitor_list)
    {
        monitor = ualist_entry(pos, struct audio_path_monitor, list);
        if (monitor->cb)
        {
            monitor->cb(event, tmp_buf, sizeof(*msghdr) + len);
        }
    }
}

static void app_audio_path_free(T_AUDIO_PATH *path)
{
    if (!path)
    {
        return;
    }

    APP_PRINT_INFO1("app_audio_path_free, path %x", path);

    if (path->op)
    {
        APP_PRINT_ERROR0("app_audio_path_free, op is not NULL!");
    }

    ualist_del(&path->list);
    ualist_del(&path->mix_action);
    /* For audio path temporary release, the path id is reused by a new
     * subsequent audio path and released event would be still received.  The
     * path id exceeds the range (0 - AUDIO_PATH_ID_MAX) because its highest
     * bit is set to 1.
     * */
    if (!(path->id & 0x80))
    {
        path->id &= AUDIO_PATH_ID_MAX;
        path_id_state[path->id] = 0;
    }
    if (path->ident)
    {
        free(path->ident);
    }
    if (path->timer_idx_stream)
    {
        /* TODO: We should check the return value. */
        app_stop_timer(&path->timer_idx_stream);
        path->timer_idx_stream = 0;
    }
    free(path);
}

/* FIXME: We use app_audio_path_fill() or app_audio_path_fill_async() instead
 * of this function.
 * */
#if 0
static void audio_path_track_dec_fill(T_AUDIO_PATH *path)
{
    uint8_t ret;
    uint8_t *buf;
    T_CYCLIC_BUF *cyclic;
    struct cyclic_ihdr hdr;
    uint16_t frame_len;
    uint32_t dev;
    bool rc;

    if (!path || !path->cyclic)
    {
        return;
    }

    cyclic = path->cyclic;
    frame_len = path->frame_len + sizeof(hdr);

    if (!cyclic_buf_peek(cyclic, (void *)&hdr, sizeof(hdr)))
    {
        return;
    }

    /* This frame is not for this codec, remove it and process next frame.
     * Be careful, there is recursion.
     * */
    if (hdr.len != frame_len)
    {
        APP_PRINT_WARN4("audio_path_track_dec_fill: frame len (%u!=%u) mismatch,"
                        " drop the frame, path (%u->%u)", hdr.len, frame_len,
                        path->it, path->ot);
        if (cyclic_buf_drop(cyclic, hdr.len))
        {
            audio_path_track_dec_fill(path);
        }
        else
        {
            APP_PRINT_ERROR0("audio_path_track_dec_fill: drop data err");
            return;
        }
    }

    if (cyclic_buf_count(cyclic) < frame_len)
    {
        return;
    }

    buf = calloc(frame_len);
    if (!buf)
    {
        APP_PRINT_ERROR0("audio_path_track_dec_fill: zalloc failed");
        return;
    }

    APP_PRINT_INFO1("audio_path_track_dec_fill: frame_len %u", frame_len);

    ret = cyclic_buf_read(cyclic, buf, frame_len);
    if (!ret)
    {
        goto err;
    }

    dev = app_audio_path_track_dev(path);
    if (dev == 0xFFFFFFFF)
    {
        APP_PRINT_ERROR2("audio_path_track_dec_fill: err track dev, in-out (%u->%u)",
                         path->it, path->ot);
        goto err;
    }
    /* TODO: Is it an error if we fill data to track prior to the filled
     * event for the previous data filling.
     * */
    frame_len -= sizeof(hdr);
    rc = app_audio_track_write_with_flag(dev, buf + sizeof(hdr), hdr.flag,
                                         frame_len, 1, hdr.timestamp);
    if (!rc)
    {
        APP_PRINT_ERROR3("audio_path_track_dec_fill: Write track error, "
                         "flag %u, len %u, ts %08x", hdr.flag, frame_len,
                         hdr.timestamp);
    }
    else
    {
        /* When we received filled event, we would decrease the busy
         * variable.
         * */
        audio_path_types[path->type].busy++;
    }

err:
    free(buf);
}
#endif

static inline void app_audio_path_stream_stopped(T_AUDIO_PATH *path)
{
    uint8_t streaming;

    if (!path)
    {
        return;
    }

    streaming = path->streaming;
    path->streaming = 0;
    path->media_packets = 0;
    if (path->timer_idx_stream)
    {
        app_stop_timer(&path->timer_idx_stream);
        path->timer_idx_stream = 0;
    }

    if (!(path->id & 0x80) && streaming && path->uapi_usr_cback)
    {
        path->uapi_usr_cback(path->id, EVENT_AUDIO_PATH_STREAM_STOPPED, NULL, 0, 0);
    }
}

static inline bool state_transition(T_AUDIO_PATH *path)
{
    uint8_t state;
    uint8_t path_id;

    state = path->state;
    path_id = path->id;
    /* The path is temporarily releasing. */
    if (path_id & 0x80)
    {
        return false;
    }

    /* The bit7 of path_id is zero. */
    if (!path->renewed)
    {
        return true;
    }

    /* The bit7 of path_id is zero and path->renewed is 1
     * */

    if (state == APP_AUDIO_PATH_CREATED)
    {
        if (path->pre_state <= APP_AUDIO_PATH_CREATING)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (state == APP_AUDIO_PATH_STARTED)
    {
        if (path->pre_state <= APP_AUDIO_PATH_CREATED)
        {
            return true;
        }
        else if (path->pre_state == APP_AUDIO_PATH_STOPPING)
        {
            return true;
        }
        else if (path->pre_state == APP_AUDIO_PATH_STOPPED)
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
        return false;
    }
}

__STATIC_ALWAYS_INLINE void combined_pipe_fill(uint8_t type, T_AUDIO_PATH *path)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;
    T_AUDIO_PATH *tmp;
    uint8_t ctype;

    if (type >= CODEC_MAX_TYPE)
    {
        return;
    }
    if (!path || !path->cyclic)
    {
        return;
    }
    if (path->type != AUDIO_PATH_TYPE_PIPE)
    {
        return;
    }

    /* It's a single audio path. */
    if (!path->mix)
    {
        app_audio_pipe_fill(type, path->cyclic);
        return;
    }

    ualist_for_each_safe(pos, next, &mix_paths[path->mix])
    {
        tmp = ualist_entry(pos, T_AUDIO_PATH, mix_action);
        ctype = app_audio_path_pipe_codec(tmp);
        app_audio_pipe_fill(ctype, tmp->cyclic);
    }
}

static void audio_mgr_pipe_cback(uint8_t path_id, uint8_t event, uint8_t *buf, uint32_t len)
{
    T_AUDIO_PATH *path;
    struct audio_path_type *ptype;
    uint8_t type = AUDIO_PATH_TYPE_PIPE;
    uint8_t ctype;
    t_audio_uapi_usr_cback uapi_usr_cback;
    uint8_t previous_state;
    struct audio_monitor_msghdr mhdr;

    ptype = &audio_path_types[type];

    path = app_audio_path_get_by_id(&ptype->path_list, path_id);
    if (!path)
    {
        APP_PRINT_INFO1("audio_mgr_pipe_cback: Invalid path_id %u", path_id);
        return;
    }
    uapi_usr_cback = path->uapi_usr_cback;

    if (event == APP_AUDIO_PIPE_EVENT_DATA_IND)
    {
        uint16_t frm_num;
        uint16_t buf_len;

        if (path_id & 0x80)
        {
            APP_PRINT_WARN3("audio_mgr_pipe_cback: Unexpected data ind (len %u) "
                            "for path %u->%u", len, path->it, path->ot);
            return;
        }

        path->media_packets++;
        if (!path->streaming)
        {
            path->streaming = 1;
            if (uapi_usr_cback)
            {
                uapi_usr_cback(path_id, EVENT_AUDIO_PATH_STREAM_STARTED, NULL,
                               0, 0);
            }
            app_start_timer(&path->timer_idx_stream, "timer_idx_stream",
                            audio_path_stream_timer_id, STREAM_TIMER_ID, path->id, false,
                            STREAM_IDLE_TIMEDOUT);
        }

        buf_len = len & 0xffff;
        frm_num = (len >> 16) & 0xffff;
        if (uapi_usr_cback)
        {
            uapi_usr_cback(path_id, EVENT_AUDIO_PATH_DATA_IND, buf, buf_len,
                           frm_num);

        }

        return;
    }

    compose_monitor_msghdr(&mhdr, path);

    switch (event)
    {
    case APP_AUDIO_PIPE_EVENT_RELEASED:
        APP_PRINT_INFO1("audio_mgr_pipe_cback: released, path_id %u", path_id);

        it_statistics[path->it]--;
        ptype->busy--;

        previous_state = path->state;

        app_audio_path_stream_stopped(path);

        /* FIXME: Fake a stopped event. */
        if (!(path_id & 0x80) && previous_state != APP_AUDIO_PATH_STOPPED)
        {
            app_audio_path_notify_monitors(EVENT_AUDIO_STOPPED, &mhdr, NULL, 0);
        }

        /* Free path first, then call the uapi usr callback.
         * The calling sequence makes the user calling other api in the
         * callback safer.
         * */
        app_audio_path_free(path);
        /* If bit7 of path id is 1, it means that the release is temporary,
         * there is a subsequent creation. So do not send the internal event to
         * upper layer. */
        if (!(path_id & 0x80))
        {
            if (uapi_usr_cback)
            {
                uapi_usr_cback(path_id, EVENT_AUDIO_PATH_RELEASED, NULL, 0, 0);
            }
            app_audio_path_notify_monitors(EVENT_AUDIO_RELEASED, &mhdr, NULL, 0);
        }
        audio_path_op_dequeue_type(type);
        break;

    case APP_AUDIO_PIPE_EVENT_CREATED:
        APP_PRINT_INFO1("audio_mgr_pipe_cback: created, path_id %u", path_id);

        it_statistics[path->it]++;
        path->state = APP_AUDIO_PATH_CREATED;

        if (!(path_id & 0x80))
        {
            if (uapi_usr_cback)
            {
                uapi_usr_cback(path_id, EVENT_AUDIO_PATH_CREATED, NULL, 0, 0);
            }
            if (state_transition(path))
            {
                app_audio_path_notify_monitors(EVENT_AUDIO_CREATED, &mhdr, NULL, 0);
            }
        }

        break;

    case APP_AUDIO_PIPE_EVENT_STARTED:
        APP_PRINT_INFO1("audio_mgr_pipe_cback: started, path_id %u", path_id);

        ptype->busy--;
        path->state = APP_AUDIO_PATH_STARTED;

        ctype = app_audio_path_pipe_codec(path);
        if (ctype != 0xff)
        {
            combined_pipe_fill(ctype, path);
        }

        if (state_transition(path))
        {
            if (uapi_usr_cback)
            {
                uapi_usr_cback(path_id, EVENT_AUDIO_PATH_READY, NULL, 0, 0);
            }

            app_audio_path_notify_monitors(EVENT_AUDIO_STARTED, &mhdr, NULL, 0);
        }

        audio_path_op_dequeue_type(type);
        break;

    case APP_AUDIO_PIPE_EVENT_DATA_FILLED:
        ctype = app_audio_path_pipe_codec(path);
        if (ctype != 0xff)
        {
            combined_pipe_fill(ctype, path);
        }
        break;

    case APP_AUDIO_PIPE_EVENT_STOPPED:
        APP_PRINT_INFO1("audio_mgr_pipe_cback: stopped, path_id %u", path_id);
        path->state = APP_AUDIO_PATH_STOPPED;
        app_audio_path_stream_stopped(path);
        if (!(path_id & 0x80))
        {
            app_audio_path_notify_monitors(EVENT_AUDIO_STOPPED, &mhdr, NULL, 0);
        }
        break;

    default:
        APP_PRINT_ERROR1("audio_mgr_pipe_cback: Unknown event %u", event);
        break;
    }
}

static void audio_mgr_track_cback(uint8_t path_id, uint8_t event, uint8_t *buf, uint32_t len)
{
    T_AUDIO_PATH *path;
    struct audio_path_type *ptype;
    uint8_t type = AUDIO_PATH_TYPE_TRACK;
    t_audio_uapi_usr_cback uapi_usr_cback;
    struct audio_monitor_msghdr mhdr;

    ptype = &audio_path_types[type];

    path = app_audio_path_get_by_id(&ptype->path_list, path_id);
    if (!path)
    {
        APP_PRINT_INFO1("audio_mgr_track_cback: Invalid path_id %u", path_id);
        return;
    }

    uapi_usr_cback = path->uapi_usr_cback;
    compose_monitor_msghdr(&mhdr, path);

    APP_PRINT_INFO1("audio_mgr_track_cback: event %u", event);
    switch (event)
    {
    case APP_AUDIO_TRACK_EVENT_RELEASED:
        APP_PRINT_INFO1("audio_mgr_track_cback: released, path_id %u", path_id);

        it_statistics[path->it]--;
        ptype->busy--;

        app_audio_path_stream_stopped(path);

        /* Free path first, then call the uapi usr callback.
         * The calling sequence makes the user calling other api in the
         * callback safer.
         * */
        app_audio_path_free(path);
        /* If bit7 of path id is 1, it means that the release is temporary,
         * there is a subsequent creation. So do not send the internal event to
         * upper layer. */
        if (!(path_id & 0x80))
        {
            if (uapi_usr_cback)
            {
                uapi_usr_cback(path_id, EVENT_AUDIO_PATH_RELEASED, NULL, 0, 0);
            }
            app_audio_path_notify_monitors(EVENT_AUDIO_RELEASED, &mhdr, NULL, 0);
            //le_vcs_audio_set_input_status(path->it, false);
        }
        app_audio_path_flush(mhdr.it);
        audio_path_op_dequeue_type(type);
        break;

    case APP_AUDIO_TRACK_EVENT_CREATED:
        APP_PRINT_INFO1("audio_mgr_track_cback: created, path_id %u", path_id);

        it_statistics[path->it]++;
        path->state = APP_AUDIO_PATH_CREATED;
        if (state_transition(path))
        {
            app_audio_path_notify_monitors(EVENT_AUDIO_CREATED, &mhdr, NULL, 0);
            //le_vcs_audio_set_input_status(path->it, true);
        }
        break;

    case APP_AUDIO_TRACK_EVENT_STARTED:
        APP_PRINT_INFO1("audio_mgr_track_cback: started, path_id %u", path_id);

        ptype->busy--;
        path->state = APP_AUDIO_PATH_STARTED;

        if (state_transition(path))
        {
            if (uapi_usr_cback)
            {
                uapi_usr_cback(path_id, EVENT_AUDIO_PATH_READY, NULL, 0, 0);
            }

            app_audio_path_notify_monitors(EVENT_AUDIO_STARTED, &mhdr, NULL, 0);
        }

        audio_path_op_dequeue_type(type);
        if (path->codec_type == AUDIO_PATH_CODEC_DEC)
        {
            /* Try to fill code to low-level track */
            app_audio_path_process_code(path);
        }

        break;

    case APP_AUDIO_TRACK_EVENT_STOPPED:
        APP_PRINT_INFO1("audio_mgr_track_cback: stopped, path_id %u", path_id);

        path->state = APP_AUDIO_PATH_STOPPED;
        app_audio_path_stream_stopped(path);

        if (!(path_id & 0x80))
        {
            app_audio_path_notify_monitors(EVENT_AUDIO_STOPPED, &mhdr, NULL, 0);
        }
        app_audio_path_flush(mhdr.it);
        break;

    case APP_AUDIO_TRACK_EVENT_DATA_IND:
        if (path_id & 0x80)
        {
            APP_PRINT_WARN3("audio_mgr_pipe_cback: Unexpected data ind (len %u) "
                            "for path %u->%u", len, path->it, path->ot);
            break;
        }
        path->media_packets++;
        if (!path->streaming)
        {
            path->streaming = 1;
            if (uapi_usr_cback)
            {
                uapi_usr_cback(path_id, EVENT_AUDIO_PATH_STREAM_STARTED, NULL,
                               0, 0);
            }
            app_start_timer(&path->timer_idx_stream, "stream_timer",
                            audio_path_stream_timer_id, STREAM_TIMER_ID, path->id, false,
                            STREAM_IDLE_TIMEDOUT);
        }
        if (uapi_usr_cback)
        {
            uapi_usr_cback(path_id, EVENT_AUDIO_PATH_DATA_IND, buf + 1, len - 1,
                           buf[0]);
            APP_PRINT_INFO2("audio_mgr_track_cback: get data len %d, data %b",
                            len, TRACE_BINARY(10, buf));
        }
        else
        {
            APP_PRINT_INFO1("audio_mgr_track_cback: drop data, len %d", len);
        }
        break;

    case APP_AUDIO_TRACK_EVENT_DATA_FILLED:
        {
            /* For audio path codec is track decoder. app should fill the track
             * with encoded audio data and app would receive data filled event.
             * For track encoder, the pcm data should be read from mic, aux in,
             * line in etc without the help from app.
             * So for track encoder, this data filled event shall never be
             * received.
             * */

            APP_PRINT_INFO0("audio_mgr_track_cback: data filled event");
            if (path->codec_type == AUDIO_PATH_CODEC_DEC)
            {
                /* ptype->busy--; */
                audio_path_op_dequeue_type(type);
                /* Try to fill code to low-level track */
                app_audio_path_process_code(path);
            }
        }
        break;

    case APP_AUDIO_TRACK_EVENT_MUTED:
        APP_PRINT_INFO1("audio_mgr_track_cback: muted, path_id %u", path_id);
        /* TODO: */
        if (path->codec_type == AUDIO_PATH_CODEC_ENC)
        {
            //le_vcs_audio_set_input_mute(path->it, true);
        }
        else
        {
            //le_vcs_audio_set_output_mute(path->ot, true);
        }
        app_audio_path_notify_monitors(EVENT_AUDIO_MUTED, &mhdr, NULL, 0);
        break;

    case APP_AUDIO_TRACK_EVENT_UNMUTED:
        APP_PRINT_INFO1("audio_mgr_track_cback: unmuted, path_id %u", path_id);
        /* TODO: */
        if (path->codec_type == AUDIO_PATH_CODEC_ENC)
        {
            //le_vcs_audio_set_input_mute(path->it, false);
        }
        else
        {
            //le_vcs_audio_set_output_mute(path->ot, false);
        }
        app_audio_path_notify_monitors(EVENT_AUDIO_UNMUTED, &mhdr, NULL, 0);
        break;

    case APP_AUDIO_TRACK_EVENT_VOL_CHG:
        APP_PRINT_INFO1("audio_mgr_track_cback: vol changed, path_id %u", path_id);

        if (path->codec_type != AUDIO_PATH_CODEC_ENC)
        {
            /* TODO: */
            uint8_t *vol = (uint8_t *)buf;

            //le_vcs_audio_set_output_vol_value(path->ot, *p_volume);
            app_audio_path_notify_monitors(EVENT_AUDIO_VOLUME_CHANGED, &mhdr, vol, 1);
        }
        break;

    default:
        APP_PRINT_ERROR1("audio_mgr_track_cback: Unknown event %u", event);
        break;
    }
}

/* FIXME: Is it enough to check one path side only? */
static uint8_t app_audio_path_pipe_codec(T_AUDIO_PATH *path)
{
    switch (path->ot)
    {
    case OT_SBC:
        return CODEC_SBCENC_TYPE;
    case OT_SBC2:
        return CODEC_SBCENC2_TYPE;
    case OT_LC3FRM:
        return CODEC_LC3ENC_TYPE;
    case OT_LC3FRM2:
        return CODEC_LC3ENC2_TYPE;
    case OT_MSBC:
        return CODEC_MSBCENC_TYPE;
    default:
        break;
    }

    switch (path->it)
    {
    case IT_SBC:
        return CODEC_SBCDEC_TYPE;
    case IT_LC3FRM:
        return CODEC_LC3DEC_TYPE;
    case IT_MSBC:
        return CODEC_MSBCDEC_TYPE;
    default:
        break;
    }

    return 0xFF;
}

static uint32_t app_audio_path_track_dev(T_AUDIO_PATH *path)
{
    switch (path->ot)
    {
    case OT_SPK:
        return AUDIO_DEVICE_OUT_SPK;
    case OT_AUX:
        return AUDIO_DEVICE_OUT_AUX;
    default:
        break;
    }

    switch (path->it)
    {
    case IT_MIC:
        return AUDIO_DEVICE_IN_MIC;
    case IT_AUX:
        return AUDIO_DEVICE_IN_AUX;
    default:
        break;
    }

    return 0xFFFFFFFF;
}

/* FIXME: We should alloc different buf for different input terminal. */
RAM_TEXT_SECTION
static inline T_CYCLIC_BUF *audio_path_get_code_cyclic(uint8_t it)
{
    switch (it)
    {
    case IT_SBC:
        return &it_code_buf; /* &it_code_bt_spp; */
    case IT_MSBC:
        return &it_code_buf;
    case IT_LC3FRM:
        return &it_code_buf; /* &it_code_bt_lc3frm; */
    case IT_RELAY_CODE1:
        return &it_code_buf;
    default:
        return NULL;
    }
}

RAM_TEXT_SECTION
static inline T_CYCLIC_BUF *audio_path_get_pcm_cyclic(uint8_t it)
{
    /* TODO: IT_UDEV_IN2, ... */
    switch (it)
    {
    case IT_UDEV_IN1:
        return &it_pcm_buf1;
    case IT_UDEV_IN2:
        return &it_pcm_buf2;
    default:
        return NULL;
    }
}

static inline T_CYCLIC_BUF *audio_path_get_cyclic(uint8_t it)
{
    T_CYCLIC_BUF *cyclic = NULL;

    cyclic = audio_path_get_code_cyclic(it);
    if (cyclic)
    {
        return cyclic;
    }
    else
    {
        return audio_path_get_pcm_cyclic(it);
    }
}

static int audio_path_exec_creation(T_AUDIO_PATH_OP *op)
{
    T_AUDIO_PATH *path;
    T_AUDIO_FORMAT_INFO *codec;

    path = op->path;
    /* TODO: Is the following check redundant?*/
    if (!path)
    {
        return -1;
    }
    APP_PRINT_INFO1("audio_path_exec_creation: path->type 0x%x", path->type);
    switch (path->codec_type)
    {
    case AUDIO_PATH_CODEC_ENC:
        path->cyclic = audio_path_get_pcm_cyclic(path->it);
        codec = &path->ofmt;
        break;
    case AUDIO_PATH_CODEC_DEC:
        path->cyclic = audio_path_get_code_cyclic(path->it);
        codec = &path->ifmt;
        break;
    default:
        APP_PRINT_ERROR5("audio_path_exec_creation: Invalid codec_type %u, "
                         "path (type: %u, id :%u), iot (%u->%u)", path->codec_type,
                         path->type, path->id, path->it, path->ot);
        return -2;
    }

    if ((codec->type == AUDIO_FORMAT_TYPE_SBC) || (codec->type == AUDIO_FORMAT_TYPE_MSBC))
    {
        path->frame_len = calc_sbc_frame_size(codec->attr.sbc.chann_mode,
                                              codec->attr.sbc.block_length,
                                              codec->attr.sbc.subband_num,
                                              codec->attr.sbc.bitpool);
    }
    else if (codec->type == AUDIO_FORMAT_TYPE_LC3)
    {
        if (codec->attr.lc3.chann_location == AUDIO_CHANNEL_LOCATION_MONO)
        {
            path->frame_len = codec->attr.lc3.frame_length;
        }
        else
        {
            path->frame_len = codec->attr.lc3.frame_length *
                              count_1_bit_num(codec->attr.lc3.chann_location);
            /* path->frame_len = codec->attr.lc3.frame_length; */
        }
        APP_PRINT_INFO1("audio_path_exec_creation: lc3frame len %d", path->frame_len);
    }

    if (path->type == AUDIO_PATH_TYPE_PIPE)
    {
        uint8_t ctype;

        ctype = app_audio_path_pipe_codec(path);
        if (ctype == 0xFF)
        {
            APP_PRINT_ERROR2("audio_path_exec_creation: err pipe ctype, path (%u->%u)",
                             path->it, path->ot);
            return -3;
        }
        if ((path->codec_type == AUDIO_PATH_CODEC_DEC) && (path->ot == OT_UDEV_OUT1))
        {
            uint16_t n = app_audio_path_watermark(OT_UDEV_OUT1) & 0xFFFF;
            APP_PRINT_INFO3("audio_path_exec_creation: clear watermark %d (%u->%u)",
                            n, path->it, path->ot);
            app_audio_path_resume_dec(path->ot, n);
        }
        if (!app_audio_pipe_create(ctype, codec, path->mgr_cback, path->id, path->mix))
        {
            return -4;
        }
    }
    else if (path->type == AUDIO_PATH_TYPE_TRACK)
    {
        uint32_t dev;

        dev = app_audio_path_track_dev(path);
        if (dev == 0xFFFFFFFF)
        {
            APP_PRINT_ERROR2("audio_path_exec_creation: err track dev, path (%u->%u)",
                             path->it, path->ot);
            return -5;
        }
        if (!app_audio_track_create(dev, codec, path->mgr_cback, path->id))
        {
            return -6;
        }
    }

    audio_path_types[path->type].busy++;
    path->state = APP_AUDIO_PATH_CREATING;

    return 0;
}

static int audio_path_exec_release(T_AUDIO_PATH_OP *op)
{
    T_AUDIO_PATH *path;

    path = op->path;
    /* TODO: Is the following check redundant?*/
    if (!path)
    {
        return -1;
    }

    if (path->type == AUDIO_PATH_TYPE_PIPE)
    {
        uint8_t ctype;

        ctype = app_audio_path_pipe_codec(path);
        if (ctype == 0xFF)
        {
            APP_PRINT_ERROR2("audio_path_exec_release: err pipe ctype, in-out (%u->%u)",
                             path->it, path->ot);
            return -2;
        }
        app_audio_pipe_release(ctype);
    }

    else if (path->type == AUDIO_PATH_TYPE_TRACK)
    {
        uint32_t dev;

        dev = app_audio_path_track_dev(path);
        if (dev == 0xFFFFFFFF)
        {
            APP_PRINT_ERROR2("audio_path_exec_release: err track dev, in-out (%u, %u)",
                             path->it, path->ot);
            return -3;
        }
        app_audio_track_release(dev);
    }

    audio_path_types[path->type].busy++;
    path->state = APP_AUDIO_PATH_RELEASING;

    return 0;
}

static void audio_path_op_dequeue_type(uint8_t path_type)
{
    T_AUDIO_PATH_OP *op;
    T_AUDIO_PATH_OP *tmp;
    uint8_t opcode;
    T_UALIST_HEAD *op_list;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;
    uint8_t ret = 0;

    /* These conditions should never match. */
    /* if (path_type >= AUDIO_PATH_TYPE_MAX)
     * {
     *     return;
     * }
     */

    op_list = &audio_path_types[path_type].operation_list;

    /* If there is any path is in process, just wait for its completion, and
     * then this function would be invoked again when the path operation is
     * completed.
     */
    if (audio_path_types[path_type].busy)
    {
        ret = 1;
        goto error;
    }

    if (ualist_empty(op_list))
    {
        ret = 2;
        goto error;
    }

    op = ualist_first_entry(op_list, T_AUDIO_PATH_OP, list);
    if (op == NULL)
    {
        ret = 3;
        goto error;
    }
    ualist_del(&op->list);
    opcode = op->opcode;

    if (op->path)
    {
        APP_PRINT_INFO2("audio_path_op_dequeue_type path %d opcode 0x%x", op->path->id, op->opcode);
    }

    switch (opcode)
    {
    case AUDIO_PATH_OP_CREATE:
    case AUDIO_PATH_OP_REL:
        if (opcode == AUDIO_PATH_OP_CREATE)
        {
            audio_path_exec_creation(op);
        }
        else
        {
            audio_path_exec_release(op);
        }

        if (op->path)
        {
            /* Clear path->op. */
            APP_PRINT_INFO3("audio_path_op_dequeue_type, before free, op %x, path %x, op->path->op %x", op,
                            op->path, op->path->op);
            op->path->op = NULL;
        }
        free(op);

        /* TODO: Is the below code necessary for audio track?
         * Check the operation list, create path for the consecutive creation
         * operations in the list.
         * For multiple audio pipes, we must create the pipes simultaneously.
         * */
        ualist_for_each_safe(pos, next, op_list)
        {
            tmp = ualist_entry(pos, T_AUDIO_PATH_OP, list);

            if (tmp->opcode != opcode)
            {
                break;
            }
            if (opcode == AUDIO_PATH_OP_CREATE)
            {
                audio_path_exec_creation(tmp);
            }
            else
            {
                audio_path_exec_release(tmp);
            }
            if (tmp->path)
            {
                tmp->path->op = NULL;
            }
            ualist_del(&tmp->list);
            free(tmp);
        }
        break;
    default:
        APP_PRINT_ERROR1("audio_path_op_dequeue_type: Unknown opcode 0x%02x", opcode);
        break;
    }

error:
    APP_PRINT_ERROR1("audio_path_op_dequeue_type ret 0x%x", ret);
}

static int audio_path_op_enqueue(uint8_t opcode, T_AUDIO_PATH *path)
{
    uint8_t ret = 0;
    T_AUDIO_PATH_OP *op = NULL;

    if (opcode >= AUDIO_PATH_OP_MAX)
    {
        ret = 1;
        goto error;
    }

    if (!path || path->type >= AUDIO_PATH_TYPE_MAX)
    {
        ret = 2;
        goto error;
    }

    op = calloc(1, sizeof(*op));
    if (!op)
    {
        APP_PRINT_ERROR0("audio_path_op_enqueue: zalloc failed!");
        ret = 3;
        goto error;
    }

    APP_PRINT_INFO4("audio_path_op_enqueue: path %u, opcode %u, in-out (%u->%u)",
                    path->id, opcode, path->it, path->ot);

    op->opcode = opcode;
    op->path = path;
    path->op = op;
    ualist_add_tail(&op->list, &audio_path_types[path->type].operation_list);

    return 0;
error:
    APP_PRINT_ERROR1("audio_path_op_enqueue: ret 0x%x", ret);
    return 1;
}

static uint8_t audio_path_get_id(void)
{
    uint8_t id;
    uint8_t count = 0;

    while (count < AUDIO_PATH_ID_MAX)
    {
        id = next_path_id++;

        id &= AUDIO_PATH_ID_MAX;
        next_path_id &= AUDIO_PATH_ID_MAX;

        if (!path_id_state[id])
        {
            path_id_state[id] = 1;
            return id;
        }

        count++;
    }

    return 0xFF;
}

static T_AUDIO_PATH *app_audio_find_path_by_itot(T_UALIST_HEAD *list, uint8_t it, uint8_t ot)
{
    T_AUDIO_PATH *tmp;
    T_AUDIO_PATH *p_path = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;

    if (!list)
    {
        return NULL;
    }

    ualist_for_each_safe(pos, next, list)
    {
        /* For one it or ot, there is only one path, so we use OR operation
         * here.
         * */
        tmp = ualist_entry(pos, T_AUDIO_PATH, list);
        if (tmp->id & 0x80)
        {
            continue;
        }
        if (it != 0xff && it != tmp->it)
        {
            continue;
        }
        if (ot != 0xff && ot != tmp->ot)
        {
            continue;
        }

        p_path = tmp;
    }

    return p_path;
}

static void audio_path_ident_dup(T_AUDIO_PATH *dst, T_AUDIO_PATH *src)
{
    if (!dst || !src)
    {
        return;
    }

    if (src->ident)
    {
        dst->ident = calloc(1, strlen(src->ident) + 1);
        if (dst->ident)
        {
            strcpy(dst->ident, src->ident);
            dst->ident[strlen(src->ident)] = 0;
        }
    }
}

static void audio_path_list_release(T_UALIST_HEAD *path_list)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;
    T_AUDIO_PATH *tmp;
    t_audio_uapi_usr_cback uapi_cb;
    uint8_t id;

    if (!path_list)
    {
        return;
    }

    ualist_for_each_safe(pos, next, path_list)
    {
        tmp = ualist_entry(pos, T_AUDIO_PATH, list);
        uapi_cb = tmp->uapi_usr_cback;
        id = tmp->id;
        app_audio_path_free(tmp);
        if (uapi_cb)
        {
            uapi_cb(id, EVENT_AUDIO_PATH_RELEASED, NULL, 0, 0);
        }
    }
}

static int audio_path_update_id(T_AUDIO_PATH *path)
{
    if (path->type == AUDIO_PATH_TYPE_PIPE)
    {
        uint8_t ctype;

        ctype = app_audio_path_pipe_codec(path);
        if (ctype == 0xFF)
        {
            APP_PRINT_ERROR2("audio_path_update_id: err pipe ctype, in-out (%u->%u)",
                             path->it, path->ot);
            return -1;
        }
        if (!app_audio_pipe_change_uid(ctype, path->id))
        {
            APP_PRINT_ERROR2("audio_path_update_id: change pipe uid err, in-out (%u->%u)",
                             path->it, path->ot);
            return -2;
        }
    }
    else if (path->type == AUDIO_PATH_TYPE_TRACK)
    {
        uint32_t dev;

        dev = app_audio_path_track_dev(path);
        if (dev == 0xFFFFFFFF)
        {
            APP_PRINT_ERROR2("audio_path_update_id: err track dev, in-out (%u->%u)",
                             path->it, path->ot);
            return -3;
        }
        if (!app_audio_track_change_uid(dev, path->id))
        {
            APP_PRINT_ERROR2("audio_path_update_id: change track uid err, in-out (%u->%u)",
                             path->it, path->ot);
            return -4;
        }
    }
    else
    {
        return -5;
    }

    return 0;
}

static inline T_AUDIO_PATH *path_clone(T_AUDIO_PATH *dst, T_AUDIO_PATH *src)
{
#define C(x)    dst->x = src->x
    init_ualist_head(&dst->list);
    init_ualist_head(&dst->mix_action);
    C(id);
    C(type);
    C(codec_type);
    C(it);
    C(ot);
    dst->state = APP_AUDIO_PATH_IDLE;
    C(priority);
    C(mix);
    dst->renewed = 1;
    dst->pre_state = APP_AUDIO_PATH_IDLE;
    C(streaming);
    C(stream_idle_count);
    C(media_packets);
    C(frame_len);
    dst->ident = NULL;
    memcpy(&dst->ifmt, &src->ifmt, sizeof(dst->ifmt));
    memcpy(&dst->ofmt, &src->ofmt, sizeof(dst->ofmt));
    C(uapi_usr_cback);
    C(mgr_cback);
    dst->op = NULL;
    dst->cyclic = NULL;
    C(timer_idx_stream); /* TODO: Release src->timer_idx_stream here ? */
#undef C
    return dst;
}

static int codec_fmt_cmp(T_AUDIO_FORMAT_INFO *dst, T_AUDIO_FORMAT_INFO *src)
{
    /* TODO: Is there any simple comparison method ? */
    /* return memcmp(dst, src, sizeof(*src)); */

    if (!dst || !src)
    {
        return -1;
    }

    if (dst->type != src->type)
    {
        return -1;
    }

    switch (dst->type)
    {
    case AUDIO_FORMAT_TYPE_PCM:
        if (dst->attr.pcm.sample_rate != src->attr.pcm.sample_rate)
        {
            APP_PRINT_INFO2("codec_fmt_cmp: PCM sr %u != %u",
                            dst->attr.pcm.sample_rate,
                            src->attr.pcm.sample_rate);
            return -1;
        }
        return 0;
    case AUDIO_FORMAT_TYPE_CVSD:
        return memcmp(&dst->attr.cvsd, &src->attr.cvsd, sizeof(src->attr.cvsd));
    case AUDIO_FORMAT_TYPE_MSBC:
        return memcmp(&dst->attr.msbc, &src->attr.msbc, sizeof(src->attr.msbc));
    case AUDIO_FORMAT_TYPE_SBC:
#define SBC_CMP(x)  dst->attr.sbc.x != src->attr.sbc.x
        if (SBC_CMP(sample_rate))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: SBC sr %u != %u",
                            dst->attr.sbc.sample_rate,
                            src->attr.sbc.sample_rate);
            return -1;
        }
        if (SBC_CMP(block_length))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: SBC block_length %u != %u",
                            dst->attr.sbc.block_length,
                            src->attr.sbc.block_length);

            return -1;
        }
        if (SBC_CMP(chann_mode))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: SBC chann_mode %u != %u",
                            dst->attr.sbc.chann_mode,
                            src->attr.sbc.chann_mode);

            return -1;
        }
        if (SBC_CMP(allocation_method))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: SBC allocation_method %u != %u",
                            dst->attr.sbc.allocation_method,
                            src->attr.sbc.allocation_method);
            return -1;
        }
        if (SBC_CMP(subband_num))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: SBC subband_num %u != %u",
                            dst->attr.sbc.subband_num,
                            src->attr.sbc.subband_num);
            return -1;
        }
        if (SBC_CMP(bitpool))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: SBC bitpool %u != %u",
                            dst->attr.sbc.bitpool,
                            src->attr.sbc.bitpool);

            return -1;
        }
        return 0;
        /* return memcmp(&dst->attr.sbc, &src->attr.sbc, sizeof(src->attr.sbc)); */
#undef SBC_CMP
    case AUDIO_FORMAT_TYPE_AAC:
        return memcmp(&dst->attr.aac, &src->attr.aac, sizeof(src->attr.aac));
    case AUDIO_FORMAT_TYPE_OPUS:
        return memcmp(&dst->attr.opus, &src->attr.opus, sizeof(src->attr.opus));
    case AUDIO_FORMAT_TYPE_FLAC:
        return memcmp(&dst->attr.flac, &src->attr.flac, sizeof(src->attr.flac));
    case AUDIO_FORMAT_TYPE_MP3:
        return memcmp(&dst->attr.mp3, &src->attr.mp3, sizeof(src->attr.mp3));
    case AUDIO_FORMAT_TYPE_LC3:
#define LC3_CMP(x)  (dst->attr.lc3.x != src->attr.lc3.x)
        if (LC3_CMP(sample_rate))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: lc3 sampling_frequency %u != %u",
                            dst->attr.lc3.sample_rate,
                            src->attr.lc3.sample_rate);

            return -1;
        }
        if (LC3_CMP(chann_location))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: lc3 chann_location %u != %u",
                            dst->attr.lc3.chann_location,
                            src->attr.lc3.chann_location);

            return -1;
        }
        if (LC3_CMP(frame_duration))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: lc3 frame_duration %u != %u",
                            dst->attr.lc3.frame_duration,
                            src->attr.lc3.frame_duration);

            return -1;
        }
        if (LC3_CMP(frame_length))
        {
            APP_PRINT_INFO2("codec_fmt_cmp: lc3 frame_length %u != %u",
                            dst->attr.lc3.frame_length,
                            src->attr.lc3.frame_length);

            return -1;
        }
        return 0;
#undef LC3_CMP
    case AUDIO_FORMAT_TYPE_LDAC:
        return memcmp(&dst->attr.ldac, &src->attr.ldac, sizeof(src->attr.ldac));
    default:
        return -1;

    }
}

static int app_audio_path_create_core(uint8_t it, uint8_t ot, char *ident,
                                      T_AUDIO_FORMAT_INFO *ifmt,
                                      T_AUDIO_FORMAT_INFO *ofmt,
                                      t_audio_uapi_usr_cback uapi_usr_cback,
                                      uint8_t priority,
                                      T_AUDIO_PATH **dst_path)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PATH *tmp;
    T_UALIST_HEAD *path_list;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;
    T_UALIST_HEAD tmp_list;
    uint8_t type;
    uint8_t pid;
    int ret;

    if (!ifmt || !ofmt)
    {
        return -1;
    }

    APP_PRINT_INFO2("app_audio_path_create_core: it %u, ot %u", it, ot);
    APP_PRINT_INFO2("app_audio_path_create_core: ifmt type %d, ofmt type %d",
                    ifmt->type, ofmt->type);

    type = app_audio_path_type(it, ot);
    if (type >= AUDIO_PATH_TYPE_MAX)
    {
        return -2;
    }

    path_list = &audio_path_types[type].path_list;
    init_ualist_head(&tmp_list);

    /* If there is a path with same it or ot, we compare the priority and
     * release the old one if its priority is lower.
     * */
    tmp = app_audio_find_path_by_itot(path_list, it, ot);
    if (tmp && tmp->state != APP_AUDIO_PATH_PENDING_REL &&
        tmp->state != APP_AUDIO_PATH_RELEASING)
    {
        APP_PRINT_INFO5("app_audio_path_create_core: path id %u duplicate in-out "
                        "(%u-%u) (%u-%u)", tmp->id, tmp->it, tmp->ot, it, ot);
        if (priority < tmp->priority)
        {
            APP_PRINT_INFO3("app_audio_path_create_core: high prio path id %u "
                            "already exists, (%u<%u)", tmp->id, priority,
                            tmp->priority);
            return -3;
        }

        if (tmp->it == it && tmp->ot == ot &&
            tmp->uapi_usr_cback == uapi_usr_cback &&
            !codec_fmt_cmp(&tmp->ifmt, ifmt) &&
            !codec_fmt_cmp(&tmp->ofmt, ofmt))
        {

            APP_PRINT_INFO3("app_audio_path_create_core: path id %u, same %u->%u, "
                            "fmt ...", tmp->id, it, ot);
            return tmp->id;
        }

        ret = audio_path_op_enqueue(AUDIO_PATH_OP_REL, tmp);
        if (ret)
        {
            APP_PRINT_ERROR3("app_audio_path_create_core: add rel err (%d), path (type: %u, id :%u)",
                             ret, tmp->type, tmp->id);
            return -4;
        }
        tmp->state = APP_AUDIO_PATH_PENDING_REL;
    }

    /* For IPC 2.0, we don't need to release other running paths and re-create
     * them.
     * */
    if (ipc_version == 2)
    {
        goto label_creation;
    }

    /* Check the path list, if the path in it is running, add a release
     * operation for it */
    ualist_for_each_safe(pos, next, path_list)
    {
        tmp = ualist_entry(pos, T_AUDIO_PATH, list);

        /* Ignore the path that would be released temporarily. */
        if (tmp->id & 0x80)
        {
            continue;
        }

        /* TODO: path->state should never be AUDIO_PATH_RELEASED, because it
         * would be removed from path list and freed when it is released. */
        if (tmp->state != APP_AUDIO_PATH_IDLE &&
            tmp->state != APP_AUDIO_PATH_RELEASING &&
            tmp->state != APP_AUDIO_PATH_PENDING_REL)
        {
            /* We shall keep the pid unchanged that is used for upper layer to
             * identify the audio path. But for the temporary path release,
             * we should use a temporary pid whoes highest bit is set to 1.
             * */
            /*
             * Keep the path in the list until it is really released.
             * */
            ret = audio_path_op_enqueue(AUDIO_PATH_OP_REL, tmp);
            if (ret)
            {
                APP_PRINT_ERROR3("app_audio_path_create_core: add rel op failed (%d), path (type: %u, id :%u)",
                                 ret, tmp->type, tmp->id);
                ret = -5;
                goto err;
            }

            path = calloc(1, sizeof(*path));
            if (!path)
            {
                APP_PRINT_ERROR2("app_audio_path_create_core: alloc failed for path (type: %u, id :%u)",
                                 tmp->type, tmp->id);
                /* FIXME: The tmp->op shall not be null. It was set in the
                 * above queuing REL operation.
                 * */
                if (tmp->op)
                {
                    T_AUDIO_PATH_OP *tmp_op = tmp->op;

                    /* Delete and free the op, and restore path state and id */
                    ualist_del(&tmp_op->list);
                    free(tmp_op);
                }
                else
                {
                    APP_PRINT_ERROR2("app_audio_path_create_core: tmp path (type: %u, id :%u) op is null",
                                     tmp->type, tmp->id);
                }
                ret = -6;
                goto err;
            }
            init_ualist_head(&path->list);
            init_ualist_head(&path->mix_action);

            /* TODO: Be careful with the pointers. */
            /* memcpy(path, tmp, sizeof(*path)); */
            path_clone(path, tmp);

            /* Save the state before releasing. It will be used to determine if
             * pipe/track mgr event should be sent to upper layer. Upper layer
             * wouldn't want to receive double created or started events.
             * */
            path->pre_state = tmp->state;

            tmp->id |= 0x80;
            tmp->state = APP_AUDIO_PATH_PENDING_REL;
            /* In above memcpy, we transfer the stream state and timer_idx_stream
             * to the renewed path. So we detach them from old path.
             * */
            tmp->timer_idx_stream = 0;
            tmp->streaming = 0;
            audio_path_update_id(tmp);

            /* op will be set later.
             * when the path is exec created, path->buf may be set.
             * */
            init_ualist_head(&path->list);
            path->op = NULL;
            path->cyclic = NULL;
            path->state = APP_AUDIO_PATH_IDLE;
            path->renewed = 1;
            /* Keep using the old path id. Because upper layer would still use
             * the id.
             * */
            path->ident = NULL;
            audio_path_ident_dup(path, tmp);
            ualist_add_tail(&path->list, &tmp_list);
        }
    }

label_creation:
    /* Add creation operation for renewed audio paths */
    ualist_for_each_safe(pos, next, &tmp_list)
    {
        t_audio_uapi_usr_cback uapi_cb;
        uint8_t tmp_id;

        tmp = ualist_entry(pos, T_AUDIO_PATH, list);

        ret = audio_path_op_enqueue(AUDIO_PATH_OP_CREATE, tmp);
        if (ret)
        {
            APP_PRINT_ERROR3("app_audio_path_create_core: add cr op failed (%d), path (type: %u, id :%u)",
                             ret, tmp->type, tmp->id);

            uapi_cb = tmp->uapi_usr_cback;
            tmp_id = tmp->id;
            app_audio_path_free(tmp);
            if (uapi_cb)
            {
                uapi_cb(tmp_id, EVENT_AUDIO_PATH_RELEASED, NULL, 0, 0);
            }
        }
    }

    /* Add renewed audio paths to global path list tail. */
    ualist_join_tail(&tmp_list, path_list);
    init_ualist_head(&tmp_list);

    pid = audio_path_get_id();
    if (pid == 0xFF)
    {
        APP_PRINT_ERROR0("app_audio_path_create_core: All path ids are in use");
        ret = -7;
        goto err;
    }

    path = calloc(1, sizeof(*path));
    if (!path)
    {
        APP_PRINT_ERROR2("app_audio_path_create_core: alloc2 failed for path (type: %u, id :%u)",
                         type, pid);
        ret = -8;
        goto err;
    }

    if (ident)
    {
        path->ident = calloc(1, strlen(ident) + 1);
        if (path->ident)
        {
            strcpy(path->ident, ident);
            path->ident[strlen(ident)] = 0;
        }
        else
        {
            /* TODO: We do not return, because the ident is not necessary. */
            APP_PRINT_ERROR2("app_audio_path_create_core: alloc ident failed for path (type: %u, id :%u)",
                             type, pid);
        }
    }

    init_ualist_head(&path->list);
    init_ualist_head(&path->mix_action);
    path->id = pid;
    path->it = it;
    path->ot = ot;
    path->type = type;
    path->state = APP_AUDIO_PATH_IDLE;
    path->priority = priority;
    path->codec_type = app_audio_path_codec_type(it, ot);
    memcpy(&path->ifmt, ifmt, sizeof(path->ifmt));
    memcpy(&path->ofmt, ofmt, sizeof(path->ofmt));
    path->uapi_usr_cback = uapi_usr_cback;

    switch (type)
    {
    case AUDIO_PATH_TYPE_PIPE:
        if (it == IT_UDEV_IN1 || it == IT_UDEV_IN2 || ot == OT_UDEV_OUT1)
        {
            path->mgr_cback = audio_mgr_pipe_cback;
            break;
        }
        break;
    case AUDIO_PATH_TYPE_TRACK:
        if (it == IT_MIC || it == IT_AUX || ot == OT_SPK || ot == OT_AUX)
        {
            path->mgr_cback = audio_mgr_track_cback;
            break;
        }
        break;
    default:
        APP_PRINT_ERROR2("app_audio_path_create_core: Unknown path path (type: %u, id :%u)",
                         type, pid);
        ret = -9;
        goto err2;
    }

    ualist_add_tail(&path->list, path_list);

    ret = audio_path_op_enqueue(AUDIO_PATH_OP_CREATE, path);
    if (ret)
    {
        APP_PRINT_ERROR2("app_audio_path_create_core: add2 cr op failed for path (type: %u, id :%u)",
                         type, pid);
        ret = -10;
        goto err2;
    }

    if (dst_path)
    {
        *dst_path = path;
    }

    /* Return the path id for later filling and release by upper layer. */
    return pid;

err2:
    app_audio_path_free(path);
err:
    audio_path_list_release(&tmp_list);
    return ret;
}

int app_audio_path_create(uint8_t it, uint8_t ot, char *ident,
                          T_AUDIO_FORMAT_INFO *ifmt,
                          T_AUDIO_FORMAT_INFO *ofmt,
                          t_audio_uapi_usr_cback uapi_usr_cback,
                          uint8_t priority)
{
    int ret;
    uint8_t type;

    ret = app_audio_path_create_core(it, ot, ident, ifmt, ofmt, uapi_usr_cback, priority, NULL);
    type = app_audio_path_type(it, ot);
    if (type >= AUDIO_PATH_TYPE_MAX)
    {
        APP_PRINT_ERROR1("app_audio_path_create: invalid type %u", type);
        return -1;
    }
    /* kick off */
    audio_path_op_dequeue_type(type);

    return ret;
}

static inline void audio_path_op_dequeue(void)
{
    uint8_t i;

    for (i = 0; i < AUDIO_PATH_TYPE_MAX; i++)
    {
        audio_path_op_dequeue_type(i);
    }
}

int app_audio_path_createv(const struct path_iovec *iov, int iovcnt, uint8_t *ids)
{
    APP_PRINT_INFO0("app_audio_path_createv");
    int i;
    int ret;
    T_AUDIO_PATH *path = NULL;

    memset(ids, 0xff, iovcnt);

    for (i = 0; i < iovcnt; i++)
    {
        path = NULL;
        ret = app_audio_path_create_core(iov[i].it, iov[i].ot, iov[i].ident,
                                         iov[i].ifmt, iov[i].ofmt,
                                         iov[i].uapi_cback,
                                         iov[i].priority,
                                         &path);
        if (ret < 0)
        {
            APP_PRINT_ERROR1("app_audio_path_createv: Create %ith path failure", i);
            audio_path_op_dequeue();
            return ret;
        }
        else
        {
            if (path && iov[i].mix)
            {
                path->mix = iov[i].mix;

                path->mix &= MIX_PATHS_MASK;
                ualist_add_tail(&path->mix_action, &mix_paths[path->mix]);
            }
            ids[i] = (uint8_t)ret;
        }
    }

    /* kick off */
    audio_path_op_dequeue();

    return 0;
}

static int app_audio_single_path_rel(T_AUDIO_PATH *path)
{
    uint8_t tmp_id;
    uint8_t streaming;
    t_audio_uapi_usr_cback uapi_cback;
    int ret;

    if (!path)
    {
        return -1;
    }

    uapi_cback = path->uapi_usr_cback;
    streaming = path->streaming;
    /* if a path is not activated, we just remove it and notify the upper
     * layer.
     *
     * if a path was releasing, it means that previously upper layer called
     * this func with the same path id.
     * */
    if (path->state == APP_AUDIO_PATH_IDLE)
    {
        if (path->op)
        {
            T_AUDIO_PATH *op = path->op;
            ualist_del(&op->list);
            free(op);
            path->op = NULL;
        }
        tmp_id = path->id;
        app_audio_path_free(path);
        if (uapi_cback)
        {
            /* Maybe the path is renewed one, its stream state is unchanged. */
            if (streaming)
            {
                uapi_cback(tmp_id, EVENT_AUDIO_PATH_STREAM_STOPPED, NULL, 0, 0);
            }
            uapi_cback(tmp_id, EVENT_AUDIO_PATH_RELEASED, NULL, 0, 0);
        }
        return 0;
    }
    else if (path->state == APP_AUDIO_PATH_PENDING_REL)
    {
        return -2;
    }

    else if (path->state == APP_AUDIO_PATH_RELEASING)
    {
        return -3;
    }

    /* when audio pipe release, app will get event stopped and the released */
    else if (path->state == APP_AUDIO_PATH_STOPPED)
    {
        return -4;
    }

    ret = audio_path_op_enqueue(AUDIO_PATH_OP_REL, path);
    if (ret)
    {
        return -5;
    }
    path->state = APP_AUDIO_PATH_PENDING_REL;

    return 0;
}

static int app_audio_path_release_by_itot_internal(uint8_t it, uint8_t ot)
{
    T_AUDIO_PATH *path;
    uint8_t i;
    int ret;

    for (i = 0; i < AUDIO_PATH_TYPE_MAX; i++)
    {
        path = app_audio_find_path_by_itot(&audio_path_types[i].path_list, it, ot);
        if (!path)
        {
            continue;
        }

        ret = app_audio_single_path_rel(path);
        if (ret)
            APP_PRINT_ERROR5("app_audio_path_release_by_itot_internal: release "
                             "err %d, path (type: %u, id :%u), iot (%u->%u)",
                             ret, path->type, path->id, path->it, path->ot);
        else
            APP_PRINT_INFO4("app_audio_path_release_by_itot_internal: release "
                            "path (type: %u, id :%u), iot (%u->%u)", path->type, path->id,
                            path->it, path->ot);
        return ret;
    }

    APP_PRINT_WARN2("app_audio_path_release_by_itot_internal: No path with iot (%u->%u)",
                    it, ot);
    return -1;
}

void app_audio_path_releasev_itot(const struct rel_iovec *iov, uint8_t count)
{
    uint8_t i;

    if (!iov || !count)
    {
        return;
    }

    APP_PRINT_INFO0("app_audio_path_releasev_itot");

    for (i = 0; i < count; i++)
    {
        app_audio_path_release_by_itot_internal(iov[i].it, iov[i].ot);
    }
    audio_path_op_dequeue();
}

void app_audio_path_release_by_itot(uint8_t it, uint8_t ot)
{
    app_audio_path_release_by_itot_internal(it, ot);
    audio_path_op_dequeue();
}

int app_audio_path_flush(uint8_t it)
{
    T_CYCLIC_BUF *cyclic = NULL;
    uint16_t count = 0;
    bool ret = false;

    cyclic = audio_path_get_cyclic(it);

    if (!cyclic)
    {
        return -1;
    }

    count = cyclic_buf_count(cyclic);
    if (count > 0)
    {
        ret = cyclic_buf_drop(cyclic, count);
        if (ret == false)
        {
            return -1;
        }
    }

    return count;
}

/* Be careful, this func might be called in interrupt context. */
RAM_TEXT_SECTION
int app_audio_path_fill_cyclic(uint8_t it, uint8_t *buf, uint16_t len,
                               uint8_t flag, uint32_t timestamp)
{
    T_CYCLIC_BUF *cyclic = NULL;
    uint16_t hdr_len = 0;
    uint16_t count = 0;
    struct cyclic_ihdr hdr;
    bool rc;

    switch (it)
    {
    case IT_SBC:
    case IT_MSBC:
    case IT_LC3FRM:
    case IT_RELAY_CODE1:
        hdr_len = sizeof(hdr);
        cyclic = audio_path_get_code_cyclic(it);
        break;
    case IT_UDEV_IN1:
    case IT_UDEV_IN2:
        cyclic = audio_path_get_pcm_cyclic(it);
        break;
    default:
        APP_PRINT_ERROR1("app_audio_path_fill_cyclic: Invalid input terminal %u", it);
        return -2;
    }

    /* The content in cyclic buf: len, it, flag, ts, data... */
    if (cyclic_buf_room(cyclic) < hdr_len + len)
    {
        APP_PRINT_ERROR3("app_audio_path_fill_cyclic: iterminal %u has no enough"
                         " room, len %u/%u", it, len,
                         cyclic_buf_room(cyclic));
        return -3;
    }

    if (hdr_len > 0)
    {
        hdr.len = hdr_len + len;
        hdr.it = it;
        hdr.flag = flag;
        hdr.timestamp = timestamp;
        rc = cyclic_buf_write(cyclic, (void *)&hdr, sizeof(hdr));
        if (!rc)
        {
            APP_PRINT_ERROR1("app_audio_path_fill_cyclic: iterm %u writes hdr err", it);
            return -4;
        }
        count += sizeof(hdr);
    }
    /* TODO: Write failure should never happen, otherwise the cyclic buf would
     * be messed up and there might not be any valid frame in buf. Because we
     * can't find where the correct position of the headers.
     * */
    rc = cyclic_buf_write(cyclic, buf, len);
    if (!rc)
    {
        APP_PRINT_ERROR2("app_audio_path_fill_cyclic: iterm %u write data (%u)",
                         it, len);
        return -5;
    }
    count += len;

    return count;
}

RAM_TEXT_SECTION
int app_audio_path_send_fill_msg(uint8_t it)
{
    uint16_t subtype;
    uint8_t evt;
    T_IO_MSG msg;
    bool rc;

    switch (it)
    {
    case IT_SBC:
    case IT_MSBC:
    case IT_LC3FRM:
    case IT_RELAY_CODE1:
        subtype = MSG_TYPE_CODE_RECV;
        break;
    case IT_UDEV_IN1:
    case IT_UDEV_IN2:
        subtype = MSG_TYPE_PCM_RECV;
        break;
    default:
        APP_PRINT_ERROR1("app_audio_path_send_fill_msg: Invalid input terminal %u", it);
        return -1;
    }

    if (!audio_path_evt_q || !audio_path_msg_q)
    {
        return -2;
    }

    evt = EVENT_IO_TO_APP;
    msg.type    = IO_MSG_TYPE_APP_AUDIO_PATH;
    msg.subtype = subtype;
    msg.u.param = it;

    rc = os_msg_send(audio_path_msg_q, &msg, 0);
    if (!rc)
    {
        APP_PRINT_ERROR0("app_audio_path_send_fill_msg: send msg fail");
        return -3;
    }

    rc = os_msg_send(audio_path_evt_q, &evt, 0);
    if (!rc)
    {
        APP_PRINT_ERROR0("app_audio_path_send_fill_msg: Send evt fail");
        return -4;
    }

    return 0;
}

RAM_TEXT_SECTION
int app_audio_path_fill_async(uint8_t it, uint8_t *buf, uint16_t len,
                              uint8_t flag, uint32_t timestamp)
{
    int ret;
    uint16_t count;
    uint16_t threshold = 0;
    T_CYCLIC_BUF *cyclic = NULL;

    switch (it)
    {
    case IT_SBC:
    case IT_MSBC:
    case IT_LC3FRM:
    case IT_RELAY_CODE1:
        cyclic = audio_path_get_code_cyclic(it);
        break;
    case IT_UDEV_IN1:
    case IT_UDEV_IN2:
        cyclic = audio_path_get_pcm_cyclic(it);
        threshold = 512;
        break;
    default:
        APP_PRINT_ERROR1("app_audio_path_fill_async: Invalid input terminal %u", it);
        return -1;
    }

    ret = app_audio_path_fill_cyclic(it, buf, len, flag, timestamp);
    if (ret <= 0)
    {
        APP_PRINT_ERROR1("app_audio_path_fill_async: fill err, ret %d", ret);
        return -2;
    }

    count = cyclic_buf_count(cyclic);
    if (count < threshold)
    {
        return 0;
    }

    ret = app_audio_path_send_fill_msg(it);

    return ret;
}

/* It is invoked in USB isochronous interrupt context. */
RAM_TEXT_SECTION
void app_audio_path_resume_dec(uint8_t ot, uint16_t data_sent)
{
    T_IO_MSG msg;
    uint8_t  evt;
    bool rc;

    if (ot != OT_UDEV_OUT1)
    {
        return;
    }

    if (!audio_path_evt_q || !audio_path_msg_q)
    {
        APP_PRINT_ERROR0("app_audio_path_resume_dec: evt/msg queue is null");
        return;
    }

    if (!data_sent)
    {
        /* if the pre-defined dummy data is sent, it means no change on
         * watermark. So we don't need to send signal to app task to resume
         * decoding. */
        return;
    }

    evt = EVENT_IO_TO_APP;
    msg.type    = IO_MSG_TYPE_APP_AUDIO_PATH;
    msg.subtype = MSG_TYPE_AUDIO_PIPE_RESUME_DEC;
    msg.u.param = ((uint32_t)data_sent << 16);

    rc = os_msg_send(audio_path_msg_q, &msg, 0);
    if (!rc)
    {
        APP_PRINT_ERROR0("app_audio_path_resume_dec: send msg fail");
        return;
    }

    rc = os_msg_send(audio_path_evt_q, &evt, 0);
    if (!rc)
    {
        APP_PRINT_ERROR0("app_audio_path_resume_dec: send event fail");
        return;
    }
}

static void app_audio_path_process_code(T_AUDIO_PATH *path)
{
    struct cyclic_ihdr hdr;
    T_CYCLIC_BUF *cyclic;
    uint8_t *buf = NULL;
    uint16_t frame_len = 0;

    if (!path)
    {
        return;
    }

    cyclic = path->cyclic;

    /* It is normal if the cyclic is empty. We also read cyclic buf and fill
     * data to decoder when filled event is received.
     * */
    if (cyclic_buf_count(cyclic) < sizeof(hdr))
    {
        /* APP_PRINT_ERROR1("app_audio_path_process_code: less count %u",
         *                  cyclic_buf_count(cyclic));
         */
        return;
    }

    if (cyclic_buf_peek(cyclic, (void *)&hdr, sizeof(hdr)))
    {
        frame_len = hdr.len;
    }
    else
    {
        APP_PRINT_ERROR0("app_audio_path_process_code: peek hdr err");
        return;
    }

    if (!frame_len || frame_len <= sizeof(hdr))
    {
        return;
    }

    if (path->type == AUDIO_PATH_TYPE_PIPE)
    {
        uint8_t ctype;

        ctype = app_audio_path_pipe_codec(path);
        if (ctype == 0xFF)
        {
            APP_PRINT_ERROR2("app_audio_path_process_code: err pipe ctype, in-out (%u->%u)",
                             path->it, path->ot);
            goto err;
        }
        /* Be careful, pipe uses the code cyclic buf from app audio path. */
        combined_pipe_fill(ctype, path);
    }
    else if (path->type == AUDIO_PATH_TYPE_TRACK)
    {
        uint32_t dev;
        bool rc;

        buf = calloc(1, frame_len);
        if (!buf)
        {
            APP_PRINT_ERROR3("app_audio_path_process_code: alloc buf err, path "
                             "(%u->%u), len %u", path->it, path->ot, frame_len);
            return;
        }

        if (!cyclic_buf_read(cyclic, buf, frame_len))
        {
            APP_PRINT_ERROR2("app_audio_path_process_code: Read frame err, path (%u->%u)",
                             path->it, path->ot);
            goto err;
        }

        dev = app_audio_path_track_dev(path);
        if (dev == 0xFFFFFFFF)
        {
            APP_PRINT_ERROR2("app_audio_path_process_code: err track dev, in-out (%u->%u)",
                             path->it, path->ot);
            goto err;
        }
        /* TODO: Is it an error if we fill data to track prior to the filled
         * event for the previous data filling.
         * */
        frame_len -= sizeof(hdr);
        rc = app_audio_track_write_with_flag(dev, buf + sizeof(hdr), hdr.flag,
                                             frame_len, 1, hdr.timestamp);
        if (!rc)
        {
            APP_PRINT_ERROR3("app_audio_path_process_code: Write track error, "
                             "flag %u, len %u, ts %08x", hdr.flag,
                             frame_len, hdr.timestamp);
        }
        else
        {
            /* TODO: When we received filled event, we would decrease the busy
             * variable.
             * */
            /* audio_path_types[path->type].busy++; */
        }
    }

err:
    if (buf)
    {
        free(buf);
    }
    return;
}

static void app_audio_path_process_pcm(T_AUDIO_PATH *path)
{
    uint32_t tmp;

    if (!path)
    {
        return;
    }

    switch (path->type)
    {
    case AUDIO_PATH_TYPE_PIPE:
        tmp = app_audio_path_pipe_codec(path);
        if ((uint8_t)tmp == 0xFF)
        {
            APP_PRINT_ERROR2("app_audio_path_process_pcm: err pipe ctype, in-out (%u->%u)",
                             path->it, path->ot);
            return;
        }

        /* Be careful, pipe uses the pcm cyclic buf from app audio path. */
        combined_pipe_fill((uint8_t)tmp, path);

        break;
    case AUDIO_PATH_TYPE_TRACK:
        APP_PRINT_ERROR0("app_audio_path_process_pcm: Unexpected track data");
        return;
    default:
        APP_PRINT_ERROR1("app_audio_path_process_pcm: Unknown path type %u", path->type);
        return;
    }

}

static int app_audio_pcm_drop(T_CYCLIC_BUF *cyclic)
{
    uint16_t count = 0;
    uint16_t threshold;
    uint8_t depth;

    if (!cyclic)
    {
        return -1;
    }

    /* TODO: Be careful with the threshold, it may cause that producer can't
     * fill data to cyclic buf and we drop nothing. It's a deadlock until one
     * path is created for consuming the data.
     * */
    threshold = (cyclic->len / 2);
    count = cyclic_buf_count(cyclic);
    if (count <= threshold)
    {
        return 0;
    }

    count -= threshold;

    /* FIXME: Align the count to audio bit depth.
     * We don't know the precise bit depth, we use rough value that is
     * enough for us. 24-byte is the greatest common divisor for stereo
     * audio with 8-bit or 16-bit or 24-bit or 32-bit bit depth.
     * */
    depth = 24;
    count /= depth;
    count *= depth;

    if (count)
    {
        if (!cyclic_buf_drop(cyclic, count))
        {
            return -2;
        }
    }

    return count;
}

static int app_audio_code_drop(T_CYCLIC_BUF *cyclic)
{
    uint16_t count = 0;
    uint16_t threshold;
    uint16_t n = 0;
    struct cyclic_ihdr hdr;

    if (!cyclic)
    {
        return -1;
    }

    threshold = (cyclic->len / 2);
    count = cyclic_buf_count(cyclic);
    if (count <= threshold)
    {
        return 0;
    }

    count -= threshold;

    while (n < count)
    {
        if (!cyclic_buf_peek(cyclic, (void *)&hdr, sizeof(hdr)))
        {
            return n;
        }
        if (n + hdr.len > count)
        {
            return n;
        }
        if (!cyclic_buf_drop(cyclic, hdr.len))
        {
            return n;
        }
        n += hdr.len;
    }

    return n;
}

void app_audio_path_handle_msg(T_IO_MSG *msg)
{
    uint16_t subtype;
    uint8_t it;
    uint8_t i;
    T_AUDIO_PATH *path = NULL;

    if (!msg)
    {
        return;
    }

    subtype = msg->subtype;

    switch (subtype)
    {
    case MSG_TYPE_AUDIO_PIPE_CODEC:
    case MSG_TYPE_AUDIO_PIPE_RESUME_DEC:
        app_audio_pipe_handle_msg(msg);
        return;
    default:
        break;
    }

    it = (uint8_t)msg->u.param;

    if (subtype != MSG_TYPE_PCM_RECV && subtype != MSG_TYPE_CODE_RECV)
    {
        APP_PRINT_ERROR1("app_audio_path_handle_msg: Received err subtype %u",
                         subtype);
        return;
    }

    for (i = 0; i < AUDIO_PATH_TYPE_MAX; i++)
    {
        path = app_audio_find_path_by_itot(&audio_path_types[i].path_list, it, 0xFF);
        if (path)
        {
            break;
        }
    }

    if (!path || path->state != APP_AUDIO_PATH_STARTED)
    {
        T_CYCLIC_BUF *cyclic;
        int ret = 0;

        if (subtype == MSG_TYPE_PCM_RECV)
        {
            cyclic = audio_path_get_pcm_cyclic(it);
            if (!cyclic)
            {
                return;
            }
            ret = app_audio_pcm_drop(cyclic);
        }
        else if (subtype == MSG_TYPE_CODE_RECV)
        {
            cyclic = audio_path_get_code_cyclic(it);
            if (!cyclic)
            {
                return;
            }
            ret = app_audio_code_drop(cyclic);
        }

        if (ret > 0 || ret < 0)
            /*APP_PRINT_WARN2("app_audio_path_handle_msg: iterminal %u drop %d",
                            it, ret);*/

        {
            return;
        }
    }

    switch (subtype)
    {
    case MSG_TYPE_PCM_RECV:
        app_audio_path_process_pcm(path);
        break;
    case MSG_TYPE_CODE_RECV:
        app_audio_path_process_code(path);
        break;
    default:
        APP_PRINT_ERROR1("app_audio_path_handle_msg: Invalid msg type %04x", subtype);
        return;
    }
}

/* FIXME: It's dangerous to call this func and the fill async func
 * simultaneously for the same cyclic buf. There might be a race condition for
 * changing the cyclic buffer write pointer.
 * */
int app_audio_path_fill(uint8_t it, uint8_t *buf, uint16_t len, uint8_t flag,
                        uint32_t timestamp)
{
    T_AUDIO_PATH *path;
    uint8_t i;
    int ret;

    ret = app_audio_path_fill_cyclic(it, buf, len, flag, timestamp);
    if (ret <= 0)
    {
        APP_PRINT_WARN1("app_audio_path_fill: Fill cyclic err %d", ret);
    }

    for (i = 0; i < AUDIO_PATH_TYPE_MAX; i++)
    {
        path = app_audio_find_path_by_itot(&audio_path_types[i].path_list, it, 0xFF);
        if (!path || path->state != APP_AUDIO_PATH_STARTED)
        {
            continue;
        }
        if (path->codec_type == AUDIO_PATH_CODEC_ENC)
        {
            app_audio_path_process_pcm(path);
        }
        else if (path->codec_type == AUDIO_PATH_CODEC_DEC)
        {
            app_audio_path_process_code(path);
        }
    }

    return 0;
}

/* TODO: For usb upstream. */
RAM_TEXT_SECTION uint32_t app_audio_path_watermark(uint8_t ot)
{
    if (ot == OT_UDEV_OUT1)
    {
        return app_audio_pipe_watermark();
    }
    else
    {
        return 0;
    }
}

/* TODO: For lc3enc pipe volume */
bool app_audio_path_set_pipe_volume(uint16_t value)
{
    APP_PRINT_INFO1("app_audio_path_set_pipe_volume vol %x", value);
    return app_audio_pipe_set_volume(value);
}

bool app_audio_path_set_gain(uint8_t it, uint8_t ot, uint16_t gain)
{
    T_AUDIO_PATH *path;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    uint8_t i;
    uint8_t ctype;


    for (i = 0; i < AUDIO_PATH_TYPE_MAX; i++)
    {
        ualist_for_each_safe(pos, n, &audio_path_types[i].path_list)
        {
            path = ualist_entry(pos, T_AUDIO_PATH, list);

            if (path->it == it && path->ot == ot)
            {
                if (path->type == AUDIO_PATH_TYPE_PIPE)
                {
                    ctype = app_audio_path_pipe_codec(path);
                    if (ctype == 0xff)
                    {
                        APP_PRINT_ERROR0("app_audio_path_set_gain: No pipe");
                        return false;
                    }
                    return app_audio_pipe_set_gain(ctype, gain);
                }
                else
                {
                    APP_PRINT_ERROR1("app_audio_path_set_gain: type (%u) is not supported",
                                     path->type);
                    return false;
                }
            }
        }
    }

    return 0;
}

static void audio_path_stream_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    uint8_t i;
    T_AUDIO_PATH *path;
    uint8_t pid = param & AUDIO_PATH_ID_MAX;

    for (i = 0; i < AUDIO_PATH_TYPE_MAX; i++)
    {
        path = app_audio_path_get_by_id(&audio_path_types[i].path_list, pid);
        if (path)
        {
            break;
        }
    }

    if (!path)
    {
        return;
    }

    /* timer_idx_stream is stopped before task deals with the gap timer msg. */
    if (!path->timer_idx_stream)
    {
        return;
    }

    if (path->state != APP_AUDIO_PATH_STARTED)
    {
        return;
    }

    if (!path->streaming)
    {
        return;
    }

    if (path->media_packets > 0)
    {
        path->stream_idle_count = 0;
    }
    else
    {
        path->stream_idle_count++;
    }

    path->media_packets = 0;

    if (path->stream_idle_count >= STREAM_IDLE_MAX)
    {
        APP_PRINT_INFO3("audio_path_stream_timeout_cb: stream %u, %u->%u stopped",
                        pid, path->it, path->ot);
        app_audio_path_stream_stopped(path);
    }
    else
    {
        app_start_timer(&path->timer_idx_stream, "stream_timer",
                        audio_path_stream_timer_id, STREAM_TIMER_ID, pid, false,
                        STREAM_IDLE_TIMEDOUT);
    }
}

int app_audio_path_format_by_id(uint8_t id, T_AUDIO_FORMAT_INFO **ifmt, T_AUDIO_FORMAT_INFO **ofmt)
{
    T_AUDIO_PATH *path;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *next = NULL;
    uint8_t i;

    for (i = 0; i < AUDIO_PATH_TYPE_MAX; i++)
    {
        ualist_for_each_safe(pos, next, &audio_path_types[i].path_list)
        {
            path = ualist_entry(pos, T_AUDIO_PATH, list);
            if (id == path->id)
            {
                *ifmt = &(path->ifmt);
                *ofmt = &(path->ofmt);
                return 0;
            }
        }
    }

    APP_PRINT_WARN1("app_audio_path_format_by_id: No path with id %u", id);
    return -1;
}

int app_audio_path_reg_monitor(t_audio_monitor_cback cback)
{
    struct audio_path_monitor *monitor;

    monitor = calloc(1, sizeof(*monitor));
    if (!monitor)
    {
        APP_PRINT_ERROR0("app_audio_path_reg_monitor: alloc monitor err");
        return -1;
    }
    init_ualist_head(&monitor->list);
    monitor->cb = cback;
    ualist_add_tail(&monitor->list, &audio_monitor_list);

    return 0;
}

bool app_audio_path_init(void *evt_queue, void *msg_queue, uint8_t ipc_ver)
{
    uint8_t i;

    audio_path_evt_q = evt_queue;
    audio_path_msg_q = msg_queue;
    ipc_version = ipc_ver;

    for (i = 0; i < AUDIO_PATH_TYPE_MAX; i++)
    {
        init_ualist_head(&audio_path_types[i].path_list);
        init_ualist_head(&audio_path_types[i].operation_list);
    }

    for (i = 0; i < MIX_PATHS_MAX; i++)
    {
        init_ualist_head(&mix_paths[i]);
    }

    /* FIXME: alloc more buffers for paths.
     * Currently we only have one PCM input terminal, so we allocate a PCM buf,
     * For all decoding paths, the data in the code buf is packet-based, so
     * they can share a common buf. But it is better for using dedicated buf
     * for each input terminal with code stream.
     * */

    if (!cyclic_buf_init(&it_code_buf, IT_CODE_BUF_SIZE))
    {
        APP_PRINT_ERROR0("app_audio_path_init: alloc input code buf err");
        goto err;
    }
    if (!cyclic_buf_init(&it_pcm_buf1, IT_PCM_BUF_SIZE))
    {
        APP_PRINT_ERROR0("app_audio_path_init: alloc input pcm buf err");
        goto err;
    }

    if (ipc_version == 2)
    {
        if (!cyclic_buf_init(&it_pcm_buf2, IT_PCM_BUF_SIZE))
        {
            APP_PRINT_ERROR0("app_audio_path_init: alloc pcm buf2 err");
            goto err;
        }
    }

    app_audio_pipe_init(evt_queue, msg_queue, IO_MSG_TYPE_APP_AUDIO_PATH);
    app_audio_track_init();

    app_timer_reg_cb(audio_path_stream_timeout_cb, &audio_path_stream_timer_id);

    return true;
err:
    cyclic_buf_deinit(&it_code_buf);
    cyclic_buf_deinit(&it_pcm_buf1);
    return false;
}
