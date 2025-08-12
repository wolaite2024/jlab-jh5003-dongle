/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <string.h>
#include <os_mem.h>
#include <stdlib.h>
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
#include "ble_bg_conn.h"
#include "adapter_int.h"
#include "ble_privacy.h"
#include "gap_vendor.h"

#define WL_ATTEMPT_COUNT            3


typedef enum
{
    BG_OP_NULL,
    BG_OP_ADD,
    BG_OP_REMOVE,
} T_BG_OPCODE;

typedef struct bgconn_dev
{
    T_UALIST_HEAD                 list;
    uint8_t                       bd_addr[6];
    T_GAP_REMOTE_ADDR_TYPE        bd_type;
    bool                          need_conn;
    T_BG_OPCODE                   opcode;
    T_BG_OPCODE                   next_opcode;
} T_BG_DEV;


typedef struct
{
    T_UALIST_HEAD   wl_list;
    uint8_t         conn_id;
    uint8_t         max_wl_size;
    uint8_t         wl_num;
    uint16_t        scan_int;
    uint16_t        scan_win;
    uint8_t         conn_state;
    bool            suspend;
    T_BG_DEV       *p_op_dev;
    bool            game_mode;
} T_BG_MGR;

static const uint8_t null_bdaddr[6] = {0, 0, 0, 0, 0, 0};

static T_BG_MGR bg_mgr;

static bool ble_src_have_remote_key(uint8_t *bd_addr);
static void ble_src_send_rpa_cmd_to_lowerstack(bool enable);
static T_BG_DEV *ble_bg_alloc_dev(uint8_t *bd_addr, uint8_t bd_type);
static void ble_bg_free_all_dev(void);
static void ble_bg_create_conn(void);
static bool ble_bg_wl_need_conn(void);
static T_BG_DEV *ble_bg_find_dev_by_addr(uint8_t *bd_addr, T_BLE_BD_TYPE bd_type);
static bool ble_bg_whitelist_add(uint8_t *bd_addr, uint8_t bd_type);
static bool ble_bg_whitelist_remove(uint8_t *bd_addr, uint8_t bd_type);
static bool ble_bg_execute_op(void);

static bool ble_src_have_remote_key(uint8_t *bd_addr)
{
    T_BT_DEVICE *p_dev = NULL;

    if (!bd_addr)
    {
        return false;
    }

    p_dev = ual_find_device_by_addr(bd_addr);
    if (!p_dev)
    {
        return false;
    }

    return p_dev->has_irk ? true : false;
}

static void ble_src_send_rpa_cmd_to_lowerstack(bool enable)
{
    uint8_t params[2] = {0};

    params[0] = 0x1e;
    params[1] = enable; //0: disable/ 1: enable

    APP_PRINT_TRACE1("src_send_rpa_cmd_to_lowerstack: %d", enable);

    gap_vendor_cmd_req(0xfd80, sizeof(params), params);
}

static void ble_bg_cancel_conn(void)
{
    APP_PRINT_INFO1("ble_bg_cancel_conn: conn_id %d", bg_mgr.conn_id);
    T_GAP_CONN_INFO conn_info;

    if (!le_get_conn_info(bg_mgr.conn_id, &conn_info))
    {
        bg_mgr.conn_id = 0xFF;
        APP_PRINT_INFO1("ble_bg_cancel_conn: conn_id %d get info fail", bg_mgr.conn_id);
        return;
    }

    if (conn_info.conn_state == GAP_CONN_STATE_CONNECTING)
    {
        le_disconnect(bg_mgr.conn_id);
    }
}

static void ble_bg_create_conn(void)
{
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    T_GAP_LOCAL_ADDR_TYPE local_bd_type = (T_GAP_LOCAL_ADDR_TYPE)ble_get_local_bd_type();
    T_GAP_REMOTE_ADDR_TYPE peer_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  init_phys = GAP_PHYS_CONN_INIT_2M_BIT | GAP_PHYS_CONN_INIT_1M_BIT;
    conn_req_param.scan_interval = (bg_mgr.scan_int == BLE_SCAN_PARAM_UNDEF)
                                   ? BLE_SCAN_SLOW_INT_1 : bg_mgr.scan_int;
    conn_req_param.scan_window = (bg_mgr.scan_win == BLE_SCAN_PARAM_UNDEF)
                                 ? BLE_SCAN_SLOW_WIN_1 : bg_mgr.scan_win;;
#if  (LE_AUDIO_CONNECT_FASTER == 1)
    //set ci as 7.5 to speed up the connection
    conn_req_param.conn_interval_min = BLE_CONN_FAST_CI_DEF;
    conn_req_param.conn_interval_max = BLE_CONN_FAST_CI_DEF;
#else
    conn_req_param.conn_interval_min = BLE_CONN_NORMAL_CI_DEF;
    conn_req_param.conn_interval_max = BLE_CONN_NORMAL_CI_DEF;
    if (ble_is_gaming_mode())
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

    if (le_connect(init_phys, (uint8_t *)null_bdaddr, peer_bd_type, local_bd_type,
                   0) == GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR0("ble_create_conn fail, set state to suspend");
    }

}

static T_BG_DEV *ble_bg_alloc_dev(uint8_t *bd_addr, uint8_t bd_type)
{
    if (!bd_addr)
    {
        return NULL;
    }

    T_BG_DEV *p_dev = NULL;

    p_dev = calloc(1, sizeof(T_BG_DEV));
    if (p_dev == NULL)
    {
        APP_PRINT_ERROR0("ble_bg_alloc_dev: calloc fail");
        return NULL;
    }

    memcpy(p_dev->bd_addr, bd_addr, BD_ADDR_LEN);
    p_dev->bd_type = (T_GAP_REMOTE_ADDR_TYPE)bd_type;
    init_ualist_head(&p_dev->list);
    ualist_add_tail(&p_dev->list, &bg_mgr.wl_list);
    return p_dev;
}

static void ble_bg_free_all_dev(void)
{
    T_BG_DEV *p_dev = NULL;
    T_UALIST_HEAD *pos, *n;

    ualist_for_each_safe(pos, n, &bg_mgr.wl_list)
    {
        p_dev = ualist_entry(pos, T_BG_DEV, list);
        ualist_del(&(p_dev->list));
        free(p_dev);
    }
}

static bool ble_bg_wl_need_conn(void)
{
    T_BG_DEV *p_dev = NULL;
    T_UALIST_HEAD *pos, *n;

    ualist_for_each_safe(pos, n, &bg_mgr.wl_list)
    {
        p_dev = ualist_entry(pos, T_BG_DEV, list);
        if (p_dev->need_conn)
        {
            return true;
        }
    }
    return false;
}

static T_BG_DEV *ble_bg_find_dev_by_addr(uint8_t *bd_addr, T_BLE_BD_TYPE bd_type)
{
    T_BG_DEV *p_dev = NULL;
    T_UALIST_HEAD *pos, *n;
    if (!bd_addr)
    {
        return NULL;
    }

    ualist_for_each_safe(pos, n, &bg_mgr.wl_list)
    {
        p_dev = ualist_entry(pos, T_BG_DEV, list);
        if (!memcmp(p_dev->bd_addr, bd_addr, 6) &&
            (p_dev->bd_type == bd_type))
        {
            return p_dev;
        }
    }

    return NULL;
}

static bool ble_bg_whitelist_add(uint8_t *bd_addr, uint8_t bd_type)
{
    T_BG_DEV *p_dev = NULL;
    T_BG_DEV *p_op_dev = bg_mgr.p_op_dev;

    if (p_op_dev)
    {
        if (!memcmp(p_op_dev->bd_addr, bd_addr, 6) &&
            (p_op_dev->bd_type == bd_type) &&
            (p_op_dev->opcode == BG_OP_ADD))
        {
            APP_PRINT_INFO2("ble_bg_whitelist_add: bd_addr %s %d is adding", TRACE_BDADDR(bd_addr), bd_type);
            return true;
        }
    }

    p_dev = ble_bg_find_dev_by_addr(bd_addr, bd_type);
    if (!p_dev)
    {
        p_dev = ble_bg_alloc_dev(bd_addr, bd_type);
        if (!p_dev)
        {
            return false;
        }
    }

    p_dev->opcode = BG_OP_NULL;
    p_dev->next_opcode = BG_OP_ADD;
    p_dev->need_conn = true;

    ble_bg_execute_op();

    return true;
}

static bool ble_bg_whitelist_remove(uint8_t *bd_addr, uint8_t bd_type)
{
    T_BG_DEV *p_dev = NULL;
    T_BG_DEV *p_op_dev = bg_mgr.p_op_dev;

    if (p_op_dev)
    {
        if (!memcmp(p_op_dev->bd_addr, bd_addr, 6) &&
            (p_op_dev->bd_type == bd_type) &&
            (p_op_dev->opcode == BG_OP_REMOVE))
        {
            APP_PRINT_INFO2("ble_bg_whitelist_remove: bd_addr %s %d is removing", TRACE_BDADDR(bd_addr),
                            bd_type);
            return true;
        }
    }

    p_dev = ble_bg_find_dev_by_addr(bd_addr, bd_type);
    if (!p_dev)
    {
        return false;
    }

    p_dev->next_opcode = BG_OP_REMOVE;
    ble_bg_execute_op();
    return true;
}

static bool ble_bg_execute_op(void)
{
    T_BG_DEV *p_dev = NULL;
    T_UALIST_HEAD *pos, *n;

    if (bg_mgr.conn_state != GAP_CONN_DEV_STATE_IDLE)
    {
        APP_PRINT_ERROR1("ble_bg_execute_op: dev state %x", bg_mgr.conn_state);
        return false;
    }

    if (bg_mgr.p_op_dev != NULL)
    {
        return false;
    }

    ualist_for_each_safe(pos, n, &bg_mgr.wl_list)
    {
        p_dev = ualist_entry(pos, T_BG_DEV, list);

        if (p_dev->next_opcode == BG_OP_ADD)
        {
            if (le_modify_white_list(GAP_WHITE_LIST_OP_ADD, p_dev->bd_addr,
                                     p_dev->bd_type) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR0("execute_wl_dev_operation remove white list fail");
                continue;
            }
            p_dev->opcode = BG_OP_ADD;
            p_dev->next_opcode = BG_OP_NULL;
            bg_mgr.p_op_dev = p_dev;
        }
        else if (p_dev->next_opcode == BG_OP_REMOVE)
        {
            if (le_modify_white_list(GAP_WHITE_LIST_OP_REMOVE, p_dev->bd_addr,
                                     p_dev->bd_type) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR0("execute_wl_dev_operation remove white list fail");
                continue;
            }
            p_dev->opcode = BG_OP_REMOVE;
            p_dev->next_opcode = BG_OP_NULL;
            bg_mgr.p_op_dev = p_dev;
        }

        if (bg_mgr.p_op_dev)
        {
            return true;
        }
    }

    if (ble_bg_wl_need_conn())
    {
        ble_bg_create_conn();
    }

    return true;
}


void ble_bg_handle_dev_conn_state(uint8_t state)
{
    APP_PRINT_INFO1("ble_bg_handle_dev_conn_state: new_state %d", state);
    bg_mgr.conn_state = state;
    if (state == GAP_CONN_DEV_STATE_IDLE)
    {
        bg_mgr.conn_id = 0xFF;
        ble_bg_execute_op();
    }
    else if (state == GAP_CONN_DEV_STATE_INITIATING)
    {

    }
}

void ble_bg_update_conn_id(uint8_t conn_id)
{
    bg_mgr.conn_id = conn_id;
}

void ble_bg_handle_wl_rsp(T_LE_MODIFY_WHITE_LIST_RSP *p_wl_rsp)
{
    APP_PRINT_INFO2("ble_bg_handle_wl_rsp: cause %x op %x", p_wl_rsp->cause, p_wl_rsp->operation);

    if (bg_mgr.p_op_dev == NULL)
    {
        APP_PRINT_ERROR0("ble_bg_handle_wl_rsp: op_dev NULL");
        ble_bg_execute_op();
        return;
    }

    if (p_wl_rsp->cause == GAP_SUCCESS)
    {
        if (p_wl_rsp->operation == GAP_WHITE_LIST_OP_ADD)
        {
            bg_mgr.wl_num ++;
            bg_mgr.p_op_dev->opcode = BG_OP_NULL;
            bg_mgr.p_op_dev = NULL;
        }
        else if (p_wl_rsp->operation == GAP_WHITE_LIST_OP_REMOVE)
        {
            if (bg_mgr.wl_num)
            {
                bg_mgr.wl_num --;
            }
            bg_mgr.p_op_dev->opcode = BG_OP_NULL;
            ualist_del(&(bg_mgr.p_op_dev->list));
            free(bg_mgr.p_op_dev);
            bg_mgr.p_op_dev = NULL;
        }
        else
        {
            ble_bg_free_all_dev();
            bg_mgr.wl_num = 0;
            bg_mgr.p_op_dev = NULL;
        }
    }

    ble_bg_execute_op();
}

bool ble_bg_connect(uint8_t *bd_addr, uint8_t bd_type)
{
    ble_bg_whitelist_add(bd_addr, bd_type);

    /* FIXME: what if dev have remote key and dev do not have key both in whitelist */
    if (ble_src_have_remote_key(bd_addr))
    {
        ble_src_send_rpa_cmd_to_lowerstack(true);
    }
    else
    {
        ble_src_send_rpa_cmd_to_lowerstack(false);
    }

    if (bg_mgr.conn_state != GAP_CONN_DEV_STATE_IDLE)
    {
        ble_bg_cancel_conn();
    }
    return true;
}

bool ble_bg_disconnect(uint8_t *bd_addr, uint8_t bd_type)
{
    T_BT_DEVICE *p_dev = NULL;
    uint8_t conn_id;

    ble_bg_whitelist_remove(bd_addr, bd_type);
    if (bg_mgr.conn_state != GAP_CONN_DEV_STATE_IDLE)
    {
        ble_bg_cancel_conn();
    }

    p_dev = ual_find_device_by_addr(bd_addr);
    if (p_dev == NULL)
    {
        APP_PRINT_ERROR0("ble_bg_disconnect: can't find device record");
        return false;
    }

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

void ble_bg_set_mode(bool enable)
{
    bg_mgr.game_mode = enable;
}


/* disconnection complete */
void ble_bg_handle_disconnected(uint8_t *bd_addr, uint8_t bd_type)
{
    APP_PRINT_INFO2("ble_bg_handle_disconnected: bd_addr %s, type %d", TRACE_BDADDR(bd_addr), bd_type);
    ble_bg_whitelist_remove(bd_addr, bd_type);
}

void ble_bg_handle_connect(uint8_t *bd_addr, uint8_t bd_type)
{
    APP_PRINT_INFO2("ble_bg_handle_connect: bd_addr %s, type %d", TRACE_BDADDR(bd_addr), bd_type);
    ble_bg_whitelist_remove(bd_addr, bd_type);
}

bool ble_bg_resume(void)
{
    APP_PRINT_TRACE2("ble_bg_resume:  suspend %d, conn_id %d",
                     bg_mgr.suspend, bg_mgr.conn_id);

    if (bg_mgr.suspend == false)
    {
        return true;
    }

    ble_bg_execute_op();

    return true;
}

bool ble_bg_suspend(void)
{
    APP_PRINT_TRACE2("ble_bg_suspend:  suspend %d, conn_id %d",
                     bg_mgr.suspend, bg_mgr.conn_id);
    if (bg_mgr.suspend)
    {
        return true;
    }

    if (bg_mgr.conn_state != GAP_CONN_DEV_STATE_IDLE)
    {
        le_disconnect(bg_mgr.conn_id);
    }
    return true;
}

void ble_bg_conn_init()
{
    memset(&bg_mgr, 0, sizeof(bg_mgr));
    bg_mgr.conn_id = 0xFF;
    bg_mgr.scan_int = BLE_SCAN_FAST_INT;
    bg_mgr.scan_win = BLE_SCAN_FAST_WIN;
    le_get_gap_param(GAP_PARAM_MAX_WL_SIZE, &bg_mgr.max_wl_size);
    init_ualist_head(&bg_mgr.wl_list);
}
