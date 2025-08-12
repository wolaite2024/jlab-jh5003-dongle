/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __BG_CONN_H__
#define __BG_CONN_H__
#include "ual_adapter.h"
#include "gap_callback_le.h"
#include "adapter_int.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void ble_bg_handle_dev_conn_state(uint8_t state);
void ble_bg_update_conn_id(uint8_t conn_id);
void ble_bg_handle_wl_rsp(T_LE_MODIFY_WHITE_LIST_RSP *p_wl_rsp);

void ble_bg_handle_connect(uint8_t *bd_addr, uint8_t bd_type);

void ble_bg_handle_disconnected(uint8_t *bd_addr, uint8_t bd_type);

void ble_bg_set_mode(bool enable);

bool ble_bg_connect(uint8_t *bd_addr, uint8_t bd_type);

bool ble_bg_disconnect(uint8_t *bd_addr, uint8_t bd_type);
void ble_bg_conn_init(void);


//These api shall only be called by ble privacy
bool ble_bg_resume(void);
bool ble_bg_suspend(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
