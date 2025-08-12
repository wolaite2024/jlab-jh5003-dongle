/**
*****************************************************************************************
*     Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    module_psram.h
* @brief   This file provides psram functions
* @author
* @date    2023-09-15
* @version v1.0
* *************************************************************************************
*/

#ifndef _RTL876X_MODULE_PSRAM_H_
#define _RTL876X_MODULE_PSRAM_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"

void app_apm_qspi_psram_init(void);
void app_apm_opi_psram_init(void);
void app_apm_psram_read_write_test(void);
void app_wb_opi_psram_init(void);
void app_wb_opi_psram_read_write_test(void);
void app_psram_enter_dlps(void);
void app_psram_exit_dlps(void);
void app_psram_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_MODULE_LCD_TE_H_ */
