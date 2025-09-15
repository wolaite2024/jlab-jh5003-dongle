/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "mpa.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

#define BT_RDTP_MTU_SIZE        1017
#define BT_RDTP_FLUSH_TOUT      40

typedef enum t_bt_rdtp_state
{
    BT_RDTP_STATE_DISCONNECTED  = 0x00,
    BT_RDTP_STATE_CONNECTING    = 0x01,
    BT_RDTP_STATE_CONNECTED     = 0x02,
    BT_RDTP_STATE_DISCONNECTING = 0x03,
} T_BT_RDTP_STATE;

typedef struct t_bt_rdtp_link
{
    struct t_bt_rdtp_link *next;
    uint8_t                bd_addr[6];
    uint16_t               cid;
    T_BT_RDTP_STATE        state;
} T_BT_RDTP_LINK;

typedef struct t_bt_rdtp
{
    T_OS_QUEUE      list;
    uint16_t        data_offset;
    uint8_t         queue_id;
} T_BT_RDTP;

static T_BT_RDTP *bt_rdtp = NULL;

T_BT_RDTP_LINK *bt_rdtp_link_alloc(uint8_t bd_addr[6])
{
    T_BT_RDTP_LINK *link;

    link = malloc(sizeof(T_BT_RDTP_LINK));
    if (link != NULL)
    {
        link->state = BT_RDTP_STATE_DISCONNECTED;
        link->cid   = 0;
        memcpy(link->bd_addr, bd_addr, 6);

        os_queue_in(&bt_rdtp->list, link);
    }

    return link;
}

void bt_rdtp_link_free(T_BT_RDTP_LINK *link)
{
    if (os_queue_delete(&bt_rdtp->list, link) == true)
    {
        free(link);
    }
}

T_BT_RDTP_LINK *bt_rdtp_link_find_by_addr(uint8_t bd_addr[6])
{
    T_BT_RDTP_LINK *link;

    link = os_queue_peek(&bt_rdtp->list, 0);
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

T_BT_RDTP_LINK *bt_rdtp_link_find_by_cid(uint16_t cid)
{
    T_BT_RDTP_LINK *link;

    link = os_queue_peek(&bt_rdtp->list, 0);
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

static void bt_rdtp_handle_conn_ind(T_MPA_L2C_CONN_IND *conn_ind)
{
    T_BT_RDTP_LINK           *link;
    T_MPA_L2C_CONN_CFM_CAUSE  cause = MPA_L2C_CONN_INVALID_PARAM;

    link = bt_rdtp_link_find_by_addr(conn_ind->bd_addr);
    if (link == NULL)
    {
        link = bt_rdtp_link_alloc(conn_ind->bd_addr);
    }

    if (link != NULL)
    {
        if (link->state == BT_RDTP_STATE_DISCONNECTED)
        {
            cause = MPA_L2C_CONN_ACCEPT;

            link->cid   = conn_ind->cid;
            link->state = BT_RDTP_STATE_CONNECTING;
        }
        else if (link->state == BT_RDTP_STATE_CONNECTING)
        {
            cause = MPA_L2C_CONN_NO_RESOURCE;
        }
        else if (link->state == BT_RDTP_STATE_CONNECTED)
        {
            cause = MPA_L2C_CONN_INVALID_SOURCE_CID;
        }
    }

    mpa_send_l2c_conn_cfm(cause,
                          conn_ind->cid,
                          BT_RDTP_MTU_SIZE,
                          MPA_L2C_MODE_BASIC,
                          BT_RDTP_FLUSH_TOUT);
}

static void bt_rdtp_handle_conn_rsp(T_MPA_L2C_CONN_RSP *conn_rsp)
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_addr(conn_rsp->bd_addr);
    if (link != NULL)
    {
        if (conn_rsp->cause == 0)
        {
            if (link->state == BT_RDTP_STATE_CONNECTING)
            {
                link->cid = conn_rsp->cid;
            }
        }
        else
        {
            if (link->state == BT_RDTP_STATE_CONNECTING)
            {
                T_BT_MSG_PAYLOAD payload;

                bt_rdtp_link_free(link);

                memcpy(payload.bd_addr, conn_rsp->bd_addr, 6);
                payload.msg_buf = &conn_rsp->cause;
                bt_mgr_dispatch(BT_MSG_RDTP_CONN_FAIL, &payload);
            }
        }
    }
}

static void bt_rdtp_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *conn_cmpl)
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_cid(conn_cmpl->cid);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, conn_cmpl->bd_addr, 6);

        if (conn_cmpl->cause == 0)
        {
            link->state          = BT_RDTP_STATE_CONNECTED;
            bt_rdtp->data_offset = conn_cmpl->ds_data_offset;

            payload.msg_buf = &conn_cmpl->remote_mtu;
            bt_mgr_dispatch(BT_MSG_RDTP_CONN_CMPL, &payload);
        }
        else
        {
            if (link->state == BT_RDTP_STATE_CONNECTING)
            {
                bt_rdtp_link_free(link);

                payload.msg_buf = &conn_cmpl->cause;
                bt_mgr_dispatch(BT_MSG_RDTP_CONN_FAIL, &payload);
            }
        }
    }
}

static void bt_rdtp_handle_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_cid(data_ind->cid);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD   payload;
        T_BT_RDTP_DATA_IND ind;

        ind.buf = data_ind->data + data_ind->gap;
        ind.len = data_ind->length;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &ind;
        bt_mgr_dispatch(BT_MSG_RDTP_DATA_IND, &payload);
    }
}

static void bt_rdtp_handle_data_rsp(T_MPA_L2C_DATA_RSP *data_rsp)
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_cid(data_rsp->cid);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = NULL;
        bt_mgr_dispatch(BT_MSG_RDTP_DATA_RSP, &payload);
    }
}

static void bt_rdtp_handle_disconn_ind(T_MPA_L2C_DISCONN_IND *disconn_ind)
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_cid(disconn_ind->cid);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &disconn_ind->cause;
        bt_mgr_dispatch(BT_MSG_RDTP_DISCONN_CMPL, &payload);

        bt_rdtp_link_free(link);
    }

    mpa_send_l2c_disconn_cfm(disconn_ind->cid);
}

static void bt_rdtp_handle_disconn_rsp(T_MPA_L2C_DISCONN_RSP *disconn_rsp)
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_cid(disconn_rsp->cid);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &disconn_rsp->cause;
        bt_mgr_dispatch(BT_MSG_RDTP_DISCONN_CMPL, &payload);

        bt_rdtp_link_free(link);
    }
}

static void bt_rdtp_cback(void        *p_buf,
                          T_PROTO_MSG  msg)
{
    switch (msg)
    {
    case L2C_CONN_IND:
        bt_rdtp_handle_conn_ind((T_MPA_L2C_CONN_IND *)p_buf);
        break;

    case L2C_CONN_RSP:
        bt_rdtp_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)p_buf);
        break;

    case L2C_CONN_CMPL:
        bt_rdtp_handle_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)p_buf);
        break;

    case L2C_DATA_IND:
        bt_rdtp_handle_data_ind((T_MPA_L2C_DATA_IND *)p_buf);
        break;

    case L2C_DATA_RSP:
        bt_rdtp_handle_data_rsp((T_MPA_L2C_DATA_RSP *)p_buf);
        break;

    case L2C_DISCONN_IND:
        bt_rdtp_handle_disconn_ind((T_MPA_L2C_DISCONN_IND *)p_buf);
        break;

    case L2C_DISCONN_RSP:
        bt_rdtp_handle_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)p_buf);
        break;

    default:
        break;
    }
}

bool bt_rdtp_init(void)
{
    int32_t ret = 0;

    bt_rdtp = calloc(1, sizeof(T_BT_RDTP));
    if (bt_rdtp == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    if (mpa_reg_l2c_proto(PSM_RDTP, bt_rdtp_cback, &bt_rdtp->queue_id) == false)
    {
        ret = 2;
        goto fail_reg_cback;
    }

    return true;

fail_reg_cback:
    free(bt_rdtp);
    bt_rdtp = NULL;
fail_alloc_db:
    PROFILE_PRINT_ERROR1("bt_rdtp_init: failed %d", -ret);
    return false;
}

bool bt_rdtp_connect_req(uint8_t bd_addr[6])
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_rdtp_link_alloc(bd_addr);
    }

    if (link != NULL)
    {
        if (link->state == BT_RDTP_STATE_DISCONNECTED)
        {
            PROFILE_PRINT_TRACE1("bt_rdtp_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            link->state = BT_RDTP_STATE_CONNECTING;

            bt_sniff_mode_exit(bd_addr, false);
            mpa_send_l2c_conn_req(PSM_RDTP, UUID_RDTP, bt_rdtp->queue_id, BT_RDTP_MTU_SIZE,
                                  bd_addr, MPA_L2C_MODE_BASIC, BT_RDTP_FLUSH_TOUT);
            return true;
        }
    }

    return false;
}

bool bt_rdtp_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_addr(bd_addr);
    if (link != NULL)
    {
        PROFILE_PRINT_TRACE1("bt_rdtp_disconnect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

        link->state = BT_RDTP_STATE_DISCONNECTING;

        bt_sniff_mode_exit(bd_addr, false);
        mpa_send_l2c_disconn_req(link->cid);
        return true;
    }

    return false;
}

bool bt_rdtp_data_send(uint8_t   bd_addr[6],
                       void     *buf,
                       uint16_t  len,
                       bool      flush)
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->state == BT_RDTP_STATE_CONNECTED)
        {
            uint8_t *pkt;

            pkt = mpa_get_l2c_buf(bt_rdtp->queue_id, link->cid, 0, len, bt_rdtp->data_offset, true);
            if (pkt != NULL)
            {
                bt_sniff_mode_exit(bd_addr, false);

                memcpy(pkt + bt_rdtp->data_offset, buf, len);
                mpa_send_l2c_data_req(pkt, bt_rdtp->data_offset, link->cid, len, flush);
                return true;
            }
        }
    }

    return false;
}

bool bt_rdtp_set_roleswap_info(uint8_t prev_bd_addr[6],
                               uint8_t curr_bd_addr[6])
{
    T_BT_RDTP_LINK *link;

    link = bt_rdtp_link_find_by_addr(prev_bd_addr);
    if (link != NULL)
    {
        memcpy(link->bd_addr, curr_bd_addr, 6);
        return true;
    }

    return false;
}
