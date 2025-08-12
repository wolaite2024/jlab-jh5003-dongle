/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_MGR_H_
#define _BT_MGR_H_

#include <stdint.h>
#include <stdbool.h>

#include "os_queue.h"
#include "sys_timer.h"
#include "remote.h"
#include "btm.h"
#include "bt_a2dp_int.h"
#include "bt_avrcp_int.h"
#include "bt_hfp_int.h"
#include "bt_hid_int.h"
#include "bt_spp_int.h"
#include "bt_iap_int.h"
#include "bt_pbap_int.h"
#include "bt_map_int.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BT_IPC_TOPIC                    "BT_TOPIC"

#define A2DP_PROFILE_MASK               0x00000001    /* A2DP profile bitmask */
#define AVRCP_PROFILE_MASK              0x00000002    /* AVRCP profile bitmask */
#define HFP_PROFILE_MASK                0x00000004    /* HFP profile bitmask */
#define HSP_PROFILE_MASK                0x00000008    /* HSP profile bitmask */
#define SPP_PROFILE_MASK                0x00000010    /* SPP profile bitmask */
#define IAP_PROFILE_MASK                0x00000020    /* iAP profile bitmask */
#define PBAP_PROFILE_MASK               0x00000040    /* PBAP profile bitmask */
#define HID_DEVICE_PROFILE_MASK         0x00000080    /* HID Device profile bitmask */
#define HID_HOST_PROFILE_MASK           0x00000200    /* HID Host profile bitmask */
#define MAP_PROFILE_MASK                0x00000400    /* MAP profile bitmask */
#define OPP_PROFILE_MASK                0x00000800    /* OPP profile bitmask */
#define RDTP_PROFILE_MASK               0x00008000    /* Remote Control vendor profile bitmask */

typedef enum t_bt_ipc_id
{
    BT_IPC_REMOTE_CONNECTED         = 0x00000080,
    BT_IPC_REMOTE_DISCONNECTED      = 0x00000081,
    BT_IPC_REMOTE_SESSION_ACTIVE    = 0x00000082,
    BT_IPC_REMOTE_SESSION_SLEEP     = 0x00000083,
    BT_IPC_REMOTE_DATA_IND          = 0x00000084,
    BT_IPC_REMOTE_DATA_RSP          = 0x00000085,
    BT_IPC_REMOTE_SWAP_START        = 0x00000086,
    BT_IPC_REMOTE_SWAP_STOP         = 0x00000087,
} T_BT_IPC_ID;

typedef enum t_bt_clk_ref
{
    BT_CLK_NONE         = 0x00,
    BT_CLK_SNIFFING     = 0x01,
    BT_CLK_CTRL         = 0x02,
} T_BT_CLK_REF;

typedef struct t_bt_ipc_msg_remote_connected
{

} T_BT_IPC_MSG_REMOTE_CONNECTED;

typedef struct t_bt_ipc_msg_remote_disconnected
{

} T_BT_IPC_MSG_REMOTE_DISCONNECTED;

typedef struct t_bt_ipc_msg_remote_session_active
{

} T_BT_IPC_MSG_REMOTE_SESSION_ACTIVE;

typedef struct t_bt_ipc_msg_remote_session_sleep
{

} T_BT_IPC_MSG_REMOTE_SESSION_SLEEP;

typedef struct t_bt_ipc_msg_remote_data_ind
{
    uint8_t    *buf;
    uint16_t    len;
} T_BT_IPC_MSG_REMOTE_DATA_IND;

typedef union t_bt_ipc_msg
{
    T_BT_IPC_MSG_REMOTE_CONNECTED        bt_remote_connected;
    T_BT_IPC_MSG_REMOTE_DISCONNECTED     bt_remote_disconnected;
    T_BT_IPC_MSG_REMOTE_SESSION_ACTIVE   bt_remote_session_active;
    T_BT_IPC_MSG_REMOTE_SESSION_SLEEP    bt_remote_session_sleep;
    T_BT_IPC_MSG_REMOTE_DATA_IND         bt_remote_data_ind;
} T_BT_IPC_MSG;

typedef enum t_bt_role_switch_status
{
    BT_ROLE_SWITCH_IDLE             = 0x00, /* role switch not in progress */
    BT_ROLE_SWITCH_MASTER_RUNNING   = 0x01, /* role switch to master in progress */
    BT_ROLE_SWITCH_MASTER_PENDING   = 0x02, /* role switch to master waiting */
    BT_ROLE_SWITCH_SLAVE_RUNNING    = 0x03, /* role switch to slave in progress */
    BT_ROLE_SWITCH_SLAVE_PENDING    = 0x04, /* role switch to slave waiting */
} T_BT_ROLE_SWITCH_STATUS;

typedef enum t_bt_link_pm_state
{
    BT_LINK_PM_STATE_ACTIVE         = 0x00, /* BR/EDR link in active steady state */
    BT_LINK_PM_STATE_SNIFF_PENDING  = 0x01, /* BR/EDR link in sniff pending transient state */
    BT_LINK_PM_STATE_SNIFF          = 0x02, /* BR/EDR link in sniff steady state */
    BT_LINK_PM_STATE_ACTIVE_PENDING = 0x03, /* BR/EDR link in active pending transient state */
} T_BT_LINK_PM_STATE;

typedef enum t_bt_link_pm_action
{
    BT_LINK_PM_ACTION_IDLE          = 0x00, /* BR/EDR link in idle (no pending) action */
    BT_LINK_PM_ACTION_SNIFF_ENTER   = 0x01, /* BR/EDR link in sniff enter pending action */
    BT_LINK_PM_ACTION_SNIFF_EXIT    = 0x02, /* BR/EDR link in sniff exit pending action */
} T_BT_LINK_PM_ACTION;

typedef enum t_bt_link_state
{
    BT_LINK_STATE_IDLE          = 0x00,
    BT_LINK_STATE_DISCONNECTED  = 0x01,
    BT_LINK_STATE_CONNECTED     = 0x02,
} T_BT_LINK_STATE;

typedef enum t_bt_device_mode_action
{
    BT_DEVICE_MODE_ACTION_NONE                     = 0x00,
    BT_DEVICE_MODE_ACTION_IDLE                     = 0x01,
    BT_DEVICE_MODE_ACTION_DISCOVERABLE             = 0x02,
    BT_DEVICE_MODE_ACTION_CONNECTABLE              = 0x03,
    BT_DEVICE_MODE_ACTION_DISCOVERABLE_CONNECTABLE = 0x04,
} T_BT_DEVICE_MODE_ACTION;

typedef struct t_bt_br_link
{
    uint8_t                 bd_addr[6];
    uint8_t                 link_id;
    uint16_t                acl_handle;
    uint16_t                sco_handle;

    T_BT_LINK_STATE         acl_link_state;
    bool                    acl_link_authenticated;
    bool                    acl_link_encrypted;
    bool                    acl_link_sc_ongoing;
    bool                    acl_link_role_master;
    uint16_t                acl_link_policy;
    T_BT_ROLE_SWITCH_STATUS role_switch_status;
    bool                    pm_enable;
    T_BT_LINK_PM_STATE      pm_state;
    T_BT_LINK_PM_ACTION     pm_action;
    uint32_t                pm_timeout;
    uint16_t                min_interval;
    uint16_t                max_interval;
    uint16_t                sniff_attempt;
    uint16_t                sniff_timeout;

    T_OS_QUEUE              pm_cback_list;

    uint8_t                 is_esco;
    uint8_t                 sco_air_mode;
    uint8_t                 sco_packet_length;
    uint8_t                 curr_sco_len;
    uint8_t                 sco_buf[60];

    uint32_t                connected_profile;
    T_HFP_LINK_DATA         hfp_data;
    T_HID_LINK_DATA         hid_data;
    T_AVRCP_LINK_DATA       avrcp_data;
    T_PBAP_LINK_DATA        pbap_data;
    T_A2DP_LINK_DATA        a2dp_data;
    T_SPP_LINK_DATA         spp_data;
    T_IAP_LINK_DATA         iap_data;
    T_MAP_LINK_DATA         map_data;

    T_SYS_TIMER_HANDLE      timer_enter_sniff;
} T_BT_BR_LINK;

typedef struct t_roleswap_info
{
    bool                used;
    uint8_t             bd_addr[6];
    T_OS_QUEUE          info_list;
} T_ROLESWAP_INFO;

typedef struct t_btm_db
{
    T_BT_BR_LINK            *br_link;
    T_BT_BR_LINK            *remote_link;
    T_ROLESWAP_INFO         *roleswap_info;
    T_REMOTE_RELAY_HANDLE    relay_handle;
    T_OS_QUEUE               cback_list;
    T_BT_DEVICE_MODE         curr_dev_mode;
    T_BT_DEVICE_MODE_ACTION  pending_dev_mode_action;
    T_BT_DEVICE_MODE_ACTION  next_dev_mode_action;
    uint8_t                  active_a2dp_index;
    uint8_t                  br_link_num;
    bool                     stack_ready;
} T_BTM_DB;

extern T_BTM_DB btm_db;

T_BT_BR_LINK *bt_find_br_link(uint8_t bd_addr[6]);

T_BT_BR_LINK *bt_find_br_link_by_handle(uint16_t handle);

T_BT_BR_LINK *bt_alloc_br_link(uint8_t bd_addr[6]);

void bt_free_br_link(T_BT_BR_LINK *p_link);

bool bt_link_policy_set(uint8_t  bd_addr[6],
                        uint16_t link_policy);

bool bt_link_policy_get(uint8_t   bd_addr[6],
                        uint16_t *link_policy);

T_BT_CLK_REF bt_piconet_clk_get(T_BT_CLK_REF  clk_ref,
                                uint32_t     *clk_slot,
                                uint16_t     *clk_us);

bool bt_piconet_id_get(T_BT_CLK_REF  clk_ref,
                       uint8_t      *pid,
                       uint8_t      *role);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_MGR_H_ */
