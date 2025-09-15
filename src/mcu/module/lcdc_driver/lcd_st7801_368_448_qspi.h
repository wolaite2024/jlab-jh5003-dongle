#ifndef _LCD_ST7801_368_448_QSPI_H_
#define _LCD_ST7801_368_448_QSPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stdint.h"

void st7801_init(void);
void lcd_ST7801_qspi_368_448_set_window(uint16_t xStart, uint16_t yStart, uint16_t xEnd,
                                        uint16_t yEnd);
void lcd_vci_en_pin_config(uint8_t lcd_vci_en);
void lcd_vci_en_init(void);
void lcd_set_vci_en(bool set);

#ifdef __cplusplus
}
#endif

#endif /* _LCD_ST7801_368_448_QSPI_H_ */
