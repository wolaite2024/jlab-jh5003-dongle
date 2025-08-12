/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __BLE_PRIVACY_H__
#define __BLE_PRIVACY_H__
#include "ual_adapter.h"
#include "gap_callback_le.h"
#include "adapter_int.h"
#include "dev_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if BLE_PRIVACY_SPT

/* resolving list using state as a bit mask */
#define BLE_RL_ACT_IDLE           0x00
#define BLE_RL_ACT_SUSPEND        0x01
#define BLE_RL_ACT_MODIFY_RL      0x02
#define BLE_RL_ACT_SET_LOCAL_RPA  0x04
typedef uint8_t T_BLE_RL_ACT_STATE;


/* resolving list using state as a bit mask */
#define BLE_RL_IDLE     0x00
#define BLE_RL_INIT     0x01
#define BLE_RL_SCAN     0x02
#define BLE_RL_ADV      0x04
#define BLE_RL_PA_SYNC  0x10

typedef uint8_t T_BLE_RL_SUSPEND_STATE;

#define BLE_RL_DISABLE      0x00
#define BLE_RL_ENABLE       0x01
#define BLE_RL_DISABLING    0x02
#define BLE_RL_ENABLING     0x04

typedef uint8_t T_BLE_RL_STATE;


void handle_scan_stop_in_suspend(void);
void ble_handle_rpa_set_rsp(T_LE_SET_RAND_ADDR_RSP *p_rsp);
void handle_bgconn_stop_in_suspend(void);
void handle_adv_stop_in_suspend(void);
void handle_pa_sync_stop_in_suspend(void);

bool ble_resolving_list_load_dev(T_BT_DEVICE *p_dev_rec);
bool ble_resolving_list_remove_dev(T_BT_DEVICE *p_dev_rec);
bool ble_resolving_list_is_enable(void);
void ble_change_local_rpa(uint8_t *rpa);
void ble_privacy_init(void);

#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
