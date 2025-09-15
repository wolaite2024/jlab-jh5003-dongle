/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "btm.h"
#include "bt_a2dp.h"
#include "app_cfg.h"
#include "app_link_util.h"
#include "app_src_a2dp.h"
#include "string.h"

static void app_src_a2dp_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;

    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_A2DP_CONN_IND:
        {
            p_link = app_find_br_link(param->a2dp_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_a2dp_connect_cfm(p_link->bd_addr, 0, true);
            }
        }
        break;

    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
            p_link = app_find_br_link(param->a2dp_config_cmpl.bd_addr);
            if (p_link != NULL)
            {
                T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL *cfg = &param->a2dp_config_cmpl;
                p_link->a2dp_codec_type = param->a2dp_config_cmpl.codec_type;

                if (param->a2dp_config_cmpl.role == BT_A2DP_ROLE_SNK)
                {
                    bt_a2dp_stream_delay_report_req(param->a2dp_config_cmpl.bd_addr, A2DP_LATENCY_MS);
                }

                if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_SBC)
                {
                    p_link->a2dp_codec_info.sbc.sampling_frequency = cfg->codec_info.sbc.sampling_frequency;
                    p_link->a2dp_codec_info.sbc.channel_mode = cfg->codec_info.sbc.channel_mode;
                    p_link->a2dp_codec_info.sbc.block_length = cfg->codec_info.sbc.block_length;
                    p_link->a2dp_codec_info.sbc.subbands = cfg->codec_info.sbc.subbands;
                    p_link->a2dp_codec_info.sbc.allocation_method = cfg->codec_info.sbc.allocation_method;
                    p_link->a2dp_codec_info.sbc.min_bitpool = cfg->codec_info.sbc.min_bitpool;
                    p_link->a2dp_codec_info.sbc.max_bitpool = cfg->codec_info.sbc.max_bitpool;
                }
                else if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_AAC)
                {
                    p_link->a2dp_codec_info.aac.object_type = cfg->codec_info.aac.object_type;
                    p_link->a2dp_codec_info.aac.sampling_frequency = cfg->codec_info.aac.sampling_frequency;
                    p_link->a2dp_codec_info.aac.channel_number = cfg->codec_info.aac.channel_number;
                    p_link->a2dp_codec_info.aac.vbr_supported = cfg->codec_info.aac.vbr_supported;
                    //p_link->a2dp_codec_info.aac.allocation_method = cfg->codec_info.aac.allocation_method;
                    p_link->a2dp_codec_info.aac.bit_rate = cfg->codec_info.aac.bit_rate;
                }
                else
                {
                    memcpy(p_link->a2dp_codec_info.vendor.info, cfg->codec_info.vendor.info, 12);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {

        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {

        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {

        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {

        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {

        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_src_a2dp_cback: event_type 0x%04x", event_type);
    }
}

static void app_src_a2dp_profile_init(void)
{
    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        T_BT_A2DP_STREAM_ENDPOINT sep1;
        //T_BT_A2DP_STREAM_END_POINT sep2;

        sep1.codec_type = BT_A2DP_CODEC_TYPE_SBC;
        sep1.role = BT_A2DP_ROLE_SRC;
        //sep1.u.codec_sbc.sampling_frequency_mask = BT_A2DP_SBC_SAMPLING_FREQUENCY_16KHZ |
        //                                           BT_A2DP_SBC_SAMPLING_FREQUENCY_32KHZ |
        //                                           BT_A2DP_SBC_SAMPLING_FREQUENCY_44_1KHZ |
        //                                           BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ;
        sep1.u.codec_sbc.sampling_frequency_mask = BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ;

        sep1.u.codec_sbc.channel_mode_mask = BT_A2DP_SBC_CHANNEL_MODE_MONO |
                                             BT_A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL |
                                             BT_A2DP_SBC_CHANNEL_MODE_STEREO |
                                             BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO;

        sep1.u.codec_sbc.block_length_mask = BT_A2DP_SBC_BLOCK_LENGTH_4 | BT_A2DP_SBC_BLOCK_LENGTH_8 |
                                             BT_A2DP_SBC_BLOCK_LENGTH_12 | BT_A2DP_SBC_BLOCK_LENGTH_16;

        sep1.u.codec_sbc.subbands_mask = BT_A2DP_SBC_SUBBANDS_4 | BT_A2DP_SBC_SUBBANDS_8;

        sep1.u.codec_sbc.allocation_method_mask = BT_A2DP_SBC_ALLOCATION_METHOD_SNR |
                                                  BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS;
        sep1.u.codec_sbc.min_bitpool = 0x02;
        sep1.u.codec_sbc.max_bitpool = 0x23;

        //sep2.codec_type = BT_A2DP_CODEC_TYPE_AAC;
        //sep2.u.codec_aac.object_type_mask = BT_A2DP_AAC_OBJECT_TYPE_MPEG_2_AAC_LC;

        //sep2.u.codec_aac.sampling_frequency_mask = BT_A2DP_AAC_SAMPLING_FREQUENCY_44_1KHZ |
        //                                           BT_A2DP_AAC_SAMPLING_FREQUENCY_48KHZ;

        //sep2.u.codec_aac.channel_number_mask = BT_A2DP_AAC_CHANNEL_NUMBER_1 | BT_A2DP_AAC_CHANNEL_NUMBER_2;
        //sep2.u.codec_aac.vbr_supported = true;
        //sep2.u.codec_aac.bit_rate = 0;

        bt_a2dp_init(BT_A2DP_CAPABILITY_MEDIA_TRANSPORT | BT_A2DP_CAPABILITY_MEDIA_CODEC |
                     BT_A2DP_CAPABILITY_DELAY_REPORTING);
        bt_a2dp_stream_endpoint_add(sep1);
        //bt_a2dp_stream_end_point_set(sep2);
    }
}

void app_src_a2dp_init(void)
{
    app_src_a2dp_profile_init();
    bt_mgr_cback_register(app_src_a2dp_cback);
}


