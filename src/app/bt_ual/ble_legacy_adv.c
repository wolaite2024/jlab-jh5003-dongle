/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "gap_msg.h"
#include "gap.h"
#include "gap_adv.h"
#include "os_mem.h"
#include "gap_vendor.h"
#include "gap_conn_le.h"
#include "ual_types.h"
#include "ual_list.h"
#include "app_timer.h"
#include "ble_legacy_adv.h"
#include "ble_privacy.h"

#define LEGACY_ADV_HANDLE_NUM_MAX     8

#define BLE_ADV_UDPATE_NONE           0x00
#define BLE_ADV_UDPATE_ADV_DATA       0x01
#define BLE_ADV_UPDATE_SCAN_RSP_DATA  0x02
#define BLE_ADV_UDPATE_ADV_PARAS      0x04
#define BLE_ADV_UDPATE_RANDOM_ADDR    0x08


#define BLE_ADV_NONE_STATE           0x00
#define BLE_ADV_SUSPEND_STATE        0x01

typedef enum
{
    BLE_ADV_PENDING_ACTION_IDLE,
    BLE_ADV_PENDING_ACTION_START,
    BLE_ADV_PENDING_ACTION_STOP,
    BLE_ADV_PENDING_ACTION_UPDATE,
} T_BLE_ADV_PENDING_ACTION;

typedef enum
{
    BLE_ADV_ACTION_IDLE,
    BLE_ADV_ACTION_START,
    BLE_ADV_ACTION_RUNNING,
    BLE_ADV_ACTION_UPDATE,
    BLE_ADV_ACTION_STOP,
    BLE_ADV_ACTION_ONE_SHOT,
} T_BLE_ADV_ACTION;

typedef struct
{
    P_FUN_ADV_APP_CB               app_callback;
    bool                           is_used;
    uint8_t                        adv_handle;
    uint8_t                        adv_update_flag;
    T_BLE_ADV_STATE                tg_state;
    T_BLE_ADV_STATE                state;
    T_BLE_ADV_ACTION               action;
    T_BLE_ADV_STOP_CAUSE           stop_cause;
    uint8_t                        app_cause;
//adv param
    T_GAP_ADTYPE                   adv_type;
    uint16_t                       adv_interval_min;
    uint16_t                       adv_interval_max;
    T_GAP_LOCAL_ADDR_TYPE          own_address_type;
    T_GAP_REMOTE_ADDR_TYPE         peer_address_type;
    uint8_t                        peer_address[GAP_BD_ADDR_LEN];
    T_GAP_ADV_FILTER_POLICY        filter_policy;
//adv data
    uint16_t                       adv_data_len;
    uint8_t                        adv_data[GAP_MAX_ADV_LEN];
    uint16_t                       scan_rsp_data_len;
    uint8_t                        scan_rsp_data[GAP_MAX_ADV_LEN];
//adv enable param
    uint32_t                       duration;
    uint8_t                        timer_idx_legacy_adv;
//remote addr
    bool                           remote_addr_set;
    uint8_t                        remote_addr_type;
    uint8_t                        remote_addr[GAP_BD_ADDR_LEN];
} T_LEGACY_ADV_SET;


typedef struct
{
    T_GAP_DEV_STATE           gap_state;
    T_BLE_ADV_PENDING_ACTION  pending_action;
    T_LEGACY_ADV_SET          *p_current_adv;
    uint8_t                   state;
} T_LEGACY_ADV_CB;


static T_LEGACY_ADV_SET *ble_legacy_adv_set_table;
static T_LEGACY_ADV_CB ble_legacy_adv_cb;
static uint8_t legacy_adv_timer_id;
static void ble_legacy_adv_mgr_check_next_step(void);

static T_LEGACY_ADV_SET *ble_legacy_adv_mgr_find_by_handle(uint8_t adv_handle)
{
    uint8_t i;
    T_LEGACY_ADV_SET *p_adv;

    for (i = 0; i < LEGACY_ADV_HANDLE_NUM_MAX; i++)
    {
        p_adv = &ble_legacy_adv_set_table[i];
        if (p_adv->is_used && (p_adv->adv_handle == adv_handle))
        {
            return p_adv;
        }
    }
    BTM_PRINT_ERROR1("ble_legacy_adv_mgr_find_by_handle: not found adv_handle %d", adv_handle);
    return NULL;
}

static void ble_legacy_adv_mgr_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    T_LEGACY_ADV_SET *p_adv;

    BTM_PRINT_TRACE1("ble_legacy_adv_mgr_timeout_cb: timer_evt %d", timer_evt);
    p_adv = ble_legacy_adv_mgr_find_by_handle(timer_evt);
    if (p_adv != NULL)
    {
        app_stop_timer(&p_adv->timer_idx_legacy_adv);

        if (p_adv->action != BLE_ADV_ACTION_IDLE)
        {
            p_adv->action = BLE_ADV_ACTION_STOP;
            p_adv->stop_cause = BLE_ADV_STOP_CAUSE_TIMEOUT;
            ble_legacy_adv_mgr_check_next_step();
        }
    }
}

static T_LEGACY_ADV_SET *ble_legacy_adv_mgr_allocate(void)
{
    uint8_t i;
    T_LEGACY_ADV_SET *p_adv;

    for (i = 0; i < LEGACY_ADV_HANDLE_NUM_MAX; i++)
    {
        p_adv = &ble_legacy_adv_set_table[i];
        if (!p_adv->is_used)
        {
            p_adv->is_used = true;
            p_adv->adv_handle = i;
            BTM_PRINT_TRACE1("ble_legacy_adv_mgr_allocate: adv_handle %d", p_adv->adv_handle);
            return p_adv;
        }

    }
    BTM_PRINT_ERROR0("ble_adv_mgr_allocate: failed");

    return NULL;
}


static void ble_legacy_adv_mgr_config_adv_param(T_LEGACY_ADV_SET *p_adv)
{
    BTM_PRINT_TRACE2("ble_legacy_adv_mgr_config_adv_param: adv_handle %d, adv_update_flag 0x%02x",
                     p_adv->adv_handle,
                     p_adv->adv_update_flag);
    /* advertising parameters */
    if (p_adv->adv_update_flag & BLE_ADV_UDPATE_ADV_PARAS)
    {
        uint8_t  adv_evt_type = p_adv->adv_type;
        uint8_t  adv_direct_type = p_adv->peer_address_type;
        uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
        uint8_t  adv_filter_policy = p_adv->filter_policy;
        uint16_t adv_int_min = p_adv->adv_interval_min;
        uint16_t adv_int_max = p_adv->adv_interval_max;
        uint8_t  local_bd_type = p_adv->own_address_type;

        memcpy(adv_direct_addr, p_adv->peer_address, GAP_BD_ADDR_LEN);

        /* Set advertising parameters */
        le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
        le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
        le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
        le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
        le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
        le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
        le_adv_set_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, 1, &local_bd_type);
    }
    if (p_adv->adv_update_flag & BLE_ADV_UDPATE_ADV_DATA)
    {
        le_adv_set_param(GAP_PARAM_ADV_DATA, p_adv->adv_data_len, p_adv->adv_data);
    }
    if (p_adv->adv_update_flag & BLE_ADV_UPDATE_SCAN_RSP_DATA)
    {
        le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, p_adv->scan_rsp_data_len, p_adv->scan_rsp_data);
    }
    p_adv->adv_update_flag = BLE_ADV_UDPATE_NONE;
}

static void ble_legacy_adv_mgr_adv_started(T_LEGACY_ADV_SET *p_adv)
{
    p_adv->state = BLE_ADV_STATE_ADVERTISING;
    if (p_adv->app_callback)
    {
        T_BLE_ADV_CB_DATA cb_data;
        T_BLE_ADV_STATE_CHANGE adv_state;
        adv_state.state = BLE_ADV_STATE_ADVERTISING;
        adv_state.stop_cause = BLE_ADV_STOP_CAUSE_UNKNOWN;
        adv_state.app_cause = 0;
        adv_state.adv_handle = p_adv->adv_handle;
        cb_data.p_ble_state_change = &adv_state;
        p_adv->app_callback(BLE_ADV_STATE_CHANGE, &cb_data);
    }
}

static void ble_legacy_adv_mgr_adv_stoped(T_LEGACY_ADV_SET *p_adv, uint8_t conn_id)
{
    if (p_adv == NULL)
    {
        APP_PRINT_ERROR0("ble_legacy_adv_mgr_adv_stoped: p_adv pointer is null");
        return;
    }

    p_adv->state = BLE_ADV_STATE_IDLE;
    p_adv->action = BLE_ADV_ACTION_IDLE;
    if (p_adv->timer_idx_legacy_adv)
    {
        app_stop_timer(&p_adv->timer_idx_legacy_adv);
    }

    if (ble_legacy_adv_cb.state == BLE_ADV_SUSPEND_STATE &&
        p_adv->app_cause == BLE_ADV_STOP_CAUSE_SUSPEND)
    {
#if BLE_PRIVACY_SPT
        handle_adv_stop_in_suspend();
#endif
        return;
    }

    if (p_adv->app_callback)
    {
        T_BLE_ADV_CB_DATA cb_data;
        T_BLE_ADV_STATE_CHANGE adv_state;
        adv_state.state = BLE_ADV_STATE_IDLE;
        adv_state.stop_cause = p_adv->stop_cause;
        adv_state.adv_handle = p_adv->adv_handle;
        adv_state.app_cause = p_adv->app_cause;
        cb_data.p_ble_state_change = &adv_state;
        p_adv->app_callback(BLE_ADV_STATE_CHANGE, &cb_data);
        p_adv->stop_cause = BLE_ADV_STOP_CAUSE_UNKNOWN;
        if (conn_id != 0xFF)
        {
            T_BLE_ADV_SET_CONN_INFO conn_info;
            conn_info.adv_handle = p_adv->adv_handle;
            conn_info.conn_id = conn_id;
            cb_data.p_ble_conn_info = &conn_info;
            p_adv->app_callback(BLE_ADV_SET_CONN_INFO, &cb_data);
        }
    }


}

static void ble_legacy_adv_mgr_check_next_step(void)
{
    BTM_PRINT_TRACE1("ble_legacy_adv_mgr_check_next_step: start pending_action %d",
                     ble_legacy_adv_cb.pending_action);
    T_LEGACY_ADV_SET *p_adv_set = ble_legacy_adv_cb.p_current_adv;
    if (ble_legacy_adv_cb.pending_action != BLE_ADV_PENDING_ACTION_IDLE)
    {
        return;
    }

    BTM_PRINT_TRACE2("ble_legacy_adv_mgr_check_next_step: next step p_adv_set adv_handle %d, action %d",
                     p_adv_set->adv_handle, p_adv_set->action);
    if (p_adv_set->action == BLE_ADV_ACTION_START)
    {
        if (ble_legacy_adv_cb.gap_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            ble_legacy_adv_mgr_config_adv_param(p_adv_set);
            if (le_adv_start() == GAP_SUCCESS)
            {
                ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_START;
            }
        }
    }
    else if (p_adv_set->action == BLE_ADV_ACTION_STOP)
    {
        if (ble_legacy_adv_cb.gap_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
        {
            if (le_adv_stop() == GAP_SUCCESS)
            {
                ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_STOP;
            }
        }
        else if (ble_legacy_adv_cb.gap_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            ble_legacy_adv_mgr_adv_stoped(p_adv_set, 0xff);
        }
    }
    else if (p_adv_set->action == BLE_ADV_ACTION_UPDATE)
    {
        if (ble_legacy_adv_cb.gap_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
        {
            if (p_adv_set->adv_update_flag == BLE_ADV_UDPATE_ADV_DATA)
            {
                ble_legacy_adv_mgr_config_adv_param(p_adv_set);
                if (le_adv_update_param() == GAP_SUCCESS)
                {
                    ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_UPDATE;
                }
            }
            else
            {
                if (le_adv_stop() == GAP_SUCCESS)
                {
                    ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_STOP;
                }
            }
        }
    }
    else if (p_adv_set->action == BLE_ADV_ACTION_RUNNING)
    {
        if (ble_legacy_adv_cb.gap_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            ble_legacy_adv_mgr_config_adv_param(p_adv_set);
            if (le_adv_start() == GAP_SUCCESS)
            {
                ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_START;
            }
        }
    }
}

void ble_legacy_adv_mgr_start(void)
{
    T_LEGACY_ADV_SET *p_adv = ble_legacy_adv_cb.p_current_adv;

    if (p_adv != NULL && p_adv->duration != 0)
    {
        app_start_timer(&p_adv->timer_idx_legacy_adv, "legacy adv timer",
                        legacy_adv_timer_id, p_adv->adv_handle, 0, false,
                        p_adv->duration);
    }

    ble_legacy_adv_mgr_check_next_step();
}


T_GAP_CAUSE ble_legacy_adv_mgr_init()
{
    uint8_t conn_ind = 1;
    memset(&ble_legacy_adv_cb, 0, sizeof(ble_legacy_adv_cb));

    ble_legacy_adv_set_table = (T_LEGACY_ADV_SET *)os_mem_zalloc(RAM_TYPE_BT_UAL,
                                                                 LEGACY_ADV_HANDLE_NUM_MAX * sizeof(T_LEGACY_ADV_SET));
    if (ble_legacy_adv_set_table == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_adv_mgr_init: allocate failed");
        return GAP_CAUSE_NO_RESOURCE;
    }
    le_set_gap_param(GAP_PARAM_HANDLE_CREATE_CONN_IND, sizeof(uint8_t), &conn_ind);

    app_timer_reg_cb(ble_legacy_adv_mgr_timeout_cb, &legacy_adv_timer_id);

    return GAP_CAUSE_SUCCESS;
}

uint8_t ble_legacy_adv_mgr_alloc_adv(P_FUN_ADV_APP_CB app_callback)
{
    T_LEGACY_ADV_SET *p_adv;
    p_adv = ble_legacy_adv_mgr_allocate();
    if (p_adv == NULL)
    {
        return 0xFF;
    }
    p_adv->app_callback = app_callback;
    p_adv->adv_update_flag = BLE_ADV_UDPATE_ADV_DATA | BLE_ADV_UDPATE_ADV_PARAS;
    BTM_PRINT_TRACE1("ble_legacy_adv_mgr_alloc_adv_handle: adv_handle %d", p_adv->adv_handle);
    return p_adv->adv_handle;
}

T_GAP_CAUSE ble_legacy_adv_mgr_remove_adv(uint8_t adv_handle)
{
    T_LEGACY_ADV_SET *p_adv;
    p_adv = ble_legacy_adv_mgr_find_by_handle(adv_handle);
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_adv_mgr_remove_adv: not find");
        return GAP_CAUSE_NOT_FIND;
    }

    if (p_adv->state != BLE_ADV_STATE_IDLE ||
        p_adv->action != BLE_ADV_ACTION_IDLE ||
        p_adv->tg_state != BLE_ADV_STATE_IDLE)
    {
        BTM_PRINT_ERROR2("ble_legacy_adv_mgr_remove_adv: failed, adv(%d : %d) still running",
                         p_adv->adv_handle,
                         p_adv->action);
        return GAP_CAUSE_INVALID_STATE;
    }

    if (p_adv == ble_legacy_adv_cb.p_current_adv)
    {
        ble_legacy_adv_cb.p_current_adv = NULL;
    }

    if (p_adv->timer_idx_legacy_adv)
    {
        app_stop_timer(&p_adv->timer_idx_legacy_adv);
    }

    memset(p_adv, 0, sizeof(T_LEGACY_ADV_SET));
    return GAP_CAUSE_SUCCESS;
}

T_GAP_CAUSE ble_legacy_set_adv_param_by_mask(uint8_t adv_handle, T_ADV_PARAM_CONF conf_msk,
                                             T_ADV_PARAMS *p_param)
{
    T_LEGACY_ADV_SET *p_adv;
    p_adv = ble_legacy_adv_mgr_find_by_handle(adv_handle);
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_set_adv_param_by_mask: not find");
        return GAP_CAUSE_NOT_FIND;
    }
    T_GAP_ADTYPE adv_type = p_adv->adv_type;
    uint16_t adv_interval_min = (conf_msk & ADV_INTERVAL_RANGE_CONF) ? p_param->adv_interval_min :
                                p_adv->adv_interval_min;
    uint16_t adv_interval_max = (conf_msk & ADV_INTERVAL_RANGE_CONF) ? p_param->adv_interval_max :
                                p_adv->adv_interval_max;
    if (adv_interval_max < adv_interval_min)
    {
        return GAP_CAUSE_NOT_FIND;
    }
    T_GAP_LOCAL_ADDR_TYPE own_address_type = (conf_msk & LOCAL_BD_TYPE_CONF) ?
                                             (T_GAP_LOCAL_ADDR_TYPE)p_param->local_bd_type : p_adv->own_address_type;
    T_GAP_ADV_FILTER_POLICY filter_policy = (conf_msk & FILTER_POLICY_CONF) ?
                                            (T_GAP_ADV_FILTER_POLICY)p_param->filter_policy : p_adv->filter_policy;

    if (conf_msk & ADV_EVENT_PROP_CONF)
    {
        if (!(p_param->adv_event_prop & EXT_ADV_EVT_PROP_USE_LEGACY_ADV))
        {
            APP_PRINT_ERROR1("ble_legacy_set_adv_param_by_mask: not Legacy event adv_event_prop %d",
                             p_param->adv_event_prop);
            return GAP_CAUSE_INVALID_PARAM;
        }
        if (p_param->adv_event_prop == LEGACY_ADV_IND_EVT)
        {
            adv_type = GAP_ADTYPE_ADV_IND;
        }
        else if (p_param->adv_event_prop == LEGACY_ADV_DIRECT_IND_LOW_EVT)
        {
            adv_type = GAP_ADTYPE_ADV_LDC_DIRECT_IND;
        }
        else if (p_param->adv_event_prop == LEGACY_ADV_DIRECT_IND_HIGH_EVT)
        {
            adv_type = GAP_ADTYPE_ADV_HDC_DIRECT_IND;
        }
        else if (p_param->adv_event_prop == LEGACY_ADV_SCAN_IND_EVT)
        {
            adv_type = GAP_ADTYPE_ADV_SCAN_IND;
        }
        else if (p_param->adv_event_prop == LEGACY_ADV_NONCONN_IND_EVT)
        {
            adv_type = GAP_ADTYPE_ADV_NONCONN_IND;
        }
        else
        {
            APP_PRINT_ERROR1("ble_legacy_set_adv_param_by_mask:  adv_event_prop %d",
                             p_param->adv_event_prop);
            return GAP_CAUSE_INVALID_PARAM;
        }
    }

    if ((p_adv->adv_type == adv_type) &&
        (p_adv->adv_interval_min == adv_interval_min) &&
        (p_adv->adv_interval_max == adv_interval_max) &&
        (p_adv->own_address_type == own_address_type) &&
        (p_adv->filter_policy == filter_policy))
    {
        if ((adv_type == GAP_ADTYPE_ADV_LDC_DIRECT_IND ||
             adv_type == GAP_ADTYPE_ADV_HDC_DIRECT_IND) &&
            (conf_msk & PEER_ADDR_INFO_CONF))
        {
            if (p_adv->peer_address_type == p_param->peer_bd_type &&
                memcmp(p_adv->peer_address, p_param->peer_addr, GAP_BD_ADDR_LEN) == 0)
            {
                return GAP_CAUSE_SUCCESS;
            }
        }
        else
        {
            return GAP_CAUSE_SUCCESS;
        }
    }

    BTM_PRINT_TRACE6("ble_legacy_set_adv_param_by_mask: adv_handle %d, adv_type %d, adv_interval_min 0x%04x, adv_interval_max 0x%04x, own_address_type %d, filter_policy %d",
                     adv_handle, adv_type, adv_interval_min, adv_interval_max,
                     own_address_type, filter_policy);

    p_adv->adv_type = adv_type;
    p_adv->adv_interval_min = adv_interval_min;
    p_adv->adv_interval_max = adv_interval_max;
    p_adv->own_address_type = own_address_type;
    p_adv->filter_policy = filter_policy;
    if (conf_msk & PEER_ADDR_INFO_CONF)
    {
        p_adv->peer_address_type = (T_GAP_REMOTE_ADDR_TYPE)p_param->peer_bd_type;
        memcpy(p_adv->peer_address, p_param->peer_addr, GAP_BD_ADDR_LEN);
    }

    p_adv->adv_update_flag |= BLE_ADV_UDPATE_ADV_PARAS;
    if (p_adv->action == BLE_ADV_ACTION_RUNNING)
    {
        p_adv->action = BLE_ADV_ACTION_UPDATE;
        ble_legacy_adv_mgr_check_next_step();
    }

    return GAP_CAUSE_SUCCESS;
}

T_GAP_CAUSE ble_legacy_adv_mgr_set_adv_param(uint8_t adv_handle, T_GAP_ADTYPE adv_type,
                                             uint16_t adv_interval_min, uint16_t adv_interval_max,
                                             T_GAP_LOCAL_ADDR_TYPE own_address_type,
                                             T_GAP_REMOTE_ADDR_TYPE peer_address_type, uint8_t *p_peer_address,
                                             T_GAP_ADV_FILTER_POLICY filter_policy)
{
    T_LEGACY_ADV_SET *p_adv;
    p_adv = ble_legacy_adv_mgr_find_by_handle(adv_handle);
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_adv_mgr_set_adv_param: not find");
        return GAP_CAUSE_NOT_FIND;
    }
    if ((p_adv->adv_type == adv_type) &&
        (p_adv->adv_interval_min == adv_interval_min) &&
        (p_adv->adv_interval_max == adv_interval_max) &&
        (p_adv->own_address_type == own_address_type) &&
        (p_adv->peer_address_type == peer_address_type) &&
        (p_adv->filter_policy == filter_policy))
    {
        return GAP_CAUSE_SUCCESS;
    }

    BTM_PRINT_TRACE6("ble_legacy_adv_mgr_set_adv_param: adv_handle %d, adv_type %d, adv_interval_min 0x%04x, adv_interval_max 0x%04x, own_address_type %d, filter_policy %d",
                     adv_handle, adv_type, adv_interval_min, adv_interval_max,
                     own_address_type, filter_policy);

    p_adv->adv_type = adv_type;
    p_adv->adv_interval_min = adv_interval_min;
    p_adv->adv_interval_max = adv_interval_max;
    p_adv->own_address_type = own_address_type;
    p_adv->peer_address_type = peer_address_type;
    p_adv->filter_policy = filter_policy;

    if (p_peer_address != NULL)
    {
        memcpy(p_adv->peer_address, p_peer_address, GAP_BD_ADDR_LEN);
    }

    p_adv->adv_update_flag |= BLE_ADV_UDPATE_ADV_PARAS;
    if (p_adv->action == BLE_ADV_ACTION_RUNNING)
    {
        p_adv->action = BLE_ADV_ACTION_UPDATE;
        ble_legacy_adv_mgr_check_next_step();
    }

    return GAP_CAUSE_SUCCESS;
}

T_GAP_CAUSE ble_legacy_adv_mgr_set_adv_data(uint8_t adv_handle, uint16_t adv_data_len,
                                            uint8_t *p_adv_data)
{
    T_LEGACY_ADV_SET *p_adv;
    p_adv = ble_legacy_adv_mgr_find_by_handle(adv_handle);
    BTM_PRINT_TRACE3("ble_legacy_adv_mgr_set_adv_data: adv_handle %d, adv_data_len %d, p_adv_data %p",
                     adv_handle, adv_data_len, p_adv_data);
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_adv_mgr_set_adv_data: not find");
        return GAP_CAUSE_NOT_FIND;
    }
    if (adv_data_len > GAP_MAX_ADV_LEN || p_adv_data == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_adv_mgr_set_adv_data: Invalid param");
        return GAP_CAUSE_INVALID_PARAM;
    }
    if (p_adv_data != NULL && (p_adv->adv_data_len != adv_data_len  ||
                               memcmp(p_adv->adv_data, p_adv_data, adv_data_len) != 0))
    {
        p_adv->adv_data_len = adv_data_len;
        memcpy(p_adv->adv_data, p_adv_data, adv_data_len);
        p_adv->adv_update_flag |= BLE_ADV_UDPATE_ADV_DATA;

    }

    if ((p_adv->adv_update_flag & BLE_ADV_UDPATE_ADV_DATA) == 0)
    {
        return GAP_CAUSE_SUCCESS;
    }

    if (p_adv->action == BLE_ADV_ACTION_RUNNING)
    {
        p_adv->action = BLE_ADV_ACTION_UPDATE;
        ble_legacy_adv_mgr_check_next_step();
    }

    return GAP_CAUSE_SUCCESS;
}

T_GAP_CAUSE ble_legacy_adv_mgr_set_scan_rsp_data(uint8_t adv_handle, uint16_t scan_rsp_data_len,
                                                 uint8_t *p_scan_rsp_data)
{
    T_LEGACY_ADV_SET *p_adv;
    p_adv = ble_legacy_adv_mgr_find_by_handle(adv_handle);
    BTM_PRINT_TRACE3("ble_legacy_adv_mgr_set_scan_rsp_data: adv_handle %d, scan_rsp_data_len %d, p_scan_rsp_data %p",
                     adv_handle, scan_rsp_data_len, p_scan_rsp_data);
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_adv_mgr_set_scan_rsp_data: not find");
        return GAP_CAUSE_NOT_FIND;
    }
    if (scan_rsp_data_len > GAP_MAX_ADV_LEN || p_scan_rsp_data == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_adv_mgr_set_scan_rsp_data: Invalid param");
        return GAP_CAUSE_INVALID_PARAM;
    }

    if (p_scan_rsp_data != NULL && (p_adv->scan_rsp_data_len != scan_rsp_data_len  ||
                                    memcmp(p_adv->scan_rsp_data, p_scan_rsp_data, scan_rsp_data_len) != 0))
    {
        p_adv->scan_rsp_data_len = scan_rsp_data_len;
        memcpy(p_adv->scan_rsp_data, p_scan_rsp_data, scan_rsp_data_len);
        p_adv->adv_update_flag |= BLE_ADV_UPDATE_SCAN_RSP_DATA;

    }

    if ((p_adv->adv_update_flag & BLE_ADV_UPDATE_SCAN_RSP_DATA) == 0)
    {
        return GAP_CAUSE_SUCCESS;
    }

    if (p_adv->action == BLE_ADV_ACTION_RUNNING)
    {
        p_adv->action = BLE_ADV_ACTION_UPDATE;
        ble_legacy_adv_mgr_check_next_step();
    }

    return GAP_CAUSE_SUCCESS;
}



T_GAP_CAUSE ble_legacy_adv_mgr_enable(uint8_t adv_handle, uint32_t duration)
{
    T_LEGACY_ADV_SET *p_adv;
    BTM_PRINT_INFO2("ble_legacy_adv_mgr_enable: adv_handle %d, duration %d",
                    adv_handle, duration);
    p_adv = ble_legacy_adv_mgr_find_by_handle(adv_handle);
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_adv_mgr_enable: not find");
        return GAP_CAUSE_NOT_FIND;
    }

    if (ble_legacy_adv_cb.p_current_adv != NULL &&
        p_adv != ble_legacy_adv_cb.p_current_adv)
    {
        if (ble_legacy_adv_cb.p_current_adv->state != BLE_ADV_STATE_IDLE ||
            ble_legacy_adv_cb.p_current_adv->action != BLE_ADV_ACTION_IDLE)
        {
            BTM_PRINT_ERROR2("ble_legacy_adv_mgr_enable: failed, already has adv(%d : %d) running",
                             ble_legacy_adv_cb.p_current_adv->adv_handle,
                             ble_legacy_adv_cb.p_current_adv->action);
            return GAP_CAUSE_INVALID_STATE;
        }
    }

    p_adv->duration = duration;
    ble_legacy_adv_cb.p_current_adv = p_adv;
    p_adv->tg_state = BLE_ADV_STATE_ADVERTISING;
    ble_legacy_adv_mgr_adv_started(p_adv);
    if (ble_legacy_adv_cb.state == BLE_ADV_SUSPEND_STATE)
    {
        return GAP_CAUSE_SUCCESS;
    }
    p_adv->action = BLE_ADV_ACTION_START;
    ble_legacy_adv_mgr_start();

    return GAP_CAUSE_SUCCESS;
}

T_GAP_CAUSE ble_legacy_adv_mgr_disable(uint8_t adv_handle, uint8_t app_cause)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    T_LEGACY_ADV_SET *p_adv;
    BTM_PRINT_INFO1("ble_legacy_adv_mgr_disable: adv_handle %d", adv_handle);
    p_adv = ble_legacy_adv_mgr_find_by_handle(adv_handle);
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_legacy_adv_mgr_disable: not find");
        return GAP_CAUSE_NOT_FIND;
    }

    if (p_adv != ble_legacy_adv_cb.p_current_adv)
    {
        BTM_PRINT_ERROR1("ble_legacy_adv_mgr_disable: adv_handle %d is not current adv", p_adv->adv_handle);
        return GAP_CAUSE_INVALID_STATE;
    }

    p_adv->tg_state = BLE_ADV_STATE_IDLE;
    p_adv->action = BLE_ADV_ACTION_STOP;
    p_adv->stop_cause = BLE_ADV_STOP_CAUSE_APP;
    p_adv->app_cause = app_cause;

    ble_legacy_adv_mgr_check_next_step();
    return ret;
}

static void ble_legacy_adv_mgr_pend_action_for_state_change(T_GAP_DEV_STATE new_state,
                                                            uint16_t cause)
{
    BTM_PRINT_INFO2("ble_legacy_adv_mgr_pend_action_for_state_change start: gap_adv_state %d, pending_action %d",
                    new_state.gap_adv_state, ble_legacy_adv_cb.pending_action);
    if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
    {
        if (new_state.gap_adv_sub_state != GAP_ADV_TO_IDLE_CAUSE_CONN)
        {
            if (ble_legacy_adv_cb.pending_action == BLE_ADV_PENDING_ACTION_STOP)
            {
                ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_IDLE;
                if (ble_legacy_adv_cb.p_current_adv->action == BLE_ADV_ACTION_STOP)
                {
                    T_LEGACY_ADV_SET *p_adv = ble_legacy_adv_cb.p_current_adv;
                    ble_legacy_adv_mgr_adv_stoped(p_adv, 0xff);
                }
                else if (ble_legacy_adv_cb.p_current_adv->action != BLE_ADV_ACTION_IDLE)
                {
                    ble_legacy_adv_mgr_config_adv_param(ble_legacy_adv_cb.p_current_adv);
                    if (le_adv_start() == GAP_SUCCESS)
                    {
                        ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_START;
                    }
                }
            }
        }
    }
    else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
    {
        if (ble_legacy_adv_cb.pending_action == BLE_ADV_PENDING_ACTION_START)
        {
            ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_IDLE;
            if (ble_legacy_adv_cb.p_current_adv->action == BLE_ADV_ACTION_START)
            {
                ble_legacy_adv_cb.p_current_adv->adv_update_flag = BLE_ADV_UDPATE_NONE;
                ble_legacy_adv_cb.p_current_adv->action = BLE_ADV_ACTION_RUNNING;
            }
            else if (ble_legacy_adv_cb.p_current_adv->action == BLE_ADV_ACTION_STOP)
            {
                if (le_adv_stop() == GAP_SUCCESS)
                {
                    ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_STOP;
                }
            }
            else if (ble_legacy_adv_cb.p_current_adv->action == BLE_ADV_ACTION_UPDATE)
            {
                if (ble_legacy_adv_cb.p_current_adv->adv_update_flag == BLE_ADV_UDPATE_NONE)
                {
                    ble_legacy_adv_cb.p_current_adv->action = BLE_ADV_ACTION_RUNNING;
                }
            }

        }
    }
    if (ble_legacy_adv_cb.pending_action == BLE_ADV_PENDING_ACTION_IDLE)
    {
        ble_legacy_adv_cb.p_current_adv = NULL;
        ble_legacy_adv_mgr_check_next_step();
    }
    BTM_PRINT_INFO1("ble_legacy_adv_mgr_pend_action_for_state_change end: pending_action %d",
                    ble_legacy_adv_cb.pending_action);
}

static void ble_legacy_adv_mgr_pend_action_for_update_param(uint16_t cause)
{
    BTM_PRINT_INFO1("ble_legacy_adv_mgr_pend_action_for_update_param start: pending_action %d",
                    ble_legacy_adv_cb.pending_action);
    if (ble_legacy_adv_cb.pending_action == BLE_ADV_PENDING_ACTION_UPDATE)
    {
        ble_legacy_adv_cb.pending_action = BLE_ADV_PENDING_ACTION_IDLE;
        if (cause == GAP_SUCCESS)
        {
            if (ble_legacy_adv_cb.p_current_adv->action == BLE_ADV_ACTION_UPDATE)
            {
                ble_legacy_adv_cb.p_current_adv->action = BLE_ADV_ACTION_RUNNING;
            }
        }
        else
        {
            BTM_PRINT_ERROR1("ble_legacy_adv_mgr_pend_action_for_update_param: failed cause 0x%04x", cause);
        }
    }
    else
    {
        BTM_PRINT_ERROR1("ble_legacy_adv_mgr_pend_action_for_update_param: Invalid pending_action %d",
                         ble_legacy_adv_cb.pending_action);
    }
    if (ble_legacy_adv_cb.pending_action == BLE_ADV_PENDING_ACTION_IDLE)
    {
        ble_legacy_adv_mgr_check_next_step();
    }
    BTM_PRINT_INFO1("ble_legacy_adv_mgr_pend_action_for_update_param: end pending_action %d",
                    ble_legacy_adv_cb.pending_action);
}

void ble_legacy_adv_mgr_handle_adv_state(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    T_GAP_DEV_STATE old_state = ble_legacy_adv_cb.gap_state;
    BTM_PRINT_TRACE4("ble_legacy_adv_mgr_handle_adv_state: old gap_adv_state %d, new_state.gap_adv_state %d, gap_adv_sub_state %d, cause 0x%04x",
                     ble_legacy_adv_cb.gap_state.gap_adv_state, new_state.gap_adv_state,
                     new_state.gap_adv_sub_state, cause);
    ble_legacy_adv_cb.gap_state = new_state;
    if (old_state.gap_adv_state != new_state.gap_adv_state)
    {
        if (ble_legacy_adv_cb.pending_action != BLE_ADV_PENDING_ACTION_IDLE)
        {
            ble_legacy_adv_mgr_pend_action_for_state_change(new_state, cause);
        }
        if ((new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
            && (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN)
            && (ble_legacy_adv_cb.p_current_adv != NULL))
        {
            uint8_t conn_id = 0xff;
            ble_legacy_adv_cb.p_current_adv->stop_cause = BLE_ADV_STOP_CAUSE_CONN;
            if (ble_legacy_adv_cb.p_current_adv->remote_addr_set)
            {
                le_get_conn_id(ble_legacy_adv_cb.p_current_adv->remote_addr,
                               ble_legacy_adv_cb.p_current_adv->remote_addr_type, &conn_id);
            }
            if (conn_id == 0xff)
            {
                BTM_PRINT_ERROR0("ble_legacy_adv_mgr_handle_adv_state: GAP_ADV_TO_IDLE_CAUSE_CONN not find link");
            }
            ble_legacy_adv_mgr_adv_stoped(ble_legacy_adv_cb.p_current_adv, conn_id);
        }
    }
}

T_APP_RESULT ble_legacy_adv_mgr_handle_gap_callback(uint8_t cb_type, T_LE_CB_DATA *p_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    if (cb_type == GAP_MSG_LE_ADV_UPDATE_PARAM)
    {
        BTM_PRINT_TRACE1("ble_legacy_adv_mgr_handle_gap_callback: GAP_MSG_LE_ADV_UPDATE_PARAM cause 0x%04x",
                         p_data->p_le_adv_update_param_rsp->cause);
        ble_legacy_adv_mgr_pend_action_for_update_param(p_data->p_le_adv_update_param_rsp->cause);

    }
    else if (cb_type == GAP_MSG_LE_CREATE_CONN_IND)
    {
        BTM_PRINT_TRACE2("ble_legacy_adv_mgr_handle_gap_callback: GAP_MSG_LE_CREATE_CONN_IND bd_addr %s, remote_addr_type %d",
                         TRACE_BDADDR(p_data->p_le_create_conn_ind->bd_addr),
                         p_data->p_le_create_conn_ind->remote_addr_type);
        if (ble_legacy_adv_cb.p_current_adv)
        {
            ble_legacy_adv_cb.p_current_adv->remote_addr_set = true;
            ble_legacy_adv_cb.p_current_adv->remote_addr_type = p_data->p_le_create_conn_ind->remote_addr_type;
            memcpy(ble_legacy_adv_cb.p_current_adv->remote_addr, p_data->p_le_create_conn_ind->bd_addr, 6);
        }
        result = APP_RESULT_ACCEPT;
    }
    return result;
}

bool ble_legacy_adv_mgr_suspend(void)
{
    ble_legacy_adv_cb.state = BLE_ADV_SUSPEND_STATE;
    if (ble_legacy_adv_cb.p_current_adv)
    {
        if (ble_legacy_adv_cb.p_current_adv->tg_state == BLE_ADV_STATE_ADVERTISING)
        {
            ble_legacy_adv_cb.p_current_adv->action = BLE_ADV_ACTION_STOP;
            ble_legacy_adv_cb.p_current_adv->stop_cause = BLE_ADV_STOP_CAUSE_SUSPEND;
            ble_legacy_adv_mgr_check_next_step();
            return false;
        }
        else
        {
            if (ble_legacy_adv_cb.p_current_adv->action != BLE_ADV_ACTION_IDLE ||
                ble_legacy_adv_cb.p_current_adv->state != BLE_ADV_STATE_IDLE)
            {
                return false;
            }
        }
    }
    return true;
}

bool ble_legacy_adv_mgr_resume(void)
{
    ble_legacy_adv_cb.state = BLE_ADV_NONE_STATE;
    if (ble_legacy_adv_cb.p_current_adv)
    {
        if (ble_legacy_adv_cb.p_current_adv->tg_state == BLE_ADV_STATE_ADVERTISING)
        {
            ble_legacy_adv_cb.p_current_adv->action = BLE_ADV_ACTION_START;
            ble_legacy_adv_mgr_check_next_step();
        }
    }
    return true;
}

