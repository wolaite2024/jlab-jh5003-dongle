/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
//#include <stdbool.h>
#include "trace.h"
#include "app_timer.h"
//#include "platform_utils.h"
#include "app_cfg.h"
#include "app_mmi.h"
#include "app_main.h"
//#include "app_relay.h"

#define TIMER_AUTO_POWER_OFF 0x00

static uint8_t auto_power_off_timer_id = 0;
static uint8_t timer_idx_auto_power_off = 0;
static uint32_t poweroff_flag = 0;

static void app_auto_power_off_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_auto_power_off_timeout_cb: timer_evt 0x%02x, param 0x%04x",
                     timer_evt, param);

    switch (timer_evt)
    {
    case TIMER_AUTO_POWER_OFF:
        {
            app_stop_timer(&timer_idx_auto_power_off);

            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_mmi_handle_action(MMI_DEV_POWER_OFF);
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_auto_power_off_enable(uint32_t flag, uint16_t timeout)
{
    APP_PRINT_INFO3("app_auto_power_off_enable: curr flag 0x%08x, clear flag 0x%08x, timeout %u",
                    poweroff_flag, flag, timeout);

    poweroff_flag &= ~flag;

    if (poweroff_flag == 0)
    {
        if (timeout == 0)
        {
            app_stop_timer(&timer_idx_auto_power_off);
        }
        else
        {
            app_start_timer(&timer_idx_auto_power_off, "auto_power_off",
                            auto_power_off_timer_id, TIMER_AUTO_POWER_OFF, 0, false,
                            timeout * 1000);
        }
    }
}

void app_auto_power_off_disable(uint32_t flag)
{
    APP_PRINT_TRACE2("app_auto_power_off_disable: curr flag 0x%08x, set flag 0x%08x",
                     poweroff_flag, flag);

    poweroff_flag |= flag;

    if (poweroff_flag != 0)
    {
        app_stop_timer(&timer_idx_auto_power_off);
    }
}

void app_auto_power_off_init(void)
{
    poweroff_flag = 0;

    app_timer_reg_cb(app_auto_power_off_timeout_cb, &auto_power_off_timer_id);
}
