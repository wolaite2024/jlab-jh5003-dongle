/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __DEV_MGR_H__
#define __DEV_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "ual_types.h"
#include "ual_dev_mgr.h"
#include <stdbool.h>
#include "gap_msg.h"
#include "gap_callback_le.h"
#include "gap_conn_le.h"
#include "connection_mgr.h"

#define LE_AUDIO_CONNECT_FASTER     0

#define MAX_NAME_LENGTH     16

#define BLE_WHITE_LIST_BIT          0x01
#define BLE_RESOLVING_LIST_BIT      0x02

struct bt_device;

struct bonding_req
{
    struct bt_device *device;
    uint8_t  type; /* le or bredr or dual */
    uint8_t  status;
};

struct browse_req
{
    struct bt_device *device;
    uint8_t  type; /* bredr or le */
};

typedef enum
{
    BLE_UPDATE_WAIT_RSP         = 0x01, /* waiting for connection update finished */
    BLE_UPDATE_DISABLE          = 0x02, /* conn update disabled */
    BLE_UPDATE_PEND             = 0x04, /* conn update pending */
} T_BLE_CONN_UPDATE;

typedef struct bt_device
{
    T_UALIST_HEAD           list;

    struct  bt_adapter      *adapter;

    uint8_t
    bd_addr[6];        //This address may be changed when BR/EDR struct and LE struct are merged
    uint8_t                 pseudo_addr[6]; /* First rpa or ident addr */
    T_BLE_BD_TYPE           bd_type;   //This address type bonded to pseudo_addr
    uint8_t                 identity_addr[6];
    T_BLE_BD_TYPE           identity_bd_type;
    uint8_t                 cur_rand_addr[6];   //current random address

    T_GAP_ROLE              role;
    uint8_t                 in_controller_list; /* in controller resolving list or not */
    uint8_t                 resolving_list_index;

    /* disconnected, connecting, connected, disconnecting */
    uint8_t                 ble_state;
    uint32_t                le_connect_mask;

    bool                    mtu_received;
    bool                    auth_cmpl;

    uint16_t                conn_handle;
    uint16_t                le_conn_handle;
    uint8_t                 le_conn_id;
    uint8_t                 link_key_type;
    uint8_t                 ltk_type;

    uint8_t                 irk[16];
    uint8_t                 link_key[16];
    uint8_t                 ltk[16];
    uint8_t                 ltk_len;

    uint32_t                cod;
    uint8_t                 name[MAX_NAME_LENGTH + 1];

    bool                    has_irk;
    uint8_t                 le_bonded;
    uint8_t                 bond_state;
    uint8_t                 auto_connect;
    uint8_t                 le_svc_resolved;
    uint8_t                 bredr_svc_resolved;

    T_BLE_CONN_PARAM        ble_conn_param;
    struct browse_req       *browse;
    struct bonding_req      *bonding;

    uint32_t                supported_profiles; /* bit mask, enough? */
    uint32_t                autoconn_profiles;  /* bit mask */
    T_UALIST_HEAD           profile_list;

    /* for profile connecting */
    T_UALIST_HEAD           pending;    /* pending profiles */
    uint8_t                 custom[4];
} T_BT_DEVICE;

struct bt_dev_bond_storage
{
    T_UALIST_HEAD       list;
    uint32_t            storage_offset;

    uint8_t             bd_addr[6];
    T_BLE_BD_TYPE       bd_type;
    uint8_t             irk[16];
    union
    {
        uint8_t         link_key[16];
        uint8_t         ltk[16];
    } key;
    uint32_t            supported_profiles; /* bit mask */
    uint32_t            autoconn_profiles; /* bit mask */
    uint8_t             custom[4];
};

typedef struct
{
    T_UALIST_HEAD       list;
    uint8_t             bd_addr[6];
    T_BLE_BD_TYPE       bd_type;
    uint32_t            cod;
    bool                adv_cmplt;
    uint8_t             *p_data;
    uint16_t            data_len;
} T_DISC_RESULT;


int bt_dev_mgr_init(void);
void clear_disc_db(void);
T_BT_DEVICE *ual_get_first_sec_dev(void);
T_BT_DEVICE *ual_get_next_sec_dev(T_BT_DEVICE *p_sec_dev);
T_DISC_RESULT *ual_find_disc_db_by_addr(uint8_t *bd_addr);
T_DISC_RESULT *ual_find_or_alloc_disc_db(uint8_t *bd_addr);
T_BT_DEVICE *ual_find_device_by_addr(uint8_t *bd_addr);
T_BT_DEVICE *ual_find_alloc_device_by_addr(uint8_t *bd_addr, T_BLE_BD_TYPE bd_type);
T_BT_DEVICE *ual_find_device_by_conn_id(uint8_t conn_id);
T_BT_DEVICE *ual_find_device_by_conn_handle(uint16_t conn_handle);
void le_conn_cback(uint8_t conn_id, uint8_t *bd_addr, uint8_t state);
void dev_mgr_handle_le_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause);
void dev_mgr_handle_le_bond_info(T_LE_BOND_MODIFY_INFO *p_modify_info);
T_BT_DEVICE *ble_resolve_random_addr(uint8_t *bd_addr);
bool ble_init_pseudo_addr(T_BT_DEVICE *p_dev_rec, uint8_t *new_pseudo_addr, T_BLE_BD_TYPE bd_type);
bool ble_identity_addr_to_random_pseudo(uint8_t **bd_addr, uint8_t *p_bd_type, bool refresh);
void dev_mgr_handle_le_conn_state(T_BT_DEVICE *p_dev_rec, T_GAP_CONN_STATE new_state,
                                  uint16_t disc_cause);
void dev_mgr_handle_le_pairing_confirm(uint8_t conn_id, uint16_t subtype);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DEV_MGR_H__ */
