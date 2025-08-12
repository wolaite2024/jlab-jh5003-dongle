
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "rtl876x_pinmux.h"
#include "rtl876x_wdg.h"
#include "hal_gpio_int.h"
#include "hal_gpio.h"
#include "trace.h"
#include "sysm.h"
#include "bt_avrcp.h"
#include "app_timer.h"
#include "audio_track.h"
#include "app_cfg.h"
#include "app_io_msg.h"
#include "app_dlps.h"
#include "app_line_in.h"
#include "section.h"
#include "app_device.h"
#include "app_mmi.h"
//#include "app_a2dp.h"
#include "app_main.h"
//#include "app_hfp.h"
#include "app_auto_power_off.h"
#include "app_key_process.h"

#define APP_LINE_IN_DYNAMIC_CHECK_SUPPORT       0

#define APP_LINE_IN_DETECT_DEBOUNCE (0)

static bool is_playing = false;
static bool line_in_plug_in = false;
static uint8_t app_line_in_timer_id = 0;
static uint8_t timer_idx_line_in = 0;
//static bool a2dp_pause_flg = false;

static void app_line_in_handler(bool is_in)
{
    APP_PRINT_INFO1("app_line_in_handler %d", is_in);

    //disable irq not support dynamic change src
    hal_gpio_irq_disable(app_cfg_const.line_in_pinmux);
#if 0
    uint8_t app_idx = app_a2dp_get_active_idx();

    APP_PRINT_INFO2("app_line_in_handler: is_in %d, is_playing %d", is_in, is_playing);

    if (is_in)
    {
        if (!is_playing)
        {
            if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
            {
                return;
            }

            if ((app_db.br_link[app_idx].connected_profile & A2DP_PROFILE_MASK) &&
                (app_db.br_link[app_idx].connected_profile & AVRCP_PROFILE_MASK))
            {
                if (app_db.br_link[app_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    if (app_cfg_const.enable_aux_in_supress_a2dp)
                    {
                        key_execute_action(MMI_AV_PAUSE);
                        a2dp_pause_flg = true;
                    }
                    else
                    {
                        return;
                    }
                }
            }

            APP_PRINT_INFO0("app_line_in_handler, audio_analog_start");

            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_LINE_IN);

            audio_track_volume_set(app_db.line_in_track_handle, app_cfg_nv.line_in_gain_level);
            audio_track_start(app_db.line_in_track_handle);

            is_playing = true;
        }
    }
    else
    {
        if (is_playing)
        {
            is_playing = false;

            APP_PRINT_INFO0("app_line_in_handler, audio_analog_stop");

            audio_track_stop(app_db.line_in_track_handle);

            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_LINE_IN, app_cfg_const.timer_auto_power_off);

            if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
            {
                return;
            }

            if ((app_db.br_link[app_idx].connected_profile & A2DP_PROFILE_MASK) &&
                (app_db.br_link[app_idx].connected_profile & AVRCP_PROFILE_MASK))
            {
                if (a2dp_pause_flg)
                {
                    key_execute_action(MMI_AV_RESUME);
                }
            }
            a2dp_pause_flg = false;
        }
    }
#endif
}

static void app_line_in_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_LINE_IN_DETECT_DEBOUNCE:
        {
            uint8_t gpio_status = param;

            app_stop_timer(&timer_idx_line_in);

            if (0 == gpio_status)
            {
                app_line_in_handler(true);
            }
            else
            {
                app_line_in_handler(false);
            }

            app_dlps_enable(APP_DLPS_ENTER_CHECK_LINEIN);
        }
        break;

    default:
        break;
    }
}

void app_line_in_driver_init(void)
{
    /*gpio init*/
    hal_gpio_init_pin(app_cfg_const.line_in_pinmux, GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_UP);
    hal_gpio_set_up_irq(app_cfg_const.line_in_pinmux, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
    hal_gpio_register_isr_callback(app_cfg_const.line_in_pinmux, app_line_in_detect_intr_callback, 0);
    hal_gpio_irq_disable(app_cfg_const.line_in_pinmux);

    app_timer_reg_cb(app_line_in_timeout_cb, &app_line_in_timer_id);
    timer_idx_line_in = NULL;
}

void app_line_in_power_on_check(void)
{
//    a2dp_pause_flg = false;

#if APP_LINE_IN_DYNAMIC_CHECK_SUPPORT
    if (0 == hal_gpio_get_input_level(app_cfg_const.line_in_pinmux))
    {
        app_line_in_handler(true);
    }

    hal_gpio_irq_enable(app_cfg_const.line_in_pinmux);
#else
    if (hal_gpio_get_input_level(app_cfg_const.line_in_pinmux))
    {
        line_in_plug_in = false;
    }
    else
    {
        line_in_plug_in = true;
    }
#endif
    APP_PRINT_INFO2("app_line_in_power_on_check line_lin_pinmux %x, state %d",
                    app_cfg_const.line_in_pinmux, line_in_plug_in);
}

RAM_TEXT_SECTION
bool app_line_in_plug_in(void)
{
    return line_in_plug_in;
}

void app_line_in_power_off_check(void)
{
    APP_PRINT_INFO0("app_line_in_power_off_check");
    hal_gpio_irq_disable(app_cfg_const.line_in_pinmux);

    app_line_in_handler(false);
}

RAM_TEXT_SECTION void app_line_in_detect_intr_callback(uint32_t param)
{
    uint8_t pin_num;
    uint8_t gpio_status;
    T_IO_MSG gpio_msg;

    app_dlps_disable(APP_DLPS_ENTER_CHECK_LINEIN);

    pin_num = app_cfg_const.line_in_pinmux;

    gpio_status = hal_gpio_get_input_level(pin_num);

    APP_PRINT_INFO1("app_line_in_detect_intr_handler %d", gpio_status);

    if (GPIO_LEVEL_HIGH == gpio_status)
    {
        hal_gpio_irq_change_polarity(pin_num, GPIO_IRQ_ACTIVE_LOW);
    }
    else
    {
        hal_gpio_irq_change_polarity(pin_num, GPIO_IRQ_ACTIVE_HIGH);
    }

    /* Disable GPIO interrupt */
    hal_gpio_irq_disable(pin_num);

    gpio_msg.u.param = gpio_status;
    gpio_msg.type = IO_MSG_TYPE_GPIO;
    gpio_msg.subtype = IO_MSG_GPIO_LINE_IN;
    app_io_msg_send(&gpio_msg);

    /* Enable GPIO interrupt */
    hal_gpio_irq_enable(pin_num);
}

void app_line_in_detect_msg_handler(T_IO_MSG *io_driver_msg_recv)
{
    uint8_t gpio_status;

    gpio_status = io_driver_msg_recv->u.param;
    app_stop_timer(&timer_idx_line_in);
    app_start_timer(&timer_idx_line_in, "line_in",
                    app_line_in_timer_id, APP_LINE_IN_DETECT_DEBOUNCE, gpio_status, false,
                    500);
}

bool app_line_in_is_playing(void)
{
    return is_playing;
}

void app_line_in_call_status_update(bool is_idle)
{
    if (0 == hal_gpio_get_input_level(app_cfg_const.line_in_pinmux))
    {
        APP_PRINT_INFO1("app_line_in_call_status_update: is_idle %d", is_idle);

        if (is_idle)
        {
            app_line_in_handler(true);
        }
        else
        {
            app_line_in_handler(false);
        }
    }
}
