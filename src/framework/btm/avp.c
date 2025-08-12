/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_AVP_SUPPORT == 1)
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "avp.h"
#include "avp_att.h"


#define AVP_LINK_NUM       2
#define AVP_MTU_SIZE       1017
#define AVP_FLUSH_TOUT     40
#define PSM_AVP_CONTROL    0x1001
#define UUID_AVP_CONTROL   0x1001

typedef struct
{
    uint16_t            cid;
    T_AVP_STATE         state;
    uint16_t            data_offset;
} T_AVP_CONTROL_CHANN;

typedef struct
{
    uint16_t            cid;
    T_AVP_STATE         state;
    uint16_t            data_offset;
} T_AVP_AUDIO_CHANN;

typedef struct
{
    uint8_t             bd_addr[6];
    T_AVP_CONTROL_CHANN control_chann;
    T_AVP_AUDIO_CHANN   audio_chann;
} T_AVP_LINK;

typedef struct
{
    T_AVP_LINK         link[AVP_LINK_NUM];
    P_AVP_CBACK        cback;
    uint8_t            queue_id_control;
    uint8_t            queue_id_audio;
} T_AVP;

static T_AVP *p_avp;

T_AVP_LINK *avp_alloc_link(uint8_t bd_addr[6])
{
    uint8_t        i;
    T_AVP_LINK    *p_link = NULL;

    for (i = 0; i < AVP_LINK_NUM; i++)
    {
        if ((p_avp->link[i].control_chann.state == AVP_STATE_DISCONNECTED) &&
            (p_avp->link[i].audio_chann.state == AVP_STATE_DISCONNECTED))
        {
            p_link = &p_avp->link[i];
            memcpy(p_link->bd_addr, bd_addr, 6);
            break;
        }
    }

    return p_link;
}

void avp_free_link(T_AVP_LINK *p_link)
{
    memset(p_link, 0, sizeof(T_AVP_LINK));
}

T_AVP_LINK *avp_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t        i;
    T_AVP_LINK    *p_link = NULL;

    if (bd_addr == NULL)
    {
        return p_link;
    }

    for (i = 0; i < AVP_LINK_NUM; i++)
    {
        if (!memcmp(p_avp->link[i].bd_addr, bd_addr, 6))
        {
            p_link = &p_avp->link[i];
            break;
        }
    }

    return p_link;
}

T_AVP_LINK *avp_find_link_by_cid(uint16_t cid)
{
    uint8_t         i;
    T_AVP_LINK    *p_link = NULL;

    for (i = 0; i < AVP_LINK_NUM; i++)
    {
        if ((p_avp->link[i].control_chann.cid == cid) ||
            (p_avp->link[i].audio_chann.cid == cid))
        {
            p_link = &p_avp->link[i];
            break;
        }
    }

    return p_link;
}

bool avp_control_conn_req(uint8_t bd_addr[6])
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->control_chann.state == AVP_STATE_DISCONNECTED)
        {
            p_link->control_chann.state = AVP_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_AVP_CONTROL, UUID_AVP_CONTROL, p_avp->queue_id_control, AVP_MTU_SIZE,
                                  bd_addr, MPA_L2C_MODE_BASIC, AVP_FLUSH_TOUT);
            return true;
        }
    }
    else
    {
        p_link = avp_alloc_link(bd_addr);
        if (p_link != NULL)
        {
            p_link->control_chann.state = AVP_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_AVP_CONTROL, UUID_AVP_CONTROL, p_avp->queue_id_control, AVP_MTU_SIZE,
                                  bd_addr, MPA_L2C_MODE_BASIC, AVP_FLUSH_TOUT);
            return true;
        }
    }

    return false;
}

bool avp_audio_conn_req(uint8_t bd_addr[6])
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->audio_chann.state == AVP_STATE_DISCONNECTED)
        {
            p_link->audio_chann.state = AVP_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_ATT, UUID_ATT, p_avp->queue_id_audio, AVP_MTU_SIZE,
                                  bd_addr, MPA_L2C_MODE_BASIC, AVP_FLUSH_TOUT);
            return true;
        }
    }
    else
    {
        p_link = avp_alloc_link(bd_addr);
        if (p_link != NULL)
        {
            p_link->audio_chann.state = AVP_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_ATT, UUID_ATT, p_avp->queue_id_audio, AVP_MTU_SIZE,
                                  bd_addr, MPA_L2C_MODE_BASIC, AVP_FLUSH_TOUT);
            return true;
        }
    }

    return false;
}

bool avp_control_disconn_req(uint8_t bd_addr[6])
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->control_chann.state == AVP_STATE_CONNECTED)
        {
            mpa_send_l2c_disconn_req(p_link->control_chann.cid);
        }

        return true;
    }

    return false;
}

bool avp_audio_disconn_req(uint8_t bd_addr[6])
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->audio_chann.state == AVP_STATE_CONNECTED)
        {
            mpa_send_l2c_disconn_req(p_link->audio_chann.cid);
        }

        return true;
    }

    return false;
}

void avp_handle_data_ind(T_MPA_L2C_DATA_IND *p_data_ind)
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_cid(p_data_ind->cid);
    if (p_link != NULL)
    {
        if (p_data_ind->proto_id == p_avp->queue_id_control)
        {
            T_AVP_DATA_IND rx_data;

            rx_data.p_data = p_data_ind->data + p_data_ind->gap;
            rx_data.data_length = p_data_ind->length;
            p_avp->cback(p_link->bd_addr, AVP_MSG_CONTROL_DATA_IND, &rx_data);
        }
        else if (p_data_ind->proto_id == p_avp->queue_id_audio)
        {
            T_AVP_DATA_IND rx_data;

            rx_data.p_data = p_data_ind->data + p_data_ind->gap;
            rx_data.data_length = p_data_ind->length;
            p_avp->cback(p_link->bd_addr, AVP_MSG_AUDIO_DATA_IND, &rx_data);
        }
    }
}

void avp_handle_conn_req(T_MPA_L2C_CONN_IND *p_conn_ind)
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(p_conn_ind->bd_addr);

    if (p_conn_ind->proto_id == p_avp->queue_id_control)
    {
        if (p_link == NULL)
        {
            p_link = avp_alloc_link(p_conn_ind->bd_addr);
            if (p_link != NULL)
            {
                p_link->control_chann.cid = p_conn_ind->cid;
                p_link->control_chann.state = AVP_STATE_CONNECTING;

                p_avp->cback(p_link->bd_addr, AVP_MSG_CONTROL_CONN_REQ, &p_link->control_chann.cid);
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT, p_conn_ind->cid, AVP_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                      AVP_FLUSH_TOUT);
            }
            else
            {
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, p_conn_ind->cid, AVP_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                      AVP_FLUSH_TOUT);
            }
        }
        else
        {
            if (p_link->control_chann.state == AVP_STATE_DISCONNECTED)
            {
                p_link->control_chann.cid = p_conn_ind->cid;
                p_link->control_chann.state = AVP_STATE_CONNECTING;

                p_avp->cback(p_link->bd_addr, AVP_MSG_CONTROL_CONN_REQ, &p_link->control_chann.cid);
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT, p_conn_ind->cid, AVP_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                      AVP_FLUSH_TOUT);
            }
            else
            {
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, p_conn_ind->cid, AVP_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                      AVP_FLUSH_TOUT);
            }
        }
    }
    else if (p_conn_ind->proto_id == p_avp->queue_id_audio)
    {
        if (p_link == NULL)
        {
            p_link = avp_alloc_link(p_conn_ind->bd_addr);
            if (p_link != NULL)
            {
                p_link->audio_chann.cid = p_conn_ind->cid;
                p_link->audio_chann.state = AVP_STATE_CONNECTING;

                p_avp->cback(p_link->bd_addr, AVP_MSG_AUDIO_CONN_REQ, &p_link->audio_chann.cid);
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT, p_conn_ind->cid, AVP_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                      AVP_FLUSH_TOUT);
            }
            else
            {
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, p_conn_ind->cid, AVP_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                      AVP_FLUSH_TOUT);
            }
        }
        else
        {
            if (p_link->audio_chann.state == AVP_STATE_DISCONNECTED)
            {
                p_link->audio_chann.cid = p_conn_ind->cid;
                p_link->audio_chann.state = AVP_STATE_CONNECTING;

                p_avp->cback(p_link->bd_addr, AVP_MSG_AUDIO_CONN_REQ, &p_link->audio_chann.cid);
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT, p_conn_ind->cid, AVP_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                      AVP_FLUSH_TOUT);
            }
            else
            {
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, p_conn_ind->cid, AVP_MTU_SIZE, MPA_L2C_MODE_BASIC,
                                      AVP_FLUSH_TOUT);
            }
        }
    }
}

void avp_handle_conn_rsp(T_MPA_L2C_CONN_RSP *p_conn_rsp)
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(p_conn_rsp->bd_addr);
    if (p_link != NULL)
    {
        if (p_conn_rsp->proto_id == p_avp->queue_id_control)
        {
            if (p_conn_rsp->cause == 0)
            {
                if (p_link->control_chann.state == AVP_STATE_CONNECTING)
                {
                    p_link->control_chann.cid = p_conn_rsp->cid;
                    p_avp->cback(p_conn_rsp->bd_addr, AVP_MSG_CONTROL_CONN_RSP, &p_link->control_chann.cid);
                }
            }
            else
            {
                if (p_link->control_chann.state == AVP_STATE_CONNECTING)
                {
                    if (p_link->audio_chann.state == AVP_STATE_DISCONNECTED)
                    {
                        avp_free_link(p_link);
                    }
                    else
                    {
                        memset(&(p_link->control_chann), 0, sizeof(T_AVP_CONTROL_CHANN));
                    }

                    p_avp->cback(p_conn_rsp->bd_addr, AVP_MSG_CONTROL_CONN_FAIL, NULL);
                }
            }
        }
        else if (p_conn_rsp->proto_id == p_avp->queue_id_audio)
        {
            if (p_conn_rsp->cause == 0)
            {
                if (p_link->audio_chann.state == AVP_STATE_CONNECTING)
                {
                    p_link->audio_chann.cid = p_conn_rsp->cid;
                    p_avp->cback(p_conn_rsp->bd_addr, AVP_MSG_AUDIO_CONN_RSP, &p_link->audio_chann.cid);
                }
            }
            else
            {
                if (p_link->audio_chann.state == AVP_STATE_CONNECTING)
                {
                    if (p_link->control_chann.state == AVP_STATE_DISCONNECTED)
                    {
                        avp_free_link(p_link);
                    }
                    else
                    {
                        memset(&(p_link->audio_chann), 0, sizeof(T_AVP_AUDIO_CHANN));
                    }

                    p_avp->cback(p_conn_rsp->bd_addr, AVP_MSG_AUDIO_CONN_FAIL, NULL);
                }
            }
        }
    }
}

void avp_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *p_conn_cmpl)
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(p_conn_cmpl->bd_addr);
    if (p_link != NULL)
    {
        if (p_conn_cmpl->proto_id == p_avp->queue_id_control)
        {
            if (p_conn_cmpl->cause)
            {
                if (p_link->control_chann.state == AVP_STATE_CONNECTING)
                {
                    if (p_link->audio_chann.state == AVP_STATE_DISCONNECTED)
                    {
                        avp_free_link(p_link);
                    }
                    else
                    {
                        memset(&(p_link->control_chann), 0, sizeof(T_AVP_CONTROL_CHANN));
                    }

                    p_avp->cback(p_conn_cmpl->bd_addr, AVP_MSG_CONTROL_CONN_FAIL, NULL);
                }
            }
            else
            {
                p_link->control_chann.state  = AVP_STATE_CONNECTED;
                p_link->control_chann.data_offset = p_conn_cmpl->ds_data_offset;
                p_avp->cback(p_conn_cmpl->bd_addr, AVP_MSG_CONTROL_CONNECTED, NULL);
            }
        }
        else if (p_conn_cmpl->proto_id == p_avp->queue_id_audio)
        {
            if (p_conn_cmpl->cause)
            {
                if (p_link->audio_chann.state == AVP_STATE_CONNECTING)
                {
                    if (p_link->control_chann.state == AVP_STATE_DISCONNECTED)
                    {
                        avp_free_link(p_link);
                    }
                    else
                    {
                        memset(&(p_link->audio_chann), 0, sizeof(T_AVP_AUDIO_CHANN));
                    }

                    p_avp->cback(p_conn_cmpl->bd_addr, AVP_MSG_AUDIO_CONN_FAIL, NULL);
                }
            }
            else
            {
                p_link->audio_chann.state  = AVP_STATE_CONNECTED;
                p_link->audio_chann.data_offset = p_conn_cmpl->ds_data_offset;
                p_avp->cback(p_conn_cmpl->bd_addr, AVP_MSG_AUDIO_CONNECTED, NULL);
            }
        }
    }
}

void avp_handle_disconn_ind(T_MPA_L2C_DISCONN_IND *p_disconn_ind)
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_cid(p_disconn_ind->cid);
    if (p_link != NULL)
    {
        uint16_t cause;

        cause = p_disconn_ind->cause;
        if (p_disconn_ind->proto_id == p_avp->queue_id_control)
        {
            p_avp->cback(p_link->bd_addr, AVP_MSG_CONTROL_DISCONNECTED, &cause);

            if (p_link->audio_chann.state == AVP_STATE_DISCONNECTED)
            {
                avp_free_link(p_link);
            }
            else
            {
                memset(&(p_link->control_chann), 0, sizeof(T_AVP_CONTROL_CHANN));
            }
        }
        else if (p_disconn_ind->proto_id == p_avp->queue_id_audio)
        {
            p_avp->cback(p_link->bd_addr, AVP_MSG_AUDIO_DISCONNECTED, &cause);

            if (p_link->control_chann.state == AVP_STATE_DISCONNECTED)
            {
                avp_free_link(p_link);
            }
            else
            {
                memset(&(p_link->audio_chann), 0, sizeof(T_AVP_AUDIO_CHANN));
            }
        }
    }

    mpa_send_l2c_disconn_cfm(p_disconn_ind->cid);
}

void avp_handle_disconn_rsp(T_MPA_L2C_DISCONN_RSP *p_disconn_rsp)
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_cid(p_disconn_rsp->cid);
    if (p_link != NULL)
    {
        uint16_t cause;

        cause = p_disconn_rsp->cause;
        if (p_disconn_rsp->proto_id == p_avp->queue_id_control)
        {
            p_avp->cback(p_link->bd_addr, AVP_MSG_CONTROL_DISCONNECTED, &cause);

            if (p_link->audio_chann.state == AVP_STATE_DISCONNECTED)
            {
                avp_free_link(p_link);
            }
            else
            {
                memset(&(p_link->control_chann), 0, sizeof(T_AVP_CONTROL_CHANN));
            }
        }
        else if (p_disconn_rsp->proto_id == p_avp->queue_id_audio)
        {
            p_avp->cback(p_link->bd_addr, AVP_MSG_AUDIO_DISCONNECTED, &cause);

            if (p_link->control_chann.state == AVP_STATE_DISCONNECTED)
            {
                avp_free_link(p_link);
            }
            else
            {
                memset(&(p_link->audio_chann), 0, sizeof(T_AVP_AUDIO_CHANN));
            }
        }
    }
}

bool avp_control_send_data(uint8_t   bd_addr[6],
                           uint8_t  *p_data,
                           uint16_t  data_len,
                           bool      flushable)
{
    T_AVP_LINK *p_link;
    uint8_t     *p_pkt;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->control_chann.state == AVP_STATE_CONNECTED)
        {
            p_pkt = mpa_get_l2c_buf(p_avp->queue_id_control, p_link->control_chann.cid, 0, data_len,
                                    p_link->control_chann.data_offset, false);
            if (p_pkt != NULL)
            {
                memcpy(p_pkt + p_link->control_chann.data_offset, p_data, data_len);
                mpa_send_l2c_data_req(p_pkt, p_link->control_chann.data_offset,
                                      p_link->control_chann.cid, data_len, flushable);
                return true;
            }
        }
    }

    return false;
}

bool avp_audio_send_data(uint8_t  bd_addr[6],
                         uint8_t *p_data,
                         int      data_len)
{
    T_AVP_LINK   *p_link;
    uint8_t      *p_pkt;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->audio_chann.state == AVP_STATE_CONNECTED)
        {
            p_pkt = mpa_get_l2c_buf(p_avp->queue_id_audio, p_link->audio_chann.cid, 0, data_len,
                                    p_link->audio_chann.data_offset, false);
            if (p_pkt != NULL)
            {
                memcpy(p_pkt + p_link->audio_chann.data_offset, p_data, data_len);
                mpa_send_l2c_data_req(p_pkt, p_link->audio_chann.data_offset,
                                      p_link->audio_chann.cid, data_len, false);
                return true;
            }
        }
    }

    return false;
}

void avp_l2c_cback(void        *p_buf,
                   T_PROTO_MSG  msg)
{
    PROTOCOL_PRINT_TRACE1("avp_l2c_cback: msg 0x%02x", msg);

    switch (msg)
    {
    case L2C_CONN_IND:
        avp_handle_conn_req((T_MPA_L2C_CONN_IND *)p_buf);
        break;

    case L2C_CONN_RSP:
        avp_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)p_buf);
        break;

    case L2C_CONN_CMPL:
        avp_handle_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)p_buf);
        break;

    case L2C_DATA_IND:
        avp_handle_data_ind((T_MPA_L2C_DATA_IND *)p_buf);
        break;

    case L2C_DISCONN_IND:
        avp_handle_disconn_ind((T_MPA_L2C_DISCONN_IND *)p_buf);
        break;

    case L2C_DISCONN_RSP:
        avp_handle_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)p_buf);
        break;

    default:
        break;
    }
}

bool avp_profile_callback(uint8_t   service_id,
                          uint16_t  cid,
                          uint8_t  *data,
                          uint16_t  length)
{
    T_AVP_LINK   *p_link;
    T_AVP_DATA_IND rx_data;

    rx_data.p_data = data;
    rx_data.data_length = length;

    p_link = avp_find_link_by_cid(cid);
    if (p_link != NULL)
    {
        p_avp->cback(p_link->bd_addr, AVP_MSG_AUDIO_DATA_IND, &rx_data);
        return true;
    }

    return false;
}

bool avp_voice_recognition_enable_req(uint8_t   bd_addr[6],
                                      uint16_t  conn_handle,
                                      void     *p_value,
                                      uint16_t  length)
{
    T_AVP_LINK   *p_link;
    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->audio_chann.state == AVP_STATE_CONNECTED)
    {
        return avp_att_service_send_notify3(conn_handle,  p_link->audio_chann.cid, p_value, length);
    }

    return false;
}

bool avp_voice_recognition_disable_req(uint8_t   bd_addr[6],
                                       uint16_t  conn_handle,
                                       void     *p_value,
                                       uint16_t  length)
{
    T_AVP_LINK   *p_link;
    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->audio_chann.state == AVP_STATE_CONNECTED)
    {
        return avp_att_service_send_notify3(conn_handle,  p_link->audio_chann.cid, p_value, length);
    }

    return false;
}

bool avp_voice_recognition_encode_start(uint8_t   bd_addr[6],
                                        uint16_t  conn_handle,
                                        void     *p_value,
                                        uint16_t  length)
{
    T_AVP_LINK   *p_link;
    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->audio_chann.state == AVP_STATE_CONNECTED)
    {
        return avp_att_service_send_notify2(conn_handle,  p_link->audio_chann.cid, p_value, length);
    }

    return false;
}

bool avp_voice_recognition_encode_stop(uint8_t   bd_addr[6],
                                       uint16_t  conn_handle,
                                       void     *p_value,
                                       uint16_t  length)
{
    T_AVP_LINK   *p_link;
    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->audio_chann.state == AVP_STATE_CONNECTED)
    {
        return avp_att_service_send_notify3(conn_handle,  p_link->audio_chann.cid, p_value, length);
    }

    return false;
}

bool avp_voice_recognition_data_send(uint8_t   bd_addr[6],
                                     uint16_t  conn_handle,
                                     void     *p_value,
                                     uint16_t  length)
{
    T_AVP_LINK   *p_link;
    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->audio_chann.state == AVP_STATE_CONNECTED)
    {
        return avp_att_service_send_notify1(conn_handle,  p_link->audio_chann.cid, p_value, length);
    }

    return false;
}

bool avp_init(P_AVP_CBACK cback)
{
    int32_t ret = 0;

    p_avp = os_mem_zalloc2(sizeof(T_AVP));
    if (p_avp == NULL)
    {
        ret = 1;
        goto fail_alloc_avp;
    }

    if (mpa_reg_l2c_proto(PSM_AVP_CONTROL, avp_l2c_cback, &p_avp->queue_id_control) == false)
    {
        ret = 2;
        goto fail_reg_l2c;
    }

    mpa_set_att_psm_data_path(MPA_ATT_PSM_DATA_PATH_GATT);
    mpa_reg_l2c_proto(PSM_ATT, avp_l2c_cback, &p_avp->queue_id_audio);

    //app other module do this
    //server_cfg_use_ext_api(true);
    //server_init(2);
    avp_att_add_service(avp_profile_callback);

    p_avp->cback = cback;

    return true;

fail_reg_l2c:
    os_mem_free(p_avp);
    p_avp = NULL;
fail_alloc_avp:
    PROFILE_PRINT_ERROR1("avp_init: failed %d", -ret);
    return false;
}

bool avp_control_del_roleswap_info(uint8_t bd_addr[6])
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        memset(&(p_link->control_chann), 0, sizeof(T_AVP_CONTROL_CHANN));

        if (p_link->audio_chann.state == AVP_STATE_DISCONNECTED)
        {
            avp_free_link(p_link);
        }

        return true;
    }

    return false;
}

bool avp_audio_del_roleswap_info(uint8_t bd_addr[6])
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        memset(&(p_link->audio_chann), 0, sizeof(T_AVP_AUDIO_CHANN));

        if (p_link->control_chann.state == AVP_STATE_DISCONNECTED)
        {
            avp_free_link(p_link);
        }

        return true;
    }

    return false;
}

bool avp_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_AVP_INFO *p_info)
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        p_info->control_l2c_cid = p_link->control_chann.cid;
        p_info->control_state = p_link->control_chann.state;
        p_info->control_data_offset = p_link->control_chann.data_offset;
        p_info->audio_l2c_cid = p_link->audio_chann.cid;
        p_info->audio_state = p_link->audio_chann.state;
        p_info->audio_data_offset = p_link->audio_chann.data_offset;

        return true;
    }

    return false;
}

bool avp_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_AVP_INFO *p_info)
{
    T_AVP_LINK *p_link;

    p_link = avp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = avp_alloc_link(bd_addr);
    }

    if (p_link != NULL)
    {
        memcpy(p_link->bd_addr, bd_addr, 6);

        p_link->control_chann.cid = p_info->control_l2c_cid;
        p_link->control_chann.state = (T_AVP_STATE)p_info->control_state;
        p_link->control_chann.data_offset = p_info->control_data_offset;
        p_link->audio_chann.cid = p_info->audio_l2c_cid;
        p_link->audio_chann.state = (T_AVP_STATE)p_info->audio_state;
        p_link->audio_chann.data_offset = p_info->audio_data_offset;

        return true;
    }

    return false;
}

#else
#include <stdint.h>
#include <stdbool.h>
#include "avp.h"

bool avp_control_conn_req(uint8_t bd_addr[6])
{
    return false;
}
bool avp_audio_conn_req(uint8_t bd_addr[6])
{
    return false;
}

bool avp_control_disconn_req(uint8_t bd_addr[6])
{
    return false;
}

bool avp_audio_disconn_req(uint8_t bd_addr[6])
{
    return false;
}

bool avp_control_send_data(uint8_t   bd_addr[6],
                           uint8_t  *p_data,
                           uint16_t  data_len,
                           bool      flushable)
{
    return false;
}

bool avp_audio_send_data(uint8_t  bd_addr[6],
                         uint8_t *p_data,
                         int      data_len)
{
    return false;
}

bool avp_init(P_AVP_CBACK cback)
{
    return false;
}

bool avp_control_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}

bool avp_audio_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}

bool avp_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_AVP_INFO *p_info)
{
    return false;
}

bool avp_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_AVP_INFO *p_info)
{
    return false;
}
#endif
