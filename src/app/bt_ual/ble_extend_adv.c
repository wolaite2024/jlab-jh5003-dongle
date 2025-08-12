/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "ual_upperstack_cfg.h"
#include "ble_extend_adv.h"
#include "os_msg.h"
#include "gap_conn_le.h"
#include "bt_types.h"
#include "gap_bond_le.h"
#include "os_queue.h"
#include "ble_privacy.h"
#include "app_timer.h"
#include "gap_pa_adv.h"


#define UAL_MAX_PA_ADV_SET_NUM                    4

#define EXT_ADV_RPA_TIMEOUT   (15 * 60 * 1000)

#define APP_IDLE_ADV_SET 0xFF
#define APP_IDLE_UPDATE_FLAGS    0xFF
#define APP_STOP_FOR_UPDATE 0xFF
#define MAX_NUM_OF_EXTEND_ADV_EVENT       0x64
#define BLE_EXTEND_ADV_NUM_MAX            7

typedef enum
{
    BLE_EXT_ADV_ACTION_IDLE,
    BLE_EXT_ADV_ACTION_START,
    BLE_EXT_ADV_ACTION_UPDATE,
    BLE_EXT_ADV_ACTION_STOP,
    BLE_EXT_ADV_ACTION_REMOVE,
    BLE_EXT_ADV_ACTION_PA_START,
    BLE_EXT_ADV_ACTION_PA_UPDATE,
    BLE_EXT_ADV_ACTION_PA_STOP,
    BLE_EXT_ADV_ACTION_BIG_CREATE,
    BLE_EXT_ADV_ACTION_BIG_TERMINATE,
} T_BLE_EXTEND_ADV_ACTION;

typedef struct t_ext_adv_event
{
    struct t_ext_adv_event    *p_next;
    uint8_t                   adv_handle;
    T_BLE_EXTEND_ADV_ACTION   expected_action;
    uint16_t                  param;
} T_EXT_ADV_EVENT;

typedef enum
{
    BLE_EXT_ADV_PENDING_ACTION_IDLE,
    BLE_EXT_ADV_PENDING_ACTION_START,
    BLE_EXT_ADV_PENDING_ACTION_UPDATE,
    BLE_EXT_ADV_PENDING_ACTION_STOP,
    BLE_EXT_ADV_PENDING_ACTION_REMOVE,
    BLE_EXT_ADV_PENDING_ACTION_PA_START,
    BLE_EXT_ADV_PENDING_ACTION_PA_UPDATE,
    BLE_EXT_ADV_PENDING_ACTION_PA_STOP,
    BLE_EXT_ADV_PENDING_ACTION_BIG_CREATING,
} T_BLE_EXT_ADV_PENDING_ACTION;

typedef struct
{
    P_FUN_ADV_APP_CB                         app_callback;
    uint8_t                                  timer_idx_rpa;

    bool                                     is_used;
    uint8_t                                  adv_handle;
    uint8_t                                  adv_sid;

    bool                                     adv_param_set;
    uint8_t                                  pending_start_stop_num;
    uint16_t                                 pending_cause;
    uint8_t                                  init_flags;
    T_BLE_ADV_STATE                          ble_ext_adv_mgr_adv_state;
    uint8_t                                  app_cause;
    uint8_t                                  pending_action;

    T_GAP_EXT_ADV_STATE                      ext_adv_state;
    T_GAP_EXT_ADV_STATE                      state_before_suspend;

    //adv param
    uint16_t                                 adv_event_prop;
    uint16_t                                 adv_interval_min;
    uint16_t                                 adv_interval_max;
    T_GAP_LOCAL_ADDR_TYPE                    own_address_type;
    T_GAP_REMOTE_ADDR_TYPE                   peer_address_type;
    uint8_t                                  peer_address[6];
    T_GAP_ADV_FILTER_POLICY                  filter_policy;
    uint16_t                                 duration;

    // Range: -127 to +20,  Units: dBm,  +127:Host has no preference
    int8_t                                   adv_tx_power;

    //adv data
    uint16_t                                 adv_data_len;
    uint8_t                                  *p_adv_data;

    //scan response data
    uint16_t                                 scan_response_data_len;
    uint8_t                                  *p_scan_response_data;

    //random addr
    uint8_t                                  random_addr[GAP_BD_ADDR_LEN];

    //pa param
    uint16_t                                 periodic_adv_interval_min;
    uint16_t                                 periodic_adv_interval_max;
    uint16_t                                 periodic_adv_prop;
    uint16_t                                 periodic_adv_data_len;
    uint8_t                                  *p_periodic_adv_data;

    T_BLE_ADV_STATE                          ble_pa_state;
    T_GAP_PA_ADV_STATE                       pa_state;

    //big
    bool                                     big_used;
    uint8_t                                  big_handle;
    T_BIG_PARAM                              big_param;
    bool                                     big_test_mode;
    T_BIG_TEST_PARAM                         big_test_param;
    P_ISOC_BROADCAST_CB                      big_callback;

} T_EXT_ADV_CB;

#define BLE_EXT_NONE_STATE                0x00
#define BLE_EXT_WAIT4PENDING_STATE        0x01        //wait for pending action complete to suspend
#define BLE_EXT_SUSPENDING_STATE          0x02        //disabling all adv
#define BLE_EXT_SUSPEND_STATE             0x03        //all adv disabled
#define BLE_EXT_SPDING2RSM_STATE          0x04        //wait for suspending complete and start resume
#define BLE_EXT_RESUME_STATE              0x05

typedef struct
{
    uint8_t   state;
    uint8_t   max_num_of_adv_sets;
    uint16_t  max_ext_adv_len;
} T_EXT_ADV_MGR_CB;


static T_EXT_ADV_MGR_CB ext_adv_mgr;
static uint8_t ext_rpa_timer_id = 0;
static void ext_adv_start_rpa_timer(T_EXT_ADV_CB *p_adv);
static void ext_adv_stop_rpa_timer(T_EXT_ADV_CB *p_adv);
static void ble_ext_adv_mgr_handle_next_step(void);
static T_GAP_CAUSE ble_ext_adv_mgr_update_adv_param(uint8_t adv_handle, T_EXT_ADV_CB *p_adv,
                                                    uint8_t update_flags);
static T_GAP_CAUSE ble_ext_adv_mgr_set_random_addr(uint8_t adv_handle, uint8_t *random_address);
#if (PA_FEATURE_SUPPORT == 1)
static T_GAP_CAUSE ble_ext_adv_start_pa_action(T_EXT_ADV_CB *p_adv);
static T_GAP_CAUSE ble_ext_adv_stop_pa_action(T_EXT_ADV_CB *p_adv, uint8_t cause);
static T_GAP_CAUSE ble_ext_adv_update_pa_action(T_EXT_ADV_CB *p_adv, uint8_t update_flags);
static void ble_adv_update_pa_state(T_EXT_ADV_CB *p_adv, T_BLE_ADV_STOP_CAUSE stop_cause,
                                    T_BLE_ADV_STATE new_state, uint8_t app_cause);
#endif
/**< advertising set table */
static T_EXT_ADV_CB *ble_ext_adv_set_table;
/**< extend adv event queue handle, used to store Asynchronous action */
static T_OS_QUEUE extend_adv_event_handle;
#if F_BT_LE_5_2_SUPPORT
static P_FUN_LE_BIG_MGR_ISOC_BC_CB big_iso_cb = NULL;
#endif
static T_EXT_ADV_CB *ble_ext_adv_mgr_find_by_adv_handle(uint8_t adv_handle);
static bool ble_ext_adv_mgr_is_adv_action_pending(void);
static uint8_t check_suspend_or_resume_state(void);


static void ble_ext_adv_queue_init()
{
    os_queue_init(&extend_adv_event_handle);
}

static T_GAP_CAUSE send_msg_2_queue(uint8_t adv_handle, T_BLE_EXTEND_ADV_ACTION action,
                                    uint16_t param)
{
    T_EXT_ADV_EVENT *p_extend_adv_event;

    if (extend_adv_event_handle.count > MAX_NUM_OF_EXTEND_ADV_EVENT)
    {
        BTM_PRINT_ERROR0("send_msg_2_queue: extend adv queue full");
        return GAP_CAUSE_NO_RESOURCE;
    }

    p_extend_adv_event = (T_EXT_ADV_EVENT *)os_mem_zalloc(RAM_TYPE_BT_UAL,
                                                          sizeof(T_EXT_ADV_EVENT));
    if (p_extend_adv_event == NULL)
    {
        BTM_PRINT_ERROR0("send_msg_2_queue: alloc fail");
        return GAP_CAUSE_NO_RESOURCE;
    }
    p_extend_adv_event->adv_handle = adv_handle;
    p_extend_adv_event->expected_action = action;
    p_extend_adv_event->param = param;

    BTM_PRINT_INFO3("send_msg_2_queue: adv_handle %d, action %d, param 0x%x",
                    adv_handle, action, param);
    os_queue_in(&extend_adv_event_handle, p_extend_adv_event);

    T_EXT_ADV_CB *p_adv;
    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);
    if (p_adv != NULL &&
        (action == BLE_EXT_ADV_ACTION_START || action == BLE_EXT_ADV_ACTION_STOP))
    {
        p_adv->pending_start_stop_num++;

    }

    return GAP_CAUSE_SUCCESS;
}

static void ble_adv_update_ext_adv_state(T_EXT_ADV_CB *p_adv, T_BLE_ADV_STOP_CAUSE stop_cause,
                                         T_BLE_ADV_STATE new_state, uint8_t app_cause)
{
    if (p_adv->ble_ext_adv_mgr_adv_state != new_state)
    {
        p_adv->ble_ext_adv_mgr_adv_state = new_state;
        if (p_adv->app_callback)
        {
            T_BLE_ADV_CB_DATA cb_data;
            T_BLE_ADV_STATE_CHANGE adv_state;
            adv_state.state = p_adv->ble_ext_adv_mgr_adv_state;
            adv_state.stop_cause = stop_cause;
            adv_state.adv_handle = p_adv->adv_handle;
            adv_state.app_cause = app_cause;
            cb_data.p_ble_state_change = &adv_state;
            p_adv->app_callback(BLE_ADV_STATE_CHANGE, &cb_data);
        }
    }
}

static T_GAP_CAUSE ble_ext_adv_start_action(T_EXT_ADV_CB *p_adv, uint16_t duration)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    ret = le_ext_adv_set_adv_enable_param(p_adv->adv_handle, duration, 0);

    if (ret == GAP_CAUSE_SUCCESS)
    {
        ret = le_ext_adv_enable(1, &p_adv->adv_handle);
    }

    BTM_PRINT_INFO2("ble_ext_adv_start_action: adv_handle %d, duration %d",
                    p_adv->adv_handle, duration);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR1("ble_ext_adv_start_action: failed, ret 0x%x", ret);
        if (p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_ADVERTISING)
        {
            ble_adv_update_ext_adv_state(p_adv, BLE_ADV_STOP_CAUSE_ENABLE_FAILED,
                                         BLE_ADV_STATE_IDLE, 0);
        }
    }
    else
    {
        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_START;
    }
    return ret;
}

static T_GAP_CAUSE ble_ext_adv_stop_action(T_EXT_ADV_CB *p_adv, uint8_t cause)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    ret = le_ext_adv_disable(1, &p_adv->adv_handle);

    BTM_PRINT_INFO1("ble_ext_adv_stop_action: adv_handle %d", p_adv->adv_handle);
    p_adv->app_cause = cause;

    if (ret != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR1("ble_ext_adv_stop_action: failed, ret 0x%x", ret);
    }
    else
    {
        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_STOP;
    }
    return ret;
}

static T_GAP_CAUSE ble_ext_adv_remove_action(T_EXT_ADV_CB *p_adv)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    ret = le_ext_adv_remove_set(p_adv->adv_handle);

    BTM_PRINT_INFO1("ble_ext_adv_remove_action: adv_handle %d", p_adv->adv_handle);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR1("ble_ext_adv_remove_action: failed, ret 0x%x", ret);
    }
    else
    {
        ext_adv_stop_rpa_timer(p_adv);
        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_REMOVE;
    }
    return ret;
}


static T_GAP_CAUSE ble_ext_adv_update_action(T_EXT_ADV_CB *p_adv, uint8_t update_flags)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    if (p_adv->adv_param_set == false)
    {
        update_flags |= p_adv->init_flags;
        update_flags |= EXT_ADV_SET_ADV_PARAS;
    }
    ret = le_ext_adv_start_setting(p_adv->adv_handle, update_flags);

    BTM_PRINT_INFO3("ble_ext_adv_update_action: adv_handle %d, adv_param_set %d, update_flags 0x%x",
                    p_adv->adv_handle, p_adv->adv_param_set, update_flags);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR1("ble_ext_adv_update_action: failed, ret 0x%x",
                         ret);
    }
    else
    {
        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_UPDATE;
        if (update_flags & EXT_ADV_SET_ADV_PARAS)
        {
            p_adv->adv_param_set = true;
            p_adv->init_flags = 0;
        }
    }
    return ret;
}

static void ext_adv_start_rpa_timer(T_EXT_ADV_CB *p_adv)
{
    if (p_adv->own_address_type == GAP_LOCAL_ADDR_LE_PUBLIC ||
        p_adv->own_address_type == GAP_LOCAL_ADDR_LE_RAP_OR_PUBLIC)
    {
        BTM_PRINT_ERROR1("ext_adv_start_rpa_timer: invalid local address type %d",
                         p_adv->own_address_type);
        return;
    }

    app_start_timer(&p_adv->timer_idx_rpa, "ext adv rpa",
                    ext_rpa_timer_id, p_adv->adv_handle, 0, false,
                    EXT_ADV_RPA_TIMEOUT);
}

static void ext_adv_stop_rpa_timer(T_EXT_ADV_CB *p_adv)
{
    if (p_adv->timer_idx_rpa)
    {
        app_stop_timer(&p_adv->timer_idx_rpa);
    }
}

static void ext_adv_rpa_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    T_EXT_ADV_CB *p_adv;
    uint8_t random_addr[GAP_BD_ADDR_LEN];

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(timer_evt);
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ext_adv_rpa_timeout_cb: timer_evt not created");
        return;
    }

    le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, random_addr);
    if (ble_ext_adv_mgr_set_random_addr(timer_evt, random_addr) == GAP_CAUSE_SUCCESS)
    {
        if (p_adv->app_callback)
        {
            T_BLE_ADV_CB_DATA cb_data;
            T_BLE_ADV_RPA_CHANGE rpa_change;
            rpa_change.state = p_adv->ble_pa_state;
            rpa_change.adv_handle = p_adv->adv_handle;
            memcpy(rpa_change.rpa, random_addr, BD_ADDR_LEN);
            cb_data.p_ble_rpa_change = &rpa_change;
            p_adv->app_callback(BLE_ADV_RPA_CHANGE, &cb_data);
        }
    }

    ext_adv_start_rpa_timer(p_adv);
}


T_GAP_CAUSE ble_extend_adv_mgr_init()
{
    BTM_PRINT_TRACE1("ble_extend_adv_mgr_init: support adv handle num %d",
                     BLE_EXTEND_ADV_NUM_MAX);
    le_ext_adv_get_param(GAP_PARAM_EXT_ADV_MAX_DATA_LEN, &ext_adv_mgr.max_ext_adv_len);
    le_ext_adv_get_param(GAP_PARAM_EXT_ADV_MAX_SETS, &ext_adv_mgr.max_num_of_adv_sets);
    //le_ext_adv_gap_msg_info_way(false);

    if (le_ext_adv_init(BLE_EXTEND_ADV_NUM_MAX) == GAP_CAUSE_SUCCESS)
    {
        ble_ext_adv_set_table = (T_EXT_ADV_CB *)os_mem_zalloc(RAM_TYPE_BT_UAL,
                                                              BLE_EXTEND_ADV_NUM_MAX * sizeof(T_EXT_ADV_CB));
    }

    if (ble_ext_adv_set_table == NULL)
    {
        BTM_PRINT_ERROR0("ble_extend_adv_mgr_init: allocate failed");
        return GAP_CAUSE_NO_RESOURCE;
    }
    else
    {
        /*set default value for ble_ext_adv_set_table*/
        for (int i = 0; i < BLE_EXTEND_ADV_NUM_MAX; i++)
        {
            ble_ext_adv_set_table[i].adv_handle = APP_IDLE_ADV_SET;
            ble_ext_adv_set_table[i].init_flags = APP_IDLE_UPDATE_FLAGS;
        }
    }

    /*create a queue to store extend advertising event*/
    ble_ext_adv_queue_init();
    app_timer_reg_cb(ext_adv_rpa_timeout_cb, &ext_rpa_timer_id);

    return GAP_CAUSE_SUCCESS;
}

T_GAP_CAUSE ble_pa_mgr_init()
{
#if (PA_FEATURE_SUPPORT == 1)
    le_pa_adv_init(UAL_MAX_PA_ADV_SET_NUM);
    return GAP_CAUSE_SUCCESS;
#else
    return GAP_CAUSE_NO_RESOURCE;
#endif
}


static T_EXT_ADV_CB *ble_ext_adv_mgr_alloc_adv_handle(uint8_t adv_handle)
{
    uint8_t i;
    T_EXT_ADV_CB *p_adv;

    for (i = 0; i < BLE_EXTEND_ADV_NUM_MAX; i++)
    {
        p_adv = &ble_ext_adv_set_table[i];
        if (!p_adv->is_used)
        {
            p_adv->is_used = true;
            p_adv->adv_handle = adv_handle;
            p_adv->adv_sid = adv_handle;
            BTM_PRINT_TRACE2("ble_ext_adv_mgr_alloc_adv_handle: ble_ext_adv_set_table %d,adv_handle %d", i,
                             p_adv->adv_handle);
            return p_adv;
        }
    }
    BTM_PRINT_ERROR0("ble_ext_adv_mgr_alloc_adv_handle: failed because all adv set has been used");

    return NULL;
}

uint8_t ble_ext_adv_mgr_create_adv(P_FUN_ADV_APP_CB app_callback)
{
    uint8_t adv_handle;
    adv_handle = le_ext_adv_create_adv_handle();

    if (adv_handle != APP_IDLE_ADV_SET)
    {
        T_EXT_ADV_CB *p_adv = ble_ext_adv_mgr_alloc_adv_handle(adv_handle);
        BTM_PRINT_INFO1("ble_ext_adv_mgr_create_adv: adv_handle %d", adv_handle);

        if (p_adv != NULL)
        {
            p_adv->app_callback = app_callback;
            /*update_flags */
            p_adv->init_flags = EXT_ADV_SET_ADV_PARAS;
            return adv_handle;
        }
    }
    BTM_PRINT_ERROR1("ble_ext_adv_mgr_create_adv: error adv handle %d", adv_handle);
    return adv_handle;
}

T_GAP_CAUSE ble_ext_adv_mgr_create_adv_param(uint8_t adv_handle, T_ADV_PARAM_CONF conf_msk,
                                             T_ADV_PARAMS *p_param, uint16_t adv_data_len, uint8_t *p_adv_data,
                                             uint16_t scan_data_len, uint8_t *p_scan_data)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);
    uint8_t primary_adv_channel_map;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy;
    uint8_t secondary_adv_max_skip;
    T_GAP_PHYS_TYPE secondary_adv_phy;
    bool scan_req_notification_enable = false;
    uint8_t update_flags = EXT_ADV_SET_ADV_PARAS;
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_ext_adv_mgr_create_adv_param: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }
    if (conf_msk & ADV_TX_POWER_CONF)
    {
        if ((p_param->tx_power < -127) || (p_param->tx_power > 20 && p_param->tx_power != 127))
        {
            return GAP_CAUSE_INVALID_PARAM;
        }
    }

    if (conf_msk & ADV_INTERVAL_RANGE_CONF)
    {
        if (p_param->adv_interval_min > p_param->adv_interval_max)
        {
            return GAP_CAUSE_INVALID_PARAM;
        }
        p_adv->adv_interval_min = p_param->adv_interval_min;
        p_adv->adv_interval_max = p_param->adv_interval_max;
    }
    p_adv->adv_event_prop = (conf_msk & ADV_EVENT_PROP_CONF) ? p_param->adv_event_prop :
                            p_adv->adv_event_prop;


    if (conf_msk & LOCAL_BD_TYPE_CONF)
    {
        if (p_param->local_bd_type != p_adv->own_address_type &&
            (p_param->local_bd_type == GAP_LOCAL_ADDR_LE_RANDOM ||
             p_param->local_bd_type == GAP_LOCAL_ADDR_LE_RAP_OR_RAND))
        {
            le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, p_adv->random_addr);
            update_flags |= EXT_ADV_SET_RANDOM_ADDR;
        }
        p_adv->own_address_type = (T_GAP_LOCAL_ADDR_TYPE)p_param->local_bd_type;
    }

    if (conf_msk & PEER_ADDR_INFO_CONF)
    {
        p_adv->peer_address_type = (T_GAP_REMOTE_ADDR_TYPE)p_param->peer_bd_type;
        memcpy(p_adv->peer_address, p_param->peer_addr, 6);
    }
    p_adv->filter_policy = (conf_msk & FILTER_POLICY_CONF) ? (T_GAP_ADV_FILTER_POLICY)
                           p_param->filter_policy : p_adv->filter_policy;
    p_adv->adv_tx_power = (conf_msk & ADV_TX_POWER_CONF) ? p_param->tx_power : p_adv->adv_tx_power;
    p_adv->adv_sid = (conf_msk & ADV_SID_CONF) ? p_param->adv_sid : p_adv->adv_sid;
    primary_adv_channel_map = (conf_msk & PRIM_ADV_CHANNEL_MAP_CONF) ?
                              p_param->primary_adv_channel_map : GAP_ADVCHAN_ALL;
    primary_adv_phy = (conf_msk & PRIM_ADV_PHY_CONF) ? (T_GAP_PHYS_PRIM_ADV_TYPE)
                      p_param->primary_adv_phy : GAP_PHYS_PRIM_ADV_1M;
    secondary_adv_max_skip = (conf_msk & SEC_ADV_MAX_SKIP_CONF) ? (T_GAP_PHYS_PRIM_ADV_TYPE)
                             p_param->secondary_adv_max_skip : 0;
    secondary_adv_phy = (conf_msk & SEC_ADV_PHY_CONF) ? (T_GAP_PHYS_TYPE)p_param->secondary_adv_phy :
                        GAP_PHYS_1M;
    scan_req_notification_enable = (conf_msk & SCAN_REQ_NOTIFY_CONF) ? p_param->scan_req_notify_enable :
                                   false;

    result = le_ext_adv_set_adv_param(adv_handle, p_adv->adv_event_prop,
                                      p_adv->adv_interval_min, p_adv->adv_interval_max,
                                      primary_adv_channel_map, p_adv->own_address_type,
                                      p_adv->peer_address_type, p_adv->peer_address,
                                      p_adv->filter_policy, p_adv->adv_tx_power,
                                      primary_adv_phy, secondary_adv_max_skip,
                                      secondary_adv_phy, p_adv->adv_sid,
                                      scan_req_notification_enable);
    if (result != GAP_CAUSE_SUCCESS)
    {
        return result;
    }

    BTM_PRINT_INFO3("ble_adv_mgr_set_ext_adv_param: adv_handle %d, ext_adv_state %d, ble_ext_adv_mgr adv state %d",
                    p_adv->adv_handle, p_adv->ext_adv_state, p_adv->ble_ext_adv_mgr_adv_state);

    if (p_adv_data != NULL && adv_data_len <= ext_adv_mgr.max_ext_adv_len &&
        !(p_adv->adv_event_prop & GAP_EXT_ADV_EVT_PROP_DIRECTED_ADV))
    {
        p_adv->p_adv_data = (uint8_t *)os_mem_zalloc(RAM_TYPE_BT_UAL, adv_data_len);
        if (adv_data_len > 0 && p_adv->p_adv_data)
        {
            memcpy(p_adv->p_adv_data, p_adv_data, adv_data_len);
            p_adv->adv_data_len = adv_data_len;
            update_flags |= EXT_ADV_SET_ADV_DATA;
            result = le_ext_adv_set_adv_data(adv_handle, adv_data_len, (uint8_t *)p_adv->p_adv_data);
        }
    }

    if (result != GAP_CAUSE_SUCCESS)
    {
        return result;
    }

    if (p_scan_data != NULL && scan_data_len <= ext_adv_mgr.max_ext_adv_len &&
        (p_adv->adv_event_prop & GAP_EXT_ADV_EVT_PROP_SCANNABLE_ADV))
    {
        p_adv->p_scan_response_data = (uint8_t *)os_mem_zalloc(RAM_TYPE_BT_UAL, scan_data_len);
        if (scan_data_len > 0 && p_adv->p_scan_response_data)
        {
            memcpy(p_adv->p_scan_response_data, p_scan_data, scan_data_len);
            p_adv->scan_response_data_len = scan_data_len;
            update_flags |= EXT_ADV_SET_SCAN_RSP_DATA;
            result = le_ext_adv_set_scan_response_data(adv_handle, scan_data_len,
                                                       (uint8_t *)p_adv->p_scan_response_data);
        }
    }

    if (result != GAP_CAUSE_SUCCESS)
    {
        return result;
    }
    if (p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_ADVERTISING)
    {
        ble_disable_ext_adv(adv_handle, APP_STOP_FOR_UPDATE);
        ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, update_flags);
        result = send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_START, p_adv->duration);
    }
    else
    {
        result = ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, update_flags);
    }

    return result;
}

uint8_t ble_ext_adv_mgr_alloc_adv(P_FUN_ADV_APP_CB app_callback, uint16_t adv_event_prop,
                                  uint32_t primary_adv_interval_min, uint32_t primary_adv_interval_max,
                                  T_GAP_LOCAL_ADDR_TYPE own_address_type, T_GAP_REMOTE_ADDR_TYPE peer_address_type,
                                  uint8_t *p_peer_address,
                                  T_GAP_ADV_FILTER_POLICY filter_policy, uint16_t adv_data_len, uint8_t *p_adv_data,
                                  uint16_t scan_data_len, uint8_t *p_scan_data)
{
    uint8_t adv_handle;
    uint8_t  primary_adv_channel_map = GAP_ADVCHAN_ALL;
    uint8_t tx_power = 127;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    uint8_t secondary_adv_max_skip = 0;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_1M;
    bool scan_req_notification_enable = false;

    adv_handle = le_ext_adv_create_adv_handle();

    if (adv_handle != APP_IDLE_ADV_SET)
    {
        T_EXT_ADV_CB *p_adv = ble_ext_adv_mgr_alloc_adv_handle(adv_handle);
        BTM_PRINT_INFO3("ble_ext_adv_mgr_alloc_adv: adv_handle %d,adv_data_len %d,p_adv_data 0x%x",
                        adv_handle, adv_data_len, p_adv_data);

        if (p_adv != NULL)
        {
            /*adv param*/
            p_adv->adv_event_prop = adv_event_prop;
            p_adv->adv_interval_min = primary_adv_interval_min;
            p_adv->adv_interval_max = primary_adv_interval_max;
            p_adv->own_address_type = own_address_type;
            p_adv->peer_address_type = peer_address_type;
            p_adv->filter_policy = filter_policy;
            p_adv->adv_tx_power = tx_power;
            p_adv->app_callback = app_callback;

            if (p_peer_address != NULL)
            {
                memcpy(p_adv->peer_address, p_peer_address, GAP_BD_ADDR_LEN);
            }

            /*update_flags */
            p_adv->init_flags = EXT_ADV_SET_ADV_PARAS;

            le_ext_adv_set_adv_param(adv_handle,
                                     adv_event_prop,
                                     primary_adv_interval_min,
                                     primary_adv_interval_max,
                                     primary_adv_channel_map,
                                     own_address_type,
                                     peer_address_type,
                                     p_peer_address,
                                     filter_policy,
                                     tx_power,
                                     primary_adv_phy,
                                     secondary_adv_max_skip,
                                     secondary_adv_phy,
                                     p_adv->adv_sid,
                                     scan_req_notification_enable);

            if (own_address_type == GAP_LOCAL_ADDR_LE_RANDOM ||
                own_address_type == GAP_LOCAL_ADDR_LE_RAP_OR_RAND)
            {
                p_adv->init_flags |= EXT_ADV_SET_RANDOM_ADDR;
                le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, p_adv->random_addr);
                le_ext_adv_set_random(adv_handle, p_adv->random_addr);
            }

            if (p_adv_data != NULL && adv_data_len <= ext_adv_mgr.max_ext_adv_len &&
                !(adv_event_prop & GAP_EXT_ADV_EVT_PROP_DIRECTED_ADV))
            {
                p_adv->p_adv_data = (uint8_t *)os_mem_zalloc(RAM_TYPE_BT_UAL, adv_data_len);
                if (adv_data_len > 0 && p_adv->p_adv_data)
                {
                    memcpy(p_adv->p_adv_data, p_adv_data, adv_data_len);
                    p_adv->adv_data_len = adv_data_len;
                    p_adv->init_flags |= EXT_ADV_SET_ADV_DATA;
                    le_ext_adv_set_adv_data(adv_handle, adv_data_len, (uint8_t *)p_adv->p_adv_data);
                }
            }

            if (p_scan_data != NULL && scan_data_len <= ext_adv_mgr.max_ext_adv_len &&
                (adv_event_prop & GAP_EXT_ADV_EVT_PROP_SCANNABLE_ADV))
            {
                p_adv->p_scan_response_data = (uint8_t *)os_mem_zalloc(RAM_TYPE_BT_UAL, scan_data_len);
                if (scan_data_len > 0 && p_adv->p_scan_response_data)
                {
                    memcpy(p_adv->p_scan_response_data, p_scan_data, scan_data_len);
                    p_adv->scan_response_data_len = scan_data_len;
                    p_adv->init_flags |= EXT_ADV_SET_SCAN_RSP_DATA;
                    le_ext_adv_set_scan_response_data(adv_handle, scan_data_len,
                                                      (uint8_t *)p_adv->p_scan_response_data);
                }
            }
            ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, 0);
            return adv_handle;
        }
    }
    BTM_PRINT_ERROR1("ble_ext_adv_mgr_alloc_adv: error adv handle %d", adv_handle);
    return adv_handle;
}

T_GAP_CAUSE ble_adv_mgr_rmv_ext_adv(uint8_t adv_handle)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_adv_mgr_rmv_ext_adv: adv_handle not created");
        result = GAP_CAUSE_NOT_FIND;
        return result;
    }

    if (p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_IDLE)
    {
        if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state != BLE_EXT_NONE_STATE)
        {
            result = send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_REMOVE, 0);
        }
        else
        {
            result = ble_ext_adv_remove_action(p_adv);
        }
    }
    else
    {
        BTM_PRINT_ERROR1("ble_adv_mgr_rmv_ext_adv invalid ble adv mgr adv state %d ",
                         p_adv->ble_ext_adv_mgr_adv_state);
        result = GAP_CAUSE_INVALID_STATE;
    }

    return result;

}

static void ble_ext_adv_mgr_free_adv(uint8_t adv_handle)
{
    uint8_t i;
    T_EXT_ADV_CB *p_adv;

    for (i = 0; i < BLE_EXTEND_ADV_NUM_MAX; i++)
    {
        p_adv = &ble_ext_adv_set_table[i];
        if (p_adv->is_used && p_adv->adv_handle == adv_handle)
        {
            if (p_adv->p_adv_data)
            {
                os_mem_free(p_adv->p_adv_data);
            }
            if (p_adv->p_scan_response_data)
            {
                os_mem_free(p_adv->p_scan_response_data);
            }
            memset(p_adv, 0, sizeof(T_EXT_ADV_CB));
            p_adv->adv_handle = APP_IDLE_ADV_SET;
            p_adv->init_flags = APP_IDLE_UPDATE_FLAGS;
        }
    }
}

static T_EXT_ADV_CB *ble_ext_adv_mgr_find_by_adv_handle(uint8_t adv_handle)
{
    uint8_t i;
    T_EXT_ADV_CB *p_adv;

    for (i = 0; i < BLE_EXTEND_ADV_NUM_MAX; i++)
    {
        p_adv = &ble_ext_adv_set_table[i];

        if (p_adv->is_used && p_adv->adv_handle == adv_handle)
        {
            return p_adv;
        }
    }

    BTM_PRINT_ERROR1("ble_ext_adv_mgr_find_by_adv_handle: not found adv_handle %d", adv_handle);
    return NULL;
}

static bool ble_ext_adv_mgr_is_adv_action_pending()
{
    uint8_t i;
    T_EXT_ADV_CB *p_adv;

    for (i = 0; i < BLE_EXTEND_ADV_NUM_MAX; i++)
    {
        p_adv = &ble_ext_adv_set_table[i];

        if (p_adv->is_used && p_adv->pending_action != BLE_EXT_ADV_PENDING_ACTION_IDLE)
        {
            BTM_PRINT_INFO2("ble_ext_adv_mgr_is_adv_action_pending:has pending_action %d ,adv_handle %d",
                            p_adv->pending_action, p_adv->adv_handle);
            return true;
        }
    }

    return false;
}

static T_GAP_CAUSE ble_ext_adv_mgr_update_adv_param(uint8_t adv_handle, T_EXT_ADV_CB *p_adv,
                                                    uint8_t update_flags)
{
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state != BLE_EXT_NONE_STATE)
    {
        return send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_UPDATE, update_flags);
    }

    result = ble_ext_adv_update_action(p_adv, update_flags);

    return result;
}

T_GAP_CAUSE ble_ext_adv_mgr_start_adv_settings(uint8_t adv_handle, uint8_t update_flags)
{
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    T_EXT_ADV_CB *p_adv;
    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_ext_adv_mgr_update_settings: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }

    if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state != BLE_EXT_NONE_STATE)
    {
        return send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_UPDATE, update_flags);
    }

    result = ble_ext_adv_update_action(p_adv, update_flags);

    return result;
}

T_GAP_CAUSE ble_adv_mgr_set_ext_adv_data(uint8_t adv_handle, uint16_t adv_data_len,
                                         uint8_t *p_adv_data)
{
    BTM_PRINT_INFO1("ble_adv_mgr_set_ext_adv_data: adv_handle %d", adv_handle);
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_adv_mgr_set_ext_adv_data: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }

    if (adv_data_len > 0 && p_adv_data == NULL)
    {
        BTM_PRINT_ERROR0("ble_adv_mgr_set_ext_adv_data: Invalid param");
        return GAP_CAUSE_INVALID_PARAM;
    }

    if (p_adv->adv_data_len < adv_data_len)
    {
        uint8_t *p_adv_data = (uint8_t *)os_mem_zalloc(RAM_TYPE_BT_UAL, adv_data_len);
        if (p_adv_data == NULL)
        {
            BTM_PRINT_ERROR0("ble_adv_mgr_set_ext_adv_data: alloc fail");
            return GAP_CAUSE_NO_RESOURCE;
        }

        if (p_adv->p_adv_data != NULL)
        {
            os_mem_free(p_adv->p_adv_data);
        }
        p_adv->p_adv_data = p_adv_data;
    }
    p_adv->adv_data_len = adv_data_len;
    memcpy(p_adv->p_adv_data, p_adv_data, adv_data_len);

    le_ext_adv_set_adv_data(adv_handle, p_adv->adv_data_len, (uint8_t *)p_adv->p_adv_data);

    ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, EXT_ADV_SET_ADV_DATA);
    return result;
}

T_GAP_CAUSE ble_adv_mgr_set_ext_scan_response_data(uint8_t adv_handle, uint16_t scan_data_len,
                                                   uint8_t *p_scan_data)
{
    BTM_PRINT_INFO1("ble_adv_mgr_set_ext_scan_response_data: adv_handle %d", adv_handle);
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_adv_mgr_set_ext_scan_response_data: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }

    if ((scan_data_len > 0 && p_scan_data == NULL) ||
        scan_data_len > ext_adv_mgr.max_ext_adv_len)
    {
        BTM_PRINT_ERROR0("ble_adv_mgr_set_ext_scan_response_data: Invalid param");
        return GAP_CAUSE_INVALID_PARAM;
    }

    if (p_adv->scan_response_data_len < scan_data_len)
    {
        uint8_t *p_scan_response_data = (uint8_t *)os_mem_zalloc(RAM_TYPE_BT_UAL, scan_data_len);
        if (p_scan_response_data == NULL)
        {
            BTM_PRINT_ERROR0("ble_adv_mgr_set_ext_scan_response_data: alloc fail");
            return GAP_CAUSE_NO_RESOURCE;
        }

        if (p_adv->p_scan_response_data != NULL)
        {
            os_mem_free(p_adv->p_scan_response_data);
        }
        p_adv->p_scan_response_data = p_scan_response_data;
    }

    p_adv->scan_response_data_len = scan_data_len;
    if (scan_data_len > 0)
    {
        memcpy(p_adv->p_scan_response_data, p_scan_data, scan_data_len);
    }
    else
    {
        if (p_adv->p_scan_response_data)
        {
            os_mem_free(p_adv->p_scan_response_data);
            p_adv->p_scan_response_data = NULL;
        }
    }

    le_ext_adv_set_scan_response_data(adv_handle, p_adv->scan_response_data_len,
                                      (uint8_t *)p_adv->p_scan_response_data);

    ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, EXT_ADV_SET_SCAN_RSP_DATA);

    return result;
}

T_GAP_CAUSE ble_enable_ext_adv(uint8_t adv_handle, uint16_t duration_10ms)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_enable_ext_adv: adv_handle not created");
        result = GAP_CAUSE_NOT_FIND;
        return result;
    }

    BTM_PRINT_INFO3("ble_enable_ext_adv: adv_handle %d, duration_10ms %d, init_flags 0x%x",
                    adv_handle,
                    duration_10ms, p_adv->init_flags);
    if (p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_IDLE)
    {
        if (p_adv->adv_param_set == false)
        {
            ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, p_adv->init_flags);
        }

        if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state == BLE_EXT_SUSPEND_STATE)
        {
            result = send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_START, duration_10ms);
        }
        else
        {
            if (p_adv->ext_adv_state == EXT_ADV_STATE_IDLE)
            {
                result = ble_ext_adv_start_action(p_adv, duration_10ms);
            }
        }
    }
    else
    {
        BTM_PRINT_ERROR1("ble_enable_ext_adv invalid ble adv mgr adv state %d ",
                         p_adv->ble_ext_adv_mgr_adv_state);
        result = GAP_CAUSE_INVALID_STATE;
    }

    if (result == GAP_CAUSE_SUCCESS)
    {
        ext_adv_start_rpa_timer(p_adv);
        p_adv->duration = duration_10ms;
        ble_adv_update_ext_adv_state(p_adv, (T_BLE_ADV_STOP_CAUSE)0, BLE_ADV_STATE_ADVERTISING, 0);
    }

    return result;
}

T_GAP_CAUSE ble_adv_mgr_set_ext_adv_param(uint8_t adv_handle, T_ADV_PARAM_CONF conf_msk,
                                          T_ADV_PARAMS *p_param)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);
    uint8_t primary_adv_channel_map;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy;
    uint8_t secondary_adv_max_skip;
    T_GAP_PHYS_TYPE secondary_adv_phy;
    bool scan_req_notification_enable = false;
    uint8_t update_flags = EXT_ADV_SET_ADV_PARAS;
    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_adv_mgr_set_ext_adv_param: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }
    if (conf_msk & ADV_TX_POWER_CONF)
    {
        if ((p_param->tx_power < -127) || (p_param->tx_power > 20 && p_param->tx_power != 127))
        {
            return GAP_CAUSE_INVALID_PARAM;
        }
    }

    if (conf_msk & ADV_INTERVAL_RANGE_CONF)
    {
        if (p_param->adv_interval_min > p_param->adv_interval_max)
        {
            return GAP_CAUSE_INVALID_PARAM;
        }
        p_adv->adv_interval_min = p_param->adv_interval_min;
        p_adv->adv_interval_max = p_param->adv_interval_max;
    }
    p_adv->adv_event_prop = (conf_msk & ADV_EVENT_PROP_CONF) ? p_param->adv_event_prop :
                            p_adv->adv_event_prop;


    if (conf_msk & LOCAL_BD_TYPE_CONF)
    {
        if (p_param->local_bd_type != p_adv->own_address_type &&
            (p_param->local_bd_type == GAP_LOCAL_ADDR_LE_RANDOM ||
             p_param->local_bd_type == GAP_LOCAL_ADDR_LE_RAP_OR_RAND))
        {
            le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, p_adv->random_addr);
            update_flags |= EXT_ADV_SET_RANDOM_ADDR;
        }
        p_adv->own_address_type = (T_GAP_LOCAL_ADDR_TYPE)p_param->local_bd_type;
    }

    if (conf_msk & PEER_ADDR_INFO_CONF)
    {
        p_adv->peer_address_type = (T_GAP_REMOTE_ADDR_TYPE)p_param->peer_bd_type;
        memcpy(p_adv->peer_address, p_param->peer_addr, 6);
    }
    p_adv->filter_policy = (conf_msk & FILTER_POLICY_CONF) ? (T_GAP_ADV_FILTER_POLICY)
                           p_param->filter_policy : p_adv->filter_policy;
    p_adv->adv_tx_power = (conf_msk & ADV_TX_POWER_CONF) ? p_param->tx_power : p_adv->adv_tx_power;
    p_adv->adv_sid = (conf_msk & ADV_SID_CONF) ? p_param->adv_sid : p_adv->adv_sid;
    primary_adv_channel_map = (conf_msk & PRIM_ADV_CHANNEL_MAP_CONF) ?
                              p_param->primary_adv_channel_map : GAP_ADVCHAN_ALL;
    primary_adv_phy = (conf_msk & PRIM_ADV_PHY_CONF) ? (T_GAP_PHYS_PRIM_ADV_TYPE)
                      p_param->primary_adv_phy : GAP_PHYS_PRIM_ADV_1M;
    secondary_adv_max_skip = (conf_msk & SEC_ADV_MAX_SKIP_CONF) ? (T_GAP_PHYS_PRIM_ADV_TYPE)
                             p_param->secondary_adv_max_skip : 0;
    secondary_adv_phy = (conf_msk & SEC_ADV_PHY_CONF) ? (T_GAP_PHYS_TYPE)p_param->secondary_adv_phy :
                        GAP_PHYS_1M;
    scan_req_notification_enable = (conf_msk & SCAN_REQ_NOTIFY_CONF) ? p_param->scan_req_notify_enable :
                                   false;

    le_ext_adv_set_adv_param(adv_handle, p_adv->adv_event_prop,
                             p_adv->adv_interval_min, p_adv->adv_interval_max,
                             primary_adv_channel_map, p_adv->own_address_type,
                             p_adv->peer_address_type, p_adv->peer_address,
                             p_adv->filter_policy, p_adv->adv_tx_power,
                             primary_adv_phy, secondary_adv_max_skip,
                             secondary_adv_phy, p_adv->adv_sid,
                             scan_req_notification_enable);

    BTM_PRINT_INFO3("ble_adv_mgr_set_ext_adv_param: adv_handle %d, ext_adv_state %d, ble_ext_adv_mgr adv state %d",
                    p_adv->adv_handle, p_adv->ext_adv_state, p_adv->ble_ext_adv_mgr_adv_state);
    if (p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_ADVERTISING)
    {
        ble_disable_ext_adv(adv_handle, APP_STOP_FOR_UPDATE);
        ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, update_flags);
        send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_START, p_adv->duration);
    }
    else
    {
        ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, update_flags);
    }

    return result;
}

static T_GAP_CAUSE ble_ext_adv_mgr_set_random_addr(uint8_t adv_handle, uint8_t *random_address)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL || random_address == NULL)
    {
        BTM_PRINT_ERROR0("ble_ext_adv_mgr_set_random_addr: NULL pointer");
        return GAP_CAUSE_NOT_FIND;
    }
    if (p_adv->own_address_type == GAP_LOCAL_ADDR_LE_PUBLIC ||
        p_adv->own_address_type == GAP_LOCAL_ADDR_LE_RAP_OR_PUBLIC)
    {
        BTM_PRINT_ERROR1("ble_ext_adv_mgr_set_random_addr: invalid local address type %d",
                         p_adv->own_address_type);
        return GAP_CAUSE_INVALID_PARAM;
    }

    memcpy(p_adv->random_addr, random_address, GAP_BD_ADDR_LEN);

    le_ext_adv_set_random(adv_handle, random_address);
    BTM_PRINT_INFO4("ble_ext_adv_mgr_set_random_addr:adv_handle %d, ext_adv_state %d,ble_ext_adv_mgr adv state %d,random_address %s",
                    adv_handle,
                    p_adv->ext_adv_state, p_adv->ble_ext_adv_mgr_adv_state, TRACE_BDADDR(random_address));


    if (p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_ADVERTISING)
    {
        ble_disable_ext_adv(adv_handle, APP_STOP_FOR_UPDATE);
        ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, EXT_ADV_SET_RANDOM_ADDR);
        send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_START, p_adv->duration);
    }
    else
    {
        ble_ext_adv_mgr_update_adv_param(adv_handle, p_adv, EXT_ADV_SET_RANDOM_ADDR);
    }

    return result;
}

bool ble_get_ext_adv_random_addr(uint8_t adv_handle, uint8_t *p_random_address)
{
    T_EXT_ADV_CB *p_adv;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL || p_random_address == NULL)
    {
        BTM_PRINT_ERROR0("ble_get_ext_adv_random_addr: NULL pointer");
        return false;
    }
    if (p_adv->own_address_type == GAP_LOCAL_ADDR_LE_PUBLIC ||
        p_adv->own_address_type == GAP_LOCAL_ADDR_LE_RAP_OR_PUBLIC)
    {
        BTM_PRINT_ERROR1("ble_get_ext_adv_random_addr: invalid local address type %d",
                         p_adv->own_address_type);
        return false;
    }

    memcpy(p_random_address, p_adv->random_addr, GAP_BD_ADDR_LEN);

    return true;
}


T_GAP_CAUSE ble_disable_ext_adv(uint8_t adv_handle, uint8_t app_cause)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_disable_ext_adv: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }

    ext_adv_stop_rpa_timer(p_adv);
    BTM_PRINT_INFO2("ble_disable_ext_adv: adv_handle %d, app_cause 0x%x", adv_handle, app_cause);

    if (p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_ADVERTISING)
    {
        if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state == BLE_EXT_SUSPEND_STATE)
        {
            result = send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_STOP, app_cause);
        }
        else
        {
            if (p_adv->ext_adv_state == EXT_ADV_STATE_ADVERTISING)
            {
                result = ble_ext_adv_stop_action(p_adv, app_cause);
            }
        }
    }
    else
    {
        BTM_PRINT_ERROR1("ble_disable_ext_adv: invalid ble adv mgr adv state %d ",
                         p_adv->ble_ext_adv_mgr_adv_state);
    }
    if (result == GAP_CAUSE_SUCCESS && app_cause != APP_STOP_FOR_UPDATE)
    {
        ble_adv_update_ext_adv_state(p_adv, BLE_ADV_STOP_CAUSE_APP, BLE_ADV_STATE_IDLE,
                                     app_cause);
    }
    return result;
}

T_GAP_CAUSE ble_disable_all_ext_adv(uint8_t app_cause)
{
    BTM_PRINT_INFO1("ble_disable_all_ext_adv:app_cause 0x%x", app_cause);
    T_EXT_ADV_CB *p_adv;

    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    for (uint8_t i = 0; i < BLE_EXTEND_ADV_NUM_MAX; i++)
    {
        p_adv = &ble_ext_adv_set_table[i];

        if (p_adv != NULL && p_adv->is_used &&
            p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_ADVERTISING)
        {
            result = ble_disable_ext_adv(p_adv->adv_handle, app_cause);

            if (result != GAP_CAUSE_SUCCESS)
            {
                BTM_PRINT_INFO2("ble_disable_all_ext_adv:adv_handle %d,result 0x%x", p_adv->adv_handle, result);
            }
        }
    }
    return result;
}

static void ble_ext_adv_mgr_hdl_state_change(T_EXT_ADV_CB *p_adv, uint16_t cause,
                                             bool state_change)
{
    T_BLE_ADV_STOP_CAUSE stop_cause = BLE_ADV_STOP_CAUSE_UNKNOWN;
    T_BLE_ADV_STATE new_adv_state = p_adv->ble_ext_adv_mgr_adv_state;
    BTM_PRINT_INFO4("ble_ext_adv_mgr_hdl_state_change: adv_handle %d, ble_ext_adv_mgr_adv_state %d, cause 0x%x, pending_start_stop_num %d",
                    p_adv->adv_handle, p_adv->ble_ext_adv_mgr_adv_state, cause, p_adv->pending_start_stop_num);
    if (state_change)
    {
        p_adv->pending_cause = cause;
    }
    cause = p_adv->pending_cause;

    if (p_adv->pending_start_stop_num == 0)
    {
        if (p_adv->ext_adv_state == EXT_ADV_STATE_IDLE)
        {
            if (cause == (HCI_ERR | HCI_ERR_OPERATION_CANCELLED_BY_HOST))
            {
                BTM_PRINT_INFO0("ble_ext_adv_mgr_hdl_state_change: EXT_ADV_STATE_IDLE because app stop");
                stop_cause = BLE_ADV_STOP_CAUSE_APP;
                if (p_adv->app_cause != APP_STOP_FOR_UPDATE)
                {
                    new_adv_state = BLE_ADV_STATE_IDLE;
                }
            }
            else if (cause == (HCI_ERR | HCI_ERR_DIRECTED_ADV_TIMEOUT))
            {
                BTM_PRINT_INFO0("ble_ext_adv_mgr_hdl_state_change: EXT_ADV_STATE_IDLE because timeout");
                stop_cause = BLE_ADV_STOP_CAUSE_TIMEOUT;
                new_adv_state = BLE_ADV_STATE_IDLE;
            }
            else if (cause == 0)
            {
                BTM_PRINT_INFO0("ble_ext_adv_mgr_hdl_state_change: EXT_ADV_STATE_IDLE because conn");
                stop_cause = BLE_ADV_STOP_CAUSE_CONN;
                new_adv_state = BLE_ADV_STATE_IDLE;
            }
            else
            {
                BTM_PRINT_INFO1("ble_ext_adv_mgr_hdl_state_change: EXT_ADV_STATE_IDLE cause 0x%x", cause);
                stop_cause = BLE_ADV_STOP_CAUSE_UNKNOWN;
                if (p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_ADVERTISING)
                {
                    stop_cause = BLE_ADV_STOP_CAUSE_ENABLE_FAILED;
                }
                new_adv_state = BLE_ADV_STATE_IDLE;
            }
        }
    }
    else
    {
        if (p_adv->ext_adv_state == EXT_ADV_STATE_IDLE && cause == 0)
        {
            stop_cause = BLE_ADV_STOP_CAUSE_CONN;
            new_adv_state = BLE_ADV_STATE_IDLE;
            if (p_adv->ble_ext_adv_mgr_adv_state == BLE_ADV_STATE_ADVERTISING)
            {
                ble_disable_ext_adv(p_adv->adv_handle, 0);
            }
        }
    }
    ble_adv_update_ext_adv_state(p_adv, stop_cause, new_adv_state, p_adv->app_cause);
}


#if (PA_FEATURE_SUPPORT == 1)
static void ble_ext_adv_mgr_hdl_pa_state(uint8_t adv_handle, T_GAP_PA_ADV_STATE new_state,
                                         uint16_t cause)
{
    T_EXT_ADV_CB *p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv != NULL)
    {
        p_adv->pa_state = new_state;
        BTM_PRINT_INFO2("ble_ext_adv_mgr_hdl_pa_state: adv_handle %d, adv_state %d",
                        adv_handle, p_adv->pa_state);
        if (new_state == PA_ADV_STATE_ADVERTISING)
        {
            ble_adv_update_pa_state(p_adv, (T_BLE_ADV_STOP_CAUSE)0, BLE_ADV_STATE_ADVERTISING, 0);
        }
        else if (new_state == PA_ADV_STATE_IDLE &&
                 p_adv->pending_action != BLE_EXT_ADV_PENDING_ACTION_PA_UPDATE)
        {
            ble_adv_update_pa_state(p_adv, (T_BLE_ADV_STOP_CAUSE)0, BLE_ADV_STATE_IDLE, 0);
        }
    }
}
#endif

void ble_ext_adv_mgr_hdl_adv_state(uint8_t adv_handle, T_GAP_EXT_ADV_STATE new_state,
                                   uint16_t cause)
{
    T_EXT_ADV_CB *p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv != NULL)
    {
        uint8_t old_adv_state = p_adv->ext_adv_state;
        p_adv->ext_adv_state = new_state;
        BTM_PRINT_INFO3("ble_ext_adv_mgr_handle_adv_state: adv_handle %d, adv_state %d -> %d",
                        adv_handle, old_adv_state, p_adv->ext_adv_state);
        if (ext_adv_mgr.state == BLE_EXT_WAIT4PENDING_STATE ||
            ext_adv_mgr.state == BLE_EXT_NONE_STATE)
        {
            ble_ext_adv_mgr_hdl_state_change(p_adv, cause, true);

            if (old_adv_state == EXT_ADV_STATE_START)
            {
                //enable procedure
                if (p_adv->pending_action == BLE_EXT_ADV_PENDING_ACTION_START)
                {
                    p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_IDLE;
                }
                if (ext_adv_mgr.state != BLE_EXT_WAIT4PENDING_STATE)
                {
                    ble_ext_adv_mgr_handle_next_step();
                }
            }
            else if (old_adv_state == EXT_ADV_STATE_STOP)
            {
                //disable procedure
                if (p_adv->pending_action == BLE_EXT_ADV_PENDING_ACTION_STOP)
                {
                    p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_IDLE;
                }

                if (ext_adv_mgr.state != BLE_EXT_WAIT4PENDING_STATE)
                {
                    ble_ext_adv_mgr_handle_next_step();
                }
            }
        }
    }

    if (ext_adv_mgr.state == BLE_EXT_WAIT4PENDING_STATE &&
        !ble_ext_adv_mgr_is_adv_action_pending())
    {
        check_suspend_or_resume_state();
    }
}

void ble_ext_adv_mgr_hdl_conn_state(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                    uint16_t disc_cause)
{
    uint8_t adv_handle = 0xFF;

    switch (new_state)
    {
    case GAP_CONN_STATE_CONNECTED:
        adv_handle = le_ext_adv_get_adv_handle_by_conn_id(conn_id);

        if (adv_handle != 0xFF)
        {
            T_EXT_ADV_CB *p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

            if (p_adv != NULL)
            {
                BTM_PRINT_INFO2("ble_ext_adv_mgr_handle_conn_state:GAP_CONN_STATE_CONNECTED adv_handle %d,p_adv->conn_id 0x%x",
                                adv_handle, conn_id);
                if (p_adv->app_callback)
                {
                    T_BLE_ADV_CB_DATA cb_data;
                    T_BLE_ADV_SET_CONN_INFO adv_conn_info;
                    adv_conn_info.adv_handle = p_adv->adv_handle;
                    adv_conn_info.conn_id = conn_id;
                    cb_data.p_ble_conn_info = &adv_conn_info;
                    p_adv->app_callback(BLE_ADV_SET_CONN_INFO, &cb_data);
                }
            }
        }
        break;

    default:
        break;
    }
}

void ble_ext_adv_mgr_hdl_gap_cb(uint8_t cb_type, T_LE_CB_DATA *p_data)
{
    APP_PRINT_INFO2("ble_ext_adv_mgr_hdl_gap_cb: cb_type 0x%x, state 0x%x", cb_type, ext_adv_mgr.state);
    switch (cb_type)
    {
    case GAP_MSG_LE_EXT_ADV_START_SETTING:
        {
            T_EXT_ADV_CB *p_adv;
            APP_PRINT_INFO3("GAP_MSG_LE_EXT_ADV_START_SETTING: cause 0x%x, flag 0x%x, adv_handle %d",
                            p_data->p_le_ext_adv_start_setting_rsp->cause, p_data->p_le_ext_adv_start_setting_rsp->flag,
                            p_data->p_le_ext_adv_start_setting_rsp->adv_handle);
            p_adv = ble_ext_adv_mgr_find_by_adv_handle(p_data->p_le_ext_adv_start_setting_rsp->adv_handle);

            if (p_adv != NULL)
            {
                if (p_adv->pending_action == BLE_EXT_ADV_PENDING_ACTION_UPDATE)
                {
                    p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_IDLE;
                }
            }
            if (p_data->p_le_ext_adv_start_setting_rsp->cause != GAP_CAUSE_SUCCESS)
            {
                BTM_PRINT_ERROR1("GAP_MSG_LE_EXT_ADV_START_SETTING: failed, cause 0x%x",
                                 p_data->p_le_ext_adv_start_setting_rsp->cause);
            }
            if (ext_adv_mgr.state == BLE_EXT_NONE_STATE)
            {
                ble_ext_adv_mgr_handle_next_step();
            }
        }
        break;
    case GAP_MSG_LE_EXT_ADV_REMOVE_SET:
        {
            T_EXT_ADV_CB *p_adv;
            T_LE_EXT_ADV_REMOVE_SET_RSP *p_result = p_data->p_le_ext_adv_remove_set_rsp;
            p_adv = ble_ext_adv_mgr_find_by_adv_handle(p_result->adv_handle);

            if (p_adv != NULL)
            {
                if (p_adv->pending_action == BLE_EXT_ADV_PENDING_ACTION_REMOVE)
                {
                    p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_IDLE;
                }
            }
            if (p_result->cause != GAP_CAUSE_SUCCESS)
            {
                BTM_PRINT_ERROR2("GAP_MSG_LE_EXT_ADV_REMOVE_SET: failed, adv_handle 0x%x, cause 0x%x",
                                 p_result->adv_handle, p_result->cause);
            }
            else
            {
                ble_ext_adv_mgr_free_adv(p_result->adv_handle);
            }
            if (ext_adv_mgr.state == BLE_EXT_NONE_STATE)
            {
                ble_ext_adv_mgr_handle_next_step();
            }
        }
        break;
    case GAP_MSG_LE_EXT_ADV_ENABLE:
    case GAP_MSG_LE_EXT_ADV_DISABLE:
        {
            if (ext_adv_mgr.state != BLE_EXT_NONE_STATE)
            {
                check_suspend_or_resume_state();
            }
        }
        break;
#if (PA_FEATURE_SUPPORT == 1)
    case GAP_MSG_LE_PA_ADV_SET_PERIODIC_ADV_ENABLE:
        {
            if (cb_type == GAP_MSG_LE_PA_ADV_SET_PERIODIC_ADV_ENABLE)
            {
                T_EXT_ADV_CB *p_adv = ble_ext_adv_mgr_find_by_adv_handle(
                                          p_data->p_le_pa_adv_set_periodic_adv_enable_rsp->adv_handle);
                if (p_adv != NULL)
                {
                    if (p_adv->pending_action == BLE_EXT_ADV_PENDING_ACTION_PA_START)
                    {
                        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_IDLE;
                    }
                    else if (p_adv->pending_action == BLE_EXT_ADV_PENDING_ACTION_PA_STOP)
                    {
                        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_IDLE;
                    }

                    if (p_adv->pending_action == BLE_EXT_ADV_PENDING_ACTION_IDLE &&
                        ext_adv_mgr.state == BLE_EXT_NONE_STATE)
                    {
                        ble_ext_adv_mgr_handle_next_step();
                    }
                }
            }
            if (ext_adv_mgr.state != BLE_EXT_NONE_STATE)
            {
                check_suspend_or_resume_state();
            }
        }
        break;
    case GAP_MSG_LE_PA_ADV_START_SETTING:
        {
            T_EXT_ADV_CB *p_adv;
            APP_PRINT_INFO3("GAP_MSG_LE_PA_ADV_START_SETTING: cause 0x%x, flag 0x%x, adv_handle %d",
                            p_data->p_le_pa_adv_start_setting_rsp->cause, p_data->p_le_pa_adv_start_setting_rsp->flag,
                            p_data->p_le_pa_adv_start_setting_rsp->adv_handle);
            p_adv = ble_ext_adv_mgr_find_by_adv_handle(p_data->p_le_ext_adv_start_setting_rsp->adv_handle);

            if (p_adv != NULL)
            {
                if (p_adv->pending_action == BLE_EXT_ADV_PENDING_ACTION_PA_UPDATE)
                {
                    p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_IDLE;
                }
            }
            if (p_data->p_le_pa_adv_start_setting_rsp->cause != GAP_CAUSE_SUCCESS)
            {
                BTM_PRINT_ERROR1("GAP_MSG_LE_PA_ADV_START_SETTING: failed, cause 0x%x",
                                 p_data->p_le_pa_adv_start_setting_rsp->cause);
            }
            if (ext_adv_mgr.state == BLE_EXT_NONE_STATE)
            {
                ble_ext_adv_mgr_handle_next_step();
            }
        }
        break;
    case GAP_MSG_LE_PA_ADV_STATE_CHANGE_INFO:
        {
            ble_ext_adv_mgr_hdl_pa_state(p_data->p_le_pa_adv_state_change_info->adv_handle,
                                         (T_GAP_PA_ADV_STATE)p_data->p_le_pa_adv_state_change_info->state,
                                         p_data->p_le_pa_adv_state_change_info->cause);
        }
        break;
#endif
    default:
        break;

    }
    if (ext_adv_mgr.state == BLE_EXT_WAIT4PENDING_STATE &&
        !ble_ext_adv_mgr_is_adv_action_pending())
    {
        check_suspend_or_resume_state();
    }
}

static T_GAP_CAUSE suspend_or_resume_action(bool is_suspend)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    uint8_t adv_handles[BLE_EXTEND_ADV_NUM_MAX] = {0};
    uint8_t idx = 0;
    uint8_t i;

    for (i = 0; i < BLE_EXTEND_ADV_NUM_MAX; i++)
    {
        p_adv = &ble_ext_adv_set_table[i];
        if (p_adv != NULL && p_adv->is_used)
        {
            if (is_suspend)
            {
                if (p_adv->ext_adv_state == EXT_ADV_STATE_ADVERTISING)
                {
                    adv_handles[idx++] = p_adv->adv_handle;
                    p_adv->state_before_suspend = p_adv->ext_adv_state;
                }
            }
            else
            {
                if (p_adv->state_before_suspend == EXT_ADV_STATE_ADVERTISING)
                {
                    adv_handles[idx++] = p_adv->adv_handle;
                    p_adv->state_before_suspend = EXT_ADV_STATE_IDLE;
                }
            }
        }
    }
    if (idx == 0)
    {
        BTM_PRINT_INFO1("suspend_or_resume_action: no adv need handle is_suspend %d", is_suspend);
        if (is_suspend)
        {
            ext_adv_mgr.state = BLE_EXT_SUSPEND_STATE;
#if BLE_PRIVACY_SPT
            handle_adv_stop_in_suspend();
#endif
        }
        else
        {
            ext_adv_mgr.state = BLE_EXT_NONE_STATE;
            //restart actions in queue
            ble_ext_adv_mgr_handle_next_step();
        }
        return result;
    }
    if (is_suspend)
    {
        result = le_ext_adv_disable(idx, adv_handles);
        if (result != GAP_CAUSE_SUCCESS)
        {
            BTM_PRINT_ERROR1("suspend_or_resume_action: disable advs fail, 0x%x", result);
            //FIX TODO
            //change state
        }
        else
        {
            ext_adv_mgr.state = BLE_EXT_SUSPENDING_STATE;
        }
    }
    else
    {
        result = le_ext_adv_enable(idx, adv_handles);
        if (result != GAP_CAUSE_SUCCESS)
        {
            BTM_PRINT_ERROR1("suspend_or_resume_action: enable advs fail, 0x%x", result);
            //FIX TODO
            //change state
        }
        else
        {
            ext_adv_mgr.state = BLE_EXT_RESUME_STATE;
        }
    }
    return result;
}

static uint8_t check_suspend_or_resume_state()
{
    APP_PRINT_INFO1("check_suspend_or_resume_state: state 0x%x", ext_adv_mgr.state);

    if (ext_adv_mgr.state == BLE_EXT_WAIT4PENDING_STATE ||
        ext_adv_mgr.state == BLE_EXT_SUSPENDING_STATE)
    {
        suspend_or_resume_action(true);
    }
    else if (ext_adv_mgr.state == BLE_EXT_SPDING2RSM_STATE ||
             ext_adv_mgr.state == BLE_EXT_RESUME_STATE)
    {
        suspend_or_resume_action(false);
    }

    return ext_adv_mgr.state;
}

//If ext_adv_mgr.state is BLE_EXT_SUSPEND_STATE at last,
//return true to inform caller suspend success
bool ble_ext_adv_suspend()
{
    if (ext_adv_mgr.state == BLE_EXT_NONE_STATE)
    {
        if (!ble_ext_adv_mgr_is_adv_action_pending())
        {
            suspend_or_resume_action(true);
        }
        else
        {
            ext_adv_mgr.state = BLE_EXT_WAIT4PENDING_STATE;
        }
    }
    else if (ext_adv_mgr.state == BLE_EXT_SPDING2RSM_STATE ||
             ext_adv_mgr.state == BLE_EXT_RESUME_STATE)
    {
        ext_adv_mgr.state = BLE_EXT_SUSPENDING_STATE;
    }

    BTM_PRINT_INFO1("ble_ext_adv_suspend: state %d", ext_adv_mgr.state);
    return ext_adv_mgr.state == BLE_EXT_SUSPEND_STATE ? true : false;
}

void ble_ext_adv_resume()
{
    if (ext_adv_mgr.state == BLE_EXT_WAIT4PENDING_STATE)
    {
        ext_adv_mgr.state = BLE_EXT_NONE_STATE;
        ble_ext_adv_mgr_handle_next_step();
        return;
    }
    else if (ext_adv_mgr.state == BLE_EXT_SUSPENDING_STATE)
    {
        ext_adv_mgr.state = BLE_EXT_SPDING2RSM_STATE;
    }
    else if (ext_adv_mgr.state == BLE_EXT_SUSPEND_STATE)
    {
        suspend_or_resume_action(false);
    }
    else
    {
        return;
    }
}

#if (PA_FEATURE_SUPPORT == 1)
static void ble_adv_update_pa_state(T_EXT_ADV_CB *p_adv, T_BLE_ADV_STOP_CAUSE stop_cause,
                                    T_BLE_ADV_STATE new_state, uint8_t app_cause)
{

    if (p_adv->ble_pa_state != new_state)
    {
        p_adv->ble_pa_state = new_state;
        if (p_adv->app_callback)
        {
            T_BLE_ADV_CB_DATA cb_data;
            T_BLE_ADV_STATE_CHANGE adv_state;
            adv_state.state = p_adv->ble_pa_state;
            adv_state.stop_cause = stop_cause;
            adv_state.adv_handle = p_adv->adv_handle;
            adv_state.app_cause = app_cause;
            cb_data.p_ble_state_change = &adv_state;
            p_adv->app_callback(BLE_PA_STATE_CHANGE, &cb_data);
        }
    }
}

static T_GAP_CAUSE ble_ext_adv_start_pa_action(T_EXT_ADV_CB *p_adv)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    ret = le_pa_adv_set_periodic_adv_enable(p_adv->adv_handle, 1);

    BTM_PRINT_INFO1("ble_ext_adv_start_pa_action: adv_handle %d", p_adv->adv_handle);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR1("ble_ext_adv_start_pa_action: failed, ret 0x%x", ret);
        if (p_adv->ble_pa_state == BLE_ADV_STATE_ADVERTISING)
        {
            ble_adv_update_pa_state(p_adv, BLE_ADV_STOP_CAUSE_ENABLE_FAILED,
                                    BLE_ADV_STATE_IDLE, 0);
        }
    }
    else
    {
        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_PA_START;
    }
    return ret;
}

static T_GAP_CAUSE ble_ext_adv_stop_pa_action(T_EXT_ADV_CB *p_adv, uint8_t cause)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    ret = le_pa_adv_set_periodic_adv_enable(p_adv->adv_handle, 0);

    BTM_PRINT_INFO1("ble_ext_adv_stop_pa_action: adv_handle %d", p_adv->adv_handle);
    p_adv->app_cause = cause;

    if (ret != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR1("ble_ext_adv_stop_pa_action: failed, ret 0x%x", ret);
    }
    else
    {
        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_PA_STOP;
    }
    return ret;
}


static T_GAP_CAUSE ble_ext_adv_update_pa_action(T_EXT_ADV_CB *p_adv, uint8_t update_flags)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;

    ret = le_pa_adv_start_setting(p_adv->adv_handle, update_flags);

    BTM_PRINT_INFO2("ble_ext_adv_update_pa_action: adv_handle %d, adv_param_set %d, update_flags 0x%x",
                    p_adv->adv_handle, update_flags);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR1("ble_ext_adv_update_pa_action: failed, ret 0x%x",
                         ret);
    }
    else
    {
        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_PA_UPDATE;
    }
    return ret;
}


T_GAP_CAUSE ble_ext_adv_mgr_start_pa_setting(uint8_t adv_handle, uint8_t update_flags)
{
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    T_EXT_ADV_CB *p_adv;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_ext_adv_mgr_start_pa_setting: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }

    if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state != BLE_EXT_NONE_STATE)
    {
        return send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_PA_UPDATE, update_flags);
    }

    result = ble_ext_adv_update_pa_action(p_adv, update_flags);

    return result;
}

#endif

T_GAP_CAUSE ble_ext_adv_mgr_create_pa(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                                      uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop,
                                      uint16_t periodic_adv_data_len, uint8_t *p_periodic_adv_data,
                                      bool pa_unchanged_data_flag)
{
#if (PA_FEATURE_SUPPORT == 1)
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_ext_adv_mgr_create_pa: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }
    result = le_pa_adv_set_periodic_adv_param(adv_handle, periodic_adv_interval_min,
                                              periodic_adv_interval_max, periodic_adv_prop);
    if (result != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR0("ble_ext_adv_mgr_create_pa: set adv param fail");
        return result;
    }

    if (p_periodic_adv_data != NULL && periodic_adv_data_len <= ext_adv_mgr.max_ext_adv_len)
    {
        p_adv->p_periodic_adv_data = (uint8_t *)os_mem_zalloc(RAM_TYPE_BT_UAL, periodic_adv_data_len);
        if (periodic_adv_data_len > 0 && p_adv->p_periodic_adv_data)
        {
            memcpy(p_adv->p_periodic_adv_data, p_periodic_adv_data, periodic_adv_data_len);
            le_pa_adv_set_periodic_adv_data(adv_handle, periodic_adv_data_len, p_adv->p_periodic_adv_data,
                                            pa_unchanged_data_flag);
        }
    }

    return result;
#else
    return GAP_CAUSE_NO_RESOURCE;
#endif
}


T_GAP_CAUSE ble_ext_adv_mgr_alloc_pa(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                                     uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop,
                                     uint16_t periodic_adv_data_len, uint8_t *p_periodic_adv_data)
{
#if (PA_FEATURE_SUPPORT == 1)
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    uint8_t update_flags = 0;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_adv_mgr_alloc_pa: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }
    result = le_pa_adv_set_periodic_adv_param(adv_handle, periodic_adv_interval_min,
                                              periodic_adv_interval_max, periodic_adv_prop);
    if (result != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR0("ble_adv_mgr_alloc_pa: set adv param fail");
        return result;
    }
    update_flags |= PA_ADV_SET_PERIODIC_ADV_PARAS;

    if (p_periodic_adv_data != NULL && periodic_adv_data_len <= ext_adv_mgr.max_ext_adv_len)
    {
        p_adv->p_periodic_adv_data = (uint8_t *)os_mem_zalloc(RAM_TYPE_BT_UAL, periodic_adv_data_len);
        if (periodic_adv_data_len > 0 && p_adv->p_periodic_adv_data)
        {
            memcpy(p_adv->p_periodic_adv_data, p_periodic_adv_data, periodic_adv_data_len);
            update_flags |= PA_ADV_SET_PERIODIC_ADV_DATA;
            le_pa_adv_set_periodic_adv_data(adv_handle, periodic_adv_data_len, p_adv->p_periodic_adv_data,
                                            false);
        }
    }

    ble_ext_adv_mgr_start_pa_setting(adv_handle, update_flags);
    return result;
#else
    return GAP_CAUSE_NO_RESOURCE;
#endif
}


T_GAP_CAUSE ble_enable_pa(uint8_t adv_handle)
{
#if (PA_FEATURE_SUPPORT == 1)
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_enable_pa: adv_handle not created");
        result = GAP_CAUSE_NOT_FIND;
        return result;
    }

    BTM_PRINT_INFO1("ble_enable_pa: adv_handle %d", adv_handle);
    if (p_adv->ble_pa_state == BLE_ADV_STATE_IDLE)
    {
        if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state == BLE_EXT_SUSPEND_STATE)
        {
            result = send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_PA_START, 0);
        }
        else
        {
            if (p_adv->pa_state == PA_ADV_STATE_IDLE)
            {
                result = ble_ext_adv_start_pa_action(p_adv);
            }
        }
    }
    else
    {
        BTM_PRINT_ERROR1("ble_enable_pa invalid ble adv mgr adv state %d ",
                         p_adv->ble_pa_state);
        result = GAP_CAUSE_INVALID_STATE;
    }

    if (result == GAP_CAUSE_SUCCESS)
    {
        ble_adv_update_pa_state(p_adv, (T_BLE_ADV_STOP_CAUSE)0, BLE_ADV_STATE_ADVERTISING, 0);
    }

    return result;
#else
    return GAP_CAUSE_NO_RESOURCE;
#endif

}

T_GAP_CAUSE ble_disable_pa(uint8_t adv_handle, uint8_t app_cause)
{
#if (PA_FEATURE_SUPPORT == 1)
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_disable_pa: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }

    BTM_PRINT_INFO2("ble_disable_pa: adv_handle %d, app_cause 0x%x", adv_handle, app_cause);

    if (p_adv->ble_pa_state == BLE_ADV_STATE_ADVERTISING)
    {
        if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state == BLE_EXT_SUSPEND_STATE)
        {
            result = send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_PA_STOP, app_cause);
        }
        else
        {
            if (p_adv->pa_state == PA_ADV_STATE_ADVERTISING)
            {
                result = ble_ext_adv_stop_pa_action(p_adv, app_cause);
            }
        }
    }
    else
    {
        BTM_PRINT_ERROR1("ble_disable_pa: invalid ble adv mgr adv state %d ",
                         p_adv->ble_ext_adv_mgr_adv_state);
    }

    if (result == GAP_CAUSE_SUCCESS && app_cause != APP_STOP_FOR_UPDATE)
    {
        ble_adv_update_pa_state(p_adv, BLE_ADV_STOP_CAUSE_APP, BLE_ADV_STATE_IDLE,
                                app_cause);
    }
    return result;
#else
    return GAP_CAUSE_NO_RESOURCE;
#endif
}


T_GAP_CAUSE ble_ext_adv_mgr_update_pa_param(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                                            uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop)
{
#if (PA_FEATURE_SUPPORT == 1)
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_adv_mgr_alloc_pa: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }

    if (p_adv->periodic_adv_interval_min == periodic_adv_interval_min &&
        p_adv->periodic_adv_interval_max == periodic_adv_interval_max &&
        p_adv->periodic_adv_prop == periodic_adv_prop)
    {
        return GAP_CAUSE_SUCCESS;
    }

    if (p_adv->ble_pa_state == BLE_ADV_STATE_ADVERTISING)
    {
        ble_disable_pa(adv_handle, APP_STOP_FOR_UPDATE);
        ble_ext_adv_mgr_start_pa_setting(adv_handle, PA_ADV_SET_PERIODIC_ADV_PARAS);
        send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_PA_START, p_adv->duration);
    }
    else
    {
        ble_ext_adv_update_pa_action(p_adv, PA_ADV_SET_PERIODIC_ADV_PARAS);
    }
    return result;
#else
    return GAP_CAUSE_NO_RESOURCE;
#endif

}

T_GAP_CAUSE ble_ext_adv_mgr_set_pa_data(uint8_t adv_handle, uint16_t periodic_adv_data_len,
                                        uint8_t *p_periodic_adv_data, bool pa_unchanged_data_flag)
{
#if (PA_FEATURE_SUPPORT == 1)
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("ble_ext_adv_mgr_set_pa_data: adv_handle not created");
        return GAP_CAUSE_NOT_FIND;
    }

    if (!pa_unchanged_data_flag)
    {
        if (p_adv->periodic_adv_data_len < periodic_adv_data_len)
        {
            uint8_t *periodic_advdata = (uint8_t *)os_mem_zalloc(RAM_TYPE_BT_UAL, periodic_adv_data_len);

            if (periodic_advdata == NULL)
            {
                BTM_PRINT_ERROR0("ble_ext_adv_mgr_set_pa_data: alloc fail");
                return GAP_CAUSE_NO_RESOURCE;
            }

            if (p_adv->p_periodic_adv_data != NULL)
            {
                os_mem_free(p_adv->p_periodic_adv_data);
            }
            p_adv->p_periodic_adv_data = periodic_advdata;
        }
        p_adv->periodic_adv_data_len = periodic_adv_data_len;
        memcpy(p_adv->p_periodic_adv_data, p_periodic_adv_data, periodic_adv_data_len);
    }
    le_pa_adv_set_periodic_adv_data(adv_handle, p_adv->periodic_adv_data_len,
                                    p_adv->p_periodic_adv_data, pa_unchanged_data_flag);

    ble_ext_adv_mgr_start_pa_setting(adv_handle, PA_ADV_SET_PERIODIC_ADV_DATA);
    return result;
#else
    return GAP_CAUSE_NO_RESOURCE;
#endif
}


#if F_BT_LE_5_2_ISOC_BIS_SUPPORT
static T_EXT_ADV_CB *ble_big_find_by_big_handle(uint8_t big_handle)
{
    uint8_t i;
    T_EXT_ADV_CB *p_adv;

    for (i = 0; i < BLE_EXTEND_ADV_NUM_MAX; i++)
    {
        p_adv = &ble_ext_adv_set_table[i];

        if (p_adv->is_used && p_adv->big_used &&
            p_adv->big_handle == big_handle)
        {
            return p_adv;
        }
    }

    BTM_PRINT_ERROR1("ble_big_find_by_big_handle: not found big_handle %d", big_handle);
    return NULL;
}

static T_GAP_CAUSE ble_big_create_action(T_EXT_ADV_CB *p_adv)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    T_BIG_RES_DATA info;
    info.big_alloc_info.adv_handle = p_adv->adv_handle;
    if (p_adv->big_test_mode)
    {
        ret = gap_big_mgr_isoc_broadcaster_create_big_test(p_adv->adv_handle,
                                                           (T_BIG_MGR_ISOC_BROADCASTER_CREATE_BIG_TEST_PARAM *)(&p_adv->big_test_param),
                                                           &p_adv->big_handle);
    }
    else
    {
        ret = gap_big_mgr_isoc_broadcaster_create_big(p_adv->adv_handle,
                                                      (T_BIG_MGR_ISOC_BROADCASTER_CREATE_BIG_PARAM *)(&p_adv->big_param),
                                                      &p_adv->big_handle);
    }


    BTM_PRINT_INFO1("ble_big_create_action: adv_handle %d", p_adv->adv_handle);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR1("ble_big_create_action: failed, ret 0x%x", ret);
        info.big_alloc_info.cause = BIG_CAUSE_ERROR;
        p_adv->big_used = false;
    }
    else
    {
        info.big_alloc_info.cause = BIG_CAUSE_SUCCESS;
        info.big_alloc_info.big_handle = p_adv->big_handle;
        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_BIG_CREATING;
    }

    if (p_adv->big_callback)
    {
        p_adv->big_callback(p_adv->adv_handle, BIG_MGR_ALLOC_INFO, &info);
    }
    return ret;
}

static T_GAP_CAUSE ble_big_terminate_action(T_EXT_ADV_CB *p_adv, uint8_t reason)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    ret = gap_big_mgr_isoc_broadcaster_terminate_big(p_adv->big_handle, reason);

    BTM_PRINT_INFO1("ble_big_terminate_action: big_handle %d", p_adv->big_handle);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        BTM_PRINT_ERROR1("ble_big_create_action: failed, ret 0x%x", ret);
    }
    return ret;
}

void ble_big_register_iso_cb_int(P_FUN_LE_BIG_MGR_ISOC_BC_CB cb_pfn)
{
#if F_BT_LE_5_2_SUPPORT
    big_iso_cb = cb_pfn;
#endif
}

T_GAP_CAUSE le_create_big(uint8_t adv_handle, T_BIG_PARAM *p_param, P_ISOC_BROADCAST_CB callback)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("le_create_big: adv_handle not created");
        result = GAP_CAUSE_NOT_FIND;
        return result;
    }

    p_adv->big_used = true;
    p_adv->big_test_mode = false;
    memcpy(&p_adv->big_param, p_param, sizeof(T_BIG_PARAM));
    p_adv->big_callback = callback;

    if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state == BLE_EXT_SUSPEND_STATE)
    {
        result = send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_BIG_CREATE, 0);
    }
    else
    {
        result = ble_big_create_action(p_adv);
    }
    return result;
}


T_GAP_CAUSE le_create_test_big(uint8_t adv_handle, T_BIG_TEST_PARAM *p_param,
                               P_ISOC_BROADCAST_CB callback)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    p_adv = ble_ext_adv_mgr_find_by_adv_handle(adv_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("le_create_test_big: adv_handle not created");
        result = GAP_CAUSE_NOT_FIND;
        return result;
    }

    p_adv->big_used = true;
    p_adv->big_test_mode = true;
    memcpy(&p_adv->big_test_param, p_param, sizeof(T_BIG_TEST_PARAM));
    p_adv->big_callback = callback;

    if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state == BLE_EXT_SUSPEND_STATE)
    {
        result = send_msg_2_queue(adv_handle, BLE_EXT_ADV_ACTION_BIG_CREATE, 0);
    }
    else
    {
        result = ble_big_create_action(p_adv);
    }
    return result;
}

T_GAP_CAUSE le_terminate_big(uint8_t big_handle, uint8_t reason)
{
    T_EXT_ADV_CB *p_adv;
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;
    p_adv = ble_big_find_by_big_handle(big_handle);

    if (p_adv == NULL)
    {
        BTM_PRINT_ERROR0("le_terminate_big: adv_handle not created");
        result = GAP_CAUSE_NOT_FIND;
        return result;
    }

    if (ble_ext_adv_mgr_is_adv_action_pending() || ext_adv_mgr.state == BLE_EXT_SUSPEND_STATE)
    {
        result = send_msg_2_queue(big_handle, BLE_EXT_ADV_ACTION_BIG_TERMINATE, reason);
    }
    else
    {
        result = ble_big_terminate_action(p_adv, reason);
    }
    return result;

}

T_APP_RESULT ble_big_isoc_broadcaster_cb(uint8_t big_handle, uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_BIG_MGR_CB_DATA *p_data = (T_BIG_MGR_CB_DATA *)p_cb_data;
    T_EXT_ADV_CB *p_adv;
    T_BIG_RES_DATA big_data;

    if (big_iso_cb)
    {
        big_iso_cb(big_handle, cb_type, p_cb_data);
    }
    switch (cb_type)
    {
    case MSG_BIG_MGR_SETUP_DATA_PATH:
        {
            p_adv = ble_big_find_by_big_handle(p_data->p_big_mgr_setup_data_path_rsp->big_handle);
            if (p_adv != NULL)
            {
                big_data.big_handle_data_path_rsp.big_handle = p_data->p_big_mgr_setup_data_path_rsp->big_handle;
                big_data.big_handle_data_path_rsp.cause = p_data->p_big_mgr_setup_data_path_rsp->cause !=
                                                          GAP_SUCCESS ?
                                                          BIG_CAUSE_ERROR : BIG_CAUSE_SUCCESS;
                big_data.big_handle_data_path_rsp.bis_conn_handle =
                    p_data->p_big_mgr_setup_data_path_rsp->bis_conn_handle;
                big_data.big_handle_data_path_rsp.data_path =
                    p_data->p_big_mgr_setup_data_path_rsp->data_path_adding_path;
                if (p_adv->big_callback)
                {
                    p_adv->big_callback(p_adv->adv_handle, BIG_MGR_SETUP_DATA_PATH, &big_data);
                }
            }
        }
        break;

    case MSG_BIG_MGR_REMOVE_DATA_PATH:
        {
            p_adv = ble_big_find_by_big_handle(p_data->p_big_mgr_remove_data_path_rsp->big_handle);
            if (p_adv != NULL)
            {
                big_data.big_handle_data_path_rsp.big_handle = p_data->p_big_mgr_remove_data_path_rsp->big_handle;
                big_data.big_handle_data_path_rsp.cause = p_data->p_big_mgr_remove_data_path_rsp->cause !=
                                                          GAP_SUCCESS ?
                                                          BIG_CAUSE_ERROR : BIG_CAUSE_SUCCESS;
                big_data.big_handle_data_path_rsp.bis_conn_handle =
                    p_data->p_big_mgr_remove_data_path_rsp->bis_conn_handle;
                big_data.big_handle_data_path_rsp.data_path =
                    p_data->p_big_mgr_remove_data_path_rsp->data_path_removing_path;
                if (p_adv->big_callback)
                {
                    p_adv->big_callback(p_adv->adv_handle, BIG_MGR_REMOVE_DATA_PATH, &big_data);
                }
            }
        }
        break;

    case MSG_BIG_MGR_ISOC_BROADCAST_STATE_CHANGE_INFO:
        {
            BTM_PRINT_INFO4("ble_big_isoc_broadcaster_cb BIG_BROADCAST_STATE_CHANGE_INFO: big_handle 0x%x, adv_handle 0x%x, cause 0x%x, state 0x%x",
                            p_data->p_big_mgr_isoc_broadcast_state_change_info->big_handle,
                            p_data->p_big_mgr_isoc_broadcast_state_change_info->adv_handle,
                            p_data->p_big_mgr_isoc_broadcast_state_change_info->cause,
                            p_data->p_big_mgr_isoc_broadcast_state_change_info->new_state);
            if ((p_adv = ble_big_find_by_big_handle(
                             p_data->p_big_mgr_isoc_broadcast_state_change_info->big_handle)) == NULL)
            {
                BTM_PRINT_WARN2("ble_big_isoc_broadcaster_cb BIG_BROADCAST_STATE_CHANGE_INFO: can't find by big_handle 0x%x,  try to find by adv_handle 0x%x",
                                p_data->p_big_mgr_isoc_broadcast_state_change_info->big_handle,
                                p_data->p_big_mgr_isoc_broadcast_state_change_info->adv_handle);
                p_adv = ble_ext_adv_mgr_find_by_adv_handle(
                            p_data->p_big_mgr_isoc_broadcast_state_change_info->adv_handle);
            }
            if (p_adv != NULL)
            {
                big_data.big_bc_state_change_info.big_handle =
                    p_data->p_big_mgr_isoc_broadcast_state_change_info->big_handle;
                big_data.big_bc_state_change_info.cause = p_data->p_big_mgr_isoc_broadcast_state_change_info->cause
                                                          != GAP_SUCCESS ?
                                                          BIG_CAUSE_ERROR : BIG_CAUSE_SUCCESS;
                big_data.big_bc_state_change_info.adv_handle =
                    p_data->p_big_mgr_isoc_broadcast_state_change_info->adv_handle;
                big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_IDLE;
                T_GAP_BIG_ISOC_BROADCAST_STATE big_state =
                    p_data->p_big_mgr_isoc_broadcast_state_change_info->new_state;
                if (p_adv->pending_action == BLE_EXT_ADV_PENDING_ACTION_BIG_CREATING)
                {
                    BTM_PRINT_INFO1("pending_action creating big_state 0x%x", big_state);
                    if (big_state != BIG_ISOC_BROADCAST_STATE_CREATING_EXT_ADV_STATE_PA_ADV_STATE_IDLE &&
                        big_state != BIG_ISOC_BROADCAST_STATE_CREATING_PA_ADV_STATE_IDLE &&
                        big_state != BIG_ISOC_BROADCAST_STATE_CREATING_EXT_ADV_STATE_IDLE &&
                        big_state != BIG_ISOC_BROADCAST_STATE_CREATING)
                    {
                        p_adv->pending_action = BLE_EXT_ADV_PENDING_ACTION_IDLE;
                        if (ext_adv_mgr.state != BLE_EXT_NONE_STATE)
                        {
                            check_suspend_or_resume_state();
                        }
                        else
                        {
                            ble_ext_adv_mgr_handle_next_step();
                        }
                    }
                }

                switch (big_state)
                {
                case BIG_ISOC_BROADCAST_STATE_IDLE:
                    {
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_IDLE;
                    }
                    break;
                case BIG_ISOC_BROADCAST_STATE_CREATING_EXT_ADV_STATE_PA_ADV_STATE_IDLE:
                    {
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_CREATING;
                    }
                    break;
                case BIG_ISOC_BROADCAST_STATE_WAIT_EXT_ADV_STATE_PA_ADV_STATE_ADVERTISING:
                    {
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_WAITING;
                    }
                    break;
                case BIG_ISOC_BROADCAST_STATE_CREATING_PA_ADV_STATE_IDLE:
                    {
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_CREATING |
                                                                           BIG_ISOC_EXT_ADVERTISING;
                    }
                    break;
                case BIG_ISOC_BROADCAST_STATE_WAIT_PA_ADV_STATE_ADVERTISING:
                    {
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_WAITING |
                                                                           BIG_ISOC_EXT_ADVERTISING;
                    }
                    break;
                case BIG_ISOC_BROADCAST_STATE_CREATING_EXT_ADV_STATE_IDLE:
                    {
                        //PA is enabled, but ext adv is idle, when the ext adv enabled, PA is advertising
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_CREATING;
                    }
                    break;
                case BIG_ISOC_BROADCAST_STATE_WAIT_EXT_ADV_STATE_ADVERTISING:
                    {
                        //PA is enabled, but ext adv is idle, when the ext adv enabled, PA is advertising
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_WAITING;
                    }
                    break;
                case BIG_ISOC_BROADCAST_STATE_CREATING:
                    {
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_CREATING;
                    }
                    break;
                case BIG_ISOC_BROADCAST_STATE_BROADCASTING:
                    {
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_BROADCASTING;
                    }
                    break;
                case BIG_ISOC_BROADCAST_STATE_TERMINATING:
                    {
                        big_data.big_bc_state_change_info.new_comb_state = BIG_ISOC_BC_STATE_TERMINATING;
                    }
                    break;
                default:
                    break;
                }

                if (p_adv->big_callback)
                {
                    p_adv->big_callback(p_adv->adv_handle, BIG_MGR_ISOC_STATE_CHANGE_INFO, &big_data);
                }
            }
            else
            {
                BTM_PRINT_ERROR0("MSG_BIG_MGR_ISOC_BROADCAST_STATE_CHANGE_INFO can't find adv");
            }
        }
        break;

    case MSG_BIG_MGR_ISOC_BROADCASTER_CREATE_BIG_CMPL_INFO:
        {
            p_adv = ble_big_find_by_big_handle(p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->big_handle);
            if (p_adv != NULL)
            {
                big_data.big_bc_create_big_cmpl_info.big_handle =
                    p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->big_handle;
                big_data.big_bc_create_big_cmpl_info.cause = p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->cause
                                                             != GAP_SUCCESS ?
                                                             BIG_CAUSE_ERROR : BIG_CAUSE_SUCCESS;
                big_data.big_bc_create_big_cmpl_info.adv_handle =
                    p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->adv_handle;
                big_data.big_bc_create_big_cmpl_info.big_sync_delay =
                    p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->big_sync_delay;
                big_data.big_bc_create_big_cmpl_info.transport_latency_big =
                    p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->transport_latency_big;
                big_data.big_bc_create_big_cmpl_info.phy = p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->phy;
                big_data.big_bc_create_big_cmpl_info.nse = p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->nse;
                big_data.big_bc_create_big_cmpl_info.bn = p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->bn;
                big_data.big_bc_create_big_cmpl_info.pto = p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->pto;
                big_data.big_bc_create_big_cmpl_info.irc = p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->irc;
                big_data.big_bc_create_big_cmpl_info.max_pdu =
                    p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->max_pdu;
                big_data.big_bc_create_big_cmpl_info.iso_interval =
                    p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->iso_interval;
                big_data.big_bc_create_big_cmpl_info.num_bis =
                    p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->num_bis;
                memcpy(big_data.big_bc_create_big_cmpl_info.bis_conn_handle_info,
                       p_data->p_big_mgr_isoc_bc_create_big_cmpl_info->bis_conn_handle_info,
                       GAP_BIG_MGR_MAX_BIS_NUM * sizeof(T_BIG_BIS_CONN_HANDLE_INFO));
                if (p_adv->big_callback)
                {
                    p_adv->big_callback(p_adv->adv_handle, BIG_MGR_ISOC_CREATE_CMPL_INFO, &big_data);
                }
            }
        }
        break;

    default:
        break;
    }
    return result;
}

#endif

static void ble_ext_adv_mgr_handle_next_step(void)
{
    T_EXT_ADV_CB *p_adv;
    T_EXT_ADV_EVENT *p_extend_adv_event;
    T_GAP_CAUSE ret = GAP_CAUSE_ALREADY_IN_REQ;

    /*pop queue element*/
    while ((p_extend_adv_event = os_queue_out(&extend_adv_event_handle)) != NULL)
    {
        BTM_PRINT_INFO2("ble_ext_adv_mgr_handle_next_step: adv_handle %d, expected_action %d",
                        p_extend_adv_event->adv_handle,
                        p_extend_adv_event->expected_action);

        /*find p_adv by adv_handle in queue*/
        p_adv = ble_ext_adv_mgr_find_by_adv_handle(p_extend_adv_event->adv_handle);

        if (p_adv == NULL)
        {
            BTM_PRINT_ERROR0("ble_ext_adv_mgr_handle_next_step: adv_handle not created");
            os_mem_free(p_extend_adv_event);
            continue;
        }
        else
        {
            /*handle expected_action*/
            switch (p_extend_adv_event->expected_action)
            {
            case BLE_EXT_ADV_ACTION_START:
                p_adv->pending_start_stop_num--;
                if (p_adv->ext_adv_state == EXT_ADV_STATE_IDLE)
                {
                    ret = ble_ext_adv_start_action(p_adv, p_extend_adv_event->param);
                }
                else
                {
                    BTM_PRINT_ERROR1("ble_ext_adv_mgr_handle_next_step: BLE_EXT_ADV_ACTION_START invalid state %d",
                                     p_adv->ext_adv_state);
                }
                break;

            case BLE_EXT_ADV_ACTION_STOP:
                p_adv->pending_start_stop_num--;
                if (p_adv->ext_adv_state == EXT_ADV_STATE_ADVERTISING)
                {
                    ret = ble_ext_adv_stop_action(p_adv, p_extend_adv_event->param);
                }
                else
                {
                    BTM_PRINT_ERROR1("ble_ext_adv_mgr_handle_next_step: BLE_EXT_ADV_ACTION_STOP invalid state %d",
                                     p_adv->ext_adv_state);
                }
                break;
            case BLE_EXT_ADV_ACTION_UPDATE:
                {
                    ret = ble_ext_adv_update_action(p_adv, p_extend_adv_event->param);
                }
                break;
            case BLE_EXT_ADV_ACTION_REMOVE:
                {
                    ret = ble_ext_adv_remove_action(p_adv);
                }
                break;
#if (PA_FEATURE_SUPPORT == 1)
            case BLE_EXT_ADV_ACTION_PA_START:
                {
                    if (p_adv->pa_state == PA_ADV_STATE_IDLE)
                    {
                        ret = ble_ext_adv_start_pa_action(p_adv);
                    }
                }
                break;
            case BLE_EXT_ADV_ACTION_PA_UPDATE:
                {
                    ret = ble_ext_adv_update_pa_action(p_adv, p_extend_adv_event->param);
                }
                break;
            case BLE_EXT_ADV_ACTION_PA_STOP:
                {
                    if (p_adv->pa_state == PA_ADV_STATE_ADVERTISING)
                    {
                        ret = ble_ext_adv_stop_pa_action(p_adv, p_extend_adv_event->param);
                    }
                }
                break;
#endif
#if F_BT_LE_5_2_ISOC_BIS_SUPPORT
            case BLE_EXT_ADV_ACTION_BIG_CREATE:
                {
                    ret = ble_big_create_action(p_adv);
                }
                break;
            case BLE_EXT_ADV_ACTION_BIG_TERMINATE:
                {
                    ble_big_terminate_action(p_adv, (uint8_t)p_extend_adv_event->param);
                    ret = GAP_CAUSE_ERROR_UNKNOWN;    //NO need to hang on
                }
                break;
#endif
            }

            if (p_adv->pending_start_stop_num == 0 && ret != GAP_CAUSE_SUCCESS)
            {
                if (p_extend_adv_event->expected_action == BLE_EXT_ADV_ACTION_START ||
                    p_extend_adv_event->expected_action == BLE_EXT_ADV_ACTION_STOP)
                {
                    ble_ext_adv_mgr_hdl_state_change(p_adv, 0, false);
                }
            }

            if (ret != GAP_CAUSE_SUCCESS)
            {
                os_mem_free(p_extend_adv_event);
                continue;
            }
        }
        os_mem_free(p_extend_adv_event);
        break;
    }
}

