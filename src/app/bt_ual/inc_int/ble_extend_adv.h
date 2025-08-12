/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#ifndef _BLE_EXTEND_ADV_MGR__
#define _BLE_EXTEND_ADV_MGR__

#include "gap_ext_adv.h"
#include "gap_le.h"
#include "gap_msg.h"
#include "ual_adv.h"
#include "gap_big_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PA_FEATURE_SUPPORT        1


T_GAP_CAUSE ble_extend_adv_mgr_init(void);
uint8_t ble_ext_adv_mgr_create_adv(P_FUN_ADV_APP_CB app_callback);
T_GAP_CAUSE ble_ext_adv_mgr_create_adv_param(uint8_t adv_handle, T_ADV_PARAM_CONF conf_msk,
                                             T_ADV_PARAMS *p_param, uint16_t adv_data_len, uint8_t *p_adv_data,
                                             uint16_t scan_data_len, uint8_t *p_scan_data);
uint8_t ble_ext_adv_mgr_alloc_adv(P_FUN_ADV_APP_CB app_callback, uint16_t adv_event_prop,
                                  uint32_t primary_adv_interval_min, uint32_t primary_adv_interval_max,
                                  T_GAP_LOCAL_ADDR_TYPE own_address_type, T_GAP_REMOTE_ADDR_TYPE peer_address_type,
                                  uint8_t *p_peer_address,
                                  T_GAP_ADV_FILTER_POLICY filter_policy, uint16_t adv_data_len, uint8_t *p_adv_data,
                                  uint16_t scan_data_len, uint8_t *p_scan_data);
T_GAP_CAUSE ble_adv_mgr_rmv_ext_adv(uint8_t adv_handle);
T_GAP_CAUSE ble_enable_ext_adv(uint8_t adv_handle, uint16_t duration_10ms);
T_GAP_CAUSE ble_disable_ext_adv(uint8_t adv_handle, uint8_t app_cause);
T_GAP_CAUSE ble_adv_mgr_set_ext_adv_param(uint8_t adv_handle, T_ADV_PARAM_CONF conf_msk,
                                          T_ADV_PARAMS *p_param);
T_GAP_CAUSE ble_ext_adv_mgr_start_adv_settings(uint8_t adv_handle, uint8_t update_flags);

T_GAP_CAUSE ble_disable_all_ext_adv(uint8_t app_cause);
T_GAP_CAUSE ble_adv_mgr_set_ext_adv_data(uint8_t adv_handle, uint16_t adv_data_len,
                                         uint8_t *p_adv_data);
T_GAP_CAUSE ble_adv_mgr_set_ext_scan_response_data(uint8_t adv_handle, uint16_t scan_data_len,
                                                   uint8_t *p_scan_data);
void ble_ext_adv_mgr_hdl_adv_state(uint8_t adv_handle, T_GAP_EXT_ADV_STATE new_state,
                                   uint16_t cause);
void ble_ext_adv_mgr_hdl_gap_cb(uint8_t cb_type, T_LE_CB_DATA *p_data);
void ble_ext_adv_mgr_hdl_conn_state(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                    uint16_t disc_cause);
bool ble_ext_adv_suspend(void);
void ble_ext_adv_resume(void);
T_GAP_CAUSE ble_pa_mgr_init(void);
T_GAP_CAUSE ble_ext_adv_mgr_create_pa(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                                      uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop,
                                      uint16_t periodic_adv_data_len, uint8_t *p_periodic_adv_data,
                                      bool pa_unchanged_data_flag);

T_GAP_CAUSE ble_ext_adv_mgr_alloc_pa(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                                     uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop,
                                     uint16_t periodic_adv_data_len, uint8_t *p_periodic_adv_data);
T_GAP_CAUSE ble_enable_pa(uint8_t adv_handle);
T_GAP_CAUSE ble_disable_pa(uint8_t adv_handle, uint8_t app_cause);
T_GAP_CAUSE ble_ext_adv_mgr_update_pa_param(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                                            uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop);
T_GAP_CAUSE ble_ext_adv_mgr_set_pa_data(uint8_t adv_handle, uint16_t periodic_adv_data_len,
                                        uint8_t *p_periodic_adv_data, bool pa_unchanged_data_flag);
void ble_big_register_iso_cb_int(P_FUN_LE_BIG_MGR_ISOC_BC_CB cb_pfn);
T_GAP_CAUSE le_create_big(uint8_t adv_handle, T_BIG_PARAM *p_param, P_ISOC_BROADCAST_CB callback);
T_GAP_CAUSE le_create_test_big(uint8_t adv_handle, T_BIG_TEST_PARAM *p_param,
                               P_ISOC_BROADCAST_CB callback);
T_GAP_CAUSE le_terminate_big(uint8_t big_handle, uint8_t reason);
T_APP_RESULT ble_big_isoc_broadcaster_cb(uint8_t big_handle, uint8_t cb_type, void *p_cb_data);
T_GAP_CAUSE ble_ext_adv_mgr_start_pa_setting(uint8_t adv_handle, uint8_t update_flags);


#ifdef __cplusplus
}
#endif

#endif
