/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "rtl876x_pinmux.h"
#include "hal_gpio.h"
#include "trace.h"
#include "board.h"
#include "section.h"
#include "ringtone.h"
#include "gap_vendor.h"
#include "app_key_process.h"
#include "app_charger.h"
#include "app_led_ctl.h"
#include "app_mmi.h"
#include "app_main.h"
//#include "app_hfp.h"
#include "app_cfg.h"
#include "app_dlps.h"
//#include "app_relay.h"
#include "app_timer.h"
#include "remote.h"
//#include "single_tone.h"
//#include "app_roleswap.h"
//#include "app_audio_policy.h"
//#include "app_in_out_box.h"
#if F_APP_GPIO_ONOFF_SUPPORT
#include "app_gpio_on_off.h"
#endif
//#include "app_multilink.h"
#if (F_APP_AIRPLANE_SUPPORT == 1)
#include "app_airplane.h"
#endif
//#include "app_bt_policy_api.h"
//#include "app_auto_power_off.h"
#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif
//#include "wdg.h"

#define LONG_PRESS                          1
#define LONG_PRESS_POWER_ON                 2
#define LONG_PRESS_POWER_OFF                3
#define LONG_PRESS_ENTER_PAIRING            4
#define LONG_PRESS_FACTORY_RESET            5
#define LONG_PRESS_REPEAT                   6
#define BETWEEN_LONG_AND_ULTRA_LONG_PRESS   7
#define ULTRA_LONG_PRESS                    8
#define LONG_PRESS_FACTORY_RESET_RWS        9

#define KEY_TIMER_UNIT_MS                   100
#define KEY_IN_OUT_EAR_IMITATE              0

/* Hybrid key type */
typedef enum t_hybrid_key
{
    HYBRID_KEY_SHORT_PRESS,
    HYBRID_KEY_LONG_PRESS,
    HYBRID_KEY_ULTRA_LONG_PRESS,
    HYBRID_KEY_2_CLICK,
    HYBRID_KEY_3_CLICK,
    HYBRID_KEY_4_CLICK,
    HYBRID_KEY_5_CLICK,
    HYBRID_KEY_HALL_SWITCH_HIGH,
    HYBRID_KEY_HALL_SWITCH_LOW,
    HYBRID_KEY_COMBINEKEY_POWER_ONOFF = 9,

    HYBRID_KEY_6_CLICK,
    HYBRID_KEY_7_CLICK,
    HYBRID_KEY_TOTAL
} T_HYBRID_KEY;

/* App define IO timer type, such as key, uart etc. */
typedef enum
{
    APP_IO_TIMER_KEY_MULTI_CLICK,
    APP_IO_TIMER_KEY_LONG_PRESS,
    APP_IO_TIMER_KEY_LONG_PRESS_REPEAT,
    APP_IO_TIMER_KEY_POWER_ON_LONG_PRESS,
    APP_IO_TIMER_KEY_POWER_OFF_LONG_PRESS,
    APP_IO_TIMER_KEY_ENTER_PAIRING,
    APP_IO_TIMER_KEY_FACTORY_RESET,
    APP_IO_TIMER_KEY_ULTRA_LONG_PRESS,
    APP_IO_TIMER_KEY_SLIDE_SWITCH,
    APP_IO_TIMER_KEY_HALL_SWITCH_STABLE,
    APP_IO_TIMER_KEY_FACTORY_RESET_RWS
} T_APP_KEY_TIMER;

/* Key data for record key various status */
typedef struct
{
    uint8_t         mfb_key;                      /* MFB key (power on)*/
    uint8_t         pre_key;                /* previous key value */
    uint8_t         combine_key_bits;       /* record current hybrid press key value*/
    uint8_t         pre_combine_key_bits;   /* record previous hybrid press key value*/
    uint8_t         key_bit_mask;           /* key mask */
    uint8_t         key_long_pressed;       /* key long press category*/
    uint8_t         key_detected_num;       /* detecte key pressed num*/
    uint8_t         key_click;              /* click key num */
    uint8_t         key_action;             /* action of key value corresponding*/
    uint8_t         key_slide_switch_on;    /* current slide switch status*/
    uint8_t         key_enter_pairing;      /* long press enter pairing*/
    uint8_t         key_enter_factory_reset;    /* long press enter factory reset*/
    uint8_t         key_bit_mask_airplane;  /* key mask of airplane mode combine key*/
} T_KEY_DATA;

/* Key check parameters */
typedef struct
{
    uint8_t         key;                /* key Mask */
    uint8_t         key_pressed;        /* key action KEY_RELEASE or  KEY_PRESS*/
} T_KEY_CHECK;

static bool is_power_on;
static bool is_key_volume_set;
static bool disallow_sync_play_vol_changed_tone = false;
static uint8_t key_timer_id = 0;

static uint8_t timer_idx_key_multi_click = 0;
static uint8_t timer_idx_key_long_press = 0;
static uint8_t timer_idx_key_long_repeat = 0;
static uint8_t timer_idx_key_power_on_long_press = 0;
static uint8_t timer_idx_key_power_off_long_press = 0;
static uint8_t timer_idx_key_enter_pairing = 0;
static uint8_t timer_idx_key_factory_reset = 0;
static uint8_t timer_idx_key_ultra_long_press = 0;
static uint8_t timer_idx_key_factory_reset_rws = 0;

static T_KEY_DATA key_data = {0};           /**<record key variable */

/*  Clear key parameters.
 * After perform the corresponding action,app will call this function to clear key parameters.
 */
static void key_clear(void)
{
    key_data.pre_key = KEY_NULL;
    key_data.key_click = 0;
    key_data.pre_combine_key_bits = 0;
}

/*
 * Find out the key index by key mask.
 * For example: The key index 0 correspond to key0, such as KEY0_MASK, KEY1_MASK etc.
 */
static uint8_t key_search_index(uint8_t key)
{
    uint8_t index;

    switch (key)
    {
    case KEY0_MASK:
        index = 0;
        break;

    case KEY1_MASK:
        index = 1;
        break;

    case KEY2_MASK:
        index = 2;
        break;

    case KEY3_MASK:
        index = 3;
        break;

    case KEY4_MASK:
        index = 4;
        break;

    case KEY5_MASK:
        index = 5;
        break;

    case KEY6_MASK:
        index = 6;
        break;

    case KEY7_MASK:
        index = 7;
        break;

    default:
        index = 0;
        break;
    }

    return index;
}
/**
    * @brief  Depending on the current connection state, single or multi-spk mode,
    *         app hanldes the corresponding procedures.
    * @param  action The MMI action to be executed.
    * @return void
    */
void key_execute_action(uint8_t action)
{
    app_mmi_handle_action(action);
}

/*
 * App process single key action include key long press and short press.
 * When app process the specific key that bbpro assign those key,
 * app will call ringtone_play() to push specific ringtone.
 * After app read key action from key table and perform the corresponding action,
 * then will call key_clear().
 * key mask, such as KEY0_MASK, KEY1_MASK etc.
 */
static void key_single_click(uint8_t key)
{
    uint8_t long_press = 0;

    if (is_power_on == true) //APP in non-off state
    {
        if (key_data.key_long_pressed != 0)
        {
            long_press = 1;
        }
        key_data.key_action =
            app_cfg_const.key_table[long_press][app_dongle_get_status()][key_search_index(key)];

#if F_APP_LEA_DONGLE_BINDING
        if (key == KEY1_MASK)
        {
            if (long_press)
            {
                //key1 long press always be cis scan.
                key_data.key_action = MMI_BT_CIS_SCAN;
            }
            else
            {
                //key1 short press always be cis recnnect used for cancel pairing.
                key_data.key_action = MMI_BT_CIS_RECONNECT;
            }
        }
#endif

        key_execute_action(key_data.key_action);
    }

    key_clear();
}

/**
    * @brief  App process hybrid key action include key long press and short press.
    *         When app process the specific key that bbpro assign those key,
    *         app will call au_push_ringtone() to push specific ringtone.
    *         After app read key action from key table and perform the corresponding action,
    *         then will call key_clear().
    * @param  key key mask, such as KEY0_MASK, KEY1_MASK etc.
    * @return void
    */
static void key_hybrid_click(uint8_t key)
{
    uint8_t i;
    uint8_t hybrid_type = HYBRID_KEY_SHORT_PRESS;
    bool is_only_allow_factory_reset = false;

    APP_PRINT_INFO3("key_hybrid_click: is_power_on = %d, enable_factory_reset_when_in_the_box = %d  in box = %d",
                    is_power_on, app_cfg_const.enable_factory_reset_when_in_the_box == 1, app_device_is_in_the_box());

    if ((is_power_on == false) && (app_cfg_const.enable_factory_reset_when_in_the_box == 1) &&
        app_device_is_in_the_box())
    {
        /* power off, only allow factory reset MMI */
        is_only_allow_factory_reset = true;
    }

    if (is_power_on || is_only_allow_factory_reset)
    {
        if (key_data.key_long_pressed == ULTRA_LONG_PRESS)
        {
            hybrid_type = HYBRID_KEY_ULTRA_LONG_PRESS;
        }
        else if (key_data.key_long_pressed)
        {
            hybrid_type = HYBRID_KEY_LONG_PRESS;
        }
        else if (key_data.key_click >= 2)
        {
            hybrid_type = key_data.key_click + 1;
            if (key_data.key_click >= 6)
            {
                hybrid_type = key_data.key_click + 4;
            }
        }

        for (i = 0; i < HYBRID_KEY_NUM; i++)
        {
            if ((app_cfg_const.hybrid_key_mapping[i][0] == key) &&
                (app_cfg_const.hybrid_key_mapping[i][1] == hybrid_type))
            {
                uint8_t action = app_cfg_const.hybrid_key_table[app_dongle_get_status()][i];

                if (action == MMI_DEV_FACTORY_RESET || is_power_on)
                {
                    key_data.key_action = action;
                    key_execute_action(key_data.key_action);
                }

                break;
            }
        }
    }

    key_clear();
}

/**
    * @brief  Register timer callback to gap layer. Called by app when gap timer is timeout.
    * @note
    * @param  timer_evt distinguish io timer type
    * @param  param indicate which key
    * @return void
    */
static void app_key_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_key_timeout_cb: timer_evt 0x%02x, param %d",
                     timer_evt, param);

    switch (timer_evt)
    {
    case APP_IO_TIMER_KEY_MULTI_CLICK:
        {
            if ((key_data.key_click == 1) && (key_data.pre_combine_key_bits == 0))
            {
                key_single_click(param);
            }
            else
            {
                key_hybrid_click(param);
            }

            app_stop_timer(&timer_idx_key_multi_click);
        }
        break;

    case APP_IO_TIMER_KEY_LONG_PRESS:
        {
            //uint8_t index = 0;
            app_stop_timer(&timer_idx_key_long_press);
            key_data.key_long_pressed = LONG_PRESS;

            if (key_data.combine_key_bits == 0)
            {
                key_single_click(param);
            }
            else
            {
                key_hybrid_click(param);
            }

            if ((app_cfg_const.key_long_press_repeat_interval != 0) &&
                ((app_cfg_const.key_long_press_repeat_mask & param) == param))
            {
                //Check long pressed key repeat
                app_start_timer(&timer_idx_key_long_repeat, "long_press_repeat",
                                key_timer_id, APP_IO_TIMER_KEY_LONG_PRESS_REPEAT, param, false,
                                app_cfg_const.key_long_press_repeat_interval * KEY_TIMER_UNIT_MS);
            }
        }
        break;

    case APP_IO_TIMER_KEY_LONG_PRESS_REPEAT:
        {
            app_stop_timer(&timer_idx_key_long_repeat);

            //Check long pressed key repeat
            app_start_timer(&timer_idx_key_long_repeat, "long_press_repeat",
                            key_timer_id, APP_IO_TIMER_KEY_LONG_PRESS_REPEAT, param, false,
                            app_cfg_const.key_long_press_repeat_interval * KEY_TIMER_UNIT_MS);

            key_data.key_long_pressed = LONG_PRESS_REPEAT;

            if (key_data.combine_key_bits == 0)
            {
                key_single_click(param);
            }
            else
            {
                key_hybrid_click(param);
            }
        }
        break;

    case APP_IO_TIMER_KEY_POWER_ON_LONG_PRESS:
        {
            app_stop_timer(&timer_idx_key_power_on_long_press);

            if (app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                app_led_set_mode(APP_LED_MODE_POWER_ON);
                app_db.ignore_led_mode = true;

                key_data.key_long_pressed = LONG_PRESS_POWER_ON;
            }
            else
            {
                app_stop_timer(&timer_idx_key_enter_pairing);
                app_stop_timer(&timer_idx_key_factory_reset);
                app_stop_timer(&timer_idx_key_factory_reset_rws);
            }
        }
        break;

    case APP_IO_TIMER_KEY_POWER_OFF_LONG_PRESS:
        {
            app_stop_timer(&timer_idx_key_power_off_long_press);
            key_data.key_long_pressed = LONG_PRESS_POWER_OFF;
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED) //single mode
            {
                app_mmi_handle_action(MMI_DEV_POWER_OFF);
            }
        }
        break;

    case APP_IO_TIMER_KEY_ENTER_PAIRING:
        {
            key_data.key_long_pressed = LONG_PRESS_ENTER_PAIRING;
            key_data.key_enter_pairing = 1;
            app_stop_timer(&timer_idx_key_enter_pairing);
        }
        break;

    case APP_IO_TIMER_KEY_FACTORY_RESET:
        {
            app_led_set_mode(APP_LED_MODE_FACTORY_RESET);
            app_db.ignore_led_mode = true;

            key_data.key_long_pressed = LONG_PRESS_FACTORY_RESET;
            key_data.key_enter_pairing = 0;
            key_data.key_enter_factory_reset = 1;
            app_stop_timer(&timer_idx_key_factory_reset);
        }
        break;

    case APP_IO_TIMER_KEY_FACTORY_RESET_RWS:
        {
            app_led_set_mode(APP_LED_MODE_FACTORY_RESET);
            app_db.ignore_led_mode = true;

            key_data.key_long_pressed = LONG_PRESS_FACTORY_RESET_RWS;
            key_data.key_enter_pairing = 0;
            key_data.key_enter_factory_reset = 1;
            app_stop_timer(&timer_idx_key_factory_reset_rws);
        }
        break;
    default:
        break;
    }
}

static void app_key_start_factory_reset_timer(void)
{
    if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE) && //Long press power factory reset rws
        (app_cfg_const.key_factory_reset_rws_interval != 0) &&
        ((app_cfg_const.key_disable_power_on_off == 0) || (app_cfg_nv.factory_reset_done == 0)))
    {
        app_start_timer(&timer_idx_key_factory_reset_rws, "key_factory_reset_rws",
                        key_timer_id, APP_IO_TIMER_KEY_FACTORY_RESET_RWS, 0, false,
                        app_cfg_const.key_factory_reset_rws_interval * KEY_TIMER_UNIT_MS);
    }

    if ((app_cfg_const.key_factory_reset_interval != 0) &&
        ((app_cfg_const.key_disable_power_on_off == 0) ||
         (app_cfg_nv.factory_reset_done == 0))) //Long press power factory reset
    {
        app_start_timer(&timer_idx_key_factory_reset, "key_factory_reset",
                        key_timer_id, APP_IO_TIMER_KEY_FACTORY_RESET, 0, false,
                        app_cfg_const.key_factory_reset_interval * KEY_TIMER_UNIT_MS);
    }
}

static void app_key_stop_timer(void)
{
    app_stop_timer(&timer_idx_key_power_on_long_press);
    app_stop_timer(&timer_idx_key_power_off_long_press);
    app_stop_timer(&timer_idx_key_enter_pairing);
    app_stop_timer(&timer_idx_key_factory_reset);
    app_stop_timer(&timer_idx_key_factory_reset_rws);
    app_stop_timer(&timer_idx_key_long_press);
    app_stop_timer(&timer_idx_key_long_repeat);
    app_stop_timer(&timer_idx_key_ultra_long_press);
}

/**
    * @brief  App check status of key press or release in this function.
    *         The major content is divided into two parts to deal with, including  key press and key release.
    *         And app start gap timer to judge key action status, such as LONG_PRESS,
    *         LONG_PRESS_POWER_ON, LONG_PRESS_POWER_OFF, LONG_PRESS_ENTER_PAIRING etc.
    * @param  key_check this struct include key press status and key mask.
    * @return void
    */
static void key_check_press(T_KEY_CHECK key_check)
{
    uint8_t key = key_check.key;
    uint8_t i;

    APP_PRINT_TRACE4("key_check_press: key mask %02x, key_pressed %d, pre_clicks %d, long %d",
                     key, key_check.key_pressed, key_data.key_click, key_data.key_long_pressed);

    if (key_check.key_pressed == KEY_PRESS)
    {
        key_data.key_bit_mask |= key;
        key_data.key_detected_num++;

#if (F_APP_AIRPLANE_SUPPORT == 1)
        key_data.key_bit_mask_airplane = 0;
#endif
        for (i = 0; i < HYBRID_KEY_NUM; i++)
        {
            if ((app_cfg_const.hybrid_key_mapping[i][0] == key_data.key_bit_mask) &&
                (app_cfg_const.hybrid_key_mapping[i][1] == HYBRID_KEY_COMBINEKEY_POWER_ONOFF))
            {
                key_data.combine_key_bits = key_data.key_bit_mask;

#if (F_APP_AIRPLANE_SUPPORT == 1)
                if (app_cfg_const.hybrid_key_table[0][i] == MMI_AIRPLANE_MODE)
                {
                    key_data.key_bit_mask_airplane = key_data.key_bit_mask;
                }
#endif
                break;
            }
        }

        if ((key == key_data.mfb_key) && (key_data.key_detected_num == 1))
        {
            app_dlps_disable(APP_DLPS_ENTER_CHECK_MFB_KEY);
            if (is_power_on == false) //Power on press, or Slide switch ON
            {
#if 0
                if (app_cfg_const.discharger_support)
                {
                    T_CHARGER_STATE charger_state;
                    uint8_t state_of_charge;
                }
#endif
                if ((app_cfg_const.key_power_on_interval != 0) &&
                    ((app_cfg_const.key_disable_power_on_off == 0) ||
                     (app_cfg_nv.factory_reset_done == 0))) //Long press power on
                {
                    app_start_timer(&timer_idx_key_power_on_long_press, "key_pw_on_long",
                                    key_timer_id, APP_IO_TIMER_KEY_POWER_ON_LONG_PRESS, key, false,
                                    app_cfg_const.key_power_on_interval * KEY_TIMER_UNIT_MS);
                }

                if ((app_cfg_const.key_enter_pairing_interval != 0) &&
                    ((app_cfg_const.key_disable_power_on_off == 0) ||
                     (app_cfg_nv.factory_reset_done == 0))) //Long press power enter pairing
                {
                    app_start_timer(&timer_idx_key_enter_pairing, "key_enter_pairing",
                                    key_timer_id, APP_IO_TIMER_KEY_ENTER_PAIRING, key, false,
                                    app_cfg_const.key_enter_pairing_interval * KEY_TIMER_UNIT_MS);
                }
                if ((app_cfg_const.enable_factory_reset_when_in_the_box == 0) ||
                    app_device_is_in_the_box())
                {
                    app_key_start_factory_reset_timer();
                }
            }
            else //Power off press
            {
                if ((app_cfg_const.key_power_off_interval != 0) &&
                    ((app_cfg_const.key_disable_power_on_off == 0) ||
                     (app_cfg_nv.factory_reset_done == 0))) //Long press power off
                {
                    app_start_timer(&timer_idx_key_power_off_long_press, "key_pw_off_long",
                                    key_timer_id, APP_IO_TIMER_KEY_POWER_OFF_LONG_PRESS, key, false,
                                    app_cfg_const.key_power_off_interval * KEY_TIMER_UNIT_MS);
                }

                if (app_cfg_const.key_long_press_interval != 0)
                {
                    app_start_timer(&timer_idx_key_long_press, "key_long",
                                    key_timer_id, APP_IO_TIMER_KEY_LONG_PRESS, key, false,
                                    app_cfg_const.key_long_press_interval * KEY_TIMER_UNIT_MS);
                }
            }
        }
        else
        {
            //Combine Key pressed
            if (key_data.key_detected_num >= 2)
            {
                key = key_data.key_bit_mask;
                key_data.combine_key_bits = key_data.key_bit_mask;
                app_key_stop_timer();
            }

            if (app_cfg_const.key_long_press_interval != 0)
            {
                app_start_timer(&timer_idx_key_long_press, "key_long",
                                key_timer_id, APP_IO_TIMER_KEY_LONG_PRESS, key, false,
                                app_cfg_const.key_long_press_interval * KEY_TIMER_UNIT_MS);
            }
        }
    }
    else //Key release
    {
        key_data.key_bit_mask &= ~key;

        //fix hall switch not power off issue
        if (key_data.key_detected_num == 0)
        {
            key_data.key_detected_num = 0;
        }
        else
        {
            key_data.key_detected_num--;
        }

        if (key_data.combine_key_bits)
        {
            if (key_data.key_detected_num != 0)
            {
                //once combine key was pressed, detect key/action after all key release
                app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                return;
            }
            else
            {
                key = key_data.combine_key_bits;
            }
        }

        app_key_stop_timer();

        if (key == key_data.mfb_key)
        {
            app_dlps_enable(APP_DLPS_ENTER_CHECK_MFB_KEY);
            if (is_power_on == false) //Power on release, or Slide switch OFF
            {
                if (app_cfg_const.discharger_support)
                {
                    T_CHARGER_STATE charger_state;
                    uint8_t state_of_charge;

                    charger_state = charger_api_get_charger_state();
                    state_of_charge = charger_api_get_state_of_charge();
                    if ((charger_state == STATE_CHARGER_END) && (state_of_charge == BAT_CAPACITY_0))
                    {
                        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                        return;
                    }
                }

                if (app_cfg_const.key_power_on_interval != 0) //Long press power on
                {
                    if (key_data.key_long_pressed == LONG_PRESS_POWER_ON)
                    {
                        app_mmi_handle_action(MMI_DEV_POWER_ON);
                    }
                }
                else
                {
                    // Before factory reset, allow long press MFB to power on and enter pairing when key_disable_power_on_off
                    if (key_data.key_long_pressed == 0 &&
                        ((app_cfg_const.key_disable_power_on_off == 0) ||
                         (app_cfg_nv.factory_reset_done == 0))) //Short press power on
                    {
                        // app_audio_tone_play(app_cfg_const.tone_power_on, false, false);
                        app_mmi_handle_action(MMI_DEV_POWER_ON);
                    }
                }

                if ((app_cfg_const.key_factory_reset_interval != 0) &&
                    ((app_cfg_const.key_disable_power_on_off == 0) ||
                     (app_cfg_nv.factory_reset_done == 0)))//Long press power factory reset
                {
                    if ((key_data.key_long_pressed == LONG_PRESS_FACTORY_RESET) ||
                        (key_data.key_long_pressed == LONG_PRESS_FACTORY_RESET_RWS))
                    {
                        if (key_data.key_long_pressed == LONG_PRESS_FACTORY_RESET)
                        {
                            app_mmi_handle_action(MMI_DEV_FACTORY_RESET);
                        }
                    }
                }

                key_data.key_enter_pairing = 0;
                key_data.key_enter_factory_reset = 0;
                key_data.key_long_pressed = 0;
                app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                if (app_cfg_const.enable_factory_reset_when_in_the_box == 1)
                {
                    goto hybrid;
                }

                if (key_data.combine_key_bits)
                {
                    goto exit;
                }
                return;
            }
            else //Power off release, or Slide switch OFF
            {
                if (app_cfg_const.key_power_off_interval != 0) //Long press power off
                {
                    if (key_data.key_long_pressed == LONG_PRESS_POWER_OFF)
                    {
                        if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED) //single mode
                        {
                            is_power_on = false;
                        }
                        key_data.key_long_pressed = 0;
                        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                        return;
                    }
                    else if (key_data.key_long_pressed == LONG_PRESS ||
                             key_data.key_long_pressed == BETWEEN_LONG_AND_ULTRA_LONG_PRESS)
                    {
                        if (((app_cfg_const.key_trigger_long_press != 1) &&
                             (key_data.key_long_pressed == BETWEEN_LONG_AND_ULTRA_LONG_PRESS)))
                        {
                            key_single_click(key);
                        }
                        key_data.key_long_pressed = 0;
                        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);

                        return;
                    }
                }
                else if ((app_cfg_const.key_disable_power_on_off == 0) ||
                         (app_cfg_nv.factory_reset_done == 0)) //Short press power off
                {
                    app_mmi_handle_action(MMI_DEV_POWER_OFF);
                    is_power_on = false;
                    key_data.key_long_pressed = 0;
                    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                    return;
                }
            }
        }

hybrid:
        if (key_data.key_long_pressed == 0) //Short press
        {
            if (key_data.key_click == 0) //First key
            {
                key_data.key_click++;

                if (app_cfg_const.key_multi_click_interval != 0)
                {
                    app_start_timer(&timer_idx_key_multi_click, "key_multi_click",
                                    key_timer_id, APP_IO_TIMER_KEY_MULTI_CLICK, key, false,
                                    app_cfg_const.key_multi_click_interval * KEY_TIMER_UNIT_MS);
                }
                else
                {
                    if (key_data.combine_key_bits == 0) //Single Key press
                    {
                        key_single_click(key);
                    }
                    else
                    {
                        key_hybrid_click(key_data.combine_key_bits);
                    }
                }
            }
            else
            {
                if (key_data.pre_key == key) //Same key repeat click
                {
                    key_data.key_click++;

                    if (app_cfg_const.key_multi_click_interval != 0)
                    {
                        app_stop_timer(&timer_idx_key_multi_click);
                        app_start_timer(&timer_idx_key_multi_click, "key_multi_click",
                                        key_timer_id, APP_IO_TIMER_KEY_MULTI_CLICK, key, false,
                                        app_cfg_const.key_multi_click_interval * KEY_TIMER_UNIT_MS);
                    }
                }
                else //Different keys, process directly
                {
                    //Previous key
                    if ((key_data.key_click == 1) && (key_data.pre_combine_key_bits == 0))
                    {
                        key_single_click(key_data.pre_key);
                    }
                    else
                    {
                        key_hybrid_click(key_data.pre_key);
                    }

                    //Current key
                    if (key_data.combine_key_bits == 0)
                    {
                        key_single_click(key);
                    }
                    else
                    {
                        key_hybrid_click(key_data.combine_key_bits);
                    }

                    app_stop_timer(&timer_idx_key_multi_click);
                }
            }
        }
exit:
        if (key_data.key_bit_mask == 0) //All key release
        {
            key_data.pre_key = key;
            key_data.pre_combine_key_bits = key_data.combine_key_bits;
            key_data.key_long_pressed = 0;
            key_data.combine_key_bits = 0;
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_MFB_KEY);
        }
    }
}

void app_key_init(void)
{
    key_data.mfb_key = KEY0_MASK;

    app_timer_reg_cb(app_key_timeout_cb, &key_timer_id);
}

bool app_key_is_slide_switch_on(void)
{
    return (key_data.key_slide_switch_on == 1);
}

bool app_key_is_enter_pairing(void)
{
    return (key_data.key_enter_pairing == 1);
}

bool app_key_is_enter_factory_reset(void)
{
    return (key_data.key_enter_factory_reset == 1);
}

void app_key_set_volume_status(bool volume_status)
{
    is_key_volume_set = volume_status;
}

bool app_key_is_set_volume(void)
{
    return is_key_volume_set;
}

void app_key_set_power_on(bool on)
{
    is_power_on = on;
}


void app_key_set_sync_play_vol_changed_tone_disallow(bool disallow_sync)
{
    disallow_sync_play_vol_changed_tone = disallow_sync;
}

bool app_key_is_sync_play_vol_changed_tone_disallow(void)
{
    return disallow_sync_play_vol_changed_tone;
}

void app_key_single_click(uint8_t key)
{
    key_single_click(key);
}

void app_key_hybrid_multi_clicks(uint8_t key, uint8_t clicks)
{
    key_data.key_click = clicks;
    key_hybrid_click(key);
}

void app_power_onoff_combinekey_dlps_process(void)
{
    PAD_WAKEUP_POL_VAL wake_up_val;
    uint8_t i, j, tmp_combinekey;

    for (i = 0; i < HYBRID_KEY_NUM; i++)
    {
        if (app_cfg_const.hybrid_key_mapping[i][1] == HYBRID_KEY_COMBINEKEY_POWER_ONOFF)
        {
            tmp_combinekey = app_cfg_const.hybrid_key_mapping[i][0];
            for (j = 0; j < MAX_KEY_NUM; j++)
            {
                if (tmp_combinekey % 2 == 1)
                {
                    wake_up_val = (app_cfg_const.key_high_active_mask & BIT(j)) ? PAD_WAKEUP_POL_HIGH :
                                  PAD_WAKEUP_POL_LOW;
                    Pad_ControlSelectValue(app_cfg_const.key_pinmux[j], PAD_SW_MODE);
                    System_WakeUpPinEnable(app_cfg_const.key_pinmux[j], wake_up_val);
                }
                tmp_combinekey = tmp_combinekey >> 1;
            }
            break;
        }
    }
}

RAM_TEXT_SECTION
bool app_key_is_combinekey_power_on_off(uint8_t key)
{
    uint8_t i;
    bool ret = false;

    for (i = 0; i < HYBRID_KEY_NUM; i++)
    {
        if (app_cfg_const.hybrid_key_mapping[i][1] == HYBRID_KEY_COMBINEKEY_POWER_ONOFF)
        {
            if (app_cfg_const.hybrid_key_mapping[i][0] & BIT(key))
            {
                ret = true;
            }
        }
    }
    return ret;
}

RAM_TEXT_SECTION
bool app_key_is_gpio_combinekey_power_on_off(uint8_t gpio_index)
{
    uint8_t i;
    bool ret = false;

    for (i = 1; i < MAX_KEY_NUM; i++)
    {
        if (gpio_index == app_cfg_const.key_pinmux[i])
        {
            if (app_key_is_combinekey_power_on_off(i))
            {
                ret = true;
            }
        }
    }
    return ret;
}

void app_key_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    T_KEY_CHECK key_check = {0};

    key_check.key = io_driver_msg_recv->u.param >> 8;
    key_check.key_pressed = io_driver_msg_recv->u.param & 0xFF;

    key_check_press(key_check);
}
