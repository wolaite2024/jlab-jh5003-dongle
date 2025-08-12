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
#include "ual_upperstack_cfg.h"
#include "btm.h"
#include "gap_msg.h"
#include "gap_ext_scan.h"
#include "dev_mgr.h"
#include "ual_api_types.h"
#include "gap_le.h"
#include "adapter_int.h"
#include "ble_pa_sync.h"
#include "ble_privacy.h"
#include "ual_adapter.h"
#include "app_timer.h"

#define BLE_PAL_IDLE                 0
#define BLE_PAL_MODIFY               0x01
#define BLE_PAL_PENDING_SYNCING      0x02
#define BLE_PAL_SYNC                 0x04
#define BLE_PAL_CANCEL_SYNC          0x08
#define BLE_PAL_SUSPENDING           0x10

#define PAL_ATTEMPT_COUNT            3

#define BLE_PA_SYNC_TIME_OUT          1000
#define BLE_PA_SYNC_SKIP              10

#define BROADCAST_ID_LEN               (3)

typedef struct
{
    T_UALIST_HEAD           list;
    T_APP_ID                 app_id;
    T_LE_PA_SYNC_CBACK    callback;
} T_APP_PA_SYNC_CBACK;


typedef struct t_app_pa_sync_cb
{
    T_UALIST_HEAD         list;
    uint8_t               real_sync_id;
    uint16_t              sync_handle;
    uint8_t               adv_addr_type;
    uint8_t               adv_sid;
    uint8_t               adv_addr[BD_ADDR_LEN];
    uint8_t               broadcast_id[BROADCAST_ID_LEN];
    uint32_t              doing_bg_sync_set;
    uint32_t              doing_direct_sync_set;

    T_GAP_PA_SYNC_STATE   app_pa_sync_state;
    T_GAP_PA_SYNC_STATE   pa_sync_state;
    bool                  pending_removal;
    bool                  in_controller_pal;
    bool                  in_adding_pal;
    uint16_t              time_rank;
    uint8_t               timer_idx_direct_pa_sync;
} T_APP_PA_SYNC_CB;


typedef struct
{
    uint8_t               pl_state;
    uint8_t               pal_size;
    uint8_t               pal_added_num;
    uint8_t               added_num;
    uint8_t               pal_attempt;
    uint8_t               pa_sync_id;
    int                   scan_id;
    T_GAP_PA_SYNC_STATE   pa_sync_state;
    T_APP_PA_SYNC_CB      *p_pal_op_dev;
    T_UALIST_HEAD         pal_list;
    T_UALIST_HEAD         app_pa_sync_callbacks;

    uint8_t               pa_sync_timer_id;
} T_PA_SYNC_CB;

#define UAL_MAX_SYNC_HANDLE_NUM                   4
#define DIRECT_PA_SYNC_TIMEOUT_ID                 1
#define DIRECT_PA_SYNC_TIME_OUT                   30000

static T_PA_SYNC_CB ble_pa_sync_cb;
static P_FUN_LE_BIG_MGR_SYNC_RX_CB ble_audio_sync_cb = NULL;
static T_GAP_PA_SYNC_DEV_STATE pa_sync_dev_state = {0, 0, 0};
static uint16_t pa_time_rank = 0;

static T_APP_PA_SYNC_CBACK *find_pa_sync_app_callback(T_APP_ID app_id)
{
    T_UALIST_HEAD *pos, *n;
    T_APP_PA_SYNC_CBACK *p_pa_sync_callback = NULL;

    ualist_for_each_safe(pos, n, &ble_pa_sync_cb.app_pa_sync_callbacks)
    {
        p_pa_sync_callback = ualist_entry(pos, T_APP_PA_SYNC_CBACK, list);
        if (p_pa_sync_callback->app_id == app_id)
        {
            return p_pa_sync_callback;
        }
    }
    return NULL;
}


static T_APP_PA_SYNC_CB *find_pa_node_by_time_rank(uint16_t rank)
{
    T_UALIST_HEAD *pos, *n;
    T_APP_PA_SYNC_CB *p_pa_dev;
    ualist_for_each_safe(pos, n, &ble_pa_sync_cb.pal_list)
    {
        p_pa_dev = ualist_entry(pos, T_APP_PA_SYNC_CB, list);
        if (rank == p_pa_dev->time_rank)
        {
            return p_pa_dev;
        }
    }

    return NULL;
}

static T_APP_PA_SYNC_CB *find_pa_by_adv_info(uint8_t *address, uint8_t addr_type, uint8_t adv_sid)
{
    T_UALIST_HEAD *pos, *n;
    T_APP_PA_SYNC_CB *p_pa_dev;

    ualist_for_each_safe(pos, n, &ble_pa_sync_cb.pal_list)
    {
        p_pa_dev = ualist_entry(pos, T_APP_PA_SYNC_CB, list);
        if (!memcmp(p_pa_dev->adv_addr, address, BD_ADDR_LEN) &&
            p_pa_dev->adv_addr_type == addr_type &&
            p_pa_dev->adv_sid == adv_sid)
        {
            return p_pa_dev;
        }
        else
        {
            T_BT_DEVICE *p_sec_dev = ual_find_device_by_addr(address);
            if (p_sec_dev)
            {
                T_BT_DEVICE *p_sec_dev_temp = ual_find_device_by_addr(p_pa_dev->adv_addr);
                if (p_sec_dev == p_sec_dev_temp)
                {
                    return p_pa_dev;
                }
            }
        }
    }

    return NULL;
}

static T_APP_PA_SYNC_CB *alloc_pa_dev_node_by_adv_info(uint8_t *address, uint8_t addr_type,
                                                       uint8_t adv_sid, uint8_t *broadcast_id)
{
    T_APP_PA_SYNC_CB *p_pa_dev = NULL;

    p_pa_dev = os_mem_zalloc(RAM_TYPE_BT_UAL, sizeof(T_APP_PA_SYNC_CB));
    if (p_pa_dev)
    {
        p_pa_dev->real_sync_id = 0xFF;
        p_pa_dev->sync_handle = 0xFFFF;
        memcpy(p_pa_dev->adv_addr, address, BD_ADDR_LEN);
        memcpy(p_pa_dev->broadcast_id, broadcast_id, BROADCAST_ID_LEN);
        p_pa_dev->adv_addr_type = addr_type;
        p_pa_dev->adv_sid = adv_sid;
        init_ualist_head(&p_pa_dev->list);
        ualist_add_tail(&p_pa_dev->list, &ble_pa_sync_cb.pal_list);
        ble_pa_sync_cb.pal_added_num++;
    }
    return p_pa_dev;
}

static T_APP_PA_SYNC_CB *find_pa_by_sync_id(uint8_t sync_id)
{
    T_UALIST_HEAD *pos, *n;
    T_APP_PA_SYNC_CB *p_pa_dev;

    ualist_for_each_safe(pos, n, &ble_pa_sync_cb.pal_list)
    {
        p_pa_dev = ualist_entry(pos, T_APP_PA_SYNC_CB, list);
        if (p_pa_dev->real_sync_id == sync_id)
        {
            return p_pa_dev;
        }
    }

    return NULL;
}

static T_APP_PA_SYNC_CB *find_init_pa_sync(void)
{
    T_UALIST_HEAD *pos, *n;
    T_APP_PA_SYNC_CB *p_pa_dev;

    ualist_for_each_safe(pos, n, &ble_pa_sync_cb.pal_list)
    {
        p_pa_dev = ualist_entry(pos, T_APP_PA_SYNC_CB, list);
        if ((p_pa_dev->real_sync_id == 0xFF) &&
            (p_pa_dev->app_pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING))
        {
            return p_pa_dev;
        }
    }
    return NULL;
}


static bool check_pal_for_sync()
{
    T_UALIST_HEAD *pos, *n;
    T_APP_PA_SYNC_CB *p_pa_dev;
    bool need_sync = false;
    ualist_for_each_safe(pos, n, &ble_pa_sync_cb.pal_list)
    {
        p_pa_dev = ualist_entry(pos, T_APP_PA_SYNC_CB, list);
        if (p_pa_dev->in_controller_pal && (!p_pa_dev->pending_removal) &&
            p_pa_dev->pa_sync_state == GAP_PA_SYNC_STATE_TERMINATED)
        {
            need_sync = true;
            break;
        }
    }
    APP_PRINT_INFO2("check_pal_for_sync need_sync %d, scan_id %d", need_sync, ble_pa_sync_cb.scan_id);
    if (need_sync)
    {
        if (ble_pa_sync_cb.scan_id < 0)
        {
            ble_pa_sync_cb.scan_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, GAP_SCAN_FILTER_ANY, NULL);
            if (ble_pa_sync_cb.scan_id < 0)
            {
                APP_PRINT_WARN1("check_pal_for_sync scan start fail: scan_id 0x%x", ble_pa_sync_cb.scan_id);
            }
        }
    }
    else
    {
        if (ble_pa_sync_cb.scan_id > 0)
        {
            if (bt_adap_stop_discovery(ble_pa_sync_cb.scan_id) < 0)
            {
                APP_PRINT_WARN0("check_pal_for_sync scan stop fail");
            }
            ble_pa_sync_cb.scan_id = -1;
        }
    }
    return need_sync;
}

static void pa_create_sync()
{
    T_GAP_CAUSE cause;
    if ((ble_pa_sync_cb.pl_state & BLE_PAL_SUSPENDING) == 0 &&
        ble_pa_sync_cb.pa_sync_id == GAP_INVALID_SYNC_ID &&
        check_pal_for_sync())
    {
        T_GAP_PA_SYNC_CREATE_SYNC_PARAM sync_param;
        memset(&sync_param, 0, sizeof(T_GAP_PA_SYNC_CREATE_SYNC_PARAM));
        sync_param.options = PA_SYNC_CREATE_SYNC_OPTIONS_USE_PERIODIC_ADV_LIST;
        sync_param.sync_cte_type = 0;
        sync_param.skip = BLE_PA_SYNC_SKIP;
        sync_param.sync_timeout = BLE_PA_SYNC_TIME_OUT;

        cause = le_pa_sync_create_sync(&sync_param, &ble_pa_sync_cb.pa_sync_id);
        if (cause != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_INFO1("pa_create_sync fail: cause 0x%x", cause);
        }
    }
}

static void ble_terminate_pa_sync()
{
    T_UALIST_HEAD *pos, *n;
    T_APP_PA_SYNC_CB *p_bg_pa_dev;
    T_GAP_CAUSE cause;

    if (pa_sync_dev_state.gap_terminate_sync_state == GAP_PA_TERMINATE_SYNC_DEV_STATE_TERMINATING)
    {
        return;
    }
    ualist_for_each_safe(pos, n, &ble_pa_sync_cb.pal_list)
    {
        p_bg_pa_dev = ualist_entry(pos, T_APP_PA_SYNC_CB, list);
        if (p_bg_pa_dev->pending_removal)
        {
            if (p_bg_pa_dev->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
            {
                cause = le_pa_sync_terminate_sync(p_bg_pa_dev->real_sync_id);
                if (cause != GAP_CAUSE_SUCCESS)
                {
                    APP_PRINT_ERROR1("ble_terminate_pa_sync terminate fail 0x%x", cause);
                }
                else
                {
                    if (!p_bg_pa_dev->in_controller_pal)
                    {
                        ualist_del(&p_bg_pa_dev->list);
                        os_mem_free(p_bg_pa_dev);
                        ble_pa_sync_cb.pal_added_num--;
                    }
                    break;
                }
            }
        }
    }

}

static void ble_handle_pa_dev_state_evt(T_GAP_PA_SYNC_DEV_STATE new_state, uint16_t cause)
{
    T_GAP_PA_SYNC_DEV_STATE old_state = pa_sync_dev_state;
    pa_sync_dev_state = new_state;
    if (old_state.gap_terminate_sync_state != new_state.gap_terminate_sync_state)
    {
        if (new_state.gap_terminate_sync_state == GAP_PA_TERMINATE_SYNC_DEV_STATE_IDLE)
        {
            APP_PRINT_INFO0("GAP_PA_TERMINATE_SYNC_DEV_STATE_IDLE");
            ble_terminate_pa_sync();
        }
        else if (new_state.gap_terminate_sync_state == GAP_PA_TERMINATE_SYNC_DEV_STATE_TERMINATING)
        {
            APP_PRINT_INFO0("GAP_PA_TERMINATE_SYNC_DEV_STATE_TERMINATING0");
        }
    }

    if (old_state.gap_create_sync_state != new_state.gap_create_sync_state)
    {
        if (new_state.gap_create_sync_state == GAP_PA_CREATE_SYNC_DEV_STATE_IDLE)
        {

        }
        else if (new_state.gap_create_sync_state == GAP_PA_CREATE_SYNC_DEV_STATE_SYNCHRONIZING)
        {
            APP_PRINT_INFO0("GAP_PA_CREATE_SYNC_DEV_STATE_SYNCHRONIZING");
        }
    }
}

static bool execute_pal_dev_operation(void)
{
    T_UALIST_HEAD *pos, *n;
    T_APP_PA_SYNC_CB *p_bg_pa_dev;
    T_BT_DEVICE *p_dev_rec;
    T_GAP_CAUSE cause;
    uint8_t null_bdaddr[6] = {0, 0, 0, 0, 0, 0};

    APP_PRINT_INFO2("execute_pal_dev_operation pa_sync_state 0x%x, sync_id %d",
                    ble_pa_sync_cb.pa_sync_state, ble_pa_sync_cb.pa_sync_id);
    if (ble_pa_sync_cb.pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_SCAN_IDLE ||
        ble_pa_sync_cb.pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_WAIT_SCANNING ||
        ble_pa_sync_cb.pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING)
    {
        APP_PRINT_WARN2("execute_pal_dev_operation cancel create sync first, sync_id %d, pa_sync_state 0x%x",
                        ble_pa_sync_cb.pa_sync_id, ble_pa_sync_cb.pa_sync_state);
        if ((cause = le_pa_sync_terminate_sync(ble_pa_sync_cb.pa_sync_id)) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("execute_pal_dev_operation cancel create sync fail cause 0x%x", cause);
            return false;
        }
        return true;
    }
    else if (ble_pa_sync_cb.pa_sync_state == GAP_PA_SYNC_STATE_TERMINATING)
    {
        APP_PRINT_ERROR0("execute_pal_dev_operation cancel wait for sync cancel");
        return true;
    }

    if (ble_pa_sync_cb.p_pal_op_dev != NULL)
    {
        APP_PRINT_ERROR0("execute_pal_dev_operation already has operation");
        return false;
    }

    if (ble_pa_sync_cb.pal_attempt >= PAL_ATTEMPT_COUNT)
    {
        APP_PRINT_ERROR0("execute_pal_dev_operation: pa attempting exceeded");
        goto done;
    }

    ualist_for_each_safe(pos, n, &ble_pa_sync_cb.pal_list)
    {
        p_bg_pa_dev = ualist_entry(pos, T_APP_PA_SYNC_CB, list);
        p_dev_rec = ual_find_device_by_addr(p_bg_pa_dev->adv_addr);
        uint8_t adv_addr[BD_ADDR_LEN];
        uint8_t adv_addr_type;
        if (p_dev_rec && p_dev_rec->has_irk)
        {
            if (le_check_supported_features(LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX0,
                                            LE_SUPPORT_FEATURES_LL_PRIVACY_MASK_BIT))
            {
                memcpy(adv_addr, p_dev_rec->identity_addr, BD_ADDR_LEN);
                adv_addr_type = p_dev_rec->identity_bd_type & (~BLE_BD_TYPE_ID_BIT);
            }
            else
            {
                if (memcmp(p_dev_rec->cur_rand_addr, null_bdaddr, BD_ADDR_LEN))
                {
                    memcpy(adv_addr, p_dev_rec->cur_rand_addr, BD_ADDR_LEN);
                    adv_addr_type = BLE_ADDR_RANDOM;
                }
                else
                {
                    memcpy(adv_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
                    adv_addr_type = p_dev_rec->bd_type & (~BLE_BD_TYPE_ID_BIT);
                }
            }
        }
        else
        {
            memcpy(adv_addr, p_bg_pa_dev->adv_addr, BD_ADDR_LEN);
            adv_addr_type = p_bg_pa_dev->adv_addr_type & (~BLE_BD_TYPE_ID_BIT);
        }
        if (p_bg_pa_dev->pending_removal)
        {
            if (p_bg_pa_dev->in_controller_pal)
            {
                if (le_pa_sync_modify_periodic_adv_list(GAP_PA_SYNC_PERIODIC_ADV_LIST_OP_REMOVE, adv_addr,
                                                        (T_GAP_PA_SYNC_ADV_ADDR_TYPE)adv_addr_type,
                                                        p_bg_pa_dev->adv_sid) != GAP_CAUSE_SUCCESS)
                {
                    APP_PRINT_ERROR0("execute_pal_dev_operation remove pa list fail");
                    continue;
                }
                ble_pa_sync_cb.p_pal_op_dev = p_bg_pa_dev;
            }
            else
            {
                if (p_bg_pa_dev->pa_sync_state != GAP_PA_SYNC_STATE_SYNCHRONIZED)
                {
                    ualist_del(&p_bg_pa_dev->list);
                    os_mem_free(p_bg_pa_dev);
                    ble_pa_sync_cb.pal_added_num--;
                }
                continue;
            }
        }
        else if (!p_bg_pa_dev->in_controller_pal &&
                 (p_bg_pa_dev->pa_sync_state != GAP_PA_SYNC_STATE_SYNCHRONIZED))
        {
            if (le_pa_sync_modify_periodic_adv_list(GAP_PA_SYNC_PERIODIC_ADV_LIST_OP_ADD, adv_addr,
                                                    (T_GAP_PA_SYNC_ADV_ADDR_TYPE)adv_addr_type,
                                                    p_bg_pa_dev->adv_sid) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR0("execute_pal_dev_operation add pa list fail");
                continue;
            }

            p_bg_pa_dev->in_adding_pal = true;
            ble_pa_sync_cb.p_pal_op_dev = p_bg_pa_dev;
        }
        else if (p_bg_pa_dev->in_controller_pal &&
                 (p_bg_pa_dev->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED))
        {
            if (le_pa_sync_modify_periodic_adv_list(GAP_PA_SYNC_PERIODIC_ADV_LIST_OP_REMOVE, adv_addr,
                                                    (T_GAP_PA_SYNC_ADV_ADDR_TYPE)adv_addr_type,
                                                    p_bg_pa_dev->adv_sid) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR0("execute_pal_dev_operation remove pa list fail");
                continue;
            }
            ble_pa_sync_cb.p_pal_op_dev = p_bg_pa_dev;
        }

        if (ble_pa_sync_cb.p_pal_op_dev)
        {
            return true;
        }
    }

done:
    ble_pa_sync_cb.pal_attempt = 0;
    pa_create_sync();

    return true;
}


//FIX TODO
T_APP_RESULT ble_big_sync_receiver_cb(uint8_t big_handle, uint8_t cb_type,
                                      void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;

    switch (cb_type)
    {
    case MSG_BIG_MGR_SETUP_DATA_PATH:
        {

        }
        break;

    case MSG_BIG_MGR_REMOVE_DATA_PATH:
        {

        }
        break;

    case MSG_BIG_MGR_SYNC_RECEIVER_BIG_SYNC_ESTABLISHED_INFO:
        {

        }
        break;

    case MSG_BIG_MGR_SYNC_RECEIVER_DEV_STATE_CHANGE_INFO:
        {

        }
        break;

    case MSG_BIG_MGR_SYNC_RECEIVER_SYNC_STATE_CHANGE_INFO:
        {

        }
        break;

    default:
        break;
    }
    if (ble_audio_sync_cb)
    {
        result = ble_audio_sync_cb(big_handle, cb_type, p_cb_data);
    }
    return result;
}

static void ble_handle_pa_synced_or_cancel()
{
#if BLE_PRIVACY_SPT
    if (ble_pa_sync_cb.pl_state & BLE_PAL_SUSPENDING)
    {
        handle_pa_sync_stop_in_suspend();
    }
    else
#endif
    {
        execute_pal_dev_operation();
    }
}

void ble_pa_sync_handle_gap_cb(uint8_t cb_type, T_LE_CB_DATA *p_data)
{
    switch (cb_type)
    {
#if F_BT_LE_5_0_PA_SYNC_SCAN_SUPPORT
    case GAP_MSG_LE_PA_SYNC_MODIFY_PERIODIC_ADV_LIST:
        {
            APP_PRINT_INFO2("GAP_MSG_LE_PA_SYNC_MODIFY_PERIODIC_ADV_LIST: operation %d, cause 0x%x",
                            p_data->p_le_pa_sync_modify_periodic_adv_list_rsp->operation,
                            p_data->p_le_pa_sync_modify_periodic_adv_list_rsp->cause);
            if (ble_pa_sync_cb.p_pal_op_dev)
            {
                if (p_data->p_le_pa_sync_modify_periodic_adv_list_rsp->cause == GAP_CAUSE_SUCCESS)
                {
                    if (p_data->p_le_pa_sync_modify_periodic_adv_list_rsp->operation ==
                        GAP_PA_SYNC_PERIODIC_ADV_LIST_OP_REMOVE)
                    {
                        if (ble_pa_sync_cb.p_pal_op_dev->pending_removal)
                        {
                            if (ble_pa_sync_cb.p_pal_op_dev->pa_sync_state != GAP_PA_SYNC_STATE_SYNCHRONIZED)
                            {
                                ualist_del(&ble_pa_sync_cb.p_pal_op_dev->list);
                                os_mem_free(ble_pa_sync_cb.p_pal_op_dev);
                                ble_pa_sync_cb.pal_added_num--;
                            }
                            else
                            {
                                ble_pa_sync_cb.p_pal_op_dev->in_controller_pal = false;
                            }
                        }
                        else
                        {
                            ble_pa_sync_cb.p_pal_op_dev->in_controller_pal = false;
                        }
                    }
                    else if (p_data->p_le_pa_sync_modify_periodic_adv_list_rsp->operation ==
                             GAP_PA_SYNC_PERIODIC_ADV_LIST_OP_ADD)
                    {
                        ble_pa_sync_cb.p_pal_op_dev->in_controller_pal = true;
                        ble_pa_sync_cb.p_pal_op_dev->in_adding_pal = false;
                    }

                    ble_pa_sync_cb.pal_attempt = 0;
                }
                else
                {
                    ble_pa_sync_cb.pal_attempt++;
                }
            }
            ble_pa_sync_cb.p_pal_op_dev = NULL;
            execute_pal_dev_operation();
        }
        break;

    case GAP_MSG_LE_PA_SYNC_DEV_STATE_CHANGE_INFO:
        {
            ble_handle_pa_dev_state_evt(p_data->p_le_pa_sync_dev_state_change_info->state,
                                        p_data->p_le_pa_sync_dev_state_change_info->cause);
        }
        break;

    case GAP_MSG_LE_PA_SYNC_STATE_CHANGE_INFO:
        {
            bool need_check_sync = false;
            APP_PRINT_INFO4("GAP_MSG_LE_PA_SYNC_STATE_CHANGE_INFO: sync_id %d, sync_handle 0x%x, state %d , terminate_cause 0x%x",
                            p_data->p_le_pa_sync_state_change_info->sync_id,
                            p_data->p_le_pa_sync_state_change_info->sync_handle,
                            (T_GAP_PA_SYNC_STATE)p_data->p_le_pa_sync_state_change_info->state,
                            p_data->p_le_pa_sync_state_change_info->cause);
            T_APP_PA_SYNC_CB *p_pa_dev = NULL;
            if (p_data->p_le_pa_sync_state_change_info->state == GAP_PA_SYNC_STATE_SYNCHRONIZING_SCAN_IDLE)
            {
                p_pa_dev = find_init_pa_sync();
            }
            else if (p_data->p_le_pa_sync_state_change_info->state == GAP_PA_SYNC_STATE_SYNCHRONIZING)
            {
                p_pa_dev = find_pa_by_sync_id(p_data->p_le_pa_sync_state_change_info->sync_id);
                if (p_pa_dev == NULL)
                {
                    p_pa_dev = find_init_pa_sync();
                }
            }
            else
            {
                p_pa_dev = find_pa_by_sync_id(p_data->p_le_pa_sync_state_change_info->sync_id);
            }
            if (p_pa_dev != NULL)
            {
                p_pa_dev->real_sync_id = p_data->p_le_pa_sync_state_change_info->sync_id;
                p_pa_dev->pa_sync_state = (T_GAP_PA_SYNC_STATE)p_data->p_le_pa_sync_state_change_info->state;
                p_pa_dev->app_pa_sync_state = (T_GAP_PA_SYNC_STATE)p_data->p_le_pa_sync_state_change_info->state;
                p_pa_dev->sync_handle = p_data->p_le_pa_sync_state_change_info->sync_handle;

                T_PA_SYNC_STATE_CHANGE_INFO sync_info;
                sync_info.sync_id = p_pa_dev->real_sync_id;
                sync_info.state = p_pa_dev->app_pa_sync_state;
                sync_info.sync_handle = p_pa_dev->sync_handle;
                for (uint8_t i = 0; i < 32; i++)
                {
                    if ((p_pa_dev->doing_direct_sync_set & ((uint32_t)1 << i)) ||
                        (p_pa_dev->doing_bg_sync_set & ((uint32_t)1 << i)))
                    {
                        T_APP_PA_SYNC_CBACK *p_pa_sync_callback = find_pa_sync_app_callback(i);
                        if (p_pa_sync_callback)
                        {
                            p_pa_sync_callback->callback(p_pa_dev->adv_addr_type, p_pa_dev->adv_addr, p_pa_dev->adv_sid,
                                                         p_pa_dev->broadcast_id, &sync_info);
                        }
                    }
                }
                if (p_data->p_le_pa_sync_state_change_info->state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
                {
                    p_pa_dev->app_pa_sync_state = GAP_PA_SYNC_STATE_TERMINATED;
                    if (p_pa_dev->timer_idx_direct_pa_sync)
                    {
                        if (app_stop_timer(&p_pa_dev->timer_idx_direct_pa_sync))
                        {
                            p_pa_dev->time_rank = 0;
                        }
                    }
                }
                else if (p_data->p_le_pa_sync_state_change_info->state == GAP_PA_SYNC_STATE_TERMINATED)
                {
                    if (p_pa_dev->doing_bg_sync_set == 0 &&
                        p_pa_dev->doing_direct_sync_set == 0 &&
                        !p_pa_dev->in_controller_pal)
                    {
                        ualist_del(&p_pa_dev->list);
                        os_mem_free(p_pa_dev);
                        ble_pa_sync_cb.pal_added_num--;
                        p_pa_dev = NULL;
                    }
                    else
                    {
                        APP_PRINT_INFO2("GAP_MSG_LE_PA_SYNC_STATE_CHANGE_INFO: doing_bg_sync_set %d, doing_direct_sync_set 0x%x",
                                        p_pa_dev->doing_bg_sync_set, p_pa_dev->doing_direct_sync_set);
                        if (p_pa_dev->doing_bg_sync_set)
                        {
                            ble_terminate_pa_sync_by_sync_id(6,
                                                             p_data->p_le_pa_sync_state_change_info->sync_id,
                                                             true);
                        }
                        else if (p_pa_dev->doing_direct_sync_set)
                        {
                            ble_terminate_pa_sync_by_sync_id(6,
                                                             p_data->p_le_pa_sync_state_change_info->sync_id,
                                                             false);
                        }
                    }
                }
            }

            if (p_data->p_le_pa_sync_state_change_info->state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
            {
                if (p_data->p_le_pa_sync_state_change_info->sync_id != ble_pa_sync_cb.pa_sync_id)
                {
                    APP_PRINT_ERROR2(" sync_id  mis-match  %d : %d", p_data->p_le_pa_sync_state_change_info->sync_id,
                                     ble_pa_sync_cb.pa_sync_id);
                }
            }

            if (ble_pa_sync_cb.pa_sync_id != GAP_INVALID_SYNC_ID &&
                p_data->p_le_pa_sync_state_change_info->sync_id == ble_pa_sync_cb.pa_sync_id)
            {
                if (p_data->p_le_pa_sync_state_change_info->state == GAP_PA_SYNC_STATE_SYNCHRONIZED ||
                    p_data->p_le_pa_sync_state_change_info->state == GAP_PA_SYNC_STATE_TERMINATED)
                {
                    ble_pa_sync_cb.pa_sync_id = GAP_INVALID_SYNC_ID;
                    ble_pa_sync_cb.pa_sync_state = GAP_PA_SYNC_STATE_TERMINATED;
                    need_check_sync = true;
                }
                else
                {
                    ble_pa_sync_cb.pa_sync_state = (T_GAP_PA_SYNC_STATE)p_data->p_le_pa_sync_state_change_info->state;
                }
            }
            if (need_check_sync)
            {
                ble_handle_pa_synced_or_cancel();
            }
        }
        break;

    case GAP_MSG_LE_PERIODIC_ADV_REPORT_INFO:
        {
            /*APP_PRINT_INFO7("GAP_MSG_LE_PERIODIC_ADV_REPORT_INFO: sync_id %d, sync_handle 0x%x, tx_power %d, rssi %d, cte_type %d, data_status 0x%x, data_len %d",
                                 p_data->p_le_periodic_adv_report_info->sync_id,
                                 p_data->p_le_periodic_adv_report_info->sync_handle,
                                 p_data->p_le_periodic_adv_report_info->tx_power,
                                 p_data->p_le_periodic_adv_report_info->rssi,
                                 p_data->p_le_periodic_adv_report_info->cte_type,
                                 p_data->p_le_periodic_adv_report_info->data_status,
                                 p_data->p_le_periodic_adv_report_info->data_len);*/

            if (p_data->p_le_periodic_adv_report_info->data_len)
            {
                APP_PRINT_INFO2("GAP_MSG_LE_PERIODIC_ADV_REPORT_INFO: data_len %d, data %b",
                                p_data->p_le_periodic_adv_report_info->data_len,
                                TRACE_BINARY(p_data->p_le_periodic_adv_report_info->data_len,
                                             p_data->p_le_periodic_adv_report_info->p_data));
            }
        }
        break;
#if F_BT_LE_5_2_SUPPORT
    case GAP_MSG_LE_BIGINFO_ADV_REPORT_INFO:
        {
            /*APP_PRINT_INFO8("GAP_MSG_LE_BIGINFO_ADV_REPORT_INFO: sync_id %d, sync_handle 0x%x, num_bis %d, nse %d, iso_interval 0x%x, bn %d, pto %d, irc %d",
                                 p_data->p_le_biginfo_adv_report_info->sync_id,
                                 p_data->p_le_biginfo_adv_report_info->sync_handle,
                                 p_data->p_le_biginfo_adv_report_info->num_bis,
                                 p_data->p_le_biginfo_adv_report_info->nse,
                                 p_data->p_le_biginfo_adv_report_info->iso_interval,
                                 p_data->p_le_biginfo_adv_report_info->bn,
                                 p_data->p_le_biginfo_adv_report_info->pto,
                                 p_data->p_le_biginfo_adv_report_info->irc);
            APP_PRINT_INFO8("GAP_MSG_LE_BIGINFO_ADV_REPORT_INFO: sync_id %d, sync_handle 0x%x, max_pdu %d, sdu_interval 0x%x, max_sdu %d, phy %d, framing %d, encryption %d",
                                 p_data->p_le_biginfo_adv_report_info->sync_id,
                                 p_data->p_le_biginfo_adv_report_info->sync_handle,
                                 p_data->p_le_biginfo_adv_report_info->max_pdu,
                                 p_data->p_le_biginfo_adv_report_info->sdu_interval,
                                 p_data->p_le_biginfo_adv_report_info->max_sdu,
                                 p_data->p_le_biginfo_adv_report_info->phy,
                                 p_data->p_le_biginfo_adv_report_info->framing,
                                 p_data->p_le_biginfo_adv_report_info->encryption);*/
        }
        break;
#endif
#endif
#if F_BT_LE_5_1_PAST_RECIPIENT_SUPPORT
    case GAP_MSG_LE_PAST_RECIPIENT_SET_DEFAULT_PERIODIC_ADV_SYNC_TRANSFER_PARAMS:
        APP_PRINT_INFO1("GAP_MSG_LE_PAST_RECIPIENT_SET_DEFAULT_PERIODIC_ADV_SYNC_TRANSFER_PARAMS: cause 0x%x",
                        p_data->p_le_past_recipient_set_default_periodic_adv_sync_transfer_params_rsp->cause);
        break;

    case GAP_MSG_LE_PAST_RECIPIENT_SET_PERIODIC_ADV_SYNC_TRANSFER_PARAMS:
        APP_PRINT_INFO2("GAP_MSG_LE_PAST_RECIPIENT_SET_PERIODIC_ADV_SYNC_TRANSFER_PARAMS: cause 0x%x, conn_id 0x%x",
                        p_data->p_le_past_recipient_set_periodic_adv_sync_transfer_params_rsp->cause,
                        p_data->p_le_past_recipient_set_periodic_adv_sync_transfer_params_rsp->conn_id);
        break;
    case GAP_MSG_LE_PAST_RECIPIENT_PERIODIC_ADV_SYNC_TRANSFER_RECEIVED_INFO:
        APP_PRINT_INFO0("GAP_MSG_LE_PAST_RECIPIENT_PERIODIC_ADV_SYNC_TRANSFER_RECEIVED_INFO");
        break;
#endif
    default:
        break;
    }
}

void ble_big_sync_register_cb_int(P_FUN_LE_BIG_MGR_SYNC_RX_CB cb_pfn)
{
    ble_audio_sync_cb = cb_pfn;
}

bool ble_suspend_pa_sync(void)
{
    T_GAP_CAUSE cause;
    APP_PRINT_TRACE1("ble_suspend_pa_sync  need to complete, pa_sync_state 0x%x",
                     ble_pa_sync_cb.pa_sync_state);
    ble_pa_sync_cb.pl_state |= BLE_PAL_SUSPENDING;

    if (ble_pa_sync_cb.pa_sync_state != GAP_PA_SYNC_STATE_TERMINATED)
    {
        if (ble_pa_sync_cb.pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_SCAN_IDLE ||
            ble_pa_sync_cb.pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_WAIT_SCANNING ||
            ble_pa_sync_cb.pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING)
        {
            if ((cause = le_pa_sync_terminate_sync(ble_pa_sync_cb.pa_sync_id)) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR2("ble_suspend_pa_sync terminate fail sync id %d, cause 0x%x",
                                 ble_pa_sync_cb.pa_sync_id, cause);
            }
        }
        return false;
    }

    return true;
}

void ble_pa_sync_resume_req(void)
{
    APP_PRINT_TRACE0("ble_pa_sync_resume_req  need to complete");
    ble_pa_sync_cb.pl_state &= (~BLE_PAL_SUSPENDING);
    execute_pal_dev_operation();
    return;
}


bool direct_pa_sync_add(uint8_t app_id, uint8_t *adv_addr, uint8_t addr_type, uint8_t adv_sid,
                        uint8_t *broadcast_id)
{
    T_APP_PA_SYNC_CB *p_pa_dev = find_pa_by_adv_info(adv_addr, addr_type, adv_sid);
    bool in_pa_list = false;
    T_PA_SYNC_STATE_CHANGE_INFO sync_info;
    APP_PRINT_INFO2("direct_pa_sync_add app_id %x, adv_addr %s", app_id, TRACE_BDADDR(adv_addr));
    T_APP_PA_SYNC_CBACK *p_pa_sync_callback = find_pa_sync_app_callback(app_id);
    if (p_pa_dev)
    {
        p_pa_dev->pending_removal = false;
    }
    else
    {
        if (ble_pa_sync_cb.pal_added_num > ble_pa_sync_cb.pal_size)
        {
            APP_PRINT_ERROR0("direct_pa_sync_add exceed size ");
            return false;
        }
        p_pa_dev = alloc_pa_dev_node_by_adv_info(adv_addr, addr_type, adv_sid, broadcast_id);
        if (p_pa_dev == NULL)
        {
            APP_PRINT_ERROR0("direct_pa_sync_add alloc fail");
            return false;
        }
        p_pa_dev->app_pa_sync_state = GAP_PA_SYNC_STATE_SYNCHRONIZING;
    }

    if (p_pa_sync_callback)
    {
        sync_info.sync_id = p_pa_dev->real_sync_id;
        sync_info.state = p_pa_dev->app_pa_sync_state;
        sync_info.sync_handle = p_pa_dev->sync_handle;
        p_pa_sync_callback->callback(p_pa_dev->adv_addr_type, p_pa_dev->adv_addr, p_pa_dev->adv_sid,
                                     p_pa_dev->broadcast_id, &sync_info);
    }

    //anyone_connecting
    if (p_pa_dev->doing_bg_sync_set || p_pa_dev->doing_direct_sync_set)
    {
        in_pa_list = true;
    }
    APP_PRINT_INFO2("direct_pa_sync_add, bg_sync %x, direct_sync %x", p_pa_dev->doing_bg_sync_set,
                    p_pa_dev->doing_direct_sync_set);
    p_pa_dev->doing_direct_sync_set |= (1 << app_id);
    if (!in_pa_list)
    {
        execute_pal_dev_operation();
    }
    if (p_pa_dev->timer_idx_direct_pa_sync == 0)
    {
        pa_time_rank = (pa_time_rank + 1) == 0 ? 1 : (pa_time_rank + 1);
        p_pa_dev->time_rank = pa_time_rank;

        app_start_timer(&p_pa_dev->timer_idx_direct_pa_sync, "direct pa sync",
                        ble_pa_sync_cb.pa_sync_timer_id, DIRECT_PA_SYNC_TIMEOUT_ID, pa_time_rank, false,
                        DIRECT_PA_SYNC_TIME_OUT);
    }

    return true;
}

bool bg_pa_sync_add(uint8_t app_id, uint8_t *adv_addr, uint8_t addr_type, uint8_t adv_sid,
                    uint8_t *broadcast_id)
{
    T_APP_PA_SYNC_CB *p_pa_dev = find_pa_by_adv_info(adv_addr, addr_type, adv_sid);
    bool in_pa_list = false;
    T_PA_SYNC_STATE_CHANGE_INFO sync_info;
    T_APP_PA_SYNC_CBACK *p_pa_sync_callback = find_pa_sync_app_callback(app_id);

    APP_PRINT_INFO2("bg_pa_sync_add app_id %x, adv_addr %s", app_id, TRACE_BDADDR(adv_addr));
    if (p_pa_dev)
    {
        p_pa_dev->pending_removal = false;
    }
    else
    {
        if (ble_pa_sync_cb.pal_added_num > ble_pa_sync_cb.pal_size)
        {
            APP_PRINT_ERROR0("bg_pa_sync_add exceed size ");
            return false;
        }

        p_pa_dev = alloc_pa_dev_node_by_adv_info(adv_addr, addr_type, adv_sid, broadcast_id);
        if (p_pa_dev == NULL)
        {
            APP_PRINT_TRACE0("bg_pa_sync_add alloc fail");
            return false;
        }
        p_pa_dev->app_pa_sync_state = GAP_PA_SYNC_STATE_SYNCHRONIZING;
    }

    if (p_pa_sync_callback)
    {
        sync_info.sync_id = p_pa_dev->real_sync_id;
        sync_info.state = p_pa_dev->app_pa_sync_state;
        sync_info.sync_handle = p_pa_dev->sync_handle;
        p_pa_sync_callback->callback(p_pa_dev->adv_addr_type, p_pa_dev->adv_addr, p_pa_dev->adv_sid,
                                     p_pa_dev->broadcast_id, &sync_info);
    }

    //anyone_connecting
    if (p_pa_dev->doing_bg_sync_set || p_pa_dev->doing_direct_sync_set)
    {
        in_pa_list = true;
    }
    APP_PRINT_INFO2("bg_pa_sync_add, bg_sync %x, direct_sync %x", p_pa_dev->doing_bg_sync_set,
                    p_pa_dev->doing_direct_sync_set);
    p_pa_dev->doing_bg_sync_set |= (1 << app_id);
    if (!in_pa_list)
    {
        execute_pal_dev_operation();
    }
    return true;
}

bool ble_register_pa_sync_callback(uint8_t app_id, T_LE_PA_SYNC_CBACK callback)
{
    T_APP_PA_SYNC_CBACK *p_pa_sync_callback = find_pa_sync_app_callback(app_id);
    if (p_pa_sync_callback)
    {
        APP_PRINT_ERROR1("ble_register_pa_sync_callback already register callback %d", app_id);
        return false;
    }

    p_pa_sync_callback = os_mem_zalloc(RAM_TYPE_BT_UAL, sizeof(T_APP_PA_SYNC_CBACK));
    if (p_pa_sync_callback)
    {
        p_pa_sync_callback->callback = callback;
        p_pa_sync_callback->app_id = app_id;
        init_ualist_head(&p_pa_sync_callback->list);
        ualist_add_tail(&p_pa_sync_callback->list, &ble_pa_sync_cb.app_pa_sync_callbacks);
        return true;
    }
    return false;
}


bool ble_pa_sync(uint8_t app_id, uint8_t *address, uint8_t addr_type,
                 uint8_t adv_sid, uint8_t *broadcast_id, bool add_to_pal)
{
    if (add_to_pal)
    {
        return bg_pa_sync_add(app_id, address, addr_type, adv_sid, broadcast_id);
    }
    else
    {
        return direct_pa_sync_add(app_id, address, addr_type, adv_sid, broadcast_id);
    }
}

bool ble_terminate_pa_sync_by_adv_info(int8_t app_id, uint8_t *address, uint8_t addr_type,
                                       uint8_t adv_sid,
                                       bool rm_from_pal)
{
    T_APP_PA_SYNC_CB *p_pa_dev = find_pa_by_adv_info(address, addr_type, adv_sid);
    if (p_pa_dev == NULL)
    {
        APP_PRINT_ERROR0("ble_terminate_pa_sync_by_adv_info can't find PA device");
        return false;
    }

    if (p_pa_dev->doing_direct_sync_set & (1 << app_id))
    {
        p_pa_dev->doing_direct_sync_set &= ~(1 << app_id);
        T_PA_SYNC_STATE_CHANGE_INFO sync_info;
        T_APP_PA_SYNC_CBACK *p_pa_sync_callback = find_pa_sync_app_callback(app_id);
        if (p_pa_sync_callback)
        {
            sync_info.sync_id = p_pa_dev->real_sync_id;
            sync_info.state = GAP_PA_SYNC_STATE_TERMINATED;
            sync_info.sync_handle = p_pa_dev->sync_handle;
            p_pa_sync_callback->callback(p_pa_dev->adv_addr_type, p_pa_dev->adv_addr, p_pa_dev->adv_sid,
                                         p_pa_dev->broadcast_id, &sync_info);
        }

        //any_direct_syncleft
        if (p_pa_dev->doing_direct_sync_set == 0)
        {
            p_pa_dev->app_pa_sync_state = GAP_PA_SYNC_STATE_TERMINATED;
            if (p_pa_dev->timer_idx_direct_pa_sync)
            {
                if (app_stop_timer(&p_pa_dev->timer_idx_direct_pa_sync))
                {
                    p_pa_dev->time_rank = 0;
                }
            }
        }
    }

    if (rm_from_pal)
    {
        p_pa_dev->doing_bg_sync_set &= ~(1 << app_id);
    }

    //anyone_connecting
    if (p_pa_dev->doing_bg_sync_set || p_pa_dev->doing_direct_sync_set)
    {
        return true;
    }

    p_pa_dev->pending_removal = true;
    execute_pal_dev_operation();
    if (p_pa_dev->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
    {
        ble_terminate_pa_sync();
    }
    return true;
}

bool ble_terminate_pa_sync_by_sync_id(uint8_t app_id, uint8_t sync_id, bool rm_from_pal)
{
    T_APP_PA_SYNC_CB *p_pa_dev = find_pa_by_sync_id(sync_id);
    if (p_pa_dev == NULL)
    {
        APP_PRINT_ERROR1("ble_terminate_pa_sync_by_sync_id can't find PA device, sync_id %d", sync_id);
        return false;
    }

    if (p_pa_dev->doing_direct_sync_set & (1 << app_id))
    {
        p_pa_dev->doing_direct_sync_set &= ~(1 << app_id);
        T_PA_SYNC_STATE_CHANGE_INFO sync_info;
        T_APP_PA_SYNC_CBACK *p_pa_sync_callback = find_pa_sync_app_callback(app_id);
        if (p_pa_sync_callback)
        {
            sync_info.sync_id = p_pa_dev->real_sync_id;
            sync_info.state = GAP_PA_SYNC_STATE_TERMINATED;
            sync_info.sync_handle = p_pa_dev->sync_handle;
            p_pa_sync_callback->callback(p_pa_dev->adv_addr_type, p_pa_dev->adv_addr, p_pa_dev->adv_sid,
                                         p_pa_dev->broadcast_id, &sync_info);
        }

        //any_direct_syncleft
        if (p_pa_dev->doing_direct_sync_set == 0)
        {
            p_pa_dev->app_pa_sync_state = GAP_PA_SYNC_STATE_TERMINATED;
            if (p_pa_dev->timer_idx_direct_pa_sync)
            {
                if (app_stop_timer(&p_pa_dev->timer_idx_direct_pa_sync))
                {
                    p_pa_dev->time_rank = 0;
                }
            }
        }
    }

    if (rm_from_pal)
    {
        p_pa_dev->doing_bg_sync_set &= ~(1 << app_id);
    }

    //anyone_connecting
    if (p_pa_dev->doing_bg_sync_set || p_pa_dev->doing_direct_sync_set)
    {
        return true;
    }

    p_pa_dev->pending_removal = true;
    execute_pal_dev_operation();
    if (p_pa_dev->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
    {
        ble_terminate_pa_sync();
    }
    return true;
}

static void pa_sync_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    T_APP_PA_SYNC_CB *p_pa_dev = find_pa_node_by_time_rank(param);
    if (p_pa_dev == NULL)
    {
        APP_PRINT_ERROR1("pa_sync_timeout_cb can't find dev(%d)!", param);
        return;
    }

    APP_PRINT_INFO3("pa_sync_timeout_cb: timer_evt %d param %d, doing_direct_sync_set 0x%x",
                    timer_evt, param, p_pa_dev->doing_direct_sync_set);
    if (timer_evt == DIRECT_PA_SYNC_TIMEOUT_ID)
    {
        for (uint8_t i = 0; i < 32; i++)
        {
            if (p_pa_dev->doing_direct_sync_set & ((uint32_t)1 << i))
            {
                T_PA_SYNC_STATE_CHANGE_INFO sync_info;
                T_APP_PA_SYNC_CBACK *p_pa_sync_callback = find_pa_sync_app_callback(i);
                if (p_pa_sync_callback)
                {
                    sync_info.sync_id = p_pa_dev->real_sync_id;
                    sync_info.state = GAP_PA_SYNC_STATE_TERMINATED;
                    sync_info.sync_handle = p_pa_dev->sync_handle;
                    sync_info.cause = 0xFFFF;
                    p_pa_sync_callback->callback(p_pa_dev->adv_addr_type, p_pa_dev->adv_addr, p_pa_dev->adv_sid,
                                                 p_pa_dev->broadcast_id, &sync_info);
                }
            }
        }
        app_stop_timer(&p_pa_dev->timer_idx_direct_pa_sync);
        p_pa_dev->time_rank = 0;
        p_pa_dev->doing_direct_sync_set = 0;
    }

    //anyone_connecting
    if (p_pa_dev->doing_bg_sync_set == 0)
    {
        p_pa_dev->app_pa_sync_state = GAP_PA_SYNC_STATE_TERMINATED;
        p_pa_dev->pending_removal = true;
        execute_pal_dev_operation();
    }
}


void ble_pa_sync_init(void)
{
    memset(&ble_pa_sync_cb, 0, sizeof(ble_pa_sync_cb));
    ble_pa_sync_cb.scan_id = -1;
    ble_pa_sync_cb.pa_sync_id = GAP_INVALID_SYNC_ID;
    le_pa_sync_init(UAL_MAX_SYNC_HANDLE_NUM);
    le_pa_sync_get_param(PA_SYNC_PARAM_PERIODIC_ADV_LIST_SIZE, &ble_pa_sync_cb.pal_size, 0);
    init_ualist_head(&ble_pa_sync_cb.pal_list);
    init_ualist_head(&ble_pa_sync_cb.app_pa_sync_callbacks);
    app_timer_reg_cb(pa_sync_timeout_cb, &ble_pa_sync_cb.pa_sync_timer_id);

    APP_PRINT_INFO1("ble_pa_sync_init, pal_size %d", ble_pa_sync_cb.pal_size);
}

