/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BLE_LEGACY_ADV_H_
#define _BLE_LEGACY_ADV_H_

#include "gap_le.h"
#include "gap_msg.h"
#include "ual_adv.h"

#ifdef __cplusplus
extern "C" {
#endif

T_GAP_CAUSE ble_legacy_adv_mgr_set_adv_data(uint8_t adv_handle, uint16_t adv_data_len,
                                            uint8_t *p_adv_data);
T_GAP_CAUSE ble_legacy_adv_mgr_set_scan_rsp_data(uint8_t adv_handle, uint16_t scan_rsp_data_len,
                                                 uint8_t *p_scan_rsp_data);

T_GAP_CAUSE ble_legacy_set_adv_param_by_mask(uint8_t adv_handle, T_ADV_PARAM_CONF conf_msk,
                                             T_ADV_PARAMS *p_param);
T_GAP_CAUSE ble_legacy_adv_mgr_set_adv_param(uint8_t adv_handle, T_GAP_ADTYPE adv_type,
                                             uint16_t adv_interval_min, uint16_t adv_interval_max,
                                             T_GAP_LOCAL_ADDR_TYPE local_bd_type,
                                             T_GAP_REMOTE_ADDR_TYPE peer_bd_type, uint8_t *p_peer_addr,
                                             T_GAP_ADV_FILTER_POLICY filter_policy);
T_GAP_CAUSE ble_legacy_adv_mgr_enable(uint8_t adv_handle, uint32_t duration);
T_GAP_CAUSE ble_legacy_adv_mgr_disable(uint8_t adv_handle, uint8_t app_cause);

uint8_t ble_legacy_adv_mgr_alloc_adv(P_FUN_ADV_APP_CB app_callback);
T_GAP_CAUSE ble_legacy_adv_mgr_remove_adv(uint8_t adv_handle);

void ble_legacy_adv_mgr_handle_adv_state(T_GAP_DEV_STATE new_state, uint16_t cause);
T_APP_RESULT ble_legacy_adv_mgr_handle_gap_callback(uint8_t cb_type, T_LE_CB_DATA *p_data);
bool ble_legacy_adv_mgr_suspend(void);
bool ble_legacy_adv_mgr_resume(void);
T_GAP_CAUSE ble_legacy_adv_mgr_init(void);


#ifdef __cplusplus
}
#endif

#endif /* _BLE_LEGACY_ADV_H_ */
