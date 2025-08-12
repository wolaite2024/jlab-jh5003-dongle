/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     adc_key_demo.c
* @brief    This file provides sample code for detecting key presses using ADC.
* @details
* @author
* @date
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "rtl876x_nvic.h"
#include "vector_table.h"
#include "rtl876x_adc.h"
#include "rtl876x_lpc.h"
#include "adc_manager.h"
#include "rtl876x_pinmux.h"
#include "os_task.h"
#include "os_msg.h"
#include "trace.h"
#include "dlps_util.h"
#include "io_dlps.h"
#include "pm.h"

/** @defgroup  ADC_KEY_DEMO ADC Key Demo
    * @brief  adc implementation demo code
    * @{
    */
#define     ADC_SAMPLE_PIN                          ADC_3
#define     KEY_RELEASE                             0
#define     KEY_PRESS                               1
#define     MAX_KEY_NUM                             6
#define     MAX_NUMBER_OF_ADC_KEY_MESSAGE           0x10
#define     MAX_NUMBER_OF_IO_MESSAGE                0x40
#define     EVENT_ADC_KEY_MSG                       0x20
#define     IO_DEMO_EVENT_LPC                       0x30
#define     LPC_COMP_VALUE                          0x1

typedef enum
{
    APP_TIMER_ADC_KEY_READ_VOLTAGE = 0x00,
} T_APP_TIMER_ADC_KEY;

typedef struct t_key_data
{
    uint8_t key_press[MAX_KEY_NUM];         /**< key action KEY_RELEASE or KEY_PRESS */
    uint8_t key_index;                      /**< Indicates the key index that was last pressed */
} T_KEY_DATA;

typedef struct t_key_check
{
    uint8_t         key_mask;               /* key Mask */
    uint8_t         key_press;              /* key action KEY_RELEASE or KEY_PRESS*/
} T_KEY_CHECK;

static void *io_evt_queue_handle;
static void *adc_key_queue_handle;
static void *io_demo_app_task_handle;
static void adc_key_read_voltage_callback(void *pvPara, uint32_t int_status);
static uint8_t adc_channel_key_voltage = 0;
static T_KEY_DATA adc_key_data;
static void lpc_handler(void);

/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_adc_init(void)
{
    Pad_Config(ADC_SAMPLE_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pinmux_Config(ADC_SAMPLE_PIN, IDLE_MODE);
}

/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_lpc_init(void)
{
    Pad_Config(ADC_SAMPLE_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pinmux_Config(ADC_SAMPLE_PIN, IDLE_MODE);
}

/**
  * @brief  Initialize ADC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_adc_init(void)
{
    ADC_InitTypeDef ADC_InitStruct;
    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.adcClock = ADC_CLK_39K;
    ADC_InitStruct.bitmap = 0x0001;
    ADC_InitStruct.schIndex[0] = EXT_SINGLE_ENDED(3);

    if (!adc_mgr_register_req(&ADC_InitStruct,
                              (adc_callback_function_t)adc_key_read_voltage_callback,
                              &adc_channel_key_voltage))
    {
        IO_PRINT_ERROR0("driver_adc_init: adc_mgr_register_req failed");
        return;
    }
}

/**
  * @brief  Initialize LPC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_lpc_init(void)
{
    LPC_InitTypeDef LPC_InitStruct;
    LPC_StructInit(&LPC_InitStruct);
    LPC_InitStruct.LPC_Channel   = ADC_SAMPLE_PIN;
    LPC_InitStruct.LPC_Edge      = LPC_Vin_Below_Vth;
    LPC_InitStruct.LPC_Threshold = LPC_3000_mV;
    LPC_Init(&LPC_InitStruct);
    LPC_CounterReset();
    LPC_WriteComparator(LPC_COMP_VALUE);
    LPC_INTConfig(LPC_INT_COUNT_COMP, ENABLE);
#if ((TARGET_RTL87X3E == 1) || (TARGET_RTL87X3EP == 1))
    RTC_CpuNVICEnable(ENABLE);
    RamVectorTableUpdate(RTC_VECTORn, lpc_handler);
    /* Config LPC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
#else
    LPC_INTConfig(LPC_INT_VOLTAGE_COMP, ENABLE);
    RamVectorTableUpdate(LPCOMP_VECTORn, lpc_handler);
    /* Config LPC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LPCOMP_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
#endif
    LPC_Cmd(ENABLE);
    LPC_CounterCmd(ENABLE);
}

/**
  * @brief  ADC callback function execute when adc_mgr_enable_req be called.
  * @param   No used.
  * @return  void
  */
static void adc_key_read_voltage_callback(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data = 0;
    int32_t adc_voltage = 0;
    uint16_t sched_bit_map = 0x0001;
    uint8_t key_status_update_fg = 0;
    uint8_t key_index = 0xff;
    uint8_t event = EVENT_ADC_KEY_MSG;
    T_KEY_CHECK key_check;
    adc_mgr_read_data_req(adc_channel_key_voltage, &adc_data, sched_bit_map);
    adc_voltage = ADC_GetRes(adc_data, EXT_SINGLE_ENDED(3));
    if (adc_voltage < 20)   //KEY0 -- S6
    {
        key_index = 0;
    }
    else if (adc_voltage < 446 && adc_voltage >= 376)    //KEY1 -- S8
    {
        key_index = 1;
    }
    else if (adc_voltage < 930 && adc_voltage >= 830)    //KEY2 -- S11
    {
        key_index = 2;
    }
    else if (adc_voltage < 1305 && adc_voltage >= 1185)   //KEY3 -- S12
    {
        key_index = 3;
    }
    else if (adc_voltage < 1735 && adc_voltage >= 1565)   //KEY4 -- S10
    {
        key_index = 4;
    }
    else if (adc_voltage < 2136 && adc_voltage >= 1951)   //KEY5 -- S2
    {
        key_index = 5;
    }
    else if (adc_voltage >= 3000)
    {
        key_index = 0xff;
    }

    if (key_index != 0xff)
    {
        if (adc_key_data.key_press[key_index] == KEY_RELEASE)
        {
            key_status_update_fg = 1;
            adc_key_data.key_press[key_index] = KEY_PRESS;
            adc_key_data.key_index = key_index;
        }
        board_lpc_init();
        LPC_SetTriggerEdge(LPC_Vin_Over_Vth);
        LPC_Cmd(ENABLE);
        LPC_CounterCmd(ENABLE);
    }
    else
    {
        if (adc_key_data.key_index != 0xff)
        {
            if (adc_key_data.key_press[adc_key_data.key_index] == KEY_PRESS)
            {
                key_status_update_fg = 1;
                adc_key_data.key_press[adc_key_data.key_index] = KEY_RELEASE;
                key_index = adc_key_data.key_index;
                adc_key_data.key_index = 0xff;
            }
        }
        board_lpc_init();
        LPC_SetTriggerEdge(LPC_Vin_Below_Vth);
        LPC_Cmd(ENABLE);
        LPC_CounterCmd(ENABLE);
    }

    if (key_status_update_fg)
    {
        key_check.key_mask = BIT(key_index);
        key_check.key_press = adc_key_data.key_press[key_index];
        if (os_msg_send(io_evt_queue_handle, &event, 0) == true)
        {
            if (os_msg_send(adc_key_queue_handle, &key_check, 0) == false)
            {
                IO_PRINT_ERROR0("adc_key_read_voltage_callback: Send Queue Error");
            }
        }
    }
}

/**
  * @brief  Enable ADC sampling.
  * @param   No parameter.
  * @return  void
  */
static void adc_key_parse(void)
{
    LPC_Cmd(DISABLE);
    LPC_CounterCmd(DISABLE);
    board_adc_init();
    adc_mgr_enable_req(adc_channel_key_voltage);
}

/**
  * @brief   DLPS enter callback.
  * @param   No parameter.
  * @return  void
  */
static void io_dlps_enter_callback(void)
{
    RTC_SystemWakeupConfig(ENABLE);
    DBG_DIRECT("io_dlps_enter_callback");
}

/**
  * @brief  Demo code of operation about ADC and LPC.
  * @param   No parameter.
  * @return  void
  */
static void adc_key_demo_init(void)
{
    adc_key_data.key_index = 0xff;
    driver_adc_init();
    board_lpc_init();
    driver_lpc_init();

    io_dlps_register();
    io_dlps_register_enter_cb(io_dlps_enter_callback);

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    power_mode_set(POWER_DLPS_MODE);
}

/**
  * @brief  io_demo_app_task handle.
  * @param   No parameter.
  * @return  void
  */
static void io_demo_app_task(void *param)
{
    uint8_t event = 0;
    adc_key_demo_init();

    while (1)
    {
        if (os_msg_recv(io_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == IO_DEMO_EVENT_LPC)
            {
                adc_key_parse();
            }
            else if (event == EVENT_ADC_KEY_MSG)
            {
                T_KEY_CHECK key_check;

                if (os_msg_recv(adc_key_queue_handle, &key_check, 0) == true)
                {
                    IO_PRINT_INFO2("io_demo_app_task: key mask 0x%x, key_press %d", key_check.key_mask,
                                   key_check.key_press);
                }
            }
        }
    }
}

/**
  * @brief   Task demo code interface, call by main.c.
  * @param   void
  * @return  void
  */
void adc_key_demo(void)
{
    os_msg_queue_create(&io_evt_queue_handle, "ioQ", MAX_NUMBER_OF_IO_MESSAGE,
                        sizeof(unsigned char));
    os_msg_queue_create(&adc_key_queue_handle, "adc_key_msg", MAX_NUMBER_OF_ADC_KEY_MESSAGE,
                        sizeof(T_KEY_CHECK));
    os_task_create(&io_demo_app_task_handle, "io_demo_app_task", io_demo_app_task, NULL, 384 * 4, 2);
}

/**
  * @brief  LPC interrupt handle function.
  * @param  None.
  * @return None.
  */
static void lpc_handler(void)
{
    uint8_t event = IO_DEMO_EVENT_LPC;
    /* LPC counter comparator interrupt */
    if (LPC_GetINTStatus(LPC_INT_COUNT_COMP) == SET)
    {
        LPC_CounterReset();
        LPC_ClearINTPendingBit(LPC_INT_COUNT_COMP);
        if (os_msg_send(io_evt_queue_handle, &event, 0) == false)
        {
            IO_PRINT_ERROR0("lpc_handler: Send Queue Error");
        }
    }
}

/** @} */ /* End of group ADC_KEY_DEMO */

