/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "app_cfg.h"
#include "rtl876x_pinmux.h"
#include "trace.h"
#include "app_timer.h"
#include "app_io_output.h"

static bool app_io_output_enable_10hz_pulse = false;
static uint8_t timer_idx_create_10hz_pulse = 0;
static uint8_t io_output_timer_id = 0;

/**  @brief  Define IO output time event */
typedef enum
{
    APP_IO_TIMER_CREATE_10HZ_PULSE,
} T_APP_IO_OUTPUT_TIMER;

static void app_io_output_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_io_output_timeout_cb: timer_evt 0x%02x, param %d",
                     timer_evt, param);

    switch (timer_evt)
    {
    case APP_IO_TIMER_CREATE_10HZ_PULSE:
        {
            static uint8_t current_state = 0;

            if (app_io_output_enable_10hz_pulse)
            {
                app_start_timer(&timer_idx_create_10hz_pulse, "create_10hz_pulse",
                                io_output_timer_id, APP_IO_TIMER_CREATE_10HZ_PULSE, 0, false,
                                50);


                current_state ^= 1;
                Pad_OutputControlValue(app_cfg_const.external_mcu_input_pinmux, (PAD_OUTPUT_VAL)current_state);
            }
            else
            {
                app_stop_timer(&timer_idx_create_10hz_pulse);

                Pad_OutputControlValue(app_cfg_const.external_mcu_input_pinmux, PAD_OUT_LOW);
                current_state = 0;
            }
        }
        break;

    default:
        break;
    }
}

void app_io_output_ctrl_ext_mcu_pin(uint8_t enable)
{
    APP_PRINT_TRACE1("app_io_output_ctrl_ext_mcu_pin ENABLE=%d", enable);

    if (enable)
    {
        if (app_io_output_enable_10hz_pulse == false)
        {
            app_io_output_enable_10hz_pulse = true;
            app_start_timer(&timer_idx_create_10hz_pulse, "create_10hz_pulse",
                            io_output_timer_id, APP_IO_TIMER_CREATE_10HZ_PULSE, 0, false,
                            50);
        }
    }
    else
    {
        app_io_output_enable_10hz_pulse = false;
    }
}

void app_io_output_init(void)
{
    app_timer_reg_cb(app_io_output_timeout_cb, &io_output_timer_id);
}


