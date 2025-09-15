/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_AVRCP_SUPPORT == 1)
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "avctp.h"

typedef enum t_avctp_state
{
    AVCTP_STATE_DISCONNECTED  = 0x00,
    AVCTP_STATE_CONNECTING    = 0x01,
    AVCTP_STATE_CONNECTED     = 0x02,
    AVCTP_STATE_DISCONNECTING = 0x03,
} T_AVCTP_STATE;

typedef struct t_avctp_control_chann
{
    uint16_t          cid;
    uint16_t          remote_mtu;
    uint16_t          profile_id;
    uint16_t          ds_data_offset;
    T_AVCTP_STATE     state;
    uint8_t           pkt_num;
    uint16_t          write_idx;
    uint16_t          reassembly_len;
    uint8_t          *reassembly_buf;
} T_AVCTP_CONTROL_CHANN;

typedef struct t_avctp_browsing_chann
{
    T_AVCTP_STATE     state;
    uint16_t          cid;
    uint16_t          remote_mtu;
    uint16_t          profile_id;
    uint16_t          ds_data_offset;
} T_AVCTP_BROWSING_CHANN;

typedef struct t_avctp_link
{
    struct t_avctp_link      *next;
    uint8_t                   bd_addr[6];
    T_AVCTP_CONTROL_CHANN     control_chann;
    T_AVCTP_BROWSING_CHANN    browsing_chann;
} T_AVCTP_LINK;

typedef struct t_avctp
{
    T_OS_QUEUE         list;
    uint8_t            queue_id;
    uint8_t            queue_id_browsing;
    T_AVCTP_CBACK      cback;
} T_AVCTP;

static T_AVCTP *avctp = NULL;

T_AVCTP_LINK *avctp_alloc_link(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *link;

    link = calloc(1, sizeof(T_AVCTP_LINK));
    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);
        link->control_chann.state = AVCTP_STATE_DISCONNECTED;
        os_queue_in(&avctp->list, link);
    }

    return link;
}

void avctp_free_link(T_AVCTP_LINK *link)
{
    if (os_queue_delete(&avctp->list, link) == true)
    {
        if (link->control_chann.reassembly_buf != NULL)
        {
            free(link->control_chann.reassembly_buf);
        }

        free(link);
    }
}

T_AVCTP_LINK *avctp_find_link_by_cid(uint16_t cid)
{
    T_AVCTP_LINK *link;

    link = os_queue_peek(&avctp->list, 0);
    while (link != NULL)
    {
        if ((link->control_chann.state != AVCTP_STATE_DISCONNECTED && link->control_chann.cid == cid) ||
            (link->browsing_chann.state != AVCTP_STATE_DISCONNECTED && link->browsing_chann.cid == cid))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

T_AVCTP_LINK *avctp_find_link_by_addr(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *link;

    link = os_queue_peek(&avctp->list, 0);
    while (link != NULL)
    {
        if (link->control_chann.state != AVCTP_STATE_DISCONNECTED &&
            !memcmp(link->bd_addr, bd_addr, 6))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

bool avctp_connect_req(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = avctp_alloc_link(bd_addr);
        if (link != NULL)
        {
            PROTOCOL_PRINT_TRACE1("avctp_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            link->control_chann.state = AVCTP_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_AVCTP,
                                  UUID_AVCTP,
                                  avctp->queue_id,
                                  AVCTP_L2CAP_MTU_SIZE,
                                  bd_addr,
                                  MPA_L2C_MODE_BASIC,
                                  0xFFFF);

            return true;
        }
    }

    return false;
}

bool avctp_browsing_connect_req(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        PROTOCOL_PRINT_TRACE1("avctp_browsing_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

        if (link->browsing_chann.state == AVCTP_STATE_DISCONNECTED)
        {
            link->browsing_chann.state = AVCTP_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_AVCTP_BROWSING,
                                  UUID_AVCTP,
                                  avctp->queue_id_browsing,
                                  AVCTP_BROWSING_L2CAP_MTU_SIZE,
                                  bd_addr,
                                  MPA_L2C_MODE_ERTM,
                                  0xFFFF);
        }

        return true;
    }

    return false;
}

bool avctp_disconnect_req(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->control_chann.reassembly_buf != NULL)
        {
            free(link->control_chann.reassembly_buf);
            memset(&link->control_chann, 0, sizeof(link->control_chann));
        }

        mpa_send_l2c_disconn_req(link->control_chann.cid);
        return true;
    }

    return false;
}

bool avctp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        mpa_send_l2c_disconn_req(link->browsing_chann.cid);
        return true;
    }

    return false;
}

bool avctp_connect_cfm(uint8_t bd_addr[6],
                       bool    accept)
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint16_t                 cid;
        T_MPA_L2C_CONN_CFM_CAUSE rsp;

        cid = link->control_chann.cid;
        rsp = MPA_L2C_CONN_ACCEPT;
        if (accept == false)
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;
            avctp_free_link(link);
        }

        mpa_send_l2c_conn_cfm(rsp,
                              cid,
                              AVCTP_L2CAP_MTU_SIZE,
                              MPA_L2C_MODE_BASIC,
                              0xFFFF);
        return true;
    }

    return false;
}

bool avctp_browsing_connect_cfm(uint8_t bd_addr[6],
                                bool    accept)
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint16_t                 cid;
        T_MPA_L2C_CONN_CFM_CAUSE rsp;

        cid = link->browsing_chann.cid;
        rsp = MPA_L2C_CONN_ACCEPT;
        if (accept == false)
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;
            link->browsing_chann.state = AVCTP_STATE_DISCONNECTED;
        }

        mpa_send_l2c_conn_cfm(rsp,
                              cid,
                              AVCTP_BROWSING_L2CAP_MTU_SIZE,
                              MPA_L2C_MODE_ERTM,
                              0xFFFF);
        return true;
    }

    return false;
}

bool avctp_data_send(uint8_t   bd_addr[6],
                     uint8_t  *data,
                     uint16_t  len,
                     uint8_t  *vendor_data,
                     uint16_t  vendor_len,
                     uint8_t   transact,
                     uint8_t   crtype)
{
    T_AVCTP_LINK *link;
    uint8_t      *buf;
    uint8_t      *p;
    int32_t       ret = 0;

    link = avctp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_find_link;
    }

    if ((AVCTP_NON_FRAG_HDR_LENGTH + len + vendor_len) > link->control_chann.remote_mtu)
    {
        ret = 2;
        goto fail_invalid_length;
    }

    buf = mpa_get_l2c_buf(avctp->queue_id,
                          link->control_chann.cid,
                          0,
                          AVCTP_NON_FRAG_HDR_LENGTH + len + vendor_len,
                          link->control_chann.ds_data_offset,
                          false);
    if (buf == NULL)
    {
        ret = 3;
        goto fail_get_l2c_buf;
    }

    p = buf + link->control_chann.ds_data_offset;

    *p++ = ((transact << 4) & 0xf0) | ((AVCTP_PKT_TYPE_UNFRAG << 2) & 0x0c) | ((crtype << 1) & 0x2);
    BE_UINT16_TO_STREAM(p, UUID_AV_REMOTE_CONTROL);

    memcpy(p, data, len);
    p += len;
    if (vendor_data != NULL)
    {
        memcpy(p, vendor_data, vendor_len);
    }

    mpa_send_l2c_data_req(buf,
                          link->control_chann.ds_data_offset,
                          link->control_chann.cid,
                          AVCTP_NON_FRAG_HDR_LENGTH + len + vendor_len,
                          false);

    return true;

fail_get_l2c_buf:
fail_invalid_length:
fail_find_link:
    PROTOCOL_PRINT_ERROR1("avctp_data_send: failed %d", -ret);
    return false;
}

bool avctp_browsing_data_send(uint8_t   bd_addr[6],
                              uint8_t  *data,
                              uint16_t  length,
                              uint8_t   transact,
                              uint8_t   crtype)
{
    T_AVCTP_LINK *link;
    uint8_t      *buf;
    uint8_t      *p;
    int32_t       ret = 0;

    link = avctp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_find_link;
    }

    if ((AVCTP_NON_FRAG_HDR_LENGTH + length) > link->browsing_chann.remote_mtu)
    {
        ret = 2;
        goto fail_invalid_length;
    }

    buf = mpa_get_l2c_buf(avctp->queue_id,
                          link->browsing_chann.cid,
                          0,
                          AVCTP_NON_FRAG_HDR_LENGTH + length,
                          link->browsing_chann.ds_data_offset,
                          false);
    if (buf == NULL)
    {
        ret = 3;
        goto fail_get_l2c_buf;
    }

    p = buf + link->browsing_chann.ds_data_offset;

    /* 3byte avctp header;*/
    *p++ = ((transact << 4) & 0xf0) | ((AVCTP_PKT_TYPE_UNFRAG << 2) & 0x0c) | ((crtype << 1) & 0x2);
    BE_UINT16_TO_STREAM(p, UUID_AV_REMOTE_CONTROL);

    memcpy(p, data, length);

    mpa_send_l2c_data_req(buf,
                          link->browsing_chann.ds_data_offset,
                          link->browsing_chann.cid,
                          AVCTP_NON_FRAG_HDR_LENGTH + length,
                          false);

    return true;

fail_get_l2c_buf:
fail_invalid_length:
fail_find_link:
    PROTOCOL_PRINT_ERROR1("avctp_browsing_data_send: failed %d", -ret);
    return false;
}

void avctp_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_cid(data_ind->cid);
    if (link != NULL)
    {
        uint8_t  *data = data_ind->data + data_ind->gap;
        uint16_t  length = data_ind->length;
        uint8_t   crtype = (*data & 0x02) >> 1;
        uint8_t   packet_type = (*data & 0x0c) >> 2;
        uint8_t   transact_label = *data >> 4;

        data++;
        length--;

        /* recombine avctp frames(<=L2CAP MTU) to avrcp packets(<=512 bytes). */
        if (packet_type == AVCTP_PKT_TYPE_UNFRAG)
        {
            /* last recombination not completed, abort it. */
            if (link->control_chann.reassembly_buf != NULL)
            {
                free(link->control_chann.reassembly_buf);
                memset(&link->control_chann, 0, sizeof(link->control_chann));
            }

            /* profile_id also stores Profile Identifier of UNFRAGMENT packets. */
            BE_STREAM_TO_UINT16(link->control_chann.profile_id, data);
            length -= 2;
        }
        else
        {
            if (packet_type == AVCTP_PKT_TYPE_START)
            {
                link->control_chann.pkt_num = *data++;
                length--;
                BE_STREAM_TO_UINT16(link->control_chann.profile_id, data);
                length -= 2;

                /* init recombine struct*/
                if (link->control_chann.reassembly_buf == NULL)
                {
                    link->control_chann.reassembly_len =
                        link->control_chann.pkt_num * (length - AVCTP_START_HDR_LENGTH);
                    link->control_chann.reassembly_buf = malloc(link->control_chann.reassembly_len);
                    if (link->control_chann.reassembly_buf == NULL)
                    {
                        avctp_disconnect_req(link->bd_addr);
                        return;
                    }
                }
                else /* last recombination not completed, retain buf*/
                {}

                link->control_chann.write_idx = 0;
            }

            /* avctp recombine recv CONTINUE/END without START. */
            if (link->control_chann.reassembly_buf == NULL)
            {
                return;
            }

            /* received packets > pkt_num or received < pkt_num or would exceed buf. */
            if ((link->control_chann.pkt_num == 0) ||
                ((packet_type == AVCTP_PKT_TYPE_END) && (link->control_chann.pkt_num > 1)) ||
                (link->control_chann.write_idx + length >
                 link->control_chann.reassembly_len))
            {
                free(link->control_chann.reassembly_buf);
                memset(&link->control_chann, 0, sizeof(link->control_chann));
                avctp_disconnect_req(link->bd_addr);
                return;
            }

            memcpy(link->control_chann.reassembly_buf + link->control_chann.write_idx, data, length);
            link->control_chann.write_idx += length;
            link->control_chann.pkt_num--;

            if (packet_type == AVCTP_PKT_TYPE_END)
            {
                data = link->control_chann.reassembly_buf;
                length = link->control_chann.write_idx;
            }
        }

        if ((packet_type == AVCTP_PKT_TYPE_UNFRAG) || (packet_type == AVCTP_PKT_TYPE_END))
        {
            if (link->control_chann.profile_id == UUID_AV_REMOTE_CONTROL)
            {
                T_AVCTP_DATA_IND avctp_data_ind;

                avctp_data_ind.transact_label = transact_label;
                avctp_data_ind.crtype = crtype;
                avctp_data_ind.length = length;
                avctp_data_ind.data = data;
                avctp->cback(data_ind->cid, AVCTP_MSG_DATA_IND, (uint8_t *)&avctp_data_ind);
            }
            else
            {
                uint8_t *buf;

                buf = mpa_get_l2c_buf(avctp->queue_id,
                                      link->control_chann.cid,
                                      0,
                                      AVCTP_NON_FRAG_HDR_LENGTH,
                                      link->control_chann.ds_data_offset,
                                      false);
                if (buf != NULL)
                {
                    buf[link->control_chann.ds_data_offset] = ((transact_label << 4) & 0xf0) |
                                                              ((AVCTP_PKT_TYPE_UNFRAG << 2) & 0x0c) |
                                                              ((AVCTP_MSG_TYPE_RSP << 1) & 0x2) |
                                                              (0x01);
                    buf[link->control_chann.ds_data_offset + 1] =
                        (uint8_t)(link->control_chann.profile_id >> 8);
                    buf[link->control_chann.ds_data_offset + 2] =
                        (uint8_t)(link->control_chann.profile_id);
                    mpa_send_l2c_data_req(buf,
                                          link->control_chann.ds_data_offset,
                                          link->control_chann.cid,
                                          AVCTP_NON_FRAG_HDR_LENGTH,
                                          false);
                }
            }

            if (packet_type == AVCTP_PKT_TYPE_END)
            {
                free(link->control_chann.reassembly_buf);
                memset(&link->control_chann, 0, sizeof(link->control_chann));
            }
        }
    }
}

void avctp_browsing_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_cid(data_ind->cid);
    if (link != NULL)
    {
        uint8_t  *data = data_ind->data + data_ind->gap;
        uint16_t  length = data_ind->length;
        uint8_t   crtype = (*data & 0x02) >> 1;
        uint8_t   transact_label = *data >> 4;

        data++;
        length--;

        BE_STREAM_TO_UINT16(link->browsing_chann.profile_id, data);
        length -= 2;

        if (link->browsing_chann.profile_id == UUID_AV_REMOTE_CONTROL)
        {
            T_AVCTP_DATA_IND avctp_data_ind;

            avctp_data_ind.transact_label = transact_label;
            avctp_data_ind.crtype = crtype;
            avctp_data_ind.length = length;
            avctp_data_ind.data = data;
            avctp->cback(data_ind->cid, AVCTP_MSG_BROWSING_DATA_IND, (uint8_t *)&avctp_data_ind);
        }
        else
        {
            uint8_t *buf;

            buf = mpa_get_l2c_buf(avctp->queue_id,
                                  link->browsing_chann.cid,
                                  0,
                                  AVCTP_NON_FRAG_HDR_LENGTH,
                                  link->browsing_chann.ds_data_offset,
                                  false);
            if (buf != NULL)
            {
                buf[link->browsing_chann.ds_data_offset] = ((transact_label << 4) & 0xf0) |
                                                           ((AVCTP_PKT_TYPE_UNFRAG << 2) & 0x0c) |
                                                           ((AVCTP_MSG_TYPE_RSP << 1) & 0x2) |
                                                           (0x01);
                buf[link->browsing_chann.ds_data_offset + 1] =
                    (uint8_t)(link->browsing_chann.profile_id >> 8);
                buf[link->browsing_chann.ds_data_offset + 2] =
                    (uint8_t)(link->browsing_chann.profile_id);
                mpa_send_l2c_data_req(buf,
                                      link->browsing_chann.ds_data_offset,
                                      link->browsing_chann.cid,
                                      AVCTP_NON_FRAG_HDR_LENGTH,
                                      false);
            }
        }
    }
}

void avctp_callback(void        *buf,
                    T_PROTO_MSG  l2c_msg)
{
    switch (l2c_msg)
    {
    case L2C_CONN_IND:
        {
            T_MPA_L2C_CONN_IND *ind;
            T_AVCTP_LINK       *link;

            ind = (T_MPA_L2C_CONN_IND *)buf;
            link = avctp_find_link_by_addr(ind->bd_addr);
            if (ind->proto_id == avctp->queue_id)
            {
                if (link == NULL)
                {
                    link = avctp_alloc_link(ind->bd_addr);
                    if (link != NULL)
                    {
                        link->control_chann.cid = ind->cid;
                        link->control_chann.state = AVCTP_STATE_CONNECTING;
                        avctp->cback(ind->cid, AVCTP_MSG_CONN_IND, buf);
                    }
                    else
                    {
                        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                                              ind->cid,
                                              AVCTP_L2CAP_MTU_SIZE,
                                              MPA_L2C_MODE_BASIC,
                                              0xFFFF);
                    }
                }
                else
                {
                    mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                                          ind->cid,
                                          AVCTP_L2CAP_MTU_SIZE,
                                          MPA_L2C_MODE_BASIC,
                                          0xFFFF);
                }
            }
            else if (ind->proto_id == avctp->queue_id_browsing)
            {
                if (link == NULL)
                {
                    mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                                          ind->cid,
                                          AVCTP_BROWSING_L2CAP_MTU_SIZE,
                                          MPA_L2C_MODE_ERTM,
                                          0xFFFF);
                }
                else
                {
                    if (link->browsing_chann.state == AVCTP_STATE_DISCONNECTED)
                    {
                        link->browsing_chann.cid = ind->cid;
                        link->browsing_chann.state = AVCTP_STATE_CONNECTING;
                        avctp->cback(ind->cid, AVCTP_MSG_BROWSING_CONN_IND, buf);
                    }
                    else
                    {
                        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                                              ind->cid,
                                              AVCTP_BROWSING_L2CAP_MTU_SIZE,
                                              MPA_L2C_MODE_ERTM,
                                              0xFFFF);
                    }
                }
            }
        }
        break;

    case L2C_CONN_RSP:
        {
            T_MPA_L2C_CONN_RSP *ind;
            T_AVCTP_LINK       *link;

            ind = (T_MPA_L2C_CONN_RSP *)buf;
            link = avctp_find_link_by_addr(ind->bd_addr);
            if (ind->proto_id == avctp->queue_id)
            {
                if (ind->cause == 0)
                {
                    if (link != NULL)
                    {
                        link->control_chann.cid = ind->cid;
                        avctp->cback(ind->cid, AVCTP_MSG_CONN_RSP, buf);
                    }
                    else
                    {
                        avctp_disconnect_req(ind->bd_addr);
                    }
                }
                else
                {
                    avctp->cback(ind->cid, AVCTP_MSG_CONN_FAIL, &ind->cause);
                    avctp_disconnect_req(ind->bd_addr);
                }
            }
            else if (ind->proto_id == avctp->queue_id_browsing)
            {
                if (ind->cause == 0)
                {
                    if (link != NULL)
                    {
                        link->browsing_chann.cid = ind->cid;
                        avctp->cback(ind->cid, AVCTP_MSG_BROWSING_CONN_RSP, buf);
                    }
                    else
                    {
                        avctp_browsing_disconnect_req(ind->bd_addr);
                    }
                }
                else
                {
                    avctp_browsing_disconnect_req(ind->bd_addr);
                }
            }
        }
        break;

    case L2C_CONN_CMPL:
        {
            T_MPA_L2C_CONN_CMPL_INFO *info;
            T_AVCTP_LINK             *link;

            info = (T_MPA_L2C_CONN_CMPL_INFO *)buf;
            link = avctp_find_link_by_cid(info->cid);
            if (link != NULL)
            {
                if (info->proto_id == avctp->queue_id)
                {
                    if (info->cause)
                    {
                        avctp_free_link(link);
                        avctp->cback(info->cid, AVCTP_MSG_CONN_FAIL, &info->cause);
                    }
                    else
                    {
                        link->control_chann.remote_mtu = info->remote_mtu;
                        link->control_chann.ds_data_offset = info->ds_data_offset;
                        link->control_chann.state = AVCTP_STATE_CONNECTED;
                        avctp->cback(info->cid, AVCTP_MSG_CONN_CMPL_IND, buf);
                    }
                }
                else if (info->proto_id == avctp->queue_id_browsing)
                {
                    if (info->cause)
                    {
                        memset(&(link->browsing_chann), 0, sizeof(T_AVCTP_BROWSING_CHANN));
                        avctp->cback(info->cid, AVCTP_MSG_BROWSING_DISCONN_IND, &info->cause);
                    }
                    else
                    {
                        link->browsing_chann.remote_mtu = info->remote_mtu;
                        link->browsing_chann.ds_data_offset = info->ds_data_offset;
                        link->browsing_chann.state = AVCTP_STATE_CONNECTED;
                        avctp->cback(info->cid, AVCTP_MSG_BROWSING_CONN_CMPL_IND, buf);
                    }
                }
            }
        }
        break;

    case L2C_DATA_IND:
        {
            T_MPA_L2C_DATA_IND *ind = (T_MPA_L2C_DATA_IND *)buf;

            if (ind->proto_id == avctp->queue_id)
            {
                avctp_handle_l2c_data_ind(ind);
            }
            else if (ind->proto_id == avctp->queue_id_browsing)
            {
                avctp_browsing_handle_l2c_data_ind(ind);
            }
        }
        break;

    case L2C_DISCONN_IND:
        {
            T_MPA_L2C_DISCONN_IND *ind;
            T_AVCTP_LINK          *link;

            ind = (T_MPA_L2C_DISCONN_IND *)buf;
            link = avctp_find_link_by_cid(ind->cid);
            if (ind->proto_id == avctp->queue_id)
            {
                if (link != NULL)
                {
                    avctp_free_link(link);
                }

                mpa_send_l2c_disconn_cfm(ind->cid);
                avctp->cback(ind->cid, AVCTP_MSG_DISCONN_IND, &ind->cause);
            }
            else if (ind->proto_id == avctp->queue_id_browsing)
            {
                if (link != NULL)
                {
                    memset(&(link->browsing_chann), 0, sizeof(T_AVCTP_BROWSING_CHANN));
                }

                mpa_send_l2c_disconn_cfm(ind->cid);
                avctp->cback(ind->cid, AVCTP_MSG_BROWSING_DISCONN_IND, &ind->cause);
            }
        }
        break;

    case L2C_DISCONN_RSP:
        {
            T_MPA_L2C_DISCONN_RSP *ind;
            T_AVCTP_LINK          *link;

            ind = (T_MPA_L2C_DISCONN_RSP *)buf;
            link = avctp_find_link_by_cid(ind->cid);
            if (link != NULL)
            {
                if (ind->proto_id == avctp->queue_id)
                {
                    avctp_free_link(link);
                    avctp->cback(ind->cid, AVCTP_MSG_DISCONN_IND, &ind->cause);
                }
                else if (ind->proto_id == avctp->queue_id_browsing)
                {
                    link->browsing_chann.state = AVCTP_STATE_DISCONNECTED;
                    avctp->cback(ind->cid, AVCTP_MSG_BROWSING_DISCONN_IND, &ind->cause);
                }
            }
        }
        break;

    default:
        PROTOCOL_PRINT_WARN1("avctp rx unkown mpa msg=%x", l2c_msg);
        break;
    }
}

bool avctp_init(T_AVCTP_CBACK cback)
{
    int32_t ret = 0;

    avctp = calloc(1, sizeof(T_AVCTP));
    if (avctp == NULL)
    {
        ret = 1;
        goto fail_alloc_avctp;
    }

    if (mpa_reg_l2c_proto(PSM_AVCTP, avctp_callback, &avctp->queue_id) == false)
    {
        ret = 2;
        goto fail_reg_l2c;
    }
    if (mpa_reg_l2c_proto(PSM_AVCTP_BROWSING, avctp_callback, &avctp->queue_id_browsing) == false)
    {
        ret = 3;
        goto fail_reg_browsing_l2c;
    }

    avctp->cback = cback;
    return true;

fail_reg_browsing_l2c:
fail_reg_l2c:
    free(avctp);
    avctp = NULL;
fail_alloc_avctp:
    PROFILE_PRINT_ERROR1("avctp_init: failed %d", -ret);
    return false;
}


bool avctp_roleswap_info_get(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVCTP_INFO *info)
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        info->l2c_cid = link->control_chann.cid;
        info->remote_mtu = link->control_chann.remote_mtu;
        info->data_offset = link->control_chann.ds_data_offset;
        info->avctp_state = link->control_chann.state;

        return true;
    }

    return false;
}

bool avctp_roleswap_info_set(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVCTP_INFO *info)
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = avctp_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        link->control_chann.state = (T_AVCTP_STATE)info->avctp_state;
        link->control_chann.cid = info->l2c_cid;
        link->control_chann.remote_mtu = info->remote_mtu;
        link->control_chann.ds_data_offset = info->data_offset;

        return true;
    }

    return false;
}

bool avctp_roleswap_info_del(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *link;

    link = avctp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        avctp_free_link(link);
        return true;
    }

    return false;
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "avctp.h"

bool avctp_init(T_AVCTP_CBACK cback)
{
    return false;
}

bool avctp_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool avctp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool avctp_connect_cfm(uint8_t bd_addr[6],
                       bool    accept)
{
    return false;
}

bool avctp_data_send(uint8_t   bd_addr[6],
                     uint8_t  *p_avrcp_data,
                     uint16_t  avrcp_length,
                     uint8_t  *p_avrcp_data2,
                     uint16_t  length2,
                     uint8_t   transact,
                     uint8_t   crtype)
{
    return false;
}

bool avctp_browsing_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool avctp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool avctp_browsing_connect_cfm(uint8_t bd_addr[6],
                                bool    accept)
{
    return false;
}

bool avctp_browsing_data_send(uint8_t   bd_addr[6],
                              uint8_t  *p_avrcp_data,
                              uint16_t  avrcp_length,
                              uint8_t   transact,
                              uint8_t   crtype)
{
    return false;
}

bool avctp_roleswap_info_get(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVCTP_INFO *info)
{
    return false;
}

bool avctp_roleswap_info_set(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVCTP_INFO *info)
{
    return false;
}

bool avctp_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}
#endif
