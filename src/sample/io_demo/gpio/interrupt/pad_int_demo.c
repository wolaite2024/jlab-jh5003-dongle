/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     pad_int_demo.c
* @brief    This file provides demo code of PAD interrupt mode, Note, PAD dose not support hardware debounce function.
* @details
* @author   renee
* @date
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "vector_table.h"

/** @defgroup  GPIO_INT_DEMO  GPIO INTERRUPT DEMO
    * @brief  Gpio interrupt implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Gpio_Interrupt_Exported_Macros Gpio Interrupt Exported Macros
  * @brief
  * @{
  */
#define GPIO_DEMO_INPUT_PIN0                      P1_0
#define GPIO_DEMO_INPUT_PIN1                      P1_1
#define GPIO_DEMO_INPUT_PIN2                      P2_1
#define GPIO_DEMO_INPUT_PIN3                      P2_2

static uint8_t pin_0_state = 0;
static uint8_t pin_1_state = 0;
static uint8_t pin_2_state = 0;
static uint8_t pin_3_state = 0;


/** @} */ /* End of group Gpio_Interrupt_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gpio_Interrupt_Exported_Functions Gpio Interrupt Exported Functions
  * @brief
  * @{
  */
static void system_handler(void)
{
    if (System_WakeUpInterruptValue(GPIO_DEMO_INPUT_PIN0))
    {
        if (pin_0_state == 0)
        {
            pin_0_state = 1;
            System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN0, PAD_WAKEUP_POL_LOW);
        }
        else
        {
            pin_0_state = 0;
            System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN0, PAD_WAKEUP_POL_HIGH);
        }
        IO_PRINT_INFO1("system_handler: pin0 interrupt triggered, pin_0_state %d", pin_0_state);
    }

    if (System_WakeUpInterruptValue(GPIO_DEMO_INPUT_PIN1))
    {
        if (pin_1_state == 0)
        {
            pin_1_state = 1;
            System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN1, PAD_WAKEUP_POL_LOW);
        }
        else
        {
            pin_1_state = 0;
            System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN1, PAD_WAKEUP_POL_HIGH);
        }
        IO_PRINT_INFO1("system_handler: pin1 interrupt triggered, pin_1_state %d", pin_1_state);
    }


    if (System_WakeUpInterruptValue(GPIO_DEMO_INPUT_PIN2))
    {
        if (pin_2_state == 0)
        {
            pin_2_state = 1;
            System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN2, PAD_WAKEUP_POL_LOW);
        }
        else
        {
            pin_2_state = 0;
            System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN2, PAD_WAKEUP_POL_HIGH);
        }
        IO_PRINT_INFO1("system_handler: pin2 interrupt triggered, pin_2_state %d", pin_2_state);
    }


    if (System_WakeUpInterruptValue(GPIO_DEMO_INPUT_PIN3))
    {
        if (pin_3_state == 0)
        {
            pin_3_state = 1;
            System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN3, PAD_WAKEUP_POL_LOW);
        }
        else
        {
            pin_3_state = 0;
            System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN3, PAD_WAKEUP_POL_HIGH);
        }
        IO_PRINT_INFO1("system_handler: pin3 interrupt triggered, pin_3_state %d", pin_3_state);
    }

    Pad_ClearAllWakeupINT();
}

void pad_int_demo(void)
{
    IO_PRINT_INFO0("pad_int_demo: pad interrupt demo start");

    Pad_Config(GPIO_DEMO_INPUT_PIN0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN0, PAD_WAKEUP_POL_LOW);
    System_WakeUpInterruptEnable(GPIO_DEMO_INPUT_PIN0);

    Pad_Config(GPIO_DEMO_INPUT_PIN1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN1, PAD_WAKEUP_POL_LOW);
    System_WakeUpInterruptEnable(GPIO_DEMO_INPUT_PIN1);

    Pad_Config(GPIO_DEMO_INPUT_PIN2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN2, PAD_WAKEUP_POL_LOW);
    System_WakeUpInterruptEnable(GPIO_DEMO_INPUT_PIN2);

    Pad_Config(GPIO_DEMO_INPUT_PIN3, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    System_WakeUpPinEnable(GPIO_DEMO_INPUT_PIN3, PAD_WAKEUP_POL_LOW);
    System_WakeUpInterruptEnable(GPIO_DEMO_INPUT_PIN3);

    RamVectorTableUpdate(System_VECTORn, system_handler);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = System_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 4;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/** @} */ /* End of group Gpio_Interrupt_Exported_Functions */
/** @} */ /* End of group GPIO_INT_DEMO */

