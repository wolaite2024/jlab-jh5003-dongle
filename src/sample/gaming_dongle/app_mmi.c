/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "test_mode.h"
#include "app_timer.h"
#include "bt_avrcp.h"
#include "sysm.h"
#include "eq.h"
#include "audio.h"
#include "audio_track.h"
#include "remote.h"
#include "app_mmi.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_link_util.h"
#include "app_dlps.h"
#include "rtl876x_pinmux.h"
#include "app_io_output.h"
#include "app_audio_path.h"

#if LOCAL_PLAYBACK_FEATURE_SUPPORT
#include "app_playback.h"
#endif
#if (ANC_ENABLE == 1)
//#include "app_anc.h"
#endif
#if (APP_SITRON_INIT == 1)
#include "app_dual_audio_effect.h"
#endif
//#include "app_airplane.h"

#include "app_led_ctl.h"
#include "legacy_audio_wrapper.h"
#include "general_audio_wrapper.h"
#include "le_audio_wrapper.h"
#include "single_tone.h"
#include "gap_vendor.h"
#include "os_sched.h"

#ifdef LEGACY_BT_GAMING
#include "gaming_bt.h"
#endif
#ifdef LEGACY_BT_GENERAL
#include "ual_bluetooth.h"
#endif


#define BT_MODE_CHANGE_TIMEOUT  1000
#define REBOOT_CHECK_MAX_TIMES  56

static uint8_t app_mmi_timer_id = 0;
static uint8_t timer_idx_app_mmi_delay_power_off = 0;
static uint8_t timer_idx_bt_mode_change = 0;

static uint8_t curr_bt_mode = DONGLE_BT_MODE_LEGACY;
static uint8_t next_bt_mode = DONGLE_BT_MODE_LEGACY;

static uint8_t timer_idx_reboot_check = 0;
static uint8_t reboot_check_times = REBOOT_CHECK_MAX_TIMES;

static void app_mmi_reboot_check(void);
static void app_bt_mode_load(void);

typedef enum
{
    APP_TIMER_OTA_JIG_DELAY_WDG_RESET,
    APP_TIMER_DELAY_POWER_OFF,
    APP_TIMER_BT_MODE_CHANGE,
    APP_TIMER_REBOOT_CHECK,
} T_APP_MMI_TIMER;

static void app_mmi_delay_power_off(void)
{
    app_start_timer(&timer_idx_app_mmi_delay_power_off, "app_mmi_delay_power_off",
                    app_mmi_timer_id, APP_TIMER_DELAY_POWER_OFF, 0, false,
                    600);
}

static void app_mmi_power_off(void)
{
    sys_mgr_power_off();
    app_device_state_change(APP_DEVICE_STATE_OFF);
#if 0
    if (app_cfg_const.enable_external_mcu_reset)
    {
        APP_PRINT_TRACE0("power_off disable output 10hz");
        app_io_output_ctrl_ext_mcu_pin(DISABLE);
    }
#endif
}



uint8_t app_get_cur_bt_mode(void)
{
    return curr_bt_mode;
}

void app_bt_mode_init(void)
{
    uint8_t mode = app_get_cur_bt_mode();
    APP_PRINT_INFO1("app_bt_mode_init %x", mode);

#if (F_APP_LE_AUDIO_DONGLE_CIS_ONLY == 1)
    mode = DONGLE_BT_MODE_CIS;
    le_audio_cis_enter();
    app_led_set_mode(APP_LED_MODE_BT_CIS);
#else
    if (mode == DONGLE_BT_MODE_LEGACY)
    {
#ifdef LEGACY_BT_GAMING
        legacy_audio_enter();
#else
        general_audio_enter();
#endif
        app_led_set_mode(APP_LED_MODE_BT_LEGACY);
    }
    else if (mode == DONGLE_BT_MODE_BIS)
    {
        le_audio_bis_enter();
        app_led_set_mode(APP_LED_MODE_BT_BIS);
    }
    else if (mode == DONGLE_BT_MODE_CIS)
    {
        le_audio_cis_enter();
        app_led_set_mode(APP_LED_MODE_BT_CIS);
    }
#endif
}

static void bt_mode_transfer_to_legacy(uint8_t prev)
{
    APP_PRINT_INFO0("bt_mode_transfer_to_legacy");

    switch (prev)
    {
    case DONGLE_BT_MODE_BIS:
        le_audio_bis_exit();
        break;
    case DONGLE_BT_MODE_CIS:
        le_audio_cis_exit();
        break;
    default:
        APP_PRINT_ERROR1("bt_mode_transfer_to_legacy: Incorrect mode %u", prev);
        return;
    }
    curr_bt_mode = DONGLE_BT_MODE_LEGACY;
    app_led_set_mode(APP_LED_MODE_BT_LEGACY);
#ifdef LEGACY_BT_GAMING
    legacy_audio_enter();
#endif
#ifdef LEGACY_BT_GENERAL
    general_audio_enter();
#endif
}

static void bt_mode_transfer_to_bis(uint8_t prev)
{
    APP_PRINT_INFO0("bt_mode_transfer_to_bis");

    switch (prev)
    {
    case DONGLE_BT_MODE_CIS:
        le_audio_cis_exit();
        break;
    case DONGLE_BT_MODE_LEGACY:
#ifdef LEGACY_BT_GAMING
        legacy_audio_exit();
#endif

#ifdef LEGACY_BT_GENERAL
        general_audio_exit();
#endif
        break;
    default:
        APP_PRINT_ERROR1("bt_mode_transfer_to_bis: Incorrect mode %u", prev);
        return;
    }
    curr_bt_mode = DONGLE_BT_MODE_BIS;
    le_audio_bis_enter();
    app_led_set_mode(APP_LED_MODE_BT_BIS);
}

static void bt_mode_transfer_to_cis(uint8_t prev)
{
    APP_PRINT_INFO0("bt_mode_transfer_to_cis");

    switch (prev)
    {
    case DONGLE_BT_MODE_LEGACY:
#ifdef LEGACY_BT_GAMING
        legacy_audio_exit();
#endif

#ifdef LEGACY_BT_GENERAL
        general_audio_exit();
#endif
        break;
    case DONGLE_BT_MODE_BIS:
        le_audio_bis_exit();
        break;
    default:
        APP_PRINT_ERROR1("bt_mode_transfer_to_cis: Incorrect mode %u", prev);
        return;
    }
    curr_bt_mode = DONGLE_BT_MODE_CIS;
    le_audio_cis_enter();
    app_led_set_mode(APP_LED_MODE_BT_CIS);
}

static void app_mmi_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_DELAY_POWER_OFF:
        {
            bt_mode_change_app_stop_timer();
            if (app_get_cur_bt_mode() == DONGLE_BT_MODE_LEGACY)
            {
#ifdef LEGACY_BT_GAMING
                legacy_audio_exit();
#endif

#ifdef LEGACY_BT_GENERAL
                general_audio_exit();
#endif
            }
            else if (app_get_cur_bt_mode() == DONGLE_BT_MODE_BIS)
            {
                le_audio_bis_exit();
            }
            else
            {
                le_audio_cis_exit();
            }

            bt_device_mode_set(BT_DEVICE_MODE_IDLE);
            curr_bt_mode = DONGLE_BT_MODE_LEGACY;
            app_stop_timer(&timer_idx_app_mmi_delay_power_off);
            app_mmi_power_off();
        }
        break;
    case APP_TIMER_BT_MODE_CHANGE:
        APP_PRINT_INFO2("app_mmi_timeout_cb: bt mode %u->%u", curr_bt_mode,
                        next_bt_mode);
        bt_mode_change_app_stop_timer();
        if (next_bt_mode == curr_bt_mode)
        {
            return;
        }
        if (next_bt_mode >= DONGLE_BT_MODE_MAX)
        {
            APP_PRINT_ERROR1("app_mmi_timeout_cb: bt mode %u exceeds max",
                             next_bt_mode);
            return;
        }
        switch (next_bt_mode)
        {
        case DONGLE_BT_MODE_LEGACY:
            bt_mode_transfer_to_legacy(curr_bt_mode);
            break;
        case DONGLE_BT_MODE_BIS:
            bt_mode_transfer_to_bis(curr_bt_mode);
            break;
        case DONGLE_BT_MODE_CIS:
            bt_mode_transfer_to_cis(curr_bt_mode);
            break;
        default:
            APP_PRINT_ERROR1("app_mmi_timeout_cb: Unknown bt mode %u",
                             curr_bt_mode);
            break;
        }

        break;
    case APP_TIMER_REBOOT_CHECK:
        {
            APP_PRINT_INFO0("app_mmi_timeout_cb: APP_TIMER_REBOOT_CHECK");
            app_mmi_reboot_check();
        }
        break;

    default:
        break;
    }
}

#if (F_APP_LE_AUDIO_DONGLE_CIS_ONLY == 1)
#else
static void dongle_mode_change(void)
{
    uint8_t mode = 0xFF;
    uint8_t i = 0;
    while (i < DONGLE_BT_MODE_MAX)
    {
        mode = (curr_bt_mode + i + 1) % DONGLE_BT_MODE_MAX;
        i++;
        if (mode == DONGLE_BT_MODE_LEGACY && app_cfg_const.dongle_legacy_mode_support)
        {
            next_bt_mode = mode;
            break;
        }
        else if (mode == DONGLE_BT_MODE_BIS && app_cfg_const.dongle_bis_mode_support)
        {
            next_bt_mode = mode;
            break;
        }
        else if (mode == DONGLE_BT_MODE_CIS && app_cfg_const.dongle_cis_mode_support)
        {
            next_bt_mode = mode;
            break;
        }
    }
    APP_PRINT_INFO4("dongle_mode_change prio %x legacy %x bis %x cis %x",
                    app_cfg_const.dongle_bt_src_prio,
                    app_cfg_const.dongle_legacy_mode_support,
                    app_cfg_const.dongle_bis_mode_support,
                    app_cfg_const.dongle_cis_mode_support);
    APP_PRINT_INFO2("dongle_mode_change: curr mode %x, next mode %x",
                    curr_bt_mode, next_bt_mode);
    if (curr_bt_mode == next_bt_mode)
    {
        return;
    }
    app_start_timer(&timer_idx_bt_mode_change, "app_mmi_bt_mode_change",
                    app_mmi_timer_id, APP_TIMER_BT_MODE_CHANGE, 0, false, BT_MODE_CHANGE_TIMEOUT);
}
#endif

static void app_bt_mode_load(void)
{
    if (app_cfg_const.dongle_bt_src_prio == 2)
    {
        curr_bt_mode = DONGLE_BT_MODE_CIS;
    }
    else if (app_cfg_const.dongle_bt_src_prio == 1)
    {
        curr_bt_mode = DONGLE_BT_MODE_BIS;
    }
    else
    {
        curr_bt_mode = DONGLE_BT_MODE_LEGACY;
    }
    next_bt_mode = curr_bt_mode;
    APP_PRINT_TRACE1("app_mmi_init curr_bt_mode %x", curr_bt_mode);
}

void app_mmi_init(void)
{
    app_bt_mode_load();
    app_timer_reg_cb(app_mmi_timeout_cb, &app_mmi_timer_id);
}

void app_mmi_reboot_check_timer_start(uint32_t period_timer)
{
    app_start_timer(&timer_idx_reboot_check, "reboot_check",
                    app_mmi_timer_id, APP_TIMER_REBOOT_CHECK, 0, false,
                    period_timer);
}

static void app_mmi_reboot(void)
{
    app_device_factory_reset();

    app_device_state_change(APP_DEVICE_STATE_OFF);

    os_delay(20);
    chip_reset(RESET_ALL_EXCEPT_AON);

}

static void app_mmi_reboot_check(void)
{
    app_stop_timer(&timer_idx_reboot_check);

    if (reboot_check_times)
    {
        reboot_check_times--;
    }

    if (!reboot_check_times)
    {
        app_mmi_reboot();
    }
    else
    {
#ifdef LEGACY_BT_GAMING
        if (gaming_bt_is_connected())
#endif
#ifdef LEGACY_BT_GENERAL
            extern uint8_t src_get_audio_device_conn_state(void);
        if (src_get_audio_device_conn_state() == BT_CONN_STATE_CONNECTED)
#endif
        {
            APP_PRINT_INFO0("app_mmi_reboot_check: check fail reason");
            app_mmi_reboot_check_timer_start(100);
        }
        else
        {
            app_mmi_reboot();
        }
    }
}

void app_mmi_handle_action(uint8_t action)
{
    uint32_t device = 0xffffffff;

    if (app_cfg_const.dongle_media_device == 1)
    {
        device = AUDIO_DEVICE_IN_AUX;
    }
    else if (app_cfg_const.dongle_media_device == 2)
    {
        device = AUDIO_DEVICE_IN_MIC;
    }

    APP_PRINT_INFO2("app_mmi_handle_action: action 0x%02x, device 0x%x", action, device);
    switch (action)
    {
    case MMI_DEV_POWER_ON:
        {
            sys_mgr_power_on();
            app_bt_mode_load();
#if (F_APP_LE_AUDIO_DONGLE_CIS_ONLY == 1)
            bt_mode_transfer_to_cis(curr_bt_mode);
#endif
        }
        break;

    case MMI_DEV_POWER_OFF:
        {
            app_mmi_delay_power_off();
        }
        break;

    case MMI_DEV_FACTORY_RESET:
        {
            app_dlps_disable(APP_DLPS_ENTER_CHECK_WAIT_RESET);

            app_stop_timer(&timer_idx_reboot_check);
            app_mmi_reboot_check_timer_start(500);

            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                app_device_state_change(APP_DEVICE_STATE_OFF_ING);
                sys_mgr_power_off();
            }
        }
        break;

    case MMI_DUT_TEST_MODE:
        {
            if (app_db.force_enter_dut_mode_when_acl_connected)
            {
                //Enable Device Under Test mode
                gap_vendor_cmd_req(0x1803, 0, NULL);
#ifdef LEGACY_BT_GAMING
                if (app_cfg_const.led_support)
                {
                    app_led_set_mode(APP_LED_MODE_ALL_ON);
                }
#endif
            }
            else
            {
                switch_into_single_tone_test_mode();
            }
        }
        break;

    case MMI_DEV_BT_MODE_SWITCH:
        {
#if (F_APP_LE_AUDIO_DONGLE_CIS_ONLY == 1)
#else
            dongle_mode_change();
#endif
        }
        break;

    case MMI_BT_MODE_LEGACY:
        {
            if (app_cfg_const.dongle_legacy_mode_support)
            {
                bt_mode_transfer_to_legacy(curr_bt_mode);
            }
        }
        break;

    case MMI_DEV_MIC_MUTE:
        {
        }
        break;

    case MMI_DEV_MIC_UNMUTE:
        {
        }
        break;

    case MMI_DEV_MIC_VOL_UP:
        {
        }
        break;

    case MMI_DEV_MIC_VOL_DOWN:
        {
        }
        break;

    case MMI_BT_LEGACY_LINK1_SCAN:
    case MMI_BT_LEGACY_LINK1_RECONNECT:
    case MMI_BT_LEGACY_LINK1_DISCONNECT:
    case MMI_BT_LEGACY_LINK2_SCAN:
    case MMI_BT_LEGACY_LINK2_RECONNECT:
    case MMI_BT_LEGACY_LINK2_DISCONNECT:
        {
#ifdef LEGACY_BT_GAMING
            legacy_audio_mmi(action, NULL);
#endif
#ifdef LEGACY_BT_GENERAL
            general_audio_mmi(action, NULL);
#endif
        }
        break;

    case MMI_BT_MODE_BIS:
        {
            if (app_cfg_const.dongle_bis_mode_support)
            {
                bt_mode_transfer_to_bis(curr_bt_mode);
            }
        }
        break;

    case MMI_BT_BIS_START:
    case MMI_BT_BIS_STOP:
        le_audio_mmi_bis(action, NULL);
        break;

    case MMI_BT_MODE_CIS:
        {
            if (app_cfg_const.dongle_cis_mode_support)
            {
                bt_mode_transfer_to_cis(curr_bt_mode);
            }
        }
        break;

    case MMI_BT_CIS_SCAN:
    case MMI_BT_CIS_RECONNECT:
    case MMI_BT_CIS_DISCONNECT:
        {
            le_audio_mmi_cis(action, NULL);
        }
        break;

    default:
        break;
    }
}

bool app_mmi_is_local_execute(uint8_t action)
{
    bool ret = false;

    if (action == MMI_DEV_FACTORY_RESET)
    {
        ret = true;
    }

    return ret;
}

uint8_t app_dongle_get_status(void)
{
    uint8_t status = 0;

    if (curr_bt_mode == DONGLE_BT_MODE_LEGACY)
    {
#if LEGACY_BT_GENERAL
        status = (!general_get_status(0)) ? DONGLE_LEGACY_IDLE : DONGLE_LEGACY_CONNECT;
#endif

#if LEGACY_BT_GAMING
        status = (!gaming_get_status(0)) ? DONGLE_LEGACY_IDLE : DONGLE_LEGACY_CONNECT;
#endif
    }
    else if (curr_bt_mode == DONGLE_BT_MODE_BIS)
    {
#if DONGLE_LE_AUDIO
        status = (!le_audio_bis_get_status()) ? DONGLE_BIS_IDLE : DONGLE_BIS_STREAMING;
#endif
    }
    else
    {
#if DONGLE_LE_AUDIO
        status = (!le_audio_cis_get_status()) ? DONGLE_CIS_IDLE : DONGLE_CIS_CONNECT;
#endif
    }

    APP_PRINT_INFO2("app_dongle_get_status: mode %x status %x", curr_bt_mode, status);
    return status;
}

/* Below weak funcs are defined for compiling error */
__weak bool app_audio_tone_play(uint8_t index, bool flush, bool relay)
{
    return false;
}

void bt_mode_change_app_stop_timer(void)
{
    app_stop_timer(&timer_idx_bt_mode_change);
}
