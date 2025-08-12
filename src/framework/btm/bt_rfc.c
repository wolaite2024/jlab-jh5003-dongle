/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "rfc.h"
#include "bt_rfc_int.h"
#include "bt_roleswap.h"

BT_RFC_PROFILE *p_bt_rfc_profile;
BT_RFC_CHANN *p_bt_rfc_chann;

BT_RFC_CHANN *bt_rfc_alloc_chann(uint8_t bd_addr[6],
                                 uint8_t local_server_chann)
{
    uint8_t i;
    BT_RFC_CHANN *p_chann;

    for (i = 0; i < BT_RFC_DLCI_NUM; i++)
    {
        p_chann = &p_bt_rfc_chann[i];
        if (p_chann->used == false)
        {
            p_chann->used = true;
            memcpy(p_chann->bd_addr, bd_addr, 6);
            p_chann->local_server_chann = local_server_chann;
            return p_chann;
        }
    }

    return NULL;
}

BT_RFC_CHANN *bt_rfc_find_chann_by_dlci(uint8_t bd_addr[6],
                                        uint8_t dlci)
{
    uint8_t i;
    BT_RFC_CHANN *p_chann;

    for (i = 0; i < BT_RFC_DLCI_NUM; i++)
    {
        p_chann = &p_bt_rfc_chann[i];
        if (p_chann->used == true && !memcmp(p_chann->bd_addr, bd_addr, 6) && p_chann->dlci == dlci)
        {
            return p_chann;
        }
    }

    return NULL;
}

BT_RFC_CHANN *bt_rfc_find_chann_by_server_chann(uint8_t bd_addr[6],
                                                uint8_t chann)
{
    uint8_t i;
    BT_RFC_CHANN *p_chann;

    for (i = 0; i < BT_RFC_DLCI_NUM; i++)
    {
        p_chann = &p_bt_rfc_chann[i];
        if (p_chann->used == true && !memcmp(p_chann->bd_addr, bd_addr, 6) &&
            p_chann->local_server_chann == chann)
        {
            return p_chann;
        }
    }

    return NULL;
}

P_BT_RFC_PROFILE_CBACK bt_rfc_find_cback_by_server_chann(uint8_t chann)
{
    uint8_t i;

    for (i = 0; i < BT_RFC_PROFILE_NUM; i++)
    {
        if (p_bt_rfc_profile[i].server_chann == chann)
        {
            return p_bt_rfc_profile[i].cback;
        }
    }

    return NULL;
}

bool bt_rfc_find_profile_idx_by_server_chann(uint8_t  local_server_chann,
                                             uint8_t *profile_idx)
{
    uint8_t i;

    for (i = 0; i < BT_RFC_PROFILE_NUM; i++)
    {
        if (p_bt_rfc_profile[i].server_chann == local_server_chann)
        {
            *profile_idx = p_bt_rfc_profile[i].profile_index;

            return true;
        }
    }

    return false;
}

void bt_rfc_free_chann(BT_RFC_CHANN *p_chann)
{
    memset(p_chann, 0, sizeof(BT_RFC_CHANN));
    p_chann->used = false;
}

void bt_rfc_cback(T_RFC_MSG_TYPE  type,
                  void           *p_msg)
{
    BT_RFC_CHANN *p_chann;
    P_BT_RFC_PROFILE_CBACK cback = NULL;

    switch (type)
    {
    case RFC_CONN_IND:
        {
            T_RFC_CONN_IND *p;
            T_BT_RFC_CONN_IND conn_ind_msg;
            uint8_t local_server_chann = 0;
            uint8_t i;

            p = (T_RFC_CONN_IND *)p_msg;

            p_chann = bt_rfc_find_chann_by_dlci(p->bd_addr, p->dlci);
            if (p_chann)
            {
                return;
            }

            local_server_chann = (p->dlci >> 1) & 0x1F;
            for (i = 0; i < BT_RFC_PROFILE_NUM; i++)
            {
                if (p_bt_rfc_profile[i].server_chann == local_server_chann)
                {
                    p_chann = bt_rfc_alloc_chann(p->bd_addr, local_server_chann);
                    if (p_chann)
                    {
                        p_chann->dlci = p->dlci;
                        cback = p_bt_rfc_profile[i].cback;
                    }
                    break;
                }
            }

            if (cback)
            {
                conn_ind_msg.local_server_chann = p_chann->local_server_chann;
                conn_ind_msg.frame_size = p->frame_size;
                cback(p_chann->bd_addr, BT_RFC_MSG_CONN_IND, &conn_ind_msg);
            }
        }
        break;

    case RFC_CONN_CMPL:
        {
            T_RFC_CONN_CMPL *p;
            T_BT_RFC_CONN_CMPL conn_cmpl_msg;

            p = (T_RFC_CONN_CMPL *)p_msg;

            p_chann = bt_rfc_find_chann_by_dlci(p->bd_addr, p->dlci);
            if (p_chann)
            {
                cback = bt_rfc_find_cback_by_server_chann(p_chann->local_server_chann);
                if (cback)
                {
                    conn_cmpl_msg.local_server_chann = p_chann->local_server_chann;
                    conn_cmpl_msg.frame_size = p->frame_size;
                    conn_cmpl_msg.remain_credits = p->remain_credits;
                    cback(p_chann->bd_addr, BT_RFC_MSG_CONN_CMPL, &conn_cmpl_msg);
                }

                bt_roleswap_handle_bt_rfc_conn(p->bd_addr, p_chann->local_server_chann);
            }
        }
        break;

    case RFC_DISCONN_CMPL:
        {
            T_RFC_DISCONN_CMPL *p;
            T_BT_RFC_DISCONN_CMPL disconn_cmpl_msg;
            uint8_t local_server_chann;

            p = (T_RFC_DISCONN_CMPL *)p_msg;

            p_chann = bt_rfc_find_chann_by_dlci(p->bd_addr, p->dlci);
            if (p_chann)
            {
                local_server_chann = p_chann->local_server_chann;
                bt_rfc_free_chann(p_chann);

                cback = bt_rfc_find_cback_by_server_chann(local_server_chann);
                if (cback)
                {
                    disconn_cmpl_msg.local_server_chann = local_server_chann;
                    disconn_cmpl_msg.cause = p->cause;
                    cback(p->bd_addr, BT_RFC_MSG_DISCONN_CMPL, &disconn_cmpl_msg);
                }

                if (p->cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                {
                    bt_roleswap_handle_bt_rfc_disconn(p->bd_addr, local_server_chann, p->cause);
                }
            }
        }
        break;

    case RFC_CREDIT_INFO:
        {
            T_RFC_CREDIT_INFO *p;
            T_BT_RFC_CREDIT_INFO credit_msg;

            p = (T_RFC_CREDIT_INFO *)p_msg;

            p_chann = bt_rfc_find_chann_by_dlci(p->bd_addr, p->dlci);
            if (p_chann)
            {
                cback = bt_rfc_find_cback_by_server_chann(p_chann->local_server_chann);
                if (cback)
                {
                    credit_msg.local_server_chann = p_chann->local_server_chann;
                    credit_msg.remain_credits = p->remain_credits;
                    cback(p_chann->bd_addr, BT_RFC_MSG_CREDIT_INFO, &credit_msg);
                }
            }
        }
        break;

    case RFC_DATA_IND:
        {
            T_RFC_DATA_IND *p;
            T_BT_RFC_DATA_IND data_ind_msg;

            p = (T_RFC_DATA_IND *)p_msg;

            p_chann = bt_rfc_find_chann_by_dlci(p->bd_addr, p->dlci);
            if (p_chann)
            {
                cback = bt_rfc_find_cback_by_server_chann(p_chann->local_server_chann);
                if (cback)
                {
                    data_ind_msg.local_server_chann = p_chann->local_server_chann;
                    data_ind_msg.buf = p->buf;
                    data_ind_msg.length = p->length;
                    data_ind_msg.remain_credits = p->remain_credits;
                    cback(p_chann->bd_addr, BT_RFC_MSG_DATA_IND, &data_ind_msg);
                }
            }
        }
        break;

    case RFC_DATA_RSP:
        {
            T_BT_RFC_DATA_RSP data_rsp_msg;
            T_RFC_DATA_RSP *p = (T_RFC_DATA_RSP *)p_msg;

            p_chann = bt_rfc_find_chann_by_dlci(p->bd_addr, p->dlci);
            if (p_chann)
            {
                cback = bt_rfc_find_cback_by_server_chann(p_chann->local_server_chann);
                if (cback)
                {
                    data_rsp_msg.local_server_chann = p_chann->local_server_chann;
                    cback(p_chann->bd_addr, BT_RFC_MSG_DATA_RSP, &data_rsp_msg);
                }
            }
        }
        break;

    case RFC_DLCI_CHANGE:
        {
            T_RFC_DLCI_CHANGE_INFO *p_info = (T_RFC_DLCI_CHANGE_INFO *)p_msg;

            p_chann = bt_rfc_find_chann_by_dlci(p_info->bd_addr, p_info->pre_dlci);
            if (p_chann)
            {
                p_chann->dlci = p_info->curr_dlci;
            }
        }
        break;

    default:
        break;
    }
}

bool bt_rfc_init(void)
{
    p_bt_rfc_profile = os_mem_zalloc2(sizeof(BT_RFC_PROFILE) * BT_RFC_PROFILE_NUM);
    if (p_bt_rfc_profile == NULL)
    {
        return false;
    }

    p_bt_rfc_chann = os_mem_zalloc2(sizeof(BT_RFC_CHANN) * BT_RFC_DLCI_NUM);
    if (p_bt_rfc_chann == NULL)
    {
        os_mem_free(p_bt_rfc_profile);
        return false;
    }
    return true;
}

bool bt_rfc_profile_register(uint8_t                server_chann,
                             P_BT_RFC_PROFILE_CBACK cback)
{
    uint8_t i;
    uint8_t profile_idx;

    if (p_bt_rfc_profile == NULL)
    {
        if (bt_rfc_init() == false)
        {
            return false;
        }
    }

    if (server_chann != 0)
    {
        if (rfc_reg_cb(server_chann, bt_rfc_cback, &profile_idx))
        {
            for (i = 0; i < BT_RFC_PROFILE_NUM; i++)
            {
                if (p_bt_rfc_profile[i].server_chann == 0)
                {
                    p_bt_rfc_profile[i].cback = cback;
                    p_bt_rfc_profile[i].server_chann = server_chann;
                    p_bt_rfc_profile[i].profile_index = profile_idx;

                    return true;
                }
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
    uint8_t profile_idx;
    BT_RFC_CHANN *p_chann;

    p_chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (p_chann)
    {
        return false;
    }
    p_chann = bt_rfc_alloc_chann(bd_addr, local_server_chann);
    if (p_chann == NULL)
    {
        return false;
    }

    if (bt_rfc_find_profile_idx_by_server_chann(local_server_chann, &profile_idx) == true)
    {
        if (rfc_conn_req(bd_addr, remote_server_chann, frame_size, init_credits,
                         profile_idx, &p_chann->dlci) == true)
        {
            return true;
        }
    }

    bt_rfc_free_chann(p_chann);

    return false;
}

bool bt_rfc_conn_cfm(uint8_t  bd_addr[6],
                     uint8_t  local_server_chann,
                     bool     accept,
                     uint16_t frame_size,
                     uint8_t  init_credits)
{
    BT_RFC_CHANN *p_chann;

    p_chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (p_chann == NULL)
    {
        return false;
    }

    if (accept)
    {
        rfc_conn_cfm(bd_addr, p_chann->dlci, RFC_ACCEPT, frame_size, init_credits);
    }
    else
    {
        rfc_conn_cfm(bd_addr, p_chann->dlci, RFC_REJECT, 0, 0);
        bt_rfc_free_chann(p_chann);
    }
    return true;
}

bool bt_rfc_data_send(uint8_t   bd_addr[6],
                      uint8_t   local_server_chann,
                      uint8_t  *p_data,
                      uint16_t  data_len,
                      bool      ack)
{
    BT_RFC_CHANN *p_chann;

    p_chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (p_chann)
    {
        if (rfc_data_req(bd_addr, p_chann->dlci, p_data, data_len, ack) == true)
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
    BT_RFC_CHANN *p_chann;

    p_chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (p_chann)
    {
        if (rfc_data_cfm(bd_addr, p_chann->dlci, credits) == true)
        {
            return true;
        }
    }

    return false;
}

bool bt_rfc_disconn_req(uint8_t bd_addr[6],
                        uint8_t local_server_chann)
{
    BT_RFC_CHANN *p_chann;

    p_chann = bt_rfc_find_chann_by_server_chann(bd_addr, local_server_chann);
    if (p_chann)
    {
        if (rfc_disconn_req(bd_addr, p_chann->dlci) == true)
        {
            return true;
        }
    }

    return false;
}

bool bt_rfc_get_roleswap_info(uint8_t                 bd_addr[6],
                              uint8_t                 server_chann,
                              T_ROLESWAP_BT_RFC_INFO *p_info)
{
    BT_RFC_CHANN *p_chann;

    p_chann = bt_rfc_find_chann_by_server_chann(bd_addr, server_chann);
    if (p_chann)
    {
        p_info->dlci = p_chann->dlci;

        return true;
    }

    return false;
}

bool bt_rfc_set_roleswap_info(T_ROLESWAP_BT_RFC_INFO *p_info)
{
    BT_RFC_CHANN *p_chann;

    p_chann = bt_rfc_find_chann_by_server_chann(p_info->bd_addr, p_info->local_server_chann);
    if (p_chann == NULL)
    {
        p_chann = bt_rfc_alloc_chann(p_info->bd_addr, p_info->local_server_chann);
    }

    if (p_chann)
    {
        p_chann->dlci = p_info->dlci;

        return true;
    }

    return false;
}

bool bt_rfc_del_roleswap_info(uint8_t bd_addr[6],
                              uint8_t server_chann)
{
    BT_RFC_CHANN *p_chann;

    p_chann = bt_rfc_find_chann_by_server_chann(bd_addr, server_chann);
    if (p_chann)
    {
        bt_rfc_free_chann(p_chann);

        return true;
    }

    return false;
}
