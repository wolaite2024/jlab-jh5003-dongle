/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_A2DP_SUPPORT == 1)
#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "a2dp.h"
#include "avdtp.h"
#include "mpa.h"

/* connected_flag */
#define A2DP_SIG_MASK           0x01
#define A2DP_STREAM_MASK        0x02

/* a2dp_initiator_flag */
#define INITIATE_CONN_MASK       0x01
#define INITIATE_DISCONN_MASK    0x02

typedef struct t_a2dp_link
{
    uint8_t         bd_addr[6];
    uint16_t        sig_cid;
    uint16_t        stream_cid;
    T_A2DP_STATE    state;
    uint8_t         int_flag;
    uint8_t         connected_flag;
} T_A2DP_LINK;

typedef struct t_a2dp
{
    P_A2DP_CBACK    cback;
    T_A2DP_LINK    *link;
    uint8_t         link_num;
} T_A2DP;

T_A2DP *p_a2dp;

T_A2DP_LINK *a2dp_alloc_link(uint8_t bd_addr[6])
{
    uint8_t      i;
    T_A2DP_LINK *p_link = NULL;

    for (i = 0; i < p_a2dp->link_num; i++)
    {
        if (p_a2dp->link[i].state == A2DP_STATE_DISCONNECTED)
        {
            p_link = &p_a2dp->link[i];
            memcpy(p_link->bd_addr, bd_addr, 6);
            p_link->state = A2DP_STATE_ALLOCATED;
            break;
        }
    }

    PROFILE_PRINT_TRACE2("a2dp_alloc_link: bd_addr %s, link %p",
                         TRACE_BDADDR(bd_addr), p_link);

    return p_link;
}

void a2dp_free_link(T_A2DP_LINK *p_link)
{
    PROFILE_PRINT_TRACE1("a2dp_free_link: link %p", p_link);
    memset(p_link, 0, sizeof(T_A2DP_LINK));
}

T_A2DP_LINK *a2dp_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t      i;
    T_A2DP_LINK *p_link = NULL;

    if (bd_addr == NULL)
    {
        return p_link;
    }

    for (i = 0; i < p_a2dp->link_num; i++)
    {
        if (!memcmp(p_a2dp->link[i].bd_addr, bd_addr, 6))
        {
            p_link = &p_a2dp->link[i];
            break;
        }
    }

    return p_link;
}

T_A2DP_LINK *a2dp_find_link_by_cid(uint16_t cid)
{
    uint8_t      i;
    T_A2DP_LINK *p_link = NULL;

    for (i = 0; i < p_a2dp->link_num; i++)
    {
        if (p_a2dp->link[i].sig_cid == cid ||
            p_a2dp->link[i].stream_cid == cid)
        {
            p_link = &p_a2dp->link[i];
            break;
        }
    }

    return p_link;
}

void a2dp_callback(uint16_t     cid,
                   T_AVDTP_MSG  msg_type,
                   void        *msg_buf)
{
    T_A2DP_LINK *p_link;

    if (msg_type != AVDTP_MSG_STREAM_DATA_IND)
    {
        PROFILE_PRINT_TRACE2("a2dp_callback: cid 0x%04x, msg_type 0x%04x", cid, msg_type);
    }

    switch (msg_type)
    {
    case AVDTP_MSG_STREAM_DATA_IND:
        p_link = a2dp_find_link_by_cid(cid);
        if (p_link != NULL)
        {
            T_A2DP_STREAM_IND stream_data;

            stream_data.pkt_ptr = ((T_MPA_L2C_DATA_IND *)msg_buf)->data +
                                  ((T_MPA_L2C_DATA_IND *)msg_buf)->gap;
            stream_data.pkt_len = ((T_MPA_L2C_DATA_IND *)msg_buf)->length;
            stream_data.bt_clock = ((T_MPA_L2C_DATA_IND *)msg_buf)->bt_clock;
            p_a2dp->cback(p_link->bd_addr, A2DP_MSG_STREAM_DATA_IND, (uint8_t *)&stream_data);
        }
        break;

    case AVDTP_MSG_STREAM_DATA_RSP:
        p_link = a2dp_find_link_by_cid(cid);
        if (p_link != NULL)
        {
            p_a2dp->cback(p_link->bd_addr, A2DP_MSG_STREAM_DATA_RSP, NULL);
        }
        break;

    case AVDTP_MSG_SIG_CONN_REQ:
        p_link = a2dp_find_link_by_addr((uint8_t *)msg_buf);
        if (p_link == NULL)
        {
            p_link = a2dp_alloc_link((uint8_t *)msg_buf);
            if (p_link != NULL)
            {
                p_link->state = A2DP_STATE_CONNECTING;
                p_link->sig_cid = cid;
                p_link->int_flag = 0;
                p_a2dp->cback(p_link->bd_addr, A2DP_MSG_CONN_IND, NULL);
            }
            else
            {
                a2dp_signal_connect_cfm((uint8_t *)msg_buf, false);
            }
        }
        else
        {
            a2dp_signal_connect_cfm((uint8_t *)msg_buf, false);
        }
        break;

    case AVDTP_MSG_SIG_CONN_RSP:
        p_link = a2dp_find_link_by_addr((uint8_t *)msg_buf);
        if (p_link != NULL)
        {
            p_link->sig_cid = cid;
        }
        break;

    case AVDTP_MSG_SIG_CONNECTED:
        {
            T_MPA_L2C_CONN_CMPL_INFO *p_info;

            p_info = (T_MPA_L2C_CONN_CMPL_INFO *)msg_buf;

            p_link = a2dp_find_link_by_addr(p_info->bd_addr);
            if (p_link != NULL)
            {
                p_link->connected_flag |= A2DP_SIG_MASK;
                p_a2dp->cback(p_link->bd_addr, A2DP_MSG_CONN_CMPL, NULL);

                if (p_link->int_flag & INITIATE_CONN_MASK)
                {
                    a2dp_signal_discover(p_link->bd_addr);
                }
            }
            else
            {
                avdtp_signal_disconnect_req(p_info->bd_addr);
            }
        }
        break;

    case AVDTP_MSG_SIG_CONN_FAIL:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_a2dp->cback(p_link->bd_addr, A2DP_MSG_CONN_FAIL, msg_buf);
                a2dp_free_link(p_link);
            }
        }
        break;

    case AVDTP_MSG_SIG_DISCONNECTED:
        {
            uint16_t info = *(uint16_t *)msg_buf;
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_a2dp->cback(p_link->bd_addr, A2DP_MSG_DISCONN, &info);
                a2dp_free_link(p_link);
            }
        }
        break;

    case AVDTP_MSG_STREAM_CONN_REQ:
    case AVDTP_MSG_STREAM_CONN_RSP:
        p_link = a2dp_find_link_by_addr((uint8_t *)msg_buf);
        if (p_link != NULL)
        {
            p_link->stream_cid = cid;
        }
        break;

    case AVDTP_MSG_STREAM_CONNECTED:
        {
            T_MPA_L2C_CONN_CMPL_INFO *p_info;

            p_info = (T_MPA_L2C_CONN_CMPL_INFO *)msg_buf;

            p_link = a2dp_find_link_by_addr(p_info->bd_addr);
            if (p_link != NULL)
            {
                p_link->connected_flag |= A2DP_STREAM_MASK;
                p_a2dp->cback(p_link->bd_addr, A2DP_MSG_OPEN, &p_info->remote_mtu);
            }
            else
            {
                avdtp_stream_disconnect_req(p_info->bd_addr);
            }
        }
        break;

    case AVDTP_MSG_STREAM_CONN_FAIL:
        p_link = a2dp_find_link_by_cid(cid);
        if (p_link != NULL)
        {
            p_link->stream_cid = 0;
            p_a2dp->cback(p_link->bd_addr, A2DP_MSG_OPEN_FAIL, msg_buf);
        }
        break;

    case AVDTP_MSG_STREAM_DISCONNECTED:
        p_link = a2dp_find_link_by_cid(cid);
        if (p_link != NULL)
        {
            uint16_t info = *(uint16_t *)msg_buf;

            p_link->connected_flag &= ~A2DP_STREAM_MASK;
            p_link->stream_cid = 0;
            p_a2dp->cback(p_link->bd_addr, A2DP_MSG_CLOSE, &info);

            if (p_link->int_flag & INITIATE_DISCONN_MASK)
            {
                p_link->int_flag &= ~INITIATE_DISCONN_MASK;
                avdtp_signal_disconnect_req(p_link->bd_addr);
            }
        }
        break;

    case AVDTP_MSG_SIG_START_IND:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_a2dp->cback(p_link->bd_addr, A2DP_MSG_START_IND, NULL);
            }
        }
        break;

    case AVDTP_MSG_SIG_START_RSP:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_link->state = A2DP_STATE_STREAMING;
                p_a2dp->cback(p_link->bd_addr, A2DP_MSG_START_RSP, NULL);
            }
        }
        break;

    case AVDTP_MSG_SIG_OPEN_IND:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_link->state = A2DP_STATE_CONNECTED;
            }
        }
        break;

    case AVDTP_MSG_SIG_OPEN_RSP:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_link->state = A2DP_STATE_CONNECTED;
                a2dp_stream_connect_req(p_link->bd_addr);
            }
        }
        break;

    case AVDTP_MSG_SIG_SET_CFG_IND:
    case AVDTP_MSG_SIG_SET_CFG_RSP:
    case AVDTP_MSG_SIG_RECFG_IND:
    case AVDTP_MSG_SIG_RECFG_RSP:
        {
            uint8_t *p_data;

            p_data = (uint8_t *)msg_buf;

            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                T_A2DP_CFG cfg_param;

                cfg_param.role = p_data[0];
                cfg_param.codec_type = p_data[1];
                cfg_param.cp_flag = p_data[2];
                cfg_param.delay_report_flag = p_data[3];
                memcpy(cfg_param.codec_info, &p_data[4], MAX_CODEC_INFO_SIZE);

                if ((msg_type == AVDTP_MSG_SIG_SET_CFG_IND) || (msg_type == AVDTP_MSG_SIG_SET_CFG_RSP))
                {
                    p_a2dp->cback(p_link->bd_addr, A2DP_MSG_SET_CFG, (uint8_t *)&cfg_param);
                }
                else
                {
                    p_a2dp->cback(p_link->bd_addr, A2DP_MSG_RE_CFG, (uint8_t *)&cfg_param);
                }
            }
        }
        break;

    case AVDTP_MSG_SIG_SUSPEND_IND:
    case AVDTP_MSG_SIG_SUSPEND_RSP:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_link->state = A2DP_STATE_CONNECTED;
                p_a2dp->cback(p_link->bd_addr, A2DP_MSG_SUSPEND, NULL);
            }
        }
        break;

    case AVDTP_MSG_SIG_ABORT_IND:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_link->state = A2DP_STATE_DISCONNECTING;
            }
        }
        break;

    case AVDTP_MSG_SIG_ABORT_RSP:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_link->state = A2DP_STATE_DISCONNECTING;

                if (p_link->stream_cid)
                {
                    avdtp_stream_disconnect_req(p_link->bd_addr);
                }
            }
        }
        break;

    case AVDTP_MSG_SIG_CLOSE_IND:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_link->state = A2DP_STATE_DISCONNECTING;
            }
        }
        break;

    case AVDTP_MSG_SIG_CLOSE_RSP:
        {
            p_link = a2dp_find_link_by_cid(cid);
            if (p_link != NULL)
            {
                p_link->state = A2DP_STATE_DISCONNECTING;

                if (p_link->stream_cid)
                {
                    avdtp_stream_disconnect_req(p_link->bd_addr);
                }
            }
        }
        break;

    default:
        break;
    }
}

bool a2dp_signal_connect_req(uint8_t  bd_addr[6],
                             uint16_t avdtp_ver,
                             uint8_t  role)
{
    T_A2DP_LINK *p_link;
    bool         ret = false;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = a2dp_alloc_link(bd_addr);
        if (p_link != NULL)
        {
            p_link->state = A2DP_STATE_CONNECTING;
            p_link->int_flag = INITIATE_CONN_MASK;
            avdtp_signal_connect_req(bd_addr, avdtp_ver, role);
            ret = true;
        }
    }

    PROFILE_PRINT_TRACE2("a2dp_signal_connect_req: bd_addr %s, avdtp_ver 0x%04x",
                         TRACE_BDADDR(bd_addr), avdtp_ver);

    return ret;
}

bool a2dp_stream_connect_req(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;
    bool         ret = false;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        avdtp_stream_connect_req(bd_addr);
        ret = true;
    }

    PROFILE_PRINT_TRACE1("a2dp_stream_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    return ret;
}

bool a2dp_signal_connect_cfm(uint8_t bd_addr[6],
                             bool    accept)
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (accept == false)
        {
            a2dp_free_link(p_link);
        }

        return avdtp_signal_connect_cfm(bd_addr, accept);
    }

    return avdtp_signal_connect_cfm(bd_addr, false);
}

bool a2dp_disconnect_req(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->stream_cid)
        {
            p_link->int_flag |= INITIATE_DISCONN_MASK;
            return avdtp_signal_close_req(bd_addr);
        }
        else if (p_link->sig_cid)
        {
            return avdtp_signal_disconnect_req(p_link->bd_addr);
        }
    }

    return false;
}

bool a2dp_signal_discover(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_signal_discover_req(bd_addr);
    }

    return false;
}

bool a2dp_signal_cpbs_get(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_signal_get_capability_req(bd_addr);
    }

    return false;
}

bool a2dp_signal_cfg_set(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_signal_cfg_req(bd_addr);
    }

    return false;
}

bool a2dp_signal_open(uint8_t bd_addr[6],
                      uint8_t role)
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_signal_open_req(bd_addr, role);
    }

    return false;
}

bool a2dp_signal_start(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_signal_start_req(bd_addr);
    }

    return false;
}

bool a2dp_signal_close(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_signal_close_req(bd_addr);
    }

    return false;
}

bool a2dp_signal_suspend(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_signal_suspend_req(bd_addr);
    }

    return false;
}

bool a2dp_signal_abort(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_signal_abort_req(bd_addr);
    }

    return false;
}

bool a2dp_signal_delay_report(uint8_t  bd_addr[6],
                              uint16_t latency)
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_signal_delay_report_req(bd_addr, latency);
    }

    return false;
}

bool a2dp_signal_start_cfm(uint8_t bd_addr[6],
                           bool    accept)
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (accept == true)
        {
            p_link->state = A2DP_STATE_STREAMING;
        }

        return avdtp_signal_start_cfm(bd_addr, accept);
    }

    return false;
}

bool a2dp_stream_data_send(uint8_t   bd_addr[6],
                           uint16_t  seq_num,
                           uint32_t  time_stamp,
                           uint8_t   frame_num,
                           uint8_t  *p_data,
                           uint16_t  len)
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avdtp_stream_data_send(p_link->bd_addr, seq_num, time_stamp, frame_num, p_data, len);
    }

    return false;
}

bool a2dp_codec_add(uint8_t            role,
                    T_A2DP_CODEC_TYPE  codec_type,
                    uint8_t           *media_codec_info)
{
    return avdtp_codec_add(role, codec_type, media_codec_info);
}

bool a2dp_codec_delete(uint8_t            role,
                       T_A2DP_CODEC_TYPE  codec_type)
{
    return avdtp_codec_delete(role, codec_type);
}

bool a2dp_init(uint8_t      link_num,
               uint16_t     latency,
               uint8_t      service_capabilities,
               P_A2DP_CBACK cback)
{
    int32_t ret = 0;

    p_a2dp = os_mem_zalloc2(sizeof(T_A2DP));
    if (p_a2dp == NULL)
    {
        ret = 1;
        goto fail_alloc_a2dp;
    }

    p_a2dp->link_num = link_num;
    p_a2dp->link = os_mem_zalloc2(p_a2dp->link_num * sizeof(T_A2DP_LINK));
    if (p_a2dp->link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    p_a2dp->cback = cback;

    if (avdtp_init(p_a2dp->link_num, latency, service_capabilities, a2dp_callback) == false)
    {
        ret = 3;
        goto fail_init_avdtp;
    }

    return true;

fail_init_avdtp:
    os_mem_free(p_a2dp->link);
fail_alloc_link:
    os_mem_free(p_a2dp);
    p_a2dp = NULL;
fail_alloc_a2dp:
    PROFILE_PRINT_ERROR1("a2dp_init: failed %d", -ret);
    return false;
}

bool a2dp_get_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_A2DP_INFO *p_info)
{
    T_A2DP_LINK *p_link;
    bool         ret = false;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (avdtp_get_roleswap_info(p_link->bd_addr, p_info) == true)
        {
            p_info->sig_cid = p_link->sig_cid;
            p_info->stream_cid = p_link->stream_cid;
            p_info->state = p_link->state;

            ret = true;
        }
    }

    return ret;
}

bool a2dp_set_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_A2DP_INFO *p_info)
{
    T_A2DP_LINK *p_link;
    bool         ret = false;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = a2dp_alloc_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if (avdtp_set_roleswap_info(bd_addr, p_info) == true)
        {
            p_link->int_flag = 0;
            p_link->connected_flag = A2DP_SIG_MASK | A2DP_STREAM_MASK;
            p_link->state = (T_A2DP_STATE)p_info->state;
            p_link->sig_cid = p_info->sig_cid;
            p_link->stream_cid = p_info->stream_cid;

            ret = true;
        }
    }

    return ret;
}

bool a2dp_del_roleswap_info(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    a2dp_free_link(p_link);

    return avdtp_del_roleswap_info(bd_addr);
}

uint16_t a2dp_get_stream_cid(uint8_t bd_addr[6])
{
    T_A2DP_LINK *p_link;

    p_link = a2dp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return 0xffff;
    }
    else
    {
        return p_link->stream_cid;
    }
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "a2dp.h"

bool a2dp_init(uint8_t      link_num,
               uint16_t     latency,
               uint8_t      service_capabilities,
               P_A2DP_CBACK cback)
{
    return false;
}

bool a2dp_codec_add(uint8_t            role,
                    T_A2DP_CODEC_TYPE  codec_type,
                    uint8_t           *media_codec_info)
{
    return false;
}

bool a2dp_codec_delete(uint8_t            role,
                       T_A2DP_CODEC_TYPE  codec_type)
{
    return false;
}

bool a2dp_signal_discover(uint8_t bd_addr[6])
{
    return false;
}

bool a2dp_signal_cpbs_get(uint8_t bd_addr[6])
{
    return false;
}

bool a2dp_signal_cfg_set(uint8_t bd_addr[6])
{
    return false;
}

bool a2dp_signal_open(uint8_t bd_addr[6], uint8_t role)
{
    return false;
}

bool a2dp_signal_start(uint8_t bd_addr[6])
{
    return false;
}

bool a2dp_signal_close(uint8_t bd_addr[6])
{
    return false;
}

bool a2dp_signal_suspend(uint8_t bd_addr[6])
{
    return false;
}

bool a2dp_signal_abort(uint8_t bd_addr[6])
{
    return false;
}

bool a2dp_signal_delay_report(uint8_t  bd_addr[6],
                              uint16_t latency)
{
    return false;
}

bool a2dp_stream_data_send(uint8_t   bd_addr[6],
                           uint16_t  seq_num,
                           uint32_t  time_stamp,
                           uint8_t   frame_num,
                           uint8_t  *p_data,
                           uint16_t  len)
{
    return false;
}

bool a2dp_signal_connect_req(uint8_t  bd_addr[6],
                             uint16_t avdtp_ver,
                             uint8_t  role)
{
    return false;
}

bool a2dp_stream_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool a2dp_signal_connect_cfm(uint8_t bd_addr[6],
                             bool    accept)
{
    return false;
}

bool a2dp_signal_start_cfm(uint8_t bd_addr[6],
                           bool    accept)
{
    return false;
}

bool a2dp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool a2dp_get_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_A2DP_INFO *p_info)
{
    return false;
}

bool a2dp_set_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_A2DP_INFO *p_info)
{
    return false;
}

bool a2dp_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}

uint16_t a2dp_get_stream_cid(uint8_t bd_addr[6])
{
    return false;
}
#endif
