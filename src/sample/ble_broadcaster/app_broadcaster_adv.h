/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_broadcaster_adv.h
   * @brief     This file handles BLE broadcaster application routines.
   * @author    danni
   * @date      2022-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

#ifndef _APP_BROADCASTER_ADV__
#define _APP_BROADCASTER_ADV__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "stdbool.h"
#include "stdint.h"
#include "ble_ext_adv.h"

/** @defgroup BROADCASTER_APP Broadcaster Application
  * @brief Broadcaster Application
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief app_broadcaster_adv_init_nonconn_public
 * advertising parameter initialization: use public address to advertise nonconnectable Scannable undirected advertising.
 * Advertising data or scan response data shall not exceed 31 bytes.
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_LEGACY_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval for undirected and low duty directed advertising.
 *                   In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval for undirected and low duty directed advertising.
 *                   In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_broadcaster_adv_init_nonconn_public(void);

/**
 * @brief app_broadcaster_adv_init_nonconn_random
 * advertising parameter initialization: use static random address to advertise nonconnectable Scannable undirected advertising.
 * Advertising data or scan response data shall not exceed 31 bytes.
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_LEGACY_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval for undirected and low duty directed advertising.
 *                   In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval for undirected and low duty directed advertising.
 *                   In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_broadcaster_adv_init_nonconn_random(void);

/**
 * @brief app_broadcaster_adv_start
 * start advertising
 * @param duration_10ms If non-zero, indicates the duration that advertising is enabled.
                        0x0000:        Always advertising, no advertising duration.
                        0x0001-0xFFFF: Advertising duration, in units of 10ms.
 * @return true  BLE protocol stack has already receive this command and ready to execute,
 *   when this command execution complete, BLE protocol stack will send BLE_EXT_ADV_MGR_ADV_ENABLED to APP. @ref app_broadcaster_adv_callback
 * @return false  There has some errors that cause the BLE protocol stack fail to receive this command.
 */
bool app_broadcaster_adv_start(uint16_t duration_10ms);

/**
 * @brief app_broadcaster_adv_stop
 * stop advertising
 * @param app_cause please reference @ref app_adv_stop_cause.h
 * if you want to add new advertising stop cause, please added in @ref app_adv_stop_cause.h
 * @return true  BLE protocol stack has already receive this command and ready to execute,
 *   when this command execution complete, BLE protocol stack will send BLE_EXT_ADV_MGR_ADV_DISABLED to APP. @ref app_broadcaster_adv_callback
 * @return false There has some errors that cause the BLE protocol stack fail to receive this command.
 */
bool app_broadcaster_adv_stop(int8_t app_cause);


/**
 * @brief app_broadcaster_adv_get_state
 * get advertising state
 * @return @ref T_BLE_EXT_ADV_MGR_STATE
 */
T_BLE_EXT_ADV_MGR_STATE app_broadcaster_adv_get_state(void);

/**
 * @brief app_broadcaster_adv_update_randomaddr  update random address
 *
 * @param random_address
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_broadcaster_adv_update_randomaddr(uint8_t *random_address);

/**
 * @brief app_broadcaster_adv_update_advdata  update advertising data
 *
 * @param p_adv_data   BLE protocol stack will not reallocate memory for adv data,
 *                     so p_adv_data shall point to a global memory.
 *                     if you don't want to set adv data, set default value NULL.
 * @param adv_data_len Advertising data or scan response data length shall not exceed 31 bytes.
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_broadcaster_adv_update_advdata(uint8_t *p_adv_data, uint16_t adv_data_len);

/**
 * @brief app_broadcaster_adv_update_scanrspdata update scan response data
 *
 * @param p_scan_data   BLE protocol stack will not reallocate memory for scan response data,
 *                      so p_scan_data shall point to a global memory.
 *                      if you don't want to set scan response data, set default value NULL.
 * @param scan_data_len Advertising data or scan response data length shall not exceed 31 bytes.
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_broadcaster_adv_update_scanrspdata(uint8_t *p_scan_data, uint16_t scan_data_len);

/**
 * @brief app_broadcaster_adv_update_interval update advertising interval
 *
 * @param adv_interval advertising interval for undirected and low duty directed advertising.
 *                     In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_broadcaster_adv_update_interval(uint16_t adv_interval);

/** End of BROADCASTER_APP
* @}
*/

#ifdef __cplusplus
}
#endif
#endif
