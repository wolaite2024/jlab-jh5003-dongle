/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     adc_charger_demo.c
* @brief    This file provides demo code for reading battery voltage and NTC voltage by ADC.
* @details
* @author   arthur
* @date     2022-07-20
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "rtl876x_adc.h"
#include "rtl876x_pinmux.h"
#include "adc_manager.h"
#include "trace.h"
#include "app_timer.h"
#include "os_task.h"
#include "os_msg.h"

/** @defgroup  ADC_CHARGER_DEMO ADC Charger Demo
    * @brief  adc implementation demo code
    * @{
    */
static void *io_demo_app_task_handle;
static void *io_evt_queue_handle;

#define     MAX_NUMBER_OF_APP_TIMER_MODULE          0x30    //!< indicate app timer module size
#define     EVENT_APP_TIMER_MSG                     0x30

#define ADC_READ_VBAT_NTC_VOLTAGE_TIMEOUT_MS  500

typedef enum
{
    APP_TIMER_ADC_READ_VBAT_NTC_VOLTAGE = 0x00,
} T_APP_ADC_TIMER;

static uint8_t app_adc_timer_id = 0;
static uint8_t timer_idx_adc_read_vbat_ntc_voltage = 0;
static uint8_t adc_channel_vbat_ntc_voltage = 0;
static bool adc_vbat_ntc_voltage_init = false;

typedef struct
{
    int32_t temperature_battery_1;
    int32_t temperature_battery_2;
    int32_t voltage_battery;
} ADC_CHARGER_DATA_MANAGER;

static ADC_CHARGER_DATA_MANAGER adc_charger_data_mgr;

/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_adc_init(void)
{
    Pad_Config(ADC_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(ADC_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(ADC_0, IDLE_MODE);
    Pinmux_Config(ADC_1, IDLE_MODE);
}

/**
  * @brief  adc callback function execute when adc_mgr_enable_req be called  .
  * @param   No used.
  * @return  void
  */
static void app_adc_vbat_ntc_voltage_read_callback(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data[3];
    uint16_t sched_bit_map = 0x0007;

    adc_mgr_read_data_req(adc_channel_vbat_ntc_voltage, adc_data, sched_bit_map);

    adc_charger_data_mgr.temperature_battery_1 = ADC_GetHighBypassRes(adc_data[0], EXT_SINGLE_ENDED(0));
    adc_charger_data_mgr.temperature_battery_2 = ADC_GetHighBypassRes(adc_data[1], EXT_SINGLE_ENDED(1));
    adc_charger_data_mgr.voltage_battery = ADC_GetRes(adc_data[2], INTERNAL_VBAT_MODE);

    IO_PRINT_INFO3("app_adc_vbat_ntc_voltage_read_callback: temperature_battery_1 %d, temperature_battery_2 %d, voltage_battery %d",
                   adc_charger_data_mgr.temperature_battery_1, adc_charger_data_mgr.temperature_battery_2,
                   adc_charger_data_mgr.voltage_battery);
}

/**
  * @brief  Initialize ADC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void app_adc_vbat_ntc_voltage_init(void)
{
    IO_PRINT_INFO1("app_adc_vbat_ntc_voltage_init: adc_vbat_ntc_voltage_init %d",
                   adc_vbat_ntc_voltage_init);

    if (adc_vbat_ntc_voltage_init)
    {
        /* already init */
        return;
    }

    board_adc_init();

    /* High bypass resistance mode config, Please notice that the input voltage of
       adc channel using high bypass mode should not be over 0.9V */
    ADC_HighBypassCmd(0, ENABLE);
    ADC_HighBypassCmd(1, ENABLE);

    ADC_InitTypeDef ADC_InitStruct;

    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.adcClock = ADC_CLK_39K;
    ADC_InitStruct.bitmap = 0x0007;
    ADC_InitStruct.schIndex[0] = EXT_SINGLE_ENDED(0);
    ADC_InitStruct.schIndex[1] = EXT_SINGLE_ENDED(1);
    ADC_InitStruct.schIndex[2] = INTERNAL_VBAT_MODE;

    if (!adc_mgr_register_req(&ADC_InitStruct,
                              (adc_callback_function_t)app_adc_vbat_ntc_voltage_read_callback,
                              &adc_channel_vbat_ntc_voltage))
    {
        IO_PRINT_ERROR0("app_adc_vbat_ntc_voltage_init: adc_mgr_register_req failed");
        return;
    }

    adc_vbat_ntc_voltage_init = true;
}

/**
  * @brief  Deinit ADC peripheral.
  * @param   No parameter.
  * @return  void
  */
void app_adc_adp_voltage_deinit(void)
{
    IO_PRINT_INFO2("app_adc_adp_voltage_deinit: adc_vbat_ntc_voltage_init %d, adc_channel_vbat_ntc_voltage %d",
                   adc_vbat_ntc_voltage_init, adc_channel_vbat_ntc_voltage);

    if (adc_vbat_ntc_voltage_init)
    {
        adc_mgr_free_chann(adc_channel_vbat_ntc_voltage);
    }

    app_stop_timer(&timer_idx_adc_read_vbat_ntc_voltage);
}


static void app_adc_vbat_ntc_voltage_read(void)
{
    IO_PRINT_INFO1("app_adc_vbat_ntc_voltage_read: adc_vbat_ntc_voltage_init %d",
                   adc_vbat_ntc_voltage_init);

    if (adc_vbat_ntc_voltage_init)
    {
        adc_mgr_enable_req(adc_channel_vbat_ntc_voltage);
    }
}

/**
  * @brief   Timeout callback function for app timer.
  * @param   No used.
  * @return  void
  */
static void app_adc_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_ADC_READ_VBAT_NTC_VOLTAGE:
        {
            app_adc_vbat_ntc_voltage_read();
        }
        break;

    default:
        break;
    }
}

/**
  * @brief  demo code of operation about ADC.
  * @param   No parameter.
  * @return  void
  */
static void adc_charger_demo_init(void)
{
    app_timer_reg_cb(app_adc_timeout_cb, &app_adc_timer_id);

    app_adc_vbat_ntc_voltage_init();

    app_start_timer(&timer_idx_adc_read_vbat_ntc_voltage, "adc_read_vbat_ntc_voltage",
                    app_adc_timer_id, APP_TIMER_ADC_READ_VBAT_NTC_VOLTAGE, 0, true,
                    ADC_READ_VBAT_NTC_VOLTAGE_TIMEOUT_MS);
}

/**
  * @brief  IO_Demo Task Handle.
  * @param   No parameter.
  * @return  void
  */
static void io_demo_task(void *param)
{
    uint8_t event;

    IO_PRINT_TRACE0("io_demo_task: running");
    adc_charger_demo_init();

    while (true)
    {
        if (os_msg_recv(io_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            IO_PRINT_INFO1("io_demo_task: event 0x%x", event);
            if (event == EVENT_APP_TIMER_MSG)
            {
                app_timer_handle_msg(event);
            }
        }
    }
}

/**
  * @brief  task demo code interface , call by main.c.
  * @param   void
  * @return  void
  */
void adc_charger_demo(void)
{
    os_msg_queue_create(&io_evt_queue_handle, "evtQ", MAX_NUMBER_OF_APP_TIMER_MODULE,
                        sizeof(unsigned char));
    app_init_timer(io_evt_queue_handle, MAX_NUMBER_OF_APP_TIMER_MODULE);
    os_task_create(&io_demo_app_task_handle, "app", io_demo_task, NULL, 384 * 4, 2);
}

/** @} */ /* End of group ADC_CHARGER_DEMO */

