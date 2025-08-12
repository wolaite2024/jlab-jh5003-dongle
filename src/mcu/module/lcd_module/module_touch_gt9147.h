#ifndef _RTL876X_MODULE_TOUCH_GESTURE_BUTTON__
#define _RTL876X_MODULE_TOUCH_GESTURE_BUTTON__
#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "rtl876x_tim.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x.h"
#include "trace.h"

//I2C Read/Write cmd
#define GT_CMD_WR       0X28
#define GT_CMD_RD       0X29

//GT9147 Register
#define GT_CTRL_REG     0X8040
#define GT_CFGS_REG     0X8047
#define GT_CHECK_REG    0X80FF
#define GT_PID_REG      0X8140

#define GT_GSTID_REG    0X814E
#define GT_TP1_REG      0X8150
#define GT_TP2_REG      0X8158
#define GT_TP3_REG      0X8160
#define GT_TP4_REG      0X8168
#define GT_TP5_REG      0X8170

typedef struct
{
    int16_t x;
    int16_t y;
    uint16_t t;
    bool  is_press;
    uint8_t get_point;
} TOUCH_DATA;

void touch_pin_config(uint8_t touch_i2c_scl, uint8_t touch_i2c_sda, uint8_t touch_int,
                      uint8_t touch_rst);
bool touch_write(uint16_t reg, uint8_t data);
bool touch_read_key_value(TOUCH_DATA *p_touch_data);
void touch_gesture_process_timeout(void);
bool touch_driver_init(void);
void enable_touch_sensor(void);
void disable_touch_sensor(void);
bool touch_get_chip_id(uint8_t *p_chip_id);
void touch_resume(void);
void touch_suspend(void);
void touch_register_irq_callback(void (*indicate)(void *), void *context);

#endif //_RTL876X_MODULE_TOUCH_GESTURE_BUTTON__




