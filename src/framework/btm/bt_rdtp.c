/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "rdtp.h"

bool bt_rdtp_connect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & RDTP_PROFILE_MASK) == 0)
        {
            return rdtp_conn_req(bd_addr);
        }
    }

    return false;
}

bool bt_rdtp_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return rdtp_disconn_req(bd_addr);
    }

    return false;
}

bool bt_rdtp_data_send(uint8_t   bd_addr[6],
                       void     *buf,
                       uint16_t  len,
                       bool      flush)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & RDTP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return rdtp_data_send(bd_addr, buf, len, flush);
        }
    }

    return false;
}

void bt_rdtp_cback(uint8_t     bd_addr[6],
                   T_RDTP_MSG  msg_type,
                   void       *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = msg_buf;

    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_INFO1("bt_rdtp_cback: msg_type 0x%02x", msg_type);

    switch (msg_type)
    {
    case RDTP_MSG_DATA_IND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_RDTP_DATA_IND, &payload);
        }
        break;

    case RDTP_MSG_DATA_RSP:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_RDTP_DATA_RSP, &payload);
        }
        break;

    case RDTP_MSG_CONNECTED:
        if (p_link != NULL)
        {
            p_link->connected_profile |= RDTP_PROFILE_MASK;
            btm_db.remote_link = p_link;

            bt_mgr_dispatch(BT_MSG_RDTP_CONN_CMPL, &payload);
        }
        break;

    case RDTP_MSG_CONN_FAIL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_RDTP_CONN_FAIL, &payload);
        }
        break;

    case RDTP_MSG_DISCONNECTED:
        if (p_link != NULL)
        {
            p_link->connected_profile &= ~RDTP_PROFILE_MASK;
            btm_db.remote_link = NULL;

            bt_mgr_dispatch(BT_MSG_RDTP_DISCONN_CMPL, &payload);
        }
        break;

    default:
        break;
    }
}

bool bt_rdtp_init(void)
{
    return rdtp_init(bt_rdtp_cback);
}
