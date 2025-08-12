#include "hal_gpio.h"
#include "hal_gpio_int.h"
#include "rtk_hal_lcd.h"
#include "module_lcd_te.h"

static uint8_t lcd_te_pin;
static void GPIO_TE_HANDLER(uint32_t context);

static T_LCDC_TE_TYPE use_TE = LCDC_TE_TYPE_NO_TE;
static bool te_trigger_state = true;

void rtk_lcd_hal_set_TE_type(T_LCDC_TE_TYPE state)
{
    use_TE = state;
}

T_LCDC_TE_TYPE rtk_lcd_hal_get_TE_type(void)
{
    return use_TE;
}

void rtk_lcd_hal_set_te_trigger_state(bool state)
{
    te_trigger_state = state;
}

bool rtk_lcd_hal_get_te_trigger_state(void)
{
    return te_trigger_state;
}

void lcd_te_pin_config(uint8_t lcd_te)
{
    lcd_te_pin = lcd_te;
}

void lcd_te_device_init(void)
{
    hal_gpio_init();
    hal_gpio_int_init();
    hal_gpio_init_pin(lcd_te_pin, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_DOWN);
    hal_gpio_set_up_irq(lcd_te_pin, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, false);
    hal_gpio_register_isr_callback(lcd_te_pin, GPIO_TE_HANDLER, 0);
}

void rtk_lcd_hal_te_enable(void)
{
    hal_gpio_irq_enable(lcd_te_pin);
}

void rtk_lcd_hal_te_disable(void)
{
    hal_gpio_irq_disable(lcd_te_pin);
}

static void GPIO_TE_HANDLER(uint32_t context)
{
    rtk_lcd_hal_te_disable();

    rtk_lcd_hal_set_te_trigger_state(true);
}
