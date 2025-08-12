/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_input_demo.c
* @brief      This file provides demo code of GPIO input mode.
* @details
* @author   renee
* @date     2017-06-23
* @version  v1.0
*********************************************************************************************************
*/



/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "rtl876x.h"

#include "hal_gpio.h"
/** @defgroup  GPIO_INPUT_DEMO  GPIO INPUT DEMO
    * @brief  Gpio read input data implementation demo code
    * @{
    */


/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup GPIO_Input_Exported_Macros Gpio Input Exported Macros
  * @brief
  * @{
  */
#define TEST_Pin            ADC_1

/** @} */ /* End of group GPIO_Input_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup GPIO_Input_Exported_Functions Gpio Input Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  Initialize gpio
  * @param   No parameter.
  * @return  void
  */
static void driver_gpio_init(void)
{
    hal_gpio_init();
    hal_gpio_init_pin(TEST_Pin, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_UP);
}

/**
  * @brief  Get gpio level
  * @param   No parameter.
  * @return  void
  */
static void gpio_test(void)
{
    T_GPIO_LEVEL gpio_value;

    gpio_value  = hal_gpio_get_input_level(TEST_Pin);
    IO_PRINT_INFO1("gpio_test: gpio_value %d", gpio_value);
}

/**
  * @brief  demo code of operation get gpio input value
  * @param   No parameter.
  * @return  void
  */
void gpio_input_demo(void)
{
    IO_PRINT_INFO0("gpio_input_demo");
    /* Initialize GPIO peripheral */
    driver_gpio_init();
    /* GPIO function */
    gpio_test();
}
/** @} */ /* End of group GPIO_Input_Exported_Functions */
/** @} */ /* End of group GPIO_INPUT_DEMO */

