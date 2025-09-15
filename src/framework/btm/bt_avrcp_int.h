/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_AVRCP_INT_H_
#define _BT_AVRCP_INT_H_

#include <stdint.h>
#include <stdbool.h>
#include "bt_avrcp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
{
    uint8_t  state;
    uint32_t length_ms;
    uint32_t position_ms;
    uint8_t  play_status;
} T_BT_AVRCP_RSP_GET_PLAY_STATUS;

typedef struct t_bt_avrcp_req_get_element_attr
{
    uint8_t    attr_num;
    uint32_t   attr_id[8];
} T_BT_AVRCP_REQ_GET_ELEMENT_ATTR;

typedef struct
{
    uint8_t                  state;
    uint8_t                  num_of_attr;
    T_BT_AVRCP_ELEMENT_ATTR *attr;
} T_BT_AVRCP_RSP_GET_ELEMENT_ATTR;

typedef struct t_bt_avrcp_rsp_get_capabilities
{
    uint8_t  state;
    uint8_t  capability_count;
    uint8_t *capabilities;
} T_BT_AVRCP_RSP_GET_CAPABILITIES;

typedef T_BT_AVRCP_RSP_GET_ELEMENT_ATTR T_BT_AVRCP_RSP_GET_ITEM_ATTR;

typedef struct
{
    uint16_t player_id;
    uint16_t uid_counter;
} T_BT_AVRCP_RSP_ADDRESSED_PLAYER_CHANGED;

typedef struct
{
    uint8_t  state;
    uint8_t  num_of_attr;
    uint8_t *p_attr_id;
} T_BT_AVRCP_RSP_LIST_APP_SETTING_ATTRS;

typedef struct
{
    uint8_t  state;
    uint8_t  num_of_value;
    uint8_t *p_value;
} T_BT_AVRCP_RSP_LIST_APP_SETTING_VALUES;

typedef struct
{
    uint8_t                  state;
    uint8_t                  num_of_attr;
    T_BT_AVRCP_APP_SETTING  *p_app_setting;
} T_BT_AVRCP_RSP_GET_APP_SETTING_VALUE;

typedef struct t_bt_avrcp_req_get_folder_items
{
    uint8_t   scope;
    uint32_t  start_item;
    uint32_t  end_item;
    uint8_t   attr_count;
    uint32_t  attr_id[8];
} T_BT_AVRCP_REQ_GET_FOLDER_ITEMS;

typedef struct
{
    uint8_t   status_code;
    uint16_t  uid_counter;
    uint16_t  num_of_items;
    uint8_t   item_type;
    union
    {
        T_BT_AVRCP_MEDIA_PLAYER_ITEM  *p_media_player_items;
        T_BT_AVRCP_FOLDER_ITEM        *p_folder_items;
        T_BT_AVRCP_MEDIA_ELEMENT_ITEM *p_media_element_items;
    } u;
} T_BT_AVRCP_RSP_GET_FOLDER_ITEMS;

typedef struct
{
    uint8_t   status_code;
    uint16_t  uid_counter;
    uint32_t  num_of_items;
} T_BT_AVRCP_RSP_SEARCH;

typedef struct
{
    uint8_t             status_code;
    uint16_t            uid_counter;
    uint32_t            num_of_items;
    uint16_t            character_set_id;
    uint8_t             folder_depth;
    T_BT_AVRCP_FOLDER  *p_folders;
} T_BT_AVRCP_RSP_SET_BROWSED_PLAYER;

typedef struct
{
    uint8_t   status_code;
    uint32_t  num_of_items;
} T_BT_AVRCP_RSP_CHANGE_PATH;

typedef struct t_bt_avrcp_rsp_set_abs_vol
{
    uint8_t  state;
    uint8_t  volume;
} T_BT_AVRCP_RSP_SET_ABS_VOL;

typedef struct
{
    uint8_t                  num_of_attr;
    T_BT_AVRCP_APP_SETTING  *p_app_setting;
} T_BT_AVRCP_RSP_APP_SETTING_CHANGED;

typedef struct
{
    uint8_t   *p_data;
    uint16_t   data_len;
    bool       data_end;
} T_BT_AVRCP_COVER_ART_DATA_IND;

typedef struct
{
    uint8_t   ctype;
    uint32_t  company_id;
    uint8_t  *p_cmd;
    uint16_t  cmd_len;
} T_BT_AVRCP_VENDOR_CMD;

typedef struct
{
    uint8_t   response;
    uint32_t  company_id;
    uint8_t  *p_rsp;
    uint16_t  rsp_len;
} T_BT_AVRCP_VENDOR_RSP;

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
    uint8_t     vol_change_pending_transact;
} T_ROLESWAP_AVRCP_TRANSACT;

bool bt_avrcp_roleswap_info_get(uint8_t                bd_addr[6],
                                T_ROLESWAP_AVRCP_INFO *info);

bool bt_avrcp_roleswap_info_set(uint8_t                bd_addr[6],
                                T_ROLESWAP_AVRCP_INFO *info);

bool bt_avrcp_roleswap_info_del(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_AVRCP_INT_H_ */
