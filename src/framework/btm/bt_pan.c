/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_PAN_SUPPORT == 1)

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "mpa.h"
#include "bnep.h"
#include "btm.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_pan.h"

typedef enum t_bt_pan_state
{
    BT_PAN_STATE_DISCONNECTED  = 0x00,
    BT_PAN_STATE_CONNECTING    = 0x01,
    BT_PAN_STATE_CONNECTED     = 0x02,
    BT_PAN_STATE_DISCONNECTING = 0x03,
} T_BT_PAN_STATE;

typedef struct t_bt_pan_cback_item
{
    struct t_bt_pan_cback_item *next;
    T_BT_PAN_CBACK              cback;
} T_BT_PAN_CBACK_ITEM;

typedef struct t_bt_pan_link
{
    struct t_bt_pan_link *next;
    uint8_t               local_addr[6];
    uint8_t               remote_addr[6];
    uint16_t              cid;
    T_BT_PAN_STATE        state;
    uint8_t               src_role;
    uint8_t               dst_role;
    uint8_t               prev_src_role;
    uint8_t               prev_dst_role;
} T_BT_PAN_LINK;

typedef struct t_bt_pan
{
    T_OS_QUEUE cback_list;
    T_OS_QUEUE link_list;
} T_BT_PAN;

static uint16_t uuid[3] = {UUID_PANU, UUID_NAP, UUID_GN};

static T_BT_PAN *bt_pan = NULL;

T_BT_PAN_LINK *bt_pan_alloc_link(uint8_t bd_addr[6])
{
    T_BT_PAN_LINK *link;

    link = calloc(1, sizeof(T_BT_PAN_LINK));
    if (link != NULL)
    {
        link->state = BT_PAN_STATE_DISCONNECTED;
        memcpy(link->remote_addr, bd_addr, 6);
        os_queue_in(&bt_pan->link_list, link);
    }

    return link;
}

void bt_pan_free_link(T_BT_PAN_LINK *link)
{
    if (os_queue_delete(&bt_pan->link_list, link) == true)
    {
        free(link);
    }
}

T_BT_PAN_LINK *bt_pan_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_PAN_LINK *link;

    link = os_queue_peek(&bt_pan->link_list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->remote_addr, bd_addr, 6))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

T_BT_PAN_LINK *bt_pan_find_link_by_cid(uint16_t cid)
{
    T_BT_PAN_LINK *link;

    link = os_queue_peek(&bt_pan->link_list, 0);
    while (link != NULL)
    {
        if (link->cid == cid)
        {
            break;
        }

        link = link->next;
    }

    return link;
}


bool bt_pan_connect_req(uint8_t       local_addr[6],
                        uint8_t       remote_addr[6],
                        T_BT_PAN_ROLE src_role,
                        T_BT_PAN_ROLE dst_role)
{
    if (src_role == BT_PAN_ROLE_PANU ||
        dst_role == BT_PAN_ROLE_PANU)
    {
        T_BT_PAN_LINK *link;
        uint16_t       src_uuid;
        uint16_t       dst_uuid;

        src_uuid = uuid[src_role];
        dst_uuid = uuid[dst_role];

        link = bt_pan_find_link_by_addr(remote_addr);
        if (link == NULL)
        {
            link = bt_pan_alloc_link(remote_addr);
            if (link != NULL)
            {
                link->state    = BT_PAN_STATE_CONNECTING;
                link->src_role = src_role;
                link->dst_role = dst_role;
                memcpy(link->local_addr, local_addr, 6);

                bt_sniff_mode_exit(remote_addr, false);
                return bnep_connect_req(local_addr,
                                        remote_addr,
                                        src_uuid,
                                        dst_uuid);
            }
        }
        else
        {
            if (link->state == BT_PAN_STATE_CONNECTED)
            {
                link->prev_src_role = link->src_role;
                link->prev_dst_role = link->dst_role;
                link->src_role      = src_role;
                link->dst_role      = dst_role;

                bt_sniff_mode_exit(remote_addr, false);
                return bnep_connect_req(local_addr,
                                        remote_addr,
                                        src_uuid,
                                        dst_uuid);
            }
        }
    }

    return false;
}

bool bt_pan_connect_cfm(uint8_t local_addr[6],
                        uint8_t remote_addr[6],
                        bool    accept)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_addr(remote_addr);
    if (link != NULL)
    {
        if (accept)
        {
            memcpy(link->local_addr, local_addr, 6);
        }
        else
        {
            bt_pan_free_link(link);
        }

        bt_sniff_mode_exit(remote_addr, false);
        return bnep_connect_cfm(local_addr, remote_addr, accept);
    }

    return bnep_connect_cfm(local_addr, remote_addr, false);
}

bool bt_pan_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->state == BT_PAN_STATE_CONNECTED)
        {
            bt_sniff_mode_exit(bd_addr, false);
            link->state = BT_PAN_STATE_DISCONNECTING;
            return bnep_disconnect_req(link->remote_addr);
        }
    }

    return false;
}

bool bt_pan_setup_connection_rsp(uint8_t  bd_addr[6],
                                 uint16_t rsp_msg)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bnep_setup_connection_rsp(bd_addr, rsp_msg);
    }

    return false;
}

bool bt_pan_filter_net_type_set(uint8_t   bd_addr[6],
                                uint16_t  filter_num,
                                uint16_t *start_array,
                                uint16_t *end_array)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bnep_filter_net_type_set(bd_addr,
                                        filter_num,
                                        start_array,
                                        end_array);
    }

    return false;
}

bool bt_pan_filter_multi_addr_set(uint8_t  bd_addr[6],
                                  uint16_t filter_num,
                                  uint8_t  start_array[][6],
                                  uint8_t  end_array[][6])
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bnep_filter_multi_addr_set(bd_addr,
                                          filter_num,
                                          start_array,
                                          end_array);
    }

    return false;
}

bool bt_pan_send(uint8_t   bd_addr[6],
                 uint8_t  *buf,
                 uint16_t  len)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bnep_send(bd_addr, buf, len);
    }

    return false;
}

bool bt_pan_event_post(T_BT_PAN_EVENT       event_type,
                       T_BT_PAN_EVENT_PARAM param)
{
    T_BT_PAN_CBACK_ITEM *item;

    PROFILE_PRINT_INFO1("bt_pan_event_post: event_type 0x%02x", event_type);

    item = (T_BT_PAN_CBACK_ITEM *)bt_pan->cback_list.p_first;
    while (item != NULL)
    {
        item->cback(event_type, &param, sizeof(param));
        item = item->next;
    }

    return true;
}

static void bt_pan_handle_conn_ind(uint16_t cid,
                                   uint8_t  bd_addr[6])
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link == NULL)
    {
        link = bt_pan_alloc_link(bd_addr);
        if (link != NULL)
        {
            T_BT_PAN_EVENT_PARAM param;

            link->state = BT_PAN_STATE_CONNECTING;
            link->cid = cid;
            memcpy(param.pan_conn_ind.bd_addr, link->remote_addr, 6);
            bt_pan_event_post(BT_PAN_EVENT_CONN_IND, param);
        }
        else
        {
            bnep_connect_cfm(NULL, bd_addr, false);
        }
    }
    else
    {
        bnep_connect_cfm(NULL, bd_addr, false);
    }
}

static void bt_pan_handle_conn_rsp(T_MPA_L2C_CONN_RSP *rsp)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_addr(rsp->bd_addr);
    if (link != NULL)
    {
        link->cid = rsp->cid;
    }
}

static void bt_pan_handle_conn_fail(uint16_t cid,
                                    uint16_t cause)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        memcpy(param.pan_conn_fail.bd_addr, link->remote_addr, 6);
        param.pan_conn_fail.cause = cause;
        bt_pan_free_link(link);
        bt_pan_event_post(BT_PAN_EVENT_CONN_FAIL, param);
    }
}

static void bt_pan_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *info)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_addr(info->bd_addr);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        link->state = BT_PAN_STATE_CONNECTED;
        memcpy(param.pan_conn_cmpl.bd_addr, info->bd_addr, 6);
        bt_pan_event_post(BT_PAN_EVENT_CONN_CMPL, param);
    }
}

static void bt_pan_handle_disconn_cmpl(uint16_t cid,
                                       uint16_t cause)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        memcpy(param.pan_disconn_cmpl.bd_addr, link->remote_addr, 6);
        param.pan_disconn_cmpl.cause = cause;
        bt_pan_free_link(link);
        bt_pan_event_post(BT_PAN_EVENT_DISCONN_CMPL, param);
    }
}

static void bt_pan_handle_setup_conn_ind(uint16_t               cid,
                                         T_BNEP_SETUP_CONN_IND *ind)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        memcpy(param.pan_setup_conn_ind.bd_addr, link->remote_addr, 6);
        param.pan_setup_conn_ind.dst_uuid = ind->dst_uuid;
        param.pan_setup_conn_ind.src_uuid = ind->src_uuid;
        bt_pan_event_post(BT_PAN_EVENT_SETUP_CONN_IND, param);
    }
}

static void bt_pan_handle_setup_conn_rsp(uint16_t               cid,
                                         T_BNEP_SETUP_CONN_RSP *rsp)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        if (rsp->rsp_msg)
        {
            link->src_role = link->prev_src_role;
            link->dst_role = link->prev_dst_role;
        }

        memcpy(param.pan_setup_conn_rsp.bd_addr, link->remote_addr, 6);
        param.pan_setup_conn_rsp.rsp_msg = rsp->rsp_msg;
        bt_pan_event_post(BT_PAN_EVENT_SETUP_CONN_RSP, param);
    }
}

static void bt_pan_handle_filter_net_type_set(uint16_t                        cid,
                                              T_BNEP_FILTER_NET_TYPE_SET_IND *ind)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        memcpy(param.pan_filter_net_type_set_ind.bd_addr, link->remote_addr, 6);
        param.pan_filter_net_type_set_ind.filter_num = ind->filter_num;
        param.pan_filter_net_type_set_ind.filter_start = ind->filter_start;
        param.pan_filter_net_type_set_ind.filter_end = ind->filter_end;
        bt_pan_event_post(BT_PAN_EVENT_FILTER_NET_TYPE_SET_IND, param);
    }
}

static void bt_pan_handle_filter_net_type_set_rsp(uint16_t                        cid,
                                                  T_BNEP_FILTER_NET_TYPE_SET_RSP *rsp)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        memcpy(param.pan_filter_net_type_set_rsp.bd_addr, link->remote_addr, 6);
        param.pan_filter_net_type_set_rsp.rsp_msg = rsp->rsp_msg;
        bt_pan_event_post(BT_PAN_EVENT_FILTER_NET_TYPE_SET_RSP, param);
    }
}

static void bt_pan_handle_filter_multi_addr_set(uint16_t                          cid,
                                                T_BNEP_FILTER_MULTI_ADDR_SET_IND *ind)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        memcpy(param.pan_filter_multi_addr_set_ind.bd_addr, link->remote_addr, 6);
        param.pan_filter_multi_addr_set_ind.filter_num = ind->filter_num;
        param.pan_filter_multi_addr_set_ind.filter_start = ind->filter_start;
        param.pan_filter_multi_addr_set_ind.filter_end = ind->filter_end;
        bt_pan_event_post(BT_PAN_EVENT_FILTER_MULTI_ADDR_SET_IND, param);
    }
}

static void bt_pan_handle_filter_multi_addr_set_rsp(uint16_t                          cid,
                                                    T_BNEP_FILTER_MULTI_ADDR_SET_RSP *rsp)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        memcpy(param.pan_filter_multi_addr_set_rsp.bd_addr, link->remote_addr, 6);
        param.pan_filter_multi_addr_set_rsp.rsp_msg = rsp->rsp_msg;
        bt_pan_event_post(BT_PAN_EVENT_FILTER_MULTI_ADDR_SET_RSP, param);
    }
}

static void bt_pan_handle_ethernet_packet_ind(uint16_t                    cid,
                                              T_BNEP_ETHERNET_PACKET_IND *ind)
{
    T_BT_PAN_LINK *link;

    link = bt_pan_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_PAN_EVENT_PARAM param;

        memcpy(param.pan_ethernet_packet_ind.bd_addr, link->remote_addr, 6);
        param.pan_ethernet_packet_ind.buf = ind->buf;
        param.pan_ethernet_packet_ind.len = ind->len;
        bt_pan_event_post(BT_PAN_EVENT_ETHERNET_PACKET_IND, param);
    }
}

void bt_pan_cback(uint16_t         cid,
                  T_BNEP_MSG_TYPE  msg_type,
                  void            *msg_buf)
{
    if (msg_type != BNEP_MSG_ETHERNET_PACKET_IND)
    {
        PROFILE_PRINT_INFO2("bt_pan_cback: cid 0x%04x, msg_type 0x%04x", cid, msg_type);
    }

    switch (msg_type)
    {
    case BNEP_MSG_CONN_IND:
        bt_pan_handle_conn_ind(cid, (uint8_t *)msg_buf);
        break;

    case BNEP_MSG_CONN_RSP:
        {
            T_MPA_L2C_CONN_RSP *rsp = (T_MPA_L2C_CONN_RSP *)msg_buf;
            bt_pan_handle_conn_rsp(rsp);
        }
        break;

    case BNEP_MSG_CONN_FAIL:
        {
            uint16_t cause = *((uint16_t *)msg_buf);
            bt_pan_handle_conn_fail(cid, cause);
        }
        break;

    case BNEP_MSG_CONN_CMPL:
        {
            T_MPA_L2C_CONN_CMPL_INFO *info = (T_MPA_L2C_CONN_CMPL_INFO *)msg_buf;
            bt_pan_handle_conn_cmpl(info);
        }
        break;

    case BNEP_MSG_DISCONN_CMPL:
        {
            uint16_t cause = *((uint16_t *)msg_buf);
            bt_pan_handle_disconn_cmpl(cid, cause);
        }
        break;

    case BNEP_MSG_SETUP_CONN_IND:
        {
            T_BNEP_SETUP_CONN_IND *ind = (T_BNEP_SETUP_CONN_IND *)msg_buf;
            bt_pan_handle_setup_conn_ind(cid, ind);
        }
        break;

    case BNEP_MSG_SETUP_CONN_RSP:
        {
            T_BNEP_SETUP_CONN_RSP *rsp = (T_BNEP_SETUP_CONN_RSP *)msg_buf;
            bt_pan_handle_setup_conn_rsp(cid, rsp);
        }
        break;

    case BNEP_MSG_FILTER_NET_TYPE_SET_IND:
        {
            T_BNEP_FILTER_NET_TYPE_SET_IND *ind = (T_BNEP_FILTER_NET_TYPE_SET_IND *)msg_buf;
            bt_pan_handle_filter_net_type_set(cid, ind);
        }
        break;

    case BNEP_MSG_FILTER_NET_TYPE_SET_RSP:
        {
            T_BNEP_FILTER_NET_TYPE_SET_RSP *rsp = (T_BNEP_FILTER_NET_TYPE_SET_RSP *)msg_buf;
            bt_pan_handle_filter_net_type_set_rsp(cid, rsp);
        }
        break;

    case BNEP_MSG_FILTER_MULTI_ADDR_SET_IND:
        {
            T_BNEP_FILTER_MULTI_ADDR_SET_IND *ind = (T_BNEP_FILTER_MULTI_ADDR_SET_IND *)msg_buf;
            bt_pan_handle_filter_multi_addr_set(cid, ind);
        }
        break;

    case BNEP_MSG_FILTER_MULTI_ADDR_SET_RSP:
        {
            T_BNEP_FILTER_MULTI_ADDR_SET_RSP *rsp = (T_BNEP_FILTER_MULTI_ADDR_SET_RSP *)msg_buf;
            bt_pan_handle_filter_multi_addr_set_rsp(cid, rsp);
        }
        break;

    case BNEP_MSG_ETHERNET_PACKET_IND:
        {
            T_BNEP_ETHERNET_PACKET_IND *ind = (T_BNEP_ETHERNET_PACKET_IND *)msg_buf;
            bt_pan_handle_ethernet_packet_ind(cid, ind);
        }
        break;

    default:
        break;
    }
}

bool bt_pan_cback_register(T_BT_PAN_CBACK cback)
{
    T_BT_PAN_CBACK_ITEM *item;

    item = (T_BT_PAN_CBACK_ITEM *)bt_pan->cback_list.p_first;
    while (item != NULL)
    {
        if (item->cback == cback)
        {
            return true;
        }

        item = item->next;
    }

    item = malloc(sizeof(T_BT_PAN_CBACK_ITEM));
    if (item != NULL)
    {
        item->cback = cback;
        os_queue_in(&bt_pan->cback_list, item);
        return true;
    }

    return false;
}

bool bt_pan_cback_unregister(T_BT_PAN_CBACK cback)
{
    T_BT_PAN_CBACK_ITEM *item;
    bool                 ret = false;

    item = (T_BT_PAN_CBACK_ITEM *)bt_pan->cback_list.p_first;
    while (item != NULL)
    {
        if (item->cback == cback)
        {
            os_queue_delete(&bt_pan->cback_list, item);
            free(item);
            ret = true;
            break;
        }

        item = item->next;
    }

    return ret;
}

bool bt_pan_init(void)
{
    int32_t ret = 0;

    bt_pan = calloc(1, sizeof(T_BT_PAN));
    if (bt_pan == NULL)
    {
        ret = 1;
        goto fail_alloc_pan;
    }

    os_queue_init(&bt_pan->cback_list);
    os_queue_init(&bt_pan->link_list);

    if (bnep_init(bt_pan_cback) == false)
    {
        ret = 2;
        goto fail_init_pan;
    }

    return true;

fail_init_pan:
    free(bt_pan);
    bt_pan = NULL;
fail_alloc_pan:
    PROFILE_PRINT_ERROR1("bt_pan_init: failed %d", -ret);
    return false;
}

void bt_pan_deinit(void)
{
    if (bt_pan != NULL)
    {
        T_BT_PAN_LINK       *link;
        T_BT_PAN_CBACK_ITEM *item;

        bnep_deinit();

        link = os_queue_out(&bt_pan->link_list);
        while (link != NULL)
        {
            free(link);
            link = os_queue_out(&bt_pan->link_list);
        }

        item = os_queue_out(&bt_pan->cback_list);
        while (item != NULL)
        {
            free(item);
            item = os_queue_out(&bt_pan->cback_list);
        }

        free(bt_pan);
        bt_pan = NULL;
    }
}

#else
#include <stdint.h>
#include <stdbool.h>
#include "bt_pan.h"

bool bt_pan_init(void)
{
    return false;
}

void bt_pan_deinit(void)
{

}

bool bt_pan_cback_register(T_BT_PAN_CBACK cback)
{
    return false;
}

bool bt_pan_cback_unregister(T_BT_PAN_CBACK cback)
{
    return false;
}

bool bt_pan_connect_req(uint8_t       local_addr[6],
                        uint8_t       remote_addr[6],
                        T_BT_PAN_ROLE src_role,
                        T_BT_PAN_ROLE dst_role)
{
    return false;
}

bool bt_pan_connect_cfm(uint8_t local_addr[6],
                        uint8_t remote_addr[6],
                        bool    accept)
{
    return false;
}

bool bt_pan_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_pan_setup_connection_req(uint8_t       bd_addr[6],
                                 T_BT_PAN_ROLE src_role,
                                 T_BT_PAN_ROLE dst_role)
{
    return false;
}

bool bt_pan_setup_connection_rsp(uint8_t  bd_addr[6],
                                 uint16_t rsp_msg)
{
    return false;
}

bool bt_pan_filter_net_type_set(uint8_t   bd_addr[6],
                                uint16_t  filter_num,
                                uint16_t *range_start_array,
                                uint16_t *range_end_array)
{
    return false;
}

bool bt_pan_filter_multi_addr_set(uint8_t  bd_addr[6],
                                  uint16_t filter_num,
                                  uint8_t  addr_start_array[][6],
                                  uint8_t  addr_end_array[][6])
{
    return false;
}

bool bt_pan_send(uint8_t   bd_addr[6],
                 uint8_t  *buf,
                 uint16_t  len)
{
    return false;
}

#endif
