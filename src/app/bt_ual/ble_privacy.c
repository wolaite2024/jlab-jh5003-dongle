/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <trace.h>
#include "ual_types.h"
#include "ual_list.h"
#include "ual_upperstack_cfg.h"
#include "btm.h"
#include "gap_msg.h"
#include "gap_ext_scan.h"
#include "dev_mgr.h"
#include "gap_le.h"
#include "gap_privacy.h"
#include "ble_privacy.h"
#include "scan_mgr.h"
#include "adv_mgr.h"
#include "ble_pa_sync.h"
#include "ble_bg_conn.h"

#if BLE_PRIVACY_SPT
#include <stdint.h>
#include <string.h>
#include <os_mem.h>

#define RL_ATTEMPT_COUNT            3

typedef struct rl_dev
{
    T_UALIST_HEAD                 list;
    uint8_t                            identity_addr[6];
    T_BLE_BD_TYPE                identity_addr_type;
    T_GAP_IDENT_ADDR_TYPE         addr_type;
    bool                          in_controller_rl;
    bool                          pending_removal;
} T_RL_DEV;

typedef struct
{
    uint8_t                     rl_size;
    uint8_t                     resolving_list_avail_size;
    uint8_t                          rl_attempt;
    T_BLE_RL_STATE               rl_state;
    T_BLE_RL_SUSPEND_STATE       suspended_rl_state;
    T_BLE_RL_ACT_STATE           rl_act_state;
    bool                        to_resume;
    T_RL_DEV                     *p_rl_op_dev;
    T_UALIST_HEAD               rl_list;
} T_BLE_PRIVACY_CB;



static T_BLE_PRIVACY_CB ble_privacy_cb;
static void check_rl_pending_action(void);

static void ble_resume_resolving_list_activity()
{
    ble_bg_resume();
    le_scan_resume_req();
    adv_resume_req();
    ble_pa_sync_resume_req();
}

static bool rl_add_device(T_BLE_BD_TYPE identity_bd_type, uint8_t *bd_addr)
{
    bool found = false;
    T_UALIST_HEAD *pos, *n;
    T_RL_DEV *p_rl_dev;

    ualist_for_each_safe(pos, n, &ble_privacy_cb.rl_list)
    {
        p_rl_dev = ualist_entry(pos, T_RL_DEV, list);
        if (memcmp(p_rl_dev->identity_addr, bd_addr, BD_ADDR_LEN) == 0 &&
            p_rl_dev->identity_addr_type == identity_bd_type)
        {
            APP_PRINT_WARN0("rl_add alloc rl device already added");
            found = true;
            break;
        }
    }

    if (!found)
    {
        p_rl_dev = os_mem_zalloc(RAM_TYPE_BT_UAL, sizeof(T_RL_DEV));
        if (p_rl_dev == NULL)
        {
            APP_PRINT_ERROR0("rl_add alloc rl device fail");
            return false;
        }
        memcpy(p_rl_dev->identity_addr, bd_addr, BD_ADDR_LEN);
        p_rl_dev->identity_addr_type = identity_bd_type;
        p_rl_dev->addr_type = (T_GAP_IDENT_ADDR_TYPE)(identity_bd_type & (~BLE_BD_TYPE_ID_BIT));
        p_rl_dev->in_controller_rl = false;
        ualist_add_tail(&p_rl_dev->list, &ble_privacy_cb.rl_list);
        ble_privacy_cb.resolving_list_avail_size--;
    }
    return true;
}

static bool rl_rmv_device(T_BLE_BD_TYPE identity_bd_type, uint8_t *bd_addr)
{
    T_UALIST_HEAD *pos, *n;
    T_RL_DEV *p_rl_dev;

    ualist_for_each_safe(pos, n, &ble_privacy_cb.rl_list)
    {
        p_rl_dev = ualist_entry(pos, T_RL_DEV, list);
        if (memcmp(p_rl_dev->identity_addr, bd_addr, 6) == 0 &&
            p_rl_dev->identity_addr_type == identity_bd_type)
        {

            if (p_rl_dev->in_controller_rl)
            {
                p_rl_dev->pending_removal = true;
                return false;
            }
            else
            {
                ualist_del(pos);
                os_mem_free(p_rl_dev);
                ble_privacy_cb.resolving_list_avail_size++;
                return true;
            }
        }
    }
    return true;
}

static bool check_rl_to_enable_resolving()
{
    //FIX TODO if resolving list has devices enable resolving address always
    if (ualist_empty(&ble_privacy_cb.rl_list))
    {
        if (ble_privacy_cb.rl_state == BLE_RL_DISABLE)
        {
            return false;
        }
        else if (ble_privacy_cb.rl_state == BLE_RL_ENABLE)
        {
            T_GAP_CAUSE cause;
            if ((cause = le_privacy_set_addr_resolution(false)) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR1("check_rl_to_enable_resolving disable address resolution fail cause 0x%x fail",
                                 cause);
                return false;
            }
            else
            {
                ble_privacy_cb.rl_state = BLE_RL_DISABLING;
            }
            return true;
        }
    }
    else
    {
        if (ble_privacy_cb.rl_state == BLE_RL_ENABLE)
        {
            return false;
        }
        else if (ble_privacy_cb.rl_state == BLE_RL_DISABLE)
        {
            T_GAP_CAUSE cause;
            if ((cause = le_privacy_set_addr_resolution(true)) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR1("check_rl_to_enable_resolving enable address resolution fail cause 0x%x fail",
                                 cause);
                return false;
            }
            else
            {
                ble_privacy_cb.rl_state = BLE_RL_ENABLING;
            }
            return true;
        }
    }
    return true;
}


static bool execute_rl_dev_operation()
{
    T_UALIST_HEAD *pos, *n;
    T_RL_DEV *p_rl_dev;

    APP_PRINT_INFO1("execute_rl_dev_operation rl_attempt %d", ble_privacy_cb.rl_attempt);
    if (ble_privacy_cb.rl_attempt >= RL_ATTEMPT_COUNT)
    {
        APP_PRINT_ERROR0("execute_rl_dev_operation: resolving attempting exceeded");
        ble_privacy_cb.rl_attempt = 0;
        goto done;
    }
    if (ble_privacy_cb.p_rl_op_dev != NULL)
    {
        APP_PRINT_ERROR0("execute_rl_dev_operation: already has operation");
        return false;
    }

    ualist_for_each_safe(pos, n, &ble_privacy_cb.rl_list)
    {
        p_rl_dev = ualist_entry(pos, T_RL_DEV, list);
        if (p_rl_dev->pending_removal)
        {
            if (ble_privacy_cb.rl_state != BLE_RL_DISABLE)
            {
                goto disable;
            }
            if (le_privacy_modify_resolv_list(GAP_RESOLV_LIST_OP_REMOVE, p_rl_dev->addr_type,
                                              p_rl_dev->identity_addr) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR0("execute_rl_dev_operation remove resolving list fail");
                continue;
            }
            ble_privacy_cb.p_rl_op_dev = p_rl_dev;
        }
        else if (!p_rl_dev->in_controller_rl)
        {
            if (ble_privacy_cb.rl_state != BLE_RL_DISABLE)
            {
                goto disable;
            }
            if (le_privacy_modify_resolv_list(GAP_RESOLV_LIST_OP_ADD, p_rl_dev->addr_type,
                                              p_rl_dev->identity_addr) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR0("execute_rl_dev_operation add resoling list fail");
                continue;
            }
            ble_privacy_cb.p_rl_op_dev = p_rl_dev;
        }

        if (ble_privacy_cb.p_rl_op_dev)
        {
            return true;
        }
    }

done:
    ble_privacy_cb.rl_attempt = 0;
    ble_privacy_cb.rl_act_state &= ~BLE_RL_ACT_MODIFY_RL;
    check_rl_pending_action();

    return true;

disable:
    if (ble_privacy_cb.rl_state == BLE_RL_DISABLING ||
        ble_privacy_cb.rl_state == BLE_RL_ENABLING)
    {
        APP_PRINT_WARN1("execute_rl_dev_operation: wait for rl_state 0x%x", ble_privacy_cb.rl_state);
    }
    else if (ble_privacy_cb.rl_state == BLE_RL_ENABLE)
    {
        T_GAP_CAUSE cause;
        APP_PRINT_WARN0("execute_rl_dev_operation: wait for rl_state to disable");
        if ((cause = le_privacy_set_addr_resolution(false)) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("execute_rl_dev_operation disable address resolution fail cause 0x%x fail", cause);
        }
        else
        {
            ble_privacy_cb.rl_state = BLE_RL_DISABLING;
        }
    }
    return false;
}


static T_APP_RESULT ble_privacy_callback(uint8_t msg_type, T_LE_PRIVACY_CB_DATA msg_data)
{
    switch (msg_type)
    {
    case GAP_MSG_LE_PRIVACY_RESOLUTION_STATUS_INFO:
        {
            APP_PRINT_INFO2("GAP_MSG_LE_PRIVACY_RESOLUTION_STATUS_INFO status %d, rl_act_state 0x%x",
                            msg_data.le_privacy_resolution_status_info.status, ble_privacy_cb.rl_act_state);
            if (msg_data.le_privacy_resolution_status_info.status == LE_PRIVACY_RESOLUTION_DISABLED)
            {
                ble_privacy_cb.rl_state = BLE_RL_DISABLE;
                if (ble_privacy_cb.rl_act_state & BLE_RL_ACT_MODIFY_RL)
                {
                    execute_rl_dev_operation();
                }
                else
                {
                    if (!check_rl_to_enable_resolving())
                    {
                        ble_privacy_cb.rl_act_state = BLE_RL_ACT_IDLE;
                        if (ble_privacy_cb.to_resume)
                        {
                            ble_resume_resolving_list_activity();
                        }
                    }
                }
            }
            else if (msg_data.le_privacy_resolution_status_info.status == LE_PRIVACY_RESOLUTION_ENABLED)
            {
                ble_privacy_cb.rl_state = BLE_RL_ENABLE;
                if (ble_privacy_cb.rl_act_state & BLE_RL_ACT_MODIFY_RL)
                {
                    execute_rl_dev_operation();
                }
                else
                {
                    if (!check_rl_to_enable_resolving())
                    {
                        ble_privacy_cb.rl_act_state = BLE_RL_ACT_IDLE;
                        if (ble_privacy_cb.to_resume)
                        {
                            ble_resume_resolving_list_activity();
                        }
                    }
                }
            }
        }
        break;
    case GAP_MSG_LE_PRIVACY_SET_RESOLV_PRIV_ADDR_TIMEOUT:
        break;
    case GAP_MSG_LE_PRIVACY_MODIFY_RESOLV_LIST:
        {
            T_LE_PRIVACY_MODIFY_RESOLV_LIST_RSP *p_rl_rsp = msg_data.p_le_privacy_modify_resolv_list_rsp;
            if (p_rl_rsp->cause == GAP_SUCCESS && ble_privacy_cb.p_rl_op_dev)
            {
                ble_privacy_cb.rl_attempt = 0;
                if (p_rl_rsp->operation == GAP_RESOLV_LIST_OP_ADD)
                {
                    ble_privacy_cb.p_rl_op_dev->in_controller_rl = true;
                    le_privacy_set_mode(ble_privacy_cb.p_rl_op_dev->addr_type,
                                        ble_privacy_cb.p_rl_op_dev->identity_addr,
                                        GAP_PRIVACY_MODE_DEVICE);
                }
                else if (p_rl_rsp->operation == GAP_RESOLV_LIST_OP_REMOVE)
                {
                    ualist_del(&ble_privacy_cb.p_rl_op_dev->list);
                    os_mem_free(ble_privacy_cb.p_rl_op_dev);
                }
            }
            else
            {
                ble_privacy_cb.rl_attempt++;
            }

            if (p_rl_rsp->cause == GAP_SUCCESS &&
                p_rl_rsp->operation == GAP_RESOLV_LIST_OP_CLEAR)
            {
                APP_PRINT_INFO0("GAP_MSG_LE_PRIVACY_MODIFY_RESOLV_LIST receive clear");
                T_UALIST_HEAD *pos, *n;
                T_RL_DEV *p_rl_dev;
                ualist_for_each_safe(pos, n, &ble_privacy_cb.rl_list)
                {
                    ualist_del(pos);
                    p_rl_dev = ualist_entry(pos, T_RL_DEV, list);
                    os_mem_free(p_rl_dev);
                    ble_privacy_cb.resolving_list_avail_size++;
                }
            }
            ble_privacy_cb.p_rl_op_dev = NULL;
            execute_rl_dev_operation();
        }
        break;
    case GAP_MSG_LE_PRIVACY_READ_PEER_RESOLV_ADDR:
        {
            T_LE_PRIVACY_READ_PEER_RESOLV_ADDR_RSP *p_rsp = msg_data.p_le_privacy_read_peer_resolv_addr_rsp;
            if (p_rsp->cause == GAP_SUCCESS)
            {
                T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(p_rsp->peer_resolv_addr);
                if (p_dev_rec)
                {
                    memcpy(p_dev_rec->cur_rand_addr, p_rsp->peer_resolv_addr, BD_ADDR_LEN);
                }
            }
        }
        break;
    case GAP_MSG_LE_PRIVACY_READ_LOCAL_RESOLV_ADDR:
        {
            APP_PRINT_INFO0("GAP_MSG_LE_PRIVACY_READ_LOCAL_RESOLV_ADDR");
        }
        break;
    case GAP_MSG_LE_PRIVACY_SET_MODE:
        break;
    }

    return APP_RESULT_SUCCESS;
}

//return true means suspend is on the way
//return false means suspend is finished
static bool ble_suspend_resolving_list_activity(void)
{
    APP_PRINT_INFO1("ble_suspend_resolving_list_activity suspended_rl_state 0x%x",
                    ble_privacy_cb.suspended_rl_state);
    if (ble_privacy_cb.suspended_rl_state != BLE_RL_IDLE)
    {
        return true;
    }

    ble_privacy_cb.suspended_rl_state = BLE_RL_ADV | BLE_RL_SCAN | BLE_RL_INIT | BLE_RL_PA_SYNC;
    if (adv_suspend_req())
    {
        ble_privacy_cb.suspended_rl_state &= (~BLE_RL_ADV);
    }

    if (le_scan_suspend_req())
    {
        ble_privacy_cb.suspended_rl_state &= (~BLE_RL_SCAN);
    }

    if (ble_bg_suspend())
    {
        ble_privacy_cb.suspended_rl_state &= (~BLE_RL_INIT);
    }

    if (ble_suspend_pa_sync())
    {
        ble_privacy_cb.suspended_rl_state &= (~BLE_RL_PA_SYNC);
    }

    APP_PRINT_INFO1("update suspended_rl_state 0x%x", ble_privacy_cb.suspended_rl_state);
    if (ble_privacy_cb.suspended_rl_state != BLE_RL_IDLE)
    {
        return true;
    }

    return false;
}


static bool ble_exe_disable_resolving_list(bool to_resume)
{
    T_GAP_CAUSE cause;
    if (!(ble_privacy_cb.rl_act_state & BLE_RL_ACT_SUSPEND) &&
        ble_privacy_cb.rl_act_state != BLE_RL_ACT_IDLE)
    {
        return false;
    }
    ble_privacy_cb.rl_act_state = BLE_RL_ACT_IDLE;
    ble_privacy_cb.to_resume = to_resume;
    ble_privacy_cb.rl_act_state |= (BLE_RL_ACT_SUSPEND);
    if (ble_suspend_resolving_list_activity())
    {
        return true;
    }

    if ((cause = le_privacy_set_addr_resolution(false)) != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("ble_exe_disable_resolving_list cause 0x%x fail", cause);
        return false;
    }
    ble_privacy_cb.rl_state = BLE_RL_DISABLING;
    return true;
}

static bool ble_exe_enable_resolving_list(void)
{
    T_GAP_CAUSE cause;
    if (!(ble_privacy_cb.rl_act_state & BLE_RL_ACT_SUSPEND) &&
        ble_privacy_cb.rl_act_state != BLE_RL_ACT_IDLE)
    {
        return false;
    }
    ble_privacy_cb.rl_act_state = BLE_RL_ACT_IDLE;
    ble_privacy_cb.to_resume = true;

    ble_privacy_cb.rl_act_state |= (BLE_RL_ACT_SUSPEND);
    if (ble_suspend_resolving_list_activity())
    {
        return true;
    }

    if ((cause = le_privacy_set_addr_resolution(true)) != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("ble_exe_enable_resolving_list cause 0x%x fail", cause);
        return false;
    }
    ble_privacy_cb.rl_state = BLE_RL_ENABLING;
    return true;
}


bool ble_enable_resolving_list()
{
    if (ble_privacy_cb.rl_size == 0)
    {
        return false;
    }

    if (ble_privacy_cb.rl_state == BLE_RL_DISABLE &&
        ble_privacy_cb.rl_size != 0)
    {
        return ble_exe_enable_resolving_list();
    }
    return true;
}

bool ble_disable_resolving_list(bool to_resume)
{
    /* if controller does not support RPA offloading or privacy 1.2, skip */
    if (ble_privacy_cb.rl_size == 0)
    {
        return false;
    }

    if (ble_privacy_cb.rl_state == BLE_RL_ENABLE)
    {
        return ble_exe_disable_resolving_list(to_resume);
    }

    return true;
}

static void check_rl_pending_action()
{
    APP_PRINT_INFO1("check_rl_pending_action suspended_rl_state 0x%x",
                    ble_privacy_cb.suspended_rl_state);
    if (ble_privacy_cb.suspended_rl_state == BLE_RL_IDLE)
    {
        ble_privacy_cb.rl_act_state &= ~BLE_RL_ACT_SUSPEND;
        if (ble_privacy_cb.rl_act_state & BLE_RL_ACT_SET_LOCAL_RPA)
        {
            le_set_rand_addr(get_adapter_rpa());
            ble_privacy_cb.rl_act_state &= ~BLE_RL_ACT_SET_LOCAL_RPA;
        }

        if (ble_privacy_cb.rl_act_state & BLE_RL_ACT_MODIFY_RL)
        {
            execute_rl_dev_operation();
        }
        else if (!check_rl_to_enable_resolving())
        {
            ble_privacy_cb.rl_act_state = BLE_RL_ACT_IDLE;
            if (ble_privacy_cb.to_resume)
            {
                ble_resume_resolving_list_activity();
            }
        }
    }
}
void handle_scan_stop_in_suspend()
{
    ble_privacy_cb.suspended_rl_state &= ~BLE_RL_SCAN;
    check_rl_pending_action();
}

void handle_bgconn_stop_in_suspend()
{
    ble_privacy_cb.suspended_rl_state &= ~BLE_RL_INIT;
    check_rl_pending_action();
}

void handle_adv_stop_in_suspend()
{
    ble_privacy_cb.suspended_rl_state &= ~BLE_RL_ADV;
    check_rl_pending_action();
}

void handle_pa_sync_stop_in_suspend()
{
    ble_privacy_cb.suspended_rl_state &= ~BLE_RL_PA_SYNC;
    check_rl_pending_action();
}


bool ble_resolving_list_load_dev(T_BT_DEVICE *p_dev_rec)
{
    if (ble_privacy_cb.rl_size == 0)
    {
        APP_PRINT_ERROR0("ble_resolving_list_load_dev: Controller does not support  privacy 1.2");
        return false;
    }

    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("ble_resolving_list_load_dev: No device security record");
        return false;
    }
    if (le_check_supported_features(LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX0,
                                    LE_SUPPORT_FEATURES_LL_PRIVACY_MASK_BIT))
    {
        if (!p_dev_rec->has_irk)
        {
            APP_PRINT_ERROR0("ble_resolving_list_load_dev: privacy 1.2: Device not a RPA enabled device");
            return false;
        }
    }
    else if (!p_dev_rec->has_irk)
    {
        APP_PRINT_ERROR0("ble_resolving_list_load_dev: RPA offloading: Device not a RPA enabled device");
        return false;
    }

    if (ble_privacy_cb.resolving_list_avail_size == 0)
    {
        APP_PRINT_ERROR0("ble_resolving_list_load_dev: resolving_list_avail_size is 0");
        return false;
    }

    if (!rl_add_device(p_dev_rec->identity_bd_type, p_dev_rec->identity_addr))
    {
        APP_PRINT_ERROR0("ble_resolving_list_load_dev: add rl device fail");
        return false;
    }
    if (ble_privacy_cb.rl_act_state & BLE_RL_ACT_MODIFY_RL)
    {
        APP_PRINT_INFO0("ble_resolving_list_load_dev: already in process");
        return true;
    }

    p_dev_rec->in_controller_list |= BLE_RESOLVING_LIST_BIT;
    ble_privacy_cb.to_resume = true;
    ble_privacy_cb.rl_act_state |= BLE_RL_ACT_MODIFY_RL;

    if (ble_suspend_resolving_list_activity())
    {
        ble_privacy_cb.rl_act_state |= BLE_RL_ACT_SUSPEND;
    }
    else
    {
        execute_rl_dev_operation();
    }

    APP_PRINT_TRACE1("ble_resolving_list_load_dev: rl_act_state 0x%x", ble_privacy_cb.rl_act_state);

    return true;
}

bool ble_resolving_list_remove_dev(T_BT_DEVICE *p_dev_rec)
{
    if (ble_privacy_cb.rl_size == 0)
    {
        APP_PRINT_ERROR0("ble_resolving_list_remove_dev: Controller does not support  privacy 1.2");
        return false;
    }

    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("ble_resolving_list_remove_dev: No device security record");
        return false;
    }
    p_dev_rec->in_controller_list &= ~BLE_RESOLVING_LIST_BIT;
    if (rl_rmv_device(p_dev_rec->identity_bd_type, p_dev_rec->identity_addr))
    {
        return true;
    }

    if (ble_privacy_cb.rl_act_state & BLE_RL_ACT_MODIFY_RL)
    {
        APP_PRINT_INFO0("ble_resolving_list_remove_dev: already in process");
        return true;
    }

    ble_privacy_cb.to_resume = true;
    ble_privacy_cb.rl_act_state |= BLE_RL_ACT_MODIFY_RL;

    if (ble_suspend_resolving_list_activity())
    {
        ble_privacy_cb.rl_act_state |= BLE_RL_ACT_SUSPEND;
    }
    else
    {
        execute_rl_dev_operation();
    }
    APP_PRINT_TRACE1("ble_resolving_list_remove_dev: rl_act_state 0x%x", ble_privacy_cb.rl_act_state);

    return true;
}

bool ble_resolving_list_is_enable()
{
    if (ble_privacy_cb.rl_state == BLE_RL_ENABLE)
    {
        return true;
    }

    return false;
}

void ble_handle_rpa_set_rsp(T_LE_SET_RAND_ADDR_RSP *p_rsp)
{
    if (p_rsp->cause != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_INFO0("ble_handle_rpa_set_rsp: FAIL");
    }
}

void ble_change_local_rpa(uint8_t *rpa)
{
    APP_PRINT_TRACE0("ble_change_local_rpa");
    if (ble_suspend_resolving_list_activity())
    {
        if (ble_privacy_cb.rl_act_state == BLE_RL_ACT_IDLE)
        {
            ble_privacy_cb.to_resume = true;
        }
        ble_privacy_cb.rl_act_state |= (BLE_RL_ACT_SUSPEND | BLE_RL_ACT_SET_LOCAL_RPA);
        return;
    }

    le_set_rand_addr(get_adapter_rpa());
    if (ble_privacy_cb.rl_act_state == BLE_RL_ACT_IDLE)
    {
        ble_resume_resolving_list_activity();
    }
}

void ble_privacy_init()
{
    memset(&ble_privacy_cb, 0, sizeof(ble_privacy_cb));

    le_privacy_register_cb(ble_privacy_callback);
    le_privacy_get_param(GAP_PARAM_PRIVACY_RESOLV_LIST_SIZE, &ble_privacy_cb.rl_size);
    ble_privacy_cb.resolving_list_avail_size = ble_privacy_cb.rl_size;
    init_ualist_head(&ble_privacy_cb.rl_list);
}
#endif
