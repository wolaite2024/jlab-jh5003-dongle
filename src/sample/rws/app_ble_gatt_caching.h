/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_BLE_GATT_CACHING_H_
#define _APP_BLE_GATT_CACHING_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief clear database hash value when factory reset
 *
 */
void app_ble_gatt_caching_clear_database_hash(void);

/**
 * @brief clear service change state value when factory reset
 *
 */
void app_ble_gatt_caching_clear_service_change_state_all(void);

void app_ble_gatt_caching_common_callback(uint8_t cb_type, void *p_cb_data);

/**
 * @brief app ble gatt caching init
 *
 */
void app_ble_gatt_caching_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BLE_GATT_CACHING_H_ */
