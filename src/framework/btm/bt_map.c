/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_MAP_SUPPORT == 1)

#include <string.h>
#include "trace.h"
#include "map.h"
#include "bt_map_int.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_map.h"

#define PATH_TELECOM_LEN            0x10
#define PATH_MSG_LEN                0x08
#define PATH_INBOX_LEN              0x0c
#define PATH_OUTBOX_LEN             0x0e
#define PATH_SENT_LEN               0x0a
#define PATH_DELETED_LEN            0x10
#define PATH_DRAFT_LEN              0x0c

//NULL terminated UNICODE : telecom
const uint8_t bt_map_path_telecom[PATH_TELECOM_LEN] =
{
    0x00, 0x74, 0x00, 0x65, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x63, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x00
};

//NULL terminated UNICODE : msg
const uint8_t bt_map_path_msg[PATH_MSG_LEN] =
{
    0x00, 0x6d, 0x00, 0x73, 0x00, 0x67, 0x00, 0x00
};

//NULL terminated UNICODE : inbox
const uint8_t bt_map_path_inbox[PATH_INBOX_LEN] =
{
    0x00, 0x69, 0x00, 0x6e, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x78, 0x00, 0x00
};

//NULL terminated UNICODE : outbox
const uint8_t bt_map_path_outbox[PATH_OUTBOX_LEN] =
{
    0x00, 0x6f, 0x00, 0x75, 0x00, 0x74, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x78, 0x00, 0x00
};

//NULL terminated UNICODE : sent
const uint8_t bt_map_path_sent[PATH_SENT_LEN] =
{
    0x00, 0x73, 0x00, 0x65, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x00
};

//NULL terminated UNICODE : deleted
const uint8_t bt_map_path_deleted[PATH_DELETED_LEN] =
{
    0x00, 0x64, 0x00, 0x65, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x74, 0x00, 0x65, 0x00, 0x64, 0x00, 0x00
};

//NULL terminated UNICODE : draft
const uint8_t bt_map_path_draft[PATH_DRAFT_LEN] =
{
    0x00, 0x64, 0x00, 0x72, 0x00, 0x61, 0x00, 0x66, 0x00, 0x74, 0x00, 0x00
};

bool bt_map_set_folder(uint8_t         bd_addr[6],
                       T_BT_MAP_FOLDER folder)
{
    const uint8_t *path_ptr;
    uint16_t path_len;

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

    return map_set_folder(bd_addr, false, (uint8_t *)path_ptr, path_len);
}

void bt_map_cback(uint8_t    bd_addr[6],
                  T_MAP_MSG  msg_type,
                  void      *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    BTM_PRINT_INFO1("bt_map_cback: msg_type 0x%02x", msg_type);

    p_link = bt_find_br_link(bd_addr);
    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = msg_buf;

    switch (msg_type)
    {
    case MAP_MSG_MNS_CONN_IND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_MNS_CONN_IND, &payload);
        }
        else
        {
            map_mns_connect_cfm(bd_addr, false);
        }
        break;

    case MAP_MSG_MNS_CONNECTED:
        if (p_link != NULL)
        {
            p_link->map_data.mns_connected = true;
            bt_mgr_dispatch(BT_MSG_MAP_MNS_CONN_CMPL, &payload);
        }
        break;

    case MAP_MSG_MNS_CONN_FAIL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_MNS_CONN_FAIL, &payload);
        }
        break;

    case MAP_MSG_MNS_DISCONNECTED:
        if (p_link != NULL)
        {
            p_link->map_data.mns_connected = false;
            bt_mgr_dispatch(BT_MSG_MAP_MNS_DISCONN_CMPL, &payload);
        }
        break;

    case MAP_MSG_MAS_CONNECTED:
        if (p_link != NULL)
        {
            p_link->connected_profile |= MAP_PROFILE_MASK;
            bt_mgr_dispatch(BT_MSG_MAP_MAS_CONN_CMPL, &payload);

            //set folder to root/telecom/msg
            bt_map_set_folder(bd_addr, BT_MAP_FOLDER_ROOT);
            bt_map_set_folder(bd_addr, BT_MAP_FOLDER_TELECOM);
            bt_map_set_folder(bd_addr, BT_MAP_FOLDER_MSG);
        }
        else
        {
            map_disconn_mas(bd_addr);
        }
        break;

    case MAP_MSG_MAS_CONN_FAIL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_MAS_CONN_FAIL, &payload);
        }
        break;

    case MAP_MSG_MAS_DISCONNECTED:
        if (p_link != NULL)
        {
            p_link->connected_profile &= ~MAP_PROFILE_MASK;
            bt_mgr_dispatch(BT_MSG_MAP_MAS_DISCONN_CMPL, &payload);
        }
        break;

    case MAP_MSG_SET_FOLDER_CMPL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_SET_FOLDER_CMPL, &payload);
        }
        break;

    case MAP_MSG_REG_NOTIF_CMPL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_REG_NOTIF_CMPL, &payload);
        }
        break;

    case MAP_MSG_GET_FOLDER_LISTING_CMPL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_GET_FOLDER_LISTING_CMPL, &payload);
        }
        break;

    case MAP_MSG_GET_MSG_LISTING_CMPL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_GET_MSG_LISTING_CMPL, &payload);
        }
        break;

    case MAP_MSG_GET_MSG_CMPL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_GET_MSG_CMPL, &payload);
        }
        break;

    case MAP_MSG_PUSH_MSG_CMPL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_PUSH_MSG_CMPL, &payload);
        }
        break;

    case MAP_MSG_DATA_NOTIFICATION:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_MAP_MSG_NOTIFICATION, &payload);
        }
        break;

    default:
        break;
    }
}

bool bt_map_mns_connect_cfm(uint8_t bd_addr[6],
                            bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return map_mns_connect_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_map_mas_connect_over_rfc_req(uint8_t bd_addr[6],
                                     uint8_t server_chann)
{
    T_BT_BR_LINK *p_link;
    p_link = bt_find_br_link(bd_addr);

    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & MAP_PROFILE_MASK) == 0)
        {
            return map_conn_mas_over_rfc(bd_addr, server_chann, true);
        }
    }

    return false;
}

bool bt_map_mas_connect_over_l2c_req(uint8_t  bd_addr[6],
                                     uint16_t l2c_psm)
{
    T_BT_BR_LINK *p_link;
    p_link = bt_find_br_link(bd_addr);

    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & MAP_PROFILE_MASK) == 0)
        {
            return map_conn_mas_over_l2c(bd_addr, l2c_psm, true);
        }
    }

    return false;
}

bool bt_map_mas_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return map_disconn_mas(bd_addr);
    }

    return false;
}

bool bt_map_mns_send_event_rsp(uint8_t                bd_addr[6],
                               T_BT_MAP_RESPONSE_CODE rsp_code)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & MAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return map_send_event_rsp(bd_addr, rsp_code);
        }
    }

    return false;
}

bool bt_map_mas_msg_notification_set(uint8_t bd_addr[6],
                                     bool    enable)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & MAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return map_register_msg_notification(bd_addr, enable);
        }
    }

    return false;
}

bool bt_map_mas_folder_set(uint8_t         bd_addr[6],
                           T_BT_MAP_FOLDER folder)
{
    T_BT_BR_LINK *p_link;
    bool ret = true;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & MAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);

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
        }
    }

    return ret;
}

bool bt_map_mas_folder_listing_get(uint8_t  bd_addr[6],
                                   uint16_t max_list_count,
                                   uint16_t start_offset)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & MAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return map_get_folder_listing(bd_addr, max_list_count, start_offset);
        }
    }

    return false;
}

bool bt_map_mas_msg_listing_get(uint8_t   bd_addr[6],
                                uint8_t  *folder,
                                uint16_t  folder_len,
                                uint16_t  max_list_count,
                                uint16_t  start_offset)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & MAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return map_get_msg_listing(bd_addr, folder, folder_len, max_list_count, start_offset);
        }
    }

    return false;
}

bool bt_map_mas_msg_get(uint8_t  bd_addr[6],
                        uint8_t *msg_handle,
                        uint8_t  handle_len,
                        bool     native)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & MAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return map_get_msg(bd_addr, msg_handle, handle_len, native);
        }
    }

    return false;
}

bool bt_map_mas_get_continue(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & MAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return map_get_continue(bd_addr);
        }
    }

    return false;
}

bool bt_map_mas_get_abort(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & MAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return map_get_abort(bd_addr);
        }
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
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & MAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return map_push_msg(bd_addr, folder, folder_len, native, more_data, msg, msg_len);
        }
    }

    return false;
}

bool bt_map_init(uint8_t  link_num,
                 uint8_t  mns_server_chann,
                 uint16_t mns_l2c_psm,
                 uint32_t support_feat)
{
    return map_init(link_num, bt_map_cback, mns_server_chann, mns_l2c_psm, support_feat);
}
#else
#include <stdint.h>
#include <stdbool.h>
#include "bt_map.h"

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

bool bt_map_init(uint8_t  link_num,
                 uint8_t  mns_server_chann,
                 uint16_t mns_l2c_psm,
                 uint32_t support_feat)
{
    return false;
}
#endif
