/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_output_demo.c
* @brief    This file provides demo code of GPIO output mode.
* @details
* @author   elliot chen
* @date     2015-06-10
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "rtl876x.h"

#include "hal_gpio.h"
/** @defgroup  GPIO_OUTPUT_DEMO  GPIO OUTPUT DEMO
    * @brief  Gpio output data implementation demo code
    * @{
    */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Gpio_Output_Exported_Macros Gpio Output Exported Macros
  * @brief
  * @{
  */

#define TEST_PIN            P0_1
#define TEST_PIN_2          P0_2

/** @} */ /* End of group Gpio_Output_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gpio_Output_Exported_Functions Gpio Output Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  demo code of operation about GPIO.
  * @param   No parameter.
  * @return  void
  */
void gpio_output_demo(void)
{
    IO_PRINT_INFO0("gpio_output_demo: start");

    hal_gpio_init();
    hal_gpio_init_pin(TEST_PIN, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT, GPIO_PULL_UP);
    hal_gpio_init_pin(TEST_PIN_2, GPIO_TYPE_AON, GPIO_DIR_OUTPUT, GPIO_PULL_UP);

    for (uint16_t i = 0; i < 10; i++)
    {
        hal_gpio_set_level(TEST_PIN, GPIO_LEVEL_LOW);
        hal_gpio_set_level(TEST_PIN_2, GPIO_LEVEL_LOW);
        hal_gpio_set_level(TEST_PIN, GPIO_LEVEL_HIGH);
        hal_gpio_set_level(TEST_PIN_2, GPIO_LEVEL_HIGH);
    }
}

/** @} */ /* End of group Gpio_Output_Exported_Functions */
/** @} */ /* End of group GPIO_OUTPUT_DEMO */

