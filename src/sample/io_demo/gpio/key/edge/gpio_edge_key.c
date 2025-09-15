/**
*********************************************************************************************************
*               Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_edge_key.c
* @brief     This file provides demo code of GPIO used as a key by edge trigger mode with hw debounce.
* @details
* @author   colin
* @date     2025-04-16
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x.h"
#include "trace.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"

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

#define KEY_DEBOUNCE_TIME             (10)            //10ms

/** @} */ /* End of group Gpio_Edge_Key_Exported_Macros */

static void key_handler(uint32_t key_index);

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
* @brief  GPIO and TIM peripheral initial function.
* @param   none.
* @return  void
*/
static void key_init(void)
{
    hal_gpio_init();
    hal_gpio_int_init();
    hal_gpio_set_debounce_time(KEY_DEBOUNCE_TIME);

    hal_gpio_init_pin(KEY_PIN, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_UP);
    hal_gpio_set_up_irq(KEY_PIN, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
    hal_gpio_register_isr_callback(KEY_PIN, key_handler, KEY_PIN);
    hal_gpio_irq_enable(KEY_PIN);
}

/**
  * @brief  demo code of operation about GPIO key.
  * @param   No parameter.
  * @return  void
  */
void gpio_edge_key(void)
{
    /* Initialize GPIO peripheral */
    key_init();
}

/**
* @brief  GPIO Interrupt handler
* @return  void
*/
static void key_handler(uint32_t key_index)
{
    /*  Disable GPIO interrupt */
    hal_gpio_irq_disable(key_index);

    key_status = hal_gpio_get_input_level(key_index);

    if (key_status)
    {
        IO_PRINT_INFO0("key_handler: Key release");
        hal_gpio_irq_change_polarity(key_index, GPIO_IRQ_ACTIVE_LOW);
    }
    else
    {
        IO_PRINT_INFO0("key_handler: Key press");
        hal_gpio_irq_change_polarity(key_index, GPIO_IRQ_ACTIVE_HIGH);
    }

    /*  Enable GPIO interrupt */
    hal_gpio_irq_enable(key_index);
}

/** @} */ /* End of group Gpio_Edge_Key_Exported_Functions */
/** @} */ /* End of group GPIO_EDGE_KEY */

