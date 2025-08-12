/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "bt_types.h"
#include "obex.h"
#include "opp.h"

typedef enum t_opp_state
{
    OPP_STATE_DISCONNECTED       = 0x00,
    OPP_STATE_CONNECTING         = 0x01,
    OPP_STATE_CONNECTED          = 0x02,
} T_OPP_STATE;

typedef struct t_opp_link
{
    uint8_t       bd_addr[6];
    uint8_t       srm_status;
    T_OPP_STATE   state;
    T_OBEX_HANDLE handle;
    bool          obex_over_l2c;
} T_OPP_LINK;

typedef struct t_opp
{
    P_OPP_CBACK     cback;
    T_OPP_LINK     *p_link;
    uint8_t         obex_over_rfc_idx;
    uint8_t         obex_over_l2c_idx;
    uint8_t         link_num;
} T_OPP;

static T_OPP *p_opp;

static const uint8_t count_hender[6] =
{
    /* indicate total length: 5 */
    0x05,
    OBEX_HI_COUNT,
    /* Count of Objects: 1 */
    0x00, 0x00, 0x00, 0x01
};

T_OPP_LINK *opp_alloc_link(uint8_t bd_addr[6])
{
    uint8_t     i;
    T_OPP_LINK *p_link;

    for (i = 0; i < p_opp->link_num; i++)
    {
        p_link = &p_opp->p_link[i];
        if (p_link->state == OPP_STATE_DISCONNECTED)
        {
            p_link->handle = NULL;
            memcpy(p_link->bd_addr, bd_addr, 6);

            return p_link;
        }
    }

    return NULL;
}

void opp_free_link(T_OPP_LINK *p_link)
{
    memset(p_link, 0, sizeof(T_OPP_LINK));
}

T_OPP_LINK *opp_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t i;
    T_OPP_LINK *p_link;

    for (i = 0; i < p_opp->link_num; i++)
    {
        p_link = &p_opp->p_link[i];

        if ((p_link->state != OPP_STATE_DISCONNECTED) && (memcmp(p_link->bd_addr, bd_addr, 6) == 0))
        {
            return p_link;
        }
    }

    return NULL;
}

void opp_obex_cb(uint8_t  msg_type,
                 void    *p_msg)
{
    T_OPP_LINK *p_link;

    switch (msg_type)
    {
    case OBEX_CB_MSG_CONN_IND:
        {
            T_OBEX_CONN_IND_DATA *p_data = (T_OBEX_CONN_IND_DATA *)p_msg;

            p_link = opp_find_link_by_addr(p_data->bd_addr);
            if (p_link == NULL)
            {
                p_link = opp_alloc_link(p_data->bd_addr);
                if (p_link != NULL)
                {
                    p_link->state = OPP_STATE_CONNECTING;
                    p_link->handle = p_data->handle;
                    p_opp->cback(p_link->bd_addr, OPP_MSG_CONN_IND, NULL);
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

    case OBEX_CB_MSG_CONN_CMPL:
        {
            T_OBEX_CONN_CMPL_DATA *p_data = (T_OBEX_CONN_CMPL_DATA *)p_msg;

            p_link = opp_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                p_link->state = OPP_STATE_CONNECTED;
                p_link->obex_over_l2c = p_data->obex_over_l2c;
                if (p_opp->cback)
                {
                    p_opp->cback(p_link->bd_addr, OPP_MSG_CONNECTED, NULL);
                }
            }
            else
            {
                obex_disconn_req(p_data->handle);
            }
        }
        break;

    case OBEX_CB_MSG_PUT_DONE:
        {
            T_OBEX_PUT_CMPL_DATA *p_data = (T_OBEX_PUT_CMPL_DATA *)p_msg;

            p_link = opp_find_link_by_addr(p_data->bd_addr);
            if (p_link && p_link->handle == p_data->handle)
            {
                uint8_t cause;

                cause = p_data->cause;
                p_link->srm_status = p_data->srm_status;

                p_opp->cback(p_link->bd_addr, OPP_MSG_DATA_RSP, &cause);
            }
        }
        break;

    case OBEX_CB_MSG_PUT_DATA_RSP:
        {
            T_OBEX_PUT_DATA_RSP *p_data = (T_OBEX_PUT_DATA_RSP *)p_msg;

            p_link = opp_find_link_by_addr(p_data->bd_addr);
            if (p_link && p_link->handle == p_data->handle)
            {
                if (p_link->srm_status == SRM_ENABLE)
                {
                    uint8_t cause;

                    cause = OBEX_RSP_CONTINUE;
                    p_opp->cback(p_link->bd_addr, OPP_MSG_DATA_RSP, &cause);
                }
            }
        }
        break;

    case OBEX_CB_MSG_REMOTE_PUT:
        {
            bool     ret;
            uint8_t *p_hdr_data;
            uint16_t hdr_data_len;
            T_OPP_NOTIF_MSG_DATA opp_data;
            T_OPP_NOTIF_MSG_DATA_HEADERS header;
            T_OBEX_REMOTE_PUT_DATA *p_data = (T_OBEX_REMOTE_PUT_DATA *)p_msg;

            p_link = opp_find_link_by_addr(p_data->bd_addr);
            if (p_link == NULL || p_link->handle != p_data->handle || p_opp->cback == NULL)
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

                p_opp->cback(p_link->bd_addr, OPP_MSG_DATA_HEADER_IND, (void *)&header);
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

            p_opp->cback(p_link->bd_addr, OPP_MSG_DATA_IND, (void *)&opp_data);
        }
        break;

    case OBEX_CB_MSG_DISCONN:
        {
            T_OBEX_DISCONN_CMPL_DATA *p_data = (T_OBEX_DISCONN_CMPL_DATA *)p_msg;

            p_link = opp_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                if (p_data->handle == p_link->handle)
                {
                    T_OPP_STATE   state;

                    state = p_link->state;
                    opp_free_link(p_link);

                    if (p_opp->cback)
                    {
                        if (state == OPP_STATE_CONNECTING)
                        {
                            p_opp->cback(p_data->bd_addr, OPP_MSG_CONN_FAIL, &p_data->cause);
                        }
                        else
                        {
                            p_opp->cback(p_data->bd_addr, OPP_MSG_DISCONNECTED, &p_data->cause);
                        }
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

bool opp_connect_cfm(uint8_t bd_addr[6],
                     bool    accept)
{
    T_OPP_LINK *p_link;

    p_link = opp_find_link_by_addr(bd_addr);
    if ((p_link != NULL) && (p_link->handle != NULL))
    {
        return obex_conn_cfm(p_link->handle, accept, NULL, 0);
    }

    return false;
}

bool opp_conn_over_rfc(uint8_t bd_addr[6],
                       uint8_t server_chann)
{
    T_OPP_LINK *p_link;

    p_link = opp_find_link_by_addr(bd_addr);
    if (p_link)
    {
        return false;
    }

    p_link = opp_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (obex_conn_req_over_rfc(bd_addr, (uint8_t *)count_hender, server_chann, opp_obex_cb,
                               &p_link->handle))
    {
        p_link->state = OPP_STATE_CONNECTING;
        return true;
    }
    else
    {
        opp_free_link(p_link);
        return false;
    }
}

bool opp_conn_over_l2c(uint8_t  bd_addr[6],
                       uint16_t l2c_psm)
{
    T_OPP_LINK *p_link;

    p_link = opp_find_link_by_addr(bd_addr);
    if (p_link)
    {
        return false;
    }

    p_link = opp_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_link->state = OPP_STATE_CONNECTING;

    if (obex_conn_req_over_l2c(bd_addr, (uint8_t *)count_hender, l2c_psm, opp_obex_cb, &p_link->handle))
    {
        return true;
    }
    else
    {
        opp_free_link(p_link);
        return false;
    }
}

bool opp_push_data_header_req(uint8_t   bd_addr[6],
                              uint32_t  total_len,
                              uint8_t  *name,
                              uint16_t  name_len,
                              uint8_t  *type,
                              uint16_t  type_len,
                              bool      srm_enable)
{
    bool        ret = false;
    uint8_t    *p_data;
    uint8_t    *p;
    T_OPP_LINK *p_link;

    p_link = opp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        uint16_t pkt_len = OBEX_HDR_NAME_LEN + name_len + OBEX_HDR_TYPE_LEN +
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

            ret = obex_put_data(p_link->handle, p_data, p - p_data, true, NULL, 0, true);

            free(p_data);
        }
    }

    return ret;
}

bool opp_push_data_req(uint8_t   bd_addr[6],
                       uint8_t  *data,
                       uint16_t  data_len,
                       bool      more_data)
{
    T_OPP_LINK *p_link;

    p_link = opp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        return obex_put_data(p_link->handle, NULL, 0, more_data, data, data_len, true);
    }

    return false;
}

bool opp_send_rsp(uint8_t bd_addr[6],
                  uint8_t rsp_code)
{
    bool        ret = false;
    T_OPP_LINK *p_link;

    p_link = opp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        if (p_link->srm_status == SRM_ENABLE && rsp_code == OBEX_RSP_CONTINUE)
        {
            ret = true;
        }
        else
        {
            ret = obex_send_rsp(p_link->handle, rsp_code);
        }
    }

    return ret;
}

bool opp_push_abort(uint8_t bd_addr[6])
{
    T_OPP_LINK *p_link;

    p_link = opp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        return obex_abort(p_link->handle);
    }

    return false;
}

bool opp_disconnect_req(uint8_t bd_addr[6])
{
    T_OPP_LINK *p_link;

    p_link = opp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        return obex_disconn_req(p_link->handle);
    }

    return false;
}

bool opp_init(uint8_t     link_num,
              P_OPP_CBACK cback,
              uint8_t     server_chann,
              uint16_t    l2c_psm)
{
    int32_t ret = 0;

    p_opp = calloc(1, sizeof(T_OPP));
    if (p_opp == NULL)
    {
        ret = 1;
        goto fail_alloc_opp;
    }

    p_opp->link_num = link_num;

    p_opp->p_link = calloc(p_opp->link_num, sizeof(T_OPP_LINK));
    if (p_opp->p_link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (obex_init() == false)
    {
        ret = 3;
        goto fail_init_obex;
    }

    if (obex_reg_cb_over_rfc(server_chann, opp_obex_cb, &p_opp->obex_over_rfc_idx) == false)
    {
        ret = 4;
        goto fail_reg_rfc_cb;
    }

    if (l2c_psm)
    {
        if (obex_reg_cb_over_l2c(l2c_psm, opp_obex_cb, &p_opp->obex_over_l2c_idx) == false)
        {
            ret = 5;
            goto fail_reg_l2c_cb;
        }
    }

    p_opp->cback = cback;

    return true;

fail_reg_l2c_cb:
fail_reg_rfc_cb:
fail_init_obex:
    free(p_opp->p_link);
fail_alloc_link:
    free(p_opp);
    p_opp = NULL;
fail_alloc_opp:
    PROFILE_PRINT_ERROR1("opp_init: failed %d", -ret);
    return false;
}
