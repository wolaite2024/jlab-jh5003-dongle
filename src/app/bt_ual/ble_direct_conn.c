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
#include "ble_direct_conn.h"
#include "adapter_int.h"
#include "ble_privacy.h"
#include "app_timer.h"

#define DIRECT_CONN_TIMEOUT   (1000)
typedef enum
{
    TIMER_ID_DIRECT_CONN = 0x01,
} T_DIRECT_TIMER_ID;


typedef struct
{
    T_UALIST_HEAD     list;
    uint8_t           bd_addr[6];
    uint8_t           bd_type;
    bool              need_conn;
} T_DIRECT_DEV;

typedef struct
{
    T_UALIST_HEAD   direct_devs_list;
    uint8_t         timer_id;
    uint8_t         timer_idx_conn;
    T_DIRECT_DEV    *p_op_dev;
    bool            game_mode;
} T_DIRECT_MGR;


static T_DIRECT_MGR direct_mgr;

static T_DIRECT_DEV *alloc_direct_node_by_addr(uint8_t *bd_addr, uint8_t bd_type);
static T_DIRECT_DEV *find_direct_node_by_addr(uint8_t *bd_addr, uint8_t bd_type);
static bool ble_direct_create_conn(uint8_t *bd_addr, uint8_t bd_type);
static void ble_direct_conn_timer_start(void);
static void ble_direct_conn_timer_stop(void);


static T_DIRECT_DEV *alloc_direct_node_by_addr(uint8_t *bd_addr, uint8_t bd_type)
{
    T_DIRECT_DEV *p_dev;

    p_dev = os_mem_zalloc(RAM_TYPE_BT_UAL, sizeof(T_DIRECT_DEV));
    if (p_dev)
    {
        memcpy(p_dev->bd_addr, bd_addr, BD_ADDR_LEN);
        p_dev->bd_type = bd_type;
        init_ualist_head(&p_dev->list);
        ualist_add_tail(&p_dev->list, &direct_mgr.direct_devs_list);
    }
    return p_dev;
}

static T_DIRECT_DEV *find_direct_node_by_addr(uint8_t *bd_addr, uint8_t bd_type)
{
    T_UALIST_HEAD *pos, *n;
    T_DIRECT_DEV *p_dev;

    ualist_for_each_safe(pos, n, &direct_mgr.direct_devs_list)
    {
        p_dev = ualist_entry(pos, T_DIRECT_DEV, list);
        if (!memcmp(p_dev->bd_addr, bd_addr, BD_ADDR_LEN) &&
            (p_dev->bd_type == bd_type))
        {
            return p_dev;
        }
    }
    return NULL;
}

static void ble_direct_conn_timer_start(void)
{
    app_start_timer(&direct_mgr.timer_idx_conn,
                    "direct_conn",
                    direct_mgr.timer_id,
                    TIMER_ID_DIRECT_CONN,
                    0,
                    false,
                    DIRECT_CONN_TIMEOUT);
}

static void ble_direct_conn_timer_stop(void)
{
    app_stop_timer(&direct_mgr.timer_idx_conn);
}

static uint8_t direct_get_need_conn_num(void)
{
    T_UALIST_HEAD *pos, *n;
    T_DIRECT_DEV *p_dev;
    uint8_t num = 0;
    ualist_for_each_safe(pos, n, &direct_mgr.direct_devs_list)
    {
        p_dev = ualist_entry(pos, T_DIRECT_DEV, list);
        if (!p_dev)
        {
            continue;
        }

        if (!p_dev->need_conn)
        {
            continue;
        }

        num++;
    }
    return num;
}

static void direct_check_conn_list(void)
{
    T_UALIST_HEAD *pos, *n;
    T_DIRECT_DEV *p_dev;
    ualist_for_each_safe(pos, n, &direct_mgr.direct_devs_list)
    {
        p_dev = ualist_entry(pos, T_DIRECT_DEV, list);
        if (!p_dev)
        {
            continue;
        }

        if (!p_dev->need_conn)
        {
            continue;
        }

        if (direct_mgr.p_op_dev != NULL)
        {
            if (!memcmp(direct_mgr.p_op_dev->bd_addr, p_dev->bd_addr, 6) &&
                (direct_mgr.p_op_dev->bd_type == p_dev->bd_type))
            {
                continue;
            }
        }

        if (ble_direct_create_conn(p_dev->bd_addr, p_dev->bd_type))
        {
            ble_direct_conn_timer_start();
            direct_mgr.p_op_dev = p_dev;
            break;
        }
    }
}

static void direct_handle_conn_timeout(void)
{
    /* only one device need conn, just wait */
    if (direct_get_need_conn_num() == 1)
    {
        ble_direct_conn_timer_start();
    }
    else
    {
        if (direct_mgr.p_op_dev)
        {
            ble_direct_disconnect(direct_mgr.p_op_dev->bd_addr, direct_mgr.p_op_dev->bd_type);
        }
        direct_check_conn_list();
    }
}

static void direct_conn_timeout_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_INFO2("direct_conn_timeout_cback: id %x chann %x", timer_id, timer_chann);
    switch (timer_id)
    {
    case TIMER_ID_DIRECT_CONN:
        {
            ble_direct_conn_timer_stop();
            direct_handle_conn_timeout();
        }
        break;
    default:
        break;
    }
}

static bool ble_direct_create_conn(uint8_t *bd_addr, uint8_t bd_type)
{
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    //FIX TODO
    T_GAP_LOCAL_ADDR_TYPE local_bd_type = (T_GAP_LOCAL_ADDR_TYPE)ble_get_local_bd_type();
    T_GAP_REMOTE_ADDR_TYPE peer_bd_type = (T_GAP_REMOTE_ADDR_TYPE)bd_type;
    uint8_t  init_phys = GAP_PHYS_CONN_INIT_2M_BIT | GAP_PHYS_CONN_INIT_1M_BIT;
    conn_req_param.scan_interval = BLE_SCAN_SLOW_INT_1;
    conn_req_param.scan_window = BLE_SCAN_SLOW_WIN_1;
#if  (LE_AUDIO_CONNECT_FASTER == 1)
    //set ci as 7.5 to speed up the connection
    conn_req_param.conn_interval_min = BLE_CONN_FAST_CI_DEF;
    conn_req_param.conn_interval_max = BLE_CONN_FAST_CI_DEF;
#else
    conn_req_param.conn_interval_min = BLE_CONN_NORMAL_CI_DEF;
    conn_req_param.conn_interval_max = BLE_CONN_NORMAL_CI_DEF;
    if (direct_mgr.game_mode)
    {
        conn_req_param.conn_interval_min = BLE_CONN_GAMING_CI_DEF;
        conn_req_param.conn_interval_max = BLE_CONN_GAMING_CI_DEF;
    }
#endif
    conn_req_param.conn_latency = BLE_CONN_SLAVE_LATENCY_DEF;
    conn_req_param.supv_tout = BLE_CONN_TIMEOUT_DEF;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
    le_set_conn_param(GAP_CONN_PARAM_2M, &conn_req_param);

#if BLE_PRIVACY_SPT
    if (ble_resolving_list_is_enable())
    {
        local_bd_type |= BLE_BD_TYPE_ID_BIT;
        peer_bd_type |= BLE_BD_TYPE_ID_BIT;
    }
#endif

    if (le_connect(init_phys, bd_addr, peer_bd_type, local_bd_type, 0) != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR0("ble_direct_create_conn: le_connect fail");
        return false;
    }

    return true;
}

bool ble_direct_connect(uint8_t *bd_addr, uint8_t bd_type)
{
    T_DIRECT_DEV *p_dev;
    p_dev = find_direct_node_by_addr(bd_addr, bd_type);
    if (p_dev == NULL)
    {
        p_dev = alloc_direct_node_by_addr(bd_addr, bd_type);
        if (p_dev == NULL)
        {
            APP_PRINT_ERROR0("ble_direct_connect: alloc fail");
            return false;
        }
    }

    p_dev->need_conn = true;
    if (direct_mgr.timer_idx_conn)
    {
        return true;
    }

    if (ble_direct_create_conn(bd_addr, bd_type))
    {
        ble_direct_conn_timer_start();
        direct_mgr.p_op_dev = p_dev;
        return true;
    }

    return false;
}


bool ble_direct_disconnect(uint8_t *bd_addr, uint8_t bd_type)
{
    T_BT_DEVICE *p_dev = ual_find_device_by_addr(bd_addr);
    uint8_t conn_id;

    if (p_dev == NULL)
    {
        APP_PRINT_ERROR0("ble_direct_disconnect: can't find device record");
        return false;
    }

    ble_direct_conn_timer_stop();
    if (p_dev->ble_state == GAP_CONN_STATE_DISCONNECTING)
    {
        return true;
    }

    if (le_get_conn_id_by_handle(p_dev->le_conn_handle, &conn_id))
    {
        le_disconnect(conn_id);
    }

    return true;
}



void ble_direct_handle_connected(uint8_t *bd_addr, uint8_t bd_type)
{
    T_DIRECT_DEV *p_dev;
    APP_PRINT_INFO2("ble_direct_handle_connected: %s %d", TRACE_BDADDR(bd_addr), bd_type);
    p_dev = find_direct_node_by_addr(bd_addr, bd_type);
    if (p_dev == NULL)
    {
        APP_PRINT_INFO2("ble_direct_handle_connected: canot find %s %d", TRACE_BDADDR(bd_addr), bd_type);
        return;
    }

    if (direct_mgr.p_op_dev == p_dev)
    {
        direct_mgr.p_op_dev = NULL;
    }

    ble_direct_conn_timer_stop();
    ualist_del(&p_dev->list);
    os_mem_free(p_dev);

    direct_check_conn_list();
}

void ble_direct_handle_disconnected(uint8_t *bd_addr, uint8_t bd_type)
{

    T_DIRECT_DEV *p_dev;
    p_dev = find_direct_node_by_addr(bd_addr, bd_type);
    if (p_dev == NULL)
    {
        return;
    }
    ble_direct_conn_timer_stop();
    ualist_del(&p_dev->list);
    os_mem_free(p_dev);
}

void ble_direct_set_mode(bool enable)
{
    direct_mgr.game_mode = enable;
}

void ble_direct_conn_init(void)
{
    memset(&direct_mgr, 0, sizeof(direct_mgr));
    init_ualist_head(&direct_mgr.direct_devs_list);
    app_timer_reg_cb(direct_conn_timeout_cback, &direct_mgr.timer_id);
}
