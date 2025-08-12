#include "app_bqb_ble.h"
#include "le_bg_conn_mgr.h"
#include "app_src_bond_storage_le.h"
#include "app_src_storage_flash_le.h"
#include "app_ctrl_cfg.h"
#include "gap.h"
#include "gap_le.h"
#include "gap_le_types.h"
#include "gap_scan.h"
#include "gap_conn_le.h"
#include "gap_bond_le.h"
#include "gap_callback_le.h"
#include "profile_client.h"
//#include "hogp_client.h"
#include "trace.h"
#include "app_link_util.h"
//#include "app_ble_gap.h"
#include "app_cfg.h"
#include "ble_conn.h"
#include "bqb.h"
#include "hids_client.h"
#include "sps_client.h"

#define BQB_DEFAULT_SCAN_INTERVAL       (0x60)
#define BQB_DEFAULT_SCAN_WINDOW         (0x20)

#define BQB_DEFAULT_CONN_SCAN_INTERVAL  (0x60)
#define BQB_DEFAULT_CONN_SCAN_WINDOW    (0x20)
#define BQB_DEFAULT_CONN_MAX_INTERVAL   (6)
#define BQB_DEFAULT_CONN_MIN_INTERVAL   (6)
#define BQB_DEFAULT_CONN_SLAVE_LATENCY  (0)
#define BQB_DEFAULT_CONN_SVTO           (1000)

static T_GAP_DEV_STATE src_le_state;

#define     DONGLE_GATT_CLIENT_NUM                  4

static void app_bqb_ble_handle_dev_state_change_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_TRACE5("app_bqb_ble_handle_dev_state_change_evt: cause 0x%04x, conn: %d, %d, scan: %d, %d",
                     cause,
                     src_le_state.gap_conn_state,
                     new_state.gap_conn_state,
                     src_le_state.gap_scan_state,
                     new_state.gap_scan_state);

    if (src_le_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_TRACE0("app_bqb_ble_handle_dev_state_change_evt: ble stack ready!");
        }
    }

    if (src_le_state.gap_conn_state != new_state.gap_conn_state)
    {
    }

    if (src_le_state.gap_scan_state != new_state.gap_scan_state)
    {

    }

    src_le_state = new_state;
}

static void app_bqb_ble_handle_new_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                                  uint16_t disc_cause)
{
    T_APP_LE_LINK *p_link;

    APP_PRINT_TRACE3("app_bqb_ble_handle_new_conn_state_evt: conn_id %d, new_state %d, cause 0x%04x",
                     conn_id, new_state, disc_cause);

    p_link = app_find_le_link_by_conn_id(conn_id);
    if (p_link == NULL)
    {
        APP_PRINT_WARN1("app_bqb_ble_handle_new_conn_state_evt: no link for %d", conn_id);
    }

    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTING:
        if (p_link != NULL)
        {
            p_link->state = LE_LINK_STATE_DISCONNECTING;
        }
        break;

    case GAP_CONN_STATE_DISCONNECTED:
        if (p_link != NULL)
        {
            for (uint8_t i = 0; i < p_link->disc_cb_list.count; i++)
            {
                T_LE_DISC_CB_ENTRY *p_entry;
                p_entry = os_queue_peek(&p_link->disc_cb_list, i);
                if (p_entry != NULL && p_entry->disc_callback != NULL)
                {
                    p_entry->disc_callback(conn_id, p_link->local_disc_cause, disc_cause);
                }
            }

            hogp_client_handle_disconnect(conn_id);

            app_free_le_link(p_link);
        }
        break;

    case GAP_CONN_STATE_CONNECTING:
        if (p_link == NULL)
        {
            p_link = app_alloc_le_link_by_conn_id(conn_id);
            if (p_link != NULL)
            {
                p_link->state = LE_LINK_STATE_CONNECTING;
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        if (p_link != NULL)
        {
            if (p_link->state == LE_LINK_STATE_CONNECTING)
            {
                p_link->state = LE_LINK_STATE_CONNECTED;

                le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &p_link->mtu_size, conn_id);

                //le_bond_pair(conn_id);
            }
        }
        break;

    default:
        break;
    }

    ble_handle_conn_state_evt(conn_id, new_state);
}

static void app_bqb_ble_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{

    APP_PRINT_INFO3("app_src_ble_handle_authen_state_evt:conn_id %d, state %d, cause 0x%x",
                    conn_id, new_state, cause);

    if (new_state == GAP_AUTHEN_STATE_COMPLETE)
    {
        T_APP_LE_LINK *p_link;
        p_link = app_find_le_link_by_conn_id(conn_id);
        if (p_link != NULL)
        {
            if (cause == GAP_SUCCESS)
            {
                uint8_t addr[6];
                T_GAP_REMOTE_ADDR_TYPE bd_type;
                uint8_t resolved_addr[6];
                T_GAP_IDENT_ADDR_TYPE resolved_bd_type;

                le_get_conn_addr(conn_id, addr, (uint8_t *)&bd_type);
                if (bd_type == GAP_REMOTE_ADDR_LE_RANDOM)
                {
                    if (le_resolve_random_address(addr, resolved_addr, &resolved_bd_type) == true)
                    {
                        if (resolved_bd_type == GAP_IDENT_ADDR_PUBLIC)
                        {
                            memcpy(p_link->bd_addr, resolved_addr, 6);

                        }
                    }
                }
                else if (bd_type == GAP_REMOTE_ADDR_LE_PUBLIC)
                {
                    memcpy(p_link->bd_addr, addr, 6);
                }
            }
            else
            {
                app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED);
            }
        }
    }

}


void app_bqb_ble_handle_gap_msg(T_IO_MSG *io_msg)
{
    APP_PRINT_TRACE1("app_bqb_ble_handle_gap_msg: subtype %d", io_msg->subtype);
    T_LE_GAP_MSG stack_msg;
    T_APP_LE_LINK *p_link;

    memcpy(&stack_msg, &io_msg->u.param, sizeof(io_msg->u.param));

    switch (io_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            app_bqb_ble_handle_dev_state_change_evt(
                stack_msg.msg_data.gap_dev_state_change.new_state,
                stack_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_bqb_ble_handle_new_conn_state_evt(
                stack_msg.msg_data.gap_conn_state_change.conn_id,
                (T_GAP_CONN_STATE)stack_msg.msg_data.gap_conn_state_change.new_state,
                stack_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            p_link = app_find_le_link_by_conn_id(stack_msg.msg_data.gap_conn_mtu_info.conn_id);
            if (p_link != NULL)
            {
                p_link->mtu_size = stack_msg.msg_data.gap_conn_mtu_info.mtu_size;
            }
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            app_bqb_ble_handle_authen_state_evt(
                stack_msg.msg_data.gap_authen_state.conn_id,
                stack_msg.msg_data.gap_authen_state.new_state,
                stack_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            uint8_t conn_id = stack_msg.msg_data.gap_bond_just_work_conf.conn_id;
            le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("app_src_ble_handle_gap_msg: GAP_MSG_LE_BOND_JUST_WORK");
        }
        break;

    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {
            le_bond_user_confirm(stack_msg.msg_data.gap_bond_user_conf.conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            ble_handle_conn_update_info(stack_msg.msg_data.gap_conn_param_update);
        }
        break;

    default:
        break;
    }
}

static T_APP_RESULT app_bqb_ble_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;

    switch (cb_type)
    {

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
                        p_data->p_le_modify_white_list_rsp->operation,
                        p_data->p_le_modify_white_list_rsp->cause);
        break;

    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        APP_PRINT_INFO5("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, max_tx_octets 0x%x, max_tx_time 0x%x, max_rx_octets 0x%x, max_rx_time 0x%x",
                        p_data->p_le_data_len_change_info->conn_id,
                        p_data->p_le_data_len_change_info->max_tx_octets,
                        p_data->p_le_data_len_change_info->max_tx_time,
                        p_data->p_le_data_len_change_info->max_rx_octets,
                        p_data->p_le_data_len_change_info->max_rx_time);
        break;

    case GAP_MSG_LE_SCAN_INFO:
        APP_PRINT_INFO5("GAP_MSG_LE_SCAN_INFO:adv_type 0x%x, bd_addr %s, remote_addr_type %d, rssi %d, data_len %d",
                        p_data->p_le_scan_info->adv_type,
                        TRACE_BDADDR(p_data->p_le_scan_info->bd_addr),
                        p_data->p_le_scan_info->remote_addr_type,
                        p_data->p_le_scan_info->rssi,
                        p_data->p_le_scan_info->data_len);
        /* User can split interested information by using the function as follow. */
        break;

    case GAP_MSG_LE_CONN_UPDATE_IND:
        APP_PRINT_INFO5("GAP_MSG_LE_CONN_UPDATE_IND: conn_id %d, conn_interval_max 0x%x, conn_interval_min 0x%x, conn_latency 0x%x,supervision_timeout 0x%x",
                        p_data->p_le_conn_update_ind->conn_id,
                        p_data->p_le_conn_update_ind->conn_interval_max,
                        p_data->p_le_conn_update_ind->conn_interval_min,
                        p_data->p_le_conn_update_ind->conn_latency,
                        p_data->p_le_conn_update_ind->supervision_timeout);
        /* if reject the proposed connection parameter from peer device, use APP_RESULT_REJECT. */
        result = APP_RESULT_ACCEPT;
        //result = APP_RESULT_REJECT;
        break;

    default:
        APP_PRINT_ERROR1("app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}

static void app_bqb_ble_gap_param_init(void)
{
    /* Device name and device appearance */
    uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "HID_HOST";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

    /* Scan parameters */
    uint8_t  scan_mode = GAP_SCAN_MODE_ACTIVE;
    uint16_t scan_interval = BQB_DEFAULT_SCAN_INTERVAL;
    uint16_t scan_window = BQB_DEFAULT_SCAN_WINDOW;
    uint8_t  scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t  scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

    T_GAP_LE_CONN_REQ_PARAM conn_req_param;

    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    le_gap_init(MAX_BLE_LINK_NUM);

    /* Set device name and device appearance */
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

    /* Set scan parameters */
    le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                      &scan_filter_policy);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                      &scan_filter_duplicate);

    /* Set conn parameters */
    conn_req_param.scan_interval = BQB_DEFAULT_CONN_SCAN_INTERVAL;
    conn_req_param.scan_window = BQB_DEFAULT_CONN_SCAN_WINDOW;
    conn_req_param.conn_interval_min = BQB_DEFAULT_CONN_MAX_INTERVAL;
    conn_req_param.conn_interval_max = BQB_DEFAULT_CONN_MIN_INTERVAL;
    conn_req_param.conn_latency = BQB_DEFAULT_CONN_SLAVE_LATENCY;
    conn_req_param.supv_tout = BQB_DEFAULT_CONN_SVTO;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    /* Setup the GAP Bond Manager */
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);

    /* register gap message callback */
    le_register_app_cb(app_bqb_ble_gap_callback);
}


#if BTDONGLE_INCLUDE_HOGP_HOST
static T_APP_RESULT dongle_app_general_client_cb(T_CLIENT_ID client_id, uint8_t conn_id,
                                                 void *p_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    APP_PRINT_TRACE0("dongle_app_general_client_cb");
    return result;
}


static void app_hogp_cb(uint8_t conn_id, T_HOGP_RESULT result)
{
    APP_PRINT_TRACE2("app_hogp_cb get callback from conn_id %d, result %d", conn_id, result);
}
#endif


void app_handle_hogp_cmd(uint8_t action, uint8_t *bd_addr)
{
    uint8_t conn_id;
    if (action != BQB_ACTION_HOGP_CONNECT)
    {
        if (!le_get_conn_id(bd_addr, GAP_REMOTE_ADDR_LE_PUBLIC, &conn_id))
        {
            return;
        }
    }
    switch (action)
    {
    case BQB_ACTION_HOGP_CONNECT:
        le_connect(0, bd_addr, GAP_REMOTE_ADDR_LE_PUBLIC, GAP_LOCAL_ADDR_LE_PUBLIC, 1000);
        break;
    case BQB_ACTION_HOGP_HID_DISCOVERY:
        hogp_client_connect(conn_id);
        break;
    case BQB_ACTION_HOGP_SPS_DISCOVERY:
        sps_client_start_discovery(conn_id);
        break;
    case BQB_ACTION_HOGP_SPS_INTER_WIN:
        sps_client_set_scan_interval_window(conn_id, 0x60, 0x30);
        break;
    default:
        break;
    }
}
void app_bqb_ble_init(void)
{
#if BTDONGLE_INCLUDE_HOGP_HOST
    client_init(0 + DONGLE_GATT_CLIENT_NUM);

//    ancs_add_client(ancs_client_cb, MAX_BLE_LINK_NUM);
    /* dis client */
    hogp_client_init(app_hogp_cb);

    /* register general client callback */
    client_register_general_client_cb(dongle_app_general_client_cb);
#endif

    app_bqb_ble_gap_param_init();

    ble_conn_update_init(MAX_BLE_LINK_NUM);

}

