/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_key.c
* @brief      This file provides demo code of GPIO used as a key by level trigger mode with hw timer debounce.
* @details
* @author   renee
* @date     2017-3-10
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x.h"
#include <string.h>
#include "trace.h"
#include "hw_tim.h"
#include "section.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"

/** @defgroup  GPIO_KEY  GPIO KEY DEMO
    * @brief  Gpio key using hw tim to debounce implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Gpio_Key_Exported_Macros Gpio Key Exported Macros
  * @brief
  * @{
  */

#define KEY_PIN                       ADC_0

#define KEY_PRESS_DEBOUNCE_TIME       (30 * 1000)            //30ms
#define KEY_RELEASE_DEBOUNCE_TIME     (30 * 1000)            //30ms

/** @} */ /* End of group Gpio_Key_Exported_Macros */

static void key_handler(uint32_t key_index);

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup Gpio_Key_Exported_Variables Gpio Key Exported Variables
  * @brief
  * @{
  */
static uint8_t key_status = 1;
static uint8_t isPress = false;

static T_HW_TIMER_HANDLE debounce_timer_handle = NULL;

/** @} */ /* End of group Gpio_Key_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gpio_Key_Exported_Functions Gpio Key Exported Functions
  * @brief
  * @{
  */

/**
* @brief  When debouncing timer of key is timeout, this function shall be called.
*
*
* @param
* @return  void
*/
RAM_TEXT_SECTION
static void debounce_hw_timer_callback(T_HW_TIMER_HANDLE handle)
{
    hw_timer_stop(debounce_timer_handle);

    if (key_status != hal_gpio_get_input_level(KEY_PIN))
    {
        hal_gpio_irq_enable(KEY_PIN);
        return;
    }

    if (key_status)
    {
        hal_gpio_irq_change_polarity(KEY_PIN, GPIO_IRQ_ACTIVE_LOW);
        isPress = false;
        IO_PRINT_INFO0("debounce_hw_timer_callback: Key release");
    }
    else
    {
        hal_gpio_irq_change_polarity(KEY_PIN, GPIO_IRQ_ACTIVE_HIGH);
        isPress = true;
        IO_PRINT_INFO0("debounce_hw_timer_callback: Key press");
    }
    hal_gpio_irq_enable(KEY_PIN);
}

/**
* @brief  GPIO and TIM peripheral initial function.
*
*
* @param   none.
* @return  void
*/
void gpio_key(void)
{
    hal_gpio_init();
    hal_gpio_int_init();

    hal_gpio_init_pin(KEY_PIN, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_UP);
    hal_gpio_set_up_irq(KEY_PIN, GPIO_IRQ_LEVEL, GPIO_IRQ_ACTIVE_LOW, false);
    hal_gpio_register_isr_callback(KEY_PIN, key_handler, KEY_PIN);
    hal_gpio_irq_enable(KEY_PIN);

    debounce_timer_handle = hw_timer_create("debouce_hw_timer", KEY_PRESS_DEBOUNCE_TIME, true,
                                            debounce_hw_timer_callback);
    if (debounce_timer_handle == NULL)
    {
        IO_PRINT_ERROR0("gpio_key: fail to create hw timer, check hw timer usage");
        return;
    }
}


/**
* @brief  GPIO interrupt trigger by button is handled in this function.
*
*
* @param
* @return  void
*/
/**
* @brief  GPIO Interrupt handler
*
*
* @return  void
*/
static void key_handler(uint32_t key_index)
{
    /*  Disable GPIO interrupt */
    hal_gpio_irq_disable(key_index);

    key_status = hal_gpio_get_input_level(key_index);
    IO_PRINT_INFO1("key_handler: key_status %d", key_status);

    if (isPress == false)
    {
        hw_timer_restart(debounce_timer_handle, KEY_PRESS_DEBOUNCE_TIME);
    }
    else
    {
        hw_timer_restart(debounce_timer_handle, KEY_RELEASE_DEBOUNCE_TIME);
    }
}

/** @} */ /* End of group Gpio_Key_Exported_Functions */
/** @} */ /* End of group GPIO_KEY */

