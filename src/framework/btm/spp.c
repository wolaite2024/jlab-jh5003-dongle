/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>

#include "os_mem.h"
#include "trace.h"
#include "rfc.h"
#include "spp.h"

typedef enum t_spp_state
{
    SPP_STATE_DISCONNECTED = 0x00,
    SPP_STATE_ALLOCATED    = 0x01,
    SPP_STATE_CONNECTING   = 0x02,
    SPP_STATE_CONNECTED    = 0x03,
} T_SPP_STATE;

typedef enum
{
    SPP_ROLE_CLIENT = 0x00,
    SPP_ROLE_SERVER = 0x01,
} T_SPP_ROLE;

typedef struct
{
    P_SPP_CBACK         callback;

    uint16_t            frame_size;
    uint8_t             server_chann;   // used for rfc conn rsp to find link, optimize later
    T_SPP_ROLE          role;           // used for rfc conn rsp to find link, optimize later
    T_SPP_STATE         state;
    uint8_t             bd_addr[6];
    uint8_t             remote_credit;
    uint8_t             dlci;
    uint8_t             local_server_chann; // used for upper layer to distinguish spp service
} T_SPP_LINK;

typedef struct
{
    uint8_t             server_chann;
    uint8_t             rfc_index;
    P_SPP_CBACK         callback;
} T_SPP_CALLBACK;

typedef struct
{
    T_SPP_CALLBACK      *p_callback;
    T_SPP_LINK          *p_link;
    uint8_t              timer_queue_id;
    uint8_t              spp_cb_num;
    uint8_t              link_num;
} T_SPP;

T_SPP *p_spp;

T_SPP_LINK *spp_alloc_link(uint8_t    bd_addr[6],
                           uint8_t    server_chann,
                           T_SPP_ROLE role)
{
    uint8_t i;
    T_SPP_LINK *p_link;

    for (i = 0; i < p_spp->link_num; i++)
    {
        p_link = &p_spp->p_link[i];

        if (p_link->state == SPP_STATE_DISCONNECTED)
        {
            p_link->server_chann = server_chann;
            p_link->role = role;
            p_link->state = SPP_STATE_ALLOCATED;
            memcpy(p_link->bd_addr, bd_addr, 6);

            return p_link;
        }
    }

    return NULL;
}

void spp_free_link(T_SPP_LINK *p_link)
{
    memset(p_link, 0, sizeof(T_SPP_LINK));
}

T_SPP_LINK *spp_find_link_by_dlci(uint8_t bd_addr[6],
                                  uint8_t dlci)
{
    uint8_t i;
    T_SPP_LINK *p_link;

    for (i = 0; i < p_spp->link_num; i++)
    {
        p_link = &p_spp->p_link[i];
        if ((p_link->dlci == dlci) && (memcmp(p_link->bd_addr, bd_addr, 6) == 0))
        {
            return p_link;
        }
    }

    return NULL;
}

T_SPP_LINK *spp_find_link_by_local_server_chann(uint8_t bd_addr[6],
                                                uint8_t local_server_chann)
{
    uint8_t i;
    T_SPP_LINK *p_link;

    for (i = 0; i < p_spp->link_num; i++)
    {
        p_link = &p_spp->p_link[i];
        if ((p_link->local_server_chann == local_server_chann) &&
            (memcmp(p_link->bd_addr, bd_addr, 6) == 0))
        {
            return p_link;
        }
    }

    return NULL;
}

void spp_update_credit(T_SPP_LINK *p_link,
                       uint8_t     credit)
{
    T_SPP_CREDIT_INFO msg;

    p_link->remote_credit = credit;

    msg.credit = p_link->remote_credit;
    msg.local_server_chann = p_link->local_server_chann;

    if (p_link->callback)
    {
        p_link->callback(p_link->bd_addr, SPP_MSG_CREDIT_INFO, (void *)&msg);
    }
}

void spp_handle_conn_ind(T_RFC_CONN_IND *p_ind)
{
    uint8_t local_server_chann = (p_ind->dlci >> 1) & 0x1F;
    uint8_t i;
    T_SPP_LINK *p_link;
    T_SPP_CONN_IND ind;

    // first check if this spp service is already connected
    p_link = spp_find_link_by_local_server_chann(p_ind->bd_addr, local_server_chann);
    if (p_link)
    {
        PROFILE_PRINT_ERROR2("spp_handle_conn_ind: spp link already exists for addr[%s] dlci[0x%02x]",
                             TRACE_BDADDR(p_ind->bd_addr), p_ind->dlci);
        return;
    }

    for (i = 0; i < p_spp->spp_cb_num; i++)
    {
        if (p_spp->p_callback[i].server_chann == local_server_chann)
        {
            break;
        }
    }

    if (i == p_spp->spp_cb_num)
    {
        PROFILE_PRINT_ERROR1("spp_handle_conn_ind: fail to find callback func for server chann[0x%02x]",
                             local_server_chann);
        rfc_conn_cfm(p_ind->bd_addr, p_ind->dlci, RFC_REJECT, 0, 0);
        return;
    }

    p_link = spp_alloc_link(p_ind->bd_addr, local_server_chann, SPP_ROLE_SERVER);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_handle_conn_ind: fail to allocate link for addr[%s] dlci[0x%02x]",
                             TRACE_BDADDR(p_ind->bd_addr), p_ind->dlci);
        rfc_conn_cfm(p_ind->bd_addr, p_ind->dlci, RFC_REJECT, 0, 0);
        return;
    }

    p_link->state = SPP_STATE_CONNECTING;
    p_link->dlci = p_ind->dlci;
    p_link->local_server_chann = local_server_chann;
    p_link->callback = p_spp->p_callback[i].callback;

    p_link->frame_size = p_ind->frame_size;

    ind.frame_size = p_ind->frame_size;
    ind.local_server_chann = local_server_chann;
    if (p_link->callback)
    {
        p_link->callback(p_ind->bd_addr, SPP_MSG_CONN_IND, (void *)&ind);
    }
}

void spp_handle_conn_cmpl(T_RFC_CONN_CMPL *p_cmpl)
{
    T_SPP_CONN_CMPL msg;
    T_SPP_LINK *p_link;

    p_link = spp_find_link_by_dlci(p_cmpl->bd_addr, p_cmpl->dlci);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_handle_conn_cmpl: fail to find link for addr[%s] dcli[0x%02x]",
                             TRACE_BDADDR(p_cmpl->bd_addr), p_cmpl->dlci);
        rfc_disconn_req(p_cmpl->bd_addr, p_cmpl->dlci);
        return;
    }

    p_link->frame_size = p_cmpl->frame_size;
    p_link->remote_credit = p_cmpl->remain_credits;
    p_link->state = SPP_STATE_CONNECTED;

    msg.frame_size = p_link->frame_size;
    msg.credit = p_link->remote_credit;
    msg.local_server_chann = p_link->local_server_chann;

    if (p_link->callback)
    {
        p_link->callback(p_link->bd_addr, SPP_MSG_CONN_CMPL, (void *)&msg);
    }
}

void spp_handle_disconn_cmpl(T_RFC_DISCONN_CMPL *p_cmpl)
{
    T_SPP_LINK *p_link;
    P_SPP_CBACK callback;
    uint8_t bd_addr[6];
    T_SPP_DISCONN_CMPL disconn_cmpl;
    T_SPP_STATE        state;

    p_link = spp_find_link_by_dlci(p_cmpl->bd_addr, p_cmpl->dlci);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_handle_disconn_cmpl: fail to find link for addr[%s] dlci[0x%02x]",
                             TRACE_BDADDR(p_cmpl->bd_addr), p_cmpl->dlci);
        return;
    }

    callback = p_link->callback;
    memcpy(bd_addr, p_link->bd_addr, 6);
    state = p_link->state;
    disconn_cmpl.local_server_chann = p_link->local_server_chann;
    disconn_cmpl.cause = p_cmpl->cause;

    spp_free_link(p_link);

    if (callback)
    {
        if (state == SPP_STATE_CONNECTING)
        {
            callback(bd_addr, SPP_MSG_CONN_FAIL, (void *)&p_cmpl->cause);
        }
        else
        {
            callback(bd_addr, SPP_MSG_DISCONN_CMPL, (void *)&disconn_cmpl);
        }
    }
}

void spp_handle_credit_info(T_RFC_CREDIT_INFO *p_info)
{
    T_SPP_LINK *p_link = spp_find_link_by_dlci(p_info->bd_addr, p_info->dlci);

    if (p_link)
    {
        spp_update_credit(p_link, p_info->remain_credits);
    }
}

void spp_handle_data_ind(T_RFC_DATA_IND *p_ind)
{
    T_SPP_LINK *p_link;
    T_SPP_DATA_IND ind;

    p_link = spp_find_link_by_dlci(p_ind->bd_addr, p_ind->dlci);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_handle_data_ind: fail to find link for addr[%s] dcli[0x%02x]",
                             TRACE_BDADDR(p_ind->bd_addr), p_ind->dlci);
        return;
    }

    spp_update_credit(p_link, p_ind->remain_credits);

    if (p_ind->length)
    {
        ind.local_server_chann = p_link->local_server_chann;
        ind.p_data = p_ind->buf;
        ind.len = p_ind->length;

        if (p_link->callback)
        {
            p_link->callback(p_link->bd_addr, SPP_MSG_DATA_IND, (void *)&ind);
        }
    }
}

void spp_handle_data_rsp(T_RFC_DATA_RSP *p_rsp)
{
    T_SPP_LINK *p_link;
    T_SPP_DATA_RSP rsp;

    p_link = spp_find_link_by_dlci(p_rsp->bd_addr, p_rsp->dlci);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_handle_data_rsp: fail to find link for addr[%s] dcli[0x%02x]",
                             TRACE_BDADDR(p_rsp->bd_addr), p_rsp->dlci);
        return;
    }

    rsp.local_server_chann = p_link->local_server_chann;

    if (p_link->callback)
    {
        p_link->callback(p_link->bd_addr, SPP_MSG_DATA_RSP, (void *)&rsp);
    }
}

void spp_handle_rfc_dlci_change(T_RFC_DLCI_CHANGE_INFO *p_info)
{
    T_SPP_LINK *p_link;

    p_link = spp_find_link_by_dlci(p_info->bd_addr, p_info->pre_dlci);
    if (p_link)
    {
        p_link->dlci = p_info->curr_dlci;
    }
}

void spp_handle_rfc_msg(T_RFC_MSG_TYPE  type,
                        void           *p_msg)
{
    PROFILE_PRINT_TRACE1("spp_handle_rfc_msg: msg type 0x%02x", type);

    switch (type)
    {
    case RFC_CONN_IND:
        spp_handle_conn_ind((T_RFC_CONN_IND *)p_msg);
        break;

    case RFC_CONN_CMPL:
        spp_handle_conn_cmpl((T_RFC_CONN_CMPL *)p_msg);
        break;

    case RFC_DISCONN_CMPL:
        spp_handle_disconn_cmpl((T_RFC_DISCONN_CMPL *)p_msg);
        break;

    case RFC_CREDIT_INFO:
        spp_handle_credit_info((T_RFC_CREDIT_INFO *)p_msg);
        break;

    case RFC_DATA_IND:
        spp_handle_data_ind((T_RFC_DATA_IND *)p_msg);
        break;

    case RFC_DATA_RSP:
        spp_handle_data_rsp((T_RFC_DATA_RSP *)p_msg);
        break;

    case RFC_DLCI_CHANGE:
        spp_handle_rfc_dlci_change((T_RFC_DLCI_CHANGE_INFO *)p_msg);
        break;

    default:
        break;
    }
}

bool spp_connect_req(uint8_t  bd_addr[6],
                     uint8_t  rmt_server_chann,
                     uint16_t frame_size,
                     uint8_t  credits,
                     uint8_t  local_server_chann)
{
    uint8_t rfc_idx = 0;
    P_SPP_CBACK callback = NULL;
    T_SPP_LINK *p_link;
    uint8_t i;
    uint8_t dlci;

    // first check if this spp service is alredy there
    p_link = spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (p_link)
    {
        PROFILE_PRINT_ERROR2("spp_connect_req: spp link already exists for addr[%s] local server chann[0x%02x]",
                             TRACE_BDADDR(bd_addr), local_server_chann);
        return false;
    }

    for (i = 0; i < p_spp->spp_cb_num; i++)
    {
        if (p_spp->p_callback[i].server_chann == local_server_chann)
        {
            rfc_idx = p_spp->p_callback[i].rfc_index;
            callback = p_spp->p_callback[i].callback;
            break;
        }
    }

    if (i == p_spp->spp_cb_num)
    {
        PROFILE_PRINT_ERROR1("spp_connect_req: callback func not registered for server channel[0x%02x]",
                             local_server_chann);
        return false;
    }

    p_link = spp_alloc_link(bd_addr, rmt_server_chann, SPP_ROLE_CLIENT);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_connect_req: alloc link fail for addr[%s] local server channel[0x%02x]",
                             TRACE_BDADDR(bd_addr), local_server_chann);
        return false;
    }

    p_link->state = SPP_STATE_CONNECTING;
    p_link->callback = callback;
    p_link->local_server_chann = local_server_chann;

    if (rfc_conn_req(bd_addr, rmt_server_chann, frame_size, credits, rfc_idx, &dlci))
    {
        p_link->dlci = dlci;
        return true;

    }
    else
    {
        spp_free_link(p_link);
        return false;
    }
}

bool spp_connect_cfm(uint8_t  bd_addr[6],
                     uint8_t  local_server_chann,
                     bool     accept,
                     uint16_t frame_size,
                     uint8_t  credits)
{
    T_SPP_LINK *p_link;

    p_link = spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_connect_cfm: fail to find spp link for addr[%s] server chann[0x%02x]",
                             TRACE_BDADDR(bd_addr), local_server_chann);
        return false;
    }

    if (accept == false)
    {
        rfc_conn_cfm(bd_addr, p_link->dlci, RFC_REJECT, frame_size, credits);
        spp_free_link(p_link);  // if app reject, do not need to send conn fail msg to app
    }
    else
    {
        // incase upper layer cfm frame size larger than the one in indicate
        if (frame_size > p_link->frame_size)
        {
            frame_size = p_link->frame_size;
        }
        rfc_conn_cfm(bd_addr, p_link->dlci, RFC_ACCEPT, frame_size, credits);
    }
    return true;
}

bool spp_disconnect_req(uint8_t bd_addr[6],
                        uint8_t local_server_chann)
{
    T_SPP_LINK *p_link;

    p_link = spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_disconnect_req: fail to find link for addr[%s] server chann[0x%02x]",
                             TRACE_BDADDR(bd_addr), local_server_chann);
        return false;
    }

    return rfc_disconn_req(p_link->bd_addr, p_link->dlci);
}

void spp_disconnect_all(uint8_t bd_addr[6])
{
    uint8_t i;
    T_SPP_LINK *p_link;

    for (i = 0; i < p_spp->link_num; i++)
    {
        p_link = &p_spp->p_link[i];
        if (memcmp(p_link->bd_addr, bd_addr, 6) == 0)
        {
            rfc_disconn_req(p_link->bd_addr, p_link->dlci);
        }
    }
}

bool spp_send_data(uint8_t   bd_addr[6],
                   uint8_t   local_server_chann,
                   uint8_t  *p_data,
                   uint16_t  len,
                   bool      ack)
{
    T_SPP_LINK *p_link;

    p_link = spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_send_data: fail to find link for addr[%s] server chann[0x%02x]",
                             TRACE_BDADDR(bd_addr), local_server_chann);
        return false;
    }

    if (len > p_link->frame_size)
    {
        PROFILE_PRINT_ERROR2("spp_send_data: data length [%d] is larger than frame size [%d]",
                             len, p_link->frame_size);
        return false;
    }

    if (p_link->remote_credit == 0)
    {
        PROFILE_PRINT_ERROR0("spp_send_data: no credits for this link");
        return false;
    }

    if (rfc_data_req(p_link->bd_addr, p_link->dlci, p_data, len, ack) == true)
    {
        p_link->remote_credit--;
        return true;
    }

    return false;
}

bool spp_send_credit(uint8_t bd_addr[6],
                     uint8_t local_server_chann,
                     uint8_t credits)
{
    T_SPP_LINK *p_link;

    p_link = spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_send_credit: fail to find link for addr[%s] server chann[0x%02x]",
                             TRACE_BDADDR(bd_addr), local_server_chann);
        return false;
    }

    PROFILE_PRINT_INFO3("spp_send_credit: addr[%s] server chann[0x%02x] credit[%d]",
                        TRACE_BDADDR(bd_addr), local_server_chann, credits);

    return rfc_data_cfm(p_link->bd_addr, p_link->dlci, credits);
}

bool spp_set_ertm_mode(bool enable)
{
    return rfc_set_ertm_mode(enable);
}

bool spp_reg_cb(uint8_t     server_chann,
                P_SPP_CBACK callback)
{
    uint8_t i;
    uint8_t index = 0;

    if (p_spp == NULL)
    {
        return false;
    }

    for (i = 0; i < p_spp->spp_cb_num; i++)
    {
        if (p_spp->p_callback[i].server_chann == 0)
        {
            break;
        }
    }

    if (i < p_spp->spp_cb_num)
    {
        if (rfc_reg_cb(server_chann, spp_handle_rfc_msg, &index))
        {
            p_spp->p_callback[i].callback = callback;
            p_spp->p_callback[i].rfc_index = index;
            p_spp->p_callback[i].server_chann = server_chann;

            return true;
        }
    }

    return false;
}

bool spp_init(uint8_t link_num,
              uint8_t service_num)
{
    int32_t ret = 0;

    p_spp = os_mem_zalloc2(sizeof(T_SPP));
    if (p_spp == NULL)
    {
        ret = 1;
        goto fail_alloc_spp;
    }
    p_spp->link_num = link_num;

    p_spp->p_link = os_mem_zalloc2(p_spp->link_num * sizeof(T_SPP_LINK));
    if (p_spp->p_link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    p_spp->p_callback = os_mem_zalloc2(service_num * sizeof(T_SPP_CALLBACK));
    if (p_spp->p_callback == NULL)
    {
        ret = 3;
        goto fail_alloc_callback;
    }

    p_spp->spp_cb_num = service_num;

    return true;

fail_alloc_callback:
    os_mem_free(p_spp->p_link);
fail_alloc_link:
    os_mem_free(p_spp);
    p_spp = NULL;
fail_alloc_spp:
    PROFILE_PRINT_ERROR1("spp_init: failed %d", -ret);
    return false;
}

bool spp_get_roleswap_info(uint8_t              bd_addr[6],
                           uint8_t              local_server_chann,
                           T_ROLESWAP_SPP_INFO *p_info)
{
    T_SPP_LINK *p_link;

    p_link = spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR2("spp_get_roleswap_info: fail to find link for addr[%s] server_chann[0x%02x]",
                             TRACE_BDADDR(bd_addr), local_server_chann);
        return false;
    }

    p_info->local_server_chann = p_link->local_server_chann;
    p_info->frame_size    = p_link->frame_size;
    p_info->remote_credit = p_link->remote_credit;
    p_info->rfc_dlci = p_link->dlci;
    p_info->role = (uint8_t)p_link->role;
    p_info->state = (uint8_t)p_link->state;

    return rfc_get_cid(bd_addr, p_info->rfc_dlci, &p_info->l2c_cid);
}

bool spp_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_SPP_INFO *p_info)
{
    uint8_t i;
    T_SPP_LINK *p_link;

    p_link = spp_find_link_by_dlci(bd_addr, p_info->rfc_dlci);
    if (p_link == NULL)
    {
        p_link = spp_alloc_link(bd_addr, ((p_info->rfc_dlci >> 1) & 0x1F), (T_SPP_ROLE)p_info->role);
        if (p_link == NULL)
        {
            PROFILE_PRINT_ERROR2("spp_set_roleswap_info: fail to allocate link for addr[%s] dcli[0x%02x]",
                                 TRACE_BDADDR(bd_addr), p_info->rfc_dlci);
            return false;
        }
    }

    for (i = 0; i < p_spp->spp_cb_num; i++)
    {
        if (p_info->local_server_chann == p_spp->p_callback[i].server_chann)
        {
            p_link->callback = p_spp->p_callback[i].callback;
            break;
        }
    }

    if (i == p_spp->spp_cb_num)
    {
        PROFILE_PRINT_ERROR1("spp_set_roleswap_info: fail to find registered callback for server channel[%d]",
                             p_info->local_server_chann);
        spp_free_link(p_link);
        return false;
    }

    p_link->state         = (T_SPP_STATE)p_info->state;
    p_link->remote_credit = p_info->remote_credit;
    p_link->frame_size    = p_info->frame_size;
    p_link->dlci          = p_info->rfc_dlci;
    p_link->role          = (T_SPP_ROLE)p_info->role;
    p_link->local_server_chann = p_info->local_server_chann;
    memcpy(p_link->bd_addr, bd_addr, 6);

    return true;
}

bool spp_del_roleswap_info(uint8_t bd_addr[6],
                           uint8_t local_server_chann)
{
    T_SPP_LINK *p_link;

    p_link = spp_find_link_by_local_server_chann(bd_addr, local_server_chann);
    if (p_link == NULL)
    {
        return false;
    }

    spp_free_link(p_link);

    return true;
}

bool spp_get_rfc_profile_idx(uint8_t  local_server_chann,
                             uint8_t *p_rfc_idx)
{
    uint8_t i;

    for (i = 0; i < p_spp->spp_cb_num; i++)
    {
        if (p_spp->p_callback[i].server_chann == local_server_chann)
        {
            *p_rfc_idx = p_spp->p_callback[i].rfc_index;
            return true;
        }
    }

    return false;
}
