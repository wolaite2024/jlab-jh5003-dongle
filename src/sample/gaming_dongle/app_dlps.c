/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "board.h"
#include "rtl876x_gdma.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "rtl876x_uart.h"
#include "app_timer.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"
#include "app_dlps.h"
#include "app_led_ctl.h"
#include "app_main.h"
#include "section.h"
#include "app_key_process.h"
#include "app_key_gpio.h"
#include "app_charger.h"
#if F_APP_GPIO_ONOFF_SUPPORT
#include "app_gpio_on_off.h"
#endif
#include "app_cfg.h"
#include "app_mmi.h"
//#include "app_audio_policy.h"
#include "app_sensor.h"
#include "pm.h"
#include "app_auto_power_off.h"
#include "os_timer.h"
//#include "app_adp.h"
#include "system_status_api.h"
#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
#include "app_slide_switch.h"
#endif
#include "io_dlps.h"
#include "console_uart.h"
#include "os_sync.h"

#if F_APP_BB2_EXTERNAL_PA_SUPPORT
#include "gap_vendor.h"
#endif

#define APP_IO_TIMER_POWER_DOWN_WDG (0)
#define POWER_DOWN_WDG_TIMER     (500)
#define POWER_DOWN_WDG_CHK_TIMES (40)

static uint32_t dlps_bitmap;                /**< dlps locking bitmap */

static uint8_t app_dlps_timer_id = 0;
static uint8_t timer_idx_power_down_wdg = 0;

static uint32_t pd_wdg_chk_times = 0;
uint8_t mfb_value_enter_dlps = 0;

//extern void (*dlps_utils_print_wake_up_info)(void);     /* add print dlps wake up reason if needed */

RAM_TEXT_SECTION void app_dlps_enable(uint32_t bit)
{
    uint32_t s;
    if (dlps_bitmap & bit)
    {
        //APP_PRINT_TRACE3("app_dlps_enable: %08x %08x -> %08x", bit, dlps_bitmap,
        //                 (dlps_bitmap & ~bit));
    }
    s = os_lock();
    dlps_bitmap &= ~bit;
    os_unlock(s);
}

RAM_TEXT_SECTION void app_dlps_disable(uint32_t bit)
{
    uint32_t s;
    if (app_cfg_const.charger_control_by_mcu && (bit == APP_DLPS_ENTER_CHECK_CHARGER))
    {
        /* charger control by mcu */
        return;
    }
    if ((dlps_bitmap & bit) == 0)
    {
        //APP_PRINT_TRACE3("app_dlps_disable: %08x %08x -> %08x", bit, dlps_bitmap,
        //                 (dlps_bitmap | bit));
    }
    s = os_lock();
    dlps_bitmap |= bit;
    os_unlock(s);
}

RAM_TEXT_SECTION bool app_dlps_check_callback(void)
{
    static uint16_t dlps_bitmap_pre;
    bool dlps_enter_en = false;
    if ((app_cfg_const.enable_dlps) && (dlps_bitmap == 0))
    {
        dlps_enter_en = true;
    }

    if ((dlps_bitmap_pre != dlps_bitmap) && !dlps_enter_en && app_cfg_const.enable_dlps)
    {
        APP_PRINT_WARN2("app_dlps_check_callback: dlps_bitmap_pre 0x%x dlps_bitmap 0x%x", dlps_bitmap_pre,
                        dlps_bitmap);
    }
    dlps_bitmap_pre = dlps_bitmap;
    return dlps_enter_en;
}

void app_dlps_set_pad_wake_up(uint8_t pinmux,
                              PAD_WAKEUP_POL_VAL wake_up_val)
{
    Pad_ControlSelectValue(pinmux, PAD_SW_MODE);
    System_WakeUpPinEnable(pinmux, wake_up_val);
    System_WakeUpInterruptEnable(pinmux);
}

void app_dlps_restore_pad(uint8_t pinmux)
{

    Pad_ControlSelectValue(pinmux, PAD_PINMUX_MODE);
    System_WakeUpPinDisable(pinmux);

    if (System_WakeUpInterruptValue(pinmux) == 1)
    {
        P_GPIO_CBACK cb = NULL;
        uint32_t context = NULL;

        //Edge trigger will mis-detect when wake up
        //    APP_PRINT_INFO1("app_dlps_restore_pad gpio_handler pin= %s",
        //                    TRACE_STRING(Pad_GetPinName(pinmux)));
        hal_gpio_get_isr_callback(pinmux, &cb, &context);

        if (cb)
        {
            cb(context);
        }
    }
}

#if F_APP_BB2_EXTERNAL_PA_SUPPORT
bool app_dlps_is_support_external_pa(void)
{
    uint8_t *cfg = (uint8_t *)0x00280d44;

    if (*cfg & BIT(5))
    {
        return true;
    }
    else
    {
        return false;
    }
}
#endif

/**
    * @brief   Need to handle message in this callback function,when App enter dlps mode
    * @param  void
    * @return void
    */
void app_dlps_enter_callback(void)
{
    mfb_value_enter_dlps = key_get_mfb_state();
    POWERMode lps_mode = power_mode_get();
    uint32_t i;

    if ((POWER_POWERDOWN_MODE == lps_mode) || (POWER_SHIP_MODE == lps_mode))
    {
        DBG_DIRECT("app_dlps_enter_callback: lps_mode %d", lps_mode);
    }
    if (app_cfg_const.led_support == 1)
    {
        if (led_is_all_keep_off())
        {
            io_dlps_set_vio_power(false);
        }
        else
        {
            io_dlps_set_vio_power(true);  //Disable turn off LDO AUX1/2 for drive sleep led
        }
    }

    if (app_cfg_const.key_gpio_support)
    {
        if (app_cfg_const.key_enable_mask & KEY0_MASK)
        {
            PAD_WAKEUP_POL_VAL wake_up_val;

            if (app_cfg_const.key_high_active_mask & BIT0) // high active
            {
                wake_up_val = PAD_WAKEUP_POL_HIGH;
            }
            else // low active
            {
                wake_up_val = PAD_WAKEUP_POL_LOW;
            }

            Pad_ControlSelectValue(app_cfg_const.key_pinmux[0], PAD_SW_MODE);
            System_WakeUpPinEnable(app_cfg_const.key_pinmux[0], wake_up_val);
            System_WakeUpInterruptEnable(app_cfg_const.key_pinmux[0]);
        }
    }

    if (app_cfg_const.enable_data_uart)
    {
        console_uart_enter_low_power(lps_mode);
    }
#if F_APP_GPIO_ONOFF_SUPPORT
    if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        app_gpio_on_off_enter_dlps_pad_set();
    }
#endif
    if (lps_mode == POWER_DLPS_MODE)
    {
        if (app_db.device_state != APP_DEVICE_STATE_OFF)
        {
            if (app_cfg_const.key_gpio_support)
            {
                //Key1 ~ Key7 are allowed to wake up system in non-off state
                for (i = 1; i < MAX_KEY_NUM; i++)
                {
                    if (app_cfg_const.key_enable_mask & (1U << i))
                    {
                        Pad_ControlSelectValue(app_cfg_const.key_pinmux[i], PAD_SW_MODE);
                        if (app_cfg_const.key_high_active_mask & BIT(i))
                        {
                            // high active
                            System_WakeUpPinEnable(app_cfg_const.key_pinmux[i], PAD_WAKEUP_POL_HIGH);
                        }
                        else
                        {
                            // low active
                            System_WakeUpPinEnable(app_cfg_const.key_pinmux[i], PAD_WAKEUP_POL_LOW);
                        }
                        System_WakeUpInterruptEnable(app_cfg_const.key_pinmux[i]);
                    }
                }
            }
#if F_APP_LINEIN_SUPPORT
            if (app_cfg_const.line_in_support)
            {
                Pad_ControlSelectValue(app_cfg_const.line_in_pinmux, PAD_SW_MODE);
                System_WakeUpPinEnable(app_cfg_const.line_in_pinmux, PAD_WAKEUP_POL_LOW);
                System_WakeUpInterruptEnable(app_cfg_const.line_in_pinmux);
            }
#endif
        }
    }
    else if (power_mode_get() == POWER_POWERDOWN_MODE)
    {
        if (app_cfg_const.key_gpio_support)
        {
            for (i = 1; i < MAX_KEY_NUM; i++)
            {
                if (app_cfg_const.key_enable_mask & BIT(i) & !(app_key_is_combinekey_power_on_off(i)))
                {
                    System_WakeUpPinDisable(app_cfg_const.key_pinmux[i]);
                    System_WakeUpInterruptDisable(app_cfg_const.key_pinmux[i]);
                }
            }
        }
#if F_APP_LINEIN_SUPPORT
        if (app_cfg_const.line_in_support)
        {
            System_WakeUpPinDisable(app_cfg_const.line_in_pinmux);
            System_WakeUpInterruptDisable(app_cfg_const.line_in_pinmux);
        }
#endif
    }
}

extern void app_adp_det_handler(void);
void app_dlps_exit_callback(void)
{
#if F_APP_BB2_EXTERNAL_PA_SUPPORT
    if (app_dlps_is_support_external_pa())
    {
        uint8_t p_buf[3] = {0x0B, 0x0E, 0x0};
        gap_vendor_cmd_req(0xFC6E, sizeof(p_buf), p_buf);

        *((volatile uint32_t *)0x40058030) &= 0xFFFFFC00;
        *((volatile uint32_t *)0x40058030) |= 0xA0;

        *((volatile uint32_t *)0x400002A8) |= BIT(28);
        *((volatile uint32_t *)0x400002A8) &= 0xFFF1FFFF;
        *((volatile uint32_t *)0x400002A8) |= BIT(16);

        Pinmux_Config(P1_0, DIGI_DEBUG);
        Pinmux_Config(P1_1, DIGI_DEBUG);
        Pad_Config(P1_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
        Pad_Config(P1_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    }
#endif

    /* add print dlps wake up reason if needed */
    //dlps_utils_print_wake_up_info();
    //LPM_POWERDOWN_MODE and LPM_HIBERNATE_MODE will reboot directly and not execute exit callback
    // need to add power off mode
#if 0
    if (sys_hall_adp_read_adp_level() == 0)
    {
        app_adp_det_handler();
    }
#endif
    if ((power_mode_get() == POWER_DLPS_MODE))
    {
        if (app_cfg_const.key_gpio_support)
        {
            uint32_t i;

            for (i = 0; i < MAX_KEY_NUM; i++)
            {
                if (app_cfg_const.key_enable_mask & (1U << i))
                {
                    Pad_ControlSelectValue(app_cfg_const.key_pinmux[i], PAD_PINMUX_MODE);

                    //Key1 ~ Key5 are edge trigger. Handle key press directly
                    if ((i >= 1) && (System_WakeUpInterruptValue(app_cfg_const.key_pinmux[i]) == 1))
                    {
                        //Edge trigger will mis-detect when wake up
                        APP_PRINT_INFO0("app_dlps_exit_callback: key_gpio_support gpio_handler");
                        key_gpio_intr_callback(i);
                    }
                }
            }
        }
#if F_APP_LINEIN_SUPPORT
        if (app_cfg_const.line_in_support)
        {
            Pad_ControlSelectValue(app_cfg_const.line_in_pinmux, PAD_PINMUX_MODE);
            app_dlps_restore_pad(app_cfg_const.line_in_pinmux);
        }
#endif
        if (app_cfg_const.enable_data_uart)
        {
            console_uart_exit_low_power(POWER_DLPS_MODE);
        }
#if F_APP_GPIO_ONOFF_SUPPORT
        if (app_cfg_const.box_detect_method == GPIO_DETECT)
        {
            app_gpio_detect_onoff_exit_dlps_process();
        }
#endif
#if (MFB_KEY_SUPPORT == 1)
        if (!(app_cfg_const.key_enable_mask & KEY0_MASK))
        {
            /* IO uses level trigger & MFB DET uses both edge trigger */
            SoC_VENDOR->REG_LOW_PRI_INT_MODE |= BIT9;
            //  PERIPHINT->EDGE_MODE |= BIT9; //check later

        }
#endif
    }
}

static bool app_dlps_platform_pm_check(void)
{
    uint8_t platform_pm_error_code = power_get_error_code();
    APP_PRINT_INFO1("app_dlps_platform_pm_check, ERR Code:%d", platform_pm_error_code);
    return (platform_pm_error_code == PM_ERROR_WAKEUP_TIME);
}

static void app_dlps_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE3("app_dlps_timer_callback: timer_evt 0x%02x, param %d, chk_times:%d",
                     timer_evt, param, pd_wdg_chk_times);

    switch (timer_evt)
    {
    case APP_IO_TIMER_POWER_DOWN_WDG:
        {
            app_stop_timer(&timer_idx_power_down_wdg);
            pd_wdg_chk_times++;
            if (pd_wdg_chk_times == POWER_DOWN_WDG_CHK_TIMES)
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF);
                app_dlps_enable(0xFFFF);
            }

            if (app_dlps_platform_pm_check() && app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                pd_wdg_chk_times = 0;
                power_stop_all_non_excluded_timer();
                os_timer_dump();
            }
            else
            {
                if (app_db.device_state != APP_DEVICE_STATE_ON)
                {
                    app_start_timer(&timer_idx_power_down_wdg, "power_down_wdg",
                                    app_dlps_timer_id, APP_IO_TIMER_POWER_DOWN_WDG, 0, false,
                                    POWER_DOWN_WDG_TIMER);
                }
                else
                {
                    pd_wdg_chk_times = 0;
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_dlps_power_off(void)
{
    if (app_cfg_const.enable_power_off_to_dlps_mode)
    {
        power_mode_set(POWER_DLPS_MODE);
    }
    else
    {
        power_mode_set(POWER_POWERDOWN_MODE);
        app_start_timer(&timer_idx_power_down_wdg, "power_down_wdg",
                        app_dlps_timer_id, APP_IO_TIMER_POWER_DOWN_WDG, 0, false,
                        POWER_DOWN_WDG_TIMER);
        app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF);
    }
}

void app_dlps_stop_power_down_wdg_timer(void)
{
    pd_wdg_chk_times = 0;
    app_stop_timer(&timer_idx_power_down_wdg);
}

void app_dlps_start_power_down_wdg_timer(void)
{
    if (app_db.device_state != APP_DEVICE_STATE_ON)
    {
        app_start_timer(&timer_idx_power_down_wdg, "power_down_wdg",
                        app_dlps_timer_id, APP_IO_TIMER_POWER_DOWN_WDG, 0, false,
                        POWER_DOWN_WDG_TIMER);
    }
}

bool app_dlps_check_short_press_power_on(void)
{
    bool ret = false;

    //When use LPM_POWERDOWN_MODE,
    //system will re-boot after wake up and not execute DLPS exit callback
    if ((app_cfg_const.key_gpio_support) && (app_cfg_const.key_power_on_interval == 0))
    {
        if (System_WakeUpInterruptValue(app_cfg_const.key_pinmux[0]) == 1)
        {
            //GPIO INT not triggered before short click release MFB key
            //Use direct power on for short press power on case
            if (app_cfg_const.discharger_support)
            {
                T_CHARGER_STATE charger_state;
                uint8_t state_of_charge; //MUST be detected after task init

                charger_state = charger_api_get_charger_state();
                state_of_charge = charger_api_get_state_of_charge();
                if ((charger_state == STATE_CHARGER_END) && (state_of_charge == BAT_CAPACITY_0))
                {
                    /* led_set_mode(LED_MODE_LOW_BATTERY_DISABLE_POWER_ON); */
                    /* FIXME: call app_led_set_mode(XXX) */
                }
                else
                {
                    ret = true;
                }
            }
            else
            {
                ret = true;
            }
        }
    }
    APP_PRINT_INFO1("app_dlps_check_short_press_power_on: ret %d", ret);
    return ret;
}

void app_dlps_init(void)
{
    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    if (!app_cfg_const.enable_dlps)
    {
        return;
    }

    io_dlps_register();

    /* register of call back function */
    if (power_check_cb_register(app_dlps_check_callback) != 0)
    {
        APP_PRINT_ERROR0("app_dlps_init: dlps_check_cb_reg failed");
    }

    io_dlps_register_enter_cb(app_dlps_enter_callback);
    io_dlps_register_exit_cb(app_dlps_exit_callback);

    app_timer_reg_cb(app_dlps_timeout_cb, &app_dlps_timer_id);

    app_dlps_power_off();
}

