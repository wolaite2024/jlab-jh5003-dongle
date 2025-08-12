/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __SCAN_MGR_H__
#define __SCAN_MGR_H__
#include "gap_callback_le.h"
#include "adapter_int.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BLE_LIMIT_DISC_FLAG (0x01 << 0)
#define BLE_GEN_DISC_FLAG (0x01 << 1)
#define BLE_BREDR_NOT_SPT (0x01 << 2)

typedef enum
{
    SCAN_IDLE_STATE = 0,
    SCAN_SUSPEND_STATE = 1,
    SCAN_ENABLE_STATE = 2,
} T_SCAN_STATE;


void ble_scan_mgr_init(void);
void ual_handle_ext_adv_report(T_BT_ADAPTER *adapter,
                               T_LE_EXT_ADV_REPORT_INFO *report);
void scan_mgr_handle_scan_state_info(uint8_t state, uint16_t cause);
bool bt_adap_start_le_scan(uint8_t scan_mode, uint8_t filter_policy);
bool bt_adap_stop_le_scan(void);

//These api shall only be called by ble privacy
bool le_scan_suspend_req(void);
bool le_scan_resume_req(void);
T_SCAN_STATE le_scan_get_scan_state(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
