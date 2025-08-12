/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AVRCP_H_
#define _AVRCP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AVRCP_SUBUNIT_TYPE_PANEL             0x09
#define AVRCP_SUBUNIT_TYPE_VENDOR_UNIQUE     0x1C
#define AVRCP_SUBUNIT_ID                     0x00

#define AVRCP_PASS_THROUGH_KEY_PRESS       0x00
#define AVRCP_PASS_THROUGH_KEY_RELEASE     0x80
#define AVRCP_PASS_THROUGH_NEXT_GROUP      0x00
#define AVRCP_PASS_THROUGH_PREVIOUS_GROUP  0x01

#define COMPANY_BT_SIG                  0x001958

#define PDU_ID_GET_CAPABILITIES              0x10
#define PDU_ID_LIST_APP_SETTING_ATTRS        0x11
#define PDU_ID_LIST_APP_SETTING_VALUES       0x12
#define PDU_ID_GET_CURRENT_APP_SETTING_VALUE 0x13
#define PDU_ID_SET_APP_SETTING_VALUE         0x14
#define PDU_ID_GET_ELEMENT_ATTRS             0x20
#define PDU_ID_GET_PLAY_STATUS               0x30
#define PDU_ID_REGISTER_NOTIFICATION         0x31
#define PDU_ID_REQUEST_CONTINUE_RSP          0x40
#define PDU_ID_ABORT_CONTINUE_RSP            0x41
#define PDU_ID_SET_ABSOLUTE_VOLUME           0x50
#define PDU_ID_SET_ADDRESSED_PLAYER          0x60
#define PDU_ID_SET_BROWSED_PLAYER            0x70
#define PDU_ID_GET_FOLDER_ITEMS              0x71
#define PDU_ID_CHANGE_PATH                   0x72
#define PDU_ID_GET_ITEM_ATTRS                0x73
#define PDU_ID_PLAY_ITEM                     0x74
#define PDU_ID_GET_TOTAL_NUM_OF_ITEMS        0x75
#define PDU_ID_SEARCH                        0x80
#define PDU_ID_GENERAL_REJECT                0xA0

/** Folder Item Type */
#define AVRCP_ITEM_TYPE_MEDIA_PLAYER  0x01
#define AVRCP_ITEM_TYPE_FOLDER        0x02
#define AVRCP_ITEM_TYPE_MEDIA_ELEMENT 0x03

/** Get Element Attributes */
#define MAX_ELEMENT_ATTR_NUM    8
typedef enum t_avrcp_element_attr_type
{
    ELEMENT_ATTR_TITLE             = 0x01,
    ELEMENT_ATTR_ARTIST            = 0x02,
    ELEMENT_ATTR_ALBUM             = 0x03,
    ELEMENT_ATTR_TRACK             = 0x04,
    ELEMENT_ATTR_TOTAL_TRACK       = 0x05,
    ELEMENT_ATTR_GENRE             = 0x06,
    ELEMENT_ATTR_PLAYING_TIME      = 0x07,
    ELEMENT_ATTR_DEFAULT_COVER_ART = 0x08,
} T_AVRCP_ELEMENT_ATTR_TYPE;

/*play status*/
typedef enum t_avrcp_play_status_type
{
    AVRCP_PLAY_STATUS_STOPPED  = 0x00,
    AVRCP_PLAY_STATUS_PLAYING  = 0x01,
    AVRCP_PLAY_STATUS_PAUSED   = 0x02,
    AVRCP_PLAY_STATUS_FWD_SEEK = 0x03,
    AVRCP_PLAY_STATUS_REV_SEEK = 0x04,
    AVRCP_PLAY_STATUS_FAST_FWD = 0x05,
    AVRCP_PLAY_STATUS_REWIND   = 0x06,
} T_AVRCP_PLAY_STATUS_TYPE;

/*event id */
typedef enum
{
    EVENT_PLAYBACK_STATUS_CHANGED            = 0x01,  /* Change in playback status of the current track */
    EVENT_TRACK_CHANGED                      = 0x02,  /* Change of current track */
    EVENT_TRACK_REACHED_END                  = 0x03,  /* Reached end of a track */
    EVENT_TRACK_REACHED_START                = 0x04,  /* Reached start of a track */
    EVENT_PLAYBACK_POS_CHANGED               = 0x05,  /* Change in playback position */
    EVENT_BATT_STATUS_CHANGED                = 0x06,  /* Change in battery status */
    EVENT_SYSTEM_STATUS_CHANGED              = 0x07,  /* Change in system status */
    EVENT_PLAYER_APPLICATION_SETTING_CHANGED = 0x08,  /* Change in player application setting */
    EVENT_NOW_PLAYING_CONTENT_CHANGED        = 0x09,  /* The content of the Now Playing list has changed */
    EVENT_AVAILABLE_PLAYERS_CHANGED          = 0x0a,  /* The available players have changed */
    EVENT_ADDRESSED_PLAYER_CHANGED           = 0x0b,  /* The Addressed Player has been changed */
    EVENT_UIDS_CHANGED                       = 0x0c,  /* The UIDs have changed */
    EVENT_VOLUME_CHANGED                     = 0x0d,  /* (TG) The volume has been changed locally on the TG */
} T_AVRCP_EVENT_ID;

/*Response Status and Error Codes*/
typedef enum
{
    /* Sent if TG received a PDU that it did not understand */
    AVRCP_RSP_STATUS_INVALID_COMMAND           = 0x00,

    /* Sent if the TG received a PDU with a parameter ID that it did not understand */
    AVRCP_RSP_STATUS_INVALID_PARAMETER         = 0x01,

    /* Sent if the parameter ID is understood, but content is wrong or corrupted */
    AVRCP_RSP_STATUS_PARAMETER_CONTENT_ERROR   = 0x02,

    /* Sent if there are error conditions not covered by a more specific error code */
    AVRCP_RSP_STATUS_INTERNAL_ERROR            = 0x03,

    /* This is the status that should be returned if the operation was successful */
    AVRCP_RSP_STATUS_SUCCESS                   = 0x04,

    /* The UIDs on the device have changed */
    AVRCP_RSP_STATUS_UID_CHANGED               = 0x05,

    /* The Direction parameter is invalid */
    AVRCP_RSP_STATUS_INVALID_DIRECTION         = 0x07,

    /* The UID provided does not refer to a folder item */
    AVRCP_RSP_STATUS_NOT_DIRECTORY             = 0x08,

    /* The UID provided does not refer to any currently valid item */
    AVRCP_RSP_STATUS_NOT_EXIST                 = 0x09,

    /* The scope parameter is invalid */
    AVRCP_RSP_STATUS_INVALID_SCOPE             = 0x0a,

    /* The start of range provided is not valid */
    AVRCP_RSP_STATUS_OUT_OF_BOUNDS             = 0x0b,

    /* The UID provided refers to a folder item which cannot be handled by this media player */
    AVRCP_RSP_STATUS_NOT_PLAYABLE              = 0x0c,

    /* The media is not able to be used for this operation at this time */
    AVRCP_RSP_STATUS_MEDIA_IN_USE              = 0x0d,

    /* No more items can be added to the Now Playing List */
    AVRCP_RSP_STATUS_PLAYING_LIST_FULL         = 0x0e,

    /* The Browsed Media Player does not support search */
    AVRCP_RSP_STATUS_SEARCH_NOT_SUPPORTED      = 0x0f,

    /* A search operation is already in progress */
    AVRCP_RSP_STATUS_SEARCH_IN_PROGRESS        = 0x10,

    /* The specified Player Id does not refer to a valid player */
    AVRCP_RSP_STATUS_INVALID_PLAYER_ID         = 0x11,

    /* The Player Id supplied refers to a Media Player which does not support browsing */
    AVRCP_RSP_STATUS_PLAYER_NOT_BROWSABLE      = 0x12,

    /* The Player Id supplied refers to a player which is not currently addressed */
    AVRCP_RSP_STATUS_PLAYER_NOT_ADDRESSED      = 0x13,

    /* The Search result list does not contain valid entries */
    AVRCP_RSP_STATUS_NO_VALID_SEARCH_RESULTS   = 0x14,

    AVRCP_RSP_STATUS_NO_AVAILABLE_PLAYERS      = 0x15,

    AVRCP_RSP_STATUS_ADDRESSED_PLAYER_CHANGED  = 0x16,
} T_AVRCP_RSP_ERROR_STATUS;

/*player application setting attributes*/
#define MAX_APP_SETTING_ATTR_NUM    4
#define MAX_APP_SETTING_VALUE_NUM   4
typedef enum
{
    APP_SETTING_ATTR_EQUALIZER        = 0x01,
    APP_SETTING_ATTR_REPEAT_MODE      = 0x02,
    APP_SETTING_ATTR_SHUFFLE          = 0x03,
    APP_SETTING_ATTR_SCAN             = 0x04,
} T_AVRCP_APP_SETTING_ATTR_ID;

typedef enum
{
    EQUALIZER_STATUS_OFF          = 0x01,
    EQUALIZER_STATUS_ON           = 0x02,
} T_AVRCP_APP_SETTING_VALUE_EQUALIZER_STATUS;

typedef enum
{
    REPEAT_MODE_STATUS_OFF        = 0x01,
    REPEAT_MODE_STATUS_SINGLE     = 0x02,
    REPEAT_MODE_STATUS_ALL        = 0x03,
    REPEAT_MODE_STATUS_GROUP      = 0x04,
} T_AVRCP_APP_SETTING_VALUE_REPEAT_MODE_STATUS;

typedef enum
{
    SHUFFLE_STATUS_OFF            = 0x01,
    SHUFFLE_STATUS_ALL            = 0x02,
    SHUFFLE_STATUS_GROUP          = 0x03,
} T_AVRCP_APP_SETTING_VALUE_SHUFFLE_STATUS;

typedef enum
{
    SCAN_STATUS_OFF               = 0x01,
    SCAN_STATUS_ALL               = 0x02,
    SCAN_STATUS_GROUP             = 0x03,
} T_AVRCP_APP_SETTING_VALUE_SCAN_STATUS;

typedef enum
{
    SCOPE_MEDIA_PLAYER_LIST               = 0x00,
    SCOPE_MEDIA_PLAYER_VIRTUAL_FILESYSTEM = 0x01,
    SCOPE_SEARCH                          = 0x02,
    SCOPE_NOW_PLAYING                     = 0x03,
} T_AVRCP_SCOPE_ID;

typedef enum t_capability_id
{
    CAPABILITY_ID_COMPANY_ID       = 0x02,
    CAPABILITY_ID_EVENTS_SUPPORTED = 0x03,
} T_CAPABILITY_ID;

typedef enum t_avrcp_msg
{
    AVRCP_MSG_CONN_IND                          = 0x00,
    AVRCP_MSG_CONN_CMPL                         = 0x01,
    AVRCP_MSG_CONN_FAIL                         = 0x02,
    AVRCP_MSG_DISCONN                           = 0x03,

    /*volume sync feature as catogory2(amplifier) TG*/
    AVRCP_MSG_CMD_VOL_UP                        = 0x04,
    AVRCP_MSG_CMD_VOL_DOWN                      = 0x05,
    AVRCP_MSG_CMD_ABS_VOL                       = 0x06,
    AVRCP_MSG_CMD_REG_VOL_CHANGE                = 0x07,

    /*catogory1(player) CT*/
    AVRCP_MSG_RSP_UNIT_INFO                     = 0x08,
    AVRCP_MSG_RSP_PASSTHROUGH                   = 0x09,
    AVRCP_MSG_RSP_GET_CPBS                      = 0x0a,
    AVRCP_MSG_RSP_GET_PLAYSTATUS                = 0x0b,
    AVRCP_MSG_RSP_GET_ELEMENT_ATTR              = 0x0c,
    AVRCP_MSG_RSP_REG_NOTIFICATION              = 0x0d,

    AVRCP_MSG_RSP_DUMMY                         = 0x0e,

    AVRCP_MSG_NOTIF_CHANGED                     = 0x0f,
    AVRCP_MSG_RCV_RSP                           = 0x10,

    AVRCP_MSG_RSP_LIST_APP_SETTING_ATTRS        = 0x11,
    AVRCP_MSG_RSP_LIST_APP_SETTING_VALUES       = 0x12,
    AVRCP_MSG_RSP_GET_CUR_APP_SETTING_VALUE     = 0x13,
    AVRCP_MSG_RSP_SET_APP_SETTING_VALUE         = 0x14,
    AVRCP_MSG_RSP_SET_ADDRESSED_PLAYER          = 0x15,
    AVRCP_MSG_RSP_PLAY_ITEM                     = 0x16,

    AVRCP_MSG_RSP_GET_FOLDER_ITEMS              = 0x17,
    AVRCP_MSG_RSP_GET_ITEM_ATTRS                = 0x18,
    AVRCP_MSG_RSP_SEARCH                        = 0x19,
    AVRCP_MSG_RSP_SET_BROWSED_PLAYER            = 0x1a,
    AVRCP_MSG_RSP_CHANGE_PATH                   = 0x1b,
    AVRCP_MSG_RSP_SET_ABS_VOL                   = 0x1c,

    AVRCP_MSG_BROWSING_CONN_IND                 = 0x20,
    AVRCP_MSG_BROWSING_CONN_CMPL                = 0x21,
    AVRCP_MSG_BROWSING_DISCONN                  = 0x22,

    AVRCP_MSG_CMD_PLAY                          = 0x31,
    AVRCP_MSG_CMD_STOP                          = 0x32,
    AVRCP_MSG_CMD_PAUSE                         = 0x33,
    AVRCP_MSG_CMD_REWIND_START                  = 0x34,
    AVRCP_MSG_CMD_REWIND_STOP                   = 0x35,
    AVRCP_MSG_CMD_FAST_FORWARD_START            = 0x36,
    AVRCP_MSG_CMD_FAST_FORWARD_STOP             = 0x37,
    AVRCP_MSG_CMD_FORWARD                       = 0x38,
    AVRCP_MSG_CMD_BACKWARD                      = 0x39,
    AVRCP_MSG_CMD_POWER                         = 0x3a,
    AVRCP_MSG_CMD_MUTE                          = 0x3b,

    /*catogory1(player) TG*/
    AVRCP_MSG_CMD_GET_ELEMENT_ATTRS             = 0x40,
    AVRCP_MSG_CMD_GET_PLAY_STATUS               = 0x41,
    AVRCP_MSG_CMD_SET_ADDRESSED_PLAYER          = 0x42,
    AVRCP_MSG_CMD_REG_PLAY_STATUS_CHANGE        = 0x43,
    AVRCP_MSG_CMD_REG_TRACK_CHANGE              = 0x44,
    AVRCP_MSG_CMD_REG_ADDRESSED_PLAYER_CHANGE   = 0x45,
    AVRCP_MSG_CMD_GET_FOLDER_ITEMS              = 0x46,
    AVRCP_MSG_CMD_GET_TOTAL_NUM_OF_ITEMS        = 0x47,

    AVRCP_MSG_COVER_ART_CONN_CMPL               = 0x50,
    AVRCP_MSG_COVER_ART_DISCONN                 = 0x51,
    AVRCP_MSG_COVER_ART_DATA_IND                = 0x52,

    AVRCP_MSG_VENDOR_CMD_IND                    = 0x60,
    AVRCP_MSG_VENDOR_RSP                        = 0x61,

    AVRCP_MSG_ERR                               = 0xff,
} T_AVRCP_MSG;

typedef enum t_avrcp_key
{
    AVRCP_KEY_POWER         = 0x40,
    AVRCP_KEY_VOL_UP        = 0x41,
    AVRCP_KEY_VOL_DOWN      = 0x42,
    AVRCP_KEY_MUTE          = 0x43,
    AVRCP_KEY_PLAY          = 0x44,
    AVRCP_KEY_STOP          = 0x45,
    AVRCP_KEY_PAUSE         = 0x46,
    AVRCP_KEY_REWIND        = 0x48,
    AVRCP_KEY_FAST_FORWARD  = 0x49,
    AVRCP_KEY_FORWARD       = 0x4B,
    AVRCP_KEY_BACKWARD      = 0x4C,
} T_AVRCP_KEY;

typedef struct t_element_attr
{
    uint32_t  attribute_id;
    uint16_t  character_set_id;
    uint16_t  length;
    uint8_t  *p_buf;
} T_ELEMENT_ATTR;

typedef enum t_avrcp_rsp_state
{
    AVRCP_RSP_STATE_SUCCESS = 0x00,
    AVRCP_RSP_STATE_FAIL    = 0x01,
//  AVRCP_REMOTE_REJECT?
} T_AVRCP_RSP_STATE;

typedef enum t_avrcp_msg_err
{
    AVRCP_WAIT_RSP_TO = 0x00,
} T_AVRCP_MSG_ERR;

typedef struct t_rsp_unit_info
{
    T_AVRCP_RSP_STATE  state;
    uint8_t            sub_unit_type;
    uint8_t            sub_unit_id;
    uint32_t           company_id;
} T_RSP_UNIT_INFO;

typedef struct t_rsp_passthrough
{
    T_AVRCP_RSP_STATE  state;
    T_AVRCP_KEY        key;
    bool               pressed;
} T_RSP_PASSTHROUGH;

typedef struct t_rsp_cpbs
{
    T_AVRCP_RSP_STATE  state;
    uint8_t            capability_id;
    uint8_t            capability_count;
    uint8_t           *p_buf;
} T_RSP_CPBS;

typedef struct t_rsp_get_play_status
{
    T_AVRCP_RSP_STATE  state;
    uint32_t           length_ms;
    uint32_t           position_ms;
    uint8_t            play_status;
} T_RSP_GET_PLAY_STATUS;

typedef struct
{
    uint8_t    attr_num;
    uint32_t   attr_id[MAX_ELEMENT_ATTR_NUM];
} T_AVRCP_REQ_GET_ELEMENT_ATTR;

typedef struct t_rsp_get_element_attr
{
    T_AVRCP_RSP_STATE  state;
    uint8_t            num_of_attr;
    T_ELEMENT_ATTR     attr[MAX_ELEMENT_ATTR_NUM];
} T_RSP_GET_ELEMENT_ATTR;

typedef struct
{
    uint8_t  key;
    bool     pressed;
} T_AVRCP_REQ_PASS_THROUGH;

typedef struct
{
    uint8_t attr;
    uint8_t value;
} T_AVRCP_APP_SETTING;

typedef struct
{
    uint8_t                  num_of_attr;
    T_AVRCP_APP_SETTING      app_setting[MAX_APP_SETTING_ATTR_NUM];
} T_AVRCP_RSP_PARAM_APP_SETTING_CHANGED;

typedef struct
{
    uint16_t player_id;
    uint16_t uid_counter;
} T_AVRCP_RSP_PARAM_ADDRESSED_PLAYER_CHANGED;

typedef struct
{
    T_AVRCP_RSP_STATE  state;
    uint8_t            event_id;
    union
    {
        uint8_t                                     play_status;
        uint64_t                                    track_id;
        T_AVRCP_RSP_PARAM_APP_SETTING_CHANGED       app_settings;
        T_AVRCP_RSP_PARAM_ADDRESSED_PLAYER_CHANGED  addressed_player;
        uint16_t                                    uid_counter;
        uint8_t                                     volume;
    } u;
} T_RSP_REG_NOTIFICATION;

typedef struct t_notification_changed
{
    uint8_t event_id;
    union
    {
        uint8_t                                     play_status;
        uint64_t                                    track_id;
        T_AVRCP_RSP_PARAM_APP_SETTING_CHANGED       app_settings;
        T_AVRCP_RSP_PARAM_ADDRESSED_PLAYER_CHANGED  addressed_player;
        uint16_t                                    uid_counter;
        uint8_t                                     volume;
    } u;
} T_NOTIF_CHANGED;

typedef struct
{
    uint16_t    cause;
} T_AVRCP_DISCONN_INFO;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    l2c_cid;
    uint16_t    remote_mtu;
    uint8_t     data_offset;
    uint8_t     avctp_state;
    uint8_t     state;
    uint8_t     play_status;
    uint8_t     cmd_credits;
    uint8_t     transact_label;
    bool        vol_change_registered;
    uint8_t     vol_change_pending_transact;
    uint8_t     vendor_cmd_transact;
} T_ROLESWAP_AVRCP_INFO;

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     avctp_state;
    uint8_t     state;
    uint8_t     play_status;
    uint8_t     cmd_credits;
    uint8_t     transact_label;
    bool        vol_change_registered;
    uint8_t     vol_change_pending_transact;
} T_ROLESWAP_AVRCP_TRANSACT;

typedef struct
{
    T_AVRCP_RSP_STATE  state;
    uint8_t            num_of_attr;
    uint8_t            attr_id[MAX_APP_SETTING_ATTR_NUM];
} T_AVRCP_RSP_LIST_APP_SETTING_ATTRS;

typedef struct
{
    T_AVRCP_RSP_STATE  state;
    uint8_t            num_of_value;
    uint8_t            value[MAX_APP_SETTING_VALUE_NUM];
} T_AVRCP_RSP_LIST_APP_SETTING_VALUES;

typedef struct
{
    uint8_t  attr_num;
    uint8_t  attr_id[MAX_APP_SETTING_ATTR_NUM];
} T_AVRCP_REQ_GET_APP_SETTING_VALUE;

typedef struct
{
    T_AVRCP_RSP_STATE        state;
    uint8_t                  num_of_attr;
    T_AVRCP_APP_SETTING      app_setting[MAX_APP_SETTING_ATTR_NUM];
} T_AVRCP_RSP_GET_APP_SETTING_VALUE;

typedef struct
{
    uint8_t              attr_num;
    T_AVRCP_APP_SETTING  app_setting[MAX_APP_SETTING_ATTR_NUM];
} T_AVRCP_REQ_SET_APP_SETTING_VALUE;

typedef struct
{
    T_AVRCP_RSP_STATE        state;
    T_AVRCP_RSP_ERROR_STATUS status_code;
} T_AVRCP_RSP_SET_ADDRESSED_PLAYER;

typedef struct
{
    T_AVRCP_SCOPE_ID  scope_id;
    uint64_t          uid;
    uint16_t          uid_counter;
} T_AVRCP_REQ_PLAY_ITEM;

typedef struct
{
    T_AVRCP_RSP_STATE        state;
    T_AVRCP_RSP_ERROR_STATUS status_code;
} T_AVRCP_RSP_PLAY_ITEM;

typedef struct t_avrcp_rsp_set_abs_vol
{
    T_AVRCP_RSP_STATE  state;
    uint8_t            volume;
} T_AVRCP_RSP_SET_ABS_VOL;

typedef struct
{
    uint8_t   scope;
    uint32_t  start_item;
    uint32_t  end_item;
    uint8_t   attr_count;
    uint32_t  attr_id[MAX_ELEMENT_ATTR_NUM];
} T_AVRCP_REQ_GET_FOLDER_ITEMS;

typedef struct
{
    uint8_t   item_type;
    uint16_t  item_length;
    uint16_t  player_id;
    uint8_t   major_type;
    uint32_t  sub_type;
    uint8_t   play_status;
    uint8_t   feature_bitmask[16];
    uint16_t  character_set_id;
    uint16_t  display_name_length;
    uint8_t  *p_display_name;
} T_MEDIA_PLAYER_ITEM;

typedef struct
{
    uint8_t   item_type;
    uint16_t  item_length;
    uint64_t  folder_uid;
    uint8_t   folder_type;
    uint8_t   is_playable;
    uint16_t  character_set_id;
    uint16_t  display_name_length;
    uint8_t  *p_display_name;
} T_FOLDER_ITEM;

typedef struct
{
    uint8_t           item_type;
    uint16_t          item_length;
    uint64_t          media_element_uid;
    uint8_t           media_type;
    uint16_t          character_set_id;
    uint16_t          display_name_length;
    uint8_t          *p_display_name;
    uint8_t           num_of_attr;
    T_ELEMENT_ATTR    attr[MAX_ELEMENT_ATTR_NUM];
} T_MEDIA_ELEMENT_ITEM;

typedef struct t_cmd_get_folder_items
{
    uint8_t     scope_id;
    uint32_t    start_item;
    uint32_t    end_item;
    uint8_t     attr_count;
    uint32_t    attr_id[MAX_ELEMENT_ATTR_NUM];
} T_CMD_GET_FOLDER_ITEMS;

typedef struct
{
    T_AVRCP_RSP_ERROR_STATUS status_code;
    uint16_t                 uid_counter;
    uint16_t                 num_of_items;
    uint8_t                  item_type;
    union
    {
        T_MEDIA_PLAYER_ITEM  *p_media_player_items;
        T_FOLDER_ITEM        *p_folder_items;
        T_MEDIA_ELEMENT_ITEM *p_media_element_items;
    } u;
} T_RSP_GET_FOLDER_ITEMS;

typedef struct
{
    uint8_t   scope;
    uint64_t  uid;
    uint16_t  uid_counter;
    uint8_t   num_of_attr;
    uint32_t  attr_id[MAX_ELEMENT_ATTR_NUM];
} T_AVRCP_REQ_GET_ITEM_ATTRS;

typedef struct
{
    T_AVRCP_RSP_ERROR_STATUS status_code;
    uint8_t                  num_of_attr;
    T_ELEMENT_ATTR           attr[MAX_ELEMENT_ATTR_NUM];
} T_AVRCP_RSP_GET_ITEM_ATTRS;

typedef struct
{
    uint16_t length;
    uint8_t  *p_search_str;
} T_AVRCP_REQ_SEARCH;

typedef struct
{
    T_AVRCP_RSP_ERROR_STATUS  status_code;
    uint16_t                  uid_counter;
    uint32_t                  num_of_items;
} T_AVRCP_RSP_SEARCH;

#define MAX_FOLDER_DEPTH 10
typedef struct
{
    uint16_t name_length;
    uint8_t  *p_name;
} T_AVRCP_FOLDER;

typedef struct
{
    T_AVRCP_RSP_ERROR_STATUS  status_code;
    uint16_t                  uid_counter;
    uint32_t                  num_of_items;
    uint16_t                  character_set_id;
    uint8_t                   folder_depth;
    T_AVRCP_FOLDER            folder[MAX_FOLDER_DEPTH];
} T_AVRCP_RSP_SET_BROWSED_PLAYER;

typedef struct
{
    uint16_t uid_counter;
    uint8_t  direction;
    uint64_t folder_uid;
} T_AVRCP_REQ_CHANGE_PATH;

typedef struct
{
    T_AVRCP_RSP_ERROR_STATUS  status_code;
    uint32_t                  num_of_items;
} T_AVRCP_RSP_CHANGE_PATH;

typedef struct
{
    uint8_t   *p_data;
    uint16_t   data_len;
    bool       data_end;
} T_AVRCP_COVER_ART_MSG_DATA;

typedef struct
{
    uint8_t  ctype;
    uint32_t company_id;
    uint8_t *p_cmd;
    uint16_t cmd_len;
} T_AVRCP_VENDOR_CMD;

typedef struct
{
    uint8_t  response;
    uint32_t company_id;
    uint8_t *p_rsp;
    uint16_t rsp_len;
} T_AVRCP_VENDOR_RSP;

typedef void (*P_AVRCP_CBACK)(uint8_t      bd_addr[6],
                              T_AVRCP_MSG  msg_type,
                              void        *msg_buf);

bool avrcp_init(uint8_t       link_num,
                uint32_t      company_id,
                P_AVRCP_CBACK cback);

bool avrcp_set_supported_features(uint8_t ct_features,
                                  uint8_t tg_features);

bool avrcp_connect_req(uint8_t bd_addr[6]);

bool avrcp_disconnect_req(uint8_t bd_addr[6]);

bool avrcp_connect_cfm(uint8_t bd_addr[6],
                       bool    accept);

bool avrcp_send_unit_info(uint8_t bd_addr[6]);

bool avrcp_send_pass_through(uint8_t     bd_addr[6],
                             T_AVRCP_KEY key,
                             bool        pressed);

bool avrcp_get_capability(uint8_t bd_addr[6],
                          uint8_t capability_id);

bool avrcp_get_play_status(uint8_t bd_addr[6]);

bool avrcp_get_play_status_rsp(uint8_t  bd_addr[6],
                               uint32_t song_length,
                               uint32_t song_pos,
                               uint8_t  play_status);

bool avrcp_get_element_attr(uint8_t  bd_addr[6],
                            uint8_t  attr_num,
                            uint8_t *p_attr);

bool avrcp_get_element_attr_rsp(uint8_t         bd_addr[6],
                                uint8_t         attr_num,
                                T_ELEMENT_ATTR *p_attr);

bool avrcp_register_notification(uint8_t bd_addr[6],
                                 uint8_t event_id);

bool avrcp_volume_change_register_rsp(uint8_t bd_addr[6],
                                      uint8_t volume);

bool avrcp_notify_volume_change(uint8_t bd_addr[6],
                                uint8_t volume);

bool avrcp_play_status_change_register_rsp(uint8_t bd_addr[6],
                                           uint8_t play_status);

bool avrcp_notify_play_status_change(uint8_t bd_addr[6],
                                     uint8_t play_status);

bool avrcp_track_change_register_rsp(uint8_t  bd_addr[6],
                                     uint64_t track_id);

bool avrcp_notify_track_change(uint8_t  bd_addr[6],
                               uint64_t track_id);

bool avrcp_get_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info);

bool avrcp_set_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info);

bool avrcp_del_roleswap_info(uint8_t bd_addr[6]);

bool avrcp_list_app_setting_attrs(uint8_t bd_addr[6]);

bool avrcp_list_app_setting_values(uint8_t bd_addr[6],
                                   uint8_t attr_id);

bool avrcp_get_current_app_setting_value(uint8_t  bd_addr[6],
                                         uint8_t  attr_num,
                                         uint8_t *p_attr);

bool avrcp_set_app_setting_value(uint8_t              bd_addr[6],
                                 uint8_t              attr_num,
                                 T_AVRCP_APP_SETTING *p_app_setting);

bool avrcp_req_continuing_rsp(uint8_t bd_addr[6]);

bool avrcp_abort_continuing_rsp(uint8_t bd_addr[6]);

bool avrcp_set_absolute_volume(uint8_t bd_addr[6],
                               uint8_t absolute_volume);

bool avrcp_set_addressed_player(uint8_t  bd_addr[6],
                                uint16_t player_id);

bool avrcp_set_addressed_player_rsp(uint8_t bd_addr[6],
                                    uint8_t status);

bool avrcp_addressed_player_change_register_rsp(uint8_t  bd_addr[6],
                                                uint16_t player_id,
                                                uint16_t uid_counter);

bool avrcp_notify_addressed_player_change(uint8_t  bd_addr[6],
                                          uint16_t player_id,
                                          uint16_t uid_counter);

bool avrcp_play_item(uint8_t          bd_addr[6],
                     T_AVRCP_SCOPE_ID scope_id,
                     uint64_t         uid,
                     uint16_t         uid_counter);

bool avrcp_navigate_group(uint8_t bd_addr[6],
                          bool    next,
                          bool    pressed);

bool avrcp_send_vendor_cmd(uint8_t   bd_addr[6],
                           uint8_t   subunit_type,
                           uint8_t   subunit_id,
                           uint8_t   ctype,
                           uint32_t  company_id,
                           uint8_t  *p_pdu,
                           uint16_t  pdu_length);

bool avrcp_send_vendor_rsp(uint8_t   bd_addr[6],
                           uint8_t   subunit_type,
                           uint8_t   subunit_id,
                           uint8_t   response,
                           uint32_t  company_id,
                           uint8_t  *p_pdu,
                           uint16_t  pdu_length);

bool avrcp_browsing_connect_req(uint8_t bd_addr[6]);

bool avrcp_browsing_disconnect_req(uint8_t bd_addr[6]);

bool avrcp_browsing_connect_cfm(uint8_t bd_addr[6],
                                bool    accept);

bool avrcp_browsing_get_folder_items(uint8_t                       bd_addr[6],
                                     T_AVRCP_REQ_GET_FOLDER_ITEMS *p_cmd_para);

bool avrcp_browsing_get_folder_items_rsp(uint8_t   bd_addr[6],
                                         uint8_t   status,
                                         uint16_t  uid_counter,
                                         uint16_t  num_of_items,
                                         void     *p_items);

bool avrcp_browsing_get_total_num_of_items_rsp(uint8_t  bd_addr[6],
                                               uint8_t  status,
                                               uint16_t uid_counter,
                                               uint32_t num_of_items);

bool avrcp_browsing_get_item_attrs(uint8_t                     bd_addr[6],
                                   T_AVRCP_REQ_GET_ITEM_ATTRS *p_cmd_para);

bool avrcp_browsing_search(uint8_t             bd_addr[6],
                           T_AVRCP_REQ_SEARCH *p_cmd_para);

bool avrcp_browsing_set_browsed_player(uint8_t  bd_addr[6],
                                       uint16_t player_id);

bool avrcp_browsing_change_path(uint8_t                  bd_addr[6],
                                T_AVRCP_REQ_CHANGE_PATH *p_cmd_para);

bool avrcp_cover_art_conn_over_l2c(uint8_t  bd_addr[6],
                                   uint16_t l2c_psm);

bool avrcp_cover_art_disconnect(uint8_t bd_addr[6]);

bool avrcp_cover_art_get_image(uint8_t  bd_addr[6],
                               uint8_t *img_handle);

bool avrcp_cover_art_get_linked_thumbnail(uint8_t  bd_addr[6],
                                          uint8_t *img_handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AVRCP_H_ */
