/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __CONNECTION_MAR_H__
#define __CONNECTION_MAR_H__
#include "ual_adapter.h"
#include "gap_callback_le.h"
#include "adapter_int.h"
#include "ual_dev_mgr.h"
#include "gap_msg.h"
#include "gap.h"
#include "bt_gatt_client.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct ble_conn_param
{
    uint16_t ci_min;
    uint16_t ci_max;
    uint16_t latency;
    uint16_t timeout;
    uint16_t ce_min;
    uint16_t ce_max;
    uint16_t ci_min_pend;
    uint16_t ci_max_pend;
    uint16_t latency_pend;
    uint16_t timeout_pend;
    uint16_t ce_min_pend;
    uint16_t ce_max_pend;
    bool     ce_2slot_used;
    bool     param_update_pend;
    uint8_t conn_mask;
} T_BLE_CONN_PARAM;

typedef bool (*T_UAL_CONN_MGR_APP_CBACK)(uint8_t event);
void ble_conn_mgr_handle_le_conn_state(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                       uint16_t disc_cause);
void ble_conn_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size);
bool ble_conn_mgr_connect(uint8_t *bd_addr, uint8_t bd_type, bool bg_conn);
bool ble_conn_mgr_disconnect(uint8_t *bd_addr, uint8_t bd_type);
void ble_handle_conn_update_event(T_GAP_CONN_PARAM_UPDATE update_info);
T_APP_RESULT ble_handle_conn_update_ind(T_LE_CONN_UPDATE_IND *p_conn_ind);
T_APP_RESULT ble_gatt_client_discover_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                                         void *p_data);
bool ble_register_conn_callback_by_id(T_APP_ID app_id, T_LE_CONN_CBACK callback);
void ble_unregister_conn_callback_by_id(T_APP_ID app_id);
void ble_conn_mgr_register_app_cb(T_UAL_CONN_MGR_APP_CBACK cback);
void ble_conn_update_by_app(bool enable);
void ble_conn_mgr_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
