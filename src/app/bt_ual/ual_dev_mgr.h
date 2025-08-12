/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __UAL_DEV_MGR_H__
#define __UAL_DEV_MGR_H__

#include <stdint.h>
#include "ual_types.h"
#include "gap_bond_le.h"
#include "gap_conn_le.h"
#include "ual_bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BLE_BD_TYPE_ID_BIT 0x02

#define BLE_ADDR_PUBLIC 0x00
#define BLE_ADDR_RANDOM 0x01
#define BLE_ADDR_PUBLIC_ID 0x02
#define BLE_ADDR_RANDOM_ID 0x03
#define REMOTE_ADDR_CLASSIC 0x10
#define BLE_ADDR_ANONYMOUS 0xFF
typedef uint8_t T_BLE_BD_TYPE;

#define F_CONNECT_BY_WHITELIST      0


//reserve 0 ~ 5 for ual lib interval ID
#define DEV_MGR_APP_ID          0
//reserve 6 ~ 10 for ohter lib interval ID

#define APP_CONN_ID_MAX         31
typedef uint8_t T_APP_ID;

typedef enum
{
    CI_EVENT_GATT_DISCOVERY,
    CI_EVENT_GATT_DONE,
    CI_EVENT_DISCV_ALL_DONE,
    CI_EVENT_CIS_ESTABLISH,
    CI_EVENT_CIS_DISCONNECT,
} T_UPDATE_CI_EVENT;

typedef void (*T_LE_CONN_CBACK)(uint8_t conn_id,  uint8_t *bd_addr, uint8_t state);
void bt_le_passkey_display_confirm(bool accept, uint8_t *bd_addr);
void bt_le_num_comp_confirm(bool accept, uint8_t *bd_addr);
void bt_le_passkey_input_confirm(bool accept, uint8_t *bd_addr, uint32_t passky);
int bt_dev_bredr_connect(uint8_t *bd_addr);
int bt_dev_bredr_disconnct(uint8_t *bd_addr);
int bt_dev_le_gatt_connect(uint8_t *bd_addr, T_BLE_BD_TYPE bd_type, T_APP_ID app_id);
int bt_dev_le_gatt_disconnect(uint8_t *bd_addr, T_APP_ID app_id);
int bt_dev_clear_bond_info(uint8_t *bd_addr);
int bt_dev_create_bond(uint8_t *bd_addr, T_BLE_BD_TYPE bd_type);
int bt_dev_remove_bond(uint8_t *bd_addr);
int bt_dev_cancel_bonding(uint8_t *bd_addr);
bool bt_dev_enable_conn_update(uint8_t *addr);
bool bt_dev_disable_conn_update(uint8_t *addr);
bool bt_dev_update_conn_params(uint8_t *bd_addr, uint16_t ci_min,
                               uint16_t ci_max, uint16_t latency, uint16_t timeout,
                               uint16_t ce_min, uint16_t ce_max);
bool bt_dev_register_le_conn_callback(T_APP_ID app_id, T_LE_CONN_CBACK callback);
void bt_dev_unregister_le_conn_callback(T_APP_ID app_id);
bool bt_dev_handle_conn_update_event(uint8_t *bd_addr, T_UPDATE_CI_EVENT event);
/**
 * @brief               This function of setting ble mode
 *
 * @param gaming        the ble mode which want to set
 */
void ble_set_gaming_mode(bool gaming);

/**
 * @brief               This function set ble mode
 *
 * @return true         in gaming mode now
 * @return false        not in gaming mode
 */
bool ble_is_gaming_mode(void);
void bt_dev_set_support_dongle_dual_mode(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UAL_DEV_MGR_H__ */
