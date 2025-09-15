/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_MAP_INT_H_
#define _BT_MAP_INT_H_

#include <stdint.h>
#include <stdbool.h>
#include "bt_map.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_bt_map_folder_listing_data_ind
{
    uint8_t  *buf;
    uint16_t  len;
    uint16_t  folder_listing_size;
    bool      data_end;
} T_BT_MAP_FOLDER_LISTING_DATA_IND;

typedef struct t_bt_map_msg_listing_data_ind
{
    uint8_t  *buf;
    uint16_t  len;
    uint16_t  msg_listing_size;
    uint8_t   new_msg;
    bool      data_end;
} T_BT_MAP_MSG_LISTING_DATA_IND;

typedef struct t_bt_map_msg_data_ind
{
    uint8_t  *buf;
    uint16_t  len;
    bool      data_end;
} T_BT_MAP_MSG_DATA_IND;

typedef struct t_bt_map_push_msg_cmpl
{
    T_BT_MAP_PUSH_MSG_ACTION  action;
    uint8_t                   rsp_code;
    uint8_t                  *msg_handle;
    uint16_t                  msg_handle_len;
} T_BT_MAP_PUSH_MSG_CMPL;

typedef struct t_bt_map_msg_report_notif
{
    uint8_t  *buf;
    uint16_t  len;
    bool      data_end;
} T_BT_MAP_MSG_REPORT_NOTIF;

typedef struct t_roleswap_map_info
{
    uint32_t mas_obex_conn_id;
    uint16_t mas_l2c_cid;
    uint16_t mas_psm;
    uint16_t mas_local_max_pkt_len;
    uint16_t mas_remote_max_pkt_len;
    uint8_t  mas_obex_state;
    uint8_t  mas_rfc_dlci;

    uint8_t  mas_state;
    uint8_t  mns_state;
    bool     feat_flag;

    uint32_t mns_obex_conn_id;
    uint16_t mns_l2c_cid;
    uint16_t mns_psm;
    uint16_t mns_local_max_pkt_len;
    uint16_t mns_remote_max_pkt_len;
    uint8_t  mns_obex_state;
    uint8_t  mns_rfc_dlci;

    uint16_t data_offset;
} T_ROLESWAP_MAP_INFO;

bool bt_map_get_roleswap_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_MAP_INFO *p_info);

bool bt_map_set_roleswap_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_MAP_INFO *p_info);

uint8_t bt_map_service_id_get(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_MAP_INT_H_ */
