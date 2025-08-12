/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_A2DP_SUPPORT == 1)
#include <string.h>
#include "trace.h"

#include "sys_ipc.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_a2dp.h"
#include "bt_a2dp_int.h"
#include "a2dp.h"

#define BT_A2DP_AAC_SR_NUM    (12)
#define BT_A2DP_USAC_SR_NUM   (26)

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

void bt_a2dp_cback(uint8_t     bd_addr[6],
                   T_A2DP_MSG  msg_type,
                   void       *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = NULL;

    if (msg_type != A2DP_MSG_STREAM_DATA_IND)
    {
        BTM_PRINT_INFO2("bt_a2dp_cback: bd_addr %s, msg 0x%02x",
                        TRACE_BDADDR(bd_addr), msg_type);
    }

    switch (msg_type)
    {
    case A2DP_MSG_CONN_IND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_A2DP_CONN_IND, &payload);
        }
        else
        {
            bt_a2dp_connect_cfm(bd_addr, false);
        }
        break;

    case A2DP_MSG_CONN_CMPL:
        if (p_link != NULL)
        {
            p_link->connected_profile |= A2DP_PROFILE_MASK;
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_A2DP_CONN_CMPL, &payload);
        }
        else
        {
            a2dp_disconnect_req(bd_addr);
        }
        break;

    case A2DP_MSG_CONN_FAIL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            memset(&(p_link->a2dp_data), 0, sizeof(T_A2DP_LINK_DATA));
            bt_mgr_dispatch(BT_MSG_A2DP_CONN_FAIL, &payload);
        }
        break;

    case A2DP_MSG_DISCONN:
        if (p_link != NULL)
        {
            p_link->connected_profile &= ~A2DP_PROFILE_MASK;
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_A2DP_DISCONN_CMPL, &payload);
            memset(&(p_link->a2dp_data), 0, sizeof(T_A2DP_LINK_DATA));
        }
        break;

    case A2DP_MSG_SET_CFG:
    case A2DP_MSG_RE_CFG:
        if (p_link != NULL)
        {
            p_link->a2dp_data.codec_type = ((T_A2DP_CFG *)msg_buf)->codec_type;
            p_link->a2dp_data.role = ((T_A2DP_CFG *)msg_buf)->role;

            if (((T_A2DP_CFG *)msg_buf)->cp_flag)
            {
                p_link->a2dp_data.a2dp_content_protect = 1;
            }
            else
            {
                p_link->a2dp_data.a2dp_content_protect = 0;
            }

            if (((T_A2DP_CFG *)msg_buf)->delay_report_flag)
            {
                p_link->a2dp_data.a2dp_delay_report = 1;
            }
            else
            {
                p_link->a2dp_data.a2dp_delay_report = 0;
            }

            if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_SBC)
            {
                uint8_t temp;

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[0] & 0xf0;
                if (temp & 0x10)
                {
                    p_link->a2dp_data.codec_info.sbc.sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ;
                }
                else if (temp & 0x20)
                {
                    p_link->a2dp_data.codec_info.sbc.sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_44_1KHZ;
                }
                else if (temp & 0x40)
                {
                    p_link->a2dp_data.codec_info.sbc.sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_32KHZ;
                }
                else if (temp & 0x80)
                {
                    p_link->a2dp_data.codec_info.sbc.sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_16KHZ;
                }

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[0] & 0x0f;
                if (temp & 0x01)
                {
                    p_link->a2dp_data.codec_info.sbc.channel_mode = BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO;
                }
                else if (temp & 0x02)
                {
                    p_link->a2dp_data.codec_info.sbc.channel_mode = BT_A2DP_SBC_CHANNEL_MODE_STEREO;
                }
                else if (temp & 0x04)
                {
                    p_link->a2dp_data.codec_info.sbc.channel_mode = BT_A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL;
                }
                else if (temp & 0x08)
                {
                    p_link->a2dp_data.codec_info.sbc.channel_mode = BT_A2DP_SBC_CHANNEL_MODE_MONO;
                }

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[1] & 0xf0;
                if (temp & 0x10)
                {
                    p_link->a2dp_data.codec_info.sbc.block_length = BT_A2DP_SBC_BLOCK_LENGTH_16;
                }
                else if (temp & 0x20)
                {
                    p_link->a2dp_data.codec_info.sbc.block_length = BT_A2DP_SBC_BLOCK_LENGTH_12;
                }
                else if (temp & 0x40)
                {
                    p_link->a2dp_data.codec_info.sbc.block_length = BT_A2DP_SBC_BLOCK_LENGTH_8;
                }
                else if (temp & 0x80)
                {
                    p_link->a2dp_data.codec_info.sbc.block_length = BT_A2DP_SBC_BLOCK_LENGTH_4;
                }

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[1] & 0x0c;
                if (temp & 0x04)
                {
                    p_link->a2dp_data.codec_info.sbc.subbands = BT_A2DP_SBC_SUBBANDS_8;
                }
                else if (temp & 0x08)
                {
                    p_link->a2dp_data.codec_info.sbc.subbands = BT_A2DP_SBC_SUBBANDS_4;
                }

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[1] & 0x03;
                if (temp & 0x01)
                {
                    p_link->a2dp_data.codec_info.sbc.allocation_method = BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS;
                }
                else if (temp & 0x02)
                {
                    p_link->a2dp_data.codec_info.sbc.allocation_method = BT_A2DP_SBC_ALLOCATION_METHOD_SNR;
                }

                p_link->a2dp_data.codec_info.sbc.min_bitpool = ((T_A2DP_CFG *)
                                                                msg_buf)->codec_info[2];
                p_link->a2dp_data.codec_info.sbc.max_bitpool = ((T_A2DP_CFG *)
                                                                msg_buf)->codec_info[3];
            }
            else if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_AAC)
            {
                uint8_t i;
                uint8_t temp;
                uint16_t sam_freq;
                uint16_t sam_freq_bit;

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[0];
                if (temp & 0x01)
                {
                    p_link->a2dp_data.codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_D_DRC;
                }
                else if (temp & 0x02)
                {
                    p_link->a2dp_data.codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_AAC_ELDV2;
                }
                else if (temp & 0x04)
                {
                    p_link->a2dp_data.codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_HE_AACV2;
                }
                else if (temp & 0x08)
                {
                    p_link->a2dp_data.codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_HE_AAC;
                }
                else if (temp & 0x10)
                {
                    p_link->a2dp_data.codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_AAC_SCALABLE;
                }
                else if (temp & 0x20)
                {
                    p_link->a2dp_data.codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_AAC_LTP;
                }
                else if (temp & 0x40)
                {
                    p_link->a2dp_data.codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_4_AAC_LC;
                }
                else if (temp & 0x80)
                {
                    p_link->a2dp_data.codec_info.aac.object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_2_AAC_LC;
                }

                sam_freq = (((T_A2DP_CFG *)msg_buf)->codec_info[1] << 8) +
                           (((T_A2DP_CFG *)msg_buf)->codec_info[2] & 0x80);
                for (i = 0; i < BT_A2DP_AAC_SR_NUM; i++)
                {
                    sam_freq_bit = (1 << (4 + i));
                    if (sam_freq & sam_freq_bit)
                    {
                        p_link->a2dp_data.codec_info.aac.sampling_frequency = sam_freq_bit;
                        break;
                    }
                }

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[2] & 0x0c;
                if (temp & 0x04)
                {
                    p_link->a2dp_data.codec_info.aac.channel_number = BT_A2DP_AAC_CHANNEL_NUMBER_2;
                }
                else if (temp & 0x08)
                {
                    p_link->a2dp_data.codec_info.aac.channel_number = BT_A2DP_AAC_CHANNEL_NUMBER_1;
                }

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[3] & 0x80;
                if (temp)
                {
                    p_link->a2dp_data.codec_info.aac.vbr_supported = true;
                }
                else
                {
                    p_link->a2dp_data.codec_info.aac.vbr_supported = false;
                }

                p_link->a2dp_data.codec_info.aac.bit_rate = ((T_A2DP_CFG *)msg_buf)->codec_info[5] +
                                                            (((T_A2DP_CFG *)msg_buf)->codec_info[4] << 8) +
                                                            ((((T_A2DP_CFG *)msg_buf)->codec_info[3] & 0x7f) << 16);
            }
            else if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_USAC)
            {
                uint8_t i;
                uint8_t temp;
                uint32_t sam_freq;
                uint32_t sam_freq_bit;

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[0] & 0xc0;
                if (temp & 0x80)
                {
                    p_link->a2dp_data.codec_info.usac.object_type = BT_A2DP_USAC_OBJECT_TYPE_MPEG_D_USAC_WITH_DRC;
                }

                sam_freq = ((((T_A2DP_CFG *)msg_buf)->codec_info[0] & 0x3f) << 24) +
                           (((T_A2DP_CFG *)msg_buf)->codec_info[1] << 16) +
                           (((T_A2DP_CFG *)msg_buf)->codec_info[2] << 8) +
                           (((T_A2DP_CFG *)msg_buf)->codec_info[3] & 0xf0);
                for (i = 0; i < BT_A2DP_USAC_SR_NUM; i++)
                {
                    sam_freq_bit = (1 << (4 + i));
                    if (sam_freq & sam_freq_bit)
                    {
                        p_link->a2dp_data.codec_info.usac.sampling_frequency = sam_freq_bit;
                        break;
                    }
                }

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[3] & 0x0f;
                if (temp & 0x04)
                {
                    p_link->a2dp_data.codec_info.usac.channel_number = BT_A2DP_USAC_CHANNEL_NUMBER_2;
                }
                else if (temp & 0x08)
                {
                    p_link->a2dp_data.codec_info.usac.channel_number = BT_A2DP_USAC_CHANNEL_NUMBER_1;
                }

                temp = ((T_A2DP_CFG *)msg_buf)->codec_info[4] & 0x80;
                if (temp)
                {
                    p_link->a2dp_data.codec_info.usac.vbr_supported = true;
                }
                else
                {
                    p_link->a2dp_data.codec_info.usac.vbr_supported = false;
                }

                p_link->a2dp_data.codec_info.usac.bit_rate = ((T_A2DP_CFG *)msg_buf)->codec_info[6] +
                                                             (((T_A2DP_CFG *)msg_buf)->codec_info[5] << 8) +
                                                             ((((T_A2DP_CFG *)msg_buf)->codec_info[4] & 0x7f) << 16);
            }
            else if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_VENDOR)
            {
                if (!memcmp(((T_A2DP_CFG *)msg_buf)->codec_info, bt_a2dp_ldac_codec_info, 6))
                {
                    p_link->a2dp_data.codec_type = BT_A2DP_CODEC_TYPE_LDAC;
                    p_link->a2dp_data.codec_info.ldac.sampling_frequency = ((T_A2DP_CFG *)msg_buf)->codec_info[6];
                    p_link->a2dp_data.codec_info.ldac.channel_mode = ((T_A2DP_CFG *)msg_buf)->codec_info[7];
                }
                else if (!memcmp(((T_A2DP_CFG *)msg_buf)->codec_info, bt_a2dp_lc3_codec_info, 6))
                {
                    uint8_t temp;

                    p_link->a2dp_data.codec_type = BT_A2DP_CODEC_TYPE_LC3;

                    temp = ((T_A2DP_CFG *)msg_buf)->codec_info[6] & 0xfc;
                    if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_48KHZ)
                    {
                        p_link->a2dp_data.codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_48KHZ;
                    }
                    else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_44_1KHZ)
                    {
                        p_link->a2dp_data.codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_44_1KHZ;
                    }
                    else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_32KHZ)
                    {
                        p_link->a2dp_data.codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_32KHZ;
                    }
                    else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_24KHZ)
                    {
                        p_link->a2dp_data.codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_24KHZ;
                    }
                    else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_16KHZ)
                    {
                        p_link->a2dp_data.codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_16KHZ;
                    }
                    else if (temp & BT_A2DP_LC3_SAMPLING_FREQUENCY_8KHZ)
                    {
                        p_link->a2dp_data.codec_info.lc3.sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_8KHZ;
                    }

                    temp = ((T_A2DP_CFG *)msg_buf)->codec_info[6] & 0x03;
                    if (temp & BT_A2DP_LC3_CHANNEL_NUM_2)
                    {
                        p_link->a2dp_data.codec_info.lc3.channel_number = BT_A2DP_LC3_CHANNEL_NUM_2;
                    }
                    else if (temp & BT_A2DP_LC3_CHANNEL_NUM_1)
                    {
                        p_link->a2dp_data.codec_info.lc3.channel_number = BT_A2DP_LC3_CHANNEL_NUM_1;
                    }

                    temp = ((T_A2DP_CFG *)msg_buf)->codec_info[7] & 0x06;
                    if (temp & BT_A2DP_LC3_FRAME_DURATION_10MS)
                    {
                        p_link->a2dp_data.codec_info.lc3.frame_duration = BT_A2DP_LC3_FRAME_DURATION_10MS;
                    }
                    else if (temp & BT_A2DP_LC3_FRAME_DURATION_7_5MS)
                    {
                        p_link->a2dp_data.codec_info.lc3.frame_duration = BT_A2DP_LC3_FRAME_DURATION_7_5MS;
                    }

                    p_link->a2dp_data.codec_info.lc3.frame_length = ((T_A2DP_CFG *)msg_buf)->codec_info[8] +
                                                                    ((((T_A2DP_CFG *)msg_buf)->codec_info[7] & 0x01) << 8);
                }
                else if (!memcmp(((T_A2DP_CFG *)msg_buf)->codec_info, bt_a2dp_lhdc_v5_codec_info, 6))
                {
                    uint8_t temp;

                    p_link->a2dp_data.codec_type = BT_A2DP_CODEC_TYPE_LHDC;

                    temp = ((T_A2DP_CFG *)msg_buf)->codec_info[6] & 0x35;
                    if (temp & BT_A2DP_LHDC_SAMPLING_FREQUENCY_192KHZ)
                    {
                        p_link->a2dp_data.codec_info.lhdc.sampling_frequency = BT_A2DP_LHDC_SAMPLING_FREQUENCY_192KHZ;
                    }
                    else if (temp & BT_A2DP_LHDC_SAMPLING_FREQUENCY_96KHZ)
                    {
                        p_link->a2dp_data.codec_info.lhdc.sampling_frequency = BT_A2DP_LHDC_SAMPLING_FREQUENCY_96KHZ;
                    }
                    else if (temp & BT_A2DP_LHDC_SAMPLING_FREQUENCY_48KHZ)
                    {
                        p_link->a2dp_data.codec_info.lhdc.sampling_frequency = BT_A2DP_LHDC_SAMPLING_FREQUENCY_48KHZ;
                    }
                    else if (temp & BT_A2DP_LHDC_SAMPLING_FREQUENCY_44_1KHZ)
                    {
                        p_link->a2dp_data.codec_info.lhdc.sampling_frequency = BT_A2DP_LHDC_SAMPLING_FREQUENCY_44_1KHZ;
                    }

                    temp = ((T_A2DP_CFG *)msg_buf)->codec_info[7] & 0xc0;
                    p_link->a2dp_data.codec_info.lhdc.min_bitrate = (T_BT_A2DP_LHDC_MIN_BITRATE)(temp >> 6);

                    temp = ((T_A2DP_CFG *)msg_buf)->codec_info[7] & 0x30;
                    p_link->a2dp_data.codec_info.lhdc.max_bitrate = (T_BT_A2DP_LHDC_MAX_BITRATE)(temp >> 4);

                    temp = ((T_A2DP_CFG *)msg_buf)->codec_info[7] & 0x07;
                    if (temp & BT_A2DP_LHDC_BIT_DEPTH_32BIT)
                    {
                        p_link->a2dp_data.codec_info.lhdc.bit_depth = BT_A2DP_LHDC_BIT_DEPTH_32BIT;
                    }
                    else if (temp & BT_A2DP_LHDC_BIT_DEPTH_24BIT)
                    {
                        p_link->a2dp_data.codec_info.lhdc.bit_depth = BT_A2DP_LHDC_BIT_DEPTH_24BIT;
                    }
                    else if (temp & BT_A2DP_LHDC_BIT_DEPTH_16BIT)
                    {
                        p_link->a2dp_data.codec_info.lhdc.bit_depth = BT_A2DP_LHDC_BIT_DEPTH_16BIT;
                    }

                    temp = ((T_A2DP_CFG *)msg_buf)->codec_info[8] & 0x0f;
                    if (temp & BT_A2DP_LHDC_VERSION_NUMBER_5_3)
                    {
                        p_link->a2dp_data.codec_info.lhdc.version_number = BT_A2DP_LHDC_VERSION_NUMBER_5_3;
                    }
                    else if (temp & BT_A2DP_LHDC_VERSION_NUMBER_5_2)
                    {
                        p_link->a2dp_data.codec_info.lhdc.version_number = BT_A2DP_LHDC_VERSION_NUMBER_5_2;
                    }
                    else if (temp & BT_A2DP_LHDC_VERSION_NUMBER_5_1)
                    {
                        p_link->a2dp_data.codec_info.lhdc.version_number = BT_A2DP_LHDC_VERSION_NUMBER_5_1;
                    }
                    else if (temp & BT_A2DP_LHDC_VERSION_NUMBER_5_0)
                    {
                        p_link->a2dp_data.codec_info.lhdc.version_number = BT_A2DP_LHDC_VERSION_NUMBER_5_0;
                    }

                    if (((T_A2DP_CFG *)msg_buf)->codec_info[9] & 0x40)
                    {
                        p_link->a2dp_data.codec_info.lhdc.low_latency = true;
                    }
                    else
                    {
                        p_link->a2dp_data.codec_info.lhdc.low_latency = false;
                    }

                    if (((T_A2DP_CFG *)msg_buf)->codec_info[9] & 0x04)
                    {
                        p_link->a2dp_data.codec_info.lhdc.meta = true;
                    }
                    else
                    {
                        p_link->a2dp_data.codec_info.lhdc.meta = false;
                    }

                    if (((T_A2DP_CFG *)msg_buf)->codec_info[9] & 0x02)
                    {
                        p_link->a2dp_data.codec_info.lhdc.jas = true;
                    }
                    else
                    {
                        p_link->a2dp_data.codec_info.lhdc.jas = false;
                    }

                    if (((T_A2DP_CFG *)msg_buf)->codec_info[9] & 0x01)
                    {
                        p_link->a2dp_data.codec_info.lhdc.ar = true;
                    }
                    else
                    {
                        p_link->a2dp_data.codec_info.lhdc.ar = false;
                    }
                }
                else
                {
                    memcpy(p_link->a2dp_data.codec_info.vendor.info, ((T_A2DP_CFG *)msg_buf)->codec_info, 12);
                }
            }

            payload.msg_buf = &(p_link->a2dp_data);
            bt_mgr_dispatch(BT_MSG_A2DP_CONFIG_CMPL, &payload);
        }
        break;

    case A2DP_MSG_OPEN:
        if (p_link != NULL)
        {
            uint16_t max_pkt_len;

            max_pkt_len = *((uint16_t *)msg_buf);

            if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_LHDC)
            {
                /* avdtp header(12 bytes) + lhdc frame_num and seq_num header(2 byte) */
                max_pkt_len -= 14;
            }
            else if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_SBC ||
                     p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_LDAC ||
                     p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_LC3)
            {
                /* avdtp header(12 bytes) + frame num(1 byte) */
                max_pkt_len -= 13;
            }
            else if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_AAC ||
                     p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_USAC)
            {
                /* avdtp header(12 bytes) */
                max_pkt_len -= 12;
            }

            if (p_link->a2dp_data.a2dp_content_protect)
            {
                /* cp flag(1 byte)*/
                max_pkt_len -= 1;
            }

            p_link->a2dp_data.streaming_fg = false;
            payload.msg_buf = &max_pkt_len;
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_OPEN, &payload);
        }
        break;

    case A2DP_MSG_OPEN_FAIL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_OPEN_FAIL, &payload);
        }
        break;

    case A2DP_MSG_START_IND:
        if (p_link != NULL)
        {
            payload.msg_buf = &(p_link->a2dp_data);
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_START_IND, &payload);
        }
        else
        {
            a2dp_signal_start_cfm(bd_addr, false);
        }
        break;

    case A2DP_MSG_START_RSP:
        if (p_link != NULL)
        {
            p_link->a2dp_data.streaming_fg = true;
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_START_RSP, &payload);
        }
        break;

    case A2DP_MSG_SUSPEND:
        if (p_link != NULL)
        {
            p_link->a2dp_data.streaming_fg = false;
            p_link->a2dp_data.last_seq_num = 0;
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_STOP, &payload);
        }
        break;

    case A2DP_MSG_CLOSE:
        if (p_link != NULL)
        {
            p_link->a2dp_data.streaming_fg = false;
            p_link->a2dp_data.last_seq_num = 0;
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_CLOSE, &payload);
        }
        break;

    case A2DP_MSG_STREAM_DATA_IND:
        if (p_link != NULL)
        {
            T_BT_A2DP_STREAM_DATA_IND data;
            uint8_t csrc_len;
            uint8_t cp_len = 0;
            T_A2DP_STREAM_IND *stream_data = (T_A2DP_STREAM_IND *)msg_buf;

            csrc_len = (*stream_data->pkt_ptr & 0x0f) << 2;
            data.bt_clock = stream_data->bt_clock;
            data.seq_num = (*(stream_data->pkt_ptr + 2) << 8) +
                           *(stream_data->pkt_ptr + 3);
            data.timestamp = (*(stream_data->pkt_ptr + 4) << 24) +
                             (*(stream_data->pkt_ptr + 5) << 16) +
                             (*(stream_data->pkt_ptr + 6) << 8) +
                             *(stream_data->pkt_ptr + 7);

            if (p_link->a2dp_data.a2dp_content_protect)
            {
                cp_len = 1;
            }
            if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_SBC ||
                p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_LDAC ||
                p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_LC3)
            {
                /* remove header */
                data.frame_num = *(((T_A2DP_STREAM_IND *)msg_buf)->pkt_ptr + 12 + cp_len + csrc_len) & 0x0f;
                data.payload = ((T_A2DP_STREAM_IND *)msg_buf)->pkt_ptr + 13 + cp_len + csrc_len;
                data.len = ((T_A2DP_STREAM_IND *)msg_buf)->pkt_len - 13 - csrc_len - cp_len;
            }
            else if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_AAC ||
                     p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_USAC)
            {
                data.frame_num = 1;
                data.payload = (stream_data)->pkt_ptr + 12 + csrc_len + cp_len;
                data.len = (stream_data)->pkt_len - 12 - csrc_len - cp_len;
            }
            else if (p_link->a2dp_data.codec_type == BT_A2DP_CODEC_TYPE_LHDC)
            {
                data.frame_num = (*(((T_A2DP_STREAM_IND *)msg_buf)->pkt_ptr + 12 + cp_len + csrc_len) & 0x3C) >> 2;
                data.payload = (stream_data)->pkt_ptr + 12 + csrc_len + cp_len + 2;
                data.len = (stream_data)->pkt_len - 12 - csrc_len - cp_len - 2;
            }

            p_link->a2dp_data.last_seq_num = data.seq_num;

            payload.msg_buf = &data;
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_DATA_IND, &payload);
        }
        break;

    case A2DP_MSG_STREAM_DATA_RSP:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_A2DP_STREAM_DATA_RSP, &payload);
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
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            return a2dp_stream_data_send(bd_addr, seq_num, time_stamp, frame_num, frame_buf, len);
        }
    }

    return false;
}

bool bt_a2dp_stream_open_req(uint8_t        bd_addr[6],
                             T_BT_A2DP_ROLE role)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return a2dp_signal_open(bd_addr, role);
        }
    }

    return false;
}

bool bt_a2dp_stream_start_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, true);
            return a2dp_signal_start(bd_addr);
        }
    }

    return false;
}

bool bt_a2dp_stream_close_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return a2dp_signal_close(bd_addr);
        }
    }

    return false;
}

bool bt_a2dp_stream_suspend_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return a2dp_signal_suspend(bd_addr);
        }
    }

    return false;
}

bool bt_a2dp_stream_abort_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return a2dp_signal_abort(bd_addr);
        }
    }

    return false;
}

bool bt_a2dp_stream_delay_report_request(uint8_t  bd_addr[6],
                                         uint16_t latency)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return a2dp_signal_delay_report(bd_addr, latency);
        }
    }

    return false;
}

bool bt_a2dp_connect_req(uint8_t        bd_addr[6],
                         uint16_t       avdtp_ver,
                         T_BT_A2DP_ROLE role)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & A2DP_PROFILE_MASK) == 0)
        {
            bt_sniff_mode_exit(p_link, false);
            return a2dp_signal_connect_req(bd_addr, avdtp_ver, role);
        }
    }

    return false;
}

bool bt_a2dp_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return a2dp_disconnect_req(bd_addr);
    }

    return false;
}

bool bt_a2dp_connect_cfm(uint8_t bd_addr[6],
                         bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return a2dp_signal_connect_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_a2dp_stream_start_cfm(uint8_t bd_addr[6],
                              bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            if (accept == true)
            {
                p_link->a2dp_data.streaming_fg = true;
            }

            bt_sniff_mode_exit(p_link, true);
            return a2dp_signal_start_cfm(bd_addr, accept);
        }
    }

    return false;
}

bool bt_a2dp_active_link_set(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            if (btm_db.active_a2dp_index != p_link->link_id)
            {
                btm_db.active_a2dp_index = p_link->link_id;
                BTM_PRINT_INFO1("bt_a2dp_active_link_set: active_a2dp_index 0x%02x", btm_db.active_a2dp_index);
            }

            return true;
        }
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

        return a2dp_codec_add(sep.role, A2DP_CODEC_TYPE_SBC, media_codec_info);
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

        return a2dp_codec_add(sep.role, A2DP_CODEC_TYPE_AAC, media_codec_info);
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

        return a2dp_codec_add(sep.role, A2DP_CODEC_TYPE_USAC, media_codec_info);
    }
    else if (sep.codec_type == BT_A2DP_CODEC_TYPE_LDAC)
    {
        memcpy(media_codec_info, bt_a2dp_ldac_codec_info, 6);
        media_codec_info[6] = sep.u.codec_ldac.sampling_frequency_mask;
        media_codec_info[7] = sep.u.codec_ldac.channel_mode_mask;

        return a2dp_codec_add(sep.role, A2DP_CODEC_TYPE_LDAC, media_codec_info);
    }
    else if (sep.codec_type == BT_A2DP_CODEC_TYPE_LC3)
    {
        memcpy(media_codec_info, bt_a2dp_lc3_codec_info, 6);
        media_codec_info[6] = sep.u.codec_lc3.sampling_frequency_mask |
                              sep.u.codec_lc3.channel_num_mask;
        media_codec_info[7] = (uint8_t)(sep.u.codec_lc3.frame_length >> 8) |
                              sep.u.codec_lc3.frame_duration_mask;
        media_codec_info[8] = (uint8_t)sep.u.codec_lc3.frame_length;

        return a2dp_codec_add(sep.role, A2DP_CODEC_TYPE_LC3, media_codec_info);
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

        return a2dp_codec_add(sep.role, A2DP_CODEC_TYPE_LHDC, media_codec_info);
    }

    return false;
}

bool bt_a2dp_stream_endpoint_delete(T_BT_A2DP_STREAM_ENDPOINT sep)
{
    return a2dp_codec_delete(sep.role, (T_A2DP_CODEC_TYPE)sep.codec_type);
}

bool bt_a2dp_init(uint8_t  link_num,
                  uint16_t latency,
                  uint8_t  service_capabilities)
{
    a2dp_init(link_num, latency, service_capabilities, bt_a2dp_cback);

    return true;
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bt_a2dp.h"

bool bt_a2dp_stream_endpoint_add(T_BT_A2DP_STREAM_ENDPOINT sep)
{
    return false;
}

bool bt_a2dp_stream_endpoint_delete(T_BT_A2DP_STREAM_ENDPOINT sep)
{
    return false;
}

bool bt_a2dp_init(uint8_t  link_num,
                  uint16_t latency,
                  uint8_t  service_capabilities)
{
    return false;
}

bool bt_a2dp_connect_req(uint8_t  bd_addr[6],
                         uint16_t avdtp_ver,
                         T_BT_A2DP_ROLE role)
{
    return false;
}

bool bt_a2dp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_a2dp_connect_cfm(uint8_t bd_addr[6],
                         bool    accept)
{
    return false;
}

bool bt_a2dp_stream_start_cfm(uint8_t bd_addr[6],
                              bool    accept)
{
    return false;
}

bool bt_a2dp_stream_open_req(uint8_t bd_addr[6],
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

bool bt_a2dp_stream_delay_report_request(uint8_t  bd_addr[6],
                                         uint16_t latency)
{
    return false;
}
#endif
