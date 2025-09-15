/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_ATT_SUPPORT == 1)
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "bt_att.h"
#include "bt_att_int.h"
#include "bt_roleswap.h"

#define BT_ATT_MTU_SIZE       1017
#define BT_ATT_FLUSH_TOUT     40

typedef enum t_bt_att_state
{
    BT_ATT_STATE_DISCONNECTED  = 0x00,
    BT_ATT_STATE_CONNECTING    = 0x01,
    BT_ATT_STATE_CONNECTED     = 0x02,
    BT_ATT_STATE_DISCONNECTING = 0x03,
} T_BT_ATT_STATE;

typedef struct t_bt_att_link
{
    struct t_bt_att_link  *next;
    uint8_t                bd_addr[6];
    uint16_t               cid;
    T_BT_ATT_STATE         state;
    uint16_t               data_offset;
    uint16_t               remote_mtu;
    uint16_t               local_mtu;
    uint16_t               conn_handle;
} T_BT_ATT_LINK;

typedef struct t_bt_att
{
    T_OS_QUEUE         link_list;
    P_BT_ATT_CBACK     callback;
    uint8_t            queue_id;
} T_BT_ATT;

static T_BT_ATT *bt_att;

T_BT_ATT_LINK *bt_att_alloc_link(uint8_t bd_addr[6])
{
    T_BT_ATT_LINK *link;

    link = calloc(1, (sizeof(T_BT_ATT_LINK)));
    if (link != NULL)
    {
        link->state = BT_ATT_STATE_DISCONNECTED;
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_in(&bt_att->link_list, link);
    }

    return link;
}

void bt_att_free_link(T_BT_ATT_LINK *link)
{
    if (os_queue_delete(&bt_att->link_list, link) == true)
    {
        free(link);
    }
}

T_BT_ATT_LINK *bt_att_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_ATT_LINK *link;

    link = os_queue_peek(&bt_att->link_list, 0);
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

T_BT_ATT_LINK *bt_att_find_link_by_cid(uint16_t cid)
{
    T_BT_ATT_LINK *link;

    link = os_queue_peek(&bt_att->link_list, 0);
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

bool bt_att_connect_req(uint8_t bd_addr[6])
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_att_alloc_link(bd_addr);
        if (link != NULL)
        {
            link->state = BT_ATT_STATE_CONNECTING;
            bt_sniff_mode_exit(bd_addr, false);
            mpa_send_l2c_conn_req(PSM_ATT, UUID_ATT, bt_att->queue_id, BT_ATT_MTU_SIZE,
                                  bd_addr, MPA_L2C_MODE_BASIC, BT_ATT_FLUSH_TOUT);
            return true;
        }
    }

    return false;
}

bool bt_att_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->state == BT_ATT_STATE_CONNECTED)
        {
            bt_sniff_mode_exit(bd_addr, false);
            mpa_send_l2c_disconn_req(link->cid);

            return true;
        }
    }

    return false;
}

void bt_att_handle_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_cid(data_ind->cid);
    if (link != NULL)
    {
        T_BT_ATT_DATA_IND rx_data;

        rx_data.buf = data_ind->data + data_ind->gap;
        rx_data.len = data_ind->length;
        bt_att->callback(link->bd_addr, BT_ATT_MSG_DATA_IND, &rx_data);
    }
}

void bt_att_handle_conn_req(T_MPA_L2C_CONN_IND *conn_ind)
{
    T_BT_ATT_LINK            *link;
    T_MPA_L2C_CONN_CFM_CAUSE  cause;

    link = bt_att_find_link_by_addr(conn_ind->bd_addr);
    if (link == NULL)
    {
        link = bt_att_alloc_link(conn_ind->bd_addr);
        if (link != NULL)
        {
            link->cid = conn_ind->cid;
            link->state = BT_ATT_STATE_CONNECTING;
            cause = MPA_L2C_CONN_ACCEPT;
        }
        else
        {
            cause = MPA_L2C_CONN_NO_RESOURCE;
        }
    }
    else
    {
        cause = MPA_L2C_CONN_NO_RESOURCE;
    }

    mpa_send_l2c_conn_cfm(cause, conn_ind->cid, BT_ATT_MTU_SIZE,
                          MPA_L2C_MODE_BASIC, BT_ATT_FLUSH_TOUT);
}

void bt_att_handle_conn_rsp(T_MPA_L2C_CONN_RSP *conn_rsp)
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_addr(conn_rsp->bd_addr);
    if (link != NULL)
    {
        if (conn_rsp->cause == 0)
        {
            link->cid = conn_rsp->cid;
        }
        else
        {
            uint16_t cause;

            cause = conn_rsp->cause;
            bt_att_free_link(link);
            bt_att->callback(conn_rsp->bd_addr, BT_ATT_MSG_CONN_FAIL, &cause);
        }
    }
}

void bt_att_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *conn_cmpl)
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_addr(conn_cmpl->bd_addr);
    if (link != NULL)
    {
        if (conn_cmpl->cause)
        {
            uint16_t cause;

            cause = conn_cmpl->cause;
            bt_att_free_link(link);
            bt_att->callback(conn_cmpl->bd_addr, BT_ATT_MSG_CONN_FAIL, &cause);
        }
        else
        {
            T_BT_ATT_CONN_CMPL param;

            link->state  = BT_ATT_STATE_CONNECTED;
            link->data_offset = conn_cmpl->ds_data_offset;
            link->remote_mtu = conn_cmpl->remote_mtu;
            link->local_mtu = conn_cmpl->local_mtu;
            link->conn_handle = conn_cmpl->conn_handle;
            bt_roleswap_handle_bt_att_conn(conn_cmpl->bd_addr);
            param.conn_handle = link->conn_handle;
            param.cid = link->cid;
            param.mtu_size = ((link->remote_mtu < link->local_mtu) ? link->remote_mtu : link->local_mtu);
            bt_att->callback(conn_cmpl->bd_addr, BT_ATT_MSG_CONN_CMPL, &param);
        }
    }
}

void bt_att_handle_disconn_ind(T_MPA_L2C_DISCONN_IND *disconn_ind)
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_cid(disconn_ind->cid);
    if (link != NULL)
    {
        uint16_t cause;
        uint8_t  bd_addr[6];

        cause = disconn_ind->cause;
        memcpy(bd_addr, link->bd_addr, 6);
        bt_att_free_link(link);
        bt_roleswap_handle_bt_att_disconn(bd_addr, cause);
        bt_att->callback(bd_addr, BT_ATT_MSG_DISCONN_CMPL, &cause);
    }

    mpa_send_l2c_disconn_cfm(disconn_ind->cid);
}

void bt_att_handle_disconn_rsp(T_MPA_L2C_DISCONN_RSP *disconn_rsp)
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_cid(disconn_rsp->cid);
    if (link != NULL)
    {
        uint16_t cause;
        uint8_t  bd_addr[6];

        cause = disconn_rsp->cause;
        memcpy(bd_addr, link->bd_addr, 6);
        bt_att_free_link(link);
        bt_roleswap_handle_bt_att_disconn(bd_addr, cause);
        bt_att->callback(bd_addr, BT_ATT_MSG_DISCONN_CMPL, &cause);
    }
}

void bt_att_l2c_cback(void        *buf,
                      T_PROTO_MSG  msg)
{
    PROTOCOL_PRINT_TRACE1("bt_att_l2c_cback: msg 0x%02x", msg);

    switch (msg)
    {
    case L2C_CONN_IND:
        bt_att_handle_conn_req((T_MPA_L2C_CONN_IND *)buf);
        break;

    case L2C_CONN_RSP:
        bt_att_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)buf);
        break;

    case L2C_CONN_CMPL:
        bt_att_handle_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)buf);
        break;

    case L2C_DATA_IND:
        bt_att_handle_data_ind((T_MPA_L2C_DATA_IND *)buf);
        break;

    case L2C_DISCONN_IND:
        bt_att_handle_disconn_ind((T_MPA_L2C_DISCONN_IND *)buf);
        break;

    case L2C_DISCONN_RSP:
        bt_att_handle_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)buf);
        break;

    default:
        break;
    }
}

bool bt_att_init(P_BT_ATT_CBACK cback)
{
    int32_t ret = 0;

    bt_att = calloc(1, sizeof(T_BT_ATT));
    if (bt_att == NULL)
    {
        ret = 1;
        goto fail_alloc_att;
    }

    mpa_set_att_psm_data_path(MPA_ATT_PSM_DATA_PATH_GATT);

    if (mpa_reg_l2c_proto(PSM_ATT, bt_att_l2c_cback, &bt_att->queue_id) == false)
    {
        ret = 2;
        goto fail_reg_l2c;
    }

    bt_att->callback = cback;
    return true;

fail_reg_l2c:
    free(bt_att);
    bt_att = NULL;
fail_alloc_att:
    PROFILE_PRINT_ERROR1("bt_att_init: failed %d", -ret);
    return false;
}

bool bt_att_roleswap_info_del(uint8_t bd_addr[6])
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_att_free_link(link);
        return true;
    }

    return false;
}

bool bt_att_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_ATT_INFO *info)
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        memcpy(info->bd_addr, bd_addr, 6);
        info->l2c_cid = link->cid;
        info->state = link->state;
        info->data_offset = link->data_offset;
        info->remote_mtu = link->remote_mtu;
        info->local_mtu = link->local_mtu;
        info->conn_handle = link->conn_handle;
        return true;
    }

    return false;
}

bool bt_att_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_ATT_INFO *info)
{
    T_BT_ATT_LINK *link;

    link = bt_att_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_att_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);

        link->cid = info->l2c_cid;
        link->state = (T_BT_ATT_STATE)info->state;
        link->data_offset = info->data_offset;
        link->remote_mtu = info->remote_mtu;
        link->local_mtu = info->local_mtu;
        link->conn_handle = info->conn_handle;

        return true;
    }

    return false;
}

#else
#include <stdint.h>
#include <stdbool.h>
#include "bt_att.h"
#include "bt_att_int.h"

bool bt_att_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_att_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_att_init(P_BT_ATT_CBACK cback)
{
    return false;
}

bool bt_att_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}

bool bt_att_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_ATT_INFO *info)
{
    return false;
}

bool bt_att_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_ATT_INFO *info)
{
    return false;
}
#endif
