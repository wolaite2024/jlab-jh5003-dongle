/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "btm.h"
#include "bt_a2dp.h"
#include "app_cfg.h"
#include "app_link_util.h"
#include "app_a2dp.h"

static void app_a2dp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
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
                else
                {
                    memcpy(p_link->a2dp_codec_info.vendor.info, cfg->codec_info.vendor.info, 12);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            // This message is handled in a2dp src policy
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            p_link = app_find_br_link(param->a2dp_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_a2dp_stream_start_cfm(p_link->bd_addr, true);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            // This message is handled in a2dp src policy
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            // This message is handled in a2dp src policy
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            // This message is handled in a2dp src policy
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
        APP_PRINT_INFO1("app_a2dp_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_a2dp_init(void)
{
    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        T_BT_A2DP_STREAM_ENDPOINT sep;

        bt_a2dp_init(BT_A2DP_CAPABILITY_MEDIA_TRANSPORT |
                     BT_A2DP_CAPABILITY_MEDIA_CODEC |
                     BT_A2DP_CAPABILITY_DELAY_REPORTING);

        sep.codec_type = BT_A2DP_CODEC_TYPE_SBC;
        sep.role = BT_A2DP_ROLE_SRC;
        sep.u.codec_sbc.sampling_frequency_mask = BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ;
        sep.u.codec_sbc.channel_mode_mask = BT_A2DP_SBC_CHANNEL_MODE_MONO |
                                            BT_A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL |
                                            BT_A2DP_SBC_CHANNEL_MODE_STEREO |
                                            BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO;
        sep.u.codec_sbc.block_length_mask = BT_A2DP_SBC_BLOCK_LENGTH_4 |
                                            BT_A2DP_SBC_BLOCK_LENGTH_8 |
                                            BT_A2DP_SBC_BLOCK_LENGTH_12 |
                                            BT_A2DP_SBC_BLOCK_LENGTH_16;
        sep.u.codec_sbc.subbands_mask = BT_A2DP_SBC_SUBBANDS_4 |
                                        BT_A2DP_SBC_SUBBANDS_8;
        sep.u.codec_sbc.allocation_method_mask = BT_A2DP_SBC_ALLOCATION_METHOD_SNR |
                                                 BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS;
        sep.u.codec_sbc.min_bitpool = 2;
        sep.u.codec_sbc.max_bitpool = 53;
        bt_a2dp_stream_endpoint_add(sep);

#if (F_APP_A2DP_CODEC_LC3_SUPPORT == 1)
        T_BT_A2DP_STREAM_ENDPOINT sep1;
        sep1.role = BT_A2DP_ROLE_SRC;
        sep1.codec_type = BT_A2DP_CODEC_TYPE_LC3;
        sep1.u.codec_lc3.sampling_frequency_mask = BT_A2DP_LC3_SAMPLING_FREQUENCY_48KHZ |
                                                   BT_A2DP_LC3_SAMPLING_FREQUENCY_44_1KHZ;
        sep1.u.codec_lc3.channel_num_mask = BT_A2DP_LC3_CHANNEL_NUM_1 |
                                            BT_A2DP_LC3_CHANNEL_NUM_2;
        sep1.u.codec_lc3.frame_duration_mask = BT_A2DP_LC3_FRAME_DURATION_10MS |
                                               BT_A2DP_LC3_FRAME_DURATION_7_5MS;
        sep1.u.codec_lc3.frame_length = 120;
        bt_a2dp_stream_endpoint_add(sep1);
#endif
        bt_mgr_cback_register(app_a2dp_bt_cback);
    }
}
