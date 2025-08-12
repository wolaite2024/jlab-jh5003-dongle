#include "app_src_ble.h"
#include "le_bg_conn_mgr.h"
#include "app_src_bond_storage_le.h"
#include "app_src_storage_flash_le.h"
#include "app_src_storage_target_dev.h"
#include "app_ctrl_cfg.h"
#include "ual_bluetooth.h"

#include "gap.h"
#include "gap_le.h"
#include "gap_le_types.h"
#include "gap_scan.h"
#include "gap_conn_le.h"
#include "gap_bond_le.h"
#include "section.h"
#include "gap_vendor.h"
//#include "gap_mp.h"

#include "gap_callback_le.h"

#include "profile_client.h"
#include "gaps_client.h"
//#include "hogp_client.h"

#include "trace.h"

#include "app_link_util.h"
//#include "app_ble_gap.h"
#include "app_cfg.h"
//#include "app_report.h"
//#include "ble_conn.h"
#include "app_ctrl_cfg.h"

#include "app_vendor_cfg.h"

#define SRC_DEFAULT_SCAN_INTERVAL       (0x60)
#define SRC_DEFAULT_SCAN_WINDOW         (0x20)

#define SRC_DEFAULT_CONN_SCAN_INTERVAL  (0x60)
#define SRC_DEFAULT_CONN_SCAN_WINDOW    (0x20)
#define SRC_DEFAULT_CONN_MAX_INTERVAL   (6)
#define SRC_DEFAULT_CONN_MIN_INTERVAL   (6)
#define SRC_DEFAULT_CONN_SLAVE_LATENCY  (66)
#define SRC_DEFAULT_CONN_SVTO           (300)

#define NUM_OF_LE_TARGET_DEV            (2)
T_APP_SRC_LE_TARGET_DEV le_target_dev[NUM_OF_LE_TARGET_DEV];

static T_GAP_DEV_STATE src_le_state;

static APP_SRC_HOGP_CONN_RESULT_CB hogp_pair_state_cb = NULL;


#define     DONGLE_GATT_CLIENT_NUM                  3

#define MS_USB_VID   0x045E

#if BTDONGLE_INCLUDE_HOGP_HOST
static T_CLIENT_ID gaps_client_id;
#endif

#if 0
static bool le_target_dev_get_index_by_addr(uint8_t *bd_addr, uint8_t addr_type, uint8_t *index)
{
    uint8_t i;

    for (i = 0; i < NUM_OF_LE_TARGET_DEV; i++)
    {
        if (le_target_dev[i].in_use &&
            !memcmp(le_target_dev[i].bd_addr, bd_addr, 6) &&
            (le_target_dev[i].addr_type == addr_type))
        {
            *index = i;
            return true;
        }
    }

    return false;
}
#endif

static bool le_target_dev_get_index_by_connId(uint8_t conn_id, uint8_t *index)
{
    uint8_t i;

    for (i = 0; i < NUM_OF_LE_TARGET_DEV; i++)
    {
        if (le_target_dev[i].in_use &&
            le_target_dev[i].acl_exist &&
            (le_target_dev[i].conn_id == conn_id))
        {
            *index = i;
            return true;
        }
    }

    return false;
}

static T_APP_SRC_LE_TARGET_DEV *le_target_dev_find_by_addr(uint8_t *bd_addr, uint8_t addr_type)
{
    uint8_t i;

    for (i = 0; i < NUM_OF_LE_TARGET_DEV; i++)
    {
        if (le_target_dev[i].in_use &&
            !memcmp(le_target_dev[i].bd_addr, bd_addr, 6) &&
            (le_target_dev[i].addr_type == addr_type))
        {
            return &(le_target_dev[i]);
        }
    }

    return NULL;
}

static T_APP_SRC_LE_TARGET_DEV *le_target_dev_find_by_connId(uint8_t conn_id)
{
    uint8_t i;

    for (i = 0; i < NUM_OF_LE_TARGET_DEV; i++)
    {
        if (le_target_dev[i].in_use &&
            le_target_dev[i].acl_exist &&
            (le_target_dev[i].conn_id == conn_id))
        {
            return &(le_target_dev[i]);
        }
    }

    return NULL;
}

static uint8_t le_target_dev_get_nolink_num(void)
{
    uint8_t i;
    uint8_t num = 0;

    for (i = 0; i < NUM_OF_LE_TARGET_DEV; i++)
    {
        if (le_target_dev[i].in_use && (!le_target_dev[i].acl_exist))
        {
            num++;
        }
    }

    return num;
}

static bool rf_scheduler_set_dev_slot_vsc(bool enable, uint8_t offset, uint8_t addr_type,
                                          uint8_t *addr)
{
    uint8_t params[10];

    params[0] = 0x15;
    params[1] = enable ? 1 : 0;
    params[2] = offset;
    params[3] = addr_type;
    memcpy(&params[4], addr, 6);

    return gap_vendor_cmd_req(0xfd80, 10, params);
}

static void rf_scheduler_le_dev_enable(bool enable, uint8_t index, uint8_t addr_type,
                                       uint8_t *addr)
{
    uint8_t offset = 0;

    APP_PRINT_INFO4("rf_scheduler_le_dev_enable: %d %d %d %s",
                    enable, index, addr_type, TRACE_BDADDR(addr));

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("rf_scheduler_le_dev_enable: no such device %d", index);
        return;
    }

    if (index == 0)
    {
        offset = 8;
    }
    else if (index == 1)
    {
        offset = 10;
    }

    rf_scheduler_set_dev_slot_vsc(enable, offset, addr_type, addr);
}

void le_conn_target_devices(void)
{
    uint8_t dev_mask = 0;
    uint8_t i;
    uint8_t dev_num_to_conn = 0;

    APP_PRINT_INFO0("le_conn_target_devices");

    //1st. add disconnected device into WL
    for (i = 0; i < NUM_OF_LE_TARGET_DEV; i++)
    {
        if (le_target_dev[i].in_use &&
            !(le_target_dev[i].acl_exist || le_target_dev[i].in_wl))
        {
            dev_mask |= 1 << i;
        }
    }

    //2nd, start reconn
    if (dev_mask != 0)
    {
        le_bg_conn_mgr_disconn();

        for (i = 0; i < NUM_OF_LE_TARGET_DEV; i++)
        {
            if (dev_mask & (1 << i))
            {
                if (le_bg_conn_mgr_wl_add(le_target_dev[i].bd_addr, le_target_dev[i].addr_type))
                {
                    dev_num_to_conn++;
                    le_target_dev[i].in_wl = 1;

                    rf_scheduler_le_dev_enable(true,
                                               i,
                                               le_target_dev[i].addr_type,
                                               le_target_dev[i].bd_addr);
                }
            }
        }

        APP_PRINT_INFO1("le_conn_target_devices: num %d", dev_num_to_conn);

        if (dev_num_to_conn > 0)
        {
            le_bg_conn_mgr_create_conn();
        }
    }
}

static void app_src_ble_conn_new_device(uint8_t index, uint8_t addr_type,
                                        uint8_t *bd_addr, uint32_t appearance)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;


    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("app_src_ble_conn_new_device: no such device %d", index);
        return;
    }

    p_target_dev = &le_target_dev[index];

    memset(p_target_dev, 0, sizeof(T_APP_SRC_LE_TARGET_DEV));

    memcpy(p_target_dev->bd_addr, bd_addr, 6);
    p_target_dev->addr_type = addr_type;
    p_target_dev->appearance = appearance;
    p_target_dev->in_use = 1;

    //need to report to host
    p_target_dev->host_cmd_connect = 1;

    le_bg_conn_mgr_disconn();

    le_bg_conn_mgr_wl_add(bd_addr, addr_type);
    p_target_dev->in_wl = 1;

    rf_scheduler_le_dev_enable(true, index, addr_type, bd_addr);

    //bg_conn_mgr will filter duplicated connecte req
    le_bg_conn_mgr_create_conn();
}

static void app_src_ble_reconn(uint8_t index, uint8_t addr_type,
                               uint8_t *bd_addr, uint32_t appearance)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("app_src_ble_reconn: no such device %d", index);
        return;
    }

    p_target_dev = &le_target_dev[index];

    memset(p_target_dev, 0, sizeof(T_APP_SRC_LE_TARGET_DEV));

    memcpy(p_target_dev->bd_addr, bd_addr, 6);
    p_target_dev->addr_type = addr_type;
    p_target_dev->appearance = appearance;
    p_target_dev->in_use = true;

    le_bg_conn_mgr_disconn();

    le_bg_conn_mgr_wl_add(bd_addr, addr_type);
    p_target_dev->in_wl = 1;

    rf_scheduler_le_dev_enable(true, index, addr_type, bd_addr);

    //bg_conn_mgr will filter duplicated connecte req
    le_bg_conn_mgr_create_conn();
}


//all pending evt need to remove old device from WL
static void app_src_ble_handle_pending_evt(uint8_t index)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;
    T_APP_SRC_LE_PENDING_INFO *pending_info = NULL;
    uint8_t remote_bd[6];

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("app_src_ble_hanlde_pending_evt: no such device %d", index);
        return;
    }

    p_target_dev = &le_target_dev[index];
    pending_info = &p_target_dev->pending_info;

    //all api reset pending_info inside or do it here
    switch (pending_info->pending_evt)
    {
    case LE_PENDING_EVT_CLEANUP:
        {
            //remove from WL
            le_bg_conn_mgr_disconn();
            le_bg_conn_mgr_wl_del(p_target_dev->bd_addr, p_target_dev->addr_type);
            p_target_dev->in_wl = 0;

            memset(p_target_dev, 0, sizeof(T_APP_SRC_LE_TARGET_DEV));
        }
        break;

    case LE_PENDING_EVT_RECONN:
        {
            //remove from WL
            le_bg_conn_mgr_disconn();
            le_bg_conn_mgr_wl_del(p_target_dev->bd_addr, p_target_dev->addr_type);
            p_target_dev->in_wl = 0;

            memcpy(remote_bd, pending_info->remote_bd, 6);

            app_src_ble_reconn(index,
                               pending_info->addr_type,
                               remote_bd,
                               pending_info->appearance);
        }
        break;

    case LE_PENDING_EVT_NEW_TARGET:
        {
            //remove from WL
            le_bg_conn_mgr_disconn();
            le_bg_conn_mgr_wl_del(p_target_dev->bd_addr, p_target_dev->addr_type);
            p_target_dev->in_wl = 0;

            memcpy(remote_bd, pending_info->remote_bd, 6);

            app_src_ble_conn_new_device(index,
                                        pending_info->addr_type,
                                        remote_bd,
                                        pending_info->appearance);
        }
        break;

    default:
        {
            APP_PRINT_INFO1("app_src_ble_hanlde_pending_evt: unsupported event %d", pending_info->pending_evt);
        }
        break;
    }
}

static void app_src_ble_handle_connected(uint8_t *bd_addr, uint8_t bd_type, uint8_t conn_id)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = le_target_dev_find_by_addr(bd_addr, bd_type);

    if (p_target_dev != NULL)
    {
        p_target_dev->acl_exist = 1;

        p_target_dev->conn_id = conn_id;

        le_bg_conn_mgr_wl_del(p_target_dev->bd_addr, p_target_dev->addr_type);

        p_target_dev->in_wl = 0;
    }
}

static void app_src_ble_handle_disconnected(uint8_t conn_id)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;
    uint8_t index = 0;

    if (le_target_dev_get_index_by_connId(conn_id, &index))
    {
        p_target_dev = &le_target_dev[index];

        p_target_dev->acl_exist = 0;

        if (p_target_dev->pending_info.pending_evt != LE_PENDING_EVT_NON)
        {
            app_src_ble_handle_pending_evt(index);
        }
        else
        {
            le_bg_conn_mgr_disconn();
            le_bg_conn_mgr_wl_add(p_target_dev->bd_addr, p_target_dev->addr_type);

            p_target_dev->in_wl = 1;

            if (p_target_dev->hogp_connected)
            {
                p_target_dev->hogp_connected = 0;
                app_usb_hid_connection_state_notify(index + 2, BT_CONN_STATE_DISCONNECTED, 0);
            }
        }
    }
}

static void app_src_ble_handle_dev_state_change_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_TRACE5("app_src_ble_handle_dev_state_change_evt: cause 0x%04x, origin: %d, %d, new: %d, %d",
                     cause,
                     src_le_state.gap_conn_state,
                     new_state.gap_conn_state,
                     src_le_state.gap_scan_state,
                     new_state.gap_scan_state);

    if (src_le_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_TRACE0("app_ble_gap_handle_dev_state_change_evt: ble stack ready!");
            //trigger bg conn
            //app_src_ble_start_auto_conn();
            //le_conn_target_devices();
        }
    }

    if (src_le_state.gap_conn_state != new_state.gap_conn_state)
    {
        //What to do???
    }

    if (src_le_state.gap_scan_state != new_state.gap_scan_state)
    {
#if BTDONGLE_INCLUDE_CTRL
        app_ble_handle_scan_start_stop(src_le_state, new_state, cause);
#endif
    }

    src_le_state = new_state;
}

static void app_src_ble_handle_new_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                                  uint16_t disc_cause)
{
    T_APP_LE_LINK *p_link;

    uint8_t bd_addr[6];
    uint8_t bd_type;

    APP_PRINT_TRACE3("app_src_ble_handle_new_conn_state_evt: conn_id %d, new_state %d, cause 0x%04x",
                     conn_id, new_state, disc_cause);

    le_bg_conn_mgr_handle_conn_state_change(conn_id, new_state, disc_cause);

    p_link = app_find_le_link_by_conn_id(conn_id);
    if (p_link == NULL)
    {
        APP_PRINT_WARN1("app_src_ble_handle_new_conn_state_evt: no link for %d", conn_id);
    }

    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTING:
        if (p_link != NULL)
        {
            p_link->state = BT_CONN_STATE_DISCONNECTING;
        }
        break;

    case GAP_CONN_STATE_DISCONNECTED:
        {
            if (p_link != NULL)
            {
                if (app_cfg_const.enable_data_uart)
                {
                    uint8_t event_buff[3];

                    event_buff[0] = p_link->id;
                    event_buff[1] = (uint8_t)(disc_cause);
                    event_buff[2] = (uint8_t)(disc_cause >> 8);
                    //app_report_uart_event(EVENT_LE_DISCONNECTED, &event_buff[0], 3);
                }

                for (uint8_t i = 0; i < p_link->disc_cb_list.count; i++)
                {
                    T_LE_DISC_CB_ENTRY *p_entry;
                    p_entry = os_queue_peek(&p_link->disc_cb_list, i);
                    if (p_entry != NULL && p_entry->disc_callback != NULL)
                    {
                        p_entry->disc_callback(conn_id, p_link->local_disc_cause, disc_cause);
                    }
                }

                app_src_ble_handle_disconnected(conn_id);

                app_free_le_link(p_link);
            }

            if (le_target_dev_get_nolink_num() > 0)
            {
                le_bg_conn_mgr_create_conn();
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTING:
        if (p_link == NULL)
        {
            p_link = app_alloc_le_link_by_conn_id(conn_id);
            if (p_link != NULL)
            {
                p_link->state = BT_CONN_STATE_CONNECTING;
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        {
            if (p_link != NULL)
            {
                if (p_link->state == BT_CONN_STATE_CONNECTING)
                {
                    p_link->state = BT_CONN_STATE_CONNECTED;

                    le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &p_link->mtu_size, conn_id);
                    if (app_cfg_const.enable_data_uart)
                    {
                        //app_report_uart_event(EVENT_LE_CONNECTED, &p_link->id, 1);
                    }

                    le_get_conn_addr(conn_id, bd_addr, &bd_type);
                    memcpy(p_link->bd_addr, bd_addr, 6);

                    app_src_ble_handle_connected(bd_addr, bd_type, conn_id);

                    //os_delay(10);

                    le_bond_pair(conn_id);
                }
            }

            if (le_target_dev_get_nolink_num() > 0)
            {
                le_bg_conn_mgr_create_conn();
            }
        }
        break;

    default:
        break;
    }

    //app_src_ble_handle_new_conn_state_evt(conn_id, new_state);
}

static void app_src_ble_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = le_target_dev_find_by_connId(conn_id);

    APP_PRINT_INFO3("app_src_ble_handle_authen_state_evt:conn_id %d, state %d, cause 0x%x",
                    conn_id, new_state, cause);

    if (new_state == GAP_AUTHEN_STATE_COMPLETE)
    {
        T_APP_LE_LINK *p_link;
        p_link = app_find_le_link_by_conn_id(conn_id);
        if (p_link != NULL)
        {
            if (app_cfg_const.enable_data_uart)
            {
                uint8_t event_buff[3];
                event_buff[0] = p_link->id;
                event_buff[1] = cause;
                event_buff[2] = cause >> 8;
                app_report_uart_event(EVENT_LE_PAIR_STATUS, &event_buff[0], 3);
            }

            if (cause == GAP_SUCCESS)
            {
                uint8_t addr[6];
                T_GAP_REMOTE_ADDR_TYPE bd_type;
                uint8_t resolved_addr[6];
                T_GAP_IDENT_ADDR_TYPE resolved_bd_type;

                //p_link->transmit_srv_tx_enable_fg |= TX_ENABLE_AUTHEN_BIT;

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

#if BTDONGLE_INCLUDE_HOGP_HOST
                if (p_target_dev != NULL)
                {
                    //only get dev name for the first time
                    if (p_target_dev->host_cmd_connect)
                    {
                        gaps_start_discovery(conn_id);
                    }
                    else
                    {
                        hogp_client_connect(conn_id);
                    }
                }
#endif
            }
            else
            {
                //p_link->transmit_srv_tx_enable_fg &= ~TX_ENABLE_AUTHEN_BIT;
                //app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED);

                if (p_target_dev != NULL)
                {
                    if (cause == (SM_ERR | SM_ERR_INVALID_STATE))   ////for collision, wait next msg
                    {
                        APP_PRINT_INFO0("app_src_ble_handle_authen_state_evt:ignore invalid state");
                        return;
                    }
                    else if (cause != (SM_ERR | SM_ERR_DISCONNECT))
                    {
#if F_BT_LE_APP_KEY_MANAGER
                        app_src_le_delete_bond_by_addr(p_target_dev->bd_addr, p_target_dev->addr_type);
#else
                        le_bond_delete_by_bd(p_target_dev->bd_addr,
                                             (T_GAP_REMOTE_ADDR_TYPE)p_target_dev->addr_type);
#endif

                        //hogp_client_erase(p_target_dev->bd_addr);
                    }

                    le_disconnect(conn_id);
                }
            }
        }
    }

}

static void app_src_ble_handle_passkey_display(uint8_t conn_id)
{
    uint32_t pin_code = 0;
    uint8_t pin_code_array[6] = {0};
    uint8_t i;
    uint32_t temp = 100000;
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;
    uint8_t index = 0;

    if (le_target_dev_get_index_by_connId(conn_id, &index))
    {
        p_target_dev = &le_target_dev[index];

        if (!le_bond_get_display_key(conn_id, &pin_code))
        {
            APP_PRINT_INFO2("app_src_ble_handle_passkey_display: get pin code %d for conn_id %d",
                            pin_code, conn_id);

            le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);

            if (hogp_pair_state_cb != NULL)
            {
                for (i = 0; i < 6; i++)
                {
                    pin_code_array[i] = pin_code / temp;
                    pin_code = pin_code % temp;
                    temp = temp / 10;
                }

                hogp_pair_state_cb(index,
                                   p_target_dev->bd_addr,
                                   p_target_dev->addr_type,
                                   PAIR_STATE_WAIT_PIN_CODE,
                                   pin_code_array);
            }
        }
    }
}

void app_src_ble_handle_gap_msg(T_IO_MSG *io_msg)
{
    APP_PRINT_TRACE1("app_src_ble_handle_gap_msg: subtype %d", io_msg->subtype);
    T_LE_GAP_MSG stack_msg;
    T_APP_LE_LINK *p_link;

    memcpy(&stack_msg, &io_msg->u.param, sizeof(io_msg->u.param));

    switch (io_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            app_src_ble_handle_dev_state_change_evt(
                stack_msg.msg_data.gap_dev_state_change.new_state,
                stack_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_src_ble_handle_new_conn_state_evt(
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
            app_src_ble_handle_authen_state_evt(
                stack_msg.msg_data.gap_authen_state.conn_id,
                stack_msg.msg_data.gap_authen_state.new_state,
                stack_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:
        {
            app_src_ble_handle_passkey_display(stack_msg.msg_data.gap_bond_passkey_display.conn_id);
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
            //ble_handle_conn_update_info(stack_msg.msg_data.gap_conn_param_update);
        }
        break;

    default:
        break;
    }
}

#if F_BT_LE_APP_KEY_MANAGER
void app_src_ble_handle_authen_key_req_ind(T_LE_AUTHEN_KEY_REQ_IND *key_req_ind)
{
    APP_PRINT_INFO3("app_src_ble_handle_authen_key_req_ind: bd_addr %s, remote_addr_type %d, key_type %d",
                    TRACE_BDADDR(key_req_ind->bd_addr),
                    key_req_ind->remote_addr_type,
                    key_req_ind->key_type);

    switch (key_req_ind->key_type)
    {
    case GAP_KEY_LE_REMOTE_LTK:
        {
            uint8_t ltk[28] = {0};
            uint8_t ltk_len = 0;

            if (app_src_le_get_ltk_by_addr(key_req_ind->bd_addr, key_req_ind->remote_addr_type, ltk, &ltk_len))
            {
                APP_PRINT_INFO1("app_src_ble_handle_authen_key_req_ind: found %d", ltk_len);

                le_bond_authen_key_req_confirm(key_req_ind->bd_addr,
                                               key_req_ind->remote_addr_type,
                                               ltk_len,
                                               ltk,
                                               key_req_ind->key_type,
                                               GAP_CFM_CAUSE_ACCEPT);
            }
            else
            {
                APP_PRINT_INFO0("app_src_ble_handle_authen_key_req_ind: not found");

                le_bond_authen_key_req_confirm(key_req_ind->bd_addr,
                                               key_req_ind->remote_addr_type,
                                               0,
                                               NULL,
                                               key_req_ind->key_type,
                                               GAP_CFM_CAUSE_REJECT);
            }
        }
        break;

    default:
        {
            APP_PRINT_INFO0("app_src_ble_handle_authen_key_req_ind: not found");

            le_bond_authen_key_req_confirm(key_req_ind->bd_addr,
                                           key_req_ind->remote_addr_type,
                                           0,
                                           NULL,
                                           key_req_ind->key_type,
                                           GAP_CFM_CAUSE_REJECT);
        }
        break;
    }
}

void app_src_ble_handle_authen_result_ind(T_LE_AUTHEN_RESULT_IND *auth_ind)
{
    APP_PRINT_INFO5("app_src_ble_handle_authen_result_ind: bd_addr %s, remote_addr_type %d, key_len %d, key_type %d, cause 0x%x",
                    TRACE_BDADDR(auth_ind->bd_addr),
                    auth_ind->remote_addr_type,
                    auth_ind->key_len,
                    auth_ind->key_type,
                    auth_ind->cause);

    if (auth_ind->cause == GAP_SUCCESS)
    {
        switch (auth_ind->key_type)
        {
        case GAP_KEY_LE_REMOTE_LTK:
            {
                uint8_t index = 0xff;
                if (le_target_dev_get_index_by_addr(auth_ind->bd_addr, auth_ind->remote_addr_type, &index))
                {
                    app_src_le_save_ltk(index,
                                        auth_ind->bd_addr,
                                        auth_ind->remote_addr_type,
                                        auth_ind->link_key,
                                        auth_ind->key_len);
                }
            }
            break;

        case GAP_KEY_LE_REMOTE_IRK:
            {
                T_APP_SRC_LE_REMOTE_IRK *p_irk = (T_APP_SRC_LE_REMOTE_IRK *)auth_ind->link_key;
                p_irk->key_exist = 1;

                app_src_le_save_irk(auth_ind->bd_addr,
                                    auth_ind->remote_addr_type,
                                    (uint8_t *)p_irk);
            }
            break;

        default:
            break;
        }

        le_bond_authen_result_confirm(auth_ind->bd_addr, auth_ind->remote_addr_type,
                                      auth_ind->key_type, GAP_CFM_CAUSE_ACCEPT);
    }
    else
    {
        le_bond_authen_result_confirm(auth_ind->bd_addr, auth_ind->remote_addr_type,
                                      auth_ind->key_type, GAP_CFM_CAUSE_REJECT);
    }

}

void app_src_ble_handle_gatt_server_store_ind(T_LE_GATT_SERVER_STORE_IND *store_ind)
{
    APP_PRINT_INFO4("app_src_ble_handle_gatt_server_store_ind: bd_addr %s, remote_addr_type %d, op %d, data_len %d",
                    TRACE_BDADDR(store_ind->bd_addr),
                    store_ind->remote_addr_type,
                    store_ind->op,
                    store_ind->data_len);
    le_bond_gatt_server_store_confirm(store_ind->op,
                                      store_ind->bd_addr,
                                      store_ind->remote_addr_type,
                                      0,
                                      NULL,
                                      GAP_CFM_CAUSE_REJECT);
}

#endif

static void src_ble_set_pending_info(uint8_t index, T_APP_SRC_LE_PENDING_EVT event,
                                     uint8_t addr_type, uint8_t *addr, uint32_t appearance)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("src_ble_set_pending_info: no such device %d", index);
        return;
    }

    p_target_dev = &le_target_dev[index];

    switch (event)
    {
    case LE_PENDING_EVT_CLEANUP:
        {
            p_target_dev->pending_info.pending_evt = LE_PENDING_EVT_CLEANUP;
        }
        break;

    case LE_PENDING_EVT_RECONN:
        {
            p_target_dev->pending_info.pending_evt = LE_PENDING_EVT_RECONN;
            p_target_dev->pending_info.addr_type = addr_type;
            memcpy(p_target_dev->pending_info.remote_bd, addr, 6);
            p_target_dev->pending_info.appearance = appearance;
        }
        break;

    case LE_PENDING_EVT_NEW_TARGET:
        {
            p_target_dev->pending_info.pending_evt = LE_PENDING_EVT_NEW_TARGET;
            p_target_dev->pending_info.addr_type = addr_type;
            memcpy(p_target_dev->pending_info.remote_bd, addr, 6);
            p_target_dev->pending_info.appearance = appearance;
        }
        break;

    default:
        {
            APP_PRINT_INFO1("src_ble_set_pending_info: unsupported event %d", event);
        }
        break;
    }
}

/* MS APIs begin */
bool src_ble_connect_hogp_device(uint8_t index, uint8_t *bd_addr,
                                 uint8_t addr_type, uint32_t appearance)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("src_ble_connect_hogp_device: no such device %d", index);
        return false;
    }

    p_target_dev = &le_target_dev[index];

    if (p_target_dev->in_use)
    {
        APP_PRINT_WARN1("src_ble_connect_hogp_device: %d not idle", index);

        rf_scheduler_le_dev_enable(false, index,
                                   p_target_dev->addr_type, p_target_dev->bd_addr);

        //after new device conn succeed
        //hogp_client_erase(p_target_dev->bd_addr);

        if (p_target_dev->acl_exist)
        {
            //wait for disconnected
            src_ble_set_pending_info(index, LE_PENDING_EVT_NEW_TARGET, addr_type, bd_addr, appearance);
            le_disconnect(p_target_dev->conn_id);
            return true;
        }
        else if (p_target_dev->in_wl)
        {
            le_bg_conn_mgr_disconn();
            le_bg_conn_mgr_wl_del(p_target_dev->bd_addr, p_target_dev->addr_type);
            p_target_dev->in_wl = 0;
        }
    }

    app_src_ble_conn_new_device(index, addr_type, bd_addr, appearance);

    return true;
}


bool src_ble_remove_hogp_device(uint8_t index)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("src_ble_connect_hogp_device: no such device %d", index);
        return false;
    }

    p_target_dev = &le_target_dev[index];

    if (p_target_dev->in_use)
    {
        APP_PRINT_WARN1("src_ble_remove_hogp_device: %d not idle", index);

        rf_scheduler_le_dev_enable(false, index,
                                   p_target_dev->addr_type, p_target_dev->bd_addr);

        //hogp_client_erase(p_target_dev->bd_addr);

        if (p_target_dev->acl_exist)
        {
            src_ble_set_pending_info(index, LE_PENDING_EVT_CLEANUP, 0, NULL, 0);
            le_disconnect(p_target_dev->conn_id);
            return true;
        }
        else if (p_target_dev->in_wl)
        {
            le_bg_conn_mgr_disconn();
            le_bg_conn_mgr_wl_del(p_target_dev->bd_addr, p_target_dev->addr_type);
            p_target_dev->in_wl = 0;
        }
    }

    memset(p_target_dev, 0, sizeof(T_APP_SRC_LE_TARGET_DEV));

    /*
        no need to resume bg conn, handled in disconnected
    */

    return true;
}

bool src_get_hogp_device_pair_state(uint8_t index, uint8_t *pair_state)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("src_ble_connect_hogp_device: no such device %d", index);
        return false;
    }

    p_target_dev = &le_target_dev[index];

    if (p_target_dev->in_use)
    {
        if (p_target_dev->hogp_connected)
        {
            *pair_state = PAIR_STATE_SUCCESS;
        }
        else    //always in pairing state
        {
            *pair_state = PAIR_STATE_IN_PROCESS;
        }
    }
    else
    {
        *pair_state = PAIR_STATE_IDLE;
    }

    return true;
}

bool src_get_hogp_paired_device(uint8_t index, uint8_t *bd_addr, uint8_t *dev_name)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("src_ble_connect_hogp_device: no such device %d", index);
        return false;
    }

    p_target_dev = &le_target_dev[index];

    memset(bd_addr, 0, 6);
    memset(dev_name, 0, GAP_DEVICE_NAME_LEN);

    if (p_target_dev->in_use)
    {
        memcpy(bd_addr, p_target_dev->bd_addr, 6);
        memcpy(dev_name, p_target_dev->remote_name, GAP_DEVICE_NAME_LEN - 1);
    }

    return true;
}

void src_register_hogp_pair_state_nofity(APP_SRC_HOGP_CONN_RESULT_CB pair_state_cb)
{
    hogp_pair_state_cb = pair_state_cb;
}

bool src_get_hogp_device_conn_state(uint8_t index, uint8_t *conn_state)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("src_get_hogp_device_conn_state: no such device %d", index);
        return false;
    }

    p_target_dev = &le_target_dev[index];

    if (p_target_dev->hogp_connected)
    {
        *conn_state = BT_CONN_STATE_CONNECTED;
    }
    else
    {
        *conn_state = BT_CONN_STATE_DISCONNECTED;
    }

    return true;
}

bool src_ble_connect(uint8_t index)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("src_ble_connect: no such device %d", index);
        return false;
    }

    p_target_dev = &le_target_dev[index];

    if (p_target_dev->in_use)
    {
        if (!p_target_dev->hogp_connected)
        {
            //always connecting after disconnected
            return true;
        }
    }

    return false;
}

bool src_ble_disconnect(uint8_t index)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    if (index >= NUM_OF_LE_TARGET_DEV)
    {
        APP_PRINT_ERROR1("src_ble_disconnect: no such device %d", index);
        return false;
    }

    p_target_dev = &le_target_dev[index];

    if (p_target_dev->in_use)
    {
        if (p_target_dev->hogp_connected)
        {
            le_disconnect(p_target_dev->conn_id);
            return true;
        }
    }

    return false;
}

/* MS APIs end */

static T_APP_RESULT app_src_ble_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;

    switch (cb_type)
    {

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
                        p_data->p_le_modify_white_list_rsp->operation,
                        p_data->p_le_modify_white_list_rsp->cause);

        le_bg_conn_mgr_handle_wl_op_result(p_data->p_le_modify_white_list_rsp->operation,
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
#if BTDONGLE_INCLUDE_CTRL
        //app_ble_handle_scan_info(p_data);
        app_src_handle_le_adv_report(p_data->p_le_scan_info);
#endif
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
        //result = APP_RESULT_ACCEPT;
        result = APP_RESULT_REJECT;
        break;

#if F_BT_LE_APP_KEY_MANAGER
    case GAP_MSG_LE_AUTHEN_KEY_REQ_IND:
        app_src_ble_handle_authen_key_req_ind(p_data->p_le_authen_key_req_ind);
        break;

    case GAP_MSG_LE_AUTHEN_RESULT_IND:
        app_src_ble_handle_authen_result_ind(p_data->p_le_authen_result_ind);
        break;

    case GAP_MSG_LE_GATT_SERVER_STORE_IND:
        app_src_ble_handle_gatt_server_store_ind(p_data->p_le_gatt_server_store_ind);
        break;
#endif

    default:
        APP_PRINT_ERROR1("app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}

static void app_src_ble_gap_param_init(void)
{
    /* Device name and device appearance */
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

    /* Scan parameters */
    uint8_t  scan_mode = GAP_SCAN_MODE_ACTIVE;
    uint16_t scan_interval = SRC_DEFAULT_SCAN_INTERVAL;
    uint16_t scan_window = SRC_DEFAULT_SCAN_WINDOW;
    uint8_t  scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t  scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

    T_GAP_LE_CONN_REQ_PARAM conn_req_param;

    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    //uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_io_cap = GAP_IO_CAP_KEYBOARD_DISPLAY;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    le_gap_init(MAX_BLE_LINK_NUM);

    /* Set device name and device appearance */

#ifdef CUSTOMER_FIXED_SETTINGS
    T_APP_DONGLE_NAME target_name;
    if (app_storage_get_dongle_usb_name(&target_name) && (target_name.is_valid == 1))
    {
        le_set_gap_param(GAP_PARAM_DEVICE_NAME, target_name.length, target_name.name);
    }
    else
    {
        if (app_vendor_cfg_is_audio_hid())
        {
            le_set_gap_param(GAP_PARAM_DEVICE_NAME, sizeof(DEVICE_CUSTOMER_NAME_AUDIO_HID),
                             DEVICE_CUSTOMER_NAME_AUDIO_HID);
        }
        else
        {
            le_set_gap_param(GAP_PARAM_DEVICE_NAME, sizeof(DEVICE_CUSTOMER_NAME_AUDIO_ONLY_BT),
                             DEVICE_CUSTOMER_NAME_AUDIO_ONLY_BT);
        }
    }
#else
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, app_cfg_nv.device_name_le);
#endif

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
    conn_req_param.scan_interval = SRC_DEFAULT_CONN_SCAN_INTERVAL;
    conn_req_param.scan_window = SRC_DEFAULT_CONN_SCAN_WINDOW;
    conn_req_param.conn_interval_min = SRC_DEFAULT_CONN_MAX_INTERVAL;
    conn_req_param.conn_interval_max = SRC_DEFAULT_CONN_MIN_INTERVAL;
    conn_req_param.conn_latency = SRC_DEFAULT_CONN_SLAVE_LATENCY;
    conn_req_param.supv_tout = SRC_DEFAULT_CONN_SVTO;
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
    le_register_app_cb(app_src_ble_gap_callback);
}

void app_src_ble_load_target_devices(void)
{
    T_APP_SRC_TARGET_DEV temp_dev;

    memset(&le_target_dev, 0, sizeof(le_target_dev));

    if (app_src_storage_get_target_dev(TARGET_DEVICE_TYPE_LE_SLOT1, &temp_dev))
    {
        APP_PRINT_INFO0("app_src_ble_load_target_devices, found mouse");
        le_target_dev[0].in_use = 1;

        le_target_dev[0].addr_type = temp_dev.addr_type;
        le_target_dev[0].appearance = temp_dev.ms_dev_type;
        memcpy(le_target_dev[0].bd_addr, temp_dev.remote_bd, 6);
    }

    if (app_src_storage_get_target_dev(TARGET_DEVICE_TYPE_LE_SLOT2, &temp_dev))
    {
        APP_PRINT_INFO0("app_src_ble_load_target_devices, found keyboard");

        le_target_dev[1].in_use = 1;

        le_target_dev[1].addr_type = temp_dev.addr_type;
        le_target_dev[1].appearance = temp_dev.ms_dev_type;
        memcpy(le_target_dev[1].bd_addr, temp_dev.remote_bd, 6);
    }
}

static void src_ble_save_target_hogp_dev(uint8_t index)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = &le_target_dev[index];
    T_APP_SRC_TARGET_DEV save_dev;

    memset(&save_dev, 0, sizeof(T_APP_SRC_TARGET_DEV));

    save_dev.dev_type = index + 2;
    save_dev.bt_mode = DEV_MODE_LE;
    save_dev.addr_type = p_target_dev->addr_type;

    memcpy(save_dev.remote_bd, p_target_dev->bd_addr, 6);

    save_dev.ms_dev_type = p_target_dev->appearance;

    save_dev.vid = p_target_dev->vid;
    save_dev.pid = p_target_dev->pid;

    memcpy(save_dev.remote_name, p_target_dev->remote_name, GAP_DEVICE_NAME_LEN - 1);

    save_dev.is_valid = 1;

    app_src_storage_set_target_dev(save_dev);
}

static void src_ble_get_hogp_vid_pid(uint8_t index)
{
    uint16_t vid = 0;
    uint16_t pid = 0;
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = &le_target_dev[index];

    if (hogp_client_get_vid(p_target_dev->bd_addr, &vid))
    {
        p_target_dev->vid = vid;
    }
    else
    {
        p_target_dev->vid = 0;
    }

    if (hogp_client_get_pid(p_target_dev->bd_addr, &pid))
    {
        p_target_dev->pid = pid;
    }
    else
    {
        p_target_dev->pid = 0;
    }

    APP_PRINT_INFO3("src_ble_get_hogp_vid_pid: %s vid = %04x, pid = %04x",
                    TRACE_BDADDR(p_target_dev->bd_addr),
                    p_target_dev->vid, p_target_dev->pid);
}

static void src_ble_handle_pair_result(uint8_t index, bool is_success)
{
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = &le_target_dev[index];
    T_APP_SRC_TARGET_DEV old_dev;
    bool old_dev_valid = false;

    old_dev_valid = app_src_storage_get_target_dev(index + 2, &old_dev);

    if (is_success)
    {
        //new and old are different
        if (memcmp(p_target_dev->bd_addr, old_dev.remote_bd, 6))
        {
            le_bond_delete_by_bd(old_dev.remote_bd,
                                 (T_GAP_REMOTE_ADDR_TYPE)old_dev.addr_type);

            hogp_client_erase(old_dev.remote_bd);
        }

        //src_ble_save_target_hogp_dev(index);
    }
    else
    {
        hogp_client_erase(p_target_dev->bd_addr);

        if (!old_dev_valid)
        {
            //1st device, reset to idle

            rf_scheduler_le_dev_enable(false, index,
                                       p_target_dev->addr_type, p_target_dev->bd_addr);

            le_bond_delete_by_bd(p_target_dev->bd_addr,
                                 (T_GAP_REMOTE_ADDR_TYPE)p_target_dev->addr_type);

            if (p_target_dev->acl_exist)
            {
                src_ble_set_pending_info(index, LE_PENDING_EVT_CLEANUP, 0, NULL, 0);
                le_disconnect(p_target_dev->conn_id);
                return;
            }
            else
            {
                //in wl
                le_bg_conn_mgr_disconn();
                le_bg_conn_mgr_wl_del(p_target_dev->bd_addr, p_target_dev->addr_type);
                memset(p_target_dev, 0, sizeof(T_APP_SRC_LE_TARGET_DEV));
            }
        }
        else
        {
            //not 1st, clear all if new, then reconn old

            //for new device, remove all include rf schedule
            if (memcmp(p_target_dev->bd_addr, old_dev.remote_bd, 6))
            {
                rf_scheduler_le_dev_enable(false, index,
                                           p_target_dev->addr_type, p_target_dev->bd_addr);

                le_bond_delete_by_bd(p_target_dev->bd_addr,
                                     (T_GAP_REMOTE_ADDR_TYPE)p_target_dev->addr_type);
            }

            if (p_target_dev->acl_exist)
            {
                src_ble_set_pending_info(index, LE_PENDING_EVT_RECONN, old_dev.addr_type,
                                         old_dev.remote_bd, old_dev.ms_dev_type);

                le_disconnect(p_target_dev->conn_id);
                return;
            }
            else
            {
                //in wl
                le_bg_conn_mgr_disconn();
                le_bg_conn_mgr_wl_del(p_target_dev->bd_addr, p_target_dev->addr_type);

                app_src_ble_reconn(index, old_dev.addr_type, old_dev.remote_bd, old_dev.ms_dev_type);
            }
        }
    }
}

#if BTDONGLE_INCLUDE_HOGP_HOST
static T_APP_RESULT dongle_app_general_client_cb(T_CLIENT_ID client_id, uint8_t conn_id,
                                                 void *p_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    APP_PRINT_TRACE0("dongle_app_general_client_cb");
    return result;
}

void app_hogp_cb(uint8_t conn_id, T_HOGP_RESULT result)
{
    uint8_t index = 0;
    bool is_success = false;
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    APP_PRINT_TRACE2("app_hogp_cb get callback from conn_id %d, result %d", conn_id, result);

    if (!le_target_dev_get_index_by_connId(conn_id, &index))
    {
        APP_PRINT_WARN1("app_hogp_cb: no such conn_id %d", conn_id);
        return;
    }

    p_target_dev = &le_target_dev[index];

    if (result == HOGP_RESULT_SUCESS)
    {
        p_target_dev->hogp_connected = 1;

        src_ble_get_hogp_vid_pid(index);

        is_success = true;
        app_usb_hid_connection_state_notify(index + 2, BT_CONN_STATE_CONNECTED, 0);

    }

    if (p_target_dev->host_cmd_connect)
    {
        p_target_dev->host_cmd_connect = 0;

        if (hogp_pair_state_cb != NULL)
        {
            if (is_success)
            {
                src_ble_save_target_hogp_dev(index);

                hogp_pair_state_cb(index,
                                   p_target_dev->bd_addr,
                                   p_target_dev->addr_type,
                                   PAIR_STATE_SUCCESS,
                                   NULL);
            }
            else
            {

                hogp_pair_state_cb(index,
                                   p_target_dev->bd_addr,
                                   p_target_dev->addr_type,
                                   PAIR_STATE_FAIL,
                                   NULL);
            }
        }
    }

    src_ble_handle_pair_result(index, is_success);
}

//gaps
static void app_handle_gaps_disc_state(uint8_t conn_id, T_GAPS_DISC_STATE state)
{
    bool res = true;
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    p_target_dev = le_target_dev_find_by_connId(conn_id);

    if (p_target_dev == NULL)
    {
        APP_PRINT_INFO1("app_handle_gaps_disc_state: unknown conn_id %d", conn_id);
        return;
    }

    if (state == DISC_GAPS_DONE)
    {
        res = gaps_read(conn_id, GAPS_READ_DEVICE_NAME);
    }
    else if (state == DISC_GAPS_FAILED)
    {
        res = false;
    }

    if (!res)
    {
        hogp_client_connect(conn_id);
    }
}

static void app_handle_gaps_handle_read_result(uint8_t conn_id, T_GAPS_READ_RESULT result)
{
    uint16_t val_len = 0;
    T_APP_SRC_LE_TARGET_DEV *p_target_dev = NULL;

    p_target_dev = le_target_dev_find_by_connId(conn_id);

    if (p_target_dev == NULL)
    {
        APP_PRINT_INFO1("app_handle_gaps_handl_read_result: unknown conn_id %d", conn_id);
        return;
    }

    if (result.type == GAPS_READ_DEVICE_NAME)
    {
        if (result.cause == GAP_SUCCESS)
        {
            val_len = result.data.device_name.value_size;

            if (val_len >= GAP_DEVICE_NAME_LEN - 1)
            {
                val_len = GAP_DEVICE_NAME_LEN - 1;
            }

            memcpy(p_target_dev->remote_name, result.data.device_name.p_value, val_len);
            p_target_dev->remote_name[val_len] = 0;
        }

        hogp_client_connect(conn_id);
    }
}

static T_APP_RESULT app_gaps_general_client_cb(T_CLIENT_ID client_id, uint8_t conn_id,
                                               void *p_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_GAPS_CLIENT_CB_DATA *p_gaps_cb_data;

    if (client_id == gaps_client_id)
    {
        p_gaps_cb_data = (T_GAPS_CLIENT_CB_DATA *)p_data;
        APP_PRINT_TRACE3("gaps_app_general_client_cb: client_id %d, conn_id %d, type %d",
                         client_id, conn_id, p_gaps_cb_data->cb_type);

        switch (p_gaps_cb_data->cb_type)
        {
        case GAPS_CLIENT_CB_TYPE_DISC_STATE:
            {
                app_handle_gaps_disc_state(conn_id, p_gaps_cb_data->cb_content.disc_state);
            }
            break;

        case GAPS_CLIENT_CB_TYPE_READ_RESULT:
            {
                app_handle_gaps_handle_read_result(conn_id, p_gaps_cb_data->cb_content.read_result);
            }
            break;

        default:
            break;
        }
    }

    return result;
}
#endif


void app_src_ble_init(void)
{
#if BTDONGLE_INCLUDE_HOGP_HOST

    //gaps + dis/bas/hids
    client_init(1 + DONGLE_GATT_CLIENT_NUM);

//    ancs_add_client(ancs_client_cb, MAX_BLE_LINK_NUM);
    /* dis client */
    hogp_client_init(app_hogp_cb);

    /* register general client callback */
    client_register_general_client_cb(dongle_app_general_client_cb);

//gaps
    gaps_client_id = gaps_add_client(app_gaps_general_client_cb, NUM_OF_LE_TARGET_DEV);
#endif

    app_src_ble_gap_param_init();
    ble_conn_update_init(MAX_BLE_LINK_NUM);

    if (app_vendor_cfg_is_audio_hid())
    {
        app_src_ble_load_target_devices();
    }

#if F_BT_LE_APP_KEY_MANAGER
    app_src_le_key_init();
#endif
}

