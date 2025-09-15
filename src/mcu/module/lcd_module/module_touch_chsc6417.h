/**
*****************************************************************************************
*     Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    module_touch_chsc6417.h
* @brief   This file provides touch pad functions
* @author
* @date
* @version
* *************************************************************************************
*/

#ifndef MODULE_TOUCH_CHSC6417_H
#define MODULE_TOUCH_CHSC6417_H

#include "rtl876x_rcc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"

typedef struct
{
    int16_t x;
    int16_t y;
    uint16_t t;
    bool is_press;
} TOUCH_DATA;

void touch_pin_config(uint8_t touch_i2c_scl, uint8_t touch_i2c_sda, uint8_t touch_int,
                      uint8_t touch_rst);
bool touch_driver_init(void);
bool touch_read_key_value(TOUCH_DATA *p_touch_data);
void touch_gesture_process_timeout(void);
void touch_int_config(bool is_enable);
void touch_register_irq_callback(void (*indicate)(void *), void *context);

#endif // MODULE_TOUCH_CHSC6417_H
