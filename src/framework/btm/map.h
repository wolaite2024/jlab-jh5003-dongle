/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _MAP_H_
#define _MAP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    MAP_PUSH_MSG_COMPLETE           = 0x00,
    MAP_PUSH_MSG_CONTINUE           = 0x01,
    MAP_PUSH_MSG_AGAIN              = 0x02,
    MAP_PUSH_MSG_TO_END             = 0x03,
} T_MAP_PUSH_MSG_ACTION;

typedef struct
{
    uint16_t    cause;
} T_MAP_DISCONN_INFO;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    bool        data_end;
} T_MAP_NOTIF_MSG_DATA;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    uint16_t    folder_listing_size;
    bool        data_end;
} T_MAP_GET_FOLDER_LISTING_MSG_DATA;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    uint16_t    msg_listing_size;
    uint8_t     new_msg;
    bool        data_end;
} T_MAP_GET_MSG_LISTING_MSG_DATA;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    bool        data_end;
} T_MAP_GET_MSG_DATA;

typedef struct
{
    T_MAP_PUSH_MSG_ACTION action;
    uint8_t               rsp_code;
    uint8_t              *p_msg_handle;
    uint16_t              msg_handle_len;
} T_MAP_PUSH_MSG_CMPL;

typedef enum
{
    MAP_MSG_MNS_CONN_IND            = 0x00,
    MAP_MSG_MNS_CONNECTED           = 0x01,
    MAP_MSG_MNS_CONN_FAIL           = 0x02,
    MAP_MSG_MNS_DISCONNECTED        = 0x03,

    MAP_MSG_MAS_CONNECTED           = 0x04,
    MAP_MSG_MAS_CONN_FAIL           = 0x05,
    MAP_MSG_MAS_DISCONNECTED        = 0x06,

    MAP_MSG_SET_FOLDER_CMPL         = 0x10,
    MAP_MSG_REG_NOTIF_CMPL          = 0x11,
    MAP_MSG_GET_FOLDER_LISTING_CMPL = 0x12,
    MAP_MSG_GET_MSG_LISTING_CMPL    = 0x13,
    MAP_MSG_GET_MSG_CMPL            = 0x14,
    MAP_MSG_PUSH_MSG_CMPL           = 0x15,

    MAP_MSG_DATA_NOTIFICATION       = 0x20,
} T_MAP_MSG;

typedef struct
{
    bool        back;
    uint8_t    *p_folder;
    uint16_t    folder_len;
} T_MAP_REQ_SET_FOLDER;

typedef struct
{
    uint16_t   max_list_count;
    uint16_t   start_offset;
} T_MAP_REQ_GET_FOLDER_LISTING;

typedef struct
{
    uint8_t    *p_folder;
    uint16_t    folder_len;
    uint16_t    max_list_count;
    uint16_t    start_offset;
} T_MAP_REQ_GET_MSG_LISTING;

typedef struct
{
    uint8_t   *msg_handle;
    uint8_t    handle_len;
    bool       native;
} T_MAP_REQ_GET_MSG;

typedef struct
{
    uint32_t        mas_obex_conn_id;
    uint16_t        mas_l2c_cid;
    uint16_t        mas_psm;
    uint16_t        mas_local_max_pkt_len;
    uint16_t        mas_remote_max_pkt_len;
    uint8_t         mas_obex_state;
    uint8_t         mas_rfc_dlci;

    uint8_t         map_state;
    bool            feat_flag;

    uint32_t        mns_obex_conn_id;
    uint16_t        mns_l2c_cid;
    uint16_t        mns_psm;
    uint16_t        mns_local_max_pkt_len;
    uint16_t        mns_remote_max_pkt_len;
    uint8_t         mns_obex_state;
    uint8_t         mns_rfc_dlci;

    uint16_t        data_offset;
} T_ROLESWAP_MAP_INFO;

typedef void(*P_MAP_CBACK)(uint8_t    bd_addr[6],
                           T_MAP_MSG  msg_type,
                           void      *msg_buf);

bool map_init(uint8_t     link_num,
              P_MAP_CBACK cback,
              uint8_t     mns_server_chann,
              uint16_t    mns_l2c_psm,
              uint32_t    support_feat);

bool map_mns_connect_cfm(uint8_t bd_addr[6],
                         bool    accept);

bool map_conn_mas_over_rfc(uint8_t bd_addr[6],
                           uint8_t server_chann,
                           bool    feat_flag);

bool map_conn_mas_over_l2c(uint8_t  bd_addr[6],
                           uint16_t mas_l2c_psm,
                           bool     feat_flag);

bool map_send_event_rsp(uint8_t bd_addr[6],
                        uint8_t rsp_code);

bool map_set_folder(uint8_t   bd_addr[6],
                    bool      back,
                    uint8_t  *folder,
                    uint16_t  folder_len);

bool map_register_msg_notification(uint8_t bd_addr[6],
                                   bool    enable);

bool map_get_folder_listing(uint8_t  bd_addr[6],
                            uint16_t max_list_cnt,
                            uint16_t start_offset);

bool map_get_msg_listing(uint8_t   bd_addr[6],
                         uint8_t  *folder,
                         uint16_t  folder_len,
                         uint16_t  max_list_count,
                         uint16_t  start_offset);

bool map_get_msg(uint8_t  bd_addr[6],
                 uint8_t *msg_handle,
                 uint8_t  handle_len,
                 bool     native);

bool map_get_continue(uint8_t bd_addr[6]);

bool map_get_abort(uint8_t bd_addr[6]);

bool map_push_msg(uint8_t   bd_addr[6],
                  uint8_t  *folder,
                  uint16_t  folder_len,
                  bool      native,
                  bool      more_data,
                  uint8_t  *msg,
                  uint16_t  msg_len);

bool map_disconn_mas(uint8_t bd_addr[6]);

bool map_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_MAP_INFO *p_info);

bool map_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_MAP_INFO *p_info);

uint8_t map_get_rfc_profile_idx(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MAP_H_ */
