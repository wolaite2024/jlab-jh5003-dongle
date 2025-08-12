/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_ATT_SUPPORT == 1)
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "att_br.h"

typedef struct
{
    uint8_t *p_data;
    uint16_t data_length;
} T_ATT_DATA_IND;

typedef enum
{
    ATT_STATE_DISCONNECTED  = 0x00,
    ATT_STATE_CONNECTING    = 0x01,
    ATT_STATE_CONNECTED     = 0x02,
    ATT_STATE_DISCONNECTING = 0x03,
} T_ATT_STATE;

#define ATT_LINK_NUM       2
#define ATT_MTU_SIZE       1017
#define ATT_FLUSH_TOUT     40

typedef struct
{
    uint8_t             bd_addr[6];
    uint16_t            cid;
    T_ATT_STATE         state;
    uint16_t            data_offset;
    uint16_t            remote_mtu;
    uint16_t            local_mtu;
} T_ATT_LINK;

typedef struct
{
    T_ATT_LINK         link[ATT_LINK_NUM];
    P_ATT_CBACK        cback;
    uint8_t            queue_id;
} T_ATT;

static T_ATT *p_att;

T_ATT_LINK *att_alloc_link(uint8_t bd_addr[6])
{
    uint8_t        i;
    T_ATT_LINK    *p_link = NULL;

    for (i = 0; i < ATT_LINK_NUM; i++)
    {
        if (p_att->link[i].state == ATT_STATE_DISCONNECTED)
        {
            p_link = &p_att->link[i];
            memcpy(p_link->bd_addr, bd_addr, 6);
            break;
        }
    }

    return p_link;
}

void att_free_link(T_ATT_LINK *p_link)
{
    memset(p_link, 0, sizeof(T_ATT_LINK));
}

T_ATT_LINK *att_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t        i;
    T_ATT_LINK    *p_link = NULL;

    if (bd_addr == NULL)
    {
        return p_link;
    }

    for (i = 0; i < ATT_LINK_NUM; i++)
    {
        if (!memcmp(p_att->link[i].bd_addr, bd_addr, 6))
        {
            p_link = &p_att->link[i];
            break;
        }
    }

    return p_link;
}

T_ATT_LINK *att_find_link_by_cid(uint16_t cid)
{
    uint8_t         i;
    T_ATT_LINK    *p_link = NULL;

    for (i = 0; i < ATT_LINK_NUM; i++)
    {
        if (p_att->link[i].cid == cid)
        {
            p_link = &p_att->link[i];
            break;
        }
    }

    return p_link;
}

bool att_conn_req(uint8_t bd_addr[6])
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = att_alloc_link(bd_addr);
        if (p_link != NULL)
        {
            p_link->state = ATT_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_ATT, UUID_ATT, p_att->queue_id, ATT_MTU_SIZE,
                                  bd_addr, MPA_L2C_MODE_BASIC, ATT_FLUSH_TOUT);
            return true;
        }
    }

    return false;
}

bool att_disconn_req(uint8_t bd_addr[6])
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->state == ATT_STATE_CONNECTED)
        {
            mpa_send_l2c_disconn_req(p_link->cid);
        }

        return true;
    }

    return false;
}

void att_handle_data_ind(T_MPA_L2C_DATA_IND *p_data_ind)
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_cid(p_data_ind->cid);
    if (p_link != NULL)
    {
        T_ATT_DATA_IND rx_data;

        rx_data.p_data = p_data_ind->data + p_data_ind->gap;
        rx_data.data_length = p_data_ind->length;
        p_att->cback(p_link->bd_addr, ATT_MSG_DATA_IND, &rx_data);
    }
}

void att_handle_conn_req(T_MPA_L2C_CONN_IND *p_conn_ind)
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_addr(p_conn_ind->bd_addr);
    if (p_link == NULL)
    {
        p_link = att_alloc_link(p_conn_ind->bd_addr);
        if (p_link != NULL)
        {
            p_link->cid = p_conn_ind->cid;
            p_link->state = ATT_STATE_CONNECTING;

            mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT, p_conn_ind->cid, ATT_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                  ATT_FLUSH_TOUT);
        }
        else
        {
            mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, p_conn_ind->cid, ATT_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                  ATT_FLUSH_TOUT);
        }
    }
    else
    {
        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, p_conn_ind->cid, ATT_MTU_SIZE, MPA_L2C_MODE_BASIC,
                              ATT_FLUSH_TOUT);
    }
}

void att_handle_conn_rsp(T_MPA_L2C_CONN_RSP *p_conn_rsp)
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_addr(p_conn_rsp->bd_addr);
    if (p_link != NULL)
    {
        if (p_conn_rsp->cause == 0)
        {
            p_link->cid = p_conn_rsp->cid;
        }
        else
        {
            uint16_t cause;

            cause = p_conn_rsp->cause;
            att_free_link(p_link);
            p_att->cback(p_conn_rsp->bd_addr, ATT_MSG_CONN_FAIL, &cause);
        }
    }
}

void att_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *p_conn_cmpl)
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_addr(p_conn_cmpl->bd_addr);
    if (p_link != NULL)
    {
        if (p_conn_cmpl->cause)
        {
            uint16_t cause;

            cause = p_conn_cmpl->cause;
            att_free_link(p_link);
            p_att->cback(p_conn_cmpl->bd_addr, ATT_MSG_CONN_FAIL, &cause);
        }
        else
        {
            p_link->state  = ATT_STATE_CONNECTED;
            p_link->data_offset = p_conn_cmpl->ds_data_offset;
            p_link->remote_mtu = p_conn_cmpl->remote_mtu;
            p_link->local_mtu = p_conn_cmpl->local_mtu;
            p_att->cback(p_conn_cmpl->bd_addr, ATT_MSG_CONNECTED, NULL);
        }
    }
}

void att_handle_disconn_ind(T_MPA_L2C_DISCONN_IND *p_disconn_ind)
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_cid(p_disconn_ind->cid);
    if (p_link != NULL)
    {
        uint16_t cause;

        cause = p_disconn_ind->cause;
        p_att->cback(p_link->bd_addr, ATT_MSG_DISCONNECTED, &cause);

        att_free_link(p_link);
    }

    mpa_send_l2c_disconn_cfm(p_disconn_ind->cid);
}

void att_handle_disconn_rsp(T_MPA_L2C_DISCONN_RSP *p_disconn_rsp)
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_cid(p_disconn_rsp->cid);
    if (p_link != NULL)
    {
        uint16_t cause;

        cause = p_disconn_rsp->cause;

        p_att->cback(p_link->bd_addr, ATT_MSG_DISCONNECTED, &cause);

        att_free_link(p_link);
    }
}

void att_l2c_cback(void        *p_buf,
                   T_PROTO_MSG  msg)
{
    PROTOCOL_PRINT_TRACE1("att_l2c_cback: msg 0x%02x", msg);

    switch (msg)
    {
    case L2C_CONN_IND:
        att_handle_conn_req((T_MPA_L2C_CONN_IND *)p_buf);
        break;

    case L2C_CONN_RSP:
        att_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)p_buf);
        break;

    case L2C_CONN_CMPL:
        att_handle_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)p_buf);
        break;

    case L2C_DATA_IND:
        att_handle_data_ind((T_MPA_L2C_DATA_IND *)p_buf);
        break;

    case L2C_DISCONN_IND:
        att_handle_disconn_ind((T_MPA_L2C_DISCONN_IND *)p_buf);
        break;

    case L2C_DISCONN_RSP:
        att_handle_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)p_buf);
        break;

    default:
        break;
    }
}

bool att_init(P_ATT_CBACK cback)
{
    int32_t ret = 0;

    p_att = os_mem_zalloc2(sizeof(T_ATT));
    if (p_att == NULL)
    {
        ret = 1;
        goto fail_alloc_att;
    }

    mpa_set_att_psm_data_path(MPA_ATT_PSM_DATA_PATH_GATT);

    if (mpa_reg_l2c_proto(PSM_ATT, att_l2c_cback, &p_att->queue_id) == false)
    {
        ret = 2;
        goto fail_reg_l2c;
    }

    p_att->cback = cback;
    return true;

fail_reg_l2c:
    os_mem_free(p_att);
fail_alloc_att:
    PROFILE_PRINT_ERROR1("att_init: failed %d", -ret);
    return false;
}

bool att_del_roleswap_info(uint8_t bd_addr[6])
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        att_free_link(p_link);
        return true;
    }

    return false;
}

bool att_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_ATT_INFO *p_info)
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        p_info->l2c_cid = p_link->cid;
        p_info->state = p_link->state;
        p_info->data_offset = p_link->data_offset;
        p_info->remote_mtu = p_link->remote_mtu;
        p_info->local_mtu = p_link->local_mtu;
        return true;
    }

    return false;
}

bool att_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_ATT_INFO *p_info)
{
    T_ATT_LINK *p_link;

    p_link = att_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = att_alloc_link(bd_addr);
    }

    if (p_link != NULL)
    {
        memcpy(p_link->bd_addr, bd_addr, 6);

        p_link->cid = p_info->l2c_cid;
        p_link->state = (T_ATT_STATE)p_info->state;
        p_link->data_offset = p_info->data_offset;
        p_link->remote_mtu = p_info->remote_mtu;
        p_link->local_mtu = p_info->local_mtu;

        return true;
    }

    return false;
}
#else
#include <stdint.h>
#include <stdbool.h>
#include "att_br.h"

bool att_conn_req(uint8_t bd_addr[6])
{
    return false;
}

bool att_disconn_req(uint8_t bd_addr[6])
{
    return false;
}

bool att_init(P_ATT_CBACK cback)
{
    return false;
}

bool att_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}

bool att_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_ATT_INFO *p_info)
{
    return false;
}

bool att_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_ATT_INFO *p_info)
{
    return false;
}
#endif
