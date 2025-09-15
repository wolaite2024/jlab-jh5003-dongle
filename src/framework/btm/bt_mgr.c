/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "os_mem.h"
#include "trace.h"
#include "gap_br.h"
#include "sys_ipc.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_hfp_ag_int.h"
#include "bt_roleswap.h"
#include "bt_gap.h"
#include "bt_avrcp_int.h"
#include "bt_avrcp.h"
#include "bt_a2dp_int.h"
#include "bt_a2dp.h"
#include "bt_map.h"
#include "bt_hfp.h"
#include "bt_hfp_int.h"
#include "bt_hfp_ag_int.h"
#include "bt_iap.h"
#include "bt_spp_int.h"
#include "bt_iap_int.h"
#include "bt_pbap_int.h"
#include "bt_hid_int.h"
#include "bt_map_int.h"
#include "bt_opp.h"

#define BT_VND_NEGOTIATE_TRAFFIC_QOS_CMD     0x1C
#define BT_VND_NON_NEGOTIATE_TRAFFIC_QOS_CMD 0x1D
#define BT_VND_NEGOTIATE_TRAFFIC_QOS_EVT     0x59
#define BT_VND_NON_NEGOTIATE_TRAFFIC_QOS_EVT 0x61

static const uint8_t sco_h2_header[4] = {0x08, 0x38, 0xc8, 0xf8};
typedef enum
{
    BT_DEVICE_MODE_EVENT_REQ   = 0x01,
    BT_DEVICE_MODE_EVENT_RSP   = 0x02,
} T_BT_DEVICE_MODE_EVENT;

typedef struct t_bt_mgr_cback_item
{
    struct t_bt_mgr_cback_item *p_next;
    P_BT_MGR_CBACK              cback;
} T_BT_MGR_CBACK_ITEM;

T_BTM_DB btm_db;

bool bt_mgr_cback_register(P_BT_MGR_CBACK cback)
{
    T_BT_MGR_CBACK_ITEM *p_item;

    p_item = (T_BT_MGR_CBACK_ITEM *)btm_db.cback_list.p_first;
    while (p_item != NULL)
    {
        /* Sanity check if callback already registered */
        if (p_item->cback == cback)
        {
            return true;
        }

        p_item = p_item->p_next;
    }

    p_item = os_mem_alloc2(sizeof(T_BT_MGR_CBACK_ITEM));
    if (p_item != NULL)
    {
        p_item->cback = cback;
        os_queue_in(&btm_db.cback_list, p_item);
        return true;
    }

    return false;
}

bool bt_mgr_cback_unregister(P_BT_MGR_CBACK cback)
{
    T_BT_MGR_CBACK_ITEM *p_item;
    bool                 ret = false;

    p_item = (T_BT_MGR_CBACK_ITEM *)btm_db.cback_list.p_first;
    while (p_item != NULL)
    {
        if (p_item->cback == cback)
        {
            os_queue_delete(&btm_db.cback_list, p_item);
            os_mem_free(p_item);
            ret = true;
            break;
        }

        p_item = p_item->p_next;
    }

    return ret;
}

bool bt_mgr_event_post(T_BT_EVENT  event_type,
                       void       *event_buf,
                       uint16_t    buf_len)
{
    T_BT_MGR_CBACK_ITEM *p_item;

    BTM_PRINT_INFO1("bt_mgr_event_post: event_type 0x%04x", event_type);

    p_item = (T_BT_MGR_CBACK_ITEM *)btm_db.cback_list.p_first;
    while (p_item != NULL)
    {
        p_item->cback(event_type, event_buf, buf_len);
        p_item = p_item->p_next;
    }

    return true;
}

T_BT_LINK *bt_link_alloc(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = calloc(1, sizeof(T_BT_LINK));
    if (link != NULL)
    {
        uint8_t index;

        for (index = 0; index < btm_db.link_list.count; index++)
        {
            T_BT_LINK *prev_link;
            bool       index_used = false;

            prev_link = os_queue_peek(&btm_db.link_list, 0);
            while (prev_link != NULL)
            {
                if (prev_link->index == index)
                {
                    index_used = true;
                    break;
                }

                prev_link = prev_link->next;
            }

            if (index_used == false)
            {
                break;
            }
        }

        link->index = index;

        link->acl_link_state = BT_LINK_STATE_DISCONNECTED;
        link->acl_link_policy = GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE;
        os_queue_init(&link->pm_cback_list);
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_in(&btm_db.link_list, link);
    }

    return link;
}

void bt_link_free(T_BT_LINK *link)
{
    T_BT_PM_CBACK_ITEM *item;

    if (link->timer_sniff != NULL)
    {
        sys_timer_delete(link->timer_sniff);
    }

    if (link->sco_buf != NULL)
    {
        free(link->sco_buf);
    }

    item = os_queue_out(&link->pm_cback_list);
    while (item != NULL)
    {
        free(item);
        item = os_queue_out(&link->pm_cback_list);
    }

    os_queue_delete(&btm_db.link_list, link);
    free(link);
}

T_BT_LINK *bt_link_find(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = os_queue_peek(&btm_db.link_list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

T_BT_LINK *bt_link_find_by_handle(uint16_t handle)
{
    T_BT_LINK *link;

    link = os_queue_peek(&btm_db.link_list, 0);
    while (link != NULL)
    {
        if (link->acl_handle == handle)
        {
            break;
        }

        link = link->next;
    }

    return link;
}

bool bt_local_name_set(uint8_t *p_name,
                       uint8_t  len)
{
    if (gap_br_set_dev_name(p_name, len) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_remote_name_req(uint8_t bd_addr[6])
{
    if (gap_br_get_remote_name(bd_addr) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_inquiry_start(bool    limited_inquiry,
                      uint8_t inquiry_length)
{
    if (gap_br_start_inquiry(limited_inquiry, inquiry_length) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_inquiry_stop(void)
{
    if (gap_br_stop_inquiry() == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_periodic_inquiry_start(bool     limited_inquiry,
                               uint16_t max_period_length,
                               uint16_t min_period_length,
                               uint8_t  inquiry_length)
{
    if (gap_br_start_periodic_inquiry(limited_inquiry, max_period_length, min_period_length,
                                      inquiry_length) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_periodic_inquiry_stop(void)
{
    if (gap_br_stop_periodic_inquiry() == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_link_policy_set(uint8_t  bd_addr[6],
                        uint16_t link_policy)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (gap_br_cfg_acl_link_policy(link->bd_addr, link_policy) == GAP_CAUSE_SUCCESS)
        {
            link->acl_link_policy = link_policy;
            return true;
        }
    }

    return false;
}

bool bt_role_switch_enable(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        return bt_link_policy_set(bd_addr, link->acl_link_policy | GAP_LINK_POLICY_ROLE_SWITCH);
    }

    return false;
}

bool bt_role_switch_disable(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        return bt_link_policy_set(bd_addr, link->acl_link_policy & (~GAP_LINK_POLICY_ROLE_SWITCH));
    }

    return false;
}

bool bt_link_policy_get(uint8_t   bd_addr[6],
                        uint16_t *link_policy)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        *link_policy = link->acl_link_policy;
        return true;
    }

    return false;
}

bool bt_link_role_switch(uint8_t bd_addr[6],
                         bool    set_master)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link == NULL)
    {
        BTM_PRINT_ERROR1("bt_link_role_switch: invalid bd_addr %s", TRACE_BDADDR(bd_addr));
        return false;
    }

    if (link->acl_link_state != BT_LINK_STATE_CONNECTED)
    {
        BTM_PRINT_ERROR1("bt_link_role_switch: bd_addr %s link not created", TRACE_BDADDR(bd_addr));
        return false;
    }

    if (link->acl_link_role_master == set_master)
    {
        BTM_PRINT_INFO2("bt_link_role_switch: bd_addr %s, set_master %u already",
                        TRACE_BDADDR(bd_addr), set_master);
        return true;
    }

    if (link->role_switch_status == BT_ROLE_SWITCH_MASTER_RUNNING ||
        link->role_switch_status == BT_ROLE_SWITCH_SLAVE_RUNNING)
    {
        BTM_PRINT_INFO2("bt_link_role_switch: bd_addr %s, set_master %u, switch running",
                        TRACE_BDADDR(bd_addr), set_master);
        return true;
    }

    if (link->role_switch_status == BT_ROLE_SWITCH_IDLE)
    {
        if (set_master == true)
        {
            link->role_switch_status = BT_ROLE_SWITCH_MASTER_PENDING;
        }
        else
        {
            link->role_switch_status = BT_ROLE_SWITCH_SLAVE_PENDING;
        }
    }

    if (link->sco_state != BT_LINK_SCO_STATE_DISCONNECTED)
    {
        BTM_PRINT_INFO2("bt_link_role_switch: bd_addr %s, set_master %u, SCO link existed",
                        TRACE_BDADDR(bd_addr), set_master);
        return true;
    }

    if (link->acl_link_sc_ongoing == true)
    {
        BTM_PRINT_INFO2("bt_link_role_switch: bd_addr %s, set_master %u, secure connection ongoing",
                        TRACE_BDADDR(bd_addr), set_master);
        return true;
    }

    if (bt_sniff_mode_exit(link->bd_addr, false) == false)
    {
        BTM_PRINT_INFO3("bt_link_role_switch: bd_addr %s, set_master %u, pm_state %u",
                        TRACE_BDADDR(bd_addr), set_master, link->pm_state);
        return true;
    }

    if (set_master == true)
    {
        link->role_switch_status = BT_ROLE_SWITCH_MASTER_RUNNING;
        gap_br_cfg_acl_link_role(link->bd_addr, GAP_BR_LINK_ROLE_MASTER);
    }
    else
    {
        link->role_switch_status = BT_ROLE_SWITCH_SLAVE_RUNNING;
        gap_br_cfg_acl_link_role(link->bd_addr, GAP_BR_LINK_ROLE_SLAVE);
    }

    return true;
}

void bt_device_mode_sm(T_BT_DEVICE_MODE_EVENT  event,
                       void                   *buf)
{
    T_BT_EVENT_PARAM param;
    T_BT_DEVICE_MODE dev_mode;

    BTM_PRINT_TRACE1("bt_device_mode_sm: event 0x%02x", event);

    if (buf == NULL)
    {
        return;
    }

    switch (event)
    {
    case BT_DEVICE_MODE_EVENT_REQ:
        {
            T_BT_DEVICE_MODE_ACTION dev_mode_action;

            dev_mode = *(T_BT_DEVICE_MODE *)buf;
            if (dev_mode == BT_DEVICE_MODE_IDLE)
            {
                dev_mode_action = BT_DEVICE_MODE_ACTION_IDLE;
            }
            else if (dev_mode == BT_DEVICE_MODE_DISCOVERABLE)
            {
                dev_mode_action = BT_DEVICE_MODE_ACTION_DISCOVERABLE;
            }
            else if (dev_mode == BT_DEVICE_MODE_CONNECTABLE)
            {
                dev_mode_action = BT_DEVICE_MODE_ACTION_CONNECTABLE;
            }
            else if (dev_mode == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
            {
                dev_mode_action = BT_DEVICE_MODE_ACTION_DISCOVERABLE_CONNECTABLE;
            }

            if (btm_db.pending_dev_mode_action != BT_DEVICE_MODE_ACTION_NONE)
            {
                if (btm_db.pending_dev_mode_action == dev_mode_action)
                {
                    btm_db.next_dev_mode_action = BT_DEVICE_MODE_ACTION_NONE;
                }
                else
                {
                    btm_db.next_dev_mode_action = dev_mode_action;
                }
            }
            else
            {
                /* Sanity check device mode */
                if (dev_mode == btm_db.curr_dev_mode)
                {
                    param.device_mode_rsp.cause = 0;
                    param.device_mode_rsp.dev_mode = btm_db.curr_dev_mode;
                    bt_mgr_event_post(BT_EVENT_DEVICE_MODE_RSP, &param, sizeof(param));
                }
                else
                {
                    btm_db.pending_dev_mode_action = dev_mode_action;

                    if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_IDLE)
                    {
                        gap_br_set_radio_mode(GAP_RADIO_MODE_NONE_DISCOVERABLE, false, 0);
                    }
                    else if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_DISCOVERABLE)
                    {
                        gap_br_set_radio_mode(GAP_RADIO_MODE_VISIBLE, false, 0);
                    }
                    else if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_CONNECTABLE)
                    {
                        gap_br_set_radio_mode(GAP_RADIO_MODE_CONNECTABLE, false, 0);
                    }
                    else if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_DISCOVERABLE_CONNECTABLE)
                    {
                        gap_br_set_radio_mode(GAP_RADIO_MODE_VISIBLE_CONNECTABLE, false, 0);
                    }
                }
            }
        }
        break;

    case BT_DEVICE_MODE_EVENT_RSP:
        {
            uint16_t cause = *(uint16_t *)(buf);

            if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_IDLE)
            {
                dev_mode = BT_DEVICE_MODE_IDLE;
            }
            else if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_DISCOVERABLE)
            {
                dev_mode = BT_DEVICE_MODE_DISCOVERABLE;
            }
            else if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_CONNECTABLE)
            {
                dev_mode = BT_DEVICE_MODE_CONNECTABLE;
            }
            else if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_DISCOVERABLE_CONNECTABLE)
            {
                dev_mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
            }
            else
            {
                return;
            }


            if (cause == 0)
            {
                btm_db.curr_dev_mode = dev_mode;
            }

            btm_db.pending_dev_mode_action = btm_db.next_dev_mode_action;
            btm_db.next_dev_mode_action = BT_DEVICE_MODE_ACTION_NONE;

            param.device_mode_rsp.cause = cause;
            param.device_mode_rsp.dev_mode = dev_mode;
            bt_mgr_event_post(BT_EVENT_DEVICE_MODE_RSP, &param, sizeof(param));

            if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_IDLE)
            {
                gap_br_set_radio_mode(GAP_RADIO_MODE_NONE_DISCOVERABLE, false, 0);
            }
            else if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_DISCOVERABLE)
            {
                gap_br_set_radio_mode(GAP_RADIO_MODE_VISIBLE, false, 0);
            }
            else if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_CONNECTABLE)
            {
                gap_br_set_radio_mode(GAP_RADIO_MODE_CONNECTABLE, false, 0);
            }
            else if (btm_db.pending_dev_mode_action == BT_DEVICE_MODE_ACTION_DISCOVERABLE_CONNECTABLE)
            {
                gap_br_set_radio_mode(GAP_RADIO_MODE_VISIBLE_CONNECTABLE, false, 0);
            }
        }
        break;
    }
}

bool bt_device_mode_set(T_BT_DEVICE_MODE device_mode)
{
    bt_device_mode_sm(BT_DEVICE_MODE_EVENT_REQ, &device_mode);

    return true;
}

T_BT_DEVICE_MODE bt_device_mode_get(void)
{
    return btm_db.curr_dev_mode;
}

T_BT_CLK_REF bt_piconet_clk_get(T_BT_CLK_REF  clk_ref,
                                uint32_t     *clk_slot,
                                uint16_t     *clk_us)
{
    return bt_roleswap_get_piconet_clk(clk_ref, clk_slot, clk_us);
}

bool bt_piconet_id_get(T_BT_CLK_REF  clk_ref,
                       uint8_t      *pid,
                       uint8_t      *role)
{
    return bt_roleswap_get_piconet_id(clk_ref, pid, role);
}

bool bt_acl_conn_accept(uint8_t        bd_addr[6],
                        T_BT_LINK_ROLE role)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (link->acl_link_state == BT_LINK_STATE_CONNECTING)
        {
            if (gap_br_accept_acl_conn(bd_addr, (T_GAP_BR_LINK_ROLE)role) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_acl_conn_reject(uint8_t                bd_addr[6],
                        T_BT_ACL_REJECT_REASON reason)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (link->acl_link_state == BT_LINK_STATE_CONNECTING)
        {
            if (gap_br_reject_acl_conn(bd_addr, (T_GAP_ACL_REJECT_CONN_REASON)reason) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_sco_conn_cfm(uint8_t  bd_addr[6],
                     uint32_t tx_bandwidth,
                     uint32_t rx_bandwidth,
                     uint16_t max_latency,
                     uint16_t voice_setting,
                     uint8_t  retrans_effort,
                     uint16_t packet_type,
                     bool     accept)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (accept == true)
        {
            if (gap_br_send_sco_conn_cfm(bd_addr,
                                         tx_bandwidth,
                                         rx_bandwidth,
                                         max_latency,
                                         voice_setting,
                                         retrans_effort,
                                         packet_type,
                                         GAP_CFM_CAUSE_ACCEPT) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
        else
        {
            if (gap_br_send_sco_conn_cfm(bd_addr,
                                         tx_bandwidth,
                                         rx_bandwidth,
                                         max_latency,
                                         voice_setting,
                                         retrans_effort,
                                         packet_type,
                                         GAP_CFM_CAUSE_REJECT) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_sco_data_send(uint8_t  bd_addr[6],
                      uint8_t  seq_num,
                      uint8_t *buf,
                      uint8_t  len)
{
    T_BT_LINK *link;
    uint8_t   *sco_buf;
    uint8_t    sco_buf_len;
    int32_t    ret = 0;

    link = bt_link_find(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (link->sco_state != BT_LINK_SCO_STATE_CONNECTED)
    {
        ret = 2;
        goto fail_invalid_handle;
    }

    if (link->sco_air_mode == GAP_AIR_MODE_TRANSPARENT)
    {
        sco_buf = gap_br_get_sco_buffer(len + 3);
        if (sco_buf == NULL)
        {
            ret = 3;
            goto fail_alloc_buf;
        }

        memcpy(&sco_buf[2], buf, len);
        sco_buf[0] = 0x01;
        sco_buf[1] = sco_h2_header[seq_num & 0x03];
        sco_buf[59] = 0; /*with one byte zero padding*/
        sco_buf_len = len + 3;
    }
    else
    {
        sco_buf = gap_br_get_sco_buffer(len);
        if (sco_buf == NULL)
        {
            ret = 4;
            goto fail_alloc_buf;
        }

        memcpy(sco_buf, buf, len);
        sco_buf_len = len;
    }

    if (gap_br_send_sco_data(bd_addr, sco_buf, sco_buf_len) != GAP_CAUSE_SUCCESS)
    {
        ret = 5;
        goto fail_send_data;
    }

    return true;

fail_send_data:
fail_alloc_buf:
fail_invalid_handle:
fail_invalid_addr:
    BTM_PRINT_ERROR3("bt_sco_data_send: failed %d, bd_addr %s, seq_num %d",
                     -ret, TRACE_BDADDR(bd_addr), seq_num);
    return false;
}

bool bt_link_key_cfm(uint8_t             bd_addr[6],
                     bool                found,
                     T_BT_LINK_KEY_TYPE  type,
                     uint8_t            *p_key)
{
    if (gap_br_link_key_cfm(bd_addr, found, (T_GAP_BR_LINK_KEY_TYPE)type,
                            p_key) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_link_pin_code_cfm(uint8_t  bd_addr[6],
                          uint8_t *pin_code,
                          uint8_t  len,
                          bool     accept)
{
    if (len > 0x10)
    {
        len = 0x10;
    }

    if (gap_br_pin_code_req_cfm(bd_addr, pin_code, len, accept) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_link_encryption_set(uint8_t bd_addr[6],
                            bool    enable)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (gap_br_set_conn_encryption(link->acl_handle, enable) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_sco_link_switch(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
        {
            BTM_PRINT_TRACE2("bt_sco_link_switch: active_addr %s, active_index %d",
                             TRACE_BDADDR(bd_addr), link->index);

            gap_br_vendor_set_active_sco(link->sco_handle, 1, 1);

            T_BT_EVENT_PARAM param;
            memcpy(param.sco_link_switch.bd_addr, bd_addr, 6);
            param.sco_link_switch.handle = link->sco_handle;
            bt_mgr_event_post(BT_EVENT_SCO_LINK_SWITCH, &param, sizeof(param));
            return true;
        }
    }

    return false;
}

bool bt_sco_link_retrans_window_set(uint8_t bd_addr[6],
                                    uint8_t retrans_window)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
        {
            if (gap_br_set_sco_retrans_window(link->sco_handle, retrans_window) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
    }

    return false;
}

void bt_handle_sco_disconnect(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        link->sco_handle = 0;
        link->sco_air_mode = 0;
        link->is_esco = 0;
        link->sco_packet_length = 0;
        link->curr_sco_len = 0;
        link->sco_state = BT_LINK_SCO_STATE_DISCONNECTED;

        if (link->sco_buf != NULL)
        {
            free(link->sco_buf);
            link->sco_buf = NULL;
        }

        if (link->role_switch_status == BT_ROLE_SWITCH_MASTER_PENDING)
        {
            bt_link_role_switch(bd_addr, true);
        }
        else if (link->role_switch_status == BT_ROLE_SWITCH_SLAVE_PENDING)
        {
            bt_link_role_switch(bd_addr, false);
        }
    }
}

bool bt_acl_pkt_type_set(uint8_t           bd_addr[6],
                         T_BT_ACL_PKT_TYPE link_pkt_type)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        uint16_t pkt_type = 0;

        if (link_pkt_type == BT_ACL_PKT_TYPE_1M)
        {
            pkt_type = GAP_PKT_TYPE_DM1 | GAP_PKT_TYPE_DH1 | \
                       GAP_PKT_TYPE_DM3 | GAP_PKT_TYPE_DH3 | \
                       GAP_PKT_TYPE_DM5 | GAP_PKT_TYPE_DH5 | \
                       GAP_PKT_TYPE_NO_2DH1 | GAP_PKT_TYPE_NO_2DH3 | GAP_PKT_TYPE_NO_2DH5 | \
                       GAP_PKT_TYPE_NO_3DH1 | GAP_PKT_TYPE_NO_3DH3 | GAP_PKT_TYPE_NO_3DH5;
        }
        else if (link_pkt_type == BT_ACL_PKT_TYPE_2M)
        {
            pkt_type = GAP_PKT_TYPE_DM1 | GAP_PKT_TYPE_DH1 | \
                       GAP_PKT_TYPE_DM3 | GAP_PKT_TYPE_DH3 | \
                       GAP_PKT_TYPE_DM5 | GAP_PKT_TYPE_DH5 | \
                       GAP_PKT_TYPE_NO_3DH1 | GAP_PKT_TYPE_NO_3DH3 | GAP_PKT_TYPE_NO_3DH5;
        }
        else if (link_pkt_type == BT_ACL_PKT_TYPE_3M)
        {
            pkt_type = GAP_PKT_TYPE_DM1 | GAP_PKT_TYPE_DH1 | \
                       GAP_PKT_TYPE_DM3 | GAP_PKT_TYPE_DH3 | \
                       GAP_PKT_TYPE_DM5 | GAP_PKT_TYPE_DH5;
        }

        if (gap_br_cfg_acl_pkt_type(bd_addr, pkt_type) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_link_preferred_data_rate_set(uint8_t             bd_addr[6],
                                     T_BT_LINK_DATA_RATE basic_data_rate,
                                     T_BT_LINK_DATA_RATE enhanced_data_rate)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (gap_br_link_preferred_data_rate_set(bd_addr,
                                                basic_data_rate,
                                                enhanced_data_rate) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_link_qos_set(uint8_t       bd_addr[6],
                     T_BT_QOS_TYPE type,
                     uint16_t      tpoll)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        uint32_t latency;

        /* Tpoll = min(180000 / token_rate, latency / 1250), where the Spec limits
         * Tpoll between 0x06 and 0x1000.
         * We fix Tpoll calculated by token rate at the maximum value 0x1000, and
         * thus Tpoll depends on latency only. That is token_rate = 180000 / 0x1000.
         */
        latency = tpoll * 1250;
        if (gap_br_send_setup_qos_req(link->acl_handle,
                                      2,
                                      type,
                                      44,
                                      0,
                                      latency,
                                      0) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_link_per_report(uint8_t  bd_addr[6],
                        bool     enable,
                        uint16_t period)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (link->acl_link_state == BT_LINK_STATE_CONNECTED)
        {
            if (gap_br_vendor_set_monitor(link->acl_handle, 0, enable, period) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_link_rssi_report(uint8_t  bd_addr[6],
                         bool     enable,
                         uint16_t period)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (link->acl_link_state == BT_LINK_STATE_CONNECTED)
        {
            if (gap_br_vendor_set_monitor(link->acl_handle, 1, enable, period) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_pairing_tx_power_set(int8_t offset_level)
{
    if (gap_br_vendor_tx_power_set(0xffff, offset_level) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_link_tx_power_set(uint8_t bd_addr[6],
                          int8_t  offset_level)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        if (gap_br_vendor_tx_power_set(link->acl_handle, offset_level) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_link_rssi_golden_range_set(uint8_t bd_addr[6],
                                   int16_t max_rssi,
                                   int16_t min_rssi)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        if (gap_br_vendor_rssi_golden_range_set(link->acl_handle, max_rssi,
                                                min_rssi) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_gap_br_param_set(T_GAP_BR_PARAM_TYPE  type,
                         uint8_t              len,
                         void                *p_value)
{
    if (gap_br_set_param(type, len, p_value) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }

}

bool bt_link_radio_mode_set(uint8_t radio_mode,
                            bool    limited_discov,
                            uint8_t limited_discov_time)
{
    if (gap_br_set_radio_mode(radio_mode, limited_discov, limited_discov_time) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_link_role_accept(uint8_t role)
{
    if (gap_br_cfg_accept_role(role) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_sco_conn_req(uint8_t  bd_addr[6],
                     uint32_t tx_bandwidth,
                     uint32_t rx_bandwidth,
                     uint16_t max_latency,
                     uint16_t voice_setting,
                     uint8_t  retrans_effort,
                     uint16_t packet_type)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (gap_br_send_sco_conn_req(bd_addr, tx_bandwidth, rx_bandwidth, max_latency, voice_setting,
                                     retrans_effort, packet_type) == GAP_CAUSE_SUCCESS)
        {
            link->sco_state = BT_LINK_SCO_STATE_CONNECTING;
            return true;
        }
    }

    return false;
}

bool bt_sco_disconn_req(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
        {
            if (gap_br_send_sco_disconn_req(bd_addr) == GAP_CAUSE_SUCCESS)
            {
                link->sco_state = BT_LINK_SCO_STATE_DISCONNECTING;
                return true;
            }
        }
    }

    return false;
}

bool bt_link_inquiry_scan_param_set(uint8_t  type,
                                    uint16_t interval,
                                    uint16_t window)
{
    if (gap_br_cfg_inquiry_scan_param(type, interval, window) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_link_page_scan_param_set(uint8_t  type,
                                 uint16_t interval,
                                 uint16_t window)
{
    if (gap_br_cfg_page_scan_param(type, interval, window) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_link_sniff_mode_enter(uint8_t  bd_addr[6],
                              uint16_t min_interval,
                              uint16_t max_interval,
                              uint16_t sniff_attempt,
                              uint16_t sniff_timeout)
{
    if (gap_br_enter_sniff_mode(bd_addr, min_interval, max_interval, sniff_attempt, sniff_timeout)
        == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_link_sniff_mode_exit(uint8_t bd_addr[6])
{
    if (gap_br_exit_sniff_mode(bd_addr) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_link_default_policy_set(uint16_t link_policy)
{
    if (gap_br_cfg_default_link_policy(link_policy) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_link_disconn_req(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        if (gap_br_send_acl_disconn_req(bd_addr) == GAP_CAUSE_SUCCESS)
        {
            link->acl_link_state = BT_LINK_STATE_DISCONNECTING;
            return true;
        }
    }

    return false;
}

bool bt_link_flush_tout_set(uint8_t  bd_addr[6],
                            uint16_t flush_tout)
{
    if (gap_br_cfg_acl_link_flush_tout(bd_addr, flush_tout) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_vendor_data_rate_set(uint8_t rate)
{
    if (gap_br_vendor_data_rate_set(rate) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_link_rssi_read(uint8_t bd_addr[6])
{
    if (gap_br_read_rssi(bd_addr) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_link_tpoll_range_set(uint8_t  bd_addr[6],
                             uint16_t min_tpoll,
                             uint16_t max_tpoll)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        bool enable;

        if (min_tpoll == 0x06 && max_tpoll == 0x1000)
        {
            enable = false;
        }
        else
        {
            enable = true;
        }

        if (gap_br_vendor_remote_tpoll_set(link->acl_handle, enable) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_link_traffic_qos_set(uint8_t                bd_addr[6],
                             T_BT_TRAFFIC_QOS_TYPE  type,
                             T_BT_TRAFFIC_QOS_PARAM param)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        switch (type)
        {
        case BT_RANDOM_TRAFFIC_QOS:
            {
                if (gap_br_vendor_htpoll_set(0x00,
                                             true,
                                             link->acl_handle,
                                             0,
                                             0,
                                             param.u.random_param.rsvd_slots,
                                             param.u.random_param.traffic_mask) == GAP_CAUSE_SUCCESS)
                {
                    return true;
                }
            }
            break;

        case BT_PERIODIC_SYNC_TRAFFIC_QOS:
            {
                if (gap_br_vendor_htpoll_set(0x01,
                                             true,
                                             link->acl_handle,
                                             param.u.periodic_sync_param.sync_weighting,
                                             0,
                                             param.u.periodic_sync_param.rsvd_slots,
                                             param.u.periodic_sync_param.traffic_mask) == GAP_CAUSE_SUCCESS)
                {
                    return true;
                }
            }
            break;

        case BT_NON_NEGOTIATE_PERIODIC_TRAFFIC_QOS:
            {
                if (gap_br_vendor_htpoll_set(2,
                                             true,
                                             link->acl_handle,
                                             0,
                                             param.u.none_negotiate_periodic_param.interval,
                                             param.u.none_negotiate_periodic_param.rsvd_slots,
                                             param.u.none_negotiate_periodic_param.traffic_mask) == GAP_CAUSE_SUCCESS)
                {
                    return true;
                }
            }
            break;

        case BT_NEGOTIATE_PERIODIC_TRAFFIC_QOS:
            {
                if (gap_br_vendor_htpoll_set(0xff,
                                             true,
                                             link->acl_handle,
                                             0,
                                             param.u.negotiate_periodic_param.interval,
                                             param.u.negotiate_periodic_param.rsvd_slots,
                                             0xffff) == GAP_CAUSE_SUCCESS)
                {
                    return true;
                }
            }
            break;

        default:
            break;
        }
    }

    return false;
}

bool bt_link_idle_zone_set(uint8_t bd_addr[6],
                           uint8_t interval,
                           uint8_t idle_slot,
                           uint8_t idle_skip)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        if (gap_br_set_idle_zone(link->acl_handle, interval,
                                 idle_slot, idle_skip) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_link_traffic_qos_clear(uint8_t               bd_addr[6],
                               T_BT_TRAFFIC_QOS_TYPE type)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        uint8_t htpoll_type;

        if (type == BT_NEGOTIATE_PERIODIC_TRAFFIC_QOS)
        {
            htpoll_type = 0xff;
        }
        else
        {
            htpoll_type = type;
        }

        if (gap_br_vendor_htpoll_set(htpoll_type, false, link->acl_handle, 0, 0, 0, 0) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_link_periodic_traffic_set(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
        {
            if (gap_br_periodic_traffic_set(link->sco_handle) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
        else
        {
            if (gap_br_periodic_traffic_set(link->acl_handle) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_mgr_pm_cback(uint8_t       bd_addr[6],
                     T_BT_PM_EVENT event)
{
    T_BT_LINK *link;
    bool       ret = true;

    link = bt_link_find(bd_addr);
    if (link)
    {
        switch (event)
        {
        case BT_PM_EVENT_LINK_CONNECTED:
            break;

        case BT_PM_EVENT_LINK_DISCONNECTED:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_SUCCESS:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_FAIL:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_REQ:
            if ((link->pm_enable == false) || (link->acl_link_authenticated == false))
            {
                /* sniff mode disallowed */
                ret = false;
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_SUCCESS:
            break;

        case BT_PM_EVENT_SNIFF_EXIT_FAIL:
            break;

        case BT_PM_EVENT_SNIFF_EXIT_REQ:
            break;
        }
    }

    return ret;
}

bool bt_sniffing_link_connect(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        return bt_roleswap_conn_sniffing_link(bd_addr);
    }

    return false;
}

bool bt_sniffing_link_disconnect(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        return bt_roleswap_disconn_sniffing_link(bd_addr);
    }

    return false;
}

bool bt_sniffing_link_audio_start(uint8_t  bd_addr[6],
                                  uint16_t interval,
                                  uint16_t flush_tout,
                                  uint8_t  rsvd_slot,
                                  uint8_t  idle_slot,
                                  uint8_t  idle_skip)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        return bt_roleswap_conn_audio_recovery(bd_addr, interval, flush_tout,
                                               rsvd_slot, idle_slot, idle_skip);
    }

    return false;
}


bool bt_sniffing_link_audio_stop(uint8_t bd_addr[6],
                                 uint8_t reason)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        return bt_roleswap_disconn_audio_recovery(bd_addr, reason);
    }

    return false;
}

bool bt_sniffing_link_audio_cfg(uint8_t  bd_addr[6],
                                uint16_t interval,
                                uint16_t flush_tout,
                                uint8_t  rsvd_slot,
                                uint8_t  idle_slot,
                                uint8_t  idle_skip)
{
    return bt_roleswap_cfg_audio_recovery(bd_addr, interval, flush_tout,
                                          rsvd_slot, idle_slot, idle_skip);
}

bool bt_sniffing_link_voice_start(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
        {
            return bt_roleswap_conn_voice_recovery(bd_addr);
        }
    }

    return false;
}

bool bt_sniffing_link_voice_stop(uint8_t bd_addr[6],
                                 uint8_t reason)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        return bt_roleswap_disconn_voice_recovery(bd_addr, reason);
    }

    return false;
}

void bt_mgr_handle_gap_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM param;

    switch (msg)
    {
    case BT_MSG_GAP_BT_READY:
        {
            bt_sdp_enable(true);

            bt_sdp_record_register();

            memcpy(param.ready.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_READY, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_RADIO_MODE_RSP:
        {
            bt_device_mode_sm(BT_DEVICE_MODE_EVENT_RSP, payload->msg_buf);
        }
        break;

    case BT_MSG_GAP_LOCAL_ADDR_CHANGED:
        {
            memcpy(param.local_addr_changed.bd_addr, payload->bd_addr, 6);
            param.local_addr_changed.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_LOCAL_ADDR_CHANGED, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_LOCAL_NAME_CHANGED:
        {
            param.local_name_changed.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_LOCAL_NAME_CHANGED, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_REMOTE_NAME_RSP:
        {
            T_GAP_REMOTE_NAME_INFO *p_data = (T_GAP_REMOTE_NAME_INFO *)payload->msg_buf;

            memcpy(param.remote_name_rsp.bd_addr, p_data->bd_addr, 6);
            param.remote_name_rsp.cause = p_data->cause;
            strncpy(param.remote_name_rsp.name, (char *)(p_data->name), 40);
            param.remote_name_rsp.name[39] = '\0';
            bt_mgr_event_post(BT_EVENT_REMOTE_NAME_RSP, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_ADD_SDP_RECORD_RSP:
        {
            T_BT_EVENT_PARAM param;
            T_GAP_ADD_SDP_RECORD_RSP *p_info = (T_GAP_ADD_SDP_RECORD_RSP *)payload->msg_buf;

            memcpy(param.sdp_add_record.bd_addr, payload->bd_addr, 6);
            param.sdp_add_record.cause = p_info->cause;

            bt_handle_add_sdp_rsp(p_info);
            bt_mgr_event_post(BT_EVENT_ADD_SDP_RECORD_RSP, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_DEL_SDP_RECORD_RSP:
        {
            T_BT_EVENT_PARAM param;
            T_GAP_DEL_SDP_RECORD_RSP *p_info = (T_GAP_DEL_SDP_RECORD_RSP *)payload->msg_buf;

            memcpy(param.sdp_del_record.bd_addr, payload->bd_addr, 6);
            param.sdp_del_record.cause = p_info->cause;

            bt_handle_del_sdp_rsp(p_info);
            bt_mgr_event_post(BT_EVENT_DEL_SDP_RECORD_RSP, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_SDP_ATTR_INFO:
        {
            T_BT_SDP_ATTR_INFO *sdp_attr_info = (T_BT_SDP_ATTR_INFO *)payload->msg_buf;

            memcpy(param.sdp_attr_info.bd_addr, payload->bd_addr, 6);
            memcpy(&param.sdp_attr_info.info, sdp_attr_info, sizeof(T_BT_SDP_ATTR_INFO));
            bt_mgr_event_post(BT_EVENT_SDP_ATTR_INFO, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_DID_ATTR_INFO:
        {
            T_GAP_DID_ATTR_INFO *p_info = (T_GAP_DID_ATTR_INFO *)payload->msg_buf;

            param.did_attr_info.specific_id = p_info->specific_id;
            param.did_attr_info.vendor_id = p_info->vendor_id;
            param.did_attr_info.vendor_src = p_info->vendor_src;
            param.did_attr_info.product_id = p_info->product_id;
            param.did_attr_info.version = p_info->version;
            memcpy(param.did_attr_info.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_DID_ATTR_INFO, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_SDP_DISCOV_CMPL:
        {
            memcpy(param.sdp_discov_cmpl.bd_addr, payload->bd_addr, 6);
            param.sdp_discov_cmpl.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_SDP_DISCOV_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_SDP_DISCOV_STOP:
        {
            memcpy(param.sdp_discov_stop.bd_addr, payload->bd_addr, 6);
            param.sdp_discov_stop.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_SDP_DISCOV_STOP, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_INQUIRY_RESULT_INFO:
        {
            T_GAP_INQUIRY_RESULT_INFO *p_info = (T_GAP_INQUIRY_RESULT_INFO *)payload->msg_buf;

            param.inquiry_result.cod = p_info->cod;
            param.inquiry_result.rssi = p_info->rssi;
            param.inquiry_result.eir_len = p_info->eir_len;
            param.inquiry_result.p_eir = p_info->p_eir;
            memcpy(param.inquiry_result.bd_addr, p_info->bd_addr, 6);
            strncpy(param.inquiry_result.name, (char *)(p_info->name), 40);
            param.inquiry_result.name[39] = '\0';
            bt_mgr_event_post(BT_EVENT_INQUIRY_RESULT, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_INQUIRY_RSP:
        {
            param.inquiry_rsp.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_INQUIRY_RSP, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_INQUIRY_CMPL:
        {
            param.inquiry_cmpl.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_INQUIRY_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_INQUIRY_CANCEL_RSP:
        {
            param.inquiry_cancel_rsp.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_INQUIRY_CANCEL_RSP, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_LOCAL_OOB_DATA_RSP:
        {
            T_GAP_LOCAL_OOB_RSP *p_info = (T_GAP_LOCAL_OOB_RSP *)payload->msg_buf;

            param.local_oob_data_rsp.cause = p_info->cause;
            memcpy(&param.local_oob_data_rsp.data_c[0], &p_info->data_c[0], 16);
            memcpy(&param.local_oob_data_rsp.data_r[0], &p_info->data_r[0], 16);
            bt_mgr_event_post(BT_EVENT_LOCAL_OOB_DATA_RSP, &param, sizeof(param));
        }
        break;

    case BT_MSG_GAP_LOCAL_OOB_EXTENDED_DATA_RSP:
        {
            T_GAP_LOCAL_OOB_EXTENDED_DATA_RSP *p_info = (T_GAP_LOCAL_OOB_EXTENDED_DATA_RSP *)payload->msg_buf;

            param.local_oob_extended_data_rsp.cause = p_info->cause;
            memcpy(&param.local_oob_extended_data_rsp.data_c_192[0], &p_info->data_c_192[0], 16);
            memcpy(&param.local_oob_extended_data_rsp.data_r_192[0], &p_info->data_r_192[0], 16);
            memcpy(&param.local_oob_extended_data_rsp.data_c_256[0], &p_info->data_c_256[0], 16);
            memcpy(&param.local_oob_extended_data_rsp.data_r_256[0], &p_info->data_r_256[0], 16);
            bt_mgr_event_post(BT_EVENT_LOCAL_OOB_EXTENDED_DATA_RSP, &param, sizeof(param));
        }
        break;

    default:
        break;
    }
}

void bt_mgr_handle_acl_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM param;
    uint8_t peer_addr[6];
    remote_peer_addr_get(peer_addr);
    bt_roleswap_handle_acl_status(payload->bd_addr, msg, payload->msg_buf);

    /* do not post event if secondary got acl disconn when sniffing link disconnected */
    if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY &&
        memcmp(payload->bd_addr, peer_addr, 6))
    {
        return;
    }

    switch (msg)
    {
    case BT_MSG_ACL_CONN_IND:
        {
            param.acl_conn_ind.cod = *(uint32_t *)payload->msg_buf;
            memcpy(param.acl_conn_ind.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_ACL_CONN_IND, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_ACTIVE:
        {
            memcpy(param.acl_conn_active.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_ACL_CONN_ACTIVE, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_SNIFF:
        {
            memcpy(param.acl_conn_sniff.bd_addr, payload->bd_addr, 6);
            param.acl_conn_sniff.interval = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_ACL_CONN_SNIFF, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_SNIFF_ENTER_FAIL:
        {
            T_BT_LINK *link;

            link = bt_link_find(payload->bd_addr);
            if (link != NULL)
            {
                bt_pm_sm(link->bd_addr, BT_PM_EVENT_SNIFF_ENTER_FAIL);
            }
        }
        break;

    case BT_MSG_ACL_SNIFF_EXIT_FAIL:
        {
            T_BT_LINK *link;

            link = bt_link_find(payload->bd_addr);
            if (link != NULL)
            {
                bt_pm_sm(link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_FAIL);
            }
        }
        break;

    case BT_MSG_ACL_AUTHEN_START:
        {
            memcpy(param.acl_authen_start.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_ACL_AUTHEN_START, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_AUTHEN_SUCCESS:
        {
            memcpy(param.acl_authen_success.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_ACL_AUTHEN_SUCCESS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_AUTHEN_FAIL:
        {
            memcpy(param.acl_authen_fail.bd_addr, payload->bd_addr, 6);
            param.acl_authen_fail.cause = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_ACL_AUTHEN_FAIL, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_ENCRYPTED:
        {
            memcpy(param.acl_conn_encrypted.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_ACL_CONN_ENCRYPTED, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_NOT_ENCRYPTED:
        {
            memcpy(param.acl_conn_not_encrypted.bd_addr, payload->bd_addr, 6);
            param.acl_conn_not_encrypted.cause = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_ACL_CONN_NOT_ENCRYPTED, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_DISCONN:
        {
            memcpy(param.acl_conn_disconn.bd_addr, payload->bd_addr, 6);
            param.acl_conn_disconn.cause = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_ACL_CONN_DISCONN, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_ROLE_MASTER:
        {
            memcpy(param.acl_role_master.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_ACL_ROLE_MASTER, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_ROLE_SLAVE:
        {
            memcpy(param.acl_role_slave.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_ACL_ROLE_SLAVE, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_ROLE_SWITCH_FAIL:
        {
            memcpy(param.acl_role_switch_fail.bd_addr, payload->bd_addr, 6);
            param.acl_role_switch_fail.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_ACL_ROLE_SWITCH_FAIL, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_FAIL:
        {
            memcpy(param.acl_conn_fail.bd_addr, payload->bd_addr, 6);
            param.acl_conn_fail.cause = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_ACL_CONN_FAIL, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_SUCCESS:
        {
            T_BT_LINK *link;

            link = bt_link_find(payload->bd_addr);
            if (link != NULL)
            {
                bt_pm_cback_register(link->bd_addr, bt_mgr_pm_cback);
            }

            memcpy(param.acl_conn_success.bd_addr, payload->bd_addr, 6);
            param.acl_conn_success.handle = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_ACL_CONN_SUCCESS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_READY:
        {
            memcpy(param.acl_conn_ready.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_ACL_CONN_READY, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_BR_SC_START:
        {
            memcpy(param.acl_conn_br_sc_start.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_ACL_CONN_BR_SC_START, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_CONN_BR_SC_CMPL:
        {
            memcpy(param.acl_conn_br_sc_cmpl.bd_addr, payload->bd_addr, 6);
            param.acl_conn_br_sc_cmpl.cause = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_ACL_CONN_BR_SC_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_LINK_KEY_INFO:
        {
            T_GAP_LINK_KEY_INFO *p_info = (T_GAP_LINK_KEY_INFO *)(payload->msg_buf);

            param.link_key_info.key_type = (T_BT_LINK_KEY_TYPE)(p_info->key_type);
            memcpy(param.link_key_info.bd_addr, p_info->bd_addr, 6);
            memcpy(param.link_key_info.link_key, p_info->link_key, 16);
            bt_mgr_event_post(BT_EVENT_LINK_KEY_INFO, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_LINK_KEY_REQ_IND:
        {
            T_GAP_LINK_KEY_REQ_IND *p_info = (T_GAP_LINK_KEY_REQ_IND *)(payload->msg_buf);
            memcpy(param.link_key_req.bd_addr, p_info->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_LINK_KEY_REQ, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_PIN_CODE_REQ_IND:
        {
            T_GAP_PIN_CODE_REQ_IND *p_info = (T_GAP_PIN_CODE_REQ_IND *)payload->msg_buf;
            memcpy(param.link_pin_code_req.bd_addr, p_info->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_LINK_PIN_CODE_REQ, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_USER_CONFIRM_REQ:
        {
            T_GAP_USER_CFM_REQ_IND *p_ind = (T_GAP_USER_CFM_REQ_IND *)payload->msg_buf;

            memcpy(param.link_user_confirmation_req.bd_addr, p_ind->bd_addr, 6);
            param.link_user_confirmation_req.just_works = p_ind->just_works;
            param.link_user_confirmation_req.display_value = p_ind->display_value;
            bt_mgr_event_post(BT_EVENT_LINK_USER_CONFIRMATION_REQ, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_USER_PASSKEY_REQ_IND:
        {
            T_GAP_USER_PASSKEY_REQ_IND *p_info = (T_GAP_USER_PASSKEY_REQ_IND *)payload->msg_buf;

            memcpy(param.link_user_passkey_req.bd_addr, payload->bd_addr, 6);
            param.link_user_passkey_req.key_press = p_info->key_press;
            bt_mgr_event_post(BT_EVENT_LINK_USER_PASSKEY_REQ, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_USER_PASSKEY_NOTIF:
        {
            T_GAP_USER_PASSKEY_NOTIF_INFO *p_info = (T_GAP_USER_PASSKEY_NOTIF_INFO *)payload->msg_buf;

            memcpy(param.link_user_passkey_notif.bd_addr, payload->bd_addr, 6);
            param.link_user_passkey_notif.display_value = p_info->display_value;
            bt_mgr_event_post(BT_EVENT_LINK_USER_PASSKEY_NOTIF, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_READ_RSSI_RSP:
        {
            T_GAP_READ_RSSI_RSP *p_info = (T_GAP_READ_RSSI_RSP *)payload->msg_buf;

            memcpy(param.link_read_rssi_rsp.bd_addr, payload->bd_addr, 6);
            param.link_read_rssi_rsp.cause = p_info->cause;
            param.link_read_rssi_rsp.rssi = p_info->rssi;
            bt_mgr_event_post(BT_EVENT_LINK_READ_RSSI_RSP, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_SET_QOS_RSP:
        {
            T_GAP_SETUP_QOS_RSP *p_rsp = (T_GAP_SETUP_QOS_RSP *)payload->msg_buf;

            memcpy(param.link_qos_set_rsp.bd_addr, payload->bd_addr, 6);
            param.link_qos_set_rsp.cause = p_rsp->cause;
            bt_mgr_event_post(BT_EVENT_LINK_QOS_SET_RSP, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_SET_QOS_CMPL:
        {
            T_GAP_SETUP_QOS_CMPL *p_cmpl = (T_GAP_SETUP_QOS_CMPL *)payload->msg_buf;

            memcpy(param.link_qos_set_cmpl.bd_addr, p_cmpl->bd_addr, 6);
            param.link_qos_set_cmpl.cause = p_cmpl->cause;
            param.link_qos_set_cmpl.tpoll = p_cmpl->tpoll;
            bt_mgr_event_post(BT_EVENT_LINK_QOS_SET_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_ACL_LINK_POLICY_RSP:
        {
            bt_roleswap_handle_link_policy(payload->bd_addr);
        }
        break;

    default:
        break;
    }
}

void bt_mgr_handle_sco_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM param;

    switch (msg)
    {
    case BT_MSG_SCO_CONN_IND:
        {
            T_GAP_SCO_CONN_IND *p_info = (T_GAP_SCO_CONN_IND *)payload->msg_buf;

            memcpy(param.sco_conn_ind.bd_addr, p_info->bd_addr, 6);
            param.sco_conn_ind.is_esco = p_info->is_esco;
            bt_mgr_event_post(BT_EVENT_SCO_CONN_IND, &param, sizeof(param));
        }
        break;

    case BT_MSG_SCO_CONN_RSP:
        {
            T_BT_LINK *link;
            T_GAP_SCO_CONN_RSP *p_info = (T_GAP_SCO_CONN_RSP *)payload->msg_buf;

            link = bt_link_find(p_info->bd_addr);
            if (link != NULL)
            {
                bt_hfp_ag_handle_audio_conn_rsp(p_info->bd_addr, p_info->cause);

                param.sco_conn_rsp.cause = p_info->cause;
                memcpy(param.sco_conn_rsp.bd_addr, p_info->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_SCO_CONN_RSP, &param, sizeof(param));
            }
        }
        break;

    case BT_MSG_SCO_CONN_CMPL:
        {
            T_BT_LINK *link;
            T_GAP_SCO_CONN_CMPL_INFO *p_info = (T_GAP_SCO_CONN_CMPL_INFO *)payload->msg_buf;

            link = bt_link_find(p_info->bd_addr);
            if (link != NULL)
            {
                if (p_info->cause == 0)
                {
                    if (remote_session_role_get() != REMOTE_SESSION_ROLE_SINGLE)
                    {
                        bt_roleswap_handle_sco_conn(p_info->bd_addr);
                    }
                }

                bt_hfp_handle_audio_conn_cmpl(p_info->bd_addr);
                bt_hfp_ag_handle_audio_conn_cmpl(p_info->bd_addr, p_info->cause);

                memcpy(param.sco_conn_cmpl.bd_addr, p_info->bd_addr, 6);
                param.sco_conn_cmpl.handle = p_info->handle;
                param.sco_conn_cmpl.cause = p_info->cause;
                param.sco_conn_cmpl.air_mode = p_info->air_mode;
                param.sco_conn_cmpl.is_esco = p_info->is_esco;

                if (p_info->is_esco)
                {
                    param.sco_conn_cmpl.tx_pkt_len = p_info->tx_length;
                    param.sco_conn_cmpl.rx_pkt_len = p_info->rx_length;
                }
                else
                {
                    param.sco_conn_cmpl.tx_pkt_len = 30;
                    param.sco_conn_cmpl.rx_pkt_len = 30;
                }

                bt_mgr_event_post(BT_EVENT_SCO_CONN_CMPL, &param, sizeof(param));
            }
        }
        break;

    case BT_MSG_SCO_DISCONN_CMPL:
        {
            T_BT_LINK *link;

            link = bt_link_find(payload->bd_addr);
            if (link != NULL)
            {
                bt_hfp_handle_audio_disconn(payload->bd_addr);
                bt_hfp_ag_handle_audio_disconn(payload->bd_addr);
            }

            bt_handle_sco_disconnect(payload->bd_addr);
            bt_roleswap_handle_sco_disconn(payload->bd_addr, *(uint16_t *)payload->msg_buf);

            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                param.sco_disconnected.cause = *(uint16_t *)payload->msg_buf;
                memcpy(param.sco_disconnected.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_SCO_DISCONNECTED, &param, sizeof(param));
            }
            else
            {
                param.sco_sniffing_disconn_cmpl.cause = *(uint16_t *)payload->msg_buf;
                memcpy(param.sco_sniffing_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_SCO_SNIFFING_DISCONN_CMPL, &param, sizeof(param));
            }

        }
        break;

    case BT_MSG_SCO_DATA_IND:
        {
            T_BT_LINK *link;

            link = bt_link_find(payload->bd_addr);
            if (link != NULL)
            {
                T_BT_SCO_DATA_IND *sco_data;

                sco_data = (T_BT_SCO_DATA_IND *)payload->msg_buf;
                memcpy(param.sco_data_ind.bd_addr, link->bd_addr, 6);
                param.sco_data_ind.handle = sco_data->handle;
                param.sco_data_ind.length = sco_data->length;
                param.sco_data_ind.status = (T_BT_SCO_PKT_STATUS)sco_data->status;
                param.sco_data_ind.p_data = sco_data->p_data;
                param.sco_data_ind.bt_clk = sco_data->bt_clk;
                bt_mgr_event_post(BT_EVENT_SCO_DATA_IND, &param, sizeof(param));
            }
        }
        break;

    default:
        break;
    }
}

void bt_mgr_handle_a2dp_msg(T_BT_MSG          msg,
                            T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM  param;
    T_BT_LINK        *link;

    link = bt_link_find(payload->bd_addr);
    if (link != NULL)
    {
        switch (msg)
        {
        case BT_MSG_A2DP_CONN_IND:
            {
                memcpy(param.a2dp_conn_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_A2DP_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_CONN_CMPL:
            {
                memcpy(param.a2dp_conn_cmpl.bd_addr, payload->bd_addr, 6);
                bt_roleswap_handle_profile_conn(link->bd_addr, A2DP_PROFILE_MASK,
                                                ROLESWAP_A2DP_PARAM_SINGAL);
                bt_mgr_event_post(BT_EVENT_A2DP_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_CONN_FAIL:
            {
                uint16_t cause = *(uint16_t *)(payload->msg_buf);
                memcpy(param.a2dp_conn_fail.bd_addr, payload->bd_addr, 6);
                param.a2dp_conn_fail.cause = cause;
                bt_mgr_event_post(BT_EVENT_A2DP_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_STREAM_OPEN:
            {
                memcpy(param.a2dp_stream_open.bd_addr, payload->bd_addr, 6);
                param.a2dp_stream_open.max_pkt_len = *((uint16_t *)payload->msg_buf);
                bt_roleswap_handle_profile_conn(link->bd_addr, A2DP_PROFILE_MASK,
                                                ROLESWAP_A2DP_PARAM_STREAM);
                bt_mgr_event_post(BT_EVENT_A2DP_STREAM_OPEN, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_STREAM_OPEN_FAIL:
            {
                uint16_t cause = *(uint16_t *)(payload->msg_buf);
                memcpy(param.a2dp_stream_open_fail.bd_addr, payload->bd_addr, 6);
                param.a2dp_stream_open_fail.cause = cause;
                bt_mgr_event_post(BT_EVENT_A2DP_STREAM_OPEN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_DISCONN_CMPL:
            {
                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;
                uint16_t cause = *(uint16_t *)(payload->msg_buf);

                disconn_param.profile_mask = A2DP_PROFILE_MASK;
                disconn_param.cause = cause;
                disconn_param.param = ROLESWAP_A2DP_PARAM_SINGAL;
                bt_roleswap_handle_profile_disconn(link->bd_addr,
                                                   &disconn_param);

                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    memcpy(param.a2dp_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.a2dp_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_A2DP_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
                else
                {
                    memcpy(param.a2dp_sniffing_disconn_cmpl.bd_addr,
                           payload->bd_addr, 6);
                    param.a2dp_sniffing_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_A2DP_SNIFFING_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
            }
            break;

        case BT_MSG_A2DP_CONFIG_CMPL:
            {
                T_BT_A2DP_CFG_INFO *data = (T_BT_A2DP_CFG_INFO *)payload->msg_buf;
                T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL *cfg;

                memcpy(param.a2dp_config_cmpl.bd_addr, payload->bd_addr, 6);
                cfg = &param.a2dp_config_cmpl;

                param.a2dp_config_cmpl.codec_type = data->codec_type;
                param.a2dp_config_cmpl.role = data->role;
                if (param.a2dp_config_cmpl.codec_type == BT_A2DP_CODEC_TYPE_SBC)
                {
                    cfg->codec_info.sbc.sampling_frequency = data->codec_info.sbc.sampling_frequency;
                    cfg->codec_info.sbc.channel_mode = data->codec_info.sbc.channel_mode;
                    cfg->codec_info.sbc.block_length = data->codec_info.sbc.block_length;
                    cfg->codec_info.sbc.subbands = data->codec_info.sbc.subbands;
                    cfg->codec_info.sbc.allocation_method = data->codec_info.sbc.allocation_method;
                    cfg->codec_info.sbc.min_bitpool = data->codec_info.sbc.min_bitpool;
                    cfg->codec_info.sbc.max_bitpool = data->codec_info.sbc.max_bitpool;
                }
                else if (param.a2dp_config_cmpl.codec_type == BT_A2DP_CODEC_TYPE_AAC)
                {
                    cfg->codec_info.aac.object_type = data->codec_info.aac.object_type;
                    cfg->codec_info.aac.sampling_frequency = data->codec_info.aac.sampling_frequency;
                    cfg->codec_info.aac.channel_number = data->codec_info.aac.channel_number;
                    cfg->codec_info.aac.vbr_supported = data->codec_info.aac.vbr_supported;
                    cfg->codec_info.aac.bit_rate = data->codec_info.aac.bit_rate;
                }
                else if (param.a2dp_config_cmpl.codec_type == BT_A2DP_CODEC_TYPE_LDAC)
                {
                    cfg->codec_info.ldac.sampling_frequency = data->codec_info.ldac.sampling_frequency;
                    cfg->codec_info.ldac.channel_mode = data->codec_info.ldac.channel_mode;
                }
                else if (param.a2dp_config_cmpl.codec_type == BT_A2DP_CODEC_TYPE_LC3)
                {
                    cfg->codec_info.lc3.sampling_frequency = data->codec_info.lc3.sampling_frequency;
                    cfg->codec_info.lc3.channel_number = data->codec_info.lc3.channel_number;
                    cfg->codec_info.lc3.frame_duration = data->codec_info.lc3.frame_duration;
                    cfg->codec_info.lc3.frame_length = data->codec_info.lc3.frame_length;
                }
                else if (param.a2dp_config_cmpl.codec_type == BT_A2DP_CODEC_TYPE_LHDC)
                {
                    cfg->codec_info.lhdc.sampling_frequency = data->codec_info.lhdc.sampling_frequency;
                    cfg->codec_info.lhdc.min_bitrate = data->codec_info.lhdc.min_bitrate;
                    cfg->codec_info.lhdc.max_bitrate = data->codec_info.lhdc.max_bitrate;
                    cfg->codec_info.lhdc.bit_depth = data->codec_info.lhdc.bit_depth;
                    cfg->codec_info.lhdc.version_number = data->codec_info.lhdc.version_number;
                    cfg->codec_info.lhdc.low_latency = data->codec_info.lhdc.low_latency;
                    cfg->codec_info.lhdc.meta = data->codec_info.lhdc.meta;
                    cfg->codec_info.lhdc.jas = data->codec_info.lhdc.jas;
                    cfg->codec_info.lhdc.ar = data->codec_info.lhdc.ar;
                }
                else
                {
                    memcpy(cfg->codec_info.vendor.info, data->codec_info.vendor.info, 12);
                }

                bt_mgr_event_post(BT_EVENT_A2DP_CONFIG_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_DELAY_REPORT:
            {
                memcpy(param.a2dp_delay_report.bd_addr, payload->bd_addr, 6);
                param.a2dp_delay_report.delay = *(uint16_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_A2DP_DELAY_REPORT, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_STREAM_START_IND:
            {
                T_BT_A2DP_CFG_INFO *data = (T_BT_A2DP_CFG_INFO *)payload->msg_buf;
                T_BT_EVENT_PARAM_A2DP_STREAM_START_IND *cfg;

                cfg = &param.a2dp_stream_start_ind;

                param.a2dp_stream_start_ind.codec_type = data->codec_type;
                param.a2dp_stream_start_ind.role = data->role;
                if (param.a2dp_stream_start_ind.codec_type == BT_A2DP_CODEC_TYPE_SBC)
                {
                    cfg->codec_info.sbc.sampling_frequency = data->codec_info.sbc.sampling_frequency;
                    cfg->codec_info.sbc.channel_mode = data->codec_info.sbc.channel_mode;
                    cfg->codec_info.sbc.block_length = data->codec_info.sbc.block_length;
                    cfg->codec_info.sbc.subbands = data->codec_info.sbc.subbands;
                    cfg->codec_info.sbc.allocation_method = data->codec_info.sbc.allocation_method;
                    cfg->codec_info.sbc.min_bitpool = data->codec_info.sbc.min_bitpool;
                    cfg->codec_info.sbc.max_bitpool = data->codec_info.sbc.max_bitpool;
                }
                else if (param.a2dp_stream_start_ind.codec_type == BT_A2DP_CODEC_TYPE_AAC)
                {
                    cfg->codec_info.aac.object_type = data->codec_info.aac.object_type;
                    cfg->codec_info.aac.sampling_frequency = data->codec_info.aac.sampling_frequency;
                    cfg->codec_info.aac.channel_number = data->codec_info.aac.channel_number;
                    cfg->codec_info.aac.vbr_supported = data->codec_info.aac.vbr_supported;
                    cfg->codec_info.aac.bit_rate = data->codec_info.aac.bit_rate;
                }
                else if (param.a2dp_stream_start_ind.codec_type == BT_A2DP_CODEC_TYPE_LDAC)
                {
                    cfg->codec_info.ldac.sampling_frequency = data->codec_info.ldac.sampling_frequency;
                    cfg->codec_info.ldac.channel_mode = data->codec_info.ldac.channel_mode;
                }
                else if (param.a2dp_stream_start_ind.codec_type == BT_A2DP_CODEC_TYPE_LC3)
                {
                    cfg->codec_info.lc3.sampling_frequency = data->codec_info.lc3.sampling_frequency;
                    cfg->codec_info.lc3.channel_number = data->codec_info.lc3.channel_number;
                    cfg->codec_info.lc3.frame_duration = data->codec_info.lc3.frame_duration;
                    cfg->codec_info.lc3.frame_length = data->codec_info.lc3.frame_length;
                }
                else if (param.a2dp_stream_start_ind.codec_type == BT_A2DP_CODEC_TYPE_LHDC)
                {
                    cfg->codec_info.lhdc.sampling_frequency = data->codec_info.lhdc.sampling_frequency;
                    cfg->codec_info.lhdc.min_bitrate = data->codec_info.lhdc.min_bitrate;
                    cfg->codec_info.lhdc.max_bitrate = data->codec_info.lhdc.max_bitrate;
                    cfg->codec_info.lhdc.bit_depth = data->codec_info.lhdc.bit_depth;
                    cfg->codec_info.lhdc.version_number = data->codec_info.lhdc.version_number;
                    cfg->codec_info.lhdc.low_latency = data->codec_info.lhdc.low_latency;
                    cfg->codec_info.lhdc.meta = data->codec_info.lhdc.meta;
                    cfg->codec_info.lhdc.jas = data->codec_info.lhdc.jas;
                    cfg->codec_info.lhdc.ar = data->codec_info.lhdc.ar;
                }
                else
                {
                    memcpy(cfg->codec_info.vendor.info, data->codec_info.vendor.info, 12);
                }
                memcpy(param.a2dp_stream_start_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_A2DP_STREAM_START_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_STREAM_START_RSP:
            {
                memcpy(param.a2dp_stream_start_rsp.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_A2DP_STREAM_START_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_STREAM_DATA_IND:
            {
                T_BT_A2DP_STREAM_DATA_IND *data = (T_BT_A2DP_STREAM_DATA_IND *)payload->msg_buf;

                param.a2dp_stream_data_ind.payload = data->payload;
                param.a2dp_stream_data_ind.len = data->len;
                param.a2dp_stream_data_ind.seq_num = data->seq_num;
                param.a2dp_stream_data_ind.bt_clock = data->bt_clock;
                param.a2dp_stream_data_ind.frame_num = data->frame_num;
                param.a2dp_stream_data_ind.timestamp = data->timestamp;
                memcpy(param.a2dp_stream_data_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_A2DP_STREAM_DATA_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_STREAM_DATA_RSP:
            {
                memcpy(param.a2dp_stream_data_rsp.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_A2DP_STREAM_DATA_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_STREAM_STOP:
            {
                memcpy(param.a2dp_stream_stop.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_A2DP_STREAM_STOP, &param, sizeof(param));
            }
            break;

        case BT_MSG_A2DP_STREAM_CLOSE:
            {
                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;
                uint16_t cause = *(uint16_t *)(payload->msg_buf);

                disconn_param.profile_mask = A2DP_PROFILE_MASK;
                disconn_param.cause = cause;
                disconn_param.param = ROLESWAP_A2DP_PARAM_STREAM;
                bt_roleswap_handle_profile_disconn(link->bd_addr, &disconn_param);

                memcpy(param.a2dp_stream_close.bd_addr, payload->bd_addr, 6);
                param.a2dp_stream_close.cause = cause;
                bt_mgr_event_post(BT_EVENT_A2DP_STREAM_CLOSE, &param, sizeof(param));
            }
            break;

        default:
            break;
        }
    }
}

void bt_mgr_handle_avrcp_msg(T_BT_MSG          msg,
                             T_BT_MSG_PAYLOAD *payload)
{
    T_BT_LINK *link;

    link = bt_link_find(payload->bd_addr);
    if (link != NULL)
    {
        T_BT_EVENT_PARAM param;

        switch (msg)
        {
        case BT_MSG_AVRCP_CONN_IND:
            {
                memcpy(param.avrcp_conn_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_CONN_CMPL:
            {
                memcpy(param.avrcp_conn_cmpl.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_CONN_CMPL, &param, sizeof(param));

                bt_roleswap_handle_profile_conn(link->bd_addr, AVRCP_PROFILE_MASK, 0);
            }
            break;

        case BT_MSG_AVRCP_CONN_FAIL:
            {
                memcpy(param.avrcp_conn_fail.bd_addr, payload->bd_addr, 6);
                param.avrcp_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_AVRCP_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_DISCONN_CMPL:
            {
                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;
                uint16_t cause = *(uint16_t *)(payload->msg_buf);

                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    memcpy(param.avrcp_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.avrcp_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_AVRCP_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
                else
                {
                    memcpy(param.avrcp_sniffing_disconn_cmpl.bd_addr,
                           payload->bd_addr, 6);
                    param.avrcp_sniffing_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_AVRCP_SNIFFING_DISCONN_CMPL,
                                      &param, sizeof(param));
                }

                disconn_param.profile_mask = AVRCP_PROFILE_MASK;
                disconn_param.cause = cause;
                bt_roleswap_handle_profile_disconn(link->bd_addr, &disconn_param);
            }
            break;

        case BT_MSG_AVRCP_GET_CAPABILITIES_RSP:
            {
                T_BT_AVRCP_RSP_GET_CAPABILITIES *capabilities = (T_BT_AVRCP_RSP_GET_CAPABILITIES *)payload->msg_buf;

                memcpy(param.avrcp_get_capabilities_rsp.bd_addr, link->bd_addr, 6);
                param.avrcp_get_capabilities_rsp.state = capabilities->state;
                param.avrcp_get_capabilities_rsp.capability_count = capabilities->capability_count;
                param.avrcp_get_capabilities_rsp.capabilities = capabilities->capabilities;
                bt_mgr_event_post(BT_EVENT_AVRCP_GET_CAPABILITIES_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_ABSOLUTE_VOLUME_SET:
            {
                memcpy(param.avrcp_absolute_volume_set.bd_addr, payload->bd_addr, 6);
                param.avrcp_absolute_volume_set.volume = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_AVRCP_ABSOLUTE_VOLUME_SET, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_KEY_VOLUME_UP:
            {
                memcpy(param.avrcp_volume_up.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_VOLUME_UP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_KEY_VOLUME_DOWN:
            {
                memcpy(param.avrcp_volume_down.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_VOLUME_DOWN, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_POWER:
            {
                memcpy(param.avrcp_power.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_POWER, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_MUTE:
            {
                memcpy(param.avrcp_mute.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_MUTE, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_PLAY:
            {
                memcpy(param.avrcp_play.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_PLAY, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_STOP:
            {
                memcpy(param.avrcp_stop.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_STOP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_PAUSE:
            {
                memcpy(param.avrcp_pause.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_PAUSE, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_REWIND_START:
            {
                memcpy(param.avrcp_rewind_start.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_REWIND_START, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_REWIND_STOP:
            {
                memcpy(param.avrcp_rewind_stop.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_REWIND_STOP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_FAST_FORWARD_START:
            {
                memcpy(param.avrcp_fast_forward_start.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_FAST_FORWARD_START, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_FAST_FORWARD_STOP:
            {
                memcpy(param.avrcp_fast_forward_stop.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_FAST_FORWARD_STOP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_FORWARD:
            {
                memcpy(param.avrcp_forward.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_FORWARD, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_BACKWARD:
            {
                memcpy(param.avrcp_backward.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_BACKWARD, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_REG_VOL_CHANGE:
            {
                memcpy(param.avrcp_reg_volume_changed.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_REG_VOLUME_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_VOLUME_CHANGED:
            {
                memcpy(param.avrcp_volume_changed.bd_addr, payload->bd_addr, 6);
                param.avrcp_volume_changed.volume = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_AVRCP_VOLUME_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_REG_TRACK_CHANGE:
            {
                memcpy(param.avrcp_reg_track_changed.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_TRACK_CHANGED_REG_REQ, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_REG_PLAY_STATUS_CHANGE:
            {
                memcpy(param.avrcp_reg_play_status_changed.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_PLAY_STATUS_CHANGED_REG_REQ, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_PLAY_STATUS_RSP:
            {
                memcpy(param.avrcp_play_status_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_play_status_rsp.play_status = *(uint8_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_AVRCP_PLAY_STATUS_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_PLAY_STATUS_CHANGED:
            {
                memcpy(param.avrcp_play_status_changed.bd_addr, payload->bd_addr, 6);
                param.avrcp_play_status_changed.play_status = *(uint8_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_AVRCP_PLAY_STATUS_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_TRACK_CHANGED:
            {
                memcpy(param.avrcp_track_changed.bd_addr, payload->bd_addr, 6);
                param.avrcp_track_changed.track_id = *(uint64_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_AVRCP_TRACK_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_TRACK_REACHED_END:
            {
                memcpy(param.avrcp_track_reached_end.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_TRACK_REACHED_END, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_TRACK_REACHED_START:
            {
                memcpy(param.avrcp_track_reached_start.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_TRACK_REACHED_START, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_PLAYBACK_POS_CHANGED:
            {
                memcpy(param.avrcp_playback_pos_changed.bd_addr, payload->bd_addr, 6);
                param.avrcp_playback_pos_changed.playback_pos = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_AVRCP_PLAYBACK_POS_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_BATT_STATUS_CHANGED:
            {
                memcpy(param.avrcp_batt_status_changed.bd_addr, payload->bd_addr, 6);
                param.avrcp_batt_status_changed.batt_status = *(uint8_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_AVRCP_BATT_STATUS_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_SYSTEM_STATUS_CHANGED:
            {
                memcpy(param.avrcp_system_status_changed.bd_addr, payload->bd_addr, 6);
                param.avrcp_system_status_changed.system_status = *(uint8_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_AVRCP_SYSTEM_STATUS_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_GET_PLAY_STATUS_RSP:
            {
                T_BT_AVRCP_RSP_GET_PLAY_STATUS *play_status = (T_BT_AVRCP_RSP_GET_PLAY_STATUS *)payload->msg_buf;

                memcpy(param.avrcp_get_play_status_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_get_play_status_rsp.state = play_status->state;
                param.avrcp_get_play_status_rsp.play_status = play_status->play_status;
                param.avrcp_get_play_status_rsp.length_ms = play_status->length_ms;
                param.avrcp_get_play_status_rsp.position_ms = play_status->position_ms;
                bt_mgr_event_post(BT_EVENT_AVRCP_GET_PLAY_STATUS_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_GET_ELEM_ATTR_RSP:
            {
                T_BT_AVRCP_RSP_GET_ELEMENT_ATTR *attr = (T_BT_AVRCP_RSP_GET_ELEMENT_ATTR *)payload->msg_buf;

                memcpy(param.avrcp_elem_attr.bd_addr, link->bd_addr, 6);
                param.avrcp_elem_attr.state = attr->state;
                param.avrcp_elem_attr.num_of_attr = attr->num_of_attr;
                param.avrcp_elem_attr.attr = (T_BT_AVRCP_ELEMENT_ATTR *)attr->attr;
                bt_mgr_event_post(BT_EVENT_AVRCP_ELEM_ATTR, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_APP_SETTING_CHANGED:
            {
                T_BT_AVRCP_RSP_APP_SETTING_CHANGED *rsp;

                rsp = (T_BT_AVRCP_RSP_APP_SETTING_CHANGED *)payload->msg_buf;
                memcpy(param.avrcp_app_setting_changed.bd_addr, link->bd_addr, 6);
                param.avrcp_app_setting_changed.num_of_attr = rsp->num_of_attr;
                param.avrcp_app_setting_changed.p_app_setting = rsp->p_app_setting;
                bt_mgr_event_post(BT_EVENT_AVRCP_APP_SETTING_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_PLAYING_CONTENT_CHANGED:
            {
                memcpy(param.avrcp_playing_content_changed.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_PLAYING_CONTENT_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_AVAILABLE_PLAYER_CHANGED:
            {
                memcpy(param.avrcp_available_player_changed.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_AVAILABLE_PLAYER_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_GET_ELEM_ATTRS:
            {
                T_BT_AVRCP_REQ_GET_ELEMENT_ATTR *req;
                int i;

                req = (T_BT_AVRCP_REQ_GET_ELEMENT_ATTR *)payload->msg_buf;
                memcpy(param.avrcp_elem_attrs_get.bd_addr, payload->bd_addr, 6);
                param.avrcp_elem_attrs_get.attr_num = req->attr_num;
                for (i = 0; i < req->attr_num; i++)
                {
                    param.avrcp_elem_attrs_get.attr_id[i] = req->attr_id[i];
                }
                bt_mgr_event_post(BT_EVENT_AVRCP_ELEM_ATTRS_GET, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_GET_PLAY_STATUS:
            {
                memcpy(param.avrcp_play_status_get.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_PLAY_STATUS_GET, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_SET_ADDRESSED_PLAYER:
            {
                memcpy(param.avrcp_addressed_player_set.bd_addr, payload->bd_addr, 6);
                param.avrcp_addressed_player_set.player_id = *(uint16_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_AVRCP_ADDRESSED_PLAYER_SET, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_REG_ADDRESSED_PLAYER_CHANGE:
            {
                memcpy(param.avrcp_reg_addressed_player_changed.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_ADDRESSED_PLAYER_CHANGED_REG_REQ, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_GET_FOLDER_ITEMS:
            {
                T_BT_AVRCP_REQ_GET_FOLDER_ITEMS *req;
                int i;

                req = (T_BT_AVRCP_REQ_GET_FOLDER_ITEMS *)payload->msg_buf;
                memcpy(param.avrcp_folder_items_get.bd_addr, payload->bd_addr, 6);
                param.avrcp_folder_items_get.scope_id = req->scope;
                param.avrcp_folder_items_get.start_item = req->start_item;
                param.avrcp_folder_items_get.end_item = req->end_item;
                param.avrcp_folder_items_get.attr_count = req->attr_count;
                for (i = 0; i < req->attr_count; i++)
                {
                    param.avrcp_folder_items_get.attr_id[i] = req->attr_id[i];
                }
                bt_mgr_event_post(BT_EVENT_AVRCP_FOLDER_ITEMS_GET, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_GET_TOTAL_NUM_OF_ITEMS:
            {
                memcpy(param.avrcp_total_num_of_items_get.bd_addr, payload->bd_addr, 6);
                param.avrcp_total_num_of_items_get.scope_id = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_AVRCP_TOTAL_NUM_OF_ITEMS_GET, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_ADDRESSED_PLAYER_CHANGED:
            {
                T_BT_AVRCP_RSP_ADDRESSED_PLAYER_CHANGED *rsp;

                rsp = (T_BT_AVRCP_RSP_ADDRESSED_PLAYER_CHANGED *)payload->msg_buf;
                memcpy(param.avrcp_addressed_player_changed.bd_addr, payload->bd_addr, 6);
                param.avrcp_addressed_player_changed.player_id = rsp->player_id;
                param.avrcp_addressed_player_changed.uid_counter = rsp->uid_counter;
                bt_mgr_event_post(BT_EVENT_AVRCP_ADDRESSED_PLAYER_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_UIDS_CHANGED:
            {
                memcpy(param.avrcp_uid_changed.bd_addr, payload->bd_addr, 6);
                param.avrcp_uid_changed.uid_counter = *(uint16_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_AVRCP_UID_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_APP_SETTING_ATTRS:
            {
                T_BT_AVRCP_RSP_LIST_APP_SETTING_ATTRS *rsp;

                rsp = (T_BT_AVRCP_RSP_LIST_APP_SETTING_ATTRS *)payload->msg_buf;
                memcpy(param.avrcp_app_setting_attrs_list_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_app_setting_attrs_list_rsp.state = rsp->state;
                param.avrcp_app_setting_attrs_list_rsp.num_of_attr = rsp->num_of_attr;
                param.avrcp_app_setting_attrs_list_rsp.p_attr_id = rsp->p_attr_id;
                bt_mgr_event_post(BT_EVENT_AVRCP_APP_SETTING_ATTRS_LIST_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_APP_SETTING_VALUES:
            {
                T_BT_AVRCP_RSP_LIST_APP_SETTING_VALUES *rsp;

                rsp = (T_BT_AVRCP_RSP_LIST_APP_SETTING_VALUES *)payload->msg_buf;
                memcpy(param.avrcp_app_setting_values_list_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_app_setting_values_list_rsp.state = rsp->state;
                param.avrcp_app_setting_values_list_rsp.num_of_value = rsp->num_of_value;
                param.avrcp_app_setting_values_list_rsp.p_value = rsp->p_value;
                bt_mgr_event_post(BT_EVENT_AVRCP_APP_SETTING_VALUES_LIST_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_CUR_APP_SETTING_VALUE:
            {
                T_BT_AVRCP_RSP_GET_APP_SETTING_VALUE *rsp;

                rsp = (T_BT_AVRCP_RSP_GET_APP_SETTING_VALUE *)payload->msg_buf;
                memcpy(param.avrcp_app_setting_get_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_app_setting_get_rsp.state = rsp->state;
                param.avrcp_app_setting_get_rsp.num_of_attr = rsp->num_of_attr;
                param.avrcp_app_setting_get_rsp.p_app_setting = rsp->p_app_setting;
                bt_mgr_event_post(BT_EVENT_AVRCP_APP_SETTING_GET_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_SET_ADDRESSED_PLAYER_RSP:
            {
                memcpy(param.avrcp_addressed_player_set_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_addressed_player_set_rsp.state = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_AVRCP_ADDRESSED_PLAYER_SET_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_PLAY_ITEM:
            {
                memcpy(param.avrcp_item_play_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_item_play_rsp.state = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_AVRCP_ITEM_PLAY_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_FOLDER_ITEMS:
            {
                T_BT_AVRCP_RSP_GET_FOLDER_ITEMS *rsp;

                rsp = (T_BT_AVRCP_RSP_GET_FOLDER_ITEMS *)payload->msg_buf;
                memcpy(param.avrcp_folder_items_get_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_folder_items_get_rsp.state = rsp->status_code;
                param.avrcp_folder_items_get_rsp.uid_counter = rsp->uid_counter;
                param.avrcp_folder_items_get_rsp.num_of_items = rsp->num_of_items;
                param.avrcp_folder_items_get_rsp.item_type = rsp->item_type;
                switch (rsp->item_type)
                {
                case BT_AVRCP_ITEM_TYPE_MEDIA_PLAYER:
                    param.avrcp_folder_items_get_rsp.u.p_media_player_items = rsp->u.p_media_player_items;
                    break;

                case BT_AVRCP_ITEM_TYPE_FOLDER:
                    param.avrcp_folder_items_get_rsp.u.p_folder_items = rsp->u.p_folder_items;
                    break;

                case BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT:
                    param.avrcp_folder_items_get_rsp.u.p_media_element_items = rsp->u.p_media_element_items;
                    break;

                default:
                    break;
                }

                bt_mgr_event_post(BT_EVENT_AVRCP_FOLDER_ITEMS_GET_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_ITEM_ATTR:
            {
                T_BT_AVRCP_RSP_GET_ITEM_ATTR *rsp;

                rsp = (T_BT_AVRCP_RSP_GET_ITEM_ATTR *)payload->msg_buf;
                memcpy(param.avrcp_item_attr_get_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_item_attr_get_rsp.state = rsp->state;
                param.avrcp_item_attr_get_rsp.num_of_attr = rsp->num_of_attr;
                param.avrcp_item_attr_get_rsp.p_attr = (T_BT_AVRCP_ITEM_ATTR *)rsp->attr;
                bt_mgr_event_post(BT_EVENT_AVRCP_ITEM_ATTR_GET_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_SEARCH:
            {
                T_BT_AVRCP_RSP_SEARCH *rsp;

                rsp = (T_BT_AVRCP_RSP_SEARCH *)payload->msg_buf;
                memcpy(param.avrcp_search_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_search_rsp.state = rsp->status_code;
                param.avrcp_search_rsp.uid_counter = rsp->uid_counter;
                param.avrcp_search_rsp.num_of_items = rsp->num_of_items;
                bt_mgr_event_post(BT_EVENT_AVRCP_SEARCH_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_SET_BROWSED_PLAYER:
            {
                T_BT_AVRCP_RSP_SET_BROWSED_PLAYER *rsp;

                rsp = (T_BT_AVRCP_RSP_SET_BROWSED_PLAYER *)payload->msg_buf;
                memcpy(param.avrcp_browsed_player_set_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_browsed_player_set_rsp.state = rsp->status_code;
                param.avrcp_browsed_player_set_rsp.uid_counter = rsp->uid_counter;
                param.avrcp_browsed_player_set_rsp.num_of_items = rsp->num_of_items;
                param.avrcp_browsed_player_set_rsp.character_set_id = rsp->character_set_id;
                param.avrcp_browsed_player_set_rsp.folder_depth = rsp->folder_depth;
                param.avrcp_browsed_player_set_rsp.p_folders = rsp->p_folders;
                bt_mgr_event_post(BT_EVENT_AVRCP_BROWSED_PLAYER_SET_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_CHANGE_PATH:
            {
                T_BT_AVRCP_RSP_CHANGE_PATH *rsp;

                rsp = (T_BT_AVRCP_RSP_CHANGE_PATH *)payload->msg_buf;
                memcpy(param.avrcp_path_change_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_path_change_rsp.state = rsp->status_code;
                param.avrcp_path_change_rsp.num_of_items = rsp->num_of_items;
                bt_mgr_event_post(BT_EVENT_AVRCP_PATH_CHANGE_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_SET_ABSOLUTE_VOLUME_RSP:
            {
                T_BT_AVRCP_RSP_SET_ABS_VOL *rsp;

                rsp = (T_BT_AVRCP_RSP_SET_ABS_VOL *)payload->msg_buf;
                memcpy(param.avrcp_absolute_volume_set_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_absolute_volume_set_rsp.state = rsp->state;
                param.avrcp_absolute_volume_set_rsp.volume = rsp->volume;
                bt_mgr_event_post(BT_EVENT_AVRCP_ABSOLUTE_VOLUME_SET_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_BROWSING_CONN_IND:
            {
                memcpy(param.avrcp_browsing_conn_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_BROWSING_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_BROWSING_CONN_CMPL:
            {
                memcpy(param.avrcp_browsing_conn_cmpl.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_BROWSING_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_BROWSING_DISCONN_CMPL:
            {
                memcpy(param.avrcp_browsing_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                param.avrcp_browsing_disconn_cmpl.cause = *(uint16_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_AVRCP_BROWSING_DISCONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_COVER_ART_CONN_CMPL:
            {
                memcpy(param.avrcp_cover_art_conn_cmpl.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_AVRCP_COVER_ART_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_COVER_ART_DISCONN_CMPL:
            {
                memcpy(param.avrcp_cover_art_conn_cmpl.bd_addr, payload->bd_addr, 6);
                param.avrcp_cover_art_disconn_cmpl.cause = *(uint16_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_AVRCP_COVER_ART_DISCONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_COVER_ART_DATA_IND:
            {
                T_BT_AVRCP_COVER_ART_DATA_IND *rsp;

                rsp = (T_BT_AVRCP_COVER_ART_DATA_IND *)payload->msg_buf;
                memcpy(param.avrcp_cover_art_data_ind.bd_addr, payload->bd_addr, 6);
                param.avrcp_cover_art_data_ind.p_data = rsp->p_data;
                param.avrcp_cover_art_data_ind.data_len = rsp->data_len;
                param.avrcp_cover_art_data_ind.data_end = rsp->data_end;
                bt_mgr_event_post(BT_EVENT_AVRCP_COVER_ART_DATA_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_VENDOR_CMD_IND:
            {
                T_BT_AVRCP_VENDOR_CMD *vendor_cmd;

                vendor_cmd = (T_BT_AVRCP_VENDOR_CMD *)payload->msg_buf;
                memcpy(param.avrcp_vendor_cmd_ind.bd_addr, payload->bd_addr, 6);
                param.avrcp_vendor_cmd_ind.ctype = vendor_cmd->ctype;
                param.avrcp_vendor_cmd_ind.company_id = vendor_cmd->company_id;
                param.avrcp_vendor_cmd_ind.p_cmd = vendor_cmd->p_cmd;
                param.avrcp_vendor_cmd_ind.cmd_len = vendor_cmd->cmd_len;
                bt_mgr_event_post(BT_EVENT_AVRCP_VENDOR_CMD_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_AVRCP_VENDOR_RSP:
            {
                T_BT_AVRCP_VENDOR_RSP *vendor_rsp;

                vendor_rsp = (T_BT_AVRCP_VENDOR_RSP *)payload->msg_buf;
                memcpy(param.avrcp_vendor_rsp.bd_addr, payload->bd_addr, 6);
                param.avrcp_vendor_rsp.response = vendor_rsp->response;
                param.avrcp_vendor_rsp.company_id = vendor_rsp->company_id;
                param.avrcp_vendor_rsp.p_rsp = vendor_rsp->p_rsp;
                param.avrcp_vendor_rsp.rsp_len = vendor_rsp->rsp_len;
                bt_mgr_event_post(BT_EVENT_AVRCP_VENDOR_RSP, &param, sizeof(param));
            }
            break;

        default:
            break;
        }
    }
}

void bt_mgr_handle_hfp_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM  param;
    T_BT_LINK        *link;

    link = bt_link_find(payload->bd_addr);
    if (link != NULL)
    {
        switch (msg)
        {
        case BT_MSG_HFP_CONN_IND:
            {
                memcpy(param.hfp_conn_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_CONN_CMPL:
            {
                bt_roleswap_handle_profile_conn(link->bd_addr, HFP_PROFILE_MASK, 0);

                memcpy(param.hfp_conn_cmpl.bd_addr, payload->bd_addr, 6);
                param.hfp_conn_cmpl.is_hfp = true;
                bt_mgr_event_post(BT_EVENT_HFP_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_CONN_FAIL:
            {
                memcpy(param.hfp_conn_fail.bd_addr, payload->bd_addr, 6);
                param.hfp_conn_fail.is_hfp = true;
                param.hfp_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_HFP_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_DISCONN_CMPL:
            {
                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;
                uint16_t cause = *(uint16_t *)(payload->msg_buf);

                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
                    {
                        gap_br_send_sco_disconn_req(link->bd_addr);
                    }

                    memcpy(param.hfp_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.hfp_disconn_cmpl.is_hfp = true;
                    param.hfp_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_HFP_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
                else
                {
                    memcpy(param.hfp_sniffing_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.hfp_sniffing_disconn_cmpl.is_hfp = true;
                    param.hfp_sniffing_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_HFP_SNIFFING_DISCONN_CMPL,
                                      &param, sizeof(param));
                }

                disconn_param.profile_mask = HFP_PROFILE_MASK;
                disconn_param.cause = cause;
                bt_roleswap_handle_profile_disconn(link->bd_addr, &disconn_param);
            }
            break;

        case BT_MSG_HFP_CALL_STATUS_CHANGED:
            {
                T_BT_HFP_CALL_STATUS_INFO *info = (T_BT_HFP_CALL_STATUS_INFO *)payload->msg_buf;

                memcpy(param.hfp_call_status.bd_addr, link->bd_addr, 6);
                param.hfp_call_status.prev_status = info->prev_call_status;
                param.hfp_call_status.curr_status = info->curr_call_status;
                bt_mgr_event_post(BT_EVENT_HFP_CALL_STATUS, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_SERVICE_STATUS_CHANGED:
            {
                uint8_t status = *(uint8_t *)(payload->msg_buf);

                memcpy(param.hfp_service_status.bd_addr, payload->bd_addr, 6);
                param.hfp_service_status.status = status;
                bt_mgr_event_post(BT_EVENT_HFP_SERVICE_STATUS, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_VND_AT_CMD_RSP:
            {
                memcpy(param.hfp_vendor_cmd_result.bd_addr, payload->bd_addr, 6);
                param.hfp_vendor_cmd_result.result = *(T_BT_HFP_CMD_RESULT *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_HFP_VENDOR_CMD_RESULT, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_CALLER_ID_IND:
            {
                T_BT_HFP_CALLER_ID_IND *caller_id = (T_BT_HFP_CALLER_ID_IND *)payload->msg_buf;

                memcpy(param.hfp_caller_id_ind.bd_addr, payload->bd_addr, 6);
                memcpy(param.hfp_caller_id_ind.number, caller_id->number, 20);
                param.hfp_caller_id_ind.type = caller_id->type;
                bt_mgr_event_post(BT_EVENT_HFP_CALLER_ID_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_RING_ALERT:
            {
                bool is_inband = *(bool *)(payload->msg_buf);

                memcpy(param.hfp_ring_alert.bd_addr, payload->bd_addr, 6);
                param.hfp_ring_alert.is_inband = is_inband;
                bt_mgr_event_post(BT_EVENT_HFP_RING_ALERT, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_SPK_GAIN_CHANGED:
            {
                memcpy(param.hfp_spk_volume_changed.bd_addr, payload->bd_addr, 6);
                param.hfp_spk_volume_changed.volume = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_HFP_SPK_VOLUME_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_MIC_GAIN_CHANGED:
            {
                memcpy(param.hfp_mic_volume_changed.bd_addr, payload->bd_addr, 6);
                param.hfp_mic_volume_changed.volume = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_HFP_MIC_VOLUME_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_NETWORK_OPERATOR_IND:
            {
                T_BT_HFP_NETWORK_OPERATOR_IND *network_operator;

                network_operator = (T_BT_HFP_NETWORK_OPERATOR_IND *)payload->msg_buf;
                memcpy(param.hfp_network_operator_ind.bd_addr, link->bd_addr, 6);
                param.hfp_network_operator_ind.format = network_operator->format;
                param.hfp_network_operator_ind.mode = network_operator->mode;
                memcpy(param.hfp_network_operator_ind.name, network_operator->name, 17);
                bt_mgr_event_post(BT_EVENT_HFP_NETWORK_OPERATOR_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_SUBSCRIBER_NUMBER_IND:
            {
                T_BT_HFP_SUBSCRIBER_NUMBER_IND *subscriber_num;

                subscriber_num = (T_BT_HFP_SUBSCRIBER_NUMBER_IND *)payload->msg_buf;
                memcpy(param.hfp_subscriber_number_ind.bd_addr, link->bd_addr, 6);
                memcpy(param.hfp_subscriber_number_ind.number, subscriber_num->number, 20);
                param.hfp_subscriber_number_ind.type = subscriber_num->type;
                param.hfp_subscriber_number_ind.service = subscriber_num->service;
                bt_mgr_event_post(BT_EVENT_HFP_SUBSCRIBER_NUMBER_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_CURRENT_CALL_LIST_IND:
            {
                T_BT_HFP_CLCC_IND *clcc_rsp = (T_BT_HFP_CLCC_IND *)payload->msg_buf;

                memcpy(param.hfp_current_call_list_ind.bd_addr, link->bd_addr, 6);
                param.hfp_current_call_list_ind.call_idx = clcc_rsp->call_idx;
                param.hfp_current_call_list_ind.dir_incoming = clcc_rsp->dir_incoming;
                param.hfp_current_call_list_ind.status = clcc_rsp->status;
                param.hfp_current_call_list_ind.mode = clcc_rsp->mode;
                param.hfp_current_call_list_ind.mpty = clcc_rsp->mpty;
                memcpy(param.hfp_current_call_list_ind.number, clcc_rsp->number, 20);
                param.hfp_current_call_list_ind.type = clcc_rsp->type;
                bt_mgr_event_post(BT_EVENT_HFP_CURRENT_CALL_LIST_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_CALL_WAITING_IND:
            {
                T_BT_HFP_CALL_WAITING_IND *call_waiting;

                call_waiting = (T_BT_HFP_CALL_WAITING_IND *)payload->msg_buf;
                memcpy(param.hfp_call_waiting_ind.bd_addr, link->bd_addr, 6);
                memcpy(param.hfp_call_waiting_ind.number, call_waiting->number, 20);
                param.hfp_call_waiting_ind.type = call_waiting->type;
                bt_mgr_event_post(BT_EVENT_HFP_CALL_WAITING_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_SIGNAL_IND:
            {
                uint8_t signal_status = *(uint8_t *)(payload->msg_buf);

                memcpy(param.hfp_signal_ind.bd_addr, link->bd_addr, 6);
                param.hfp_signal_ind.state = signal_status;
                bt_mgr_event_post(BT_EVENT_HFP_SIGNAL_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_ROAM_IND:
            {
                uint8_t roam_status = *(uint8_t *)(payload->msg_buf);

                memcpy(param.hfp_roam_ind.bd_addr, link->bd_addr, 6);
                param.hfp_roam_ind.state = roam_status;
                bt_mgr_event_post(BT_EVENT_HFP_ROAM_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_BATTERY_IND:
            {
                uint8_t batt_chg_status = *(uint8_t *)(payload->msg_buf);

                memcpy(param.hfp_battery_ind.bd_addr, link->bd_addr, 6);
                param.hfp_battery_ind.state = batt_chg_status;
                bt_mgr_event_post(BT_EVENT_HFP_BATTERY_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_SUPPORTED_FEATURES_IND:
            {
                uint16_t cpbs = *(uint16_t *)(payload->msg_buf);

                memcpy(param.hfp_supported_features_ind.bd_addr, link->bd_addr, 6);
                param.hfp_supported_features_ind.ag_bitmap = cpbs;
                bt_mgr_event_post(BT_EVENT_HFP_SUPPORTED_FEATURES_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_DIAL_WITH_NUMBER_RESULT:
            {
                param.hfp_dial_with_number_result.result = *(T_BT_HFP_CMD_RESULT *)payload->msg_buf;
                memcpy(param.hfp_dial_with_number_result.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_DIAL_WITH_NUMBER_RESULT, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_VENDOR_CMD:
            {
                memcpy(param.hfp_vendor_cmd.bd_addr, payload->bd_addr, 6);
                param.hfp_vendor_cmd.cmd = (uint8_t *)payload->msg_buf;
                param.hfp_vendor_cmd.len = strlen((const char *)payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_HFP_VENDOR_CMD, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION:
            {
                param.hfp_voice_recognition_activation.result = *(T_BT_HFP_CMD_RESULT *)payload->msg_buf;
                memcpy(param.hfp_voice_recognition_activation.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_VOICE_RECOGNITION_ACTIVATION, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION:
            {
                param.hfp_voice_recognition_deactivation.result = *(T_BT_HFP_CMD_RESULT *)payload->msg_buf;
                memcpy(param.hfp_voice_recognition_deactivation.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_VOICE_RECOGNITION_DEACTIVATION, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_CODEC_TYPE_SELECTED:
            {
                memcpy(param.hfp_codec_type_selected.bd_addr, link->bd_addr, 6);
                param.hfp_codec_type_selected.codec_type = *(T_BT_HFP_CODEC_TYPE *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_HFP_CODEC_TYPE_SELECTED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_CONN_IND:
            {
                memcpy(param.hfp_ag_conn_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_CONN_CMPL:
            {
                memcpy(param.hfp_ag_conn_cmpl.bd_addr, payload->bd_addr, 6);
                param.hfp_ag_conn_cmpl.is_hfp = true;
                bt_mgr_event_post(BT_EVENT_HFP_AG_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_DISCONN_CMPL:
            {
                T_BT_HFP_AG_DISCONN_INFO *p_info = (T_BT_HFP_AG_DISCONN_INFO *)payload->msg_buf;

                if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
                {
                    gap_br_send_sco_disconn_req(link->bd_addr);
                }
                memcpy(param.hfp_ag_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                param.hfp_ag_disconn_cmpl.is_hfp = true;
                param.hfp_ag_disconn_cmpl.cause = p_info->cause;
                bt_mgr_event_post(BT_EVENT_HFP_AG_DISCONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_CALL_STATUS_CHANGED:
            {
                T_BT_HFP_AG_CALL_STATUS_INFO *info = (T_BT_HFP_AG_CALL_STATUS_INFO *)payload->msg_buf;

                memcpy(param.hfp_ag_call_status_changed.bd_addr, link->bd_addr, 6);
                param.hfp_ag_call_status_changed.prev_status = info->prev_call_status;
                param.hfp_ag_call_status_changed.curr_status = info->curr_call_status;
                bt_mgr_event_post(BT_EVENT_HFP_AG_CALL_STATUS_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_MIC_VOLUME_CHANGED:
            {
                memcpy(param.hfp_ag_mic_volume_changed.bd_addr, link->bd_addr, 6);
                param.hfp_ag_mic_volume_changed.volume = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_HFP_AG_MIC_VOLUME_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_SPK_VOLUME_CHANGED:
            {
                memcpy(param.hfp_ag_spk_volume_changed.bd_addr, link->bd_addr, 6);
                param.hfp_ag_spk_volume_changed.volume = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_HFP_AG_SPK_VOLUME_CHANGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_INDICATORS_STATUS_REQ:
            {
                memcpy(param.hfp_ag_indicators_status_req.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_INDICATORS_STATUS_REQ, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_BATTERY_LEVEL:
            {
                memcpy(param.hfp_ag_battery_level.bd_addr, link->bd_addr, 6);
                param.hfp_ag_battery_level.battery_level = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_HFP_AG_BATTERY_LEVEL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_SUPPORTED_FEATURES:
            {
                uint16_t cpbs = *(uint16_t *)(payload->msg_buf);

                memcpy(param.hfp_ag_supported_features.bd_addr, link->bd_addr, 6);
                param.hfp_ag_supported_features.hf_bitmap = cpbs;
                bt_mgr_event_post(BT_EVENT_HFP_AG_SUPPORTED_FEATURES, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_INBAND_RINGING_REQ:
            {
                memcpy(param.hfp_ag_inband_ringing_req.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_INBAND_RINGING_REQ, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_CALL_ANSWER_REQ:
            {
                memcpy(param.hfp_ag_call_answer_req.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_CALL_ANSWER_REQ, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_CALL_TERMINATE_REQ:
            {
                memcpy(param.hfp_ag_call_terminate_req.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_CALL_TERMINATE_REQ, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_CURR_CALLS_LIST_QUERY:
            {
                memcpy(param.hfp_ag_curr_calls_list_query.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_CURR_CALLS_LIST_QUERY, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_MSG_DTMF_CODE:
            {
                T_BT_HFP_AG_CALL_DTMF_CODE *p_info = (T_BT_HFP_AG_CALL_DTMF_CODE *)payload->msg_buf;

                memcpy(param.hfp_ag_dtmf_code.bd_addr, payload->bd_addr, 6);
                param.hfp_ag_dtmf_code.dtmf_code = p_info->dtmf_code;
                bt_mgr_event_post(BT_EVENT_HFP_AG_DTMF_CODE, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_MSG_DIAL_WITH_NUMBER:
            {
                T_BT_HFP_AG_DIAL_WITH_NUMBER *p_info = (T_BT_HFP_AG_DIAL_WITH_NUMBER *)payload->msg_buf;

                memcpy(param.hfp_ag_dial_with_number.bd_addr, payload->bd_addr, 6);
                memcpy(param.hfp_ag_dial_with_number.number, p_info->number, 20);
                bt_mgr_event_post(BT_EVENT_HFP_AG_DIAL_WITH_NUMBER, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_MSG_DIAL_WITH_MEMORY:
            {
                T_BT_HFP_AG_DIAL_WITH_MEMORY *p_info = (T_BT_HFP_AG_DIAL_WITH_MEMORY *)payload->msg_buf;

                memcpy(param.hfp_ag_dial_with_memory.bd_addr, payload->bd_addr, 6);
                param.hfp_ag_dial_with_memory.memory_location = p_info->num;
                bt_mgr_event_post(BT_EVENT_HFP_AG_DIAL_WITH_MEMORY, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_MSG_DIAL_LAST_NUMBER:
            {
                memcpy(param.hfp_ag_dial_last_number.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_DIAL_LAST_NUMBER, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_3WAY_HELD_CALL_RELEASED:
            {
                memcpy(param.hfp_ag_3way_held_call_released.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_3WAY_HELD_CALL_RELEASED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_3WAY_ACTIVE_CALL_RELEASED:
            {
                memcpy(param.hfp_ag_3way_active_call_released.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_3WAY_ACTIVE_CALL_RELEASED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_3WAY_SWITCHED:
            {
                memcpy(param.hfp_ag_3way_switched.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_3WAY_SWITCHED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_3WAY_MERGED:
            {
                memcpy(param.hfp_ag_3way_merged.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_3WAY_MERGED, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_SUBSCRIBER_NUMBER_QUERY:
            {
                memcpy(param.hfp_ag_subscriber_number_query.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_SUBSCRIBER_NUMBER_QUERY, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_NETWORK_NAME_FORMAT_SET:
            {
                memcpy(param.hfp_ag_network_name_format_set.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_NETWORK_NAME_FORMAT_SET, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_CURR_OPERATOR_QUERY:
            {
                memcpy(param.hfp_ag_curr_operator_query.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_CURR_OPERATOR_QUERY, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_ENHANCED_SAFETY_STATUS:
            {
                memcpy(param.hfp_ag_enhanced_safety_status.bd_addr, link->bd_addr, 6);
                param.hfp_ag_enhanced_safety_status.status = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_HFP_AG_ENHANCED_SAFETY_STATUS, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_NREC_STATUS:
            {
                memcpy(param.hfp_ag_nrec_status.bd_addr, link->bd_addr, 6);
                param.hfp_ag_nrec_status.status = *(uint8_t *)payload->msg_buf;
                bt_mgr_event_post(BT_EVENT_HFP_AG_NREC_STATUS, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_VOICE_RECOGNITION_ACTIVATION:
            {
                memcpy(param.hfp_ag_voice_recognition_activation.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_VOICE_RECOGNITION_ACTIVATION, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_VOICE_RECOGNITION_DEACTIVATION:
            {
                memcpy(param.hfp_ag_voice_recognition_deactivation.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_VOICE_RECOGNITION_DEACTIVATION, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_VENDOR_CMD:
            {
                memcpy(param.hfp_ag_vendor_cmd.bd_addr, link->bd_addr, 6);
                param.hfp_ag_vendor_cmd.cmd = (uint8_t *)payload->msg_buf;
                param.hfp_ag_vendor_cmd.len = strlen((const char *)payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_HFP_AG_VENDOR_CMD, &param, sizeof(param));
            }
            break;

        case BT_MSG_HFP_AG_CODEC_TYPE_SELECTED:
            {
                T_BT_HFP_CODEC_TYPE codec_type = *(T_BT_HFP_CODEC_TYPE *)(payload->msg_buf);

                memcpy(param.hfp_ag_codec_type_selected.bd_addr, link->bd_addr, 6);
                param.hfp_ag_codec_type_selected.codec_type = codec_type;
                bt_mgr_event_post(BT_EVENT_HFP_AG_CODEC_TYPE_SELECTED, &param, sizeof(param));
            }
            break;

        default:
            break;
        }
    }
}

void bt_mgr_handle_hsp_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM  param;
    T_BT_LINK        *link;

    link = bt_link_find(payload->bd_addr);
    if (link != NULL)
    {
        switch (msg)
        {
        case BT_MSG_HSP_CONN_CMPL:
            {
                bt_roleswap_handle_profile_conn(link->bd_addr, HSP_PROFILE_MASK, 0);

                memcpy(param.hfp_conn_cmpl.bd_addr, payload->bd_addr, 6);
                param.hfp_conn_cmpl.is_hfp = false;
                bt_mgr_event_post(BT_EVENT_HFP_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HSP_CONN_FAIL:
            {
                memcpy(param.hfp_conn_fail.bd_addr, payload->bd_addr, 6);
                param.hfp_conn_fail.is_hfp = false;
                param.hfp_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_HFP_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HSP_DISCONN_CMPL:
            {
                uint16_t cause = *(uint16_t *)(payload->msg_buf);
                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;

                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
                    {
                        gap_br_send_sco_disconn_req(link->bd_addr);
                    }
                    memcpy(param.hfp_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.hfp_disconn_cmpl.is_hfp = false;
                    param.hfp_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_HFP_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
                else
                {
                    memcpy(param.hfp_sniffing_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.hfp_sniffing_disconn_cmpl.is_hfp = false;
                    param.hfp_sniffing_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_HFP_SNIFFING_DISCONN_CMPL,
                                      &param, sizeof(param));
                }

                disconn_param.profile_mask = HSP_PROFILE_MASK;
                disconn_param.cause = cause;
                bt_roleswap_handle_profile_disconn(link->bd_addr, &disconn_param);
            }
            break;

        case BT_MSG_HSP_AG_CONN_CMPL:
            {
                memcpy(param.hfp_ag_conn_cmpl.bd_addr, payload->bd_addr, 6);
                param.hfp_ag_conn_cmpl.is_hfp = false;
                bt_mgr_event_post(BT_EVENT_HFP_AG_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HSP_AG_DISCONN_CMPL:
            {
                T_BT_HFP_AG_DISCONN_INFO *p_info = (T_BT_HFP_AG_DISCONN_INFO *)payload->msg_buf;

                if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
                {
                    gap_br_send_sco_disconn_req(link->bd_addr);
                }
                memcpy(param.hfp_ag_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                param.hfp_ag_disconn_cmpl.is_hfp = false;
                param.hfp_ag_disconn_cmpl.cause = p_info->cause;
                bt_mgr_event_post(BT_EVENT_HFP_AG_DISCONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HSP_AG_INBAND_RINGING_REQ:
            {
                memcpy(param.hfp_ag_inband_ringing_req.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HFP_AG_INBAND_RINGING_REQ, &param, sizeof(param));
            }
            break;

        case BT_MSG_HSP_AG_BUTTON_PRESS:
            {
                if (link->sco_state == BT_LINK_SCO_STATE_DISCONNECTED)
                {
                    memcpy(param.hfp_ag_call_answer_req.bd_addr, payload->bd_addr, 6);
                    bt_mgr_event_post(BT_EVENT_HFP_AG_CALL_ANSWER_REQ, &param, sizeof(param));
                }
                else
                {
                    memcpy(param.hfp_ag_call_terminate_req.bd_addr, payload->bd_addr, 6);
                    bt_mgr_event_post(BT_EVENT_HFP_AG_CALL_TERMINATE_REQ, &param, sizeof(param));
                }
            }
            break;

        default:
            break;
        }
    }
}

void bt_mgr_handle_spp_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM  param;
    T_BT_LINK        *link;
    uint8_t           profile_param;

    link = bt_link_find(payload->bd_addr);
    if (link != NULL)
    {
        switch (msg)
        {
        case BT_MSG_SPP_CONN_IND:
            {
                T_BT_SPP_CONN_IND *p_ind;

                p_ind = (T_BT_SPP_CONN_IND *)payload->msg_buf;
                memcpy(param.spp_conn_ind.bd_addr, payload->bd_addr, 6);
                param.spp_conn_ind.local_server_chann = p_ind->local_server_chann;
                param.spp_conn_ind.frame_size = p_ind->frame_size;
                bt_mgr_event_post(BT_EVENT_SPP_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_SPP_CONN_CMPL:
            {
                T_BT_SPP_CONN_CMPL *p_cmpl;

                p_cmpl = (T_BT_SPP_CONN_CMPL *)payload->msg_buf;
                profile_param = p_cmpl->local_server_chann;

                memcpy(param.spp_conn_cmpl.bd_addr, payload->bd_addr, 6);
                param.spp_conn_cmpl.local_server_chann = p_cmpl->local_server_chann;
                param.spp_conn_cmpl.link_credit = p_cmpl->credit;
                param.spp_conn_cmpl.frame_size = p_cmpl->frame_size;
                bt_mgr_event_post(BT_EVENT_SPP_CONN_CMPL, &param, sizeof(param));

                bt_roleswap_handle_profile_conn(link->bd_addr, SPP_PROFILE_MASK, profile_param);
            }
            break;

        case BT_MSG_SPP_CONN_FAIL:
            {
                memcpy(param.spp_conn_fail.bd_addr, payload->bd_addr, 6);
                param.spp_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_SPP_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_SPP_DISCONN_CMPL:
            {
                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;
                T_BT_SPP_DISCONN_CMPL *p_info;

                p_info = (T_BT_SPP_DISCONN_CMPL *)payload->msg_buf;
                profile_param = p_info->local_server_chann;

                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    memcpy(param.spp_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.spp_disconn_cmpl.local_server_chann = p_info->local_server_chann;
                    param.spp_disconn_cmpl.cause = p_info->cause;
                    bt_mgr_event_post(BT_EVENT_SPP_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
                else
                {
                    memcpy(param.spp_sniffing_disconn_cmpl.bd_addr,
                           payload->bd_addr, 6);
                    param.spp_sniffing_disconn_cmpl.local_server_chann =
                        p_info->local_server_chann;
                    param.spp_sniffing_disconn_cmpl.cause = p_info->cause;
                    bt_mgr_event_post(BT_EVENT_SPP_SNIFFING_DISCONN_CMPL,
                                      &param, sizeof(param));
                }

                disconn_param.profile_mask = SPP_PROFILE_MASK;
                disconn_param.cause = p_info->cause;
                disconn_param.param = profile_param;
                bt_roleswap_handle_profile_disconn(link->bd_addr, &disconn_param);
            }
            break;

        case BT_MSG_SPP_DATA_IND:
            {
                T_BT_SPP_DATA_IND *p_ind;

                p_ind = (T_BT_SPP_DATA_IND *)payload->msg_buf;
                memcpy(param.spp_data_ind.bd_addr, payload->bd_addr, 6);
                param.spp_data_ind.local_server_chann = p_ind->local_server_chann;
                param.spp_data_ind.data = p_ind->p_data;
                param.spp_data_ind.len = p_ind->len;
                bt_mgr_event_post(BT_EVENT_SPP_DATA_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_SPP_DATA_RSP:
            {
                T_BT_SPP_DATA_RSP *p_rsp;

                p_rsp = (T_BT_SPP_DATA_RSP *)payload->msg_buf;
                memcpy(param.spp_data_rsp.bd_addr, payload->bd_addr, 6);
                param.spp_data_rsp.local_server_chann = p_rsp->local_server_chann;
                bt_mgr_event_post(BT_EVENT_SPP_DATA_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_SPP_CREDIT_INFO:
            {
                T_BT_SPP_CREDIT_INFO *p_info;

                p_info = (T_BT_SPP_CREDIT_INFO *)payload->msg_buf;
                memcpy(param.spp_credit_rcvd.bd_addr, payload->bd_addr, 6);
                param.spp_credit_rcvd.local_server_chann = p_info->local_server_chann;
                param.spp_credit_rcvd.link_credit = p_info->credit;
                bt_mgr_event_post(BT_EVENT_SPP_CREDIT_RCVD, &param, sizeof(param));
            }
            break;

        default:
            break;
        }
    }
}

void bt_mgr_handle_iap_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM  param;
    T_BT_LINK        *link;

    link = bt_link_find(payload->bd_addr);
    if (link != NULL)
    {
        switch (msg)
        {
        case BT_MSG_IAP_CONN_IND:
            {
                T_BT_IAP_CONN_IND *p_iap_conn_ind = (T_BT_IAP_CONN_IND *)payload->msg_buf;

                memcpy(param.iap_conn_ind.bd_addr, payload->bd_addr, 6);
                param.iap_conn_ind.frame_size = p_iap_conn_ind->rfc_frame_size;
                bt_mgr_event_post(BT_EVENT_IAP_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_CONN_CMPL:
            {
                T_BT_IAP_CONN_INFO *p_info = (T_BT_IAP_CONN_INFO *)payload->msg_buf;

                memcpy(param.iap_conn_cmpl.bd_addr, payload->bd_addr, 6);
                param.iap_conn_cmpl.frame_size = p_info->max_data_len;
                bt_mgr_event_post(BT_EVENT_IAP_CONN_CMPL, &param, sizeof(param));

                bt_roleswap_handle_profile_conn(link->bd_addr, IAP_PROFILE_MASK, 0);
            }
            break;

        case BT_MSG_IAP_CONN_FAIL:
            {
                memcpy(param.iap_conn_fail.bd_addr, payload->bd_addr, 6);
                param.iap_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_IAP_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_DISCONN_CMPL:
            {
                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;
                T_BT_IAP_DISCONN_INFO *p_info = (T_BT_IAP_DISCONN_INFO *)payload->msg_buf;

                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    memcpy(param.iap_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.iap_disconn_cmpl.cause = p_info->cause;
                    bt_mgr_event_post(BT_EVENT_IAP_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
                else
                {
                    memcpy(param.iap_sniffing_disconn_cmpl.bd_addr,
                           payload->bd_addr, 6);
                    param.iap_sniffing_disconn_cmpl.cause = p_info->cause;
                    bt_mgr_event_post(BT_EVENT_IAP_SNIFFING_DISCONN_CMPL,
                                      &param, sizeof(param));
                }

                disconn_param.profile_mask = IAP_PROFILE_MASK;
                disconn_param.cause = p_info->cause;
                bt_roleswap_handle_profile_disconn(link->bd_addr, &disconn_param);
            }
            break;

        case BT_MSG_IAP_START_IDENT_REQ:
            {
                memcpy(param.iap_identity_info_req.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_IAP_IDENTITY_INFO_REQ, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_AUTHEN_SUCCESS:
            {
                memcpy(param.iap_authen_cmpl.bd_addr, payload->bd_addr, 6);
                param.iap_authen_cmpl.result = true;
                bt_mgr_event_post(BT_EVENT_IAP_AUTHEN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_AUTHEN_FAIL:
            {
                memcpy(param.iap_authen_cmpl.bd_addr, payload->bd_addr, 6);
                param.iap_authen_cmpl.result = false;
                bt_mgr_event_post(BT_EVENT_IAP_AUTHEN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_DATA_IND:
            {
                T_BT_IAP_DATA_IND *p_iap_data_ind = (T_BT_IAP_DATA_IND *)payload->msg_buf;
                memcpy(param.iap_data_ind.bd_addr, payload->bd_addr, 6);
                param.iap_data_ind.eap_session_id = p_iap_data_ind->eap_session_id;
                param.iap_data_ind.dev_seq_num = p_iap_data_ind->dev_seq_num;
                param.iap_data_ind.p_data = p_iap_data_ind->p_data;
                param.iap_data_ind.len = p_iap_data_ind->len;
                bt_mgr_event_post(BT_EVENT_IAP_DATA_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_START_EAP_SESSION:
            {
                T_BT_IAP_START_EAP_SESSION *p_info = (T_BT_IAP_START_EAP_SESSION *)payload->msg_buf;
                memcpy(param.iap_data_session_open.bd_addr, payload->bd_addr, 6);
                param.iap_data_session_open.protocol_id = p_info->eap_id;
                param.iap_data_session_open.session_id = p_info->eap_session_id;
                bt_mgr_event_post(BT_EVENT_IAP_DATA_SESSION_OPEN, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_STOP_EAP_SESSION:
            {
                T_BT_IAP_STOP_EAP_SESSION *p_info = (T_BT_IAP_STOP_EAP_SESSION *)payload->msg_buf;
                memcpy(param.iap_data_session_close.bd_addr, payload->bd_addr, 6);
                param.iap_data_session_close.session_id = p_info->eap_session_id;
                bt_mgr_event_post(BT_EVENT_IAP_DATA_SESSION_CLOSE, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_EAP_SESSION_STATUS:
            {
                T_BT_IAP_EAP_SESSION_STATUS_INFO *p_info = (T_BT_IAP_EAP_SESSION_STATUS_INFO *)payload->msg_buf;
                memcpy(param.iap_data_session_status.bd_addr, payload->bd_addr, 6);
                param.iap_data_session_status.session_id = p_info->eap_session_id;
                param.iap_data_session_status.session_status = (T_BT_IAP_EAP_SESSION_STATUS)
                                                               p_info->eap_session_status;
                bt_mgr_event_post(BT_EVENT_IAP_DATA_SESSION_STATUS, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_DATA_TRANSMITTED:
            {
                T_BT_IAP_DATA_TRANSMITTED *p_info = (T_BT_IAP_DATA_TRANSMITTED *)payload->msg_buf;
                memcpy(param.iap_data_sent.bd_addr, payload->bd_addr, 6);
                param.iap_data_sent.session_id = p_info->eap_session_id;
                param.iap_data_sent.success = p_info->success;
                bt_mgr_event_post(BT_EVENT_IAP_DATA_SENT, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_RESET:
            {
                memcpy(param.iap_reset.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_IAP_RESET, &param, sizeof(param));
            }
            break;

        case BT_MSG_IAP_CTRL_MSG_IND:
            {
                T_BT_IAP_CTRL_MSG_IND *p_ind = (T_BT_IAP_CTRL_MSG_IND *)payload->msg_buf;
                memcpy(param.iap_ctrl_msg_ind.bd_addr, payload->bd_addr, 6);
                param.iap_ctrl_msg_ind.msg_id = p_ind->msg_id;
                param.iap_ctrl_msg_ind.param_len = p_ind->param_len;
                param.iap_ctrl_msg_ind.p_param = p_ind->p_param;
                bt_mgr_event_post(BT_EVENT_IAP_CTRL_MSG_IND, &param, sizeof(param));
            }
            break;

        default:
            break;
        }
    }
}

void bt_mgr_handle_pbap_msg(T_BT_MSG          msg,
                            T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM  param;
    T_BT_LINK        *link;

    link = bt_link_find(payload->bd_addr);

    switch (msg)
    {
    case BT_MSG_PBAP_CONN_CMPL:
        {
            if (link != NULL)
            {
                bt_roleswap_handle_profile_conn(link->bd_addr, PBAP_PROFILE_MASK, 0);
                memcpy(param.pbap_conn_cmpl.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_PBAP_CONN_CMPL, &param, sizeof(param));
            }
        }
        break;

    case BT_MSG_PBAP_CONN_FAIL:
        {
            memcpy(param.pbap_conn_fail.bd_addr, payload->bd_addr, 6);
            param.pbap_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_PBAP_CONN_FAIL, &param, sizeof(param));
        }
        break;

    case BT_MSG_PBAP_DISCONN_CMPL:
        {
            T_BT_PBAP_DISCONN_INFO *p_info = (T_BT_PBAP_DISCONN_INFO *)payload->msg_buf;

            if (link != NULL)
            {
                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;

                disconn_param.profile_mask = PBAP_PROFILE_MASK;
                disconn_param.cause = p_info->cause;
                bt_roleswap_handle_profile_disconn(link->bd_addr, &disconn_param);
            }

            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                memcpy(param.pbap_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                param.pbap_disconn_cmpl.cause = p_info->cause;
                bt_mgr_event_post(BT_EVENT_PBAP_DISCONN_CMPL,
                                  &param, sizeof(param));
            }
            else
            {
                memcpy(param.pbap_sniffing_disconn_cmpl.bd_addr,
                       payload->bd_addr, 6);
                param.pbap_sniffing_disconn_cmpl.cause = p_info->cause;
                bt_mgr_event_post(BT_EVENT_PBAP_SNIFFING_DISCONN_CMPL,
                                  &param, sizeof(param));
            }
        }
        break;

    case BT_MSG_PBAP_GET_PHONE_BOOK_CMPL:
        {
            T_BT_PBAP_GET_PHONE_BOOK_MSG_DATA *p_info = (T_BT_PBAP_GET_PHONE_BOOK_MSG_DATA *)payload->msg_buf;

            memcpy(param.pbap_get_phone_book_cmpl.bd_addr, payload->bd_addr, 6);
            param.pbap_get_phone_book_cmpl.p_data = p_info->data;
            param.pbap_get_phone_book_cmpl.data_len = p_info->data_len;
            param.pbap_get_phone_book_cmpl.pb_size = p_info->pb_size;
            param.pbap_get_phone_book_cmpl.new_missed_calls = p_info->new_missed_calls;
            param.pbap_get_phone_book_cmpl.data_end = p_info->data_end;
            bt_mgr_event_post(BT_EVENT_PBAP_GET_PHONE_BOOK_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_PBAP_GET_PHONE_BOOK_SIZE_CMPL:
        {
            T_BT_PBAP_GET_PHONE_BOOK_MSG_DATA *p_info = (T_BT_PBAP_GET_PHONE_BOOK_MSG_DATA *)payload->msg_buf;

            memcpy(param.pbap_get_phone_book_size_cmpl.bd_addr, payload->bd_addr, 6);
            param.pbap_get_phone_book_size_cmpl.pb_size = p_info->pb_size;
            param.pbap_get_phone_book_size_cmpl.new_missed_calls = p_info->new_missed_calls;
            bt_mgr_event_post(BT_EVENT_PBAP_GET_PHONE_BOOK_SIZE_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_PBAP_SET_PHONE_BOOK_CMPL:
        {
            T_BT_PBAP_SET_PHONE_BOOK_CMPL *p_info = (T_BT_PBAP_SET_PHONE_BOOK_CMPL *)payload->msg_buf;

            memcpy(param.pbap_set_phone_book_cmpl.bd_addr, payload->bd_addr, 6);
            param.pbap_set_phone_book_cmpl.result = p_info->result;
            param.pbap_set_phone_book_cmpl.path = p_info->path;
            bt_mgr_event_post(BT_EVENT_PBAP_SET_PHONE_BOOK_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_PBAP_CALLER_ID_NAME:
        {
            T_BT_PBAP_CALLER_ID_NAME *p_info = (T_BT_PBAP_CALLER_ID_NAME *)payload->msg_buf;

            memcpy(param.pbap_caller_id_name.bd_addr, payload->bd_addr, 6);
            param.pbap_caller_id_name.name_ptr = p_info->name_ptr;
            param.pbap_caller_id_name.name_len = p_info->name_len;
            bt_mgr_event_post(BT_EVENT_PBAP_CALLER_ID_NAME, &param, sizeof(param));
        }
        break;

    default:
        break;
    }
}

void bt_mgr_handle_hid_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_LINK *link;

    link = bt_link_find(payload->bd_addr);
    if (link != NULL)
    {
        T_BT_EVENT_PARAM param;

        switch (msg)
        {
        case BT_MSG_HID_DEVICE_CONN_IND:
            {
                memcpy(param.hid_device_conn_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HID_DEVICE_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_DEVICE_CONN_CMPL:
            {
                memcpy(param.hid_device_conn_cmpl.bd_addr, payload->bd_addr, 6);

                bt_roleswap_handle_profile_conn(link->bd_addr, HID_DEVICE_PROFILE_MASK, 0);

                bt_mgr_event_post(BT_EVENT_HID_DEVICE_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_DEVICE_CONN_FAIL:
            {
                memcpy(param.hid_device_conn_fail.bd_addr, payload->bd_addr, 6);
                param.hid_device_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_HID_DEVICE_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_DEVICE_DISCONN_CMPL:
            {
                uint16_t cause = *(uint16_t *)(payload->msg_buf);

                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;

                disconn_param.profile_mask = HID_DEVICE_PROFILE_MASK;
                disconn_param.cause = cause;
                bt_roleswap_handle_profile_disconn(link->bd_addr, &disconn_param);

                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    memcpy(param.hid_device_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.hid_device_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_HID_DEVICE_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
                else
                {
                    memcpy(param.hid_device_sniffing_disconn_cmpl.bd_addr,
                           payload->bd_addr, 6);
                    param.hid_device_sniffing_disconn_cmpl.cause = cause;
                    bt_mgr_event_post(BT_EVENT_HID_DEVICE_SNIFFING_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
            }
            break;

        case BT_MSG_HID_DEVICE_CONTROL_DATA_IND:
            {
                T_BT_HID_DEVICE_DATA_IND *rsp;

                rsp = (T_BT_HID_DEVICE_DATA_IND *)payload->msg_buf;
                memcpy(param.hid_device_control_data_ind.bd_addr, payload->bd_addr, 6);
                param.hid_device_control_data_ind.p_data = rsp->p_data;
                param.hid_device_control_data_ind.report_type = rsp->report_type;
                param.hid_device_control_data_ind.report_size = rsp->report_size;
                param.hid_device_control_data_ind.report_id = rsp->report_id;
                bt_mgr_event_post(BT_EVENT_HID_DEVICE_CONTROL_DATA_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_DEVICE_GET_REPORT_IND:
            {
                T_BT_HID_DEVICE_GET_REPORT_IND *rsp;

                rsp = (T_BT_HID_DEVICE_GET_REPORT_IND *)payload->msg_buf;
                memcpy(param.hid_device_get_report_ind.bd_addr, payload->bd_addr, 6);
                param.hid_device_get_report_ind.report_type = rsp->report_type;
                param.hid_device_get_report_ind.report_id = rsp->report_id;
                param.hid_device_get_report_ind.report_size = rsp->report_size;
                bt_mgr_event_post(BT_EVENT_HID_DEVICE_GET_REPORT_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_DEVICE_SET_REPORT_IND:
            {
                T_BT_HID_DEVICE_SET_REPORT_IND *rsp;

                rsp = (T_BT_HID_DEVICE_SET_REPORT_IND *)payload->msg_buf;
                memcpy(param.hid_device_set_report_ind.bd_addr, payload->bd_addr, 6);
                param.hid_device_set_report_ind.p_data = rsp->p_data;
                param.hid_device_set_report_ind.report_type = rsp->report_type;
                param.hid_device_set_report_ind.report_id = rsp->report_id;
                param.hid_device_set_report_ind.report_size = rsp->report_size;
                bt_mgr_event_post(BT_EVENT_HID_DEVICE_SET_REPORT_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_DEVICE_GET_PROTOCOL_IND:
            {
                T_BT_HID_DEVICE_GET_PROTOCOL_IND *rsp;

                rsp = (T_BT_HID_DEVICE_GET_PROTOCOL_IND *)payload->msg_buf;
                memcpy(param.hid_device_get_protocol_ind.bd_addr, payload->bd_addr, 6);
                param.hid_device_get_protocol_ind.proto_mode = rsp->proto_mode;
                bt_mgr_event_post(BT_EVENT_HID_DEVICE_GET_PROTOCOL_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_DEVICE_SET_PROTOCOL_IND:
            {
                T_BT_HID_DEVICE_SET_PROTOCOL_IND *rsp;

                rsp = (T_BT_HID_DEVICE_SET_PROTOCOL_IND *)payload->msg_buf;
                memcpy(param.hid_device_set_protocol_ind.bd_addr, payload->bd_addr, 6);
                param.hid_device_set_protocol_ind.proto_mode = rsp->proto_mode;
                bt_mgr_event_post(BT_EVENT_HID_DEVICE_SET_PROTOCOL_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_DEVICE_SET_IDLE_IND:
            {
                T_BT_HID_DEVICE_SET_IDLE_IND *rsp;

                rsp = (T_BT_HID_DEVICE_SET_IDLE_IND *)payload->msg_buf;
                memcpy(param.hid_device_set_idle_ind.bd_addr, payload->bd_addr, 6);
                param.hid_device_set_idle_ind.report_status = rsp->report_status;
                bt_mgr_event_post(BT_EVENT_HID_DEVICE_SET_IDLE_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_DEVICE_INTERRUPT_DATA_IND:
            {
                T_BT_HID_DEVICE_DATA_IND *rsp;

                rsp = (T_BT_HID_DEVICE_DATA_IND *)payload->msg_buf;
                memcpy(param.hid_device_interrupt_data_ind.bd_addr, payload->bd_addr, 6);
                param.hid_device_interrupt_data_ind.p_data = rsp->p_data;
                param.hid_device_interrupt_data_ind.report_type = rsp->report_type;
                param.hid_device_interrupt_data_ind.report_size = rsp->report_size;
                param.hid_device_interrupt_data_ind.report_id = rsp->report_id;
                bt_mgr_event_post(BT_EVENT_HID_DEVICE_INTERRUPT_DATA_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_HOST_CONN_IND:
            {
                memcpy(param.hid_host_conn_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_HID_HOST_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_HOST_CONN_CMPL:
            {
                memcpy(param.hid_host_conn_cmpl.bd_addr, payload->bd_addr, 6);

                bt_roleswap_handle_profile_conn(link->bd_addr, HID_HOST_PROFILE_MASK, 0);

                bt_mgr_event_post(BT_EVENT_HID_HOST_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_HOST_CONN_FAIL:
            {
                memcpy(param.hid_host_conn_fail.bd_addr, payload->bd_addr, 6);
                param.hid_host_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_HID_HOST_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_HOST_DISCONN_CMPL:
            {
                T_BT_HID_HOST_DISCONN_CMPL *rsp;
                rsp = (T_BT_HID_HOST_DISCONN_CMPL *)payload->msg_buf;

                T_ROLESWAP_PROFILE_DISCONN_PARAM disconn_param;

                disconn_param.profile_mask = HID_HOST_PROFILE_MASK;
                disconn_param.cause = rsp->cause;
                disconn_param.param = rsp->virtual_unplug;
                bt_roleswap_handle_profile_disconn(link->bd_addr, &disconn_param);

                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    memcpy(param.hid_host_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                    param.hid_host_disconn_cmpl.cause = rsp->cause;
                    param.hid_host_disconn_cmpl.virtual_unplug = rsp->virtual_unplug;
                    bt_mgr_event_post(BT_EVENT_HID_HOST_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
                else
                {
                    memcpy(param.hid_host_sniffing_disconn_cmpl.bd_addr,
                           payload->bd_addr, 6);
                    param.hid_host_sniffing_disconn_cmpl.cause = rsp->cause;
                    param.hid_host_sniffing_disconn_cmpl.virtual_unplug = rsp->virtual_unplug;
                    bt_mgr_event_post(BT_EVENT_HID_HOST_SNIFFING_DISCONN_CMPL,
                                      &param, sizeof(param));
                }
            }
            break;

        case BT_MSG_HID_HOST_GET_REPORT_RSP:
            {
                T_BT_HID_HOST_GET_REPORT_RSP *rsp;

                rsp = (T_BT_HID_HOST_GET_REPORT_RSP *)payload->msg_buf;
                memcpy(param.hid_host_get_report_rsp.bd_addr, payload->bd_addr, 6);
                param.hid_host_get_report_rsp.report_type = rsp->report_type;
                param.hid_host_get_report_rsp.report_id = rsp->report_id;
                param.hid_host_get_report_rsp.report_size = rsp->report_size;
                param.hid_host_get_report_rsp.p_data = rsp->p_data;
                bt_mgr_event_post(BT_EVENT_HID_HOST_GET_REPORT_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_HOST_SET_REPORT_RSP:
            {
                T_BT_HID_HOST_SET_REPORT_RSP *rsp;

                rsp = (T_BT_HID_HOST_SET_REPORT_RSP *)payload->msg_buf;
                memcpy(param.hid_host_set_report_rsp.bd_addr, payload->bd_addr, 6);
                param.hid_host_set_report_rsp.result_code = rsp->result_code;
                bt_mgr_event_post(BT_EVENT_HID_HOST_SET_REPORT_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_HOST_GET_PROTOCOL_RSP:
            {
                T_BT_HID_HOST_GET_PROTOCOL_RSP *rsp;

                rsp = (T_BT_HID_HOST_GET_PROTOCOL_RSP *)payload->msg_buf;
                memcpy(param.hid_host_get_protocol_rsp.bd_addr, payload->bd_addr, 6);
                param.hid_host_get_protocol_rsp.proto_mode = rsp->proto_mode;
                bt_mgr_event_post(BT_EVENT_HID_HOST_GET_PROTOCOL_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_HOST_SET_PROTOCOL_RSP:
            {
                T_BT_HID_HOST_SET_PROTOCOL_RSP *rsp;

                rsp = (T_BT_HID_HOST_SET_PROTOCOL_RSP *)payload->msg_buf;
                memcpy(param.hid_host_set_protocol_rsp.bd_addr, payload->bd_addr, 6);
                param.hid_host_set_protocol_rsp.result_code = rsp->result_code;
                bt_mgr_event_post(BT_EVENT_HID_HOST_SET_PROTOCOL_RSP, &param, sizeof(param));
            }
            break;

        case BT_MSG_HID_HOST_INTERRUPT_DATA_IND:
            {
                T_BT_HID_HOST_DATA_IND *rsp;

                rsp = (T_BT_HID_HOST_DATA_IND *)payload->msg_buf;
                memcpy(param.hid_host_interrupt_data_ind.bd_addr, payload->bd_addr, 6);
                param.hid_host_interrupt_data_ind.p_data = rsp->p_data;
                param.hid_host_interrupt_data_ind.report_type = rsp->report_type;
                param.hid_host_interrupt_data_ind.report_size = rsp->report_size;
                param.hid_host_interrupt_data_ind.report_id = rsp->report_id;
                bt_mgr_event_post(BT_EVENT_HID_HOST_INTERRUPT_DATA_IND, &param, sizeof(param));
            }
            break;

        default:
            break;
        }
    }
}

void bt_mgr_handle_map_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_LINK *link;

    link = bt_link_find(payload->bd_addr);
    if (link != NULL)
    {
        T_BT_EVENT_PARAM param;

        switch (msg)
        {
        case BT_MSG_MAP_MNS_CONN_IND:
            {
                memcpy(param.map_mns_conn_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_MAP_MNS_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_MNS_CONN_CMPL:
            {
                memcpy(param.map_mns_conn_cmpl.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_MAP_MNS_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_MNS_CONN_FAIL:
            {
                memcpy(param.map_mns_conn_fail.bd_addr, payload->bd_addr, 6);
                param.map_mns_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_MAP_MNS_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_MNS_DISCONN_CMPL:
            {
                uint16_t cause = *(uint16_t *)(payload->msg_buf);

                memcpy(param.map_mns_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                param.map_mns_disconn_cmpl.cause = cause;
                bt_mgr_event_post(BT_EVENT_MAP_MNS_DISCONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_MAS_CONN_CMPL:
            {
                uint16_t max_pkt_len = *(uint16_t *)(payload->msg_buf);

                memcpy(param.map_mas_conn_cmpl.bd_addr, payload->bd_addr, 6);
                param.map_mas_conn_cmpl.max_pkt_len = max_pkt_len;
                bt_mgr_event_post(BT_EVENT_MAP_MAS_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_MAS_CONN_FAIL:
            {
                memcpy(param.map_mas_conn_fail.bd_addr, payload->bd_addr, 6);
                param.map_mas_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_MAP_MAS_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_MAS_DISCONN_CMPL:
            {
                uint16_t cause = *(uint16_t *)(payload->msg_buf);

                memcpy(param.map_mas_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                param.map_mas_disconn_cmpl.cause = cause;
                bt_mgr_event_post(BT_EVENT_MAP_MAS_DISCONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_SET_FOLDER_CMPL:
            {
                bool result = *(bool *)(payload->msg_buf);

                memcpy(param.map_set_folder_cmpl.bd_addr, payload->bd_addr, 6);
                param.map_set_folder_cmpl.result = result;
                bt_mgr_event_post(BT_EVENT_MAP_SET_FOLDER_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_REG_NOTIF_CMPL:
            {
                bool result = *(bool *)(payload->msg_buf);

                memcpy(param.map_reg_notif_cmpl.bd_addr, payload->bd_addr, 6);
                param.map_reg_notif_cmpl.result = result;
                bt_mgr_event_post(BT_EVENT_MAP_REG_NOTIF_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_GET_FOLDER_LISTING_CMPL:
            {
                T_BT_MAP_FOLDER_LISTING_DATA_IND *p_info = (T_BT_MAP_FOLDER_LISTING_DATA_IND *)payload->msg_buf;

                memcpy(param.map_get_folder_listing_cmpl.bd_addr, payload->bd_addr, 6);
                param.map_get_folder_listing_cmpl.p_data = p_info->buf;
                param.map_get_folder_listing_cmpl.data_len = p_info->len;
                param.map_get_folder_listing_cmpl.folder_listing_size = p_info->folder_listing_size;
                param.map_get_folder_listing_cmpl.data_end = p_info->data_end;
                bt_mgr_event_post(BT_EVENT_MAP_GET_FOLDER_LISTING_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_GET_MSG_LISTING_CMPL:
            {
                T_BT_MAP_MSG_LISTING_DATA_IND *p_info = (T_BT_MAP_MSG_LISTING_DATA_IND *)payload->msg_buf;

                memcpy(param.map_get_msg_listing_cmpl.bd_addr, payload->bd_addr, 6);
                param.map_get_msg_listing_cmpl.p_data = p_info->buf;
                param.map_get_msg_listing_cmpl.data_len = p_info->len;
                param.map_get_msg_listing_cmpl.new_msg = p_info->new_msg;
                param.map_get_msg_listing_cmpl.msg_listing_size = p_info->msg_listing_size;
                param.map_get_msg_listing_cmpl.data_end = p_info->data_end;
                bt_mgr_event_post(BT_EVENT_MAP_GET_MSG_LISTING_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_GET_MSG_CMPL:
            {
                T_BT_MAP_MSG_DATA_IND *p_info = (T_BT_MAP_MSG_DATA_IND *)payload->msg_buf;

                memcpy(param.map_get_msg_cmpl.bd_addr, payload->bd_addr, 6);
                param.map_get_msg_cmpl.p_data = p_info->buf;
                param.map_get_msg_cmpl.data_len = p_info->len;
                param.map_get_msg_cmpl.data_end = p_info->data_end;
                bt_mgr_event_post(BT_EVENT_MAP_GET_MSG_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_PUSH_MSG_CMPL:
            {
                T_BT_MAP_PUSH_MSG_CMPL *p_info = (T_BT_MAP_PUSH_MSG_CMPL *)payload->msg_buf;

                memcpy(param.map_push_msg_cmpl.bd_addr, payload->bd_addr, 6);
                param.map_push_msg_cmpl.action = p_info->action;
                param.map_push_msg_cmpl.rsp_code = p_info->rsp_code;
                param.map_push_msg_cmpl.p_msg_handle = p_info->msg_handle;
                param.map_push_msg_cmpl.msg_handle_len = p_info->msg_handle_len;
                bt_mgr_event_post(BT_EVENT_MAP_PUSH_MSG_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_MAP_MSG_NOTIFICATION:
            {
                T_BT_MAP_MSG_REPORT_NOTIF *p_info = (T_BT_MAP_MSG_REPORT_NOTIF *)payload->msg_buf;

                memcpy(param.map_msg_notification.bd_addr, payload->bd_addr, 6);
                param.map_msg_notification.p_data = p_info->buf;
                param.map_msg_notification.data_len = p_info->len;
                param.map_msg_notification.data_end = p_info->data_end;
                bt_mgr_event_post(BT_EVENT_MAP_MSG_NOTIFICATION, &param, sizeof(param));
            }
            break;

        default:
            break;
        }
    }
}

void bt_mgr_handle_opp_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_LINK        *link;
    T_BT_EVENT_PARAM  param;

    link = bt_link_find(payload->bd_addr);
    if (link != NULL)
    {
        switch (msg)
        {
        case BT_MSG_OPP_CONN_IND:
            {
                memcpy(param.opp_conn_ind.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_OPP_CONN_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_OPP_CONN_CMPL:
            {
                memcpy(param.opp_conn_cmpl.bd_addr, payload->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_OPP_CONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_OPP_CONN_FAIL:
            {
                memcpy(param.opp_conn_fail.bd_addr, payload->bd_addr, 6);
                param.opp_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_OPP_CONN_FAIL, &param, sizeof(param));
            }
            break;

        case BT_MSG_OPP_DISCONN_CMPL:
            {
                memcpy(param.opp_disconn_cmpl.bd_addr, payload->bd_addr, 6);
                param.opp_disconn_cmpl.cause = *(uint16_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_OPP_DISCONN_CMPL, &param, sizeof(param));
            }
            break;

        case BT_MSG_OPP_DATA_HEADER_IND:
            {
                T_BT_OPP_DATA_HEADER_IND *p_info = (T_BT_OPP_DATA_HEADER_IND *)payload->msg_buf;

                memcpy(param.opp_data_header_ind.bd_addr, payload->bd_addr, 6);
                param.opp_data_header_ind.name = p_info->name;
                param.opp_data_header_ind.name_len = p_info->name_len;
                param.opp_data_header_ind.type = p_info->type;
                param.opp_data_header_ind.type_len = p_info->type_len;
                param.opp_data_header_ind.total_data_len = p_info->total_len;
                bt_mgr_event_post(BT_EVENT_OPP_DATA_HEADER_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_OPP_DATA_IND:
            {
                T_BT_OPP_DATA_IND *p_info = (T_BT_OPP_DATA_IND *)payload->msg_buf;

                memcpy(param.opp_data_ind.bd_addr, payload->bd_addr, 6);
                param.opp_data_ind.p_data = p_info->p_data;
                param.opp_data_ind.data_len = p_info->data_len;
                param.opp_data_ind.data_end = p_info->data_end;
                bt_mgr_event_post(BT_EVENT_OPP_DATA_IND, &param, sizeof(param));
            }
            break;

        case BT_MSG_OPP_DATA_RSP:
            {
                memcpy(param.opp_data_rsp.bd_addr, payload->bd_addr, 6);
                param.opp_data_rsp.cause = *(uint8_t *)(payload->msg_buf);
                bt_mgr_event_post(BT_EVENT_OPP_DATA_RSP, &param, sizeof(param));
            }
            break;

        default:
            break;
        }
    }
}

void bt_mgr_handle_rdtp_msg(T_BT_MSG          msg,
                            T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM  param;
    T_BT_LINK        *link;

    link = bt_link_find(payload->bd_addr);

    switch (msg)
    {
    case BT_MSG_RDTP_CONN_CMPL:
        if (link != NULL)
        {
            sys_ipc_publish(BT_IPC_TOPIC, BT_IPC_REMOTE_CONNECTED, NULL);

            bt_roleswap_handle_ctrl_conn();

            memcpy(param.remote_conn_cmpl.bd_addr, payload->bd_addr, 6);
            param.remote_conn_cmpl.max_pkt_len = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_REMOTE_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_RDTP_CONN_FAIL:
        if (link != NULL)
        {
            memcpy(param.remote_conn_fail.bd_addr, payload->bd_addr, 6);
            param.remote_conn_fail.cause = *(uint16_t *)(payload->msg_buf);
            bt_mgr_event_post(BT_EVENT_REMOTE_CONN_FAIL, &param, sizeof(param));
        }
        break;

    case BT_MSG_RDTP_DISCONN_CMPL:
        if (link != NULL)
        {
            uint16_t cause = *(uint16_t *)(payload->msg_buf);

            sys_ipc_publish(BT_IPC_TOPIC, BT_IPC_REMOTE_DISCONNECTED, NULL);

            param.remote_disconn_cmpl.cause = cause;
            memcpy(param.remote_disconn_cmpl.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_REMOTE_DISCONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_RDTP_DATA_IND:
        if (link != NULL)
        {
            T_BT_IPC_MSG msg;

            msg.bt_remote_data_ind.buf = ((T_BT_RDTP_DATA_IND *)payload->msg_buf)->buf;
            msg.bt_remote_data_ind.len = ((T_BT_RDTP_DATA_IND *)payload->msg_buf)->len;

            sys_ipc_publish(BT_IPC_TOPIC, BT_IPC_REMOTE_DATA_IND, &msg);
        }
        break;

    case BT_MSG_RDTP_DATA_RSP:
        {
            sys_ipc_publish(BT_IPC_TOPIC, BT_IPC_REMOTE_DATA_RSP, NULL);
        }
        break;

    default:
        break;
    }
}

void bt_mgr_handle_roleswap_msg(T_BT_MSG          msg,
                                T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM param;

    switch (msg)
    {
    case BT_MSG_ROLESWAP_ACL_STATUS:
        {
            T_BT_LINK *link;
            T_ROLESWAP_ACL_INFO *p_info = (T_ROLESWAP_ACL_INFO *)payload->msg_buf;

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_ACL_INFO;
            memcpy(param.remote_roleswap_status.u.acl.bd_addr, payload->bd_addr, 6);
            param.remote_roleswap_status.u.acl.role = (T_BT_LINK_ROLE)p_info->role;
            param.remote_roleswap_status.u.acl.authenticated = p_info->authen_state;
            if (p_info->encrypt_state == 1)
            {
                param.remote_roleswap_status.u.acl.encrypted = true;
            }
            else
            {
                param.remote_roleswap_status.u.acl.encrypted = false;
            }

            link = bt_link_find(p_info->bd_addr);
            if (link)
            {
                bt_pm_cback_register(link->bd_addr, bt_mgr_pm_cback);

                if (link->pm_state == BT_LINK_PM_STATE_ACTIVE)
                {
                    sys_timer_start(link->timer_sniff);
                }
            }

            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_A2DP_STATUS:
        {
            T_ROLESWAP_A2DP_INFO *p_info = (T_ROLESWAP_A2DP_INFO *)payload->msg_buf;

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_A2DP_INFO;
            memcpy(param.remote_roleswap_status.u.a2dp.bd_addr, payload->bd_addr, 6);
            if (p_info->state == BT_A2DP_STATE_STREAMING)
            {
                param.remote_roleswap_status.u.a2dp.streaming_fg = true;
            }
            else
            {
                param.remote_roleswap_status.u.a2dp.streaming_fg = false;
            }

            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_AVRCP_STATUS:
        {
            T_ROLESWAP_AVRCP_INFO *p_info = (T_ROLESWAP_AVRCP_INFO *)payload->msg_buf;

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_AVRCP_INFO;
            memcpy(param.remote_roleswap_status.u.avrcp.bd_addr, payload->bd_addr, 6);
            param.remote_roleswap_status.u.avrcp.play_status = p_info->play_status;
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_HFP_STATUS:
        {
            T_ROLESWAP_HFP_INFO *p_info = (T_ROLESWAP_HFP_INFO *)payload->msg_buf;

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_HFP_INFO;
            memcpy(param.remote_roleswap_status.u.hfp.bd_addr, payload->bd_addr, 6);
            param.remote_roleswap_status.u.hfp.call_status = p_info->curr_call_status;
            param.remote_roleswap_status.u.hfp.service_status = p_info->service_status;

            if (p_info->uuid == UUID_HANDSFREE)
            {
                param.remote_roleswap_status.u.hfp.is_hfp = true;
            }
            else
            {
                param.remote_roleswap_status.u.hfp.is_hfp = false;
            }
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_PBAP_STATUS:
        {
            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_PBAP_INFO;
            memcpy(param.remote_roleswap_status.u.pbap.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_HID_DEVICE_STATUS:
        {
            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_HID_DEVICE_INFO;
            memcpy(param.remote_roleswap_status.u.hid_device.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_HID_HOST_STATUS:
        {
            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_HID_HOST_INFO;
            memcpy(param.remote_roleswap_status.u.hid_host.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_SPP_STATUS:
        {
            T_ROLESWAP_SPP_INFO *spp_info = (T_ROLESWAP_SPP_INFO *)payload->msg_buf;

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_SPP_INFO;
            memcpy(param.remote_roleswap_status.u.spp.bd_addr, payload->bd_addr, 6);
            param.remote_roleswap_status.u.spp.remote_credit = spp_info->remote_credit;
            param.remote_roleswap_status.u.spp.frame_size = spp_info->frame_size;
            param.remote_roleswap_status.u.spp.local_server_chann = spp_info->local_server_chann;

            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_IAP_STATUS:
        {
            T_ROLESWAP_IAP_INFO *iap_info = (T_ROLESWAP_IAP_INFO *)payload->msg_buf;

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_IAP_INFO;
            memcpy(param.remote_roleswap_status.u.iap.bd_addr, payload->bd_addr, 6);
            param.remote_roleswap_status.u.iap.remote_credit = iap_info->remote_credit;
            param.remote_roleswap_status.u.iap.frame_size = iap_info->rfc_frame_size;

            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_SCO_STATUS:
        {
            T_ROLESWAP_SCO_INFO *sco_info = (T_ROLESWAP_SCO_INFO *)payload->msg_buf;

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_SCO_INFO;
            memcpy(param.remote_roleswap_status.u.sco.bd_addr, payload->bd_addr, 6);
            param.remote_roleswap_status.u.sco.handle = sco_info->handle;

            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_ADDR_STATUS:
        {
            T_GAP_HANDOVER_BUD_INFO *info = (T_GAP_HANDOVER_BUD_INFO *)payload->msg_buf;

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_BDADDR;
            param.remote_roleswap_status.u.bdaddr.curr_link_role = info->curr_link_role;
            memcpy(param.remote_roleswap_status.u.bdaddr.pre_bd_addr, info->pre_bd_addr, 6);
            memcpy(param.remote_roleswap_status.u.bdaddr.curr_bd_addr, info->curr_bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_CMPL:
        {
            T_REMOTE_SESSION_ROLE device_role;
            uint16_t cause = *(uint16_t *)(payload->msg_buf);

            sys_ipc_publish(BT_IPC_TOPIC, BT_IPC_REMOTE_SWAP_STOP, NULL);

            if (cause)
            {
                param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_FAIL;
            }
            else
            {
                device_role = remote_session_role_get();
                param.remote_roleswap_status.device_role = device_role;
                param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_SUCCESS;
            }
            param.remote_roleswap_status.cause = cause;
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_FLOW_STOP:
        {
            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_FLOW_STOP;
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_FLOW_GO:
        {
            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_FLOW_GO;
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_SYNC_CMPL:
        {
            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_DATA_SYNC_CMPL;
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_IDENT_CHANGE:
        {
            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_IDENT_CHANGE;
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_TERMINATED:
        {
            sys_ipc_publish(BT_IPC_TOPIC, BT_IPC_REMOTE_SWAP_STOP, NULL);

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_TERMINATED;
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_REQ:
        {
            T_BT_ROLESWAP_REQ *roleswap_req = (T_BT_ROLESWAP_REQ *)payload->msg_buf;

            param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_START_REQ;
            param.remote_roleswap_status.u.start_req.context = roleswap_req->context;
            bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
        }
        break;

    case BT_MSG_ROLESWAP_RSP:
        {
            T_BT_ROLESWAP_RSP *roleswap_rsp = (T_BT_ROLESWAP_RSP *)payload->msg_buf;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (roleswap_rsp->accept == true)
                {
                    sys_ipc_publish(BT_IPC_TOPIC, BT_IPC_REMOTE_SWAP_START, NULL);
                }

                param.remote_roleswap_status.status = BT_ROLESWAP_STATUS_START_RSP;
                param.remote_roleswap_status.u.start_rsp.accept = roleswap_rsp->accept;
                param.remote_roleswap_status.u.start_rsp.context = roleswap_rsp->context;
                bt_mgr_event_post(BT_EVENT_REMOTE_ROLESWAP_STATUS, &param, sizeof(param));
            }
        }
        break;

    case BT_MSG_ROLESWAP_START:
        {
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                sys_ipc_publish(BT_IPC_TOPIC, BT_IPC_REMOTE_SWAP_START, NULL);
            }
        }
        break;

    default:
        break;
    }
}

void bt_mgr_handle_sniffing_msg(T_BT_MSG          msg,
                                T_BT_MSG_PAYLOAD *payload)
{
    T_BT_LINK        *link;
    T_BT_EVENT_PARAM  param;

    link = bt_link_find(payload->bd_addr);

    switch (msg)
    {
    case BT_MSG_SNIFFING_ACL_CONN_CMPL:
        {
            memcpy(param.acl_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            param.acl_sniffing_conn_cmpl.cause = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_ACL_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_ACL_DISCONN_CMPL:
        {
            memcpy(param.acl_sniffing_disconn_cmpl.bd_addr, payload->bd_addr, 6);
            param.acl_sniffing_disconn_cmpl.cause = *(uint16_t *)payload->msg_buf;
            bt_mgr_event_post(BT_EVENT_ACL_SNIFFING_DISCONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_A2DP_CONN_CMPL:
        if (link != NULL)
        {
            T_BT_EVENT_PARAM_A2DP_SNIFFING_CONN_CMPL *cfg;
            T_ROLESWAP_A2DP_INFO *data = (T_ROLESWAP_A2DP_INFO *)payload->msg_buf;

            memcpy(param.a2dp_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            cfg = &param.a2dp_sniffing_conn_cmpl;

            param.a2dp_sniffing_conn_cmpl.codec_type = data->codec_type;
            param.a2dp_sniffing_conn_cmpl.role = data->role;
            if (param.a2dp_sniffing_conn_cmpl.codec_type == BT_A2DP_CODEC_TYPE_SBC)
            {
                cfg->codec_info.sbc.sampling_frequency = data->codec_info.sbc.sampling_frequency;
                cfg->codec_info.sbc.channel_mode = data->codec_info.sbc.channel_mode;
                cfg->codec_info.sbc.block_length = data->codec_info.sbc.block_length;
                cfg->codec_info.sbc.subbands = data->codec_info.sbc.subbands;
                cfg->codec_info.sbc.allocation_method = data->codec_info.sbc.allocation_method;
                cfg->codec_info.sbc.min_bitpool = data->codec_info.sbc.min_bitpool;
                cfg->codec_info.sbc.max_bitpool = data->codec_info.sbc.max_bitpool;
            }
            else if (param.a2dp_sniffing_conn_cmpl.codec_type == BT_A2DP_CODEC_TYPE_AAC)
            {
                cfg->codec_info.aac.object_type = data->codec_info.aac.object_type;
                cfg->codec_info.aac.sampling_frequency = data->codec_info.aac.sampling_frequency;
                cfg->codec_info.aac.channel_number = data->codec_info.aac.channel_number;
                cfg->codec_info.aac.vbr_supported = data->codec_info.aac.vbr_supported;
                cfg->codec_info.aac.bit_rate = data->codec_info.aac.bit_rate;
            }
            else if (param.a2dp_sniffing_conn_cmpl.codec_type == BT_A2DP_CODEC_TYPE_LDAC)
            {
                cfg->codec_info.ldac.sampling_frequency = data->codec_info.ldac.sampling_frequency;
                cfg->codec_info.ldac.channel_mode = data->codec_info.ldac.channel_mode;
            }
            else if (param.a2dp_sniffing_conn_cmpl.codec_type == BT_A2DP_CODEC_TYPE_LC3)
            {
                cfg->codec_info.lc3.sampling_frequency = data->codec_info.lc3.sampling_frequency;
                cfg->codec_info.lc3.channel_number = data->codec_info.lc3.channel_number;
                cfg->codec_info.lc3.frame_duration = data->codec_info.lc3.frame_duration;
                cfg->codec_info.lc3.frame_length = data->codec_info.lc3.frame_length;
            }
            else if (param.a2dp_sniffing_conn_cmpl.codec_type == BT_A2DP_CODEC_TYPE_LHDC)
            {
                cfg->codec_info.lhdc.sampling_frequency = data->codec_info.lhdc.sampling_frequency;
                cfg->codec_info.lhdc.min_bitrate = data->codec_info.lhdc.min_bitrate;
                cfg->codec_info.lhdc.max_bitrate = data->codec_info.lhdc.max_bitrate;
                cfg->codec_info.lhdc.bit_depth = data->codec_info.lhdc.bit_depth;
                cfg->codec_info.lhdc.version_number = data->codec_info.lhdc.version_number;
                cfg->codec_info.lhdc.low_latency = data->codec_info.lhdc.low_latency;
                cfg->codec_info.lhdc.meta = data->codec_info.lhdc.meta;
                cfg->codec_info.lhdc.jas = data->codec_info.lhdc.jas;
                cfg->codec_info.lhdc.ar = data->codec_info.lhdc.ar;
            }
            else
            {
                memcpy(cfg->codec_info.vendor.info, data->codec_info.vendor.info, 12);
            }

            bt_a2dp_active_link_set(payload->bd_addr);

            bt_mgr_event_post(BT_EVENT_A2DP_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_AVRCP_CONN_CMPL:
        if (link != NULL)
        {
            memcpy(param.avrcp_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_AVRCP_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_HFP_CONN_CMPL:
        if (link != NULL)
        {
            T_ROLESWAP_HFP_INFO *p_info = (T_ROLESWAP_HFP_INFO *)(payload->msg_buf);

            if (p_info->uuid == UUID_HANDSFREE)
            {
                param.hfp_sniffing_conn_cmpl.is_hfp = true;
            }
            else
            {
                param.hfp_sniffing_conn_cmpl.is_hfp = false;
            }
            memcpy(param.hfp_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_HFP_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_PBAP_CONN_CMPL:
        if (link != NULL)
        {
            memcpy(param.pbap_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_PBAP_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_HID_DEVICE_CONN_CMPL:
        if (link != NULL)
        {
            memcpy(param.hid_device_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_HID_DEVICE_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_HID_HOST_CONN_CMPL:
        if (link != NULL)
        {
            memcpy(param.hid_host_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_HID_HOST_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_SPP_CONN_CMPL:
        if (link != NULL)
        {
            T_ROLESWAP_SPP_INFO *p_info = (T_ROLESWAP_SPP_INFO *)(payload->msg_buf);

            param.spp_sniffing_conn_cmpl.local_server_chann = p_info->local_server_chann;
            memcpy(param.spp_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_SPP_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_IAP_CONN_CMPL:
        if (link != NULL)
        {
            memcpy(param.iap_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_IAP_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_SCO_CONN_CMPL:
        if (link != NULL)
        {
            T_ROLESWAP_SCO_INFO *p_info = (T_ROLESWAP_SCO_INFO *)(payload->msg_buf);

            memcpy(param.sco_sniffing_conn_cmpl.bd_addr, payload->bd_addr, 6);
            param.sco_sniffing_conn_cmpl.handle = p_info->handle;
            param.sco_sniffing_conn_cmpl.is_esco = (p_info->type == 0) ? false : true;
            param.sco_sniffing_conn_cmpl.air_mode = p_info->air_mode;
            if (param.sco_sniffing_conn_cmpl.is_esco)
            {
                param.sco_sniffing_conn_cmpl.tx_pkt_len = p_info->pkt_len;
                param.sco_sniffing_conn_cmpl.rx_pkt_len = p_info->pkt_len;
            }
            else
            {
                param.sco_sniffing_conn_cmpl.tx_pkt_len = 30;
                param.sco_sniffing_conn_cmpl.rx_pkt_len = 30;
            }

            bt_mgr_event_post(BT_EVENT_SCO_SNIFFING_CONN_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_A2DP_START_IND:
        if (link != NULL)
        {
            T_BT_EVENT_PARAM_A2DP_SNIFFING_START_IND *cfg;
            T_ROLESWAP_A2DP_INFO *data = (T_ROLESWAP_A2DP_INFO *)payload->msg_buf;

            memcpy(param.a2dp_sniffing_start_ind.bd_addr, payload->bd_addr, 6);
            cfg = &param.a2dp_sniffing_start_ind;

            param.a2dp_sniffing_start_ind.codec_type = data->codec_type;
            param.a2dp_sniffing_start_ind.role = data->role;
            if (param.a2dp_sniffing_start_ind.codec_type == BT_A2DP_CODEC_TYPE_SBC)
            {
                cfg->codec_info.sbc.sampling_frequency = data->codec_info.sbc.sampling_frequency;
                cfg->codec_info.sbc.channel_mode = data->codec_info.sbc.channel_mode;
                cfg->codec_info.sbc.block_length = data->codec_info.sbc.block_length;
                cfg->codec_info.sbc.subbands = data->codec_info.sbc.subbands;
                cfg->codec_info.sbc.allocation_method = data->codec_info.sbc.allocation_method;
                cfg->codec_info.sbc.min_bitpool = data->codec_info.sbc.min_bitpool;
                cfg->codec_info.sbc.max_bitpool = data->codec_info.sbc.max_bitpool;
            }
            else if (param.a2dp_sniffing_start_ind.codec_type == BT_A2DP_CODEC_TYPE_AAC)
            {
                cfg->codec_info.aac.object_type = data->codec_info.aac.object_type;
                cfg->codec_info.aac.sampling_frequency = data->codec_info.aac.sampling_frequency;
                cfg->codec_info.aac.channel_number = data->codec_info.aac.channel_number;
                cfg->codec_info.aac.vbr_supported = data->codec_info.aac.vbr_supported;
                cfg->codec_info.aac.bit_rate = data->codec_info.aac.bit_rate;
            }
            else if (param.a2dp_sniffing_start_ind.codec_type == BT_A2DP_CODEC_TYPE_LDAC)
            {
                cfg->codec_info.ldac.sampling_frequency = data->codec_info.ldac.sampling_frequency;
                cfg->codec_info.ldac.channel_mode = data->codec_info.ldac.channel_mode;
            }
            else if (param.a2dp_sniffing_start_ind.codec_type == BT_A2DP_CODEC_TYPE_LC3)
            {
                cfg->codec_info.lc3.sampling_frequency = data->codec_info.lc3.sampling_frequency;
                cfg->codec_info.lc3.channel_number = data->codec_info.lc3.channel_number;
                cfg->codec_info.lc3.frame_duration = data->codec_info.lc3.frame_duration;
                cfg->codec_info.lc3.frame_length = data->codec_info.lc3.frame_length;
            }
            else if (param.a2dp_sniffing_start_ind.codec_type == BT_A2DP_CODEC_TYPE_LHDC)
            {
                cfg->codec_info.lhdc.sampling_frequency = data->codec_info.lhdc.sampling_frequency;
                cfg->codec_info.lhdc.min_bitrate = data->codec_info.lhdc.min_bitrate;
                cfg->codec_info.lhdc.max_bitrate = data->codec_info.lhdc.max_bitrate;
                cfg->codec_info.lhdc.bit_depth = data->codec_info.lhdc.bit_depth;
                cfg->codec_info.lhdc.version_number = data->codec_info.lhdc.version_number;
                cfg->codec_info.lhdc.low_latency = data->codec_info.lhdc.low_latency;
                cfg->codec_info.lhdc.meta = data->codec_info.lhdc.meta;
                cfg->codec_info.lhdc.jas = data->codec_info.lhdc.jas;
                cfg->codec_info.lhdc.ar = data->codec_info.lhdc.ar;
            }
            else
            {
                memcpy(cfg->codec_info.vendor.info, data->codec_info.vendor.info, 12);
            }

            bt_mgr_event_post(BT_EVENT_A2DP_SNIFFING_START_IND, &param,
                              sizeof(T_BT_EVENT_PARAM_A2DP_SNIFFING_START_IND));
        }
        break;

    case BT_MSG_SNIFFING_A2DP_START:
        if (link != NULL)
        {
            T_ROLESWAP_RECOVERY_CONN_PARAM *conn_param;

            conn_param = (T_ROLESWAP_RECOVERY_CONN_PARAM *)(payload->msg_buf);
            param.a2dp_sniffing_started.cause = conn_param->cause;
            memcpy(param.a2dp_sniffing_started.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_A2DP_SNIFFING_STARTED, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_A2DP_CONFIG_CMPL:
        if (link != NULL)
        {
            memcpy(param.a2dp_sniffing_config_cmpl.bd_addr, payload->bd_addr, 6);
            param.a2dp_sniffing_config_cmpl.cause = *(uint16_t *)(payload->msg_buf);

            bt_mgr_event_post(BT_EVENT_A2DP_SNIFFING_CONFIG_CMPL, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_A2DP_STOP:
        if (link != NULL)
        {
            memcpy(param.a2dp_sniffing_stopped.bd_addr, payload->bd_addr, 6);
            param.a2dp_sniffing_stopped.cause = *(uint16_t *)(payload->msg_buf);

            bt_mgr_event_post(BT_EVENT_A2DP_SNIFFING_STOPPED, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_SCO_START:
        if (link != NULL)
        {
            T_ROLESWAP_RECOVERY_CONN_PARAM *conn_param;
            T_ROLESWAP_SCO_INFO *p_info;

            conn_param = (T_ROLESWAP_RECOVERY_CONN_PARAM *)(payload->msg_buf);
            p_info = (T_ROLESWAP_SCO_INFO *)(conn_param->param);

            param.sco_sniffing_started.cause = conn_param->cause;
            memcpy(param.sco_sniffing_started.bd_addr, payload->bd_addr, 6);

            if (p_info)
            {
                param.sco_sniffing_started.is_esco = (p_info->type == 0) ? false : true;
                param.sco_sniffing_started.air_mode = p_info->air_mode;
                param.sco_sniffing_started.tx_pkt_len = p_info->pkt_len;
                param.sco_sniffing_started.rx_pkt_len = p_info->pkt_len;
            }

            bt_mgr_event_post(BT_EVENT_SCO_SNIFFING_STARTED, &param, sizeof(param));
        }
        break;

    case BT_MSG_SNIFFING_SCO_STOP:
        if (link != NULL)
        {
            memcpy(param.sco_sniffing_stopped.bd_addr, payload->bd_addr, 6);
            param.sco_sniffing_stopped.cause = *(uint16_t *)(payload->msg_buf);

            bt_mgr_event_post(BT_EVENT_SCO_SNIFFING_STOPPED, &param, sizeof(param));
        }
        break;

    default:
        break;
    }
}

void bt_mgr_handle_vnd_msg(T_BT_MSG          msg,
                           T_BT_MSG_PAYLOAD *payload)
{
    T_BT_EVENT_PARAM param;

    switch (msg)
    {
    case BT_MSG_VND_PER_INFO:
        {
            uint8_t *p = payload->msg_buf;

            LE_STREAM_TO_UINT32(param.link_per_info.total_pkts, p);
            LE_STREAM_TO_UINT32(param.link_per_info.err_pkts, p);
            memcpy(param.link_per_info.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_LINK_PER_INFO, &param, sizeof(param));
        }
        break;

    case BT_MSG_VND_RSSI_INFO:
        {
            uint8_t *p = payload->msg_buf;

            LE_STREAM_TO_UINT8(param.link_rssi_info.rssi, p);
            memcpy(param.link_per_info.bd_addr, payload->bd_addr, 6);
            bt_mgr_event_post(BT_EVENT_LINK_RSSI_INFO, &param, sizeof(param));
        }
        break;

    case BT_MSG_VND_SET_TX_POWER_RSP:
        {
            T_GAP_SET_TX_POWER_RSP *p_info;
            T_BT_LINK *link;

            p_info = (T_GAP_SET_TX_POWER_RSP *)(payload->msg_buf);

            if (p_info->handle == 0xffff)
            {
                param.paging_tx_power_set_rsp.cause = p_info->cause;
                param.paging_tx_power_set_rsp.expected_tx_power_offset = p_info->expected_tx_power_offset;
                param.paging_tx_power_set_rsp.actual_tx_power_offset = p_info->actual_tx_power_offset;
                bt_mgr_event_post(BT_EVENT_PAGING_TX_POWER_SET_RSP, &param,
                                  sizeof(T_BT_EVENT_PARAM_PAGING_TX_POWER_SET_RSP));
            }
            else
            {
                link = bt_link_find_by_handle(p_info->handle);
                if (link)
                {
                    param.link_tx_power_set_rsp.cause = p_info->cause;
                    param.link_tx_power_set_rsp.expected_tx_power_offset = p_info->expected_tx_power_offset;
                    param.link_tx_power_set_rsp.actual_tx_power_offset = p_info->actual_tx_power_offset;
                    memcpy(param.link_tx_power_set_rsp.bd_addr, link->bd_addr, 6);
                    bt_mgr_event_post(BT_EVENT_LINK_TX_POWER_SET_RSP, &param,
                                      sizeof(T_BT_EVENT_PARAM_LINK_TX_POWER_SET_RSP));
                }
            }
        }
        break;

    case BT_MSG_VND_SET_RSSI_GOLDEN_RANGE_RSP:
        {
            T_BT_EVENT_PARAM param;
            T_GAP_SET_RSSI_GOLDEN_RANGE_RSP *p_info;
            T_BT_LINK *link;

            p_info = (T_GAP_SET_RSSI_GOLDEN_RANGE_RSP *)(payload->msg_buf);

            link = bt_link_find_by_handle(p_info->handle);
            if (link)
            {
                param.link_rssi_golden_range_set_rsp.cause = p_info->cause;
                param.link_rssi_golden_range_set_rsp.max_rssi = p_info->max_rssi;
                param.link_rssi_golden_range_set_rsp.min_rssi = p_info->min_rssi;
                memcpy(param.link_rssi_golden_range_set_rsp.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_LINK_RSSI_GOLDEN_RANGE_SET_RSP, &param,
                                  sizeof(T_BT_EVENT_PARAM_LINK_RSSI_GOLDEN_RANGE_SET_RSP));
            }
        }
        break;

    case BT_MSG_VND_SET_IDLE_ZONE_RSP:
        {
            T_BT_EVENT_PARAM param;
            T_GAP_SET_IDLE_ZONE_RSP *p_info;
            T_BT_LINK *link;

            p_info = (T_GAP_SET_IDLE_ZONE_RSP *)(payload->msg_buf);

            link = bt_link_find_by_handle(p_info->handle);
            if (link)
            {
                param.link_idle_zone_set_rsp.cause = p_info->cause;
                param.link_idle_zone_set_rsp.interval = p_info->interval;
                param.link_idle_zone_set_rsp.idle_slot = p_info->idle_slot;
                param.link_idle_zone_set_rsp.idle_skip = p_info->idle_skip;
                memcpy(param.link_idle_zone_set_rsp.bd_addr, link->bd_addr, 6);
                bt_mgr_event_post(BT_EVENT_LINK_IDLE_ZONE_SET_RSP, &param,
                                  sizeof(T_BT_EVENT_PARAM_LINK_IDLE_ZONE_SET_RSP));
            }
        }
        break;

    case BT_MSG_VND_TRAFFIC_QOS_RSP:
        {
            T_BT_EVENT_PARAM               param;
            T_GAP_VND_SET_TRAFFIC_QOS_RSP *info;
            uint8_t                        sub_cmd;

            info = (T_GAP_VND_SET_TRAFFIC_QOS_RSP *)(payload->msg_buf);
            param.traffic_qos_rsp.cause = info->cause;
            sub_cmd = info->param[0];
            if (sub_cmd == BT_VND_NEGOTIATE_TRAFFIC_QOS_CMD)
            {
                param.traffic_qos_rsp.negotiate = 1;
            }
            else if (sub_cmd == BT_VND_NON_NEGOTIATE_TRAFFIC_QOS_CMD)
            {
                param.traffic_qos_rsp.negotiate = 0;
            }
            bt_mgr_event_post(BT_EVENT_TRAFFIC_QOS_RSP, &param,
                              sizeof(T_BT_EVENT_PARAM_TRAFFIC_QOS_RSP));
        }
        break;

    case BT_MSG_VND_TRAFFIC_QOS_CMPL:
        {
            T_BT_EVENT_PARAM                param;
            T_GAP_VND_SET_TRAFFIC_QOS_CMPL *info;
            T_BT_LINK                      *link;
            uint16_t                        handle;
            uint8_t                        *p;

            info = (T_GAP_VND_SET_TRAFFIC_QOS_CMPL *)(payload->msg_buf);
            p = info->param;
            LE_STREAM_TO_UINT16(handle, p);
            link = bt_link_find_by_handle(handle);
            if (link != NULL)
            {
                memcpy(param.traffic_qos_cmpl.bd_addr, link->bd_addr, 6);
                param.traffic_qos_cmpl.cause = info->cause;
                if (info->subevt == BT_VND_NEGOTIATE_TRAFFIC_QOS_EVT)
                {
                    param.traffic_qos_cmpl.type = BT_NEGOTIATE_PERIODIC_TRAFFIC_QOS;
                    LE_STREAM_TO_UINT16(param.traffic_qos_cmpl.u.negotiate_periodic_param.interval, p);
                    LE_STREAM_TO_UINT8(param.traffic_qos_cmpl.u.negotiate_periodic_param.rsvd_slots, p);
                    LE_STREAM_TO_UINT8(param.traffic_qos_cmpl.enable, p);
                }
                else if (info->subevt == BT_VND_NON_NEGOTIATE_TRAFFIC_QOS_EVT)
                {
                    uint8_t type;

                    LE_STREAM_TO_UINT8(type, p);
                    if (type == 0)
                    {
                        param.traffic_qos_cmpl.type = BT_RANDOM_TRAFFIC_QOS;
                    }
                    if (type == 1)
                    {
                        param.traffic_qos_cmpl.type = BT_PERIODIC_SYNC_TRAFFIC_QOS;
                    }
                    else if (type == 2)
                    {
                        param.traffic_qos_cmpl.type = BT_NON_NEGOTIATE_PERIODIC_TRAFFIC_QOS;
                    }
                    LE_STREAM_TO_UINT8(param.traffic_qos_cmpl.enable, p);
                }
                bt_mgr_event_post(BT_EVENT_TRAFFIC_QOS_CMPL, &param,
                                  sizeof(T_BT_EVENT_PARAM_TRAFFIC_QOS_CMPL));
            }
        }
        break;

    default:
        break;
    }
}

bool bt_mgr_dispatch(T_BT_MSG  msg,
                     void     *buf)
{
    uint16_t          msg_group;
    T_BT_MSG_PAYLOAD *payload = buf;

    BTM_PRINT_TRACE1("bt_mgr_dispatch: msg 0x%04x", msg);

    msg_group = msg & 0xff00;
    switch (msg_group)
    {
    case BT_MSG_GROUP_GAP:
        bt_mgr_handle_gap_msg(msg, payload);
        break;

    case BT_MSG_GROUP_ACL:
        bt_mgr_handle_acl_msg(msg, payload);
        break;

    case BT_MSG_GROUP_SCO:
        bt_mgr_handle_sco_msg(msg, payload);
        break;

    case BT_MSG_GROUP_A2DP:
        bt_mgr_handle_a2dp_msg(msg, payload);
        break;

    case BT_MSG_GROUP_AVRCP:
        bt_mgr_handle_avrcp_msg(msg, payload);
        break;

    case BT_MSG_GROUP_HFP:
        bt_mgr_handle_hfp_msg(msg, payload);
        break;

    case BT_MSG_GROUP_HSP:
        bt_mgr_handle_hsp_msg(msg, payload);
        break;

    case BT_MSG_GROUP_SPP:
        bt_mgr_handle_spp_msg(msg, payload);
        break;

    case BT_MSG_GROUP_IAP:
        bt_mgr_handle_iap_msg(msg, payload);
        break;

    case BT_MSG_GROUP_PBAP:
        bt_mgr_handle_pbap_msg(msg, payload);
        break;

    case BT_MSG_GROUP_HID:
        bt_mgr_handle_hid_msg(msg, payload);
        break;

    case BT_MSG_GROUP_MAP:
        bt_mgr_handle_map_msg(msg, payload);
        break;

    case BT_MSG_GROUP_OPP:
        bt_mgr_handle_opp_msg(msg, payload);
        break;

    case BT_MSG_GROUP_RDTP:
        bt_mgr_handle_rdtp_msg(msg, payload);
        break;

    case BT_MSG_GROUP_ROLESWAP:
        bt_mgr_handle_roleswap_msg(msg, payload);
        break;

    case BT_MSG_GROUP_SNIFFING:
        bt_mgr_handle_sniffing_msg(msg, payload);
        break;

    case BT_MSG_GROUP_VND:
        bt_mgr_handle_vnd_msg(msg, payload);
        break;

    default:
        break;
    }

    return true;
}

void bt_mgr_relay_cback(uint16_t               event,
                        T_REMOTE_RELAY_STATUS  status,
                        void                  *buf,
                        uint16_t               len)
{
    switch (event)
    {
    case BT_REMOTE_MSG_ACL_DISCONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_A2DP_DISCONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_AVRCP_CONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_AVRCP_DISCONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_HFP_DISCONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_SPP_CONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_SPP_DISCONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_IAP_CONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_IAP_DISCONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_PBAP_CONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_PBAP_DISCONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_HID_CONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_HID_DISCONNECTED:
        {
        }
        break;

    case BT_REMOTE_MSG_ROLESWAP_INFO_XMIT:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
        {
            bt_roleswap_recv(buf, len);
        }
        break;

    default:
        break;
    }
}

bool bt_mgr_init(void)
{
    int32_t ret = 0;

#if CONFIG_REALTEK_REMOTE_CONTROL_SUPPORT
    btm_db.relay_handle = remote_relay_register(bt_mgr_relay_cback);
    if (btm_db.relay_handle == NULL)
    {
        ret = 1;
        goto fail_register_remote_relay;
    }
#endif

    if (bt_gap_init() == false)
    {
        ret = 2;
        goto fail_init_gap;
    }

    if (bt_sdp_init() == false)
    {
        ret = 3;
        goto fail_init_sdp;
    }

    if (bt_roleswap_init() == false)
    {
        ret = 4;
        goto fail_init_roleswap;
    }

    os_queue_init(&btm_db.link_list);
    os_queue_init(&btm_db.cback_list);

    return true;

fail_init_roleswap:
fail_init_sdp:
fail_init_gap:
#if CONFIG_REALTEK_REMOTE_CONTROL_SUPPORT
    remote_relay_unregister(btm_db.relay_handle);
fail_register_remote_relay:
#endif
    BTM_PRINT_ERROR1("bt_mgr_init: failed %d", -ret);
    return false;
}
