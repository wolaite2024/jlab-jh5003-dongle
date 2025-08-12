/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "os_mem.h"
#include "os_msg.h"
#include "os_queue.h"
#include "trace.h"
#include "sys_event.h"
#include "audio_mgr.h"
#include "audio_codec.h"
#include "audio_path.h"
#include "dsp_mgr.h"
#include "codec_mgr.h"
#include "anc_mgr.h"
#include "sport_mgr.h"

/* TODO Remove Start */
#include "bin_loader.h"
#include "bt_types.h"
#include "bt_mgr.h"
#include "dsp_shm.h"
#include "dsp_driver.h"
#include "dsp_ipc.h"
#include "audio_route.h"
#include "app_msg.h"
#include "sys_mgr.h"

void *hAudioPathMsgQueueHandle = NULL;
extern void *hEventQueueHandleAu;
/* TODO Remove End */

#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr)-(size_t)(&((type *)0)->member)))

#define LIST_ENTRY(ptr, type, member) \
    CONTAINER_OF(ptr, type, member)

#define LIST_FIRST_ENTRY(p_queue, type, member) \
    ((p_queue)->p_first ? \
     LIST_ENTRY((p_queue)->p_first, type, member) : NULL)

#define LIST_NEXT_ENTRY(pos, member) \
    (pos && (pos)->member.p_next ? \
     LIST_ENTRY((pos)->member.p_next, typeof(*(pos)), member) : NULL)

#define LIST_FOR_EACH_ENTRY(pos, p_queue, member) \
    for (pos = LIST_FIRST_ENTRY(p_queue, typeof(*(pos)), member); \
         pos != NULL; \
         pos = LIST_NEXT_ENTRY(pos, member))

#define LIST_FOR_EACH_ENTRY_SAFE(pos, n, p_queue, member) \
    for (pos = LIST_FIRST_ENTRY(p_queue, typeof(*(pos)), member), \
         n = LIST_NEXT_ENTRY(pos, member); \
         pos != NULL; \
         pos = n, n = LIST_NEXT_ENTRY(n, member))

#define BIT(_n)   (uint32_t)(1U << (_n))

#define AUDIO_PATH_EVENT_FLAG_PATH_STARTING                 (0x00000001)
#define AUDIO_PATH_EVENT_FLAG_ACTION_START_ACK              (0x00000002)
#define AUDIO_PATH_EVENT_FLAG_ACTION_STOP_ACK               (0x00000004)
#define AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_ENABLING            (0x00000008)
#define AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING           (0x00000010)
#define AUDIO_PATH_EVENT_FLAG_NOTIFICATION_FINISH           (0x00000020)
#define AUDIO_PATH_EVENT_FLAG_SIDETONE_ENABLING             (0x00000040)
#define AUDIO_PATH_EVENT_FLAG_SIDETONE_DISABLING            (0x00000080)

#define EVENT_FLAG_CODEC_ENABLE                 (0x00000001)
#define EVENT_FLAG_DSP_READY                    (0x00000002)
#define EVENT_FLAG_PLUGIN_ENABLE                (0x00000004)

#define  AUDIO_BIT    BIT(AUDIO_CATEGORY_AUDIO)
#define  VOICE_BIT    BIT(AUDIO_CATEGORY_VOICE)
#define  RECORD_BIT   BIT(AUDIO_CATEGORY_RECORD)
#define  ANALOG_BIT   BIT(AUDIO_CATEGORY_ANALOG)
#define  TONE_BIT     BIT(AUDIO_CATEGORY_TONE)
#define  VP_BIT       BIT(AUDIO_CATEGORY_VP)
#define  APT_BIT      BIT(AUDIO_CATEGORY_APT)
#define  LLAPT_BIT    BIT(AUDIO_CATEGORY_LLAPT)
#define  ANC_BIT      BIT(AUDIO_CATEGORY_ANC)
#define  VAD_BIT      BIT(AUDIO_CATEGORY_VAD)
#define  SIDETONE_BIT BIT(AUDIO_CATEGORY_SIDETONE)

#define TIMER_CODEC_POWER_DOWN  1000
#define TIMER_DSP_POWER_DOWN    2000

typedef enum t_audio_path_state
{
    AUDIO_PATH_STATE_IDLE,
    AUDIO_PATH_STATE_PENDING,
    AUDIO_PATH_STATE_READY,
    AUDIO_PATH_STATE_RUNNING,
    AUDIO_PATH_STATE_SUSPEND,
    AUDIO_PATH_STATE_STOPPING,
} T_AUDIO_PATH_STATE;

typedef enum t_audio_path_stream_state
{
    AUDIO_PATH_STREAM_STATE_IDLE,
    AUDIO_PATH_STREAM_STATE_FIRST,
    AUDIO_PATH_STREAM_STATE_CONTINUE,
    AUDIO_PATH_STREAM_STATE_END,
} T_AUDIO_PATH_STREAM_STATE;

typedef struct t_audio_path
{
    T_OS_QUEUE_ELEM             state_list;
    T_AUDIO_CATEGORY            category;
    T_AUDIO_PATH_STREAM_STATE   stream_state;
    T_AUDIO_PATH_STATE          state;
    T_AUDIO_STREAM_MODE         data_mode;
    P_AUDIO_PATH_CBACK          cback;
    T_CODEC_MGR_SESSION_HANDLE  codec_session;
    T_DSP_MGR_SESSION_HANDLE    dsp_session;
    T_SPORT_MGR_SESSION_HANDLE  sport_session;
    T_SYS_EVENT_GROUP_HANDLE    event_group;
} T_AUDIO_PATH;

typedef void (*P_AUDIO_PATH_STOP)(T_AUDIO_PATH *path);
typedef struct t_audio_path_db
{
    T_OS_QUEUE                  idle_list;
    T_OS_QUEUE                  pending_list;
    T_OS_QUEUE                  suspending_list;
    T_OS_QUEUE                  ready_list;
    T_OS_QUEUE                  running_list;
    T_OS_QUEUE                  stopping_list;

    T_SYS_EVENT_GROUP_HANDLE    event_group;
    T_SYS_IPC_HANDLE            dsp_event;
    T_SYS_IPC_HANDLE            path_event;

    bool                        wait_power_off;
    bool                        lps_en;

    T_SYS_TIMER_HANDLE          codec_timer;
    T_SYS_TIMER_HANDLE          dsp_timer;
} T_AUDIO_PATH_DB;

static T_AUDIO_PATH_DB *audio_path_db = NULL;

static const uint16_t mix_strategy_map[AUDIO_CATEGORY_NUMBER] =
{
    /* AUDIO */
    [AUDIO_CATEGORY_AUDIO] =
    (RECORD_BIT | APT_BIT | VP_BIT | TONE_BIT | LLAPT_BIT | ANC_BIT | VAD_BIT | SIDETONE_BIT),
    /* VOICE */
    [AUDIO_CATEGORY_VOICE] =
    (VP_BIT | TONE_BIT | LLAPT_BIT | ANC_BIT | SIDETONE_BIT),
    /* RECORD */
    [AUDIO_CATEGORY_RECORD] =
    (AUDIO_BIT | VP_BIT | TONE_BIT | LLAPT_BIT | ANC_BIT | SIDETONE_BIT),
    /* ANALOG */
    [AUDIO_CATEGORY_ANALOG] =
    (VP_BIT | TONE_BIT | LLAPT_BIT | ANC_BIT | SIDETONE_BIT),

#if (TARGET_RTL8753GFE == 1)
    /* TONE */
    [AUDIO_CATEGORY_TONE] =
    (RECORD_BIT | LLAPT_BIT | ANC_BIT | SIDETONE_BIT),
    /* VP */
    [AUDIO_CATEGORY_VP] =
    (RECORD_BIT | LLAPT_BIT | ANC_BIT | SIDETONE_BIT),
    /* APT */
    [AUDIO_CATEGORY_APT] =
    (VP_BIT | TONE_BIT | ANC_BIT | VAD_BIT | SIDETONE_BIT),
#else

#if (CONFIG_REALTEK_AM_AUDIO_STEREO_SUPPORT == 1)
    /* TONE */
    [AUDIO_CATEGORY_TONE] =
    (RECORD_BIT | LLAPT_BIT | ANC_BIT | SIDETONE_BIT),
    /* VP */
    [AUDIO_CATEGORY_VP] =
    (RECORD_BIT | LLAPT_BIT | ANC_BIT | SIDETONE_BIT),
    /* APT */
    [AUDIO_CATEGORY_APT] =
    (VP_BIT | TONE_BIT | ANC_BIT | VAD_BIT | SIDETONE_BIT),
#else
    /* TONE */
    [AUDIO_CATEGORY_TONE] =
    (AUDIO_BIT | VOICE_BIT | RECORD_BIT | APT_BIT | LLAPT_BIT | ANC_BIT | SIDETONE_BIT),
    /* VP */
    [AUDIO_CATEGORY_VP] =
    (AUDIO_BIT | VOICE_BIT | RECORD_BIT | APT_BIT | LLAPT_BIT | ANC_BIT | SIDETONE_BIT),
    /* APT */
    [AUDIO_CATEGORY_APT] =
    (VP_BIT | TONE_BIT | ANC_BIT | VAD_BIT | AUDIO_BIT | SIDETONE_BIT),
#endif

#endif


    /* LLAPT */
    [AUDIO_CATEGORY_LLAPT] =
    (AUDIO_BIT | VOICE_BIT | RECORD_BIT | ANALOG_BIT | VP_BIT | TONE_BIT | VAD_BIT | SIDETONE_BIT),
    /* ANC */
    [AUDIO_CATEGORY_ANC] =
    (AUDIO_BIT | VOICE_BIT | RECORD_BIT | ANALOG_BIT | APT_BIT | VP_BIT | TONE_BIT | VAD_BIT | SIDETONE_BIT),
    /* VAD */
    [AUDIO_CATEGORY_VAD] =
    (AUDIO_BIT | APT_BIT | VP_BIT | TONE_BIT | ANALOG_BIT | SIDETONE_BIT),
    /*SIDETONE*/
    [AUDIO_CATEGORY_SIDETONE] =
    (AUDIO_BIT |
     VOICE_BIT | RECORD_BIT | ANALOG_BIT | TONE_BIT | VP_BIT | APT_BIT | LLAPT_BIT | ANC_BIT | VAD_BIT),
};

static const uint16_t priority_strategy_map[AUDIO_CATEGORY_NUMBER] =
{
    /* AUDIO */
    [AUDIO_CATEGORY_AUDIO] =
    (VOICE_BIT | VP_BIT | TONE_BIT | ANC_BIT | LLAPT_BIT | SIDETONE_BIT),
    /* VOICE */
    [AUDIO_CATEGORY_VOICE] =
    (VP_BIT | TONE_BIT),
    /* RECORD */
    [AUDIO_CATEGORY_RECORD] =
    (VOICE_BIT),
    /* ANALOG */
    [AUDIO_CATEGORY_ANALOG] =
    (VOICE_BIT | VP_BIT | TONE_BIT | SIDETONE_BIT),
    /* TONE */
    [AUDIO_CATEGORY_TONE] = 0,
    /* VP */
    [AUDIO_CATEGORY_VP] = 0,
    /* APT */
    [AUDIO_CATEGORY_APT] =
    (AUDIO_BIT | VOICE_BIT | VP_BIT | TONE_BIT | SIDETONE_BIT),
    /* LLAPT */
    [AUDIO_CATEGORY_LLAPT] = 0,
    /* ANC */
    [AUDIO_CATEGORY_ANC] = 0,
    /* VAD */
    [AUDIO_CATEGORY_VAD] =
    (VOICE_BIT | RECORD_BIT | ANALOG_BIT | APT_BIT | VP_BIT | TONE_BIT | LLAPT_BIT | ANC_BIT),
    /*SIDETONE*/
    [AUDIO_CATEGORY_SIDETONE] = 0,
};

void audio_path_schedule(void);
static void audio_path_post_event(uint8_t state,
                                  T_AUDIO_CATEGORY category,
                                  T_AUDIO_PATH_EVENT event);
static void audio_path_audio_start(T_AUDIO_PATH *audio_path);
static void audio_path_anc_start(T_AUDIO_PATH *audio_path);
static void audio_path_notification_start(T_AUDIO_PATH *audio_path);
static void audio_path_voice_start(T_AUDIO_PATH *audio_path);
static T_AUDIO_PATH *audio_path_peek(uint8_t state);
static void audio_path_state_set(T_AUDIO_PATH *audio_path, T_AUDIO_PATH_STATE state);
static T_AUDIO_PATH *audio_path_schedule_switch(void);
static void audio_path_apt_suspend(T_AUDIO_PATH *apt_path);
static void audio_path_suspend(T_AUDIO_PATH *running_path, T_AUDIO_PATH *audio_path);
static void audio_path_record_start(T_AUDIO_PATH *audio_path);
static void audio_path_llapt_start(T_AUDIO_PATH *audio_path);
static void audio_path_sidetone_start(T_AUDIO_PATH *audio_path);
#if 0
static void audio_path_vad_suspend(T_AUDIO_PATH *audio_path);
#endif
static bool audio_path_ipc_event_handler(T_DSP_IPC_EVENT event, uint32_t param);
static void audio_path_normal_apt_start(T_AUDIO_PATH *audio_path);

static void audio_path_audio_stop(T_AUDIO_PATH *audio_path);
static void audio_path_voice_stop(T_AUDIO_PATH *audio_path);
static void audio_path_record_stop(T_AUDIO_PATH *audio_path);
static void audio_path_analog_stop(T_AUDIO_PATH *audio_path);
static void audio_path_tone_stop(T_AUDIO_PATH *path);
static void audio_path_vp_stop(T_AUDIO_PATH *path);
static void audio_path_normal_apt_stop(T_AUDIO_PATH *audio_path);
static void audio_path_llapt_stop(T_AUDIO_PATH *audio_path);
static void audio_path_anc_stop(T_AUDIO_PATH *audio_path);
static void audio_path_vad_stop(T_AUDIO_PATH *audio_path);
static void audio_path_sidetone_stop(T_AUDIO_PATH *audio_path);

static const P_AUDIO_PATH_STOP path_stop_cb[] =
{
    [AUDIO_CATEGORY_AUDIO]  = audio_path_audio_stop,
    [AUDIO_CATEGORY_VOICE]  = audio_path_voice_stop,
    [AUDIO_CATEGORY_RECORD] = audio_path_record_stop,
    [AUDIO_CATEGORY_ANALOG] = audio_path_analog_stop,
    [AUDIO_CATEGORY_TONE]   = audio_path_tone_stop,
    [AUDIO_CATEGORY_VP]     = audio_path_vp_stop,
    [AUDIO_CATEGORY_APT]    = audio_path_normal_apt_stop,
    [AUDIO_CATEGORY_LLAPT]  = audio_path_llapt_stop,
    [AUDIO_CATEGORY_ANC]    = audio_path_anc_stop,
    [AUDIO_CATEGORY_VAD]    = audio_path_vad_stop,
    [AUDIO_CATEGORY_SIDETONE] = audio_path_sidetone_stop,
};

static void path_wait_codec_enable_handler(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;

    dsp_mgr_session_enable(path->dsp_session);
}

static bool audio_path_wait_codec_enabled(T_AUDIO_PATH *path)
{
    if (! sys_event_flag_check(path->event_group, EVENT_FLAG_CODEC_ENABLE))
    {
        sys_event_flag_wait(path->event_group,
                            path_wait_codec_enable_handler,
                            (void *)path,
                            EVENT_FLAG_CODEC_ENABLE,
                            SYS_EVENT_FLAG_TYPE_SET_AND);

        return false;
    }

    return true;
}

static void apt_wait_action_start_ack_cback(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        path->stream_state = AUDIO_PATH_STREAM_STATE_CONTINUE;
        audio_path_state_set(path, AUDIO_PATH_STATE_RUNNING);
    }
    audio_path_schedule();
}

static void apt_wait_running_handler(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        dsp_mgr_decoder_action(path->category, DSP_IPC_ACTION_START);
        dsp_mgr_encoder_action(path->category, DSP_IPC_ACTION_START);
        dsp_mgr_apt_action(DSP_IPC_ACTION_START);

        sys_event_flag_post(audio_path_db->event_group,
                            SYS_EVENT_FLAG_OPT_SET,
                            AUDIO_PATH_EVENT_FLAG_ACTION_START_ACK);

        sys_event_flag_wait(audio_path_db->event_group,
                            apt_wait_action_start_ack_cback,
                            (void *)path,
                            AUDIO_PATH_EVENT_FLAG_ACTION_START_ACK,
                            SYS_EVENT_FLAG_TYPE_CLEAR_AND);
    }
}

static void apt_wait_action_stop_ack_cback(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        dsp_mgr_encoder_action(path->category, DSP_IPC_ACTION_STOP);
        dsp_mgr_decoder_action(path->category, DSP_IPC_ACTION_STOP);
        dsp_mgr_session_disable(path->dsp_session);
    }
}

static void audio_path_start_fade_out(T_AUDIO_PATH *path)
{
    bool stream_continue;

    stream_continue = (path->stream_state == AUDIO_PATH_STREAM_STATE_CONTINUE) ? true : false;

    dsp_mgr_fade_out_start(stream_continue, path->dsp_session);
}

void audio_path_dac_gain_set(T_AUDIO_CATEGORY category,
                             int16_t          left_gain,
                             int16_t          right_gain)
{
    if (category == AUDIO_CATEGORY_APT)
    {
        dsp_ipc_set_apt_dac_gain(left_gain, right_gain);
    }
    else if (category == AUDIO_CATEGORY_LLAPT)
    {
        anc_mgr_gain_set(left_gain, right_gain);
    }
    else if ((category == AUDIO_CATEGORY_TONE) ||
             (category == AUDIO_CATEGORY_VP))
    {
        dsp_ipc_set_tone_gain(left_gain, right_gain);
    }
    else
    {
        dsp_ipc_set_dac_gain(left_gain, right_gain);
    }
}

static void audio_path_adc_gain_set(T_AUDIO_CATEGORY category,
                                    int16_t          left_gain,
                                    int16_t          right_gain)
{
    switch (category)
    {
    case AUDIO_CATEGORY_VOICE:
        dsp_ipc_set_voice_adc_gain(left_gain, right_gain);
        break;

    case AUDIO_CATEGORY_RECORD:
        dsp_ipc_set_record_adc_gain(left_gain, right_gain);
        break;

    case AUDIO_CATEGORY_ANALOG:
        dsp_ipc_set_aux_adc_gain(left_gain, right_gain);
        break;

    default:
        break;
    }
}

static bool audio_path_mgr_idle(void)
{
    if ((audio_path_db->pending_list.count == 0) &&
        (audio_path_db->ready_list.count == 0) &&
        (audio_path_db->running_list.count == 0) &&
        (audio_path_db->stopping_list.count == 0) &&
        (audio_path_db->suspending_list.count == 0))
    {
        return true;
    }

    return false;
}

static void audio_path_codec_event_handler(uint32_t param)
{
    T_DSP_SCHED_MSG *dsp_msg;
    T_CODEC_MGR_EVENT_PARAM *event_param;
    T_AUDIO_PATH *session_path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    dsp_msg = (T_DSP_SCHED_MSG *)param;
    event_param = (T_CODEC_MGR_EVENT_PARAM *)(dsp_msg->p_data);
    session_path = (T_AUDIO_PATH *)(event_param->context);

    AUDIO_PRINT_TRACE2("audio_path_codec_event_handler: codec state %d, path %p",
                       event_param->state, session_path);

    if (session_path != NULL)
    {
        if (session_path->category == AUDIO_CATEGORY_ANC ||
            session_path->category == AUDIO_CATEGORY_LLAPT)
        {
            plugin_param.dac_sample_rate = 48000; /* fixed in 48K */
            plugin_param.adc_sample_rate = 48000; /* fixed in 48K */
        }
        else
        {
            plugin_param.dac_sample_rate = sport_mgr_tx_sample_rate_get(session_path->sport_session);
            plugin_param.adc_sample_rate = sport_mgr_rx_sample_rate_get(session_path->sport_session);
        }

        if (event_param->state == CODEC_MGR_SESSION_STATE_ENABLED)
        {
            sys_event_flag_post(session_path->event_group,
                                SYS_EVENT_FLAG_OPT_SET,
                                EVENT_FLAG_CODEC_ENABLE);
            if (session_path->category == AUDIO_CATEGORY_LLAPT ||
                session_path->category == AUDIO_CATEGORY_ANC ||
                session_path->category == AUDIO_CATEGORY_SIDETONE)
            {
                /* post the AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON after codec on */
                audio_plugin_occasion_occur((T_AUDIO_PATH_HANDLE)session_path,
                                            session_path->category,
                                            AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON,
                                            plugin_param);
            }

        }
        else if (event_param->state == CODEC_MGR_SESSION_STATE_DISABLED)
        {
            sys_event_flag_post(session_path->event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                EVENT_FLAG_CODEC_ENABLE);
            audio_plugin_occasion_occur((T_AUDIO_PATH_HANDLE)session_path,
                                        session_path->category,
                                        AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_OFF,
                                        plugin_param);
        }
    }
    else
    {
        if (event_param->state == CODEC_MGR_STATE_POWER_OFF)
        {
            if (audio_path_mgr_idle() == true)
            {
                if (audio_path_db->wait_power_off == true)
                {
                    dsp_mgr_power_off();
                }
                else
                {
                    if (audio_path_db->lps_en == true)
                    {
                        //set timer to power down DSP
                        sys_timer_start(audio_path_db->dsp_timer);
                    }
                }
            }
        }
    }
}

static T_OS_QUEUE *audio_path_get_queue(uint8_t state)
{
    T_OS_QUEUE *queue;

    queue = NULL;

    switch (state)
    {
    case AUDIO_PATH_STATE_IDLE:
        {
            queue = &audio_path_db->idle_list;
        }
        break;

    case AUDIO_PATH_STATE_PENDING:
        {
            queue = &audio_path_db->pending_list;
        }
        break;

    case AUDIO_PATH_STATE_READY:
        {
            queue = &audio_path_db->ready_list;
        }
        break;

    case AUDIO_PATH_STATE_RUNNING:
        {
            queue = &audio_path_db->running_list;
        }
        break;

    case AUDIO_PATH_STATE_SUSPEND:
        {
            queue = &audio_path_db->suspending_list;
        }
        break;

    case AUDIO_PATH_STATE_STOPPING:
        {
            queue = &audio_path_db->stopping_list;
        }
        break;

    default:
        break;
    }

    return queue;
}

static T_AUDIO_PATH *audio_path_peek(uint8_t state)
{
    T_OS_QUEUE *queue;

    queue = audio_path_get_queue(state);

    if (queue != NULL)
    {
        return LIST_FIRST_ENTRY(queue, struct t_audio_path, state_list);
    }

    return NULL;
}

/*
TODO:
    Move to dsp-mgr component.    dsp-mgr should know the paths running on DSP FW
 */
static T_AUDIO_PATH *dsp_mgr_peek_running_path(void)
{
    T_AUDIO_PATH *running_path;
    T_AUDIO_PATH *next_path;

    running_path = audio_path_peek(AUDIO_PATH_STATE_RUNNING);

    while (running_path != NULL)
    {
        next_path = LIST_NEXT_ENTRY(running_path, state_list);

        if (running_path->category != AUDIO_CATEGORY_ANC)
        {
            break;
        }

        running_path = next_path;
    }

    return running_path;
}

static T_AUDIO_PATH *audio_path_find_category(uint8_t state, T_AUDIO_CATEGORY category)
{
    T_AUDIO_PATH *audio_path;

    audio_path = audio_path_peek(state);
    while (audio_path != NULL)
    {
        if (audio_path->category == category)
        {
            return audio_path;
        }

        audio_path = LIST_NEXT_ENTRY(audio_path, state_list);
    }
    return NULL;
}

static bool audio_path_state_exit(T_AUDIO_PATH *path, T_AUDIO_PATH_STATE state)
{
    T_OS_QUEUE *queue;

    if (state == AUDIO_PATH_STATE_RUNNING)
    {
        path->stream_state = AUDIO_PATH_STREAM_STATE_IDLE;
    }

    queue = audio_path_get_queue(state);

    return os_queue_delete(queue, path);
}

static void audio_path_state_set(T_AUDIO_PATH *audio_path, T_AUDIO_PATH_STATE state)
{
    T_OS_QUEUE *queue;
    T_AUDIO_PATH_EVENT event = AUDIO_PATH_EVT_NONE;

    AUDIO_PRINT_INFO4("audio_path_state_set: path %p, category 0x%02x, old state 0x%02x, new state 0x%02x",
                      audio_path, audio_path->category, audio_path->state, state);

    if (audio_path_state_exit(audio_path, audio_path->state) == false)
    {
        return;
    }

    audio_path->state = state;

    switch (state)
    {
    case AUDIO_PATH_STATE_IDLE:
        {
            event = AUDIO_PATH_EVT_IDLE;
        }
        break;

    case AUDIO_PATH_STATE_READY:
        {
            // Once an audio path enters the ready state, set PATH_STARTING flag
            sys_event_flag_post(audio_path_db->event_group,
                                SYS_EVENT_FLAG_OPT_SET,
                                AUDIO_PATH_EVENT_FLAG_PATH_STARTING);
        }
        break;

    case AUDIO_PATH_STATE_RUNNING:
        {
            event = AUDIO_PATH_EVT_RUNNING;
        }
        break;

    case AUDIO_PATH_STATE_SUSPEND:
        {
            event = AUDIO_PATH_EVT_SUSPEND;
        }
        break;

    default:
        break;
    }

    queue = audio_path_get_queue(audio_path->state);
    os_queue_in(queue, &audio_path->state_list);

    if (event != AUDIO_PATH_EVT_NONE)
    {
        audio_path->cback(audio_path, event, 0);
    }
    if (event == AUDIO_PATH_EVT_RUNNING)
    {
        // When all paths are in running state, clear the PATH_STARTING flag
        if (audio_path_db->ready_list.count == 0)
        {
            sys_event_flag_post(audio_path_db->event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                AUDIO_PATH_EVENT_FLAG_NOTIFICATION_FINISH);
            sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_CLEAR,
                                AUDIO_PATH_EVENT_FLAG_PATH_STARTING);
        }
    }
}

static bool audio_path_ipc_event_handler(T_DSP_IPC_EVENT event, uint32_t param)
{
    int8_t ret;
    ret = 0;

    AUDIO_PRINT_TRACE2("audio_path_ipc_event_handler: event 0x%02X, param 0x%04X", event, param);

    switch (event)
    {
    case DSP_IPC_EVT_VP_REQUEST_DATA:
        {
            audio_path_post_event(AUDIO_PATH_STATE_RUNNING, AUDIO_CATEGORY_VP,
                                  AUDIO_PATH_EVT_REQ_DATA);
        }
        break;

    case DSP_IPC_EVT_B2BMSG:
        {
            sys_ipc_publish("path_mgr", AUDIO_PATH_EVT_DSP_INTER_MSG, (void *)(param));
        }
        break;
#if 0
    case DSP_IPC_EVT_KEYWORD:
        {
            T_AUDIO_PATH *audio_path;

            audio_path = audio_path_find_category(AUDIO_PATH_STATE_RUNNING, AUDIO_CATEGORY_VAD);
            if (audio_path != NULL)
            {
                audio_path->cback(audio_path, AUDIO_PATH_EVT_DATA_IND, 0);
            }
        }
        break;
#endif

    case DSP_IPC_EVT_APT_ACTION_ACK:
        {
            uint8_t result;

            result = (uint8_t)param;

            AUDIO_PRINT_TRACE1("audio_path_ipc_event_handler: DSP_IPC_EVT_APT_ACTION_ACK, "
                               "result %d", result);

            if (sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_ACTION_STOP_ACK))
            {
                sys_event_flag_post(audio_path_db->event_group,
                                    SYS_EVENT_FLAG_OPT_CLEAR,
                                    AUDIO_PATH_EVENT_FLAG_ACTION_STOP_ACK);
            }
            else if (sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_ACTION_START_ACK))
            {
                sys_event_flag_post(audio_path_db->event_group,
                                    SYS_EVENT_FLAG_OPT_CLEAR,
                                    AUDIO_PATH_EVENT_FLAG_ACTION_START_ACK);
            }
        }
        break;

    default:
        break;
    }


    if (ret == 0)
    {
        return true;
    }
    else
    {
        AUDIO_PRINT_ERROR3("audio_path_ipc_event_handler: ret %d, event %02x, param %02x",
                           ret, event, param);
        return false;
    }
}

static void audio_path_post_event(uint8_t state,
                                  T_AUDIO_CATEGORY category,
                                  T_AUDIO_PATH_EVENT event)
{
    T_AUDIO_PATH *audio_path;

    audio_path = audio_path_find_category(state, category);
    if (audio_path != NULL)
    {
        audio_path->cback(audio_path, event, 0);
    }
}

static bool audio_path_mix_strategy(T_AUDIO_PATH *origin, T_AUDIO_PATH *mix)
{
    bool is_mix;

    is_mix = ((mix_strategy_map[origin->category] & BIT(mix->category)) != 0) ? true : false;

#if (TARGET_RTL8753GFE == 1)
    if (mix->category == AUDIO_CATEGORY_VP ||
        mix->category == AUDIO_CATEGORY_TONE)
    {
        if ((origin->category == AUDIO_CATEGORY_VOICE) || (origin->category == AUDIO_CATEGORY_AUDIO))
        {
            if (origin->stream_state == AUDIO_PATH_STREAM_STATE_IDLE)
            {
                is_mix = false;
            }
        }
    }
#else

#if (CONFIG_REALTEK_AM_AUDIO_STEREO_SUPPORT == 1)
    if (mix->category == AUDIO_CATEGORY_TONE)
    {
        if (origin->category == AUDIO_CATEGORY_AUDIO)
        {
            if (origin->stream_state == AUDIO_PATH_STREAM_STATE_IDLE)
            {
                is_mix = false;
            }
        }
    }
#endif /* CONFIG_REALTEK_AM_AUDIO_STEREO_SUPPORT */

#endif

    return is_mix;
}

static bool audio_path_is_mixable(T_AUDIO_PATH *mix)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PATH *next;

    LIST_FOR_EACH_ENTRY_SAFE(path, next, &audio_path_db->running_list, state_list)
    {
        if (audio_path_mix_strategy(path, mix) == false)
        {
            return false;
        }
    }

    return true;
}

static T_AUDIO_PATH *audio_path_schedule_priority(T_AUDIO_PATH *first_path,
                                                  T_AUDIO_PATH *second_path)
{
    T_AUDIO_PATH *path;

    path = ((priority_strategy_map[first_path->category] & BIT(second_path->category)) != 0) ?
           second_path : first_path;

    return path;
}

static T_AUDIO_PATH *audio_path_schedule_switch(void)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PATH *select_path;

    path =  audio_path_peek(AUDIO_PATH_STATE_PENDING);
    select_path = path;
    while (path != NULL)
    {
        select_path = audio_path_schedule_priority(select_path, path);

        path = LIST_NEXT_ENTRY(path, state_list);
    }

    return select_path;
}

static void audio_path_plugin_msg_handle(T_AUDIO_PATH *path, T_AUDIO_PLUGIN_OCCASION occasion)
{
    AUDIO_PRINT_INFO2("audio_path_plugin_msg_handle: path 0x%08x, occasion %d", path, occasion);

    if (path != NULL)
    {
        switch (occasion)
        {
        case AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON:
            {
                if (path->category == AUDIO_CATEGORY_LLAPT ||
                    path->category == AUDIO_CATEGORY_ANC ||
                    path->category == AUDIO_CATEGORY_SIDETONE)
                {
                    T_AUDIO_PLUGIN_PARAM param;

                    param.dac_sample_rate = 48000; /* fixed in 48K */
                    param.adc_sample_rate = 48000; /* fixed in 48K */
                    /* post a dummy AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON to make path running */
                    audio_plugin_occasion_occur(path,
                                                path->category,
                                                AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON,
                                                param);
                }
                else
                {
                    dsp_mgr_session_run(path->dsp_session);
                }
            }
            break;

        case AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_OFF:
            {
                if (path->state == AUDIO_PATH_STATE_STOPPING)
                {
                    audio_path_state_set(path, AUDIO_PATH_STATE_IDLE);
                }
                else if (path->state == AUDIO_PATH_STATE_SUSPEND)
                {
                    audio_path_state_set(path, AUDIO_PATH_STATE_PENDING);
                }

                audio_path_schedule();

                if (audio_path_mgr_idle() == true)
                {
                    if (audio_path_db->wait_power_off == true)
                    {
                        codec_mgr_power_off();
                    }
                    else
                    {
                        if (audio_path_db->lps_en == true)
                        {
                            /*set timer to power down CODEC*/
                            sys_timer_start(audio_path_db->codec_timer);
                        }
                    }

                }
            }
            break;

        case AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON:
            {
                sys_event_flag_post(path->event_group,
                                    SYS_EVENT_FLAG_OPT_SET,
                                    EVENT_FLAG_PLUGIN_ENABLE);
            }
            break;

        case AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF:
            {
                sys_event_flag_post(path->event_group,
                                    SYS_EVENT_FLAG_OPT_CLEAR,
                                    EVENT_FLAG_PLUGIN_ENABLE);

                if ((path->state == AUDIO_PATH_STATE_SUSPEND) ||
                    (path->state == AUDIO_PATH_STATE_STOPPING))
                {
                    if (path->category == AUDIO_CATEGORY_LLAPT ||
                        path->category == AUDIO_CATEGORY_ANC ||
                        path->category == AUDIO_CATEGORY_SIDETONE)
                    {
                        codec_mgr_session_disable(path->codec_session);
                    }
                    else
                    {
                        codec_mgr_session_disable(path->codec_session);
                        sport_mgr_disable(path->sport_session);
                    }
                }
            }
            break;

        default:
            break;
        }
    }
}

void audio_path_msg_handler(void)
{
    T_AUDIO_PATH_MSG msg;

    if (os_msg_recv(hAudioPathMsgQueueHandle, &msg, 0) == true)
    {
        switch (msg.type)
        {
        case AUDIO_PATH_MSG_TYPE_PLUGIN:
            {
                audio_path_plugin_msg_handle((T_AUDIO_PATH *)msg.path,
                                             (T_AUDIO_PLUGIN_OCCASION)msg.data.plugin_msg.occasion);
            }
            break;

        default:
            break;
        }
    }
}

static void analog_wait_running_handler(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        path->stream_state = AUDIO_PATH_STREAM_STATE_CONTINUE;

        dsp_mgr_line_in_action(DSP_IPC_ACTION_START);

        audio_path_state_set(path, AUDIO_PATH_STATE_RUNNING);
    }

    audio_path_schedule();
}

static void audio_path_analog_start(T_AUDIO_PATH *audio_path)
{
    uint32_t dev_map;

    dev_map = EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_DSP_READY | EVENT_FLAG_PLUGIN_ENABLE;

    if (audio_path_is_mixable(audio_path))
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_READY);

        codec_mgr_session_enable(audio_path->codec_session);
        dsp_mgr_session_enable(audio_path->dsp_session);

        sys_event_flag_wait(audio_path->event_group,
                            analog_wait_running_handler,
                            (void *)audio_path,
                            dev_map,
                            SYS_EVENT_FLAG_TYPE_SET_AND);
    }
}

static void record_wait_running_handler(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        audio_path_state_set(path, AUDIO_PATH_STATE_RUNNING);
    }

    audio_path_schedule();
}

static void audio_path_record_start(T_AUDIO_PATH *audio_path)
{
    if (audio_path_is_mixable(audio_path))
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_READY);

        codec_mgr_session_enable(audio_path->codec_session);

        dsp_mgr_session_enable(audio_path->dsp_session);

        sys_event_flag_wait(audio_path->event_group,
                            record_wait_running_handler,
                            (void *)audio_path,
                            EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_DSP_READY | EVENT_FLAG_PLUGIN_ENABLE,
                            SYS_EVENT_FLAG_TYPE_SET_AND);

    }
}

bool audio_path_is_stable(T_DSP_MGR_SESSION_HANDLE handle)
{
    bool dsp_stable;
    bool is_ack;

    dsp_stable = dsp_mgr_is_stable(handle);
    is_ack = sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_ACTION_STOP_ACK);

    if ((dsp_stable == false) || is_ack)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void audio_path_schedule(void)
{
    T_AUDIO_PATH *audio_path;

    audio_path = audio_path_schedule_switch();

    if (audio_path != NULL)
    {
        if (audio_path_db->ready_list.count != 0 ||
            audio_path_db->stopping_list.count != 0 ||
            audio_path_db->suspending_list.count != 0)
        {
            return;
        }

        if (!audio_path_is_stable(audio_path->dsp_session))
        {
            return;
        }

        if (dsp_mgr_power_on_check() == false)
        {
            dsp_mgr_power_on();
        }

        sys_timer_stop(audio_path_db->codec_timer);
        sys_timer_stop(audio_path_db->dsp_timer);

        switch (audio_path->category)
        {
        case AUDIO_CATEGORY_VOICE:
            {
                audio_path_voice_start(audio_path);
            }
            break;

        case AUDIO_CATEGORY_AUDIO:
            {
                audio_path_audio_start(audio_path);
            }
            break;

        case AUDIO_CATEGORY_RECORD:
            {
                audio_path_record_start(audio_path);
            }
            break;

        case AUDIO_CATEGORY_TONE:
        case AUDIO_CATEGORY_VP:
            {
                audio_path_notification_start(audio_path);
            }
            break;
#if 0
        case AUDIO_CATEGORY_VAD:
            {
                audio_path_vad_start(audio_path);
            }
            break;
#endif
        case AUDIO_CATEGORY_ANALOG:
            {
                audio_path_analog_start(audio_path);
            }
            break;

        case AUDIO_CATEGORY_LLAPT:
            {
                audio_path_llapt_start(audio_path);
            }
            break;

        case AUDIO_CATEGORY_APT:
            {
                audio_path_normal_apt_start(audio_path);
            }
            break;

        case AUDIO_CATEGORY_ANC:
            {
                audio_path_anc_start(audio_path);
            }
            break;

        case AUDIO_CATEGORY_SIDETONE:
            {
                audio_path_sidetone_start(audio_path);
            }
            break;

        default:
            break;
        }
    }
}

static bool audio_path_dsp_cback(uint32_t event, void *msg)
{
    T_AUDIO_PATH *ready_path;
    T_AUDIO_PATH *next_path;

    AUDIO_PRINT_TRACE2("audio_path_dsp_cback: event 0x%02x param %p", event, msg);

    switch (event)
    {
    case DSP_MGR_EVT_EFFECT_REQ:
        {
            LIST_FOR_EACH_ENTRY_SAFE(ready_path, next_path, &audio_path_db->ready_list, state_list)
            {
                ready_path->cback(ready_path, AUDIO_PATH_EVT_EFFECT_REQ, 0);
            }
        }
        break;

    case DSP_MGR_EVT_INIT_FINISH:
        {
            audio_path_schedule();

            if (audio_path_mgr_idle() == true)
            {
                if (audio_path_db->wait_power_off == true)
                {
                    dsp_mgr_power_off();
                }
                else
                {
                    if (audio_path_db->lps_en == true)
                    {
                        //set timer to power down DSP
                        sys_timer_start(audio_path_db->dsp_timer);
                    }
                }
            }
        }
        break;

    case DSP_MGR_EVT_DSP_LOAD_FINISH:
        {
            /* FIXME decoupling */
            extern void dsp_cfg_comm_setting(uint32_t scenario);
            extern void dsp_mgr_load_finish(void);
            dsp_cfg_comm_setting((uint32_t)msg);
            dsp_mgr_load_finish();

            audio_path_schedule();
        }
        break;

    case DSP_MGR_EVT_SPORT_STOP_FAKE:
        {
            audio_path_schedule();
        }
        break;

    case DSP_MGR_EVT_CODEC_STATE:
        {
            audio_path_codec_event_handler((uint32_t)msg);
        }
        break;

    case DSP_MGR_EVT_POWER_OFF:
        {
            audio_path_db->wait_power_off = false;
            /* TODO: remove */
            audio_path_schedule();
        }
        break;

    default:
        break;

    }

    return true;
}

static bool audio_path_anc_cback(T_ANC_MGR_EVENT event)
{
    switch (event)
    {
    case ANC_MGR_EVENT_ENABLED:
        {
            if (sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_ENABLING))
            {
                sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_CLEAR,
                                    AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_ENABLING);
            }
        }
        break;

    case ANC_MGR_EVENT_DISABLED:
        {
            if (sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING))
            {
                sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_CLEAR,
                                    AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING);
            }
        }
        break;

    default:
        break;
    }

    return true;
}

static bool audio_path_codec_cback(T_CODEC_MGR_EVENT event)
{
    switch (event)
    {
    case CODEC_MGR_EVENT_SIDETONE_ENABLED:
        {
            if (sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_SIDETONE_ENABLING))
            {
                sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_CLEAR,
                                    AUDIO_PATH_EVENT_FLAG_SIDETONE_ENABLING);
            }
        }
        break;

    case CODEC_MGR_EVENT_SIDETONE_DISABLED:
        {
            if (sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_SIDETONE_DISABLING))
            {
                sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_CLEAR,
                                    AUDIO_PATH_EVENT_FLAG_SIDETONE_DISABLING);
            }
        }
        break;

    default:
        break;
    }

    return true;
}

static void audio_mgr_codec_timer_cback(T_SYS_TIMER_HANDLE handle)
{
    sys_timer_stop(audio_path_db->codec_timer);
    if (audio_path_mgr_idle() && (dsp_mgr_dsp2_ref_get() == 0))
    {
        codec_mgr_power_off();
    }
}

static void audio_mgr_dsp_timer_cback(T_SYS_TIMER_HANDLE handle)
{
    sys_timer_stop(audio_path_db->dsp_timer);
    if (audio_path_mgr_idle() && (dsp_mgr_dsp2_ref_get() == 0))
    {
        dsp_mgr_power_off();
    }
}

bool audio_path_init(void)
{
    int32_t ret = 0;

    audio_path_db = os_mem_zalloc2(sizeof(T_AUDIO_PATH_DB));
    if (audio_path_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    if (audio_route_init() == false)
    {
        ret = 2;
        goto fail_init_audio_route;
    }

    if (bin_loader_init() == false)
    {
        ret = 3;
        goto fail_init_bin_loader;
    }

    if (dsp_mgr_init() == false)
    {
        ret = 4;
        goto fail_init_dsp_mgr;
    }

    if (sport_mgr_init() == false)
    {
        ret = 5;
        goto fail_init_sport_mgr;
    }

    // move to audio_path_dsp_cback later
    dsp_ipc_cback_register(audio_path_ipc_event_handler);

    if (codec_mgr_init(audio_path_codec_cback) == false)
    {
        ret = 6;
        goto fail_init_codec_mgr;
    }

    if (anc_mgr_init(audio_path_anc_cback) == false)
    {
        ret = 7;
        goto fail_init_anc_mgr;
    }

    audio_path_db->dsp_event = dsp_mgr_register_cback(audio_path_dsp_cback);

    os_queue_init(&audio_path_db->idle_list);
    os_queue_init(&audio_path_db->pending_list);
    os_queue_init(&audio_path_db->suspending_list);
    os_queue_init(&audio_path_db->ready_list);
    os_queue_init(&audio_path_db->running_list);
    os_queue_init(&audio_path_db->stopping_list);

    audio_path_db->lps_en = true;
    audio_path_db->event_group = sys_event_group_create(0);

    audio_path_db->codec_timer = sys_timer_create("codec_stop",
                                                  SYS_TIMER_TYPE_LOW_PRECISION,
                                                  0,
                                                  TIMER_CODEC_POWER_DOWN * 1000,
                                                  false,
                                                  audio_mgr_codec_timer_cback);
    if (audio_path_db->codec_timer == NULL)
    {
        ret = 8;
        goto fail_create_codec_timer;
    }

    audio_path_db->dsp_timer = sys_timer_create("dsp_stop",
                                                SYS_TIMER_TYPE_LOW_PRECISION,
                                                0,
                                                TIMER_DSP_POWER_DOWN * 1000,
                                                false,
                                                audio_mgr_dsp_timer_cback);
    if (audio_path_db->dsp_timer == NULL)
    {
        ret = 9;
        goto fail_create_dsp_timer;
    }

    if (audio_plugin_init() == false)
    {
        ret = 10;
        goto fail_init_audio_plugin;
    }

    if (os_msg_queue_create(&hAudioPathMsgQueueHandle,
                            "pathQ",
                            8,
                            sizeof(T_AUDIO_PATH_MSG)) == false)
    {
        ret = 11;
        goto fail_create_msg_queue;
    }

    sys_mgr_event_register(EVENT_AUDIO_PATH_MSG, audio_path_msg_handler);

    return true;

fail_create_msg_queue:
    audio_plugin_deinit();
fail_init_audio_plugin:
    sys_timer_delete(audio_path_db->dsp_timer);
fail_create_dsp_timer:
    sys_timer_delete(audio_path_db->codec_timer);
fail_create_codec_timer:
    dsp_mgr_unregister_cback(audio_path_db->dsp_event);
    anc_mgr_deinit();
fail_init_anc_mgr:
    codec_mgr_deinit();
fail_init_codec_mgr:
    sport_mgr_deinit();
fail_init_sport_mgr:
    dsp_mgr_deinit();
fail_init_dsp_mgr:
    bin_loader_deinit();
fail_init_bin_loader:
    audio_route_deinit();
fail_init_audio_route:
    os_mem_free(audio_path_db);
    audio_path_db = NULL;
fail_alloc_db:
    AUDIO_PRINT_ERROR1("audio_path_init: failed %d", -ret);
    return false;
}

void audio_path_deinit(void)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PATH *next;

    if (audio_path_db == NULL)
    {
        return;
    }

    LIST_FOR_EACH_ENTRY_SAFE(path, next, &audio_path_db->idle_list, state_list)
    {
        os_mem_free(path);
    }
    os_queue_init(&audio_path_db->idle_list);

    LIST_FOR_EACH_ENTRY_SAFE(path, next, &audio_path_db->pending_list, state_list)
    {
        os_mem_free(path);
    }
    os_queue_init(&audio_path_db->pending_list);

    LIST_FOR_EACH_ENTRY_SAFE(path, next, &audio_path_db->suspending_list, state_list)
    {
        os_mem_free(path);
    }
    os_queue_init(&audio_path_db->suspending_list);

    LIST_FOR_EACH_ENTRY_SAFE(path, next, &audio_path_db->ready_list, state_list)
    {
        os_mem_free(path);
    }
    os_queue_init(&audio_path_db->ready_list);

    LIST_FOR_EACH_ENTRY_SAFE(path, next, &audio_path_db->running_list, state_list)
    {
        os_mem_free(path);
    }
    os_queue_init(&audio_path_db->running_list);

    LIST_FOR_EACH_ENTRY_SAFE(path, next, &audio_path_db->stopping_list, state_list)
    {
        os_mem_free(path);
    }
    os_queue_init(&audio_path_db->stopping_list);

    sys_event_group_delete(audio_path_db->event_group);

    dsp_mgr_unregister_cback(audio_path_db->dsp_event);

    codec_mgr_deinit();
    audio_route_deinit();
    dsp_mgr_deinit();
    anc_mgr_deinit();
    audio_plugin_deinit();

    sys_timer_delete(audio_path_db->codec_timer);
    sys_timer_delete(audio_path_db->dsp_timer);

    os_msg_queue_delete(hAudioPathMsgQueueHandle);

    os_mem_free(audio_path_db);
    audio_path_db = NULL;
}

void voice_dsp_mgr_session_cb(void *handle, T_DSP_MGR_EVENT event, uint32_t param)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    path = (T_AUDIO_PATH *)handle;

    if (path == NULL)
    {
        return;
    }

    plugin_param.dac_sample_rate = sport_mgr_tx_sample_rate_get(path->sport_session);
    plugin_param.adc_sample_rate = sport_mgr_rx_sample_rate_get(path->sport_session);
    switch (event)
    {
    case DSP_MGR_EVT_FADE_OUT_FINISH:
        {
            dsp_mgr_encoder_action(path->category, DSP_IPC_ACTION_STOP);
            dsp_mgr_decoder_action(path->category, DSP_IPC_ACTION_STOP);

            dsp_mgr_session_disable(path->dsp_session);
        }
        break;

    case DSP_MGR_EVT_FW_STOP:
    case DSP_MGR_EVT_SPORT_STOP_FAKE:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_PREPARE_READY:
        {
            if (audio_path_wait_codec_enabled(path))
            {
                sport_mgr_enable(path->sport_session);
                audio_plugin_occasion_occur(path,
                                            path->category,
                                            AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON,
                                            plugin_param);
            }
        }
        break;

    case DSP_MGR_EVT_FW_READY:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_SET,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_DSP_UNSYNC:
        {
            path->cback(path, AUDIO_PATH_EXC_DSP_UNSYNC, 0);
        }
        break;

    case DSP_MGR_EVT_DSP_SYNC_UNLOCK:
        {
            path->cback(path, AUDIO_PATH_EVT_DSP_SYNC_UNLOCK, 0);
        }
        break;

    case DSP_MGR_EVT_REQ_DATA:
        {
            if (path->state == AUDIO_PATH_STATE_RUNNING)
            {
                path->cback(path, AUDIO_PATH_EVT_REQ_DATA, 0);
            }
        }
        break;

    case DSP_MGR_EVT_DATA_IND:
        {
            if (path->state == AUDIO_PATH_STATE_RUNNING)
            {
                path->cback(path, AUDIO_PATH_EVT_DATA_IND, 0);
            }
            else
            {
                d2h_data_flush();
            }
        }
        break;

    default:
        break;
    }

}

void audio_dsp_mgr_session_cb(void *handle, T_DSP_MGR_EVENT event, uint32_t param)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    path = (T_AUDIO_PATH *)handle;

    if (path == NULL)
    {
        return;
    }

    plugin_param.dac_sample_rate = sport_mgr_tx_sample_rate_get(path->sport_session);
    plugin_param.adc_sample_rate = sport_mgr_rx_sample_rate_get(path->sport_session);
    switch (event)
    {
    case DSP_MGR_EVT_PREPARE_READY:
        {
            if (audio_path_wait_codec_enabled(path))
            {
                sport_mgr_enable(path->sport_session);
                audio_plugin_occasion_occur(path,
                                            path->category,
                                            AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON,
                                            plugin_param);
            }
        }
        break;

    case DSP_MGR_EVT_SPORT_START_FAKE:
    case DSP_MGR_EVT_FW_READY:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_SET,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_FADE_OUT_FINISH:
        {
            dsp_mgr_decoder_action(path->category, DSP_IPC_ACTION_STOP);
            dsp_mgr_session_disable(path->dsp_session);
        }
        break;

    case DSP_MGR_EVT_SPORT_STOP_FAKE:
    case DSP_MGR_EVT_FW_STOP:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_AUDIOPLAY_VOLUME_INFO:
        {
            T_AUDIO_MSG_SIGNAL_OUT_REFRESH vol_info;
            uint32_t temp;
            uint8_t *buf = (uint8_t *)param;

            LE_STREAM_TO_UINT16(temp, buf);
            vol_info.left_gain = temp;
            LE_STREAM_TO_UINT16(temp, buf);
            vol_info.right_gain = temp;

            path->cback(path, AUDIO_PATH_EVT_SIGNAL_OUT_REFRESH, (uint32_t)(&vol_info));
        }
        break;

    case DSP_MGR_EVT_LATENCY_REPORT:
        {
            T_AUDIO_MSG_STREAM_LATENCY_RPT_EVENT lat_rpt;
            uint32_t temp;
            uint8_t *buf = (uint8_t *)param;

            LE_STREAM_TO_UINT16(temp, buf);
            lat_rpt.normal_packet_count = temp;
            LE_STREAM_TO_UINT16(temp, buf);
            lat_rpt.average_packet_latency = temp;
            LE_STREAM_TO_UINT32(temp, buf);
            lat_rpt.average_fifo_queuing = temp;
            LE_STREAM_TO_UINT32(temp, buf);
            lat_rpt.dsp_plc_sum = temp;

            path->cback(path, AUDIO_PATH_EVT_DSP_LATENCY_RPT, (uint32_t)(&lat_rpt));
        }
        break;

    case DSP_MGR_EVT_PLC_NUM:
        {
            T_AUDIO_MSG_DSP_PLC_EVENT plc;
            uint32_t temp;
            uint8_t *buf = (uint8_t *)param;

            LE_STREAM_TO_UINT32(temp, buf);
            plc.total_frames = temp;
            LE_STREAM_TO_UINT32(temp, buf);
            plc.frame_counter = temp & 0xffff;
            LE_STREAM_TO_UINT32(temp, buf);
            plc.local_seq = temp & 0xffff;
            LE_STREAM_TO_UINT32(temp, buf);
            plc.plc_frame_num = temp & 0xff;

            path->cback(path, AUDIO_PATH_EVT_DSP_PLC, (uint32_t)(&plc));
        }
        break;

    case DSP_MGR_EVT_DECODER_PLC_NOTIFY:
        {
            T_AUDIO_MSG_DECODER_PLC_NOTIFY_EVENT plc_notify;
            uint32_t temp;
            uint8_t *buf = (uint8_t *)param;

            LE_STREAM_TO_UINT32(temp, buf);
            plc_notify.plc_sample_num = temp;
            LE_STREAM_TO_UINT32(temp, buf);
            plc_notify.total_sample_num = temp;
            LE_STREAM_TO_UINT32(temp, buf);
            plc_notify.continue_sample_num = temp;

            path->cback(path, AUDIO_PATH_EVT_DECODER_PLC_NOTIFY, (uint32_t)(&plc_notify));
        }
        break;

    case DSP_MGR_EVT_DECODE_EMPTY:
        {
            if (audio_path_find_category(AUDIO_PATH_STATE_RUNNING, AUDIO_CATEGORY_APT))
            {
                dsp_ipc_set_force_dummy_pkt(DSP_IPC_DUMMY_PKT_START);
            }
            path->cback(path, AUDIO_PATH_EVT_DATA_EMPTY, 0);
        }
        break;

    case DSP_MGR_EVT_DSP_SYNC_UNLOCK:
        {
            path->cback(path, AUDIO_PATH_EVT_DSP_SYNC_UNLOCK, 0);
        }
        break;

    case DSP_MGR_EVT_DSP_SYNC_LOCK:
        {
            path->cback(path, AUDIO_PATH_EVT_DSP_SYNC_LOCK, 0);
        }
        break;

    case DSP_MGR_EVT_DSP_SYNC_V2_SUCC:
        {
            path->cback(path, AUDIO_PATH_EVT_DSP_SYNC_V2_SUCC, 0);
        }
        break;

    case DSP_MGR_EVT_SYNC_EMPTY:
        {
            path->cback(path, AUDIO_PATH_EXC_DSP_SYNC_EMPTY, 0);
        }
        break;

    case DSP_MGR_EVT_SYNC_LOSE_TIMESTAMP:
        {
            path->cback(path, AUDIO_PATH_EXC_DSP_LOST_TIMESTAMP, 0);
        }
        break;

    case DSP_MGR_EVT_DSP_UNSYNC:
        {
            path->cback(path, AUDIO_PATH_EXC_DSP_UNSYNC, 0);
        }
        break;

    case DSP_MGR_EVT_DSP_JOIN_INFO:
        {
            path->cback(path, AUDIO_PATH_EVT_DSP_JOIN_INFO, param);
        }
        break;

    case DSP_MGR_EVT_REQ_DATA:
        {
            path->cback(path, AUDIO_PATH_EVT_REQ_DATA, 0);
        }
        break;

    default:
        break;
    }
}

void record_dsp_mgr_session_cb(void *handle, T_DSP_MGR_EVENT event, uint32_t param)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    path = (T_AUDIO_PATH *)handle;

    if (path == NULL)
    {
        return;
    }

    plugin_param.dac_sample_rate = sport_mgr_tx_sample_rate_get(path->sport_session);
    plugin_param.adc_sample_rate = sport_mgr_rx_sample_rate_get(path->sport_session);
    switch (event)
    {
    case DSP_MGR_EVT_SPORT_STOP_FAKE:
    case DSP_MGR_EVT_FW_STOP:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_PREPARE_READY:
        {
            if (audio_path_wait_codec_enabled(path))
            {
                sport_mgr_enable(path->sport_session);
                audio_plugin_occasion_occur(path,
                                            path->category,
                                            AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON,
                                            plugin_param);
            }
        }
        break;

    case DSP_MGR_EVT_SPORT_START_FAKE:
    case DSP_MGR_EVT_FW_READY:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_SET,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_DATA_IND:
        {
            if (path->state == AUDIO_PATH_STATE_RUNNING)
            {
                path->cback((T_AUDIO_PATH_HANDLE)path, AUDIO_PATH_EVT_DATA_IND, 0);
            }
            else
            {
                d2h_data_flush();
            }
        }
        break;

    default:
        {

        }
        break;
    }
}

static void audio_path_notification_wait_path_ready2running(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    dsp_mgr_session_disable(path->dsp_session);
}

void notification_dsp_mgr_session_cb(void *handle, T_DSP_MGR_EVENT event, uint32_t param)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    path = (T_AUDIO_PATH *)handle;

    if (path == NULL)
    {
        return;
    }

    plugin_param.dac_sample_rate = sport_mgr_tx_sample_rate_get(path->sport_session);
    plugin_param.adc_sample_rate = sport_mgr_rx_sample_rate_get(path->sport_session);
    switch (event)
    {
    case DSP_MGR_EVT_SPORT_START_FAKE:
    case DSP_MGR_EVT_FW_READY:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_SET,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_SPORT_STOP_FAKE:
    case DSP_MGR_EVT_FW_STOP:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_PREPARE_READY:
        {
            if (audio_path_wait_codec_enabled(path))
            {
                sport_mgr_enable(path->sport_session);
                audio_plugin_occasion_occur(path,
                                            path->category,
                                            AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON,
                                            plugin_param);
            }
        }
        break;

    case DSP_MGR_EVT_NOTIFICATION_FINISH:
        {
            audio_path_state_set(path, AUDIO_PATH_STATE_STOPPING);

            dsp_mgr_suppress_tx_gain(0, 1);
            dsp_mgr_decoder_action(path->category, DSP_IPC_ACTION_STOP);

            if (audio_path_db->ready_list.count == 0)
            {
                dsp_mgr_session_disable(path->dsp_session);
            }
            else
            {
                sys_event_flag_post(audio_path_db->event_group,
                                    SYS_EVENT_FLAG_OPT_SET,
                                    AUDIO_PATH_EVENT_FLAG_NOTIFICATION_FINISH);

                sys_event_flag_wait(audio_path_db->event_group,
                                    audio_path_notification_wait_path_ready2running,
                                    (void *)path,
                                    AUDIO_PATH_EVENT_FLAG_NOTIFICATION_FINISH,
                                    SYS_EVENT_FLAG_TYPE_CLEAR_AND);
            }
        }
        break;

    default:
        {

        }
        break;
    }
}

void vad_dsp_mgr_session_cb(void *handle, T_DSP_MGR_EVENT event, uint32_t param)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    path = (T_AUDIO_PATH *)handle;

    if (path == NULL)
    {
        return;
    }

    plugin_param.dac_sample_rate = sport_mgr_tx_sample_rate_get(path->sport_session);
    plugin_param.adc_sample_rate = sport_mgr_rx_sample_rate_get(path->sport_session);
    switch (event)
    {
    case DSP_MGR_EVT_PREPARE_READY:
        {
            if (audio_path_wait_codec_enabled(path))
            {
                sport_mgr_enable(path->sport_session);
                audio_plugin_occasion_occur(path,
                                            path->category,
                                            AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON,
                                            plugin_param);
            }
        }
        break;

    case DSP_MGR_EVT_SPORT_START_FAKE:
    case DSP_MGR_EVT_FW_READY:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_SET,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_SPORT_STOP_FAKE:
    case DSP_MGR_EVT_FW_STOP:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
                                        plugin_param);
        }
        break;

    default:
        {

        }
        break;
    }
}

void analog_dsp_mgr_session_cb(void *handle, T_DSP_MGR_EVENT event, uint32_t param)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    path = (T_AUDIO_PATH *)handle;

    if (path == NULL)
    {
        return;
    }

    plugin_param.dac_sample_rate = sport_mgr_tx_sample_rate_get(path->sport_session);
    plugin_param.adc_sample_rate = sport_mgr_rx_sample_rate_get(path->sport_session);
    switch (event)
    {
    case DSP_MGR_EVT_PREPARE_READY:
        {
            if (audio_path_wait_codec_enabled(path))
            {
                sport_mgr_enable(path->sport_session);
                audio_plugin_occasion_occur(path,
                                            path->category,
                                            AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON,
                                            plugin_param);
            }
        }
        break;

    case DSP_MGR_EVT_SPORT_START_FAKE:
    case DSP_MGR_EVT_FW_READY:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_SET,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_FADE_OUT_FINISH:
        {
            dsp_mgr_line_in_action(DSP_IPC_ACTION_STOP);
            dsp_mgr_session_disable(path->dsp_session);
        }
        break;

    case DSP_MGR_EVT_FW_STOP:
    case DSP_MGR_EVT_SPORT_STOP_FAKE:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_AUDIOPLAY_VOLUME_INFO:
        {
            T_AUDIO_MSG_SIGNAL_OUT_REFRESH vol_info;
            uint32_t temp;
            uint8_t *buf = (uint8_t *)param;

            LE_STREAM_TO_UINT16(temp, buf);
            vol_info.left_gain = temp;
            LE_STREAM_TO_UINT16(temp, buf);
            vol_info.right_gain = temp;

            path->cback(path, AUDIO_PATH_EVT_SIGNAL_OUT_REFRESH, (uint32_t)(&vol_info));
        }
        break;

    default:
        {

        }
        break;
    }

}

void apt_dsp_mgr_session_cb(void *handle, T_DSP_MGR_EVENT event, uint32_t param)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    path = (T_AUDIO_PATH *)handle;

    if (path == NULL)
    {
        return;
    }

    plugin_param.dac_sample_rate = sport_mgr_tx_sample_rate_get(path->sport_session);
    plugin_param.adc_sample_rate = sport_mgr_rx_sample_rate_get(path->sport_session);
    switch (event)
    {
    case DSP_MGR_EVT_FADE_OUT_FINISH:
        {
            dsp_mgr_session_disable(path->dsp_session);
        }
        break;

    case DSP_MGR_EVT_FW_STOP:
    case DSP_MGR_EVT_SPORT_STOP_FAKE:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
                                        plugin_param);
        }
        break;

    case DSP_MGR_EVT_PREPARE_READY:
        {
            if (audio_path_wait_codec_enabled(path))
            {
                sport_mgr_enable(path->sport_session);
                audio_plugin_occasion_occur(path,
                                            path->category,
                                            AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON,
                                            plugin_param);
            }
        }
        break;

    case DSP_MGR_EVT_FW_READY:
        {
            sys_event_flag_post(path->event_group,
                                SYS_EVENT_FLAG_OPT_SET,
                                EVENT_FLAG_DSP_READY);
            audio_plugin_occasion_occur(path,
                                        path->category,
                                        AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON,
                                        plugin_param);
        }
        break;

    default:
        {

        }
        break;
    }
}

static bool audio_path_audio_init(void *param, uint8_t dac_level, T_AUDIO_PATH *path)
{
    uint32_t dac_sample_rate = 0;
    uint32_t adc_sample_rate = 0;
    uint8_t  adc_gain_level  = 0;
    uint8_t  dac_gain_level  = dac_level;
    uint8_t  tx_chann_num    = 0;
    T_AUDIO_FORMAT_INFO *format_info = (T_AUDIO_FORMAT_INFO *)param;
    T_DSP_SESSION_CFG dsp_cfg;

    if (format_info->type == AUDIO_FORMAT_TYPE_SBC)
    {
        dac_sample_rate = format_info->attr.sbc.sample_rate;
        tx_chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_SBC, &format_info->attr.sbc);
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_AAC)
    {
        dac_sample_rate = format_info->attr.aac.sample_rate;
        tx_chann_num = format_info->attr.aac.chann_num;
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_MP3)
    {
        dac_sample_rate = format_info->attr.mp3.sample_rate;
        tx_chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_MP3, &format_info->attr.mp3);
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_LC3)
    {
        dac_sample_rate = format_info->attr.lc3.sample_rate;
        tx_chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_LC3, &format_info->attr.lc3);
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_PCM)
    {
        dac_sample_rate = format_info->attr.pcm.sample_rate;
        tx_chann_num = format_info->attr.pcm.chann_num;
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_FLAC)
    {
        dac_sample_rate = format_info->attr.flac.sample_rate;
        tx_chann_num = 2;
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_LDAC)
    {
        dac_sample_rate = format_info->attr.ldac.sample_rate;
        tx_chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_LDAC, &format_info->attr.ldac);
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_LHDC)
    {
        dac_sample_rate = format_info->attr.lhdc.sample_rate;
        tx_chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_LHDC, &format_info->attr.lhdc);
    }

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_AUDIO,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        goto fail_create_codec_session;
    }

    path->sport_session = sport_mgr_session_create(AUDIO_CATEGORY_AUDIO, tx_chann_num, 0,
                                                   dac_sample_rate,
                                                   adc_sample_rate);
    if (path->sport_session == NULL)
    {
        goto fail_create_sport_session;
    }

    dsp_cfg.context = path;
    dsp_cfg.callback = audio_dsp_mgr_session_cb;
    dsp_cfg.sport_handle = path->sport_session;
    dsp_cfg.data_mode = path->data_mode;
    dsp_cfg.format_info = format_info;
    path->dsp_session = dsp_mgr_session_create(DSP_SESSION_TYPE_AUDIO, dsp_cfg);
    if (path->dsp_session == NULL)
    {
        goto fail_create_dsp_session;
    }

    return true;

fail_create_dsp_session:
    sport_mgr_destroy(path->sport_session);
fail_create_sport_session:
    codec_mgr_session_destroy(path->codec_session);
fail_create_codec_session:
    return false;
}

static bool audio_path_voice_init(void *param,
                                  uint8_t dac_level,
                                  uint8_t adc_level,
                                  T_AUDIO_PATH *path)
{
    uint32_t dac_sample_rate = 0;
    uint32_t adc_sample_rate = 0;
    uint8_t  adc_gain_level  = adc_level;
    uint8_t  dac_gain_level  = dac_level;
    uint8_t  tx_chann_num    = 1;
    uint8_t  rx_chann_num    = 2;
    T_AUDIO_FORMAT_INFO *format_info = (T_AUDIO_FORMAT_INFO *)param;
    T_DSP_SESSION_CFG dsp_cfg;

    if (format_info->type == AUDIO_FORMAT_TYPE_CVSD)
    {
        dac_sample_rate = 16000;
        adc_sample_rate = 16000;
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_MSBC)
    {
        dac_sample_rate = format_info->attr.msbc.sample_rate;
        adc_sample_rate = format_info->attr.msbc.sample_rate;
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_LC3)
    {
        dac_sample_rate = format_info->attr.lc3.sample_rate;
        adc_sample_rate = format_info->attr.lc3.sample_rate;
    }

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_VOICE,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        goto fail_create_codec_session;
    }

    path->sport_session = sport_mgr_session_create(AUDIO_CATEGORY_VOICE, tx_chann_num, rx_chann_num,
                                                   dac_sample_rate,
                                                   adc_sample_rate);
    if (path->sport_session == NULL)
    {
        goto fail_create_sport_session;
    }

    dsp_cfg.context = path;
    dsp_cfg.callback = voice_dsp_mgr_session_cb;
    dsp_cfg.sport_handle = path->sport_session;
    dsp_cfg.data_mode = path->data_mode;
    dsp_cfg.format_info = format_info;
    path->dsp_session = dsp_mgr_session_create(DSP_SESSION_TYPE_VOICE, dsp_cfg);
    if (path->dsp_session == NULL)
    {
        goto fail_create_dsp_session;
    }

    return true;

fail_create_dsp_session:
    sport_mgr_destroy(path->sport_session);
fail_create_sport_session:
    codec_mgr_session_destroy(path->codec_session);
fail_create_codec_session:
    return false;
}

static bool audio_path_record_init(void *param,
                                   uint32_t device,
                                   uint8_t adc_level,
                                   T_AUDIO_PATH *path)
{
    uint32_t dac_sample_rate = 0;
    uint32_t adc_sample_rate = 0;
    uint8_t  dac_gain_level  = 0;
    uint8_t  adc_gain_level  = adc_level;
    uint8_t  rx_chann_num    = 0;
    T_AUDIO_FORMAT_INFO *format_info = (T_AUDIO_FORMAT_INFO *)param;
    T_DSP_SESSION_CFG dsp_cfg;

    audio_route_record_dev_set(device);

    if (format_info->type == AUDIO_FORMAT_TYPE_SBC)
    {
        adc_sample_rate = format_info->attr.sbc.sample_rate;
        rx_chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_SBC, &format_info->attr.sbc);
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_MSBC)
    {
        adc_sample_rate = format_info->attr.msbc.sample_rate;
        rx_chann_num = 1;
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_OPUS)
    {
        adc_sample_rate = format_info->attr.opus.sample_rate;
        rx_chann_num = format_info->attr.opus.chann_num;
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_PCM)
    {
        adc_sample_rate = format_info->attr.pcm.sample_rate;
        rx_chann_num = format_info->attr.pcm.chann_num;
    }
    else if (format_info->type == AUDIO_FORMAT_TYPE_LC3)
    {
        adc_sample_rate = format_info->attr.lc3.sample_rate;
        rx_chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_LC3, &format_info->attr.lc3);
    }

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_RECORD,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        goto fail_create_codec_session;
    }

    path->sport_session = sport_mgr_session_create(AUDIO_CATEGORY_RECORD, 0, rx_chann_num,
                                                   dac_sample_rate, adc_sample_rate);
    if (path->sport_session == NULL)
    {
        goto fail_create_sport_session;
    }

    dsp_cfg.context = path;
    dsp_cfg.callback = record_dsp_mgr_session_cb;
    dsp_cfg.sport_handle = path->sport_session;
    dsp_cfg.data_mode = path->data_mode;
    dsp_cfg.format_info = format_info;

    path->dsp_session = dsp_mgr_session_create(DSP_SESSION_TYPE_RECORD, dsp_cfg);
    if (path->dsp_session == NULL)
    {
        goto fail_create_dsp_session;
    }

    return true;

fail_create_dsp_session:
    sport_mgr_destroy(path->sport_session);
fail_create_sport_session:
    codec_mgr_session_destroy(path->codec_session);
fail_create_codec_session:
    return false;
}

static bool audio_path_tone_init(uint8_t dac_level, T_AUDIO_PATH *path)
{
    uint32_t  dac_sample_rate = 16000;
    uint32_t  adc_sample_rate = 0;
    uint8_t   adc_gain_level  = 0;
    uint8_t   dac_gain_level  = dac_level;
    uint8_t   tx_chann_num    = 1;
    T_DSP_SESSION_CFG dsp_cfg;

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_TONE,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        goto fail_create_codec_session;
    }

    path->sport_session = sport_mgr_session_create(AUDIO_CATEGORY_TONE, tx_chann_num, 0,
                                                   dac_sample_rate, adc_sample_rate);
    if (path->sport_session == NULL)
    {
        goto fail_create_sport_session;
    }

    dsp_cfg.context = path;
    dsp_cfg.callback = notification_dsp_mgr_session_cb;
    dsp_cfg.sport_handle = path->sport_session;
    dsp_cfg.data_mode = path->data_mode;
    path->dsp_session = dsp_mgr_session_create(DSP_SESSION_TYPE_TONE, dsp_cfg);
    if (path->dsp_session == NULL)
    {
        goto fail_create_dsp_session;
    }

    return true;

fail_create_dsp_session:
    sport_mgr_destroy(path->sport_session);
fail_create_sport_session:
    codec_mgr_session_destroy(path->codec_session);
fail_create_codec_session:
    return false;
}

static bool audio_path_vp_init(uint8_t dac_level, T_AUDIO_PATH *path)
{
    uint32_t  dac_sample_rate = 16000;
    uint32_t  adc_sample_rate = 0;
    uint8_t   adc_gain_level  = 0;
    uint8_t   dac_gain_level  = dac_level;
    uint8_t   tx_chann_num    = 1;
    T_DSP_SESSION_CFG dsp_cfg;

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_VP,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        goto fail_create_codec_session;
    }

    path->sport_session = sport_mgr_session_create(AUDIO_CATEGORY_VP, tx_chann_num, 0, dac_sample_rate,
                                                   adc_sample_rate);
    if (path->sport_session == NULL)
    {
        goto fail_create_sport_session;
    }

    dsp_cfg.context = path;
    dsp_cfg.callback = notification_dsp_mgr_session_cb;
    dsp_cfg.sport_handle = path->sport_session;
    dsp_cfg.data_mode = path->data_mode;
    path->dsp_session = dsp_mgr_session_create(DSP_SESSION_TYPE_VP, dsp_cfg);
    if (path->dsp_session == NULL)
    {
        goto fail_create_dsp_session;
    }

    return true;

fail_create_dsp_session:
    sport_mgr_destroy(path->sport_session);
fail_create_sport_session:
    codec_mgr_session_destroy(path->codec_session);
fail_create_codec_session:
    return false;
}

static bool audio_path_analog_init(void   *info,
                                   uint8_t dac_level,
                                   uint8_t adc_level,
                                   T_AUDIO_PATH *path)
{
    T_ANALOG_INFO *analog_info = (T_ANALOG_INFO *)info;
    uint32_t       dac_sample_rate = analog_info->sample_rate;
    uint32_t       adc_sample_rate = analog_info->sample_rate;
    uint8_t        adc_gain_level = adc_level;
    uint8_t        dac_gain_level = dac_level;
    uint8_t        tx_chann_num   = 2;
    uint8_t        rx_chann_num   = 2;
    T_DSP_SESSION_CFG dsp_cfg;

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_ANALOG,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        goto fail_create_codec_session;
    }

    path->sport_session = sport_mgr_session_create(AUDIO_CATEGORY_ANALOG, tx_chann_num, rx_chann_num,
                                                   dac_sample_rate,
                                                   adc_sample_rate);
    if (path->sport_session == NULL)
    {
        goto fail_create_sport_session;
    }

    dsp_cfg.context = path;
    dsp_cfg.callback = analog_dsp_mgr_session_cb;
    dsp_cfg.sport_handle = path->sport_session;
    dsp_cfg.data_mode = path->data_mode;
    path->dsp_session = dsp_mgr_session_create(DSP_SESSION_TYPE_ANALOG, dsp_cfg);
    if (path->dsp_session == NULL)
    {
        goto fail_create_dsp_session;
    }

    return true;

fail_create_dsp_session:
    sport_mgr_destroy(path->sport_session);
fail_create_sport_session:
    codec_mgr_session_destroy(path->codec_session);
fail_create_codec_session:
    return false;
}

static bool audio_path_apt_init(void *info,
                                uint8_t dac_level,
                                uint8_t adc_level,
                                T_AUDIO_PATH *path)
{
    T_APT_INFO        *apt_info = (T_APT_INFO *)info;
    uint32_t           dac_sample_rate = apt_info->sample_rate;
    uint32_t           adc_sample_rate = apt_info->sample_rate;
    uint8_t            adc_gain_level = adc_level;
    uint8_t            dac_gain_level = dac_level;
    uint8_t            tx_chann_num   = 2;
    uint8_t            rx_chann_num   = 2;
    T_DSP_SESSION_CFG  dsp_cfg;

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_APT,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        goto fail_create_codec_session;
    }

    path->sport_session = sport_mgr_session_create(AUDIO_CATEGORY_APT, tx_chann_num, rx_chann_num,
                                                   dac_sample_rate,
                                                   adc_sample_rate);
    if (path->sport_session == NULL)
    {
        goto fail_create_sport_session;
    }

    dsp_cfg.context = path;
    dsp_cfg.callback = apt_dsp_mgr_session_cb;
    dsp_cfg.sport_handle = path->sport_session;
    dsp_cfg.data_mode = path->data_mode;
    path->dsp_session = dsp_mgr_session_create(DSP_SESSION_TYPE_APT, dsp_cfg);
    if (path->dsp_session == NULL)
    {
        goto fail_create_dsp_session;
    }

    return true;

fail_create_dsp_session:
    sport_mgr_destroy(path->sport_session);
fail_create_sport_session:
    codec_mgr_session_destroy(path->codec_session);
fail_create_codec_session:
    return false;
}

static bool audio_path_llapt_init(uint8_t dac_level,
                                  uint8_t adc_level,
                                  T_AUDIO_PATH *path)
{
    uint32_t dac_sample_rate = 0;
    uint32_t adc_sample_rate = 0;
    uint8_t  adc_gain_level  = adc_level;
    uint8_t  dac_gain_level  = dac_level;

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_LLAPT,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        return false;
    }

    path->dsp_session = NULL;

    return true;
}

static bool audio_path_anc_init(uint8_t dac_level,
                                uint8_t adc_level,
                                T_AUDIO_PATH *path)
{
    uint32_t dac_sample_rate = 0;
    uint32_t adc_sample_rate = 0;
    uint8_t  adc_gain_level  = adc_level;
    uint8_t  dac_gain_level  = dac_level;

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_ANC,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        return false;
    }

    path->dsp_session = NULL;

    return true;
}
#if 0
static bool audio_path_vad_init(uint8_t adc_level,
                                T_AUDIO_PATH *path)
{
    uint32_t           dac_sample_rate = 16000;
    uint32_t           adc_sample_rate = 16000;
    uint8_t            adc_gain_level = adc_level;;
    uint8_t            dac_gain_level = 0;
    uint8_t            rx_chann_num   = 2;
    T_DSP_SESSION_CFG  dsp_cfg;

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_VAD,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);
    if (path->codec_session == NULL)
    {
        goto fail_create_codec_session;
    }

    path->sport_session = sport_mgr_session_create(AUDIO_CATEGORY_VAD, 0, rx_chann_num, dac_sample_rate,
                                                   adc_sample_rate);
    if (path->sport_session == NULL)
    {
        goto fail_create_sport_session;
    }

    dsp_cfg.context = path;
    dsp_cfg.callback = vad_dsp_mgr_session_cb;
    dsp_cfg.sport_handle = path->sport_session;
    dsp_cfg.data_mode = path->data_mode;
    path->dsp_session = dsp_mgr_session_create(DSP_SESSION_TYPE_VAD, dsp_cfg);
    if (path->dsp_session == NULL)
    {
        goto fail_create_dsp_session;
    }

    return true;

fail_create_dsp_session:
    sport_mgr_destroy(path->sport_session);
fail_create_sport_session:
    codec_mgr_session_destroy(path->codec_session);
fail_create_codec_session:
    return false;
}
#endif

static bool audio_path_sidetone_init(uint8_t dac_level,
                                     uint8_t adc_level,
                                     T_AUDIO_PATH *path)
{
    uint32_t dac_sample_rate = 0;
    uint32_t adc_sample_rate = 0;
    uint8_t  adc_gain_level  = adc_level;
    uint8_t  dac_gain_level  = dac_level;

    path->codec_session = codec_mgr_session_create(AUDIO_CATEGORY_SIDETONE,
                                                   dac_sample_rate,
                                                   adc_sample_rate,
                                                   dac_gain_level,
                                                   adc_gain_level,
                                                   path);

    if (path->codec_session == NULL)
    {
        return false;
    }

    path->dsp_session = NULL;

    return true;
}

T_AUDIO_PATH_HANDLE audio_path_create(T_AUDIO_CATEGORY          category,
                                      uint32_t                  device,
                                      void                     *info,
                                      T_AUDIO_STREAM_MODE       mode,
                                      uint8_t                   dac_level,
                                      uint8_t                   adc_level,
                                      P_AUDIO_PATH_CBACK        cback)
{
    T_OS_QUEUE *idle_queue;
    T_AUDIO_PATH *path;
    int32_t ret = 0;

    if (cback == NULL)
    {
        ret = 1;
        goto fail_check_param;
    }

    path = os_mem_zalloc2(sizeof(T_AUDIO_PATH));
    if (path == NULL)
    {
        ret = 2;
        goto fail_alloc_path;
    }

    path->category = category;
    path->cback = cback;
    path->state = AUDIO_PATH_STATE_IDLE;
    path->event_group = sys_event_group_create(0);
    path->data_mode = mode;

    switch (category)
    {
    case AUDIO_CATEGORY_AUDIO:
        {
            if (!audio_path_audio_init(info, dac_level, path))
            {
                ret = 3;
                goto fail_init_path;
            }
        }
        break;

    case AUDIO_CATEGORY_VOICE:
        {
            if (!audio_path_voice_init(info, dac_level, adc_level, path))
            {
                ret = 4;
                goto fail_init_path;
            }
        }
        break;

    case AUDIO_CATEGORY_RECORD:
        {
            if (!audio_path_record_init(info, device, adc_level, path))
            {
                ret = 5;
                goto fail_init_path;
            }
        }
        break;

    case AUDIO_CATEGORY_TONE:
        {
            if (!audio_path_tone_init(dac_level, path))
            {
                ret = 6;
                goto fail_init_path;
            }
        }
        break;

    case AUDIO_CATEGORY_VP:
        {
            if (!audio_path_vp_init(dac_level, path))
            {
                ret = 7;
                goto fail_init_path;
            }
        }
        break;

    case AUDIO_CATEGORY_ANALOG:
        {
            if (!audio_path_analog_init(info, dac_level, adc_level, path))
            {
                ret = 8;
                goto fail_init_path;
            }
        }
        break;

    case AUDIO_CATEGORY_APT:
        {
            if (!audio_path_apt_init(info, dac_level, adc_level, path))
            {
                ret = 9;
                goto fail_init_path;
            }
        }
        break;

    case AUDIO_CATEGORY_LLAPT:
        {
            if (!audio_path_llapt_init(dac_level, adc_level, path))
            {
                ret = 10;
                goto fail_init_path;
            }
        }
        break;

    case AUDIO_CATEGORY_ANC:
        {
            if (!audio_path_anc_init(dac_level, adc_level, path))
            {
                ret = 11;
                goto fail_init_path;
            }
        }
        break;
#if 0
    case AUDIO_CATEGORY_VAD:
        {
            if (!audio_path_vad_init(adc_level, path))
            {
                ret = 12;
                goto fail_init_path;
            }
        }
        break;
#endif

    case AUDIO_CATEGORY_SIDETONE:
        {
            if (!audio_path_sidetone_init(dac_level, adc_level, path))
            {
                ret = 11;
                goto fail_init_path;
            }
        }
        break;

    default:
        {
            ret = 13;
            goto fail_error_category;
        }
    }

    idle_queue = audio_path_get_queue(AUDIO_PATH_STATE_IDLE);
    os_queue_in(idle_queue, &path->state_list);

    path->cback((T_AUDIO_PATH_HANDLE)path, AUDIO_PATH_EVT_CREATE, 0);

    return path;

fail_error_category:
fail_init_path:
    os_mem_free(path);
fail_alloc_path:
fail_check_param:
    AUDIO_PRINT_ERROR4("audio_path_create: category %u, device 0x%08x, cback %p, ret %d",
                       category, device, cback, -ret);
    return NULL;
}

bool audio_path_destory(T_AUDIO_PATH_HANDLE handle)
{
    T_AUDIO_PATH *audio_path;
    int8_t ret = 0;

    audio_path = (T_AUDIO_PATH *)handle;
    if (audio_path == NULL)
    {
        ret = 1;
        goto fail_check_param;
    }

    if (audio_path->state != AUDIO_PATH_STATE_IDLE)
    {
        ret = 2;
        goto fail_check_state;
    }

    if (audio_path_state_exit(audio_path, audio_path->state) == false)
    {
        ret = 3;
        goto fail_remove_element;
    }

    audio_path->cback((T_AUDIO_PATH_HANDLE)audio_path, AUDIO_PATH_EVT_RELEASE, 0);

    codec_mgr_session_destroy(audio_path->codec_session);
    sport_mgr_destroy(audio_path->sport_session);
    dsp_mgr_session_destroy(audio_path->dsp_session);
    sys_event_group_delete(audio_path->event_group);

    os_mem_free(audio_path);

    return true;

fail_remove_element:
fail_check_state:
fail_check_param:
    AUDIO_PRINT_ERROR1("audio_path_destory: ret %d", -ret);
    return false;
}

static void audio_path_analog_stop(T_AUDIO_PATH *audio_path)
{
    audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);
    audio_path_start_fade_out(audio_path);
}

static void audio_path_audio_stop(T_AUDIO_PATH *audio_path)
{
    T_AUDIO_PATH *normal_amb_path;

    normal_amb_path = audio_path_find_category(AUDIO_PATH_STATE_RUNNING, AUDIO_CATEGORY_APT);
    /* TODO: optimize */
    if (normal_amb_path == NULL)
    {
        normal_amb_path = audio_path_find_category(AUDIO_PATH_STATE_READY, AUDIO_CATEGORY_APT);
    }

    if (normal_amb_path == NULL)
    {
        audio_path_start_fade_out(audio_path);
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);
    }
    else
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);

        dsp_ipc_set_force_dummy_pkt(DSP_IPC_DUMMY_PKT_START);
        dsp_mgr_decoder_action(audio_path->category, DSP_IPC_ACTION_STOP);
        dsp_mgr_session_disable(audio_path->dsp_session);
    }
}

static void audio_path_suspend(T_AUDIO_PATH *running_path, T_AUDIO_PATH *audio_path)
{
    T_AUDIO_PATH *next_path;

    while (running_path != NULL)
    {
        next_path = LIST_NEXT_ENTRY(running_path, state_list);

        if (audio_path == audio_path_schedule_priority(running_path, audio_path))
        {
            switch (running_path->category)
            {
            case AUDIO_CATEGORY_AUDIO:
            case AUDIO_CATEGORY_VOICE:
            case AUDIO_CATEGORY_ANALOG:
                {
                    audio_path_state_set(running_path, AUDIO_PATH_STATE_SUSPEND);
                    audio_path_start_fade_out(running_path);
                }
                break;

            case AUDIO_CATEGORY_APT:
                {
                    audio_path_apt_suspend(running_path);
                }
                break;

            case AUDIO_CATEGORY_RECORD:
                {
                    audio_path_state_set(running_path, AUDIO_PATH_STATE_SUSPEND);
                    dsp_mgr_session_disable(running_path->dsp_session);
                }
                break;
#if 0
            case AUDIO_CATEGORY_VAD:
                {
                    audio_path_vad_suspend(running_path);
                }
                break;
#endif
            default:
                break;
            }
        }

        running_path = next_path;
    }
}

static void audio_path_normal_apt_stop(T_AUDIO_PATH *audio_path)
{
    dsp_mgr_apt_action(DSP_IPC_ACTION_STOP);

    sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_SET,
                        AUDIO_PATH_EVENT_FLAG_ACTION_STOP_ACK);

    sys_event_flag_wait(audio_path_db->event_group,
                        apt_wait_action_stop_ack_cback,
                        (void *)audio_path,
                        AUDIO_PATH_EVENT_FLAG_ACTION_STOP_ACK,
                        SYS_EVENT_FLAG_TYPE_CLEAR_AND);

    audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);
}

static void anc_llapt_wait_disable_finish_cback(void *handle)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    path = (T_AUDIO_PATH *)handle;
    plugin_param.dac_sample_rate = 48000;
    plugin_param.adc_sample_rate = 48000;

    if (path == NULL)
    {
        return;
    }
    else
    {
        /* post a dummy AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF to make path into IDLE */
        audio_plugin_occasion_occur((T_AUDIO_PATH_HANDLE)path,
                                    path->category,
                                    AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
                                    plugin_param);
    }
}

static void audio_path_llapt_stop(T_AUDIO_PATH *audio_path)
{
    if (audio_path)
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);

        if (!sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING))
        {
            sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_SET,
                                AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING);

            sys_event_flag_wait(audio_path_db->event_group,
                                anc_llapt_wait_disable_finish_cback,
                                (void *)audio_path,
                                AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING,
                                SYS_EVENT_FLAG_TYPE_CLEAR_AND);
            anc_mgr_disable();
        }
    }
}

static void anc_llapt_wait_enable_finish_cback(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        audio_path_state_set(path, AUDIO_PATH_STATE_RUNNING);
    }

    audio_path_schedule();
}

static void notification_wait_running_handler(void *handle)
{
    int16_t ramp_gain;
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        ramp_gain = audio_route_ramp_gain_get(path->category);
        dsp_mgr_decoder_action(path->category, DSP_IPC_ACTION_START);
        dsp_mgr_suppress_tx_gain(ramp_gain, 1);

        audio_path_state_set(path, AUDIO_PATH_STATE_RUNNING);
    }

    audio_path_schedule();
}

static void audio_path_notification_start(T_AUDIO_PATH *audio_path)
{
    uint32_t dev_map;

    dev_map = EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_DSP_READY | EVENT_FLAG_PLUGIN_ENABLE;

    if (audio_path_is_mixable(audio_path))
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_READY);

        codec_mgr_session_enable(audio_path->codec_session);

        dsp_mgr_session_enable(audio_path->dsp_session);

        sys_event_flag_wait(audio_path->event_group,
                            notification_wait_running_handler,
                            audio_path,
                            dev_map,
                            SYS_EVENT_FLAG_TYPE_SET_AND);
    }
    else
    {
        T_AUDIO_PATH *running_path;

        running_path = dsp_mgr_peek_running_path();
        audio_path_suspend(running_path, audio_path);
    }
}

static void anc_llapt_wait_running_handler(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path == NULL)
    {
        return;
    }

    if (!sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_ENABLING))
    {
        sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_SET,
                            AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_ENABLING);

        sys_event_flag_wait(audio_path_db->event_group,
                            anc_llapt_wait_enable_finish_cback,
                            (void *)path,
                            AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_ENABLING,
                            SYS_EVENT_FLAG_TYPE_CLEAR_AND);
        anc_mgr_enable();
    }
}

static void audio_path_anc_start(T_AUDIO_PATH *audio_path)
{
    uint32_t      dev_map;

    dev_map = EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_PLUGIN_ENABLE;

    if (sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING))
    {
        return;
    }

    if (audio_path_is_mixable(audio_path))
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_READY);

        codec_mgr_session_enable(audio_path->codec_session);

        sys_event_flag_wait(audio_path->event_group,
                            anc_llapt_wait_running_handler,
                            (void *)audio_path,
                            dev_map,
                            SYS_EVENT_FLAG_TYPE_SET_AND);
    }
}

static void audio_wait_running_handler(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        dsp_mgr_decoder_action(path->category, DSP_IPC_ACTION_START);

        audio_path_state_set(path, AUDIO_PATH_STATE_RUNNING);
    }

    audio_path_schedule();
}

static void audio_path_audio_start(T_AUDIO_PATH *audio_path)
{
    T_AUDIO_PATH *running_path;
    uint32_t dev_map;

    dev_map = EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_DSP_READY | EVENT_FLAG_PLUGIN_ENABLE;

    if (audio_path_is_mixable(audio_path))
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_READY);

        codec_mgr_session_enable(audio_path->codec_session);
        dsp_mgr_session_enable(audio_path->dsp_session);

        sys_event_flag_wait(audio_path->event_group,
                            audio_wait_running_handler,
                            audio_path,
                            dev_map,
                            SYS_EVENT_FLAG_TYPE_SET_AND);
    }
    else
    {
        running_path = dsp_mgr_peek_running_path();
        audio_path_suspend(running_path, audio_path);
    }
}

static void voice_wait_running_handler(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        dsp_mgr_decoder_action(path->category, DSP_IPC_ACTION_START);
        dsp_mgr_encoder_action(path->category, DSP_IPC_ACTION_START);

        audio_path_state_set(path, AUDIO_PATH_STATE_RUNNING);
    }

    audio_path_schedule();
}

static void audio_path_voice_start(T_AUDIO_PATH *audio_path)
{
    T_AUDIO_PATH *tone_path;
    T_AUDIO_PATH *vp_path;
    T_AUDIO_PATH *running_path;
    uint32_t dev_map;

    dev_map = EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_DSP_READY | EVENT_FLAG_PLUGIN_ENABLE;

    tone_path = audio_path_find_category(AUDIO_PATH_STATE_RUNNING,
                                         AUDIO_CATEGORY_TONE);
    vp_path = audio_path_find_category(AUDIO_PATH_STATE_RUNNING,
                                       AUDIO_CATEGORY_VP);

    if ((tone_path != NULL) || (vp_path != NULL))
    {
        return;
    }

    if (audio_path_is_mixable(audio_path))
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_READY);

        codec_mgr_session_enable(audio_path->codec_session);
        dsp_mgr_session_enable(audio_path->dsp_session);

        sys_event_flag_wait(audio_path->event_group,
                            voice_wait_running_handler,
                            audio_path,
                            dev_map,
                            SYS_EVENT_FLAG_TYPE_SET_AND);
    }
    else
    {
        running_path = dsp_mgr_peek_running_path();
        if (running_path != NULL)
        {
            audio_path_suspend(running_path, audio_path);
        }
    }
}

#if 0
static void audio_path_vad_suspend(T_AUDIO_PATH *audio_path)
{
    audio_path_state_set(audio_path, AUDIO_PATH_STATE_SUSPEND);

    dsp_mgr_vad_action(DSP_IPC_ACTION_STOP);
    dsp_mgr_session_disable(audio_path->dsp_session);
}


static void vad_wait_running_handler(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        dsp_mgr_vad_action(DSP_IPC_ACTION_START);

        audio_path_state_set(path, AUDIO_PATH_STATE_RUNNING);
    }

    audio_path_schedule();
}

static void audio_path_vad_start(T_AUDIO_PATH *vad_path)
{
    T_AUDIO_PATH *audio_path;

    if (audio_path_is_mixable(vad_path))
    {
        audio_path_state_set(vad_path, AUDIO_PATH_STATE_READY);

        audio_path = audio_path_find_category(AUDIO_PATH_STATE_RUNNING, AUDIO_CATEGORY_AUDIO);
        if (audio_path != NULL)
        {
            dsp_vad_param_set(VAD_SCENARIO_A2DP);
        }
        else
        {
            dsp_vad_param_set(VAD_SCENARIO_IDLE);
        }

        codec_mgr_session_enable(vad_path->codec_session);
        dsp_mgr_session_enable(vad_path->dsp_session);

        sys_event_flag_wait(vad_path->event_group,
                            vad_wait_running_handler,
                            vad_path,
                            EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_DSP_READY | EVENT_FLAG_PLUGIN_ENABLE,
                            SYS_EVENT_FLAG_TYPE_SET_AND);
    }
}
#endif

static void audio_path_apt_suspend(T_AUDIO_PATH *apt_path)
{
    dsp_mgr_apt_action(DSP_IPC_ACTION_STOP);

    sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_SET,
                        AUDIO_PATH_EVENT_FLAG_ACTION_STOP_ACK);

    sys_event_flag_wait(audio_path_db->event_group,
                        apt_wait_action_stop_ack_cback,
                        (void *)apt_path,
                        AUDIO_PATH_EVENT_FLAG_ACTION_STOP_ACK,
                        SYS_EVENT_FLAG_TYPE_CLEAR_AND);

    audio_path_state_set(apt_path, AUDIO_PATH_STATE_SUSPEND);
}

static void audio_path_normal_apt_start(T_AUDIO_PATH *audio_path)
{
    uint32_t dev_map;

    dev_map = EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_DSP_READY | EVENT_FLAG_PLUGIN_ENABLE;

    if (audio_path_is_mixable(audio_path))
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_READY);

        dsp_mgr_session_enable(audio_path->dsp_session);
        codec_mgr_session_enable(audio_path->codec_session);
        sys_event_flag_wait(audio_path->event_group,
                            apt_wait_running_handler,
                            (void *)audio_path,
                            dev_map,
                            SYS_EVENT_FLAG_TYPE_SET_AND);
    }
}

static void audio_path_llapt_start(T_AUDIO_PATH *audio_path)
{
    uint32_t      dev_map;

    dev_map = EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_PLUGIN_ENABLE;

    if (audio_path_is_mixable(audio_path))
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_READY);

        codec_mgr_session_enable(audio_path->codec_session);

        sys_event_flag_wait(audio_path->event_group,
                            anc_llapt_wait_running_handler,
                            (void *)audio_path,
                            dev_map,
                            SYS_EVENT_FLAG_TYPE_SET_AND);
    }
}

static void sidetone_wait_enable_finish_cback(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;
    if (path != NULL)
    {
        audio_path_state_set(path, AUDIO_PATH_STATE_RUNNING);
    }

    audio_path_schedule();
}

static void sidetone_wait_disable_finish_cback(void *handle)
{
    T_AUDIO_PATH *path;
    T_AUDIO_PLUGIN_PARAM plugin_param;

    path = (T_AUDIO_PATH *)handle;
    plugin_param.dac_sample_rate = 48000;
    plugin_param.adc_sample_rate = 48000;

    if (path == NULL)
    {
        return;
    }
    else
    {
        /* post a dummy AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF to make path into IDLE */
        audio_plugin_occasion_occur((T_AUDIO_PATH_HANDLE)path,
                                    path->category,
                                    AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
                                    plugin_param);
    }
}

static void sidetone_wait_running_handler(void *handle)
{
    T_AUDIO_PATH *path;
    T_AUDIO_ROUTE_SIDETONE_INFO info;

    path = (T_AUDIO_PATH *)handle;
    if (path == NULL)
    {
        return;
    }

    if (!sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_SIDETONE_ENABLING))
    {
        sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_SET,
                            AUDIO_PATH_EVENT_FLAG_SIDETONE_ENABLING);

        sys_event_flag_wait(audio_path_db->event_group,
                            sidetone_wait_enable_finish_cback,
                            (void *)path,
                            AUDIO_PATH_EVENT_FLAG_SIDETONE_ENABLING,
                            SYS_EVENT_FLAG_TYPE_CLEAR_AND);

        if (audio_route_sidetone_info_get(&info) == true)
        {
            codec_mgr_sidetone_set(info.sidetone_gain, info.level, 1);
        }
    }
}

static void audio_path_sidetone_start(T_AUDIO_PATH *audio_path)
{
    uint32_t      dev_map;

    dev_map = EVENT_FLAG_CODEC_ENABLE | EVENT_FLAG_PLUGIN_ENABLE;

    if (audio_path_is_mixable(audio_path))
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_READY);

        codec_mgr_session_enable(audio_path->codec_session);

        sys_event_flag_wait(audio_path->event_group,
                            sidetone_wait_running_handler,
                            (void *)audio_path,
                            dev_map,
                            SYS_EVENT_FLAG_TYPE_SET_AND);
    }
}

bool audio_path_start(T_AUDIO_PATH_HANDLE handle)
{
    T_AUDIO_PATH *audio_path;
    audio_path = (T_AUDIO_PATH *)handle;

    if (audio_path == NULL)
    {
        AUDIO_PRINT_ERROR1("audio_path_start: handle %p, no such path", handle);
        return false;
    }

    AUDIO_PRINT_INFO2("audio_path_start: handle %p, current status: 0x%02x", handle, audio_path->state);

    if ((audio_path->state == AUDIO_PATH_STATE_PENDING) ||
        (audio_path->state == AUDIO_PATH_STATE_READY) ||
        (audio_path->state == AUDIO_PATH_STATE_SUSPEND))
    {
        return true;
    }
    else if (audio_path->state != AUDIO_PATH_STATE_IDLE)
    {
        return false;
    }

    audio_path_state_set(audio_path, AUDIO_PATH_STATE_PENDING);

    audio_path_schedule();

    return true;
}

static void audio_path_record_stop(T_AUDIO_PATH *audio_path)
{
    audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);
    dsp_mgr_session_disable(audio_path->dsp_session);
}

static void audio_path_anc_stop(T_AUDIO_PATH *audio_path)
{
    audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);

    if (!sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING))
    {
        sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_SET,
                            AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING);

        sys_event_flag_wait(audio_path_db->event_group,
                            anc_llapt_wait_disable_finish_cback,
                            (void *)audio_path,
                            AUDIO_PATH_EVENT_FLAG_ANC_LLAPT_DISABLING,
                            SYS_EVENT_FLAG_TYPE_CLEAR_AND);
        anc_mgr_disable();
    }
}

static void audio_path_voice_stop(T_AUDIO_PATH *audio_path)
{
    audio_path_start_fade_out(audio_path);
    audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);
}

static void audio_path_vad_stop(T_AUDIO_PATH *audio_path)
{
#if 0
    audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);
    dsp_mgr_vad_action(DSP_IPC_ACTION_STOP);
    dsp_mgr_session_disable(audio_path->dsp_session);
#endif
}

void audio_path_ready_stop_cback(void *handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;

    if (path->state == AUDIO_PATH_STATE_RUNNING)
    {
        audio_path_stop(handle);
    }
}

static void audio_path_tone_stop(T_AUDIO_PATH *path)
{
    dsp_mgr_composite_stop(path->dsp_session);
}

static void audio_path_vp_stop(T_AUDIO_PATH *path)
{
    if (path->stream_state != AUDIO_PATH_STREAM_STATE_END)
    {
        dsp_mgr_voice_prompt_stop(path->dsp_session);
        audio_path_state_set(path, AUDIO_PATH_STATE_STOPPING);
    }
}

static void audio_path_sidetone_stop(T_AUDIO_PATH *audio_path)
{
    audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);

    if (!sys_event_flag_check(audio_path_db->event_group, AUDIO_PATH_EVENT_FLAG_SIDETONE_DISABLING))
    {
        sys_event_flag_post(audio_path_db->event_group, SYS_EVENT_FLAG_OPT_SET,
                            AUDIO_PATH_EVENT_FLAG_SIDETONE_DISABLING);

        sys_event_flag_wait(audio_path_db->event_group,
                            sidetone_wait_disable_finish_cback,
                            (void *)audio_path,
                            AUDIO_PATH_EVENT_FLAG_SIDETONE_DISABLING,
                            SYS_EVENT_FLAG_TYPE_CLEAR_AND);
        codec_mgr_sidetone_set(0, 0, 0);
    }
}

bool audio_path_stop(T_AUDIO_PATH_HANDLE handle)
{
    T_AUDIO_PATH *audio_path;
    audio_path = (T_AUDIO_PATH *)handle;

    if (audio_path == NULL)
    {
        AUDIO_PRINT_ERROR1("audio_path_stop: handle %p, no such path", handle);
        return false;
    }

    AUDIO_PRINT_INFO2("audio_path_stop: handle %p, current status: 0x%02x", handle, audio_path->state);

    if (audio_path->state == AUDIO_PATH_STATE_IDLE)
    {
        // Upper layers may need the AUDIO_PATH_EVT_IDLE event when they call audio_path_stop
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_IDLE);
    }
    ///TODO: The path to stop should only concern its own state.
    /// Currently the path to stop should wait all paths in ready state
    else if (audio_path_db->ready_list.count != 0)
    {
        sys_event_flag_wait(audio_path_db->event_group,
                            audio_path_ready_stop_cback,
                            (void *)audio_path,
                            AUDIO_PATH_EVENT_FLAG_PATH_STARTING,
                            SYS_EVENT_FLAG_TYPE_CLEAR_AND);
    }
    else if (audio_path->state == AUDIO_PATH_STATE_PENDING)
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_IDLE);
    }
    else if (audio_path->state == AUDIO_PATH_STATE_SUSPEND)
    {
        audio_path_state_set(audio_path, AUDIO_PATH_STATE_STOPPING);
    }
    else if (audio_path->state == AUDIO_PATH_STATE_RUNNING)
    {
        path_stop_cb[audio_path->category](audio_path);
    }

    return true;
}

static int16_t audio_path_gain_scale(int16_t gain_db, float scale)
{
    float gain;

    if (fabs(scale) >= (0.375f / 128.0f))
    {
        gain  = pow(10.0f, gain_db / 128.0f / 20.0f);
        gain *= scale;
    }
    else
    {
        gain = pow(10.0f, (-128 * 128) / 128.0f / 20.0f);
    }

    return (int16_t)(20 * log10(gain) * 128);
}

bool audio_path_dac_level_set(T_AUDIO_PATH_HANDLE handle, uint8_t level, float scale)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;

    if (path->state == AUDIO_PATH_STATE_RUNNING)
    {
        T_AUDIO_ROUTE_DAC_GAIN route_gain;
        int16_t                left_gain;
        int16_t                right_gain;

        if (audio_route_dac_gain_get(path->category, level, &route_gain) == true)
        {
            if (scale == 0.0f)
            {
                left_gain  = route_gain.dac_gain;
                right_gain = route_gain.dac_gain;
            }
            else if (scale > 0.0f)
            {
                left_gain  = audio_path_gain_scale(route_gain.dac_gain, 1.0f - scale);
                right_gain = route_gain.dac_gain;
            }
            else
            {
                left_gain  = route_gain.dac_gain;
                right_gain = audio_path_gain_scale(route_gain.dac_gain, 1.0f + scale);
            }

            AUDIO_PRINT_TRACE6("audio_path_dac_level_set: category %u, level %u, scale %d/1000, dac_gain 0x%04x, left_gain 0x%04x, right_gain 0x%04x",
                               path->category, level, (int32_t)(scale * 1000), route_gain.dac_gain, left_gain, right_gain);

            audio_path_dac_gain_set(path->category, left_gain, right_gain);
        }

        codec_mgr_dac_gain_set(path->codec_session, level);
    }

    return true;
}

bool audio_path_dac_mute(T_AUDIO_PATH_HANDLE handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;

    if (path->state == AUDIO_PATH_STATE_RUNNING)
    {
        audio_path_dac_gain_set(path->category, -128 * 128, -128 * 128);
    }

    return true;
}

bool audio_path_adc_level_set(T_AUDIO_PATH_HANDLE handle, uint8_t level, float scale)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;

    if (path->state == AUDIO_PATH_STATE_RUNNING)
    {
        T_AUDIO_ROUTE_ADC_GAIN route_gain;
        int16_t               left_gain;
        int16_t               right_gain;

        if (audio_route_adc_gain_get(path->category, level, &route_gain) == true)
        {
            if (scale == 0.0f)
            {
                left_gain  = route_gain.adc_gain;
                right_gain = route_gain.adc_gain;
            }
            else if (scale > 0.0f)
            {
                left_gain  = audio_path_gain_scale(route_gain.adc_gain, 1.0f - scale);
                right_gain = route_gain.adc_gain;
            }
            else
            {
                left_gain  = route_gain.adc_gain;
                right_gain = audio_path_gain_scale(route_gain.adc_gain, 1.0f + scale);
            }

            AUDIO_PRINT_TRACE6("audio_path_adc_level_set: category %u, level %u, scale %d/1000, adc_gain 0x%04x, left_gain 0x%04x, right_gain 0x%04x",
                               path->category, level, (int32_t)(scale * 1000), route_gain.adc_gain, left_gain, right_gain);

            audio_path_adc_gain_set(path->category, left_gain, right_gain);
        }

        codec_mgr_adc_gain_set(path->codec_session, level);
    }

    return true;
}

bool audio_path_adc_mute(T_AUDIO_PATH_HANDLE handle)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;

    if (path->state == AUDIO_PATH_STATE_RUNNING)
    {
        audio_path_adc_gain_set(path->category, -128 * 128, -128 * 128);
        codec_mgr_adc_gain_mute(path->codec_session);
    }

    return true;
}

bool audio_path_sidetone_gain_set(T_AUDIO_PATH_HANDLE handle, int16_t gain)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;

    if (path->state == AUDIO_PATH_STATE_RUNNING)
    {
        codec_mgr_sidetone_gain_set(gain);
    }

    return true;
}

bool audio_path_power_off(void)
{
    T_CODEC_MGR_STATE codec_state;
    T_DSP_STATE dsp_state;

    codec_state = codec_mgr_get_state();
    dsp_state = dsp_mgr_get_state();

    AUDIO_PRINT_TRACE2("audio_path_power_off: codec_state %d, dsp_state %d", codec_state, dsp_state);

    if (dsp_state == DSP_STATE_OFF)
    {
        return true;
    }

    if (codec_state == CODEC_MGR_STATE_MUTED)
    {
        audio_mgr_codec_timer_cback(audio_path_db->codec_timer);
    }

    if (dsp_mgr_get_state() == DSP_STATE_IDLE && codec_state == CODEC_MGR_STATE_POWER_OFF)
    {
        audio_mgr_dsp_timer_cback(audio_path_db->dsp_timer);
    }

    audio_path_db->wait_power_off = true;

    return true;
}

bool audio_path_power_on(void)
{
    if (dsp_mgr_power_on_check() == false)
    {
        dsp_mgr_power_on();
    }

    if (audio_path_db->lps_en == true)
    {
        //set timer to power down DSP
        sys_timer_start(audio_path_db->dsp_timer);
    }

    return true;
}

bool audio_path_data_send(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_DATA_TYPE type, uint8_t seq,
                          void *p_data, uint32_t len, bool flush)
{
    T_AUDIO_PATH *audio_path;

    audio_path = (T_AUDIO_PATH *)handle;

    if (audio_path == NULL)
    {
        return false;
    }

    if (audio_path->state != AUDIO_PATH_STATE_RUNNING)
    {
        AUDIO_PRINT_ERROR2("audio_path_data_send: handle %p, invalid state %u",
                           handle, audio_path->state);
        return false;
    }

    if ((audio_path->category == AUDIO_CATEGORY_AUDIO) ||
        (audio_path->category == AUDIO_CATEGORY_VOICE))
    {
        T_H2D_STREAM_HEADER2 *hdr = (T_H2D_STREAM_HEADER2 *)p_data;

        hdr->sync_word = 0x3F3F3F3F;
        hdr->tail = 0xffffffff;

        if (h2d_data_send((uint8_t)type, p_data, len, flush, seq) == false)
        {
            return false;
        }

        audio_path->stream_state = AUDIO_PATH_STREAM_STATE_CONTINUE;
    }
    else if (audio_path->category == AUDIO_CATEGORY_TONE)
    {
        dsp_mgr_composite_start(p_data, len);
        audio_path->stream_state = AUDIO_PATH_STREAM_STATE_FIRST;
    }
    else if (audio_path->category == AUDIO_CATEGORY_VP)
    {
        if (audio_path->stream_state == AUDIO_PATH_STREAM_STATE_IDLE)
        {
            uint32_t cfg = ((uint32_t *)p_data)[0];
            uint32_t cfg_bt_clk_mix = ((uint32_t *)p_data)[1];
            uint32_t clk_ref = ((uint32_t *)p_data)[2];

            audio_path_timestamp_set(handle, clk_ref, cfg_bt_clk_mix, true);

            AUDIO_PRINT_TRACE2("audio_path_data_send: config_voice_prompt cfg 0x%08x,cfg_bt_clk_mix 0x%08x",
                               cfg, cfg_bt_clk_mix);

            dsp_mgr_voice_prompt_start(cfg, cfg_bt_clk_mix);

            audio_path->stream_state = AUDIO_PATH_STREAM_STATE_FIRST;
        }
        else if ((audio_path->stream_state == AUDIO_PATH_STREAM_STATE_FIRST) ||
                 (audio_path->stream_state == AUDIO_PATH_STREAM_STATE_CONTINUE))
        {
            uint8_t *p_frame = (uint8_t *)p_data;
            if ((len == 2) && (p_frame[0] == 0xFF) && (p_frame[1] == 0xFF))
            {
                AUDIO_PRINT_TRACE1("audio_path_data_send: handle %p, voice prompt end!", handle);

                dsp_mgr_voice_prompt_stop(audio_path->dsp_session);
                audio_path->stream_state = AUDIO_PATH_STREAM_STATE_END;
            }
            else
            {
                dsp_mgr_voice_prompt_send(p_data, len);
                audio_path->stream_state = AUDIO_PATH_STREAM_STATE_CONTINUE;
            }
        }
    }

    return true;
}

uint16_t audio_path_data_recv(T_AUDIO_PATH_HANDLE handle,
                              uint8_t *buf,
                              uint16_t len)
{
    T_AUDIO_PATH *audio_path;

    audio_path = (T_AUDIO_PATH *)handle;
    if (audio_path != NULL)
    {
        return d2h_data_recv(NULL, buf, len);
    }

    return 0;
}

uint16_t audio_path_data_peek(T_AUDIO_PATH_HANDLE handle)
{
    T_AUDIO_PATH *audio_path;

    audio_path = (T_AUDIO_PATH *)handle;
    if (audio_path != NULL)
    {
        return d2h_data_length_peek();
    }

    return 0;
}

bool audio_path_timestamp_set(T_AUDIO_PATH_HANDLE handle, uint8_t clk_ref, uint32_t timestamp,
                              bool sync_flag)
{
    T_AUDIO_PATH *audio_path;

    audio_path = (T_AUDIO_PATH *)handle;

    dsp_mgr_signal_proc_start(audio_path->dsp_session, timestamp, clk_ref, sync_flag);

    return true;
}

bool audio_path_synchronization_role_swap(T_AUDIO_PATH_HANDLE handle, uint8_t role, bool start)
{
    return dsp_ipc_set_handover_info(role, start);
}

bool audio_path_synchronization_data_send(T_AUDIO_PATH_HANDLE handle, uint8_t *buf, uint16_t len)
{
    return dsp_ipc_synchronization_data_send(buf, len);
}

void audio_path_b2bmsg_interaction_timeout(void)
{
    dsp_ipc_set_b2bmsg_interaction_timeout(0);
}

bool audio_path_synchronization_join_set(T_AUDIO_PATH_HANDLE handle, uint8_t role)
{
    T_AUDIO_PATH *audio_path;

    audio_path = (T_AUDIO_PATH *)handle;

    if (role == 1)// Pri for join
    {
        dsp_rws_set_role(RWS_ROLE_SRC, audio_path->category);
        dsp_rws_seamless(RWS_RESYNC_V2_MASTER);
    }
    else if (role == 2)// Sec for join
    {
        dsp_rws_seamless(RWS_RESYNC_V2_SLAVE);
    }
    else
    {
        dsp_rws_set_role(RWS_ROLE_NONE, audio_path->category);
        dsp_rws_seamless(RWS_RESYNC_V2_OFF);
    }

    return true;
}

void audio_path_synchronization_join_stop(void)
{
    dsp_rws_seamless(RWS_RESYNC_V2_OFF);
}

bool audio_path_is_running(T_AUDIO_PATH_HANDLE handle)
{
    T_AUDIO_PATH *audio_path;

    audio_path = (T_AUDIO_PATH *)handle;

    if (audio_path != NULL)
    {
        if (audio_path->state == AUDIO_PATH_STATE_RUNNING)
        {
            return true;
        }
    }

    return false;
}

void audio_path_low_latency_set(bool enable_plc, bool upgrade_clk)
{
    uint8_t low_latency_param = 0;

    if (enable_plc)
    {
        low_latency_param |= 1 << 4;
    }

    if (upgrade_clk)
    {
        low_latency_param |= 1;
    }

    dsp_ipc_set_low_latency_mode(low_latency_param);
}

void audio_path_latency_rpt_set(bool enable, uint8_t num)
{
    dsp_ipc_set_latency_report(num);
}

void audio_path_plc_notify_set(uint16_t interval, uint32_t threshold, bool enable)
{
    dsp_ipc_set_plc_notify(interval, threshold, enable);
}

bool audio_path_sw_sidetone_enable(int16_t gain, uint8_t level)
{
    return dsp_ipc_sidetone_set(1, gain, level);
}

bool audio_path_sw_sidetone_disable(void)
{
    return dsp_ipc_sidetone_set(0, 0, 0);
}

bool audio_path_hw_sidetone_enable(int16_t gain, uint8_t level)
{
    return codec_mgr_sidetone_set(gain, level, 1);
}

bool audio_path_hw_sidetone_disable(void)
{
    return codec_mgr_sidetone_set(0, 0, 0);
}

bool audio_path_cback_register(P_SYS_IPC_CBACK cback)
{
    audio_path_db->path_event = sys_ipc_subscribe("path_mgr", cback);

    return true;
}

void audio_path_cback_unregister(void)
{
    sys_ipc_unsubscribe(audio_path_db->path_event);
}

bool audio_path_cfg_set(T_AUDIO_PATH_HANDLE handle, void *cfg)
{
    T_AUDIO_PATH *path;

    path = (T_AUDIO_PATH *)handle;

    switch (path->category)
    {
    case AUDIO_CATEGORY_ANC:
    case AUDIO_CATEGORY_LLAPT:
        {
            T_ANC_LLAPT_CFG *config;

            config = (T_ANC_LLAPT_CFG *)cfg;

            anc_mgr_load_cfg_set(config->sub_type, config->scenario_id);
        }
        break;

    default:
        break;
    }

    return true;
}

void audio_path_lpm_set(bool enable)
{
    if (enable)
    {
        sys_timer_start(audio_path_db->codec_timer);
        sys_timer_start(audio_path_db->dsp_timer);
    }
    else
    {
        sys_timer_stop(audio_path_db->codec_timer);
        sys_timer_stop(audio_path_db->dsp_timer);
    }

    audio_path_db->lps_en = enable;
}

void audio_path_brightness_set(T_AUDIO_PATH_HANDLE handle, float strength)
{
    T_AUDIO_PATH *audio_path;

    audio_path = (T_AUDIO_PATH *)handle;
    if (audio_path)
    {
        if (audio_path->state == AUDIO_PATH_STATE_RUNNING)
        {
            anc_mgr_eq_set(strength);
        }
    }
}

void audio_path_msg_send(T_AUDIO_PATH_MSG *msg)
{
    uint8_t evt;

    evt = EVENT_AUDIO_PATH_MSG;
    if (os_msg_send(hAudioPathMsgQueueHandle, msg, 0) == true)
    {
        os_msg_send(hEventQueueHandleAu, &evt, 0);
    }
}

bool audio_path_signal_out_monitoring_set(T_AUDIO_PATH_HANDLE handle, bool enable,
                                          uint16_t refresh_interval)
{
    T_AUDIO_PATH *audio_path;

    audio_path = (T_AUDIO_PATH *)handle;
    if (audio_path)
    {
        if (audio_path->state == AUDIO_PATH_STATE_RUNNING)
        {
            return dsp_ipc_signal_out_monitoring_set(enable, refresh_interval);
        }
    }
    return false;
}

bool audio_path_signal_in_monitoring_set(T_AUDIO_PATH_HANDLE handle, bool enable,
                                         uint16_t refresh_interval)
{
    T_AUDIO_PATH *audio_path;

    audio_path = (T_AUDIO_PATH *)handle;
    if (audio_path)
    {
        if (audio_path->state == AUDIO_PATH_STATE_RUNNING)
        {
            return dsp_ipc_signal_in_monitoring_set(enable, refresh_interval);
        }
    }

    return false;
}
