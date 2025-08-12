/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_ATT_SUPPORT == 1)
#include <string.h>
#include "trace.h"
#include "att_br.h"
#include "bt_att.h"
#include "bt_roleswap.h"

P_BT_ATT_CBACK callback = NULL;

bool bt_att_connect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        return att_conn_req(bd_addr);
    }

    return false;
}

bool bt_att_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return att_disconn_req(bd_addr);
    }

    return false;
}

void bt_att_cback(uint8_t    bd_addr[6],
                  T_ATT_MSG  msg_type,
                  void      *msg_buf)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE1("bt_att_cback: msg_type 0x%02x", msg_type);

    switch (msg_type)
    {
    case ATT_MSG_DATA_IND:
        if (p_link != NULL)
        {

        }
        break;

    case ATT_MSG_CONNECTED:
        if (p_link != NULL)
        {
            bt_roleswap_handle_bt_att_conn(bd_addr);

            if (callback)
            {
                callback(bd_addr, BT_ATT_MSG_CONN_CMPL, NULL);
            }
        }
        break;

    case ATT_MSG_CONN_FAIL:
        if (p_link != NULL)
        {
            if (callback)
            {
                uint16_t cause;

                cause = *(uint16_t *)msg_buf;
                callback(bd_addr, BT_ATT_MSG_CONN_FAIL, &cause);
            }
        }
        break;

    case ATT_MSG_DISCONNECTED:
        if (p_link != NULL)
        {
            uint16_t cause;

            cause = *(uint16_t *)msg_buf;

            bt_roleswap_handle_bt_att_disconn(bd_addr, cause);

            if (callback)
            {
                callback(bd_addr, BT_ATT_MSG_DISCONN_CMPL, &cause);
            }
        }
        break;

    default:
        break;
    }
}

bool bt_att_init(P_BT_ATT_CBACK cback)
{
    callback = cback;
    return att_init(bt_att_cback);
}
#else
#include <stdint.h>
#include <stdbool.h>
#include "bt_att.h"

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
#endif
