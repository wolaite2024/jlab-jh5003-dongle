/**
*****************************************************************************************
*     Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    lcd_sh8601z_454_qspi.c
* @brief   This file provides sh8601z LCD driver functions
* @author
* @date    2023-09-15
* @version v1.0
* *************************************************************************************
*/

#ifndef LCD_SH8601Z_454_QSPI_H
#define LCD_SH8601Z_454_QSPI_H

#include <stdint.h>

void sh8601a_init(void);
void SH8601A_qspi_power_off(void);
void SH8601A_qspi_power_on(void);
void lcd_SH8601A_qspi_454_set_window(uint16_t xStart, uint16_t yStart,
                                     uint16_t xEnd, uint16_t yEnd);
void lcd_avdd_en_pin_config(uint8_t lcd_avdd_en);
void lcd_avdd_en_init(void);
void lcd_set_avdd_en(bool set);

#endif // LCD_SH8601Z_454_QSPI_H
