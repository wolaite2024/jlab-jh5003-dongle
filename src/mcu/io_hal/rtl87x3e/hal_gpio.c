/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     hal_gpio.c
* @brief    This file provides all the gpio hal functions.
* @details
* @author
* @date
* @version  v0.1
*********************************************************************************************************
*/
#include <stdint.h>
#include <string.h>
#include "hal_def.h"
#include "trace.h"
#include "section.h"
#include "pm.h"
#include "rtl876x.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_gpio.h"
#include "io_dlps.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"
#define CONFIG_REALTEK_GPIOB_SUPPORT

#define TOTAL_HAL_GPIO_NUM                     (64)
#define TOTAL_HAL_GPIO_PORT_NUM                (32)

#define TOTAL_HAL_PIN_NUM                      TOTAL_PIN_NUM

//#define PRINT_GPIO_LOGS
#ifdef PRINT_GPIO_LOGS
#define GPIO_PRINT_INFO(fmt, ...)                DBG_DIRECT(fmt, __VA_ARGS__)
#else
#define GPIO_PRINT_INFO(...)
#endif

#define IS_GPIO_INVALID(gpio_num)                   (gpio_num == 0xff)
#define PIN_FLAG_TYPE_BIT                           (0x03)
#define PIN_GET_GPIO_TYPE(pin_index)                ((T_GPIO_TYPE)(hal_gpio_sw_context.pin_flags[pin_index] & PIN_FLAG_TYPE_BIT))

#define PIN_FLAG_FIRST_WAKE_UP_CONFIG_BIT           BIT2
#define PIN_FLAG_FIRST_WAKE_UP_CONFIG(pin_index)    (!(hal_gpio_sw_context.pin_flags[pin_index] & PIN_FLAG_FIRST_WAKE_UP_CONFIG_BIT))
#define PIN_FLAG_WAKE_UP_POLARITY_BIT               BIT3
#define PIN_FLAG_WAKE_UP_POLARITY(pin_index)        ((hal_gpio_sw_context.pin_flags[pin_index] & PIN_FLAG_WAKE_UP_POLARITY_BIT) >> 3)
#define PIN_FLAG_POLARITY_BIT                       BIT4
#define PIN_FLAG_POLARITY(pin_index)                ((hal_gpio_sw_context.pin_flags[pin_index] & PIN_FLAG_POLARITY_BIT) >> 4)
#define PIN_FLAG_NEED_STORE_BIT                     BIT5
#define PIN_FLAG_IS_NEED_STORE(pin_index)           ((hal_gpio_sw_context.pin_flags[pin_index] & PIN_FLAG_NEED_STORE_BIT))

typedef struct t_gpio_state
{
    uint8_t pin_flags[TOTAL_HAL_PIN_NUM];
    uint32_t debounce_time;
} T_GPIO_SW_CONTEXT;

T_GPIO_SW_CONTEXT hal_gpio_sw_context =
{
    .pin_flags = 0,
    .debounce_time = 30,
};

bool hal_gpio_init_flag = false;

extern void io_driver_dlps_register_exit_cb(P_IO_DLPS_CALLBACK func);
extern void io_driver_dlps_register_enter_cb(P_IO_DLPS_CALLBACK func);

ISR_TEXT_SECTION
GPIO_TypeDef *get_gpio(uint8_t gpio_num)
{
    if (gpio_num < 32)
    {
        return GPIOA;
    }
#ifdef CONFIG_REALTEK_GPIOB_SUPPORT
    else if (gpio_num < 64)
    {
        return GPIOB;
    }
#endif
#if CONFIG_REALTEK_GPIOC_SUPPORT
    else if (gpio_num < 96)
    {
        return GPIOC;
    }
#endif
    else
    {
        return NULL;
    }
}

T_GPIO_STATUS hal_gpio_set_pull_value(uint8_t pin_index, T_GPIO_PULL_VALUE pull_value)
{
    Pad_PullEnableValue_Dir(pin_index, 1, (PAD_Pull_Mode)pull_value);
    return GPIO_STATUS_OK;
}

static PAD_Pull_Mode hal_pull_to_pad_pull(T_GPIO_PULL_VALUE pull_value)
{
    switch (pull_value)
    {
    case GPIO_PULL_DOWN:
        return PAD_PULL_DOWN;
    case GPIO_PULL_UP:
        return PAD_PULL_UP;
    case GPIO_PULL_NONE:
        return PAD_PULL_NONE;
    default:
        return PAD_PULL_DOWN;
    }
}

static void hal_gpio_init_peri(void)
{
    if (hal_gpio_init_flag == false)
    {
        hal_gpio_init();
        hal_gpio_int_init();
    }
}

T_GPIO_STATUS hal_gpio_init_pin(uint8_t pin_index, T_GPIO_TYPE type, T_GPIO_DIRECTION direction,
                                T_GPIO_PULL_VALUE pull_value)
{
    uint8_t gpio_num = GPIO_GetNum(pin_index);

    if (IS_GPIO_INVALID(gpio_num))
    {
        return GPIO_STATUS_ERROR_PIN;
    }

    PAD_Mode aon_pad_mode = PAD_SW_MODE;
    PAD_PWR_Mode pad_power_mode = PAD_IS_PWRON;
    PAD_Pull_Mode aon_pad_pull = hal_pull_to_pad_pull(pull_value);
    PAD_OUTPUT_ENABLE_Mode pad_output_mode = (type == GPIO_TYPE_CORE) ? PAD_OUT_DISABLE :
                                             (PAD_OUTPUT_ENABLE_Mode)direction;
    PAD_OUTPUT_VAL output_value = PAD_OUT_LOW;

    hal_gpio_sw_context.pin_flags[pin_index] = (hal_gpio_sw_context.pin_flags[pin_index] &
                                                (~PIN_FLAG_TYPE_BIT)) | type;

    if (type != GPIO_TYPE_AON)
    {
        uint32_t gpio_pin = GPIO_GetPin(pin_index);
        GPIO_TypeDef *GPIOx = get_gpio(gpio_num);
        hal_gpio_init_peri();

        Pinmux_Config(pin_index, DWGPIO);
        GPIOx_ModeSet(GPIOx, gpio_pin, (GPIOMode_TypeDef)direction);
        aon_pad_mode = PAD_PINMUX_MODE;
    }

    Pad_Config(pin_index, aon_pad_mode, pad_power_mode, aon_pad_pull, pad_output_mode, output_value);

    return GPIO_STATUS_OK;
}

T_GPIO_TYPE hal_gpio_get_type(uint8_t pin_index)
{
    return PIN_GET_GPIO_TYPE(pin_index);
}

T_GPIO_STATUS hal_gpio_change_direction(uint8_t pin_index, T_GPIO_DIRECTION direction)
{
    T_GPIO_TYPE type = PIN_GET_GPIO_TYPE(pin_index);
    uint32_t gpio_pin = GPIO_GetPin(pin_index);
    uint8_t gpio_num = GPIO_GetNum(pin_index);
    GPIO_TypeDef *GPIOx = get_gpio(gpio_num);

    if (IS_GPIO_INVALID(gpio_num))
    {
        return GPIO_STATUS_ERROR_PIN;
    }

    switch (type)
    {
    case GPIO_TYPE_CORE:
        GPIOx_ModeSet(GPIOx, gpio_pin, (GPIOMode_TypeDef)direction);
        break;

    case GPIO_TYPE_AON:
        Pad_OutputEnableValue(pin_index, direction);
        break;

    case GPIO_TYPE_AUTO:
        Pad_OutputEnableValue(pin_index, direction);
        GPIOx_ModeSet(GPIOx, gpio_pin, (GPIOMode_TypeDef)direction);
        break;

    default:
        break;
    }

    return GPIO_STATUS_OK;
}

ISR_TEXT_SECTION
T_GPIO_STATUS hal_gpio_set_level(uint8_t pin_index, T_GPIO_LEVEL level)
{
    T_GPIO_TYPE type = PIN_GET_GPIO_TYPE(pin_index);
    uint8_t gpio_num = GPIO_GetNum(pin_index);

    if (IS_GPIO_INVALID(gpio_num))
    {
        return GPIO_STATUS_ERROR_PIN;
    }

    if (type ==  GPIO_TYPE_AON)
    {
        Pad_OutputControlValue(pin_index, level);
    }
    else
    {
        GPIOx_WriteBit(get_gpio(gpio_num), GPIO_GetPin(pin_index), (BitAction)level);
    }

    return GPIO_STATUS_OK;
}

ISR_TEXT_SECTION
T_GPIO_LEVEL hal_gpio_get_input_level(uint8_t pin_index)
{
    T_GPIO_TYPE type = PIN_GET_GPIO_TYPE(pin_index);
    uint8_t gpio_num = GPIO_GetNum(pin_index);

    if (IS_GPIO_INVALID(gpio_num) || (type == GPIO_TYPE_AON))
    {
        return GPIO_LEVEL_UNKNOWN;
    }

    return (T_GPIO_LEVEL)GPIOx_ReadInputDataBit(get_gpio(gpio_num), GPIO_GetPin(pin_index));
}

static T_GPIO_STATUS gpio_set_up_irq_active(uint8_t pin_index, T_GPIO_IRQ_MODE mode,
                                            T_GPIO_IRQ_POLARITY polarity,
                                            bool debounce_enable)
{
    uint32_t gpio_pin = GPIO_GetPin(pin_index);
    uint32_t gpio_num = GPIO_GetNum(pin_index);
    GPIO_TypeDef *p_gpio = get_gpio(gpio_num);

    if (IS_GPIO_INVALID(gpio_num))
    {
        return GPIO_STATUS_ERROR_PIN;
    }

    hal_gpio_sw_context.pin_flags[pin_index] = (hal_gpio_sw_context.pin_flags[pin_index] &
                                                (~PIN_FLAG_POLARITY_BIT)) | (polarity << 4);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_PinBit  = gpio_pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_ITCmd = ENABLE;
    GPIO_InitStruct.GPIO_ITTrigger = (GPIO_IRQ_EDGE == mode) ? GPIO_INT_Trigger_EDGE :
                                     GPIO_INT_Trigger_LEVEL;
    GPIO_InitStruct.GPIO_ITPolarity = (polarity == GPIO_IRQ_ACTIVE_HIGH) ?
                                      GPIO_INT_POLARITY_ACTIVE_HIGH : GPIO_INT_POLARITY_ACTIVE_LOW;
    GPIO_InitStruct.GPIO_ITDebounce = debounce_enable ? GPIO_INT_DEBOUNCE_ENABLE :
                                      GPIO_INT_DEBOUNCE_DISABLE;
    GPIO_InitStruct.GPIO_DebounceTime = hal_gpio_sw_context.debounce_time;

    GPIOx_Init(p_gpio, &GPIO_InitStruct);

    return GPIO_STATUS_OK;
}

T_GPIO_STATUS hal_gpio_set_up_irq(uint8_t pin_index, T_GPIO_IRQ_MODE mode,
                                  T_GPIO_IRQ_POLARITY polarity,
                                  bool debounce_enable)
{
    return gpio_set_up_irq_active(pin_index, mode, polarity, debounce_enable);
}

ISR_TEXT_SECTION
T_GPIO_STATUS hal_gpio_irq_change_polarity(uint8_t pin_index, T_GPIO_IRQ_POLARITY polarity)
{
    uint32_t gpio_num = GPIO_GetNum(pin_index);
    uint32_t gpio_pin = GPIO_GetPin(pin_index);

    if (IS_GPIO_INVALID(gpio_num))
    {
        return GPIO_STATUS_ERROR_PIN;
    }

    hal_gpio_sw_context.pin_flags[pin_index] = (hal_gpio_sw_context.pin_flags[pin_index] &
                                                (~PIN_FLAG_POLARITY_BIT)) | (polarity << 4);

    GPIOx_IntPolaritySet(get_gpio(gpio_num), gpio_pin, (GPIOIT_PolarityType)polarity);
    return GPIO_STATUS_OK;
}

ISR_TEXT_SECTION
T_GPIO_STATUS hal_gpio_irq_enable(uint8_t pin_index)
{
    uint32_t gpio_pin = GPIO_GetPin(pin_index);
    uint32_t gpio_num = GPIO_GetNum(pin_index);
    GPIO_TypeDef *p_gpio = get_gpio(gpio_num);
    T_GPIO_TYPE type = PIN_GET_GPIO_TYPE(pin_index);

    if (IS_GPIO_INVALID(gpio_num))
    {
        return GPIO_STATUS_ERROR_PIN;
    }

    GPIOx_MaskINTConfig(p_gpio, gpio_pin, (FunctionalState)DISABLE);
    GPIOx_INTConfig(p_gpio, gpio_pin, (FunctionalState)ENABLE);
    if (type == GPIO_TYPE_AUTO)
    {
        hal_gpio_sw_context.pin_flags[pin_index] |= PIN_FLAG_NEED_STORE_BIT;
    }
    return GPIO_STATUS_OK;
}

T_GPIO_STATUS hal_gpio_irq_disable(uint8_t pin_index)
{
    uint32_t gpio_pin = GPIO_GetPin(pin_index);
    uint32_t gpio_num = GPIO_GetNum(pin_index);
    GPIO_TypeDef *p_gpio = get_gpio(gpio_num);
    T_GPIO_TYPE type = PIN_GET_GPIO_TYPE(pin_index);

    if (IS_GPIO_INVALID(gpio_num))
    {
        return GPIO_STATUS_ERROR_PIN;
    }

    GPIOx_MaskINTConfig(p_gpio, gpio_pin, (FunctionalState)ENABLE);
    GPIOx_INTConfig(p_gpio, gpio_pin, (FunctionalState)DISABLE);
    if (type == GPIO_TYPE_AUTO)
    {
        hal_gpio_sw_context.pin_flags[pin_index] &= (~PIN_FLAG_NEED_STORE_BIT);
        hal_gpio_sw_context.pin_flags[pin_index] = hal_gpio_sw_context.pin_flags[pin_index] &
                                                   (~PIN_FLAG_FIRST_WAKE_UP_CONFIG_BIT);
        System_WakeUpPinDisable(pin_index);
        System_WakeUpInterruptDisable(pin_index);
    }
    return GPIO_STATUS_OK;
}

T_GPIO_STATUS hal_gpio_set_debounce_time(uint8_t ms)
{
    if (!ms)
    {
        ms = 1;
    }

    hal_gpio_sw_context.debounce_time = ms;

    return GPIO_STATUS_OK;
}

void hal_gpio_pm_store_config_wake_up(uint8_t pin_index)
{
    T_GPIO_TYPE type = PIN_GET_GPIO_TYPE(pin_index);

    if (type != GPIO_TYPE_AUTO)
    {
        return;
    }

    T_GPIO_IRQ_POLARITY polarity = (T_GPIO_IRQ_POLARITY)(PIN_FLAG_POLARITY(pin_index));

    if (PIN_FLAG_FIRST_WAKE_UP_CONFIG(pin_index) || (PIN_FLAG_WAKE_UP_POLARITY(pin_index) != polarity))
    {
        if (polarity == GPIO_IRQ_ACTIVE_HIGH)
        {
            System_WakeUpPinEnable(pin_index, PAD_WAKEUP_POL_HIGH);
        }
        else
        {
            System_WakeUpPinEnable(pin_index, PAD_WAKEUP_POL_LOW);
        }
        System_WakeUpInterruptEnable(pin_index);
        hal_gpio_sw_context.pin_flags[pin_index] = (hal_gpio_sw_context.pin_flags[pin_index] &
                                                    (~PIN_FLAG_WAKE_UP_POLARITY_BIT)) | (polarity << 3) | PIN_FLAG_FIRST_WAKE_UP_CONFIG_BIT;
    }
}

void hal_gpio_pm_store(void)
{
    for (uint8_t i = 0; i < TOTAL_HAL_PIN_NUM; i++)
    {
        if (PIN_FLAG_IS_NEED_STORE(i))
        {
            hal_gpio_pm_store_config_wake_up(i);
        }
    }
}

static void hal_gpio_pm_regsiter(void)
{
    static bool is_hal_gpio_pm_registered = false;
    if (!is_hal_gpio_pm_registered)
    {
        is_hal_gpio_pm_registered = true;
        io_driver_dlps_register_enter_cb(hal_gpio_pm_store);
    }
}

void hal_gpio_init(void)
{
    if (hal_gpio_init_flag)
    {
        return;
    }

    hal_gpio_init_flag = true;

    RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
#ifdef CONFIG_REALTEK_GPIOB_SUPPORT
    RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);
#endif
#ifdef CONFIG_REALTEK_GPIOC_SUPPORT
    RCC_PeriphClockCmd(APBPeriph_GPIOC, APBPeriph_GPIOC_CLOCK, ENABLE);
#endif
    hal_gpio_pm_regsiter();
}

void hal_gpio_deinit(void)
{
    hal_gpio_init_flag = false;

    RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, DISABLE);
#ifdef CONFIG_REALTEK_GPIOB_SUPPORT
    RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, DISABLE);
#endif
#ifdef CONFIG_REALTEK_GPIOC_SUPPORT
    RCC_PeriphClockCmd(APBPeriph_GPIOC, APBPeriph_GPIOC_CLOCK, DISABLE);
#endif
    memset(hal_gpio_sw_context.pin_flags, 0, TOTAL_HAL_PIN_NUM);
}

