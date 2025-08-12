/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "rdtp.h"

#define RDTP_LINK_NUM       2
#define RDTP_MTU_SIZE       1017
#define RDTP_FLUSH_TOUT     40

typedef enum
{
    RDTP_STATE_DISCONNECTED  = 0x00,
    RDTP_STATE_CONNECTING    = 0x01,
    RDTP_STATE_CONNECTED     = 0x02,
    RDTP_STATE_DISCONNECTING = 0x03,
} T_RDTP_STATE;

typedef struct
{
    uint8_t             bd_addr[6];
    uint16_t            cid;
    T_RDTP_STATE        state;
} T_RDTP_LINK;

typedef struct
{
    T_RDTP_LINK         link[RDTP_LINK_NUM];
    P_RDTP_CBACK        cback;
    uint16_t            data_offset;
    uint8_t             queue_id;
} T_RDTP;

T_RDTP *p_rdtp;

T_RDTP_LINK *rdtp_alloc_link(uint8_t bd_addr[6])
{
    uint8_t         i;
    T_RDTP_LINK    *p_link = NULL;

    for (i = 0; i < RDTP_LINK_NUM; i++)
    {
        if (p_rdtp->link[i].state == RDTP_STATE_DISCONNECTED)
        {
            p_link = &p_rdtp->link[i];

            memcpy(p_link->bd_addr, bd_addr, 6);
            break;
        }
    }

    PROTOCOL_PRINT_TRACE2("rdtp_alloc_link: bd_addr %s, link %p",
                          TRACE_BDADDR(bd_addr), p_link);

    return p_link;
}

void rdtp_free_link(T_RDTP_LINK *p_link)
{
    PROTOCOL_PRINT_TRACE1("rdtp_free_link: link %p", p_link);

    memset(p_link, 0, sizeof(T_RDTP_LINK));
}

T_RDTP_LINK *rdtp_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t         i;
    T_RDTP_LINK    *p_link = NULL;

    if (bd_addr == NULL)
    {
        return p_link;
    }

    for (i = 0; i < RDTP_LINK_NUM; i++)
    {
        if (!memcmp(p_rdtp->link[i].bd_addr, bd_addr, 6))
        {
            p_link = &p_rdtp->link[i];
            break;
        }
    }

    return p_link;
}

T_RDTP_LINK *rdtp_find_link_by_cid(uint16_t cid)
{
    uint8_t         i;
    T_RDTP_LINK    *p_link = NULL;

    for (i = 0; i < RDTP_LINK_NUM; i++)
    {
        if (p_rdtp->link[i].cid == cid)
        {
            p_link = &p_rdtp->link[i];
            break;
        }
    }

    return p_link;
}

bool rdtp_conn_req(uint8_t bd_addr[6])
{
    T_RDTP_LINK *p_link;

    p_link = rdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return false;
    }

    p_link = rdtp_alloc_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->state = RDTP_STATE_CONNECTING;
        mpa_send_l2c_conn_req(PSM_RDTP, UUID_RDTP, p_rdtp->queue_id, RDTP_MTU_SIZE,
                              bd_addr, MPA_L2C_MODE_BASIC, RDTP_FLUSH_TOUT);
        return true;
    }

    return false;
}

bool rdtp_disconn_req(uint8_t bd_addr[6])
{
    T_RDTP_LINK *p_link;

    p_link = rdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        mpa_send_l2c_disconn_req(p_link->cid);
        return true;
    }

    return false;
}

void rdtp_handle_data_ind(T_MPA_L2C_DATA_IND *p_data_ind)
{
    T_RDTP_LINK *p_link;

    p_link = rdtp_find_link_by_cid(p_data_ind->cid);
    if (p_link != NULL)
    {
        T_RDTP_DATA_IND data_ind;

        data_ind.buf = p_data_ind->data + p_data_ind->gap;
        data_ind.len = p_data_ind->length;

        p_rdtp->cback(p_link->bd_addr, RDTP_MSG_DATA_IND, &data_ind);
    }
}

void rdtp_handle_data_rsp(T_MPA_L2C_DATA_RSP *p_data_rsp)
{
    T_RDTP_LINK *p_link;

    p_link = rdtp_find_link_by_cid(p_data_rsp->cid);
    if (p_link != NULL)
    {
        p_rdtp->cback(p_link->bd_addr, RDTP_MSG_DATA_RSP, NULL);
    }
}

void rdtp_handle_conn_req(T_MPA_L2C_CONN_IND *p_conn_ind)
{
    T_RDTP_LINK *p_link;
    T_MPA_L2C_CONN_CFM_CAUSE rsp = MPA_L2C_CONN_NO_RESOURCE;

    p_link = rdtp_find_link_by_addr(p_conn_ind->bd_addr);
    if (p_link == NULL)
    {
        p_link = rdtp_alloc_link(p_conn_ind->bd_addr);
        if (p_link != NULL)
        {
            rsp = MPA_L2C_CONN_ACCEPT;

            p_link->cid = p_conn_ind->cid;
            p_link->state = RDTP_STATE_CONNECTING;

            p_rdtp->cback(p_link->bd_addr, RDTP_MSG_CONN_REQ, &p_link->cid);
        }
    }
    else
    {
        if (p_link->state == RDTP_STATE_CONNECTING)
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;
        }
        else if (p_link->state == RDTP_STATE_CONNECTED)
        {
            rsp = MPA_L2C_CONN_INVALID_SOURCE_CID;
        }
        else if (p_link->state == RDTP_STATE_DISCONNECTED)
        {
            rsp = MPA_L2C_CONN_ACCEPT;

            p_link->cid = p_conn_ind->cid;
            p_link->state = RDTP_STATE_CONNECTING;

            p_rdtp->cback(p_link->bd_addr, RDTP_MSG_CONN_REQ, &p_link->cid);
        }
    }

    mpa_send_l2c_conn_cfm(rsp, p_conn_ind->cid, RDTP_MTU_SIZE, MPA_L2C_MODE_BASIC, RDTP_FLUSH_TOUT);
}

void rdtp_handle_conn_rsp(T_MPA_L2C_CONN_RSP *p_conn_rsp)
{
    T_RDTP_LINK *p_link;

    p_link = rdtp_find_link_by_addr(p_conn_rsp->bd_addr);
    if (p_link != NULL)
    {
        if (p_conn_rsp->cause == 0)
        {
            if (p_link->state == RDTP_STATE_CONNECTING)
            {
                p_link->cid = p_conn_rsp->cid;
                p_rdtp->cback(p_conn_rsp->bd_addr, RDTP_MSG_CONN_RSP, &p_link->cid);
            }
        }
        else
        {
            if (p_link->state == RDTP_STATE_CONNECTING)
            {
                rdtp_free_link(p_link);
                p_rdtp->cback(p_conn_rsp->bd_addr, RDTP_MSG_CONN_FAIL, &p_conn_rsp->cause);
            }
        }
    }
}

void rdtp_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *p_conn_cmpl)
{
    T_RDTP_LINK *p_link;
    uint16_t mtu_size;

    p_link = rdtp_find_link_by_cid(p_conn_cmpl->cid);
    if (p_link != NULL)
    {
        if (p_conn_cmpl->cause)
        {
            if (p_link->state == RDTP_STATE_CONNECTING)
            {
                rdtp_free_link(p_link);
                p_rdtp->cback(p_conn_cmpl->bd_addr, RDTP_MSG_CONN_FAIL, &p_conn_cmpl->cause);
            }
        }
        else
        {
            p_link->state       = RDTP_STATE_CONNECTED;
            p_rdtp->data_offset = p_conn_cmpl->ds_data_offset;
            mtu_size = p_conn_cmpl->remote_mtu;
            p_rdtp->cback(p_conn_cmpl->bd_addr, RDTP_MSG_CONNECTED, &mtu_size);
        }
    }
}

void rdtp_handle_disconn_ind(T_MPA_L2C_DISCONN_IND *p_disconn_ind)
{
    T_RDTP_LINK *p_link;

    p_link = rdtp_find_link_by_cid(p_disconn_ind->cid);
    if (p_link != NULL)
    {
        T_RDTP_DISCONN_INFO info;
        info.cause = p_disconn_ind->cause;
        p_rdtp->cback(p_link->bd_addr, RDTP_MSG_DISCONNECTED, &info);
        rdtp_free_link(p_link);
    }

    mpa_send_l2c_disconn_cfm(p_disconn_ind->cid);
}

void rdtp_handle_disconn_rsp(T_MPA_L2C_DISCONN_RSP *p_disconn_rsp)
{
    T_RDTP_LINK *p_link;

    p_link = rdtp_find_link_by_cid(p_disconn_rsp->cid);
    if (p_link != NULL)
    {
        T_RDTP_DISCONN_INFO info;
        info.cause = p_disconn_rsp->cause;
        p_rdtp->cback(p_link->bd_addr, RDTP_MSG_DISCONNECTED, &info);
        rdtp_free_link(p_link);
    }
}

bool rdtp_handle_roleswap(uint8_t  bd_addr[6],
                          uint8_t *curr_bd_addr)
{
    T_RDTP_LINK *p_link;

    p_link = rdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        memcpy(p_link->bd_addr, curr_bd_addr, 6);
        return true;
    }

    return false;
}

bool rdtp_data_send(uint8_t   bd_addr[6],
                    void     *buf,
                    uint16_t  len,
                    bool      flush)
{
    T_RDTP_LINK *p_link;
    uint8_t     *p_pkt;

    p_link = rdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        p_pkt = mpa_get_l2c_buf(p_rdtp->queue_id, p_link->cid, 0, len, p_rdtp->data_offset, true);
        if (p_pkt != NULL)
        {
            memcpy(p_pkt + p_rdtp->data_offset, buf, len);
            mpa_send_l2c_data_req(p_pkt, p_rdtp->data_offset, p_link->cid, len, flush);
            return true;
        }
    }

    return false;
}

void rdtp_l2c_cback(void        *p_buf,
                    T_PROTO_MSG  msg)
{
    //PROTOCOL_PRINT_TRACE1("rdtp_l2c_cback: msg 0x%02x", msg);

    switch (msg)
    {
    case L2C_CONN_IND:
        rdtp_handle_conn_req((T_MPA_L2C_CONN_IND *)p_buf);
        break;

    case L2C_CONN_RSP:
        rdtp_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)p_buf);
        break;

    case L2C_CONN_CMPL:
        rdtp_handle_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)p_buf);
        break;

    case L2C_DATA_IND:
        rdtp_handle_data_ind((T_MPA_L2C_DATA_IND *)p_buf);
        break;

    case L2C_DATA_RSP:
        rdtp_handle_data_rsp((T_MPA_L2C_DATA_RSP *)p_buf);
        break;

    case L2C_DISCONN_IND:
        rdtp_handle_disconn_ind((T_MPA_L2C_DISCONN_IND *)p_buf);
        break;

    case L2C_DISCONN_RSP:
        rdtp_handle_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)p_buf);
        break;

    default:
        break;
    }
}

bool rdtp_init(P_RDTP_CBACK cback)
{
    int32_t ret = 0;

    p_rdtp = os_mem_zalloc2(sizeof(T_RDTP));
    if (p_rdtp == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    if (mpa_reg_l2c_proto(PSM_RDTP, rdtp_l2c_cback, &p_rdtp->queue_id) == false)
    {
        ret = 2;
        goto fail_reg_l2c;
    }

    p_rdtp->cback = cback;
    return true;

fail_reg_l2c:
    os_mem_free(p_rdtp);
    p_rdtp = NULL;
fail_alloc_db:
    PROTOCOL_PRINT_ERROR1("rdtp_init: failed %d", -ret);
    return false;
}
