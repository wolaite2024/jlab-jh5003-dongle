/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     lpc_comparator_demo.c
* @brief        This file provides demo code of lpc counter.
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
#include "trace.h"
#include "vector_table.h"

/** @defgroup  LPC_DEMO_LPC  LPC DEMO
    * @brief  Led work in counting pulse mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup LPC_Demo_Exported_Macros LPC Demo Exported Macros
  * @brief
  * @{
  */

#define LPC_CAPTURE_PIN         ADC_0 //ADC_0 as input
#define LPC_COMP_VALUE          0x4

/** @} */ /* End of group LPC_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup LPC_Demo_Exported_Functions LPC Demo Exported Functions
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
    LPC_InitStruct.LPC_Threshold = LPC_400_mV;
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
void lpc_comparator_demo(void)
{
    IO_PRINT_INFO0("lpc_comparator_demo: run");
    /* Configure PAD and pinmux firstly! */
    board_lpc_init();

    /* Initialize LPC peripheral */
    driver_lpc_init();
}

/**
  * @brief  RTC interrupt handle function.
  * @param  None.
  * @return None.
  */
static void lpc_rtc_handler(void)
{
    uint16_t lpc_counter_value = 0;

    IO_PRINT_INFO0("lpc_rtc_handler");

    /* LPC counter comparator interrupt */
    if (LPC_GetINTStatus(LPC_INT_COUNT_COMP) == SET)
    {
        lpc_counter_value = LPC_ReadCounter();
        IO_PRINT_INFO1("lpc_rtc_handler: lpc_counter_value %d", lpc_counter_value);

        LPC_WriteComparator(lpc_counter_value + LPC_COMP_VALUE);
        LPC_ClearINTPendingBit(LPC_INT_COUNT_COMP);
    }
}

/** @} */ /* End of group LPC_Demo_Exported_Functions */
/** @} */ /* End of group LPC_DEMO_LPC */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

