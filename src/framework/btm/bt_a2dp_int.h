/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_A2DP_INT_H_
#define _BT_A2DP_INT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_CODEC_INFO_SIZE      12

typedef enum t_bt_a2dp_state
{
    BT_A2DP_STATE_DISCONNECTED  = 0x00,
    BT_A2DP_STATE_CONNECTING    = 0x01,
    BT_A2DP_STATE_CONNECTED     = 0x02,
    BT_A2DP_STATE_DISCONNECTING = 0x03,
    BT_A2DP_STATE_STREAMING     = 0x04,
} T_BT_A2DP_STATE;

typedef union t_bt_a2dp_codec_info
{
    struct
    {
        uint8_t sampling_frequency;
        uint8_t channel_mode;
        uint8_t block_length;
        uint8_t subbands;
        uint8_t allocation_method;
        uint8_t min_bitpool;
        uint8_t max_bitpool;
    } sbc;

    struct
    {
        uint8_t  object_type;
        uint16_t sampling_frequency;
        uint8_t  channel_number;
        bool     vbr_supported;
        uint32_t bit_rate;
    } aac;

    struct
    {
        uint8_t  object_type;
        uint32_t sampling_frequency;
        uint8_t  channel_number;
        bool     vbr_supported;
        uint32_t bit_rate;
    } usac;

    struct
    {
        uint8_t sampling_frequency;
        uint8_t channel_mode;
    } ldac;

    struct
    {
        uint8_t  sampling_frequency;
        uint8_t  channel_number;
        uint8_t  frame_duration;
        uint16_t frame_length;
    } lc3;

    struct
    {
        uint8_t  sampling_frequency;
        uint8_t  min_bitrate;
        uint8_t  max_bitrate;
        uint8_t  bit_depth;
        uint8_t  version_number;
        bool     low_latency;
        bool     meta;
        bool     jas;
        bool     ar;
    } lhdc;

    struct
    {
        uint8_t  info[MAX_CODEC_INFO_SIZE];
    } vendor;
} T_BT_A2DP_CODEC_INFO;

typedef struct t_bt_a2dp_cfg_info
{
    uint8_t              role;
    uint8_t              codec_type;
    T_BT_A2DP_CODEC_INFO codec_info;
} T_BT_A2DP_CFG_INFO;

typedef struct t_bt_a2dp_stream_data_ind
{
    uint32_t  bt_clock;
    uint16_t  seq_num;
    uint32_t  timestamp;
    uint8_t   frame_num;
    uint16_t  len;
    uint8_t  *payload;
} T_BT_A2DP_STREAM_DATA_IND;

typedef struct t_roleswap_a2dp_info
{
    uint8_t              bd_addr[6];
    uint16_t             sig_cid;
    uint16_t             stream_cid;
    uint8_t              int_flag;
    uint8_t              int_seid;
    uint8_t              acp_seid_idx;
    uint8_t              acp_seid;
    uint8_t              delay_report;
    uint8_t              data_offset;
    uint8_t              content_proect;
    uint8_t              sig_state;
    uint8_t              state;
    uint8_t              tx_trans_label;
    uint8_t              rx_start_trans_label;
    uint8_t              cmd_flag;
    uint16_t             last_seq_number;
    uint8_t              codec_type;
    uint8_t              role;
    T_BT_A2DP_CODEC_INFO codec_info;
} T_ROLESWAP_A2DP_INFO;

typedef struct t_roleswap_a2dp_transact
{
    uint8_t              bd_addr[6];
    uint8_t              sig_state;
    uint8_t              state;
    uint8_t              tx_trans_label;
    uint8_t              rx_start_trans_label;
    uint16_t             last_seq_number;
    uint8_t              cmd_flag;
    uint8_t              codec_type;
    T_BT_A2DP_CODEC_INFO codec_info;
} T_ROLESWAP_A2DP_TRANSACT;

bool bt_a2dp_last_seq_num_get(uint8_t   bd_addr[6],
                              uint16_t *last_seq_num);

bool bt_a2dp_roleswap_info_get(uint8_t               bd_addr[6],
                               T_ROLESWAP_A2DP_INFO *info);

bool bt_a2dp_roleswap_info_set(uint8_t               bd_addr[6],
                               T_ROLESWAP_A2DP_INFO *info);

bool bt_a2dp_roleswap_info_del(uint8_t bd_addr[6]);

bool bt_a2dp_stream_cid_get(uint8_t bd_addr[6], uint16_t *cid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_A2DP_INT_H_ */
