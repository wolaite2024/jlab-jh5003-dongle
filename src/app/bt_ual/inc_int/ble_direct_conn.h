/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __BLE_DIRECT_CONN_H__
#define __BLE_DIRECT_CONN_H__
#include "ual_adapter.h"
#include "gap_callback_le.h"
#include "adapter_int.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void ble_direct_set_conn_interval(uint16_t interval);

bool ble_direct_connect(uint8_t *bd_addr, uint8_t bd_type);

bool ble_direct_disconnect(uint8_t *bd_addr, uint8_t bd_type);

void ble_direct_handle_disconnected(uint8_t *bd_addr, uint8_t bd_type);

void ble_direct_handle_connected(uint8_t *bd_addr, uint8_t bd_type);

void ble_direct_set_mode(bool enable);

void ble_direct_conn_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
