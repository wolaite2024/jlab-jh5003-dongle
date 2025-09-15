/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_MGR_H_
#define _BT_MGR_H_

#include <stdint.h>
#include <stdbool.h>
#include "os_queue.h"
#include "sys_timer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BT_IPC_TOPIC                    "BT_TOPIC"

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

typedef enum t_bt_device_mode_action
{
    BT_DEVICE_MODE_ACTION_NONE                     = 0x00,
    BT_DEVICE_MODE_ACTION_IDLE                     = 0x01,
    BT_DEVICE_MODE_ACTION_DISCOVERABLE             = 0x02,
    BT_DEVICE_MODE_ACTION_CONNECTABLE              = 0x03,
    BT_DEVICE_MODE_ACTION_DISCOVERABLE_CONNECTABLE = 0x04,
} T_BT_DEVICE_MODE_ACTION;

typedef enum t_bt_pm_event
{
    BT_PM_EVENT_LINK_CONNECTED      = 0x00,
    BT_PM_EVENT_LINK_DISCONNECTED   = 0x01,
    BT_PM_EVENT_SNIFF_ENTER_SUCCESS = 0x02,
    BT_PM_EVENT_SNIFF_ENTER_FAIL    = 0x03,
    BT_PM_EVENT_SNIFF_ENTER_REQ     = 0x04,
    BT_PM_EVENT_SNIFF_EXIT_SUCCESS  = 0x05,
    BT_PM_EVENT_SNIFF_EXIT_FAIL     = 0x06,
    BT_PM_EVENT_SNIFF_EXIT_REQ      = 0x07,
} T_BT_PM_EVENT;

typedef bool (*P_BT_PM_CBACK)(uint8_t       bd_addr[6],
                              T_BT_PM_EVENT event);

typedef struct t_bt_pm_cback_item
{
    struct t_bt_pm_cback_item *p_next;
    P_BT_PM_CBACK              cback;
} T_BT_PM_CBACK_ITEM;

typedef void (*P_BT_ROLESWAP_SYNC_CBACK)(void);

T_BT_CLK_REF bt_piconet_clk_get(T_BT_CLK_REF  clk_ref,
                                uint32_t     *clk_slot,
                                uint16_t     *clk_us);

bool bt_piconet_id_get(T_BT_CLK_REF  clk_ref,
                       uint8_t      *pid,
                       uint8_t      *role);

bool bt_pm_cback_register(uint8_t       bd_addr[6],
                          P_BT_PM_CBACK cback);

bool bt_pm_cback_unregister(uint8_t       bd_addr[6],
                            P_BT_PM_CBACK cback);

void bt_pm_sm(uint8_t       bd_addr[6],
              T_BT_PM_EVENT event);

bool bt_pm_state_get(uint8_t             bd_addr[6],
                     T_BT_LINK_PM_STATE *pm_state);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_MGR_H_ */
