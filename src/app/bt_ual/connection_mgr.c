/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <string.h>
#include <os_mem.h>
#include <trace.h>
#include "ual_types.h"
#include "ual_list.h"
//#include "ual_errno.h"
#include "ual_upperstack_cfg.h"

#include "btm.h"
#include "gap_msg.h"
#include "gap_ext_scan.h"

#include "dev_mgr.h"
#include "ual_api_types.h"
#include "gap_le.h"
#include "connection_mgr.h"
#include "app_timer.h"
#include "gap_privacy.h"
#include "ual_bluetooth.h"
#include "gatt.h"
#include "ble_direct_conn.h"
#include "ble_bg_conn.h"


#define CONN_TIME_OUT       30000

#define LE_DATA_LEN_TX_OCTETS      251     //Maximum allowable setting value
#define LE_DATA_LEN_TX_TIME        0x0848  //ms, refers to the setting value of upperstack

#define CONN_CE_MIN         0x0002

typedef struct
{
    T_UALIST_HEAD     list;
    T_APP_ID           app_id;
    T_LE_CONN_CBACK    callback;
} T_APP_CONN_CBACK;

typedef struct
{
    T_UALIST_HEAD     list;
    uint8_t           bd_addr[6];
    uint8_t           bd_type;
    bool              bg_conn;
} T_CONN_DEV_NODE;

typedef struct app_conn_mgr
{
    T_UALIST_HEAD   conn_devs_set;
    T_UALIST_HEAD   app_conn_cback;
    T_BLE_CONN_PARAM conn_param;
    bool            gaming;
} T_APP_CONN_MGR;


static T_APP_CONN_MGR conn_mgr;
static T_CONN_DEV_NODE *alloc_conn_node_by_addr(uint8_t *bd_addr, uint8_t bd_type);
static T_CONN_DEV_NODE *find_conn_node_by_addr(uint8_t *bd_addr, uint8_t bd_type);
static void ble_conn_mgr_handle_connected(uint8_t conn_id);
static void ble_conn_mgr_handle_disconnected(uint8_t conn_id);

extern void gatt_client_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state);
extern void gatt_svc_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state);

static T_UAL_CONN_MGR_APP_CBACK conn_mgr_app_callback;
static uint8_t do_conn_update = true;
static bool conn_update_by_app = false;

static T_APP_CONN_CBACK *conn_mgr_find_app_callback(T_APP_ID app_id)
{
    T_UALIST_HEAD *pos, *n;
    T_APP_CONN_CBACK *p_conn_callback = NULL;

    ualist_for_each_safe(pos, n, &conn_mgr.app_conn_cback)
    {
        p_conn_callback = ualist_entry(pos, T_APP_CONN_CBACK, list);
        if (p_conn_callback->app_id == app_id)
        {
            return p_conn_callback;
        }
    }
    return NULL;
}

static T_CONN_DEV_NODE *alloc_conn_node_by_addr(uint8_t *bd_addr, uint8_t bd_type)
{
    T_CONN_DEV_NODE *p_dev;

    p_dev = os_mem_zalloc(RAM_TYPE_BT_UAL, sizeof(T_CONN_DEV_NODE));
    if (p_dev)
    {
        memcpy(p_dev->bd_addr, bd_addr, BD_ADDR_LEN);
        p_dev->bd_type = bd_type;
        init_ualist_head(&p_dev->list);
        ualist_add_tail(&p_dev->list, &conn_mgr.conn_devs_set);
    }
    return p_dev;
}

static T_CONN_DEV_NODE *find_conn_node_by_addr(uint8_t *bd_addr, uint8_t bd_type)
{
    T_UALIST_HEAD *pos, *n;
    T_CONN_DEV_NODE *p_dev;

    ualist_for_each_safe(pos, n, &conn_mgr.conn_devs_set)
    {
        p_dev = ualist_entry(pos, T_CONN_DEV_NODE, list);
        if (!memcmp(p_dev->bd_addr, bd_addr, BD_ADDR_LEN) &&
            (p_dev->bd_type == bd_type))
        {
            return p_dev;
        }
    }
    return NULL;
}

void ble_set_gaming_mode(bool gaming)
{
    conn_mgr.gaming = gaming;
    APP_PRINT_INFO1("ble_set_gaming_mode: mode %d", gaming);
    ble_direct_set_mode(gaming);
}

bool ble_is_gaming_mode(void)
{
    APP_PRINT_INFO1("ble_is_gaming_mode: mode %d", conn_mgr.gaming);
    return conn_mgr.gaming ? true : false;
}


void ble_conn_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
    APP_PRINT_INFO2("conn_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
    T_GAP_CONN_INFO conn_info;
    T_BT_DEVICE *p_dev;

    if (!le_get_conn_info(conn_id, &conn_info))
    {
        APP_PRINT_ERROR1("conn_handle_conn_mtu_info_evt: can't find conn info %d", conn_id);
        return;
    }

    p_dev = ual_find_device_by_addr(conn_info.remote_bd);

    if (p_dev)
    {
        p_dev->mtu_received = true;
        if (p_dev->auth_cmpl)
        {
            gatt_client_start_discovery_all(le_get_conn_handle(conn_id), ble_gatt_client_discover_cb);
        }
    }
}

void ble_conn_mgr_handle_le_conn_state(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                       uint16_t disc_cause)
{
    T_GAP_CONN_INFO conn_info;
    T_BT_DEVICE *p_dev = NULL;
    uint16_t conn_interval;
    APP_PRINT_INFO3("conn_mgr_handle_le_conn_state: conn_id %d, conn_state %d, disc_cause 0x%x",
                    conn_id, new_state, disc_cause);
    memset(&conn_info, 0, sizeof(T_GAP_CONN_INFO));
    switch (new_state)
    {
    case GAP_CONN_STATE_CONNECTING:
        {
            if (!le_get_conn_info(conn_id, &conn_info))
            {
                APP_PRINT_ERROR1("conn_mgr_handle_le_conn_state: can't find conn info %d", conn_id);
                break;
            }
            p_dev = ual_find_alloc_device_by_addr(conn_info.remote_bd, conn_info.remote_bd_type);
            if (p_dev != NULL)
            {

                p_dev->le_conn_handle = le_get_conn_handle(conn_id);
                p_dev->ble_state = GAP_CONN_STATE_CONNECTING;
                p_dev->role = conn_info.role;
                p_dev->le_conn_id = conn_id;
                ble_init_pseudo_addr(p_dev, conn_info.remote_bd, conn_info.remote_bd_type);
            }
        }
        break;
    case GAP_CONN_STATE_CONNECTED:
        {
            /*update maximum transmission payload size and maximum packet transmission time*/
            le_set_data_len(conn_id, LE_DATA_LEN_TX_OCTETS, LE_DATA_LEN_TX_TIME);

            if (!le_get_conn_info(conn_id, &conn_info))
            {
                APP_PRINT_ERROR1("conn_mgr_handle_le_conn_state: can't find conn info %d", conn_id);
                break;
            }

            p_dev = ual_find_alloc_device_by_addr(conn_info.remote_bd, conn_info.remote_bd_type);
            if (p_dev != NULL)
            {
                p_dev->le_conn_handle = le_get_conn_handle(conn_id);
                p_dev->ble_state = GAP_CONN_STATE_CONNECTED;
                p_dev->role = conn_info.role;
                p_dev->le_conn_id = conn_id;
                ble_init_pseudo_addr(p_dev, conn_info.remote_bd, conn_info.remote_bd_type);
                if (conn_info.remote_bd_type & BLE_BD_TYPE_ID_BIT)
                {
                    APP_PRINT_INFO0("GAP_CONN_STATE_CONNECTED, address is identity address, read RPA");
                    le_privacy_read_peer_resolv_addr((T_GAP_IDENT_ADDR_TYPE)(conn_info.remote_bd_type &
                                                                             (~BLE_BD_TYPE_ID_BIT)),
                                                     conn_info.remote_bd);
                    le_privacy_read_local_resolv_addr((T_GAP_IDENT_ADDR_TYPE)(conn_info.remote_bd_type &
                                                                              (~BLE_BD_TYPE_ID_BIT)),
                                                      conn_info.remote_bd);
                }
                else if (conn_info.remote_bd_type == BLE_ADDR_RANDOM)
                {
                    memcpy(p_dev->cur_rand_addr, conn_info.remote_bd, BD_ADDR_LEN);
                }

                le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
                le_get_conn_param(GAP_PARAM_CONN_LATENCY, &p_dev->ble_conn_param.latency, conn_id);
                le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &p_dev->ble_conn_param.timeout, conn_id);

                APP_PRINT_INFO3("GAP_CONN_STATE_CONNECTED interval %x latency %x timeout %x",
                                conn_interval,
                                p_dev->ble_conn_param.latency,
                                p_dev->ble_conn_param.timeout);
                p_dev->ble_conn_param.ci_min = conn_interval;
                p_dev->ble_conn_param.ci_max = conn_interval;
                if (p_dev->ble_conn_param.ce_2slot_used)
                {
                    p_dev->ble_conn_param.ce_2slot_used = false;
                    p_dev->ble_conn_param.ce_min = CONN_CE_MIN;
                    p_dev->ble_conn_param.ce_max = CONN_CE_MIN;
                }
                else
                {
                    p_dev->ble_conn_param.ce_min = 2 * (conn_interval - 1);
                    p_dev->ble_conn_param.ce_max = 2 * (conn_interval - 1);
                }
                ble_conn_mgr_handle_connected(conn_id);
            }
        }
        break;
    case GAP_CONN_STATE_DISCONNECTING:
        {
            p_dev = ual_find_device_by_conn_id(conn_id);
            if (p_dev != NULL)
            {
                memcpy(conn_info.remote_bd, p_dev->pseudo_addr, BD_ADDR_LEN);
                conn_info.remote_bd_type = p_dev->bd_type;
            }
        }
        break;

    case GAP_CONN_STATE_DISCONNECTED:
        {
            p_dev = ual_find_device_by_conn_id(conn_id);
            if (p_dev != NULL)
            {
                memcpy(conn_info.remote_bd, p_dev->pseudo_addr, BD_ADDR_LEN);
                conn_info.remote_bd_type = p_dev->bd_type;
                ble_conn_mgr_handle_disconnected(conn_id);
                p_dev->le_conn_handle = 0;
                p_dev->ble_state = GAP_CONN_STATE_DISCONNECTED;
                p_dev->mtu_received = false;
                p_dev->auth_cmpl = false;
                p_dev->le_conn_id = 0xFF;
                memset((uint8_t *)&p_dev->ble_conn_param, 0, sizeof(T_BLE_CONN_PARAM));
            }
        }
        break;
    }
    if (p_dev)
    {
        memcpy(conn_info.remote_bd, p_dev->pseudo_addr, BD_ADDR_LEN);
        conn_info.remote_bd_type = p_dev->bd_type;
    }
    dev_mgr_handle_le_conn_state(p_dev, new_state, disc_cause);

    if (bt_hal_cbacks)
    {
        T_BT_CONN_INFO info;
        info.state = (T_BT_CONN_STATE)new_state;
        info.conn_id = conn_id;
        info.bd_type = conn_info.remote_bd_type;
        info.type = TRANSPORT_TYPE_LE;
        memcpy(info.bd_addr, conn_info.remote_bd, BD_ADDR_LEN);
        info.disc_cause = disc_cause;
        bt_hal_cbacks(UAL_DEV_CONN_STATE_CHANGE, (uint8_t *)&info, sizeof(T_BT_CONN_INFO));
    }

    gatt_client_handle_conn_state_evt(conn_id, new_state);
    gatt_svc_handle_conn_state_evt(conn_id, new_state);
}

bool ble_conn_mgr_connect(uint8_t *bd_addr, uint8_t bd_type, bool bg_conn)
{
    if (!bd_addr)
    {
        return false;
    }

    T_BT_DEVICE *p_dev = NULL;
    T_CONN_DEV_NODE *p_node = NULL;
    T_APP_CONN_CBACK *p_conn_cb = NULL;
    uint8_t conn_id;

    APP_PRINT_INFO3("ble_conn_mgr_connect: bd_addr %s %d bg_conn %d", TRACE_BDADDR(bd_addr), bd_type,
                    bg_conn);
    p_dev = ual_find_device_by_addr(bd_addr);

    if (p_dev == NULL)
    {
        APP_PRINT_ERROR0("ble_conn_mgr_connect: can't find device record");
        return false;
    }

    /* get dev current conn state */
    if (p_dev->ble_state == BT_CONN_STATE_CONNECTED)
    {
        if (le_get_conn_id_by_handle(p_dev->le_conn_handle, &conn_id))
        {
            p_conn_cb = conn_mgr_find_app_callback(DEV_MGR_APP_ID);
            if (p_conn_cb)
            {
                p_conn_cb->callback(conn_id, bd_addr, GAP_CONN_STATE_CONNECTED);
            }
        }
        return true;
    }

    p_node = find_conn_node_by_addr(p_dev->pseudo_addr, p_dev->bd_type);
    if (p_node == NULL)
    {
        p_node = alloc_conn_node_by_addr(p_dev->pseudo_addr, p_dev->bd_type);
        if (p_node == NULL)
        {
            APP_PRINT_ERROR0("ble_conn_mgr_connect: alloc fail");
            return false;
        }
        p_node->bg_conn = bg_conn;
    }

    if (p_dev->ble_state == BT_CONN_STATE_CONNECTING)
    {
        return true;
    }

    if (bg_conn)
    {
        return ble_bg_connect(p_dev->pseudo_addr, bd_type);
    }
    else
    {
        return ble_direct_connect(p_dev->pseudo_addr, bd_type);
    }

}

bool ble_conn_mgr_disconnect(uint8_t *bd_addr, uint8_t bd_type)
{
    T_BT_DEVICE *p_dev = ual_find_device_by_addr(bd_addr);
    T_APP_CONN_CBACK *p_conn_cb = NULL;
    T_CONN_DEV_NODE *p_node = NULL;
    bool ret = false;

    APP_PRINT_INFO2("ble_conn_mgr_disconnect: bd_addr %s %d", TRACE_BDADDR(bd_addr), bd_type);
    if (p_dev == NULL)
    {
        APP_PRINT_ERROR0("ble_conn_mgr_disconnect: can't find device record");
        return false;
    }

    p_node = find_conn_node_by_addr(p_dev->pseudo_addr, p_dev->bd_type);

    if (p_dev->ble_state == GAP_CONN_STATE_DISCONNECTED)
    {
        p_conn_cb = conn_mgr_find_app_callback(DEV_MGR_APP_ID);
        if (p_conn_cb)
        {
            p_conn_cb->callback(0xFF, bd_addr, GAP_CONN_STATE_DISCONNECTED);
        }

        if (p_node)
        {
            ualist_del(&p_node->list);
            os_mem_free(p_node);
        }
        return true;
    }

    if (p_dev->ble_state == BT_CONN_STATE_DISCONNECTING)
    {
        return true;
    }

    if (p_node)
    {
        if (p_node->bg_conn)
        {
            ret = ble_bg_disconnect(p_dev->pseudo_addr, bd_type);
        }
        else
        {
            ret = ble_direct_disconnect(p_dev->pseudo_addr, bd_type);
        }
    }
    else
    {
        ret = ble_direct_disconnect(p_dev->pseudo_addr, bd_type);
    }

    return ret;
}

static void ble_conn_mgr_handle_connected(uint8_t conn_id)
{
    APP_PRINT_INFO1("ble_conn_mgr_handle_connected conn_id %d", conn_id);
    T_APP_CONN_CBACK *p_conn_cb = NULL;
    T_BT_DEVICE *p_dev = ual_find_device_by_conn_id(conn_id);
    if (!p_dev)
    {
        APP_PRINT_ERROR1("ble_conn_mgr_handle_connected conn_id %d", conn_id);
        return;
    }
    T_CONN_DEV_NODE *p_node = find_conn_node_by_addr(p_dev->pseudo_addr, p_dev->bd_type);
    if (p_node)
    {
        p_conn_cb = conn_mgr_find_app_callback(DEV_MGR_APP_ID);
        if (p_conn_cb)
        {
            p_conn_cb->callback(conn_id, p_dev->pseudo_addr, GAP_CONN_STATE_CONNECTED);
        }

        if (p_node->bg_conn)
        {
            ble_bg_handle_connect(p_node->bd_addr, p_node->bd_type);
        }
        else
        {
            ble_direct_handle_connected(p_node->bd_addr, p_node->bd_type);
        }
    }
}

static void ble_conn_mgr_handle_disconnected(uint8_t conn_id)
{
    APP_PRINT_INFO1("ble_conn_mgr_handle_disconnected: conn_id %d", conn_id);
    T_CONN_DEV_NODE *p_node;
    T_APP_CONN_CBACK *p_conn_cb = NULL;
    T_BT_DEVICE *p_dev = ual_find_device_by_conn_id(conn_id);
    if (!p_dev)
    {
        APP_PRINT_INFO1("ble_conn_mgr_handle_disconnected: conn_id %d find dev fail", conn_id);
        return;
    }

    p_node = find_conn_node_by_addr(p_dev->pseudo_addr, p_dev->bd_type);

    if (!p_node)
    {
        APP_PRINT_ERROR2("ble_conn_mgr_handle_disconnected: cannot find %s %d",
                         TRACE_BDADDR(p_dev->pseudo_addr), p_dev->bd_type);
        return;
    }

    if (p_node->bg_conn)
    {
        ble_bg_handle_disconnected(p_dev->pseudo_addr, p_dev->bd_type);
    }
    else
    {
        ble_direct_handle_disconnected(p_dev->pseudo_addr, p_dev->bd_type);
    }

    p_conn_cb = conn_mgr_find_app_callback(DEV_MGR_APP_ID);
    if (p_conn_cb)
    {
        p_conn_cb->callback(conn_id, p_dev->pseudo_addr, GAP_CONN_STATE_DISCONNECTED);
    }
    ualist_del(&p_node->list);
    os_mem_free(p_node);
}


static bool ble_conn_pend_params_need_update(T_BT_DEVICE *p_dev)
{
    if (!p_dev)
    {
        return false;
    }

    T_BLE_CONN_PARAM *p_conn = &(p_dev->ble_conn_param);
    if (!p_conn)
    {
        return false;
    }
    if (!p_conn->param_update_pend)
    {
        return false;
    }

    if ((p_conn->ci_min == p_conn->ci_min_pend) &&
        (p_conn->ci_max == p_conn->ci_max_pend) &&
        (p_conn->latency == p_conn->latency_pend) &&
        (p_conn->timeout == p_conn->timeout_pend) &&
        (p_conn->ce_min == p_conn->ce_min_pend) &&
        (p_conn->ce_max == p_conn->ce_max_pend))
    {
        return false;
    }

    return true;

}
/*******************************************************************************
 *
 *  Function        ble_start_conn_update
 *
 *  Description     Start the BLE connection parameter update process based on
 *                  status.
 *
 *  Parameters:     lcb : l2cap link control block
 *
 *  Return value:   none
 *
 ******************************************************************************/
static void ble_start_conn_update(T_BT_DEVICE *p_dev)
{
    bool ret = false;
    T_GAP_CAUSE cause;
    if (!p_dev)
    {
        APP_PRINT_ERROR0("ble_start_conn_update No known connection ACL for");
        return;
    }

    T_BLE_CONN_PARAM *p_conn = &(p_dev->ble_conn_param);
    if (!p_conn)
    {
        return;
    }

    if (!do_conn_update || conn_update_by_app)
    {
        APP_PRINT_ERROR0("ble_start_conn_update: it is not allow to do connection update now.");
        return;
    }

    APP_PRINT_INFO2("ble_start_conn_update id %d, mask 0x%x", p_dev->le_conn_id,
                    p_conn->conn_mask);

    // TODO(armansito): The return value of this call wasn't being used but the
    // logic of this function might be depending on its side effects. We should
    // verify if this call is needed at all and remove it otherwise.


    if ((p_conn->conn_mask & BLE_UPDATE_WAIT_RSP) ||
        (p_conn->conn_mask & BLE_UPDATE_DISABLE)  ||
        (p_conn->conn_mask & BLE_UPDATE_PEND))
    {
        return;
    }

    ret = ble_conn_pend_params_need_update(p_dev);
    if (ret == false)
    {
        APP_PRINT_INFO0("ble_start_conn_update: no need update");
        return;
    }

    p_conn->param_update_pend = false;

    cause = le_update_conn_param(p_dev->le_conn_id,
                                 p_conn->ci_min_pend, p_conn->ci_max_pend,
                                 p_conn->latency_pend, p_conn->timeout_pend,
                                 p_conn->ce_min_pend, p_conn->ce_max_pend);
    if (cause == GAP_CAUSE_SUCCESS)
    {
        if (p_dev->role == GAP_LINK_ROLE_MASTER)
        {
            p_conn->conn_mask |= BLE_UPDATE_WAIT_RSP;
        }
        if ((p_conn->ce_min_pend == CONN_CE_MIN) && (p_conn->ce_max_pend == CONN_CE_MIN))
        {
            p_conn->ce_2slot_used = true;
        }
    }
    else
    {
        APP_PRINT_ERROR1("ble_start_conn_update update default param fail cause 0x%x", cause);
        return;
    }
}

bool bt_dev_enable_conn_update(uint8_t *addr)
{
    T_BT_DEVICE *p_dev = ual_find_device_by_addr(addr);
    if (p_dev == NULL)
    {
        APP_PRINT_ERROR1("bt_dev_enable_conn_update unknow %s", TRACE_BDADDR(addr));
        return false;
    }
    APP_PRINT_INFO2("bt_dev_enable_conn_update conn_id %d conn_mask 0x%x",
                    p_dev->le_conn_id, p_dev->ble_conn_param.conn_mask);
    if ((p_dev->ble_conn_param.conn_mask & BLE_UPDATE_DISABLE) == 0)
    {
        return true;
    }
    p_dev->ble_conn_param.conn_mask &= ~BLE_UPDATE_DISABLE;

    ble_start_conn_update(p_dev);

    return true;
}

bool bt_dev_disable_conn_update(uint8_t *addr)
{
    T_BT_DEVICE *p_dev = ual_find_device_by_addr(addr);
    if (p_dev == NULL)
    {
        APP_PRINT_ERROR1("bt_dev_disable_conn_update unknow %s", TRACE_BDADDR(addr));
        return false;
    }
    APP_PRINT_INFO2("bt_dev_disable_conn_update conn_id %d conn_mask 0x%x",
                    p_dev->le_conn_id, p_dev->ble_conn_param.conn_mask);
    if (p_dev->ble_conn_param.conn_mask & BLE_UPDATE_DISABLE)
    {
        return true;
    }
    p_dev->ble_conn_param.conn_mask |= BLE_UPDATE_DISABLE;

    ble_start_conn_update(p_dev);

    return true;
}

void ble_conn_mgr_register_app_cb(T_UAL_CONN_MGR_APP_CBACK cback)
{
    conn_mgr_app_callback = cback;
}

bool bt_dev_handle_conn_update_event(uint8_t *bd_addr, T_UPDATE_CI_EVENT event)
{
    uint16_t ci_min, ci_max, ce_min, ce_max;
    T_BT_DEVICE *p_dev = ual_find_device_by_addr(bd_addr);

    if (p_dev == NULL)
    {
        APP_PRINT_ERROR1("bt_dev_handle_conn_update_event: unkown addr %s", TRACE_BDADDR(bd_addr));
        return false;
    }

    APP_PRINT_INFO2("bt_dev_handle_conn_update_event: conn_id %d, event 0x%x",
                    p_dev->le_conn_id, event);

    //always assume it is allow to do connection update
    do_conn_update = true;

    switch (event)
    {
    case CI_EVENT_GATT_DISCOVERY:
        {
            ci_min = BLE_CONN_FAST_CI_DEF;
            ci_max = BLE_CONN_FAST_CI_DEF;
            ce_min = 2 * (ci_min - 1);
            ce_max = 2 * (ci_max - 1);
        }
        break;
    case CI_EVENT_GATT_DONE:
        {
            ci_min = BLE_CONN_NORMAL_CI_DEF;
            ci_max = BLE_CONN_NORMAL_CI_DEF;
            if (ble_is_gaming_mode())
            {
                ci_min = BLE_CONN_GAMING_CI_DEF;
                ci_max = BLE_CONN_GAMING_CI_DEF;
            }
            ce_min = 2 * (ci_min - 1);
            ce_max = 2 * (ci_max - 1);
        }
        break;
    case CI_EVENT_DISCV_ALL_DONE:
    case CI_EVENT_CIS_DISCONNECT:
        {
            ci_min = BLE_CONN_NORMAL_CI_DEF;
            ci_max = BLE_CONN_NORMAL_CI_DEF;
            if (ble_is_gaming_mode())
            {
                ci_min = BLE_CONN_GAMING_CI_DEF;
                ci_max = BLE_CONN_GAMING_CI_DEF;
            }
            ce_min = 2 * (ci_min - 1);
            ce_max = 2 * (ci_max - 1);
        }
        break;
    case CI_EVENT_CIS_ESTABLISH:
        {
            ci_min = BLE_CONN_NORMAL_CI_DEF;
            ci_max = BLE_CONN_NORMAL_CI_DEF;
            if (ble_is_gaming_mode())
            {
                ci_min = BLE_CONN_GAMING_CI_DEF;
                ci_max = BLE_CONN_GAMING_CI_DEF;
            }
            ce_min = CONN_CE_MIN;
            ce_max = CONN_CE_MIN;
        }
        break;

    default:
        break;

    }

    if (conn_mgr_app_callback)
    {
        do_conn_update = conn_mgr_app_callback(event);
    }

    return bt_dev_update_conn_params(p_dev->bd_addr, ci_min, ci_max,
                                     BLE_CONN_SLAVE_LATENCY_DEF, BLE_CONN_TIMEOUT_DEF,
                                     ce_min, ce_max);
}

static bool bt_dev_check_conn_params(uint16_t ci_min, uint16_t ci_max,
                                     uint16_t latency, uint16_t timeout,
                                     uint16_t ce_min, uint16_t ce_max)
{
    if (ci_min < BLE_CONN_INT_MIN || ci_min > BLE_CONN_INT_MAX)
    {
        return false;
    }

    if (ci_max < BLE_CONN_INT_MIN || ci_max > BLE_CONN_INT_MAX)
    {
        return false;
    }

    if (ci_min > ci_max)
    {
        return false;
    }

    if (latency > BLE_CONN_LATENCY_MAX)
    {
        return false;
    }

    if (timeout < BLE_CONN_SUP_TOUT_MIN || timeout > BLE_CONN_SUP_TOUT_MAX)
    {
        return false;
    }

    if (ce_min > ce_max)
    {
        return false;
    }

    return true;
}

bool bt_dev_update_conn_params(uint8_t *bd_addr, uint16_t ci_min, uint16_t ci_max,
                               uint16_t latency, uint16_t timeout, uint16_t ce_min, uint16_t ce_max)
{
    T_BT_DEVICE *p_dev = ual_find_device_by_addr(bd_addr);
    if (p_dev == NULL)
    {
        APP_PRINT_ERROR1("bt_dev_update_conn_params: unkown addr %s", TRACE_BDADDR(bd_addr));
        return false;
    }
    T_BLE_CONN_PARAM *p_conn = &(p_dev->ble_conn_param);
    if (!p_conn)
    {
        APP_PRINT_ERROR0("bt_dev_update_conn_params: ble_conn_param NULL");
        return false;
    }
    APP_PRINT_TRACE7("bt_dev_update_conn_params conn_id %d ci_min 0x%x ci_max 0x%x latency 0x%x timeout 0x%x, ce_min 0x%x,ce_max 0x%x",
                     p_dev->le_conn_id, ci_min, ci_max, latency, timeout, ce_min, ce_max);

    if (!bt_dev_check_conn_params(ci_min, ci_max, latency, timeout, ce_min, ce_max))
    {
        APP_PRINT_ERROR0("bt_dev_update_conn_params: check conn_param fail");
        return false;
    }

    p_conn->ci_min_pend = ci_min;
    p_conn->ci_max_pend = ci_max;
    p_conn->latency_pend = latency;
    p_conn->timeout_pend = timeout;
    p_conn->ce_min_pend = ce_min;
    p_conn->ce_max_pend = ce_max;

    p_conn->param_update_pend = true;

    ble_start_conn_update(p_dev);
    return true;
}


T_APP_RESULT ble_handle_conn_update_ind(T_LE_CONN_UPDATE_IND *p_conn_ind)
{
    uint16_t ci_min, ci_max, latency, timeout, ce_min, ce_max;
    T_BT_DEVICE *p_dev = ual_find_device_by_conn_id(p_conn_ind->conn_id);
    if (p_dev == NULL)
    {
        APP_PRINT_ERROR1("ble_handle_conn_update_ind unkown conn_id %d", p_conn_ind->conn_id);
        return APP_RESULT_REJECT;
    }

    T_BLE_CONN_PARAM *p_conn = &(p_dev->ble_conn_param);
    if (!p_conn)
    {
        APP_PRINT_ERROR0("ble_handle_conn_update_ind: ble_conn_param NULL");
        return APP_RESULT_REJECT;
    }

    APP_PRINT_TRACE5("ble_handle_conn_update_ind: conn_id 0x%x, min 0x%x, max 0x%x latency 0x%x timeout 0x%x",
                     p_conn_ind->conn_id, p_conn_ind->conn_interval_min, p_conn_ind->conn_interval_max,
                     p_conn_ind->conn_latency, p_conn_ind->supervision_timeout);

    ci_min = p_conn_ind->conn_interval_min;
    ci_max = p_conn_ind->conn_interval_max;
    latency = p_conn_ind->conn_latency;
    timeout = p_conn_ind->supervision_timeout;
    ce_min = p_conn->ce_min;
    ce_max = p_conn->ce_max;

    APP_PRINT_TRACE2("ble_handle_conn_update_ind, mask %x role %x",
                     p_dev->ble_conn_param.conn_mask,
                     p_dev->role);
    if (p_dev->role != GAP_LINK_ROLE_MASTER)
    {
        return APP_RESULT_REJECT;
    }
    if (!bt_dev_check_conn_params(ci_min, ci_max, latency, timeout, ce_min, ce_max))
    {
        return APP_RESULT_REJECT;
    }

    p_conn->ci_min_pend = ci_min;
    p_conn->ci_max_pend = ci_max;
    p_conn->latency_pend = latency;
    p_conn->timeout_pend = timeout;
    p_conn->ce_min_pend = ce_min;
    p_conn->ce_max_pend = ce_max;

    p_conn->param_update_pend = true;

    //FIX TODO reject here but still record param, update when next time enable update
    if ((p_conn->conn_mask & BLE_UPDATE_DISABLE) ||
        (p_conn->conn_mask & BLE_UPDATE_WAIT_RSP) ||
        (p_conn->conn_mask & BLE_UPDATE_PEND))
    {
        return APP_RESULT_REJECT;
    }
    else
    {
        ble_start_conn_update(p_dev);
        return APP_RESULT_SUCCESS;
    }
}


void ble_handle_conn_update_event(T_GAP_CONN_PARAM_UPDATE update_info)
{
    APP_PRINT_TRACE3("ble_handle_conn_update_event: conn_id 0x%x, status %d, cause 0x%04x",
                     update_info.conn_id, update_info.status, update_info.cause);
    T_BT_DEVICE *p_dev = ual_find_device_by_conn_id(update_info.conn_id);
    if (p_dev == NULL)
    {
        APP_PRINT_ERROR1("ble_handle_conn_update_event: unkown conn_id 0x%x", update_info.conn_id);
        return;
    }

    T_BLE_CONN_PARAM *p_conn = &(p_dev->ble_conn_param);
    if (!p_conn)
    {
        APP_PRINT_ERROR0("ble_handle_conn_update_event: ble_conn_param NULL");
        return;
    }

    switch (update_info.status)
    {
    case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS:
        {
            uint16_t conn_interval;

            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, update_info.conn_id);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &p_conn->latency, update_info.conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &p_conn->timeout, update_info.conn_id);

            p_conn->ci_min = conn_interval;
            p_conn->ci_max = conn_interval;
            if (p_conn->ce_2slot_used)
            {
                p_conn->ce_2slot_used = false;
                p_conn->ce_min = CONN_CE_MIN;
                p_conn->ce_max = CONN_CE_MIN;
            }
            else
            {
                p_conn->ce_min = 2 * (conn_interval - 1);
                p_conn->ce_max = 2 * (conn_interval - 1);
            }

            /*FIXME: where to get ce param*/
            APP_PRINT_INFO4("ble_handle_conn_update_event update success:conn_id 0x%x, conn_interval 0x%04x, conn_slave_latency 0x%04x, conn_supervision_timeout 0x%04x",
                            update_info.conn_id, conn_interval, p_dev->ble_conn_param.latency,
                            p_dev->ble_conn_param.timeout);
            p_conn->conn_mask &= ~BLE_UPDATE_WAIT_RSP;
            p_conn->conn_mask &= ~BLE_UPDATE_PEND;
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            p_conn->conn_mask &= ~BLE_UPDATE_WAIT_RSP;
            p_conn->conn_mask &= ~BLE_UPDATE_PEND;
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
        {
            p_conn->conn_mask |= BLE_UPDATE_PEND;
        }
        break;
    default:
        break;
    }

    ble_start_conn_update(p_dev);
}

T_APP_RESULT ble_gatt_client_discover_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                                         void *p_data)
{
    uint8_t conn_id;
    T_BT_DEVICE *p_sec_dev = NULL;

    le_get_conn_id_by_handle(conn_handle, &conn_id);
    p_sec_dev = ual_find_device_by_conn_id(conn_id);
    if (p_sec_dev == NULL)
    {
        APP_PRINT_ERROR1("gatt_client_discover_cb: conn_handle 0x%x not find device", conn_handle);
        return APP_RESULT_APP_ERR;
    }

    if (type == GATT_CLIENT_EVENT_DIS_ALL_STATE)
    {
        T_GATT_CLIENT_DIS_ALL_DONE *p_disc = (T_GATT_CLIENT_DIS_ALL_DONE *)p_data;
        APP_PRINT_INFO2("gatt_client_discover_cb:state 0x%x, load_from_ftl %d",
                        p_disc->state, p_disc->load_from_ftl);

        if (p_disc->state == GATT_CLIENT_STATE_DISCOVERY)
        {
#if (LE_AUDIO_CONNECT_FASTER == 0)
            bt_dev_handle_conn_update_event(p_sec_dev->bd_addr, CI_EVENT_GATT_DISCOVERY);
            bt_dev_disable_conn_update(p_sec_dev->bd_addr);
#endif
        }
        else if (p_disc->state == GATT_CLIENT_STATE_DONE &&
                 !p_disc->load_from_ftl)
        {
#if (LE_AUDIO_CONNECT_FASTER == 0)
            bt_dev_handle_conn_update_event(p_sec_dev->bd_addr, CI_EVENT_GATT_DONE);
            bt_dev_enable_conn_update(p_sec_dev->bd_addr);
#endif
        }
    }
    return APP_RESULT_SUCCESS;
}


static T_APP_RESULT gap_client_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    uint8_t conn_id;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;
    T_BT_DEVICE *p_sec_dev;
    le_get_conn_id_by_handle(conn_handle, &conn_id);
    p_sec_dev = ual_find_device_by_conn_id(conn_id);

    if (p_sec_dev == NULL)
    {
        APP_PRINT_ERROR1("gap_client_cb: conn_handle 0x%x not find device", conn_handle);
        return APP_RESULT_APP_ERR;
    }
    APP_PRINT_INFO1("gap_client_cb: type 0x%x", type);

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            if (p_client_cb_data->dis_done.is_found)
            {
                if (p_sec_dev->role == GAP_LINK_ROLE_MASTER)
                {
                    T_ATTR_UUID srv_uuid;
                    T_ATTR_UUID char_uuid;
                    uint16_t handle = 0;

                    srv_uuid.is_uuid16 = true;
                    srv_uuid.instance_id = 0;
                    srv_uuid.p.uuid16 = UUID_GAP;
                    char_uuid.is_uuid16 = true;
                    char_uuid.instance_id = 0;
                    char_uuid.p.uuid16 = GATT_UUID_CHAR_PER_PREF_CONN_PARAM;
                    if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
                    {
                        gatt_client_read(conn_handle, handle, NULL);
                    }
                }
            }
        }
        break;

    case GATT_CLIENT_EVENT_CCCD_CFG:

        break;

    case GATT_CLIENT_EVENT_READ_RESULT:
        {
            if (p_sec_dev->role == GAP_LINK_ROLE_MASTER)
            {
                if (p_client_cb_data->read_result.cause == GAP_SUCCESS &&
                    p_client_cb_data->read_result.char_uuid.p.uuid16 == GATT_UUID_CHAR_PER_PREF_CONN_PARAM &&
                    p_client_cb_data->read_result.value_size == 8)
                {
                    uint8_t *pp = p_client_cb_data->read_result.p_value;
                    uint16_t ci_min, ci_max, latency, timeout, ce_min, ce_max;
                    LE_STREAM_TO_UINT16(ci_min, pp);
                    LE_STREAM_TO_UINT16(ci_max, pp);
                    LE_STREAM_TO_UINT16(latency, pp);
                    LE_STREAM_TO_UINT16(timeout, pp);
                    APP_PRINT_INFO4("gap_client_cb: ci_min %d ci_max %d latency %d timeout %d",
                                    ci_min, ci_max, latency, timeout);
                    ce_min = 2 * (ci_min - 1);
                    ce_max = 2 * (ci_max - 1);
                    bt_dev_update_conn_params(p_sec_dev->bd_addr, ci_min, ci_max,
                                              latency, timeout, ce_min, ce_max);
                }
            }
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:

        break;

    case GATT_CLIENT_EVENT_WRITE_RESULT:

        break;

    default:
        break;
    }

    return result;
}

bool ble_register_conn_callback_by_id(T_APP_ID app_id, T_LE_CONN_CBACK callback)
{
    T_APP_CONN_CBACK *p_conn_callback;
    p_conn_callback = conn_mgr_find_app_callback(app_id);
    if (p_conn_callback)
    {
        APP_PRINT_ERROR1("ble_register_conn_callback_by_id already register callback %d", app_id);
        return false;
    }

    p_conn_callback = os_mem_zalloc(RAM_TYPE_BT_UAL, sizeof(T_APP_CONN_CBACK));
    if (p_conn_callback)
    {
        p_conn_callback->callback = callback;
        p_conn_callback->app_id = app_id;
        init_ualist_head(&p_conn_callback->list);
        ualist_add_tail(&p_conn_callback->list, &conn_mgr.app_conn_cback);
        return true;
    }
    return false;
}

void ble_unregister_conn_callback_by_id(T_APP_ID app_id)
{
    T_APP_CONN_CBACK *p_conn_callback = conn_mgr_find_app_callback(app_id);

    if (p_conn_callback)
    {
        ualist_del(&p_conn_callback->list);
        os_mem_free(p_conn_callback);
    }
}

void ble_conn_update_by_app(bool enable)
{
    conn_update_by_app = enable;
}

void ble_conn_mgr_init()
{
    T_ATTR_UUID srv_uuid;

    memset(&conn_mgr, 0, sizeof(conn_mgr));
    init_ualist_head(&conn_mgr.conn_devs_set);
    init_ualist_head(&conn_mgr.app_conn_cback);

    ble_direct_conn_init();
    ble_bg_conn_init();

    srv_uuid.is_uuid16 = true;
    srv_uuid.p.uuid16 = UUID_GAP;
    gatt_client_spec_register(&srv_uuid, gap_client_cb);
}

