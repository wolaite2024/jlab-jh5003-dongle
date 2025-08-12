/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __UAL_ADV_H__
#define __UAL_ADV_H__

#include <stdint.h>
#include "ual_types.h"
#include "ual_dev_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BLE_ADV_STATE_CHANGE  0x01 /**< used to notify adv set application callback function about the change of ext adv state*/
#define BLE_ADV_SET_CONN_INFO 0x02
#define BLE_PA_STATE_CHANGE   0x03
#define BLE_ADV_RPA_CHANGE    0x04

typedef enum
{
    BLE_ADV_STOP_CAUSE_UNKNOWN,
    BLE_ADV_STOP_CAUSE_APP,
    BLE_ADV_STOP_CAUSE_CONN,
    BLE_ADV_STOP_CAUSE_TIMEOUT,
    BLE_ADV_STOP_CAUSE_ENABLE_FAILED,
    BLE_ADV_STOP_CAUSE_SUSPEND,
} T_BLE_ADV_STOP_CAUSE;

typedef enum
{
    BLE_ADV_STATE_IDLE,
    BLE_ADV_STATE_ADVERTISING,
} T_BLE_ADV_STATE;

typedef struct
{
    uint8_t adv_handle;
    uint8_t conn_id;
} T_BLE_ADV_SET_CONN_INFO;

typedef struct
{
    uint8_t adv_handle;
    T_BLE_ADV_STATE state;
    T_BLE_ADV_STOP_CAUSE stop_cause; /**< Used when: BLE_ADV_STATE_IDLE*/
    uint8_t app_cause; /**< Used when: BLE_ADV_STATE_IDLE(BLE_ADV_STOP_CAUSE_APP)*/
} T_BLE_ADV_STATE_CHANGE;

typedef struct
{
    uint8_t adv_handle;
    T_BLE_ADV_STATE state;
    uint8_t   rpa[BD_ADDR_LEN];
} T_BLE_ADV_RPA_CHANGE;


typedef union
{
    T_BLE_ADV_STATE_CHANGE *p_ble_state_change;
    T_BLE_ADV_SET_CONN_INFO *p_ble_conn_info;
    T_BLE_ADV_RPA_CHANGE *p_ble_rpa_change;
} T_BLE_ADV_CB_DATA;

typedef void(*P_FUN_ADV_APP_CB)(uint8_t cb_type, T_BLE_ADV_CB_DATA *p_cb_data);

#define LEGACY_ADV_IND_EVT                    0x13
#define LEGACY_ADV_DIRECT_IND_LOW_EVT         0x15
#define LEGACY_ADV_DIRECT_IND_HIGH_EVT        0x1D
#define LEGACY_ADV_SCAN_IND_EVT               0x12
#define LEGACY_ADV_NONCONN_IND_EVT            0x10


#define EXT_ADV_EVT_PROP_CONNECTABLE_ADV      0x01
#define EXT_ADV_EVT_PROP_SCANNABLE_ADV        0x02
#define EXT_ADV_EVT_PROP_DIRECTED_ADV         0x04
#define EXT_ADV_EVT_PROP_HDC_DIRECTED_ADV     0x08
#define EXT_ADV_EVT_PROP_USE_LEGACY_ADV       0x10
#define EXT_ADV_EVT_PROP_OMIT_ADV_ADDR        0x20
#define EXT_ADV_EVT_PROP_INCLUDE_TX_POWER     0x40

#define EXT_ADV_EXTENDED_ADV_NON_SCAN_NON_CONN_UNDIRECTED    0x00
#define EXT_ADV_EXTENDED_ADV_NON_SCAN_NON_CONN_DIRECTED      0x04
#define EXT_ADV_EXTENDED_ADV_CONN_UNDIRECTED                 0x01
#define EXT_ADV_EXTENDED_ADV_CONN_DIRECTED                   0x05
#define EXT_ADV_EXTENDED_ADV_SCAN_UNDIRECTED                 0x02
#define EXT_ADV_EXTENDED_ADV_SCAN_DIRECTED                   0x06

#define PRIM_ADV_CHANNEL_MAP_CONF             0x0001
#define FILTER_POLICY_CONF                    0x0002
#define PRIM_ADV_PHY_CONF                     0x0004
#define ADV_EVENT_PROP_CONF                   0x0008
#define SEC_ADV_MAX_SKIP_CONF                 0x0010
#define SEC_ADV_PHY_CONF                      0x0020
#define ADV_SID_CONF                          0x0040
#define SCAN_REQ_NOTIFY_CONF                  0x0080
#define LOCAL_BD_TYPE_CONF                 0x0100
#define PEER_ADDR_INFO_CONF                0x0200
#define ADV_INTERVAL_RANGE_CONF               0x0400
#define ADV_TX_POWER_CONF                     0x0800
typedef uint16_t T_ADV_PARAM_CONF;

typedef enum
{
    ADV_PHYS_1M       = 0x01,
    ADV_PHYS_2M       = 0x02,
    ADV_PHYS_CODED    = 0x03
} T_ADV_PHYS_TYPE;

typedef enum
{
    ADV_PHYS_PRIM_ADV_1M      = 0x01,
    ADV_PHYS_PRIM_ADV_CODED   = 0x03
} T_PHYS_PRIM_ADV_TYPE;


typedef enum
{
    ADV_FILTER_ANY = 0x00,
    ADV_FILTER_WHITE_LIST_SCAN = 0x01,
    ADV_FILTER_WHITE_LIST_CONN = 0x02,
    ADV_FILTER_WHITE_LIST_ALL = 0x03,
} T_ADV_FILTER_POLICY;


typedef struct
{
    uint8_t                 primary_adv_channel_map;
    T_ADV_FILTER_POLICY     filter_policy;
    T_PHYS_PRIM_ADV_TYPE    primary_adv_phy;
    uint8_t                 secondary_adv_max_skip;
    T_ADV_PHYS_TYPE         secondary_adv_phy;
    uint8_t                 adv_sid;
    bool                    scan_req_notify_enable;
    int8_t                  tx_power;
    T_BLE_BD_TYPE          local_bd_type;
    T_BLE_BD_TYPE          peer_bd_type;
    uint8_t                 peer_addr[BD_ADDR_LEN];
    uint16_t                adv_event_prop;
    uint32_t                adv_interval_min;
    uint32_t                adv_interval_max;
} T_ADV_PARAMS;

typedef struct
{
    uint8_t num_bis;
    uint32_t sdu_interval;/*3bytes*/
    uint16_t max_sdu;
    uint16_t max_transport_latency;
    uint8_t rtn;
    uint8_t phy; /* bit-field */
    uint8_t  packing;
    uint8_t  framing;
    uint8_t encryption;
    uint8_t broadcast_code[16];
} T_BIG_PARAM;
typedef struct
{
    uint8_t num_bis;
    uint32_t sdu_interval;
    uint16_t iso_interval;
    uint8_t nse;
    uint16_t max_sdu;
    uint16_t max_pdu;
    uint8_t phy;
    uint8_t packing;
    uint8_t framing;
    uint8_t bn;
    uint8_t irc;
    uint8_t pto;
    uint8_t encryption;
    uint8_t broadcast_code[16];
} T_BIG_TEST_PARAM;

typedef enum
{
    BIG_MGR_ALLOC_INFO                        = 0x01,
    BIG_MGR_SETUP_DATA_PATH                   = 0x02,
    BIG_MGR_REMOVE_DATA_PATH                  = 0x03,
    BIG_MGR_ISOC_STATE_CHANGE_INFO            = 0x04,
    BIG_MGR_ISOC_CREATE_CMPL_INFO             = 0x05,
} T_BIG_BC_MSG_TYPE;


typedef enum
{
    BIG_CAUSE_SUCCESS           = 0x00,
    BIG_CAUSE_ERROR             = 0x01,
} T_BLE_BIG_CAUSE;

typedef struct
{
    T_BLE_BIG_CAUSE cause;
    uint16_t adv_handle;
    uint8_t  big_handle;
} T_BIG_ALLOC_INFO;

typedef struct
{
    uint16_t cause;
    uint16_t bis_conn_handle;
    uint8_t  big_handle;
    uint8_t  data_path;
} T_BIG_HDL_DATA_PATH_RSP;


#define BIG_ISOC_BC_STATE_IDLE          0x00
#define BIG_ISOC_BC_STATE_CREATING      0x01
#define BIG_ISOC_BC_STATE_WAITING       0x02
#define BIG_ISOC_BC_STATE_BROADCASTING  0x03
#define BIG_ISOC_BC_STATE_TERMINATING   0x04

//extra state
#define BIG_ISOC_EXT_ADVERTISING        0x40
#define BIG_ISOC_PA_ADVERTISING         0x10
typedef uint8_t T_GAP_BIG_BC_STATE;

typedef struct
{
    uint16_t            cause;
    uint8_t             big_handle;
    uint8_t             adv_handle;
    T_GAP_BIG_BC_STATE  new_comb_state;
} T_BIG_ISOC_BC_STATE_CHANGE_INFO;

typedef struct
{
    uint8_t bis_idx;
    uint16_t bis_conn_handle;
} T_BIG_BIS_CONN_HANDLE_INFO;


typedef struct
{
    uint16_t cause;
    uint8_t  big_handle;
    uint8_t adv_handle;
    uint32_t big_sync_delay;
    uint32_t transport_latency_big;
    uint8_t phy;
    uint8_t nse;
    uint8_t bn;
    uint8_t pto;
    uint8_t irc;
    uint16_t max_pdu;
    uint16_t iso_interval;
    uint8_t num_bis;
    T_BIG_BIS_CONN_HANDLE_INFO bis_conn_handle_info[4];
} T_BIG_BC_CMPL_EVT_INFO;


typedef union
{
    T_BIG_ALLOC_INFO                         big_alloc_info;
    T_BIG_HDL_DATA_PATH_RSP                  big_handle_data_path_rsp;
    T_BIG_ISOC_BC_STATE_CHANGE_INFO          big_bc_state_change_info;
    T_BIG_BC_CMPL_EVT_INFO                   big_bc_create_big_cmpl_info;
} T_BIG_RES_DATA;


typedef void (*P_ISOC_BROADCAST_CB)(uint8_t big_handle, T_BIG_BC_MSG_TYPE cb_type,
                                    T_BIG_RES_DATA *p_cb_data);


uint8_t ble_alloc_adv_instance(P_FUN_ADV_APP_CB app_callback, uint16_t adv_event_prop,
                               uint32_t primary_adv_interval_min, uint32_t primary_adv_interval_max,
                               T_BLE_BD_TYPE local_bd_type, T_BLE_BD_TYPE peer_bd_type,
                               uint8_t *p_peer_addr, uint16_t adv_data_len, uint8_t *p_adv_data,
                               uint16_t scan_data_len, uint8_t *p_scan_data);
void ble_remove_adv(uint8_t adv_handle);
bool ble_enable_adv(uint8_t adv_handle, uint32_t duration_ms);
bool ble_disable_adv(uint8_t adv_handle, uint8_t app_cause);
bool ble_set_adv_param(uint8_t adv_handle, T_ADV_PARAM_CONF conf_msk, T_ADV_PARAMS *p_param);
bool ble_set_adv_data(uint8_t adv_handle, uint16_t adv_data_len, uint8_t *p_adv_data);
bool ble_set_scan_response_data(uint8_t adv_handle, uint16_t scan_data_len, uint8_t *p_scan_data);
bool ble_adv_init_pa(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                     uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop,
                     uint16_t periodic_adv_data_len, uint8_t *p_periodic_adv_data);
bool ble_adv_enable_pa(uint8_t adv_handle);
bool ble_adv_disable_pa(uint8_t adv_handle, uint8_t app_cause);

bool ble_adv_update_pa_param(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                             uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop);

bool ble_adv_update_set_pa_data(uint8_t adv_handle, uint16_t periodic_adv_data_len,
                                uint8_t *p_periodic_adv_data, bool pa_unchanged_data_flag);
bool ble_is_ext_adv(void);
bool ble_adv_create_big(uint8_t adv_handle, T_BIG_PARAM *p_param, P_ISOC_BROADCAST_CB callback);
bool ble_adv_terminate_big(uint8_t big_handle, uint8_t reason);
bool ble_adv_big_setup_data_path(uint16_t bis_conn_handle, uint8_t data_path_direction,
                                 uint8_t data_path_id, uint8_t codec_id[5], uint32_t controller_delay,
                                 uint8_t codec_config_len, uint8_t *p_codec_config);
bool ble_adv_big_remove_data_path(uint16_t bis_conn_handle, uint8_t data_path_direction);
bool ble_get_ext_adv_random_addr(uint8_t adv_handle, uint8_t *p_random_addr);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UAL_ADAPTER_H__ */
