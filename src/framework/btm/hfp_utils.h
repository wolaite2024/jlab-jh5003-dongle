/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _HFP_UTILS_H_
#define _HFP_UTILS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*The Hands Free Profile specification limits the number of indicators returned by the AG to a maximum of 20.*/
#define MAX_AG_INDICATOR_COUNT 20

#define AG_CAPABILITY_3WAY                              (1 << 0)
#define AG_CAPABILITY_EC_NR                             (1 << 1)
#define AG_CAPABILITY_VOICE_RECOGNITION                 (1 << 2)
#define AG_CAPABILITY_INBAND_RINGING                    (1 << 3)
#define AG_CAPABILITY_VOICE_TAG                         (1 << 4)
#define AG_CAPABILITY_REJECT_CALLS                      (1 << 5)
#define AG_CAPABILITY_ENHANCED_CALL_STATUS              (1 << 6)
#define AG_CAPABILITY_ENHANCED_CALL_CONTROL             (1 << 7)
#define AG_CAPABILITY_EXTENED_ERROR_RESULT              (1 << 8)
#define AG_CAPABILITY_CODEC_NEGOTIATION                 (1 << 9)
#define AG_CAPABILITY_HF_INDICATORS                     (1 << 10)
#define AG_CAPABILITY_ESCO_S4_T2_SUPPORTED              (1 << 11)

/* bitfield containing the capabilities of the handsfree unit (HF)*/
#define HF_CAPABILITY_EC_NR                            (1 << 0)
#define HF_CAPABILITY_CALL_WAITING_OR_3WAY             (1 << 1)
#define HF_CAPABILITY_CLI                              (1 << 2)
#define HF_CAPABILITY_VOICE_RECOGNITION                (1 << 3)
#define HF_CAPABILITY_REMOTE_VOLUME_CONTROL            (1 << 4)
#define HF_CAPABILITY_ENHANCED_CALL_STATUS             (1 << 5)
#define HF_CAPABILITY_ENHANCED_CALL_CONTROL            (1 << 6)
#define HF_CAPABILITY_CODEC_NEGOTIATION                (1 << 7)
#define HF_CAPABILITY_HF_INDICATORS                    (1 << 8)
#define HF_CAPABILITY_ESCO_S4_T2_SUPPORTED             (1 << 9)

#define HF_SUPPORT_FEATURE_IN_SDP_EC_NR                 0x01;
#define HF_SUPPORT_FEATURE_IN_SDP_3WAY_CALL             0x02;
#define HF_SUPPORT_FEATURE_IN_SDP_CLIP                  0x04;
#define HF_SUPPORT_FEATURE_IN_SDP_VOICE_RECOGNITION     0x08;
#define HF_SUPPORT_FEATURE_IN_SDP_REMOTE_VOL_CONTROL    0x10;
#define HF_SUPPORT_FEATURE_IN_SDP_WIDEBAND_SPEECH       0x20;//version >= 1.6

typedef enum t_hfp_ag_indicator_type
{
    AG_INDICATOR_TYPE_SERVICE   = 0x00,
    AG_INDICATOR_TYPE_CALL      = 0x01,
    AG_INDICATOR_TYPE_CALLSETUP = 0x02,
    AG_INDICATOR_TYPE_CALLHELD  = 0x03,
    AG_INDICATOR_TYPE_SIGNAL    = 0x04,
    AG_INDICATOR_TYPE_ROAM      = 0x05,
    AG_INDICATOR_TYPE_BATTCHG   = 0x06,
    AG_INDICATOR_TYPE_UNKOWN    = 0xff,
} T_HFP_AG_INDICATOR_TYPE;

/* service (registration) status of the phone as indicated */
typedef enum t_hfp_service_status
{
    HFP_SERVICE_STATUS_NOT_REG = 0x00,
    HFP_SERVICE_STATUS_REG     = 0x01,
} T_HFP_SERVICE_STATUS;

typedef enum t_hfp_call_status
{
    HFP_CALL_STATUS_IDLE   = 0x00,
    HFP_CALL_STATUS_ACTIVE = 0x01,
} T_HFP_CALL_STATUS;

/* call setup status of the phone as indicated */
typedef enum t_hfp_call_setup_status
{
    HFP_CALL_SETUP_STATUS_IDLE          = 0x00,
    HFP_CALL_SETUP_STATUS_INCOMING_CALL = 0x01,
    HFP_CALL_SETUP_STATUS_OUTGOING_CALL = 0x02,
    HFP_CALL_SETUP_STATUS_ALERTING      = 0x03,
} T_HFP_CALL_SETUP_STATUS;

/* call held status of the phone as indicated */
typedef enum t_hfp_call_held_status
{
    HFP_CALL_HELD_STATUS_IDLE                 = 0x00,
    HFP_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL = 0x01,
    HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL  = 0x02,
} T_HFP_CALL_HELD_STATUS;

/* roam status as indicated*/
typedef enum t_hfp_roam_status
{
    HFP_ROAM_INACTIVE = 0x00,
    HFP_ROAM_ACTIVE   = 0x01,
} T_HFP_ROAM_STATUS;

typedef enum t_hfp_codec_type
{
    CODEC_TYPE_NONE = 0x00,
    CODEC_TYPE_CVSD = 0x01,
    CODEC_TYPE_MSBC = 0x02,
    CODEC_TYPE_LC3  = 0x03,
} T_HFP_CODEC_TYPE;

typedef enum t_bt_hfp_state
{
    HFP_STATE_DISCONNECTED          = 0x00,
    HFP_STATE_CONNECTING            = 0x01,
    HFP_STATE_RFC_CONN_NO_CREDITS   = 0x02,
    HFP_STATE_SRV_LEVEL_CONNECTING  = 0x03,
    HFP_STATE_CONNECTED             = 0x04,
    HSP_STATE_CONNECTED             = 0x05,
} T_BT_HFP_STATE;

typedef enum t_hfp_srv_level_step
{
    HFP_SRV_LEVEL_NONE               = 0x00,
    HFP_SRV_LEVEL_BRSF               = 0x01,
    HFP_SRV_LEVEL_BAC                = 0x02,
    HFP_SRV_LEVEL_CIND_TEST          = 0x03,
    HFP_SRV_LEVEL_CIND_INQUIRY       = 0x04,
    HFP_SRV_LEVEL_ACTIVATE_INDICATOR = 0x05,
    HFP_SRV_LEVEL_CHLD_TEST          = 0x06,
    HFP_SRV_LEVEL_BIND               = 0x07,
    HFP_SRV_LEVEL_BIND_TEST          = 0x08,
    HFP_SRV_LEVEL_BIND_INQUIRY       = 0x09,
    HFP_SRV_LEVEL_ESTABLISHED        = 0x0a,
} T_HFP_SRV_LEVEL_STEP;

typedef struct t_hfp_cmd_item
{
    struct t_hfp_cmd_item *p_next;
    uint8_t                cmd_id;
    uint8_t                cmd_len;
    const char             at_cmd[0];
} T_HFP_CMD_ITEM;

/* ..+CIND: ("service",(0-1)),("call",(0-1)),("callsetup",(0-3)),("battchg",(0-5)),("signal",(0-5)),("roam",(0-1)),("callheld",(0-2)).. */
typedef struct t_hfp_ag_status_ind
{
    T_HFP_SERVICE_STATUS    service_status;
    T_HFP_CALL_STATUS       call_status;
    T_HFP_CALL_SETUP_STATUS call_setup_status;
    T_HFP_CALL_HELD_STATUS  call_held_status;
    uint8_t                 signal_status;
    T_HFP_ROAM_STATUS       roam_status;
    uint8_t                 batt_chg_status;
} T_HFP_AG_STATUS_IND;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HFP_UTILS_H_ */
