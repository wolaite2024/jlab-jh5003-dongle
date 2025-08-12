/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     module_touch_gt9147.c
* @brief
* @details
* @author
* @date
* @version  v0.1
*********************************************************************************************************
*/
#include "app_gui.h"
#if (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_GT9147)
#include "string.h"
#include "board.h"
#include "os_timer.h"
#include "rtl876x_pinmux.h"
#include "module_touch_gt9147.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"
#include "platform_utils.h"
#include "vector_table.h"
#include "rtl876x_i2c.h"

#define TOUCH_I2C_BUS                             I2C1
#define TOUCH_I2C_FUNC_SCL                        I2C1_CLK
#define TOUCH_I2C_FUNC_SDA                        I2C1_DAT
#define TOUCH_I2C_APBPeriph                       APBPeriph_I2C1
#define TOUCH_I2C_APBClock                        APBPeriph_I2C1_CLOCK
#define TOUCH_SLAVE_ADDRESS                       (0x14)

typedef struct _t_touch_pin
{
    uint8_t touch_i2c_scl;
    uint8_t touch_i2c_sda;
    uint8_t touch_int;
    uint8_t touch_rst;
} T_TOUCH_PIN;

static void         *touch_gesture_release_timer = NULL;
static bool         touch_initialized = false;
static bool         touch_enable_flag = false;
static uint32_t     chip_id;
static TOUCH_DATA   cur_point;
const  uint16_t     GT9147_TPX_TBL[5] = {GT_TP1_REG, GT_TP2_REG, GT_TP3_REG, GT_TP4_REG, GT_TP5_REG};

static void touch_int_handler(uint32_t context);
static void (*touch_indicate)(void *);
static void *touch_context;
static T_TOUCH_PIN touch_pin;
static void touch_gesture_release_timeout(void *pxTimer);

void touch_pin_config(uint8_t touch_i2c_scl, uint8_t touch_i2c_sda, uint8_t touch_int,
                      uint8_t touch_rst)
{
    touch_pin.touch_i2c_scl = touch_i2c_scl;
    touch_pin.touch_i2c_sda = touch_i2c_sda;
    touch_pin.touch_int     = touch_int;
    touch_pin.touch_rst     = touch_rst;
}

static void touch_i2c_init(void)
{
    Pinmux_Config(touch_pin.touch_i2c_scl, TOUCH_I2C_FUNC_SCL);
    Pinmux_Config(touch_pin.touch_i2c_sda, TOUCH_I2C_FUNC_SDA);

    Pad_Config(touch_pin.touch_i2c_scl, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(touch_pin.touch_i2c_sda, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);

    RCC_PeriphClockCmd(TOUCH_I2C_APBPeriph, TOUCH_I2C_APBClock, DISABLE);
    RCC_PeriphClockCmd(TOUCH_I2C_APBPeriph, TOUCH_I2C_APBClock, ENABLE);
    I2C_InitTypeDef  I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_Clock = 40000000;
    I2C_InitStructure.I2C_ClockSpeed   = 400000;
    I2C_InitStructure.I2C_DeviveMode   = I2C_DeviveMode_Master;
    I2C_InitStructure.I2C_AddressMode  = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_Init(TOUCH_I2C_BUS, &I2C_InitStructure);
    I2C_Cmd(TOUCH_I2C_BUS, ENABLE);
}

static void touch_reset_init(void)
{
    hal_gpio_init();
    hal_gpio_init_pin(touch_pin.touch_rst, GPIO_TYPE_AON, GPIO_DIR_OUTPUT, GPIO_PULL_UP);
    hal_gpio_set_level(touch_pin.touch_rst, GPIO_LEVEL_HIGH);
}

static void touch_interrupt_init(void)
{
    hal_gpio_int_init();
    hal_gpio_init_pin(touch_pin.touch_int, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_DOWN);
    hal_gpio_set_up_irq(touch_pin.touch_int, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_HIGH, false);
    hal_gpio_register_isr_callback(touch_pin.touch_int, touch_int_handler, 0);

    hal_gpio_irq_disable(touch_pin.touch_int);
}

bool touch_driver_init(void)
{
    touch_reset_init();
    touch_i2c_init();

    hal_gpio_set_level(touch_pin.touch_rst, GPIO_LEVEL_LOW);
    Pad_Config(touch_pin.touch_int, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_LOW);
    platform_delay_ms(1);
    Pad_Config(touch_pin.touch_int, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    platform_delay_ms(1);
    hal_gpio_set_level(touch_pin.touch_rst, GPIO_LEVEL_HIGH);
    platform_delay_ms(6);
    Pad_Config(touch_pin.touch_int, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_LOW);
    platform_delay_ms(60);

    if (touch_get_chip_id((uint8_t *)&chip_id) == false)
    {
        APP_PRINT_ERROR0("touch_driver_init: touch initialization failed!");
        return false;
    }

    touch_interrupt_init();
    if (touch_gesture_release_timer == NULL)
    {
        os_timer_create(&touch_gesture_release_timer, "touch gesture release timer", 1, 20, false,
                        touch_gesture_release_timeout);
    }

    hal_gpio_irq_enable(touch_pin.touch_int);

    touch_enable_flag = true;
    touch_initialized = true;
    return true;
}

void enable_touch_sensor(void)
{
    if (touch_enable_flag == false)
    {
        touch_enable_flag = true;
        if (touch_initialized)
        {
            touch_resume();
        }
        hal_gpio_irq_enable(touch_pin.touch_int);
    }
}

void disable_touch_sensor(void)
{
    touch_enable_flag = false;
    hal_gpio_irq_disable(touch_pin.touch_int);
    if (touch_initialized)
    {
        touch_suspend();
    }
}

bool touch_write(uint16_t reg, uint8_t data)
{
    uint8_t I2C_WriteBuf[3] = {0x0, 0x0};
    I2C_WriteBuf[0] = reg >> 8;
    I2C_WriteBuf[1] = reg & 0xFF;
    I2C_WriteBuf[2] = data;
    I2C_SetSlaveAddress(TOUCH_I2C_BUS, TOUCH_SLAVE_ADDRESS);
    I2C_Status res = I2C_MasterWrite(TOUCH_I2C_BUS, I2C_WriteBuf, 3);
    if (res != I2C_Success)
    {
        APP_PRINT_ERROR1("touch_write: master write fail error %d", res);
        return false;
    }
    return true;
}

bool touch_read(uint16_t reg, uint8_t *p_data, uint8_t len)
{
    uint8_t buf[2];
    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;
    I2C_SetSlaveAddress(TOUCH_I2C_BUS, TOUCH_SLAVE_ADDRESS);
    I2C_Status res = I2C_RepeatRead(TOUCH_I2C_BUS, buf, 2, p_data, len);
    if (res != I2C_Success)
    {
        APP_PRINT_ERROR1("touch_read: master write fail error %d", res);
        return false;
    }
    return true;
}

bool touch_read_key_value(TOUCH_DATA *p_touch_data)
{
    uint8_t mode;
    uint8_t point_num = 0;
    touch_read(GT_GSTID_REG, &mode, 1);

    if (mode & 0x80)
    {
        touch_write(GT_GSTID_REG, 0);// clear flags
    }

    point_num = mode & 0x0F;

    if (cur_point.get_point == 0)
    {
        cur_point.get_point = point_num;
    }

    if (point_num == 0)
    {
        return false;
    }

    uint8_t buf[4];
    for (uint8_t i = 0; i < point_num; i++)
    {
        touch_read(GT9147_TPX_TBL[i], buf, 4);
        cur_point.is_press = true;
        cur_point.x = buf[0] | (buf[1] << 8);
        cur_point.x = 480 - cur_point.x;
        cur_point.y = buf[2] | (buf[3] << 8);
        cur_point.y = 800 - cur_point.y;
        APP_PRINT_INFO2("touch_read_key_value: x = %d, y = %d", cur_point.x, cur_point.y);
    }
    touch_write(GT_GSTID_REG, 0);// clear flags

    if (cur_point.x > LCD_WIDTH || cur_point.y > LCD_HIGHT)
    {
        return false;
    }
    p_touch_data->x =  cur_point.x;
    p_touch_data->y =  cur_point.y;
    p_touch_data->t = cur_point.t;
    p_touch_data->is_press = cur_point.is_press;

    return true;
}

bool touch_get_chip_id(uint8_t *p_chip_id)
{
    if (touch_read(GT_PID_REG, p_chip_id, 4) == true)
    {
        APP_PRINT_INFO1("touch_get_chip_id: %b", TRACE_BINARY(4, p_chip_id));
        return true;
    }
    return false;
}

void touch_resume(void)
{
#if 0
    hal_gpio_set_level(touch_pin.touch_rst, GPIO_LEVEL_LOW);
    os_delay(1);
    hal_gpio_set_level(touch_pin.touch_rst, GPIO_LEVEL_HIGH);

    os_delay(30);
    APP_PRINT_INFO0("touch_resume");
#endif
}

void touch_suspend(void)
{
#if 0
    uint8_t sleep_cmd = CHIP_59_ENTER_SLEEP;
    if (chip_id != 0x59)
    {
        sleep_cmd = CHIP_OTHER_ENTER_SLEEP;
    }

    cst0x6_write(FTS_REG_LOW_POWER, sleep_cmd);
    APP_PRINT_INFO0("touch_suspend");
#endif
}

static void touch_int_handler(uint32_t context)
{
    cur_point.t++;
    os_timer_restart(&touch_gesture_release_timer, 20);
}

void touch_int_config(bool is_enable)
{
    if (is_enable == true)
    {
        hal_gpio_irq_enable(touch_pin.touch_int);
    }
    else
    {
        hal_gpio_irq_disable(touch_pin.touch_int);
    }
}

void touch_gesture_process_timeout(void)
{
//    APP_PRINT_INFO1("touch_gesture_process_timeout: cur_point.t %d", cur_point.t);
    cur_point.is_press = false;

    cur_point.t = 0;
    cur_point.x = 0;
    cur_point.y = 0;
    cur_point.get_point = 0;
}

static void touch_gesture_release_timeout(void *pxTimer)
{
    touch_gesture_process_timeout();
    if (touch_indicate)
    {
        touch_indicate(touch_context);
    }
}

void touch_register_irq_callback(void (*indicate)(void *), void *context)
{
    touch_indicate = indicate;
    touch_context = context;
}
#endif
