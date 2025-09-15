/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file
   * @brief
   * @details
   * @author
   * @date
   * @version
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#if F_APP_ADC_SUPPORT || (F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_ADC_DETECT)
#include "trace.h"
#include "app_adc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_adc.h"
#include "adc_manager.h"
#include "board.h"
#include "system_status_api.h"
#include "hal_adp.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_report.h"
#include "app_cfg.h"
#include "app_timer.h"
#include "app_io_msg.h"
#include "sysm.h"

#if F_APP_DISCHARGER_NTC_DETECT_PROTECT
#include "app_mmi.h"
#include "charger_utils.h"
#include "pmu_api.h"
#include "app_dlps.h"

#define CHECK_NTC_ERROR_TIMERS_TO_POWER_OFF     5

static uint8_t timer_idx_adc_ntc_voltage_read = 0;
static uint8_t timer_idx_adc_power_supply_ready = 0;
static uint16_t ntc_adc_vol = 0;
static uint8_t need_power_off = 0;
#endif

#if F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_ADC_DETECT
#include "fmc_api.h"
#include "app_dsp_cfg.h"
#include "app_ext_mic_switch.h"

#if F_APP_ERWS_SUPPORT
#include "app_audio_policy.h"
#include "app_relay.h"
#endif

#define BOOM_MIC_ADC_DETECT_THRESHOLD         3

static uint8_t timer_idx_adc_detect_ext_mic = 0;
static bool is_adc_for_ext_mic_io_registered = false;
static uint8_t ext_mic_io_adc_channel_index = 0x0;

/* unit: mv */
static const uint16_t ext_mic_detect_threshold[] = {720, 1440, 2160, 2880};
#endif

uint8_t application_key_adc_channel_index = 0x0;
bool is_adc_mgr_ever_registered = false;

static uint8_t target_io_pin;
static uint8_t cmd_path;
static uint8_t app_idx;
static uint8_t app_adc_timer_id = 0;

typedef enum
{
    TIMER_ADC_NTC_VOLTAGE_READ,
    ADC_DETECT_EXT_MIC,
    TIMER_ADC_POWER_SUPPLY_READY,
} T_APP_ADC_TIMER;

typedef enum
{
    ADC_POWER_SUPPLY_NULL,
    ADC_POWER_SUPPLY_BY_GPIO,
    ADC_POWER_SUPPLY_BY_PAD,
    ADC_POWER_SUPPLY_BY_VPA,
} T_APP_ADC_POWER_SUPPLY_METHOD;

static T_APP_ADC_POWER_SUPPLY_METHOD adc_power_supply_method = ADC_POWER_SUPPLY_NULL;

/***********************************************************
AON must use system_status_api, follow BB2
************************************************************/
/** @defgroup  APP_ADC_API Audio ADC
    * @brief app process implementation for audio sample project
    * @{
    */

/*============================================================================*
 *                              Public Functions
 *============================================================================*/
/** @defgroup APP_ADC_Exported_Functions Audio ADC Functions
    * @brief
    * @{
    */

#if F_APP_DISCHARGER_NTC_DETECT_PROTECT
static void app_adc_power_supply_method_get(void)
{
    if (app_cfg_const.discharge_ntc_protection_thermistor_option &&
        app_cfg_const.charger_support &&
        (app_cfg_const.thermistor_power_pinmux != 0xFF) &&
        app_cfg_const.thermistor_power_gpio_support)
    {
        adc_power_supply_method = ADC_POWER_SUPPLY_BY_GPIO;

        if (app_cfg_const.ntc_power_by_pad_direct)
        {
            adc_power_supply_method = ADC_POWER_SUPPLY_BY_PAD;
        }

        if (app_cfg_const.thermistor_power_vpa_support)
        {
            adc_power_supply_method = ADC_POWER_SUPPLY_BY_VPA;
        }
    }
}

static void app_adc_power_supply_turn_on(void)
{
    if ((adc_power_supply_method == ADC_POWER_SUPPLY_BY_GPIO) ||
        (adc_power_supply_method == ADC_POWER_SUPPLY_BY_PAD))
    {
        Pad_OutputControlValue(app_cfg_const.thermistor_power_pinmux, PAD_OUT_HIGH);
    }
    else if (adc_power_supply_method == ADC_POWER_SUPPLY_BY_VPA)
    {
#if TARGET_RTL8773DO || TARGET_RTL8773DFL
        pmu_ldo_pa_control(true);
#endif
    }

    if ((adc_power_supply_method == ADC_POWER_SUPPLY_BY_PAD) ||
        (adc_power_supply_method == ADC_POWER_SUPPLY_BY_VPA))
    {
        app_start_timer(&timer_idx_adc_power_supply_ready, "adc_disacharger_ntc_power_supply",
                        app_adc_timer_id, TIMER_ADC_POWER_SUPPLY_READY, 0, false,
                        20);
    }
    else if (adc_power_supply_method == ADC_POWER_SUPPLY_BY_GPIO)
    {
        uint8_t input_pin = 0;

        charger_utils_get_thermistor_1_pin(&input_pin);
        app_adc_enable_read_adc_io(input_pin);
    }
}

static void app_adc_power_supply_turn_off(void)
{
    if ((adc_power_supply_method == ADC_POWER_SUPPLY_BY_GPIO) ||
        (adc_power_supply_method == ADC_POWER_SUPPLY_BY_PAD))
    {
        Pad_OutputControlValue(app_cfg_const.thermistor_power_pinmux, PAD_OUT_LOW);
    }
    else if (adc_power_supply_method == ADC_POWER_SUPPLY_BY_VPA)
    {
#if TARGET_RTL8773DO || TARGET_RTL8773DFL
        pmu_ldo_pa_control(false);
#endif
    }
}

void app_adc_ntc_voltage_update(uint16_t final_value)
{
    if (!app_cfg_const.discharge_ntc_protection_thermistor_option)
    {
        return;
    }

    ntc_adc_vol = final_value;
}

void app_adc_ntc_voltage_power_off_check(void)
{
    APP_PRINT_TRACE3("app_adc_ntc_voltage_power_off_check: %d, %d, %d",
                     ntc_adc_vol, app_db.device_state, need_power_off);

    if ((ntc_adc_vol < app_cfg_const.high_temperature_protect_value) ||
        (ntc_adc_vol > app_cfg_const.low_temperature_protect_value))
    {
        need_power_off ++;
    }
    else
    {
        need_power_off = 0;
    }

    if ((app_db.device_state == APP_DEVICE_STATE_ON) &&
        (need_power_off == CHECK_NTC_ERROR_TIMERS_TO_POWER_OFF))
    {
        need_power_off = 0;
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
    }
}

bool app_adc_ntc_voltage_valid_check(void)
{
    bool ret = false;

    if (!app_cfg_const.discharge_ntc_protection_thermistor_option)
    {
        return false;
    }

    if (!timer_idx_adc_ntc_voltage_read)
    {
        app_adc_ntc_voltage_read_start();
    }

    if ((ntc_adc_vol >= app_cfg_const.high_temperature_protect_value) &&
        (ntc_adc_vol <= app_cfg_const.low_temperature_protect_value))
    {
        ret = true;
    }
    else
    {
        APP_PRINT_ERROR1("app_adc_ntc_voltage_valid_check: NTC ADC value is invalid, ntc_adc_vol: %d",
                         ntc_adc_vol);
    }

    return ret;
}

void app_adc_ntc_voltage_read_start(void)
{
    if (!app_cfg_const.discharge_ntc_protection_thermistor_option)
    {
        return;
    }

    app_adc_power_supply_turn_on();
    app_dlps_disable(APP_DLPS_ENTER_CHECK_DISCHARGER_NTC);

    app_start_timer(&timer_idx_adc_ntc_voltage_read, "adc_read_adp_voltage",
                    app_adc_timer_id, TIMER_ADC_NTC_VOLTAGE_READ, 0, false,
                    2000);
}

void app_adc_ntc_voltage_read_stop(void)
{
    if (!app_cfg_const.discharge_ntc_protection_thermistor_option)
    {
        return;
    }
    APP_PRINT_INFO0("app_adc_ntc_voltage_read_stop");
    if (is_adc_mgr_ever_registered)
    {
        adc_mgr_free_chann(application_key_adc_channel_index);
    }
    app_stop_timer(&timer_idx_adc_ntc_voltage_read);
}
#endif

#if F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_ADC_DETECT
static void adc_ext_mic_io_callback(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data;
    uint16_t adc_val;
    uint8_t mic_plugged = false;

    adc_mgr_read_data_req(ext_mic_io_adc_channel_index, &adc_data, 0x0001);
    adc_val = ADC_GetRes(adc_data, EXT_SINGLE_ENDED(BOOM_IN_ADC_DETECT_PIN));

#if F_APP_ERWS_SUPPORT
    APP_PRINT_TRACE5("adc_ext_mic_io_callback: adc_value %d, thrld %d, role %d, local boom mic: %d, remote boom mic: %d",
                     adc_val,
                     ext_mic_detect_threshold[BOOM_MIC_ADC_DETECT_THRESHOLD], app_cfg_nv.bud_role,
                     app_db.local_boom_mic_in, app_db.remote_boom_mic_in);
#else
    APP_PRINT_TRACE2("adc_ext_mic_io_callback: adc_value %d, thrld %d",
                     adc_val,
                     ext_mic_detect_threshold[BOOM_MIC_ADC_DETECT_THRESHOLD]);
#endif

    if (adc_val > ext_mic_detect_threshold[BOOM_MIC_ADC_DETECT_THRESHOLD])
    {
        mic_plugged = true;
    }
    else
    {
        mic_plugged = false;
    }

    if (app_db.ext_mic_plugged != mic_plugged)
    {
        app_db.ext_mic_plugged = mic_plugged;

#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
        app_ext_mic_switch_ic_mute(APP_EXT_MIC_IC_MUTE);
#endif

        if (app_db.ext_mic_plugged)
        {
#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
            //MIC_SEL_PIN Low Level : Boom Mic is Plugged
            app_ext_mic_switch_ic_mic_sel(APP_EXT_MIC_IC_BOOM_MIC);
#endif
            flash_dsp_cfg_switch(DSP_CFG_IMG_2);
        }
        else
        {
#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
            //MIC_SEL_PIN High Level : Boom Mic is not plugged
            app_ext_mic_switch_ic_mic_sel(APP_EXT_MIC_IC_INTERNAL_MIC);
#endif
            flash_dsp_cfg_switch(DSP_CFG_IMG_1);
        }

        app_dsp_cfg_apply();

#if F_APP_ERWS_SUPPORT && F_APP_EXT_MIC_SWITCH_SUPPORT
        app_db.local_boom_mic_in = app_db.ext_mic_plugged;
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_BOOM_MIC_IN);
#endif

#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
        app_ext_mic_switch_ic_mute(APP_EXT_MIC_IC_UNMUTE);
#endif
    }
}

static bool app_adc_enable_read_ext_mic_io(void)
{
    int8_t result = 0;
    bool ret = true;
    uint8_t cfg_pin = BOOM_IN_ADC_DETECT_PIN;

    if (!IS_ADC_CHANNEL(cfg_pin))
    {
        result = -1;
        goto exit;
    }

    if (is_adc_for_ext_mic_io_registered)
    {
        adc_mgr_enable_req(ext_mic_io_adc_channel_index);
        result = 1;
        goto exit;
    }

    Pad_Config(cfg_pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_PowerOrShutDownValue(cfg_pin, 0);

    ADC_InitTypeDef ADC_InitStruct;
    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.adcClock = ADC_CLK_39K;
    ADC_InitStruct.bitmap = 0x1;
    ADC_InitStruct.schIndex[0] = EXT_SINGLE_ENDED(cfg_pin);

    if (!adc_mgr_register_req(&ADC_InitStruct,
                              (adc_callback_function_t)adc_ext_mic_io_callback,
                              &ext_mic_io_adc_channel_index))
    {
        result = -2;
        goto exit;
    }

    is_adc_for_ext_mic_io_registered = true;

    adc_mgr_enable_req(ext_mic_io_adc_channel_index);

exit:

    APP_PRINT_TRACE1("app_adc_enable_read_ext_mic_io: ret %d", result);

    if (result < 0)
    {
        ret = false;
    }

    return ret;
}

static void app_adc_start_detect_ext_mic(void)
{
    app_adc_enable_read_ext_mic_io();

    app_start_timer(&timer_idx_adc_detect_ext_mic, "adc_detect_ext_mic",
                    app_adc_timer_id, ADC_DETECT_EXT_MIC,
                    0, false, 1000);
}

void app_adc_detect_ext_mic_plug_in(bool enable)
{
    APP_PRINT_TRACE1("app_adc_detect_ext_mic_plug_in: %d", enable);

    if (enable)
    {
        app_adc_start_detect_ext_mic();
    }
    else
    {
        app_stop_timer(&timer_idx_adc_detect_ext_mic);
    }
}

static void app_adc_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            app_adc_detect_ext_mic_plug_in(true);
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            app_adc_detect_ext_mic_plug_in(false);
        }
        break;

    default:
        break;
    }
}
#endif

static void app_adc_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO3("app_adc_timeout_cb: timer_evt %d, param %d, ADP_DET_status: %d",
                    timer_evt, param, adp_get_current_state(ADP_DETECT_5V));
    uint8_t input_pin = 0;
    switch (timer_evt)
    {
#if F_APP_DISCHARGER_NTC_DETECT_PROTECT
    case TIMER_ADC_NTC_VOLTAGE_READ:
        {
            app_adc_ntc_voltage_read_start();
        }
        break;

    case TIMER_ADC_POWER_SUPPLY_READY:
        {
            charger_utils_get_thermistor_1_pin(&input_pin);
            app_adc_enable_read_adc_io(input_pin);
        }
#endif

#if F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_ADC_DETECT
    case ADC_DETECT_EXT_MIC:
        {
            app_adc_start_detect_ext_mic();
        }
        break;
#endif

    default:
        break;
    }
}

void app_adc_set_cmd_info(uint8_t path, uint8_t idx)
{
    cmd_path = path;
    app_idx = idx;
}

bool app_adc_enable_read_adc_io(uint8_t input_pin)
{
    if (!IS_ADC_CHANNEL(input_pin))
    {
        APP_PRINT_ERROR0("app_adc_enable_read_adc_io: invalid ADC IO");
        return false;
    }

    if (!is_adc_mgr_ever_registered)
    {
        target_io_pin = input_pin;
        Pad_Config(input_pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
        Pad_PowerOrShutDownValue(input_pin, 0);

        ADC_HighBypassCmd(input_pin, ENABLE);

        ADC_InitTypeDef ADC_InitStruct;
        ADC_StructInit(&ADC_InitStruct);
        ADC_InitStruct.adcClock = ADC_CLK_39K;
        ADC_InitStruct.bitmap = 0x1;
        ADC_InitStruct.schIndex[0] = EXT_SINGLE_ENDED(input_pin);

        if (!adc_mgr_register_req(&ADC_InitStruct,
                                  (adc_callback_function_t)app_adc_interrupt_handler,
                                  &application_key_adc_channel_index))
        {
            APP_PRINT_ERROR0("app_adc_enable_read_adc_io: ADC Register Request Fail");
            return false;
        }
        else
        {
            is_adc_mgr_ever_registered = true;
        }
    }

    adc_mgr_enable_req(application_key_adc_channel_index);
    return true;
}

void app_adc_interrupt_handler(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data;
    uint16_t final_value[2];
    uint8_t evt_param[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    adc_mgr_read_data_req(application_key_adc_channel_index, &adc_data, 0x0001);

    final_value[0]  = ADC_GetHighBypassRes(adc_data,
                                           EXT_SINGLE_ENDED(target_io_pin));

#if F_APP_DISCHARGER_NTC_DETECT_PROTECT
    app_adc_ntc_voltage_update(final_value[0]);

    T_IO_MSG adc_msg;
    adc_msg.type = IO_MSG_TYPE_ADC;
    adc_msg.subtype = 0;
    adc_msg.u.param = 0;
    app_io_msg_send(&adc_msg);
#endif

    evt_param[0] = (uint8_t) final_value[0];
    evt_param[1] = (uint8_t)(final_value[0] >> 8);
    APP_PRINT_TRACE4("app_adc_interrupt_handler: int_status: %d, adc_data = 0x%x, Final result = 0x%02X %02X",
                     int_status, adc_data,
                     evt_param[1], evt_param[0]);

    app_report_event(cmd_path, EVENT_REPORT_PAD_VOLTAGE, app_idx, evt_param, sizeof(evt_param));

#if F_APP_DISCHARGER_NTC_DETECT_PROTECT
    app_adc_power_supply_turn_off();
    app_dlps_enable(APP_DLPS_ENTER_CHECK_DISCHARGER_NTC);
#endif
}

void app_adc_init(void)
{
    app_timer_reg_cb(app_adc_timeout_cb, &app_adc_timer_id);

#if F_APP_DISCHARGER_NTC_DETECT_PROTECT
    app_adc_power_supply_method_get();
    app_adc_ntc_voltage_read_start();
#endif

#if F_APP_EXT_MIC_SWITCH_SUPPORT && F_APP_EXT_MIC_PLUG_IN_ADC_DETECT
    sys_mgr_cback_register(app_adc_dm_cback);
#endif
}
#endif
/** @} */ /* End of group APP_ADC_Exported_Functions */
/** @} */ /* End of group APP_ADC_API */
