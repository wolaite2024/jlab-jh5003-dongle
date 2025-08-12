/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "bt_types.h"
#include "opp.h"
#include "bt_opp.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

void bt_opp_cback(uint8_t    bd_addr[6],
                  T_OPP_MSG  msg,
                  void      *msg_buf)
{
    T_BT_BR_LINK     *p_link;

    BTM_PRINT_INFO2("bt_opp_cback: bd_addr %s, msg 0x%02x", TRACE_BDADDR(bd_addr), msg);

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, bd_addr, 6);
        payload.msg_buf = msg_buf;

        switch (msg)
        {
        case OPP_MSG_CONN_IND:
            {
                bt_mgr_dispatch(BT_MSG_OPP_CONN_IND, &payload);
            }
            break;

        case OPP_MSG_CONNECTED:
            {
                p_link->connected_profile |= OPP_PROFILE_MASK;
                bt_mgr_dispatch(BT_MSG_OPP_CONN_CMPL, &payload);
            }
            break;

        case OPP_MSG_CONN_FAIL:
            {
                bt_mgr_dispatch(BT_MSG_OPP_CONN_FAIL, &payload);
            }
            break;

        case OPP_MSG_DISCONNECTED:
            {
                p_link->connected_profile &= ~OPP_PROFILE_MASK;
                bt_mgr_dispatch(BT_MSG_OPP_DISCONN_CMPL, &payload);
            }
            break;

        case OPP_MSG_DATA_HEADER_IND:
            {
                bt_mgr_dispatch(BT_MSG_OPP_DATA_HEADER_IND, &payload);
            }
            break;

        case OPP_MSG_DATA_IND:
            {
                bt_mgr_dispatch(BT_MSG_OPP_DATA_IND, &payload);
            }
            break;

        case OPP_MSG_DATA_RSP:
            {
                bt_mgr_dispatch(BT_MSG_OPP_DATA_RSP, &payload);
            }
            break;

        default:
            break;
        }
    }
}

bool bt_opp_connect_cfm(uint8_t bd_addr[6],
                        bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return opp_connect_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_opp_connect_over_rfc_req(uint8_t bd_addr[6],
                                 uint8_t server_chann)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & OPP_PROFILE_MASK) == 0)
        {
            bt_sniff_mode_exit(p_link, false);
            return opp_conn_over_rfc(bd_addr, server_chann);
        }
    }

    return false;
}

bool bt_opp_connect_over_l2c_req(uint8_t  bd_addr[6],
                                 uint16_t l2c_psm)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & OPP_PROFILE_MASK) == 0)
        {
            bt_sniff_mode_exit(p_link, false);
            return opp_conn_over_l2c(bd_addr, l2c_psm);
        }
    }

    return false;
}

bool bt_opp_push_data_header_req(uint8_t   bd_addr[6],
                                 uint32_t  total_len,
                                 uint8_t  *name,
                                 uint16_t  name_len,
                                 uint8_t  *type,
                                 uint16_t  type_len,
                                 bool      srm_enable)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & OPP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return opp_push_data_header_req(bd_addr, total_len, name, name_len, type, type_len, srm_enable);
        }
    }

    return false;
}

bool bt_opp_push_data_req(uint8_t   bd_addr[6],
                          uint8_t  *data,
                          uint16_t  data_len,
                          bool      more_data)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & OPP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return opp_push_data_req(bd_addr, data, data_len, more_data);
        }
    }

    return false;
}

bool bt_opp_send_data_rsp(uint8_t                bd_addr[6],
                          T_BT_OPP_RESPONSE_CODE rsp_code)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & OPP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return opp_send_rsp(bd_addr, rsp_code);
        }
    }

    return false;
}

bool bt_opp_push_abort(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & OPP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return opp_push_abort(bd_addr);
        }
    }

    return false;
}

bool bt_opp_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return opp_disconnect_req(bd_addr);
    }

    return false;
}

bool bt_opp_init(uint8_t  link_num,
                 uint8_t  server_chann,
                 uint16_t l2c_psm)
{
    return opp_init(link_num, bt_opp_cback, server_chann, l2c_psm);
}
