/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_AVP_SUPPORT == 1)
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "bt_mgr.h"
#include "bt_roleswap.h"
#include "bt_avp.h"
#include "bt_avp_int.h"

#define BT_AVP_MTU_SIZE       1017
#define BT_AVP_FLUSH_TOUT     40
#define PSM_AVP_CONTROL       0x1001
#define UUID_AVP_CONTROL      0x1001

typedef enum t_bt_avp_state
{
    BT_AVP_STATE_DISCONNECTED  = 0x00,
    BT_AVP_STATE_CONNECTING    = 0x01,
    BT_AVP_STATE_CONNECTED     = 0x02,
    BT_AVP_STATE_DISCONNECTING = 0x03,
} T_BT_AVP_STATE;

typedef struct t_bt_avp_link
{
    struct t_bt_avp_link *next;
    uint8_t               bd_addr[6];
    uint16_t              cid;
    uint16_t              data_offset;
    T_BT_AVP_STATE        state;
} T_BT_AVP_LINK;

typedef struct t_bt_avp
{
    T_OS_QUEUE         list;
    uint8_t            queue_id;
    P_BT_AVP_CBACK     cback;
} T_BT_AVP;

static T_BT_AVP *bt_avp;

T_BT_AVP_LINK *bt_avp_alloc_link(uint8_t bd_addr[6])
{
    T_BT_AVP_LINK *link;

    link = calloc(1, sizeof(T_BT_AVP_LINK));
    if (link != NULL)
    {
        link->state = BT_AVP_STATE_DISCONNECTED;
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_in(&bt_avp->list, link);
    }

    return link;
}

void bt_avp_free_link(T_BT_AVP_LINK *link)
{
    if (os_queue_delete(&bt_avp->list, link) == true)
    {
        free(link);
    }
}

T_BT_AVP_LINK *bt_avp_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_AVP_LINK *link;

    link = os_queue_peek(&bt_avp->list, 0);
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

T_BT_AVP_LINK *bt_avp_find_link_by_cid(uint16_t cid)
{
    T_BT_AVP_LINK *link;

    link = os_queue_peek(&bt_avp->list, 0);
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

bool bt_avp_data_send(uint8_t   bd_addr[6],
                      uint8_t  *data,
                      uint16_t  data_len,
                      bool      flush)
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->state == BT_AVP_STATE_CONNECTED)
        {
            uint8_t *pkt;

            pkt = mpa_get_l2c_buf(bt_avp->queue_id, link->cid, 0, data_len,
                                  link->data_offset, false);
            if (pkt != NULL)
            {
                memcpy(pkt + link->data_offset, data, data_len);
                mpa_send_l2c_data_req(pkt, link->data_offset,
                                      link->cid, data_len, flush);
                return true;
            }
        }
    }

    return false;
}

bool bt_avp_connect_req(uint8_t bd_addr[6])
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_avp_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        link->state = BT_AVP_STATE_CONNECTING;
        bt_sniff_mode_exit(bd_addr, false);
        mpa_send_l2c_conn_req(PSM_AVP_CONTROL, UUID_AVP_CONTROL, bt_avp->queue_id,
                              BT_AVP_MTU_SIZE, bd_addr, MPA_L2C_MODE_BASIC, BT_AVP_FLUSH_TOUT);
        return true;
    }

    return false;
}

bool bt_avp_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        if (link->state == BT_AVP_STATE_CONNECTED)
        {
            link->state = BT_AVP_STATE_DISCONNECTING;
            mpa_send_l2c_disconn_req(link->cid);
            return true;
        }
    }

    return false;
}

void bt_avp_handle_data_ind(T_MPA_L2C_DATA_IND *p_data_ind)
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_cid(p_data_ind->cid);
    if (link != NULL)
    {
        T_BT_AVP_EVENT_PARAM param;

        memcpy(param.data_ind.bd_addr, link->bd_addr, 6);
        param.data_ind.data = p_data_ind->data + p_data_ind->gap;
        param.data_ind.len = p_data_ind->length;
        bt_avp->cback(BT_AVP_EVENT_DATA_IND, &param, sizeof(param));
    }
}

void bt_avp_handle_conn_req(T_MPA_L2C_CONN_IND *ind)
{
    T_BT_AVP_LINK            *link;
    T_MPA_L2C_CONN_CFM_CAUSE  cause;

    link = bt_avp_find_link_by_addr(ind->bd_addr);
    if (link == NULL)
    {
        link = bt_avp_alloc_link(ind->bd_addr);
        if (link != NULL)
        {
            link->cid = ind->cid;
            link->state = BT_AVP_STATE_CONNECTING;
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
    mpa_send_l2c_conn_cfm(cause, ind->cid, BT_AVP_MTU_SIZE, MPA_L2C_MODE_BASIC, BT_AVP_FLUSH_TOUT);
}

void bt_avp_handle_conn_rsp(T_MPA_L2C_CONN_RSP *rsp)
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_addr(rsp->bd_addr);
    if (link != NULL)
    {
        if (rsp->cause == 0)
        {
            if (link->state == BT_AVP_STATE_CONNECTING)
            {
                link->cid = rsp->cid;
            }
        }
        else
        {
            bt_avp_free_link(link);
        }
    }
}

void bt_avp_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *info)
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_addr(info->bd_addr);
    if (link != NULL)
    {
        if (info->cause == 0)
        {
            T_BT_AVP_EVENT_PARAM param;

            link->state  = BT_AVP_STATE_CONNECTED;
            link->data_offset = info->ds_data_offset;
            bt_roleswap_handle_bt_avp_conn(link->bd_addr);

            memcpy(param.conn_cmpl.bd_addr, link->bd_addr, 6);
            bt_avp->cback(BT_AVP_EVENT_CONN_CMPL, &param, sizeof(param));
        }
        else
        {
            bt_avp_free_link(link);
        }
    }
}

void bt_avp_handle_disconn_ind(T_MPA_L2C_DISCONN_IND *ind)
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_cid(ind->cid);
    if (link != NULL)
    {
        uint16_t cause;

        cause = ind->cause;
        bt_roleswap_handle_bt_avp_disconn(link->bd_addr, cause);

        if (cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
        {
            T_BT_AVP_EVENT_PARAM param;

            memcpy(param.disconn_cmpl.bd_addr, link->bd_addr, 6);
            bt_avp->cback(BT_AVP_EVENT_DISCONN_CMPL, &param, sizeof(param));
        }

        bt_avp_free_link(link);
    }

    mpa_send_l2c_disconn_cfm(ind->cid);
}

void bt_avp_handle_disconn_rsp(T_MPA_L2C_DISCONN_RSP *rsp)
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_cid(rsp->cid);
    if (link != NULL)
    {
        uint16_t cause;

        cause = rsp->cause;
        bt_roleswap_handle_bt_avp_disconn(link->bd_addr, cause);

        if (cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
        {
            T_BT_AVP_EVENT_PARAM param;

            memcpy(param.disconn_cmpl.bd_addr, link->bd_addr, 6);
            bt_avp->cback(BT_AVP_EVENT_DISCONN_CMPL, &param, sizeof(param));
        }

        bt_avp_free_link(link);
    }
}


void bt_avp_cback(void        *p_buf,
                  T_PROTO_MSG  msg)
{
    BTM_PRINT_INFO1("bt_avp_cback: msg 0x%02x", msg);

    switch (msg)
    {
    case L2C_CONN_IND:
        bt_avp_handle_conn_req((T_MPA_L2C_CONN_IND *)p_buf);
        break;

    case L2C_CONN_RSP:
        bt_avp_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)p_buf);
        break;

    case L2C_CONN_CMPL:
        bt_avp_handle_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)p_buf);
        break;

    case L2C_DATA_IND:
        bt_avp_handle_data_ind((T_MPA_L2C_DATA_IND *)p_buf);
        break;

    case L2C_DISCONN_IND:
        bt_avp_handle_disconn_ind((T_MPA_L2C_DISCONN_IND *)p_buf);
        break;

    case L2C_DISCONN_RSP:
        bt_avp_handle_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)p_buf);
        break;

    default:
        break;
    }
}

bool bt_avp_init(P_BT_AVP_CBACK cback)
{
    int32_t ret = 0;

    bt_avp = calloc(1, sizeof(T_BT_AVP));

    if (bt_avp == NULL)
    {
        ret = 1;
        goto fail_alloc_avp;
    }

    if (mpa_reg_l2c_proto(PSM_AVP_CONTROL, bt_avp_cback, &bt_avp->queue_id) == false)
    {
        ret = 2;
        goto fail_reg_l2c;
    }

    bt_avp->cback = cback;

    return true;

fail_reg_l2c:
    free(bt_avp);
    bt_avp = NULL;
fail_alloc_avp:
    PROFILE_PRINT_ERROR1("avp_init: failed %d", -ret);
    return false;
}

void bt_avp_deinit(void)
{
    if (bt_avp != NULL)
    {
        free(bt_avp);
        bt_avp = NULL;
    }
}

bool bt_avp_roleswap_info_del(uint8_t bd_addr[6])
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_avp_free_link(link);
        return true;
    }

    return false;
}

bool bt_avp_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_AVP_INFO *info)
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        memcpy(info->bd_addr, bd_addr, 6);
        info->l2c_cid     = link->cid;
        info->state       = link->state;
        info->data_offset = link->data_offset;

        return true;
    }

    return false;
}

bool bt_avp_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_AVP_INFO *info)
{
    T_BT_AVP_LINK *link;

    link = bt_avp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_avp_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);

        link->cid         = info->l2c_cid;
        link->state       = (T_BT_AVP_STATE)info->state;
        link->data_offset = info->data_offset;

        return true;
    }

    return false;
}

#else
#include <stdint.h>
#include <stdbool.h>
#include "bt_avp.h"
#include "bt_avp_int.h"

bool bt_avp_data_send(uint8_t   bd_addr[6],
                      uint8_t  *data,
                      uint16_t  data_len,
                      bool      flush)
{
    return false;
}

bool bt_avp_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avp_init(P_BT_AVP_CBACK cback)
{
    return false;
}

void bt_avp_deinit(void)
{

}

bool bt_avp_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avp_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_AVP_INFO *info)
{
    return false;
}

bool bt_avp_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_AVP_INFO *info)
{
    return false;
}
#endif
