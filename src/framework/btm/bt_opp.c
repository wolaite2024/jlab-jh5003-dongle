/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "obex.h"
#include "bt_opp.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

typedef enum t_bt_opp_state
{
    BT_OPP_STATE_DISCONNECTED       = 0x00,
    BT_OPP_STATE_CONNECTING         = 0x01,
    BT_OPP_STATE_CONNECTED          = 0x02,
    BT_OPP_STATE_DISCONNECTING      = 0x03,
} T_BT_OPP_STATE;

typedef struct t_bt_opp_link
{
    struct t_bt_opp_link *next;
    uint8_t               bd_addr[6];
    uint8_t               srm_status;
    T_BT_OPP_STATE        state;
    T_OBEX_HANDLE         handle;
    bool                  obex_over_l2c;
} T_BT_OPP_LINK;

typedef struct t_bt_opp
{
    T_OS_QUEUE      list;
} T_BT_OPP;

static T_BT_OPP *bt_opp;

static const uint8_t count_hender[6] =
{
    /* indicate total length: 5 */
    0x05,
    OBEX_HI_COUNT,
    /* Count of Objects: 1 */
    0x00, 0x00, 0x00, 0x01
};

T_BT_OPP_LINK *bt_opp_alloc_link(uint8_t bd_addr[6])
{
    T_BT_OPP_LINK *link;

    link = malloc(sizeof(T_BT_OPP_LINK));
    if (link != NULL)
    {
        link->state = BT_OPP_STATE_DISCONNECTED;
        link->handle = NULL;
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_in(&bt_opp->list, link);
    }

    return link;
}

void bt_opp_free_link(T_BT_OPP_LINK *link)
{
    if (os_queue_delete(&bt_opp->list, link) == true)
    {
        free(link);
    }
}

T_BT_OPP_LINK *bt_opp_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_OPP_LINK *link;

    link = os_queue_peek(&bt_opp->list, 0);
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

void bt_opp_obex_cback(uint8_t  msg_type,
                       void    *p_msg)
{
    T_BT_OPP_LINK    *p_link;
    T_BT_MSG_PAYLOAD  payload;

    BTM_PRINT_INFO1("bt_opp_obex_cback: msg_type 0x%02x", msg_type);

    switch (msg_type)
    {
    case OBEX_MSG_CONN_IND:
        {
            T_OBEX_CONN_IND_DATA *p_data = (T_OBEX_CONN_IND_DATA *)p_msg;

            p_link = bt_opp_find_link_by_addr(p_data->bd_addr);
            if (p_link == NULL)
            {
                p_link = bt_opp_alloc_link(p_data->bd_addr);
                if (p_link != NULL)
                {
                    p_link->state = BT_OPP_STATE_CONNECTING;
                    p_link->handle = p_data->handle;
                    memcpy(payload.bd_addr, p_data->bd_addr, 6);
                    bt_mgr_dispatch(BT_MSG_OPP_CONN_IND, &payload);
                }
                else
                {
                    obex_conn_cfm(p_data->handle, false, NULL, 0);
                }
            }
            else
            {
                obex_conn_cfm(p_data->handle, false, NULL, 0);
            }
        }
        break;

    case OBEX_MSG_CONN_CMPL:
        {
            T_OBEX_CONN_CMPL_DATA *p_data = (T_OBEX_CONN_CMPL_DATA *)p_msg;

            p_link = bt_opp_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                p_link->state = BT_OPP_STATE_CONNECTED;
                p_link->obex_over_l2c = p_data->obex_over_l2c;
                memcpy(payload.bd_addr, p_data->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_OPP_CONN_CMPL, &payload);
            }
            else
            {
                obex_disconn_req(p_data->handle);
            }
        }
        break;

    case OBEX_MSG_PUT_DONE:
        {
            T_OBEX_PUT_CMPL_DATA *p_data = (T_OBEX_PUT_CMPL_DATA *)p_msg;

            p_link = bt_opp_find_link_by_addr(p_data->bd_addr);
            if (p_link && p_link->handle == p_data->handle)
            {
                p_link->srm_status = p_data->srm_status;
                memcpy(payload.bd_addr, p_data->bd_addr, 6);
                payload.msg_buf = &p_data->cause;
                bt_mgr_dispatch(BT_MSG_OPP_DATA_RSP, &payload);
            }
        }
        break;

    case OBEX_MSG_PUT_DATA_RSP:
        {
            T_OBEX_PUT_DATA_RSP *p_data = (T_OBEX_PUT_DATA_RSP *)p_msg;

            p_link = bt_opp_find_link_by_addr(p_data->bd_addr);
            if (p_link && p_link->handle == p_data->handle)
            {
                if (p_link->srm_status == SRM_ENABLE)
                {
                    uint8_t cause;

                    cause = OBEX_RSP_CONTINUE;
                    memcpy(payload.bd_addr, p_data->bd_addr, 6);
                    payload.msg_buf = &cause;
                    bt_mgr_dispatch(BT_MSG_OPP_DATA_RSP, &payload);
                }
            }
        }
        break;

    case OBEX_MSG_REMOTE_PUT:
        {
            bool     ret;
            uint8_t *p_hdr_data;
            uint16_t hdr_data_len;
            T_BT_OPP_DATA_IND opp_data;
            T_BT_OPP_DATA_HEADER_IND header;
            T_OBEX_REMOTE_PUT_DATA *p_data = (T_OBEX_REMOTE_PUT_DATA *)p_msg;

            p_link = bt_opp_find_link_by_addr(p_data->bd_addr);
            if (p_link == NULL || p_link->handle != p_data->handle)
            {
                break;
            }

            p_link->srm_status = p_data->srm_status;

            ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_NAME, &p_hdr_data,
                                       &hdr_data_len);
            if (ret == true)
            {
                header.name = p_hdr_data;
                header.name_len = hdr_data_len;

                ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_TYPE, &p_hdr_data,
                                           &hdr_data_len);
                if (ret == true)
                {
                    header.type = p_hdr_data;
                    header.type_len = hdr_data_len;
                }

                ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_LENGTH, &p_hdr_data,
                                           &hdr_data_len);
                if (ret == true)
                {
                    header.total_len = (*(uint8_t *)p_hdr_data << 24) + (*(uint8_t *)(p_hdr_data + 1) << 16) +
                                       (*(uint8_t *)(p_hdr_data + 2) << 8) + (*(uint8_t *)(p_hdr_data + 3));
                }

                memcpy(payload.bd_addr, p_data->bd_addr, 6);
                payload.msg_buf = &header;
                bt_mgr_dispatch(BT_MSG_OPP_DATA_HEADER_IND, &payload);
            }

            ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &p_hdr_data,
                                       &hdr_data_len);
            if (ret == false)
            {
                ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &p_hdr_data,
                                           &hdr_data_len);
                if (ret)
                {
                    opp_data.data_end = true;
                }
            }
            else
            {
                opp_data.data_end = false;
            }

            if (ret == false)
            {
                break;
            }

            opp_data.p_data = p_hdr_data;
            opp_data.data_len = hdr_data_len;

            memcpy(payload.bd_addr, p_data->bd_addr, 6);
            payload.msg_buf = &opp_data;
            bt_mgr_dispatch(BT_MSG_OPP_DATA_IND, &payload);
        }
        break;

    case OBEX_MSG_DISCONN:
        {
            T_OBEX_DISCONN_CMPL_DATA *p_data = (T_OBEX_DISCONN_CMPL_DATA *)p_msg;

            p_link = bt_opp_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                if (p_data->handle == p_link->handle)
                {
                    T_BT_OPP_STATE state;

                    state = p_link->state;
                    bt_opp_free_link(p_link);

                    if (state == BT_OPP_STATE_CONNECTING)
                    {
                        memcpy(payload.bd_addr, p_data->bd_addr, 6);
                        payload.msg_buf = &p_data->cause;
                        bt_mgr_dispatch(BT_MSG_OPP_CONN_FAIL, &payload);
                    }
                    else
                    {
                        memcpy(payload.bd_addr, p_data->bd_addr, 6);
                        payload.msg_buf = &p_data->cause;
                        bt_mgr_dispatch(BT_MSG_OPP_DISCONN_CMPL, &payload);
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

bool bt_opp_connect_cfm(uint8_t bd_addr[6],
                        bool    accept)
{
    T_BT_OPP_LINK *p_link;

    p_link = bt_opp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return obex_conn_cfm(p_link->handle, accept, NULL, 0);
    }

    return false;
}

bool bt_opp_connect_over_rfc_req(uint8_t bd_addr[6],
                                 uint8_t server_chann)
{
    T_BT_OPP_LINK *p_link;

    p_link = bt_opp_find_link_by_addr(bd_addr);
    if (p_link)
    {
        return false;
    }

    p_link = bt_opp_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    bt_sniff_mode_exit(bd_addr, false);

    if (obex_conn_req_over_rfc(bd_addr, (uint8_t *)count_hender, server_chann, bt_opp_obex_cback,
                               &p_link->handle))
    {
        p_link->state = BT_OPP_STATE_CONNECTING;
        return true;
    }
    else
    {
        bt_opp_free_link(p_link);
        return false;
    }
}

bool bt_opp_connect_over_l2c_req(uint8_t  bd_addr[6],
                                 uint16_t l2c_psm)
{
    T_BT_OPP_LINK *p_link;

    p_link = bt_opp_find_link_by_addr(bd_addr);
    if (p_link)
    {
        return false;
    }

    p_link = bt_opp_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    bt_sniff_mode_exit(bd_addr, false);

    if (obex_conn_req_over_l2c(bd_addr, (uint8_t *)count_hender, l2c_psm, bt_opp_obex_cback,
                               &p_link->handle))
    {
        p_link->state = BT_OPP_STATE_CONNECTING;
        return true;
    }
    else
    {
        bt_opp_free_link(p_link);
        return false;
    }
}

bool bt_opp_push_data_header_req(uint8_t   bd_addr[6],
                                 uint32_t  total_len,
                                 uint8_t  *name,
                                 uint16_t  name_len,
                                 uint8_t  *type,
                                 uint16_t  type_len,
                                 bool      srm_enable)
{
    T_BT_OPP_LINK *p_link;
    bool           ret = false;

    p_link = bt_opp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        uint8_t  *p_data;
        uint8_t  *p;
        uint16_t  pkt_len = OBEX_HDR_NAME_LEN + name_len + OBEX_HDR_TYPE_LEN +
                            type_len + OBEX_HDR_LENGTH_LEN;

        if (p_link->obex_over_l2c && srm_enable && p_link->srm_status == SRM_DISABLE)
        {
            pkt_len += OBEX_HDR_SRM_LEN;
        }

        p_data = malloc(pkt_len);
        if (p_data != NULL)
        {
            p = p_data;

            if (p_link->obex_over_l2c && srm_enable && p_link->srm_status == SRM_DISABLE)
            {
                BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
                BE_UINT8_TO_STREAM(p, 0x01);
            }

            BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
            BE_UINT16_TO_STREAM(p, name_len + OBEX_HDR_NAME_LEN);
            ARRAY_TO_STREAM(p, name, name_len);

            BE_UINT8_TO_STREAM(p, OBEX_HI_TYPE);
            BE_UINT16_TO_STREAM(p, type_len + OBEX_HDR_TYPE_LEN);
            ARRAY_TO_STREAM(p, type, type_len);

            BE_UINT8_TO_STREAM(p, OBEX_HI_LENGTH);
            BE_UINT32_TO_STREAM(p, total_len);

            bt_sniff_mode_exit(bd_addr, false);
            ret = obex_put_data(p_link->handle, p_data, p - p_data, true, NULL, 0, true);

            free(p_data);
        }
    }

    return ret;
}

bool bt_opp_push_data_req(uint8_t   bd_addr[6],
                          uint8_t  *data,
                          uint16_t  data_len,
                          bool      more_data)
{
    T_BT_OPP_LINK *p_link;

    p_link = bt_opp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return obex_put_data(p_link->handle, NULL, 0, more_data, data, data_len, true);
    }

    return false;
}

bool bt_opp_send_data_rsp(uint8_t                bd_addr[6],
                          T_BT_OPP_RESPONSE_CODE rsp_code)
{
    T_BT_OPP_LINK *p_link;
    bool           ret = false;

    p_link = bt_opp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->srm_status == SRM_ENABLE && rsp_code == OBEX_RSP_CONTINUE)
        {
            ret = true;
        }
        else
        {
            bt_sniff_mode_exit(bd_addr, false);
            ret = obex_send_rsp(p_link->handle, rsp_code);
        }
    }

    return ret;
}

bool bt_opp_push_abort(uint8_t bd_addr[6])
{
    T_BT_OPP_LINK *p_link;

    p_link = bt_opp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return obex_abort(p_link->handle);
    }

    return false;
}

bool bt_opp_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_OPP_LINK *p_link;

    p_link = bt_opp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        p_link->state = BT_OPP_STATE_DISCONNECTING;
        bt_sniff_mode_exit(bd_addr, false);
        return obex_disconn_req(p_link->handle);
    }

    return false;
}

bool bt_opp_init(uint8_t  server_chann,
                 uint16_t l2c_psm)
{
    int32_t ret = 0;

    bt_opp = calloc(1, sizeof(T_BT_OPP));
    if (bt_opp == NULL)
    {
        ret = 1;
        goto fail_alloc_opp_db;
    }

    if (obex_register_over_rfc(server_chann, bt_opp_obex_cback) == false)
    {
        ret = 2;
        goto fail_register_rfc;
    }

    if (l2c_psm)
    {
        if (obex_register_over_l2c(l2c_psm, bt_opp_obex_cback) == false)
        {
            ret = 3;
            goto fail_register_l2c;
        }
    }

    return true;

fail_register_l2c:
fail_register_rfc:
    free(bt_opp);
    bt_opp = NULL;
fail_alloc_opp_db:
    PROFILE_PRINT_ERROR1("bt_opp_init: failed %d", -ret);
    return false;
}
