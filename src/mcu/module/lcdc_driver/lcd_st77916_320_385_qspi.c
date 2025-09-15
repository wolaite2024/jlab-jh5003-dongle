/**
*****************************************************************************************
*     Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    lcd_st77916_320_385_qspi.c
* @brief   This file provides ST77916 LCD driver functions
* @author
* @date
* @version v1.0
* *************************************************************************************
*/
#include "app_gui.h"
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
#include "lcd_st77916_320_385_qspi.h"
#include "board.h"
#include "fmc_api.h"
#include "fmc_api_ext.h"
#include "rtl876x_pinmux.h"
#include "trace.h"
#include "hal_gpio.h"
#include "platform_utils.h"
#include "os_mem.h"

#define OUTPUT_PIXEL_BYTES                  2
#define ST77916_MAX_PARA_COUNT              20
#define SPIC_ID                             FMC_SPIC_ID_2

typedef struct _ST77916_CMD_DESC
{
    uint8_t instruction;
    uint8_t index;
    uint16_t delay;
    uint16_t wordcount;
    uint8_t  payload[ST77916_MAX_PARA_COUNT];
} ST77916_CMD_DESC;

static uint8_t lcd_bl_pin;

void lcd_bl_pin_config(uint8_t lcd_bl)
{
    lcd_bl_pin = lcd_bl;
}

void lcd_backlight_init(void)
{
    hal_gpio_init();
    hal_gpio_init_pin(lcd_bl_pin, GPIO_TYPE_AUTO, GPIO_DIR_OUTPUT, GPIO_PULL_NONE);
    hal_gpio_set_level(lcd_bl_pin, GPIO_LEVEL_LOW);
}

void lcd_set_backlight(bool set)
{
    if (set)
    {
        hal_gpio_set_level(lcd_bl_pin, GPIO_LEVEL_HIGH);
    }
    else
    {
        hal_gpio_set_level(lcd_bl_pin, GPIO_LEVEL_LOW);
    }
}

static void spic2_spi_write(uint8_t *buf, uint32_t len)
{
    fmc_spic_enable(SPIC_ID, false);
    fmc_spic_set_user_mode(SPIC_ID);
    fmc_spic_set_multi_ch(SPIC_ID, FMC_SPIC_SING_CH, FMC_SPIC_SING_CH, FMC_SPIC_SING_CH);
    fmc_spic_set_tx_mode(SPIC_ID);

    fmc_spic_set_cmd_len(SPIC_ID, 1);
    fmc_spic_set_user_addr_len(SPIC_ID, 3);

    if (len > 4)
    {
        fmc_spic_set_txndf(SPIC_ID, len - 4);
    }
    else
    {
        fmc_spic_set_txndf(SPIC_ID, 0);
    }

    for (uint32_t i = 0; i < len; i++)
    {
        fmc_spic_set_dr(SPIC_ID, buf[i], FMC_SPIC_DATA_BYTE);
    }

    fmc_spic_enable(SPIC_ID, true);
    while (fmc_spic_in_busy(SPIC_ID));
    fmc_spic_enable(SPIC_ID, false);
}

/* ST77916 QSPI Instruction Code */
#define ST77916_QSPI_INST_CMD_WRITE                     (0x02)
#define ST77916_QSPI_SEQ_FINISH_CODE                    (0x00)

static const ST77916_CMD_DESC ST77916_POST_OTP_POWERON_SEQ_CMD[] =
{
    {ST77916_QSPI_INST_CMD_WRITE,  0xF0, 0,  1,  {0x28}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF2, 0,  1,  {0x28}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x7C, 0,  1,  {0xD1}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x80, 0,  1,  {0x10}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x83, 0,  1,  {0xE0}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x84, 0,  1,  {0x61}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF2, 0,  1,  {0x82}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF0, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF0, 0,  1,  {0x01}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF1, 0,  1,  {0x01}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB0, 0,  1,  {0x52}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB1, 0,  1,  {0x49}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB2, 0,  1,  {0x24}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB4, 0,  1,  {0x69}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB5, 0,  1,  {0x45}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB6, 0,  1,  {0x8B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB7, 0,  1,  {0x41}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB8, 0,  1,  {0x05}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBA, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBB, 0,  1,  {0x08}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBC, 0,  1,  {0x08}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBD, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC0, 0,  1,  {0x80}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC1, 0,  1,  {0x08}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC2, 0,  1,  {0x35}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC3, 0,  1,  {0x80}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC4, 0,  1,  {0x08}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC5, 0,  1,  {0x35}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC6, 0,  1,  {0xA9}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC7, 0,  1,  {0x41}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC8, 0,  1,  {0x01}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC9, 0,  1,  {0xA9}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xCA, 0,  1,  {0x41}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xCB, 0,  1,  {0x01}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD0, 0,  1,  {0xD1}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD1, 0,  1,  {0x40}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD2, 0,  1,  {0x81}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF5, 0,  2,  {0x00, 0xA5}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xDD, 0,  1,  {0x50}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xDE, 0,  1,  {0x50}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF1, 0,  1,  {0x10}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF0, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF0, 0,  1,  {0x02}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE0, 0,  14, {0xF0, 0x0F, 0x10, 0x0A, 0x0A, 0x06, 0x38, 0x33, 0x4D, 0x06, 0x12, 0x11, 0x2F, 0x34}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE1, 0,  14, {0xF0, 0x10, 0x15, 0x0B, 0x0C, 0x07, 0x3D, 0x44, 0x53, 0x0C, 0x18, 0x18, 0x30, 0x34}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF0, 0,  1,  {0x10}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF3, 0,  1,  {0x10}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE0, 0,  1,  {0x08}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE1, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE2, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE3, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE4, 0,  1,  {0xE0}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE5, 0,  1,  {0x06}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE6, 0,  1,  {0x21}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE7, 0,  1,  {0x10}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE8, 0,  1,  {0x8A}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xE9, 0,  1,  {0x82}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xEA, 0,  1,  {0xE4}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xEB, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xEC, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xED, 0,  1,  {0x14}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xEE, 0,  1,  {0xFF}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xEF, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF8, 0,  1,  {0xFF}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF9, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xFA, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xFB, 0,  1,  {0x30}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xFC, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xFD, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xFE, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xFF, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x60, 0,  1,  {0x50}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x61, 0,  1,  {0x02}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x62, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x63, 0,  1,  {0x50}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x64, 0,  1,  {0x04}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x65, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x66, 0,  1,  {0x53}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x67, 0,  1,  {0x08}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x68, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x69, 0,  1,  {0x53}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x6A, 0,  1,  {0x0A}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x6B, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x70, 0,  1,  {0x50}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x71, 0,  1,  {0x01}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x72, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x73, 0,  1,  {0x50}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x74, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x75, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x76, 0,  1,  {0x53}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x77, 0,  1,  {0x07}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x78, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x79, 0,  1,  {0x53}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x7A, 0,  1,  {0x09}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x7B, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x80, 0,  1,  {0x58}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x81, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x82, 0,  1,  {0x04}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x83, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x84, 0,  1,  {0x0C}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x85, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x86, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x87, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x88, 0,  1,  {0x58}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x89, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x8A, 0,  1,  {0x06}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x8B, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x8C, 0,  1,  {0x0E}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x8D, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x8E, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x8F, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x90, 0,  1,  {0x58}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x91, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x92, 0,  1,  {0x08}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x93, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x94, 0,  1,  {0x10}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x95, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x96, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x97, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x98, 0,  1,  {0x58}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x99, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x9A, 0,  1,  {0x0A}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x9B, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x9C, 0,  1,  {0x12}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x9D, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x9E, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x9F, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA0, 0,  1,  {0x58}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA1, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA2, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA3, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA4, 0,  1,  {0x0B}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA5, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA6, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA7, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA8, 0,  1,  {0x58}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xA9, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xAA, 0,  1,  {0x05}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xAB, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xAC, 0,  1,  {0x0D}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xAD, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xAE, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xAF, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB0, 0,  1,  {0x58}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB1, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB2, 0,  1,  {0x07}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB3, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB4, 0,  1,  {0x0F}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB5, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB6, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB7, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB8, 0,  1,  {0x58}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xB9, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBA, 0,  1,  {0x09}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBB, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBC, 0,  1,  {0x11}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBD, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBE, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xBF, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC0, 0,  1,  {0x77}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC1, 0,  1,  {0x46}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC2, 0,  1,  {0x55}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC3, 0,  1,  {0x64}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC4, 0,  1,  {0xBB}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC5, 0,  1,  {0x21}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC6, 0,  1,  {0x30}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC7, 0,  1,  {0xAA}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC8, 0,  1,  {0x12}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xC9, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD0, 0,  1,  {0x77}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD1, 0,  1,  {0x46}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD2, 0,  1,  {0x55}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD3, 0,  1,  {0x64}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD4, 0,  1,  {0xBB}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD5, 0,  1,  {0x21}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD6, 0,  1,  {0x30}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD7, 0,  1,  {0xAA}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD8, 0,  1,  {0x12}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xD9, 0,  1,  {0x03}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF3, 0,  1,  {0x01}},
    {ST77916_QSPI_INST_CMD_WRITE,  0xF0, 0,  1,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x21, 0,  0,  {0x00}},
#if (ENABLE_TE_FOR_LCD == 1)
    {ST77916_QSPI_INST_CMD_WRITE,  0x44, 0,  2,  {0x00, 0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x35, 0,  1,  {0x00}},
#else
    {ST77916_QSPI_INST_CMD_WRITE,  0x34, 0,  0,  {0x00}},
#endif
    {ST77916_QSPI_INST_CMD_WRITE,  0x3A, 0,  1,  {0x55}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x11, 120, 0,  {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE,  0x29, 20, 0,  {0x00}},
#if FB_DIRECTION_ROTATE
    {ST77916_QSPI_INST_CMD_WRITE,  0x36, 0,  1,  {0xA0}},
#endif
    {ST77916_QSPI_SEQ_FINISH_CODE, 0,    0,  0,  {0}},
};

static void ST77916_Reg_Write(const ST77916_CMD_DESC *cmd)
{
    uint16_t idx = 0;

    uint8_t *sdat = os_mem_alloc(RAM_TYPE_DATA_ON, 50);
    while (cmd[idx].instruction != ST77916_QSPI_SEQ_FINISH_CODE)
    {
        sdat[0] = cmd[idx].instruction;

        sdat[1] = 0;
        sdat[2] = cmd[idx].index; // Set in the middle 8 bits ADDR[15:8] of the 24 bits ADDR[23:0]
        sdat[3] = 0;

        //APP_PRINT_INFO1("cmd[idx].index: 0x%x", cmd[idx].index);

        for (uint16_t i = 0; i < cmd[idx].wordcount; i++)
        {
            sdat[i + 4] = cmd[idx].payload[i];
        }

        spic2_spi_write(sdat, cmd[idx].wordcount + 4);
        if (cmd[idx].delay != 0)
        {
            platform_delay_ms(cmd[idx].delay);
        }

        idx++;
    }
    os_mem_free(sdat);
}

void ST77916_Init_Post_OTP(void)
{
    ST77916_Reg_Write(ST77916_POST_OTP_POWERON_SEQ_CMD);
}

static void rtl_ST77916_qspi_cmd_param4(uint8_t cmd, uint8_t *data)
{
    uint8_t sdat[] = {ST77916_QSPI_INST_CMD_WRITE, 0x00, cmd, 0x00, data[0], data[1], data[2], data[3]};
    spic2_spi_write(sdat, sizeof(sdat));
}

static void rtl_ST77916_qspi_enter_data_output_mode(uint32_t len_byte)
{
    fmc_spic_enable(SPIC_ID, false);
    fmc_spic_set_user_mode(SPIC_ID);

    fmc_spic_set_multi_ch(SPIC_ID, FMC_SPIC_SING_CH, FMC_SPIC_SING_CH, FMC_SPIC_QUAD_CH);
    fmc_spic_set_tx_mode(SPIC_ID);
    fmc_spic_set_seq_trans(SPIC_ID, true);

    fmc_spic_set_cmd_len(SPIC_ID, 1);
    fmc_spic_set_user_addr_len(SPIC_ID, 3);
    fmc_spic_set_txndf(SPIC_ID, len_byte);

    uint32_t first_word = 0x32 | (0x002c00 << 8);
    fmc_spic_set_dr(SPIC_ID, first_word, FMC_SPIC_DATA_WORD);

    fmc_spic_set_dmac_tx_en(SPIC_ID, true);
    fmc_spic_set_dmac_tx_data_lv(SPIC_ID, 4);

    //allow spic tx ndf setting and data latency
    SPIC2->CTRLR0 |= BIT11;

    fmc_spic_enable(SPIC_ID, true);
}

void lcd_ST77916_qspi_454_set_window(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    uint8_t data[4];
    data[0] = xStart >> 8;
    data[1] = xStart & 0xff;
    data[2] = xEnd >> 8;
    data[3] = xEnd & 0xff;
    rtl_ST77916_qspi_cmd_param4(0x2A, data);

    data[0] = yStart >> 8;
    data[1] = yStart & 0xff;
    data[2] = yEnd >> 8;
    data[3] = yEnd & 0xff;
    rtl_ST77916_qspi_cmd_param4(0x2B, data);

    uint32_t len_byte = (xEnd - xStart + 1) * (yEnd - yStart + 1) * OUTPUT_PIXEL_BYTES;
    //APP_PRINT_INFO5("xStartEnd: %d, %d; yStartEnd: %d, %d; len_byte: %u", xStart, xEnd, yStart, yEnd,
    //                len_byte);
    rtl_ST77916_qspi_enter_data_output_mode(len_byte);
}

void st77916_init(void)
{
    ST77916_Init_Post_OTP();
}
#endif
