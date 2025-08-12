/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_HFP_INT_H_
#define _BT_HFP_INT_H_

#include <stdint.h>
#include <stdbool.h>

#include "hfp.h"
#include "bt_hfp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define HFP_INBAND_RINGTONE_ENABLE      0x01
#define HFP_BATTERY_BIEV_ENABLE         0x02
#define HFP_ESCO_S4_ENABLE              0x04
#define HFP_CODEC_NEGOTIATION_ENABLE    0x08

#define CLCC_IND_CALL_IDLE              0x00
#define CLCC_IND_CALL_ACTIVE            0x01
#define CLCC_IND_CALL_HELD              0x02
#define CLCC_IND_CALL_DIALING           0x04
#define CLCC_IND_CALL_DIAL_ALERT        0x08
#define CLCC_IND_CALL_INCOMING          0x10
#define CLCC_IND_CALL_WAITING           0x20

typedef enum t_bt_hfp_state
{
    HF_STATE_STANDBY          = 0x00,
    HF_STATE_CONNECTING       = 0x01,
    HF_STATE_RFCOMM_CONNECTED = 0x02,
    HF_STATE_CONNECTED        = 0x03,
    HS_STATE_CONNECTED        = 0x04
} T_BT_HFP_STATE;

typedef enum t_bt_hfp_ag_sco_setting
{
    HFP_AG_SCO_SETTING_ANY  = 0x00,
    HFP_AG_SCO_SETTING_T2   = 0x01,
    HFP_AG_SCO_SETTING_T1   = 0x02,
    HFP_AG_SCO_SETTING_S4   = 0x03,
    HFP_AG_SCO_SETTING_S3   = 0x04,
    HFP_AG_SCO_SETTING_S2   = 0x05,
    HFP_AG_SCO_SETTING_S1   = 0x06,
    HFP_AG_SCO_SETTING_D1   = 0x07,
} T_BT_HFP_AG_SCO_SETTING;

typedef enum t_bt_hfp_role
{
    HFP_ROLE_HF = 0x00,
    HFP_ROLE_AG = 0x01,
} T_BT_HFP_ROLE;

typedef enum t_bt_hfp_cmd_rsp
{
    HFP_CMD_OK          = 0x00,
    HFP_CMD_ERROR       = 0x01,
    HFP_CMD_TIMEOUT     = 0x02,
} T_BT_HFP_CMD_RSP;

typedef struct t_hfp_ag_status_ind
{
    /*..+CIND: ("service",(0-1)),("call",(0-1)),("callsetup",(0-3)),("battchg",(0-5)),("signal",(0-5)),("roam",(0-1)),("callheld",(0-2))..*/
    T_HFP_SERVICE_STATUS        service_status;
    T_HFP_CALL_STATUS           call_status;
    T_HFP_CALL_SETUP_STATUS     call_setup_status;
    T_HFP_CALL_HELD_STATUS      call_held_status;
    uint8_t                     signal_status;
    T_HFP_ROAM_STATUS           roam_status;
    uint8_t                     batt_chg_status;
} T_HFP_AG_STATUS_IND;

typedef struct
{
    char    call_num[20];
    uint8_t call_num_type;
} T_HFP_AG_CALL_INFO;

typedef struct t_hfp_link_data
{
    void                           *timer_ring;
    T_BT_HFP_STATE                  hfp_state;
    uint8_t                         hsp_active_fg;
    T_BT_HFP_ROLE                   hfp_role;
    T_HFP_CODEC_TYPE                codec_type;
    uint16_t                        capabilities;
    uint8_t                         call_status;
    uint8_t                         prev_call_status;
    uint8_t                         clcc_ind;
    uint8_t                         bat_report_type;
    uint8_t                         batt_level;
    uint8_t                         supported_features;
    T_HFP_AG_STATUS_IND             ag_status_ind;
    T_BT_HFP_AG_SCO_SETTING         sco_setting;
    uint8_t                         codec_type_sent;
    bool                            codec_updated;
    bool                            sco_int_flag;

    T_HFP_AG_CALL_INFO              ag_call_info;
    uint16_t                        ring_interval;
} T_HFP_LINK_DATA;

typedef struct
{
    uint16_t cause;
} T_BT_HFP_DISCONN_INFO;

typedef struct
{
    uint8_t  mode;
    uint8_t  format;
    char     name[17];
} T_BT_HFP_NETWORK_OPERATOR_IND;

typedef struct
{
    char     number[20];
    uint8_t  type;
    uint8_t  service;
} T_BT_HFP_SUBSCRIBER_NUMBER_IND;

typedef struct
{
    char     number[20];
    uint8_t  type;
} T_BT_HFP_CALLER_ID_IND;

typedef struct
{
    char     number[20];
    uint8_t  type;
} T_BT_HFP_CALL_WAITING_IND;

typedef struct
{
    char dtmf_code;
} T_BT_HFP_AG_CALL_DTMF_CODE;

typedef struct
{
    char number[20];
} T_BT_HFP_AG_DIAL_WITH_NUMBER;

typedef struct
{
    uint8_t num;
} T_BT_HFP_AG_DIAL_WITH_MEMORY;

typedef struct
{
    uint16_t cause;
} T_BT_HFP_AG_DISCONN_INFO;

void bt_hfp_ag_ringing_start(uint8_t bd_addr[6]);

void bt_hfp_ag_codec_negotiate(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_HFP_INT_H_ */
