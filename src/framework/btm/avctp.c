/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_AVRCP_SUPPORT == 1)
#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "avctp.h"

typedef struct t_avctp
{
    uint8_t            queue_id; /**< own (input) queue */
    uint8_t            queue_id_browsing;
    uint8_t            link_num;
    T_AVCTP_LINK      *link;
    P_AVCTP_CBACK      cback;
} T_AVCTP;

T_AVCTP *p_avctp = NULL;

T_AVCTP_LINK *avctp_alloc_link(uint8_t bd_addr[6])
{
    uint8_t       i;
    T_AVCTP_LINK *p_link = NULL;

    for (i = 0; i < p_avctp->link_num; i++)
    {
        if (p_avctp->link[i].control_chann.state == AVCTP_STATE_DISCONNECTED)
        {
            p_link = &(p_avctp->link[i]);

            memcpy(p_link->bd_addr, bd_addr, 6);
            p_link->control_chann.state = AVCTP_STATE_ALLOCATED;
            break;
        }
    }

    return p_link;
}

void avctp_free_link(T_AVCTP_LINK *p_link)
{
    if (p_link == NULL)
    {
        return;
    }

    if (p_link->control_chann.recombine.p_buf != NULL)
    {
        os_mem_free(p_link->control_chann.recombine.p_buf);
    }

    memset(p_link, 0, sizeof(T_AVCTP_LINK));
}

T_AVCTP_LINK *avctp_find_link_by_cid(uint16_t cid)
{
    uint8_t       i;
    T_AVCTP_LINK *p_link;

    for (i = 0; i < p_avctp->link_num; i++)
    {
        p_link = &(p_avctp->link[i]);
        if ((p_link->control_chann.state != AVCTP_STATE_DISCONNECTED &&
             p_link->control_chann.cid == cid) ||
            (p_link->browsing_chann.state != AVCTP_STATE_DISCONNECTED &&
             p_link->browsing_chann.cid == cid))
        {
            return p_link;
        }
    }

    return NULL;
}

T_AVCTP_LINK *avctp_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t       i;
    T_AVCTP_LINK *p_link = NULL;

    if (bd_addr == NULL)
    {
        return p_link;
    }

    for (i = 0; i < p_avctp->link_num; i++)
    {
        if (p_avctp->link[i].control_chann.state != AVCTP_STATE_DISCONNECTED &&
            !memcmp(p_avctp->link[i].bd_addr, bd_addr, 6))
        {
            p_link = &(p_avctp->link[i]);
            break;
        }
    }

    return p_link;
}

bool avctp_connect_req(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *p_link;

    p_link = avctp_find_link_by_addr(bd_addr);
    if (p_link == NULL) //No avctp connection request from remote device
    {
        p_link = avctp_alloc_link(bd_addr);
        if (p_link != NULL)
        {
            PROTOCOL_PRINT_TRACE1("avctp_connect_req: bd_addr[%s]", TRACE_BDADDR(bd_addr));
            p_link->control_chann.state = AVCTP_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_AVCTP, UUID_AVCTP, p_avctp->queue_id, AVCTP_L2CAP_MTU_SIZE, bd_addr,
                                  MPA_L2C_MODE_BASIC, 0xFFFF);

            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        //Remote connection on-going
        return false;
    }
}

bool avctp_browsing_connect_req(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *p_link;

    p_link = avctp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        PROTOCOL_PRINT_ERROR1("avctp_browsing_connect_req: no control link bd_addr[%s]",
                              TRACE_BDADDR(bd_addr));
        return false;
    }

    PROTOCOL_PRINT_TRACE1("avctp_browsing_connect_req: bd_addr[%s]", TRACE_BDADDR(bd_addr));
    if (p_link->browsing_chann.state == AVCTP_STATE_DISCONNECTED)
    {
        p_link->browsing_chann.state = AVCTP_STATE_CONNECTING;
        mpa_send_l2c_conn_req(PSM_AVCTP_BROWSING, UUID_AVCTP, p_avctp->queue_id_browsing,
                              AVCTP_BROWSING_L2CAP_MTU_SIZE, bd_addr,
                              MPA_L2C_MODE_ERTM, 0xFFFF);
    }

    return true;
}

bool avctp_disconnect_req(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *p_link = avctp_find_link_by_addr(bd_addr);

    if (p_link != NULL)
    {
        if (p_link->control_chann.recombine.p_buf != NULL) //last recombination not completed, abort it
        {
            os_mem_free(p_link->control_chann.recombine.p_buf);
            memset(&p_link->control_chann.recombine, 0, sizeof(p_link->control_chann.recombine));
        }
        mpa_send_l2c_disconn_req(p_link->control_chann.cid);
        return true;
    }

    return false;
}

bool avctp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *p_link = avctp_find_link_by_addr(bd_addr);

    if (p_link != NULL)
    {
        mpa_send_l2c_disconn_req(p_link->browsing_chann.cid);
        return true;
    }

    return false;
}

bool avctp_connect_cfm(uint8_t bd_addr[6],
                       bool    accept)
{
    T_AVCTP_LINK *p_link;
    T_MPA_L2C_CONN_CFM_CAUSE rsp;

    rsp = MPA_L2C_CONN_ACCEPT;
    p_link = avctp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        uint16_t cid = p_link->control_chann.cid;
        if (accept == false)
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;
            avctp_free_link(p_link);
        }
        mpa_send_l2c_conn_cfm(rsp, cid, AVCTP_L2CAP_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
        return true;
    }
    return false;
}

bool avctp_browsing_connect_cfm(uint8_t bd_addr[6],
                                bool    accept)
{
    T_AVCTP_LINK *p_link;
    T_MPA_L2C_CONN_CFM_CAUSE rsp;

    rsp = MPA_L2C_CONN_ACCEPT;
    p_link = avctp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        uint16_t cid = p_link->browsing_chann.cid;
        if (accept == false)
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;
            p_link->browsing_chann.state = AVCTP_STATE_DISCONNECTED;
        }
        mpa_send_l2c_conn_cfm(rsp, cid, AVCTP_BROWSING_L2CAP_MTU_SIZE, MPA_L2C_MODE_ERTM, 0xFFFF);
        return true;
    }
    return false;
}

/**
 * @brief
 *
 * @date 2015/11/6
 *
 * @param cid
 * @param avrcp_data
 * @param avrcp_length
 * @param avrcp_data2
 * @param length2
 * @param transact
 * @param crtype
 * @return int
 */
bool avctp_send_data2buf(uint8_t  bd_addr[6],
                         uint8_t *avrcp_data,
                         uint16_t avrcp_length,
                         uint8_t *avrcp_data2,
                         uint16_t length2,
                         uint8_t  transact,
                         uint8_t  crtype)
{
    T_AVCTP_LINK *p_link;
    uint8_t *avctp_buf;

    p_link = avctp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        PROTOCOL_PRINT_ERROR1("avctp_send_data2buf: fail find link by bd_addr [%s]",
                              TRACE_BDADDR(bd_addr));
        return false; //invalid cid
    }

    //check length, if > mtu, fragment it
    if ((AVCTP_NON_FRAG_HDR_LENGTH + avrcp_length + length2) > p_link->control_chann.remote_mtu)
    {
        PROTOCOL_PRINT_ERROR2("avctp_send_data2buf (%d)larger than remote_mtu(%d)",
                              AVCTP_NON_FRAG_HDR_LENGTH + avrcp_length + length2, p_link->control_chann.remote_mtu);
        return false; //invalid length
    }

    avctp_buf = mpa_get_l2c_buf(p_avctp->queue_id, p_link->control_chann.cid, 0,
                                AVCTP_NON_FRAG_HDR_LENGTH + avrcp_length + length2,
                                p_link->control_chann.ds_data_offset, false);
    if (avctp_buf == NULL)
    {
        PROTOCOL_PRINT_ERROR0("avctp_send_data2buf: fail get l2c buf");
        return false; //no resouce
    }

    uint8_t *p_data = avctp_buf + p_link->control_chann.ds_data_offset;

    // 3byte avctp header;
    *p_data++ = ((transact << 4) & 0xf0) | ((AVCTP_PKT_TYPE_UNFRAG << 2) & 0x0c) |
                ((crtype << 1) & 0x2); /* IPID (b0) is 0 */
    BE_UINT16_TO_ARRAY(p_data, UUID_AV_REMOTE_CONTROL);
    p_data += 2;

    //avrcp payload
    memcpy(p_data, avrcp_data, avrcp_length);
    p_data += avrcp_length;
    memcpy(p_data, avrcp_data2, length2);

    mpa_send_l2c_data_req(avctp_buf, p_link->control_chann.ds_data_offset, p_link->control_chann.cid,
                          AVCTP_NON_FRAG_HDR_LENGTH + avrcp_length + length2, false);

    return true;
}

bool avctp_browsing_send_data2buf(uint8_t   bd_addr[6],
                                  uint8_t  *p_avrcp_data,
                                  uint16_t  avrcp_length,
                                  uint8_t   transact,
                                  uint8_t   crtype)
{
    T_AVCTP_LINK *p_link;
    uint8_t *avctp_buf;

    p_link = avctp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        PROTOCOL_PRINT_ERROR1("avctp_browsing_send_data2buf: fail find link by bd_addr [%s]",
                              TRACE_BDADDR(bd_addr));
        return false;
    }

    if (p_link->browsing_chann.state == AVCTP_STATE_CONNECTED)
    {
        if ((AVCTP_NON_FRAG_HDR_LENGTH + avrcp_length) > p_link->browsing_chann.remote_mtu)
        {
            PROTOCOL_PRINT_ERROR2("avctp_browsing_send_data2buf (%d)larger than remote_mtu(%d)",
                                  AVCTP_NON_FRAG_HDR_LENGTH + avrcp_length, p_link->browsing_chann.remote_mtu);
            return false; //invalid length
        }

        avctp_buf = mpa_get_l2c_buf(p_avctp->queue_id, p_link->browsing_chann.cid, 0,
                                    AVCTP_NON_FRAG_HDR_LENGTH + avrcp_length,
                                    p_link->browsing_chann.ds_data_offset,
                                    false);
        if (avctp_buf == NULL)
        {
            PROTOCOL_PRINT_ERROR0("avctp_browsing_send_data2buf: fail get l2c buf");
            return false; //no resouce
        }

        uint8_t *p_data = avctp_buf + p_link->browsing_chann.ds_data_offset;

        // 3byte avctp header;
        *p_data++ = ((transact << 4) & 0xf0) | ((AVCTP_PKT_TYPE_UNFRAG << 2) & 0x0c) |
                    ((crtype << 1) & 0x2); /* IPID (b0) is 0 */
        BE_UINT16_TO_ARRAY(p_data, UUID_AV_REMOTE_CONTROL);
        p_data += 2;

        //avrcp payload
        memcpy(p_data, p_avrcp_data, avrcp_length);

        mpa_send_l2c_data_req(avctp_buf, p_link->browsing_chann.ds_data_offset, p_link->browsing_chann.cid,
                              AVCTP_NON_FRAG_HDR_LENGTH + avrcp_length, false);

        return true;
    }

    return false;
}

void avctp_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *p_l2c_data_ind)
{
    T_AVCTP_LINK *p_link = avctp_find_link_by_cid(p_l2c_data_ind->cid);

    if (p_link != NULL)
    {
        uint8_t *p_data = p_l2c_data_ind->data + p_l2c_data_ind->gap;
        uint16_t length = p_l2c_data_ind->length;
        uint8_t crtype = (*p_data & 0x02) >> 1;
        uint8_t packet_type = (*p_data & 0x0c) >> 2;
        uint8_t transact_label = *p_data >> 4;

        p_data++;
        length--;

        //recombine avctp frames(<=L2CAP MTU) to avrcp packets(<=512 bytes)
        if (packet_type == AVCTP_PKT_TYPE_UNFRAG)
        {
            if (p_link->control_chann.recombine.p_buf != NULL) //last recombination not completed, abort it
            {
                os_mem_free(p_link->control_chann.recombine.p_buf);
                memset(&p_link->control_chann.recombine, 0, sizeof(p_link->control_chann.recombine));
            }

            //recombine.profile_id also stores Profile Identifier of UNFRAGMENT packets
            BE_ARRAY_TO_UINT16(p_link->control_chann.recombine.profile_id, p_data);
            p_data += 2;
            length -= 2;
        }
        else
        {
            if (packet_type == AVCTP_PKT_TYPE_START)
            {
                p_link->control_chann.recombine.number_of_packets = *p_data++;
                length--;
                BE_ARRAY_TO_UINT16(p_link->control_chann.recombine.profile_id, p_data);
                p_data += 2;
                length -= 2;

                //init recombine struct
                if (p_link->control_chann.recombine.p_buf == NULL)
                {
                    p_link->control_chann.recombine.recombine_length =
                        p_link->control_chann.recombine.number_of_packets * (length - AVCTP_START_HDR_LENGTH);
                    p_link->control_chann.recombine.p_buf = os_mem_alloc2(
                                                                p_link->control_chann.recombine.recombine_length);
                    if (p_link->control_chann.recombine.p_buf == NULL)
                    {
                        //PROTOCOL_PRINT_WARN0("avctp_handle_l2c_data_ind: get memory fail for recombine");
                        avctp_disconnect_req(p_link->bd_addr);
                        return;
                    }
                }
                else //last recombination not completed, retain p_buf
                {}

                p_link->control_chann.recombine.write_index = 0;
            }

            if (p_link->control_chann.recombine.p_buf == NULL) //avctp recombine recv CONTINUE/END without START
            {
                return;
            }

            if ((p_link->control_chann.recombine.number_of_packets ==
                 0) /*received packets > number_of_packets*/
                || ((packet_type == AVCTP_PKT_TYPE_END) &&
                    (p_link->control_chann.recombine.number_of_packets > 1) /*recvd received < number_of_packets*/)
                || (p_link->control_chann.recombine.write_index + length >
                    p_link->control_chann.recombine.recombine_length) /*would exceed p_buf[]*/)
            {
                os_mem_free(p_link->control_chann.recombine.p_buf);
                memset(&p_link->control_chann.recombine, 0, sizeof(p_link->control_chann.recombine));

                avctp_disconnect_req(p_link->bd_addr);
                return;
            }

            memcpy(p_link->control_chann.recombine.p_buf + p_link->control_chann.recombine.write_index, p_data,
                   length);
            p_link->control_chann.recombine.write_index += length;
            p_link->control_chann.recombine.number_of_packets--;

            if (packet_type == AVCTP_PKT_TYPE_END)
            {
                p_data = p_link->control_chann.recombine.p_buf;
                length = p_link->control_chann.recombine.write_index;
            }
        }

        if ((packet_type == AVCTP_PKT_TYPE_UNFRAG) || (packet_type == AVCTP_PKT_TYPE_END))
        {
            //to avrcp
            if (p_link->control_chann.recombine.profile_id == UUID_AV_REMOTE_CONTROL)
            {
                T_AVCTP_DATA_IND avctp_data_ind;
                avctp_data_ind.transact_label = transact_label;
                avctp_data_ind.crtype = crtype;
                avctp_data_ind.length = length;
                avctp_data_ind.p_data = p_data;
                p_avctp->cback(p_l2c_data_ind->cid, AVCTP_MSG_DATA_IND, (uint8_t *)&avctp_data_ind);
            }
            else
            {
                uint8_t *avctp_buf;

                avctp_buf = mpa_get_l2c_buf(p_avctp->queue_id, p_link->control_chann.cid, 0,
                                            AVCTP_NON_FRAG_HDR_LENGTH,
                                            p_link->control_chann.ds_data_offset,
                                            false);
                if (avctp_buf != NULL)
                {
                    avctp_buf[p_link->control_chann.ds_data_offset] = ((transact_label << 4) & 0xf0)
                                                                      | ((AVCTP_PKT_TYPE_UNFRAG << 2) & 0x0c)
                                                                      | ((AVCTP_MSG_TYPE_RSP << 1) & 0x2)
                                                                      | (0x01); //IPID
                    avctp_buf[p_link->control_chann.ds_data_offset + 1] = (uint8_t)(
                                                                              p_link->control_chann.recombine.profile_id >> 8);
                    avctp_buf[p_link->control_chann.ds_data_offset + 2] = (uint8_t)(
                                                                              p_link->control_chann.recombine.profile_id);
                    mpa_send_l2c_data_req(avctp_buf, p_link->control_chann.ds_data_offset, p_link->control_chann.cid,
                                          AVCTP_NON_FRAG_HDR_LENGTH, false);
                }
            }

            if (packet_type == AVCTP_PKT_TYPE_END) //normal clean
            {
                os_mem_free(p_link->control_chann.recombine.p_buf);
                memset(&p_link->control_chann.recombine, 0, sizeof(p_link->control_chann.recombine));
            }
        }
    }
}

void avctp_browsing_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *p_l2c_data_ind)
{
    T_AVCTP_LINK *p_link = avctp_find_link_by_cid(p_l2c_data_ind->cid);

    if (p_link != NULL)
    {
        uint8_t *p_data = p_l2c_data_ind->data + p_l2c_data_ind->gap;
        uint16_t length = p_l2c_data_ind->length;
        uint8_t crtype = (*p_data & 0x02) >> 1;
        uint8_t transact_label = *p_data >> 4;

        p_data++;
        length--;

        BE_ARRAY_TO_UINT16(p_link->browsing_chann.profile_id, p_data);
        p_data += 2;
        length -= 2;

        if (p_link->browsing_chann.profile_id == UUID_AV_REMOTE_CONTROL)
        {
            T_AVCTP_DATA_IND avctp_data_ind;
            avctp_data_ind.transact_label = transact_label;
            avctp_data_ind.crtype = crtype;
            avctp_data_ind.length = length;
            avctp_data_ind.p_data = p_data;
            p_avctp->cback(p_l2c_data_ind->cid, AVCTP_MSG_BROWSING_DATA_IND, (uint8_t *)&avctp_data_ind);
        }
        else
        {
            uint8_t *avctp_buf;

            avctp_buf = mpa_get_l2c_buf(p_avctp->queue_id, p_link->browsing_chann.cid,
                                        0, AVCTP_NON_FRAG_HDR_LENGTH,
                                        p_link->browsing_chann.ds_data_offset,
                                        false);
            if (avctp_buf != NULL)
            {
                avctp_buf[p_link->browsing_chann.ds_data_offset] = ((transact_label << 4) & 0xf0)
                                                                   | ((AVCTP_PKT_TYPE_UNFRAG << 2) & 0x0c)
                                                                   | ((AVCTP_MSG_TYPE_RSP << 1) & 0x2)
                                                                   | (0x01); //IPID
                avctp_buf[p_link->browsing_chann.ds_data_offset + 1] = (uint8_t)(p_link->browsing_chann.profile_id
                                                                                 >> 8);
                avctp_buf[p_link->browsing_chann.ds_data_offset + 2] = (uint8_t)(p_link->browsing_chann.profile_id);
                mpa_send_l2c_data_req(avctp_buf, p_link->browsing_chann.ds_data_offset, p_link->browsing_chann.cid,
                                      AVCTP_NON_FRAG_HDR_LENGTH, false);
            }
        }
    }
}

void avctp_callback(void        *p_buf,
                    T_PROTO_MSG  l2c_msg)
{
    switch (l2c_msg)
    {
    case L2C_CONN_IND:
        {
            T_MPA_L2C_CONN_IND *ind = (T_MPA_L2C_CONN_IND *)p_buf;
            T_AVCTP_LINK *p_link = avctp_find_link_by_addr(ind->bd_addr);

            if (ind->proto_id == p_avctp->queue_id)
            {
                if (p_link == NULL) //Local BTM not send avctp connect request
                {
                    p_link = avctp_alloc_link(ind->bd_addr);
                    if (p_link != NULL)
                    {
                        p_link->control_chann.cid = ind->cid;
                        p_link->control_chann.state = AVCTP_STATE_CONNECTING;
                        p_avctp->cback(ind->cid, AVCTP_MSG_CONN_IND, p_buf);
                    }
                    else
                    {
                        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                              AVCTP_L2CAP_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                    }
                }
                else
                {
                    mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                          AVCTP_L2CAP_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                }
            }
            else if (ind->proto_id == p_avctp->queue_id_browsing)
            {
                if (p_link == NULL)
                {
                    mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                          AVCTP_BROWSING_L2CAP_MTU_SIZE, MPA_L2C_MODE_ERTM, 0xFFFF);
                }
                else
                {
                    if (p_link->browsing_chann.state == AVCTP_STATE_DISCONNECTED)
                    {
                        p_link->browsing_chann.cid = ind->cid;
                        p_link->browsing_chann.state = AVCTP_STATE_CONNECTING;
                        p_avctp->cback(ind->cid, AVCTP_MSG_BROWSING_CONN_IND, p_buf);
                    }
                    else
                    {
                        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                              AVCTP_BROWSING_L2CAP_MTU_SIZE, MPA_L2C_MODE_ERTM, 0xFFFF);
                    }
                }
            }
        }
        break;

    case L2C_CONN_RSP:
        {
            T_MPA_L2C_CONN_RSP *ind = (T_MPA_L2C_CONN_RSP *)p_buf;
            T_AVCTP_LINK *p_link = avctp_find_link_by_addr(ind->bd_addr);

            if (ind->proto_id == p_avctp->queue_id)
            {
                if (ind->cause == 0)
                {
                    if (p_link != NULL)
                    {
                        p_link->control_chann.cid = ind->cid;
                        p_avctp->cback(ind->cid, AVCTP_MSG_CONN_RSP, p_buf);
                    }
                    else
                    {
                        avctp_disconnect_req(ind->bd_addr);
                    }
                }
                else
                {
                    p_avctp->cback(ind->cid, AVCTP_MSG_CONN_FAIL, &ind->cause);
                    avctp_disconnect_req(ind->bd_addr);
                }
            }
            else if (ind->proto_id == p_avctp->queue_id_browsing)
            {
                if (ind->cause == 0)
                {
                    if (p_link != NULL)
                    {
                        p_link->browsing_chann.cid = ind->cid;
                        p_avctp->cback(ind->cid, AVCTP_MSG_BROWSING_CONN_RSP, p_buf);
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
            T_MPA_L2C_CONN_CMPL_INFO *p_info = (T_MPA_L2C_CONN_CMPL_INFO *)p_buf;
            T_AVCTP_LINK *p_link = avctp_find_link_by_cid(p_info->cid);

            if (p_info->proto_id == p_avctp->queue_id)
            {
                if (p_link != NULL)
                {
                    if (p_info->cause)
                    {
                        avctp_free_link(p_link);
                        p_avctp->cback(p_info->cid, AVCTP_MSG_CONN_FAIL, &p_info->cause);
                    }
                    else
                    {
                        p_link->control_chann.remote_mtu = p_info->remote_mtu;
                        p_link->control_chann.ds_data_offset = p_info->ds_data_offset;
                        p_link->control_chann.state = AVCTP_STATE_CONNECTED;
                        p_avctp->cback(p_info->cid, AVCTP_MSG_CONN_CMPL_IND, p_buf);
                    }
                }
            }
            else if (p_info->proto_id == p_avctp->queue_id_browsing)
            {
                if (p_link != NULL)
                {
                    if (p_info->cause)
                    {
                        memset(&(p_link->browsing_chann), 0, sizeof(T_AVCTP_BROWSING_CHANN));
                        p_avctp->cback(p_info->cid, AVCTP_MSG_BROWSING_DISCONN_IND, &p_info->cause);
                    }
                    else
                    {
                        p_link->browsing_chann.remote_mtu = p_info->remote_mtu;
                        p_link->browsing_chann.ds_data_offset = p_info->ds_data_offset;
                        p_link->browsing_chann.state = AVCTP_STATE_CONNECTED;
                        p_avctp->cback(p_info->cid, AVCTP_MSG_BROWSING_CONN_CMPL_IND, p_buf);
                    }
                }
            }
        }
        break;

    case L2C_DATA_IND:
        {
            T_MPA_L2C_DATA_IND *ind = (T_MPA_L2C_DATA_IND *)p_buf;
            if (ind->proto_id == p_avctp->queue_id)
            {
                avctp_handle_l2c_data_ind(ind);
            }
            else if (ind->proto_id == p_avctp->queue_id_browsing)
            {
                avctp_browsing_handle_l2c_data_ind(ind);
            }
        }
        break;

    case L2C_DISCONN_IND:
        {
            T_MPA_L2C_DISCONN_IND *ind = (T_MPA_L2C_DISCONN_IND *)p_buf;
            T_AVCTP_LINK *p_link = avctp_find_link_by_cid(ind->cid);

            if (ind->proto_id == p_avctp->queue_id)
            {
                if (p_link != NULL)
                {
                    avctp_free_link(p_link);
                }
                mpa_send_l2c_disconn_cfm(ind->cid);
                p_avctp->cback(ind->cid, AVCTP_MSG_DISCONN_IND, &ind->cause);
            }
            else if (ind->proto_id == p_avctp->queue_id_browsing)
            {
                if (p_link != NULL)
                {
                    memset(&(p_link->browsing_chann), 0, sizeof(T_AVCTP_BROWSING_CHANN));
                }
                mpa_send_l2c_disconn_cfm(ind->cid);
                p_avctp->cback(ind->cid, AVCTP_MSG_BROWSING_DISCONN_IND, &ind->cause);
            }
        }
        break;

    case L2C_DISCONN_RSP:
        {
            T_MPA_L2C_DISCONN_RSP *ind = (T_MPA_L2C_DISCONN_RSP *)p_buf;
            T_AVCTP_LINK *p_link = avctp_find_link_by_cid(ind->cid);

            if (ind->proto_id == p_avctp->queue_id)
            {
                if (p_link != NULL)
                {
                    avctp_free_link(p_link);
                    p_avctp->cback(ind->cid, AVCTP_MSG_DISCONN_IND, &ind->cause);
                }
            }
            else if (ind->proto_id == p_avctp->queue_id_browsing)
            {
                if (p_link != NULL)
                {
                    p_link->browsing_chann.state = AVCTP_STATE_DISCONNECTED;
                    p_avctp->cback(ind->cid, AVCTP_MSG_BROWSING_DISCONN_IND, &ind->cause);
                }
            }
        }
        break;

    default:
        PROTOCOL_PRINT_WARN1("avctp rx unkown mpa msg=%x", l2c_msg);
        break;
    }
}

bool avctp_init(uint8_t       link_num,
                P_AVCTP_CBACK cback)
{
    int32_t ret = 0;

    p_avctp = os_mem_zalloc2(sizeof(T_AVCTP));
    if (p_avctp == NULL)
    {
        ret = 1;
        goto fail_alloc_avctp;
    }

    p_avctp->link_num = link_num;
    p_avctp->link = os_mem_zalloc2(p_avctp->link_num * sizeof(T_AVCTP_LINK));
    if (p_avctp->link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (mpa_reg_l2c_proto(PSM_AVCTP, avctp_callback, &p_avctp->queue_id) == false)
    {
        ret = 3;
        goto fail_reg_l2c;
    }
    if (mpa_reg_l2c_proto(PSM_AVCTP_BROWSING, avctp_callback, &p_avctp->queue_id_browsing) == false)
    {
        ret = 4;
        goto fail_reg_l2c;
    }

    p_avctp->cback = cback;

    return true;

fail_reg_l2c:
    os_mem_free(p_avctp->link);
fail_alloc_link:
    os_mem_free(p_avctp);
    p_avctp = NULL;
fail_alloc_avctp:
    PROFILE_PRINT_ERROR1("avctp_init: failed %d", -ret);
    return false;
}

bool avctp_get_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info)
{
    T_AVCTP_LINK *p_link;

    p_link = avctp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_info->l2c_cid = p_link->control_chann.cid;
    p_info->remote_mtu = p_link->control_chann.remote_mtu;
    p_info->data_offset = p_link->control_chann.ds_data_offset;
    p_info->avctp_state = p_link->control_chann.state;

    return true;
}

bool avctp_set_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info)
{
    T_AVCTP_LINK *p_link;

    p_link = avctp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = avctp_alloc_link(bd_addr);
    }

    if (p_link == NULL)
    {
        return false;
    }

    p_link->control_chann.state = (T_AVCTP_STATE)p_info->avctp_state;
    p_link->control_chann.cid = p_info->l2c_cid;
    p_link->control_chann.remote_mtu = p_info->remote_mtu;
    p_link->control_chann.ds_data_offset = p_info->data_offset;

    return true;
}

bool avctp_del_roleswap_info(uint8_t bd_addr[6])
{
    T_AVCTP_LINK *p_link;

    p_link = avctp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    avctp_free_link(p_link);

    return true;
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "avctp.h"

bool avctp_init(uint8_t       link_num,
                P_AVCTP_CBACK cback)
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

#define avctp_send_data(bd_addr, avrcp_data, length, transact, crtype)  \
    avctp_send_data2buf(bd_addr, avrcp_data, length, NULL, 0, transact, crtype)

bool avctp_send_data2buf(uint8_t   bd_addr[6],
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

bool avctp_browsing_send_data2buf(uint8_t   bd_addr[6],
                                  uint8_t  *p_avrcp_data,
                                  uint16_t  avrcp_length,
                                  uint8_t   transact,
                                  uint8_t   crtype)
{
    return false;
}

bool avctp_get_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info)
{
    return false;
}

bool avctp_set_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info)
{
    return false;
}

bool avctp_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}
#endif
