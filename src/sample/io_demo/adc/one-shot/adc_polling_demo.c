/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     adc_polling_demo.c
* @brief    This file provides demo code of adc in one-shot mode by polling.
* @details
* @author   arthur
* @date     2022-05-26
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_adc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_nvic.h"
#include "trace.h"

/** @defgroup  ADC_POLLING_DEMO ADC Polling Demo
    * @brief  adc implementation demo code
    * @{
    */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_adc_init(void)
{
    Pad_Config(ADC_1, PAD_SW_MODE, PAD_SHUTDOWN, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pinmux_Config(ADC_1, IDLE_MODE);
}

/**
  * @brief  Initialize ADC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_adc_init(void)
{
    uint16_t data[3];
    int32_t res[3];

    RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);

    ADC_InitTypeDef adc_init_struct;
    ADC_StructInit(&adc_init_struct);
    adc_init_struct.adcClock = ADC_CLK_39K;
    adc_init_struct.schIndex[0] = EXT_SINGLE_ENDED(1);
    adc_init_struct.schIndex[1] = INTERNAL_VBAT_MODE;
    adc_init_struct.schIndex[2] = INTERNAL_VADPIN_MODE;
    adc_init_struct.bitmap = 0x07;
    ADC_Init(ADC, &adc_init_struct);

    ADC_INTConfig(ADC, ADC_INT_ONE_SHOT_DONE, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);

    ADC_Cmd(ADC, ADC_One_Shot_Mode, ENABLE);

    while (ADC_GetIntFlagStatus(ADC, ADC_INT_ONE_SHOT_DONE) == RESET);

    ADC_ClearINTPendingBit(ADC, ADC_INT_ONE_SHOT_DONE);
    ADC_Cmd(ADC, ADC_One_Shot_Mode, DISABLE);
    data[0] = ADC_Read(ADC, 0);
    data[1] = ADC_Read(ADC, 1);
    data[2] = ADC_Read(ADC, 2);
    res[0] = ADC_GetRes(data[0], EXT_SINGLE_ENDED(1));
    res[1] = ADC_GetRes(data[1], INTERNAL_VBAT_MODE);
    res[2] = ADC_GetRes(data[2], INTERNAL_VADPIN_MODE);
    IO_PRINT_INFO3("driver_adc_init: data[0] %d, data[1] %d, data[2] %d",
                   data[0], data[1], data[2]);
    IO_PRINT_INFO3("driver_adc_init: res[0] %d, res[1] %d, res[2] %d",
                   res[0], res[1], res[2]);
}

/**
  * @brief  demo code of operation about ADC.
  * @param   No parameter.
  * @return  void
  */
void adc_polling_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_adc_init();

    /* Initialize ADC peripheral */
    driver_adc_init();
}


/** @} */ /* End of group ADC_POLLING_DEMO */

