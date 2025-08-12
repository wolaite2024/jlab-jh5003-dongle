
/*
 * Copyright (c) 2020, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stdint.h>
#include <string.h>
#include "os_mem.h"
#include "os_sched.h"
#include "os_msg.h"
#include "os_task.h"
#include "os_sched.h"
#include "rtl876x_pinmux.h"
//#include "rtl876x_io_dlps.h"
#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "hal_gpio_int.h"
#include "hal_gpio.h"
#include "trace.h"
#include "audio.h"
#include "app_timer.h"
#include "app_io_msg.h"
#include "section.h"
#include "app_dlps.h"
#include "app_switch_detect.h"
#include "app_cfg.h"
#include "app_mmi.h"

#define AUX_SWITCH_BIT_MASK 0x02
#define AUX_DETECT_BIT_MASK 0x04

typedef enum
{
    APP_TIMER_GPIO_SWITCH_DETECT_DEBOUNCE_TIMEOUT,
} T_APP_SWITCH_DETECT_TIMER;

uint8_t aux_tx_rx_switch_state = 0; // 1 for Tx , 2 for Rx , 0 for not support
uint8_t aux_in_out_detect_state = 0;// 1 for in, 2 for out , 0 for not support
static uint8_t switch_detect_timer_id = 0;
static uint8_t timer_idx_switch_detect_gpio_debounce = 0;

uint8_t app_aux_switch_state_get()
{
    if (app_cfg_const.switch_detect_support)
    {
        return aux_tx_rx_switch_state;
    }
    return 0;
}

uint8_t app_aux_detect_state_get()
{
    if (app_cfg_const.line_in_support)
    {
        return aux_in_out_detect_state;
    }
    return 0;
}

void app_update_switch_detect_state()
{
    if (app_cfg_const.switch_detect_support)
    {
        if (hal_gpio_get_input_level(app_cfg_const.switch_detect_pinmux) ==
            app_cfg_const.switch_detect_tx_polarity)
        {
            aux_tx_rx_switch_state = AUX_SWITCH_TX;
        }
        else
        {
            aux_tx_rx_switch_state = AUX_SWITCH_RX;
        }
    }
    if (app_cfg_const.line_in_support)
    {
        if (hal_gpio_get_input_level(app_cfg_const.line_in_pinmux) ==
            app_cfg_const.aux_detect_in_polarity)
        {
            aux_in_out_detect_state = AUX_DETECT_IN;
        }
        else
        {
            aux_in_out_detect_state = AUX_DETECT_OUT;
        }
    }
}

void app_switch_detect_init_gpio_init()
{
    if (app_cfg_const.switch_detect_support)
    {
        hal_gpio_init_pin(app_cfg_const.switch_detect_pinmux, GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_UP);
        hal_gpio_set_up_irq(app_cfg_const.switch_detect_pinmux, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
        hal_gpio_register_isr_callback(app_cfg_const.switch_detect_pinmux, gpio_aux_switch_intr_handler, 0);

        hal_gpio_irq_enable(app_cfg_const.switch_detect_pinmux);
    }
    if (app_cfg_const.line_in_support)
    {
        hal_gpio_init_pin(app_cfg_const.line_in_pinmux, GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_UP);
        hal_gpio_set_up_irq(app_cfg_const.line_in_pinmux, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
        hal_gpio_register_isr_callback(app_cfg_const.line_in_pinmux, gpio_aux_detect_intr_handler, 0);

        /* Enable Interrupt (Peripheral, CPU NVIC) */
        hal_gpio_irq_enable(app_cfg_const.line_in_pinmux);
    }
}

APP_FLASH_RAM_TEXT_SECTION void gpio_aux_switch_intr_handler(void)
{
    T_IO_MSG gpio_msg;
    APP_PRINT_TRACE0("gpio_switch_intr_handler");
    /* Disable GPIO interrupt */
    hal_gpio_irq_disable(app_cfg_const.switch_detect_pinmux);
    /* Change GPIO Interrupt Polarity */
    if (hal_gpio_get_input_level(app_cfg_const.switch_detect_pinmux) ==
        app_cfg_const.switch_detect_tx_polarity)
    {
        hal_gpio_irq_change_polarity(app_cfg_const.switch_detect_pinmux,
                                     GPIO_IRQ_ACTIVE_LOW);
    }
    else
    {
        hal_gpio_irq_change_polarity(app_cfg_const.switch_detect_pinmux,
                                     GPIO_IRQ_ACTIVE_HIGH);
    }
    gpio_msg.type = IO_MSG_TYPE_GPIO;
    gpio_msg.subtype = IO_MSG_GPIO_AUX_SWITCH_DETECT;
    gpio_msg.u.param = AUX_SWITCH_BIT_MASK ;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("gpio_aux_switch_detect_intr_handler: Send Chargerbox detect msg error");
    }

    /* Enable GPIO interrupt */
    hal_gpio_irq_enable(app_cfg_const.switch_detect_pinmux);
}


APP_FLASH_RAM_TEXT_SECTION void gpio_aux_detect_intr_handler()
{
    T_IO_MSG gpio_msg;
    APP_PRINT_TRACE0("gpio_aux_detect_intr_handler");
    /* Disable GPIO interrupt */
    hal_gpio_irq_disable(app_cfg_const.line_in_pinmux);

    /* Change GPIO Interrupt Polarity */
    if (hal_gpio_get_input_level(app_cfg_const.line_in_pinmux) ==
        app_cfg_const.switch_detect_tx_polarity)
    {
        hal_gpio_irq_change_polarity(app_cfg_const.line_in_pinmux,
                                     GPIO_IRQ_ACTIVE_LOW);
    }
    else
    {
        hal_gpio_irq_change_polarity(app_cfg_const.line_in_pinmux,
                                     GPIO_IRQ_ACTIVE_HIGH);
    }
    gpio_msg.type = IO_MSG_TYPE_GPIO;
    gpio_msg.subtype = IO_MSG_GPIO_AUX_SWITCH_DETECT;
    gpio_msg.u.param = AUX_DETECT_BIT_MASK ;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("gpio_aux_switch_detect_intr_handler: Send Chargerbox detect msg error");
    }

    /* Enable GPIO interrupt */
    hal_gpio_irq_enable(app_cfg_const.line_in_pinmux);
}

void app_switch_detect_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    uint16_t param = io_driver_msg_recv->u.param;
    APP_PRINT_INFO1("app_switch_detect_handle_msg param = %x", param);

    // Disallow DLPS when debouncing.
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);
    app_start_timer(&timer_idx_switch_detect_gpio_debounce, "gpio_switch_detect_detect_debounce",
                    switch_detect_timer_id, APP_TIMER_GPIO_SWITCH_DETECT_DEBOUNCE_TIMEOUT, param,
                    500);
}

void app_switch_detect_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE4("app_switch_detect_timeout_cb: timer_evt 0x%02x, param %x, "
                     "curr_tx_rx_state %d, curr_in_out_state %d",
                     timer_evt, param, aux_tx_rx_switch_state, aux_in_out_detect_state);
    app_stop_timer(&timer_idx_switch_detect_gpio_debounce);
    switch (timer_evt)
    {
    case APP_TIMER_GPIO_SWITCH_DETECT_DEBOUNCE_TIMEOUT:
        {
            if (param & AUX_SWITCH_BIT_MASK)
            {
                uint8_t switch_gpio_input = hal_gpio_get_input_level(
                                                app_cfg_const.switch_detect_pinmux);
                if (switch_gpio_input == app_cfg_const.switch_detect_tx_polarity)
                {
                    if (aux_tx_rx_switch_state == AUX_SWITCH_RX)
                    {
                        aux_tx_rx_switch_state = AUX_SWITCH_TX;
                        app_mmi_handle_action(0xfa);
                    }
                }
                else
                {
                    if (aux_tx_rx_switch_state == AUX_SWITCH_TX)
                    {
                        aux_tx_rx_switch_state = AUX_SWITCH_RX;
                        app_mmi_handle_action(0xfb);
                    }
                }
            }
            else if (param & AUX_DETECT_BIT_MASK)
            {
                if (hal_gpio_get_input_level(app_cfg_const.line_in_pinmux) ==
                    app_cfg_const.aux_detect_in_polarity)
                {
                    if (aux_in_out_detect_state == AUX_DETECT_OUT)
                    {
                        aux_in_out_detect_state = AUX_DETECT_IN;
                        app_mmi_handle_action(0xf8);
                    }
                }
                else
                {
                    if (aux_in_out_detect_state == AUX_DETECT_IN)
                    {
                        aux_in_out_detect_state = AUX_DETECT_OUT;
                        app_mmi_handle_action(0xf9);
                    }
                }
            }
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
            break;
        }
    default:
        break;
    }
}

void app_switch_detect_init(void)
{
    aux_tx_rx_switch_state = AUX_SWITCH_TX;
    aux_in_out_detect_state = AUX_DETECT_OUT;
    if (app_cfg_const.switch_detect_support ||
        app_cfg_const.line_in_support)//rtk_switch_detect
    {
        app_switch_detect_init_gpio_init();
        app_timer_reg_cb(app_switch_detect_timeout_cb, &switch_detect_timer_id);
        app_update_switch_detect_state();
    }
}
