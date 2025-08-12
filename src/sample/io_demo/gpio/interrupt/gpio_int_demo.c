/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_int_demo.c
* @brief    This file provides demo code of GPIO interrupt mode.
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
#include "section.h"
#include "rtl876x_pinmux.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"

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

/** @} */ /* End of group Gpio_Interrupt_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gpio_Interrupt_Exported_Functions Gpio Interrupt Exported Functions
  * @brief
  * @{
  */
ISR_TEXT_SECTION
static void gpio_isr_cb(uint32_t context)
{
    uint8_t pin_index = (uint32_t)context;
    T_GPIO_LEVEL gpio_level = hal_gpio_get_input_level(pin_index);

    IO_PRINT_INFO2("gpio_isr_cb: pin_name %s, gpio_level %d", TRACE_STRING(Pad_GetPinName(pin_index)),
                   gpio_level);

    if (gpio_level == GPIO_LEVEL_LOW)
    {
        hal_gpio_irq_change_polarity(pin_index, GPIO_IRQ_ACTIVE_HIGH);
    }
    else
    {
        hal_gpio_irq_change_polarity(pin_index, GPIO_IRQ_ACTIVE_LOW);
    }
}

void gpio_int_demo(void)
{
    IO_PRINT_INFO0("gpio_int_demo");

    hal_gpio_init();
    hal_gpio_int_init();
    hal_gpio_set_debounce_time(30);

    hal_gpio_init_pin(GPIO_DEMO_INPUT_PIN0, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_UP);
    hal_gpio_init_pin(GPIO_DEMO_INPUT_PIN1, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_DOWN);
    hal_gpio_init_pin(GPIO_DEMO_INPUT_PIN2, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_UP);
    hal_gpio_init_pin(GPIO_DEMO_INPUT_PIN3, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_DOWN);

    hal_gpio_set_up_irq(GPIO_DEMO_INPUT_PIN0, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
    hal_gpio_set_up_irq(GPIO_DEMO_INPUT_PIN1, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_HIGH, true);
    hal_gpio_set_up_irq(GPIO_DEMO_INPUT_PIN2, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
    hal_gpio_set_up_irq(GPIO_DEMO_INPUT_PIN3, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_HIGH, true);

    hal_gpio_register_isr_callback(GPIO_DEMO_INPUT_PIN0, gpio_isr_cb, GPIO_DEMO_INPUT_PIN0);
    hal_gpio_register_isr_callback(GPIO_DEMO_INPUT_PIN1, gpio_isr_cb, GPIO_DEMO_INPUT_PIN1);
    hal_gpio_register_isr_callback(GPIO_DEMO_INPUT_PIN2, gpio_isr_cb, GPIO_DEMO_INPUT_PIN2);
    hal_gpio_register_isr_callback(GPIO_DEMO_INPUT_PIN3, gpio_isr_cb, GPIO_DEMO_INPUT_PIN3);

    hal_gpio_irq_enable(GPIO_DEMO_INPUT_PIN0);
    hal_gpio_irq_enable(GPIO_DEMO_INPUT_PIN1);
    hal_gpio_irq_enable(GPIO_DEMO_INPUT_PIN2);
    hal_gpio_irq_enable(GPIO_DEMO_INPUT_PIN3);
}

/** @} */ /* End of group Gpio_Interrupt_Exported_Functions */
/** @} */ /* End of group GPIO_INT_DEMO */

