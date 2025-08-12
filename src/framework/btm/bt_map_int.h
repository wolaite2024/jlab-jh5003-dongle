/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_MAP_INT_H_
#define _BT_MAP_INT_H_

#include <stdint.h>
#include "bt_map.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
{
    bool mns_connected;
} T_MAP_LINK_DATA;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    uint16_t    folder_listing_size;
    bool        data_end;
} T_BT_MAP_FOLDER_LISTING_DATA_IND;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    uint16_t    msg_listing_size;
    uint8_t     new_msg;
    bool        data_end;
} T_BT_MAP_MSG_LISTING_DATA_IND;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    bool        data_end;
} T_BT_MAP_MSG_DATA_IND;

typedef struct
{
    T_BT_MAP_PUSH_MSG_ACTION  action;
    uint8_t                   rsp_code;
    uint8_t                  *p_msg_handle;
    uint16_t                  msg_handle_len;
} T_BT_MAP_PUSH_MSG_CMPL;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    bool        data_end;
} T_BT_MAP_MSG_REPORT_NOTIF;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_MAP_INT_H_ */
