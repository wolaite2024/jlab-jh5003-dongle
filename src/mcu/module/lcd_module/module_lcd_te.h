#ifndef _RTL876X_MODULE_LCD_TE_H_
#define _RTL876X_MODULE_LCD_TE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"

/**
    * @brief  Configure TE pin index.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  The TE pin index, please refer to "pin_def.h".
    * @return none.
    */
void lcd_te_pin_config(uint8_t lcd_te);

/**
    * @brief  Initialize TE pin to gpio mode.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void.
    * @return none.
    */
void lcd_te_device_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_MODULE_LCD_TE_H_ */
