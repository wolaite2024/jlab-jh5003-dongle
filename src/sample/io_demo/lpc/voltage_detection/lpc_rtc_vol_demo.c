/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     lpc_rtc_vol_demo.c
* @brief        This file provides demo code to detect voltage.
* @details
* @author   elliot chen
* @date         2016-11-30
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_lpc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "vector_table.h"
#include "trace.h"
#include "rtl876x.h"
/** @defgroup  LPC_RTC_VOL_DEMO  LPC RTC VOLTAGE DEMO
    * @brief  Led work in voltage detection mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup LPC_Voltage_Detection_Exported_Macros LPC Voltage Detection Exported Macros
  * @brief
  * @{
  */

#define LPC_CAPTURE_PIN         ADC_0//ADC_0 as input
#define LPC_COMP_VALUE          0x5

/** @} */ /* End of group LPC_Voltage_Detection_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup LPC_Voltage_Detection_Exported_Functions LPC Voltage Detection Exported Functions
  * @brief
  * @{
  */

static void lpc_rtc_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_lpc_init(void)
{
    Pad_Config(LPC_CAPTURE_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pinmux_Config(LPC_CAPTURE_PIN, IDLE_MODE);
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
    LPC_InitStruct.LPC_Channel   = LPC_CAPTURE_PIN;
    LPC_InitStruct.LPC_Edge      = LPC_Vin_Over_Vth;
    LPC_InitStruct.LPC_Threshold = LPC_1200_mV;
    LPC_Init(&LPC_InitStruct);

    LPC_CounterReset();
    LPC_WriteComparator(LPC_COMP_VALUE);
    LPC_INTConfig(LPC_INT_COUNT_COMP, ENABLE);

#if (TARGET_RTL87X3E == 1)
    RTC_CpuNVICEnable(ENABLE);
    RamVectorTableUpdate(RTC_VECTORn, lpc_rtc_handler);
    /* Config LPC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
#else
    LPC_INTConfig(LPC_INT_VOLTAGE_COMP, ENABLE);
    RamVectorTableUpdate(LPCOMP_VECTORn, lpc_rtc_handler);
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
  * @brief  demo code of operation about LPC.
  * @param   No parameter.
  * @return  void
  */
void lpc_rtc_vol_demo(void)
{
    IO_PRINT_INFO0("lpc_rtc_vol_demo: start");

    /* Configure PAD and pinmux firstly! */
    board_lpc_init();

    /* Initialize LPC peripheral */
    driver_lpc_init();
}

/**
  * @brief  LPC battery detection interrupt handle function.
  * @param  None.
  * @return None.
  */

static void lpc_rtc_handler(void)
{
    if (LPC_GetINTStatus(LPC_INT_COUNT_COMP) == SET)
    {
        IO_PRINT_INFO1("lpc_rtc_handler: lpc_counter_value %d", LPC_ReadCounter());
        LPC_CounterReset();
        LPC_ClearINTPendingBit(LPC_INT_COUNT_COMP);//Add Application code here
        //Add Application code here
    }
}

/** @} */ /* End of group LPC_Voltage_Detection_Exported_Functions */
/** @} */ /* End of group LPC_RTC_VOL_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/
