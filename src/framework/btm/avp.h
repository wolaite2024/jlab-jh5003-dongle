/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _AVP_H_
#define _AVP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
{
    uint8_t *p_data;
    uint16_t data_length;
} T_AVP_DATA_IND;

typedef enum
{
    AVP_MSG_CONTROL_CONN_REQ     = 0x00,
    AVP_MSG_CONTROL_CONN_RSP     = 0x01,
    AVP_MSG_CONTROL_CONNECTED    = 0x02,
    AVP_MSG_CONTROL_CONN_FAIL    = 0x03,
    AVP_MSG_CONTROL_DISCONNECTED = 0x04,
    AVP_MSG_CONTROL_DATA_IND     = 0x05,
    AVP_MSG_AUDIO_CONN_REQ       = 0x06,
    AVP_MSG_AUDIO_CONN_RSP       = 0x07,
    AVP_MSG_AUDIO_CONNECTED      = 0x08,
    AVP_MSG_AUDIO_CONN_FAIL      = 0x09,
    AVP_MSG_AUDIO_DISCONNECTED   = 0x0a,
    AVP_MSG_AUDIO_DATA_IND       = 0x0b,
} T_AVP_MSG;

typedef struct
{
    uint16_t    cause;
} T_AVP_DISCONN_INFO;

typedef enum
{
    AVP_STATE_DISCONNECTED  = 0x00,
    AVP_STATE_CONNECTING    = 0x01,
    AVP_STATE_CONNECTED     = 0x02,
    AVP_STATE_DISCONNECTING = 0x03,
} T_AVP_STATE;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    control_l2c_cid;
    uint8_t     control_state;
    uint8_t     control_data_offset;
    uint16_t    audio_l2c_cid;
    uint8_t     audio_state;
    uint8_t     audio_data_offset;
} T_ROLESWAP_AVP_INFO;

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     control_state;
    uint8_t     audio_state;
} T_ROLESWAP_AVP_TRANSACT;

typedef void(*P_AVP_CBACK)(uint8_t    bd_addr[6],
                           T_AVP_MSG  msg_type,
                           void      *msg_buf);

bool avp_init(P_AVP_CBACK cback);

bool avp_control_conn_req(uint8_t bd_addr[6]);

bool avp_audio_conn_req(uint8_t bd_addr[6]);

bool avp_control_disconn_req(uint8_t bd_addr[6]);

bool avp_audio_disconn_req(uint8_t bd_addr[6]);

bool avp_control_send_data(uint8_t   bd_addr[6],
                           uint8_t  *p_data,
                           uint16_t  data_len,
                           bool      flushable);

bool avp_audio_send_data(uint8_t  bd_addr[6],
                         uint8_t *p_data,
                         int      data_len);

bool avp_control_del_roleswap_info(uint8_t bd_addr[6]);

bool avp_audio_del_roleswap_info(uint8_t bd_addr[6]);

bool avp_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_AVP_INFO *p_info);

bool avp_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_AVP_INFO *p_info);

bool avp_voice_recognition_enable_req(uint8_t   bd_addr[6],
                                      uint16_t  conn_handle,
                                      void     *p_value,
                                      uint16_t  length);

bool avp_voice_recognition_disable_req(uint8_t   bd_addr[6],
                                       uint16_t  conn_handle,
                                       void     *p_value,
                                       uint16_t  length);

bool avp_voice_recognition_encode_start(uint8_t   bd_addr[6],
                                        uint16_t  conn_handle,
                                        void     *p_value,
                                        uint16_t  length);

bool avp_voice_recognition_encode_stop(uint8_t   bd_addr[6],
                                       uint16_t  conn_handle,
                                       void     *p_value,
                                       uint16_t  length);

bool avp_voice_recognition_data_send(uint8_t   bd_addr[6],
                                     uint16_t  conn_handle,
                                     void     *p_value,
                                     uint16_t  length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif








