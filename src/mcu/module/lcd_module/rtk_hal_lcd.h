/**
*****************************************************************************************
*     Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    rtk_hal_lcd.h
* @brief   This file provides HAL LCD functions
* @author
* @date    2023-09-15
* @version v1.0
* *************************************************************************************
*/

#ifndef _RTK_HAL_LCD_H_
#define _RTK_HAL_LCD_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
#include "stdbool.h"

typedef enum
{
    LCDC_TE_TYPE_NO_TE = 0x00,
    LCDC_TE_TYPE_SW_TE = 0x01,
} T_LCDC_TE_TYPE;

/**
    * @brief  Set TE type.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  TE type, please refer to T_LCDC_TE_TYPE.
    * @return none.
    */
void rtk_lcd_hal_set_TE_type(T_LCDC_TE_TYPE state);

/**
    * @brief  Get TE type.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void.
    * @return The TE type.
    */
T_LCDC_TE_TYPE rtk_lcd_hal_get_TE_type(void);

/**
    * @brief  Set TE signal status.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  TE signal status.
    *     This parameter can be following values:
    *     @arg true: TE signal is triggered.
    *     @arg false: TE signal is not triggered.
    * @return none.
    */
void rtk_lcd_hal_set_te_trigger_state(bool state);

/**
    * @brief  Get TE signal status.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void.
    * @return TE signal status.
    *     @retval true: TE signal is triggered.
    *     @retval false: TE signal is not triggered.
    */
bool rtk_lcd_hal_get_te_trigger_state(void);

/**
    * @brief  Enable GPIO interrupt for TE pin.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void.
    * @return none.
    */
void rtk_lcd_hal_te_enable(void);

/**
    * @brief  Disable GPIO interrupt for TE pin.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void.
    * @return none.
    */
void rtk_lcd_hal_te_disable(void);

void rtk_lcd_hal_init(void);
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color);
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_transfer_done(void);
void rtk_lcd_hal_set_reset(bool reset);

uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);

void rtk_lcd_hal_start_transfer_test(uint8_t *buf, uint16_t xStart, uint16_t yStart, uint16_t w,
                                     uint16_t h);

#ifdef __cplusplus
}
#endif

#endif /* _RTK_HAL_LCD_H_ */
