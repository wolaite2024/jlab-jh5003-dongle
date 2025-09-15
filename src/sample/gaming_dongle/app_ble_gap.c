/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "sysm.h"
#include "gap_conn_le.h"
#include "gap_bond_le.h"
#include "gap_ext_scan.h"
#include "dfu_api.h"
#include "app_ble_gap.h"
#include "app_ble_service.h"
#include "app_main.h"
#include "ble_stream.h"
#include "ble_scan.h"
#include "ble_mgr.h"
#include "connection_mgr.h"
#include "adv_mgr.h"
#include "app_link_util.h"
#include "app_cfg.h"
#include "app_ctrl_pkt_policy.h"
#include "app_adapter_service.h"

uint8_t scan_rsp_data_len;
uint8_t scan_rsp_data[GAP_MAX_LEGACY_ADV_LEN];

static void app_ble_gap_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    APP_PRINT_INFO3("app_ble_gap_handle_authen_state_evt:conn_id %d, state %d, cause 0x%x",
                    conn_id, new_state, cause);

    if (new_state == GAP_AUTHEN_STATE_COMPLETE)
    {
        T_APP_LE_LINK *p_link;
        p_link = app_link_find_le_link_by_conn_id(conn_id);
        if (p_link != NULL)
        {
            if (cause == GAP_SUCCESS)
            {
                p_link->transmit_srv_tx_enable_fg |= TX_ENABLE_AUTHEN_BIT;
            }
        }
    }
}

static void app_ble_gap_handle_new_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                                  uint16_t disc_cause)
{
    T_APP_LE_LINK *p_link;

    APP_PRINT_TRACE3("app_ble_gap_handle_new_conn_state_evt: conn_id %d, new_state %d, cause 0x%04x",
                     conn_id, new_state, disc_cause);

    p_link = app_link_find_le_link_by_conn_id(conn_id);

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
            if (p_link->bud_side == DEVICE_BUD_SIDE_LEFT)
            {
                dongle_status.bud_le_conn_status &= ~LEFT_BUD_LE_CONNECTED;
            }
            else if (p_link->bud_side == DEVICE_BUD_SIDE_RIGHT)
            {
                dongle_status.bud_le_conn_status &= ~RIGHT_BUD_LE_CONNECTED;
            }

            app_gaming_sync_dongle_status();

            // uint8_t local_disc_cause = p_link->local_disc_cause;
            for (uint8_t i = 0; i < p_link->disc_cb_list.count; i++)
            {
                T_LE_DISC_CB_ENTRY *p_entry;
                p_entry = os_queue_peek(&p_link->disc_cb_list, i);
                if (p_entry != NULL && p_entry->disc_callback != NULL)
                {
                    p_entry->disc_callback(conn_id, p_link->local_disc_cause, disc_cause);
                }
            }

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
            adapter_dual_mode_link_mgr(ADAPTER_EVENT_BLE_DISCONNECTED);
#endif

            app_link_free_le_link(p_link);
        }
        break;

    case GAP_CONN_STATE_CONNECTING:
        if (p_link == NULL)
        {
            p_link = app_link_alloc_le_link_by_conn_id(conn_id);
            if (p_link != NULL)
            {
                p_link->state = LE_LINK_STATE_CONNECTING;
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        if (p_link != NULL)
        {
            p_link->conn_handle = le_get_conn_handle(conn_id);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
            adapter_dual_mode_link_mgr(ADAPTER_EVENT_BLE_CONNECTED);
#endif

            if (p_link->state == LE_LINK_STATE_CONNECTING)
            {
                //app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_LINK_EXIST);
                p_link->state = LE_LINK_STATE_CONNECTED;
                le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &p_link->mtu_size, conn_id);
            }

#if TARGET_LE_AUDIO_GAMING_DONGLE
            uint8_t addr[6];
            uint8_t bd_type;
            T_DEVICE_BUD_SIDE bud_side;

            if (le_get_conn_addr(conn_id, addr, &bd_type) &&
                adapter_get_bud_side_by_addr(addr, &bud_side))
            {
                p_link->bud_side = bud_side;

                if (bud_side == DEVICE_BUD_SIDE_LEFT)
                {
                    dongle_status.bud_le_conn_status |= LEFT_BUD_LE_CONNECTED;
                }
                else if (bud_side == DEVICE_BUD_SIDE_RIGHT)
                {
                    dongle_status.bud_le_conn_status |= RIGHT_BUD_LE_CONNECTED;
                }
            }

            app_gaming_sync_dongle_status();
#endif
        }
        break;

    default:
        break;
    }
}

void app_ble_gap_handle_gap_msg(T_IO_MSG *p_io_msg)
{
    T_LE_GAP_MSG stack_msg;
    memcpy(&stack_msg, &p_io_msg->u.param, sizeof(p_io_msg->u.param));
    APP_PRINT_TRACE1("app_ble_gap_handle_gap_msg: subtype %d", p_io_msg->subtype);

    switch (p_io_msg->subtype)
    {
    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_ble_gap_handle_new_conn_state_evt(stack_msg.msg_data.gap_conn_state_change.conn_id,
                                                  (T_GAP_CONN_STATE)stack_msg.msg_data.gap_conn_state_change.new_state,
                                                  stack_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            app_ble_gap_handle_authen_state_evt(stack_msg.msg_data.gap_authen_state.conn_id,
                                                stack_msg.msg_data.gap_authen_state.new_state,
                                                stack_msg.msg_data.gap_authen_state.status);
        }
        break;

    default:
        break;
    }
}
bool app_ble_gap_gen_scan_rsp_data(uint8_t *p_scan_len, uint8_t *p_scan_data)
{
    uint8_t device_name_len;

    if (p_scan_len == NULL || p_scan_data == NULL)
    {
        return false;
    }

    device_name_len = strlen((const char *)app_cfg_nv.device_name_le);

    if (device_name_len > GAP_MAX_LEGACY_ADV_LEN - 2)
    {
        device_name_len = GAP_MAX_LEGACY_ADV_LEN - 2;
    }

    p_scan_data[0] = device_name_len + 1;
    p_scan_data[1] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&p_scan_data[2], app_cfg_nv.device_name_le, device_name_len);
    *p_scan_len = device_name_len + 2;
    return true;
}

bool app_ble_gap_disconnect(T_APP_LE_LINK *p_link, T_LE_LOCAL_DISC_CAUSE disc_cause)
{
    if (p_link != NULL)
    {
        APP_PRINT_TRACE2("app_ble_gap_disconnect: conn_id %d, disc_cause %d",
                         p_link->conn_id, disc_cause);
        if (le_disconnect(p_link->conn_id) == GAP_CAUSE_SUCCESS)
        {
            p_link->local_disc_cause = disc_cause;
            return true;
        }
    }
    return false;
}

void app_ble_gap_handle_mtu_info(uint8_t conn_id, uint16_t mtu)
{
    T_APP_LE_LINK *p_link;
    p_link = app_link_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        p_link->mtu_size = mtu;
    }
}

static void app_ble_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            APP_PRINT_INFO0("BT_EVENT_READY2");
            memcpy(app_db.factory_addr, param->ready.bd_addr, 6);
        }
        break;

    default:
        break;
    }
}

void app_ble_gap_ble_mgr_init(void)
{
    ble_conn_mgr_init();
    ble_adv_mgr_init();
}

void app_ble_gap_init(void)
{
    bt_mgr_cback_register(app_ble_gap_bt_cback);
    app_ble_gap_ble_mgr_init();
}
