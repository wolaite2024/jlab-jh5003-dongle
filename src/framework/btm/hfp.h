/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _HFP_H_
#define _HFP_H_

#include <stdint.h>
#include <stdbool.h>

#include "hfp_utils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define HF_BIND_ENHANCED_SAFETY         0x01
#define HF_BIND_BATTERY_LEVEL           0x02

/*The Hands Free Profile specification limits the number of indicators returned by the AG to a maximum of 20.*/
#define MAX_AG_INDICATOR_COUNT 20
#define HFP_AG_INDICATOR_COUNT 7

typedef enum t_hfp_bat_reporting_type
{
    BAT_REPORTING_TYPE_NONE    = 0x00,
    BAT_REPORTING_TYPE_BIEV    = 0x01,
    BAT_REPORTING_TYPE_APPLE   = 0x02,
    BAT_REPORTING_TYPE_ANDROID = 0x03,
} T_HFP_BAT_REPORTING_TYPE;

/*voice recognition status of the phone as indicated */
typedef enum t_hfp_voice_recognition
{
    HFP_VOICE_RECOGNITION_IDLE   = 0x00,
    HFP_VOICE_RECOGNITION_ACTIVE = 0x01,
} T_HFP_VOICE_RECOGNITION;

typedef enum t_hfp_api_id
{
    HFP_CMD_ID_DIAL_WITH_NUMBER               = 0x00,
    HFP_CMD_ID_DIAL_LAST_NUMBER               = 0x01,
    HFP_CMD_ID_SEND_DTMF                      = 0x02,
    HFP_CMD_ID_ACCEPT_CALL                    = 0x03,
    HFP_CMD_ID_REJECT_HANGUP_CALL             = 0x04,
    HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE   = 0x05,
    HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE = 0x06,
    HFP_CMD_ID_3WAY_CALL_CONTROL_0            = 0x07,
    HFP_CMD_ID_3WAY_CALL_CONTROL_1            = 0x08,
    HFP_CMD_ID_3WAY_CALL_CONTROL_2            = 0x09,
    HFP_CMD_ID_3WAY_CALL_CONTROL_3            = 0x0a,
    HFP_CMD_ID_3WAY_CALL_CONTROL_AB           = 0x0b,
    HFP_CMD_ID_INFORM_MICROPHONE_GAIN         = 0x0c,
    HFP_CMD_ID_INFORM_SPEAKER_GAIN            = 0x0d,
    HFP_CMD_ID_HSP_BUTTON_PRESS               = 0x0e,
    HFP_CMD_ID_SEND_CLCC                      = 0x0f,
    HFP_CMD_ID_ESTABLISH_VOICE                = 0x10,
    HFP_CMD_ID_BCS                            = 0x11,
    HFP_CMD_ID_BAC                            = 0x12,
    HFP_CMD_ID_BATT_XAPL                      = 0x13,
    HFP_CMD_ID_BATT_BIEV                      = 0x14,
    HFP_CMD_ID_BATT_IPHONEACCEV               = 0x15,
    HFP_CMD_ID_BATT_XEVENT                    = 0x16,
    HFP_CMD_ID_CLIP                           = 0x17,
    HFP_CMD_ID_NREC                           = 0x18,
    HFP_CMD_ID_CMEE                           = 0x19,
    HFP_CMD_ID_CCWA                           = 0x1a,
    HFP_CMD_ID_SET_NETWORK_OPERATOR_FORMAT    = 0x1b,
    HFP_CMD_ID_QUERY_NETWORK_NAME             = 0x1c,
    HFP_CMD_ID_QUERY_SUBSCRIBER_NUM           = 0x1d,
    HFP_CMD_ID_INDICATOR_ACTIVATE             = 0x1e,
    HFP_CMD_ID_INDICATOR_DEACTIVATE           = 0x1f,
    HFP_CMD_ID_VND_AT_CMD                     = 0x20,
} T_HFP_API_ID;

typedef enum t_clcc_status
{
    CLCC_STATUS_ACTIVE                    = 0x00,
    CLCC_STATUS_HELD                      = 0x01,
    CLCC_STATUS_DIALING                   = 0x02,
    CLCC_STATUS_ALERTING                  = 0x03,
    CLCC_STATUS_INCOMING                  = 0x04,
    CLCC_STATUS_WAITING                   = 0x05,
    CLCC_STATUS_CALL_HELD_BY_RSP_AND_HOLD = 0x06,
} T_CLCC_STATUS;

typedef enum t_clcc_mode
{
    CLCC_MODE_VOICE = 0x00,
    CLCC_MODE_DATA  = 0x01,
    CLCC_MODE_FAX   = 0x02,
} T_CLCC_MODE;

typedef enum t_hfp_call_hold_action
{
    RELEASE_HELD_OR_WAITING_CALL                    = 0x00,
    RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL = 0x01,
    HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL    = 0x02,
    JOIN_TWO_CALLS                                  = 0x03,
} T_HFP_CALL_HOLD_ACTION;

typedef enum t_hfp_msg
{
    HFP_MSG_RFC_CONN_IND                                        = 0x00,
    HFP_MSG_RFC_CONN                                            = 0x01,
    HFP_MSG_CONN                                                = 0x02,
    HFP_MSG_CONN_FAIL                                           = 0x03,
    HFP_MSG_DISCONN                                             = 0x04,

    HFP_MSG_ACK_OK                                              = 0x05,
    HFP_MSG_ACK_ERROR                                           = 0x06,
    HFP_MSG_CME_ERROR                                           = 0x07,
    HFP_MSG_ACK_TIMEOUT                                         = 0x08,

    HFP_MSG_AG_INDICATOR_EVENT                                  = 0x09,
    HFP_MSG_RING                                                = 0x0a,
    HFP_MSG_CLIP                                                = 0x0b,
    HFP_MSG_CCWA                                                = 0x0c,
    HFP_MSG_CLCC                                                = 0x0d,
    HFP_MSG_VOICE_RECOGNITION_STATUS                            = 0x0e,
    HFP_MSG_AG_INBAND_RINGTONE_SETTING                          = 0x0f,
    HFP_MSG_AG_BIND                                             = 0x10,

    HFP_MSG_SET_MICROPHONE_GAIN                                 = 0x11,
    HFP_MSG_SET_SPEAKER_GAIN                                    = 0x12,
    HFP_MSG_SET_CODEC_TYPE                                      = 0x13,
    HFP_MSG_COPS                                                = 0x14,
    HFP_MSG_CNUM                                                = 0x15,
    HFP_MSG_UNKNOWN_CMD                                         = 0x16,
} T_HFP_MSG;

typedef struct t_indicator_map
{
    uint8_t size;
    struct
    {
        uint8_t type;
        uint8_t range;//range is not used practically
    }  indicator[MAX_AG_INDICATOR_COUNT];
} T_INDICATOR_MAP;

typedef struct t_hfp_indicator_name_table
{
    char        *indicator_name;
    uint8_t     indicator_type;
} T_HFP_INDICATOR_NAME_TABLE;

typedef struct t_hfp_msg_rfc_conn
{
    uint8_t             bd_addr[6];
    bool                hsp;
} T_HFP_MSG_RFC_CONN;

typedef struct t_hfp_msg_conn
{
    uint8_t             bd_addr[6];
} T_HFP_MSG_CONN;

typedef struct
{
    uint16_t    cause;
} T_HFP_DISCONN_INFO;

typedef struct t_hfp_msg_ack_ok
{
    uint8_t             bd_addr[6];
    uint8_t             last_cmd_id;
} T_HFP_MSG_ACK_OK;

typedef struct t_hfp_msg_cme_error
{
    uint8_t             bd_addr[6];
    uint8_t             last_cmd_id;
    uint8_t             error_number;
} T_HFP_MSG_CME_ERROR;

typedef T_HFP_MSG_CME_ERROR T_HFP_MSG_ACK_ERROR;
typedef T_HFP_MSG_ACK_OK T_HFP_MSG_WAIT_RSP_TOUT;

typedef struct t_hfp_msg_ag_indicator_event
{
    uint8_t             bd_addr[6];
    T_HFP_AG_INDICATOR_TYPE event_type;
    uint8_t             state;
} T_HFP_MSG_AG_INDICATOR_EVENT;

typedef struct t_hfp_msg_ring
{
    uint8_t             bd_addr[6];
} T_HFP_MSG_RING;

typedef struct t_hfp_msg_clip
{
    uint8_t             bd_addr[6];
    char                number[20];
    uint8_t             type;
} T_HFP_MSG_CLIP;

typedef T_HFP_MSG_CLIP T_HFP_MSG_CCWA;

typedef struct t_hfp_msg_cops
{
    uint8_t             bd_addr[6];
    uint8_t             mode;
    uint8_t             format;
    char                operator[17];
} T_HFP_MSG_COPS;

typedef struct t_hfp_msg_cnum
{
    uint8_t             bd_addr[6];
    char                number[20];
    uint8_t             type;
    uint8_t             service;
} T_HFP_MSG_CNUM;

typedef struct t_hfp_msg_clcc
{
    uint8_t             bd_addr[6];
    uint8_t             call_idx;
    uint8_t             dir_incoming;
    uint8_t             status;
    uint8_t             mode;
    uint8_t             mpty;
    char                number[20];
    uint8_t             type;
} T_HFP_MSG_CLCC;

typedef struct t_hfp_msg_voice_recognition_status
{
    uint8_t             bd_addr[6];
    uint8_t             status;
} T_HFP_MSG_VOICE_RECOGNITION_STATUS;

typedef struct t_hfp_msg_ag_inband_ringtone_set
{
    uint8_t             bd_addr[6];
    bool                ag_support;
} T_HFP_MSG_AG_INBAND_RINGTONE_SET;

typedef struct t_hfp_msg_bind_status
{
    uint8_t             bd_addr[6];
    bool                batt_ind_enable;
} T_HFP_MSG_BIND_STATUS;

typedef struct t_hfp_msg_microphone_volume_set
{
    uint8_t             bd_addr[6];
    uint8_t             volume;
} T_HFP_MSG_MICROPHONE_VOLUME_SET;

typedef struct t_hfp_msg_speaker_volume_set
{
    uint8_t             bd_addr[6];
    uint8_t             volume;
} T_HFP_MSG_SPEAKER_VOLUME_SET;

typedef struct t_hfp_msg_unknown_cmd
{
    uint8_t             bd_addr[6];
    char               *at_cmd;
} T_HFP_MSG_UNKNOWN_CMD;

typedef struct t_hfp_msg_set_codec_type
{
    uint8_t             bd_addr[6];
    T_HFP_CODEC_TYPE    codec_type;
} T_HFP_MSG_SET_CODEC_TYPE;

typedef struct t_roleswap_hfp_info
{
    uint8_t    bd_addr[6];
    uint16_t   l2c_cid;
    uint16_t   uuid;
    uint8_t    rfc_dlci;
    uint8_t    indicator_cnt;
    uint8_t    indicator_type[20];
    uint8_t    indicator_range[20];
    uint16_t   cpbs;
    uint16_t   frame_size;
    uint8_t    state;
    uint8_t    bat_report_type;
    uint8_t    at_cmd_credits;
    uint8_t    call_status;
    uint8_t    prev_call_status;
    uint8_t    service_status;
    uint8_t    supported_features;
    uint8_t    codec_type;
} T_ROLESWAP_HFP_INFO;

typedef struct t_roleswap_hfp_transact
{
    uint8_t    bd_addr[6];
    uint8_t    state;
    uint8_t    bat_report_type;
    uint8_t    at_cmd_credits;
    uint8_t    call_status;
    uint8_t    prev_call_status;
    uint8_t    service_status;
    uint8_t    supported_features;
    uint8_t    codec_type;
} T_ROLESWAP_HFP_TRANSACT;

typedef void (*P_HFP_HF_CBACK)(uint8_t    bd_addr[6],
                               T_HFP_MSG  msg_type,
                               void      *msg_buf);

bool hfp_init(uint8_t        link_num,
              uint8_t        rfc_hfp_chann_num,
              uint8_t        rfc_hsp_chann_num,
              P_HFP_HF_CBACK cback,
              uint16_t       hfp_brsf_cpbs,
              uint16_t       voice_codec);

bool hfp_connect_req(uint8_t bd_addr[6],
                     uint8_t remote_dlci,
                     bool    hfp_flag);

bool hfp_disconnect_req(uint8_t bd_addr[6]); //bd_addr == null means default link

bool hfp_dial_with_number(uint8_t     bd_addr[6],
                          const char *number);

bool hfp_dial_last_number(uint8_t bd_addr[6]);

bool hfp_dtmf_send(uint8_t bd_addr[6],
                   char    c); //Dual Tone Multi-Frequency code

bool hfp_accept_phone_call(uint8_t bd_addr[6]);

bool hfp_reject_phone_call(uint8_t bd_addr[6]);

bool hfp_set_voice_recognition(uint8_t bd_addr[6],
                               bool    enable);

bool hfp_call_hold_action(uint8_t                bd_addr[6],
                          T_HFP_CALL_HOLD_ACTION control);

bool hfp_3way_call_control_with_index(uint8_t bd_addr[6],
                                      uint8_t control,
                                      uint8_t index);

bool hfp_inform_ag_microphone_gain(uint8_t bd_addr[6],
                                   uint8_t level);

bool hfp_inform_ag_speaker_gain(uint8_t bd_addr[6],
                                uint8_t level);

bool hfp_send_clcc(uint8_t bd_addr[6]);

bool hfp_send_clip(uint8_t bd_addr[6]);

bool hfp_send_nrec_disable(uint8_t bd_addr[6]);

bool hfp_send_cmee_enable(uint8_t bd_addr[6]);

bool hfp_send_ccwa(uint8_t bd_addr[6]);

bool hfp_hsp_button_press(uint8_t bd_addr[6]);

bool hfp_remote_capabilities_get(uint8_t   bd_addr[6],
                                 uint16_t *cpbs);

bool hfp_ask_ag_establish_voice_chann(uint8_t bd_addr[6]);

bool hfp_network_operator_format_set(uint8_t bd_addr[6]);

bool hfp_network_operator_name_query(uint8_t bd_addr[6]);

bool hfp_subscriber_num_query(uint8_t bd_addr[6]);

bool hfp_indicator_activate(uint8_t  bd_addr[6],
                            uint32_t indicator_types);

bool hfp_indicator_deactivate(uint8_t  bd_addr[6],
                              uint32_t indicator_types);

bool hfp_batt_level_report(uint8_t     bd_addr[6],
                           const char *at_cmd,
                           uint8_t     type);

bool hfp_send_vnd_cmd(uint8_t     bd_addr[6],
                      const char *at_cmd);

uint16_t hfp_local_capabilities_get(void);

bool hfp_connect_cfm(uint8_t bd_addr[6],
                     bool    accept);

bool hfp_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_HFP_INFO *p_info);

bool hfp_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_HFP_INFO *p_info);

bool hfp_del_roleswap_info(uint8_t bd_addr[6]);

uint8_t hfp_get_rfc_profile_idx(uint16_t uuid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HFP_H_ */
