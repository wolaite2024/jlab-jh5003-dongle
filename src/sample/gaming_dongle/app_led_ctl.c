/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <string.h>
#include "rtl876x_pinmux.h"
#include "rtl876x_tim.h"
#include "rtl876x_rcc.h"
#include "section.h"
#include "app_sensor.h"
#include "app_led_ctl.h"
#include "app_timer.h"
#include "os_mem.h"
#include "app_cfg.h"
#include "rtl876x.h"
#include "board.h"
#include "led_module.h"
#include "platform_utils.h"
#include "rtl876x_gpio.h"
#include "trace.h"
#include "audio.h"
#include "app_mmi.h"

#define LED_NUM       3

#define PWM_OUT_COUNT (400000) //time Unit: 10ms(400000/40MHz)

#ifdef LEGACY_BT_GENERAL
#define LED_FLASH_TIMEOUT                         (500)
#else
#define LED_FLASH_TIMEOUT                         (1000)
#endif
#ifdef LEGACY_BT_GENERAL
#define TEAMS_CONNECT_MODE_TIMEOUT                (2000)
#define TEAMS_NO_CONNECT_MODE_TIMEOUT             (10000)
#define TEAMS_BUTTON_WITHOUT_CONNECTION_TIMEOUT   (3000)
#define TEAMS_PULSE_WHITE_CONTINUE_TIMEOUT        (300000)
#define TEAMS_INVOKING_VOICE_SKILLS_TIMEOUT       (5000)
#endif

#ifdef LEGACY_BT_GENERAL
static uint8_t timer_idx_connect_with_teams = 0;
static uint8_t timer_idx_flash_off = 0;
uint8_t timer_idx_started_meeting_or_incoming_meeting = 0;
uint8_t timer_idx_device_disconnected = 0;
uint8_t timer_idx_device_pairing = 0;
bool teams_is_off = false;
extern bool hfp_is_connected;
static T_APP_LED_MODE mode_before_mute = APP_LED_MODE_POWER_OFF;
T_APP_LED_MODE current_mode = APP_LED_MODE_POWER_OFF;
T_APP_LED_MODE previous_mode = APP_LED_MODE_POWER_OFF;
uint8_t current_count = 0;
extern bool app_hfp_isInCall;
extern bool isMute;
extern bool isOutgoingCall;
extern bool isIncomingCall;
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
extern bool teams_connect_complete;
#endif
bool plug_flag = false;
static uint8_t app_led_teams_mode_start = 0x0a;
#endif

static T_LED_TABLE app_led_setting[LED_NUM];
static T_APP_LED_MODE app_led_next_mode;
static uint8_t app_led_timer_id = 0;
static uint8_t timer_idx_flash_on = 0;

static void app_led_pinmux_config(uint8_t pinmux, PAD_FUNCTION_CONFIG_VAL value)
{
    if (led_is_sleep_pinmux(pinmux))
    {
        Pad_FunctionConfig(pinmux, value);
    }
}

void app_led_pad_config(void)
{
    if (app_cfg_const.led_support)
    {
        if (app_cfg_const.led_0_pinmux != 0xFF)
        {
            app_led_pinmux_config(app_cfg_const.led_0_pinmux, LED0);

            if (app_cfg_const.enable_led0_low_active)
            {
                led_set_active_polarity(LED_CH_0, LED_ACTIVE_POLARITY_LOW);
                Pad_Config(app_cfg_const.led_0_pinmux,
                           PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            }
            else
            {
                led_set_active_polarity(LED_CH_0, LED_ACTIVE_POLARITY_HIGH);
                Pad_Config(app_cfg_const.led_0_pinmux,
                           PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
            }
        }

        if (app_cfg_const.led_1_pinmux != 0xFF)
        {
            app_led_pinmux_config(app_cfg_const.led_1_pinmux, LED1);

            if (app_cfg_const.enable_led1_low_active)
            {
                led_set_active_polarity(LED_CH_1, LED_ACTIVE_POLARITY_LOW);
                Pad_Config(app_cfg_const.led_1_pinmux,
                           PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            }
            else
            {
                led_set_active_polarity(LED_CH_1, LED_ACTIVE_POLARITY_HIGH);
                Pad_Config(app_cfg_const.led_1_pinmux,
                           PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
            }
        }

        if (app_cfg_const.led_2_pinmux != 0xFF)
        {
            app_led_pinmux_config(app_cfg_const.led_2_pinmux, LED2);

            if (app_cfg_const.enable_led2_low_active)
            {
                led_set_active_polarity(LED_CH_2, LED_ACTIVE_POLARITY_LOW);
                Pad_Config(app_cfg_const.led_2_pinmux,
                           PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            }
            else
            {
                led_set_active_polarity(LED_CH_2, LED_ACTIVE_POLARITY_HIGH);
                Pad_Config(app_cfg_const.led_2_pinmux,
                           PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
            }
        }

        led_reset_sleep_led();
    }
}

#ifdef LEGACY_BT_GENERAL
static bool app_is_call_relative()
{
    if (app_hfp_isInCall)
    {
        if (isMute)
        {
            app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
        }
        else
        {
            app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
        }
        return true;
    }
    else if (isIncomingCall)
    {
        app_led_set_mode(APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS);
        return true;
    }
    else if (isOutgoingCall)
    {
        app_led_set_mode(
            APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT);
        return true;
    }

    return false;
}

bool app_led_set_before_white()
{
    if (app_hfp_isInCall)
    {
        if (isMute)
        {
            app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
        }
        else
        {
            app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
        }
        return true;
    }
    else if (isOutgoingCall)
    {
        app_led_set_mode(
            APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT);
        return true;
    }
    else if (isIncomingCall)
    {
        app_led_set_mode(APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS);
        return true;
    }
    return false;
}

bool app_led_control_timer_connect_with_teams_started(void)
{
    return (timer_idx_connect_with_teams != 0) ? true : false;
}

void app_stop_missed_call_timer(void)
{
    if (timer_idx_started_meeting_or_incoming_meeting)
    {
        app_stop_timer(&timer_idx_started_meeting_or_incoming_meeting);
    }
}

void app_stop_pulse_white_timer()
{

}

void app_stop_flash_on_and_off_timer()
{
    if (timer_idx_flash_on)
    {
        current_count = 0;
        app_stop_timer(&timer_idx_flash_on);
        uint8_t config[] = {0, 0, 0, 0, 0, 0};
        for (int i = 0; i < 3; i++)
        {
            app_led_set_custom_config(i, config);
        }
    }
    if (timer_idx_flash_off)
    {
        current_count = 0;
        app_stop_timer(&timer_idx_flash_off);
    }
}
#endif

static void app_led_load_table(T_APP_LED_MODE mode)
{
    uint8_t index = (uint8_t)mode;
    /* Gaming and teams used different mode table */
#ifdef LEGACY_BT_GENERAL
    if (mode >= APP_LED_MODE_ESTABLISH_CONNECTION_WITH_TEAMS_APP)
    {
        index = index - app_led_teams_mode_start;
    }
#endif
    /* Load led mode configuration from flash */
    if (app_cfg_load_led_table(app_led_setting, index, LED_NUM * sizeof(T_LED_TABLE)) == false)
    {
        APP_PRINT_INFO0("app_led_load_table: load cfg table failed");
    }
}

void app_led_set_custom_config(uint8_t led, uint8_t *config)
{
    T_LED_TABLE *led_setting_tmp = (T_LED_TABLE *)config;
    /*
       set led brightness by on_time and off_time.
       type need set to LED_TYPE_BREATH.
       on_time indicates the totle time from darkness to bright.
       off_time indicates the totle time from bright to darkness.
       blink_count need set to zero.
    */
    if (led_setting_tmp->type == 5)
    {
        led_setting_tmp->type = LED_TYPE_BREATH;
        led_setting_tmp->blink_count = 0;
    }
    led_cmd(BIT(led), DISABLE);
    led_config((T_LED_CHANNEL)BIT(led), led_setting_tmp);
    led_cmd(BIT(led), true);
    APP_PRINT_INFO2("app_led_set_custom_config: led %d, type %d", led, led_setting_tmp->type);
}

static void app_led_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO2("app_led_timeout_cb: timer_evt %d, param %d",
                    timer_evt, param);
#if 1
    switch (timer_evt)
    {
    case LED_TIMER_ID_FLASH_ON:
        {
            app_stop_timer(&timer_idx_flash_on);
            uint8_t config[] = {0, 0, 0, 0, 0, 0};
            app_led_set_custom_config(0, config);
            break;
        }

#ifdef LEGACY_BT_GENERAL
    case LED_TIMER_ID_PULSE_CONTINUE:
        {
            app_stop_timer(&timer_idx_started_meeting_or_incoming_meeting);
            APP_PRINT_TRACE2("LED_TIMER_ID_PULSE_CONTINUE, previous_mode:%d, app_hfp_isInCall: %d",
                             previous_mode, app_hfp_isInCall);
            if (!app_hfp_isInCall)
            {
                app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
            }
            else
            {
                app_led_set_mode(current_mode);
            }
            break;
        }
#endif
    }
#else
    APP_PRINT_INFO2("app_led_timeout_cb: timer_evt %d, timer_channel %d",
                    timer_evt, param);

    switch (timer_evt)
    {
    case LED_TIMER_ID_CONNECT_WITH_TEAMS:
        {
            app_stop_timer(&timer_idx_connect_with_teams);
            if (teams_connect_complete)
            {
                APP_PRINT_TRACE3("app_hfp_isInCall: %d, isOutgoingCall: %d, isMute: %d", app_hfp_isInCall,
                                 isOutgoingCall, isMute);
                if (app_hfp_isInCall)
                {
                    if (isMute)
                    {
                        app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
                    }
                    else
                    {
                        app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
                    }
                }
                else if (isOutgoingCall && isMute)
                {
                    isOutgoingCall = false;
                    app_hfp_isInCall = true;
                    plug_flag = true;
                    bt_hfp_ag_call_answer(sink_dev_list[0].remote_info.bd_addr);
                    app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
                }
                else
                {
                    app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
                }
            }
            break;
        }
    case LED_TIMER_ID_FLASH_ON:
        {
            app_stop_timer(&timer_idx_flash_on);
            uint8_t config[] = {0, 0, 0, 0, 0, 0};
            for (int i = 0; i < 3; i++)
            {
                app_led_set_custom_config(i, config);
            }

            app_start_timer(&timer_idx_flash_off, "led_flash_off",
                            app_led_timer_id, LED_TIMER_ID_FLASH_OFF, 0, false,
                            LED_FLASH_TIMEOUT);
            break;
        }
    case LED_TIMER_ID_FLASH_OFF:
        {
            app_stop_timer(&timer_idx_flash_off);
            current_count++;
            if ((current_mode == APP_LED_MODE_FAIL_OR_NO_CONNECTION_WITH_TEAMS) && (current_count == 10))
            {
                current_count = 0;
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
                extern bool receive_version_query;
                if (!receive_version_query)
                {
                    teams_is_off = true;
                    app_led_set_mode(APP_LED_MODE_ALL_OFF);
                }
#endif
            }
            else if ((current_mode == APP_LED_MODE_TEAMS_BUTTON_IS_PRESSED_BUT_NO_CONNECTION) &&
                     (current_count == 3))
            {
                current_count = 0;
                if (app_hfp_isInCall)
                {
                    if (isMute)
                    {
                        app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
                    }
                    else
                    {
                        app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
                    }
                }
                else if (isIncomingCall)
                {
                    app_led_set_mode(APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS);
                }
                else if (isOutgoingCall)
                {
                    app_led_set_mode(
                        APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT);
                }
                else if (hfp_is_connected)
                {
                    app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
                }
                else
                {
                    app_led_set_mode(APP_LED_MODE_ALL_OFF);
                }
            }
            else if ((current_mode == APP_LED_MODE_INVOKING_VOICE_SKILLS_IN_TEAMS_APP) && (current_count == 5))
            {
                current_count = 0;
                if (!app_led_set_before_white())
                {
                    app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
                }
            }
            else
            {
                if (current_mode == APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS)
                {
                    uint8_t rconfig[] = {5, app_led_ctrl_led_max.WMax_GOn * 0xFF / app_led_ctrl_led_max.WMax_WOn, 0, 0, 0, 0};
                    uint8_t bconfig[] = {5, app_led_ctrl_led_max.RMax_GOn * 0xFF / app_led_ctrl_led_max.RMax_ROn, 0, 0, 0, 0};
                    uint8_t gconfig[] = {1, 0, 0, 0, 0, 0};
                    app_led_set_custom_config(0, rconfig);
                    app_led_set_custom_config(1, bconfig);
                    app_led_set_custom_config(2, gconfig);
                }
                else
                {
                    uint8_t rconfig[] = {1, 0, 0, 0, 0, 0};
                    uint8_t bconfig[] = {5, app_led_ctrl_led_max.RMax_WOn * 0xFF / app_led_ctrl_led_max.RMax_ROn, 0, 0, 0, 0};
                    uint8_t gconfig[] = {5, app_led_ctrl_led_max.GMax_WOn * 0xFF / app_led_ctrl_led_max.GMax_GOn, 0, 0, 0, 0};
                    app_led_set_custom_config(0, rconfig);
                    app_led_set_custom_config(1, bconfig);
                    app_led_set_custom_config(2, gconfig);
                }
                app_start_timer(&timer_idx_flash_on, "led_flash_on",
                                app_led_timer_id, LED_TIMER_ID_FLASH_ON, 0, false,
                                LED_FLASH_TIMEOUT);
            }
            break;
        }
    case LED_TIMER_ID_PULSE_CONTINUE:
        {
            app_stop_timer(&timer_idx_started_meeting_or_incoming_meeting);
            APP_PRINT_TRACE2("LED_TIMER_ID_PULSE_CONTINUE, previous_mode:%d, app_hfp_isInCall: %d",
                             previous_mode, app_hfp_isInCall);
            if (!app_hfp_isInCall)
            {
                app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
            }
            else
            {
                app_led_set_mode(current_mode);
            }
            break;
        }
    case LED_TIMER_ID_DEVICE_DISCONNECTED:
        {
            app_stop_timer(&timer_idx_device_disconnected);
            app_led_set_mode(APP_LED_MODE_ALL_OFF);
            break;
        }
    case LED_TIMER_ID_DEVICE_PAIRING:
        {
            app_stop_timer(&timer_idx_device_pairing);
            app_led_set_mode(APP_LED_MODE_ALL_OFF);
            break;
        }
    }
#endif
}

static void app_led_change_mode(void)
{
    uint8_t i;
    uint8_t channel = 0;
    static T_APP_LED_MODE pre_mode = APP_LED_MODE_POWER_OFF;

    if (pre_mode == app_led_next_mode)
    {
        return;
    }

#ifdef LEGACY_BT_GENERAL
    if ((app_led_next_mode == APP_LED_MODE_TEAMS_MEETING_HAS_STARTED_OR_INCOMING_MEETING_NOTIFICATION)
        || (app_led_next_mode == APP_LED_MODE_TEAMS_VOICE_MAIL_NOTIFICATION_OR_MISSED_CALL_NOTIFICATION))
    {
        if (app_hfp_isInCall)
        {
            return;
        }
    }

    if ((app_led_next_mode == APP_LED_MODE_DISCONNECTED)
        || (app_led_next_mode == APP_LED_MODE_CONNECTED)
        || (app_led_next_mode == APP_LED_MODE_PARING))
    {
        if (app_is_call_relative())
        {
            return;
        }
    }
#endif

    APP_PRINT_INFO2("app_led_change_mode: pre_mode 0x%02x, mode 0x%02x", pre_mode, app_led_next_mode);

    if (app_cfg_const.led_support)
    {
        app_led_load_table(app_led_next_mode);

        if (((app_cfg_const.led_0_pinmux == 0xFF) || (app_led_setting[0].type == LED_TYPE_BYPASS)) &&
            ((app_cfg_const.led_1_pinmux == 0xFF) || (app_led_setting[1].type == LED_TYPE_BYPASS)) &&
            ((app_cfg_const.led_2_pinmux == 0xFF) || (app_led_setting[2].type == LED_TYPE_BYPASS)))
        {
            //ALL led types are bypass, NO need to set led
            pre_mode = app_led_next_mode;
            return;
        }

        //Switch to GPIO first to avoid low-active led abnormal flash
        app_led_pinmux_config(app_cfg_const.led_0_pinmux, AON_GPIO);
        app_led_pinmux_config(app_cfg_const.led_1_pinmux, AON_GPIO);
        app_led_pinmux_config(app_cfg_const.led_2_pinmux, AON_GPIO);

        for (i = 0; i < LED_NUM; i++)
        {
            if (app_led_setting[i].type != LED_TYPE_KEEP_OFF)
            {
                if (app_led_setting[i].type != LED_TYPE_BYPASS)
                {
                    led_cmd(BIT(i), DISABLE);
                    channel |= BIT(i);
                    led_config((T_LED_CHANNEL)BIT(i), &app_led_setting[i]);
                }
            }
            else
            {
                led_cmd(BIT(i), DISABLE);//Avoid sleed led abnormal blink in "LOW active" case
                platform_delay_us(100);//??  why??????
                //Clear LED register
                led_deinit((T_LED_CHANNEL)BIT(i));
            }
        }

        APP_PRINT_INFO1("app_led_change_mode: CH 0x%02x", channel);
        led_cmd(channel, ENABLE);

        app_led_pinmux_config(app_cfg_const.led_0_pinmux, LED0);
        app_led_pinmux_config(app_cfg_const.led_1_pinmux, LED1);
        app_led_pinmux_config(app_cfg_const.led_2_pinmux, LED2);

        pre_mode = app_led_next_mode;
    }
}

#ifdef LEGACY_BT_GENERAL
static void app_led_check_teams_mode(T_APP_LED_MODE *mode)
{
    if ((*mode == APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE)
        || (*mode == APP_LED_MODE_TEAMS_IN_MEETING)
        || (*mode == APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS)
        || (*mode == APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT)
        || (*mode == APP_LED_MODE_TEAMS_IN_CALL)
        || (*mode == APP_LED_MODE_TEAMS_STATUS_MUTE))
    {
        if (!hfp_is_connected)
        {
            *mode = APP_LED_MODE_ALL_OFF;
        }
    }

    if (*mode == APP_LED_MODE_TEAMS_STATUS_MUTE)
    {
        mode_before_mute = previous_mode;
    }

    if (*mode == APP_LED_MODE_TEAMS_STATUS_UNMUTE)
    {
        if (previous_mode == APP_LED_MODE_TEAMS_STATUS_MUTE)
        {
            *mode = mode_before_mute;
        }
        else
        {
            *mode = previous_mode;
        }
    }

    if (*mode == APP_LED_MODE_CONNECTED)
    {
#if (LENOVO_LED_SUPPORT == 1)
        if (teams_connect_complete)
        {
            *mode = APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE;
        }
#endif
    }
}

static void app_led_start_timer_for_teams(void)
{
    switch (app_led_next_mode)
    {
    case APP_LED_MODE_ESTABLISH_CONNECTION_WITH_TEAMS_APP:
        {
            app_start_timer(&timer_idx_connect_with_teams, "led_connec_with_teams",
                            app_led_timer_id, LED_TIMER_ID_CONNECT_WITH_TEAMS, 0, false,
                            TEAMS_CONNECT_MODE_TIMEOUT);
            break;
        }
    case APP_LED_MODE_FAIL_OR_NO_CONNECTION_WITH_TEAMS:
    case APP_LED_MODE_TEAMS_BUTTON_IS_PRESSED_BUT_NO_CONNECTION:
    case APP_LED_MODE_INVOKING_VOICE_SKILLS_IN_TEAMS_APP:
    case APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS:
        {
            app_start_timer(&timer_idx_flash_on, "led_flash_on",
                            app_led_timer_id, LED_TIMER_ID_FLASH_ON, 0, false,
                            LED_FLASH_TIMEOUT);
            break;
        }
    case APP_LED_MODE_TEAMS_MEETING_HAS_STARTED_OR_INCOMING_MEETING_NOTIFICATION:
    case APP_LED_MODE_TEAMS_VOICE_MAIL_NOTIFICATION_OR_MISSED_CALL_NOTIFICATION:
        {
            app_start_timer(&timer_idx_started_meeting_or_incoming_meeting, "led_teams_notification",
                            app_led_timer_id, LED_TIMER_ID_PULSE_CONTINUE, 0, false,
                            TEAMS_PULSE_WHITE_CONTINUE_TIMEOUT);
            break;
        }
    default:
        break;
    }
}
#endif

#if 0
void dongle_le_set_mode(T_APP_LED_MODE mode)
{


    switch (mode)
    {
    case APP_LED_MODE_ALL_OFF:
    case APP_LED_MODE_ALL_ON:
    case APP_LED_MODE_POWER_OFF:
    case APP_LED_MODE_POWER_ON:

    case APP_LED_MODE_FACTORY_RESET:

    case APP_LED_MODE_BT_BIS:
    case APP_LED_MODE_BT_CIS:
    case APP_LED_MODE_BT_LEGACY:

    case APP_LED_MODE_BROADCASTING:
    case APP_LED_MODE_BST_IDLE:

    case APP_LED_MODE_UNICAST_IDLE:
    case APP_LED_MODE_UNICAST_BG_SCAN:
    case APP_LED_MODE_UNICAST_PAIRING:
    case APP_LED_MODE_UNICAST_CONNECTED:

    //gaming mode
    case APP_LED_MODE_LINK1_STANDBY:
    case APP_LED_MODE_LINK1_PAIRING:
    case APP_LED_MODE_LINK1_RECONNECTING:
    case APP_LED_MODE_LINK1_PAIRED:
    case APP_LED_MODE_LINK1_CONNECTED:

    case APP_LED_MODE_LINK2_STANDBY:
    case APP_LED_MODE_LINK2_PAIRING:
    case APP_LED_MODE_LINK2_RECONNECTING:
    case APP_LED_MODE_LINK2_PAIRED:
    case APP_LED_MODE_LINK2_CONNECTED:

        //teams mode
#ifdef LEGACY_BT_GENERAL
    case APP_LED_MODE_ESTABLISH_CONNECTION_WITH_TEAMS_APP:
    case APP_LED_MODE_FAIL_OR_NO_CONNECTION_WITH_TEAMS:
    case APP_LED_MODE_TEAMS_BUTTON_IS_PRESSED_BUT_NO_CONNECTION:
    case APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE:
    case APP_LED_MODE_TEAMS_MEETING_HAS_STARTED_OR_INCOMING_MEETING_NOTIFICATION:
    case APP_LED_MODE_TEAMS_IN_MEETING:
    case APP_LED_MODE_TEAMS_VOICE_MAIL_NOTIFICATION_OR_MISSED_CALL_NOTIFICATION:
    case APP_LED_MODE_INVOKING_VOICE_SKILLS_IN_TEAMS_APP:
    case APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS:
    case APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT:
    case APP_LED_MODE_TEAMS_IN_CALL:
    case APP_LED_MODE_TEAMS_STATUS_MUTE:
    case APP_LED_MODE_TEAMS_STATUS_UNMUTE:

    case APP_LED_MODE_DISCONNECTED:
    case APP_LED_MODE_CONNECTED:
    case APP_LED_MODE_PARING:
#endif
    }
}
#endif

void app_led_set_mode(T_APP_LED_MODE mode)
{
    uint8_t bt_mode = app_get_cur_bt_mode();
    APP_PRINT_INFO2("app_led_set_mode: bt_mode %x led_mode %x", bt_mode, mode);

    switch (mode)
    {
    case APP_LED_MODE_ALL_OFF:
    case APP_LED_MODE_ALL_ON:
    case APP_LED_MODE_POWER_OFF:
    case APP_LED_MODE_POWER_ON:
    case APP_LED_MODE_FACTORY_RESET:
        break;

    case APP_LED_MODE_BT_BIS:
    case APP_LED_MODE_BROADCASTING:
    case APP_LED_MODE_BST_IDLE:
        {
            if (bt_mode != DONGLE_BT_MODE_BIS)
            {
                return;
            }
        }
        break;

    case APP_LED_MODE_BT_CIS:
    case APP_LED_MODE_UNICAST_IDLE:
    case APP_LED_MODE_UNICAST_BG_SCAN:
    case APP_LED_MODE_UNICAST_PAIRING:
    case APP_LED_MODE_UNICAST_CONNECTED:
        {
            if (bt_mode != DONGLE_BT_MODE_CIS)
            {
                return;
            }
        }
        break;

    case APP_LED_MODE_BT_LEGACY:
        {
            if (bt_mode != DONGLE_BT_MODE_LEGACY)
            {
                return;
            }
        }
        break;

#if LEGACY_BT_GAMING
    //gaming mode
    case APP_LED_MODE_LINK1_STANDBY:
    case APP_LED_MODE_LINK1_PAIRING:
    case APP_LED_MODE_LINK1_RECONNECTING:
    case APP_LED_MODE_LINK1_PAIRED:
    case APP_LED_MODE_LINK1_CONNECTED:

    case APP_LED_MODE_LINK2_STANDBY:
    case APP_LED_MODE_LINK2_PAIRING:
    case APP_LED_MODE_LINK2_RECONNECTING:
    case APP_LED_MODE_LINK2_PAIRED:
    case APP_LED_MODE_LINK2_CONNECTED:
        {
            if (bt_mode != DONGLE_BT_MODE_LEGACY)
            {
                return;
            }
        }
        break;
#endif
        //teams mode
#ifdef LEGACY_BT_GENERAL
    case APP_LED_MODE_ESTABLISH_CONNECTION_WITH_TEAMS_APP:
    case APP_LED_MODE_FAIL_OR_NO_CONNECTION_WITH_TEAMS:
    case APP_LED_MODE_TEAMS_BUTTON_IS_PRESSED_BUT_NO_CONNECTION:
    case APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE:
    case APP_LED_MODE_TEAMS_MEETING_HAS_STARTED_OR_INCOMING_MEETING_NOTIFICATION:
    case APP_LED_MODE_TEAMS_IN_MEETING:
    case APP_LED_MODE_TEAMS_VOICE_MAIL_NOTIFICATION_OR_MISSED_CALL_NOTIFICATION:
    case APP_LED_MODE_INVOKING_VOICE_SKILLS_IN_TEAMS_APP:
    case APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS:
    case APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT:
    case APP_LED_MODE_TEAMS_IN_CALL:
    case APP_LED_MODE_TEAMS_STATUS_MUTE:
    case APP_LED_MODE_TEAMS_STATUS_UNMUTE:

    case APP_LED_MODE_DISCONNECTED:
    case APP_LED_MODE_CONNECTED:
    case APP_LED_MODE_PARING:
        {
            if (bt_mode != DONGLE_BT_MODE_LEGACY)
            {
                return;
            }
        }
        break;
#endif
    default:
        break;
    }

#ifdef LEGACY_BT_GENERAL
    app_led_check_teams_mode(&mode);
#endif

    app_led_next_mode = mode;
    app_led_change_mode();

#ifdef LEGACY_BT_GENERAL
    app_led_start_timer_for_teams();
#endif
}

bool led_is_all_keep_off(void)
{
    if ((app_led_setting[0].type != LED_TYPE_KEEP_OFF) ||
        (app_led_setting[1].type != LED_TYPE_KEEP_OFF) ||
        (app_led_setting[2].type != LED_TYPE_KEEP_OFF))
    {
        return false;
    }
    else
    {
        return true;
    }
}

void app_led_init(void)
{
    APP_PRINT_INFO0("app_led_init");

    app_led_pad_config();
    led_set_driver_mode();
    app_timer_reg_cb(app_led_timeout_cb, &app_led_timer_id);
}
