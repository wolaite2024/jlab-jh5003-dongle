/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_AVRCP_SUPPORT == 1)
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "sys_timer.h"
#include "mpa.h"
#include "avctp.h"
#include "obex.h"
#include "btm.h"
#include "bt_avrcp.h"
#include "bt_avrcp_int.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

#define AVRCP_SUBUNIT_TYPE_PANEL             0x09
#define AVRCP_SUBUNIT_TYPE_VENDOR_UNIQUE     0x1C
#define AVRCP_SUBUNIT_ID                     0x00

#define AVRCP_PASS_THROUGH_KEY_PRESS         0x00
#define AVRCP_PASS_THROUGH_KEY_RELEASE       0x80
#define AVRCP_PASS_THROUGH_NEXT_GROUP        0x00
#define AVRCP_PASS_THROUGH_PREVIOUS_GROUP    0x01

#define COMPANY_BT_SIG                       0x001958

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

/** Get Element Attributes */
#define MAX_ELEMENT_ATTR_NUM                 8
/** player application setting attributes*/
#define MAX_APP_SETTING_ATTR_NUM             4

#define AVRCP_HDR_LENGTH                     3

#define AVRCP_CATEGORY_1                     0x01
#define AVRCP_CATEGORY_2                     0x02
#define AVRCP_CATEGORY_3                     0x04
#define AVRCP_CATEGORY_4                     0x08

#define OPCODE_UNIT_INFO                     0x30
#define OPCODE_SUB_UNIT_INFO                 0x31
#define OPCODE_PASS_THROUGH                  0x7C
#define OPCODE_VENDOR_DEPENDENT              0x00

/** user defined for record cmd*/
#define PDU_ID_UNIT_INFO                     0xF0
#define PDU_ID_PASS_THROUGH                  0xF1

#define PASSTHROUGH_STATE_PRESSED            0
#define PASSTHROUGH_STATE_RELEASED           1

#define AVRCP_OP_ID_VENDOR_UNIQUE            0x7E

#define AVRCP_PKT_TYPE_NON_FRAGMENTED        0x00
#define AVRCP_PTK_TYPE_START                 0x01
#define AVRCP_PKT_TYPE_CONTINUE              0x02
#define AVRCP_PKT_TYPE_END                   0x03

/** AVRCP ctype */
#define AVRCP_CONTROL                        0x00
#define AVRCP_STATUS                         0x01
#define AVRCP_SPECIFIC_INQUIRY               0x02
#define AVRCP_NOTIFY                         0x03
#define AVRCP_GENERAL_INQUIRY                0x04

/** AVRCP Response */
#define AVRCP_NOT_IMPLEMENTED                0x08
#define AVRCP_ACCEPTED                       0x09
#define AVRCP_REJECTED                       0x0A
#define AVRCP_IN_TRANSITION                  0x0B
#define AVRCP_IMPLEMENTED                    0x0C
#define AVRCP_CHANGED                        0x0D
#define AVRCP_INTERIM                        0x0F

#define AVRCP_COVER_ART_TARGET_LEN           0x13
#define AVRCP_COVER_ART_TYPE_LEN             0x10
#define AVRCP_IMAGE_HANDLE_HEADER_ID         0x30
#define AVRCP_IMAGE_DESCRIPTION_HEADER_ID    0x71
#define AVRCP_IMAGE_HANDLE_LEN               0x10

/* Should be larger than local supported CT events number. */
#define AVRCP_MAX_QUEUED_CMD                 10

typedef enum t_bt_avrcp_element_attr_type
{
    ELEMENT_ATTR_TITLE             = 0x01,
    ELEMENT_ATTR_ARTIST            = 0x02,
    ELEMENT_ATTR_ALBUM             = 0x03,
    ELEMENT_ATTR_TRACK             = 0x04,
    ELEMENT_ATTR_TOTAL_TRACK       = 0x05,
    ELEMENT_ATTR_GENRE             = 0x06,
    ELEMENT_ATTR_PLAYING_TIME      = 0x07,
    ELEMENT_ATTR_DEFAULT_COVER_ART = 0x08,
} T_BT_AVRCP_ELEMENT_ATTR_TYPE;

typedef enum t_bt_avrcp_app_setting_attr_id
{
    APP_SETTING_ATTR_EQUALIZER        = 0x01,
    APP_SETTING_ATTR_REPEAT_MODE      = 0x02,
    APP_SETTING_ATTR_SHUFFLE          = 0x03,
    APP_SETTING_ATTR_SCAN             = 0x04,
} T_BT_AVRCP_APP_SETTING_ATTR_ID;

typedef enum t_bt_avrcp_app_setting_value_equalizer_status
{
    EQUALIZER_STATUS_OFF          = 0x01,
    EQUALIZER_STATUS_ON           = 0x02,
} T_BT_AVRCP_APP_SETTING_VALUE_EQUALIZER_STATUS;

typedef enum t_bt_avrcp_app_setting_value_repeat_mode_status
{
    REPEAT_MODE_STATUS_OFF        = 0x01,
    REPEAT_MODE_STATUS_SINGLE     = 0x02,
    REPEAT_MODE_STATUS_ALL        = 0x03,
    REPEAT_MODE_STATUS_GROUP      = 0x04,
} T_BT_AVRCP_APP_SETTING_VALUE_REPEAT_MODE_STATUS;

typedef enum t_bt_avrcp_app_setting_value_shuffle_status
{
    SHUFFLE_STATUS_OFF            = 0x01,
    SHUFFLE_STATUS_ALL            = 0x02,
    SHUFFLE_STATUS_GROUP          = 0x03,
} T_BT_AVRCP_APP_SETTING_VALUE_SHUFFLE_STATUS;

typedef enum t_bt_avrcp_app_setting_value_scan_status
{
    SCAN_STATUS_OFF               = 0x01,
    SCAN_STATUS_ALL               = 0x02,
    SCAN_STATUS_GROUP             = 0x03,
} T_BT_AVRCP_APP_SETTING_VALUE_SCAN_STATUS;

typedef enum t_capability_id
{
    CAPABILITY_ID_COMPANY_ID       = 0x02,
    CAPABILITY_ID_EVENTS_SUPPORTED = 0x03,
} T_CAPABILITY_ID;

typedef enum t_bt_avrcp_key
{
    BT_AVRCP_KEY_POWER         = 0x40,
    BT_AVRCP_KEY_VOL_UP        = 0x41,
    BT_AVRCP_KEY_VOL_DOWN      = 0x42,
    BT_AVRCP_KEY_MUTE          = 0x43,
    BT_AVRCP_KEY_PLAY          = 0x44,
    BT_AVRCP_KEY_STOP          = 0x45,
    BT_AVRCP_KEY_PAUSE         = 0x46,
    BT_AVRCP_KEY_REWIND        = 0x48,
    BT_AVRCP_KEY_FAST_FORWARD  = 0x49,
    BT_AVRCP_KEY_FORWARD       = 0x4B,
    BT_AVRCP_KEY_BACKWARD      = 0x4C,
} T_BT_AVRCP_KEY;

typedef enum t_bt_avrcp_rsp_state
{
    AVRCP_RSP_STATE_SUCCESS = 0x00,
    AVRCP_RSP_STATE_FAIL    = 0x01,
} T_BT_AVRCP_RSP_STATE;

typedef struct t_rsp_unit_info
{
    T_BT_AVRCP_RSP_STATE  state;
    uint8_t               sub_unit_type;
    uint8_t               sub_unit_id;
    uint32_t              company_id;
} T_RSP_UNIT_INFO;

typedef struct t_rsp_passthrough
{
    T_BT_AVRCP_RSP_STATE  state;
    T_BT_AVRCP_KEY        key;
    bool                  pressed;
} T_RSP_PASSTHROUGH;

typedef struct t_bt_avrcp_req_pass_through
{
    uint8_t  key;
    bool     pressed;
} T_BT_AVRCP_REQ_PASS_THROUGH;

typedef struct t_bt_avrcp_rsp_param_addressed_player_changed
{
    uint16_t player_id;
    uint16_t uid_counter;
} T_BT_AVRCP_RSP_PARAM_ADDRESSED_PLAYER_CHANGED;

typedef struct t_bt_avrcp_req_get_app_setting_value
{
    uint8_t  attr_num;
    uint8_t  attr_id[MAX_APP_SETTING_ATTR_NUM];
} T_BT_AVRCP_REQ_GET_APP_SETTING_VALUE;


typedef struct t_bt_avrcp_req_set_app_setting_value
{
    uint8_t                 attr_num;
    T_BT_AVRCP_APP_SETTING  app_setting[MAX_APP_SETTING_ATTR_NUM];
} T_BT_AVRCP_REQ_SET_APP_SETTING_VALUE;

typedef struct t_bt_avrcp_req_play_item
{
    T_BT_AVRCP_SCOPE_ID  scope_id;
    uint64_t             uid;
    uint16_t             uid_counter;
} T_BT_AVRCP_REQ_PLAY_ITEM;

typedef struct t_bt_avrcp_req_get_item_attrs
{
    uint8_t   scope;
    uint64_t  uid;
    uint16_t  uid_counter;
    uint8_t   num_of_attr;
    uint32_t  attr_id[MAX_ELEMENT_ATTR_NUM];
} T_BT_AVRCP_REQ_GET_ITEM_ATTRS;

typedef struct t_bt_avrcp_req_search
{
    uint16_t  length;
    uint8_t  *search_str;
} T_BT_AVRCP_REQ_SEARCH;

typedef struct t_bt_avrcp_req_change_path
{
    uint16_t uid_counter;
    uint8_t  direction;
    uint64_t folder_uid;
} T_BT_AVRCP_REQ_CHANGE_PATH;

typedef struct t_bt_avrcp_continuation
{
    uint8_t  *buf;
    uint16_t  write_index;
    uint16_t  recombine_length;
} T_BT_AVRCP_CONTINUATION;

typedef struct t_bt_avrcp_fragmentation
{
    uint8_t  *buf;
    uint8_t   pdu_id;
    uint16_t  read_index;
    uint16_t  total_length;
} T_BT_AVRCP_FRAGMENTATION;

typedef enum t_bt_avrcp_state
{
    BT_AVRCP_STATE_DISCONNECTED  = 0x00,
    BT_AVRCP_STATE_ALLOCATED     = 0x01,
    BT_AVRCP_STATE_CONNECTING    = 0x02,
    BT_AVRCP_STATE_CONNECTED     = 0x03,
    BT_AVRCP_STATE_DISCONNECTING = 0x04,
} T_BT_AVRCP_STATE;

typedef enum t_bt_avrcp_cover_art_state
{
    BT_AVRCP_COVER_ART_STATE_DISCONNECTED  = 0x00,
    BT_AVRCP_COVER_ART_STATE_CONNECTING    = 0x01,
    BT_AVRCP_COVER_ART_STATE_CONNECTED     = 0x02,
    BT_AVRCP_COVER_ART_STATE_DISCONNECTING = 0x03,
    BT_AVRCP_COVER_ART_STATE_GET_IMAGE     = 0x04,
    BT_AVRCP_COVER_ART_STATE_GET_THUMBNAIL = 0x05,
} T_BT_AVRCP_COVER_ART_STATE;

typedef struct t_bt_avrcp_ctrl_chann
{
    T_OS_QUEUE               cmd_queue;
    T_BT_AVRCP_STATE         state;
    uint16_t                 cid;
    uint8_t                  cmd_credits;
    uint8_t                  transact_label;
    T_BT_AVRCP_CONTINUATION  recombine;
    T_BT_AVRCP_FRAGMENTATION fragment;
    uint16_t                 remote_mtu;
    uint8_t                  vol_change_pending_transact;
    uint8_t                  play_status_change_pending_transact;
    uint8_t                  track_change_pending_transact;
    uint8_t                  addressed_player_change_pending_transact;
    uint8_t                  get_element_attr_pending_transact;
    uint8_t                  get_play_status_pending_transact;
    uint8_t                  set_addressed_player_pending_transact;
    uint8_t                  vendor_cmd_transact;
    T_SYS_TIMER_HANDLE       timer_handle;
    uint8_t                  last_pdu_id;
    uint8_t                  play_status;
} T_BT_AVRCP_CTRL_CHANN;

typedef struct t_bt_avrcp_browsing_chann
{
    T_OS_QUEUE               cmd_queue;
    T_BT_AVRCP_STATE         state;
    uint16_t                 cid;
    uint8_t                  cmd_credits;
    uint8_t                  transact_label;
    uint8_t                  pending_transact;
} T_BT_AVRCP_BROWSING_CHANN;

typedef struct t_bt_avrcp_cmd
{
    struct T_BT_AVRCP_CMD          *next;
    uint8_t                         pdu_id;
    union
    {
        uint8_t                               capability_id;
        uint8_t                               event_id;
        uint8_t                               attr_id;
        uint8_t                               pdu_id;
        uint8_t                               volume;
        uint16_t                              player_id;
        T_BT_AVRCP_REQ_GET_ELEMENT_ATTR       get_element_attrs;
        T_BT_AVRCP_REQ_PASS_THROUGH           pass_through;
        T_BT_AVRCP_REQ_GET_APP_SETTING_VALUE  get_app_setting_value;
        T_BT_AVRCP_REQ_SET_APP_SETTING_VALUE  set_app_setting_value;
        T_BT_AVRCP_REQ_PLAY_ITEM              play_item;
    } cmd_param;
} T_BT_AVRCP_CMD;

typedef struct t_bt_avrcp_browsing_cmd
{
    struct T_BT_AVRCP_BROWSING_CMD        *next;
    uint8_t                                pdu_id;
    union
    {
        T_BT_AVRCP_REQ_GET_FOLDER_ITEMS    get_folder_items;
        T_BT_AVRCP_REQ_GET_ITEM_ATTRS      get_item_attrs;
        T_BT_AVRCP_REQ_SEARCH              search;
        T_BT_AVRCP_REQ_CHANGE_PATH         change_path;
        uint16_t                           player_id;
    } cmd_param;
} T_BT_AVRCP_BROWSING_CMD;

typedef struct t_bt_avrcp_link
{
    struct t_bt_avrcp_link    *next;
    uint8_t                    bd_addr[6];
    T_BT_AVRCP_CTRL_CHANN      ctrl_chann;
    T_BT_AVRCP_BROWSING_CHANN  browsing_chann;
    T_OBEX_HANDLE              handle;
    T_BT_AVRCP_COVER_ART_STATE cover_art_state;
} T_BT_AVRCP_LINK;

typedef struct t_bt_avrcp
{
    T_OS_QUEUE      list;
    uint32_t        local_company_id;
    uint16_t        wait_rsp_tout;
    uint8_t         ct_features;
    uint8_t         tg_features;
} T_BT_AVRCP;

static T_BT_AVRCP *bt_avrcp;

#if (CONFIG_REALTEK_BTM_AVRCP_COVER_ART_SUPPORT == 1)
const uint8_t avrcp_cover_art_target[AVRCP_COVER_ART_TARGET_LEN + 1] =
{
    AVRCP_COVER_ART_TARGET_LEN,
    OBEX_HI_TARGET,
    (uint8_t)(AVRCP_COVER_ART_TARGET_LEN >> 8),
    (uint8_t)AVRCP_COVER_ART_TARGET_LEN,
    /* cover art uuid */
    0x71, 0x63, 0xDD, 0x54, 0x4A, 0x7E, 0x11, 0xE2, 0xB4, 0x7C, 0x00, 0x50, 0xC2, 0x49, 0x00, 0x48
};

/* NULL terminated ASCII : x-bt/img-img */
const uint8_t image_type[AVRCP_COVER_ART_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(AVRCP_COVER_ART_TYPE_LEN >> 8),
    (uint8_t)AVRCP_COVER_ART_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x69, 0x6d, 0x67, 0x2d, 0x69, 0x6d, 0x67, 0x00
};

/* NULL terminated ASCII : x-bt/img-thm */
const uint8_t image_thumbnail_type[AVRCP_COVER_ART_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(AVRCP_COVER_ART_TYPE_LEN >> 8),
    (uint8_t)AVRCP_COVER_ART_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x69, 0x6d, 0x67, 0x2d, 0x74, 0x68, 0x6d, 0x00
};
#endif

void bt_avrcp_tout_callback(T_SYS_TIMER_HANDLE handle);

T_BT_AVRCP_LINK *bt_avrcp_alloc_link(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = calloc(1, sizeof(T_BT_AVRCP_LINK));
    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);
        link->ctrl_chann.state = BT_AVRCP_STATE_ALLOCATED;
        link->browsing_chann.state = BT_AVRCP_STATE_ALLOCATED;
        link->ctrl_chann.timer_handle = sys_timer_create("avrcp_wait_rsp",
                                                         SYS_TIMER_TYPE_LOW_PRECISION,
                                                         (uint32_t)link,
                                                         bt_avrcp->wait_rsp_tout * 1000,
                                                         false,
                                                         bt_avrcp_tout_callback);
        os_queue_in(&bt_avrcp->list, link);
    }

    return link;
}

void bt_avrcp_free_link(T_BT_AVRCP_LINK *link)
{
    if (os_queue_delete(&bt_avrcp->list, link) == true)
    {
        if (link->ctrl_chann.timer_handle != NULL)
        {
            sys_timer_delete(link->ctrl_chann.timer_handle);
        }

        if (link->ctrl_chann.recombine.buf != NULL)
        {
            free(link->ctrl_chann.recombine.buf);
        }

        free(link);
    }
}

T_BT_AVRCP_LINK *bt_avrcp_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = os_queue_peek(&bt_avrcp->list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

T_BT_AVRCP_LINK *bt_avrcp_find_link_by_cid(uint16_t cid)
{
    T_BT_AVRCP_LINK *link;

    link = os_queue_peek(&bt_avrcp->list, 0);
    while (link != NULL)
    {
        if (link->ctrl_chann.cid == cid || link->browsing_chann.cid == cid)
        {
            break;
        }

        link = link->next;
    }

    return link;
}

bool bt_avrcp_send_generic_cmd(T_BT_AVRCP_LINK *link,
                               uint8_t         *buf,
                               uint16_t         length)
{
    link->ctrl_chann.transact_label = (link->ctrl_chann.transact_label + 1) & 0x0f;
    if (avctp_data_send(link->bd_addr, buf, length, NULL, 0,
                        link->ctrl_chann.transact_label, AVCTP_MSG_TYPE_CMD))
    {
        link->ctrl_chann.cmd_credits = 0;
        sys_timer_start(link->ctrl_chann.timer_handle);
        return true;
    }

    return false;
}

bool bt_avrcp_send_vendor_BT_SIG_cmd(T_BT_AVRCP_LINK *link,
                                     uint8_t          ctype,
                                     uint8_t          pdu_id,
                                     uint8_t         *param,
                                     uint16_t         param_len)
{
    PROFILE_PRINT_INFO2("bt_avrcp_send_vendor_BT_SIG_cmd: ctype 0x%02x,CONTROL pdu_id 0x%02x",
                        ctype, pdu_id);

    uint8_t  buf[10];
    uint8_t *p;

    link->ctrl_chann.transact_label = (link->ctrl_chann.transact_label + 1) & 0x0f;

    p = buf;
    BE_UINT8_TO_STREAM(p, 0xf & ctype);
    BE_UINT8_TO_STREAM(p, (AVRCP_SUBUNIT_TYPE_PANEL << 3) | (AVRCP_SUBUNIT_ID & 0x7));
    BE_UINT8_TO_STREAM(p, OPCODE_VENDOR_DEPENDENT);
    BE_UINT24_TO_STREAM(p, COMPANY_BT_SIG);
    BE_UINT8_TO_STREAM(p, pdu_id);
    BE_UINT8_TO_STREAM(p, AVRCP_PKT_TYPE_NON_FRAGMENTED & 0x3);
    BE_UINT16_TO_STREAM(p, param_len);

    if (avctp_data_send(link->bd_addr, buf, sizeof(buf), param,
                        param_len, link->ctrl_chann.transact_label, AVCTP_MSG_TYPE_CMD))
    {
        link->ctrl_chann.cmd_credits = 0;
        sys_timer_start(link->ctrl_chann.timer_handle);

        return true;
    }

    return false;
}

bool bt_avrcp_send_cmd_pass_through(T_BT_AVRCP_LINK *link,
                                    uint8_t          key,
                                    bool             pressed)
{
    PROFILE_PRINT_INFO2("bt_avrcp_send_cmd_pass_through: key 0x%02x, press %d", key, pressed);

    uint8_t  buf[10];
    uint8_t *p;

    p = buf;
    BE_UINT8_TO_STREAM(p, AVRCP_CONTROL);
    BE_UINT8_TO_STREAM(p, (AVRCP_SUBUNIT_TYPE_PANEL << 3) | (AVRCP_SUBUNIT_ID & 0x7));
    BE_UINT8_TO_STREAM(p, OPCODE_PASS_THROUGH);
    if (key == AVRCP_PASS_THROUGH_NEXT_GROUP || key == AVRCP_PASS_THROUGH_PREVIOUS_GROUP)
    {
        BE_UINT8_TO_STREAM(p, (pressed ? PASSTHROUGH_STATE_PRESSED : PASSTHROUGH_STATE_RELEASED) << 7 |
                           AVRCP_OP_ID_VENDOR_UNIQUE);
        BE_UINT8_TO_STREAM(p, 0x5);
        BE_UINT24_TO_STREAM(p, COMPANY_BT_SIG);
        BE_UINT8_TO_STREAM(p, 0);
        BE_UINT8_TO_STREAM(p, key);
    }
    else
    {
        BE_UINT8_TO_STREAM(p, (pressed ? PASSTHROUGH_STATE_PRESSED : PASSTHROUGH_STATE_RELEASED) << 7 |
                           (key & 0x7f));
        BE_UINT8_TO_STREAM(p, 0);
    }

    return bt_avrcp_send_generic_cmd(link, buf, p - buf);
}

void bt_avrcp_cmd_process(T_BT_AVRCP_LINK *link)
{
    T_BT_AVRCP_CMD *cmd;
    int32_t         ret = 0;
    bool            res = false;

    if (link->ctrl_chann.state != BT_AVRCP_STATE_CONNECTED)
    {
        ret = 1;
        goto fail_invalid_state;
    }

    if (link->ctrl_chann.cmd_credits == 0)
    {
        ret = 2;
        goto fail_no_credits;
    }

    cmd = os_queue_out(&link->ctrl_chann.cmd_queue);
    if (cmd != NULL)
    {
        switch (cmd->pdu_id)
        {
        case PDU_ID_GET_CAPABILITIES:
            {
                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_STATUS,
                                                      PDU_ID_GET_CAPABILITIES,
                                                      &cmd->cmd_param.capability_id,
                                                      1);
            }
            break;

        case PDU_ID_GET_ELEMENT_ATTRS:
            {
                uint8_t   pdu_para[9 + (MAX_ELEMENT_ATTR_NUM * 4)] = {0};
                uint8_t   pdu_length;
                uint32_t  attr_tmp;
                uint8_t   i;
                uint8_t  *p;

                pdu_length = 9 + 4 * cmd->cmd_param.get_element_attrs.attr_num;
                pdu_para[8] = cmd->cmd_param.get_element_attrs.attr_num;
                p = &pdu_para[9];

                for (i = 0; i < cmd->cmd_param.get_element_attrs.attr_num; i++)
                {
                    attr_tmp = cmd->cmd_param.get_element_attrs.attr_id[i];
                    BE_UINT32_TO_STREAM(p, attr_tmp);
                }

                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_STATUS,
                                                      PDU_ID_GET_ELEMENT_ATTRS,
                                                      pdu_para,
                                                      pdu_length);
            }
            break;

        case PDU_ID_GET_PLAY_STATUS:
            {
                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_STATUS,
                                                      PDU_ID_GET_PLAY_STATUS,
                                                      NULL,
                                                      0);
            }
            break;

        case PDU_ID_SET_ABSOLUTE_VOLUME:
            {
                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_CONTROL,
                                                      PDU_ID_SET_ABSOLUTE_VOLUME,
                                                      &cmd->cmd_param.volume,
                                                      1);
            }
            break;

        case PDU_ID_REGISTER_NOTIFICATION:
            {
                uint8_t pdu_para[] =
                {
                    cmd->cmd_param.event_id,
                    0, 0, 0, 0
                };
                if (cmd->cmd_param.event_id == BT_AVRCP_EVENT_ID_PLAYBACK_POS_CHANGED)
                {
                    pdu_para[4] = 1;
                }

                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_NOTIFY,
                                                      PDU_ID_REGISTER_NOTIFICATION,
                                                      pdu_para,
                                                      sizeof(pdu_para));
            }
            break;

        case PDU_ID_LIST_APP_SETTING_ATTRS:
            {
                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_STATUS,
                                                      PDU_ID_LIST_APP_SETTING_ATTRS,
                                                      NULL,
                                                      0);
            }
            break;

        case PDU_ID_LIST_APP_SETTING_VALUES:
            {
                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_STATUS,
                                                      PDU_ID_LIST_APP_SETTING_VALUES,
                                                      &cmd->cmd_param.attr_id,
                                                      1);
            }
            break;

        case PDU_ID_GET_CURRENT_APP_SETTING_VALUE:
            {
                uint8_t pdu_para[1 + MAX_APP_SETTING_ATTR_NUM] = {0};
                uint8_t pdu_length;
                uint8_t i;

                pdu_length = 1 + cmd->cmd_param.get_app_setting_value.attr_num;
                pdu_para[0] = cmd->cmd_param.get_app_setting_value.attr_num;

                for (i = 0; i < cmd->cmd_param.get_app_setting_value.attr_num; i++)
                {
                    pdu_para[1 + i] = cmd->cmd_param.get_app_setting_value.attr_id[i];
                }

                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_STATUS,
                                                      PDU_ID_GET_CURRENT_APP_SETTING_VALUE,
                                                      pdu_para,
                                                      pdu_length);
            }
            break;

        case PDU_ID_SET_APP_SETTING_VALUE:
            {
                uint8_t pdu_para[1 + (MAX_APP_SETTING_ATTR_NUM * 2)] = {0};
                uint8_t pdu_length;
                uint8_t i;

                pdu_length = 1 + cmd->cmd_param.set_app_setting_value.attr_num * 2;
                pdu_para[0] = cmd->cmd_param.set_app_setting_value.attr_num;

                for (i = 0; i < cmd->cmd_param.set_app_setting_value.attr_num; i++)
                {
                    pdu_para[i * 2 + 1] = cmd->cmd_param.set_app_setting_value.app_setting[i].attr;
                    pdu_para[i * 2 + 2] = cmd->cmd_param.set_app_setting_value.app_setting[i].value;
                }

                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_CONTROL,
                                                      PDU_ID_SET_APP_SETTING_VALUE,
                                                      pdu_para,
                                                      pdu_length);
            }
            break;

        case PDU_ID_REQUEST_CONTINUE_RSP:
            {
                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_CONTROL,
                                                      PDU_ID_REQUEST_CONTINUE_RSP,
                                                      &cmd->cmd_param.pdu_id,
                                                      1);
            }
            break;

        case PDU_ID_ABORT_CONTINUE_RSP:
            {
                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_CONTROL,
                                                      PDU_ID_ABORT_CONTINUE_RSP,
                                                      &cmd->cmd_param.pdu_id,
                                                      1);
            }
            break;

        case PDU_ID_SET_ADDRESSED_PLAYER:
            {
                uint8_t buf[2];

                BE_UINT16_TO_ARRAY(buf, cmd->cmd_param.player_id);
                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_CONTROL,
                                                      PDU_ID_SET_ADDRESSED_PLAYER,
                                                      buf,
                                                      sizeof(buf));
            }
            break;

        case PDU_ID_PLAY_ITEM:
            {
                uint8_t  buf[11];
                uint8_t *p;

                p = buf;
                BE_UINT8_TO_STREAM(p, cmd->cmd_param.play_item.scope_id);
                BE_UINT64_TO_STREAM(p, cmd->cmd_param.play_item.uid);
                BE_UINT16_TO_STREAM(p, cmd->cmd_param.play_item.uid_counter);

                res = bt_avrcp_send_vendor_BT_SIG_cmd(link,
                                                      AVRCP_CONTROL,
                                                      PDU_ID_PLAY_ITEM,
                                                      buf,
                                                      sizeof(buf));
            }
            break;

        case PDU_ID_UNIT_INFO:
            {
                uint8_t buf[] =
                {
                    AVRCP_STATUS,
                    0xff,
                    OPCODE_UNIT_INFO,
                    0xff, 0xff, 0xff, 0xff, 0xff
                };

                res = bt_avrcp_send_generic_cmd(link, buf, sizeof(buf));
            }
            break;

        case PDU_ID_PASS_THROUGH:
            {
                res = bt_avrcp_send_cmd_pass_through(link, cmd->cmd_param.pass_through.key,
                                                     cmd->cmd_param.pass_through.pressed);
            }
            break;

        default:
            break;
        }

        if (res == false)
        {
            ret = 3;
            goto fail_send_cmd;
        }

        if (cmd->pdu_id != PDU_ID_UNIT_INFO &&
            cmd->pdu_id != PDU_ID_PASS_THROUGH)
        {
            link->ctrl_chann.last_pdu_id = cmd->pdu_id;
        }

        link->ctrl_chann.cmd_credits = 0;
        free(cmd);
    }

    return;

fail_send_cmd:
    free(cmd);
fail_no_credits:
fail_invalid_state:
    PROFILE_PRINT_ERROR1("bt_avrcp_cmd_process: failed %d", -ret);
}

bool bt_avrcp_cmd_enqueue(T_BT_AVRCP_LINK *link,
                          uint8_t          pdu_id,
                          void            *param)
{
    T_BT_AVRCP_CMD *cmd;
    int32_t         ret = 0;

    if (link->ctrl_chann.cmd_queue.count == AVRCP_MAX_QUEUED_CMD)
    {
        ret = 1;
        goto fail_queue_full;
    }

    cmd = calloc(1, sizeof(T_BT_AVRCP_CMD));
    if (cmd == NULL)
    {
        ret = 2;
        goto fail_alloc_mem;
    }

    cmd->pdu_id = pdu_id;
    switch (pdu_id)
    {
    case PDU_ID_GET_CAPABILITIES:
        cmd->cmd_param.capability_id = *(uint8_t *)param;
        break;

    case PDU_ID_GET_ELEMENT_ATTRS:
        memcpy(&cmd->cmd_param.get_element_attrs, param, sizeof(T_BT_AVRCP_REQ_GET_ELEMENT_ATTR));
        break;

    case PDU_ID_GET_PLAY_STATUS:
        break;

    case PDU_ID_SET_ABSOLUTE_VOLUME:
        cmd->cmd_param.volume = *(uint8_t *)param;
        break;

    case PDU_ID_REGISTER_NOTIFICATION:
        cmd->cmd_param.event_id = *(uint8_t *)param;
        break;

    case PDU_ID_LIST_APP_SETTING_ATTRS:
        break;

    case PDU_ID_LIST_APP_SETTING_VALUES:
        cmd->cmd_param.attr_id = *(uint8_t *)param;
        break;

    case PDU_ID_GET_CURRENT_APP_SETTING_VALUE:
        memcpy(&cmd->cmd_param.get_app_setting_value, param,
               sizeof(T_BT_AVRCP_REQ_GET_APP_SETTING_VALUE));
        break;

    case PDU_ID_SET_APP_SETTING_VALUE:
        memcpy(&cmd->cmd_param.set_app_setting_value, param,
               sizeof(T_BT_AVRCP_REQ_SET_APP_SETTING_VALUE));
        break;

    case PDU_ID_REQUEST_CONTINUE_RSP:
        cmd->cmd_param.pdu_id = *(uint8_t *)param;
        break;

    case PDU_ID_ABORT_CONTINUE_RSP:
        cmd->cmd_param.pdu_id = *(uint8_t *)param;
        break;

    case PDU_ID_SET_ADDRESSED_PLAYER:
        cmd->cmd_param.player_id = *(uint16_t *)param;
        break;

    case PDU_ID_PLAY_ITEM:
        memcpy(&cmd->cmd_param.play_item, param, sizeof(T_BT_AVRCP_REQ_PLAY_ITEM));
        break;

    case PDU_ID_UNIT_INFO:
        break;

    case PDU_ID_PASS_THROUGH:
        memcpy(&cmd->cmd_param.pass_through, param, sizeof(T_BT_AVRCP_REQ_PASS_THROUGH));
        break;

    default:
        {
            ret = 3;
            goto fail_invalid_pdu;
        }
    }

    os_queue_in(&link->ctrl_chann.cmd_queue, cmd);
    bt_avrcp_cmd_process(link);
    return true;

fail_invalid_pdu:
    if (cmd != NULL)
    {
        free(cmd);
    }
fail_alloc_mem:
fail_queue_full:
    PROFILE_PRINT_ERROR1("bt_avrcp_cmd_enqueue: failed %d", -ret);
    return false;
}

void bt_avrcp_tout_callback(T_SYS_TIMER_HANDLE handle)
{
    T_BT_AVRCP_LINK *link;

    link = (void *)sys_timer_id_get(handle);
    if (link != NULL)
    {
        PROFILE_PRINT_WARN1("bt_avrcp_tout_callback: addr %s", TRACE_BDADDR(link->bd_addr));

        link->ctrl_chann.cmd_credits = 1;
        bt_avrcp_cmd_process(link);
    }
}

bool bt_avrcp_register_notification_req(uint8_t bd_addr[6],
                                        uint8_t event_id)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if ((event_id != BT_AVRCP_EVENT_ID_VOLUME_CHANGED) ||
            (bt_avrcp->ct_features & AVRCP_CATEGORY_2))
        {
            return bt_avrcp_cmd_enqueue(link, PDU_ID_REGISTER_NOTIFICATION, &event_id);
        }
    }

    return false;
}

bool bt_avrcp_get_capability(uint8_t bd_addr[6],
                             uint8_t capability_id)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        return bt_avrcp_cmd_enqueue(link, PDU_ID_GET_CAPABILITIES, &capability_id);
    }

    return false;
}

bool bt_avrcp_vendor_cmd_send(uint8_t           bd_addr[6],
                              T_BT_AVRCP_CTYPE  ctype,
                              uint32_t          company_id,
                              uint8_t          *pdu,
                              uint16_t          pdu_length)
{
    uint8_t          buf[6];
    uint8_t         *p;
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    p = buf;
    BE_UINT8_TO_STREAM(p, ctype & 0xf);
    BE_UINT8_TO_STREAM(p, (AVRCP_SUBUNIT_TYPE_VENDOR_UNIQUE << 3) | (AVRCP_SUBUNIT_ID & 0x7));
    BE_UINT8_TO_STREAM(p, OPCODE_VENDOR_DEPENDENT);
    BE_UINT24_TO_STREAM(p, company_id);

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_find_link;
    }

    if (link->ctrl_chann.cmd_credits == 0)
    {
        ret = 2;
        goto fail_no_credits;
    }

    bt_sniff_mode_exit(bd_addr, false);
    link->ctrl_chann.transact_label = (link->ctrl_chann.transact_label + 1) & 0x0f;
    if (avctp_data_send(link->bd_addr, buf, sizeof(buf), pdu, pdu_length,
                        link->ctrl_chann.transact_label, AVCTP_MSG_TYPE_CMD) == false)
    {
        ret = 3;
        goto fail_send_cmd;
    }

    link->ctrl_chann.cmd_credits = 0;
    sys_timer_start(link->ctrl_chann.timer_handle);
    return true;

fail_send_cmd:
fail_no_credits:
fail_find_link:
    PROFILE_PRINT_ERROR1("bt_avrcp_vendor_cmd_send: failed %d", -ret);
    return false;
}

bool bt_avrcp_vendor_rsp_send(uint8_t              bd_addr[6],
                              T_BT_AVRCP_RESPONSE  response,
                              uint32_t             company_id,
                              uint8_t             *pdu,
                              uint16_t             pdu_length)
{
    uint8_t          buf[6];
    uint8_t         *p;
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    p = buf;
    BE_UINT8_TO_STREAM(p, response & 0xf);
    BE_UINT8_TO_STREAM(p, (AVRCP_SUBUNIT_TYPE_VENDOR_UNIQUE << 3) | (AVRCP_SUBUNIT_ID & 0x7));
    BE_UINT8_TO_STREAM(p, OPCODE_VENDOR_DEPENDENT);
    BE_UINT24_TO_STREAM(p, company_id);

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_find_link;
    }

    if (link->ctrl_chann.vendor_cmd_transact == 0xff)
    {
        ret = 2;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    if (avctp_data_send(link->bd_addr, buf, sizeof(buf),
                        pdu, pdu_length, link->ctrl_chann.vendor_cmd_transact,
                        AVCTP_MSG_TYPE_RSP) == false)
    {
        ret = 3;
        goto fail_send_cmd;

    }

    link->ctrl_chann.vendor_cmd_transact = 0xff;
    return true;

fail_send_cmd:
fail_no_pending_transact:
fail_find_link:
    PROFILE_PRINT_ERROR1("bt_avrcp_vendor_rsp_send: failed %d", -ret);
    return false;
}

void bt_avrcp_send_vendor_BT_SIG_rsp(T_BT_AVRCP_LINK *link,
                                     uint8_t          transact,
                                     uint8_t          response,
                                     uint8_t          pdu_id,
                                     uint8_t         *param,
                                     uint16_t         param_len)
{
    uint16_t  mtu_size;
    uint8_t   pkt_type;
    uint8_t   buf[10];
    uint8_t  *p;

    PROFILE_PRINT_TRACE1("bt_avrcp_send_vendor_BT_SIG_rsp: CONTROL pdu_id 0x%02x", pdu_id);

    /* avctp header(3) + avrcp header before pdu param(10) */
    mtu_size = link->ctrl_chann.remote_mtu - 13;

    if (param_len <= mtu_size)
    {
        if (link->ctrl_chann.fragment.read_index == 0)
        {
            pkt_type = AVRCP_PKT_TYPE_NON_FRAGMENTED;
        }
        else
        {
            pkt_type = AVRCP_PKT_TYPE_END;
        }
    }
    else
    {
        if (link->ctrl_chann.fragment.read_index == 0)
        {
            pkt_type = AVRCP_PTK_TYPE_START;
        }
        else
        {
            pkt_type = AVRCP_PKT_TYPE_CONTINUE;
        }

        link->ctrl_chann.fragment.pdu_id = pdu_id;
        link->ctrl_chann.fragment.read_index += mtu_size;
        param_len = mtu_size;
    }

    p = buf;
    BE_UINT8_TO_STREAM(p, response & 0xf);
    BE_UINT8_TO_STREAM(p, (AVRCP_SUBUNIT_TYPE_PANEL << 3) | (AVRCP_SUBUNIT_ID & 0x7));
    BE_UINT8_TO_STREAM(p, OPCODE_VENDOR_DEPENDENT);
    BE_UINT24_TO_STREAM(p, COMPANY_BT_SIG);
    BE_UINT8_TO_STREAM(p, pdu_id);
    BE_UINT8_TO_STREAM(p, pkt_type & 0x3);
    BE_UINT16_TO_STREAM(p, param_len);

    avctp_data_send(link->bd_addr, buf, sizeof(buf),
                    param, param_len, transact, AVCTP_MSG_TYPE_RSP);

    if (pkt_type == AVRCP_PKT_TYPE_NON_FRAGMENTED ||
        pkt_type == AVRCP_PKT_TYPE_END)
    {
        if (link->ctrl_chann.fragment.buf != NULL)
        {
            free(link->ctrl_chann.fragment.buf);
            memset(&link->ctrl_chann.fragment, 0, sizeof(link->ctrl_chann.fragment));
        }
    }
}

bool bt_avrcp_send_unit_info(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        return bt_avrcp_cmd_enqueue(link, PDU_ID_UNIT_INFO, NULL);
    }

    return false;
}

void bt_avrcp_handle_vendor_BT_SIG_rsp(T_BT_AVRCP_LINK *link,
                                       uint8_t         *pdu,
                                       uint16_t         pdu_length,
                                       uint8_t          response)
{
    uint8_t          *p;
    uint8_t           pdu_id;
    uint8_t           packet_type;
    uint16_t          param_len;
    T_BT_MSG_PAYLOAD  payload;

    p = pdu;
    BE_STREAM_TO_UINT8(pdu_id, p);
    BE_STREAM_TO_UINT8(packet_type, p);
    packet_type &= 0x3;
    BE_STREAM_TO_UINT16(param_len, p);

    if (packet_type == AVRCP_PKT_TYPE_NON_FRAGMENTED)
    {
        /* last recombination not completed, abort it. */
        if (link->ctrl_chann.recombine.buf != NULL)
        {
            free(link->ctrl_chann.recombine.buf);
            memset(&link->ctrl_chann.recombine, 0, sizeof(link->ctrl_chann.recombine));
        }
    }
    else
    {
        uint8_t *buf_tmp;

        /* AVRCP not define specific length of fragment packets.
          Re-allocate memory for each fragmentation.*/
        link->ctrl_chann.recombine.recombine_length += param_len;
        buf_tmp = malloc(link->ctrl_chann.recombine.recombine_length);
        if (buf_tmp == NULL)
        {
            bt_avrcp_cmd_enqueue(link, PDU_ID_ABORT_CONTINUE_RSP, &pdu_id);
            return;
        }
        else
        {
            if (link->ctrl_chann.recombine.buf != NULL)
            {
                memcpy(buf_tmp, link->ctrl_chann.recombine.buf, link->ctrl_chann.recombine.write_index);
                free(link->ctrl_chann.recombine.buf);
            }
            else /* First fragment packet */
            {
                /* recv CONTINUE/END without START*/
                if (packet_type != AVRCP_PTK_TYPE_START)
                {
                    bt_avrcp_cmd_enqueue(link, PDU_ID_ABORT_CONTINUE_RSP, &pdu_id);
                    free(buf_tmp);
                    return;
                }
            }

            link->ctrl_chann.recombine.buf = buf_tmp;
        }

        memcpy(link->ctrl_chann.recombine.buf + link->ctrl_chann.recombine.write_index, p, param_len);
        link->ctrl_chann.recombine.write_index += param_len;

        if (packet_type != AVRCP_PKT_TYPE_END)
        {
            bt_avrcp_cmd_enqueue(link, PDU_ID_REQUEST_CONTINUE_RSP, &pdu_id);
            return;
        }
        else
        {
            p = link->ctrl_chann.recombine.buf;
            param_len = link->ctrl_chann.recombine.write_index;
        }
    }

    PROFILE_PRINT_TRACE2("bt_avrcp_handle_vendor_BT_SIG_rsp: packet_type 0x%02x, pdu_id 0x%02x",
                         packet_type, pdu_id);

    memcpy(payload.bd_addr, link->bd_addr, 6);

    switch (pdu_id)
    {
    case PDU_ID_GET_CAPABILITIES:
        {
            T_BT_AVRCP_RSP_GET_CAPABILITIES tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                p++;
                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                BE_STREAM_TO_UINT8(tmp.capability_count, p);
                tmp.capabilities = p;
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
            }

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_GET_CAPABILITIES_RSP, &payload);
        }
        break;

    case PDU_ID_GET_ELEMENT_ATTRS:
        {
            T_BT_AVRCP_RSP_GET_ELEMENT_ATTR tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                T_BT_AVRCP_ELEMENT_ATTR *attr;

                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                BE_STREAM_TO_UINT8(tmp.num_of_attr, p);

                attr = malloc(sizeof(T_BT_AVRCP_ELEMENT_ATTR) * tmp.num_of_attr);
                if (attr != NULL)
                {
                    for (int i = 0; i < tmp.num_of_attr; i++)
                    {
                        BE_STREAM_TO_UINT32(attr[i].attribute_id, p);
                        BE_STREAM_TO_UINT16(attr[i].character_set_id, p);
                        BE_STREAM_TO_UINT16(attr[i].length, p);
                        attr[i].p_buf = p;
                        p += attr[i].length;
                    }

                    tmp.attr = attr;
                    payload.msg_buf = &tmp;
                    bt_mgr_dispatch(BT_MSG_AVRCP_GET_ELEM_ATTR_RSP, &payload);
                    free(attr);
                }
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
                payload.msg_buf = &tmp;
                bt_mgr_dispatch(BT_MSG_AVRCP_GET_ELEM_ATTR_RSP, &payload);
            }
        }
        break;

    case PDU_ID_GET_PLAY_STATUS:
        {
            T_BT_AVRCP_RSP_GET_PLAY_STATUS tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                BE_STREAM_TO_UINT32(tmp.length_ms, p);
                BE_STREAM_TO_UINT32(tmp.position_ms, p);
                BE_STREAM_TO_UINT8(tmp.play_status, p);
                link->ctrl_chann.play_status = tmp.play_status;
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
            }

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_GET_PLAY_STATUS_RSP, &payload);
        }
        break;

    case PDU_ID_REGISTER_NOTIFICATION:
        {
            if (response == AVRCP_INTERIM)
            {
                uint8_t  event_id;

                BE_STREAM_TO_UINT8(event_id, p);
                switch (event_id)
                {
                case BT_AVRCP_EVENT_ID_PLAYBACK_STATUS_CHANGED:
                    {
                        uint8_t play_status;

                        BE_STREAM_TO_UINT8(play_status, p);

                        if (link->ctrl_chann.play_status != play_status)
                        {
                            link->ctrl_chann.play_status = play_status;
                            payload.msg_buf = &link->ctrl_chann.play_status;
                            bt_mgr_dispatch(BT_MSG_AVRCP_PLAY_STATUS_CHANGED, &payload);
                        }
                    }
                    break;

                case BT_AVRCP_EVENT_ID_TRACK_CHANGED:
                    {
                        /* uint64_t track_id;

                        BE_STREAM_TO_UINT64(track_id, p);*/
                    }
                    break;

                case BT_AVRCP_EVENT_ID_PLAYER_APPLICATION_SETTING_CHANGED:
                    {
                        T_BT_AVRCP_RSP_APP_SETTING_CHANGED  app_settings;
                        T_BT_AVRCP_APP_SETTING *settings;

                        BE_STREAM_TO_UINT8(app_settings.num_of_attr, p);

                        settings = malloc(sizeof(T_BT_AVRCP_APP_SETTING) * app_settings.num_of_attr);
                        if (settings != NULL)
                        {
                            for (uint8_t i = 0; i < app_settings.num_of_attr; i++)
                            {
                                BE_STREAM_TO_UINT8(settings[i].attr, p);
                                BE_STREAM_TO_UINT8(settings[i].value, p);
                            }

                            app_settings.p_app_setting = settings;
                            payload.msg_buf = &app_settings;
                            bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_CHANGED, &payload);
                            free(settings);
                        }
                    }
                    break;

                case BT_AVRCP_EVENT_ID_NOW_PLAYING_CONTENT_CHANGED:
                    break;

                case BT_AVRCP_EVENT_ID_AVAILABLE_PLAYERS_CHANGED:
                    break;

                case BT_AVRCP_EVENT_ID_ADDRESSED_PLAYER_CHANGED:
                    {
                        /* T_BT_AVRCP_RSP_PARAM_ADDRESSED_PLAYER_CHANGED  addressed_player;

                        BE_STREAM_TO_UINT16(addressed_player.player_id, p);
                        BE_STREAM_TO_UINT16(addressed_player.uid_counter, p);*/
                    }
                    break;

                case BT_AVRCP_EVENT_ID_UIDS_CHANGED:
                    {
                        /* uint16_t  uid_counter;

                        BE_STREAM_TO_UINT16(uid_counter, p);;*/
                    }
                    break;

                case BT_AVRCP_EVENT_ID_VOLUME_CHANGED:
                    {
                        uint8_t  volume;

                        volume = (*p++) & 0x7f;

                        payload.msg_buf = &volume;
                        bt_mgr_dispatch(BT_MSG_AVRCP_VOLUME_CHANGED, &payload);
                    }
                    break;

                default:
                    break;
                }
            }
            else if (response == AVRCP_CHANGED)
            {
                uint8_t  event_id;

                BE_STREAM_TO_UINT8(event_id, p);
                switch (event_id)
                {
                case BT_AVRCP_EVENT_ID_PLAYBACK_STATUS_CHANGED:
                    {
                        BE_STREAM_TO_UINT8(link->ctrl_chann.play_status, p);
                        payload.msg_buf = &link->ctrl_chann.play_status;
                        bt_mgr_dispatch(BT_MSG_AVRCP_PLAY_STATUS_CHANGED, &payload);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_TRACK_CHANGED:
                    {
                        uint64_t track_id;

                        BE_ARRAY_TO_UINT64(track_id, p);
                        payload.msg_buf = &track_id;
                        bt_mgr_dispatch(BT_MSG_AVRCP_TRACK_CHANGED, &payload);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_TRACK_REACHED_END:
                    {
                        bt_mgr_dispatch(BT_MSG_AVRCP_TRACK_REACHED_END, NULL);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_TRACK_REACHED_START:
                    {
                        bt_mgr_dispatch(BT_MSG_AVRCP_TRACK_REACHED_START, NULL);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_PLAYBACK_POS_CHANGED:
                    {
                        uint32_t playback_pos;

                        BE_ARRAY_TO_UINT32(playback_pos, p);
                        payload.msg_buf = &playback_pos;
                        bt_mgr_dispatch(BT_MSG_AVRCP_PLAYBACK_POS_CHANGED, &payload);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_BATT_STATUS_CHANGED:
                    {
                        uint8_t batt_status;

                        BE_STREAM_TO_UINT8(batt_status, p);
                        payload.msg_buf = &batt_status;
                        bt_mgr_dispatch(BT_MSG_AVRCP_BATT_STATUS_CHANGED, &payload);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_SYSTEM_STATUS_CHANGED:
                    {
                        uint8_t system_status;

                        BE_STREAM_TO_UINT8(system_status, p);
                        payload.msg_buf = &system_status;
                        bt_mgr_dispatch(BT_MSG_AVRCP_SYSTEM_STATUS_CHANGED, &payload);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_PLAYER_APPLICATION_SETTING_CHANGED:
                    {
                        T_BT_AVRCP_RSP_APP_SETTING_CHANGED  app_settings;
                        T_BT_AVRCP_APP_SETTING *settings;

                        BE_STREAM_TO_UINT8(app_settings.num_of_attr, p);

                        settings = malloc(sizeof(T_BT_AVRCP_APP_SETTING) * app_settings.num_of_attr);
                        if (settings != NULL)
                        {
                            for (uint8_t i = 0; i < app_settings.num_of_attr; i++)
                            {
                                BE_STREAM_TO_UINT8(settings[i].attr, p);
                                BE_STREAM_TO_UINT8(settings[i].value, p);
                            }

                            app_settings.p_app_setting = settings;
                            payload.msg_buf = &app_settings;
                            bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_CHANGED, &payload);
                            free(settings);
                        }
                    }
                    break;

                case BT_AVRCP_EVENT_ID_NOW_PLAYING_CONTENT_CHANGED:
                    {
                        bt_mgr_dispatch(BT_MSG_AVRCP_PLAYING_CONTENT_CHANGED, &payload);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_AVAILABLE_PLAYERS_CHANGED:
                    {
                        bt_mgr_dispatch(BT_MSG_AVRCP_AVAILABLE_PLAYER_CHANGED, &payload);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_ADDRESSED_PLAYER_CHANGED:
                    {
                        T_BT_AVRCP_RSP_ADDRESSED_PLAYER_CHANGED  addressed_player;

                        BE_STREAM_TO_UINT16(addressed_player.player_id, p);
                        BE_STREAM_TO_UINT16(addressed_player.uid_counter, p);
                        payload.msg_buf = &addressed_player;
                        bt_mgr_dispatch(BT_MSG_AVRCP_ADDRESSED_PLAYER_CHANGED, &payload);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_UIDS_CHANGED:
                    {
                        uint16_t  uid_counter;
                        BE_ARRAY_TO_UINT16(uid_counter, p);

                        payload.msg_buf = &uid_counter;
                        bt_mgr_dispatch(BT_MSG_AVRCP_UIDS_CHANGED, &payload);
                    }
                    break;

                case BT_AVRCP_EVENT_ID_VOLUME_CHANGED:
                    {
                        uint8_t  volume;

                        volume = (*p++) & 0x7f;

                        payload.msg_buf = &volume;
                        bt_mgr_dispatch(BT_MSG_AVRCP_VOLUME_CHANGED, &payload);
                    }
                    break;

                default:
                    break;
                }

                bt_avrcp_register_notification_req(link->bd_addr, event_id);
            }
            else
            {

            }
        }
        break;

    case PDU_ID_LIST_APP_SETTING_ATTRS:
        {
            T_BT_AVRCP_RSP_LIST_APP_SETTING_ATTRS tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                uint8_t *attr_id;

                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                BE_STREAM_TO_UINT8(tmp.num_of_attr, p);

                attr_id = malloc(sizeof(uint8_t) * tmp.num_of_attr);
                if (attr_id != NULL)
                {
                    for (int i = 0; i < tmp.num_of_attr; i++)
                    {
                        BE_STREAM_TO_UINT8(attr_id[i], p);
                    }

                    tmp.p_attr_id = attr_id;
                    payload.msg_buf = &tmp;
                    bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_ATTRS, &payload);
                    free(attr_id);
                }
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
                payload.msg_buf = &tmp;
                bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_ATTRS, &payload);
            }
        }
        break;

    case PDU_ID_LIST_APP_SETTING_VALUES:
        {
            T_BT_AVRCP_RSP_LIST_APP_SETTING_VALUES tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                uint8_t *value;

                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                BE_STREAM_TO_UINT8(tmp.num_of_value, p);

                value = malloc(sizeof(uint8_t) * tmp.num_of_value);
                if (value != NULL)
                {
                    for (int i = 0; i < tmp.num_of_value; i++)
                    {
                        BE_STREAM_TO_UINT8(value[i], p);
                    }

                    tmp.p_value = value;
                    payload.msg_buf = &tmp;
                    bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_VALUES, &payload);
                    free(value);
                }
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
                payload.msg_buf = &tmp;
                bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_VALUES, &payload);
            }
        }
        break;

    case PDU_ID_GET_CURRENT_APP_SETTING_VALUE:
        {
            T_BT_AVRCP_RSP_GET_APP_SETTING_VALUE tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                T_BT_AVRCP_APP_SETTING  *setting;

                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                BE_STREAM_TO_UINT8(tmp.num_of_attr, p);

                setting = malloc(sizeof(T_BT_AVRCP_APP_SETTING) * tmp.num_of_attr);
                if (setting != NULL)
                {
                    for (int i = 0; i < tmp.num_of_attr; i++)
                    {
                        BE_STREAM_TO_UINT8(setting[i].attr, p);
                        BE_STREAM_TO_UINT8(setting[i].value, p);
                    }

                    tmp.p_app_setting = setting;
                    payload.msg_buf = &tmp;
                    bt_mgr_dispatch(BT_MSG_AVRCP_CUR_APP_SETTING_VALUE, &payload);
                    free(setting);
                }
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
                payload.msg_buf = &tmp;
                bt_mgr_dispatch(BT_MSG_AVRCP_CUR_APP_SETTING_VALUE, &payload);
            }
        }
        break;

    case PDU_ID_SET_ADDRESSED_PLAYER:
        {
            if (response == AVRCP_ACCEPTED)
            {
                T_BT_AVRCP_RSP_ERROR_STATUS status_code;

                status_code = (T_BT_AVRCP_RSP_ERROR_STATUS) * p++;
                payload.msg_buf = &status_code;
                bt_mgr_dispatch(BT_MSG_AVRCP_SET_ADDRESSED_PLAYER_RSP, &payload);
            }
        }
        break;

    case PDU_ID_PLAY_ITEM:
        {
            if (response == AVRCP_ACCEPTED)
            {
                T_BT_AVRCP_RSP_ERROR_STATUS status_code;

                status_code = (T_BT_AVRCP_RSP_ERROR_STATUS) * p++;
                payload.msg_buf = &status_code;
                bt_mgr_dispatch(BT_MSG_AVRCP_PLAY_ITEM, &payload);
            }
        }
        break;

    case PDU_ID_SET_ABSOLUTE_VOLUME:
        {
            T_BT_AVRCP_RSP_SET_ABS_VOL rsp_abs_vol;

            if (response == AVRCP_ACCEPTED)
            {
                rsp_abs_vol.state = AVRCP_RSP_STATE_SUCCESS;
                rsp_abs_vol.volume = (*p++) & 0x7f;
            }
            else
            {
                rsp_abs_vol.state = AVRCP_RSP_STATE_FAIL;
            }

            payload.msg_buf = &rsp_abs_vol;
            bt_mgr_dispatch(BT_MSG_AVRCP_SET_ABSOLUTE_VOLUME_RSP, &payload);
        }
        break;

    case PDU_ID_SET_APP_SETTING_VALUE:
        break;

    /* only when TG response err */
    case PDU_ID_REQUEST_CONTINUE_RSP:
        break;

    case PDU_ID_ABORT_CONTINUE_RSP:
        break;

    default:
        break;
    }

    if (packet_type == AVRCP_PKT_TYPE_END)
    {
        free(link->ctrl_chann.recombine.buf);
        memset(&link->ctrl_chann.recombine, 0, sizeof(link->ctrl_chann.recombine));
    }

    return;
}

void bt_avrcp_handle_vendor_rsp(T_BT_AVRCP_LINK *link,
                                uint8_t         *oprand,
                                uint16_t         length,
                                uint8_t          response)
{
    uint8_t  *p;
    uint32_t  company_id;

    p = oprand;
    BE_STREAM_TO_UINT24(company_id, p);
    length -= AVRCP_HDR_LENGTH;

    if (company_id == COMPANY_BT_SIG)
    {
        bt_avrcp_handle_vendor_BT_SIG_rsp(link, p, length, response);
    }
    else
    {
        T_BT_AVRCP_VENDOR_RSP vendor_rsp;
        T_BT_MSG_PAYLOAD      payload;

        vendor_rsp.response = response;
        vendor_rsp.company_id = company_id;
        vendor_rsp.p_rsp = p;
        vendor_rsp.rsp_len = length;
        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &vendor_rsp;

        bt_mgr_dispatch(BT_MSG_AVRCP_VENDOR_RSP, &payload);
    }
}

void bt_avrcp_handle_rsp(T_BT_AVRCP_LINK *link,
                         uint8_t         *data,
                         uint16_t         length)
{
    uint8_t response;
    uint8_t op_code;

    BE_STREAM_TO_UINT8(response, data);
    response &= 0x0f;
    data++;
    /* subUnitType = *data & 0xF8;
      subUnitID = *data & 0x07; */
    BE_STREAM_TO_UINT8(op_code, data);

    if (length >= AVCTP_NON_FRAG_HDR_LENGTH)
    {
        switch (op_code)
        {
        case OPCODE_UNIT_INFO:
            {
                /* T_RSP_UNIT_INFO tmp;

                tmp.state = BT_AVRCP_RSP_STATE_FAIL;
                if (response == AVRCP_IMPLEMENTED)
                {
                    if (data[0] == 0x07) //const byte 0x07
                    {
                        tmp.state = BT_AVRCP_RSP_STATE_SUCCESS;
                        tmp.sub_unit_type = data[1] >> 3;
                        tmp.sub_unit_id = data[1] & 0x07;
                        tmp.company_id = (data[2] << 16) | (data[3] << 8) | data[4];
                    }
                }*/
            }
            break;

        case OPCODE_PASS_THROUGH:
            {
                T_RSP_PASSTHROUGH tmp;
                T_BT_MSG_PAYLOAD  payload;

                if (response == AVRCP_ACCEPTED)
                {
                    uint8_t op_id;
                    uint8_t state_flag;

                    tmp.state = AVRCP_RSP_STATE_SUCCESS;
                    op_id = data[0] & 0x7f;
                    state_flag = data[0] >> 7;
                    tmp.key = (T_BT_AVRCP_KEY)(op_id);
                    tmp.pressed = (state_flag == 0) ? true : false;

                    if (tmp.pressed == false)
                    {
                        bool report = false;

                        if (tmp.key == BT_AVRCP_KEY_PAUSE)
                        {
                            link->ctrl_chann.play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                            report = true;
                        }
                        else if (tmp.key == BT_AVRCP_KEY_PLAY)
                        {
                            link->ctrl_chann.play_status = BT_AVRCP_PLAY_STATUS_PLAYING;
                            report = true;
                        }
                        else if (tmp.key == BT_AVRCP_KEY_STOP)
                        {
                            link->ctrl_chann.play_status = BT_AVRCP_PLAY_STATUS_STOPPED;
                            report = true;
                        }

                        if (report == true)
                        {
                            memcpy(payload.bd_addr, link->bd_addr, 6);
                            payload.msg_buf = &link->ctrl_chann.play_status;
                            bt_mgr_dispatch(BT_MSG_AVRCP_PLAY_STATUS_RSP, &payload);
                        }
                    }
                    else
                    {
                        if ((tmp.key != BT_AVRCP_KEY_FAST_FORWARD) &&
                            (tmp.key != BT_AVRCP_KEY_REWIND))
                        {
                            T_BT_AVRCP_REQ_PASS_THROUGH pass_through;

                            pass_through.key = tmp.key;
                            pass_through.pressed = false;
                            bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &pass_through);
                        }
                    }
                }
                else
                {
                    tmp.state = AVRCP_RSP_STATE_FAIL;
                }
            }
            break;

        case OPCODE_VENDOR_DEPENDENT:
            bt_avrcp_handle_vendor_rsp(link, data, length - AVCTP_NON_FRAG_HDR_LENGTH, response);
            break;

        default:
            break;
        }
    }
}

void bt_avrcp_handle_vendor_BT_SIG_cmd(T_BT_AVRCP_LINK *link,
                                       uint8_t         *pdu,
                                       uint16_t         length,
                                       uint8_t          ctype,
                                       uint8_t          transact)
{
    T_BT_MSG_PAYLOAD  payload;
    uint8_t           pdu_id;
    uint8_t           packet_type;
    uint16_t          param_len;
    uint8_t          *p;

    p = pdu;
    BE_STREAM_TO_UINT8(pdu_id, p);
    BE_STREAM_TO_UINT8(packet_type, p);
    packet_type &= 0x3;
    BE_STREAM_TO_UINT16(param_len, p);

    memcpy(payload.bd_addr, link->bd_addr, 6);

    PROFILE_PRINT_INFO3("bt_avrcp_handle_vendor_BT_SIG_cmd: ctype 0x%02x, packet_type 0x%02x, pdu_id 0x%02x",
                        ctype, packet_type, pdu_id);

    if (packet_type != AVRCP_PKT_TYPE_NON_FRAGMENTED)
    {
        return;
    }

    switch (ctype)
    {
    case AVRCP_CONTROL:
        {
            switch (pdu_id)
            {
            case PDU_ID_SET_ABSOLUTE_VOLUME:
                if (bt_avrcp->tg_features & AVRCP_CATEGORY_2)
                {
                    if (param_len == 1)
                    {
                        uint8_t volume;

                        volume = 0x7f & *p;
                        payload.msg_buf = &volume;
                        bt_mgr_dispatch(BT_MSG_AVRCP_ABSOLUTE_VOLUME_SET, &payload);
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_ACCEPTED,
                                                        pdu_id, &volume, 1);
                    }
                    else
                    {
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {BT_AVRCP_RSP_STATUS_PARAMETER_CONTENT_ERROR}, 1);
                    }
                }
                else
                {
                    bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_NOT_IMPLEMENTED,
                                                    pdu_id, p, param_len);
                }
                break;

            case PDU_ID_SET_ADDRESSED_PLAYER:
                if (bt_avrcp->tg_features & AVRCP_CATEGORY_1 ||
                    bt_avrcp->tg_features & AVRCP_CATEGORY_3)
                {
                    if (param_len == 2)
                    {
                        uint16_t player_id;

                        BE_STREAM_TO_UINT16(player_id, p);

                        link->ctrl_chann.set_addressed_player_pending_transact = transact;
                        payload.msg_buf = &player_id;
                        bt_mgr_dispatch(BT_MSG_AVRCP_SET_ADDRESSED_PLAYER, &payload);
                    }
                    else
                    {
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {BT_AVRCP_RSP_STATUS_PARAMETER_CONTENT_ERROR}, 1);
                    }
                }
                else
                {
                    bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_NOT_IMPLEMENTED,
                                                    pdu_id, p, param_len);
                }
                break;

            case PDU_ID_REQUEST_CONTINUE_RSP:
                {
                    uint8_t target_pdu_id = *p;

                    if (link->ctrl_chann.fragment.pdu_id == target_pdu_id)
                    {
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_IMPLEMENTED, target_pdu_id,
                                                        link->ctrl_chann.fragment.buf + link->ctrl_chann.fragment.read_index,
                                                        link->ctrl_chann.fragment.total_length - link->ctrl_chann.fragment.read_index);
                    }
                    else
                    {
                        if (link->ctrl_chann.fragment.buf != NULL)
                        {
                            free(link->ctrl_chann.fragment.buf);
                            memset(&link->ctrl_chann.fragment, 0, sizeof(link->ctrl_chann.fragment));
                        }

                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {BT_AVRCP_RSP_STATUS_INVALID_PARAMETER}, 1);
                    }
                }
                break;

            case PDU_ID_ABORT_CONTINUE_RSP:
                {
                    uint8_t target_pdu_id;

                    target_pdu_id = *p;
                    if (link->ctrl_chann.fragment.pdu_id == target_pdu_id)
                    {
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_ACCEPTED, pdu_id,
                                                        NULL, 0);
                    }
                    else
                    {
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {BT_AVRCP_RSP_STATUS_INVALID_PARAMETER}, 1);
                    }

                    if (link->ctrl_chann.fragment.buf != NULL)
                    {
                        free(link->ctrl_chann.fragment.buf);
                        memset(&link->ctrl_chann.fragment, 0, sizeof(link->ctrl_chann.fragment));
                    }
                }
                break;

            default:
                bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_NOT_IMPLEMENTED,
                                                pdu_id, p, param_len);
                break;
            }
        }
        break;

    case AVRCP_STATUS:
        {
            switch (pdu_id)
            {
            case PDU_ID_GET_CAPABILITIES:
                {
                    uint8_t capability_id;

                    capability_id = *p;
                    switch (capability_id)
                    {
                    case CAPABILITY_ID_COMPANY_ID:
                        {
                            if (bt_avrcp->local_company_id == COMPANY_BT_SIG)
                            {
                                uint8_t  param[5];
                                uint8_t *p;

                                p = param;
                                BE_UINT8_TO_STREAM(p, capability_id);
                                BE_UINT8_TO_STREAM(p, 1);
                                BE_UINT24_TO_STREAM(p, COMPANY_BT_SIG);

                                bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_IMPLEMENTED,
                                                                pdu_id, param, sizeof(param));
                            }
                            else
                            {
                                uint32_t  company_id = bt_avrcp->local_company_id;
                                uint8_t   param[8];
                                uint8_t  *p;

                                p = param;
                                BE_UINT8_TO_STREAM(p, capability_id);
                                BE_UINT8_TO_STREAM(p, 2);
                                BE_UINT24_TO_STREAM(p, COMPANY_BT_SIG);
                                BE_UINT24_TO_STREAM(p, company_id);

                                bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_IMPLEMENTED,
                                                                pdu_id, param, sizeof(param));
                            }
                        }
                        break;

                    case CAPABILITY_ID_EVENTS_SUPPORTED:
                        {
                            uint8_t  param[10];
                            uint8_t  count;
                            uint8_t *p;

                            count = 0;
                            p = param;
                            BE_UINT8_TO_STREAM(p, capability_id);
                            BE_UINT8_TO_STREAM(p, 0);
                            if (bt_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                BE_UINT8_TO_STREAM(p, BT_AVRCP_EVENT_ID_PLAYBACK_STATUS_CHANGED);
                                BE_UINT8_TO_STREAM(p, BT_AVRCP_EVENT_ID_TRACK_CHANGED);
                                BE_UINT8_TO_STREAM(p, BT_AVRCP_EVENT_ID_AVAILABLE_PLAYERS_CHANGED);
                                BE_UINT8_TO_STREAM(p, BT_AVRCP_EVENT_ID_ADDRESSED_PLAYER_CHANGED);
                                count += 4;
                            }
                            if (bt_avrcp->tg_features & AVRCP_CATEGORY_2)
                            {
                                BE_UINT8_TO_STREAM(p, BT_AVRCP_EVENT_ID_VOLUME_CHANGED);
                                count++;
                            }
                            param[1] = count;

                            bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_IMPLEMENTED,
                                                            pdu_id, param, p - param);
                        }
                        break;

                    default:
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {BT_AVRCP_RSP_STATUS_INVALID_PARAMETER}, 1);
                        break;
                    }
                }
                break;

            case PDU_ID_GET_ELEMENT_ATTRS:
                {
                    if (bt_avrcp->tg_features & AVRCP_CATEGORY_1)
                    {
                        T_BT_AVRCP_REQ_GET_ELEMENT_ATTR get_element_attrs;
                        int                             i;

                        p += 8;
                        BE_STREAM_TO_UINT8(get_element_attrs.attr_num, p);
                        for (i = 0; i < get_element_attrs.attr_num; i++)
                        {
                            BE_STREAM_TO_UINT32(get_element_attrs.attr_id[i], p);
                        }

                        link->ctrl_chann.get_element_attr_pending_transact = transact;
                        payload.msg_buf = &get_element_attrs;
                        bt_mgr_dispatch(BT_MSG_AVRCP_GET_ELEM_ATTRS, &payload);
                    }
                    else
                    {
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_NOT_IMPLEMENTED,
                                                        pdu_id, p, param_len);
                    }
                }
                break;

            case PDU_ID_GET_PLAY_STATUS:
                {
                    if (bt_avrcp->tg_features & AVRCP_CATEGORY_1)
                    {
                        link->ctrl_chann.get_play_status_pending_transact = transact;
                        bt_mgr_dispatch(BT_MSG_AVRCP_GET_PLAY_STATUS, &payload);
                    }
                    else
                    {
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_NOT_IMPLEMENTED,
                                                        pdu_id, p, param_len);
                    }
                }
                break;

            default:
                {
                    bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_REJECTED, pdu_id,
                    &(uint8_t) {BT_AVRCP_RSP_STATUS_INVALID_COMMAND}, 1);
                }
                break;
            }
        }
        break;

    case AVRCP_NOTIFY:
        {
            switch (pdu_id)
            {
            case PDU_ID_REGISTER_NOTIFICATION:
                {
                    uint8_t event_id = *p;

                    switch (event_id)
                    {
                    case BT_AVRCP_EVENT_ID_VOLUME_CHANGED:
                        {
                            if (bt_avrcp->tg_features & AVRCP_CATEGORY_2)
                            {
                                link->ctrl_chann.vol_change_pending_transact = transact;
                                bt_mgr_dispatch(BT_MSG_AVRCP_REG_VOL_CHANGE, &payload);
                            }
                            else
                            {
                                bt_avrcp_send_vendor_BT_SIG_rsp(link,
                                                                transact,
                                                                AVRCP_NOT_IMPLEMENTED,
                                                                pdu_id,
                                                                p,
                                                                param_len);
                            }
                        }
                        break;

                    case BT_AVRCP_EVENT_ID_PLAYBACK_STATUS_CHANGED:
                        {
                            if (bt_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                link->ctrl_chann.play_status_change_pending_transact = transact;
                                bt_mgr_dispatch(BT_MSG_AVRCP_REG_PLAY_STATUS_CHANGE, &payload);
                            }
                            else
                            {
                                bt_avrcp_send_vendor_BT_SIG_rsp(link,
                                                                transact,
                                                                AVRCP_NOT_IMPLEMENTED,
                                                                pdu_id,
                                                                p,
                                                                param_len);
                            }
                        }
                        break;

                    case BT_AVRCP_EVENT_ID_TRACK_CHANGED:
                        {
                            if (bt_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                link->ctrl_chann.track_change_pending_transact = transact;
                                bt_mgr_dispatch(BT_MSG_AVRCP_REG_TRACK_CHANGE, &payload);
                            }
                            else
                            {
                                bt_avrcp_send_vendor_BT_SIG_rsp(link,
                                                                transact,
                                                                AVRCP_NOT_IMPLEMENTED,
                                                                pdu_id,
                                                                p,
                                                                param_len);
                            }
                        }
                        break;

                    case BT_AVRCP_EVENT_ID_AVAILABLE_PLAYERS_CHANGED:
                        {
                            if (bt_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_INTERIM,
                                                                pdu_id, &event_id, 1);
                            }
                            else
                            {
                                bt_avrcp_send_vendor_BT_SIG_rsp(link,
                                                                transact,
                                                                AVRCP_NOT_IMPLEMENTED,
                                                                pdu_id,
                                                                p,
                                                                param_len);
                            }
                        }
                        break;

                    case BT_AVRCP_EVENT_ID_ADDRESSED_PLAYER_CHANGED:
                        {
                            if (bt_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                link->ctrl_chann.addressed_player_change_pending_transact = transact;
                                bt_mgr_dispatch(BT_MSG_AVRCP_REG_ADDRESSED_PLAYER_CHANGE, &payload);
                            }
                            else
                            {
                                bt_avrcp_send_vendor_BT_SIG_rsp(link,
                                                                transact,
                                                                AVRCP_NOT_IMPLEMENTED,
                                                                pdu_id,
                                                                p,
                                                                param_len);
                            }
                        }
                        break;

                    default:
                        bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {BT_AVRCP_RSP_STATUS_INVALID_PARAMETER}, 1);
                        break;
                    }
                }
                break;

            default:
                bt_avrcp_send_vendor_BT_SIG_rsp(link, transact, AVRCP_NOT_IMPLEMENTED,
                                                pdu_id, p, param_len);
                break;
            }
        }
        break;

    default:
        break;
    }
}

void bt_avrcp_handle_vendor_cmd(T_BT_AVRCP_LINK *link,
                                uint8_t         *oprand,
                                uint16_t         length,
                                uint8_t          ctype,
                                uint8_t          transact)
{
    uint8_t *p;
    uint32_t company_id;

    p = oprand;
    BE_STREAM_TO_UINT24(company_id, p);
    length -= AVRCP_HDR_LENGTH;

    if (company_id == COMPANY_BT_SIG)
    {
        bt_avrcp_handle_vendor_BT_SIG_cmd(link, p, length, ctype, transact);
    }
    else
    {
        T_BT_AVRCP_VENDOR_CMD vendor_cmd;
        T_BT_MSG_PAYLOAD      payload;

        link->ctrl_chann.vendor_cmd_transact = transact;

        vendor_cmd.ctype = ctype;
        vendor_cmd.company_id = company_id;
        vendor_cmd.p_cmd = p;
        vendor_cmd.cmd_len = length;
        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &vendor_cmd;

        bt_mgr_dispatch(BT_MSG_AVRCP_VENDOR_CMD_IND, &payload);
    }
}

void bt_avrcp_handle_cmd(T_BT_AVRCP_LINK *link,
                         uint8_t         *data,
                         uint16_t         length,
                         uint8_t          transact)
{
    T_BT_MSG_PAYLOAD payload;
    uint8_t          ctype;
    uint8_t          sub_unit_type_and_id;
    uint8_t          op_code;
    uint8_t          response = 0;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;

    BE_STREAM_TO_UINT8(ctype, data);
    ctype &= 0x0f;
    BE_STREAM_TO_UINT8(sub_unit_type_and_id, data);
    BE_STREAM_TO_UINT8(op_code, data);

    if (length >= AVCTP_NON_FRAG_HDR_LENGTH)
    {
        if ((sub_unit_type_and_id == 0xff) ||
            (sub_unit_type_and_id == ((AVRCP_SUBUNIT_TYPE_PANEL << 3) | AVRCP_SUBUNIT_ID)))
        {
            switch (ctype)
            {
            case AVRCP_CONTROL:
                {
                    switch (op_code)
                    {
                    case OPCODE_PASS_THROUGH:
                        {
                            uint8_t state;
                            uint8_t op_id;

                            state = *data >> 7;
                            op_id = *data & 0x7f;
                            switch (op_id)
                            {
                            case BT_AVRCP_KEY_POWER:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_POWER, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_VOL_UP:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_KEY_VOLUME_UP, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_VOL_DOWN:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_KEY_VOLUME_DOWN, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_MUTE:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_MUTE, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_PLAY:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_PLAY, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_STOP:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_STOP, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_PAUSE:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_PAUSE, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_REWIND:
                                if (state == PASSTHROUGH_STATE_PRESSED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_REWIND_START, &payload);
                                }
                                else
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_REWIND_STOP, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_FAST_FORWARD:
                                if (state == PASSTHROUGH_STATE_PRESSED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_FAST_FORWARD_START, &payload);
                                }
                                else
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_FAST_FORWARD_STOP, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_FORWARD:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_FORWARD, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case BT_AVRCP_KEY_BACKWARD:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    bt_mgr_dispatch(BT_MSG_AVRCP_BACKWARD, &payload);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            default:
                                response = AVRCP_NOT_IMPLEMENTED;
                                break;
                            }
                        }
                        break;

                    case OPCODE_VENDOR_DEPENDENT:
                        bt_avrcp_handle_vendor_cmd(link,
                                                   data,
                                                   length - AVCTP_NON_FRAG_HDR_LENGTH,
                                                   AVRCP_CONTROL,
                                                   transact);
                        break;

                    default:
                        response = AVRCP_NOT_IMPLEMENTED;
                        break;
                    }
                }
                break;

            case AVRCP_STATUS:
                {
                    switch (op_code)
                    {
                    case OPCODE_UNIT_INFO:
                        {
                            uint8_t  *p;

                            data -= AVCTP_NON_FRAG_HDR_LENGTH;
                            p = data;
                            BE_UINT8_TO_STREAM(p, (*data & 0xf0) | (AVRCP_IMPLEMENTED & 0x0f));
                            STREAM_SKIP_LEN(p, 2);
                            BE_UINT8_TO_STREAM(p, 0x7);
                            BE_UINT8_TO_STREAM(p, AVRCP_SUBUNIT_TYPE_PANEL << 3 | 0);
                            BE_UINT24_TO_STREAM(p, bt_avrcp->local_company_id);

                            avctp_data_send(link->bd_addr, data, length, NULL, 0, transact,
                                            AVCTP_MSG_TYPE_RSP);
                        }
                        break;

                    case OPCODE_SUB_UNIT_INFO:
                        data -= AVCTP_NON_FRAG_HDR_LENGTH;
                        data[0] = (*data & 0xf0) | (AVRCP_IMPLEMENTED & 0x0f);
                        data[3] = 0x07;
                        data[4] = AVRCP_SUBUNIT_TYPE_PANEL << 3 | 0;
                        data[5] = 0xff;
                        data[6] = 0xff;
                        data[7] = 0xff;

                        avctp_data_send(link->bd_addr, data, length, NULL, 0, transact,
                                        AVCTP_MSG_TYPE_RSP);
                        break;

                    case OPCODE_VENDOR_DEPENDENT:
                        bt_avrcp_handle_vendor_cmd(link,
                                                   data,
                                                   length - AVCTP_NON_FRAG_HDR_LENGTH,
                                                   AVRCP_STATUS,
                                                   transact);
                        break;

                    default:
                        response = AVRCP_NOT_IMPLEMENTED;
                    }
                }
                break;

            case AVRCP_NOTIFY:
                {
                    switch (op_code)
                    {
                    case OPCODE_VENDOR_DEPENDENT:
                        bt_avrcp_handle_vendor_cmd(link,
                                                   data,
                                                   length - AVCTP_NON_FRAG_HDR_LENGTH,
                                                   AVRCP_NOTIFY,
                                                   transact);
                        break;

                    default:
                        response = AVRCP_NOT_IMPLEMENTED;
                        break;
                    }
                }
                break;

            case AVRCP_SPECIFIC_INQUIRY:
                response = AVRCP_NOT_IMPLEMENTED;
                break;

            case AVRCP_GENERAL_INQUIRY:
                response = AVRCP_NOT_IMPLEMENTED;
                break;

            default:
                response = AVRCP_NOT_IMPLEMENTED;
                break;
            }
        }
        else
        {
            response = AVRCP_NOT_IMPLEMENTED;
        }
    }
    else
    {
        response = AVRCP_NOT_IMPLEMENTED;
    }

    if (response != 0)
    {
        data -= AVCTP_NON_FRAG_HDR_LENGTH;
        *data = (*data & 0xf0) | (response & 0x0f);
        avctp_data_send(link->bd_addr, data, length, NULL, 0, transact, AVCTP_MSG_TYPE_RSP);
    }
}

void bt_avrcp_handle_data_ind(uint16_t  cid,
                              uint8_t   transact,
                              uint8_t   cr_type,
                              uint8_t  *data,
                              uint16_t  length)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_cid(cid);
    if (link != NULL)
    {
        if (cr_type == AVCTP_MSG_TYPE_CMD)
        {
            bt_avrcp_handle_cmd(link, data, length, transact);
        }
        else
        {
            /* ds cmd is blocked (to avoid repeated rsp with the same label, eg.AVRCP_CHANGED) */
            if (link->ctrl_chann.cmd_credits == 0)
                /* to avoid repeated rsp not for last sent cmd, eg.AVRCP_CHANGED)
                  && (transact == link->transact_label)) */
            {
                link->ctrl_chann.cmd_credits = 1;
                sys_timer_stop(link->ctrl_chann.timer_handle);
            }

            bt_avrcp_handle_rsp(link, data, length);
            bt_avrcp_cmd_process(link);
        }
    }
}

bool bt_avrcp_volume_change_register_rsp(uint8_t bd_addr[6],
                                         uint8_t vol)
{
    uint8_t          param[2];
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    param[0] = BT_AVRCP_EVENT_ID_VOLUME_CHANGED;
    param[1] = vol;
    if (vol > 0x7f)
    {
        ret = 1;
        goto fail_invalid_volume;
    }

    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_2) == 0)
    {
        ret = 2;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 3;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.vol_change_pending_transact == 0xff)
    {
        ret = 4;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.vol_change_pending_transact,
                                    AVRCP_INTERIM,
                                    PDU_ID_REGISTER_NOTIFICATION, param, sizeof(param));

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
fail_invalid_volume:
    PROFILE_PRINT_ERROR1("bt_avrcp_volume_change_register_rsp: failed %d", -ret);
    return false;
}

bool bt_avrcp_volume_change_req(uint8_t bd_addr[6],
                                uint8_t vol)
{
    uint8_t          param[2];
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    param[0] = BT_AVRCP_EVENT_ID_VOLUME_CHANGED;
    param[1] = vol;
    if (vol > 0x7f)
    {
        ret = 1;
        goto fail_invalid_volume;
    }

    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_2) == 0)
    {
        ret = 2;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 3;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.vol_change_pending_transact == 0xff)
    {
        ret = 4;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.vol_change_pending_transact,
                                    AVRCP_CHANGED,
                                    PDU_ID_REGISTER_NOTIFICATION, param, sizeof(param));
    link->ctrl_chann.vol_change_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
fail_invalid_volume:
    PROFILE_PRINT_ERROR1("bt_avrcp_volume_change_req: failed %d", -ret);
    return false;
}

bool bt_avrcp_play_status_change_register_rsp(uint8_t                bd_addr[6],
                                              T_BT_AVRCP_PLAY_STATUS play_status)
{
    uint8_t          param[2];
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    param[0] = BT_AVRCP_EVENT_ID_PLAYBACK_STATUS_CHANGED;
    param[1] = play_status;
    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.play_status_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.play_status_change_pending_transact,
                                    AVRCP_INTERIM, PDU_ID_REGISTER_NOTIFICATION, param,
                                    sizeof(param));

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("bt_avrcp_play_status_change_register_rsp: failed %d", -ret);
    return false;
}

bool bt_avrcp_play_status_change_req(uint8_t                bd_addr[6],
                                     T_BT_AVRCP_PLAY_STATUS play_status)
{
    uint8_t          param[2];
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    param[0] = BT_AVRCP_EVENT_ID_PLAYBACK_STATUS_CHANGED;
    param[1] = play_status;
    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.play_status_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.play_status_change_pending_transact,
                                    AVRCP_CHANGED, PDU_ID_REGISTER_NOTIFICATION, param,
                                    sizeof(param));
    link->ctrl_chann.play_status_change_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("bt_avrcp_play_status_change_req: failed %d", -ret);
    return false;
}

bool bt_avrcp_connect_req(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_avrcp_alloc_link(bd_addr);
        if (link != NULL)
        {
            bt_sniff_mode_exit(bd_addr, false);
            link->ctrl_chann.state = BT_AVRCP_STATE_CONNECTING;
            return avctp_connect_req(bd_addr);
        }
    }
    return false;
}

void bt_avrcp_handle_conn_ind(T_MPA_L2C_CONN_IND *ind)
{
    T_BT_AVRCP_LINK *link;

    PROFILE_PRINT_TRACE2("bt_avrcp_handle_conn_ind: bd_addr [%s], cid 0x%04x",
                         TRACE_BDADDR(ind->bd_addr), ind->cid);

    link = bt_avrcp_find_link_by_addr(ind->bd_addr);
    if (link == NULL)
    {
        link = bt_avrcp_alloc_link(ind->bd_addr);
        if (link != NULL)
        {
            T_BT_MSG_PAYLOAD payload;

            memcpy(payload.bd_addr, ind->bd_addr, 6);
            payload.msg_buf = NULL;

            link->ctrl_chann.state = BT_AVRCP_STATE_CONNECTING;
            link->ctrl_chann.cid = ind->cid;
            link->ctrl_chann.cmd_credits = 0;
            bt_mgr_dispatch(BT_MSG_AVRCP_CONN_IND, &payload);
        }
        else
        {
            avctp_connect_cfm(ind->bd_addr, false);
        }
    }
    else
    {
        avctp_connect_cfm(ind->bd_addr, false);
    }
}

bool bt_avrcp_connect_cfm(uint8_t bd_addr[6],
                          bool    accept)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);

        if (accept == false)
        {
            bt_avrcp_free_link(link);
        }

        return avctp_connect_cfm(bd_addr, accept);
    }

    return avctp_connect_cfm(bd_addr, false);
}

void bt_avrcp_handle_conn_rsp(T_MPA_L2C_CONN_RSP *ind)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(ind->bd_addr);
    if (link != NULL)
    {
        link->ctrl_chann.cid = ind->cid;
        link->ctrl_chann.cmd_credits = 0;
    }
}

void bt_avrcp_handle_conn_fail(uint16_t cid,
                               uint16_t cause)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &cause;

        bt_mgr_dispatch(BT_MSG_AVRCP_CONN_FAIL, &payload);
        bt_avrcp_free_link(link);
    }
}

void bt_avrcp_handle_conn_cmpl_ind(T_MPA_L2C_CONN_CMPL_INFO *ind)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(ind->bd_addr);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, ind->bd_addr, 6);
        payload.msg_buf = NULL;

        os_queue_init(&link->ctrl_chann.cmd_queue);
        os_queue_init(&link->browsing_chann.cmd_queue);

        link->ctrl_chann.state = BT_AVRCP_STATE_CONNECTED;
        link->ctrl_chann.cmd_credits = 1;
        link->ctrl_chann.transact_label = 0;
        link->ctrl_chann.remote_mtu = ind->remote_mtu;
        link->ctrl_chann.vol_change_pending_transact = 0xff;
        link->ctrl_chann.play_status_change_pending_transact = 0xff;
        link->ctrl_chann.track_change_pending_transact = 0xff;
        link->ctrl_chann.addressed_player_change_pending_transact = 0xff;
        link->ctrl_chann.get_element_attr_pending_transact = 0xff;
        link->ctrl_chann.get_play_status_pending_transact = 0xff;
        link->ctrl_chann.set_addressed_player_pending_transact = 0xff;
        link->ctrl_chann.vendor_cmd_transact = 0xff;
        memset(&link->ctrl_chann.fragment, 0, sizeof(link->ctrl_chann.fragment));

        bt_mgr_dispatch(BT_MSG_AVRCP_CONN_CMPL, &payload);
        bt_avrcp_get_capability(ind->bd_addr, CAPABILITY_ID_EVENTS_SUPPORTED);
    }
}

bool bt_avrcp_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;
    bool             ret = false;

    PROFILE_PRINT_TRACE1("bt_avrcp_disconnect_req: bd_addr[%s]", TRACE_BDADDR(bd_addr));

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);

        if (link->browsing_chann.state == BT_AVRCP_STATE_CONNECTED)
        {
            link->browsing_chann.state = BT_AVRCP_STATE_DISCONNECTING;
            avctp_browsing_disconnect_req(link->bd_addr);
        }

        ret = avctp_disconnect_req(link->bd_addr);
        if (ret)
        {
            link->ctrl_chann.state = BT_AVRCP_STATE_DISCONNECTING;
        }
    }

    return ret;
}

void bt_avrcp_handle_disconn_ind(uint16_t cid,
                                 uint16_t cause)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD  payload;
        T_BT_AVRCP_CMD   *data;
        T_BT_AVRCP_CMD   *next;

        PROFILE_PRINT_TRACE2("bt_avrcp_handle_disconn_ind: bd_addr [%s], cid 0x%04x",
                             TRACE_BDADDR(link->bd_addr), cid);

        data = (T_BT_AVRCP_CMD *)link->ctrl_chann.cmd_queue.p_first;
        while (data)
        {
            next = (T_BT_AVRCP_CMD *)data->next;

            os_queue_delete(&link->ctrl_chann.cmd_queue, data);
            free(data);

            data = next;
        }

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &cause;
        bt_avrcp_free_link(link);
        bt_mgr_dispatch(BT_MSG_AVRCP_DISCONN_CMPL, &payload);
    }
}

bool bt_avrcp_absolute_volume_set(uint8_t bd_addr[6],
                                  uint8_t volume)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (bt_avrcp->ct_features & AVRCP_CATEGORY_2)
        {
            uint8_t vol;

            vol = volume & 0x7f;
            bt_sniff_mode_exit(bd_addr, false);
            return bt_avrcp_cmd_enqueue(link, PDU_ID_SET_ABSOLUTE_VOLUME, &vol);
        }
    }

    return false;
}

bool bt_avrcp_roleswap_info_get(uint8_t                bd_addr[6],
                                T_ROLESWAP_AVRCP_INFO *info)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_ROLESWAP_AVCTP_INFO avctp_info;

        if (avctp_roleswap_info_get(bd_addr, &avctp_info) == true)
        {
            memcpy(info->bd_addr, bd_addr, 6);
            info->l2c_cid = link->ctrl_chann.cid;
            info->cmd_credits = link->ctrl_chann.cmd_credits;
            info->transact_label = link->ctrl_chann.transact_label;
            info->vol_change_pending_transact = link->ctrl_chann.vol_change_pending_transact;
            info->vendor_cmd_transact = link->ctrl_chann.vendor_cmd_transact;
            info->state = link->ctrl_chann.state;
            info->play_status = link->ctrl_chann.play_status;

            info->remote_mtu = avctp_info.remote_mtu;
            info->data_offset = avctp_info.data_offset;
            info->avctp_state = avctp_info.avctp_state;

            return true;
        }
    }

    return false;
}

bool bt_avrcp_roleswap_info_set(uint8_t                bd_addr[6],
                                T_ROLESWAP_AVRCP_INFO *info)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_avrcp_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        T_ROLESWAP_AVCTP_INFO avctp_info;

        link->ctrl_chann.cid = info->l2c_cid;
        link->ctrl_chann.cmd_credits = info->cmd_credits;
        link->ctrl_chann.transact_label = info->transact_label;
        link->ctrl_chann.vol_change_pending_transact = info->vol_change_pending_transact;
        link->ctrl_chann.vendor_cmd_transact = info->vendor_cmd_transact;
        link->ctrl_chann.state = (T_BT_AVRCP_STATE)info->state;
        link->ctrl_chann.play_status = info->play_status;

        if (link->ctrl_chann.cmd_credits == 0)
        {
            sys_timer_start(link->ctrl_chann.timer_handle);
        }

        avctp_info.l2c_cid = info->l2c_cid;
        avctp_info.remote_mtu = info->remote_mtu;
        avctp_info.data_offset = info->data_offset;
        avctp_info.avctp_state = info->avctp_state;

        return avctp_roleswap_info_set(bd_addr, &avctp_info);
    }

    return false;
}

bool bt_avrcp_roleswap_info_del(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_avrcp_free_link(link);
        return avctp_roleswap_info_del(bd_addr);
    }

    return false;
}

bool bt_avrcp_get_play_status_rsp(uint8_t                bd_addr[6],
                                  uint32_t               song_length,
                                  uint32_t               song_pos,
                                  T_BT_AVRCP_PLAY_STATUS play_status)
{
    uint8_t          param[9];
    uint8_t         *p;
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    p = param;
    BE_UINT32_TO_STREAM(p, song_length);
    BE_UINT32_TO_STREAM(p, song_pos);
    BE_UINT8_TO_STREAM(p, play_status);

    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.get_play_status_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.get_play_status_pending_transact,
                                    AVRCP_IMPLEMENTED, PDU_ID_GET_PLAY_STATUS, param,
                                    sizeof(param));
    link->ctrl_chann.get_play_status_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("bt_avrcp_get_play_status_rsp: failed %d", -ret);
    return false;
}

bool bt_avrcp_get_element_attr_rsp(uint8_t                  bd_addr[6],
                                   uint8_t                  attr_num,
                                   T_BT_AVRCP_ELEMENT_ATTR *attr)
{
    T_BT_AVRCP_LINK *link;
    uint8_t          i;
    uint8_t         *data;
    uint8_t         *p;
    uint16_t         data_len;
    int32_t          ret = 0;

    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.get_element_attr_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    data_len = 1;
    for (i = 0; i < attr_num; i++)
    {
        data_len += 8;
        data_len += attr[i].length;
    }

    data = malloc(data_len);
    if (data == NULL)
    {
        ret = 4;
        goto fail_alloc_mem;
    }

    if (link->ctrl_chann.fragment.buf != NULL)
    {
        ret = 5;
        goto fail_last_rsp_not_finish;
    }

    p = data;

    BE_UINT8_TO_STREAM(p, attr_num);
    for (i = 0; i < attr_num; i++)
    {
        BE_UINT32_TO_STREAM(p, attr[i].attribute_id);
        BE_UINT16_TO_STREAM(p, attr[i].character_set_id);
        BE_UINT16_TO_STREAM(p, attr[i].length);
        memcpy(p, attr[i].p_buf, attr[i].length);
        p += attr[i].length;
    }

    link->ctrl_chann.fragment.buf = data;
    link->ctrl_chann.fragment.read_index = 0;
    link->ctrl_chann.fragment.total_length = data_len;

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.get_element_attr_pending_transact,
                                    AVRCP_IMPLEMENTED, PDU_ID_GET_ELEMENT_ATTRS, data, data_len);
    link->ctrl_chann.get_element_attr_pending_transact = 0xff;

    return true;

fail_last_rsp_not_finish:
    free(data);
fail_alloc_mem:
fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("bt_avrcp_get_element_attr_rsp: failed %d", -ret);
    return false;
}

bool bt_avrcp_track_change_register_rsp(uint8_t  bd_addr[6],
                                        uint64_t track_id)
{
    uint8_t          param[9];
    uint8_t         *p;
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    p = param;
    BE_UINT8_TO_STREAM(p, BT_AVRCP_EVENT_ID_TRACK_CHANGED);
    BE_UINT64_TO_STREAM(p, track_id);

    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.track_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.track_change_pending_transact,
                                    AVRCP_INTERIM, PDU_ID_REGISTER_NOTIFICATION, param,
                                    sizeof(param));

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("bt_avrcp_track_change_register_rsp: failed %d", -ret);
    return false;
}

bool bt_avrcp_get_element_attr_req(uint8_t  bd_addr[6],
                                   uint8_t  attr_num,
                                   uint8_t *attr)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_GET_ELEMENT_ATTR tmp;
        uint8_t i;

        tmp.attr_num = attr_num;
        for (i = 0; i < attr_num; i++)
        {
            tmp.attr_id[i] = attr[i];
        }

        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_GET_ELEMENT_ATTRS, &tmp);
    }

    return false;
}

bool bt_avrcp_get_play_status_req(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_GET_PLAY_STATUS, NULL);
    }

    return false;
}

bool bt_avrcp_play(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_PLAY;
        tmp.pressed = true;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_pause(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_PAUSE;
        tmp.pressed = true;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_stop(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_STOP;
        tmp.pressed = true;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_forward(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_FORWARD;
        tmp.pressed = true;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_backward(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_BACKWARD;
        tmp.pressed = true;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_mute(uint8_t *bd_addr)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_MUTE;
        tmp.pressed = true;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_power(uint8_t *bd_addr)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_POWER;
        tmp.pressed = true;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_rewind_start(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_REWIND;
        tmp.pressed = true;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_rewind_stop(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_REWIND;
        tmp.pressed = false;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_fast_forward_start(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_FAST_FORWARD;
        tmp.pressed = true;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_fast_forward_stop(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = BT_AVRCP_KEY_FAST_FORWARD;
        tmp.pressed = false;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_track_change_req(uint8_t  bd_addr[6],
                               uint64_t track_id)
{
    uint8_t          param[9];
    uint8_t         *p;
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    p = param;
    BE_UINT8_TO_STREAM(p, BT_AVRCP_EVENT_ID_TRACK_CHANGED);
    BE_UINT64_TO_STREAM(p, track_id);

    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.track_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.track_change_pending_transact,
                                    AVRCP_CHANGED, PDU_ID_REGISTER_NOTIFICATION, param,
                                    sizeof(param));
    link->ctrl_chann.track_change_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("bt_avrcp_track_change_req: failed %d", -ret);
    return false;
}


bool bt_avrcp_addressed_player_set_rsp(uint8_t                     bd_addr[6],
                                       T_BT_AVRCP_RSP_ERROR_STATUS status)
{
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.set_addressed_player_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);

    if (status == BT_AVRCP_RSP_STATUS_SUCCESS)
    {
        bt_avrcp_send_vendor_BT_SIG_rsp(link,
                                        link->ctrl_chann.set_addressed_player_pending_transact,
                                        AVRCP_ACCEPTED,
                                        PDU_ID_SET_ADDRESSED_PLAYER,
                                        &status,
                                        1);
    }
    else
    {
        bt_avrcp_send_vendor_BT_SIG_rsp(link,
                                        link->ctrl_chann.set_addressed_player_pending_transact,
                                        AVRCP_REJECTED,
                                        PDU_ID_SET_ADDRESSED_PLAYER,
                                        &status,
                                        1);
    }
    link->ctrl_chann.set_addressed_player_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("bt_avrcp_addressed_player_set_rsp: failed %d", -ret);
    return false;
}

bool bt_avrcp_addressed_player_change_register_rsp(uint8_t  bd_addr[6],
                                                   uint16_t player_id,
                                                   uint16_t uid_counter)
{
    uint8_t          param[5];
    uint8_t         *p;
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    p = param;
    BE_UINT8_TO_STREAM(p, BT_AVRCP_EVENT_ID_ADDRESSED_PLAYER_CHANGED);
    BE_UINT16_TO_STREAM(p, player_id);
    BE_UINT16_TO_STREAM(p, uid_counter);

    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_1) == 0 &&
        (bt_avrcp->tg_features & AVRCP_CATEGORY_3) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.addressed_player_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.addressed_player_change_pending_transact,
                                    AVRCP_INTERIM, PDU_ID_REGISTER_NOTIFICATION, param, sizeof(param));

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("bt_avrcp_addressed_player_change_register_rsp: failed %d", -ret);
    return false;
}

bool bt_avrcp_addressed_player_change_req(uint8_t  bd_addr[6],
                                          uint16_t player_id,
                                          uint16_t uid_counter)
{
    uint8_t          param[5];
    uint8_t         *p;
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    p = param;
    BE_UINT8_TO_STREAM(p, BT_AVRCP_EVENT_ID_ADDRESSED_PLAYER_CHANGED);
    BE_UINT16_TO_STREAM(p, player_id);
    BE_UINT16_TO_STREAM(p, uid_counter);

    if ((bt_avrcp->tg_features & AVRCP_CATEGORY_1) == 0 &&
        (bt_avrcp->tg_features & AVRCP_CATEGORY_3) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (link->ctrl_chann.addressed_player_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    bt_sniff_mode_exit(bd_addr, false);
    bt_avrcp_send_vendor_BT_SIG_rsp(link, link->ctrl_chann.addressed_player_change_pending_transact,
                                    AVRCP_CHANGED, PDU_ID_REGISTER_NOTIFICATION, param,
                                    sizeof(param));
    link->ctrl_chann.addressed_player_change_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("bt_avrcp_addressed_player_change_req: failed %d", -ret);
    return false;
}

#if (CONFIG_REALTEK_BTM_AVRCP_BROWSING_SUPPORT == 1)
bool bt_avrcp_browsing_send_generic_cmd(T_BT_AVRCP_LINK *link,
                                        uint8_t         *buf,
                                        uint16_t         length)
{
    link->browsing_chann.transact_label = (link->browsing_chann.transact_label + 1) & 0x0f;
    if (avctp_browsing_data_send(link->bd_addr, buf, length,
                                 link->browsing_chann.transact_label, AVCTP_MSG_TYPE_CMD))
    {
        link->browsing_chann.cmd_credits = 0;
        return true;
    }

    return false;
}

bool bt_avrcp_browsing_send_generic_rsp(uint8_t       bd_addr[6],
                                        uint8_t      *buf,
                                        uint16_t      length)
{
    T_BT_AVRCP_LINK *link;
    int32_t          ret = 0;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_bd_addr;
    }

    if (link->browsing_chann.state != BT_AVRCP_STATE_CONNECTED)
    {
        ret = 2;
        goto fail_check_state;
    }

    if (link->browsing_chann.pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    if (avctp_browsing_data_send(link->bd_addr, buf, length,
                                 link->browsing_chann.pending_transact,
                                 AVCTP_MSG_TYPE_RSP))
    {
        link->browsing_chann.pending_transact = 0xff;
    }
    else
    {
        ret = 4;
        goto fail_send_rsp;
    }

    return true;

fail_bd_addr:
fail_check_state:
fail_send_rsp:
fail_no_pending_transact:
    PROFILE_PRINT_ERROR1("bt_avrcp_browsing_send_generic_rsp: failed %d", -ret);
    return false;
}

bool bt_avrcp_browsing_send_cmd_get_folder_items(T_BT_AVRCP_LINK                 *link,
                                                 T_BT_AVRCP_REQ_GET_FOLDER_ITEMS *cmd_param)
{
    uint8_t  buf[13 + (MAX_ELEMENT_ATTR_NUM * 4)];
    uint8_t  len;
    uint8_t *p;
    uint8_t  i;

    p = buf;
    len = 13 + cmd_param->attr_count * 4;
    BE_UINT8_TO_STREAM(p, PDU_ID_GET_FOLDER_ITEMS);
    BE_UINT16_TO_STREAM(p, 10 + (MAX_ELEMENT_ATTR_NUM * 4));
    BE_UINT8_TO_STREAM(p, cmd_param->scope);
    BE_UINT32_TO_STREAM(p, cmd_param->start_item);
    BE_UINT32_TO_STREAM(p, cmd_param->end_item);
    BE_UINT8_TO_STREAM(p, cmd_param->attr_count);

    for (i = 0; i < cmd_param->attr_count; i++)
    {
        BE_UINT32_TO_STREAM(p, cmd_param->attr_id[i]);
    }

    return bt_avrcp_browsing_send_generic_cmd(link, buf, len);
}

bool bt_avrcp_browsing_send_cmd_get_item_attrs(T_BT_AVRCP_LINK               *link,
                                               T_BT_AVRCP_REQ_GET_ITEM_ATTRS *cmd_param)
{
    uint8_t  buf[15 + (MAX_ELEMENT_ATTR_NUM * 4)];
    uint8_t  len;
    uint8_t *p;
    uint8_t  i;

    p = buf;
    len = 15 + cmd_param->num_of_attr * 4;
    BE_UINT8_TO_STREAM(p, PDU_ID_GET_ITEM_ATTRS);
    BE_UINT16_TO_STREAM(p, 12 + (MAX_ELEMENT_ATTR_NUM * 4));
    BE_UINT8_TO_STREAM(p, cmd_param->scope);
    BE_UINT64_TO_STREAM(p, cmd_param->uid);
    BE_UINT16_TO_STREAM(p, cmd_param->uid_counter);
    BE_UINT8_TO_STREAM(p, cmd_param->num_of_attr);

    for (i = 0; i < cmd_param->num_of_attr; i++)
    {
        BE_UINT32_TO_STREAM(p, cmd_param->attr_id[i]);
    }

    return bt_avrcp_browsing_send_generic_cmd(link, buf, len);
}

bool bt_avrcp_browsing_send_cmd_search(T_BT_AVRCP_LINK       *link,
                                       T_BT_AVRCP_REQ_SEARCH *cmd_param)
{
    uint8_t *buf;
    bool     ret = false;

    buf = malloc((7 + cmd_param->length));
    if (buf != NULL)
    {
        uint8_t *p;

        p = buf;
        BE_UINT8_TO_STREAM(p, PDU_ID_SEARCH);
        BE_UINT16_TO_STREAM(p, cmd_param->length + 4);
        BE_UINT8_TO_STREAM(p, 0);
        BE_UINT8_TO_STREAM(p, 0x6A);
        BE_UINT16_TO_STREAM(p, cmd_param->length);
        memcpy(p, cmd_param->search_str, cmd_param->length);

        ret = bt_avrcp_browsing_send_generic_cmd(link, buf, 7 + cmd_param->length);
        free(buf);
    }

    return ret;
}

bool bt_avrcp_browsing_send_cmd_set_browsed_player(T_BT_AVRCP_LINK *link,
                                                   uint16_t         player_id)
{
    uint8_t  buf[5];
    uint8_t *p;

    p = buf;
    BE_UINT8_TO_STREAM(p, PDU_ID_SET_BROWSED_PLAYER);
    BE_UINT8_TO_STREAM(p, 0);
    BE_UINT8_TO_STREAM(p, 0x2);
    BE_UINT16_TO_ARRAY(&buf[3], player_id);

    return bt_avrcp_browsing_send_generic_cmd(link, buf, sizeof(buf));
}

bool bt_avrcp_browsing_send_cmd_change_path(T_BT_AVRCP_LINK            *link,
                                            T_BT_AVRCP_REQ_CHANGE_PATH *cmd_param)
{
    uint8_t  buf[14];
    uint8_t *p;

    p = buf;
    BE_UINT8_TO_STREAM(p, PDU_ID_CHANGE_PATH);
    BE_UINT8_TO_STREAM(p, 0);
    BE_UINT8_TO_STREAM(p, 0xB);
    BE_UINT16_TO_STREAM(p, cmd_param->uid_counter);
    BE_UINT8_TO_STREAM(p, cmd_param->direction);
    BE_UINT64_TO_STREAM(p, cmd_param->folder_uid);

    return bt_avrcp_browsing_send_generic_cmd(link, buf, sizeof(buf));
}

void bt_avrcp_browsing_cmd_process(T_BT_AVRCP_LINK *link)
{
    T_BT_AVRCP_BROWSING_CMD *cmd;
    int32_t                  ret = 0;
    bool                     res = false;

    if (link->browsing_chann.state != BT_AVRCP_STATE_CONNECTED)
    {
        ret = 1;
        goto fail_invalid_state;
    }

    if (link->browsing_chann.cmd_credits == 0)
    {
        ret = 2;
        goto fail_no_credits;
    }

    cmd = os_queue_out(&link->browsing_chann.cmd_queue);
    if (cmd != NULL)
    {
        switch (cmd->pdu_id)
        {
        case PDU_ID_GET_FOLDER_ITEMS:
            res = bt_avrcp_browsing_send_cmd_get_folder_items(link,
                                                              (T_BT_AVRCP_REQ_GET_FOLDER_ITEMS *)&cmd->cmd_param.get_folder_items);
            break;

        case PDU_ID_GET_ITEM_ATTRS:
            res = bt_avrcp_browsing_send_cmd_get_item_attrs(link,
                                                            (T_BT_AVRCP_REQ_GET_ITEM_ATTRS *)&cmd->cmd_param.get_item_attrs);
            break;

        case PDU_ID_SEARCH:
            {
                res = bt_avrcp_browsing_send_cmd_search(link,
                                                        (T_BT_AVRCP_REQ_SEARCH *)&cmd->cmd_param.search);

                if (cmd->cmd_param.search.search_str != NULL)
                {
                    free(cmd->cmd_param.search.search_str);
                    cmd->cmd_param.search.search_str = NULL;
                }
            }
            break;

        case PDU_ID_SET_BROWSED_PLAYER:
            res = bt_avrcp_browsing_send_cmd_set_browsed_player(link,
                                                                cmd->cmd_param.player_id);
            break;

        case PDU_ID_CHANGE_PATH:
            res = bt_avrcp_browsing_send_cmd_change_path(link,
                                                         (T_BT_AVRCP_REQ_CHANGE_PATH *)&cmd->cmd_param.change_path);
            break;

        default:
            break;
        }

        free(cmd);

        if (res == false)
        {
            ret = 3;
            goto fail_send_cmd;
        }
        else
        {
            link->browsing_chann.cmd_credits = 0;
        }
    }

    return;

fail_send_cmd:
fail_no_credits:
fail_invalid_state:
    PROFILE_PRINT_ERROR1("bt_avrcp_browsing_cmd_process: failed %d", -ret);
}

bool bt_avrcp_browsing_cmd_enqueue(T_BT_AVRCP_LINK *link,
                                   uint8_t          pdu_id,
                                   void            *param)
{
    T_BT_AVRCP_BROWSING_CMD *cmd;
    int32_t                  ret = 0;

    if (link->browsing_chann.cmd_queue.count == AVRCP_MAX_QUEUED_CMD)
    {
        ret = 1;
        goto fail_queue_full;
    }

    cmd = calloc(1, sizeof(T_BT_AVRCP_BROWSING_CMD));
    if (cmd == NULL)
    {
        ret = 2;
        goto fail_alloc_mem;
    }

    cmd->pdu_id = pdu_id;
    switch (pdu_id)
    {
    case PDU_ID_GET_FOLDER_ITEMS:
        memcpy(&cmd->cmd_param.get_folder_items, param, sizeof(T_BT_AVRCP_REQ_GET_FOLDER_ITEMS));
        break;

    case PDU_ID_GET_ITEM_ATTRS:
        memcpy(&cmd->cmd_param.get_item_attrs, param, sizeof(T_BT_AVRCP_REQ_GET_ITEM_ATTRS));
        break;

    case PDU_ID_SEARCH:
        {
            T_BT_AVRCP_REQ_SEARCH *tmp = (T_BT_AVRCP_REQ_SEARCH *)param;

            cmd->cmd_param.search.search_str = malloc(tmp->length);
            if (cmd->cmd_param.search.search_str == NULL)
            {
                ret = 3;
                goto fail_alloc_mem;
            }

            cmd->cmd_param.search.length = tmp->length;
            memcpy(cmd->cmd_param.search.search_str, tmp->search_str, tmp->length);
        }
        break;

    case PDU_ID_SET_BROWSED_PLAYER:
        cmd->cmd_param.player_id = *(uint16_t *)param;
        break;

    case PDU_ID_CHANGE_PATH:
        memcpy(&cmd->cmd_param.change_path, param, sizeof(T_BT_AVRCP_REQ_CHANGE_PATH));
        break;

    default:
        {
            ret = 4;
            goto fail_invalid_pdu;
        }
    }

    os_queue_in(&link->browsing_chann.cmd_queue, cmd);
    bt_avrcp_browsing_cmd_process(link);
    return true;

fail_invalid_pdu:
fail_alloc_mem:
    if (cmd != NULL)
    {
        free(cmd);
    }
fail_queue_full:
    PROFILE_PRINT_ERROR1("bt_avrcp_browsing_cmd_enqueue: failed %d", -ret);
    return false;
}

void bt_avrcp_browsing_handle_data_ind(uint16_t  cid,
                                       uint8_t   transact,
                                       uint8_t   cr_type,
                                       uint8_t  *data,
                                       uint16_t  length)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;
        uint8_t          pdu_id;
        int              i;

        BE_STREAM_TO_UINT8(pdu_id, data);

        memcpy(payload.bd_addr, link->bd_addr, 6);

        if (cr_type == AVCTP_MSG_TYPE_RSP)
        {
            link->browsing_chann.cmd_credits = 1;

            switch (pdu_id)
            {
            case PDU_ID_SET_BROWSED_PLAYER:
                {
                    T_BT_AVRCP_RSP_SET_BROWSED_PLAYER tmp;
                    T_BT_AVRCP_FOLDER  *folders;

                    BE_STREAM_TO_UINT8(tmp.status_code, data);
                    BE_STREAM_TO_UINT16(tmp.uid_counter, data);
                    BE_STREAM_TO_UINT32(tmp.num_of_items, data);
                    BE_STREAM_TO_UINT16(tmp.character_set_id, data);
                    BE_STREAM_TO_UINT8(tmp.folder_depth, data);

                    folders = malloc(sizeof(T_BT_AVRCP_FOLDER) * tmp.folder_depth);
                    if (folders != NULL)
                    {
                        for (i = 0; i < tmp.folder_depth; i++)
                        {
                            BE_STREAM_TO_UINT16(folders[i].name_length, data);
                            folders[i].p_name = data++;
                        }

                        tmp.p_folders = folders;
                        payload.msg_buf = &tmp;
                        bt_mgr_dispatch(BT_MSG_AVRCP_SET_BROWSED_PLAYER, &payload);
                        free(folders);
                    }
                }
                break;

            case PDU_ID_GET_FOLDER_ITEMS:
                {
                    T_BT_AVRCP_RSP_GET_FOLDER_ITEMS  tmp;
                    uint8_t                         *buf = NULL;

                    BE_STREAM_TO_UINT8(tmp.status_code, data);
                    BE_STREAM_TO_UINT16(tmp.uid_counter, data);
                    BE_STREAM_TO_UINT16(tmp.num_of_items, data);

                    if (tmp.num_of_items > 0)
                    {
                        tmp.item_type = *data;

                        switch (tmp.item_type)
                        {
                        case BT_AVRCP_ITEM_TYPE_MEDIA_PLAYER:
                            {
                                T_BT_AVRCP_MEDIA_PLAYER_ITEM *item;

                                buf = malloc(sizeof(T_BT_AVRCP_MEDIA_PLAYER_ITEM) * tmp.num_of_items);
                                if (buf != NULL)
                                {
                                    for (i = 0; i < tmp.num_of_items; i++)
                                    {
                                        item = (T_BT_AVRCP_MEDIA_PLAYER_ITEM *)buf + i;

                                        BE_STREAM_TO_UINT8(item->item_type, data);
                                        BE_STREAM_TO_UINT16(item->item_length, data);
                                        BE_STREAM_TO_UINT16(item->player_id, data);
                                        BE_STREAM_TO_UINT8(item->major_type, data);
                                        BE_STREAM_TO_UINT32(item->sub_type, data);
                                        BE_STREAM_TO_UINT8(item->play_status, data);
                                        STREAM_TO_ARRAY(item->feature_bitmask, data, 16);
                                        BE_STREAM_TO_UINT16(item->character_set_id, data);
                                        BE_STREAM_TO_UINT16(item->display_name_length, data);
                                        item->display_name = data;
                                        data += item->display_name_length;
                                    }

                                    tmp.u.p_media_player_items = (T_BT_AVRCP_MEDIA_PLAYER_ITEM *)buf;
                                }
                            }
                            break;

                        case BT_AVRCP_ITEM_TYPE_FOLDER:
                            {
                                T_BT_AVRCP_FOLDER_ITEM *item;

                                buf = malloc(sizeof(T_BT_AVRCP_FOLDER_ITEM) * tmp.num_of_items);
                                if (buf != NULL)
                                {
                                    for (i = 0; i < tmp.num_of_items; i++)
                                    {
                                        item = (T_BT_AVRCP_FOLDER_ITEM *)buf + i;

                                        BE_STREAM_TO_UINT8(item->item_type, data);
                                        BE_STREAM_TO_UINT16(item->item_length, data);
                                        BE_STREAM_TO_UINT64(item->folder_uid, data);
                                        BE_STREAM_TO_UINT8(item->folder_type, data);
                                        BE_STREAM_TO_UINT8(item->is_playable, data);
                                        BE_STREAM_TO_UINT16(item->character_set_id, data);
                                        BE_STREAM_TO_UINT16(item->display_name_length, data);
                                        item->display_name = data;
                                        data += item->display_name_length;
                                    }

                                    tmp.u.p_folder_items = (T_BT_AVRCP_FOLDER_ITEM *)buf;
                                }
                            }
                            break;

                        case BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT:
                            {
                                T_BT_AVRCP_MEDIA_ELEMENT_ITEM *item;

                                buf = malloc(sizeof(T_BT_AVRCP_MEDIA_ELEMENT_ITEM) * tmp.num_of_items);
                                if (buf != NULL)
                                {
                                    for (i = 0; i < tmp.num_of_items; i++)
                                    {
                                        uint8_t j;

                                        item = (T_BT_AVRCP_MEDIA_ELEMENT_ITEM *)buf + i;
                                        BE_STREAM_TO_UINT8(item->item_type, data);
                                        BE_STREAM_TO_UINT16(item->item_length, data);
                                        BE_STREAM_TO_UINT64(item->media_element_uid, data);
                                        BE_STREAM_TO_UINT8(item->media_type, data);
                                        BE_STREAM_TO_UINT16(item->character_set_id, data);
                                        BE_STREAM_TO_UINT16(item->display_name_length, data);
                                        item->display_name = data;
                                        data += item->display_name_length;
                                        BE_STREAM_TO_UINT8(item->num_of_attr, data);

                                        for (j = 0; j < item->num_of_attr; j++)
                                        {
                                            BE_STREAM_TO_UINT32(item->p_attr[j].attribute_id, data);
                                            BE_STREAM_TO_UINT16(item->p_attr[j].character_set_id, data);
                                            BE_STREAM_TO_UINT16(item->p_attr[j].length, data);
                                            item->p_attr[j].p_buf = data;
                                            data += item->p_attr[j].length;
                                        }
                                    }
                                    tmp.u.p_media_element_items = (T_BT_AVRCP_MEDIA_ELEMENT_ITEM *)buf;
                                }
                            }
                            break;

                        default:
                            return;
                        }
                    }

                    payload.msg_buf = &tmp;
                    bt_mgr_dispatch(BT_MSG_AVRCP_FOLDER_ITEMS, &payload);

                    if (buf != NULL)
                    {
                        free(buf);
                    }
                }
                break;

            case PDU_ID_CHANGE_PATH:
                {
                    T_BT_AVRCP_RSP_CHANGE_PATH tmp;

                    BE_STREAM_TO_UINT8(tmp.status_code, data);
                    BE_STREAM_TO_UINT32(tmp.num_of_items, data);

                    payload.msg_buf = &tmp;
                    bt_mgr_dispatch(BT_MSG_AVRCP_CHANGE_PATH, &payload);
                }
                break;

            case PDU_ID_GET_ITEM_ATTRS:
                {
                    T_BT_AVRCP_RSP_GET_ITEM_ATTR tmp;
                    T_BT_AVRCP_ELEMENT_ATTR *attr;

                    BE_STREAM_TO_UINT8(tmp.state, data);
                    BE_STREAM_TO_UINT8(tmp.num_of_attr, data);

                    attr = malloc(sizeof(T_BT_AVRCP_ELEMENT_ATTR) * tmp.num_of_attr);
                    if (attr != NULL)
                    {
                        for (i = 0; i < tmp.num_of_attr; i++)
                        {
                            BE_STREAM_TO_UINT32(attr[i].attribute_id, data);
                            BE_STREAM_TO_UINT16(attr[i].character_set_id, data);
                            BE_STREAM_TO_UINT16(attr[i].length, data);
                            attr[i].p_buf = data;
                            data += attr[i].length;
                        }

                        tmp.attr = attr;
                        payload.msg_buf = &tmp;
                        bt_mgr_dispatch(BT_MSG_AVRCP_ITEM_ATTR, &payload);
                        free(attr);
                    }
                }
                break;

            case PDU_ID_SEARCH:
                {
                    T_BT_AVRCP_RSP_SEARCH tmp;

                    BE_STREAM_TO_UINT8(tmp.status_code, data);
                    BE_STREAM_TO_UINT16(tmp.uid_counter, data);
                    BE_STREAM_TO_UINT32(tmp.num_of_items, data);

                    payload.msg_buf = &tmp;
                    bt_mgr_dispatch(BT_MSG_AVRCP_SEARCH, &payload);
                }
                break;

            default:
                break;
            }

            bt_avrcp_browsing_cmd_process(link);
        }
        else if (cr_type == AVCTP_MSG_TYPE_CMD)
        {
            link->browsing_chann.pending_transact = transact;

            switch (pdu_id)
            {
            case PDU_ID_GET_FOLDER_ITEMS:
                {
                    T_BT_AVRCP_REQ_GET_FOLDER_ITEMS cmd_get_folder_items;

                    BE_STREAM_TO_UINT8(cmd_get_folder_items.scope, data);
                    BE_STREAM_TO_UINT32(cmd_get_folder_items.start_item, data);
                    BE_STREAM_TO_UINT32(cmd_get_folder_items.end_item, data);
                    BE_STREAM_TO_UINT8(cmd_get_folder_items.attr_count, data);
                    for (i = 0; i < cmd_get_folder_items.attr_count; i++)
                    {
                        BE_STREAM_TO_UINT32(cmd_get_folder_items.attr_id[i], data);
                    }

                    payload.msg_buf = &cmd_get_folder_items;
                    bt_mgr_dispatch(BT_MSG_AVRCP_GET_FOLDER_ITEMS, &payload);
                }
                break;

            case PDU_ID_GET_TOTAL_NUM_OF_ITEMS:
                {
                    uint8_t scope_id;

                    BE_STREAM_TO_UINT8(scope_id, data);
                    payload.msg_buf = &scope_id;
                    bt_mgr_dispatch(BT_MSG_AVRCP_GET_TOTAL_NUM_OF_ITEMS, &payload);
                }
                break;

            default:
                {
                    uint8_t param[] =
                    {
                        PDU_ID_GENERAL_REJECT,
                        0x00,
                        0x01,
                        BT_AVRCP_RSP_STATUS_INVALID_COMMAND
                    };

                    bt_avrcp_browsing_send_generic_rsp(link->bd_addr, param, sizeof(param));
                }
                break;
            }
        }
    }
}

void bt_avrcp_handle_browsing_conn_ind(T_MPA_L2C_CONN_IND *ind)
{
    T_BT_AVRCP_LINK *link;

    PROFILE_PRINT_TRACE2("bt_avrcp_handle_browsing_conn_ind: bd_addr [%s], cid 0x%04x",
                         TRACE_BDADDR(ind->bd_addr), ind->cid);

    link = bt_avrcp_find_link_by_addr(ind->bd_addr);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, ind->bd_addr, 6);
        payload.msg_buf = NULL;
        link->browsing_chann.state = BT_AVRCP_STATE_CONNECTING;
        link->browsing_chann.cid = ind->cid;
        link->browsing_chann.cmd_credits = 0;
        bt_mgr_dispatch(BT_MSG_AVRCP_BROWSING_CONN_IND, &payload);
    }
    else
    {
        avctp_browsing_connect_cfm(ind->bd_addr, false);
    }
}

void bt_avrcp_handle_browsing_conn_rsp(T_MPA_L2C_CONN_RSP *ind)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(ind->bd_addr);
    if (link != NULL)
    {
        link->browsing_chann.cid = ind->cid;
        link->browsing_chann.cmd_credits = 0;
    }
}

void bt_avrcp_handle_browsing_conn_cmpl_ind(T_MPA_L2C_CONN_CMPL_INFO *ind)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(ind->bd_addr);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, ind->bd_addr, 6);
        link->browsing_chann.state = BT_AVRCP_STATE_CONNECTED;
        link->browsing_chann.cmd_credits = 1;
        link->browsing_chann.transact_label = 0;
        link->browsing_chann.pending_transact = 0xff;

        bt_mgr_dispatch(BT_MSG_AVRCP_BROWSING_CONN_CMPL, &payload);

        bt_avrcp_browsing_cmd_process(link);
    }
}

void bt_avrcp_handle_browsing_disconn_ind(uint16_t cid,
                                          uint16_t cause)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_cid(cid);
    if (link != NULL)
    {
        T_BT_AVRCP_BROWSING_CMD *data;
        T_BT_AVRCP_BROWSING_CMD *next;
        T_BT_MSG_PAYLOAD         payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);

        PROFILE_PRINT_TRACE2("bt_avrcp_handle_browsing_disconn_ind: bd_addr [%s], cid 0x%04x",
                             TRACE_BDADDR(link->bd_addr), cid);

        data = (T_BT_AVRCP_BROWSING_CMD *)link->browsing_chann.cmd_queue.p_first;
        while (data)
        {
            next = (T_BT_AVRCP_BROWSING_CMD *)data->next;

            os_queue_delete(&link->browsing_chann.cmd_queue, data);
            if (data->pdu_id == PDU_ID_SEARCH)
            {
                if (data->cmd_param.search.search_str != NULL)
                {
                    free(data->cmd_param.search.search_str);
                }
            }
            free(data);

            data = next;
        }

        payload.msg_buf = &cause;
        bt_mgr_dispatch(BT_MSG_AVRCP_BROWSING_DISCONN_CMPL, &payload);
        memset(&(link->browsing_chann), 0, sizeof(T_BT_AVRCP_BROWSING_CHANN));
    }
}

bool bt_avrcp_browsing_connect_req(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if ((link->ctrl_chann.state == BT_AVRCP_STATE_CONNECTED) &&
            ((link->browsing_chann.state == BT_AVRCP_STATE_DISCONNECTED) ||
             (link->browsing_chann.state == BT_AVRCP_STATE_ALLOCATED)))
        {
            bt_sniff_mode_exit(bd_addr, false);
            link->browsing_chann.state = BT_AVRCP_STATE_CONNECTING;
            return avctp_browsing_connect_req(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;
    bool ret = false;

    PROFILE_PRINT_TRACE1("bt_avrcp_browsing_disconnect_req: bd_addr[%s]", TRACE_BDADDR(bd_addr));

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->browsing_chann.state == BT_AVRCP_STATE_CONNECTED)
        {
            bt_sniff_mode_exit(bd_addr, false);
            ret = avctp_browsing_disconnect_req(link->bd_addr);
            if (ret)
            {
                link->browsing_chann.state = BT_AVRCP_STATE_DISCONNECTING;
            }
        }
    }

    return ret;
}

bool bt_avrcp_browsing_connect_cfm(uint8_t bd_addr[6],
                                   bool    accept)
{
    T_BT_AVRCP_LINK *link;

    bt_sniff_mode_exit(bd_addr, false);

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (accept == false)
        {
            link->browsing_chann.state = BT_AVRCP_STATE_DISCONNECTED;
        }
        return avctp_browsing_connect_cfm(bd_addr, accept);
    }
    return avctp_browsing_connect_cfm(bd_addr, false);
}

bool bt_avrcp_app_setting_attrs_list(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_LIST_APP_SETTING_ATTRS, NULL);
    }

    return false;
}

bool bt_avrcp_app_setting_values_list(uint8_t bd_addr[6],
                                      uint8_t attr_id)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_LIST_APP_SETTING_VALUES, &attr_id);
    }

    return false;
}

bool bt_avrcp_app_setting_value_get(uint8_t  bd_addr[6],
                                    uint8_t  attr_num,
                                    uint8_t *attr_list)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_GET_APP_SETTING_VALUE tmp;

        tmp.attr_num = attr_num;

        if (attr_num < 4)
        {
            memcpy(tmp.attr_id, attr_list, attr_num);
        }
        else
        {
            memcpy(tmp.attr_id, attr_list, 4);
        }

        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_GET_CURRENT_APP_SETTING_VALUE, &tmp);
    }

    return false;
}

bool bt_avrcp_app_setting_value_set(uint8_t                 bd_addr[6],
                                    uint8_t                 attr_num,
                                    T_BT_AVRCP_APP_SETTING *attr_list)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_SET_APP_SETTING_VALUE tmp;

        tmp.attr_num = attr_num;

        if (attr_num < 4)
        {
            memcpy(tmp.app_setting, attr_list, attr_num);
        }
        else
        {
            memcpy(tmp.app_setting, attr_list, 4);
        }

        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_SET_APP_SETTING_VALUE, &tmp);
    }

    return false;
}

bool bt_avrcp_continuing_rsp_req(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_REQUEST_CONTINUE_RSP, &link->ctrl_chann.last_pdu_id);
    }

    return false;
}

bool bt_avrcp_continuing_rsp_abort(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_ABORT_CONTINUE_RSP, &link->ctrl_chann.last_pdu_id);
    }

    return false;
}

bool bt_avrcp_addressed_player_set(uint8_t  bd_addr[6],
                                   uint16_t player_id)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_SET_ADDRESSED_PLAYER, &player_id);
    }

    return false;
}

bool bt_avrcp_item_play(uint8_t  bd_addr[6],
                        uint8_t  scope,
                        uint64_t uid,
                        uint16_t uid_counter)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PLAY_ITEM tmp;

        tmp.scope_id = (T_BT_AVRCP_SCOPE_ID)scope;
        tmp.uid = uid;
        tmp.uid_counter = uid_counter;

        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PLAY_ITEM, &tmp);
    }

    return false;
}

bool bt_avrcp_group_navigate(uint8_t bd_addr[6],
                             bool    next)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = (next ? AVRCP_PASS_THROUGH_NEXT_GROUP : AVRCP_PASS_THROUGH_PREVIOUS_GROUP);
        tmp.pressed = true;

        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_cmd_enqueue(link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool bt_avrcp_browsing_folder_items_get(uint8_t   bd_addr[6],
                                        uint8_t   scope,
                                        uint32_t  start_item,
                                        uint32_t  end_item,
                                        uint8_t   attr_num,
                                        uint32_t *attr)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_GET_FOLDER_ITEMS tmp;

        tmp.scope = scope;
        tmp.start_item = start_item;
        tmp.end_item = end_item;
        tmp.attr_count = attr_num;

        if (attr_num < 8)
        {
            memcpy(tmp.attr_id, attr, attr_num);
        }
        else
        {
            memcpy(tmp.attr_id, attr, 8);
        }

        bt_sniff_mode_exit(bd_addr, false);

        if ((link->browsing_chann.state == BT_AVRCP_STATE_DISCONNECTED) ||
            (link->browsing_chann.state == BT_AVRCP_STATE_ALLOCATED))
        {
            bt_avrcp_browsing_connect_req(bd_addr);
        }

        return bt_avrcp_browsing_cmd_enqueue(link, PDU_ID_GET_FOLDER_ITEMS, &tmp);
    }

    return false;
}

bool bt_avrcp_browsing_item_attrs_get(uint8_t   bd_addr[6],
                                      uint8_t   scope,
                                      uint64_t  uid,
                                      uint16_t  uid_counter,
                                      uint8_t   attr_num,
                                      uint32_t *attr)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_GET_ITEM_ATTRS tmp;

        tmp.scope = scope;
        tmp.uid = uid;
        tmp.uid_counter = uid_counter;
        tmp.num_of_attr = attr_num;

        if (attr_num < 8)
        {
            memcpy(tmp.attr_id, attr, attr_num);
        }
        else
        {
            memcpy(tmp.attr_id, attr, 8);
        }

        bt_sniff_mode_exit(bd_addr, false);

        if ((link->browsing_chann.state == BT_AVRCP_STATE_DISCONNECTED) ||
            (link->browsing_chann.state == BT_AVRCP_STATE_ALLOCATED))
        {
            bt_avrcp_browsing_connect_req(bd_addr);
        }

        return bt_avrcp_browsing_cmd_enqueue(link, PDU_ID_GET_ITEM_ATTRS, &tmp);
    }

    return false;
}

bool bt_avrcp_browsing_search(uint8_t   bd_addr[6],
                              uint16_t  length,
                              uint8_t  *search_str)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_SEARCH tmp;

        tmp.length = length;
        tmp.search_str = search_str;

        bt_sniff_mode_exit(bd_addr, false);

        if ((link->browsing_chann.state == BT_AVRCP_STATE_DISCONNECTED) ||
            (link->browsing_chann.state == BT_AVRCP_STATE_ALLOCATED))
        {
            bt_avrcp_browsing_connect_req(bd_addr);
        }

        return bt_avrcp_browsing_cmd_enqueue(link, PDU_ID_SEARCH, &tmp);
    }

    return false;
}

bool bt_avrcp_browsing_browsed_player_set(uint8_t  bd_addr[6],
                                          uint16_t player_id)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);

        if ((link->browsing_chann.state == BT_AVRCP_STATE_DISCONNECTED) ||
            (link->browsing_chann.state == BT_AVRCP_STATE_ALLOCATED))
        {
            bt_avrcp_browsing_connect_req(bd_addr);
        }

        return bt_avrcp_browsing_cmd_enqueue(link, PDU_ID_SET_BROWSED_PLAYER, &player_id);
    }

    return false;
}

bool bt_avrcp_browsing_path_change(uint8_t  bd_addr[6],
                                   uint16_t uid_counter,
                                   uint8_t  direction,
                                   uint64_t folder_uid)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_AVRCP_REQ_CHANGE_PATH tmp;

        tmp.uid_counter = uid_counter;
        tmp.direction = direction;
        tmp.folder_uid = folder_uid;

        bt_sniff_mode_exit(bd_addr, false);

        if ((link->browsing_chann.state == BT_AVRCP_STATE_DISCONNECTED) ||
            (link->browsing_chann.state == BT_AVRCP_STATE_ALLOCATED))
        {
            bt_avrcp_browsing_connect_req(bd_addr);
        }

        return bt_avrcp_browsing_cmd_enqueue(link, PDU_ID_CHANGE_PATH, &tmp);
    }

    return false;
}

bool bt_avrcp_browsing_folder_items_get_rsp(uint8_t                      bd_addr[6],
                                            T_BT_AVRCP_RSP_ERROR_STATUS  status,
                                            uint16_t                     uid_counter,
                                            uint16_t                     num_of_items,
                                            void                        *items)
{
    T_BT_AVRCP_LINK *link;
    uint8_t          item_type;
    uint8_t          i;
    uint8_t         *param;
    uint8_t         *p;
    uint16_t         param_len = 5;
    int32_t          ret = 0;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_find_link;
    }

    if ((num_of_items == 0) || (items == NULL))
    {
        ret = 2;
        goto fail_check_param;
    }

    item_type = *(uint8_t *)items;
    switch (item_type)
    {
    case BT_AVRCP_ITEM_TYPE_MEDIA_PLAYER:
        {
            T_BT_AVRCP_MEDIA_PLAYER_ITEM *item = (T_BT_AVRCP_MEDIA_PLAYER_ITEM *)items;

            for (i = 0; i < num_of_items; i++)
            {
                param_len += 3;
                param_len += item[i].item_length;
            }
        }
        break;

    case BT_AVRCP_ITEM_TYPE_FOLDER:
        {
            T_BT_AVRCP_FOLDER_ITEM *item = (T_BT_AVRCP_FOLDER_ITEM *)items;

            for (i = 0; i < num_of_items; i++)
            {
                param_len += 3;
                param_len += item[i].item_length;
            }
        }
        break;

    case BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT:
        {
            T_BT_AVRCP_MEDIA_ELEMENT_ITEM *item = (T_BT_AVRCP_MEDIA_ELEMENT_ITEM *)items;

            for (i = 0; i < num_of_items; i++)
            {
                param_len += 3;
                param_len += item[i].item_length;
            }
        }
        break;

    default:
        {
            ret = 3;
            goto fail_invalid_item_type;
        }
    }

    param = malloc(param_len + 3);
    if (param == NULL)
    {
        ret = 4;
        goto fail_alloc_mem;
    }

    p = param;
    BE_UINT8_TO_STREAM(p, PDU_ID_GET_FOLDER_ITEMS);
    BE_UINT16_TO_STREAM(p, param_len);
    BE_UINT8_TO_STREAM(p, status);
    BE_UINT16_TO_STREAM(p, uid_counter);
    BE_UINT16_TO_STREAM(p, num_of_items);

    switch (item_type)
    {
    case BT_AVRCP_ITEM_TYPE_MEDIA_PLAYER:
        {
            T_BT_AVRCP_MEDIA_PLAYER_ITEM *item = (T_BT_AVRCP_MEDIA_PLAYER_ITEM *)items;

            for (i = 0; i < num_of_items; i++)
            {
                BE_UINT8_TO_STREAM(p, item->item_type);
                BE_UINT16_TO_STREAM(p, item->item_length);
                BE_UINT16_TO_STREAM(p, item->player_id);
                BE_UINT8_TO_STREAM(p, item->major_type);
                BE_UINT32_TO_STREAM(p, item->sub_type);
                BE_UINT8_TO_STREAM(p, item->play_status);
                ARRAY_TO_STREAM(p, item->feature_bitmask, 16);
                BE_UINT16_TO_STREAM(p, item->character_set_id);
                BE_UINT16_TO_STREAM(p, item->display_name_length);
                ARRAY_TO_STREAM(p, item->display_name, item->display_name_length);
            }
        }
        break;

    case BT_AVRCP_ITEM_TYPE_FOLDER:
        {
            T_BT_AVRCP_FOLDER_ITEM *item = (T_BT_AVRCP_FOLDER_ITEM *)items;

            for (i = 0; i < num_of_items; i++)
            {
                BE_UINT8_TO_STREAM(p, item->item_type);
                BE_UINT16_TO_STREAM(p, item->item_length);
                BE_UINT64_TO_STREAM(p, item->folder_uid);
                BE_UINT8_TO_STREAM(p, item->folder_type);
                BE_UINT8_TO_STREAM(p, item->is_playable);
                BE_UINT16_TO_STREAM(p, item->character_set_id);
                BE_UINT16_TO_STREAM(p, item->display_name_length);
                ARRAY_TO_STREAM(p, item->display_name, item->display_name_length);
            }
        }
        break;

    case BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT:
        {
            T_BT_AVRCP_MEDIA_ELEMENT_ITEM *item = (T_BT_AVRCP_MEDIA_ELEMENT_ITEM *)items;
            uint8_t                        j;

            for (i = 0; i < num_of_items; i++)
            {
                BE_UINT8_TO_STREAM(p, item->item_type);
                BE_UINT16_TO_STREAM(p, item->item_length);
                BE_UINT64_TO_STREAM(p, item->media_element_uid);
                BE_UINT8_TO_STREAM(p, item->media_type);
                BE_UINT16_TO_STREAM(p, item->character_set_id);
                BE_UINT16_TO_STREAM(p, item->display_name_length);
                ARRAY_TO_STREAM(p, item->display_name, item->display_name_length);
                BE_UINT8_TO_STREAM(p, item->num_of_attr);

                for (j = 0; j < item->num_of_attr; j++)
                {
                    BE_UINT32_TO_STREAM(p, item->p_attr[j].attribute_id);
                    BE_UINT16_TO_STREAM(p, item->p_attr[j].character_set_id);
                    BE_UINT16_TO_STREAM(p, item->p_attr[j].length);
                    ARRAY_TO_STREAM(p, item->p_attr[j].p_buf, item->p_attr[j].length);
                }
            }
        }
        break;
    }

    bt_sniff_mode_exit(bd_addr, false);

    if (bt_avrcp_browsing_send_generic_rsp(bd_addr, param, param_len + 3) == false)
    {
        ret = 5;
        goto fail_send_rsp;
    }

    free(param);
    return true;

fail_send_rsp:
    free(param);
fail_alloc_mem:
fail_invalid_item_type:
fail_check_param:
fail_find_link:
    PROFILE_PRINT_ERROR1("bt_avrcp_browsing_folder_items_get_rsp: failed %d", -ret);
    return false;
}

bool bt_avrcp_browsing_total_num_of_items_get_rsp(uint8_t                     bd_addr[6],
                                                  T_BT_AVRCP_RSP_ERROR_STATUS status,
                                                  uint16_t                    uid_counter,
                                                  uint32_t                    num_of_items)
{
    T_BT_AVRCP_LINK *link;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t  param[10];
        uint8_t *p;

        p = param;
        BE_UINT8_TO_STREAM(p, PDU_ID_GET_TOTAL_NUM_OF_ITEMS);
        BE_UINT8_TO_STREAM(p, 0x00);
        BE_UINT8_TO_STREAM(p, 0x07);
        BE_UINT8_TO_STREAM(p, status);
        BE_UINT16_TO_STREAM(p, uid_counter);
        BE_UINT32_TO_STREAM(p, num_of_items);

        bt_sniff_mode_exit(bd_addr, false);
        return bt_avrcp_browsing_send_generic_rsp(bd_addr, param, sizeof(param));
    }

    return false;
}

#else
bool bt_avrcp_browsing_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_browsing_connect_cfm(uint8_t bd_addr[6],
                                   bool    accept)
{
    return false;
}

bool bt_avrcp_app_setting_attrs_list(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_app_setting_values_list(uint8_t bd_addr[6],
                                      uint8_t attr_id)
{
    return false;
}

bool bt_avrcp_app_setting_value_get(uint8_t  bd_addr[6],
                                    uint8_t  attr_num,
                                    uint8_t *attr_list)
{
    return false;
}

bool bt_avrcp_app_setting_value_set(uint8_t                 bd_addr[6],
                                    uint8_t                 attr_num,
                                    T_BT_AVRCP_APP_SETTING *attr_list)
{
    return false;
}

bool bt_avrcp_continuing_rsp_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_continuing_rsp_abort(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_addressed_player_set(uint8_t  bd_addr[6],
                                   uint16_t player_id)
{
    return false;
}

bool bt_avrcp_item_play(uint8_t  bd_addr[6],
                        uint8_t  scope,
                        uint64_t uid,
                        uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_group_navigate(uint8_t bd_addr[6],
                             bool    next)
{
    return false;
}

bool bt_avrcp_browsing_folder_items_get(uint8_t   bd_addr[6],
                                        uint8_t   scope,
                                        uint32_t  start_item,
                                        uint32_t  end_item,
                                        uint8_t   attr_num,
                                        uint32_t *attr)
{
    return false;
}

bool bt_avrcp_browsing_item_attrs_get(uint8_t   bd_addr[6],
                                      uint8_t   scope,
                                      uint64_t  uid,
                                      uint16_t  uid_counter,
                                      uint8_t   attr_num,
                                      uint32_t *attr)
{
    return false;
}

bool bt_avrcp_browsing_search(uint8_t   bd_addr[6],
                              uint16_t  length,
                              uint8_t  *search_str)
{
    return false;
}

bool bt_avrcp_browsing_browsed_player_set(uint8_t  bd_addr[6],
                                          uint16_t player_id)
{
    return false;
}

bool bt_avrcp_browsing_path_change(uint8_t  bd_addr[6],
                                   uint16_t uid_counter,
                                   uint8_t  direction,
                                   uint64_t folder_uid)
{
    return false;
}
#endif

#if (CONFIG_REALTEK_BTM_AVRCP_COVER_ART_SUPPORT == 1)
void bt_avrcp_cover_art_obex_cback(uint8_t  msg_type,
                                   void    *p_msg)
{
    T_BT_AVRCP_LINK *link;
    T_BT_MSG_PAYLOAD payload;

    payload.msg_buf = NULL;

    switch (msg_type)
    {
    case OBEX_MSG_CONN_IND:
        break;

    case OBEX_MSG_CONN_CMPL:
        {
            T_OBEX_CONN_CMPL_DATA *data = (T_OBEX_CONN_CMPL_DATA *)p_msg;

            link = bt_avrcp_find_link_by_addr(data->bd_addr);
            if (link && (link->handle == data->handle))
            {
                link->cover_art_state = BT_AVRCP_COVER_ART_STATE_CONNECTED;
                memcpy(payload.bd_addr, data->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_AVRCP_COVER_ART_CONN_CMPL, &payload);
            }
            else
            {
                obex_disconn_req(data->handle);
            }
        }
        break;

    case OBEX_MSG_DISCONN:
        {
            T_OBEX_DISCONN_CMPL_DATA *data = (T_OBEX_DISCONN_CMPL_DATA *)p_msg;

            link = bt_avrcp_find_link_by_addr(data->bd_addr);
            if (link)
            {
                if (data->handle == link->handle)
                {
                    link->handle = NULL;
                    link->cover_art_state = BT_AVRCP_COVER_ART_STATE_DISCONNECTED;
                    memcpy(payload.bd_addr, data->bd_addr, 6);
                    payload.msg_buf = &data->cause;
                    bt_mgr_dispatch(BT_MSG_AVRCP_COVER_ART_DISCONN_CMPL, &payload);
                }
            }
        }
        break;

    case OBEX_MSG_GET_OBJECT:
        {
            T_OBEX_GET_OBJECT_CMPL_DATA *p_data = (T_OBEX_GET_OBJECT_CMPL_DATA *)p_msg;
            uint8_t                     *p_hdr_data;
            uint16_t                     hdr_data_len;
            bool                         ret;

            link = bt_avrcp_find_link_by_addr(p_data->bd_addr);
            if (link == NULL || link->handle != p_data->handle)
            {
                break;
            }

            if (p_data->rsp_code != OBEX_RSP_SUCCESS && p_data->rsp_code != OBEX_RSP_CONTINUE)
            {
                link->cover_art_state = BT_AVRCP_COVER_ART_STATE_CONNECTED;
                break;
            }

            if (link->cover_art_state == BT_AVRCP_COVER_ART_STATE_GET_IMAGE ||
                link->cover_art_state == BT_AVRCP_COVER_ART_STATE_GET_THUMBNAIL)
            {
                T_BT_AVRCP_COVER_ART_DATA_IND data = {0};

                ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY,
                                           &p_hdr_data, &hdr_data_len);
                if (ret == false)
                {
                    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &p_hdr_data,
                                               &hdr_data_len);
                    if (ret)
                    {
                        data.data_end = true;
                    }
                }
                else
                {
                    data.data_end = false;
                }

                if (ret)
                {
                    data.p_data = p_hdr_data;
                    data.data_len = hdr_data_len;
                }

                /* workaround for iphone cover art which has no END_OF_BODY header. */
                if (p_data->rsp_code == OBEX_RSP_SUCCESS)
                {
                    data.data_end = true;
                }

                if (data.data_end)
                {
                    link->cover_art_state = BT_AVRCP_COVER_ART_STATE_CONNECTED;
                }

                memcpy(payload.bd_addr, p_data->bd_addr, 6);
                payload.msg_buf = &data;
                bt_mgr_dispatch(BT_MSG_AVRCP_COVER_ART_DATA_IND, &payload);
            }
        }
        break;

    default:
        break;
    }
}

bool bt_avrcp_cover_art_connect_req(uint8_t  bd_addr[6],
                                    uint16_t l2c_psm)
{
    T_BT_AVRCP_LINK *link;
    uint8_t         *param;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if ((link->ctrl_chann.state == BT_AVRCP_STATE_CONNECTED) &&
            (link->cover_art_state == BT_AVRCP_COVER_ART_STATE_DISCONNECTED))
        {
            bt_sniff_mode_exit(bd_addr, false);
            link->cover_art_state = BT_AVRCP_COVER_ART_STATE_CONNECTING;

            param = (uint8_t *)avrcp_cover_art_target;
            if (obex_conn_req_over_l2c(bd_addr, param, l2c_psm, bt_avrcp_cover_art_obex_cback, &link->handle))
            {
                return true;
            }
            else
            {
                link->cover_art_state = BT_AVRCP_COVER_ART_STATE_DISCONNECTED;
                return false;
            }
        }
    }

    return false;
}

bool bt_avrcp_cover_art_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_AVRCP_LINK *link;
    bool ret = false;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        ret = obex_disconn_req(link->handle);
        if (ret)
        {
            link->cover_art_state = BT_AVRCP_COVER_ART_STATE_DISCONNECTING;
        }
    }

    return ret;
}

bool bt_avrcp_cover_art_get(uint8_t bd_addr[6],
                            uint8_t image_handle[16])
{
    T_BT_AVRCP_LINK *link;
    bool             ret = false;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);

        if (link->cover_art_state == BT_AVRCP_COVER_ART_STATE_GET_THUMBNAIL)
        {
            ret = obex_get_object_continue(link->handle);
        }
        else if (link->cover_art_state == BT_AVRCP_COVER_ART_STATE_CONNECTED)
        {
            uint16_t pkt_len;
            uint8_t *data;
            uint8_t *p;

            pkt_len = AVRCP_COVER_ART_TYPE_LEN + (3 + AVRCP_IMAGE_HANDLE_LEN);

            data = malloc(pkt_len);
            if (data != NULL)
            {
                p = data;

                ARRAY_TO_STREAM(p, image_thumbnail_type, AVRCP_COVER_ART_TYPE_LEN);

                BE_UINT8_TO_STREAM(p, AVRCP_IMAGE_HANDLE_HEADER_ID);
                BE_UINT16_TO_STREAM(p, 3 + AVRCP_IMAGE_HANDLE_LEN);
                ARRAY_TO_STREAM(p, image_handle, AVRCP_IMAGE_HANDLE_LEN);

                link->cover_art_state = BT_AVRCP_COVER_ART_STATE_GET_THUMBNAIL;

                ret = obex_get_object(link->handle, data, p - data);
                free(data);
            }
        }
    }

    return ret;
}

bool bt_avrcp_cover_art_get_image(uint8_t  bd_addr[6],
                                  uint8_t *img_handle)
{
    T_BT_AVRCP_LINK *link;
    bool             ret = false;

    link = bt_avrcp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->cover_art_state == BT_AVRCP_COVER_ART_STATE_GET_IMAGE)
        {
            ret = obex_get_object_continue(link->handle);
        }
        else if (link->cover_art_state == BT_AVRCP_COVER_ART_STATE_CONNECTED)
        {
            uint16_t  pkt_len;
            uint8_t  *data;
            uint8_t  *p;
            pkt_len = AVRCP_COVER_ART_TYPE_LEN + (3 + AVRCP_IMAGE_HANDLE_LEN) + 3;

            data = malloc(pkt_len);
            if (data != NULL)
            {
                p = data;

                ARRAY_TO_STREAM(p, image_type, AVRCP_COVER_ART_TYPE_LEN);

                BE_UINT8_TO_STREAM(p, AVRCP_IMAGE_HANDLE_HEADER_ID);
                BE_UINT16_TO_STREAM(p, 3 + AVRCP_IMAGE_HANDLE_LEN);
                ARRAY_TO_STREAM(p, img_handle, AVRCP_IMAGE_HANDLE_LEN);
                BE_UINT8_TO_STREAM(p, AVRCP_IMAGE_DESCRIPTION_HEADER_ID);
                BE_UINT16_TO_STREAM(p, 3);

                link->cover_art_state = BT_AVRCP_COVER_ART_STATE_GET_IMAGE;

                ret = obex_get_object(link->handle, data, p - data);
                free(data);
            }
        }
    }

    return ret;
}

#else
bool bt_avrcp_cover_art_connect_req(uint8_t  bd_addr[6],
                                    uint16_t l2c_psm)
{
    return false;
}

bool bt_avrcp_cover_art_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_cover_art_get(uint8_t bd_addr[6],
                            uint8_t image_handle[16])
{
    return false;
}

bool bt_avrcp_cover_art_get_image(uint8_t  bd_addr[6],
                                  uint8_t *img_handle)
{
    return false;
}
#endif

void bt_avrcp_callback(uint16_t     cid,
                       T_AVCTP_MSG  msg_type,
                       void        *msg_buf)
{
    switch (msg_type)
    {
    case AVCTP_MSG_CONN_IND:
        bt_avrcp_handle_conn_ind((T_MPA_L2C_CONN_IND *)msg_buf);
        break;

    case AVCTP_MSG_CONN_RSP:
        bt_avrcp_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)msg_buf);
        break;

    case AVCTP_MSG_CONN_FAIL:
        {
            uint16_t cause = *((uint16_t *)msg_buf);
            bt_avrcp_handle_conn_fail(cid, cause);
        }
        break;

    case AVCTP_MSG_CONN_CMPL_IND:
        bt_avrcp_handle_conn_cmpl_ind((T_MPA_L2C_CONN_CMPL_INFO *)msg_buf);
        break;

    case AVCTP_MSG_DATA_IND:
        {
            uint8_t *data = ((T_AVCTP_DATA_IND *)msg_buf)->data;
            uint16_t length = ((T_AVCTP_DATA_IND *)msg_buf)->length;
            uint8_t  crtype = ((T_AVCTP_DATA_IND *)msg_buf)->crtype;
            uint8_t  transact_label = ((T_AVCTP_DATA_IND *)msg_buf)->transact_label;
            bt_avrcp_handle_data_ind(cid, transact_label, crtype, data, length);
        }
        break;

    case AVCTP_MSG_DISCONN_IND:
        {
            uint16_t cause = *((uint16_t *)msg_buf);
            bt_avrcp_handle_disconn_ind(cid, cause);
        }
        break;

#if (CONFIG_REALTEK_BTM_AVRCP_BROWSING_SUPPORT == 1)
    case AVCTP_MSG_BROWSING_CONN_IND:
        bt_avrcp_handle_browsing_conn_ind((T_MPA_L2C_CONN_IND *)msg_buf);
        break;

    case AVCTP_MSG_BROWSING_CONN_RSP:
        bt_avrcp_handle_browsing_conn_rsp((T_MPA_L2C_CONN_RSP *)msg_buf);
        break;

    case AVCTP_MSG_BROWSING_CONN_CMPL_IND:
        bt_avrcp_handle_browsing_conn_cmpl_ind((T_MPA_L2C_CONN_CMPL_INFO *)msg_buf);
        break;

    case AVCTP_MSG_BROWSING_DATA_IND:
        {
            uint8_t *data = ((T_AVCTP_DATA_IND *)msg_buf)->data;
            uint16_t length = ((T_AVCTP_DATA_IND *)msg_buf)->length;
            uint8_t  crtype = ((T_AVCTP_DATA_IND *)msg_buf)->crtype;
            uint8_t  transact_label = ((T_AVCTP_DATA_IND *)msg_buf)->transact_label;
            bt_avrcp_browsing_handle_data_ind(cid, transact_label, crtype, data, length);
        }
        break;

    case AVCTP_MSG_BROWSING_DISCONN_IND:
        {
            uint16_t cause = *((uint16_t *)msg_buf);
            bt_avrcp_handle_browsing_disconn_ind(cid, cause);
        }
        break;
#endif

    default:
        break;
    }
}

bool bt_avrcp_init(uint8_t ct_features,
                   uint8_t tg_features)
{
    int32_t ret = 0;

    bt_avrcp = calloc(1, sizeof(T_BT_AVRCP));
    if (bt_avrcp == NULL)
    {
        ret = 1;
        goto fail_alloc_avrcp;
    }

    bt_avrcp->wait_rsp_tout         = 3000;
    bt_avrcp->local_company_id      = COMPANY_BT_SIG;
    bt_avrcp->ct_features           = ct_features;
    bt_avrcp->tg_features           = tg_features;

    if (avctp_init(bt_avrcp_callback) == false)
    {
        ret = 2;
        goto fail_init_avctp;
    }

    return true;

fail_init_avctp:
    free(bt_avrcp);
    bt_avrcp = NULL;
fail_alloc_avrcp:
    PROFILE_PRINT_ERROR1("bt_avrcp_init: failed %d", -ret);
    return false;
}

#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bt_avrcp.h"
#include "bt_avrcp_int.h"

bool bt_avrcp_init(uint8_t ct_features,
                   uint8_t tg_features)
{
    return false;
}

bool bt_avrcp_supported_features_set(uint8_t ct_features,
                                     uint8_t tg_features)
{
    return false;
}

bool bt_avrcp_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_connect_cfm(uint8_t bd_addr[6],
                          bool    accept)
{
    return false;
}

bool bt_avrcp_browsing_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_browsing_connect_cfm(uint8_t bd_addr[6],
                                   bool    accept)
{
    return false;
}

bool bt_avrcp_volume_change_register_rsp(uint8_t bd_addr[6],
                                         uint8_t vol)
{
    return false;
}

bool bt_avrcp_volume_change_req(uint8_t bd_addr[6],
                                uint8_t vol)
{
    return false;
}

bool bt_avrcp_play_status_change_register_rsp(uint8_t                bd_addr[6],
                                              T_BT_AVRCP_PLAY_STATUS play_status)
{
    return false;
}

bool bt_avrcp_play_status_change_req(uint8_t                bd_addr[6],
                                     T_BT_AVRCP_PLAY_STATUS play_status)
{
    return false;
}

bool bt_avrcp_track_change_register_rsp(uint8_t  bd_addr[6],
                                        uint64_t track_id)
{
    return false;
}

bool bt_avrcp_track_change_req(uint8_t  bd_addr[6],
                               uint64_t track_id)
{
    return false;
}

bool bt_avrcp_get_element_attr_req(uint8_t  bd_addr[6],
                                   uint8_t  attr_num,
                                   uint8_t *p_attr)
{
    return false;
}

bool bt_avrcp_get_element_attr_rsp(uint8_t                  bd_addr[6],
                                   uint8_t                  attr_num,
                                   T_BT_AVRCP_ELEMENT_ATTR *p_attr)
{
    return false;
}

bool bt_avrcp_get_play_status_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_get_play_status_rsp(uint8_t                bd_addr[6],
                                  uint32_t               song_length,
                                  uint32_t               song_pos,
                                  T_BT_AVRCP_PLAY_STATUS play_status)
{
    return false;
}

bool bt_avrcp_play(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_pause(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_stop(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_forward(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_backward(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_mute(uint8_t *bd_addr)
{
    return false;
}

bool bt_avrcp_power(uint8_t *bd_addr)
{
    return false;
}

bool bt_avrcp_rewind_start(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_rewind_stop(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_fast_forward_start(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_fast_forward_stop(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_app_setting_attrs_list(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_app_setting_values_list(uint8_t bd_addr[6],
                                      uint8_t attr_id)
{
    return false;
}

bool bt_avrcp_app_setting_value_get(uint8_t  bd_addr[6],
                                    uint8_t  attr_num,
                                    uint8_t *attr_list)
{
    return false;
}

bool bt_avrcp_app_setting_value_set(uint8_t                 bd_addr[6],
                                    uint8_t                 attr_num,
                                    T_BT_AVRCP_APP_SETTING *attr_list)
{
    return false;
}

bool bt_avrcp_continuing_rsp_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_continuing_rsp_abort(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_absolute_volume_set(uint8_t bd_addr[6],
                                  uint8_t volume)
{
    return false;
}

bool bt_avrcp_addressed_player_set(uint8_t  bd_addr[6],
                                   uint16_t player_id)
{
    return false;
}

bool bt_avrcp_addressed_player_set_rsp(uint8_t                     bd_addr[6],
                                       T_BT_AVRCP_RSP_ERROR_STATUS status)
{
    return false;
}

bool bt_avrcp_addressed_player_change_register_rsp(uint8_t  bd_addr[6],
                                                   uint16_t player_id,
                                                   uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_addressed_player_change_req(uint8_t  bd_addr[6],
                                          uint16_t player_id,
                                          uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_item_play(uint8_t  bd_addr[6],
                        uint8_t  scope,
                        uint64_t uid,
                        uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_group_navigate(uint8_t bd_addr[6],
                             bool    next)
{
    return false;
}

bool bt_avrcp_vendor_cmd_send(uint8_t           bd_addr[6],
                              T_BT_AVRCP_CTYPE  ctype,
                              uint32_t          company_id,
                              uint8_t          *pdu,
                              uint16_t          pdu_length)
{
    return false;
}

bool bt_avrcp_vendor_rsp_send(uint8_t              bd_addr[6],
                              T_BT_AVRCP_RESPONSE  response,
                              uint32_t             company_id,
                              uint8_t             *pdu,
                              uint16_t             pdu_length)
{
    return false;
}

bool bt_avrcp_browsing_folder_items_get(uint8_t   bd_addr[6],
                                        uint8_t   scope,
                                        uint32_t  start_item,
                                        uint32_t  end_item,
                                        uint8_t   attr_num,
                                        uint32_t *p_attr)
{
    return false;
}

bool bt_avrcp_browsing_folder_items_get_rsp(uint8_t                      bd_addr[6],
                                            T_BT_AVRCP_RSP_ERROR_STATUS  status,
                                            uint16_t                     uid_counter,
                                            uint16_t                     num_of_items,
                                            void                        *p_items)
{
    return false;
}

bool bt_avrcp_browsing_total_num_of_items_get_rsp(uint8_t                     bd_addr[6],
                                                  T_BT_AVRCP_RSP_ERROR_STATUS status,
                                                  uint16_t                    uid_counter,
                                                  uint32_t                    num_of_items)
{
    return false;
}

bool bt_avrcp_browsing_item_attrs_get(uint8_t   bd_addr[6],
                                      uint8_t   scope,
                                      uint64_t  uid,
                                      uint16_t  uid_counter,
                                      uint8_t   attr_num,
                                      uint32_t *p_attr)
{
    return false;
}

bool bt_avrcp_browsing_search(uint8_t   bd_addr[6],
                              uint16_t  length,
                              uint8_t  *p_search_str)
{
    return false;
}

bool bt_avrcp_browsing_browsed_player_set(uint8_t  bd_addr[6],
                                          uint16_t player_id)
{
    return false;
}

bool bt_avrcp_browsing_path_change(uint8_t  bd_addr[6],
                                   uint16_t uid_counter,
                                   uint8_t  direction,
                                   uint64_t folder_uid)
{
    return false;
}

bool bt_avrcp_cover_art_connect_req(uint8_t  bd_addr[6],
                                    uint16_t l2c_psm)
{
    return false;
}

bool bt_avrcp_cover_art_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_cover_art_get(uint8_t bd_addr[6],
                            uint8_t image_handle[16])
{
    return false;
}

bool bt_avrcp_roleswap_info_get(uint8_t                bd_addr[6],
                                T_ROLESWAP_AVRCP_INFO *info)
{
    return false;
}

bool bt_avrcp_roleswap_info_set(uint8_t                bd_addr[6],
                                T_ROLESWAP_AVRCP_INFO *info)
{
    return false;
}

bool bt_avrcp_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}

#endif
