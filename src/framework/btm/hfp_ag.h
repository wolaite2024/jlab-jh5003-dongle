/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _HFP_AG_H_
#define _HFP_AG_H_

#include <stdint.h>
#include <stdbool.h>

#include "hfp_utils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*Extended Audio Gateway Error Result Codes*/
#define HFP_AG_ERR_PHONE_FAILURE 0     /* AG Failure */
#define HFP_AG_ERR_NO_CONN_PHONE 1     /* No connection to phone */
#define HFP_AG_ERR_OP_NOT_ALLOWED 3    /* Operation not allowed */
#define HFP_AG_ERR_OP_NOT_SUPPORTED 4  /* Operation not supported */
#define HFP_AG_ERR_PHSIM_PIN_REQ 5     /* PH-SIM PIN required */
#define HFP_AG_ERR_SIM_NOT_INSERTED 10 /* SIM not inserted */
#define HFP_AG_ERR_SIM_PIN_REQ 11      /* SIM PIN required */
#define HFP_AG_ERR_SIM_PUK_REQ 12      /* SIM PUK required */
#define HFP_AG_ERR_SIM_FAILURE 13      /* SIM failure */
#define HFP_AG_ERR_SIM_BUSY 14         /* SIM busy */
#define HFP_AG_ERR_INCORRECT_PWD 16    /* Incorrect password */
#define HFP_AG_ERR_SIM_PIN2_REQ 17     /* SIM PIN2 required */
#define HFP_AG_ERR_SIM_PUK2_REQ 18     /* SIM PUK2 required */
#define HFP_AG_ERR_MEMORY_FULL 20      /* Memory full */
#define HFP_AG_ERR_INVALID_INDEX 21    /* Invalid index */
#define HFP_AG_ERR_MEMORY_FAILURE 23   /* Memory failure */
#define HFP_AG_ERR_TEXT_TOO_LONG 24    /* Text string too long */
#define HFP_AG_ERR_INV_CHAR_IN_TSTR 25 /* Invalid characters in text string */
#define HFP_AG_ERR_DSTR_TOO_LONG 26    /* Dial string too long */
#define HFP_AG_ERR_INV_CHAR_IN_DSTR 27 /* Invalid characters in dial string */
#define HFP_AG_ERR_NO_NETWORK_SERV 30  /* No network service */
#define HFP_AG_ERR_NETWORK_TIME_OUT 31 /* Network timeout */
#define HFP_AG_ERR_NO_NET_EMG_ONLY 32  /* Network not allowed - emergency service only */

#define HFP_AG_IND_SERVICE_MASK              (0x0001)
#define HFP_AG_IND_CALL_MASK                 (0x0002)
#define HFP_AG_IND_CALLSETUP_MASK            (0x0004)
#define HFP_AG_IND_CALLHELD_MASK             (0x0008)
#define HFP_AG_IND_SIGNAL_MASK               (0x0010)
#define HFP_AG_IND_ROAM_MASK                 (0x0020)
#define HFP_AG_IND_BATTCHG_MASK              (0x0040)
#define HFP_AG_IND_ALL_MASK                  (0x007f)

#define HFP_AG_CHLD_VAL "(0,1,2,3)"
#define HFP_HF_SUPPORTED_INDICATORS_MASK (0x03)
#define HFP_HF_SUPPORTED_XAPL_FEATURES_MASK  (0x02U)

/*  HFP HF indicator Mask.
    0 bit for Enhanced Safety Status
    1 bit for Battery Level Status
*/
typedef enum t_hfp_hf_ind_id
{
    HFP_HF_IND_ENHANCED_SAFETY_STATUS_ID = 0x01,
    HFP_HF_IND_BATTERY_LEVEL_STATUS_ID   = 0x02,
} T_HFP_HF_IND_ID;

/* XAPL FEATURES HF support.
   Bit 0 = reserved
   Bit 1 = The accessory supports battery reporting (reserved only for battery operated accessories).
   Bit 2 = The accessory is docked or powered (reserved only for battery operated accessories).
   Bit 3 = The accessory supports Siri status reporting.
   Bit 4 = the accessory supports noise reduction (NR) status reporting.
*/
typedef enum t_hfp_hf_xapl_id
{
    HFP_AG_HF_XAPL_RESEVED_ID        = 0,
    HFP_AG_HF_XAPL_BATTERY_STATUS_ID = 1,
    HFP_AG_HF_XAPL_DOCK_STATUS_ID    = 2,
    HFP_AG_HF_XAPL_SIRI_STATUS_ID    = 3,
    HFP_AG_HF_XAPL_NR_STATUS_ID      = 4,
} T_HFP_HF_XAPL_ID;

typedef enum t_hfp_hf_chld_id
{
    HFP_AG_HF_3WAY_HELD_CALL_RELEASED    = 0,
    HFP_AG_HF_3WAY_ACTIVE_CALL_RELEASED  = 1,
    HFP_AG_HF_3WAY_SWITCHED              = 2,
    HFP_AG_HF_3WAY_MERGED                = 3,
} T_HFP_HF_CHLD_ID;

typedef enum t_hfp_ag_msg
{
    HFP_AG_MSG_RFC_CONN_IND                                     = 0x00,
    HFP_AG_MSG_RFC_CONN                                         = 0x01,
    HFP_AG_MSG_CONN                                             = 0x02,
    HFP_AG_MSG_DISCONN                                          = 0x03,
    HFP_AG_MSG_VTS                                              = 0x04,
    HFP_AG_MSG_DIAL_WITH_NUMBER                                 = 0x05,
    HFP_AG_MSG_DIAL_WITH_MEMORY                                 = 0x06,
    HFP_AG_MSG_DIAL_LAST_NUMBER                                 = 0x07,
    HFP_AG_MSG_ACCEPT_CALL                                      = 0x08,
    HFP_AG_MSG_REJECT_HANGUP_CALL                               = 0x09,
    HFP_AG_MSG_ACTIVATE_VOICE_RECOGNITION                       = 0x0a,
    HFP_AG_MSG_DEACTIVATE_VOICE_RECOGNITION                     = 0x0b,
    HFP_AG_MSG_INFORM_MICROPHONE_GAIN                           = 0x0c,
    HFP_AG_MSG_INFORM_SPEAKER_GAIN                              = 0x0d,
    HFP_AG_MSG_HSP_BUTTON_PRESS                                 = 0x0e,
    HFP_AG_MSG_CLCC                                             = 0x0f,
    HFP_AG_MSG_CODEC_NEGOTIATE                                  = 0x10,
    HFP_AG_MSG_BIEV                                             = 0x11,
    HFP_AG_MSG_IPHONEACCEV                                      = 0x12,
    HFP_AG_MSG_XEVENT                                           = 0x13,
    HFP_AG_MSG_NREC                                             = 0x14,
    HFP_AG_MSG_CIND                                             = 0x15,
    HFP_AG_MSG_CODEC_TYPE_SELECT                                = 0x16,
    HFP_AG_MSG_BAC_SET                                          = 0x17,
    HFP_AG_MSG_CHLD                                             = 0x18,
    HFP_AG_MSG_CNUM                                             = 0x19,
    HFP_AG_MSG_COPS_SET                                         = 0x1a,
    HFP_AG_MSG_COPS_QUERY                                       = 0x1b,
    HFP_AG_MSG_UNKNOWN_CMD                                      = 0x1c,
} T_HFP_AG_MSG;

typedef struct t_hfp_ag_msg_rfc_conn
{
    uint8_t bd_addr[6];
    bool    hsp;
} T_HFP_AG_MSG_RFC_CONN;

typedef struct t_hfp_ag_msg_conn
{
    uint8_t bd_addr[6];
} T_HFP_AG_MSG_CONN;

typedef struct t_hfp_ag_disconn_info
{
    uint16_t cause;
} T_HFP_AG_DISCONN_INFO;

typedef struct t_hfp_ag_msg_mic_volume_changed
{
    uint8_t bd_addr[6];
    uint8_t volume;
} T_HFP_AG_MSG_MICROPHONE_VOLUME_CHANGED;

typedef struct t_hfp_ag_msg_speaker_volume_changed
{
    uint8_t bd_addr[6];
    uint8_t volume;
} T_HFP_AG_MSG_SPEAKER_VOLUME_CHANGED;

typedef struct t_hfp_ag_msg_hf_xevent
{
    uint8_t bd_addr[6];
    uint8_t battery_level;
} T_HFP_AG_MSG_HF_XEVENT;

typedef struct t_hfp_ag_msg_hf_xapl
{
    uint8_t bd_addr[6];
    uint8_t xapl_id;
    uint8_t xapl_value;
} T_HFP_AG_MSG_HF_XAPL;

typedef struct t_hfp_ag_msg_hf_ind
{
    uint8_t  bd_addr[6];
    uint16_t ind_id;
    uint32_t ind_value;
} T_HFP_AG_MSG_HF_IND;

typedef struct t_hfp_ag_msg_codec_type_selected
{
    uint8_t          bd_addr[6];
    T_HFP_CODEC_TYPE codec_type;
} T_HFP_AG_MSG_CODEC_TYPE_SELECTED;

typedef struct t_hfp_ag_msg_dial_with_number
{
    uint8_t bd_addr[6];
    char    number[20];
} T_HFP_AG_MSG_DIAL_WITH_NUMBER;

typedef struct t_hfp_ag_msg_dial_last_number
{
    uint8_t bd_addr[6];
    uint8_t num;
} T_HFP_AG_MSG_DIAL_WITH_MEMORY;

typedef struct t_hfp_ag_msg_chld
{
    uint8_t bd_addr[6];
    uint8_t value;
} T_HFP_AG_MSG_CHLD;

typedef struct t_hfp_ag_msg_nrec
{
    uint8_t bd_addr[6];
    uint8_t value;
} T_HFP_AG_MSG_NREC;

typedef struct t_hfp_ag_msg_call_dtmf_code
{
    uint8_t  bd_addr[6];
    char     dtmf_code;
} T_HFP_AG_MSG_CALL_DTMF_CODE;

typedef void (*P_HFP_AG_CBACK)(uint8_t       bd_addr[6],
                               T_HFP_AG_MSG  msg_type,
                               void         *msg_buf);

bool hfp_ag_init(uint8_t        link_num,
                 uint8_t        rfc_hfp_chann_num,
                 uint8_t        rfc_hsp_chann_num,
                 P_HFP_AG_CBACK cback,
                 uint16_t       hfp_ag_brsf_cpbs,
                 uint16_t       voice_codec);

bool hfp_ag_connect_req(uint8_t bd_addr[6],
                        uint8_t remote_dlci,
                        uint8_t hfp_ag_flag);

bool hfp_ag_disconnect_req(uint8_t bd_addr[6]);

bool hfp_ag_connect_cfm(uint8_t bd_addr[6],
                        bool    accept);

bool hfp_ag_remote_capabilities_get(uint8_t   bd_addr[6],
                                    uint16_t *cpbs);

bool hfp_ag_local_capabilities_get(uint16_t *cpbs);

bool hfp_ag_codec_negotiate(uint8_t  bd_addr[6],
                            bool     fallback,
                            uint8_t *codec_type_sent);

bool hfp_ag_send_ok(uint8_t bd_addr[6]);

bool hfp_ag_send_error(uint8_t bd_addr[6],
                       uint8_t error_code);

bool hfp_ag_send_ciev(uint8_t bd_addr[6],
                      uint8_t ind_id,
                      uint8_t ind_value);

bool hfp_ag_send_ring(uint8_t bd_addr[6]);

bool hfp_ag_send_clip(uint8_t     bd_addr[6],
                      const char *call_num,
                      uint8_t     call_num_type);

bool hfp_ag_send_cind(uint8_t                 bd_addr[6],
                      T_HFP_SERVICE_STATUS    service_status,
                      T_HFP_CALL_STATUS       call_status,
                      T_HFP_CALL_SETUP_STATUS call_setup_status,
                      T_HFP_CALL_HELD_STATUS  call_held_status,
                      uint8_t                 signal_status,
                      T_HFP_ROAM_STATUS       roam_status,
                      uint8_t                 batt_chg_status);

bool hfp_ag_send_vgm(uint8_t bd_addr[6],
                     uint8_t level);

bool hfp_ag_send_vgs(uint8_t bd_addr[6],
                     uint8_t level);

bool hfp_ag_send_ccwa(uint8_t     bd_addr[6],
                      const char *call_num,
                      uint8_t     call_num_type);

bool hfp_ag_send_cops(uint8_t     bd_addr[6],
                      const char *operator_name);

bool hfp_ag_send_cnum(uint8_t     bd_addr[6],
                      const char *call_num,
                      uint8_t     call_num_type,
                      uint8_t     service);

bool hfp_ag_try_general_at_cmd(uint8_t     bd_addr[6],
                               const char *at_cmd);

bool hfp_ag_inband_ringing_set(uint8_t bd_addr[6],
                               bool    enable);

bool hfp_ag_current_calls_list_send(uint8_t     bd_addr[6],
                                    uint8_t     call_idx,
                                    uint8_t     call_dir,
                                    uint8_t     call_status,
                                    uint8_t     call_mode,
                                    uint8_t     mpty,
                                    const char *call_num,
                                    uint8_t     call_num_type);

bool hfp_ag_send_bvra(uint8_t bd_addr[6],
                      bool    enable);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HFP_AG_H_ */
