/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "board.h"
#include "pm.h"
#include "dlps_util.h"
#include "system_status_api.h"
#include "section.h"
#include "os_timer.h"
#include "os_sync.h"
#include "io_dlps.h"
#include "console_uart.h"
#include "app_timer.h"
#include "app_dlps.h"
#include "app_led.h"
#include "app_main.h"
#include "hal_gpio.h"
#include "app_charger.h"
#include "app_cfg.h"
#include "app_mmi.h"
#include "app_sensor.h"
#include "app_charger.h"
#include "app_auto_power_off.h"
#include "app_adp.h"
#include "hal_adp.h"
#include "app_one_wire_uart.h"
#include "dlps_util.h"

#if F_APP_QDECODE_SUPPORT
#include "app_qdec.h"
#endif
#if F_APP_PERIODIC_WAKEUP
#include "rtl876x_rtc.h"
#include "hal_adp.h"
#endif

#if F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_GPIO_DETECT
#include "app_ext_mic_switch.h"
#endif

#if F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_ADC_DETECT
#include "app_adc.h"
#endif

#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
#include "app_gfps_finder.h"
#endif

typedef enum
{
    APP_TIMER_POWER_DOWN_WDG = 0x00,
    APP_TIMER_PROFILING_DLPS = 0x01,
} T_APP_DLPS_TIMER;

#define POWER_DOWN_WDG_TIMER        500
#define POWER_DOWN_WDG_CHK_TIMES    40
#define PROFILING_DLPS_TIMER_MS     20*1000

/*********************************************
BB2 must use io_dlps.h
*********************************************/
static uint32_t dlps_bitmap; /**< dlps locking bitmap */
static uint8_t app_dlps_timer_id = 0;
static uint8_t timer_idx_power_down_wdg = 0;
static uint8_t timer_idx_profiling_dlps = 0;
static uint32_t pd_wdg_chk_times = 0;

#if F_APP_PERIODIC_WAKEUP
static bool app_dlps_need_to_wakeup_by_rtc(void)
{
    bool ret = false;

    /* for smart charger control, get 0% should not wakeup recharge ==> this condition is same to judge adp in when enter dlps */
    T_ADP_STATE adp_state = adp_get_current_state(ADP_DETECT_5V);
    if (adp_state == ADP_STATE_IN)
    {
        ret = true;
    }

    return ret;
}

static uint32_t app_dlps_get_system_wakeup_time(void)
{
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support &&
        !extend_app_cfg_const.disable_finder_adv_when_power_off)
    {
        uint32_t wakeup_time = extend_app_cfg_const.gfps_power_off_start_finder_adv_timer_timeout_value;
    }
    else
#endif
    {
        // uint32_t wakeup_time = 2 * 24 * 60 * 60; /* 2days */
        uint32_t wakeup_time = 60; /* 60s */
    }

    return wakeup_time;
}

static void app_dlps_system_wakeup_by_rtc(uint32_t wakeup_time)
{
    uint8_t comparator_index = COMP0GT_INDEX;
    uint32_t prescaler_value = RTC_PRESCALER_VALUE; /* 1 counter : (prescaler_value + 1)/32000  sec*/

    uint32_t comparator_value = (uint32_t)(((uint64_t)wakeup_time * 32000) / (prescaler_value + 1));

    RTC_DeInit();
    RTC_SetPrescaler(prescaler_value);

    RTC_CompINTConfig(RTC_CMP0GT_INT, ENABLE);

    RTC_SystemWakeupConfig(ENABLE);
    RTC_RunCmd(ENABLE);

    uint32_t current_value = 0;

    current_value = RTC_GetCounter();
    RTC_SetComp(comparator_index, current_value + comparator_value);
}

void app_dlps_system_wakeup_clear_rtc_int(void)
{
    if (RTC_GetINTStatus(RTC_CMP0GT_INT) == SET)
    {
        RTC_ClearINTStatus(RTC_CMP0GT_INT);
    }

    if (0)
    {
        // Debug
        uint32_t rtc_counter = 0;
        rtc_counter = RTC_GetCounter();
        APP_PRINT_INFO1("app_dlps_system_wakeup_clear_rtc_int: RTC wakeup, rtc_counter %d", rtc_counter);
    }

    // RTC_RunCmd(DISABLE);
}
#endif

RAM_TEXT_SECTION void app_dlps_enable(uint32_t bit)
{
    uint32_t s;

    if (dlps_bitmap & bit)
    {
        APP_PRINT_TRACE3("app_dlps_enable: %08x %08x -> %08x", bit, dlps_bitmap,
                         (dlps_bitmap & ~bit));
    }

    s = os_lock();
    dlps_bitmap &= ~bit;
    os_unlock(s);
}

RAM_TEXT_SECTION void app_dlps_disable(uint32_t bit)
{
    uint32_t s;

    if ((dlps_bitmap & bit) == 0)
    {
        APP_PRINT_TRACE3("app_dlps_disable: %08x %08x -> %08x", bit, dlps_bitmap,
                         (dlps_bitmap | bit));
    }

    s = os_lock();
    dlps_bitmap |= bit;
    os_unlock(s);
}

RAM_TEXT_SECTION bool app_dlps_check_callback(void)
{
    static uint32_t dlps_bitmap_pre;
    bool dlps_enter_en = false;
    POWERMode lps_mode = power_mode_get();
    bool is_keep_hq = false;

#if F_APP_SENSOR_IQS773_873_SUPPORT
    if (app_cfg_const.psensor_support &&
        ((app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS773) ||
         (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS873)))
    {
        if (lps_mode == POWER_DLPS_MODE)
        {
            //iqs773 need voltage ripple less than 100mv
            is_keep_hq = true;
        }
    }
#endif

    if (app_cfg_const.led_support == 1)
    {
        if (!app_led_is_all_keep_off())
        {
            //Disable turn off LDO AUX1/2 for drive sleep led
            is_keep_hq = true;
        }
    }

    io_dlps_set_vio_power(is_keep_hq);

#if F_APP_PERIODIC_WAKEUP
    if (lps_mode == POWER_POWERDOWN_MODE && dlps_enter_en)
    {
        extern void (*set_clk_32k_power_in_powerdown)(bool);
        set_clk_32k_power_in_powerdown(true);

#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support &&
            !extend_app_cfg_const.disable_finder_adv_when_power_off)
        {
            //save some information into flash
            app_gfps_finder_rtc_wakeup_save_info();
        }
#endif
    }
#endif

    if ((app_cfg_const.enable_dlps) && (dlps_bitmap == 0))
    {
        dlps_enter_en = true;
    }

    if ((dlps_bitmap_pre != dlps_bitmap) && !dlps_enter_en)
    {
        APP_PRINT_WARN2("app_dlps_check_callback: dlps_bitmap_pre 0x%x dlps_bitmap 0x%x", dlps_bitmap_pre,
                        dlps_bitmap);
    }

    dlps_bitmap_pre = dlps_bitmap;

    return dlps_enter_en;
}

/**
    * @brief   Need to handle message in this callback function,when App enter dlps mode
    * @param  void
    * @return void
    */
void app_dlps_enter_callback(void)
{
    POWERMode lps_mode = power_mode_get();
    uint32_t i;

    if ((lps_mode == POWER_POWERDOWN_MODE) || (lps_mode == POWER_SHIP_MODE))
    {
        DBG_DIRECT("app_dlps_enter_callback: lps_mode %d", lps_mode);

        if (app_cfg_const.led_support)
        {
            app_led_reset_pad_config();
        }

#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support &&
            !extend_app_cfg_const.disable_finder_adv_when_power_off)
        {
            //save some information into flash
            app_gfps_finder_rtc_wakeup_save_info();
        }
#endif
    }

#if F_APP_PERIODIC_WAKEUP
    if (lps_mode == POWER_POWERDOWN_MODE)
    {
        uint32_t wakeup_time = app_dlps_get_system_wakeup_time();

        app_dlps_system_wakeup_by_rtc(wakeup_time);

        DBG_DIRECT("app_dlps_system_wakeup_by_rtc: %d sec", wakeup_time);
    }
#endif

    if (app_cfg_const.enable_data_uart)
    {
        console_uart_enter_low_power(lps_mode);
    }

#if F_APP_ONE_WIRE_UART_SUPPORT
    if (app_cfg_const.one_wire_uart_support)
    {
        if (one_wire_state == ONE_WIRE_STATE_IN_ONE_WIRE)
        {
            console_uart_enter_low_power(lps_mode);
        }
        else
        {
            app_adp_io_wakeup_pol_set();
        }
    }
#endif

    if (lps_mode == POWER_DLPS_MODE)
    {
        if (app_db.device_state != APP_DEVICE_STATE_OFF)
        {
#if F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_GPIO_DETECT
            app_ext_mic_gpio_detect_enter_dlps_pad_set();
#endif

#if F_APP_QDECODE_SUPPORT
            if (app_cfg_const.wheel_support)
            {
                app_qdec_pad_enter_dlps_config();
            }
#endif
        }
    }
    else if (power_mode_get() == POWER_POWERDOWN_MODE)
    {

#if F_APP_QDECODE_SUPPORT
        if (app_cfg_const.wheel_support)
        {
            app_qdec_enter_power_down_cfg();
        }
#endif

    }

    if (power_mode_get() == POWER_POWERDOWN_MODE ||
        power_mode_get() == POWER_DLPS_MODE && app_db.device_state == APP_DEVICE_STATE_OFF)
    {
#if F_APP_SENSOR_SUPPORT
        if (app_cfg_const.chip_supply_power_to_light_sensor_digital_VDD)
        {
            /* Pull low I2C pin to prevent current leakage */
            Pad_Config(app_cfg_const.sensor_detect_pinmux,
                       PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
            Pad_Config(app_cfg_const.i2c_0_clk_pinmux,
                       PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
            Pad_Config(app_cfg_const.i2c_0_dat_pinmux,
                       PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }
#endif
#if F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_ADC_DETECT
        /* config pin after device off */
        Pad_Config(BOOM_IN_ADC_DETECT_PIN,
                   PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
#endif
    }

    if (app_cfg_const.smart_charger_control)
    {
        /* after get zero box vol; the 5v will drop */
        if (app_db.disallow_adp_out_auto_power_on)
        {
            adp_wake_up_enable(ADP_WAKE_UP_GENERAL, ADP_WAKE_UP_POL_HIGH);
        }
    }
}

void app_dlps_exit_callback(void)
{
    /* add print dlps wake up reason if needed */
    //dlps_utils_print_wake_up_info();

    //POWER_POWERDOWN_MODE and LPM_HIBERNATE_MODE will reboot directly and not execute exit callback
    if ((power_mode_get() == POWER_DLPS_MODE))
    {
#if F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_GPIO_DETECT
        app_ext_mic_gpio_detect_exit_dlps_pad_set();
#endif

        if (app_cfg_const.enable_data_uart)
        {
            console_uart_exit_low_power(POWER_DLPS_MODE);
        }

#if F_APP_ONE_WIRE_UART_SUPPORT
        if (app_cfg_const.one_wire_uart_support)
        {
            if (one_wire_state == ONE_WIRE_STATE_IN_ONE_WIRE)
            {
                console_uart_exit_low_power(POWER_DLPS_MODE);
            }
            else
            {
                app_adp_io_wakeup_pol_set();
            }
        }
#endif

#if F_APP_QDECODE_SUPPORT
        if (app_cfg_const.wheel_support)
        {
            if (app_cfg_const.qdec_y_pha_pinmux != 0xFF && app_cfg_const.qdec_y_phb_pinmux != 0xFF)
            {
                if (System_WakeUpInterruptValue(app_cfg_const.qdec_y_pha_pinmux) ||
                    System_WakeUpInterruptValue(app_cfg_const.qdec_y_phb_pinmux))
                {
                    app_dlps_restore_pad(app_cfg_const.qdec_y_pha_pinmux);
                    app_dlps_restore_pad(app_cfg_const.qdec_y_phb_pinmux);
                    app_qdec_wakeup_handle();
                }
            }
            app_qdec_pad_exit_dlps_config();
        }
#endif
    }
}

static bool app_dlps_platform_pm_check(void)
{
    uint8_t platform_pm_error_code = power_get_error_code();

    APP_PRINT_INFO1("app_dlps_platform_pm_check, ERR Code:%d", platform_pm_error_code);

    return (platform_pm_error_code == PM_ERROR_WAKEUP_TIME);
    //pmu ctrl, must no error
}

static void app_dlps_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_POWER_DOWN_WDG:
        {
            app_stop_timer(&timer_idx_power_down_wdg);

            pd_wdg_chk_times++;
            if (pd_wdg_chk_times == POWER_DOWN_WDG_CHK_TIMES)
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF);
                app_dlps_enable(0xFFFFFFFF);
            }

            if (app_dlps_platform_pm_check() && app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                pd_wdg_chk_times = 0;
                power_stop_all_non_excluded_timer();
                os_timer_dump();
            }
            else
            {
                /* Because timer_idx_power_down_wdg registers the exclude handle of PM,
                   so it cannot become an auto reload timer, which will trigger assert. */
                app_start_timer(&timer_idx_power_down_wdg, "power_down_wdg",
                                app_dlps_timer_id, APP_TIMER_POWER_DOWN_WDG, 0, false,
                                POWER_DOWN_WDG_TIMER);
            }
        }
        break;

    case APP_TIMER_PROFILING_DLPS:
        {
            uint32_t wakeup_count;
            uint32_t total_wakeup_time;
            uint32_t total_sleep_time;
            uint32_t btmac_wakeup_count, last_wakeup_clk, last_sleep_clk;

            power_get_statistics(&wakeup_count, &total_wakeup_time, &total_sleep_time);
            dlps_utils_get_btmac_lpm_statics(&btmac_wakeup_count, &last_wakeup_clk, &last_sleep_clk);
            TEST_PRINT_INFO6("(LPS) WakeupCount: %d, PowerMode: %d, ErrorCode: 0x%x, RefuseReason: 0x%x. BTMAC WakeupCount: %d, ErrorCode: 0x%x.",
                             wakeup_count, power_mode_get(), dlps_util_get_platform_error_code(),
                             dlps_utils_get_platform_refuse_reason(),
                             btmac_wakeup_count, dlps_util_get_btmac_error_code());

            /* Because timer_idx_profiling_dlps registers the exclude handle of PM,
            so it cannot become an auto reload timer, which will trigger assert. */
            app_start_timer(&timer_idx_profiling_dlps, "profiling_dlps", app_dlps_timer_id,
                            APP_TIMER_PROFILING_DLPS, 0, false, PROFILING_DLPS_TIMER_MS);

        }
        break;

    default:
        break;
    }
}

void app_dlps_power_mode_set(void)
{
    bool force_shipping_mode = false;

    if (force_shipping_mode ||
        app_cfg_const.enter_shipping_mode_if_outcase_power_off
        && (app_device_is_in_the_box() == false)
#if (F_APP_PERIODIC_WAKEUP == 1)
        && !app_dlps_need_to_wakeup_by_rtc()
#endif
       )
    {
        power_mode_set(POWER_SHIP_MODE);
    }
    else
    {
        power_mode_set(POWER_POWERDOWN_MODE); // POWER_DLPS_MODE
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
        app_dlps_power_mode_set();

        app_start_timer(&timer_idx_power_down_wdg, "power_down_wdg",
                        app_dlps_timer_id, APP_TIMER_POWER_DOWN_WDG, 0, false,
                        POWER_DOWN_WDG_TIMER);

        app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF);
        app_timer_register_pm_excluded(&timer_idx_power_down_wdg);
    }
}

void app_dlps_enable_auto_poweroff_stop_wdg_timer(void)
{
    pd_wdg_chk_times = 0;
    app_auto_power_off_enable(AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF,
                              app_cfg_const.timer_auto_power_off);
    app_stop_timer(&timer_idx_power_down_wdg);
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
                        app_dlps_timer_id, APP_TIMER_POWER_DOWN_WDG, 0, false,
                        POWER_DOWN_WDG_TIMER);
    }
}

bool app_dlps_check_short_press_power_on(void)
{
    bool ret = false;

    //When use POWER_POWERDOWN_MODE,
    //system will re-boot after wake up and not execute DLPS exit callback
    if ((app_cfg_const.key_gpio_support) && (app_cfg_const.key_power_on_interval == 0))
    {
        if (System_WakeUpInterruptValue(app_cfg_const.key_pinmux[0]) == 1)
        {
            //GPIO INT not triggered before short click release MFB key
            //Use direct power on for short press power on case
            if (app_cfg_const.discharger_support)
            {
                T_APP_CHARGER_STATE app_charger_state;
                uint8_t state_of_charge; //MUST be detected after task init

                app_charger_state = app_charger_get_charge_state();
                state_of_charge = app_charger_get_soc();
                if ((app_charger_state == APP_CHARGER_STATE_NO_CHARGE) && (state_of_charge == BAT_CAPACITY_0))
                {
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

RAM_TEXT_SECTION uint32_t app_dlps_get_dlps_bitmap(void)
{
    return dlps_bitmap;
}

ISR_TEXT_SECTION void app_dlps_set_pad_wake_up(uint8_t pinmux,
                                               PAD_WAKEUP_POL_VAL wake_up_val)
{
    Pad_ControlSelectValue(pinmux, PAD_SW_MODE);
    System_WakeUpPinEnable(pinmux, wake_up_val);
    System_WakeUpInterruptEnable(pinmux);
}

ISR_TEXT_SECTION void app_dlps_pad_wake_up_enable(uint8_t pinmux)
{
    Pad_ControlSelectValue(pinmux, PAD_SW_MODE);
    Pad_WakeupEnableValue(pinmux, 1);
    System_WakeUpInterruptEnable(pinmux);
}

ISR_TEXT_SECTION void app_dlps_pad_wake_up_polarity_invert(uint8_t pinmux)
{
    if (pinmux != 0xFF)
    {
        uint8_t gpio_level = hal_gpio_get_input_level(pinmux);

        Pad_WakeupPolarityValue(pinmux,
                                gpio_level ? PAD_WAKEUP_POL_LOW : PAD_WAKEUP_POL_HIGH);
    }
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
        hal_gpio_get_isr_callback(pinmux, &cb, &context);

        if (cb)
        {
            cb(context);
        }
    }
}

void app_dlps_init(void)
{
    if (!app_cfg_const.enable_dlps)
    {
        return;
    }

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);

    io_dlps_register();

    /* register of call back function */
    power_check_cb_register(app_dlps_check_callback);

    io_dlps_register_enter_cb(app_dlps_enter_callback);
    io_dlps_register_exit_cb(app_dlps_exit_callback);

    app_timer_reg_cb(app_dlps_timeout_cb, &app_dlps_timer_id);

    app_start_timer(&timer_idx_profiling_dlps, "profiling_dlps", app_dlps_timer_id,
                    APP_TIMER_PROFILING_DLPS, 0, false, PROFILING_DLPS_TIMER_MS);
    app_timer_register_pm_excluded(&timer_idx_profiling_dlps);

    app_dlps_power_off();
}

