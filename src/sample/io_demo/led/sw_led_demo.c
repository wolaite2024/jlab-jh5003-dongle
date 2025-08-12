/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     sw_led_demo.c
* @brief    This file provides  implement of app timer(SW) or pwm timer(HW) out blink or breath led.
* @details
* @author   renee
* @date     2020-12-15
* @version  v0.1
*********************************************************************************************************
*/

#include <string.h>

#include "trace.h"
#include "os_sched.h"
#include "os_mem.h"
#include "os_sync.h"
#include "rtl876x.h"

#include "rtl876x_pinmux.h"

#include "pwm.h"
#include "hw_tim.h"
#include "sw_led_demo.h"
#include "app_timer.h"


#define LED_OUT_0       P2_1
#define LED_OUT_1       P2_2
#define LED_OUT_2       P2_7
#define LED_OUT_3       P1_6
#define LED_OUT_4       P1_7
#define LED_OUT_5       P0_1

#define LED_DEMO_NUM            3//MAX: 6 LED
#define LED_DEMO_NUM_MAX            6
#define PWM_OUT_COUNT 10000 //time Unit: 10ms(10000/1MHz)


static uint8_t LED_PIN_TABLE[LED_DEMO_NUM_MAX] =
{
    LED_OUT_0,
    LED_OUT_1,
    LED_OUT_2,
    LED_OUT_3,
    LED_OUT_4,
    LED_OUT_5,
};

static T_LED_TABLE LED_TABLE[2][LED_DEMO_NUM_MAX] =
{
    //normal led
    {
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        /*mode  , on time < Unit: 10ms>,  off time  < Unit: 10ms>, blink times for one gruop blink , each group blink interval< Unit: 100ms> */
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
    },

    //breath led
    {
        {LED_TYPE_BREATH, 200, 200, 2, 0},
        /*mode  , on time < Unit: 10ms>,  off time  < Unit: 10ms>, breath times for one gruop breath , each group breath interval< Unit: 100ms> */
        {LED_TYPE_BREATH, 200, 200, 2, 20},
        {LED_TYPE_BREATH, 200, 200, 2, 20},
        {LED_TYPE_BREATH, 200, 200, 2, 20},
        {LED_TYPE_BREATH, 200, 200, 2, 20},
        {LED_TYPE_BREATH, 200, 200, 2, 20},
    },
};



typedef struct
{
    uint32_t led_breath_timer_count;
    uint32_t led_breath_blink_count;
    uint8_t pwm_tim_id;
} T_LED_SW_BREATH_MODE;

static T_LED_SW_BREATH_MODE sw_breathe_mode[LED_DEMO_NUM];
static T_PWM_HANDLE pwm_tim_handle[LED_DEMO_NUM];
static T_LED_TABLE led_setting[LED_DEMO_NUM];
static uint32_t led_create_timer_point[LED_DEMO_NUM];
static uint32_t led_create_timer_record[LED_DEMO_NUM];
static uint8_t led_polarity[LED_DEMO_NUM];
static bool led_driver_by_sw = 0; //= true;
static const char *led_timer_name[LED_DEMO_NUM_MAX] = {"led0_on_off", "led1_on_off", "led2_on_off", "led3_on_off", "led4_on_off", "led5_on_off"};
static uint8_t timer_idx_led_blink[LED_DEMO_NUM] = {0};
static uint8_t led_module_timer_id = 0;
static T_LED_TABLE led_setting_record[LED_DEMO_NUM];
static T_LED_TABLE led_count_record[LED_DEMO_NUM];

static void led_demo_clear_para(T_LED_CHANNEL led_ch);
static void led_demo_blink_handle(T_LED_CHANNEL led_ch);
static void led_demo_deinit(T_LED_CHANNEL led_ch);
static void led_demo_breath_timer_control(uint8_t led_index, FunctionalState new_state);
static void led_demo_breath_init(uint8_t led_index);

static inline uint8_t log2_u32(uint32_t val)
{
    return (31 - __builtin_clz(val));
}

static void led_demo_set_active_polarity(T_LED_CHANNEL led_ch, T_LED_ACTIVE_POLARITY polarity)
{
    uint8_t led_idx = log2_u32(led_ch);

    led_polarity[led_idx] = polarity;
}

static uint8_t led_demo_get_pin_num(T_LED_CHANNEL led_ch)
{
    uint8_t pin_num;

    switch (led_ch)
    {
    case LED_CH_0:
        pin_num = LED_OUT_0;
        break;

    case LED_CH_1:
        pin_num = LED_OUT_1;
        break;

    case LED_CH_2:
        pin_num = LED_OUT_2;
        break;

    case LED_CH_3:
        pin_num = LED_OUT_3;
        break;

    case LED_CH_4:
        pin_num = LED_OUT_4;
        break;

    case LED_CH_5:
        pin_num = LED_OUT_5;
        break;

    default:
        break;
    }

    return pin_num;
}

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void led_demo_board_led_init(void)
{
    uint8_t led_idx;

    for (led_idx = 0; led_idx < LED_DEMO_NUM; led_idx++)
    {

        led_demo_set_active_polarity((T_LED_CHANNEL)(BIT(led_idx)), LED_ACTIVE_POLARITY_HIGH);
        Pad_Config(LED_PIN_TABLE[led_idx],
                   PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
}

static void led_demo_cmd_handle(T_LED_CHANNEL led_ch, bool state)
{
    uint8_t led_idx = log2_u32(led_ch);
    uint32_t s;
    IO_PRINT_INFO3("led_demo_cmd_handle: led_idx %d, led_driver_by_sw %d, state %d",
                   led_idx, led_driver_by_sw, state);
    if (led_driver_by_sw)
    {
        //LED should be handled simultaneously
        //Avoid APP be preempted by higher priority task
        s = os_lock();

        if (state)
        {
            //set pad and start timer
            led_demo_blink_handle(led_ch);
        }
        else
        {
            //reset pad and stop timer
            led_demo_deinit(led_ch);
            led_demo_clear_para(led_ch);
            app_stop_timer(&timer_idx_led_blink[led_idx]);
        }

        os_unlock(s);
    }
}

static void led_demo_deinit(T_LED_CHANNEL led_ch)
{
    uint8_t led_idx = log2_u32(led_ch);
    uint8_t pin_num = led_demo_get_pin_num(led_ch);

    if (led_driver_by_sw)
    {
        if (led_setting_record[led_idx].type == LED_TYPE_BREATH)
        {
            IO_PRINT_INFO2("led_demo_deinit: pwm_pin_config, led_idx %d, pin_num %d", led_idx, pin_num);
            pwm_pin_config(pwm_tim_handle[led_idx], pin_num, PWM_FUNC);
        }
        else
        {
            //reset pad
            IO_PRINT_INFO2("led_demo_deinit: by HW, led_idx %d, pin_num %d", led_idx, pin_num);
            Pad_Config(pin_num,
                       PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, (PAD_OUTPUT_VAL)led_polarity[led_idx]);
        }
    }
}

static void led_demo_config(T_LED_CHANNEL led_ch, T_LED_TABLE *table)
{
    uint8_t led_idx = log2_u32(led_ch);

    if (led_driver_by_sw)
    {
        led_setting_record[led_idx].type = table->type;// new set type
        led_setting_record[led_idx].on_time = table->on_time;
        led_setting_record[led_idx].off_time = table->off_time;
        led_setting_record[led_idx].blink_count = 2 * table->blink_count;
        led_setting_record[led_idx].blink_interval = table->blink_interval;

        led_count_record[led_idx].blink_count = 2 * table->blink_count;
        led_demo_deinit(led_ch);
    }
}

static void led_demo_create_timer(uint32_t time, uint16_t led_ch)
{
    uint8_t led_idx = log2_u32(led_ch);


    led_create_timer_point[led_idx] = os_sys_time_get();
    led_create_timer_record[led_idx] = time;

    app_start_timer(&timer_idx_led_blink[led_idx], led_timer_name[led_idx],
                    led_module_timer_id, 0, led_ch, false,
                    time);
    IO_PRINT_INFO3("led_demo_create_timer: led_idx 0x%x, time %d, led_create_timer_point %d",
                   led_idx, time, led_create_timer_point[led_idx]);
}

static void led_demo_blink_handle(T_LED_CHANNEL led_ch)
{
    uint8_t led_idx = log2_u32(led_ch);
    uint8_t pin_num = led_demo_get_pin_num(led_ch);

    IO_PRINT_INFO3("led_demo_blink_handle: led_idx %d, led_setting_record[led_idx].type %d, pin_num %d",
                   led_idx, led_setting_record[led_idx].type, pin_num);

    if (pin_num == 0xFF)
    {
        return;
    }

    if ((led_setting_record[led_idx].type == LED_TYPE_KEEP_ON) ||
        (led_setting_record[led_idx].type == LED_TYPE_ON_OFF))
    {
        if (led_polarity[led_idx])
        {
            Pad_OutputControlValue(pin_num, PAD_OUT_LOW);
        }
        else
        {
            Pad_OutputControlValue(pin_num, PAD_OUT_HIGH);
        }
    }
    else
    {
        led_demo_deinit(led_ch);
    }

    if ((led_setting_record[led_idx].type == LED_TYPE_ON_OFF) ||
        (led_setting_record[led_idx].type == LED_TYPE_OFF_ON))
    {
        led_demo_create_timer(led_setting_record[led_idx].on_time * 10, led_ch);
    }
    else if (led_setting_record[led_idx].type == LED_TYPE_BREATH)
    {
        led_demo_breath_init(led_idx);
        led_demo_breath_timer_control(led_idx, ENABLE);
    }
}

static void led_demo_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    T_LED_CHANNEL led_ch = (T_LED_CHANNEL)param;
    uint8_t led_idx = log2_u32(led_ch);
    uint8_t pin_num = led_demo_get_pin_num(led_ch);

    IO_PRINT_INFO3("led_demo_timeout_cb: led_ch 0x%x, led_idx %d, pin_num %d",
                   led_ch, led_idx, pin_num);

    if ((os_sys_time_get() - led_create_timer_point[led_idx]) >= led_create_timer_record[led_idx])
    {
        //restart repeat blink when blink_interval is not zero
        if (led_setting_record[led_idx].blink_count == 0)
        {
            led_demo_deinit(led_ch);
            led_setting_record[led_idx].blink_count = led_count_record[led_idx].blink_count;
            led_demo_cmd_handle(led_ch, ENABLE);
        }
        else if (led_setting_record[led_idx].blink_count != 1)
        {
            led_setting_record[led_idx].blink_count--;

            if (Pad_GetOutputCtrl(pin_num) == PAD_OUT_LOW)
            {
                Pad_OutputControlValue(pin_num, PAD_OUT_HIGH);
            }
            else
            {
                Pad_OutputControlValue(pin_num, PAD_OUT_LOW);
            }

            if (led_setting_record[led_idx].blink_count % 2 == 1) //start off timer when blink_cnt is odd
            {
                led_demo_create_timer(led_setting_record[led_idx].off_time * 10, led_ch);
            }
            else
            {
                led_demo_create_timer(led_setting_record[led_idx].on_time * 10, led_ch);
            }
        }
        else
        {
            if (led_setting_record[led_idx].blink_interval != 0) //blink_interval is not zero
            {
                led_demo_deinit(led_ch);
                led_setting_record[led_idx].blink_count = 0;
                led_demo_create_timer(led_setting_record[led_idx].blink_interval * 100, led_ch);
            }
            else
            {
                led_demo_deinit(led_ch);
                led_setting_record[led_idx].blink_count = led_count_record[led_idx].blink_count;
                led_demo_cmd_handle(led_ch, ENABLE);
            }
        }
    }
}

static void led_demo_clear_para(T_LED_CHANNEL led_ch)
{
    uint8_t led_idx = log2_u32(led_ch);

    memset(&led_setting_record[led_idx], 0, sizeof(T_LED_TABLE));
}

static void led_demo_set_driver_mode(void)
{
    led_driver_by_sw = true;
    app_timer_reg_cb(led_demo_timeout_cb, &led_module_timer_id);
}

/*
**
  * @brief  enable or disable led breath imer.
    * @param  TIM_TypeDef *index TIM_Type.
    * @param  FunctionalState NewState.
  * @return  void
  */
static void led_demo_breath_timer_control(uint8_t led_index, FunctionalState new_state)
{
    IO_PRINT_INFO2("led_demo_breath_timer_control: pwm_tim_handle[led_index] %d, new_state %d",
                   pwm_tim_handle[led_index], new_state);

    if (new_state)
    {
        pwm_start(pwm_tim_handle[led_index]);
    }
    else
    {
        pwm_stop(pwm_tim_handle[led_index]);
    }
}

static void led_demo_breath_timer_handler(void *handle)
{
    uint8_t i = 0;
    uint8_t led_num = 0;
    uint32_t high_count = 0;
    uint32_t low_count = 0;

    for (i = 0; i < LED_DEMO_NUM; i++)
    {
        if ((sw_breathe_mode[i].pwm_tim_id ==  hw_timer_get_id(handle)) &&
            (led_setting_record[i].type == LED_TYPE_BREATH) &&
            (led_setting_record[i].on_time != 0))
        {
            led_num = i + 1;
            IO_PRINT_INFO0("led_demo_breath_timer_handler: break");
            break;
        }
    }

    if (i == LED_DEMO_NUM)
    {
        IO_PRINT_ERROR1("led_demo_breath_timer_handler: index error, handle 0x%x", handle);
        return;
    }

    IO_PRINT_INFO2("led_demo_breath_timer_handler: i %d, handle 0x%x", i, handle);

    if (led_num != 0)
    {
        IO_PRINT_INFO1("led_demo_breath_timer_handler: led_breath_timer_count %d",
                       sw_breathe_mode[led_num - 1].led_breath_timer_count);

        sw_breathe_mode[i].led_breath_timer_count++;

        if (sw_breathe_mode[i].led_breath_timer_count <=
            2 * led_setting_record[led_num - 1].on_time) //set led breath duty during on time
        {
            high_count = PWM_OUT_COUNT * sw_breathe_mode[i].led_breath_timer_count /
                         led_setting_record[led_num - 1].on_time / 2;
            low_count = PWM_OUT_COUNT - PWM_OUT_COUNT * sw_breathe_mode[i].led_breath_timer_count /
                        led_setting_record[led_num - 1].on_time / 2;
        }
        else if ((sw_breathe_mode[i].led_breath_timer_count >
                  2 * led_setting_record[led_num - 1].on_time) && //set led breath duty during off time
                 (sw_breathe_mode[i].led_breath_timer_count <=
                  2 * (led_setting_record[led_num - 1].on_time + led_setting_record[led_num - 1].off_time)))
        {
            high_count = PWM_OUT_COUNT - PWM_OUT_COUNT * (sw_breathe_mode[i].led_breath_timer_count -
                                                          2 * led_setting_record[led_num - 1].on_time) / led_setting_record[led_num - 1].off_time / 2;
            low_count = PWM_OUT_COUNT * (sw_breathe_mode[i].led_breath_timer_count -
                                         2 * led_setting_record[led_num - 1].on_time) / led_setting_record[led_num - 1].off_time / 2;
        }
        else if (sw_breathe_mode[i].led_breath_timer_count >
                 2 * (led_setting_record[led_num - 1].on_time + led_setting_record[led_num - 1].off_time))
        {
            //restart blink when led_breath_blink_count is less than blink_count
            if (sw_breathe_mode[i].led_breath_blink_count < led_setting_record[led_num - 1].blink_count / 2)
            {
                sw_breathe_mode[i].led_breath_timer_count = 0;
            }
            else
            {
                if (sw_breathe_mode[i].led_breath_timer_count <= (2 * (led_setting_record[led_num - 1].on_time +
                                                                       led_setting_record[led_num - 1].off_time) +
                                                                  led_setting_record[led_num - 1].blink_interval *
                                                                  10)) // disable led breath when blink_interval is not zero
                {
                    high_count = 0;
                    low_count = PWM_OUT_COUNT;
                }
                else
                {
                    sw_breathe_mode[i].led_breath_timer_count = 0;
                    sw_breathe_mode[i].led_breath_blink_count = 0;
                }
            }
        }

        if (sw_breathe_mode[i].led_breath_timer_count == 2 * (led_setting_record[led_num - 1].on_time +
                                                              led_setting_record[led_num - 1].off_time))
        {
            sw_breathe_mode[i].led_breath_blink_count++;
        }

        /*up the change duty cnt */
        if (sw_breathe_mode[i].led_breath_timer_count != 0)
        {
            /*change duty */
            pwm_change_duty_and_frequency(pwm_tim_handle[i], high_count, low_count);
        }

        IO_PRINT_INFO4("led_demo_breath_timer_handler: pwm_tim_handle[%d] 0x%x, high_count %d, low_count %d",
                       i, pwm_tim_handle[i], high_count, low_count);
    }
}

static void led_demo_breath_init(uint8_t led_index)
{
    if (pwm_tim_handle[led_index] == NULL)
    {
        pwm_tim_handle[led_index] = pwm_create("ext_led", 0, PWM_OUT_COUNT, false);
        if (pwm_tim_handle[led_index] == NULL)
        {
            IO_PRINT_ERROR0("led_demo_breath_init: Could not create extend led pwm");
            return;
        }
        pwm_pin_config(pwm_tim_handle[led_index], LED_PIN_TABLE[led_index], PWM_FUNC);

        sw_breathe_mode[led_index].pwm_tim_id = hw_timer_get_id(pwm_tim_handle[led_index]);
        pwm_register_timeout_callback(pwm_tim_handle[led_index],
                                      (P_HW_TIMER_CBACK)led_demo_breath_timer_handler);
    }
    IO_PRINT_INFO3("led_demo_breath_init: led_index %d, sw_breathe_mode[led_index].pwm_tim_id 0x%x, pwm_tim_handle[led_index] 0x%x",
                   led_index, sw_breathe_mode[led_index].pwm_tim_id, pwm_tim_handle[led_index]);
}

static void led_demo_load_table(uint8_t i)
{
    memcpy((void *)&led_setting,
           (const void *)&LED_TABLE[i], LED_DEMO_NUM * sizeof(T_LED_TABLE));
}

/**
  * @brief  demo code of operation about LED breathe mode.
  * @param   No parameter.
  * @return  void
  */
static void led_demo_init(void)
{

    led_demo_set_driver_mode();
    led_demo_board_led_init();

    //index 0-->normal
    //index 1-->breath
    led_demo_load_table(1);
}

static void led_act_start(void)
{
    uint8_t i;

    for (i = 0; i < LED_DEMO_NUM; i++)
    {
        IO_PRINT_INFO2("led_act_start: i %d, (T_LED_CHANNEL)BIT(i) 0x%x", i, (T_LED_CHANNEL)BIT(i));

        led_demo_config((T_LED_CHANNEL)BIT(i), &led_setting[i]);

        led_demo_cmd_handle((T_LED_CHANNEL)BIT(i), true);

    }
}

void led_breath_demo(void)
{
    led_demo_init();
    led_act_start();
}
