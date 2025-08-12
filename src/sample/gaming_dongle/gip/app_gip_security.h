/*
 * Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_GIP_SECURITY_H_
#define _APP_GIP_SECURITY_H_

#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif


void app_gip_pscc_handle_app_msg(T_IO_MSG *io_driver_msg_recv);

void app_gip_pscc_init(void);

void app_gip_pscc_handle_data_from_xhc(uint16_t len, uint8_t *data_from_xhc);

uint16_t app_gip_pscc_get_vendor_id(void);

uint16_t app_gip_pscc_get_product_id(void);

uint8_t *app_gip_pscc_get_puid(void);

#ifdef __cplusplus
}
#endif

#endif /* _APP_GIP_SECURITY_H_ */
