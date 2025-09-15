/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_queue.h"
#include "trace.h"
#include "rfc.h"
#include "bt_spp_int.h"
#include "bt_spp.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

typedef enum t_bt_spp_state
{
    BT_SPP_STATE_DISCONNECTED  = 0x00,
    BT_SPP_STATE_CONNECTING    = 0x01,
    BT_SPP_STATE_CONNECTED     = 0x02,
    BT_SPP_STATE_DISCONNECTING = 0x03,
} T_BT_SPP_STATE;

typedef struct t_bt_spp_link
{
    struct t_bt_spp_link *next;
    uint8_t               bd_addr[6];
    uint16_t              frame_size;
    T_BT_SPP_STATE        state;
    uint8_t               remote_credit;
    uint8_t               dlci;
    uint8_t               local_server_chann;
} T_BT_SPP_LINK;

typedef struct t_bt_spp_service
{
    struct t_bt_spp_service *next;
    uint8_t                  uuid[16];
    uint8_t                  server_chann;
    uint8_t                  service_id;
} T_BT_SPP_SERVICE;

typedef struct t_bt_spp
{
    T_OS_QUEUE service_list;
    T_OS_QUEUE link_list;
} T_BT_SPP;

static T_BT_SPP *bt_spp;

T_BT_SPP_LINK *bt_spp_alloc_link(uint8_t bd_addr[6])
{
    T_BT_SPP_LINK *link;

    link = calloc(1, (sizeof(T_BT_SPP_LINK)));
    if (link != NULL)
    {
        link->state = BT_SPP_STATE_DISCONNECTED;
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_in(&bt_spp->link_list, link);
    }

    return link;
}

void bt_spp_free_link(T_BT_SPP_LINK *link)
{
    if (os_queue_delete(&bt_spp->link_list, link) == true)
    {
        free(link);
    }
}

T_BT_SPP_LINK *bt_spp_find_link_by_dlci(uint8_t bd_addr[6],
                                        uint8_t dlci)
{
    T_BT_SPP_LINK *link;

    link = os_queue_peek(&bt_spp->link_list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6) && (link->dlci == dlci))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

T_BT_SPP_LINK *bt_spp_find_link_by_local_server_chann(uint8_t bd_addr[6],
                                                      uint8_t local_server_chann)
{
    T_BT_SPP_LINK *link;

    link = os_queue_peek(&bt_spp->link_list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6) &&
            (link->local_server_chann == local_server_chann))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

void bt_spp_update_credit(T_BT_SPP_LINK *link,
                          uint8_t        credit)
{
    T_BT_MSG_PAYLOAD     payload;
    T_BT_SPP_CREDIT_INFO msg;

    link->remote_credit = credit;

    msg.credit = link->remote_credit;
    msg.local_server_chann = link->local_server_chann;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = (void *)&msg;
    bt_mgr_dispatch(BT_MSG_SPP_CREDIT_INFO, &payload);
}

void bt_spp_handle_conn_ind(T_RFC_CONN_IND *p_ind)
{
    uint8_t            local_server_chann;
    T_BT_SPP_SERVICE  *service;
    T_BT_SPP_LINK     *link;
    T_BT_SPP_CONN_IND  ind;
    T_BT_MSG_PAYLOAD   payload;
    int32_t            ret = 0;

    local_server_chann = (p_ind->dlci >> 1) & 0x1F;

    link = bt_spp_find_link_by_local_server_chann(p_ind->bd_addr, local_server_chann);
    if (link)
    {
        ret = 1;
        goto fail_link_exist;
    }

    service = os_queue_peek(&bt_spp->service_list, 0);
    while (service != NULL)
    {
        if (service->server_chann == local_server_chann)
        {
            break;
        }

        service = service->next;
    }

    if (service == NULL)
    {
        ret = 2;
        goto fail_no_local_service;
    }

    link = bt_spp_alloc_link(p_ind->bd_addr);
    if (link == NULL)
    {
        ret = 3;
        goto fail_alloc_link;
    }

    link->state = BT_SPP_STATE_CONNECTING;
    link->dlci = p_ind->dlci;
    link->local_server_chann = local_server_chann;
    link->frame_size = p_ind->frame_size;

    ind.frame_size = p_ind->frame_size;
    ind.local_server_chann = local_server_chann;

    memcpy(payload.bd_addr, p_ind->bd_addr, 6);
    payload.msg_buf = (void *)&ind;

    bt_mgr_dispatch(BT_MSG_SPP_CONN_IND, &payload);

    return;

fail_alloc_link:
fail_no_local_service:
fail_link_exist:
    rfc_conn_cfm(p_ind->bd_addr, p_ind->dlci, RFC_REJECT, 0, 0);
    PROFILE_PRINT_ERROR3("bt_spp_handle_conn_ind: failed %d, addr %s, dlci 0x%02x",
                         -ret, TRACE_BDADDR(p_ind->bd_addr), p_ind->dlci);
}

void bt_spp_handle_conn_cmpl(T_RFC_CONN_CMPL *p_cmpl)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_dlci(p_cmpl->bd_addr, p_cmpl->dlci);
    if (link)
    {
        T_BT_SPP_CONN_CMPL msg;
        T_BT_MSG_PAYLOAD   payload;

        link->frame_size = p_cmpl->frame_size;
        link->remote_credit = p_cmpl->remain_credits;
        link->state = BT_SPP_STATE_CONNECTED;

        msg.frame_size = link->frame_size;
        msg.credit = link->remote_credit;
        msg.local_server_chann = link->local_server_chann;

        memcpy(payload.bd_addr, p_cmpl->bd_addr, 6);
        payload.msg_buf = (void *)&msg;

        bt_mgr_dispatch(BT_MSG_SPP_CONN_CMPL, &payload);
    }
    else
    {
        rfc_disconn_req(p_cmpl->bd_addr, p_cmpl->dlci);
    }
}

void bt_spp_handle_disconn_cmpl(T_RFC_DISCONN_CMPL *p_cmpl)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_dlci(p_cmpl->bd_addr, p_cmpl->dlci);
    if (link)
    {
        T_BT_SPP_DISCONN_CMPL disconn_cmpl;
        T_BT_SPP_STATE        state;
        T_BT_MSG_PAYLOAD      payload;

        state = link->state;
        disconn_cmpl.local_server_chann = link->local_server_chann;
        disconn_cmpl.cause = p_cmpl->cause;

        bt_spp_free_link(link);

        memcpy(payload.bd_addr, p_cmpl->bd_addr, 6);

        if (state == BT_SPP_STATE_CONNECTING)
        {
            payload.msg_buf = (void *)&p_cmpl->cause;
            bt_mgr_dispatch(BT_MSG_SPP_CONN_FAIL, &payload);
        }
        else
        {
            payload.msg_buf = (void *)&disconn_cmpl;
            bt_mgr_dispatch(BT_MSG_SPP_DISCONN_CMPL, &payload);
        }
    }
}

void bt_spp_handle_credit_info(T_RFC_CREDIT_INFO *p_info)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_dlci(p_info->bd_addr, p_info->dlci);
    if (link)
    {
        bt_spp_update_credit(link, p_info->remain_credits);
    }
}

void bt_spp_handle_data_ind(T_RFC_DATA_IND *p_ind)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_dlci(p_ind->bd_addr, p_ind->dlci);
    if (link)
    {
        T_BT_MSG_PAYLOAD  payload;
        T_BT_SPP_DATA_IND ind;

        bt_spp_update_credit(link, p_ind->remain_credits);

        if (p_ind->length)
        {
            ind.local_server_chann = link->local_server_chann;
            ind.p_data = p_ind->buf;
            ind.len = p_ind->length;

            memcpy(payload.bd_addr, p_ind->bd_addr, 6);
            payload.msg_buf = (void *)&ind;
            bt_mgr_dispatch(BT_MSG_SPP_DATA_IND, &payload);
        }
    }
}

void bt_spp_handle_data_rsp(T_RFC_DATA_RSP *p_rsp)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_dlci(p_rsp->bd_addr, p_rsp->dlci);
    if (link)
    {
        T_BT_MSG_PAYLOAD  payload;
        T_BT_SPP_DATA_RSP rsp;

        rsp.local_server_chann = link->local_server_chann;
        memcpy(payload.bd_addr, p_rsp->bd_addr, 6);
        payload.msg_buf = (void *)&rsp;
        bt_mgr_dispatch(BT_MSG_SPP_DATA_RSP, &payload);
    }
}

void bt_spp_handle_rfc_dlci_change(T_RFC_DLCI_CHANGE_INFO *p_info)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_dlci(p_info->bd_addr, p_info->prev_dlci);
    if (link)
    {
        link->dlci = p_info->curr_dlci;
    }
}

void bt_spp_handle_rfc_msg(T_RFC_MSG_TYPE  type,
                           void           *p_msg)
{
    if ((type != RFC_DATA_IND) && (type != RFC_DATA_RSP))
    {
        PROFILE_PRINT_TRACE1("bt_spp_handle_rfc_msg: msg type 0x%02x", type);
    }

    switch (type)
    {
    case RFC_CONN_IND:
        bt_spp_handle_conn_ind((T_RFC_CONN_IND *)p_msg);
        break;

    case RFC_CONN_CMPL:
        bt_spp_handle_conn_cmpl((T_RFC_CONN_CMPL *)p_msg);
        break;

    case RFC_DISCONN_CMPL:
        bt_spp_handle_disconn_cmpl((T_RFC_DISCONN_CMPL *)p_msg);
        break;

    case RFC_CREDIT_INFO:
        bt_spp_handle_credit_info((T_RFC_CREDIT_INFO *)p_msg);
        break;

    case RFC_DATA_IND:
        bt_spp_handle_data_ind((T_RFC_DATA_IND *)p_msg);
        break;

    case RFC_DATA_RSP:
        bt_spp_handle_data_rsp((T_RFC_DATA_RSP *)p_msg);
        break;

    case RFC_DLCI_CHANGE:
        bt_spp_handle_rfc_dlci_change((T_RFC_DLCI_CHANGE_INFO *)p_msg);
        break;

    default:
        break;
    }
}

bool bt_spp_init(void)
{
    bt_spp = calloc(1, sizeof(T_BT_SPP));
    if (bt_spp != NULL)
    {
        return true;
    }

    return false;
}

bool bt_spp_service_register(uint8_t service_uuid[16],
                             uint8_t server_chann)
{
    if (bt_spp != NULL)
    {
        T_BT_SPP_SERVICE *service;

        service = calloc(1, (sizeof(T_BT_SPP_SERVICE)));
        if (service != NULL)
        {
            if (rfc_cback_register(server_chann,
                                   bt_spp_handle_rfc_msg,
                                   &service->service_id) == true)
            {
                service->server_chann = server_chann;
                memcpy(service->uuid, service_uuid, 16);
                os_queue_in(&bt_spp->service_list, service);
                return true;
            }

            free(service);
        }
    }

    return false;
}

bool bt_spp_registered_uuid_check(T_BT_SPP_UUID_TYPE  type,
                                  T_BT_SPP_UUID_DATA *data,
                                  uint8_t            *local_server_chann)
{
    T_BT_SPP_SERVICE *service;
    /* base UUID 128 for Bluetooth */
    uint8_t uuid[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

    switch (type)
    {
    case BT_SDP_UUID16:
        uuid[2] = (uint8_t)(data->uuid_16 >> 8);
        uuid[3] = (uint8_t)data->uuid_16;
        break;

    case BT_SDP_UUID32:
        uuid[0] = (uint8_t)(data->uuid_32 >> 24);
        uuid[1] = (uint8_t)(data->uuid_32 >> 16);
        uuid[2] = (uint8_t)(data->uuid_32 >> 8);
        uuid[3] = (uint8_t)data->uuid_32;
        break;

    case BT_SDP_UUID128:
        memcpy(uuid, data->uuid_128, 16);
        break;

    default:
        return false;
    }

    service = os_queue_peek(&bt_spp->service_list, 0);
    while (service != NULL)
    {
        if (!memcmp(service->uuid, uuid, 16))
        {
            *local_server_chann = service->server_chann;
            return true;
        }

        service = service->next;
    }

    return false;
}

bool bt_spp_ertm_mode_set(bool enable)
{
    return rfc_set_ertm_mode(enable);
}

bool bt_spp_connect_req(uint8_t  bd_addr[6],
                        uint8_t  remote_server_chann,
                        uint16_t frame_size,
                        uint8_t  credits,
                        uint8_t  local_server_chann)
{
    T_BT_SPP_LINK    *link;
    T_BT_SPP_SERVICE *service;
    uint8_t           dlci;
    int32_t           ret = 0;

    link = bt_spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (link)
    {
        ret = 1;
        goto fail_link_exist;
    }

    service = os_queue_peek(&bt_spp->service_list, 0);
    while (service != NULL)
    {
        if (service->server_chann == local_server_chann)
        {
            break;
        }

        service = service->next;
    }

    if (service == NULL)
    {
        ret = 2;
        goto fail_no_server_chann;
    }

    link = bt_spp_alloc_link(bd_addr);
    if (link == NULL)
    {
        ret = 3;
        goto fail_alloc_link;
    }

    link->state = BT_SPP_STATE_CONNECTING;
    link->local_server_chann = local_server_chann;

    if (rfc_conn_req(bd_addr,
                     remote_server_chann,
                     service->service_id,
                     frame_size,
                     credits,
                     &dlci) == false)
    {
        ret = 4;
        goto fail_conn_rfc;
    }

    link->dlci = dlci;
    return true;

fail_conn_rfc:
    bt_spp_free_link(link);
fail_alloc_link:
fail_no_server_chann:
fail_link_exist:
    PROFILE_PRINT_ERROR3("bt_spp_connect_req: failed %d, addr %s, local server chann 0x%02x",
                         -ret, TRACE_BDADDR(bd_addr), local_server_chann);
    return false;
}

bool bt_spp_connect_cfm(uint8_t  bd_addr[6],
                        uint8_t  local_server_chann,
                        bool     accept,
                        uint16_t frame_size,
                        uint8_t  credits)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (link != NULL)
    {
        if (accept == false)
        {
            rfc_conn_cfm(bd_addr, link->dlci, RFC_REJECT, frame_size, credits);
            bt_spp_free_link(link);
        }
        else
        {
            if (frame_size > link->frame_size)
            {
                frame_size = link->frame_size;
            }
            rfc_conn_cfm(bd_addr, link->dlci, RFC_ACCEPT, frame_size, credits);
        }

        return true;
    }
    else
    {
        BTM_PRINT_ERROR0("bt_spp_connect_cfm: error!!");
        return false;
    }
}

bool bt_spp_disconnect_req(uint8_t bd_addr[6],
                           uint8_t local_server_chann)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (link)
    {
        bt_sniff_mode_exit(bd_addr, false);
        link->state = BT_SPP_STATE_DISCONNECTING;
        return rfc_disconn_req(link->bd_addr, link->dlci);
    }

    return false;
}

bool bt_spp_disconnect_all_req(uint8_t bd_addr[6])
{
    T_BT_SPP_LINK *link;
    bool           ret = false;

    bt_sniff_mode_exit(bd_addr, false);

    link = os_queue_peek(&bt_spp->link_list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6))
        {
            link->state = BT_SPP_STATE_DISCONNECTING;
            rfc_disconn_req(link->bd_addr, link->dlci);
            ret = true;
        }

        link = link->next;
    }

    return ret;
}

bool bt_spp_data_send(uint8_t   bd_addr[6],
                      uint8_t   local_server_chann,
                      uint8_t  *data,
                      uint16_t  len,
                      bool      ack)
{
    T_BT_SPP_LINK *link;
    int32_t        ret = 0;

    link = bt_spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (link == NULL)
    {
        ret = 1;
        goto fail_no_link;
    }

    if (len > link->frame_size)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remote_credit == 0)
    {
        ret = 3;
        goto fail_no_credits;
    }

    if (rfc_data_req(link->bd_addr, link->dlci, data, len, ack) == false)
    {
        ret = 4;
        goto fail_send_data;
    }

    link->remote_credit--;
    return true;

fail_send_data:
fail_no_credits:
fail_length_exceed:
fail_no_link:
    PROFILE_PRINT_ERROR4("bt_spp_data_send: failed %d, addr %s, server chann 0x%02x, len %d",
                         -ret, TRACE_BDADDR(bd_addr), local_server_chann, len);
    return false;
}

bool bt_spp_credits_give(uint8_t bd_addr[6],
                         uint8_t local_server_chann,
                         uint8_t credits)
{
    T_BT_SPP_LINK *link;

    PROFILE_PRINT_INFO3("bt_spp_credits_give: addr %s, server chann 0x%02x, credit %d",
                        TRACE_BDADDR(bd_addr), local_server_chann, credits);

    link = bt_spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (link)
    {
        return rfc_data_cfm(link->bd_addr, link->dlci, credits);
    }

    return false;
}

bool bt_spp_roleswap_info_get(uint8_t              bd_addr[6],
                              uint8_t              local_server_chann,
                              T_ROLESWAP_SPP_INFO *info)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (link != NULL)
    {
        memcpy(info->bd_addr, bd_addr, 6);
        info->local_server_chann = link->local_server_chann;
        info->frame_size    = link->frame_size;
        info->remote_credit = link->remote_credit;
        info->rfc_dlci = link->dlci;
        info->state = (uint8_t)link->state;

        return rfc_get_cid(bd_addr, info->rfc_dlci, &info->l2c_cid);
    }

    return false;
}

bool bt_spp_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_SPP_INFO *info)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_dlci(bd_addr, info->rfc_dlci);
    if (link == NULL)
    {
        link = bt_spp_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        link->state         = (T_BT_SPP_STATE)info->state;
        link->remote_credit = info->remote_credit;
        link->frame_size    = info->frame_size;
        link->dlci          = info->rfc_dlci;
        link->local_server_chann = info->local_server_chann;
        memcpy(link->bd_addr, bd_addr, 6);

        return true;
    }

    return false;
}

bool bt_spp_roleswap_info_del(uint8_t bd_addr[6],
                              uint8_t local_server_chann)
{
    T_BT_SPP_LINK *link;

    link = bt_spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (link != NULL)
    {
        bt_spp_free_link(link);
        return true;
    }

    return false;
}

bool bt_spp_service_id_get(uint8_t  local_server_chann,
                           uint8_t *service_id)
{
    T_BT_SPP_SERVICE *service;

    service = os_queue_peek(&bt_spp->service_list, 0);
    while (service != NULL)
    {
        if (service->server_chann == local_server_chann)
        {
            *service_id = service->service_id;
            return true;
        }

        service = service->next;
    }

    return false;
}
