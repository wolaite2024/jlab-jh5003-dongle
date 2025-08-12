/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __BLE_PA_SYNC_H__
#define __BLE_PA_SYNC_H__
#include "ual_adapter.h"
#include "gap_callback_le.h"
#include "adapter_int.h"
#include "gap_big_mgr.h"
#include "gap_pa_sync.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
{
    uint8_t     sync_id;
    uint16_t    sync_handle;
    uint8_t     state;
    uint16_t    cause;
} T_PA_SYNC_STATE_CHANGE_INFO;

typedef void(*T_LE_PA_SYNC_CBACK)(uint8_t adv_bd_type, uint8_t *adv_addr,
                                  uint8_t adv_sid, uint8_t *broadcast_id, T_PA_SYNC_STATE_CHANGE_INFO *p_data);
bool ble_register_pa_sync_callback(uint8_t app_id, T_LE_PA_SYNC_CBACK callback);
void ble_big_sync_register_cb_int(P_FUN_LE_BIG_MGR_SYNC_RX_CB cb_pfn);
T_APP_RESULT ble_big_sync_receiver_cb(uint8_t big_handle, uint8_t cb_type,
                                      void *p_cb_data);
void ble_pa_sync_handle_gap_cb(uint8_t cb_type, T_LE_CB_DATA *p_data);
bool ble_suspend_pa_sync(void);
void ble_pa_sync_resume_req(void);
bool ble_pa_sync(uint8_t app_id, uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                 uint8_t *broadcast_id,
                 bool add_to_pal);
void ble_pa_sync_init(void);
bool ble_terminate_pa_sync_by_sync_id(uint8_t app_id, uint8_t sync_id, bool rm_from_pal);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
