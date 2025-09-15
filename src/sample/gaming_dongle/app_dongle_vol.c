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
#include <stdlib.h>
#include "trace.h"
#include "app_dongle_vol.h"
#include "app_usb_hid.h"
#include "app_timer.h"
#ifdef LEGACY_BT_GAMING
#include "gaming_bt.h"
#endif
#ifdef LEGACY_BT_GENERAL
#include "general_bt.h"
#endif
#include "app_main.h"
#if (DONGLE_LE_AUDIO == 1)
#include "app_le_audio.h"
#endif
#include "app_usb_layer.h"
#include "app_usb_hid_report.h"
#include "usb_host_detect.h"
/*============================================================================*
 *                         Macros
 *============================================================================*/
#define APP_DONGLE_VOL_DEBUG            1

//#define HID_SEND_INTERVAL_ENABLE


#define     U2A_SET_VOL_TIMEOUT         (10)
#define     A2U_SET_VOL_TIMEOUT         (500)
#ifdef HID_SEND_INTERVAL_ENABLE
#define     HID_SEND_TIMEOUT            (200)
#endif
#define     HID_VALID_CHECK_TIMEOUT     (30)
#define     A2U_PROC_INTER_TIMEOUT      (100)
#define     INIT_U2A_VOL_TIMEROUT       (1000)
#define     HID_VALID_CHECK_BEGIN_TIMEOUT (100)
#define     UA2_VOL_FEEDBACK_CHECK_TIMEOUT (500)
#define     HID_RELEASE_CMD_BEGIN_TIMEOUT (30)

#define     HID_VOL_INCREMENT_CMD       (0x01)
#define     HID_VOL_DECREMENT_CMD       (0x02)
#define     HID_VOL_MUTE_UNMUTE_CMD     (0x03)
#define     PC_VOL_MIN                  (0x00)
#define     PC_VOL_MAX                  (0x7f)
#define     PC_VOL_MUTE                 (0x01)
#define     PC_VOL_UNMUTE               (0x00)
#define     AVRCP_VOL_MIN               (0x00)
#define     AVRCP_VOL_MAX               (0x7F)
#define     USB_UAC_CUR_VOL             (0x7F)
/*============================================================================*
 *                         Types
 *============================================================================*/
typedef struct
{
    uint8_t busy;
    uint8_t len;
    uint8_t cur_cmd;
    uint8_t hid_cmd[3];
} hid_queue_t;

typedef struct
{
    uint8_t host_state;
    uint8_t host_type;
    uint8_t host_vol;                   //0x0-0x7F
    uint8_t host_mute;                  // 0:unmute, 1:mute
    uint8_t host_check_begin_flag;      // 0: host_check_beging_timer not start, 1: start
    uint8_t a2u_pending_flag;           //
    uint8_t a2u_pending_vol;
    uint8_t u2a_vol_adjusting;          // 1: uac is setting arvcp vol, 0: not
    uint8_t a2u_adjusting;              // 1: avrcp is setting uac vol, 0: not
    uint8_t a2u_target_vol;             // avrcp set uac vol target, when host is pc,may adjust serval times
    uint8_t uac_cur_vol;
    uint8_t incre_when_max_flag;
    uint8_t decre_when_min_flag;
    //uint8_t uac_current_vol;          // when host is pc, avrcp set host vol, host will feedback current abs vol
    bool already_receive_pc_vol;
} T_DONGLE_VOL;

/*============================================================================*
 *                         global variable
 *============================================================================*/
static uint8_t          dongle_vol_timer_id = 0;
static T_DONGLE_VOL     g_dongle_vol;
static hid_queue_t      hid_send_queue;
bool                    first_vol_changed = 0;
bool                    iphone_flag = 0;

static uint8_t  timer_idx_a2u_set_vol = 0;

/* u2a set vol timer for avrcp set vol qos, which may affect a2dp */
static uint8_t  timer_idx_u2a_set_vol = 0;

#ifdef HID_SEND_INTERVAL_ENABLE
/* hid send timer for hid cmd qos */
static uint8_t  timer_idx_hid_send = 0;
#endif
/* check usb hid send is complete to get host type PS */
static uint8_t  timer_idx_hid_valid_check = 0;

static uint8_t  timer_idx_a2u_proc_inter = 0;

static uint8_t  timer_idx_init_u2a_vol_wait = 0;

static uint8_t  timer_idx_hid_valid_check_begin = 0;

static uint8_t  timer_idx_u2a_vol_feedback_check = 0;

static uint8_t  timer_idx_hid_release_cmd_begin = 0;

/*============================================================================*
 *                         Functions
 *============================================================================*/

static bool hid_interrupt_in_queue(uint8_t *data, uint8_t len);

static bool hid_queue_in(uint8_t *data, uint8_t len)
{
    if ((!data) || (len != 3))
    {
        return false;
    }

    if (hid_send_queue.len)
    {
        APP_PRINT_ERROR2("hid_queue_in update %d %d", data[1], hid_send_queue.hid_cmd[1]);
    }
    memcpy(hid_send_queue.hid_cmd, data, len);
    hid_send_queue.len = 1;
    return true;
}


void a2u_set_vol_timer_start(void)
{
    app_start_timer(&timer_idx_a2u_set_vol, "a2u_set_vol",
                    dongle_vol_timer_id, VOL_TIMER_ID_A2U_SET_VOL, 0, false,
                    A2U_SET_VOL_TIMEOUT);
}

void u2a_set_vol_timer_start(void)
{
    app_start_timer(&timer_idx_u2a_set_vol, "u2a_set_vol",
                    dongle_vol_timer_id, VOL_TIMER_ID_U2A_SET_VOL, 0, false,
                    U2A_SET_VOL_TIMEOUT);
}

#ifdef HID_SEND_INTERVAL_ENABLE
void hid_send_timer_start(void)
{
    app_start_timer(&timer_idx_hid_send, "hid_send",
                    dongle_vol_timer_id, VOL_TIMER_ID_HID_SEND, 0, false,
                    HID_SEND_TIMEOUT);
}
#endif

void hid_valid_check_timer_start(void)
{
    app_start_timer(&timer_idx_hid_valid_check, "hid_valid_check",
                    dongle_vol_timer_id, VOL_TIMER_ID_HID_VALID_CHECK, 0, false,
                    HID_VALID_CHECK_TIMEOUT);
}

void a2u_proc_timer_start(void)
{
    app_start_timer(&timer_idx_a2u_proc_inter, "a2u_proc_inter",
                    dongle_vol_timer_id, VOL_TIMER_ID_A2U_PROC_INTERVAL, 0, false,
                    A2U_PROC_INTER_TIMEOUT);
}

void init_u2a_vol_wait_timer_start(void)
{
    APP_PRINT_INFO0("init_u2a_vol_wait_timer_start");
    app_start_timer(&timer_idx_init_u2a_vol_wait, "init_u2a_vol",
                    dongle_vol_timer_id, VOL_TIMER_ID_INIT_U2A_VOL, 0, false,
                    INIT_U2A_VOL_TIMEROUT);
}

void hid_valid_check_begin_timer_start(void)
{
    app_start_timer(&timer_idx_hid_valid_check_begin, "hid_valid_check_begin",
                    dongle_vol_timer_id, VOL_TIMER_ID_HID_VALID_CHECK_BEGIN, 0, false,
                    HID_VALID_CHECK_BEGIN_TIMEOUT);
}

void u2a_vol_feedback_check_timer_start(void)
{
    app_start_timer(&timer_idx_u2a_vol_feedback_check, "u2a_vol_feedback_check",
                    dongle_vol_timer_id, VOL_TIMER_ID_U2A_FEEDBACK_CHECK, 0, false,
                    UA2_VOL_FEEDBACK_CHECK_TIMEOUT);
}

void hid_sed_key_release_cmd_timer_start(void)
{
    app_start_timer(&timer_idx_hid_release_cmd_begin, "hid_release_cmd_begin",
                    dongle_vol_timer_id, VOL_TIMER_ID_HID_RELEASE_CMD, 0, false,
                    HID_RELEASE_CMD_BEGIN_TIMEOUT);
}

static bool hid_queue_out(void)
{
    if (hid_send_queue.busy)
    {
        return false;
    }
    if (hid_send_queue.len == 0)
    {
        return false;
    }

    hid_send_queue.busy = true;
    if (g_dongle_vol.a2u_adjusting == HID_VOL_MUTE_UNMUTE_CMD)
    {
        hid_send_queue.cur_cmd = hid_send_queue.hid_cmd[2];
    }
    else
    {
        hid_send_queue.cur_cmd = hid_send_queue.hid_cmd[1];
    }
    usb_hid_report_buffered_send(hid_send_queue.hid_cmd, 3);
    hid_send_queue.len = 0;

    /* send interval?? */
#ifdef HID_SEND_INTERVAL_ENABLE
    hid_send_timer_start();
#endif

    /* hid valid check. Sometimes the event USB_HID_MSG_TYPE_HID_IN_COMPLETE cannot be
       received during the adjustment process.This will cause the host type to change.*/
    if (g_dongle_vol.host_state != DONGLE_HOST_STATE_CHECKED)
    {
        hid_valid_check_timer_start();
    }
    return true;
}


bool hid_interrupt_in_queue(uint8_t *data, uint8_t len)
{
    hid_queue_in(data, len);

    return hid_queue_out();
}


void app_dongle_hid_send_cmpl(void)
{
    APP_PRINT_INFO1("app_dongle_hid_send_cmpl cur_cmd %x", hid_send_queue.cur_cmd);
    uint8_t data[3] =  {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};

    if (g_dongle_vol.host_state == DONGLE_HOST_STATE_CHECKING)
    {
        if (g_dongle_vol.host_type <= DONGLE_HOST_HID_INVALID)
        {
            g_dongle_vol.host_type = DONGLE_HOST_HID_NO_FEEDBACK;
            g_dongle_vol.host_state = DONGLE_HOST_STATE_CHECKED;
        }
    }

    /* stop timer */
    app_stop_timer(&timer_idx_hid_valid_check);

    if (g_dongle_vol.host_type != DONGLE_HOST_HID_NO_FEEDBACK)
    {
#ifndef HID_SEND_INTERVAL_ENABLE
        hid_send_queue.busy = false;
#endif
        /* send key release cmd */
        if (hid_send_queue.cur_cmd)
        {
            hid_send_queue.cur_cmd = 0;
            usb_hid_report_buffered_send(data, sizeof(data));
        }
#ifndef HID_SEND_INTERVAL_ENABLE
        else
        {
            hid_queue_out();
        }
#endif
    }
    else
    {
        hid_sed_key_release_cmd_timer_start();
    }
}


static void a2u_set_vol_feeback(uint32_t vol)
{
    APP_PRINT_INFO3("a2u_set_vol_feeback current %x target %x incre-decre %d", vol,
                    g_dongle_vol.a2u_target_vol, g_dongle_vol.a2u_adjusting);

    g_dongle_vol.host_vol = vol;
    /* os vol usually cannot turn exactly same as target vol*/
    uint8_t data[3] =  {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};
    if (g_dongle_vol.a2u_adjusting == HID_VOL_DECREMENT_CMD) //turn down
    {
        //os vol already smaller than target, stop turn down
        if ((g_dongle_vol.host_vol <= g_dongle_vol.a2u_target_vol) ||
            (g_dongle_vol.a2u_target_vol == 1 && g_dongle_vol.host_vol == 2))
        {
            APP_PRINT_INFO1("app_usb_handle_peer_set_vol_feedback: %x ", vol);
            g_dongle_vol.a2u_adjusting = 0;
            //hid_queue_clear();
            if (g_dongle_vol.a2u_pending_flag)
            {
                g_dongle_vol.a2u_pending_flag = 0;
                app_dongle_handle_a2u_set_vol(g_dongle_vol.a2u_pending_vol, g_dongle_vol.host_mute);
            }
        }
        else if ((g_dongle_vol.host_vol == 1) &&
                 (g_dongle_vol.a2u_target_vol == 0)) // target 0, current 1, stop and send turn down again
        {
            APP_PRINT_INFO1("app_usb_handle_peer_set_vol_feedback: %x ", vol);
            g_dongle_vol.a2u_adjusting = 0;
            g_dongle_vol.host_vol = 0;
            hid_interrupt_in_queue(data, 3);

            g_dongle_vol.a2u_pending_flag = 0;
            data[1] = HID_VOL_DECREMENT_CMD;
            hid_interrupt_in_queue(data, 3);
            //app_dongle_handle_a2u_set_vol(AVRCP_VOL_MIN);
        }
        else
        {
            data[1] = HID_VOL_DECREMENT_CMD;
            hid_interrupt_in_queue(data, 3);
            u2a_vol_feedback_check_timer_start();
        }
    }
    else if (g_dongle_vol.a2u_adjusting == HID_VOL_INCREMENT_CMD) // turn up
    {
        //os vol already bigger than target, stop turn up
        if (g_dongle_vol.host_vol >= g_dongle_vol.a2u_target_vol)
        {
            APP_PRINT_INFO1("app_usb_handle_peer_set_vol_feedback: %x ", vol);
            g_dongle_vol.a2u_adjusting = 0;
            if (g_dongle_vol.a2u_pending_flag)
            {
                g_dongle_vol.a2u_pending_flag = 0;
                app_dongle_handle_a2u_set_vol(g_dongle_vol.a2u_pending_vol, g_dongle_vol.host_mute);
            }
        }
        else if ((g_dongle_vol.host_vol == 0x7E) &&
                 (g_dongle_vol.a2u_target_vol == 0x7F)) //win7 may set vol 0x7E as max vol &&
        {
            APP_PRINT_INFO1("app_usb_handle_peer_set_vol_feedback: %x ", vol);
            g_dongle_vol.a2u_adjusting = 0;
            g_dongle_vol.host_vol = 0x7F;
            hid_interrupt_in_queue(data, 3);
            g_dongle_vol.a2u_pending_flag = 0;
            data[1] = HID_VOL_INCREMENT_CMD;
            hid_interrupt_in_queue(data, 3);
        }
        else
        {
            data[1] = HID_VOL_INCREMENT_CMD;
            hid_interrupt_in_queue(data, 3);
            u2a_vol_feedback_check_timer_start();
        }
    }
    else if (g_dongle_vol.a2u_adjusting == HID_VOL_MUTE_UNMUTE_CMD)
    {
        g_dongle_vol.a2u_adjusting = 0;
    }
}

static void hid_enable_handle_a2u_set_mute(bool mute)
{
    uint8_t data[3] =  {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x01};

    if (g_dongle_vol.host_mute != mute)
    {
        APP_PRINT_INFO1("hid_enable_handle_a2u_set_mute mute %x", mute);
        g_dongle_vol.host_mute = mute;
        g_dongle_vol.a2u_adjusting = HID_VOL_MUTE_UNMUTE_CMD;
        hid_interrupt_in_queue(data, 3);
        a2u_set_vol_timer_start();
    }
}

static void hid_enable_handle_a2u_set_vol(uint8_t vol, bool mute)
{
    /* */
    uint8_t data[3] =  {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};
    uint8_t hid_send_flag = false;

#if (APP_DONGLE_VOL_DEBUG == 1)
    APP_PRINT_INFO4("hid_enable_handle_a2u_set_vol, adjusting %d current %x, target %x, a2d_adjusting %d",
                    g_dongle_vol.a2u_adjusting, g_dongle_vol.host_vol, vol, g_dongle_vol.a2u_adjusting);
#endif

    g_dongle_vol.a2u_target_vol = vol;
    if (g_dongle_vol.a2u_adjusting)
    {
        return;
    }
    if (mute)
    {
        return;
    }

#if USB_VOL_RANGE_CHANGE_SUPPORT
    if ((g_dongle_vol.host_type == DONGLE_HOST_HID_FEEDBACK) &&
        (g_dongle_vol.a2u_target_vol != AVRCP_VOL_MIN) &&
        (g_dongle_vol.a2u_target_vol != AVRCP_VOL_MAX) &&
        (abs(g_dongle_vol.host_vol - g_dongle_vol.a2u_target_vol) <= 1))
    {
        APP_PRINT_INFO0("hid_enable_handle_a2u_set_vol: no need adjust");
        return;
    }
#endif

    if (g_dongle_vol.a2u_target_vol < g_dongle_vol.host_vol)
    {
        g_dongle_vol.a2u_adjusting = HID_VOL_DECREMENT_CMD;
        data[1] = HID_VOL_DECREMENT_CMD;
        hid_send_flag = true;
        hid_interrupt_in_queue(data, 3);
    }
    else if (g_dongle_vol.a2u_target_vol > g_dongle_vol.host_vol)
    {
        g_dongle_vol.a2u_adjusting = HID_VOL_INCREMENT_CMD;
        data[1] = HID_VOL_INCREMENT_CMD;
        hid_send_flag = true;
        hid_interrupt_in_queue(data, 3);
    }
    else
    {
        if (g_dongle_vol.a2u_target_vol == AVRCP_VOL_MIN)
        {
            //if (g_dongle_vol.host_type != DONGLE_HOST_HID_FEEDBACK)
            {
                g_dongle_vol.a2u_adjusting = HID_VOL_DECREMENT_CMD;
                g_dongle_vol.decre_when_min_flag = 1;
                g_dongle_vol.incre_when_max_flag = 0;
                data[1] = HID_VOL_DECREMENT_CMD;

                hid_send_flag = true;
                hid_interrupt_in_queue(data, 3);
            }
        }
        else if (g_dongle_vol.a2u_target_vol == AVRCP_VOL_MAX)
        {
            //if (g_dongle_vol.host_type != DONGLE_HOST_HID_FEEDBACK)
            {
                g_dongle_vol.a2u_adjusting = HID_VOL_INCREMENT_CMD;
                data[1] = HID_VOL_INCREMENT_CMD;
                g_dongle_vol.incre_when_max_flag = 1;
                g_dongle_vol.decre_when_min_flag = 0;

                hid_send_flag = true;
                hid_interrupt_in_queue(data, 3);
            }
        }
    }
    if (hid_send_flag)
    {
#if (APP_DONGLE_VOL_DEBUG == 1)
        APP_PRINT_INFO1("hid_enable_handle_a2u_set_vol send cmd %d", data[1]);
#endif
        a2u_set_vol_timer_start();
    }
    g_dongle_vol.host_vol = g_dongle_vol.a2u_target_vol;
}

static void hid_disable_handle_a2u_set_vol(uint8_t vol)
{
    g_dongle_vol.a2u_target_vol = vol;
    g_dongle_vol.host_vol = vol;
}

void app_dongle_handle_a2u_set_vol(uint8_t vol, bool mute)
{

#if (APP_DONGLE_VOL_DEBUG == 1)
    APP_PRINT_INFO4("a2u_set_vol host state %d, host_type %d, vol %x. current %x",
                    g_dongle_vol.host_state, g_dongle_vol.host_type, vol, g_dongle_vol.host_vol);
#endif

    if (g_dongle_vol.host_type <= DONGLE_HOST_HID_INVALID)
    {
        hid_disable_handle_a2u_set_vol(vol);
    }
    else
    {
        /*when the PC bar is pulled to mini and bud volume up,it will trigger hid mute and
          hid volume up, hid vol up will send fail(block).*/
        if (vol == g_dongle_vol.host_vol)
        {
            hid_enable_handle_a2u_set_mute(mute);
        }
        hid_enable_handle_a2u_set_vol(vol, mute);
    }
}


static void host_init_handle_u2a_set_vol(uint8_t vol)
{

    APP_PRINT_INFO1("host_init_handle_u2a_set_vol vol %x", vol);

    if (vol > PC_VOL_MAX)
    {
        APP_PRINT_WARN1("host_init_handle_u2a_set_vol vol %x bigger than max", vol);
        vol = PC_VOL_MAX;
    }

    g_dongle_vol.host_vol = vol - PC_VOL_MIN;

    /* get u2a init vol */
    app_stop_timer(&timer_idx_init_u2a_vol_wait);
    g_dongle_vol.host_type = DONGLE_HOST_HID_INVALID;

    /* serval u2a init vol may set, wait timeout to send hid check */
    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        hid_valid_check_begin_timer_start();
    }

}

static void host_checking_handle_u2a_set_vol(uint8_t vol)
{
    APP_PRINT_INFO1("host_checking_handle_u2a_set_vol vol %x", vol);

    if (vol > PC_VOL_MAX)
    {
        APP_PRINT_WARN1("host_checking_handle_u2a_set_vol vol %x bigger than max", vol);
        vol = PC_VOL_MAX;
    }

    g_dongle_vol.host_vol = vol - PC_VOL_MIN;

    g_dongle_vol.host_type = DONGLE_HOST_HID_FEEDBACK;
    g_dongle_vol.host_state = DONGLE_HOST_STATE_CHECKED;

    /* try to set earphone vol by avrcp */
#ifdef LEGACY_BT_GAMING
    gaming_bt_set_volume(get_dongle_host_vol(), get_dongle_host_mute());
#endif

#ifdef LEGACY_BT_GENERAL
    general_bt_set_volume(get_dongle_host_vol(), get_dongle_host_mute());
#endif
    le_audio_uac_vol_change(get_dongle_host_vol(), get_dongle_host_mute());
}

static void host_checked_handle_u2a_set_vol(uint8_t vol)
{
    if (g_dongle_vol.host_type <= DONGLE_HOST_HID_INVALID)
    {
        /* adjust ios vol by phone, vol will set down to dongle *
        *  adjust android vol by phone, vol will not set down to dongle */
        APP_PRINT_WARN2("host_checked_handle_u2a_set_vol type %d get set vol %x",
                        g_dongle_vol.host_type, vol);
        //g_dongle_vol.host_type = DONGLE_HOST_HID_NO_FEEDBACK;
    }

    if (vol > PC_VOL_MAX)
    {
        APP_PRINT_WARN1("host_checked_handle_u2a_set_vol vol %x bigger than max", vol);
        vol = PC_VOL_MAX;
    }

    g_dongle_vol.host_vol = vol - PC_VOL_MIN;

    g_dongle_vol.host_type = DONGLE_HOST_HID_FEEDBACK;
    /* a2u set vol, windows feedback */
    if (g_dongle_vol.a2u_adjusting)
    {
        app_stop_timer(&timer_idx_a2u_set_vol);
        a2u_proc_timer_start();
    }
    else
    {
        APP_PRINT_WARN1("host_checked_handle_u2a_set_vol vol %d, a2u_adjusting false", vol);
        /* u2a timer timeout send vol to avrcp,
        * in case too often avrcp affect a2dp  */
        if (g_dongle_vol.u2a_vol_adjusting == 0)
        {
            g_dongle_vol.u2a_vol_adjusting = 1;
            u2a_set_vol_timer_start();
        }
    }
}
/* FIXME: check wether android and ps4 vol have same meaning with windows */
/******************************************
 *   windows vol
 *   vol = 0, means mute
 *   vol = 1, means unmute
 *   vol = 0x2-0x81, mean vol
********************************************/
void app_dongle_handle_u2a_set_vol(uint8_t vol)
{
#if (APP_DONGLE_VOL_DEBUG == 1)
    APP_PRINT_INFO2("app_dongle_handle_u2a_set_vol state %d vol %x", g_dongle_vol.host_state, vol);
#endif
    if (g_dongle_vol.already_receive_pc_vol == false)
    {
        g_dongle_vol.already_receive_pc_vol = true;
    }

    switch (g_dongle_vol.host_state)
    {
    case DONGLE_HOST_STATE_INIT:
        host_init_handle_u2a_set_vol(vol);
        break;
    case DONGLE_HOST_STATE_CHECKING:
        host_checking_handle_u2a_set_vol(vol);
        break;
    case DONGLE_HOST_STATE_CHECKED:
        host_checked_handle_u2a_set_vol(vol);
        break;
    default:
        break;
    }
}

uint8_t get_dongle_host_state(void)
{
    return g_dongle_vol.host_state;
}

uint8_t set_dongle_host_state(uint8_t state)
{
    g_dongle_vol.host_state = state;
    return true;
}

uint8_t get_dongle_host_vol(void)
{
    return g_dongle_vol.host_vol;
}

uint8_t get_dongle_host_mute(void)
{
    return g_dongle_vol.host_mute;
}

void update_dongle_spk_mute_status(uint16_t mute)
{
    APP_PRINT_INFO1("update_dongle_spk_mute_status mute %d", mute);
    host_checked_handle_u2a_set_vol(mute);
    g_dongle_vol.host_mute = mute;
}

static void init_u2a_vol_timeout_proc(void)
{
    uint8_t vol_up_data[3] =  {REPORT_ID_CONSUMER_HOT_KEY_INPUT, HID_VOL_INCREMENT_CMD, 0x00};
    uint8_t vol_down_data[3] =  {REPORT_ID_CONSUMER_HOT_KEY_INPUT, HID_VOL_DECREMENT_CMD, 0x00};
    uint8_t vol_release_data[3] =  {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};

    APP_PRINT_INFO0("init_u2a_vol_timeout_proc, send HID_VOL_INCREMENT_CMD");
    g_dongle_vol.host_state = DONGLE_HOST_STATE_CHECKING;

    usb_hid_report_buffered_send(vol_up_data, 3);
    usb_hid_report_buffered_send(vol_release_data, 3);

    usb_hid_report_buffered_send(vol_down_data, 3);
    usb_hid_report_buffered_send(vol_release_data, 3);
}

static void hid_valid_check_begin_timeout_proc(void)
{
    uint8_t data[3] =  {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};

    hid_interrupt_in_queue(data, 3);

    g_dongle_vol.host_state = DONGLE_HOST_STATE_CHECKING;
    /* timeout means host hid not support */
    hid_valid_check_timer_start();
}

static void app_dongle_vol_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO2("uac_vol_timer_cback: timer_evt %d, param %d",
                    timer_evt, param);
    switch (timer_evt)
    {
    case VOL_TIMER_ID_A2U_SET_VOL:
        {
            app_stop_timer(&timer_idx_a2u_set_vol);
            g_dongle_vol.a2u_adjusting = 0;
            /*this a2u set vol timeout accure means host do not set vol feedback, there are two situation
            1. host is android, no feedback, so set earphone to max
            2. host is pc, no feedback, means pc vol max or min, max ok,
               when host is min, and still vol decrement, timeout do nothing //min FIXME */
            if ((g_dongle_vol.decre_when_min_flag) && (g_dongle_vol.host_type == DONGLE_HOST_HID_FEEDBACK))
            {
                g_dongle_vol.decre_when_min_flag = 0;
                break;
            }

            if (!((g_dongle_vol.host_state == DONGLE_HOST_STATE_CHECKED) &&
                  (g_dongle_vol.host_type == DONGLE_HOST_HID_FEEDBACK)))
            {
                g_dongle_vol.host_vol = AVRCP_VOL_MAX;
#ifdef LEGACY_BT_GAMING
                gaming_bt_set_volume(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
#endif

#ifdef LEGACY_BT_GENERAL
                general_bt_set_volume(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
#endif
                le_audio_uac_vol_change(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
            }
        }
        break;
    case VOL_TIMER_ID_U2A_SET_VOL:
        {
            app_stop_timer(&timer_idx_u2a_set_vol);
            g_dongle_vol.u2a_vol_adjusting = 0;
#ifdef LEGACY_BT_GAMING
#if F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME
            /* don't send vol to headset when vol change on PC */
#else
            gaming_bt_set_volume(g_dongle_vol.host_vol, g_dongle_vol.host_mute);
#endif
#endif

#ifdef LEGACY_BT_GENERAL
            general_bt_set_volume(g_dongle_vol.host_vol, g_dongle_vol.host_mute);
#endif
            le_audio_uac_vol_change(g_dongle_vol.host_vol, g_dongle_vol.host_mute);
        }
        break;
#ifdef HID_SEND_INTERVAL_ENABLE
    case VOL_TIMER_ID_HID_SEND:
        {
            app_stop_timer(&timer_idx_hid_send);
            hid_send_queue.busy = false;
            hid_queue_out();
        }
        break;
#endif
    case VOL_TIMER_ID_HID_VALID_CHECK:
        {
            app_stop_timer(&timer_idx_hid_valid_check);
            g_dongle_vol.host_type = DONGLE_HOST_HID_INVALID;
        }
        break;
    case VOL_TIMER_ID_A2U_PROC_INTERVAL:
        {
            app_stop_timer(&timer_idx_a2u_proc_inter);
            app_stop_timer(&timer_idx_u2a_vol_feedback_check);
            a2u_set_vol_feeback(g_dongle_vol.host_vol);
        }
        break;
    case VOL_TIMER_ID_INIT_U2A_VOL:
        {
            app_stop_timer(&timer_idx_init_u2a_vol_wait);
            init_u2a_vol_timeout_proc();
        }
        break;
    case VOL_TIMER_ID_HID_VALID_CHECK_BEGIN:
        {
            app_stop_timer(&timer_idx_hid_valid_check_begin);
            hid_valid_check_begin_timeout_proc();
        }
        break;
    case VOL_TIMER_ID_U2A_FEEDBACK_CHECK:
        {
            app_stop_timer(&timer_idx_u2a_vol_feedback_check);
            /*The host does not provide feedback on the volume during volume adjustment.
              therefore, this flag may always be true. */
            if (g_dongle_vol.a2u_adjusting)
            {
                g_dongle_vol.a2u_adjusting = 0;
            }
        }
        break;
    case VOL_TIMER_ID_HID_RELEASE_CMD:
        {
            app_stop_timer(&timer_idx_hid_release_cmd_begin);
            uint8_t data[3] =  {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};
#ifndef HID_SEND_INTERVAL_ENABLE
            hid_send_queue.busy = false;
#endif
            /* send key release cmd */
            if (hid_send_queue.cur_cmd)
            {
                hid_send_queue.cur_cmd = 0;
                usb_hid_report_buffered_send(data, sizeof(data));
            }
#ifndef HID_SEND_INTERVAL_ENABLE
            else
            {
                hid_queue_out();
            }
#endif
        }
        break;
    default:
        break;
    }
}

void app_dongle_set_avrcp_vol(void)
{
    //set_dongle_host_state(DONGLE_HOST_STATE_CHECKED);
    APP_PRINT_INFO2("app_dongle_set_avrcp_vol host_state %d, host_type %d",
                    g_dongle_vol.host_state, g_dongle_vol.host_type);
    if (g_dongle_vol.host_state == DONGLE_HOST_STATE_CHECKED)
    {
        /*workaround for PS4/5. when dongle is plugged into the PS,it will send a fixed vol of 0x48,
         *even if the PS itself adjusts the volume to the max. (host type is different from 8753BAU)
         */
        if ((g_dongle_vol.host_type == DONGLE_HOST_HID_INVALID) ||
            (g_dongle_vol.host_type == DONGLE_HOST_HID_NO_FEEDBACK && get_dongle_host_vol() == 0x46))
        {
#ifdef LEGACY_BT_GAMING
            gaming_bt_set_volume(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
#endif

#ifdef LEGACY_BT_GENERAL
            general_bt_set_volume(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
#endif
            le_audio_uac_vol_change(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
        }
        else
        {
#ifdef LEGACY_BT_GAMING
            gaming_bt_set_volume(get_dongle_host_vol(), get_dongle_host_mute());
#endif

#ifdef LEGACY_BT_GENERAL
            general_bt_set_volume(get_dongle_host_vol(), get_dongle_host_mute());
#endif
            le_audio_uac_vol_change(get_dongle_host_vol(), get_dongle_host_mute());
        }
    }
    else if (g_dongle_vol.host_state == DONGLE_HOST_STATE_CHECKING)
    {
        if (g_dongle_vol.host_type == DONGLE_HOST_HID_INVALID)
        {
#ifdef LEGACY_BT_GAMING
            gaming_bt_set_volume(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
#endif

#ifdef LEGACY_BT_GENERAL
            general_bt_set_volume(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
#endif
            le_audio_uac_vol_change(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
        }
        else
        {
#ifdef LEGACY_BT_GAMING
            if (g_dongle_vol.already_receive_pc_vol == false)
            {
                g_dongle_vol.host_vol = USB_UAC_CUR_VOL;
            }
            gaming_bt_set_volume(get_dongle_host_vol(), get_dongle_host_mute());
#endif

#ifdef LEGACY_BT_GENERAL
            general_bt_set_volume(USB_UAC_CUR_VOL, get_dongle_host_mute());
#endif
            le_audio_uac_vol_change(get_dongle_host_vol(), get_dongle_host_mute());
        }
        g_dongle_vol.host_state = DONGLE_HOST_STATE_CHECKED;
    }
}

void app_dongle_set_host_type(uint8_t host_type)
{
    g_dongle_vol.host_type = host_type;
}

void app_dongle_host_type_check(uint8_t type)
{
    switch (type)
    {
    case OS_TYPE_UNDEF:
    case OS_TYPE_WINDOWS:
    case OS_TYPE_IOS:
        {
            app_dongle_set_host_type(DONGLE_HOST_HID_FEEDBACK);
        }
        break;
    case OS_TYPE_ANDROID:
    case OS_TYPE_SWITCH:
        {
            app_dongle_set_host_type(DONGLE_HOST_HID_NO_FEEDBACK);
        }
        break;
    case OS_TYPE_PS:
        {
            app_dongle_set_host_type(DONGLE_HOST_HID_INVALID);
        }
        break;
    default:
        break;
    }
    APP_PRINT_INFO2("app_dongle_host_type_check os_type %d, type %d", type, g_dongle_vol.host_type);
}

void app_dongle_vol_init(void)
{
    app_timer_reg_cb(app_dongle_vol_timeout_cb, &dongle_vol_timer_id);
    memset(&g_dongle_vol, 0, sizeof(g_dongle_vol));

    /* set default host type is aux, all usb device will set uac vol down when usb init */
    g_dongle_vol.host_vol = AVRCP_VOL_MAX;
    g_dongle_vol.host_type = DONGLE_HOST_HID_NO_FEEDBACK;
    g_dongle_vol.host_state = DONGLE_HOST_STATE_CHECKING;
}
