/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_BLE_DEVICE_H_
#define _APP_BLE_DEVICE_H_

#include "app_roleswap.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_BLE_DEVICE App BLE Device
  * @brief App BLE Device
  * @{
  */

typedef enum
{
    APP_REMOTE_MSG_SECONDARY_SYNC_PRIMARY_IRK = 0x00,
    APP_REMOTE_MSG_PRIMARY_SYNC_SECONDARY_IRK = 0x01,
    APP_REMOTE_MSG_IRK_MAX_MSG_NUM,
} T_APP_BLE_IRK_SYNC_MSG;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_BLE_DEVICE_Exported_Functions App Ble Device Functions
    * @{
    */
/**
    * @brief  Ble Device module init
    * @param  void
    * @return void
    */

/**
 * @brief register bt_mgr_cback_register
 *
 */
void app_ble_device_init(void);

/**
 * @brief ble device clear keys when factory reset
 *
 */
void app_ble_device_handle_factory_reset(void);

/**
 * @brief ble device handle power on action, such as init params, start adv, ...
 *
 */
void app_ble_device_handle_power_on(void);

/**
 * @brief ble device handle power off action, such as de-init params, stop adv, ...
 *
 */
void app_ble_device_handle_power_off(void);

/**
 * @brief when radio mode changed, ble device maybe need do some actions.
 *
 * @param radio_mode @ref T_BT_DEVICE_MODE
 */
void app_ble_device_handle_radio_mode(T_BT_DEVICE_MODE radio_mode);

/**
 * @brief when device enter into pairing mode, ble device maybe need do some actions.
 *
 */
void app_ble_device_handle_enter_pair_mode(void);

/**
 * @brief when engage role decided, ble device maybe need do some actions.
 *
 * @param radio_mode @ref T_BT_DEVICE_MODE
 */
void app_ble_device_handle_engage_role_decided(T_BT_DEVICE_MODE radio_mode);

/**
 * @brief when legacy b2s connected, ble device maybe need do some actions
 *
 * @param bd_addr bd address
 */
void app_ble_device_handle_b2s_bt_connected(uint8_t *bd_addr);

/**
 * @brief when legacy b2s disconnected, ble device maybe need do some actions
 *
 * @param bd_addr bd address
 */
void app_ble_device_handle_b2s_bt_disconnected(uint8_t *bd_addr);

/**
 * @brief when b2b connected, ble device maybe need do some actions
 *
 */
void app_ble_device_handle_remote_conn_cmpl(void);

/**
 * @brief when b2b disconnected, ble device maybe need do some actions
 *
 */
void app_ble_device_handle_remote_disconn_cmpl(void);

/**
 * @brief when roleswap, ble device maybe need do some actions
 *
 * @param p_role_swap_status
 */
void app_ble_device_handle_role_swap(T_BT_EVENT_PARAM_REMOTE_ROLESWAP_STATUS *p_role_swap_status);

/**
 * @brief ble device active or not
 *
 * @return true  active
 * @return false inactive
 */
bool app_ble_device_is_active(void);

/**
 * @brief start power on rtk adv
 *
 */
void app_ble_device_handle_power_on_rtk_adv(void);

/**
 * @brief register irk sync relay cback and irk sync parse cback
 *
 * @return true  success
 * @return false fail
 */
bool app_ble_device_irk_sync_relay_init(void);
/** @} */ /* End of group APP_BLE_DEVICE_Exported_Functions */

/** End of APP_BLE_DEVICE
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BLE_DEVICE_H_ */
