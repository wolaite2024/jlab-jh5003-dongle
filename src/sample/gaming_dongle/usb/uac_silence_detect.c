/**
*****************************************************************************************
*     Copyright (C) 2021 Realtek Semiconductor Corporation.
*****************************************************************************************
  * @file
  * @brief
  * @details
  * @author
  * @date
  * @version
  ***************************************************************************************
  * @attention
  ***************************************************************************************
  */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/


/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include <string.h>
#include "trace.h"
#include "uac_silence_detect.h"
#include "app_timer.h"
#include "app_io_msg.h"
#include "section.h"
#include "le_media_player.h"
#include "mcp_def.h"
#include "app_src_policy.h"
#include "app_usb_uac.h"
#if DONGLE_LE_AUDIO
#include "app_le_audio.h"
#endif
#if DONGLE_AUDIO_AUTO_TRANSFER
#include "dongle_media.h"
#endif

#if (UAC_SILENCE_DETECT_SUPPORT == 1)
/*============================================================================*
 *                         Macros
 *============================================================================*/
#define SILENCE_CONTINUE_TIMEOUT        (3000)
#define SILENCE_WAIT_TIMEOUT            (1000)
#define STREAM_SUSPEND_TIMEOUT          (1000)

#define ABSTRACT(x)         (((x) > 0) ? (x) : ((~(x)) + 1))
#define MIN(a,b)            ((a) <= (b) ? (a) : (b))
/*============================================================================*
 *                         Types
 *============================================================================*/
typedef struct
{
    bool                silence_state;
    bool                hid_paused;
    bool                stream_suspend_enable;
    uint32_t            uac_silence_cnt;
    uint32_t            uac_unsilence_cnt;
    uint32_t            silence_threshold;
    uint8_t             silence_timer_id;
    uint8_t             silence_continue_timer_idx;
    uint8_t             silence_wait_timer_idx;
    uint8_t             stream_suspend_timer_idx;
#if (ENABLE_UAC2 == 1)
    bool                uac2_silence_state;
    uint32_t            uac2_silence_cnt;
    uint32_t            uac2_unsilence_cnt;
    uint32_t            uac2_silence_threshold;
    uint8_t             uac2_silence_continue_timer_idx;
    bool                uac1_silence_timeout;
    bool                uac2_silence_timeout;
#endif
} T_SILENCE_DETECT;

typedef enum
{
    DETECT_TIMER_ID_SILENCE_CONTINUE        = 0x01,         //
    DETECT_TIMER_ID_SILENCE_WAIT            = 0x02,
    DETECT_TIMER_ID_STREAM_SUSPEND          = 0x03,
#if (ENABLE_UAC2 == 1)
    UAC2_DETECT_TIMER_ID_SILENCE_CONTINUE   = 0x04,
#endif
} T_DETECT_TIMER_ID;

/*============================================================================*
 *                         global variable
 *============================================================================*/

T_SILENCE_DETECT g_silence;

static void silence_continue_timer_start(void);
static void silence_wait_timer_start(void);
static void silence_continue_timer_stop(void);
static void silence_wait_timer_stop(void);
static void silence_timer_cback(uint8_t timer_id, uint16_t timer_chann);
/*============================================================================*
 *                         Functions
 *============================================================================*/

bool uac_stream_is_stream_suspend(void)
{
    return g_silence.stream_suspend_enable;
}

void slience_set_stream_suspend(bool flag)
{
    APP_PRINT_INFO1("slience_set_stream_suspend flag %x", flag);
    g_silence.stream_suspend_enable = flag;
    if (flag)
    {
        app_start_timer(&g_silence.stream_suspend_timer_idx,
                        "stream_suspend",
                        g_silence.silence_timer_id,
                        DETECT_TIMER_ID_STREAM_SUSPEND,
                        0,
                        false,
                        STREAM_SUSPEND_TIMEOUT);
    }
    else
    {
        app_stop_timer(&g_silence.stream_suspend_timer_idx);
    }
}

static void send_media_state_by_bt(bool enable)
{
#if DONGLE_LE_AUDIO
    if (enable)
    {
        le_media_player_set_state(MCS_MEDIA_STATE_PLAYING);
    }
    else
    {
        le_media_player_set_state(MCS_MEDIA_STATE_PAUSED);
    }
#endif
}

#ifdef ENABLE_UAC2
#if (UAC_SILENCE_PROCESS == 1)
static void silence_continue_timeout_process(uint8_t uac_index)
{
    if (uac_index == 0)
    {
        if (!get_usb_uac2_state() || g_silence.uac2_silence_timeout)
        {
            all_src_a2dp_stream_stop();
        }
    }
    else
    {
        if ((!get_usb_uac1_state()) || g_silence.uac1_silence_timeout)
        {
            all_src_a2dp_stream_stop();
        }
    }
}
#endif

static void uac2_silence_continue_timer_start(void)
{
    app_start_timer(&g_silence.uac2_silence_continue_timer_idx, "silence_continue",
                    g_silence.silence_timer_id, UAC2_DETECT_TIMER_ID_SILENCE_CONTINUE, 0,
                    false,
                    SILENCE_CONTINUE_TIMEOUT);
}

static void uac2_silence_continue_timer_stop(void)
{
    app_stop_timer(&g_silence.uac2_silence_continue_timer_idx);
}

static void uac2_silence_continue_timer_timeout(void)
{
    APP_PRINT_INFO0("uac2_silence_continue_timer_timeout");
#if (UAC_SILENCE_PROCESS == 1)
    g_silence.uac2_silence_timeout = true;
    silence_continue_timeout_process(1);
#endif
}

bool uac2_get_silence_state(void)
{
    return g_silence.uac2_silence_state;
}

#endif

static void silence_continue_timer_timeout(void)
{
    g_silence.hid_paused = false;
    APP_PRINT_INFO0("silence_continue_timer_timeout");

#if (UAC_SILENCE_PROCESS == 1)
#if (LEGACY_BT_GAMING == 1)
#ifdef ENABLE_UAC2
    g_silence.uac1_silence_timeout = true;
    silence_continue_timeout_process(0);
#else
    all_src_a2dp_stream_stop();
#endif
#endif
#if LE_AUDIO_MCS_SERV_SUPPORT
    APP_PRINT_INFO0("le_media_player_set_state_stop 1");
    le_media_player_set_state_stop();
#endif
#if DONGLE_LE_AUDIO
    app_le_audio_set_downstream_active(false);
#endif
#if DONGLE_AUDIO_AUTO_TRANSFER
    dongle_media_set_state_stop();
#endif
#endif
}

static void silence_wait_timer_timeout(void)
{
    g_silence.hid_paused = false;
    uint8_t state = uac_get_silence_state();
    APP_PRINT_INFO1("silence_wait_timer_timeout silence state %x", state);
    /* if state is false, no need send stop, cause stop sended when send hid msg */
    if (state)
    {
        send_media_state_by_bt(true);
    }
}

static void silence_continue_timer_start(void)
{
    app_start_timer(&g_silence.silence_continue_timer_idx, "silence_continue",
                    g_silence.silence_timer_id, DETECT_TIMER_ID_SILENCE_CONTINUE, 0,
                    false,
                    SILENCE_CONTINUE_TIMEOUT);
}

static void silence_wait_timer_start(void)
{
    app_start_timer(&g_silence.silence_wait_timer_idx, "silence_wait",
                    g_silence.silence_timer_id, DETECT_TIMER_ID_SILENCE_WAIT, 0,
                    false,
                    SILENCE_WAIT_TIMEOUT);
}

static void silence_continue_timer_stop(void)
{
    app_stop_timer(&g_silence.silence_continue_timer_idx);
}

static void silence_wait_timer_stop(void)
{
    app_stop_timer(&g_silence.silence_wait_timer_idx);
}

static void silence_timer_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_INFO2("silence_timer_cback: timer_id %d, timer_channel %d",
                    timer_id, timer_chann);
    switch (timer_id)
    {
    case DETECT_TIMER_ID_SILENCE_CONTINUE:
        {
            silence_continue_timer_stop();
            silence_continue_timer_timeout();
        }
        break;
    case DETECT_TIMER_ID_SILENCE_WAIT:
        {
            silence_wait_timer_stop();
            silence_wait_timer_timeout();
        }
        break;
    case DETECT_TIMER_ID_STREAM_SUSPEND:
        {
            app_stop_timer(&g_silence.stream_suspend_timer_idx);
            g_silence.stream_suspend_enable = false;
        }
        break;
#ifdef ENABLE_UAC2
    case UAC2_DETECT_TIMER_ID_SILENCE_CONTINUE:
        {
            uac2_silence_continue_timer_stop();
            uac2_silence_continue_timer_timeout();
        }
        break;
#endif
    default:
        break;
    }
}



void uac_set_silence_threshold(uint32_t value)
{
    g_silence.silence_threshold = value;
}

bool uac_get_silence_state(void)
{
    return g_silence.silence_state;
}

RAM_TEXT_SECTION
static uint32_t app_calc_average_after_abs(uint8_t *data, uint16_t length)
{
    if ((!data) || (!length))
    {
        return 0;
    }

    uint32_t ret = 0;
    uint16_t len = 0;
    int16_t pcm_data = 0;

    len = MIN(192, length) >> 1;

    if (len == 0)
    {
        return 0;
    }

    for (uint8_t i = 0; i < len; i++)
    {
        pcm_data = (int16_t)(data[2 * i + 1] << 8 | data[2 * i]);
        ret += ABSTRACT(pcm_data);
    }
    ret /= len;
    return ret;
}

RAM_TEXT_SECTION
void uac_silence_detect_proc(uint8_t *data, uint16_t length)
{
    uint32_t pcm_abs;
    T_IO_MSG io_msg;
    pcm_abs = app_calc_average_after_abs(data, length);

    if (pcm_abs > g_silence.silence_threshold)
    {
        g_silence.uac_unsilence_cnt ++;
        if (g_silence.silence_state == UAC_SILENCE_STATE_MUTE)
        {
            g_silence.silence_state = UAC_SILENCE_STATE_UNMUTE;
            io_msg.type = IO_MSG_TYPE_DONGLE_APP;
            io_msg.u.param = g_silence.silence_state;
            io_msg.subtype = SILENCE_DETECT_EVENT;
            if (app_io_msg_send(&io_msg) == false)
            {
                APP_PRINT_ERROR0("app_usb_uac_cback_msg_ds_data_trans: app msg send silent detect start fail");
                return;
            }
            APP_PRINT_INFO2("uac_silence_detect_proc send state %x, slience_cnt %x",
                            g_silence.silence_state, g_silence.uac_silence_cnt);
            g_silence.uac_silence_cnt = 0;
#if (ENABLE_UAC2 == 1)
            g_silence.uac1_silence_timeout = false;
#endif
        }
    }
    else
    {
        g_silence.uac_silence_cnt ++;
        if (g_silence.silence_state == UAC_SILENCE_STATE_UNMUTE)
        {
            g_silence.silence_state = UAC_SILENCE_STATE_MUTE;
            io_msg.type = IO_MSG_TYPE_DONGLE_APP;
            io_msg.u.param = g_silence.silence_state;
            io_msg.subtype = SILENCE_DETECT_EVENT;
            if (app_io_msg_send(&io_msg) == false)
            {
                APP_PRINT_ERROR0("app_usb_uac_cback_msg_ds_data_trans: app msg send silent detect start fail");
                return;
            }
            APP_PRINT_INFO2("uac_silence_detect_proc send state %x, unslience_cnt %x",
                            g_silence.silence_state, g_silence.uac_unsilence_cnt);
            g_silence.uac_unsilence_cnt = 0;
        }
    }
}


void app_handle_silence_detect_event(T_IO_MSG *io_msg)
{
    bool silence_state = io_msg->u.param;

    APP_PRINT_INFO2("handle_silence_detect hid_state %x silence_state %x", g_silence.hid_paused,
                    silence_state);
    //When the PC clicks on the player to pause, mute data will also be sent.
#if 0
    if (!g_silence.hid_paused)
    {
        /*FIXME: what to do if media play mute data*/
        if (silence_state == UAC_SILENCE_STATE_MUTE)
        {
            silence_continue_timer_stop();
        }
        return;
    }
#endif

    if (silence_state == UAC_SILENCE_STATE_MUTE)
    {
        silence_continue_timer_start();
    }
    else
    {
        silence_continue_timer_stop();
    }
}


void silence_detect_set_hid_state(bool hid_state)
{
    g_silence.hid_paused = hid_state;

    APP_PRINT_INFO2("silence_detect_set_hid_state state %x silence_state %x", hid_state,
                    g_silence.silence_state);
    silence_continue_timer_stop();
    silence_wait_timer_stop();

    if (g_silence.hid_paused)
    {
        silence_wait_timer_start();
        if (g_silence.silence_state == UAC_SILENCE_STATE_MUTE)
        {
            silence_continue_timer_start();
        }
    }
}

#ifdef ENABLE_UAC2
RAM_TEXT_SECTION
void uac2_silence_detect_proc(uint8_t *data, uint16_t length)
{
    uint32_t pcm_abs;
    T_IO_MSG io_msg;
    pcm_abs = app_calc_average_after_abs(data, length);

    if (pcm_abs > g_silence.uac2_silence_threshold)
    {
        g_silence.uac2_unsilence_cnt ++;
        if (g_silence.uac2_silence_state == UAC_SILENCE_STATE_MUTE)
        {
            g_silence.uac2_silence_state = UAC_SILENCE_STATE_UNMUTE;
            io_msg.type = IO_MSG_TYPE_DONGLE_APP;
            io_msg.u.param = g_silence.uac2_silence_state;
            io_msg.subtype = UAC2_SILENCE_DETECT_EVENT;
            if (app_io_msg_send(&io_msg) == false)
            {
                APP_PRINT_ERROR0("app_usb_uac_cback_msg_ds_data_trans: app msg send silent detect start fail");
                return;
            }
            APP_PRINT_INFO2("uac2_silence_detect_proc send state %x, slience_cnt %x",
                            g_silence.uac2_silence_state, g_silence.uac2_silence_cnt);
            g_silence.uac2_silence_cnt = 0;
            g_silence.uac2_silence_timeout = false;
        }
    }
    else
    {
        g_silence.uac2_silence_cnt ++;
        if (g_silence.uac2_silence_state == UAC_SILENCE_STATE_UNMUTE)
        {
            g_silence.uac2_silence_state = UAC_SILENCE_STATE_MUTE;
            io_msg.type = IO_MSG_TYPE_DONGLE_APP;
            io_msg.u.param = g_silence.uac2_silence_state;
            io_msg.subtype = UAC2_SILENCE_DETECT_EVENT;
            if (app_io_msg_send(&io_msg) == false)
            {
                APP_PRINT_ERROR0("app_usb_uac_cback_msg_ds_data_trans: app msg send silent detect start fail");
                return;
            }
            APP_PRINT_INFO2("uac2_silence_detect_proc send state %x, unslience_cnt %x",
                            g_silence.uac2_silence_state, g_silence.uac2_silence_cnt);
            g_silence.uac2_unsilence_cnt = 0;
        }
    }
}

void app_handle_uac2_silence_detect_event(T_IO_MSG *io_msg)
{
    bool uac2_silence_state = io_msg->u.param;
    APP_PRINT_INFO1("handle_silence_detect: silence_state %x", uac2_silence_state);

    if (uac2_silence_state == UAC_SILENCE_STATE_MUTE)
    {
        uac2_silence_continue_timer_start();
    }
    else
    {
        uac2_silence_continue_timer_stop();
    }
}
#endif

void app_silence_detect_init(void)
{
    memset(&g_silence, 0, sizeof(T_SILENCE_DETECT));
    app_timer_reg_cb(silence_timer_cback, &g_silence.silence_timer_id);
}
#endif
