/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "os_mem.h"
#include "trace.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"
#include "app_key_gpio.h"
#include "rtl876x_pinmux.h"
#include "hal_gpio_int.h"
#include "hal_gpio.h"
#include "app_io_msg.h"
#include "app_cfg.h"
#include "app_dlps.h"
#include "app_main.h"
#include "section.h"
#include "app_timer.h"
//#include "rtl876x_aon_reg.h"
//#include "app_sensor.h"
//#include "indirect_access.h"

#define GPIO_DETECT_DEBOUNCE_TIME   10

/* App define KEY_GPIO timer type */
typedef enum
{
    APP_IO_TIMER_KEY0_DEBOUNCE,
} T_APP_KEY_GPIO_TIMER;

static T_KEY_GPIO_DATA *p_gpio_data = NULL;
static uint8_t key_gpio_timer_id = 0;
static uint8_t timer_idx_key0_debounce = 0;
static uint8_t key0_debounce_initial_flag;

ISR_TEXT_SECTION
bool key_get_mfb_state(void)
{
    return (mfb_get_level());
}

RAM_TEXT_SECTION
void key0_gpio_intr_callback(uint32_t gpio_num)
{
    key0_gpio_intr_handler();
}

static void key0_debounce_handler(void)
{
    T_IO_MSG button_msg;
    uint8_t key_status;
    uint8_t key_status_update_fg = 0;

    p_gpio_data->reset_count = 0;

    key_status = p_gpio_data->key_status[0];

    if (app_cfg_const.key_enable_mask & KEY0_MASK)
    {
#ifndef MFB_KEY_SUPPORT
        if (key_status != hal_gpio_get_input_level(app_cfg_const.key_pinmux[0]))
        {
            /* Enable GPIO interrupt */
            hal_gpio_irq_enable(app_cfg_const.key_pinmux[0]);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
            return;
        }
#endif
    }
    else
    {
        mfb_irq_enable();
        if (key_status != key_get_mfb_state())
        {
            APP_PRINT_WARN0("MFB press not longer than debuance timer do nothing");
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
            return;
        }
    }
    button_msg.type = IO_MSG_TYPE_GPIO;
    button_msg.subtype = IO_MSG_GPIO_KEY;

    if (app_cfg_const.key_high_active_mask & BIT0)
    {
        if (key_status) //Button pressed
        {
#ifndef MFB_KEY_SUPPORT
            hal_gpio_irq_change_polarity(app_cfg_const.key_pinmux[0], GPIO_IRQ_ACTIVE_LOW);//Polarity Low
#endif
            if (p_gpio_data->key_press[0] == KEY_RELEASE)
            {
                button_msg.u.param = (KEY0_MASK << 8) | KEY_PRESS;
                key_status_update_fg = 1;
                p_gpio_data->key_press[0] = KEY_PRESS;
            }
        }
        else //Button released
        {
            /* Change GPIO Interrupt Polarity, Enable Interrupt */
#ifndef MFB_KEY_SUPPORT
            hal_gpio_irq_change_polarity(app_cfg_const.key_pinmux[0], GPIO_IRQ_ACTIVE_HIGH); //Polarity High
#endif
            if (p_gpio_data->key_press[0] == KEY_PRESS)
            {
                button_msg.u.param = (KEY0_MASK << 8) | KEY_RELEASE;
                key_status_update_fg = 1;
                p_gpio_data->key_press[0] = KEY_RELEASE;
            }
        }
    }
    else
    {
        if (key_status) //Button released
        {
            /* Change GPIO Interrupt Polarity, Enable Interrupt */
#ifndef MFB_KEY_SUPPORT
            hal_gpio_irq_change_polarity(app_cfg_const.key_pinmux[0], GPIO_IRQ_ACTIVE_LOW); //Polarity Low
#endif
            if (p_gpio_data->key_press[0] == KEY_PRESS)
            {
                button_msg.u.param = (KEY0_MASK << 8) | KEY_RELEASE;
                key_status_update_fg = 1;
                p_gpio_data->key_press[0] = KEY_RELEASE;
            }
        }
        else //Button pressed
        {
#ifndef MFB_KEY_SUPPORT
            hal_gpio_irq_change_polarity(app_cfg_const.key_pinmux[0], GPIO_IRQ_ACTIVE_HIGH); //Polarity High
#endif
            if (p_gpio_data->key_press[0] == KEY_RELEASE)
            {
                button_msg.u.param = (KEY0_MASK << 8) | KEY_PRESS;
                key_status_update_fg = 1;
                p_gpio_data->key_press[0] = KEY_PRESS;
            }
        }
    }

    /* Send MSG to APP task */
    if (key_status_update_fg)
    {
        app_io_msg_send(&button_msg);
    }
    else
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
    }
#ifndef MFB_KEY_SUPPORT
    if (app_cfg_const.key_enable_mask & KEY0_MASK)
    {
        /* Enable GPIO interrupt */
        hal_gpio_irq_enable(app_cfg_const.key_pinmux[0]);
    }
#endif
}

static void key_gpio_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_IO_TIMER_KEY0_DEBOUNCE:
        {
            if (key0_debounce_initial_flag == 0)
            {
                key0_debounce_handler();
            }
        }
        break;

    default:
        break;
    }
}

void key_gpio_initial(void)
{
    uint32_t i;
    T_GPIO_IRQ_POLARITY key0_irq_ploarity = GPIO_IRQ_ACTIVE_LOW;
    T_GPIO_PULL_VALUE pull_mode;

    if (MAX_KEY_NUM != 0)
    {
        p_gpio_data = (T_KEY_GPIO_DATA *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_KEY_GPIO_DATA));
        memset(p_gpio_data, 0, sizeof(T_KEY_GPIO_DATA));// set p_gpio_data->key_press[i] = KEY_RELEASE;

        app_timer_reg_cb(key_gpio_timeout_cb, &key_gpio_timer_id);

        //Key 0 is used at LPM_POWERDOWN_MODE wake up. Need to set level trigger
        if (app_cfg_const.key_enable_mask & KEY0_MASK)
        {
            key0_debounce_initial_flag = 1;
            app_start_timer(&timer_idx_key0_debounce, "key0_debounce",
                            key_gpio_timer_id, APP_IO_TIMER_KEY0_DEBOUNCE, 0, false,
                            GPIO_DETECT_DEBOUNCE_TIME);

            pull_mode = (app_cfg_const.key_high_active_mask & BIT(0)) ? GPIO_PULL_DOWN : GPIO_PULL_UP;
            hal_gpio_init_pin(app_cfg_const.key_pinmux[0], GPIO_TYPE_CORE, GPIO_DIR_INPUT, pull_mode);

            if (app_cfg_const.key_high_active_mask & BIT0)
            {
                key0_irq_ploarity = GPIO_IRQ_ACTIVE_HIGH;
            }

            hal_gpio_set_up_irq(app_cfg_const.key_pinmux[0], GPIO_IRQ_EDGE, key0_irq_ploarity, true);
        }

        /*key0 key 2  key4  key6  make as high edge trigger*/
        for (i = 1; i < MAX_KEY_NUM; i++)
        {
            if ((app_cfg_const.key_enable_mask & BIT(i))
                && (app_cfg_const.key_high_active_mask & BIT(i)))
            {
                hal_gpio_init_pin(app_cfg_const.key_pinmux[i], GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_DOWN);
                hal_gpio_set_up_irq(app_cfg_const.key_pinmux[i], GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_HIGH, true);
            }
        }

        for (i = 1; i < MAX_KEY_NUM; i++)
        {
            if ((app_cfg_const.key_enable_mask & BIT(i))
                && ((app_cfg_const.key_high_active_mask & BIT(i)) == 0))
            {
                hal_gpio_init_pin(app_cfg_const.key_pinmux[i], GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_UP);
                hal_gpio_set_up_irq(app_cfg_const.key_pinmux[i], GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
            }
        }

        /* Enable Interrupt (Peripheral, CPU NVIC) */
        for (i = 0; i < MAX_KEY_NUM; i++)
        {
            if (app_cfg_const.key_enable_mask & BIT(i))
            {
                if (app_cfg_const.key_high_active_mask & BIT(i))
                {
                    Pad_WakeupPolarityValue(app_cfg_const.key_pinmux[i], PAD_WAKEUP_POL_HIGH);
                }
                else
                {
                    Pad_WakeupPolarityValue(app_cfg_const.key_pinmux[i], PAD_WAKEUP_POL_LOW);
                }

                if (i == 0)
                {
                    hal_gpio_register_isr_callback(app_cfg_const.key_pinmux[0], key0_gpio_intr_callback, i);
                }
                else
                {
                    hal_gpio_register_isr_callback(app_cfg_const.key_pinmux[i], key_gpio_intr_callback, i);
                }
                hal_gpio_irq_enable(app_cfg_const.key_pinmux[i]);
            }
        }
    }
}

RAM_TEXT_SECTION
void key_gpio_intr_callback(uint32_t key_index)
{
    uint8_t key_mask = BIT(key_index);
    uint8_t gpio_index = app_cfg_const.key_pinmux[key_index];
    uint8_t key_status;
    uint8_t key_status_update_fg = 0;
    T_IO_MSG button_msg;

    /* Control of entering DLPS */
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    key_status = hal_gpio_get_input_level(gpio_index);

    APP_PRINT_TRACE3("key_gpio_intr_callback: key_mask =0x%x, gpio_index =%d, key_status %d", key_mask,
                     gpio_index, key_status);

    /* Disable GPIO interrupt */
    hal_gpio_irq_disable(gpio_index);

    button_msg.type = IO_MSG_TYPE_GPIO;
    button_msg.subtype = IO_MSG_GPIO_KEY;

    if (key_mask & app_cfg_const.key_high_active_mask) // high active
    {
        if (key_status) //Button pressed
        {
            hal_gpio_irq_change_polarity(gpio_index, GPIO_IRQ_ACTIVE_LOW); //Polarity Low
            if (p_gpio_data->key_press[key_index] == KEY_RELEASE)
            {
                button_msg.u.param = (key_mask << 8) | KEY_PRESS;
                key_status_update_fg = 1;
                p_gpio_data->key_press[key_index] = KEY_PRESS;
            }
        }
        else //Button released
        {
            /* Change GPIO Interrupt Polarity, Enable Interrupt */
            hal_gpio_irq_change_polarity(gpio_index, GPIO_IRQ_ACTIVE_HIGH); //Polarity High
            if (p_gpio_data->key_press[key_index] == KEY_PRESS)
            {
                button_msg.u.param = (key_mask << 8) | KEY_RELEASE;
                key_status_update_fg = 1;
                p_gpio_data->key_press[key_index] = KEY_RELEASE;
            }
        }
    }
    else // low active
    {
        if (key_status) //Button released
        {
            /* Change GPIO Interrupt Polarity, Enable Interrupt */
            hal_gpio_irq_change_polarity(gpio_index, GPIO_IRQ_ACTIVE_LOW); //Polarity Low
            if (p_gpio_data->key_press[key_index] == KEY_PRESS)
            {
                button_msg.u.param = (key_mask << 8) | KEY_RELEASE;
                key_status_update_fg = 1;
                p_gpio_data->key_press[key_index] = KEY_RELEASE;
            }
        }
        else //Button pressed
        {
            hal_gpio_irq_change_polarity(gpio_index, GPIO_IRQ_ACTIVE_HIGH); //Polarity High
            if (p_gpio_data->key_press[key_index] == KEY_RELEASE)
            {
                button_msg.u.param = (key_mask << 8) | KEY_PRESS;
                key_status_update_fg = 1;
                p_gpio_data->key_press[key_index] = KEY_PRESS;
            }
        }
    }

    /* Send MSG to APP task */
    if (key_status_update_fg)
    {
        app_io_msg_send(&button_msg);
    }
    else
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
    }

    /* Enable GPIO interrupt */
    hal_gpio_irq_enable(gpio_index);
}

void key0_gpio_intr_handler(void)
{
#ifndef MFB_KEY_SUPPORT
    if (!app_cfg_const.mfb_replace_key0)
    {
        APP_PRINT_TRACE0("key0_gpio_intr_handler");
    }
    else
    {
        APP_PRINT_TRACE0("MFB_intr_handler == key0_gpio_intr_handler");
    }
#else
    APP_PRINT_TRACE0("MFB_intr_handler == key0_gpio_intr_handler");
#endif
    /* Control of entering DLPS */
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    if (app_cfg_const.key_enable_mask & KEY0_MASK)
    {
#ifndef MFB_KEY_SUPPORT
        p_gpio_data->key_status[0] = hal_gpio_get_input_level(app_cfg_const.key_pinmux[0]);

        /* Disable GPIO interrupt */
        hal_gpio_irq_disable(app_cfg_const.key_pinmux[0]);
#endif
    }
    else
    {
        p_gpio_data->key_status[0] = key_get_mfb_state();
        mfb_irq_disable();
    }

    /* Delay 50ms to read the GPIO status */
    key0_debounce_initial_flag = 0;
    app_start_timer(&timer_idx_key0_debounce, "key0_debounce",
                    key_gpio_timer_id, APP_IO_TIMER_KEY0_DEBOUNCE, 0, false,
                    GPIO_DETECT_DEBOUNCE_TIME);

}

static void key_gpio_mfb_level_change_cb(void)
{
    key0_gpio_intr_handler();
}

/**
    * @brief  MFB NVIC interrupt initial.
    *         MFB needs to simulate the behavior of key0, here are some nvic interrupt settings
    * @param void
    * @return void
    */
//need update late
void key_mfb_init(void)
{
    mfb_init((P_MFB_LEVEL_CHANGE_CBACK)key_gpio_mfb_level_change_cb);

    Pad_WakeUpCmd(MFB_MODE, POL_LOW, ENABLE);

    key0_debounce_initial_flag = 1;
    app_start_timer(&timer_idx_key0_debounce, "key0_debounce",
                    key_gpio_timer_id, APP_IO_TIMER_KEY0_DEBOUNCE, 0, false,
                    GPIO_DETECT_DEBOUNCE_TIME);
}

