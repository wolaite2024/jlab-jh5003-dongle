/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _A2DP_H_
#define _A2DP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_CODEC_INFO_SIZE      12

typedef enum t_a2dp_codec_type
{
    A2DP_CODEC_TYPE_SBC     = 0x00,
    A2DP_CODEC_TYPE_AAC     = 0x02,
    A2DP_CODEC_TYPE_USAC    = 0x03,
    A2DP_CODEC_TYPE_LDAC    = 0xf0,
    A2DP_CODEC_TYPE_LC3     = 0xf1,
    A2DP_CODEC_TYPE_LHDC    = 0xf2,
    A2DP_CODEC_TYPE_VENDOR  = 0xff,
} T_A2DP_CODEC_TYPE;

/** A2DP source/sink state */
typedef enum t_a2dp_state
{
    A2DP_STATE_DISCONNECTED  = 0x00,
    A2DP_STATE_ALLOCATED     = 0x01,
    A2DP_STATE_CONNECTING    = 0x02,
    A2DP_STATE_CONNECTED     = 0x03,
    A2DP_STATE_DISCONNECTING = 0x04,
    A2DP_STATE_STREAMING     = 0x05,
} T_A2DP_STATE;

typedef enum t_a2dp_msg
{
    A2DP_MSG_CONN_IND         = 0x00,
    A2DP_MSG_CONN_CMPL        = 0x01,
    A2DP_MSG_CONN_FAIL        = 0x02,
    A2DP_MSG_DISCONN          = 0x03,
    A2DP_MSG_SET_CFG          = 0x04,
    A2DP_MSG_RE_CFG           = 0x05,
    A2DP_MSG_OPEN             = 0x06,
    A2DP_MSG_OPEN_FAIL        = 0x07,
    A2DP_MSG_START_IND        = 0x08,
    A2DP_MSG_START_RSP        = 0x09,
    A2DP_MSG_SUSPEND          = 0x0a,
    A2DP_MSG_CLOSE            = 0x0b,
    A2DP_MSG_ABORT            = 0x0c,
    A2DP_MSG_STREAM_DATA_IND  = 0x0d,
    A2DP_MSG_STREAM_DATA_RSP  = 0x0e,
} T_A2DP_MSG;

typedef union
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
} T_A2DP_CODEC_INFO;

typedef struct t_a2dp_cfg
{
    uint8_t     role;
    uint8_t     codec_type;
    uint8_t     cp_flag;
    uint8_t     delay_report_flag;
    uint8_t     codec_info[MAX_CODEC_INFO_SIZE];
} T_A2DP_CFG;

typedef struct t_a2dp_stream_ind
{
    uint8_t    *pkt_ptr;
    uint16_t    pkt_len;
    uint32_t    bt_clock;
} T_A2DP_STREAM_IND;

typedef struct t_roleswap_a2dp_info
{
    uint8_t           bd_addr[6];
    uint16_t          sig_cid;
    uint16_t          stream_cid;
    uint8_t           int_flag;
    uint8_t           int_seid;
    uint8_t           acp_seid_idx;
    uint8_t           acp_seid;
    uint8_t           delay_report;
    uint8_t           data_offset;
    uint8_t           content_proect;
    uint8_t           sig_state;
    uint8_t           state;
    uint8_t           tx_trans_label;
    uint8_t           rx_start_trans_label;
    uint8_t           cmd_flag;
    uint16_t          last_seq_number;
    uint8_t           codec_type;
    uint8_t           role;
    T_A2DP_CODEC_INFO codec_info;
} T_ROLESWAP_A2DP_INFO;

typedef struct t_roleswap_a2dp_transact
{
    uint8_t           bd_addr[6];
    uint8_t           sig_state;
    uint8_t           state;
    uint8_t           tx_trans_label;
    uint8_t           rx_start_trans_label;
    uint16_t          last_seq_number;
    uint8_t           cmd_flag;
    uint8_t           codec_type;
    T_A2DP_CODEC_INFO codec_info;
} T_ROLESWAP_A2DP_TRANSACT;

typedef void(*P_A2DP_CBACK)(uint8_t     bd_addr[6],
                            T_A2DP_MSG  msg_type,
                            void       *msg_buf);

bool a2dp_init(uint8_t      link_num,
               uint16_t     latency,
               uint8_t      service_capabilities,
               P_A2DP_CBACK cback);

bool a2dp_codec_add(uint8_t            role,
                    T_A2DP_CODEC_TYPE  codec_type,
                    uint8_t           *media_codec_info);

bool a2dp_codec_delete(uint8_t            role,
                       T_A2DP_CODEC_TYPE  codec_type);

bool a2dp_signal_discover(uint8_t bd_addr[6]);

bool a2dp_signal_cpbs_get(uint8_t bd_addr[6]);

bool a2dp_signal_cfg_set(uint8_t bd_addr[6]);

bool a2dp_signal_open(uint8_t bd_addr[6],
                      uint8_t role);

bool a2dp_signal_start(uint8_t bd_addr[6]);

bool a2dp_signal_close(uint8_t bd_addr[6]);

bool a2dp_signal_suspend(uint8_t bd_addr[6]);

bool a2dp_signal_abort(uint8_t bd_addr[6]);

bool a2dp_signal_delay_report(uint8_t  bd_addr[6],
                              uint16_t latency);

bool a2dp_stream_data_send(uint8_t   bd_addr[6],
                           uint16_t  seq_num,
                           uint32_t  time_stamp,
                           uint8_t   frame_num,
                           uint8_t  *p_data,
                           uint16_t  len);

bool a2dp_signal_connect_req(uint8_t  bd_addr[6],
                             uint16_t avdtp_ver,
                             uint8_t  role);

bool a2dp_stream_connect_req(uint8_t bd_addr[6]);

bool a2dp_signal_connect_cfm(uint8_t bd_addr[6],
                             bool    accept);

bool a2dp_signal_start_cfm(uint8_t bd_addr[6],
                           bool    accept);

bool a2dp_disconnect_req(uint8_t bd_addr[6]);

bool a2dp_get_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_A2DP_INFO *p_info);

bool a2dp_set_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_A2DP_INFO *p_info);

bool a2dp_del_roleswap_info(uint8_t bd_addr[6]);

uint16_t a2dp_get_stream_cid(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _A2DP_H_ */
