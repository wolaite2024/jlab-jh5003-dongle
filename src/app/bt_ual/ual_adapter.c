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
#include "ual_adapter.h"
#include "ual_errno.h"
#include "ual_upperstack_cfg.h"

#include "btm.h"
#include "gap_callback_le.h"
#include "gap_msg.h"
#include "gap_ext_scan.h"
#include "scan_mgr.h"
#include "adapter_int.h"
#include "inquiry.h"
#include "dev_mgr.h"
#include "app_msg.h"

#include "connection_mgr.h"
#include "ble_privacy.h"
#include "gap_br.h"
#include "app_timer.h"
#include "gap_le.h"
#include "gap_bond_le.h"
#include "gap.h"
#include "ble_extend_adv.h"
#include "ble_legacy_adv.h"
#include "adv_mgr.h"
#include "ual_bluetooth.h"
#include "bt_gatt_client.h"
#include "gap_cig_mgr.h"
//#include "gap_pa_adv.h"
//#include "gap_pa_sync.h"
#include "gap_big_mgr.h"
#include "profile_server.h"
#include "ble_pa_sync.h"
#include "bt_storage_mgr.h"
#include "aes_api.h"
#include "ual_api_types.h"
#include "ble_bg_conn.h"

#include "gatt_builtin_services.h"

#if GATTC_TBL_STORAGE_SUPPORT
#include "gattc_tbl_storage.h"
#endif

#define UAL_MAX_BLE_SRV_NUM                       16
#define UAL_MAX_BLE_CLIENT_NUM                    16
#define UAL_ISOC_CIG_MAX_NUM                      2
#define UAL_ISOC_CIS_MAX_NUM                      4
#define UAL_ISOC_MAX_BIG_HANDLE_NUM               4
#define UAL_ISOC_MAX_BIS_NUM                      4
#define UAL_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM      4
#define UAL_SYNC_RECEIVER_MAX_BIS_NUM             4


#define RAP_CHANGE_TIMER      0
#define RPA_TIMER_MS          (15 * 60 * 1000)
static T_GAP_DEV_STATE gap_dev_state = {0, 0, 0, 0};
T_ADAPTET_CBACK bt_hal_cbacks = NULL;

uint32_t class_of_device = 0x240100;
uint8_t dev_le_name[GAP_DEVICE_NAME_LEN] = "BLE_AUDIO_DONGLE";
uint8_t dev_legacy_name[GAP_DEVICE_NAME_LEN] = "LEGACY_AUDIO_DONGLE";

T_UALIST_HEAD adapter_list =
{
    &adapter_list,
    &adapter_list
};
static uint8_t scan_client_id = 0;

#if BLE_LOCAL_PRIVACY_SPT
static uint8_t adapter_timer_id = 0;
static uint8_t timer_idx_rpa = 0;
#endif
static T_UAL_LE_GAP_CBACK app_le_gap_callback;
static T_UAL_LE_GAP_CBACK app_le_gap_callback2;
static T_UAL_LE_GAP_MSG_CBACK app_le_gap_msg_callback;
static T_GAP_SCAN_MODE app_le_scan_mode = GAP_SCAN_MODE_ACTIVE;

extern void gatt_client_handle_gap_common_cb(uint8_t cb_type, void *p_cb_data);

#if BLE_LOCAL_PRIVACY_SPT
static void change_rpa(T_BT_ADAPTER *p_adapter)
{
    le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, p_adapter->le_addr);
    ble_change_local_rpa(p_adapter->le_addr);
    app_start_timer(&timer_idx_rpa, "rpa timer",
                    adapter_timer_id, RAP_CHANGE_TIMER, 0, false,
                    RPA_TIMER_MS);

}

static void adapter_rpa_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    T_BT_ADAPTER *p_adapter = ualist_first_entry(&adapter_list, T_BT_ADAPTER, list);
    if (timer_evt == RAP_CHANGE_TIMER)
    {
        change_rpa(p_adapter);
    }
}
#endif

void ual_le_register_app_cb(T_UAL_LE_GAP_CBACK cback)
{
    app_le_gap_callback = cback;
}

void ual_le_register_app_cb2(T_UAL_LE_GAP_CBACK cback)
{
    app_le_gap_callback2 = cback;
}

void ual_le_register_gap_msg_cb(T_UAL_LE_GAP_MSG_CBACK cback)
{
    app_le_gap_msg_callback = cback;
}

static T_APP_RESULT ual_le_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;
    T_BT_ADAPTER *adapter = NULL;
    uint8_t to_upper = true;
    bool skip = false;
    //APP_PRINT_INFO1("ual_le_gap_callback: cb_type %d", cb_type);

    if (app_le_gap_callback2 && cb_type == GAP_MSG_LE_CONN_UPDATE_IND)
    {
        /*
            Only handle connection update ind here!
            In order to skip handle connection update ind.
            If lea gaming dongle do conection update,
            it may cause le link disc when earbuds are a2dp streaming.
        */
        app_le_gap_callback2(cb_type, p_cb_data);
        skip = true;
    }

    if (ualist_empty(&adapter_list))
    {
        APP_PRINT_ERROR0("ual_le_gap_callback: No adapter");
        return APP_RESULT_SUCCESS;
    }
    adapter = ualist_first_entry(&adapter_list, T_BT_ADAPTER, list);

    switch (cb_type)
    {
    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        {
            ble_bg_handle_wl_rsp(p_data->p_le_modify_white_list_rsp);
        }
        break;

    case GAP_MSG_LE_SET_RAND_ADDR:
        {
#if BLE_PRIVACY_SPT
            ble_handle_rpa_set_rsp(p_data->p_le_set_rand_addr_rsp);
#endif
        }
        break;

    case GAP_MSG_LE_BOND_MODIFY_INFO:
        {
            dev_mgr_handle_le_bond_info(p_data->p_le_bond_modify_info);
#if GATTC_TBL_STORAGE_SUPPORT
            gattc_tbl_storage_handle_bond_modify(p_data->p_le_bond_modify_info);
#endif
        }
        break;

    case GAP_MSG_LE_ADV_UPDATE_PARAM:
        {
            ble_legacy_adv_mgr_handle_gap_callback(cb_type, p_data);
        }
        break;

    case GAP_MSG_LE_PA_ADV_START_SETTING:
    case GAP_MSG_LE_PA_ADV_SET_PERIODIC_ADV_ENABLE:
    case GAP_MSG_LE_PA_ADV_STATE_CHANGE_INFO:
    case GAP_MSG_LE_EXT_ADV_START_SETTING:
    case GAP_MSG_LE_EXT_ADV_REMOVE_SET:
    case GAP_MSG_LE_EXT_ADV_CLEAR_SET:
    case GAP_MSG_LE_EXT_ADV_ENABLE:
    case GAP_MSG_LE_EXT_ADV_DISABLE:
        {
            ble_ext_adv_mgr_hdl_gap_cb(cb_type, p_data);
        }
        break;

    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE_INFO:
        {
            ble_ext_adv_mgr_hdl_adv_state(p_data->p_le_ext_adv_state_change_info->adv_handle,
                                          (T_GAP_EXT_ADV_STATE)p_data->p_le_ext_adv_state_change_info->state,
                                          p_data->p_le_ext_adv_state_change_info->cause);
        }
        break;

    case GAP_MSG_LE_EXT_ADV_REPORT_INFO:
        {
            to_upper = false;
            ual_handle_ext_adv_report(adapter, p_data->p_le_ext_adv_report_info);
        }
        break;

    case GAP_MSG_LE_EXT_SCAN_STATE_CHANGE_INFO:
        {
            scan_mgr_handle_scan_state_info(p_data->p_le_ext_scan_state_change_info->state,
                                            p_data->p_le_ext_scan_state_change_info->cause);
        }
        break;

    case GAP_MSG_LE_CONN_UPDATE_IND:
        {
            to_upper = false;

            if (skip)
            {
                result = APP_RESULT_REJECT;
            }
            else
            {

                result = ble_handle_conn_update_ind(p_data->p_le_conn_update_ind);
            }
        }
        break;

    default:
        ble_pa_sync_handle_gap_cb(cb_type, p_data);
        break;
    }



    if (to_upper && app_le_gap_callback)
    {
        app_le_gap_callback(cb_type, p_cb_data);
    }

    if (app_le_gap_callback2)
    {
        app_le_gap_callback2(cb_type, p_cb_data);
    }

    return result;
}

static void ual_mgr_cback(T_BT_EVENT event, void *buf, uint16_t len)
{
    T_BT_EVENT_PARAM *param = buf;
    bool handle = true;
    T_BT_ADAPTER *adapter = NULL;

    if (ualist_empty(&adapter_list))
    {
        APP_PRINT_ERROR0("ual_mgr_cback: No adapter");
        return;
    }
    adapter = ualist_first_entry(&adapter_list, T_BT_ADAPTER, list);

    switch (event)
    {
    case BT_EVENT_READY:
        break;

    case BT_EVENT_INQUIRY_RESULT:
        ual_handle_inquiry_result(adapter, param->inquiry_result);
        break;

    case BT_EVENT_INQUIRY_RSP:
        ual_handle_inquiry_rsp(adapter, param->inquiry_rsp.cause);
        break;

    case BT_EVENT_INQUIRY_CANCEL_RSP:
        ual_handle_inquiry_cancel_cmpl(adapter, param->inquiry_cancel_rsp.cause);
        break;

    case BT_EVENT_INQUIRY_CMPL:
        ual_handle_inquiry_cmpl(adapter, param->inquiry_cmpl.cause);
        break;

    default:
        handle = false;
        break;
    }

    if (handle)
    {
        APP_PRINT_INFO1("ual_mgr_cback: event 0x%04x", event);
    }
}

static void ual_handle_ble_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    T_GAP_DEV_STATE old_state;
    APP_PRINT_INFO4("ual_handle_ble_dev_state_evt: init state  %d, adv state %d, scan state %d, gap_conn_state 0x%x",
                    new_state.gap_init_state, new_state.gap_adv_state,
                    new_state.gap_scan_state, new_state.gap_conn_state);
    old_state = gap_dev_state;
    gap_dev_state = new_state;

    if (old_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_INFO0("GAP stack ready");
            ble_scan_mgr_init();

#if BLE_PRIVACY_SPT
            uint8_t enable = 1;
            gaps_set_parameter(GAPS_PARAM_CENTRAL_ADDRESS_RESOLUTION, 1, &enable);
            ble_privacy_init();
#endif
            ble_conn_mgr_init();
            ble_adv_mgr_init();
            ble_pa_sync_init();

#if BLE_LOCAL_PRIVACY_SPT
            T_BT_ADAPTER *adapter;
            T_LOCAL_IRK local_irk;
            memset(&local_irk, 0, sizeof(T_LOCAL_IRK));

            gap_reg_timer_cb(adapter_rpa_timeout_cb, &adapter_timer_id);

            adapter = ualist_first_entry(&adapter_list, T_BT_ADAPTER, list);
            adapter->bd_type = BLE_ADDR_RANDOM;
            change_rpa(adapter);
            flash_load_local_irk(&local_irk);
            memcpy(adapter->irk, local_irk.local_irk, 16);
#endif
            T_BT_STATE adapter_state = BT_STATE_ON;
            bt_storage_load_devices();

            if (bt_hal_cbacks)
            {
                bt_hal_cbacks(UAL_ADP_STATE_CHANGE, (uint8_t *)&adapter_state, 1);
            }
        }
    }

    if (old_state.gap_scan_state != new_state.gap_scan_state)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            APP_PRINT_INFO0("GAP scan stop");
        }
        else if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            APP_PRINT_INFO0("GAP scan start");
        }

        scan_mgr_handle_scan_state_info(new_state.gap_scan_state, cause);
    }

    if (old_state.gap_adv_state != new_state.gap_adv_state)
    {
        //handle legacy advertising
        ble_legacy_adv_mgr_handle_adv_state(new_state, cause);
    }
}

void ual_handle_ble_gap_msg(void *p_msg)
{
    T_IO_MSG *p_gap_msg = (T_IO_MSG *)p_msg;
    T_LE_GAP_MSG gap_msg;
    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

    APP_PRINT_TRACE1("ual_handle_ble_gap_msg: subtype %d", p_gap_msg->subtype);
    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            ual_handle_ble_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
                                         gap_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            ble_conn_mgr_handle_le_conn_state(gap_msg.msg_data.gap_conn_state_change.conn_id,
                                              (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
                                              gap_msg.msg_data.gap_conn_state_change.disc_cause);
            ble_ext_adv_mgr_hdl_conn_state(gap_msg.msg_data.gap_conn_state_change.conn_id,
                                           (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
                                           gap_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            ble_conn_handle_conn_mtu_info_evt(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
                                              gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            ble_handle_conn_update_event(gap_msg.msg_data.gap_conn_param_update);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            dev_mgr_handle_le_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
                                               gap_msg.msg_data.gap_authen_state.new_state,
                                               gap_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE:
        {
            ble_ext_adv_mgr_hdl_adv_state(gap_msg.msg_data.gap_ext_adv_state_change.adv_handle,
                                          (T_GAP_EXT_ADV_STATE)gap_msg.msg_data.gap_ext_adv_state_change.new_state,
                                          gap_msg.msg_data.gap_ext_adv_state_change.cause);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            le_bond_just_work_confirm(gap_msg.msg_data.gap_bond_just_work_conf.conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_JUST_WORK");
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:
        {
            dev_mgr_handle_le_pairing_confirm(gap_msg.msg_data.gap_bond_passkey_display.conn_id,
                                              p_gap_msg->subtype);
        }
        break;
    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {
            dev_mgr_handle_le_pairing_confirm(gap_msg.msg_data.gap_bond_user_conf.conn_id, p_gap_msg->subtype);
        }
        break;
    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
        {
            dev_mgr_handle_le_pairing_confirm(gap_msg.msg_data.gap_bond_passkey_input.conn_id,
                                              p_gap_msg->subtype);
        }
        break;

    }

    if (app_le_gap_msg_callback)
    {
        app_le_gap_msg_callback(p_gap_msg->subtype, gap_msg);
    }
}

bool bt_ual_set_param(uint16_t type, uint8_t len, void *p_value)
{
    if (!p_value)
    {
        return false;
    }

    bool ret = true;
    switch (type)
    {
    case GAP_BR_PARAM_COD:
        if (len == sizeof(uint32_t))
        {
            class_of_device = *((uint32_t *)p_value);
        }
        else
        {
            ret = false;
        }
        break;

    case GAP_PARAM_DEVICE_NAME:
        {
            if (len > GAP_DEVICE_NAME_LEN - 1)
            {
                len = GAP_DEVICE_NAME_LEN - 1;
            }
            memcpy(dev_le_name, p_value, len);
            dev_le_name[len] = 0;
        }
        break;

    case GAP_BR_PARAM_NAME:
        {
            if (len > GAP_DEVICE_NAME_LEN - 1)
            {
                len = GAP_DEVICE_NAME_LEN - 1;
            }
            memcpy(dev_legacy_name, p_value, len);
            dev_legacy_name[len] = 0;
        }
        break;

    default:
        ret = false;
        break;
    }
    return ret;
}

void ual_handle_gap_common_cb(uint8_t cb_type, void *p_cb_data)
{
    gatt_client_handle_gap_common_cb(cb_type, p_cb_data);
}

static T_BT_ADAPTER *bt_adapter_init(void)
{
    T_BT_ADAPTER *adapter;
    adapter = (T_BT_ADAPTER *)os_mem_zalloc(RAM_TYPE_BT_UAL,
                                            sizeof(*adapter));
    if (!adapter)
    {
        return NULL;
    }
    adapter->cod = class_of_device;
    memcpy(adapter->le_name, dev_le_name, GAP_DEVICE_NAME_LEN);
    memcpy(adapter->legacy_name, dev_legacy_name, GAP_DEVICE_NAME_LEN);
    return adapter;
}

static void app_bt_gap_init(T_BT_ADAPTER *p_adapter)
{
    uint16_t supervision_timeout = SUPVISIONTIMEOUT_DEF;
    uint16_t link_policy = GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE;

    uint8_t radio_mode = GAP_RADIO_MODE_NONE_DISCOVERABLE;
    bool limited_discoverable = false;
    bool auto_accept_acl = false;

    uint8_t pagescan_type = GAP_PAGE_SCAN_TYPE_INTERLACED;
    uint16_t pagescan_interval = PAGESCAN_INTERVAL_DEF;
    uint16_t pagescan_window = PAGESCAN_WINDOW_DEF;
    uint16_t page_timeout = PAGE_TIMEOUT_DEF;

    uint8_t inquiryscan_type = GAP_INQUIRY_SCAN_TYPE_INTERLACED;
    uint16_t inquiryscan_window = INQUIRYSCAN_WINDOW_DEF;
    uint16_t inquiryscan_interval = INQUIRYSCAN_INTERVAL_DEF;
    uint8_t inquiry_mode = GAP_INQUIRY_MODE_EXTENDED_RESULT;

    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_GENERAL_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t oob_enable = false;
    uint8_t bt_mode = GAP_BT_MODE_21ENABLED;

    /* Device name and device appearance */
    //uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "BLE_AUDIO_DONGLE";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;

    bool use_extended = true;

    gap_lib_init();
    le_gap_init(2);

    //0: to be master
    gap_br_cfg_accept_role(1);

    gap_br_set_param(GAP_BR_PARAM_NAME, GAP_DEVICE_NAME_LEN, p_adapter->legacy_name);

    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);

    gap_br_set_param(GAP_BR_PARAM_BT_MODE, sizeof(uint8_t), &bt_mode);
    gap_br_set_param(GAP_BR_PARAM_COD, sizeof(uint32_t), &p_adapter->cod);
    gap_br_set_param(GAP_BR_PARAM_LINK_POLICY, sizeof(uint16_t), &link_policy);
    gap_br_set_param(GAP_BR_PARAM_SUPV_TOUT, sizeof(uint16_t), &supervision_timeout);
    gap_br_set_param(GAP_BR_PARAM_AUTO_ACCEPT_ACL, sizeof(bool), &auto_accept_acl);

    gap_br_set_param(GAP_BR_PARAM_RADIO_MODE, sizeof(uint8_t), &radio_mode);
    gap_br_set_param(GAP_BR_PARAM_LIMIT_DISCOV, sizeof(bool), &limited_discoverable);

    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_TYPE, sizeof(uint8_t), &pagescan_type);
    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_INTERVAL, sizeof(uint16_t), &pagescan_interval);
    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_WINDOW, sizeof(uint16_t), &pagescan_window);
    gap_br_set_param(GAP_BR_PARAM_PAGE_TIMEOUT, sizeof(uint16_t), &page_timeout);

    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_TYPE, sizeof(uint8_t), &inquiryscan_type);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_INTERVAL, sizeof(uint16_t), &inquiryscan_interval);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_WINDOW, sizeof(uint16_t), &inquiryscan_window);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_MODE, sizeof(uint8_t), &inquiry_mode);


    bool cis_flag = true;
    le_set_gap_param(GAP_PARAM_CIS_HOST_SUPPORT, sizeof(cis_flag), &cis_flag);

    /* Use LE Advertising Extensions */
    le_set_gap_param(GAP_PARAM_USE_EXTENDED_ADV, sizeof(use_extended), &use_extended);

    /* Set device name and device appearance */
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, p_adapter->le_name);
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);


    /* Setup the GAP Bond Manager */
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);
#if BLE_LOCAL_PRIVACY_SPT
    //le_bond_set_param(GAP_PARAM_BOND_SET_LOCAL_IRK, 16, p_adapter->irk);
#endif
    uint8_t auto_irk = 1;
    le_bond_set_param(GAP_PARAM_BOND_GEN_LOCAL_IRK_AUTO, 1, &auto_irk);
}


void bt_adap_start_task(void *evt_queue, void *io_queue, uint16_t msg_queue_elem_num)
{
    gap_start_bt_stack(evt_queue, io_queue, msg_queue_elem_num);
    le_ext_adv_gap_msg_info_way(false);
    //le_ext_scan_gap_msg_info_way(false);      //use direct callback

#if F_BT_LE_5_2_SUPPORT
    cig_mgr_init(UAL_ISOC_CIG_MAX_NUM, UAL_ISOC_CIS_MAX_NUM);
    server_init(UAL_MAX_BLE_SRV_NUM);

    gap_big_mgr_init(UAL_ISOC_MAX_BIG_HANDLE_NUM, UAL_ISOC_MAX_BIS_NUM);


    gap_big_mgr_isoc_broadcaster_init(UAL_ISOC_MAX_BIG_HANDLE_NUM,
                                      UAL_ISOC_MAX_BIS_NUM, ble_big_isoc_broadcaster_cb);


    gap_big_mgr_sync_receiver_init(UAL_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM, UAL_SYNC_RECEIVER_MAX_BIS_NUM,
                                   ble_big_sync_receiver_cb);


#endif
#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
    gatt_client_init(GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT | GATT_CLIENT_DISCOV_MODE_CCCD_STORAGE_BIT |
                     GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT);
#endif
}
int bt_adap_init(T_ADAPTET_CBACK cb)
{
    T_BT_ADAPTER *adapter = bt_adapter_init();
    if (!adapter)
    {
        return -ENOMEM;
    }
    bt_dev_mgr_init();
    app_bt_gap_init(adapter);
    bt_hal_cbacks = cb;
    init_ualist_head(&adapter->connect_list);
    init_ualist_head(&adapter->device_list);
    init_ualist_head(&adapter->scan_clients);
    init_ualist_head(&adapter->devices);
    init_ualist_head(&adapter->adv_list);
    adapter->max_devices = 10;

    /* FIXME: There is only one le gap cb */
    le_register_app_cb(ual_le_gap_callback);

    ualist_add_tail(&adapter->list, &adapter_list);

    bt_mgr_cback_register(ual_mgr_cback);

#if GATTC_TBL_STORAGE_SUPPORT
    gattc_tbl_storage_init();
#endif
    return 0;
}

void bt_adap_set_scan_mdoe(T_GAP_SCAN_MODE scan_mode)
{
    app_le_scan_mode = scan_mode;
}

T_GAP_SCAN_MODE bt_adap_get_scan_mode(void)
{
    return app_le_scan_mode;
}

int bt_adap_start(void)
{
    return 0;
}

int bt_adap_stop(void)
{
    return 0;
}

struct bt_scan_client *ual_find_scan_client_by_cb(T_BT_ADAPTER *adapter,
                                                  T_SCAN_RESULT_CBACK cb)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    struct bt_scan_client *client;

    if (!adapter)
    {
        return NULL;
    }

    if (ualist_empty(&adapter->scan_clients))
    {
        return NULL;
    }

    ualist_for_each_safe(pos, n, &adapter->scan_clients)
    {
        client = ualist_entry(pos, struct bt_scan_client, list);
        if (client->cback == cb)
        {
            return client;
        }
    }

    return NULL;
}

struct bt_scan_client *ual_find_scan_client_by_id(T_BT_ADAPTER *adapter,
                                                  uint8_t client_id)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    struct bt_scan_client *client;

    if (!adapter)
    {
        return NULL;
    }

    if (ualist_empty(&adapter->scan_clients))
    {
        return NULL;
    }

    ualist_for_each_safe(pos, n, &adapter->scan_clients)
    {
        client = ualist_entry(pos, struct bt_scan_client, list);
        if (client->id == client_id)
        {
            return client;
        }
    }

    return NULL;
}

/* return client id or errno */
int bt_adap_start_discovery(uint8_t disc_type, uint8_t filter_policy, T_SCAN_RESULT_CBACK cb)
{
    T_BT_ADAPTER *adapter = NULL;
    struct bt_scan_client *client = NULL;

    if (!(disc_type & DISCOVERY_TYPE_DUAL))
    {
        return -EINVAL;
    }

    if (ualist_empty(&adapter_list))
    {
        return -ENODEV;
    }

    adapter = ualist_first_entry(&adapter_list, T_BT_ADAPTER, list);

    if (ual_find_scan_client_by_cb(adapter, cb))
    {
        return -EEXIST;
    }

    client = os_mem_zalloc(RAM_TYPE_BT_UAL, sizeof(struct bt_scan_client));
    if (!client)
    {
        return -ENOMEM;
    }
    if (!adapter->inquiring && !adapter->le_scanning)
    {
        clear_disc_db();
    }

    if (disc_type & DISCOVERY_TYPE_BREDR)
    {
        if (!adapter->inquiring)
        {
            if (gap_br_start_inquiry(false, 10))
            {
                APP_PRINT_ERROR0("bt_adap_start_discovery: start inquiry err");
                goto fail;
            }
            adapter->inquiring = 1;
        }
        adapter->inquiry_refcnt++;
    }
    if (disc_type & DISCOVERY_TYPE_LE)
    {
        if (!adapter->le_scanning)
        {
            if (!bt_adap_start_le_scan(app_le_scan_mode, filter_policy))
            {
                APP_PRINT_ERROR0("bt_adap_start_discovery: start lescan err");
                goto fail;
            }
            adapter->le_scanning = 1;
        }
        else
        {
            T_SCAN_STATE scan_state = le_scan_get_scan_state();
            T_BT_DISC_STATE state = LE_SCAN_ON;
            if (scan_state != SCAN_IDLE_STATE)
            {
                if (bt_hal_cbacks)
                {
                    bt_hal_cbacks(UAL_ADP_LE_SCAN_STATE_CHANGE, (uint8_t *)&state, 1);
                }
            }
        }
        adapter->lescan_refcnt++;
    }

    APP_PRINT_INFO2("bt_adap_start_discovery: inquiry_refcnt %d lescan_refcnt %d",
                    adapter->inquiry_refcnt,  adapter->lescan_refcnt);
    //FIX TODO client id may be reused
    if (scan_client_id < 1)
    {
        scan_client_id = 1;
    }

    client->id = scan_client_id++;
    init_ualist_head(&client->list);
    client->discovery_type = disc_type;
    client->cback = cb;

    ualist_add_tail(&client->list, &adapter->scan_clients);

    return client->id;

fail:
    if (client)
    {
        os_mem_free(client);
    }
    return -EIO;
}

int bt_adap_stop_discovery(uint8_t client_id)
{
    T_BT_ADAPTER *adapter = NULL;
    struct bt_scan_client *client = NULL;

    if (ualist_empty(&adapter_list))
    {
        return -ENODEV;
    }

    adapter = ualist_first_entry(&adapter_list, T_BT_ADAPTER, list);

    client = ual_find_scan_client_by_id(adapter, client_id);
    if (!client)
    {
        return -ENOENT;
    }

    if ((client->discovery_type & DISCOVERY_TYPE_BREDR) &&
        adapter->inquiry_refcnt)
    {
        adapter->inquiry_refcnt--;
    }
    if ((client->discovery_type & DISCOVERY_TYPE_LE) &&
        adapter->lescan_refcnt)
    {
        adapter->lescan_refcnt--;
    }

    if (!adapter->inquiry_refcnt)
    {
        if (adapter->inquiring)
        {
            /* clear inquiring in cancel inquiry rsp */
            gap_br_stop_inquiry();
        }
    }

    APP_PRINT_INFO1("bt_adap_stop_discovery: lescan_refcnt %d", adapter->lescan_refcnt);
    if (!adapter->lescan_refcnt)
    {
        if (adapter->le_scanning)
        {
            /* clear le_scanning in scan stopped */
            bt_adap_stop_le_scan();
            adapter->le_scanning = 0;
        }
    }

    ualist_del(&client->list);
    os_mem_free(client);

    return 0;
}

uint8_t ble_get_conn_st()
{
    return gap_dev_state.gap_conn_state;
}

uint8_t ble_get_scan_st()
{
    return gap_dev_state.gap_scan_state;
}

T_BLE_BD_TYPE ble_get_local_bd_type()
{
    T_BT_ADAPTER *p_adapter;
    p_adapter = ualist_first_entry(&adapter_list, T_BT_ADAPTER, list);
    return p_adapter->bd_type;
}

uint8_t *get_adapter_rpa()
{
    T_BT_ADAPTER *p_adapter;
    p_adapter = ualist_first_entry(&adapter_list, T_BT_ADAPTER, list);
    return p_adapter->le_addr;
}


static bool rpa_matches_local_adapter(uint8_t rpa[6], uint8_t irk[16])
{
    uint8_t buffer[16];
    uint8_t irk_temp[16];
    uint8_t encrypt_buffer[16];
    uint8_t j;

    memset(buffer, 0x00, 16);
    buffer[13] = rpa[5];
    buffer[14] = rpa[4];
    buffer[15] = rpa[3];
    for (j = 0; j < 16; j++)
    {
        irk_temp[j] = irk[16 - 1 - j];
    }

    aes128_ecb_encrypt(buffer, irk_temp, encrypt_buffer);

    if ((encrypt_buffer[13] == rpa[2])
        && (encrypt_buffer[14] == rpa[1])
        && (encrypt_buffer[15] == rpa[0]))
    {
        // match
        return true;
    }
    // not a match
    return false;
}

bool ble_rpa_matches_local_adapter(uint8_t *rpa)
{
    T_BT_ADAPTER *p_adapter;
    p_adapter = ualist_first_entry(&adapter_list, T_BT_ADAPTER, list);

    if (p_adapter == NULL || !BLE_IS_RESOLVE_BDA(rpa))
    {
        return false;
    }

    if (rpa_matches_local_adapter(rpa, p_adapter->irk))
    {
        return true;
    }

    return false;
}

bool ual_support_privacy()
{
#if BLE_PRIVACY_SPT
    return true;
#else
    return false;
#endif
}



