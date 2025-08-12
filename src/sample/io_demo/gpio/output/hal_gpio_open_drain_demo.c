/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     hal_gpio_open_drain_demo.c
* @brief    This file provides demo code of hal gpio open drain output mode.
* @details
* @author   colin
* @date     2023-12-27
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "rtl876x_pinmux.h"
#include "hal_gpio.h"
/** @defgroup  HAL_GPIO_OPEN_DRAIN_DEMO  hal gpio open drain demo
    * @brief  Hal gpio open drain output implementation demo code
    * @{
    */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Gpio_Output_Exported_Macros Gpio Output Exported Macros
  * @brief
  * @{
  */

#define TEST_PIN            P2_1
#define TEST_PIN_2          P2_2
#define TEST_PIN_3          P1_0
#define TEST_PIN_4          P1_1

/** @} */ /* End of group Gpio_Output_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Hal_Gpio_Open_Drain_Exported_Functions Hal gpio open drain Exported Functions
  * @brief
  * @{
  */

void hal_gpio_open_drian_output_high(uint8_t pin_index)
{
    hal_gpio_change_direction(pin_index, GPIO_DIR_INPUT);
}

void hal_gpio_open_drian_output_low(uint8_t pin_index)
{
    hal_gpio_change_direction(pin_index, GPIO_DIR_OUTPUT);
    hal_gpio_set_level(pin_index, GPIO_LEVEL_LOW);
}
/**
  * @brief  demo code of operation about GPIO.
  * @param   No parameter.
  * @return  void
  */
void hal_gpio_open_drain_demo(void)
{
    IO_PRINT_INFO0("hal_gpio_open_drain_demo: start");

    hal_gpio_init();
    hal_gpio_init_pin(TEST_PIN, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT, GPIO_PULL_NONE);
    hal_gpio_init_pin(TEST_PIN_2, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT, GPIO_PULL_NONE);
    hal_gpio_init_pin(TEST_PIN_3, GPIO_TYPE_AON, GPIO_DIR_OUTPUT, GPIO_PULL_NONE);
    hal_gpio_init_pin(TEST_PIN_4, GPIO_TYPE_AON, GPIO_DIR_OUTPUT, GPIO_PULL_NONE);

    hal_gpio_open_drian_output_high(TEST_PIN);
    hal_gpio_open_drian_output_low(TEST_PIN_2);
    hal_gpio_open_drian_output_high(TEST_PIN_3);
    hal_gpio_open_drian_output_low(TEST_PIN_4);
}

/** @} */ /* End of group Hal_Gpio_Open_Drain_Exported_Functions */
/** @} */ /* End of group HAL_GPIO_OPEN_DRAIN_DEMO */

