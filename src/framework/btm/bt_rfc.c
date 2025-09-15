/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "bt_rfc_int.h"
#include "bt_rfc.h"
#include "bt_roleswap.h"

typedef struct t_bt_rfc_service
{
    struct t_bt_rfc_service *next;
    P_BT_RFC_SERVICE_CBACK   cback;
    uint8_t                  server_chann;
    uint8_t                  service_id;
} T_BT_RFC_SERVICE;

typedef struct t_bt_rfc_chann
{
    struct t_bt_rfc_chann *next;
    uint8_t                bd_addr[6];
    uint8_t                local_server_chann;
    uint8_t                dlci;
} T_BT_RFC_CHANN;

typedef struct t_bt_rfc_db
{
    T_OS_QUEUE service_list;
    T_OS_QUEUE chann_list;
} T_BT_RFC_DB;

static T_BT_RFC_DB *bt_rfc_db;

T_BT_RFC_CHANN *bt_rfc_alloc_chann(uint8_t bd_addr[6],
                                   uint8_t local_server_chann)
{
    T_BT_RFC_CHANN *chann;

    chann = calloc(1, sizeof(T_BT_RFC_CHANN));
    if (chann != NULL)
    {
        memcpy(chann->bd_addr, bd_addr, 6);
        chann->local_server_chann = local_server_chann;
        os_queue_in(&bt_rfc_db->chann_list, chann);
    }

    return chann;
}

T_BT_RFC_CHANN *bt_rfc_find_chann_by_dlci(uint8_t bd_addr[6],
                                          uint8_t dlci)
{
    T_BT_RFC_CHANN *chann;

    chann = os_queue_peek(&bt_rfc_db->chann_list, 0);
    while (chann != NULL)
    {
        if (!memcmp(chann->bd_addr, bd_addr, 6) &&
            chann->dlci == dlci)
        {
            break;
        }

        chann = chann->next;
    }

    return chann;
}

T_BT_RFC_CHANN *bt_rfc_find_chann_by_server_chann(uint8_t bd_addr[6],
                                                  uint8_t server_chann)
{
    T_BT_RFC_CHANN *chann;

    chann = os_queue_peek(&bt_rfc_db->chann_list, 0);
    while (chann != NULL)
    {
        if (!memcmp(chann->bd_addr, bd_addr, 6) &&
            chann->local_server_chann == server_chann)
        {
            break;
        }

        chann = chann->next;
    }

    return chann;
}

P_BT_RFC_SERVICE_CBACK bt_rfc_find_cback_by_server_chann(uint8_t chann)
{
    T_BT_RFC_SERVICE *service;

    service = os_queue_peek(&bt_rfc_db->service_list, 0);
    while (service != NULL)
    {
        if (service->server_chann == chann)
        {
            return service->cback;
        }

        service = service->next;
    }

    return NULL;
}

bool bt_rfc_service_id_get(uint8_t  local_server_chann,
                           uint8_t *service_id)
{
    T_BT_RFC_SERVICE *service;

    service = os_queue_peek(&bt_rfc_db->service_list, 0);
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

void bt_rfc_free_chann(T_BT_RFC_CHANN *chann)
{
    os_queue_delete(&bt_rfc_db->chann_list, chann);
    free(chann);
}

void bt_rfc_cback(T_RFC_MSG_TYPE  type,
                  void           *msg)
{
    T_BT_RFC_CHANN         *chann;
    P_BT_RFC_SERVICE_CBACK  cback = NULL;

    switch (type)
    {
    case RFC_CONN_IND:
        {
            T_RFC_CONN_IND *rfc_conn_ind;

            rfc_conn_ind = (T_RFC_CONN_IND *)msg;
            chann = bt_rfc_find_chann_by_dlci(rfc_conn_ind->bd_addr, rfc_conn_ind->dlci);
            if (chann == NULL)
            {
                uint8_t           local_server_chann;
                T_BT_RFC_SERVICE *service;

                local_server_chann = (rfc_conn_ind->dlci >> 1) & 0x1F;
                service = os_queue_peek(&bt_rfc_db->service_list, 0);
                while (service != NULL)
                {
                    if (service->server_chann == local_server_chann)
                    {
                        chann = bt_rfc_alloc_chann(rfc_conn_ind->bd_addr, local_server_chann);
                        if (chann)
                        {
                            T_BT_RFC_CONN_IND conn_ind_msg;

                            chann->dlci = rfc_conn_ind->dlci;
                            conn_ind_msg.local_server_chann = chann->local_server_chann;
                            conn_ind_msg.frame_size = rfc_conn_ind->frame_size;
                            service->cback(chann->bd_addr, BT_RFC_MSG_CONN_IND, &conn_ind_msg);
                            return;
                        }
                        break;
                    }

                    service = service->next;
                }
            }

            rfc_conn_cfm(rfc_conn_ind->bd_addr, rfc_conn_ind->dlci, RFC_REJECT, 0, 0);
        }
        break;

    case RFC_CONN_CMPL:
        {
            T_RFC_CONN_CMPL *conn_cmpl;

            conn_cmpl = (T_RFC_CONN_CMPL *)msg;
            chann = bt_rfc_find_chann_by_dlci(conn_cmpl->bd_addr, conn_cmpl->dlci);
            if (chann)
            {
                cback = bt_rfc_find_cback_by_server_chann(chann->local_server_chann);
                if (cback)
                {
                    T_BT_RFC_CONN_CMPL conn_cmpl_msg;

                    conn_cmpl_msg.local_server_chann = chann->local_server_chann;
                    conn_cmpl_msg.frame_size = conn_cmpl->frame_size;
                    conn_cmpl_msg.remain_credits = conn_cmpl->remain_credits;
                    cback(chann->bd_addr, BT_RFC_MSG_CONN_CMPL, &conn_cmpl_msg);
                }

                bt_roleswap_handle_bt_rfc_conn(conn_cmpl->bd_addr, chann->local_server_chann);
            }
        }
        break;

    case RFC_DISCONN_CMPL:
        {
            T_RFC_DISCONN_CMPL *disconn_cmpl;

            disconn_cmpl = (T_RFC_DISCONN_CMPL *)msg;
            chann = bt_rfc_find_chann_by_dlci(disconn_cmpl->bd_addr, disconn_cmpl->dlci);
            if (chann)
            {
                uint8_t local_server_chann;

                local_server_chann = chann->local_server_chann;
                bt_rfc_free_chann(chann);

                cback = bt_rfc_find_cback_by_server_chann(local_server_chann);
                if (cback)
                {
                    T_BT_RFC_DISCONN_CMPL disconn_cmpl_msg;

                    disconn_cmpl_msg.local_server_chann = local_server_chann;
                    disconn_cmpl_msg.cause = disconn_cmpl->cause;
                    cback(disconn_cmpl->bd_addr, BT_RFC_MSG_DISCONN_CMPL, &disconn_cmpl_msg);
                }

                if (disconn_cmpl->cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                {
                    bt_roleswap_handle_bt_rfc_disconn(disconn_cmpl->bd_addr,
                                                      local_server_chann,
                                                      disconn_cmpl->cause);
                }
            }
        }
        break;

    case RFC_CREDIT_INFO:
        {
            T_RFC_CREDIT_INFO *credit_info;

            credit_info = (T_RFC_CREDIT_INFO *)msg;
            chann = bt_rfc_find_chann_by_dlci(credit_info->bd_addr, credit_info->dlci);
            if (chann)
            {
                cback = bt_rfc_find_cback_by_server_chann(chann->local_server_chann);
                if (cback)
                {
                    T_BT_RFC_CREDIT_INFO credit_msg;

                    credit_msg.local_server_chann = chann->local_server_chann;
                    credit_msg.remain_credits = credit_info->remain_credits;
                    cback(chann->bd_addr, BT_RFC_MSG_CREDIT_INFO, &credit_msg);
                }
            }
        }
        break;

    case RFC_DATA_IND:
        {
            T_RFC_DATA_IND *data_ind;

            data_ind = (T_RFC_DATA_IND *)msg;
            chann = bt_rfc_find_chann_by_dlci(data_ind->bd_addr, data_ind->dlci);
            if (chann)
            {
                cback = bt_rfc_find_cback_by_server_chann(chann->local_server_chann);
                if (cback)
                {
                    T_BT_RFC_DATA_IND data_ind_msg;

                    data_ind_msg.local_server_chann = chann->local_server_chann;
                    data_ind_msg.buf = data_ind->buf;
                    data_ind_msg.length = data_ind->length;
                    data_ind_msg.remain_credits = data_ind->remain_credits;
                    cback(chann->bd_addr, BT_RFC_MSG_DATA_IND, &data_ind_msg);
                }
            }
        }
        break;

    case RFC_DATA_RSP:
        {
            T_RFC_DATA_RSP *data_rsp;

            data_rsp = (T_RFC_DATA_RSP *)msg;
            chann = bt_rfc_find_chann_by_dlci(data_rsp->bd_addr, data_rsp->dlci);
            if (chann)
            {
                cback = bt_rfc_find_cback_by_server_chann(chann->local_server_chann);
                if (cback)
                {
                    T_BT_RFC_DATA_RSP data_rsp_msg;

                    data_rsp_msg.local_server_chann = chann->local_server_chann;
                    cback(chann->bd_addr, BT_RFC_MSG_DATA_RSP, &data_rsp_msg);
                }
            }
        }
        break;

    case RFC_DLCI_CHANGE:
        {
            T_RFC_DLCI_CHANGE_INFO *info;

            info = (T_RFC_DLCI_CHANGE_INFO *)msg;
            chann = bt_rfc_find_chann_by_dlci(info->bd_addr, info->prev_dlci);
            if (chann)
            {
                chann->dlci = info->curr_dlci;
            }
        }
        break;

    default:
        break;
    }
}

bool bt_rfc_init(void)
{
    bt_rfc_db = calloc(1, sizeof(T_BT_RFC_DB));
    if (bt_rfc_db != NULL)
    {
        os_queue_init(&bt_rfc_db->service_list);
        os_queue_init(&bt_rfc_db->chann_list);
        return true;
    }

    return false;
}

bool bt_rfc_service_register(uint8_t                server_chann,
                             P_BT_RFC_SERVICE_CBACK cback)
{
    if (bt_rfc_db == NULL)
    {
        if (bt_rfc_init() == false)
        {
            return false;
        }
    }

    if (server_chann != 0 && cback != NULL)
    {
        T_BT_RFC_SERVICE *service;

        service = os_queue_peek(&bt_rfc_db->service_list, 0);
        while (service != NULL)
        {
            if (service->server_chann == server_chann)
            {
                break;
            }

            service = service->next;
        }

        if (service == NULL)
        {
            service = calloc(1, sizeof(T_BT_RFC_SERVICE));
            if (service != NULL)
            {
                if (rfc_cback_register(server_chann,
                                       bt_rfc_cback,
                                       &service->service_id) == true)
                {
                    service->cback = cback;
                    service->server_chann = server_chann;
                    os_queue_in(&bt_rfc_db->service_list, service);

                    return true;
                }

                free(service);
            }
        }
    }

    return false;
}

bool bt_rfc_conn_req(uint8_t  bd_addr[6],
                     uint8_t  local_server_chann,
                     uint8_t  remote_server_chann,
                     uint16_t frame_size,
                     uint8_t  init_credits)
{
    T_BT_RFC_CHANN *chann;

    chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (chann == NULL)
    {
        chann = bt_rfc_alloc_chann(bd_addr, local_server_chann);
        if (chann != NULL)
        {
            uint8_t service_id;

            if (bt_rfc_service_id_get(local_server_chann, &service_id) == true)
            {
                if (rfc_conn_req(bd_addr,
                                 remote_server_chann,
                                 service_id,
                                 frame_size,
                                 init_credits,
                                 &chann->dlci) == true)
                {
                    return true;
                }
            }

            bt_rfc_free_chann(chann);
        }
    }

    return false;
}

bool bt_rfc_conn_cfm(uint8_t  bd_addr[6],
                     uint8_t  local_server_chann,
                     bool     accept,
                     uint16_t frame_size,
                     uint8_t  init_credits)
{
    T_BT_RFC_CHANN *chann;

    chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (chann != NULL)
    {
        if (accept)
        {
            rfc_conn_cfm(bd_addr, chann->dlci, RFC_ACCEPT, frame_size, init_credits);
        }
        else
        {
            rfc_conn_cfm(bd_addr, chann->dlci, RFC_REJECT, 0, 0);
            bt_rfc_free_chann(chann);
        }

        return true;
    }

    return false;
}

bool bt_rfc_data_send(uint8_t   bd_addr[6],
                      uint8_t   local_server_chann,
                      uint8_t  *buf,
                      uint16_t  buf_len,
                      bool      ack)
{
    T_BT_RFC_CHANN *chann;

    chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (chann)
    {
        if (rfc_data_req(bd_addr, chann->dlci, buf, buf_len, ack) == true)
        {
            return true;
        }
    }

    return false;
}

bool bt_rfc_credits_give(uint8_t bd_addr[6],
                         uint8_t local_server_chann,
                         uint8_t credits)
{
    T_BT_RFC_CHANN *chann;

    chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (chann)
    {
        if (rfc_data_cfm(bd_addr, chann->dlci, credits) == true)
        {
            return true;
        }
    }

    return false;
}

bool bt_rfc_disconn_req(uint8_t bd_addr[6],
                        uint8_t local_server_chann)
{
    T_BT_RFC_CHANN *chann;

    chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (chann)
    {
        if (rfc_disconn_req(bd_addr, chann->dlci) == true)
        {
            return true;
        }
    }

    return false;
}

bool bt_rfc_get_roleswap_info(uint8_t                 bd_addr[6],
                              uint8_t                 server_chann,
                              T_ROLESWAP_BT_RFC_INFO *info)
{
    T_BT_RFC_CHANN *chann;

    chann = bt_rfc_find_chann_by_server_chann(bd_addr, server_chann);
    if (chann)
    {
        memcpy(info->bd_addr, bd_addr, 6);
        info->local_server_chann = server_chann;
        info->dlci = chann->dlci;

        return true;
    }

    return false;
}

bool bt_rfc_set_roleswap_info(T_ROLESWAP_BT_RFC_INFO *info)
{
    T_BT_RFC_CHANN *chann;

    chann = bt_rfc_find_chann_by_server_chann(info->bd_addr, info->local_server_chann);
    if (chann == NULL)
    {
        chann = bt_rfc_alloc_chann(info->bd_addr, info->local_server_chann);
    }

    if (chann)
    {
        chann->dlci = info->dlci;
        return true;
    }

    return false;
}

bool bt_rfc_del_roleswap_info(uint8_t bd_addr[6],
                              uint8_t server_chann)
{
    T_BT_RFC_CHANN *chann;

    chann = bt_rfc_find_chann_by_server_chann(bd_addr, server_chann);
    if (chann)
    {
        bt_rfc_free_chann(chann);
        return true;
    }

    return false;
}
