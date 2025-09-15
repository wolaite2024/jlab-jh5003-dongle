/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_A2DP_SUPPORT == 1)
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"

#include "sys_ipc.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_a2dp.h"
#include "bt_a2dp_int.h"
#include "avdtp.h"
#include "mpa.h"

/* a2dp_initiator_flag */
#define INITIATE_CONN_MASK       0x01
#define INITIATE_DISCONN_MASK    0x02

#define BT_A2DP_AAC_SR_NUM    (12)
#define BT_A2DP_USAC_SR_NUM   (26)

typedef struct t_bt_a2dp_link
{
    struct t_bt_a2dp_link *next;
    uint8_t                bd_addr[6];
    uint16_t               sig_cid;
    uint16_t               stream_cid;
    T_BT_A2DP_STATE        state;
    uint8_t                int_flag;
    uint8_t                role;
    uint8_t                codec_type;
    uint8_t                content_protect;
    uint16_t               last_seq_num;
    T_BT_A2DP_CODEC_INFO   codec_info;
} T_BT_A2DP_LINK;

typedef struct t_bt_a2dp
{
    T_OS_QUEUE link_list;
} T_BT_A2DP;

static T_BT_A2DP *bt_a2dp;

static const uint8_t bt_a2dp_ldac_codec_info[6] =
{
    0x2d, 0x01, 0x00, 0x00, 0xaa, 0x00
};

static const uint8_t bt_a2dp_lc3_codec_info[6] =
{
    0x5d, 0x00, 0x00, 0x00, 0x08, 0x00
};

static const uint8_t bt_a2dp_lhdc_v5_codec_info[6] =
{
    0x3a, 0x05, 0x00, 0x00, 0x35, 0x4c
};

T_BT_A2DP_LINK *bt_a2dp_alloc_link(uint8_t bd_addr[6])
{
    T_BT_A2DP_LINK *link;

    link = calloc(1, sizeof(T_BT_A2DP_LINK));
    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_in(&bt_a2dp->link_list, link);
    }

    return link;
}

void bt_a2dp_free_link(T_BT_A2DP_LINK *link)
{
    if (os_queue_delete(&bt_a2dp->link_list, link) == true)
    {
        free(link);
    }
}

T_BT_A2DP_LINK *bt_a2dp_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_A2DP_LINK *link;

    link = os_queue_peek(&bt_a2dp->link_list, 0);
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

T_BT_A2DP_LINK *bt_a2dp_find_link_by_cid(uint16_t cid)
{
    T_BT_A2DP_LINK *link;

    link = os_queue_peek(&bt_a2dp->link_list, 0);
    while (link != NULL)
    {
        if (link->sig_cid == cid || link->stream_cid == cid)
        {
            break;
        }

        link = link->next;
    }

    return link;
}

bool bt_a2dp_pm_cback(uint8_t       bd_addr[6],
                      T_BT_PM_EVENT event)
{
    T_BT_A2DP_LINK *link;
    bool            ret = true;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        switch (event)
        {
        case BT_PM_EVENT_LINK_CONNECTED:
            break;

        case BT_PM_EVENT_LINK_DISCONNECTED:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_SUCCESS:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_FAIL:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_REQ:
            if (link->state == BT_A2DP_STATE_STREAMING)
            {
                /* sniff mode disallowed */
                ret = false;
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_SUCCESS:
            break;

        case BT_PM_EVENT_SNIFF_EXIT_FAIL:
            break;

        case BT_PM_EVENT_SNIFF_EXIT_REQ:
            break;
        }
    }

    return ret;
}

bool bt_a2dp_last_seq_num_get(uint8_t   bd_addr[6],
                              uint16_t *last_seq_num)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        *last_seq_num = link->last_seq_num;
        return true;
    }

    return false;
}

void bt_a2dp_cback(uint16_t     cid,
                   T_AVDTP_MSG  msg_type,
                   void        *msg_buf)
{
    T_BT_A2DP_LINK   *link;
    T_BT_MSG_PAYLOAD  payload;

    if (msg_type != AVDTP_MSG_STREAM_DATA_IND)
    {
        PROFILE_PRINT_INFO2("bt_a2dp_cback: cid 0x%04x, msg_type 0x%04x", cid, msg_type);
    }

    switch (msg_type)
    {
    case AVDTP_MSG_STREAM_DATA_IND:
        link = bt_a2dp_find_link_by_cid(cid);
        if (link != NULL)
        {
            T_BT_A2DP_STREAM_DATA_IND  data;
            uint8_t                   *pkt_ptr;
            uint16_t                   pkt_len;
            uint32_t                   bt_clock;
            uint8_t                    csrc_len;
            uint8_t                    cp_len = 0;

            pkt_ptr = ((T_MPA_L2C_DATA_IND *)msg_buf)->data + ((T_MPA_L2C_DATA_IND *)msg_buf)->gap;
            pkt_len = ((T_MPA_L2C_DATA_IND *)msg_buf)->length;
            bt_clock = ((T_MPA_L2C_DATA_IND *)msg_buf)->bt_clock;

            csrc_len = (*pkt_ptr & 0x0f) << 2;
            data.bt_clock = bt_clock;
            data.seq_num = (*(pkt_ptr + 2) << 8) + *(pkt_ptr + 3);
            data.timestamp = (*(pkt_ptr + 4) << 24) + (*(pkt_ptr + 5) << 16) +
                             (*(pkt_ptr + 6) << 8) + *(pkt_ptr + 7);

            if (link->content_protect)
            {
                cp_len = 1;
            }

            if (link->codec_type == BT_A2DP_CODEC_TYPE_SBC ||
                link->codec_type == BT_A2DP_CODEC_TYPE_LDAC ||
                link->codec_type == BT_A2DP_CODEC_TYPE_LC3)
            {
                /* remove header */
                data.frame_num = *(pkt_ptr + 12 + cp_len + csrc_len) & 0x0f;
                data.payload = pkt_ptr + 13 + cp_len + csrc_len;
                data.len = pkt_len - 13 - csrc_len - cp_len;
            }
            else if (link->codec_type == BT_A2DP_CODEC_TYPE_AAC ||
                     link->codec_type == BT_A2DP_CODEC_TYPE_USAC)
            {
                data.frame_num = 1;
                data.payload = pkt_ptr + 12 + csrc_len + cp_len;
                data.len = pkt_len - 12 - csrc_len - cp_len;
            }
            else if (link->codec_type == BT_A2DP_CODEC_TYPE_LHDC)
            {
                data.frame_num = (*(pkt_ptr + 12 + cp_len + csrc_len) & 0x3C) >> 2;
                data.payload = pkt_ptr + 12 + csrc_len + cp_len + 2;
                data.len = pkt_len - 12 - csrc_len - cp_len - 2;
            }

            link->last_seq_num = data.seq_num;

            payload.msg_buf = &data;
            memcpy(payload.bd_addr, link->bd_addr, 6);
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_DATA_IND, &payload);
        }
        break;

    case AVDTP_MSG_STREAM_DATA_RSP:
        link = bt_a2dp_find_link_by_cid(cid);
        if (link != NULL)
        {
            memcpy(payload.bd_addr, link->bd_addr, 6);
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_DATA_RSP, &payload);
        }
        break;

    case AVDTP_MSG_SIG_CONN_REQ:
        link = bt_a2dp_find_link_by_addr((uint8_t *)msg_buf);
        if (link == NULL)
        {
            link = bt_a2dp_alloc_link((uint8_t *)msg_buf);
            if (link != NULL)
            {
                link->state = BT_A2DP_STATE_CONNECTING;
                link->sig_cid = cid;
                link->int_flag = 0;
                memcpy(payload.bd_addr, link->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_A2DP_CONN_IND, &payload);
            }
            else
            {
                avdtp_signal_connect_cfm((uint8_t *)msg_buf, 0, false);
            }
        }
        else
        {
            avdtp_signal_connect_cfm((uint8_t *)msg_buf, 0, false);
        }
        break;

    case AVDTP_MSG_SIG_CONN_RSP:
        link = bt_a2dp_find_link_by_addr((uint8_t *)msg_buf);
        if (link != NULL)
        {
            link->sig_cid = cid;
        }
        break;

    case AVDTP_MSG_SIG_CONNECTED:
        {
            T_MPA_L2C_CONN_CMPL_INFO *info;

            info = (T_MPA_L2C_CONN_CMPL_INFO *)msg_buf;

            link = bt_a2dp_find_link_by_addr(info->bd_addr);
            if (link != NULL)
            {
                bt_pm_cback_register(link->bd_addr, bt_a2dp_pm_cback);
                memcpy(payload.bd_addr, link->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_A2DP_CONN_CMPL, &payload);

                if (link->int_flag & INITIATE_CONN_MASK)
                {
                    avdtp_signal_discover_req(link->bd_addr);
                }
            }
            else
            {
                avdtp_signal_disconnect_req(info->bd_addr);
            }
        }
        break;

    case AVDTP_MSG_SIG_CONN_FAIL:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = msg_buf;
                bt_a2dp_free_link(link);
                bt_mgr_dispatch(BT_MSG_A2DP_CONN_FAIL, &payload);
            }
        }
        break;

    case AVDTP_MSG_SIG_DISCONNECTED:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                bt_pm_cback_unregister(link->bd_addr, bt_a2dp_pm_cback);
                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = msg_buf;
                bt_a2dp_free_link(link);
                bt_mgr_dispatch(BT_MSG_A2DP_DISCONN_CMPL, &payload);
            }
        }
        break;

    case AVDTP_MSG_STREAM_CONN_REQ:
    case AVDTP_MSG_STREAM_CONN_RSP:
        link = bt_a2dp_find_link_by_addr((uint8_t *)msg_buf);
        if (link != NULL)
        {
            link->stream_cid = cid;
        }
        break;

    case AVDTP_MSG_STREAM_CONNECTED:
        {
            T_MPA_L2C_CONN_CMPL_INFO *info;

            info = (T_MPA_L2C_CONN_CMPL_INFO *)msg_buf;
            link = bt_a2dp_find_link_by_addr(info->bd_addr);
            if (link != NULL)
            {
                uint16_t max_pkt_len;

                max_pkt_len = info->remote_mtu;

                if (link->codec_type == BT_A2DP_CODEC_TYPE_LHDC)
                {
                    /* avdtp header(12 bytes) + lhdc frame_num and seq_num header(2 byte) */
                    max_pkt_len -= 14;
                }
                else if (link->codec_type == BT_A2DP_CODEC_TYPE_SBC ||
                         link->codec_type == BT_A2DP_CODEC_TYPE_LDAC ||
                         link->codec_type == BT_A2DP_CODEC_TYPE_LC3)
                {
                    /* avdtp header(12 bytes) + frame num(1 byte) */
                    max_pkt_len -= 13;
                }
                else if (link->codec_type == BT_A2DP_CODEC_TYPE_AAC ||
                         link->codec_type == BT_A2DP_CODEC_TYPE_USAC)
                {
                    /* avdtp header(12 bytes) */
                    max_pkt_len -= 12;
                }

                if (link->content_protect)
                {
                    /* cp flag(1 byte)*/
                    max_pkt_len -= 1;
                }

                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = &max_pkt_len;
                bt_mgr_dispatch(BT_MSG_A2DP_STREAM_OPEN, &payload);
            }
            else
            {
                avdtp_stream_disconnect_req(info->bd_addr);
            }
        }
        break;

    case AVDTP_MSG_STREAM_CONN_FAIL:
        link = bt_a2dp_find_link_by_cid(cid);
        if (link != NULL)
        {
            link->stream_cid = 0;
            memcpy(payload.bd_addr, link->bd_addr, 6);
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_OPEN_FAIL, &payload);
        }
        break;

    case AVDTP_MSG_STREAM_DISCONNECTED:
        link = bt_a2dp_find_link_by_cid(cid);
        if (link != NULL)
        {
            link->stream_cid = 0;
            link->last_seq_num = 0;
            memcpy(payload.bd_addr, link->bd_addr, 6);
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_CLOSE, &payload);

            if (link->int_flag & INITIATE_DISCONN_MASK)
            {
                link->int_flag &= ~INITIATE_DISCONN_MASK;
                avdtp_signal_disconnect_req(link->bd_addr);
            }
        }
        break;

    case AVDTP_MSG_SIG_START_IND:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                T_BT_A2DP_CFG_INFO cfg_info;

                cfg_info.role = link->role;
                cfg_info.codec_type = link->codec_type;
                memcpy(&cfg_info.codec_info, &link->codec_info, sizeof(T_BT_A2DP_CODEC_INFO));
                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = &cfg_info;
                bt_mgr_dispatch(BT_MSG_A2DP_STREAM_START_IND, &payload);
            }
        }
        break;

    case AVDTP_MSG_SIG_START_RSP:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                link->state = BT_A2DP_STATE_STREAMING;
                memcpy(payload.bd_addr, link->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_A2DP_STREAM_START_RSP, &payload);
            }
        }
        break;

    case AVDTP_MSG_SIG_OPEN_IND:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                link->state = BT_A2DP_STATE_CONNECTED;
            }
        }
        break;

    case AVDTP_MSG_SIG_OPEN_RSP:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                link->state = BT_A2DP_STATE_CONNECTED;
                avdtp_stream_connect_req(link->bd_addr);
            }
        }
        break;

    case AVDTP_MSG_SIG_SET_CFG_IND:
    case AVDTP_MSG_SIG_SET_CFG_RSP:
    case AVDTP_MSG_SIG_RECFG_IND:
    case AVDTP_MSG_SIG_RECFG_RSP:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                uint8_t            *data;
                uint8_t             codec_info[MAX_CODEC_INFO_SIZE];
                uint8_t             codec_type;
                T_BT_A2DP_CFG_INFO  cfg_info;

                data = (uint8_t *)msg_buf;

                link->role = data[0];
                codec_type = data[1];
                link->content_protect = data[2];
                memcpy(codec_info, &data[4], MAX_CODEC_INFO_SIZE);

                if (codec_type == AVDTP_CODEC_TYPE_SBC)
                {
                    uint8_t temp;

                    link->codec_type = BT_A2DP_CODEC_TYPE_SBC;
                    temp = codec_info[0] & 0xf0;
                    if (temp & 0x10)
                    {
                        link->codec_info.sbc.sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ;
                    }
                    else if (temp & 0x20)
                    {
                        link->codec_info.sbc.sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_44_1KHZ;
                    }
                    else if (temp & 0x40)
                    {
                        link->codec_info.sbc.sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_32KHZ;
                    }
                    else if (temp & 0x80)
                    {
                        link->codec_info.sbc.sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_16KHZ;
                    }

                    temp = codec_info[0] & 0x0f;
                    if (temp & 0x01)
                    {
                        link->codec_info.sbc.channel_mode = BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO;
                    }
                    else if (temp & 0x02)
                    {
                        link->codec_info.sbc.channel_mode = BT_A2DP_SBC_CHANNEL_MODE_STEREO;
                    }
                    else if (temp & 0x04)
                    {
                        link->codec_info.sbc.channel_mode = BT_A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL;
                    }
                    else if (temp & 0x08)
                    {
                        link->codec_info.sbc.channel_mode = BT_A2DP_SBC_CHANNEL_MODE_MONO;
                    }

                    temp = codec_info[1] & 0xf0;
                    if (temp & 0x10)
                    {
                        link->codec_info.sbc.block_length = BT_A2DP_SBC_BLOCK_LENGTH_16;
                    }
                    else if (temp & 0x20)
                    {
                        link->codec_info.sbc.block_length = BT_A2DP_SBC_BLOCK_LENGTH_12;
                    }
                    else if (temp & 0x40)
                    {
                        link->codec_info.sbc.block_length = BT_A2DP_SBC_BLOCK_LENGTH_8;
                    }
                    else if (temp & 0x80)
                    {
                        link->codec_info.sbc.block_length = BT_A2DP_SBC_BLOCK_LENGTH_4;
                    }

                    temp = codec_info[1] & 0x0c;
                    if (temp & 0x04)
                    {
                        link->codec_info.sbc.subbands = BT_A2DP_SBC_SUBBANDS_8;
                    }
                    else if (temp & 0x08)
                    {
                        link->codec_info.sbc.subbands = BT_A2DP_SBC_SUBBANDS_4;
                    }

                    temp = codec_info[1] & 0x03;
                    if (temp & 0x01)
                    {
                        link->codec_info.sbc.allocation_method = BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS;
                    }
                    else if (temp & 0x02)
                    {
                        link->codec_info.sbc.allocation_method = BT_A2DP_SBC_ALLOCATION_METHOD_SNR;
                    }

                    link->codec_info.sbc.min_bitpool = codec_info[2];
                    link->codec_info.sbc.max_bitpool = codec_info[3];
                }
                else if (codec_type == AVDTP_CODEC_TYPE_AAC)
                {
                    uint8_t  i;
                    uint8_t  temp;
                    uint16_t sam_freq;
                    uint16_t sam_freq_bit;

                    link->codec_type = BT_A2DP_CODEC_TYPE_AAC;
                    temp = codec_info[0];
                    if (temp & 0x01)
                    {
                        link->codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_D_DRC;
                    }
                    else if (temp & 0x02)
                    {
                        link->codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_AAC_ELDV2;
                    }
                    else if (temp & 0x04)
                    {
                        link->codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_HE_AACV2;
                    }
                    else if (temp & 0x08)
                    {
                        link->codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_HE_AAC;
                    }
                    else if (temp & 0x10)
                    {
                        link->codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_AAC_SCALABLE;
                    }
                    else if (temp & 0x20)
                    {
                        link->codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_AAC_LTP;
                    }
                    else if (temp & 0x40)
                    {
                        link->codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_AAC_LC;
                    }
                    else if (temp & 0x80)
                    {
                        link->codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_2_AAC_LC;
                    }

                    sam_freq = (codec_info[1] << 8) + (codec_info[2] & 0x80);
                    for (i = 0; i < BT_A2DP_AAC_SR_NUM; i++)
                    {
                        sam_freq_bit = (1 << (4 + i));
                        if (sam_freq & sam_freq_bit)
                        {
                            link->codec_info.aac.sampling_frequency = sam_freq_bit;
                            break;
                        }
                    }

                    temp = codec_info[2] & 0x0c;
                    if (temp & 0x04)
                    {
                        link->codec_info.aac.channel_number = BT_A2DP_AAC_CHANNEL_NUMBER_2;
                    }
                    else if (temp & 0x08)
                    {
                        link->codec_info.aac.channel_number = BT_A2DP_AAC_CHANNEL_NUMBER_1;
                    }

                    temp = codec_info[3] & 0x80;
                    if (temp)
                    {
                        link->codec_info.aac.vbr_supported = true;
                    }
                    else
                    {
                        link->codec_info.aac.vbr_supported = false;
                    }

                    link->codec_info.aac.bit_rate = codec_info[5] + (codec_info[4] << 8) +
                                                    ((codec_info[3] & 0x7f) << 16);
                }
                else if (codec_type == AVDTP_CODEC_TYPE_USAC)
                {
                    uint8_t  i;
                    uint8_t  temp;
                    uint32_t sam_freq;
                    uint32_t sam_freq_bit;

                    link->codec_type = BT_A2DP_CODEC_TYPE_USAC;
                    temp = codec_info[0] & 0xc0;
                    if (temp & 0x80)
                    {
                        link->codec_info.usac.object_type = BT_A2DP_USAC_OBJECT_TYPE_MPEG_D_USAC_WITH_DRC;
                    }

                    sam_freq = ((codec_info[0] & 0x3f) << 24) + (codec_info[1] << 16) + (codec_info[2] << 8) +
                               (codec_info[3] & 0xf0);
                    for (i = 0; i < BT_A2DP_USAC_SR_NUM; i++)
                    {
                        sam_freq_bit = (1 << (4 + i));
                        if (sam_freq & sam_freq_bit)
                        {
                            link->codec_info.usac.sampling_frequency = sam_freq_bit;
                            break;
                        }
                    }

                    temp = codec_info[3] & 0x0f;
                    if (temp & 0x04)
                    {
                        link->codec_info.usac.channel_number = BT_A2DP_USAC_CHANNEL_NUMBER_2;
                    }
                    else if (temp & 0x08)
                    {
                        link->codec_info.usac.channel_number = BT_A2DP_USAC_CHANNEL_NUMBER_1;
                    }

                    temp = codec_info[4] & 0x80;
                    if (temp)
                    {
                        link->codec_info.usac.vbr_supported = true;
                    }
                    else
                    {
                        link->codec_info.usac.vbr_supported = false;
                    }

                    link->codec_info.usac.bit_rate = codec_info[6] + (codec_info[5] << 8) +
                                                     ((codec_info[4] & 0x7f) << 16);
                }
                else if (codec_type == AVDTP_CODEC_TYPE_VENDOR)
                {
                    if (!memcmp(codec_info, bt_a2dp_ldac_codec_info, 6))
                    {
                        link->codec_type = BT_A2DP_CODEC_TYPE_LDAC;
                        link->codec_info.ldac.sampling_frequency = codec_info[6];
                        link->codec_info.ldac.channel_mode = codec_info[7];
                    }
                    else if (!memcmp(codec_info, bt_a2dp_lc3_codec_info, 6))
                    {
                        uint8_t temp;

                        link->codec_type = BT_A2DP_CODEC_TYPE_LC3;
                        temp = codec_info[6] & 0xfc;
                        if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_48KHZ)
                        {
                            link->codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_48KHZ;
                        }
                        else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_44_1KHZ)
                        {
                            link->codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_44_1KHZ;
                        }
                        else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_32KHZ)
                        {
                            link->codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_32KHZ;
                        }
                        else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_24KHZ)
                        {
                            link->codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_24KHZ;
                        }
                        else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_16KHZ)
                        {
                            link->codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_16KHZ;
                        }
                        else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_8KHZ)
                        {
                            link->codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_8KHZ;
                        }

                        temp = codec_info[6] & 0x03;
                        if (temp & BT_A2DP_LC3_CHANNEL_NUM_2)
                        {
                            link->codec_info.lc3.channel_number = BT_A2DP_LC3_CHANNEL_NUM_2;
                        }
                        else if (temp & BT_A2DP_LC3_CHANNEL_NUM_1)
                        {
                            link->codec_info.lc3.channel_number = BT_A2DP_LC3_CHANNEL_NUM_1;
                        }

                        temp = codec_info[7] & 0x06;
                        if (temp & BT_A2DP_LC3_FRAME_DURATION_10MS)
                        {
                            link->codec_info.lc3.frame_duration = BT_A2DP_LC3_FRAME_DURATION_10MS;
                        }
                        else if (temp & BT_A2DP_LC3_FRAME_DURATION_7_5MS)
                        {
                            link->codec_info.lc3.frame_duration = BT_A2DP_LC3_FRAME_DURATION_7_5MS;
                        }

                        link->codec_info.lc3.frame_length = codec_info[8] + ((codec_info[7] & 0x01) << 8);
                    }
                    else if (!memcmp(codec_info, bt_a2dp_lhdc_v5_codec_info, 6))
                    {
                        uint8_t temp;

                        link->codec_type = BT_A2DP_CODEC_TYPE_LHDC;
                        temp = codec_info[6] & 0x35;
                        if (temp & BT_A2DP_LHDC_SAMPLING_FREQUENCY_192KHZ)
                        {
                            link->codec_info.lhdc.sampling_frequency = BT_A2DP_LHDC_SAMPLING_FREQUENCY_192KHZ;
                        }
                        else if (temp & BT_A2DP_LHDC_SAMPLING_FREQUENCY_96KHZ)
                        {
                            link->codec_info.lhdc.sampling_frequency = BT_A2DP_LHDC_SAMPLING_FREQUENCY_96KHZ;
                        }
                        else if (temp & BT_A2DP_LHDC_SAMPLING_FREQUENCY_48KHZ)
                        {
                            link->codec_info.lhdc.sampling_frequency = BT_A2DP_LHDC_SAMPLING_FREQUENCY_48KHZ;
                        }
                        else if (temp & BT_A2DP_LHDC_SAMPLING_FREQUENCY_44_1KHZ)
                        {
                            link->codec_info.lhdc.sampling_frequency = BT_A2DP_LHDC_SAMPLING_FREQUENCY_44_1KHZ;
                        }

                        temp = codec_info[7] & 0xc0;
                        link->codec_info.lhdc.min_bitrate = (T_BT_A2DP_LHDC_MIN_BITRATE)(temp >> 6);

                        temp = codec_info[7] & 0x30;
                        link->codec_info.lhdc.max_bitrate = (T_BT_A2DP_LHDC_MAX_BITRATE)(temp >> 4);

                        temp = codec_info[7] & 0x07;
                        if (temp & BT_A2DP_LHDC_BIT_DEPTH_32BIT)
                        {
                            link->codec_info.lhdc.bit_depth = BT_A2DP_LHDC_BIT_DEPTH_32BIT;
                        }
                        else if (temp & BT_A2DP_LHDC_BIT_DEPTH_24BIT)
                        {
                            link->codec_info.lhdc.bit_depth = BT_A2DP_LHDC_BIT_DEPTH_24BIT;
                        }
                        else if (temp & BT_A2DP_LHDC_BIT_DEPTH_16BIT)
                        {
                            link->codec_info.lhdc.bit_depth = BT_A2DP_LHDC_BIT_DEPTH_16BIT;
                        }

                        temp = codec_info[8] & 0x0f;
                        if (temp & BT_A2DP_LHDC_VERSION_NUMBER_5_3)
                        {
                            link->codec_info.lhdc.version_number = BT_A2DP_LHDC_VERSION_NUMBER_5_3;
                        }
                        else if (temp & BT_A2DP_LHDC_VERSION_NUMBER_5_2)
                        {
                            link->codec_info.lhdc.version_number = BT_A2DP_LHDC_VERSION_NUMBER_5_2;
                        }
                        else if (temp & BT_A2DP_LHDC_VERSION_NUMBER_5_1)
                        {
                            link->codec_info.lhdc.version_number = BT_A2DP_LHDC_VERSION_NUMBER_5_1;
                        }
                        else if (temp & BT_A2DP_LHDC_VERSION_NUMBER_5_0)
                        {
                            link->codec_info.lhdc.version_number = BT_A2DP_LHDC_VERSION_NUMBER_5_0;
                        }

                        if (codec_info[9] & 0x40)
                        {
                            link->codec_info.lhdc.low_latency = true;
                        }
                        else
                        {
                            link->codec_info.lhdc.low_latency = false;
                        }

                        if (codec_info[9] & 0x04)
                        {
                            link->codec_info.lhdc.meta = true;
                        }
                        else
                        {
                            link->codec_info.lhdc.meta = false;
                        }

                        if (codec_info[9] & 0x02)
                        {
                            link->codec_info.lhdc.jas = true;
                        }
                        else
                        {
                            link->codec_info.lhdc.jas = false;
                        }

                        if (codec_info[9] & 0x01)
                        {
                            link->codec_info.lhdc.ar = true;
                        }
                        else
                        {
                            link->codec_info.lhdc.ar = false;
                        }
                    }
                    else
                    {
                        memcpy(link->codec_info.vendor.info, codec_info, 12);
                    }
                }

                cfg_info.role = link->role;
                cfg_info.codec_type = link->codec_type;
                memcpy(&cfg_info.codec_info, &link->codec_info, sizeof(T_BT_A2DP_CODEC_INFO));
                payload.msg_buf = &cfg_info;
                memcpy(payload.bd_addr, link->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_A2DP_CONFIG_CMPL, &payload);
            }
        }
        break;

    case AVDTP_MSG_SIG_SUSPEND_IND:
    case AVDTP_MSG_SIG_SUSPEND_RSP:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                link->state = BT_A2DP_STATE_CONNECTED;
                link->last_seq_num = 0;
                memcpy(payload.bd_addr, link->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_A2DP_STREAM_STOP, &payload);
            }
        }
        break;

    case AVDTP_MSG_SIG_DELAY_REPORT_IND:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                uint16_t delay;

                delay = *(uint16_t *)msg_buf;
                payload.msg_buf = &delay;
                memcpy(payload.bd_addr, link->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_A2DP_DELAY_REPORT, &payload);
            }
        }
        break;

    case AVDTP_MSG_SIG_ABORT_IND:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                link->state = BT_A2DP_STATE_DISCONNECTING;
            }
        }
        break;

    case AVDTP_MSG_SIG_ABORT_RSP:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                link->state = BT_A2DP_STATE_DISCONNECTING;

                if (link->stream_cid)
                {
                    avdtp_stream_disconnect_req(link->bd_addr);
                }
            }
        }
        break;

    case AVDTP_MSG_SIG_CLOSE_IND:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                link->state = BT_A2DP_STATE_DISCONNECTING;
            }
        }
        break;

    case AVDTP_MSG_SIG_CLOSE_RSP:
        {
            link = bt_a2dp_find_link_by_cid(cid);
            if (link != NULL)
            {
                link->state = BT_A2DP_STATE_DISCONNECTING;

                if (link->stream_cid)
                {
                    avdtp_stream_disconnect_req(link->bd_addr);
                }
            }
        }
        break;

    default:
        break;
    }
}

bool bt_a2dp_stream_data_send(uint8_t   bd_addr[6],
                              uint16_t  seq_num,
                              uint32_t  time_stamp,
                              uint8_t   frame_num,
                              uint8_t  *frame_buf,
                              uint16_t  len)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        return avdtp_stream_data_send(bd_addr, seq_num, time_stamp,
                                      frame_num, frame_buf, len);
    }

    return false;
}

bool bt_a2dp_stream_open_req(uint8_t        bd_addr[6],
                             T_BT_A2DP_ROLE role)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        return avdtp_signal_open_req(bd_addr, role);
    }

    return false;
}

bool bt_a2dp_stream_start_req(uint8_t bd_addr[6])
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        return avdtp_signal_start_req(bd_addr);
    }

    return false;
}

bool bt_a2dp_stream_close_req(uint8_t bd_addr[6])
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        return avdtp_signal_close_req(bd_addr);
    }

    return false;
}

bool bt_a2dp_stream_suspend_req(uint8_t bd_addr[6])
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        return avdtp_signal_suspend_req(bd_addr);
    }

    return false;
}

bool bt_a2dp_stream_abort_req(uint8_t bd_addr[6])
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        return avdtp_signal_abort_req(bd_addr);
    }

    return false;
}

bool bt_a2dp_stream_delay_report_req(uint8_t  bd_addr[6],
                                     uint16_t delay)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        return avdtp_signal_delay_report_req(bd_addr, delay);
    }

    return false;
}

bool bt_a2dp_connect_req(uint8_t        bd_addr[6],
                         uint16_t       avdtp_ver,
                         T_BT_A2DP_ROLE role,
                         uint16_t       mtu_size)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_a2dp_alloc_link(bd_addr);
        if (link != NULL)
        {
            link->state = BT_A2DP_STATE_CONNECTING;
            link->int_flag = INITIATE_CONN_MASK;
            bt_sniff_mode_exit(link->bd_addr, false);
            avdtp_signal_connect_req(bd_addr, avdtp_ver, role, mtu_size);
            return true;
        }
    }

    return false;
}

bool bt_a2dp_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);

        if (link->stream_cid)
        {
            link->int_flag |= INITIATE_DISCONN_MASK;
            return avdtp_signal_close_req(bd_addr);
        }
        else if (link->sig_cid)
        {
            return avdtp_signal_disconnect_req(bd_addr);
        }
    }

    return false;
}

bool bt_a2dp_connect_cfm(uint8_t  bd_addr[6],
                         uint16_t mtu_size,
                         bool     accept)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (accept == false)
        {
            bt_a2dp_free_link(link);
        }

        bt_sniff_mode_exit(bd_addr, false);
        return avdtp_signal_connect_cfm(bd_addr, mtu_size, accept);
    }

    return avdtp_signal_connect_cfm(bd_addr, mtu_size, false);
}

bool bt_a2dp_stream_start_cfm(uint8_t bd_addr[6],
                              bool    accept)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (accept == true)
        {
            link->state = BT_A2DP_STATE_STREAMING;
        }

        bt_sniff_mode_exit(bd_addr, false);
        return avdtp_signal_start_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_a2dp_active_link_set(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (memcmp(bd_addr, btm_db.active_a2dp_addr, 6))
        {
            memcpy(btm_db.active_a2dp_addr, bd_addr, 6);
            BTM_PRINT_TRACE1("bt_a2dp_active_link_set: active_a2dp_addr %s",
                             TRACE_BDADDR(btm_db.active_a2dp_addr));
        }

        return true;
    }

    return false;
}

bool bt_a2dp_stream_endpoint_add(T_BT_A2DP_STREAM_ENDPOINT sep)
{
    uint8_t media_codec_info[12];

    if (sep.codec_type == BT_A2DP_CODEC_TYPE_SBC)
    {
        media_codec_info[0] = sep.u.codec_sbc.sampling_frequency_mask |
                              sep.u.codec_sbc.channel_mode_mask;

        media_codec_info[1] = sep.u.codec_sbc.block_length_mask |
                              sep.u.codec_sbc.subbands_mask |
                              sep.u.codec_sbc.allocation_method_mask;

        media_codec_info[2] = sep.u.codec_sbc.min_bitpool;
        media_codec_info[3] = sep.u.codec_sbc.max_bitpool;

        return avdtp_codec_add(sep.role, AVDTP_CODEC_TYPE_SBC, media_codec_info);
    }
    else if (sep.codec_type == BT_A2DP_CODEC_TYPE_AAC)
    {
        media_codec_info[0] = sep.u.codec_aac.object_type_mask;

        media_codec_info[1] = (uint8_t)(sep.u.codec_aac.sampling_frequency_mask >> 8);

        media_codec_info[2] = (uint8_t)sep.u.codec_aac.sampling_frequency_mask |
                              sep.u.codec_aac.channel_number_mask;

        if (sep.u.codec_aac.vbr_supported)
        {
            media_codec_info[3] = 0x80;
        }
        else
        {
            media_codec_info[3] = 0;
        }

        media_codec_info[3] |= (uint8_t)(sep.u.codec_aac.bit_rate >> 16);
        media_codec_info[4] = (uint8_t)(sep.u.codec_aac.bit_rate >> 8);
        media_codec_info[5] = (uint8_t)sep.u.codec_aac.bit_rate;

        return avdtp_codec_add(sep.role, AVDTP_CODEC_TYPE_AAC, media_codec_info);
    }
    else if (sep.codec_type == BT_A2DP_CODEC_TYPE_USAC)
    {
        media_codec_info[0] = sep.u.codec_usac.object_type_mask |
                              (uint8_t)(sep.u.codec_usac.sampling_frequency_mask >> 24);
        media_codec_info[1] = (uint8_t)(sep.u.codec_usac.sampling_frequency_mask >> 16);
        media_codec_info[2] = (uint8_t)(sep.u.codec_usac.sampling_frequency_mask >> 8);
        media_codec_info[3] = (uint8_t)sep.u.codec_usac.sampling_frequency_mask |
                              sep.u.codec_usac.channel_number_mask;

        if (sep.u.codec_usac.vbr_supported)
        {
            media_codec_info[4] = 0x80;
        }
        else
        {
            media_codec_info[4] = 0;
        }

        media_codec_info[4] |= (uint8_t)(sep.u.codec_usac.bit_rate >> 16);
        media_codec_info[5] = (uint8_t)(sep.u.codec_usac.bit_rate >> 8);
        media_codec_info[6] = (uint8_t)sep.u.codec_usac.bit_rate;

        return avdtp_codec_add(sep.role, AVDTP_CODEC_TYPE_USAC, media_codec_info);
    }
    else if (sep.codec_type == BT_A2DP_CODEC_TYPE_LDAC)
    {
        memcpy(media_codec_info, bt_a2dp_ldac_codec_info, 6);
        media_codec_info[6] = sep.u.codec_ldac.sampling_frequency_mask;
        media_codec_info[7] = sep.u.codec_ldac.channel_mode_mask;

        return avdtp_codec_add(sep.role, AVDTP_CODEC_TYPE_LDAC, media_codec_info);
    }
    else if (sep.codec_type == BT_A2DP_CODEC_TYPE_LC3)
    {
        memcpy(media_codec_info, bt_a2dp_lc3_codec_info, 6);
        media_codec_info[6] = sep.u.codec_lc3.sampling_frequency_mask |
                              sep.u.codec_lc3.channel_num_mask;
        media_codec_info[7] = (uint8_t)(sep.u.codec_lc3.frame_length >> 8) |
                              sep.u.codec_lc3.frame_duration_mask;
        media_codec_info[8] = (uint8_t)sep.u.codec_lc3.frame_length;

        return avdtp_codec_add(sep.role, AVDTP_CODEC_TYPE_LC3, media_codec_info);
    }
    else if (sep.codec_type == BT_A2DP_CODEC_TYPE_LHDC)
    {
        uint8_t media_codec_info[11];

        memcpy(media_codec_info, bt_a2dp_lhdc_v5_codec_info, 6);
        media_codec_info[6] = sep.u.codec_lhdc.sampling_frequency_mask;
        media_codec_info[7] = (sep.u.codec_lhdc.min_bitrate << 6) |
                              (sep.u.codec_lhdc.max_bitrate << 4) |
                              sep.u.codec_lhdc.bit_depth_mask;
        media_codec_info[8] = 0x10 | sep.u.codec_lhdc.version_number;
        media_codec_info[9] = (sep.u.codec_lhdc.low_latency << 6) |
                              (sep.u.codec_lhdc.meta << 2) |
                              (sep.u.codec_lhdc.jas << 1) |
                              sep.u.codec_lhdc.ar;
        media_codec_info[10] = 0x00;

        return avdtp_codec_add(sep.role, AVDTP_CODEC_TYPE_LHDC, media_codec_info);
    }

    return false;
}

bool bt_a2dp_stream_endpoint_delete(T_BT_A2DP_STREAM_ENDPOINT sep)
{
    return avdtp_codec_delete(sep.role, (T_AVDTP_CODEC_TYPE)sep.codec_type);
}

bool bt_a2dp_init(uint8_t service_capabilities)
{
    int32_t ret = 0;

    bt_a2dp = calloc(1, sizeof(T_BT_A2DP));
    if (bt_a2dp == NULL)
    {
        ret = 1;
        goto fail_alloc_a2dp;
    }

    if (avdtp_init(service_capabilities, bt_a2dp_cback) == false)
    {
        ret = 2;
        goto fail_init_avdtp;
    }

    return true;

fail_init_avdtp:
    free(bt_a2dp);
    bt_a2dp = NULL;
fail_alloc_a2dp:
    PROFILE_PRINT_ERROR1("bt_a2dp_init: failed %d", -ret);
    return false;
}

bool bt_a2dp_roleswap_info_get(uint8_t               bd_addr[6],
                               T_ROLESWAP_A2DP_INFO *info)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_ROLESWAP_AVDTP_INFO avdtp_info;

        if (avdtp_roleswap_info_get(link->bd_addr, &avdtp_info) == true)
        {
            uint8_t *buf;

            memcpy(info->bd_addr, bd_addr, 6);
            info->sig_cid = link->sig_cid;
            info->stream_cid = link->stream_cid;
            info->state = link->state;
            info->codec_type = link->codec_type;
            info->role = link->role;
            info->content_proect = link->content_protect;
            info->last_seq_number = link->last_seq_num;
            buf = (uint8_t *)&info->codec_info;
            ARRAY_TO_STREAM(buf, &link->codec_info, sizeof(T_BT_A2DP_CODEC_INFO));

            info->int_flag = avdtp_info.int_flag;
            info->int_seid = avdtp_info.int_seid;
            info->acp_seid = avdtp_info.acp_seid;
            info->acp_seid_idx = avdtp_info.acp_seid_idx;
            info->delay_report = avdtp_info.delay_report;
            info->data_offset = avdtp_info.data_offset;
            info->sig_state = avdtp_info.sig_state;
            info->tx_trans_label = avdtp_info.tx_trans_label;
            info->rx_start_trans_label = avdtp_info.rx_start_trans_label;
            info->cmd_flag = avdtp_info.cmd_flag;

            return true;
        }
    }

    return false;
}

bool bt_a2dp_roleswap_info_set(uint8_t               bd_addr[6],
                               T_ROLESWAP_A2DP_INFO *info)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_a2dp_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        T_ROLESWAP_AVDTP_INFO  avdtp_info;
        uint8_t               *buf;

        link->int_flag = 0;
        link->state = (T_BT_A2DP_STATE)info->state;
        link->sig_cid = info->sig_cid;
        link->stream_cid = info->stream_cid;
        link->codec_type = info->codec_type;
        link->role = info->role;
        link->content_protect = info->content_proect;
        link->last_seq_num = info->last_seq_number;
        buf = (uint8_t *)&link->codec_info;
        ARRAY_TO_STREAM(buf, &info->codec_info, sizeof(T_BT_A2DP_CODEC_INFO));

        avdtp_info.sig_cid = info->sig_cid;
        avdtp_info.stream_cid = info->stream_cid;
        avdtp_info.int_flag = info->int_flag;
        avdtp_info.int_seid = info->int_seid;
        avdtp_info.acp_seid = info->acp_seid;
        avdtp_info.acp_seid_idx = info->acp_seid_idx;
        avdtp_info.delay_report = info->delay_report;
        avdtp_info.data_offset = info->data_offset;
        avdtp_info.sig_state = info->sig_state;
        avdtp_info.tx_trans_label = info->tx_trans_label;
        avdtp_info.rx_start_trans_label = info->rx_start_trans_label;
        avdtp_info.cmd_flag = info->cmd_flag;

        bt_pm_cback_register(bd_addr, bt_a2dp_pm_cback);

        return avdtp_roleswap_info_set(bd_addr, &avdtp_info);
    }

    return false;
}

bool bt_a2dp_roleswap_info_del(uint8_t bd_addr[6])
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_pm_cback_unregister(bd_addr, bt_a2dp_pm_cback);
        bt_a2dp_free_link(link);
        return avdtp_roleswap_info_del(bd_addr);
    }

    return false;
}

bool bt_a2dp_stream_cid_get(uint8_t bd_addr[6], uint16_t *cid)
{
    T_BT_A2DP_LINK *link;

    link = bt_a2dp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        *cid = link->stream_cid;
        return true;
    }

    return false;
}
#else

#include <string.h>
#include <stdlib.h>
#include "bt_a2dp.h"
#include "bt_a2dp_int.h"

bool bt_a2dp_stream_endpoint_add(T_BT_A2DP_STREAM_ENDPOINT sep)
{
    return false;
}

bool bt_a2dp_stream_endpoint_delete(T_BT_A2DP_STREAM_ENDPOINT sep)
{
    return false;
}

bool bt_a2dp_init(uint8_t service_capabilities)
{
    return false;
}

bool bt_a2dp_connect_req(uint8_t        bd_addr[6],
                         uint16_t       avdtp_ver,
                         T_BT_A2DP_ROLE role,
                         uint16_t       mtu_size)
{
    return false;
}

bool bt_a2dp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_a2dp_connect_cfm(uint8_t  bd_addr[6],
                         uint16_t mtu_size,
                         bool     accept)
{
    return false;
}

bool bt_a2dp_stream_start_cfm(uint8_t bd_addr[6],
                              bool    accept)
{
    return false;
}

bool bt_a2dp_stream_open_req(uint8_t        bd_addr[6],
                             T_BT_A2DP_ROLE role)
{
    return false;
}

bool bt_a2dp_stream_start_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_a2dp_stream_data_send(uint8_t   bd_addr[6],
                              uint16_t  seq_num,
                              uint32_t  time_stamp,
                              uint8_t   frame_num,
                              uint8_t  *frame_buf,
                              uint16_t  len)
{
    return false;
}

bool bt_a2dp_stream_close_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_a2dp_stream_suspend_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_a2dp_stream_abort_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_a2dp_active_link_set(uint8_t bd_addr[6])
{
    return false;
}

bool bt_a2dp_stream_delay_report_req(uint8_t  bd_addr[6],
                                     uint16_t delay)
{
    return false;
}

bool bt_a2dp_roleswap_info_get(uint8_t               bd_addr[6],
                               T_ROLESWAP_A2DP_INFO *info)
{
    return false;
}

bool bt_a2dp_roleswap_info_set(uint8_t               bd_addr[6],
                               T_ROLESWAP_A2DP_INFO *info)
{
    return false;
}

bool bt_a2dp_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}

bool bt_a2dp_stream_cid_get(uint8_t bd_addr[6], uint16_t *cid)
{
    return false;
}

#endif
