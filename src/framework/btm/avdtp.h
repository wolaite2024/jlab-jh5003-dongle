/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AVDTP_H_
#define _AVDTP_H_

#include <stdint.h>
#include <stdbool.h>

#include "a2dp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_avdtp_msg
{
    AVDTP_MSG_SIG_CONN_REQ             = 0x00,
    AVDTP_MSG_SIG_CONN_RSP             = 0x01,
    AVDTP_MSG_SIG_CONNECTED            = 0x02,
    AVDTP_MSG_SIG_CONN_FAIL            = 0x03,
    AVDTP_MSG_SIG_DISCOVER_IND         = 0x04,
    AVDTP_MSG_SIG_DISCOVER_RSP         = 0x05,
    AVDTP_MSG_SIG_GET_CPBS_IND         = 0x06,
    AVDTP_MSG_SIG_GET_CPBS_RSP         = 0x07,
    AVDTP_MSG_SIG_SET_CFG_IND          = 0x08,
    AVDTP_MSG_SIG_SET_CFG_RSP          = 0x09,
    AVDTP_MSG_SIG_RECFG_IND            = 0x0A,
    AVDTP_MSG_SIG_RECFG_RSP            = 0x0B,
    AVDTP_MSG_SIG_GET_CFG_IND          = 0x0C,
    AVDTP_MSG_SIG_GET_CFG_RSP          = 0x0D,
    AVDTP_MSG_SIG_OPEN_IND             = 0x0E,
    AVDTP_MSG_SIG_OPEN_RSP             = 0x0F,
    AVDTP_MSG_SIG_START_IND            = 0x10,
    AVDTP_MSG_SIG_START_RSP            = 0x11,
    AVDTP_MSG_SIG_SUSPEND_IND          = 0x12,
    AVDTP_MSG_SIG_SUSPEND_RSP          = 0x13,
    AVDTP_MSG_SIG_SECURITY_CONTROL_IND = 0x14,
    AVDTP_MSG_SIG_SECURITY_CONTROL_RSP = 0x15,
    AVDTP_MSG_SIG_DELAY_REPORT_IND     = 0x16,
    AVDTP_MSG_SIG_DELAY_REPORT_RSP     = 0x17,
    AVDTP_MSG_SIG_CLOSE_IND            = 0x18,
    AVDTP_MSG_SIG_CLOSE_RSP            = 0x19,
    AVDTP_MSG_SIG_ABORT_IND            = 0x1A,
    AVDTP_MSG_SIG_ABORT_RSP            = 0x1B,
    AVDTP_MSG_SIG_DISCONNECTED         = 0x1C,
    AVDTP_MSG_STREAM_CONN_REQ          = 0x20,
    AVDTP_MSG_STREAM_CONN_RSP          = 0x21,
    AVDTP_MSG_STREAM_CONNECTED         = 0x22,
    AVDTP_MSG_STREAM_CONN_FAIL         = 0x23,
    AVDTP_MSG_STREAM_DISCONNECTED      = 0x24,
    AVDTP_MSG_STREAM_DATA_IND          = 0x25,
    AVDTP_MSG_STREAM_DATA_RSP          = 0x26,
} T_AVDTP_MSG;

typedef void(*P_AVDTP_CBACK)(uint16_t     cid,
                             T_AVDTP_MSG  msg_type,
                             void        *msg_buf);

bool avdtp_init(uint8_t       link_num,
                uint16_t      latency,
                uint8_t       service_capabilities,
                P_AVDTP_CBACK cback);

bool avdtp_codec_add(uint8_t            role,
                     T_A2DP_CODEC_TYPE  codec_type,
                     uint8_t           *media_codec_info);

bool avdtp_codec_delete(uint8_t            role,
                        T_A2DP_CODEC_TYPE  codec_type);

bool avdtp_stream_data_send(uint8_t   bd_addr[6],
                            uint16_t  seq_num,
                            uint32_t  time_stamp,
                            uint8_t   frame_num,
                            uint8_t  *p_data,
                            uint16_t  len);

void avdtp_signal_connect_req(uint8_t  bd_addr[6],
                              uint16_t avdtp_ver,
                              uint8_t  role);

void avdtp_stream_connect_req(uint8_t bd_addr[6]);

bool avdtp_signal_connect_cfm(uint8_t bd_addr[6],
                              bool    accept);

bool avdtp_signal_discover_req(uint8_t bd_addr[6]);

bool avdtp_signal_get_capability_req(uint8_t bd_addr[6]);

bool avdtp_signal_cfg_req(uint8_t bd_addr[6]);

bool avdtp_signal_open_req(uint8_t bd_addr[6],
                           uint8_t role);

bool avdtp_signal_recfg_req(uint8_t bd_addr[6],
                            uint8_t codec_type,
                            uint8_t role);

bool avdtp_signal_delay_report_req(uint8_t  bd_addr[6],
                                   uint16_t latency);

bool avdtp_signal_start_req(uint8_t bd_addr[6]);

bool avdtp_signal_suspend_req(uint8_t bd_addr[6]);

bool avdtp_signal_close_req(uint8_t bd_addr[6]);

bool avdtp_signal_abort_req(uint8_t bd_addr[6]);

bool avdtp_signal_start_cfm(uint8_t bd_addr[6],
                            bool    accept);

bool avdtp_signal_disconnect_req(uint8_t bd_addr[6]);

void avdtp_stream_disconnect_req(uint8_t bd_addr[6]);

bool avdtp_get_roleswap_info(uint8_t               bd_addr[6],
                             T_ROLESWAP_A2DP_INFO *p_info);

bool avdtp_set_roleswap_info(uint8_t               bd_addr[6],
                             T_ROLESWAP_A2DP_INFO *p_info);

bool avdtp_del_roleswap_info(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AVDTP_H_ */
