/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_MGR_INT_H_
#define _BT_MGR_INT_H_

#include <stdint.h>
#include <stdbool.h>
#include "remote.h"
#include "bt_mgr.h"
#include "btm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BT_PM_TIMEOUT_REMOTE    10000000 /* microseconds */
#define BT_PM_TIMEOUT           30000000 /* microseconds */

#define BT_SNIFF_INTERVAL_REMOTE    752 /* unit in 0.625ms */
#define BT_SNIFF_INTERVAL_BASE      800 /* unit in 0.625ms */
#define BT_SNIFF_INTERVAL_OFFSET    32  /* unit in 0.625ms */
#define BT_SNIFF_ATTEMPT            4   /* unit in 1.25ms */
#define BT_SNIFF_TIMEOUT            4   /* unit in 1.25ms */

typedef enum t_bt_msg_group
{
    BT_MSG_GROUP_GAP         = 0x0000,
    BT_MSG_GROUP_ACL         = 0x0100,
    BT_MSG_GROUP_SCO         = 0x0200,
    BT_MSG_GROUP_A2DP        = 0x0300,
    BT_MSG_GROUP_AVRCP       = 0x0400,
    BT_MSG_GROUP_HFP         = 0x0500,
    BT_MSG_GROUP_HSP         = 0x0600,
    BT_MSG_GROUP_SPP         = 0x0700,
    BT_MSG_GROUP_IAP         = 0x0800,
    BT_MSG_GROUP_PBAP        = 0x0900,
    BT_MSG_GROUP_HID         = 0x0a00,
    BT_MSG_GROUP_MAP         = 0x0b00,
    BT_MSG_GROUP_OPP         = 0x0c00,
    BT_MSG_GROUP_RDTP        = 0x0d00,
    BT_MSG_GROUP_ROLESWAP    = 0x0e00,
    BT_MSG_GROUP_SNIFFING    = 0x0f00,
    BT_MSG_GROUP_VND         = 0x1000,
} T_BT_MSG_GROUP;

typedef enum t_bt_msg
{
    BT_MSG_GAP_BT_READY                          = BT_MSG_GROUP_GAP | 0x00,
    BT_MSG_GAP_RADIO_MODE_RSP                    = BT_MSG_GROUP_GAP | 0x01,
    BT_MSG_GAP_LOCAL_ADDR_CHANGED                = BT_MSG_GROUP_GAP | 0x02,
    BT_MSG_GAP_LOCAL_NAME_CHANGED                = BT_MSG_GROUP_GAP | 0x03,
    BT_MSG_GAP_REMOTE_NAME_RSP                   = BT_MSG_GROUP_GAP | 0x04,
    BT_MSG_GAP_ADD_SDP_RECORD_RSP                = BT_MSG_GROUP_GAP | 0x05,
    BT_MSG_GAP_DEL_SDP_RECORD_RSP                = BT_MSG_GROUP_GAP | 0x06,
    BT_MSG_GAP_SDP_ATTR_INFO                     = BT_MSG_GROUP_GAP | 0x07,
    BT_MSG_GAP_DID_ATTR_INFO                     = BT_MSG_GROUP_GAP | 0x08,
    BT_MSG_GAP_SDP_DISCOV_CMPL                   = BT_MSG_GROUP_GAP | 0x09,
    BT_MSG_GAP_SDP_DISCOV_STOP                   = BT_MSG_GROUP_GAP | 0x0a,
    BT_MSG_GAP_INQUIRY_RESULT_INFO               = BT_MSG_GROUP_GAP | 0x0b,
    BT_MSG_GAP_INQUIRY_RSP                       = BT_MSG_GROUP_GAP | 0x0c,
    BT_MSG_GAP_INQUIRY_CMPL                      = BT_MSG_GROUP_GAP | 0x0d,
    BT_MSG_GAP_INQUIRY_CANCEL_RSP                = BT_MSG_GROUP_GAP | 0x0e,
    BT_MSG_GAP_PERIODIC_INQUIRY_RSP              = BT_MSG_GROUP_GAP | 0x0f,
    BT_MSG_GAP_PERIODIC_INQUIRY_CANCEL_RSP       = BT_MSG_GROUP_GAP | 0x10,
    BT_MSG_GAP_LOCAL_OOB_DATA_RSP                = BT_MSG_GROUP_GAP | 0x11,
    BT_MSG_GAP_LOCAL_OOB_EXTENDED_DATA_RSP       = BT_MSG_GROUP_GAP | 0x12,

    BT_MSG_ACL_CONN_IND             = BT_MSG_GROUP_ACL | 0x00,
    BT_MSG_ACL_CONN_SUCCESS         = BT_MSG_GROUP_ACL | 0x01,
    BT_MSG_ACL_CONN_FAIL            = BT_MSG_GROUP_ACL | 0x02,
    BT_MSG_ACL_CONN_DISCONN         = BT_MSG_GROUP_ACL | 0x03,
    BT_MSG_ACL_CONN_READY           = BT_MSG_GROUP_ACL | 0x04,
    BT_MSG_ACL_AUTHEN_START         = BT_MSG_GROUP_ACL | 0x05,
    BT_MSG_ACL_AUTHEN_FAIL          = BT_MSG_GROUP_ACL | 0x06,
    BT_MSG_ACL_AUTHEN_SUCCESS       = BT_MSG_GROUP_ACL | 0x07,
    BT_MSG_ACL_CONN_BR_SC_START     = BT_MSG_GROUP_ACL | 0x08,
    BT_MSG_ACL_CONN_BR_SC_CMPL      = BT_MSG_GROUP_ACL | 0x09,
    BT_MSG_ACL_CONN_ENCRYPTED       = BT_MSG_GROUP_ACL | 0x0a,
    BT_MSG_ACL_CONN_NOT_ENCRYPTED   = BT_MSG_GROUP_ACL | 0x0b,
    BT_MSG_ACL_ROLE_MASTER          = BT_MSG_GROUP_ACL | 0x0c,
    BT_MSG_ACL_ROLE_SLAVE           = BT_MSG_GROUP_ACL | 0x0d,
    BT_MSG_ACL_ROLE_SWITCH_FAIL     = BT_MSG_GROUP_ACL | 0x0e,
    BT_MSG_ACL_CONN_ACTIVE          = BT_MSG_GROUP_ACL | 0x0f,
    BT_MSG_ACL_CONN_SNIFF           = BT_MSG_GROUP_ACL | 0x10,
    BT_MSG_ACL_SNIFF_ENTER_FAIL     = BT_MSG_GROUP_ACL | 0x11,
    BT_MSG_ACL_SNIFF_EXIT_FAIL      = BT_MSG_GROUP_ACL | 0x12,
    BT_MSG_ACL_LINK_KEY_INFO        = BT_MSG_GROUP_ACL | 0x13,
    BT_MSG_ACL_LINK_KEY_REQ_IND     = BT_MSG_GROUP_ACL | 0x14,
    BT_MSG_ACL_PIN_CODE_REQ_IND     = BT_MSG_GROUP_ACL | 0x15,
    BT_MSG_ACL_USER_CONFIRM_REQ     = BT_MSG_GROUP_ACL | 0x16,
    BT_MSG_ACL_USER_PASSKEY_REQ_IND = BT_MSG_GROUP_ACL | 0x17,
    BT_MSG_ACL_USER_PASSKEY_NOTIF   = BT_MSG_GROUP_ACL | 0x18,
    BT_MSG_ACL_READ_RSSI_RSP        = BT_MSG_GROUP_ACL | 0x19,
    BT_MSG_ACL_SET_QOS_RSP          = BT_MSG_GROUP_ACL | 0x1a,
    BT_MSG_ACL_SET_QOS_CMPL         = BT_MSG_GROUP_ACL | 0x1b,
    BT_MSG_ACL_LINK_POLICY_RSP      = BT_MSG_GROUP_ACL | 0x1c,

    BT_MSG_SCO_CONN_IND             = BT_MSG_GROUP_SCO | 0x00,
    BT_MSG_SCO_CONN_RSP             = BT_MSG_GROUP_SCO | 0x01,
    BT_MSG_SCO_CONN_CMPL            = BT_MSG_GROUP_SCO | 0x02,
    BT_MSG_SCO_DISCONN_CMPL         = BT_MSG_GROUP_SCO | 0x03,
    BT_MSG_SCO_DATA_IND             = BT_MSG_GROUP_SCO | 0x04,

    BT_MSG_A2DP_CONN_IND             = BT_MSG_GROUP_A2DP | 0x00,
    BT_MSG_A2DP_CONN_CMPL            = BT_MSG_GROUP_A2DP | 0x01,
    BT_MSG_A2DP_CONN_FAIL            = BT_MSG_GROUP_A2DP | 0x02,
    BT_MSG_A2DP_DISCONN_CMPL         = BT_MSG_GROUP_A2DP | 0x03,
    BT_MSG_A2DP_CONFIG_CMPL          = BT_MSG_GROUP_A2DP | 0x04,
    BT_MSG_A2DP_DELAY_REPORT         = BT_MSG_GROUP_A2DP | 0x05,
    BT_MSG_A2DP_STREAM_START_IND     = BT_MSG_GROUP_A2DP | 0x06,
    BT_MSG_A2DP_STREAM_START_RSP     = BT_MSG_GROUP_A2DP | 0x07,
    BT_MSG_A2DP_STREAM_OPEN          = BT_MSG_GROUP_A2DP | 0x08,
    BT_MSG_A2DP_STREAM_OPEN_FAIL     = BT_MSG_GROUP_A2DP | 0x09,
    BT_MSG_A2DP_STREAM_STOP          = BT_MSG_GROUP_A2DP | 0x0a,
    BT_MSG_A2DP_STREAM_CLOSE         = BT_MSG_GROUP_A2DP | 0x0b,
    BT_MSG_A2DP_STREAM_DATA_IND      = BT_MSG_GROUP_A2DP | 0x0c,
    BT_MSG_A2DP_STREAM_DATA_RSP      = BT_MSG_GROUP_A2DP | 0x0d,


    BT_MSG_AVRCP_CONN_IND                        = BT_MSG_GROUP_AVRCP | 0x00,
    BT_MSG_AVRCP_CONN_CMPL                       = BT_MSG_GROUP_AVRCP | 0x01,
    BT_MSG_AVRCP_CONN_FAIL                       = BT_MSG_GROUP_AVRCP | 0x02,
    BT_MSG_AVRCP_DISCONN_CMPL                    = BT_MSG_GROUP_AVRCP | 0x03,
    BT_MSG_AVRCP_GET_CAPABILITIES_RSP            = BT_MSG_GROUP_AVRCP | 0x04,
    BT_MSG_AVRCP_ABSOLUTE_VOLUME_SET             = BT_MSG_GROUP_AVRCP | 0x05,
    BT_MSG_AVRCP_KEY_VOLUME_UP                   = BT_MSG_GROUP_AVRCP | 0x06,
    BT_MSG_AVRCP_KEY_VOLUME_DOWN                 = BT_MSG_GROUP_AVRCP | 0x07,
    BT_MSG_AVRCP_REG_VOL_CHANGE                  = BT_MSG_GROUP_AVRCP | 0x08,
    BT_MSG_AVRCP_PLAY_STATUS_CHANGED             = BT_MSG_GROUP_AVRCP | 0x09,
    BT_MSG_AVRCP_PLAY_STATUS_RSP                 = BT_MSG_GROUP_AVRCP | 0x0a,
    BT_MSG_AVRCP_TRACK_CHANGED                   = BT_MSG_GROUP_AVRCP | 0x0b,
    BT_MSG_AVRCP_TRACK_REACHED_END               = BT_MSG_GROUP_AVRCP | 0x0c,
    BT_MSG_AVRCP_TRACK_REACHED_START             = BT_MSG_GROUP_AVRCP | 0x0d,
    BT_MSG_AVRCP_PLAYBACK_POS_CHANGED            = BT_MSG_GROUP_AVRCP | 0x0e,
    BT_MSG_AVRCP_BATT_STATUS_CHANGED             = BT_MSG_GROUP_AVRCP | 0x0f,
    BT_MSG_AVRCP_SYSTEM_STATUS_CHANGED           = BT_MSG_GROUP_AVRCP | 0x10,
    BT_MSG_AVRCP_APP_SETTING_CHANGED             = BT_MSG_GROUP_AVRCP | 0x11,
    BT_MSG_AVRCP_PLAYING_CONTENT_CHANGED         = BT_MSG_GROUP_AVRCP | 0x12,
    BT_MSG_AVRCP_AVAILABLE_PLAYER_CHANGED        = BT_MSG_GROUP_AVRCP | 0x13,
    BT_MSG_AVRCP_ADDRESSED_PLAYER_CHANGED        = BT_MSG_GROUP_AVRCP | 0x14,
    BT_MSG_AVRCP_UIDS_CHANGED                    = BT_MSG_GROUP_AVRCP | 0x15,
    BT_MSG_AVRCP_GET_PLAY_STATUS_RSP             = BT_MSG_GROUP_AVRCP | 0x16,
    BT_MSG_AVRCP_GET_ELEM_ATTR_RSP               = BT_MSG_GROUP_AVRCP | 0x17,
    BT_MSG_AVRCP_APP_SETTING_ATTRS               = BT_MSG_GROUP_AVRCP | 0x18,
    BT_MSG_AVRCP_APP_SETTING_VALUES              = BT_MSG_GROUP_AVRCP | 0x19,
    BT_MSG_AVRCP_CUR_APP_SETTING_VALUE           = BT_MSG_GROUP_AVRCP | 0x1a,
    BT_MSG_AVRCP_SET_ADDRESSED_PLAYER_RSP        = BT_MSG_GROUP_AVRCP | 0x1b,
    BT_MSG_AVRCP_PLAY_ITEM                       = BT_MSG_GROUP_AVRCP | 0x1c,
    BT_MSG_AVRCP_FOLDER_ITEMS                    = BT_MSG_GROUP_AVRCP | 0x1d,
    BT_MSG_AVRCP_ITEM_ATTR                       = BT_MSG_GROUP_AVRCP | 0x1e,
    BT_MSG_AVRCP_SEARCH                          = BT_MSG_GROUP_AVRCP | 0x1f,
    BT_MSG_AVRCP_SET_BROWSED_PLAYER              = BT_MSG_GROUP_AVRCP | 0x20,
    BT_MSG_AVRCP_CHANGE_PATH                     = BT_MSG_GROUP_AVRCP | 0x21,
    BT_MSG_AVRCP_SET_ABSOLUTE_VOLUME_RSP         = BT_MSG_GROUP_AVRCP | 0x22,
    BT_MSG_AVRCP_BROWSING_CONN_IND               = BT_MSG_GROUP_AVRCP | 0x23,
    BT_MSG_AVRCP_BROWSING_CONN_CMPL              = BT_MSG_GROUP_AVRCP | 0x24,
    BT_MSG_AVRCP_BROWSING_DISCONN_CMPL           = BT_MSG_GROUP_AVRCP | 0x25,
    BT_MSG_AVRCP_COVER_ART_CONN_CMPL             = BT_MSG_GROUP_AVRCP | 0x26,
    BT_MSG_AVRCP_COVER_ART_DISCONN_CMPL          = BT_MSG_GROUP_AVRCP | 0x27,
    BT_MSG_AVRCP_COVER_ART_DATA_IND              = BT_MSG_GROUP_AVRCP | 0x28,
    BT_MSG_AVRCP_VENDOR_CMD_IND                  = BT_MSG_GROUP_AVRCP | 0x30,
    BT_MSG_AVRCP_VENDOR_RSP                      = BT_MSG_GROUP_AVRCP | 0x31,
    BT_MSG_AVRCP_VOLUME_CHANGED                  = BT_MSG_GROUP_AVRCP | 0x80,
    BT_MSG_AVRCP_REG_PLAY_STATUS_CHANGE          = BT_MSG_GROUP_AVRCP | 0x81,
    BT_MSG_AVRCP_PLAY                            = BT_MSG_GROUP_AVRCP | 0x82,
    BT_MSG_AVRCP_STOP                            = BT_MSG_GROUP_AVRCP | 0x83,
    BT_MSG_AVRCP_PAUSE                           = BT_MSG_GROUP_AVRCP | 0x84,
    BT_MSG_AVRCP_REWIND_START                    = BT_MSG_GROUP_AVRCP | 0x85,
    BT_MSG_AVRCP_REWIND_STOP                     = BT_MSG_GROUP_AVRCP | 0x86,
    BT_MSG_AVRCP_FAST_FORWARD_START              = BT_MSG_GROUP_AVRCP | 0x87,
    BT_MSG_AVRCP_FAST_FORWARD_STOP               = BT_MSG_GROUP_AVRCP | 0x88,
    BT_MSG_AVRCP_FORWARD                         = BT_MSG_GROUP_AVRCP | 0x89,
    BT_MSG_AVRCP_BACKWARD                        = BT_MSG_GROUP_AVRCP | 0x8a,
    BT_MSG_AVRCP_REG_TRACK_CHANGE                = BT_MSG_GROUP_AVRCP | 0x8b,
    BT_MSG_AVRCP_GET_ELEM_ATTRS                  = BT_MSG_GROUP_AVRCP | 0x8c,
    BT_MSG_AVRCP_GET_PLAY_STATUS                 = BT_MSG_GROUP_AVRCP | 0x8d,
    BT_MSG_AVRCP_SET_ADDRESSED_PLAYER            = BT_MSG_GROUP_AVRCP | 0x8e,
    BT_MSG_AVRCP_REG_ADDRESSED_PLAYER_CHANGE     = BT_MSG_GROUP_AVRCP | 0x8f,
    BT_MSG_AVRCP_GET_FOLDER_ITEMS                = BT_MSG_GROUP_AVRCP | 0x90,
    BT_MSG_AVRCP_GET_TOTAL_NUM_OF_ITEMS          = BT_MSG_GROUP_AVRCP | 0x91,
    BT_MSG_AVRCP_POWER                           = BT_MSG_GROUP_AVRCP | 0x92,
    BT_MSG_AVRCP_MUTE                            = BT_MSG_GROUP_AVRCP | 0x93,

    BT_MSG_HFP_CONN_IND                          = BT_MSG_GROUP_HFP | 0x00,
    BT_MSG_HFP_CONN_CMPL                         = BT_MSG_GROUP_HFP | 0x01,
    BT_MSG_HFP_CONN_FAIL                         = BT_MSG_GROUP_HFP | 0x02,
    BT_MSG_HFP_DISCONN_CMPL                      = BT_MSG_GROUP_HFP | 0x03,
    BT_MSG_HFP_CALL_STATUS_CHANGED               = BT_MSG_GROUP_HFP | 0x04,
    BT_MSG_HFP_SERVICE_STATUS_CHANGED            = BT_MSG_GROUP_HFP | 0x05,
    BT_MSG_HFP_VND_AT_CMD_RSP                    = BT_MSG_GROUP_HFP | 0x06,
    BT_MSG_HFP_CALLER_ID_IND                     = BT_MSG_GROUP_HFP | 0x07,
    BT_MSG_HFP_RING_ALERT                        = BT_MSG_GROUP_HFP | 0x08,
    BT_MSG_HFP_SPK_GAIN_CHANGED                  = BT_MSG_GROUP_HFP | 0x09,
    BT_MSG_HFP_MIC_GAIN_CHANGED                  = BT_MSG_GROUP_HFP | 0x0a,
    BT_MSG_HFP_NETWORK_OPERATOR_IND              = BT_MSG_GROUP_HFP | 0x0b,
    BT_MSG_HFP_SUBSCRIBER_NUMBER_IND             = BT_MSG_GROUP_HFP | 0x0c,
    BT_MSG_HFP_CURRENT_CALL_LIST_IND             = BT_MSG_GROUP_HFP | 0x0d,
    BT_MSG_HFP_CALL_WAITING_IND                  = BT_MSG_GROUP_HFP | 0x0e,
    BT_MSG_HFP_SIGNAL_IND                        = BT_MSG_GROUP_HFP | 0x0f,
    BT_MSG_HFP_ROAM_IND                          = BT_MSG_GROUP_HFP | 0x10,
    BT_MSG_HFP_BATTERY_IND                       = BT_MSG_GROUP_HFP | 0x11,
    BT_MSG_HFP_SUPPORTED_FEATURES_IND            = BT_MSG_GROUP_HFP | 0x12,
    BT_MSG_HFP_VENDOR_CMD                        = BT_MSG_GROUP_HFP | 0x13,
    BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION      = BT_MSG_GROUP_HFP | 0x14,
    BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION    = BT_MSG_GROUP_HFP | 0x15,
    BT_MSG_HFP_DIAL_WITH_NUMBER_RESULT           = BT_MSG_GROUP_HFP | 0x16,
    BT_MSG_HFP_CODEC_TYPE_SELECTED               = BT_MSG_GROUP_HFP | 0x17,
    BT_MSG_HFP_AG_CONN_IND                       = BT_MSG_GROUP_HFP | 0x80,
    BT_MSG_HFP_AG_CONN_CMPL                      = BT_MSG_GROUP_HFP | 0x81,
    BT_MSG_HFP_AG_DISCONN_CMPL                   = BT_MSG_GROUP_HFP | 0x82,
    BT_MSG_HFP_AG_CALL_STATUS_CHANGED            = BT_MSG_GROUP_HFP | 0x83,
    BT_MSG_HFP_AG_MIC_VOLUME_CHANGED             = BT_MSG_GROUP_HFP | 0x84,
    BT_MSG_HFP_AG_SPK_VOLUME_CHANGED             = BT_MSG_GROUP_HFP | 0x85,
    BT_MSG_HFP_AG_INDICATORS_STATUS_REQ          = BT_MSG_GROUP_HFP | 0x86,
    BT_MSG_HFP_AG_BATTERY_LEVEL                  = BT_MSG_GROUP_HFP | 0x87,
    BT_MSG_HFP_AG_SUPPORTED_FEATURES             = BT_MSG_GROUP_HFP | 0x88,
    BT_MSG_HFP_AG_INBAND_RINGING_REQ             = BT_MSG_GROUP_HFP | 0x89,
    BT_MSG_HFP_AG_CALL_ANSWER_REQ                = BT_MSG_GROUP_HFP | 0x8a,
    BT_MSG_HFP_AG_CALL_TERMINATE_REQ             = BT_MSG_GROUP_HFP | 0x8b,
    BT_MSG_HFP_AG_CURR_CALLS_LIST_QUERY          = BT_MSG_GROUP_HFP | 0x8c,
    BT_MSG_HFP_AG_3WAY_HELD_CALL_RELEASED        = BT_MSG_GROUP_HFP | 0x8d,
    BT_MSG_HFP_AG_3WAY_ACTIVE_CALL_RELEASED      = BT_MSG_GROUP_HFP | 0x8e,
    BT_MSG_HFP_AG_3WAY_SWITCHED                  = BT_MSG_GROUP_HFP | 0x9f,
    BT_MSG_HFP_AG_3WAY_MERGED                    = BT_MSG_GROUP_HFP | 0x90,
    BT_MSG_HFP_AG_SUBSCRIBER_NUMBER_QUERY        = BT_MSG_GROUP_HFP | 0x91,
    BT_MSG_HFP_AG_NETWORK_NAME_FORMAT_SET        = BT_MSG_GROUP_HFP | 0x92,
    BT_MSG_HFP_AG_CURR_OPERATOR_QUERY            = BT_MSG_GROUP_HFP | 0x93,
    BT_MSG_HFP_AG_MSG_DTMF_CODE                  = BT_MSG_GROUP_HFP | 0x94,
    BT_MSG_HFP_AG_MSG_DIAL_WITH_NUMBER           = BT_MSG_GROUP_HFP | 0x95,
    BT_MSG_HFP_AG_MSG_DIAL_WITH_MEMORY           = BT_MSG_GROUP_HFP | 0x96,
    BT_MSG_HFP_AG_MSG_DIAL_LAST_NUMBER           = BT_MSG_GROUP_HFP | 0x97,
    BT_MSG_HFP_AG_ENHANCED_SAFETY_STATUS         = BT_MSG_GROUP_HFP | 0x98,
    BT_MSG_HFP_AG_NREC_STATUS                    = BT_MSG_GROUP_HFP | 0x99,
    BT_MSG_HFP_AG_VOICE_RECOGNITION_ACTIVATION   = BT_MSG_GROUP_HFP | 0x9a,
    BT_MSG_HFP_AG_VOICE_RECOGNITION_DEACTIVATION = BT_MSG_GROUP_HFP | 0x9b,
    BT_MSG_HFP_AG_VENDOR_CMD                     = BT_MSG_GROUP_HFP | 0x9c,
    BT_MSG_HFP_AG_CODEC_TYPE_SELECTED            = BT_MSG_GROUP_HFP | 0x9d,

    BT_MSG_HSP_CONN_CMPL                    = BT_MSG_GROUP_HSP | 0x00,
    BT_MSG_HSP_CONN_FAIL                    = BT_MSG_GROUP_HSP | 0x01,
    BT_MSG_HSP_DISCONN_CMPL                 = BT_MSG_GROUP_HSP | 0x02,
    BT_MSG_HSP_AG_CONN_CMPL                 = BT_MSG_GROUP_HSP | 0x03,
    BT_MSG_HSP_AG_DISCONN_CMPL              = BT_MSG_GROUP_HSP | 0x04,
    BT_MSG_HSP_AG_INBAND_RINGING_REQ        = BT_MSG_GROUP_HSP | 0x05,
    BT_MSG_HSP_AG_BUTTON_PRESS              = BT_MSG_GROUP_HSP | 0x06,

    BT_MSG_SPP_CONN_IND                     = BT_MSG_GROUP_SPP | 0x00,
    BT_MSG_SPP_CONN_CMPL                    = BT_MSG_GROUP_SPP | 0x01,
    BT_MSG_SPP_CONN_FAIL                    = BT_MSG_GROUP_SPP | 0x02,
    BT_MSG_SPP_DISCONN_CMPL                 = BT_MSG_GROUP_SPP | 0x03,
    BT_MSG_SPP_DATA_IND                     = BT_MSG_GROUP_SPP | 0x04,
    BT_MSG_SPP_DATA_RSP                     = BT_MSG_GROUP_SPP | 0x05,
    BT_MSG_SPP_CREDIT_INFO                  = BT_MSG_GROUP_SPP | 0x06,

    BT_MSG_IAP_CONN_IND                     = BT_MSG_GROUP_IAP | 0x00,
    BT_MSG_IAP_CONN_CMPL                    = BT_MSG_GROUP_IAP | 0x01,
    BT_MSG_IAP_CONN_FAIL                    = BT_MSG_GROUP_IAP | 0x02,
    BT_MSG_IAP_DISCONN_CMPL                 = BT_MSG_GROUP_IAP | 0x03,
    BT_MSG_IAP_START_IDENT_REQ              = BT_MSG_GROUP_IAP | 0x04,
    BT_MSG_IAP_AUTHEN_SUCCESS               = BT_MSG_GROUP_IAP | 0x05,
    BT_MSG_IAP_AUTHEN_FAIL                  = BT_MSG_GROUP_IAP | 0x06,
    BT_MSG_IAP_DATA_IND                     = BT_MSG_GROUP_IAP | 0x07,
    BT_MSG_IAP_START_EAP_SESSION            = BT_MSG_GROUP_IAP | 0x08,
    BT_MSG_IAP_STOP_EAP_SESSION             = BT_MSG_GROUP_IAP | 0x09,
    BT_MSG_IAP_EAP_SESSION_STATUS           = BT_MSG_GROUP_IAP | 0x0a,
    BT_MSG_IAP_DATA_TRANSMITTED             = BT_MSG_GROUP_IAP | 0x0b,
    BT_MSG_IAP_RESET                        = BT_MSG_GROUP_IAP | 0x0c,
    BT_MSG_IAP_CTRL_MSG_IND                 = BT_MSG_GROUP_IAP | 0x0d,

    BT_MSG_PBAP_CONN_CMPL                   = BT_MSG_GROUP_PBAP | 0x00,
    BT_MSG_PBAP_CONN_FAIL                   = BT_MSG_GROUP_PBAP | 0x01,
    BT_MSG_PBAP_DISCONN_CMPL                = BT_MSG_GROUP_PBAP | 0x02,
    BT_MSG_PBAP_SET_PHONE_BOOK_CMPL         = BT_MSG_GROUP_PBAP | 0x03,
    BT_MSG_PBAP_GET_PHONE_BOOK_CMPL         = BT_MSG_GROUP_PBAP | 0x04,
    BT_MSG_PBAP_GET_PHONE_BOOK_SIZE_CMPL    = BT_MSG_GROUP_PBAP | 0x05,
    BT_MSG_PBAP_CALLER_ID_NAME              = BT_MSG_GROUP_PBAP | 0x06,
    BT_MSG_PBAP_GET_VCARD_ENTRY_CMPL        = BT_MSG_GROUP_PBAP | 0x07,

    BT_MSG_HID_DEVICE_CONN_IND              = BT_MSG_GROUP_HID | 0x00,
    BT_MSG_HID_DEVICE_CONN_CMPL             = BT_MSG_GROUP_HID | 0x01,
    BT_MSG_HID_DEVICE_CONN_FAIL             = BT_MSG_GROUP_HID | 0x02,
    BT_MSG_HID_DEVICE_DISCONN_CMPL          = BT_MSG_GROUP_HID | 0x03,
    BT_MSG_HID_DEVICE_CONTROL_DATA_IND      = BT_MSG_GROUP_HID | 0x04,
    BT_MSG_HID_DEVICE_GET_REPORT_IND        = BT_MSG_GROUP_HID | 0x05,
    BT_MSG_HID_DEVICE_SET_REPORT_IND        = BT_MSG_GROUP_HID | 0x06,
    BT_MSG_HID_DEVICE_GET_PROTOCOL_IND      = BT_MSG_GROUP_HID | 0x07,
    BT_MSG_HID_DEVICE_SET_PROTOCOL_IND      = BT_MSG_GROUP_HID | 0x08,
    BT_MSG_HID_DEVICE_SET_IDLE_IND          = BT_MSG_GROUP_HID | 0x09,
    BT_MSG_HID_DEVICE_INTERRUPT_DATA_IND    = BT_MSG_GROUP_HID | 0x0a,
    BT_MSG_HID_HOST_CONN_IND                = BT_MSG_GROUP_HID | 0x80,
    BT_MSG_HID_HOST_CONN_CMPL               = BT_MSG_GROUP_HID | 0x81,
    BT_MSG_HID_HOST_CONN_FAIL               = BT_MSG_GROUP_HID | 0x82,
    BT_MSG_HID_HOST_DISCONN_CMPL            = BT_MSG_GROUP_HID | 0x83,
    BT_MSG_HID_HOST_HID_CONTROL_IND         = BT_MSG_GROUP_HID | 0x84,
    BT_MSG_HID_HOST_GET_REPORT_RSP          = BT_MSG_GROUP_HID | 0x85,
    BT_MSG_HID_HOST_SET_REPORT_RSP          = BT_MSG_GROUP_HID | 0x86,
    BT_MSG_HID_HOST_GET_PROTOCOL_RSP        = BT_MSG_GROUP_HID | 0x87,
    BT_MSG_HID_HOST_SET_PROTOCOL_RSP        = BT_MSG_GROUP_HID | 0x88,
    BT_MSG_HID_HOST_INTERRUPT_DATA_IND      = BT_MSG_GROUP_HID | 0x89,

    BT_MSG_MAP_MNS_CONN_IND            = BT_MSG_GROUP_MAP | 0x00,
    BT_MSG_MAP_MNS_CONN_CMPL           = BT_MSG_GROUP_MAP | 0x01,
    BT_MSG_MAP_MNS_CONN_FAIL           = BT_MSG_GROUP_MAP | 0x02,
    BT_MSG_MAP_MNS_DISCONN_CMPL        = BT_MSG_GROUP_MAP | 0x03,
    BT_MSG_MAP_MAS_CONN_CMPL           = BT_MSG_GROUP_MAP | 0x04,
    BT_MSG_MAP_MAS_CONN_FAIL           = BT_MSG_GROUP_MAP | 0x05,
    BT_MSG_MAP_MAS_DISCONN_CMPL        = BT_MSG_GROUP_MAP | 0x06,
    BT_MSG_MAP_SET_FOLDER_CMPL         = BT_MSG_GROUP_MAP | 0x07,
    BT_MSG_MAP_REG_NOTIF_CMPL          = BT_MSG_GROUP_MAP | 0x08,
    BT_MSG_MAP_GET_FOLDER_LISTING_CMPL = BT_MSG_GROUP_MAP | 0x09,
    BT_MSG_MAP_GET_MSG_LISTING_CMPL    = BT_MSG_GROUP_MAP | 0x0a,
    BT_MSG_MAP_GET_MSG_CMPL            = BT_MSG_GROUP_MAP | 0x0b,
    BT_MSG_MAP_PUSH_MSG_CMPL           = BT_MSG_GROUP_MAP | 0x0c,
    BT_MSG_MAP_MSG_NOTIFICATION        = BT_MSG_GROUP_MAP | 0x0d,

    BT_MSG_OPP_CONN_IND                    = BT_MSG_GROUP_OPP | 0x00,
    BT_MSG_OPP_CONN_CMPL                   = BT_MSG_GROUP_OPP | 0x01,
    BT_MSG_OPP_CONN_FAIL                   = BT_MSG_GROUP_OPP | 0x02,
    BT_MSG_OPP_DISCONN_CMPL                = BT_MSG_GROUP_OPP | 0x03,
    BT_MSG_OPP_DATA_HEADER_IND             = BT_MSG_GROUP_OPP | 0x04,
    BT_MSG_OPP_DATA_IND                    = BT_MSG_GROUP_OPP | 0x05,
    BT_MSG_OPP_DATA_RSP                    = BT_MSG_GROUP_OPP | 0x06,

    BT_MSG_RDTP_CONN_CMPL            = BT_MSG_GROUP_RDTP | 0x00,
    BT_MSG_RDTP_CONN_FAIL            = BT_MSG_GROUP_RDTP | 0x01,
    BT_MSG_RDTP_DISCONN_CMPL         = BT_MSG_GROUP_RDTP | 0x02,
    BT_MSG_RDTP_DATA_IND             = BT_MSG_GROUP_RDTP | 0x03,
    BT_MSG_RDTP_DATA_RSP             = BT_MSG_GROUP_RDTP | 0x04,

    BT_MSG_ROLESWAP_ACL_STATUS         = BT_MSG_GROUP_ROLESWAP | 0x00,
    BT_MSG_ROLESWAP_A2DP_STATUS        = BT_MSG_GROUP_ROLESWAP | 0x01,
    BT_MSG_ROLESWAP_AVRCP_STATUS       = BT_MSG_GROUP_ROLESWAP | 0x02,
    BT_MSG_ROLESWAP_HFP_STATUS         = BT_MSG_GROUP_ROLESWAP | 0x03,
    BT_MSG_ROLESWAP_PBAP_STATUS        = BT_MSG_GROUP_ROLESWAP | 0x04,
    BT_MSG_ROLESWAP_HID_DEVICE_STATUS  = BT_MSG_GROUP_ROLESWAP | 0x05,
    BT_MSG_ROLESWAP_HID_HOST_STATUS    = BT_MSG_GROUP_ROLESWAP | 0x06,
    BT_MSG_ROLESWAP_SPP_STATUS         = BT_MSG_GROUP_ROLESWAP | 0x07,
    BT_MSG_ROLESWAP_IAP_STATUS         = BT_MSG_GROUP_ROLESWAP | 0x08,
    BT_MSG_ROLESWAP_SCO_STATUS         = BT_MSG_GROUP_ROLESWAP | 0x09,
    BT_MSG_ROLESWAP_ADDR_STATUS        = BT_MSG_GROUP_ROLESWAP | 0x0a,
    BT_MSG_ROLESWAP_REQ                = BT_MSG_GROUP_ROLESWAP | 0x0b,
    BT_MSG_ROLESWAP_RSP                = BT_MSG_GROUP_ROLESWAP | 0x0c,
    BT_MSG_ROLESWAP_START              = BT_MSG_GROUP_ROLESWAP | 0x0d,
    BT_MSG_ROLESWAP_TERMINATED         = BT_MSG_GROUP_ROLESWAP | 0x0e,
    BT_MSG_ROLESWAP_CMPL               = BT_MSG_GROUP_ROLESWAP | 0x0f,
    BT_MSG_ROLESWAP_FLOW_STOP          = BT_MSG_GROUP_ROLESWAP | 0x10,
    BT_MSG_ROLESWAP_FLOW_GO            = BT_MSG_GROUP_ROLESWAP | 0x11,
    BT_MSG_ROLESWAP_SYNC_CMPL          = BT_MSG_GROUP_ROLESWAP | 0x12,
    BT_MSG_ROLESWAP_IDENT_CHANGE       = BT_MSG_GROUP_ROLESWAP | 0x13,

    BT_MSG_SNIFFING_ACL_CONN_CMPL             = BT_MSG_GROUP_SNIFFING | 0x00,
    BT_MSG_SNIFFING_ACL_DISCONN_CMPL          = BT_MSG_GROUP_SNIFFING | 0x01,
    BT_MSG_SNIFFING_SCO_CONN_CMPL             = BT_MSG_GROUP_SNIFFING | 0x02,
    BT_MSG_SNIFFING_SCO_START                 = BT_MSG_GROUP_SNIFFING | 0x03,
    BT_MSG_SNIFFING_SCO_STOP                  = BT_MSG_GROUP_SNIFFING | 0x04,
    BT_MSG_SNIFFING_A2DP_CONN_CMPL            = BT_MSG_GROUP_SNIFFING | 0x05,
    BT_MSG_SNIFFING_A2DP_START_IND            = BT_MSG_GROUP_SNIFFING | 0x06,
    BT_MSG_SNIFFING_A2DP_START                = BT_MSG_GROUP_SNIFFING | 0x07,
    BT_MSG_SNIFFING_A2DP_CONFIG_CMPL          = BT_MSG_GROUP_SNIFFING | 0x08,
    BT_MSG_SNIFFING_A2DP_STOP                 = BT_MSG_GROUP_SNIFFING | 0x09,
    BT_MSG_SNIFFING_AVRCP_CONN_CMPL           = BT_MSG_GROUP_SNIFFING | 0x0a,
    BT_MSG_SNIFFING_HFP_CONN_CMPL             = BT_MSG_GROUP_SNIFFING | 0x0b,
    BT_MSG_SNIFFING_PBAP_CONN_CMPL            = BT_MSG_GROUP_SNIFFING | 0x0c,
    BT_MSG_SNIFFING_HID_DEVICE_CONN_CMPL      = BT_MSG_GROUP_SNIFFING | 0x0d,
    BT_MSG_SNIFFING_HID_HOST_CONN_CMPL        = BT_MSG_GROUP_SNIFFING | 0x0e,
    BT_MSG_SNIFFING_SPP_CONN_CMPL             = BT_MSG_GROUP_SNIFFING | 0x0f,
    BT_MSG_SNIFFING_IAP_CONN_CMPL             = BT_MSG_GROUP_SNIFFING | 0x10,

    BT_MSG_VND_PER_INFO                   = BT_MSG_GROUP_VND | 0x00,
    BT_MSG_VND_RSSI_INFO                  = BT_MSG_GROUP_VND | 0x01,
    BT_MSG_VND_SET_TX_POWER_RSP           = BT_MSG_GROUP_VND | 0x02,
    BT_MSG_VND_SET_RSSI_GOLDEN_RANGE_RSP  = BT_MSG_GROUP_VND | 0x03,
    BT_MSG_VND_SET_IDLE_ZONE_RSP          = BT_MSG_GROUP_VND | 0x04,
    BT_MSG_VND_TRAFFIC_QOS_RSP            = BT_MSG_GROUP_VND | 0x05,
    BT_MSG_VND_TRAFFIC_QOS_CMPL           = BT_MSG_GROUP_VND | 0x06,
} T_BT_MSG;

typedef enum t_bt_role_switch_status
{
    BT_ROLE_SWITCH_IDLE             = 0x00, /* role switch not in progress */
    BT_ROLE_SWITCH_MASTER_RUNNING   = 0x01, /* role switch to master in progress */
    BT_ROLE_SWITCH_MASTER_PENDING   = 0x02, /* role switch to master waiting */
    BT_ROLE_SWITCH_SLAVE_RUNNING    = 0x03, /* role switch to slave in progress */
    BT_ROLE_SWITCH_SLAVE_PENDING    = 0x04, /* role switch to slave waiting */
} T_BT_ROLE_SWITCH_STATUS;

typedef enum t_bt_link_state
{
    BT_LINK_STATE_DISCONNECTED  = 0x00,
    BT_LINK_STATE_CONNECTING    = 0x01,
    BT_LINK_STATE_CONNECTED     = 0x02,
    BT_LINK_STATE_DISCONNECTING = 0x03,
} T_BT_LINK_STATE;

typedef enum t_bt_link_sco_state
{
    BT_LINK_SCO_STATE_DISCONNECTED  = 0x00,
    BT_LINK_SCO_STATE_CONNECTING    = 0x01,
    BT_LINK_SCO_STATE_CONNECTED     = 0x02,
    BT_LINK_SCO_STATE_DISCONNECTING = 0x03,
} T_BT_LINK_SCO_STATE;

typedef struct t_bt_rdtp_data_ind
{
    uint8_t  *buf;
    uint16_t  len;
} T_BT_RDTP_DATA_IND;

typedef struct t_bt_sco_data_ind
{
    uint16_t    handle;
    uint32_t    bt_clk;
    uint8_t     status;
    uint16_t    length;
    uint8_t    *p_data;
} T_BT_SCO_DATA_IND;

typedef struct t_bt_roleswap_req
{
    uint8_t     context;
} T_BT_ROLESWAP_REQ;

typedef struct t_bt_roleswap_rsp
{
    bool        accept;
    uint8_t     context;
} T_BT_ROLESWAP_RSP;

typedef struct t_bt_msg_payload
{
    uint8_t bd_addr[6];
    void   *msg_buf;
} T_BT_MSG_PAYLOAD;

typedef enum t_bt_remote_msg
{
    BT_REMOTE_MSG_UNKNOWN                  = 0x0000,
    BT_REMOTE_MSG_ACL_DISCONNECTED         = 0x0101,
    BT_REMOTE_MSG_A2DP_CONNECTED           = 0x0200,
    BT_REMOTE_MSG_A2DP_DISCONNECTED        = 0x0201,
    BT_REMOTE_MSG_AVRCP_CONNECTED          = 0x0300,
    BT_REMOTE_MSG_AVRCP_DISCONNECTED       = 0x0301,
    BT_REMOTE_MSG_HFP_CONNECTED            = 0x0400,
    BT_REMOTE_MSG_HFP_DISCONNECTED         = 0x0401,
    BT_REMOTE_MSG_SPP_CONNECTED            = 0x0500,
    BT_REMOTE_MSG_SPP_DISCONNECTED         = 0x0501,
    BT_REMOTE_MSG_IAP_CONNECTED            = 0x0600,
    BT_REMOTE_MSG_IAP_DISCONNECTED         = 0x0601,
    BT_REMOTE_MSG_PBAP_CONNECTED           = 0x0700,
    BT_REMOTE_MSG_PBAP_DISCONNECTED        = 0x0701,
    BT_REMOTE_MSG_ROLESWAP_INFO_XMIT       = 0x0800,
    BT_REMOTE_MSG_HID_CONNECTED            = 0x0900,
    BT_REMOTE_MSG_HID_DISCONNECTED         = 0x0901,
} T_BT_REMOTE_MSG;

typedef struct t_bt_remote_payload_acl_disconnected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_ACL_DISCONNECTED;

typedef struct t_bt_remote_payload_a2dp_connected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_A2DP_CONNECTED;

typedef struct t_bt_remote_payload_a2dp_disconnected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_A2DP_DISCONNECTED;

typedef struct t_bt_remote_payload_avrcp_connected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_AVRCP_CONNECTED;

typedef struct t_bt_remote_payload_avrcp_disconnected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_AVRCP_DISCONNECTED;

typedef struct t_bt_remote_payload_hfp_connected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_HFP_CONNECTED;

typedef struct t_bt_remote_payload_hfp_disconnected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_HFP_DISCONNECTED;

typedef struct t_bt_remote_payload_spp_connected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_SPP_CONNECTED;

typedef struct t_bt_remote_payload_spp_disconnected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_SPP_DISCONNECTED;

typedef struct t_bt_remote_payload_iap_connected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_IAP_CONNECTED;

typedef struct t_bt_remote_payload_iap_disconnected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_IAP_DISCONNECTED;

typedef struct t_bt_remote_payload_pbap_connected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_PBAP_CONNECTED;

typedef struct t_bt_remote_payload_pbap_disconnected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_PBAP_DISCONNECTED;

typedef struct t_bt_remote_payload_hid_connected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_HID_CONNECTED;

typedef struct t_bt_remote_payload_hid_disconnected
{
    uint8_t bd_addr[6];
} T_BT_REMOTE_PAYLOAD_HID_DISCONNECTED;

typedef struct t_bt_link
{
    struct t_bt_link       *next;
    uint8_t                 bd_addr[6];
    uint16_t                acl_handle;
    uint8_t                 index;

    T_BT_LINK_STATE         acl_link_state;
    bool                    acl_link_authenticated;
    bool                    acl_link_encrypted;
    bool                    acl_link_sc_ongoing;
    bool                    acl_link_role_master;
    uint16_t                acl_link_policy;
    T_BT_ROLE_SWITCH_STATUS role_switch_status;
    bool                    pm_enable;
    T_BT_LINK_PM_STATE      pm_state;
    T_BT_LINK_PM_ACTION     pm_action;
    uint32_t                pm_timeout;
    uint16_t                min_interval;
    uint16_t                max_interval;
    uint16_t                sniff_attempt;
    uint16_t                sniff_timeout;
    T_SYS_TIMER_HANDLE      timer_sniff;
    T_OS_QUEUE              pm_cback_list;
    T_BT_LINK_SCO_STATE     sco_state;
    uint16_t                sco_handle;
    uint8_t                 is_esco;
    uint8_t                 sco_air_mode;
    uint8_t                 sco_packet_length;
    uint8_t                 curr_sco_len;
    uint8_t                *sco_buf;
} T_BT_LINK;

typedef struct t_roleswap_info
{
    struct t_roleswap_info *next;
    uint8_t                 bd_addr[6];
    T_OS_QUEUE              info_list;
} T_ROLESWAP_INFO;

typedef struct t_btm_db
{
    T_OS_QUEUE               link_list;
    T_OS_QUEUE               roleswap_info_list;
    T_REMOTE_RELAY_HANDLE    relay_handle;
    T_OS_QUEUE               cback_list;
    bool                     stack_ready;
    T_BT_DEVICE_MODE         curr_dev_mode;
    T_BT_DEVICE_MODE_ACTION  pending_dev_mode_action;
    T_BT_DEVICE_MODE_ACTION  next_dev_mode_action;
    uint8_t                  active_a2dp_addr[6];
} T_BTM_DB;

extern T_BTM_DB btm_db;

T_BT_LINK *bt_link_alloc(uint8_t bd_addr[6]);

void bt_link_free(T_BT_LINK *link);

T_BT_LINK *bt_link_find(uint8_t bd_addr[6]);

T_BT_LINK *bt_link_find_by_handle(uint16_t handle);

bool bt_link_policy_set(uint8_t  bd_addr[6],
                        uint16_t link_policy);

bool bt_link_policy_get(uint8_t   bd_addr[6],
                        uint16_t *link_policy);

bool bt_mgr_dispatch(T_BT_MSG  msg,
                     void     *buf);

bool bt_sniff_mode_config(uint8_t  bd_addr[6],
                          uint16_t sniff_interval,
                          uint16_t sniff_attempt,
                          uint16_t sniff_timeout,
                          uint32_t pm_timeout);

bool bt_sniff_mode_enter(uint8_t  bd_addr[6],
                         uint16_t min_interval,
                         uint16_t max_interval,
                         uint16_t sniff_attempt,
                         uint16_t sniff_timeout);

bool bt_sdp_init(void);

void bt_sdp_enable(bool enable);

void bt_sdp_record_register(void);

void bt_handle_add_sdp_rsp(void *buf);

void bt_handle_del_sdp_rsp(void *buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_MGR_INT_H_ */
