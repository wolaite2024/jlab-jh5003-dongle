/*
 *  Copyright (C) 2020 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#ifndef __APP_SRC_ASP_H__
#define __APP_SRC_ASP_H__

#include "stdbool.h"
#include "stdint.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
void app_asp_received_proc(uint8_t *data, uint8_t length);

bool app_usb_hid_asp_host_exist(void);
void app_src_asp_init(void);
extern bool teams_connect_complete;
void app_usb_hid_handle_set_ASP_cmd(uint8_t *data, uint8_t length);

uint8_t app_usb_hid_handle_get_ASP_cmd(uint8_t *data, uint16_t *length);
#else
#define teams_connect_complete 1
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __APP_SRC_ASP_H__ */
