/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_MAP_SUPPORT == 1)

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "obex.h"
#include "bt_map_int.h"
#include "bt_map.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

#define PATH_TELECOM_LEN            0x10
#define PATH_MSG_LEN                0x08
#define PATH_INBOX_LEN              0x0c
#define PATH_OUTBOX_LEN             0x0e
#define PATH_SENT_LEN               0x0a
#define PATH_DELETED_LEN            0x10
#define PATH_DRAFT_LEN              0x0c

#define MAP_FEAT_BIT_NOTIFICATION_REGISTRATION          0x00000001
#define MAP_FEAT_BIT_NOTIFICATION                       0x00000002
#define MAP_FEAT_BIT_BROWSING                           0x00000004
#define MAP_FEAT_BIT_UPLOADING                          0x00000008
#define MAP_FEAT_BIT_DELETE                             0x00000010
#define MAP_FEAT_BIT_INSTANCE_INFOMATION                0x00000020
#define MAP_FEAT_BIT_EXT_EVT_REPORT_1_1                 0x00000040
#define MAP_FEAT_BIT_EXT_EVT_REPORT_1_2                 0x00000080
#define MAP_FEAT_BIT_MSG_FORMAT_VER_1_1                 0x00000100
#define MAP_FEAT_BIT_MSG_LISTING_FORMAT_VER_1_1         0x00000200
#define MAP_FEAT_BIT_PERSISTENT_MSG_HANLDES             0x00000400
#define MAP_FEAT_BIT_DATABASE_IDENTIFIER                0x00000800
#define MAP_FEAT_BIT_FOLDER_VER_COUNTER                 0x00001000
#define MAP_FEAT_BIT_CONVERSATION_VER_COUNTERS          0x00002000
#define MAP_FEAT_BIT_PARTI_PRESENCE_CHANGE_NOTIF        0x00004000
#define MAP_FEAT_BIT_PARTI_CHAT_STATE_CHANGE_NOTIF      0x00008000
#define MAP_FEAT_BIT_PBAP_CONTACT_CROSS_REFERENCE       0x00010000
#define MAP_FEAT_BIT_NOTIFICATION_FILTERING             0x00020000
#define MAP_FEAT_BIT_UTC_OFFSET_TIMESTAMP_FORMAT        0x00040000
#define MAP_FEAT_BIT_MAP_SUPPORTED_FEATS_IN_CONN_REQ    0x00080000
#define MAP_FEAT_BIT_CONVERSATION_LISTING               0x00100000
#define MAP_FEAT_BIT_OWNER_STATUS                       0x00200000

#define TAG_ID_MAP_MAX_LIST_COUNT   0x01
#define TAG_ID_START_OFFSET         0x02
#define TAG_ID_FILTER_MSG_TYPE      0x03
#define TAG_ID_FILTER_PERIOD_BEGIN  0x04
#define TAG_ID_FILTER_PERIDO_END    0x05
#define TAG_ID_FILTER_READ_STATUS   0x06
#define TAG_ID_FILTER_RECIPIENT     0x07
#define TAG_ID_FILTER_ORIGINATOR    0x08
#define TAG_ID_FILTER_PRIORITY      0x09
#define TAG_ID_ATTACHMENT           0x0A
#define TAG_ID_NEW_MESSAGE          0x0D
#define TAG_ID_MAP_NOTIF_STATUS     0x0E
#define TAG_ID_MAS_INSTANCE_ID      0x0F
#define TAG_ID_FOLDER_LISTING_SIZE  0x11
#define TAG_ID_MSG_LISTING_SIZE     0x12
#define TAG_ID_SUBJECT_LENGTH       0x13
#define TAG_ID_CHARSET              0x14
#define TAG_ID_MAP_SUPPORTED_FEATS  0x29

#define MAP_CHARSET_NATIVE          0x00
#define MAP_CHARSET_UTF8            0x01

#define MAP_TARGET_LEN              0x0013
#define MAP_VCARD_LISTING_TYPE_LEN  0x0016
#define MAP_VCARD_TYPE_LEN          0x000E
#define MAP_NOTIF_REG_TYPE_LEN      0x0025
#define MAP_ENABLE_NOTIF_STATUS_LEN 0x0006
#define MAP_WHO_LEN                 0x0013
#define MAP_EVT_REPORT_LEN          0x0019
#define MAP_GET_FOLDER_LISTING_LEN  0x0019
#define MAP_MSG_LISTING_LEN         0x0018
#define MAP_MSG_TYPE_LEN            0x0010

#define SEARCH_ATTR_NAME            0x00
#define SEARCH_ATTR_NUMBER          0x01
#define SEARCH_ATTR_SOUND           0x02

#define TAG_SEARCH_ATTR_LEN         0x03
#define TAG_LIST_SATRT_OFFSET_LEN   0x04
#define TAG_MAX_LIST_COUNT_LEN      0x04

#define FILLER_BYTE                 0x30

#define MAP_MAX_QUEUED_CMD 5

typedef enum t_bt_map_state
{
    BT_MAP_STATE_DISCONNECTED  = 0x00,
    BT_MAP_STATE_CONNECTING    = 0x01,
    BT_MAP_STATE_CONNECTED     = 0x02,
    BT_MAP_STATE_DISCONNECTING = 0x03,
} T_BT_MAP_STATE;

typedef enum t_bt_map_action
{
    BT_MAP_ACTION_MAS_IDLE               = 0x00,
    BT_MAP_ACTION_MAS_SET_NOTIF_REG      = 0x01,
    BT_MAP_ACTION_MAS_GET_FOLDER_LISTING = 0x02,
    BT_MAP_ACTION_MAS_GET_MSG_LISTING    = 0x03,
    BT_MAP_ACTION_MAS_GET_MSG            = 0x04,
    BT_MAP_ACTION_MAS_PUSH_MSG           = 0x05,
} T_BT_MAP_ACTION;

typedef enum t_bt_map_opcode
{
    BT_MAP_OP_SET_FOLDER         = 0x00,
    BT_MAP_OP_REG_MSG_NOTIF      = 0x01,
    BT_MAP_OP_GET_FOLDER_LISTING = 0x02,
    BT_MAP_OP_GET_MSG_LISTING    = 0x03,
    BT_MAP_OP_GET_MSG            = 0x04,
    BT_MAP_OP_GET_CONTINUE       = 0x05,
    BT_MAP_OP_GET_ABORT          = 0x06,
} T_BT_MAP_OPCODE;

typedef struct t_bt_map_req_set_folder
{
    bool      back;
    uint8_t  *folder;
    uint16_t  folder_len;
} T_BT_MAP_REQ_SET_FOLDER;

typedef struct t_bt_map_req_get_folder_listing
{
    uint16_t max_list_count;
    uint16_t start_offset;
} T_BT_MAP_REQ_GET_FOLDER_LISTING;

typedef struct t_bt_map_req_get_msg_listing
{
    uint8_t  *folder;
    uint16_t  folder_len;
    uint16_t  max_list_count;
    uint16_t  start_offset;
} T_BT_MAP_REQ_GET_MSG_LISTING;

typedef struct t_bt_map_req_get_msg
{
    uint8_t *msg_handle;
    uint8_t  handle_len;
    bool     native;
} T_BT_MAP_REQ_GET_MSG;

typedef struct t_bt_map_cmd
{
    struct t_bt_map_cmd          *next;
    uint8_t                       opcode;
    union
    {
        bool                             msg_notif_enable;
        T_BT_MAP_REQ_SET_FOLDER          set_folder;
        T_BT_MAP_REQ_GET_FOLDER_LISTING  get_folder_listing;
        T_BT_MAP_REQ_GET_MSG_LISTING     get_msg_listing;
        T_BT_MAP_REQ_GET_MSG             get_msg;
    } param;
} T_BT_MAP_CMD;

typedef struct t_bt_map_link
{
    struct t_bt_map_link *next;
    uint8_t               bd_addr[6];
    uint8_t               srm_status;
    uint8_t               cmd_credits;
    T_OS_QUEUE            cmd_queue;
    T_BT_MAP_STATE        mas_state;
    T_BT_MAP_STATE        mns_state;
    T_BT_MAP_ACTION       action;
    bool                  feat_flag;
    bool                  obex_over_l2c;
    T_OBEX_HANDLE         mas_handle;
    T_OBEX_HANDLE         mns_handle;
} T_BT_MAP_LINK;

typedef struct t_bt_map
{
    T_OS_QUEUE link_list;
} T_BT_MAP;

static T_BT_MAP *bt_map;

/* UNICODE : telecom */
const uint8_t bt_map_path_telecom[PATH_TELECOM_LEN] =
{
    0x00, 0x74, 0x00, 0x65, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x63, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x00
};

/* UNICODE : msg */
const uint8_t bt_map_path_msg[PATH_MSG_LEN] =
{
    0x00, 0x6d, 0x00, 0x73, 0x00, 0x67, 0x00, 0x00
};

/* UNICODE : inbox */
const uint8_t bt_map_path_inbox[PATH_INBOX_LEN] =
{
    0x00, 0x69, 0x00, 0x6e, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x78, 0x00, 0x00
};

/* UNICODE : outbox */
const uint8_t bt_map_path_outbox[PATH_OUTBOX_LEN] =
{
    0x00, 0x6f, 0x00, 0x75, 0x00, 0x74, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x78, 0x00, 0x00
};

/* UNICODE : sent */
const uint8_t bt_map_path_sent[PATH_SENT_LEN] =
{
    0x00, 0x73, 0x00, 0x65, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x00
};

/* UNICODE : deleted */
const uint8_t bt_map_path_deleted[PATH_DELETED_LEN] =
{
    0x00, 0x64, 0x00, 0x65, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x74, 0x00, 0x65, 0x00, 0x64, 0x00, 0x00
};

/* UNICODE : draft */
const uint8_t bt_map_path_draft[PATH_DRAFT_LEN] =
{
    0x00, 0x64, 0x00, 0x72, 0x00, 0x61, 0x00, 0x66, 0x00, 0x74, 0x00, 0x00
};

const uint8_t map_target[MAP_TARGET_LEN + 1] =
{
    MAP_TARGET_LEN,
    OBEX_HI_TARGET,
    (uint8_t)(MAP_TARGET_LEN >> 8),
    (uint8_t)MAP_TARGET_LEN,
    0xbb, 0x58, 0x2b, 0x40, 0x42, 0x0c, 0x11, 0xdb, 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};

uint8_t map_target_feats[MAP_TARGET_LEN + 9 + 1] =
{
    (MAP_TARGET_LEN + 9),
    OBEX_HI_TARGET,
    (uint8_t)(MAP_TARGET_LEN >> 8),
    (uint8_t)(MAP_TARGET_LEN),
    0xbb, 0x58, 0x2b, 0x40, 0x42, 0x0c, 0x11, 0xdb, 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66,
    OBEX_HI_APP_PARAMETER,
    0x00,
    0x09,
    TAG_ID_MAP_SUPPORTED_FEATS,
    0x04,
    0x00, 0x00, 0x00, 0x00
};

const uint8_t map_who[MAP_WHO_LEN] =
{
    OBEX_HI_WHO,
    (uint8_t)(MAP_WHO_LEN >> 8),
    (uint8_t)MAP_WHO_LEN,
    0xbb, 0x58, 0x2b, 0x41, 0x42, 0x0c, 0x11, 0xdb, 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};

/* ASCII : x-bt/MAP-NotificationRegistration  */
const uint8_t map_notif_reg_type[MAP_NOTIF_REG_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_NOTIF_REG_TYPE_LEN >> 8),
    (uint8_t)MAP_NOTIF_REG_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x4d, 0x41, 0x50, 0x2d, 0x4e, 0x6f, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61,
    0x74, 0x69, 0x6f, 0x6e, 0x52, 0x65, 0x67, 0x69, 0x73, 0x74, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x00
};

/* ASCII : x-bt/MAP-event-report */
const uint8_t map_evt_report[MAP_EVT_REPORT_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_EVT_REPORT_LEN >> 8),
    (uint8_t)MAP_EVT_REPORT_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x4d, 0x41, 0x50, 0x2d, 0x65, 0x76,
    0x65, 0x6e, 0x74, 0x2d, 0x72, 0x65, 0x70, 0x6f, 0x72, 0x74, 0x00
};

/* ASCII : x-obex/folder-listing */
const uint8_t map_get_folder_listing_type[MAP_GET_FOLDER_LISTING_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_GET_FOLDER_LISTING_LEN >> 8),
    (uint8_t)MAP_GET_FOLDER_LISTING_LEN,
    0x78, 0x2d, 0x6f, 0x62, 0x65, 0x78, 0x2f, 0x66, 0x6f, 0x6c, 0x64,
    0x65, 0x72, 0x2d, 0x6c, 0x69, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x00
};

/* ASCII : x-bt/MAP-msg_listing */
const uint8_t map_msg_listing_type[MAP_MSG_LISTING_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_MSG_LISTING_LEN >> 8),
    (uint8_t)MAP_MSG_LISTING_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x4d, 0x41, 0x50, 0x2d, 0x6d, 0x73,
    0x67, 0x2d, 0x6c, 0x69, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x00
};

/* ASCII : x-bt/message */
const uint8_t map_msg_type[MAP_MSG_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_MSG_TYPE_LEN >> 8),
    (uint8_t)MAP_MSG_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x00
};

T_BT_MAP_LINK *bt_map_alloc_link(uint8_t bd_addr[6])
{
    T_BT_MAP_LINK *link;

    link = calloc(1, sizeof(T_BT_MAP_LINK));
    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_in(&bt_map->link_list, link);
    }

    return link;
}

void bt_map_free_link(T_BT_MAP_LINK *link)
{
    if (os_queue_delete(&bt_map->link_list, link) == true)
    {
        T_BT_MAP_CMD *cmd;

        cmd = os_queue_out(&link->cmd_queue);
        while (cmd)
        {
            if (cmd->opcode == BT_MAP_OP_SET_FOLDER)
            {
                if (cmd->param.set_folder.folder != NULL)
                {
                    free(cmd->param.set_folder.folder);
                }
            }
            else if (cmd->opcode == BT_MAP_OP_GET_MSG_LISTING)
            {
                if (cmd->param.get_msg_listing.folder != NULL)
                {
                    free(cmd->param.get_msg_listing.folder);
                }
            }
            else if (cmd->opcode == BT_MAP_OP_GET_MSG)
            {
                if (cmd->param.get_msg.msg_handle != NULL)
                {
                    free(cmd->param.get_msg.msg_handle);
                }
            }

            free(cmd);
            cmd = os_queue_out(&link->cmd_queue);
        }

        free(link);
    }
}

T_BT_MAP_LINK *bt_map_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_MAP_LINK *link;

    link = os_queue_peek(&bt_map->link_list, 0);
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

bool bt_map_send_cmd_register_msg_notification(T_BT_MAP_LINK *link,
                                               bool           enable)
{
    uint8_t *buf;

    buf = malloc(MAP_NOTIF_REG_TYPE_LEN + MAP_ENABLE_NOTIF_STATUS_LEN);
    if (buf != NULL)
    {
        uint8_t *p;
        uint8_t  body;
        bool     ret;

        p = buf;
        ARRAY_TO_STREAM(p, map_notif_reg_type, MAP_NOTIF_REG_TYPE_LEN);
        BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
        BE_UINT16_TO_STREAM(p, MAP_ENABLE_NOTIF_STATUS_LEN);
        BE_UINT8_TO_STREAM(p, TAG_ID_MAP_NOTIF_STATUS);
        BE_UINT8_TO_STREAM(p, 0x01);
        if (enable)
        {
            BE_UINT8_TO_STREAM(p, 0x01);
        }
        else
        {
            BE_UINT8_TO_STREAM(p, 0x00);
        }

        link->action = BT_MAP_ACTION_MAS_SET_NOTIF_REG;

        body = FILLER_BYTE;
        ret = obex_put_data(link->mas_handle, buf, p - buf, false, &body, 1, false);
        free(buf);

        return ret;
    }

    return false;
}

bool bt_map_send_cmd_get_folder_listing(T_BT_MAP_LINK *link,
                                        uint16_t       max_list_count,
                                        uint16_t       start_offset)
{
    uint8_t  *buf;
    uint16_t  pkt_len;

    pkt_len = MAP_GET_FOLDER_LISTING_LEN + 0x000B;

    if (link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    buf = malloc(pkt_len);
    if (buf != NULL)
    {
        uint8_t *p;
        bool     ret;

        p = buf;

        if (link->obex_over_l2c)
        {
            BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
            BE_UINT8_TO_STREAM(p, 0x01);
        }

        ARRAY_TO_STREAM(p, map_get_folder_listing_type, MAP_GET_FOLDER_LISTING_LEN);
        BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
        BE_UINT16_TO_STREAM(p, 0x000B);
        BE_UINT8_TO_STREAM(p, TAG_ID_MAP_MAX_LIST_COUNT);
        BE_UINT8_TO_STREAM(p, 0x02);
        BE_UINT16_TO_STREAM(p, max_list_count);
        BE_UINT8_TO_STREAM(p, TAG_ID_START_OFFSET);
        BE_UINT8_TO_STREAM(p, 0x02);
        BE_UINT16_TO_STREAM(p, start_offset);

        link->action = BT_MAP_ACTION_MAS_GET_FOLDER_LISTING;

        ret = obex_get_object(link->mas_handle, buf, p - buf);
        free(buf);

        return ret;
    }

    return false;
}

bool bt_map_send_cmd_get_msg_listing(T_BT_MAP_LINK *link,
                                     uint8_t       *folder,
                                     uint16_t       folder_len,
                                     uint16_t       max_list_count,
                                     uint16_t       start_offset)
{
    uint8_t  *buf;
    uint16_t  pkt_len;

    pkt_len = OBEX_HDR_NAME_LEN + folder_len + MAP_MSG_LISTING_LEN + 0x000B;

    if (link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    buf = malloc(pkt_len);
    if (buf != NULL)
    {
        uint8_t *p;
        bool     ret;

        p = buf;

        if (link->obex_over_l2c)
        {
            BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
            BE_UINT8_TO_STREAM(p, 0x01);
        }

        BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
        BE_UINT16_TO_STREAM(p, folder_len + OBEX_HDR_NAME_LEN);
        ARRAY_TO_STREAM(p, folder, folder_len);

        ARRAY_TO_STREAM(p, map_msg_listing_type, MAP_MSG_LISTING_LEN);

        BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
        BE_UINT16_TO_STREAM(p, 0x000B);

        BE_UINT8_TO_STREAM(p, TAG_ID_MAP_MAX_LIST_COUNT);
        BE_UINT8_TO_STREAM(p, 0x02);
        BE_UINT16_TO_STREAM(p, max_list_count);
        BE_UINT8_TO_STREAM(p, TAG_ID_START_OFFSET);
        BE_UINT8_TO_STREAM(p, 0x02);
        BE_UINT16_TO_STREAM(p, start_offset);

        link->action = BT_MAP_ACTION_MAS_GET_MSG_LISTING;

        ret = obex_get_object(link->mas_handle, buf, p - buf);
        free(buf);

        return ret;
    }

    return false;
}

bool bt_map_send_cmd_get_msg(T_BT_MAP_LINK *link,
                             uint8_t       *msg_handle,
                             uint8_t        handle_len,
                             bool           native)
{
    uint8_t  *buf;
    uint16_t  pkt_len;

    pkt_len = OBEX_HDR_NAME_LEN + handle_len + MAP_MSG_TYPE_LEN + 0x0009;

    if (link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    buf = malloc(pkt_len);
    if (buf != NULL)
    {
        uint8_t *p;
        bool     ret;

        p = buf;

        if (link->obex_over_l2c)
        {
            BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
            BE_UINT8_TO_STREAM(p, 0x01);
        }

        BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
        BE_UINT16_TO_STREAM(p, handle_len + OBEX_HDR_NAME_LEN);
        ARRAY_TO_STREAM(p, msg_handle, handle_len);

        ARRAY_TO_STREAM(p, map_msg_type, MAP_MSG_TYPE_LEN);

        BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
        BE_UINT16_TO_STREAM(p, 0x0009);

        BE_UINT8_TO_STREAM(p, TAG_ID_ATTACHMENT);
        BE_UINT8_TO_STREAM(p, 0x01);
        BE_UINT8_TO_STREAM(p, 0x00);
        BE_UINT8_TO_STREAM(p, TAG_ID_CHARSET);
        BE_UINT8_TO_STREAM(p, 0x01);
        if (native)
        {
            BE_UINT8_TO_STREAM(p, MAP_CHARSET_NATIVE);
        }
        else
        {
            BE_UINT8_TO_STREAM(p, MAP_CHARSET_UTF8);
        }

        link->action = BT_MAP_ACTION_MAS_GET_MSG;

        ret = obex_get_object(link->mas_handle, buf, p - buf);
        free(buf);

        return ret;
    }

    return false;
}

void bt_map_cmd_process(T_BT_MAP_LINK *link)
{
    T_BT_MAP_CMD *cmd;
    int32_t       ret = 0;

    if (link->cmd_credits == 0)
    {
        ret = 1;
        goto fail_no_credits;
    }

    cmd = os_queue_out(&link->cmd_queue);
    if (cmd != NULL)
    {
        bool status = false;

        PROFILE_PRINT_INFO1("bt_map_cmd_process: opcode 0x%02x", cmd->opcode);

        switch (cmd->opcode)
        {
        case BT_MAP_OP_SET_FOLDER:
            {
                uint8_t flag;

                if (cmd->param.set_folder.back)
                {
                    flag = 0x03;
                }
                else
                {
                    flag = 0x02;
                }

                status = obex_set_path(link->mas_handle,
                                       flag,
                                       cmd->param.set_folder.folder,
                                       cmd->param.set_folder.folder_len);
            }
            break;

        case BT_MAP_OP_REG_MSG_NOTIF:
            {
                status = bt_map_send_cmd_register_msg_notification(link, cmd->param.msg_notif_enable);
            }
            break;

        case BT_MAP_OP_GET_FOLDER_LISTING:
            {
                status = bt_map_send_cmd_get_folder_listing(link,
                                                            cmd->param.get_folder_listing.max_list_count,
                                                            cmd->param.get_folder_listing.start_offset);
            }
            break;

        case BT_MAP_OP_GET_MSG_LISTING:
            {
                status = bt_map_send_cmd_get_msg_listing(link,
                                                         cmd->param.get_msg_listing.folder,
                                                         cmd->param.get_msg_listing.folder_len,
                                                         cmd->param.get_msg_listing.max_list_count,
                                                         cmd->param.get_msg_listing.start_offset);
            }
            break;

        case BT_MAP_OP_GET_MSG:
            {
                status = bt_map_send_cmd_get_msg(link,
                                                 cmd->param.get_msg.msg_handle,
                                                 cmd->param.get_msg.handle_len,
                                                 cmd->param.get_msg.native);
            }
            break;

        case BT_MAP_OP_GET_CONTINUE:
            {
                status = obex_get_object_continue(link->mas_handle);
            }
            break;

        case BT_MAP_OP_GET_ABORT:
            {
                status = obex_abort(link->mas_handle);
                link->action = BT_MAP_ACTION_MAS_IDLE;
            }
            break;

        default:
            break;
        }

        free(cmd);

        if (status == false)
        {
            ret = 2;
            goto fail_send_cmd;
        }

        link->cmd_credits = 0;
    }

    return;

fail_send_cmd:
fail_no_credits:
    PROFILE_PRINT_ERROR1("bt_map_cmd_process: failed %d", -ret);
}

void bt_map_handle_folder_listing_data_ind(T_BT_MAP_LINK                  *link,
                                           T_OBEX_GET_OBJECT_CMPL_DATA    *p_data)
{
    T_BT_MAP_FOLDER_LISTING_DATA_IND  data_ind = {0};
    uint8_t                          *hdr_data;
    uint16_t                          hdr_data_len;
    bool                              ret;
    T_BT_MSG_PAYLOAD                  payload;

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_APP_PARAMETER, &hdr_data,
                               &hdr_data_len);

    if (ret && *hdr_data == TAG_ID_FOLDER_LISTING_SIZE)
    {
        BE_ARRAY_TO_UINT16(data_ind.folder_listing_size, hdr_data + 2);
    }

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &hdr_data,
                               &hdr_data_len);
    if (ret == false)
    {
        ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &hdr_data,
                                   &hdr_data_len);
        if (ret)
        {
            link->action = BT_MAP_ACTION_MAS_IDLE;
        }
    }

    if (ret)
    {
        data_ind.buf = hdr_data;
        data_ind.len = hdr_data_len;
    }

    if (p_data->rsp_code != OBEX_RSP_CONTINUE)
    {
        data_ind.data_end = true;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &data_ind;
    bt_mgr_dispatch(BT_MSG_MAP_GET_FOLDER_LISTING_CMPL, &payload);
}

void bt_map_handle_msg_listing_data_ind(T_BT_MAP_LINK                  *link,
                                        T_OBEX_GET_OBJECT_CMPL_DATA    *p_data)
{
    T_BT_MAP_MSG_LISTING_DATA_IND  data_ind = {0};
    uint8_t                       *hdr_data;
    uint16_t                       hdr_data_len;
    uint8_t                       *tag_data;
    uint16_t                       tag_data_len;
    bool                           ret;
    T_BT_MSG_PAYLOAD               payload;

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_APP_PARAMETER, &hdr_data,
                               &hdr_data_len);
    if (ret)
    {
        ret = obex_find_value_in_app_param(hdr_data, hdr_data_len, TAG_ID_NEW_MESSAGE, &tag_data,
                                           &tag_data_len);
        if (ret)
        {
            data_ind.new_msg = *tag_data;
        }

        ret = obex_find_value_in_app_param(hdr_data, hdr_data_len, TAG_ID_MSG_LISTING_SIZE, &tag_data,
                                           &tag_data_len);
        if (ret)
        {
            BE_STREAM_TO_UINT16(data_ind.msg_listing_size, tag_data);
        }
    }

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &hdr_data,
                               &hdr_data_len);
    if (ret == false)
    {
        ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &hdr_data,
                                   &hdr_data_len);
        if (ret)
        {
            link->action = BT_MAP_ACTION_MAS_IDLE;
        }
    }

    if (p_data->rsp_code != OBEX_RSP_CONTINUE)
    {
        data_ind.data_end = true;
    }

    if (ret)
    {
        data_ind.buf = hdr_data;
        data_ind.len = hdr_data_len;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &data_ind;
    bt_mgr_dispatch(BT_MSG_MAP_GET_MSG_LISTING_CMPL, &payload);
}

void bt_map_handle_msg_data_ind(T_BT_MAP_LINK                  *link,
                                T_OBEX_GET_OBJECT_CMPL_DATA    *p_data)
{
    T_BT_MAP_MSG_DATA_IND  data_ind = {0};
    uint8_t               *hdr_data;
    uint16_t               hdr_data_len;
    bool                   ret;
    T_BT_MSG_PAYLOAD       payload;

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &hdr_data,
                               &hdr_data_len);
    if (ret == false)
    {
        ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &hdr_data,
                                   &hdr_data_len);
        if (ret)
        {
            link->action = BT_MAP_ACTION_MAS_IDLE;
        }
    }

    if (ret)
    {
        data_ind.buf = hdr_data;
        data_ind.len = hdr_data_len;
    }

    if (p_data->rsp_code != OBEX_RSP_CONTINUE)
    {
        data_ind.data_end = true;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &data_ind;
    bt_mgr_dispatch(BT_MSG_MAP_GET_MSG_CMPL, &payload);
}

bool bt_map_cmd_enqueue(T_BT_MAP_LINK *link,
                        uint8_t        opcode,
                        void          *param)
{
    T_BT_MAP_CMD *cmd;
    int32_t       ret = 0;

    PROFILE_PRINT_INFO1("bt_map_cmd_enqueue: opcode 0x%02x", opcode);

    if (link->cmd_queue.count == MAP_MAX_QUEUED_CMD)
    {
        ret = 1;
        goto fail_cmd_enqueue;
    }

    cmd = calloc(1, sizeof(T_BT_MAP_CMD));
    if (cmd == NULL)
    {
        ret = 2;
        goto fail_alloc_cmd;
    }

    cmd->opcode = opcode;
    switch (opcode)
    {
    case BT_MAP_OP_SET_FOLDER:
        {
            T_BT_MAP_REQ_SET_FOLDER *payload = (T_BT_MAP_REQ_SET_FOLDER *)param;

            cmd->param.set_folder.folder = malloc(payload->folder_len);
            if (cmd->param.set_folder.folder == NULL)
            {
                ret = 3;
                goto fail_build_cmd;
            }

            cmd->param.set_folder.back = payload->back;
            memcpy(cmd->param.set_folder.folder, payload->folder, payload->folder_len);
            cmd->param.set_folder.folder_len = payload->folder_len;
        }
        break;

    case BT_MAP_OP_REG_MSG_NOTIF:
        {
            cmd->param.msg_notif_enable = *(bool *)param;
        }
        break;

    case BT_MAP_OP_GET_FOLDER_LISTING:
        {
            T_BT_MAP_REQ_GET_FOLDER_LISTING *payload = (T_BT_MAP_REQ_GET_FOLDER_LISTING *)param;

            cmd->param.get_folder_listing.max_list_count = payload->max_list_count;
            cmd->param.get_folder_listing.start_offset = payload->start_offset;
        }
        break;

    case BT_MAP_OP_GET_MSG_LISTING:
        {
            T_BT_MAP_REQ_GET_MSG_LISTING *payload = (T_BT_MAP_REQ_GET_MSG_LISTING *)param;

            cmd->param.get_msg_listing.folder = malloc(payload->folder_len);
            if (cmd->param.get_msg_listing.folder == NULL)
            {
                ret = 4;
                goto fail_build_cmd;
            }

            memcpy(cmd->param.get_msg_listing.folder, payload->folder, payload->folder_len);
            cmd->param.get_msg_listing.folder_len = payload->folder_len;
            cmd->param.get_msg_listing.max_list_count = payload->max_list_count;
            cmd->param.get_msg_listing.start_offset = payload->start_offset;
        }
        break;

    case BT_MAP_OP_GET_MSG:
        {
            T_BT_MAP_REQ_GET_MSG *payload = (T_BT_MAP_REQ_GET_MSG *)param;

            cmd->param.get_msg.msg_handle = malloc(payload->handle_len);
            if (cmd->param.get_msg.msg_handle == NULL)
            {
                ret = 5;
                goto fail_build_cmd;
            }

            memcpy(cmd->param.get_msg.msg_handle, payload->msg_handle, payload->handle_len);
            cmd->param.get_msg.handle_len = payload->handle_len;
            cmd->param.get_msg.native = payload->native;
        }
        break;

    case BT_MAP_OP_GET_CONTINUE:
        break;

    case BT_MAP_OP_GET_ABORT:
        break;

    default:
        ret = 6;
        goto fail_build_cmd;
    }

    os_queue_in(&link->cmd_queue, cmd);
    bt_map_cmd_process(link);

    return true;

fail_build_cmd:
    free(cmd);
fail_alloc_cmd:
fail_cmd_enqueue:
    PROFILE_PRINT_ERROR1("bt_map_cmd_enqueue: failed %d", -ret);
    return false;
}

bool bt_map_set_folder(uint8_t         bd_addr[6],
                       T_BT_MAP_FOLDER folder)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_MAP_REQ_SET_FOLDER  cmd;
        const uint8_t           *path_ptr;
        uint16_t                 path_len;
        bool                     back = false;

        switch (folder)
        {
        case BT_MAP_FOLDER_ROOT:
            path_len = 0;
            path_ptr = NULL;
            break;

        case BT_MAP_FOLDER_TELECOM:
            path_len = PATH_TELECOM_LEN;
            path_ptr = bt_map_path_telecom;
            break;

        case BT_MAP_FOLDER_MSG:
            path_len = PATH_MSG_LEN;
            path_ptr = bt_map_path_msg;
            break;

        case BT_MAP_FOLDER_INBOX:
            path_len = PATH_INBOX_LEN;
            path_ptr = bt_map_path_inbox;
            break;

        case BT_MAP_FOLDER_OUTBOX:
            path_len = PATH_OUTBOX_LEN;
            path_ptr = bt_map_path_outbox;
            break;

        case BT_MAP_FOLDER_SENT:
            path_len = PATH_SENT_LEN;
            path_ptr = bt_map_path_sent;
            break;

        case BT_MAP_FOLDER_DELETED:
            path_len = PATH_DELETED_LEN;
            path_ptr = bt_map_path_deleted;
            break;

        case BT_MAP_FOLDER_DRAFT:
            path_len = PATH_DRAFT_LEN;
            path_ptr = bt_map_path_draft;
            break;

        default:
            path_ptr = NULL;
            path_len = 0;
            break;
        }

        cmd.back = back;
        cmd.folder = (uint8_t *)path_ptr;
        cmd.folder_len = path_len;

        return bt_map_cmd_enqueue(link, BT_MAP_OP_SET_FOLDER, &cmd);
    }

    return false;
}

void bt_map_cback(uint8_t  msg_type,
                  void    *msg)
{
    T_BT_MAP_LINK    *link;
    T_BT_MSG_PAYLOAD  payload;

    PROFILE_PRINT_TRACE1("bt_map_cback: msg_type 0x%02x", msg_type);

    switch (msg_type)
    {
    case OBEX_MSG_CONN_IND:
        {
            T_OBEX_CONN_IND_DATA *p_data = (T_OBEX_CONN_IND_DATA *)msg;

            link = bt_map_find_link_by_addr(p_data->bd_addr);
            if (link != NULL && link->mns_handle == NULL)
            {
                link->mns_handle = p_data->handle;
                memcpy(payload.bd_addr, link->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_MAP_MNS_CONN_IND, &payload);
            }
            else
            {
                obex_conn_cfm(p_data->handle, false, NULL, 0);
            }
        }
        break;

    case OBEX_MSG_CONN_CMPL:
        {
            T_OBEX_CONN_CMPL_DATA *p_data = (T_OBEX_CONN_CMPL_DATA *)msg;

            link = bt_map_find_link_by_addr(p_data->bd_addr);
            if (link)
            {
                if (link->mas_handle == p_data->handle)
                {
                    link->mas_state = BT_MAP_STATE_CONNECTED;
                    os_queue_init(&link->cmd_queue);
                    link->cmd_credits = 1;

                    memcpy(payload.bd_addr, link->bd_addr, 6);
                    payload.msg_buf = &p_data->max_pkt_len;
                    bt_mgr_dispatch(BT_MSG_MAP_MAS_CONN_CMPL, &payload);
                    bt_map_set_folder(link->bd_addr, BT_MAP_FOLDER_ROOT);
                    bt_map_set_folder(link->bd_addr, BT_MAP_FOLDER_TELECOM);
                    bt_map_set_folder(link->bd_addr, BT_MAP_FOLDER_MSG);
                }
                else
                {
                    link->mns_state = BT_MAP_STATE_CONNECTED;
                    memcpy(payload.bd_addr, link->bd_addr, 6);
                    bt_mgr_dispatch(BT_MSG_MAP_MNS_CONN_CMPL, &payload);
                }
            }
            else
            {
                obex_disconn_req(p_data->handle);
            }
        }
        break;

    case OBEX_MSG_DISCONN:
        {
            T_OBEX_DISCONN_CMPL_DATA *p_data = (T_OBEX_DISCONN_CMPL_DATA *)msg;

            link = bt_map_find_link_by_addr(p_data->bd_addr);
            if (link)
            {
                if (p_data->handle == link->mns_handle)
                {
                    link->mns_handle = NULL;
                    memcpy(payload.bd_addr, link->bd_addr, 6);
                    payload.msg_buf = &p_data->cause;

                    if (link->mns_state == BT_MAP_STATE_CONNECTING)
                    {
                        link->mns_state = BT_MAP_STATE_DISCONNECTED;
                        bt_mgr_dispatch(BT_MSG_MAP_MNS_CONN_FAIL, &payload);
                    }
                    else
                    {
                        link->mns_state = BT_MAP_STATE_DISCONNECTED;
                        bt_mgr_dispatch(BT_MSG_MAP_MNS_DISCONN_CMPL, &payload);
                    }
                }
                else if (p_data->handle == link->mas_handle)
                {
                    T_BT_MAP_STATE state;

                    state = link->mas_state;
                    memcpy(payload.bd_addr, link->bd_addr, 6);
                    payload.msg_buf = &p_data->cause;
                    bt_map_free_link(link);

                    if (state == BT_MAP_STATE_CONNECTING)
                    {
                        bt_mgr_dispatch(BT_MSG_MAP_MAS_CONN_FAIL, &payload);
                    }
                    else
                    {
                        bt_mgr_dispatch(BT_MSG_MAP_MAS_DISCONN_CMPL, &payload);
                    }
                }
            }
        }
        break;

    case OBEX_MSG_SET_PATH_DONE:
        {
            T_OBEX_SET_PATH_CMPL_DATA *p_data = (T_OBEX_SET_PATH_CMPL_DATA *)msg;

            link = bt_map_find_link_by_addr(p_data->bd_addr);
            if (link && link->mas_handle == p_data->handle)
            {
                bool result = false;

                if (p_data->cause == OBEX_RSP_SUCCESS)
                {
                    result = true;
                }
                else
                {
                    result = false;
                }

                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = &result;
                bt_mgr_dispatch(BT_MSG_MAP_SET_FOLDER_CMPL, &payload);

                link->cmd_credits = 1;
                bt_map_cmd_process(link);
            }
        }
        break;

    case OBEX_MSG_PUT_DONE:
        {
            T_OBEX_PUT_CMPL_DATA *p_data = (T_OBEX_PUT_CMPL_DATA *)msg;

            link = bt_map_find_link_by_addr(p_data->bd_addr);
            if (link && link->mas_handle == p_data->handle)
            {
                if (link->action == BT_MAP_ACTION_MAS_SET_NOTIF_REG)
                {
                    bool result = false;

                    if (p_data->cause == OBEX_RSP_SUCCESS)
                    {
                        result = true;
                    }

                    link->action = BT_MAP_ACTION_MAS_IDLE;
                    memcpy(payload.bd_addr, link->bd_addr, 6);
                    payload.msg_buf = &result;
                    bt_mgr_dispatch(BT_MSG_MAP_REG_NOTIF_CMPL, &payload);
                }
                else if (link->action == BT_MAP_ACTION_MAS_PUSH_MSG)
                {
                    T_BT_MAP_PUSH_MSG_CMPL data;

                    data.rsp_code = p_data->cause;
                    data.msg_handle = p_data->p_name;
                    data.msg_handle_len = p_data->name_len;

                    link->srm_status = p_data->srm_status;
                    if (p_data->cause == OBEX_RSP_CONTINUE)
                    {
                        if (link->srm_status == SRM_ENABLE)
                        {
                            data.action = BT_MAP_PUSH_MSG_TO_END;
                        }
                        else if (link->srm_status == SRM_WAIT)
                        {
                            data.action = BT_MAP_PUSH_MSG_AGAIN;
                        }
                        else
                        {
                            data.action = BT_MAP_PUSH_MSG_CONTINUE;
                        }
                    }
                    else
                    {
                        link->action = BT_MAP_ACTION_MAS_IDLE;
                        data.action = BT_MAP_PUSH_MSG_COMPLETE;
                    }

                    memcpy(payload.bd_addr, link->bd_addr, 6);
                    payload.msg_buf = &data;
                    bt_mgr_dispatch(BT_MSG_MAP_PUSH_MSG_CMPL, &payload);
                }

                link->cmd_credits = 1;
                bt_map_cmd_process(link);
            }
        }
        break;

    case OBEX_MSG_GET_OBJECT:
        {
            T_OBEX_GET_OBJECT_CMPL_DATA *p_data = (T_OBEX_GET_OBJECT_CMPL_DATA *)msg;

            link = bt_map_find_link_by_addr(p_data->bd_addr);
            if (link)
            {
                link->srm_status = p_data->srm_status;
                link->cmd_credits = 1;
                if (link->mas_handle == p_data->handle)
                {
                    if (p_data->rsp_code == OBEX_RSP_SUCCESS ||
                        p_data->rsp_code == OBEX_RSP_CONTINUE)
                    {
                        if (link->action == BT_MAP_ACTION_MAS_GET_FOLDER_LISTING)
                        {
                            bt_map_handle_folder_listing_data_ind(link, p_data);
                        }
                        else if (link->action == BT_MAP_ACTION_MAS_GET_MSG_LISTING)
                        {
                            bt_map_handle_msg_listing_data_ind(link, p_data);
                        }
                        else if (link->action == BT_MAP_ACTION_MAS_GET_MSG)
                        {
                            bt_map_handle_msg_data_ind(link, p_data);
                        }
                    }
                    else
                    {
                        link->action = BT_MAP_ACTION_MAS_IDLE;
                    }
                }

                bt_map_cmd_process(link);
            }
        }
        break;

    case OBEX_MSG_REMOTE_PUT:
        {
            T_OBEX_REMOTE_PUT_DATA *p_data = (T_OBEX_REMOTE_PUT_DATA *)msg;

            link = bt_map_find_link_by_addr(p_data->bd_addr);
            if (link != NULL)
            {
                T_BT_MAP_MSG_REPORT_NOTIF  notify;
                uint8_t                   *hdr_data;
                uint16_t                   hdr_data_len;
                bool                       ret;

                if (link->mns_handle == p_data->handle)
                {
                    ret = obex_find_hdr_in_pkt(p_data->p_data,
                                               p_data->data_len,
                                               OBEX_HI_BODY,
                                               &hdr_data,
                                               &hdr_data_len);
                    if (ret == false)
                    {
                        ret = obex_find_hdr_in_pkt(p_data->p_data,
                                                   p_data->data_len,
                                                   OBEX_HI_END_BODY,
                                                   &hdr_data,
                                                   &hdr_data_len);
                        if (ret)
                        {
                            notify.data_end = true;
                        }
                    }
                    else
                    {
                        notify.data_end = false;
                    }

                    if (ret == true)
                    {
                        notify.buf = hdr_data;
                        notify.len = hdr_data_len;

                        memcpy(payload.bd_addr, link->bd_addr, 6);
                        payload.msg_buf = &notify;
                        bt_mgr_dispatch(BT_MSG_MAP_MSG_NOTIFICATION, &payload);
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

bool bt_map_mns_connect_cfm(uint8_t bd_addr[6],
                            bool    accept)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);

        if (accept)
        {
            link->mns_state = BT_MAP_STATE_CONNECTING;
            return obex_conn_cfm(link->mns_handle, true, (uint8_t *)map_who, MAP_WHO_LEN);
        }
        else
        {
            return obex_conn_cfm(link->mns_handle, false, NULL, 0);
        }
    }

    return false;
}

bool bt_map_mas_connect_over_rfc_req(uint8_t bd_addr[6],
                                     uint8_t server_chann)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_map_alloc_link(bd_addr);
        if (link != NULL)
        {
            link->feat_flag = true;

            if (obex_conn_req_over_rfc(bd_addr,
                                       map_target_feats,
                                       server_chann,
                                       bt_map_cback,
                                       &link->mas_handle))
            {
                link->mas_state = BT_MAP_STATE_CONNECTING;
                link->obex_over_l2c = false;
                return true;
            }
            else
            {
                bt_map_free_link(link);
                return false;
            }
        }
    }

    return false;
}

bool bt_map_mas_connect_over_l2c_req(uint8_t  bd_addr[6],
                                     uint16_t l2c_psm)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_map_alloc_link(bd_addr);
        if (link != NULL)
        {
            link->feat_flag = true;

            if (obex_conn_req_over_l2c(bd_addr,
                                       map_target_feats,
                                       l2c_psm,
                                       bt_map_cback,
                                       &link->mas_handle))
            {
                link->mas_state = BT_MAP_STATE_CONNECTING;
                link->obex_over_l2c = true;
                return true;
            }
            else
            {
                bt_map_free_link(link);
                return false;
            }
        }
    }

    return false;
}

bool bt_map_mas_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        link->mas_state = BT_MAP_STATE_DISCONNECTING;
        return obex_disconn_req(link->mas_handle);
    }

    return false;
}

bool bt_map_mns_send_event_rsp(uint8_t                bd_addr[6],
                               T_BT_MAP_RESPONSE_CODE rsp_code)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return obex_send_rsp(link->mns_handle, rsp_code);
    }

    return false;
}

bool bt_map_mas_msg_notification_set(uint8_t bd_addr[6],
                                     bool    enable)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bt_map_cmd_enqueue(link, BT_MAP_OP_REG_MSG_NOTIF, &enable);
    }

    return false;
}

bool bt_map_mas_folder_set(uint8_t         bd_addr[6],
                           T_BT_MAP_FOLDER folder)
{
    bool ret = true;

    switch (folder)
    {
    case BT_MAP_FOLDER_ROOT:
        ret = bt_map_set_folder(bd_addr, BT_MAP_FOLDER_ROOT);
        break;

    case BT_MAP_FOLDER_TELECOM:
        ret = bt_map_set_folder(bd_addr, BT_MAP_FOLDER_ROOT);
        ret = (ret && bt_map_set_folder(bd_addr, BT_MAP_FOLDER_TELECOM));
        break;

    case BT_MAP_FOLDER_MSG:
        ret = bt_map_set_folder(bd_addr, BT_MAP_FOLDER_ROOT);
        ret = (ret && bt_map_set_folder(bd_addr, BT_MAP_FOLDER_TELECOM));
        ret = (ret && bt_map_set_folder(bd_addr, BT_MAP_FOLDER_MSG));
        break;

    case BT_MAP_FOLDER_INBOX:
    case BT_MAP_FOLDER_OUTBOX:
    case BT_MAP_FOLDER_SENT:
    case BT_MAP_FOLDER_DELETED:
    case BT_MAP_FOLDER_DRAFT:
        ret = bt_map_set_folder(bd_addr, BT_MAP_FOLDER_ROOT);
        ret = (ret && bt_map_set_folder(bd_addr, BT_MAP_FOLDER_TELECOM));
        ret = (ret && bt_map_set_folder(bd_addr, BT_MAP_FOLDER_MSG));
        ret = (ret && bt_map_set_folder(bd_addr, folder));
        break;

    default:
        ret = false;
        break;
    }

    return ret;
}

bool bt_map_mas_folder_listing_get(uint8_t  bd_addr[6],
                                   uint16_t max_list_count,
                                   uint16_t start_offset)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_MAP_REQ_GET_FOLDER_LISTING cmd;

        bt_sniff_mode_exit(bd_addr, false);

        cmd.max_list_count = max_list_count;
        cmd.start_offset = start_offset;
        return bt_map_cmd_enqueue(link, BT_MAP_OP_GET_FOLDER_LISTING, &cmd);
    }

    return false;
}

bool bt_map_mas_msg_listing_get(uint8_t   bd_addr[6],
                                uint8_t  *folder,
                                uint16_t  folder_len,
                                uint16_t  max_list_count,
                                uint16_t  start_offset)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_MAP_REQ_GET_MSG_LISTING cmd;

        bt_sniff_mode_exit(bd_addr, false);

        cmd.folder = folder;
        cmd.folder_len = folder_len;
        cmd.max_list_count = max_list_count;
        cmd.start_offset = start_offset;

        return bt_map_cmd_enqueue(link, BT_MAP_OP_GET_MSG_LISTING, &cmd);
    }

    return false;
}

bool bt_map_mas_msg_get(uint8_t  bd_addr[6],
                        uint8_t *msg_handle,
                        uint8_t  handle_len,
                        bool     native)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_MAP_REQ_GET_MSG cmd;

        bt_sniff_mode_exit(bd_addr, false);

        cmd.msg_handle = msg_handle;
        cmd.handle_len = handle_len;
        cmd.native = native;

        return bt_map_cmd_enqueue(link, BT_MAP_OP_GET_MSG, &cmd);
    }

    return false;
}

bool bt_map_mas_get_continue(uint8_t bd_addr[6])
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->srm_status == SRM_ENABLE)
        {
            return true;
        }
        else
        {
            bt_sniff_mode_exit(bd_addr, false);
            return bt_map_cmd_enqueue(link, BT_MAP_OP_GET_CONTINUE, NULL);
        }
    }

    return false;
}

bool bt_map_mas_get_abort(uint8_t bd_addr[6])
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        return bt_map_cmd_enqueue(link, BT_MAP_OP_GET_ABORT, NULL);
    }

    return false;
}

bool bt_map_mas_msg_push(uint8_t   bd_addr[6],
                         uint8_t  *folder,
                         uint16_t  folder_len,
                         bool      native,
                         bool      more_data,
                         uint8_t  *msg,
                         uint16_t  msg_len)
{
    T_BT_MAP_LINK *link;
    bool           status = false;
    int32_t        ret = 0;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_find_link;
    }

    if (link->cmd_credits == 0 && link->srm_status != SRM_ENABLE)
    {
        ret = 2;
        goto fail_no_credits;
    }

    bt_sniff_mode_exit(bd_addr, false);

    if (link->obex_over_l2c && link->srm_status != SRM_DISABLE)
    {
        status = obex_put_data(link->mas_handle, NULL, 0, more_data, msg, msg_len, false);
    }
    else
    {
        uint8_t  *buf;
        uint8_t  *p;
        uint16_t  pkt_len = OBEX_HDR_NAME_LEN + folder_len + MAP_MSG_TYPE_LEN + 0x0006;

        if (link->obex_over_l2c && link->srm_status == SRM_DISABLE)
        {
            pkt_len += OBEX_HDR_SRM_LEN;
        }

        buf = malloc(pkt_len);
        if (buf == NULL)
        {
            ret = 3;
            goto fail_alloc_mem;
        }

        p = buf;

        if (link->obex_over_l2c && link->srm_status == SRM_DISABLE)
        {
            BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
            BE_UINT8_TO_STREAM(p, 0x01);
        }

        BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
        BE_UINT16_TO_STREAM(p, folder_len + OBEX_HDR_NAME_LEN);
        ARRAY_TO_STREAM(p, folder, folder_len);

        ARRAY_TO_STREAM(p, map_msg_type, MAP_MSG_TYPE_LEN);

        BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
        BE_UINT16_TO_STREAM(p, 0x0006);

        BE_UINT8_TO_STREAM(p, TAG_ID_CHARSET);
        BE_UINT8_TO_STREAM(p, 0x01);
        if (native)
        {
            BE_UINT8_TO_STREAM(p, MAP_CHARSET_NATIVE);
        }
        else
        {
            BE_UINT8_TO_STREAM(p, MAP_CHARSET_UTF8);
        }

        link->action = BT_MAP_ACTION_MAS_PUSH_MSG;

        if (link->obex_over_l2c)
        {
            status = obex_put_data(link->mas_handle, buf, p - buf, more_data, NULL, 0, false);
        }
        else
        {
            status = obex_put_data(link->mas_handle, buf, p - buf, more_data, msg, msg_len, false);
        }
        free(buf);
    }

    if (status == false)
    {
        ret = 4;
        goto fail_send_data;
    }

    return true;

fail_send_data:
fail_alloc_mem:
fail_no_credits:
fail_find_link:
    PROFILE_PRINT_ERROR1("bt_map_mas_msg_push: failed %d", -ret);
    return false;
}

bool bt_map_init(uint8_t  mns_server_chann,
                 uint16_t mns_l2c_psm,
                 uint32_t support_feat)
{
    int32_t ret = 0;

    bt_map = calloc(1, sizeof(T_BT_MAP));
    if (bt_map == NULL)
    {
        ret = 1;
        goto fail_alloc_map;
    }

    if (obex_register_over_rfc(mns_server_chann, bt_map_cback) == false)
    {
        ret = 2;
        goto fail_register_rfc;
    }

    if (mns_l2c_psm)
    {
        if (obex_register_over_l2c(mns_l2c_psm, bt_map_cback) == false)
        {
            ret = 3;
            goto fail_register_l2c;
        }
    }

    if (support_feat)
    {
        BE_UINT32_TO_ARRAY(&map_target_feats[MAP_TARGET_LEN + 6], support_feat);
    }

    return true;

fail_register_l2c:
fail_register_rfc:
    free(bt_map);
    bt_map = NULL;
fail_alloc_map:
    PROFILE_PRINT_ERROR1("bt_map_init: failed %d", -ret);
    return false;
}

bool bt_map_get_roleswap_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_MAP_INFO *p_info)
{
    T_BT_MAP_LINK *link;

    link = bt_map_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_OBEX_ROLESWAP_INFO obex_info;

        if (obex_roleswap_info_get(link->mas_handle, &obex_info) == true)
        {
            p_info->feat_flag = link->feat_flag;
            p_info->mas_state = link->mas_state;
            p_info->mns_state = link->mns_state;

            p_info->mas_obex_conn_id = obex_info.conn_id;
            p_info->mas_l2c_cid = obex_info.cid;
            p_info->mas_local_max_pkt_len = obex_info.local_max_pkt_len;
            p_info->mas_remote_max_pkt_len = obex_info.remote_max_pkt_len;
            p_info->mas_obex_state = (uint8_t)obex_info.state;
            p_info->mas_psm = obex_info.psm;
            if (p_info->mas_psm)
            {
                p_info->data_offset = obex_info.data_offset;
            }
            else
            {
                p_info->mas_rfc_dlci = obex_info.rfc_dlci;
            }

            if (link->mns_handle != NULL)
            {
                if (obex_roleswap_info_get(link->mns_handle, &obex_info) == true)
                {
                    p_info->mns_obex_conn_id = obex_info.conn_id;
                    p_info->mns_l2c_cid = obex_info.cid;
                    p_info->mns_local_max_pkt_len = obex_info.local_max_pkt_len;
                    p_info->mns_remote_max_pkt_len = obex_info.remote_max_pkt_len;
                    p_info->mns_obex_state = (uint8_t)obex_info.state;
                    p_info->mns_psm = obex_info.psm;
                    if (p_info->mns_psm)
                    {
                        p_info->data_offset = obex_info.data_offset;
                    }
                    else
                    {
                        p_info->mns_rfc_dlci = obex_info.rfc_dlci;
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {
                p_info->mns_l2c_cid = 0x00;
            }

            return true;
        }
    }

    return false;
}

bool bt_map_set_roleswap_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_MAP_INFO *p_info)
{
    T_BT_MAP_LINK *link;

    link = bt_map_alloc_link(bd_addr);
    if (link != NULL)
    {
        T_OBEX_ROLESWAP_INFO obex_info;

        link->mns_state = (T_BT_MAP_STATE)p_info->mns_state;
        link->mas_state = (T_BT_MAP_STATE)p_info->mas_state;
        link->feat_flag = p_info->feat_flag;

        obex_info.conn_id = p_info->mas_obex_conn_id;
        obex_info.cid = p_info->mas_l2c_cid;
        obex_info.local_max_pkt_len = p_info->mas_local_max_pkt_len;
        obex_info.remote_max_pkt_len = p_info->mas_remote_max_pkt_len;
        obex_info.role = OBEX_ROLE_CLIENT;
        obex_info.state = (T_OBEX_STATE)p_info->mas_obex_state;
        obex_info.psm = p_info->mas_psm;
        if (obex_info.psm)
        {
            obex_info.data_offset = p_info->data_offset;
        }
        else
        {
            obex_info.rfc_dlci = p_info->mas_rfc_dlci;
        }

        if (obex_roleswap_info_set(bd_addr, bt_map_cback, &obex_info, &link->mas_handle) == false)
        {
            bt_map_free_link(link);
            return false;
        }

        if (p_info->mns_l2c_cid != 0x00)
        {
            obex_info.conn_id = p_info->mns_obex_conn_id;
            obex_info.cid = p_info->mns_l2c_cid;
            obex_info.local_max_pkt_len = p_info->mns_local_max_pkt_len;
            obex_info.remote_max_pkt_len = p_info->mns_remote_max_pkt_len;
            obex_info.role = OBEX_ROLE_SERVER;
            obex_info.state = (T_OBEX_STATE)p_info->mns_obex_state;
            obex_info.psm = p_info->mns_psm;
            if (obex_info.psm)
            {
                obex_info.data_offset = p_info->data_offset;
            }
            else
            {
                obex_info.rfc_dlci = p_info->mns_rfc_dlci;
            }

            if (obex_roleswap_info_set(bd_addr, bt_map_cback, &obex_info, &link->mns_handle) == false)
            {
                bt_map_free_link(link);
                return false;
            }
        }

        return true;
    }

    return false;
}

uint8_t bt_map_service_id_get(void)
{
    return obex_service_id_get();
}

#else
#include <stdint.h>
#include <stdbool.h>
#include "bt_map.h"
#include "bt_map_int.h"

bool bt_map_mns_connect_cfm(uint8_t bd_addr[6],
                            bool    accept)
{
    return false;
}

bool bt_map_mas_connect_over_rfc_req(uint8_t bd_addr[6],
                                     uint8_t server_chann)
{
    return false;
}

bool bt_map_mas_connect_over_l2c_req(uint8_t  bd_addr[6],
                                     uint16_t l2c_psm)
{
    return false;
}

bool bt_map_mas_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_map_mns_send_event_rsp(uint8_t                bd_addr[6],
                               T_BT_MAP_RESPONSE_CODE rsp_code)
{
    return false;
}

bool bt_map_mas_msg_notification_set(uint8_t bd_addr[6],
                                     bool    enable)
{
    return false;
}

bool bt_map_mas_folder_set(uint8_t         bd_addr[6],
                           T_BT_MAP_FOLDER folder)
{
    return false;
}

bool bt_map_mas_folder_listing_get(uint8_t  bd_addr[6],
                                   uint16_t max_list_count,
                                   uint16_t start_offset)
{
    return false;
}

bool bt_map_mas_msg_listing_get(uint8_t   bd_addr[6],
                                uint8_t  *folder,
                                uint16_t  folder_len,
                                uint16_t  max_list_count,
                                uint16_t  start_offset)
{
    return false;
}

bool bt_map_mas_msg_get(uint8_t  bd_addr[6],
                        uint8_t *msg_handle,
                        uint8_t  handle_len,
                        bool     native)
{
    return false;
}

bool bt_map_mas_get_continue(uint8_t bd_addr[6])
{
    return false;
}

bool bt_map_mas_get_abort(uint8_t bd_addr[6])
{
    return false;
}

bool bt_map_mas_msg_push(uint8_t   bd_addr[6],
                         uint8_t  *folder,
                         uint16_t  folder_len,
                         bool      native,
                         bool      more_data,
                         uint8_t  *msg,
                         uint16_t  msg_len)
{
    return false;
}

bool bt_map_init(uint8_t  mns_server_chann,
                 uint16_t mns_l2c_psm,
                 uint32_t support_feat)
{
    return false;
}

bool bt_map_get_roleswap_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_MAP_INFO *p_info)
{
    return false;
}

bool bt_map_set_roleswap_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_MAP_INFO *p_info)
{
    return false;
}

uint8_t bt_map_service_id_get(void)
{
    return 0;
}

#endif
