/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __UAL_ADAPTER_H__
#define __UAL_ADAPTER_H__

#include <stdint.h>
#include "gap_le_types.h"
#include "ual_types.h"
#include "ual_bluetooth.h"

#define  DISCOVERY_TYPE_BREDR   (1 << 0)
#define  DISCOVERY_TYPE_LE      (1 << 1)
#define  DISCOVERY_TYPE_DUAL    (DISCOVERY_TYPE_BREDR | DISCOVERY_TYPE_LE)

#define  SCAN_RESULT_BREDR      0
#define  SCAN_RESULT_LE         1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (*T_ADAPTET_CBACK)(uint16_t msg, uint8_t *msg_data, uint32_t msg_dlen);

typedef void (*T_UAL_LE_GAP_CBACK)(uint8_t cb_type, void *p_cb_data);

typedef void (*T_SCAN_RESULT_CBACK)(uint8_t type, void *result);
typedef struct le_adv_info
{
    uint16_t event_type;
    uint8_t  data_status;
    uint8_t  bd_type;
    uint8_t  bd_addr[6];
    uint8_t  primary_phy;
    uint8_t  secondary_phy;
    uint8_t  adv_sid;
    int8_t  tx_power;
    int8_t  rssi;
    uint16_t peri_adv_interval;
    uint8_t  direct_addr_type;
    uint8_t  direct_addr[6];
    uint8_t  data_len;
    uint8_t  *p_data;
} T_LE_ADV_INFO;

/* must called before  int bt_adap_init(T_ADAPTET_CBACK cb) */
bool bt_ual_set_param(uint16_t type, uint8_t len, void *p_value);

int bt_adap_init(T_ADAPTET_CBACK cb);
void bt_adap_set_scan_mdoe(T_GAP_SCAN_MODE scan_mode);
T_GAP_SCAN_MODE bt_adap_get_scan_mode(void);
int bt_adap_start(void);
int bt_adap_stop(void);
int bt_adap_start_discovery(uint8_t disc_type, uint8_t filter_policy, T_SCAN_RESULT_CBACK cb);
int bt_adap_stop_discovery(uint8_t client_id);
void bt_adap_start_task(void *evt_queue, void *io_queue, uint16_t msg_queue_elem_num);

void ual_le_register_app_cb(T_UAL_LE_GAP_CBACK cback);
void ual_le_register_app_cb2(T_UAL_LE_GAP_CBACK cback);
void ual_handle_ble_gap_msg(void *p_msg);
void ual_handle_gap_common_cb(uint8_t cb_type, void *p_cb_data);
bool ble_rpa_matches_local_adapter(uint8_t *rpa);
bool ual_support_privacy(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UAL_ADAPTER_H__ */
