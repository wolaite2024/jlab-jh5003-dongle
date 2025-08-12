/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_MAP_SUPPORT == 1)

#include <string.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "obex.h"
#include "map.h"

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

#define MAP_TARGET_LEN              0x0013    //include TLV
#define MAP_VCARD_LISTING_TYPE_LEN  0x0016    //include TLV
#define MAP_VCARD_TYPE_LEN          0x000E    //include TLV
#define MAP_NOTIF_REG_TYPE_LEN      0x0025    //include TLV
#define MAP_ENABLE_NOTIF_STATUS_LEN 0x0006    //include TLV
#define MAP_WHO_LEN                 0x0013    //include TLV
#define MAP_EVT_REPORT_LEN          0x0019    //include TLV
#define MAP_GET_FOLDER_LISTING_LEN  0x0019    //include TLV
#define MAP_MSG_LISTING_LEN         0x0018    //include TLV
#define MAP_MSG_TYPE_LEN            0x0010    //include TLV

/********seach attribute*********/
#define SEARCH_ATTR_NAME            0x00
#define SEARCH_ATTR_NUMBER          0x01
#define SEARCH_ATTR_SOUND           0x02

#define TAG_SEARCH_ATTR_LEN         0x03
#define TAG_LIST_SATRT_OFFSET_LEN   0x04
#define TAG_MAX_LIST_COUNT_LEN      0x04

#define MAP_MAX_QUEUED_CMD 5

typedef enum
{
    MAP_STATE_DISCONNECTED           = 0x00,
    MAP_STATE_MAS_CONNECTING         = 0x01,
    MAP_STATE_MAS_CONNECTED          = 0x02,
    MAP_STATE_MNS_CONNECTING         = 0x03,
    MAP_STATE_MNS_CONNECTED          = 0x04,
} T_MAP_STATE;

typedef enum
{
    MAP_ACTION_MAS_IDLE               = 0x00,
    MAP_ACTION_MAS_SET_NOTIF_REG      = 0x01,
    MAP_ACTION_MAS_GET_FOLDER_LISTING = 0x02,
    MAP_ACTION_MAS_GET_MSG_LISTING    = 0x03,
    MAP_ACTION_MAS_GET_MSG            = 0x04,
    MAP_ACTION_MAS_PUSH_MSG           = 0x05,
} T_MAP_ACTION;

typedef enum
{
    MAP_OP_SET_FOLDER         = 0x00,
    MAP_OP_REG_MSG_NOTIF      = 0x01,
    MAP_OP_GET_FOLDER_LISTING = 0x02,
    MAP_OP_GET_MSG_LISTING    = 0x03,
    MAP_OP_GET_MSG            = 0x04,
    MAP_OP_GET_CONTINUE       = 0x05,
    MAP_OP_GET_ABORT          = 0x06,
} T_MAP_OPCODE;

typedef struct
{
    struct T_MAP_CMD          *p_next;
    uint8_t                    opcode;
    union
    {
        bool                          msg_notif_enable;
        T_MAP_REQ_SET_FOLDER          set_folder;
        T_MAP_REQ_GET_FOLDER_LISTING  get_folder_listing;
        T_MAP_REQ_GET_MSG_LISTING     get_msg_listing;
        T_MAP_REQ_GET_MSG             get_msg;
    } cmd_param;
} T_MAP_CMD;

typedef struct
{
    uint8_t       bd_addr[6];
    uint8_t       srm_status;
    uint8_t       cmd_credits;
    T_OS_QUEUE    cmd_queue;
    T_MAP_STATE   state;
    T_MAP_ACTION  action;
    T_OBEX_HANDLE mas_handle;
    T_OBEX_HANDLE mns_handle;
    bool          feat_flag;
    bool          obex_over_l2c;
} T_MAP_LINK;

typedef struct
{
    P_MAP_CBACK    cback;
    T_MAP_LINK     *p_link;

    uint8_t         obex_over_rfc_idx;
    uint8_t         obex_over_l2c_idx;
    uint8_t         link_num;
} T_MAP;

T_MAP *p_map;

const uint8_t map_target[MAP_TARGET_LEN + 1] =
{
    MAP_TARGET_LEN,     //indicate total length
    OBEX_HI_TARGET,
    (uint8_t)(MAP_TARGET_LEN >> 8),
    (uint8_t)MAP_TARGET_LEN,
    //target uuid
    0xbb, 0x58, 0x2b, 0x40, 0x42, 0x0c, 0x11, 0xdb, 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};

uint8_t map_target_feats[MAP_TARGET_LEN + 9 + 1] =
{
    (MAP_TARGET_LEN + 9),   //indicate total length
    OBEX_HI_TARGET,
    (uint8_t)(MAP_TARGET_LEN >> 8),
    (uint8_t)(MAP_TARGET_LEN),
    //target uuid
    0xbb, 0x58, 0x2b, 0x40, 0x42, 0x0c, 0x11, 0xdb, 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66,
    //application parameter with Tag
    OBEX_HI_APP_PARAMETER,
    //length : 0x08
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
    //who uuid
    0xbb, 0x58, 0x2b, 0x41, 0x42, 0x0c, 0x11, 0xdb, 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};

//NULL terminated ASCII : x-bt/MAP-NotificationRegistration
const uint8_t map_notif_reg_type[MAP_NOTIF_REG_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_NOTIF_REG_TYPE_LEN >> 8),
    (uint8_t)MAP_NOTIF_REG_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x4d, 0x41, 0x50, 0x2d, 0x4e, 0x6f, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61,
    0x74, 0x69, 0x6f, 0x6e, 0x52, 0x65, 0x67, 0x69, 0x73, 0x74, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x00
};

//NULL terminated ASCII : x-bt/MAP-event-report
const uint8_t map_evt_report[MAP_EVT_REPORT_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_EVT_REPORT_LEN >> 8),
    (uint8_t)MAP_EVT_REPORT_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x4d, 0x41, 0x50, 0x2d, 0x65, 0x76,
    0x65, 0x6e, 0x74, 0x2d, 0x72, 0x65, 0x70, 0x6f, 0x72, 0x74, 0x00
};

//NULL terminated ASCII : x-obex/folder-listing
const uint8_t map_get_folder_listing_type[MAP_GET_FOLDER_LISTING_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_GET_FOLDER_LISTING_LEN >> 8),
    (uint8_t)MAP_GET_FOLDER_LISTING_LEN,
    0x78, 0x2d, 0x6f, 0x62, 0x65, 0x78, 0x2f, 0x66, 0x6f, 0x6c, 0x64,
    0x65, 0x72, 0x2d, 0x6c, 0x69, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x00
};

//NULL terminated ASCII : x-bt/MAP-msg_listing
const uint8_t map_msg_listing_type[MAP_MSG_LISTING_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_MSG_LISTING_LEN >> 8),
    (uint8_t)MAP_MSG_LISTING_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x4d, 0x41, 0x50, 0x2d, 0x6d, 0x73,
    0x67, 0x2d, 0x6c, 0x69, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x00
};

//NULL terminated ASCII : x-bt/message
const uint8_t map_msg_type[MAP_MSG_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(MAP_MSG_TYPE_LEN >> 8),
    (uint8_t)MAP_MSG_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x00
};

T_MAP_LINK *map_alloc_link(uint8_t bd_addr[6])
{
    uint8_t i;
    T_MAP_LINK *p_link;

    for (i = 0; i < p_map->link_num; i++)
    {
        p_link = &p_map->p_link[i];
        if (p_link->state == MAP_STATE_DISCONNECTED)
        {
            p_link->mas_handle = NULL;
            p_link->mns_handle = NULL;
            memcpy(p_link->bd_addr, bd_addr, 6);

            return p_link;
        }
    }

    return NULL;
}

void map_free_link(T_MAP_LINK *p_link)
{
    memset(p_link, 0, sizeof(T_MAP_LINK));
}

T_MAP_LINK *map_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t i;
    T_MAP_LINK *p_link;

    for (i = 0; i < p_map->link_num; i++)
    {
        p_link = &p_map->p_link[i];

        if ((p_link->state != MAP_STATE_DISCONNECTED) && (memcmp(p_link->bd_addr, bd_addr, 6) == 0))
        {
            return p_link;
        }
    }

    return NULL;
}

bool map_send_cmd_register_msg_notification(T_MAP_LINK *p_link,
                                            bool        enable)
{
    uint8_t *p_data;
    uint8_t *p;
    bool ret;
    uint8_t data;

    p_data = os_mem_alloc2(MAP_NOTIF_REG_TYPE_LEN + MAP_ENABLE_NOTIF_STATUS_LEN);
    if (p_data == NULL)
    {
        return false;
    }

    p = p_data;
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

    p_link->action = MAP_ACTION_MAS_SET_NOTIF_REG;

    data = 0x30;
    ret = obex_put_data(p_link->mas_handle, p_data, p - p_data, false, &data, 1, false);
    os_mem_free(p_data);

    return ret;
}

bool map_send_cmd_get_folder_listing(T_MAP_LINK *p_link,
                                     uint16_t    max_list_count,
                                     uint16_t    start_offset)
{
    uint8_t *p_data;
    uint8_t *p;
    bool ret;

    uint16_t pkt_len = MAP_GET_FOLDER_LISTING_LEN + 0x000B;
    if (p_link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    p_data = os_mem_alloc2(pkt_len);
    if (p_data == NULL)
    {
        return false;
    }

    p = p_data;

    if (p_link->obex_over_l2c)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
        BE_UINT8_TO_STREAM(p, 0x01); //ENABLE
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

    p_link->action = MAP_ACTION_MAS_GET_FOLDER_LISTING;

    ret = obex_get_object(p_link->mas_handle, p_data, p - p_data);
    os_mem_free(p_data);

    return ret;
}

bool map_send_cmd_get_msg_listing(T_MAP_LINK *p_link,
                                  uint8_t    *folder,
                                  uint16_t    folder_len,
                                  uint16_t    max_list_count,
                                  uint16_t    start_offset)
{
    uint8_t *p_data;
    uint8_t *p;
    bool ret;

    uint16_t pkt_len = OBEX_HDR_NAME_LEN + folder_len + MAP_MSG_LISTING_LEN + 0x000B;
    if (p_link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    p_data = os_mem_alloc2(pkt_len);
    if (p_data == NULL)
    {
        return false;
    }

    p = p_data;

    if (p_link->obex_over_l2c)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
        BE_UINT8_TO_STREAM(p, 0x01); //ENABLE
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
    // BE_UINT8_TO_STREAM(p, TAG_ID_SUBJECT_LENGTH);
    // BE_UINT8_TO_STREAM(p, 0x01);
    // BE_UINT8_TO_STREAM(p, subject_len);
    // BE_UINT8_TO_STREAM(p, TAG_ID_FILTER_MSG_TYPE);
    // BE_UINT8_TO_STREAM(p, 0x01);
    // BE_UINT8_TO_STREAM(p, filter_msg_type);
    // BE_UINT8_TO_STREAM(p, TAG_ID_FILTER_READ_STATUS);
    // BE_UINT8_TO_STREAM(p, 0x01);
    // BE_UINT8_TO_STREAM(p, filter_read_status);

    p_link->action = MAP_ACTION_MAS_GET_MSG_LISTING;

    ret = obex_get_object(p_link->mas_handle, p_data, p - p_data);
    os_mem_free(p_data);

    return ret;
}

bool map_send_cmd_get_msg(T_MAP_LINK *p_link,
                          uint8_t    *msg_handle,
                          uint8_t     handle_len,
                          bool        native)
{
    uint8_t *p_data;
    uint8_t *p;
    bool ret;

    uint16_t pkt_len = OBEX_HDR_NAME_LEN + handle_len + MAP_MSG_TYPE_LEN + 0x0009;
    if (p_link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    p_data = os_mem_alloc2(pkt_len);
    if (p_data == NULL)
    {
        return false;
    }

    p = p_data;

    if (p_link->obex_over_l2c)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
        BE_UINT8_TO_STREAM(p, 0x01); //ENABLE
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

    p_link->action = MAP_ACTION_MAS_GET_MSG;

    ret = obex_get_object(p_link->mas_handle, p_data, p - p_data);
    os_mem_free(p_data);

    return ret;
}

void map_cmd_process(T_MAP_LINK *p_link)
{
    if (p_link != NULL &&
        p_link->mas_handle != NULL)
    {
        T_MAP_CMD *p_cmd;
        bool ret = false;

        p_cmd = (T_MAP_CMD *)p_link->cmd_queue.p_first;
        if (p_cmd != NULL)
        {
            PROFILE_PRINT_INFO1("map_cmd_process: opcode=%d", p_cmd->opcode);

            if (p_link->cmd_credits == 0)
            {
                PROFILE_PRINT_ERROR0("map_cmd_process failed, no credits");
                return;
            }

            switch (p_cmd->opcode)
            {
            case MAP_OP_SET_FOLDER:
                {
                    uint8_t flag;

                    if (p_cmd->cmd_param.set_folder.back)
                    {
                        flag = 0x03;
                    }
                    else
                    {
                        flag = 0x02;
                    }

                    ret = obex_set_path(p_link->mas_handle, flag, p_cmd->cmd_param.set_folder.p_folder,
                                        p_cmd->cmd_param.set_folder.folder_len);
                }
                break;

            case MAP_OP_REG_MSG_NOTIF:
                {
                    ret = map_send_cmd_register_msg_notification(p_link, p_cmd->cmd_param.msg_notif_enable);
                }
                break;

            case MAP_OP_GET_FOLDER_LISTING:
                {
                    ret = map_send_cmd_get_folder_listing(p_link,
                                                          p_cmd->cmd_param.get_folder_listing.max_list_count,
                                                          p_cmd->cmd_param.get_folder_listing.start_offset);
                }
                break;

            case MAP_OP_GET_MSG_LISTING:
                {
                    ret = map_send_cmd_get_msg_listing(p_link,
                                                       p_cmd->cmd_param.get_msg_listing.p_folder,
                                                       p_cmd->cmd_param.get_msg_listing.folder_len,
                                                       p_cmd->cmd_param.get_msg_listing.max_list_count,
                                                       p_cmd->cmd_param.get_msg_listing.start_offset);
                }
                break;

            case MAP_OP_GET_MSG:
                {
                    ret = map_send_cmd_get_msg(p_link,
                                               p_cmd->cmd_param.get_msg.msg_handle,
                                               p_cmd->cmd_param.get_msg.handle_len,
                                               p_cmd->cmd_param.get_msg.native);
                }
                break;

            case MAP_OP_GET_CONTINUE:
                {
                    ret = obex_get_object_continue(p_link->mas_handle);
                }
                break;

            case MAP_OP_GET_ABORT:
                {
                    ret = obex_abort(p_link->mas_handle);
                    p_link->action = MAP_ACTION_MAS_IDLE;
                }
                break;

            default:
                break;
            }

            if (ret == true)
            {
                p_link->cmd_credits = 0; //Wait command response
                os_queue_delete(&p_link->cmd_queue, p_cmd);
                os_mem_free(p_cmd);
            }
            else
            {
                //if send cmd failed, process next cmd
                PROFILE_PRINT_ERROR0("map_cmd_process: command failed");

                os_queue_delete(&p_link->cmd_queue, p_cmd);
                os_mem_free(p_cmd);
                map_cmd_process(p_link);
            }
        }
    }
}

void map_handle_folder_listing_data_ind(T_MAP_LINK                  *p_link,
                                        T_OBEX_GET_OBJECT_CMPL_DATA *p_data)
{
    T_MAP_GET_FOLDER_LISTING_MSG_DATA data = {0};
    uint8_t *p_hdr_data;
    uint16_t hdr_data_len;
    bool ret;

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_APP_PARAMETER, &p_hdr_data,
                               &hdr_data_len);

    if (ret && *p_hdr_data == TAG_ID_FOLDER_LISTING_SIZE)
    {
        BE_ARRAY_TO_UINT16(data.folder_listing_size, p_hdr_data + 2);
    }

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &p_hdr_data,
                               &hdr_data_len);
    if (ret == false)
    {
        ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &p_hdr_data,
                                   &hdr_data_len);
        if (ret)
        {
            p_link->action = MAP_ACTION_MAS_IDLE;
        }
    }

    if (ret)
    {
        data.p_data = p_hdr_data;
        data.data_len = hdr_data_len;
    }

    if (p_data->rsp_code != OBEX_RSP_CONTINUE)
    {
        data.data_end = true;
    }

    p_map->cback(p_link->bd_addr, MAP_MSG_GET_FOLDER_LISTING_CMPL, (void *)&data);
}

void map_handle_msg_listing_data_ind(T_MAP_LINK                  *p_link,
                                     T_OBEX_GET_OBJECT_CMPL_DATA *p_data)
{
    T_MAP_GET_MSG_LISTING_MSG_DATA data = {0};
    uint8_t *p_hdr_data;
    uint16_t hdr_data_len;
    uint8_t *p_tag_data;
    uint16_t tag_data_len;
    bool ret;

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_APP_PARAMETER, &p_hdr_data,
                               &hdr_data_len);
    if (ret)
    {
        ret = obex_find_value_in_app_param(p_hdr_data, hdr_data_len, TAG_ID_NEW_MESSAGE, &p_tag_data,
                                           &tag_data_len);
        if (ret)
        {
            data.new_msg = *p_tag_data;
        }

        ret = obex_find_value_in_app_param(p_hdr_data, hdr_data_len, TAG_ID_MSG_LISTING_SIZE, &p_tag_data,
                                           &tag_data_len);
        if (ret)
        {
            BE_STREAM_TO_UINT16(data.msg_listing_size, p_tag_data);
        }
    }

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &p_hdr_data,
                               &hdr_data_len);
    if (ret == false)
    {
        ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &p_hdr_data,
                                   &hdr_data_len);
        if (ret)
        {
            p_link->action = MAP_ACTION_MAS_IDLE;
        }
    }

    if (p_data->rsp_code != OBEX_RSP_CONTINUE)
    {
        data.data_end = true;
    }

    if (ret)
    {
        data.p_data = p_hdr_data;
        data.data_len = hdr_data_len;
    }

    p_map->cback(p_link->bd_addr, MAP_MSG_GET_MSG_LISTING_CMPL, (void *)&data);
}

void map_handle_msg_data_ind(T_MAP_LINK                  *p_link,
                             T_OBEX_GET_OBJECT_CMPL_DATA *p_data)
{
    T_MAP_GET_MSG_DATA data = {0};
    uint8_t *p_hdr_data;
    uint16_t hdr_data_len;
    bool ret;

    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &p_hdr_data,
                               &hdr_data_len);
    if (ret == false)
    {
        ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &p_hdr_data,
                                   &hdr_data_len);
        if (ret)
        {
            p_link->action = MAP_ACTION_MAS_IDLE;
        }
    }

    if (ret)
    {
        data.p_data = p_hdr_data;
        data.data_len = hdr_data_len;
    }

    if (p_data->rsp_code != OBEX_RSP_CONTINUE)
    {
        data.data_end = true;
    }

    p_map->cback(p_link->bd_addr, MAP_MSG_GET_MSG_CMPL, (void *)&data);
}

void map_obex_cb(uint8_t  msg_type,
                 void    *p_msg)
{
    T_MAP_LINK *p_link;

    PROFILE_PRINT_TRACE1("map_obex_cb: msg_type=0x%x", msg_type);

    switch (msg_type)
    {
    case OBEX_CB_MSG_CONN_IND:
        {
            T_OBEX_CONN_IND_DATA *p_data = (T_OBEX_CONN_IND_DATA *)p_msg;

            p_link = map_find_link_by_addr(p_data->bd_addr);
            if (p_link != NULL && p_link->mns_handle == NULL)
            {
                p_link->mns_handle = p_data->handle;
                p_map->cback(p_link->bd_addr, MAP_MSG_MNS_CONN_IND, NULL);
            }
            else
            {
                // mas not connected or mns already connected
                obex_conn_cfm(p_data->handle, false, NULL, 0);
            }
        }
        break;

    case OBEX_CB_MSG_CONN_CMPL:
        {
            T_OBEX_CONN_CMPL_DATA *p_data = (T_OBEX_CONN_CMPL_DATA *)p_msg;

            p_link = map_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                if (p_link->mas_handle == p_data->handle)
                {
                    p_link->state = MAP_STATE_MAS_CONNECTED;
                    os_queue_init(&p_link->cmd_queue);
                    p_link->cmd_credits = 1;

                    if (p_map->cback)
                    {
                        p_map->cback(p_link->bd_addr, MAP_MSG_MAS_CONNECTED, &(p_data->max_pkt_len));
                    }
                }
                else
                {
                    p_link->state = MAP_STATE_MNS_CONNECTED;
                    if (p_map->cback)
                    {
                        p_map->cback(p_link->bd_addr, MAP_MSG_MNS_CONNECTED, NULL);
                    }
                }
            }
            else
            {
                obex_disconn_req(p_data->handle);
            }
        }
        break;

    case OBEX_CB_MSG_DISCONN:
        {
            T_OBEX_DISCONN_CMPL_DATA *p_data = (T_OBEX_DISCONN_CMPL_DATA *)p_msg;

            p_link = map_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                if (p_data->handle == p_link->mns_handle)
                {
                    p_link->mns_handle = NULL;
                    p_link->state = MAP_STATE_MAS_CONNECTED;

                    if (p_map->cback)
                    {
                        if (p_link->state == MAP_STATE_MNS_CONNECTING)
                        {
                            p_map->cback(p_link->bd_addr, MAP_MSG_MNS_CONN_FAIL, &p_data->cause);
                        }
                        else
                        {
                            T_MAP_DISCONN_INFO info;
                            info.cause = p_data->cause;
                            p_map->cback(p_link->bd_addr, MAP_MSG_MNS_DISCONNECTED, &info);
                        }
                    }
                }
                else if (p_data->handle == p_link->mas_handle)
                {
                    T_MAP_CMD *p_cmd;
                    T_MAP_CMD *p_next;

                    p_cmd = (T_MAP_CMD *)p_link->cmd_queue.p_first;
                    while (p_cmd)
                    {
                        p_next = (T_MAP_CMD *)p_cmd->p_next;

                        os_queue_delete(&p_link->cmd_queue, p_cmd);
                        if (p_cmd->opcode == MAP_OP_SET_FOLDER)
                        {
                            if (p_cmd->cmd_param.set_folder.p_folder != NULL)
                            {
                                os_mem_free(p_cmd->cmd_param.set_folder.p_folder);
                            }
                        }
                        else if (p_cmd->opcode == MAP_OP_GET_MSG_LISTING)
                        {
                            if (p_cmd->cmd_param.get_msg_listing.p_folder != NULL)
                            {
                                os_mem_free(p_cmd->cmd_param.get_msg_listing.p_folder);
                            }
                        }
                        else if (p_cmd->opcode == MAP_OP_GET_MSG)
                        {
                            if (p_cmd->cmd_param.get_msg.msg_handle != NULL)
                            {
                                os_mem_free(p_cmd->cmd_param.get_msg.msg_handle);
                            }
                        }
                        os_mem_free(p_cmd);

                        p_cmd = p_next;
                    }

                    if (p_map->cback)
                    {
                        if (p_link->state == MAP_STATE_MAS_CONNECTING)
                        {
                            p_map->cback(p_link->bd_addr, MAP_MSG_MAS_CONN_FAIL, &p_data->cause);
                        }
                        else
                        {
                            T_MAP_DISCONN_INFO info;
                            info.cause = p_data->cause;
                            p_map->cback(p_link->bd_addr, MAP_MSG_MAS_DISCONNECTED, &info);
                        }
                    }
                    map_free_link(p_link);
                }
            }
        }
        break;

    case OBEX_CB_MSG_SET_PATH_DONE:
        {
            T_OBEX_SET_PATH_CMPL_DATA *p_data = (T_OBEX_SET_PATH_CMPL_DATA *)p_msg;
            bool result = false;

            p_link = map_find_link_by_addr(p_data->bd_addr);
            if (p_link && p_link->mas_handle == p_data->handle)
            {
                if (p_data->cause == OBEX_RSP_SUCCESS)
                {
                    result = true;
                }
                else
                {
                    result = false;
                }

                if (p_map->cback)
                {
                    p_map->cback(p_link->bd_addr, MAP_MSG_SET_FOLDER_CMPL, &result);
                }

                p_link->cmd_credits = 1;
                map_cmd_process(p_link);
            }
        }
        break;

    case OBEX_CB_MSG_PUT_DONE:
        {
            T_OBEX_PUT_CMPL_DATA *p_data = (T_OBEX_PUT_CMPL_DATA *)p_msg;
            bool result = false;

            p_link = map_find_link_by_addr(p_data->bd_addr);
            if (p_link && p_link->mas_handle == p_data->handle)
            {
                if (p_link->action == MAP_ACTION_MAS_SET_NOTIF_REG)
                {
                    if (p_data->cause == OBEX_RSP_SUCCESS)
                    {
                        result = true;
                    }

                    p_link->action = MAP_ACTION_MAS_IDLE;
                    if (p_map->cback)
                    {
                        p_map->cback(p_link->bd_addr, MAP_MSG_REG_NOTIF_CMPL, &result);
                    }
                }
                else if (p_link->action == MAP_ACTION_MAS_PUSH_MSG)
                {
                    T_MAP_PUSH_MSG_CMPL data;

                    data.rsp_code = p_data->cause;
                    data.p_msg_handle = p_data->p_name;
                    data.msg_handle_len = p_data->name_len;

                    p_link->srm_status = p_data->srm_status;
                    if (p_data->cause == OBEX_RSP_CONTINUE)
                    {
                        if (p_map->cback)
                        {
                            if (p_link->srm_status == SRM_ENABLE)
                            {
                                data.action = MAP_PUSH_MSG_TO_END;
                            }
                            else if (p_link->srm_status == SRM_WAIT)
                            {
                                data.action = MAP_PUSH_MSG_AGAIN;
                            }
                            else
                            {
                                data.action = MAP_PUSH_MSG_CONTINUE;
                            }
                            p_map->cback(p_link->bd_addr, MAP_MSG_PUSH_MSG_CMPL, &data);
                        }
                    }
                    else
                    {
                        p_link->action = MAP_ACTION_MAS_IDLE;
                        if (p_map->cback)
                        {
                            data.action = MAP_PUSH_MSG_COMPLETE;
                            p_map->cback(p_link->bd_addr, MAP_MSG_PUSH_MSG_CMPL, &data);
                        }
                    }
                }

                p_link->cmd_credits = 1;
                map_cmd_process(p_link);
            }
        }
        break;

    case OBEX_CB_MSG_GET_OBJECT:
        {
            T_OBEX_GET_OBJECT_CMPL_DATA *p_data = (T_OBEX_GET_OBJECT_CMPL_DATA *)p_msg;

            p_link = map_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                p_link->srm_status = p_data->srm_status;
                p_link->cmd_credits = 1;
                if (p_link->mas_handle != p_data->handle || p_map->cback == NULL)
                {
                    map_cmd_process(p_link);
                    break;
                }

                if (p_data->rsp_code != OBEX_RSP_SUCCESS && p_data->rsp_code != OBEX_RSP_CONTINUE)
                {
                    p_link->action = MAP_ACTION_MAS_IDLE;
                    PROFILE_PRINT_ERROR1("OBEX_CB_MSG_GET_OBJECT: error rsp_code=0x%x", p_data->rsp_code);
                    map_cmd_process(p_link);
                    break;
                }

                if (p_link->action == MAP_ACTION_MAS_GET_FOLDER_LISTING)
                {
                    map_handle_folder_listing_data_ind(p_link, p_data);
                }
                else if (p_link->action == MAP_ACTION_MAS_GET_MSG_LISTING)
                {
                    map_handle_msg_listing_data_ind(p_link, p_data);
                }
                else if (p_link->action == MAP_ACTION_MAS_GET_MSG)
                {
                    map_handle_msg_data_ind(p_link, p_data);
                }

                map_cmd_process(p_link);
            }
        }
        break;

    case OBEX_CB_MSG_REMOTE_PUT:
        {
            T_OBEX_REMOTE_PUT_DATA *p_data = (T_OBEX_REMOTE_PUT_DATA *)p_msg;
            T_MAP_NOTIF_MSG_DATA data;
            uint8_t *p_hdr_data;
            uint16_t hdr_data_len;
            bool ret;

            p_link = map_find_link_by_addr(p_data->bd_addr);
            if (p_link == NULL || p_link->mns_handle != p_data->handle || p_map->cback == NULL)
            {
                break;
            }

            ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &p_hdr_data,
                                       &hdr_data_len);
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

            if (ret == false)
            {
                break;
            }

            data.p_data = p_hdr_data;
            data.data_len = hdr_data_len;

            p_map->cback(p_link->bd_addr, MAP_MSG_DATA_NOTIFICATION, (void *)&data);
        }
        break;

    default:
        break;
    }
}

bool map_mns_connect_cfm(uint8_t bd_addr[6],
                         bool    accept)
{
    T_MAP_LINK *p_link;

    p_link = map_find_link_by_addr(bd_addr);
    if ((p_link != NULL) && (p_link->mns_handle != NULL))
    {
        if (accept)
        {
            p_link->state = MAP_STATE_MNS_CONNECTING;
            return obex_conn_cfm(p_link->mns_handle, true, (uint8_t *)map_who, MAP_WHO_LEN);
        }
        else
        {
            return obex_conn_cfm(p_link->mns_handle, false, NULL, 0);
        }
    }

    return false;
}

bool map_conn_mas_over_rfc(uint8_t bd_addr[6],
                           uint8_t server_chann,
                           bool    feat_flag)
{
    T_MAP_LINK *p_link;
    uint8_t *p_param;

    ///TODO: support only one mas per link now
    p_link = map_find_link_by_addr(bd_addr);
    if (p_link)
    {
        return false;
    }

    p_link = map_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_link->feat_flag = feat_flag;
    if (feat_flag)
    {
        p_param = map_target_feats;
    }
    else
    {
        p_param = (uint8_t *)map_target;
    }

    if (obex_conn_req_over_rfc(bd_addr, p_param, server_chann, map_obex_cb, &p_link->mas_handle))
    {
        p_link->state = MAP_STATE_MAS_CONNECTING;
        p_link->obex_over_l2c = false;
        return true;
    }
    else
    {
        map_free_link(p_link);
        return false;
    }
}

bool map_conn_mas_over_l2c(uint8_t  bd_addr[6],
                           uint16_t l2c_psm,
                           bool     feat_flag)
{
    T_MAP_LINK *p_link;
    uint8_t *p_param;

    ///TODO: support only one mas per link now
    p_link = map_find_link_by_addr(bd_addr);
    if (p_link)
    {
        return false;
    }

    p_link = map_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_link->feat_flag = feat_flag;
    if (feat_flag)
    {
        p_param = map_target_feats;
    }
    else
    {
        p_param = (uint8_t *)map_target;
    }

    p_link->state = MAP_STATE_MAS_CONNECTING;

    if (obex_conn_req_over_l2c(bd_addr, p_param, l2c_psm, map_obex_cb, &p_link->mas_handle))
    {
        p_link->obex_over_l2c = true;
        return true;
    }
    else
    {
        map_free_link(p_link);
        return false;
    }
}

bool map_disconn_mas(uint8_t bd_addr[6])
{
    T_MAP_LINK *p_link;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link == NULL || p_link->mas_handle == NULL)
    {
        return false;
    }

    return obex_disconn_req(p_link->mas_handle);
}

bool map_cmd_enqueue(T_MAP_LINK *p_link,
                     uint8_t     opcode,
                     void       *param)
{
    T_MAP_CMD *p_cmd = NULL;

    PROFILE_PRINT_INFO1("map_cmd_enqueue: opcode=%d", opcode);

    if (p_link != NULL)
    {
        if (p_link->cmd_queue.count == MAP_MAX_QUEUED_CMD)
        {
            PROFILE_PRINT_ERROR0("map_cmd_enqueue: cmd queue full");
            goto fail_cmd_enqueue;
        }

        p_cmd = os_mem_zalloc2(sizeof(T_MAP_CMD));
        if (p_cmd == NULL)
        {
            goto fail_cmd_enqueue;
        }

        p_cmd->opcode = opcode;
        switch (opcode)
        {
        case MAP_OP_SET_FOLDER:
            {
                T_MAP_REQ_SET_FOLDER *tmp = (T_MAP_REQ_SET_FOLDER *)param;

                p_cmd->cmd_param.set_folder.p_folder = os_mem_alloc2(tmp->folder_len);
                if (p_cmd->cmd_param.set_folder.p_folder == NULL)
                {
                    goto fail_cmd_enqueue;
                }

                p_cmd->cmd_param.set_folder.back = tmp->back;
                memcpy(p_cmd->cmd_param.set_folder.p_folder, tmp->p_folder, tmp->folder_len);
                p_cmd->cmd_param.set_folder.folder_len = tmp->folder_len;
            }
            break;

        case MAP_OP_REG_MSG_NOTIF:
            {
                p_cmd->cmd_param.msg_notif_enable = *(bool *)param;
            }
            break;

        case MAP_OP_GET_FOLDER_LISTING:
            {
                T_MAP_REQ_GET_FOLDER_LISTING *tmp = (T_MAP_REQ_GET_FOLDER_LISTING *)param;

                p_cmd->cmd_param.get_folder_listing.max_list_count = tmp->max_list_count;
                p_cmd->cmd_param.get_folder_listing.start_offset = tmp->start_offset;
            }
            break;

        case MAP_OP_GET_MSG_LISTING:
            {
                T_MAP_REQ_GET_MSG_LISTING *tmp = (T_MAP_REQ_GET_MSG_LISTING *)param;

                p_cmd->cmd_param.get_msg_listing.p_folder = os_mem_alloc2(tmp->folder_len);
                if (p_cmd->cmd_param.get_msg_listing.p_folder == NULL)
                {
                    goto fail_cmd_enqueue;
                }

                memcpy(p_cmd->cmd_param.get_msg_listing.p_folder, tmp->p_folder, tmp->folder_len);
                p_cmd->cmd_param.get_msg_listing.folder_len = tmp->folder_len;
                p_cmd->cmd_param.get_msg_listing.max_list_count = tmp->max_list_count;
                p_cmd->cmd_param.get_msg_listing.start_offset = tmp->start_offset;
            }
            break;

        case MAP_OP_GET_MSG:
            {
                T_MAP_REQ_GET_MSG *tmp = (T_MAP_REQ_GET_MSG *)param;

                p_cmd->cmd_param.get_msg.msg_handle = os_mem_alloc2(tmp->handle_len);
                if (p_cmd->cmd_param.get_msg.msg_handle == NULL)
                {
                    goto fail_cmd_enqueue;
                }

                memcpy(p_cmd->cmd_param.get_msg.msg_handle, tmp->msg_handle, tmp->handle_len);
                p_cmd->cmd_param.get_msg.handle_len = tmp->handle_len;
                p_cmd->cmd_param.get_msg.native = tmp->native;
            }
            break;

        case MAP_OP_GET_CONTINUE:
            break;

        case MAP_OP_GET_ABORT:
            break;

        default:
            goto fail_cmd_enqueue;
        }

        os_queue_in(&p_link->cmd_queue, p_cmd);

        map_cmd_process(p_link);

        return true;
    }

fail_cmd_enqueue:
    if (p_cmd != NULL)
    {
        os_mem_free(p_cmd);
    }
    return false;
}

bool map_send_event_rsp(uint8_t bd_addr[6],
                        uint8_t rsp_code)
{
    T_MAP_LINK *p_link;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link == NULL || p_link->mns_handle == NULL)
    {
        return false;
    }

    return obex_send_rsp(p_link->mns_handle, rsp_code);
}

bool map_set_folder(uint8_t   bd_addr[6],
                    bool      back,
                    uint8_t  *folder,
                    uint16_t  folder_len)
{
    T_MAP_LINK *p_link;
    bool ret = false;
    uint8_t flag;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->mas_handle != NULL)
    {
        if (p_link->cmd_credits == 1)
        {
            if (back)
            {
                flag = 0x03;
            }
            else
            {
                flag = 0x02;
            }

            ret = obex_set_path(p_link->mas_handle, flag, folder, folder_len);
            if (ret == true)
            {
                p_link->cmd_credits = 0;
            }
        }
        else
        {
            T_MAP_REQ_SET_FOLDER tmp;

            tmp.back = back;
            tmp.p_folder = folder;
            tmp.folder_len = folder_len;

            ret = map_cmd_enqueue(p_link, MAP_OP_SET_FOLDER, &tmp);
        }
    }

    return ret;
}

bool map_register_msg_notification(uint8_t bd_addr[6],
                                   bool    enable)
{
    T_MAP_LINK *p_link;
    bool ret = false;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->mas_handle != NULL)
    {
        if (p_link->cmd_credits == 1)
        {
            ret = map_send_cmd_register_msg_notification(p_link, enable);
            if (ret == true)
            {
                p_link->cmd_credits = 0;
            }
        }
        else
        {
            ret = map_cmd_enqueue(p_link, MAP_OP_REG_MSG_NOTIF, &enable);
        }
    }

    return ret;
}

bool map_get_folder_listing(uint8_t  bd_addr[6],
                            uint16_t max_list_count,
                            uint16_t start_offset)
{
    T_MAP_LINK *p_link;
    bool ret = false;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->mas_handle != NULL)
    {
        if (p_link->cmd_credits == 1)
        {
            ret = map_send_cmd_get_folder_listing(p_link, max_list_count, start_offset);
            if (ret == true)
            {
                p_link->cmd_credits = 0;
            }
        }
        else
        {
            T_MAP_REQ_GET_FOLDER_LISTING tmp;

            tmp.max_list_count = max_list_count;
            tmp.start_offset = start_offset;

            ret = map_cmd_enqueue(p_link, MAP_OP_GET_FOLDER_LISTING, &tmp);
        }
    }

    return ret;
}

bool map_get_msg_listing(uint8_t   bd_addr[6],
                         uint8_t  *folder,
                         uint16_t  folder_len,
                         uint16_t  max_list_count,
                         uint16_t  start_offset)
{
    T_MAP_LINK *p_link;
    bool ret = false;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->mas_handle != NULL)
    {
        if (p_link->cmd_credits == 1)
        {
            ret = map_send_cmd_get_msg_listing(p_link, folder, folder_len, max_list_count,
                                               start_offset);
            if (ret == true)
            {
                p_link->cmd_credits = 0;
            }
        }
        else
        {
            T_MAP_REQ_GET_MSG_LISTING tmp;

            tmp.p_folder = folder;
            tmp.folder_len = folder_len;
            tmp.max_list_count = max_list_count;
            tmp.start_offset = start_offset;

            ret = map_cmd_enqueue(p_link, MAP_OP_GET_MSG_LISTING, &tmp);
        }
    }

    return ret;
}

bool map_get_msg(uint8_t  bd_addr[6],
                 uint8_t *msg_handle,
                 uint8_t  handle_len,
                 bool     native)
{
    T_MAP_LINK *p_link;
    bool ret = false;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->mas_handle != NULL)
    {
        if (p_link->cmd_credits == 1)
        {
            ret = map_send_cmd_get_msg(p_link, msg_handle, handle_len, native);
            if (ret == true)
            {
                p_link->cmd_credits = 0;
            }
        }
        else
        {
            T_MAP_REQ_GET_MSG tmp;

            tmp.msg_handle = msg_handle;
            tmp.handle_len = handle_len;
            tmp.native = native;

            ret = map_cmd_enqueue(p_link, MAP_OP_GET_MSG, &tmp);
        }
    }

    return ret;
}

bool map_get_continue(uint8_t bd_addr[6])
{
    T_MAP_LINK *p_link;
    bool ret = false;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->mas_handle != NULL)
    {
        if (p_link->srm_status == SRM_ENABLE)
        {
            ret = true;
        }
        else
        {
            if (p_link->cmd_credits == 1)
            {
                ret = obex_get_object_continue(p_link->mas_handle);
                if (ret == true)
                {
                    p_link->cmd_credits = 0;
                }
            }
            else
            {
                ret = map_cmd_enqueue(p_link, MAP_OP_GET_CONTINUE, NULL);
            }
        }
    }

    return ret;
}

bool map_get_abort(uint8_t bd_addr[6])
{
    T_MAP_LINK *p_link;
    bool ret = false;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->mas_handle != NULL)
    {
        if (p_link->cmd_credits == 1)
        {
            ret = obex_abort(p_link->mas_handle);
            if (ret == true)
            {
                p_link->cmd_credits = 0;
            }

            p_link->action = MAP_ACTION_MAS_IDLE;
        }
        else
        {
            ret = map_cmd_enqueue(p_link, MAP_OP_GET_ABORT, NULL);
        }
    }

    return ret;
}

bool map_push_msg(uint8_t   bd_addr[6],
                  uint8_t  *folder,
                  uint16_t  folder_len,
                  bool      native,
                  bool      more_data,
                  uint8_t  *msg,
                  uint16_t  msg_len)
{
    T_MAP_LINK *p_link;
    uint8_t *p_data;
    uint8_t *p;
    bool ret = false;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->mas_handle != NULL)
    {
        if (p_link->cmd_credits == 1 || p_link->srm_status == SRM_ENABLE)
        {
            if (p_link->obex_over_l2c && p_link->srm_status != SRM_DISABLE)
            {
                ret = obex_put_data(p_link->mas_handle, NULL, 0, more_data, msg, msg_len, false);
            }
            else
            {
                uint16_t pkt_len = OBEX_HDR_NAME_LEN + folder_len + MAP_MSG_TYPE_LEN + 0x0006;
                if (p_link->obex_over_l2c && p_link->srm_status == SRM_DISABLE)
                {
                    pkt_len += OBEX_HDR_SRM_LEN;
                }

                p_data = os_mem_alloc2(pkt_len);
                if (p_data == NULL)
                {
                    return false;
                }

                p = p_data;

                if (p_link->obex_over_l2c && p_link->srm_status == SRM_DISABLE)
                {
                    BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
                    BE_UINT8_TO_STREAM(p, 0x01); //ENABLE
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

                p_link->action = MAP_ACTION_MAS_PUSH_MSG;

                if (p_link->obex_over_l2c)
                {
                    ret = obex_put_data(p_link->mas_handle, p_data, p - p_data, more_data, NULL, 0, false);
                }
                else
                {
                    ret = obex_put_data(p_link->mas_handle, p_data, p - p_data, more_data, msg, msg_len, false);
                }
                os_mem_free(p_data);
            }
        }
        else
        {
            PROFILE_PRINT_ERROR0("map_push_msg failed: no credits");
        }
    }

    return ret;
}

bool map_init(uint8_t     link_num,
              P_MAP_CBACK cback,
              uint8_t     mns_server_chann,
              uint16_t    mns_l2c_psm,
              uint32_t    support_feat)
{
    int32_t ret = 0;

    p_map = os_mem_zalloc2(sizeof(T_MAP));
    if (p_map == NULL)
    {
        ret = 1;
        goto fail_alloc_map;
    }

    p_map->link_num = link_num;

    p_map->p_link = os_mem_zalloc2(p_map->link_num * sizeof(T_MAP_LINK));
    if (p_map->p_link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (obex_reg_cb_over_rfc(mns_server_chann, map_obex_cb, &p_map->obex_over_rfc_idx) == false)
    {
        ret = 3;
        goto fail_reg_rfc_cb;
    }

    if (mns_l2c_psm)
    {
        if (obex_reg_cb_over_l2c(mns_l2c_psm, map_obex_cb, &p_map->obex_over_l2c_idx) == false)
        {
            ret = 4;
            goto fail_reg_l2c_cb;
        }
    }

    if (support_feat)
    {
        BE_UINT32_TO_ARRAY(&map_target_feats[MAP_TARGET_LEN + 6], support_feat);
    }

    p_map->cback = cback;

    return true;

fail_reg_l2c_cb:
fail_reg_rfc_cb:
    os_mem_free(p_map->p_link);
fail_alloc_link:
    os_mem_free(p_map);
    p_map = NULL;
fail_alloc_map:
    PROFILE_PRINT_ERROR1("map_init: failed %d", -ret);
    return false;
}

bool map_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_MAP_INFO *p_info)
{
    T_MAP_LINK *p_link;
    T_OBEX_ROLESWAP_INFO obex_info;

    p_link = map_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_info->feat_flag = p_link->feat_flag;
    p_info->map_state = p_link->state;

    // get mas obex info
    if (obex_get_roleswap_info(p_link->mas_handle, &obex_info) == false)
    {
        return false;
    }

    p_info->mas_obex_conn_id = obex_info.connection_id;
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

    if (p_link->mns_handle != NULL)
    {
        if (obex_get_roleswap_info(p_link->mns_handle, &obex_info) == false)
        {
            return false;
        }

        p_info->mns_obex_conn_id = obex_info.connection_id;
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
        p_info->mns_l2c_cid = 0x00;
    }

    return true;
}

bool map_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_MAP_INFO *p_info)
{
    T_MAP_LINK *p_link;
    T_OBEX_ROLESWAP_INFO obex_info;

    p_link = map_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_link->state = (T_MAP_STATE)p_info->map_state;
    p_link->feat_flag = p_info->feat_flag;

    // set mas obex info
    obex_info.connection_id = p_info->mas_obex_conn_id;
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

    if (p_info->feat_flag)
    {
        obex_info.con_param_ptr = map_target_feats;
    }
    else
    {
        obex_info.con_param_ptr = (uint8_t *)map_target;
    }

    if (obex_set_roleswap_info(bd_addr, map_obex_cb, &obex_info, &p_link->mas_handle) == false)
    {
        map_free_link(p_link);
        return false;
    }

    if (p_info->mns_l2c_cid != 0x00)
    {
        // set mns obex info
        obex_info.connection_id = p_info->mns_obex_conn_id;
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

        if (obex_set_roleswap_info(bd_addr, map_obex_cb, &obex_info, &p_link->mns_handle) == false)
        {
            map_free_link(p_link);
            return false;
        }
    }

    return true;
}

uint8_t map_get_rfc_profile_idx(void)
{
    return obex_get_rfc_profile_idx();
}
#else
#include <stdint.h>
#include <stdbool.h>
#include "map.h"

bool map_init(uint8_t     link_num,
              P_MAP_CBACK cback,
              uint8_t     mns_server_chann,
              uint16_t    mns_l2c_psm,
              uint32_t    support_feat)
{
    return false;
}

bool map_mns_connect_cfm(uint8_t bd_addr[6],
                         bool    accept)
{
    return false;
}

bool map_conn_mas_over_rfc(uint8_t bd_addr[6],
                           uint8_t server_chann,
                           bool    feat_flag)
{
    return false;
}

bool map_conn_mas_over_l2c(uint8_t  bd_addr[6],
                           uint16_t mas_l2c_psm,
                           bool     feat_flag)
{
    return false;
}

bool map_send_event_rsp(uint8_t bd_addr[6],
                        uint8_t rsp_code)
{
    return false;
}

bool map_set_folder(uint8_t   bd_addr[6],
                    bool      back,
                    uint8_t  *folder,
                    uint16_t  folder_len)
{
    return false;
}

bool map_register_msg_notification(uint8_t bd_addr[6],
                                   bool    enable)
{
    return false;
}

bool map_get_folder_listing(uint8_t  bd_addr[6],
                            uint16_t max_list_cnt,
                            uint16_t start_offset)
{
    return false;
}

bool map_get_msg_listing(uint8_t   bd_addr[6],
                         uint8_t  *folder,
                         uint16_t  folder_len,
                         uint16_t  max_list_count,
                         uint16_t  start_offset)
{
    return false;
}

bool map_get_msg(uint8_t  bd_addr[6],
                 uint8_t *msg_handle,
                 uint8_t  handle_len,
                 bool     native)
{
    return false;
}

bool map_get_continue(uint8_t bd_addr[6])
{
    return false;
}

bool map_get_abort(uint8_t bd_addr[6])
{
    return false;
}

bool map_push_msg(uint8_t   bd_addr[6],
                  uint8_t  *folder,
                  uint16_t  folder_len,
                  bool      native,
                  bool      more_data,
                  uint8_t  *msg,
                  uint16_t  msg_len)
{
    return false;
}

bool map_disconn_mas(uint8_t bd_addr[6])
{
    return false;
}

bool map_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_MAP_INFO *p_info)
{
    return false;
}

bool map_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_MAP_INFO *p_info)
{
    return false;
}

uint8_t map_get_rfc_profile_idx(void)
{
    return false;
}
#endif
