/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_key.c
* @brief     This file provides demo code of GPIO used as a key by edge trigger mode with hw debounce.
* @details
* @author   renee
* @date     2017-3-10
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_gpio.h"
#include "rtl876x_nvic.h"
#include "trace.h"
#include "vector_table.h"

/** @defgroup  GPIO_EDGE_KEY  GPIO EDGE KEY DEMO
    * @brief  Gpio key by edge trigger implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Gpio_Edge_Key_Exported_Macros Gpio Edge Key Exported Macros
  * @brief
  * @{
  */
#define KEY_PIN                       ADC_0
#define KEY_IRQN                      GPIO0_IRQn

#define KEY_DEBOUNCE_TIME             (10)            //10ms

/** @} */ /* End of group Gpio_Edge_Key_Exported_Macros */

static void key_handler(void);

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup Gpio_Edge_Key_Exported_Variables Gpio Edge Key Exported Variables
  * @brief
  * @{
  */

static uint8_t key_status = 1;

/** @} */ /* End of group Gpio_Edge_Key_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gpio_Edge_Key_Exported_Functions Gpio Edge Key Exported Functions
  * @brief
  * @{
  */

/**
 * @brief  initialization of pinmux settings and pad settings.
 * @param   No parameter.
 * @return  void  */
static void board_key_init(void)
{
    Pinmux_Config(KEY_PIN, DWGPIO);

    Pad_Config(KEY_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

/**
* @brief  GPIO and TIM peripheral initial function.
* @param   none.
* @return  void
*/
static void key_init(void)
{
    /* turn on GPIO clock */
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);

    GPIO_InitTypeDef
    KeyboardButton_Param;     /* Define Mouse Button parameter structure. Mouse button is configed as GPIO. */
    GPIO_StructInit(&KeyboardButton_Param);
    KeyboardButton_Param.GPIO_PinBit  = GPIO_GetPin(KEY_PIN);
    KeyboardButton_Param.GPIO_Mode = GPIO_Mode_IN;
    KeyboardButton_Param.GPIO_ITCmd = ENABLE;
    KeyboardButton_Param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    KeyboardButton_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    KeyboardButton_Param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
    KeyboardButton_Param.GPIO_DebounceTime = KEY_DEBOUNCE_TIME;
    GPIOx_Init(GPIOA, &KeyboardButton_Param);

    GPIOx_INTConfig(GPIOA, GPIO_GetPin(KEY_PIN), ENABLE);
    GPIOx_MaskINTConfig(GPIOA, GPIO_GetPin(KEY_PIN), DISABLE);

    RamVectorTableUpdate(GPIO_A0_VECTORn, key_handler);

    /*  Enable GPIO0 IRQ  */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = KEY_IRQN;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  demo code of operation about GPIO key.
  * @param   No parameter.
  * @return  void
  */
void gpio_edge_key(void)
{
    /* Configure PAD and pinmux firstly! */
    board_key_init();

    /* Initialize GPIO peripheral */
    key_init();

}

/**
* @brief  GPIO Interrupt handler
* @return  void
*/
static void key_handler(void)
{
    /*  Mask GPIO interrupt */
    GPIOx_INTConfig(GPIOA, GPIO_GetPin(KEY_PIN), DISABLE);
    GPIOx_MaskINTConfig(GPIOA, GPIO_GetPin(KEY_PIN), ENABLE);
    GPIOx_ClearINTPendingBit(GPIOA, GPIO_GetPin(KEY_PIN));

    key_status = GPIOx_ReadInputDataBit(GPIOA, GPIO_GetPin(KEY_PIN));

    if (key_status)
    {
        IO_PRINT_INFO0("key_handler: Key release");
        GPIO->INTPOLARITY &= ~(GPIO_GetPin(KEY_PIN));
    }
    else
    {
        IO_PRINT_INFO0("key_handler: Key press");
        GPIO->INTPOLARITY |= GPIO_GetPin(KEY_PIN);
    }

    GPIOx_MaskINTConfig(GPIOA, GPIO_GetPin(KEY_PIN), DISABLE);
    GPIOx_INTConfig(GPIOA, GPIO_GetPin(KEY_PIN), ENABLE);
}

/** @} */ /* End of group Gpio_Edge_Key_Exported_Functions */
/** @} */ /* End of group GPIO_EDGE_KEY */

