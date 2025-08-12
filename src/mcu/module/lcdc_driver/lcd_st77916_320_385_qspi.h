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

#ifndef LCD_ST77916_320_385_QSPI_H
#define LCD_ST77916_320_385_QSPI_H

#include <stdint.h>
#include <stdbool.h>

void lcd_bl_pin_config(uint8_t lcd_bl);
void lcd_backlight_init(void);
void lcd_set_backlight(bool set);
void st77916_init(void);
void lcd_ST77916_qspi_454_set_window(uint16_t xStart, uint16_t yStart,
                                     uint16_t xEnd, uint16_t yEnd);

#endif // LCD_SH8601Z_454_QSPI_H
