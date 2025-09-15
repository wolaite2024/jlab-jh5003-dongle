/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_PBAP_INT_H_
#define _BT_PBAP_INT_H_

#include <stdint.h>
#include "bt_pbap.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_bt_pbap_disconn_info
{
    uint16_t cause;
} T_BT_PBAP_DISCONN_INFO;

typedef struct t_bt_pbap_get_phone_book_msg_data
{
    uint8_t  *data;
    uint16_t  data_len;
    uint16_t  pb_size;
    uint8_t   new_missed_calls;
    bool      data_end;
} T_BT_PBAP_GET_PHONE_BOOK_MSG_DATA;

typedef struct t_bt_pbap_caller_id_name
{
    uint8_t  *name_ptr;
    uint16_t  name_len;
} T_BT_PBAP_CALLER_ID_NAME;

typedef struct t_bt_pbap_set_phone_book_cmpl
{
    bool           result;
    T_BT_PBAP_PATH path;
} T_BT_PBAP_SET_PHONE_BOOK_CMPL;

typedef struct t_bt_pbap_get_vcard_entry_msg_data
{
    uint8_t  *data;
    uint16_t  data_len;
    bool      data_end;
} T_BT_PBAP_GET_VCARD_ENTRY_MSG_DATA;

typedef struct t_roleswap_pbap_info
{
    uint8_t              bd_addr[6];
    uint16_t             l2c_cid;
    uint32_t             obex_conn_id;
    uint16_t             obex_psm;
    uint16_t             local_max_pkt_len;
    uint16_t             remote_max_pkt_len;
    uint8_t              obex_state;
    uint8_t              rfc_dlci;
    uint8_t              pbap_state;
    bool                 feat_flag;
    uint8_t              data_offset;
    uint8_t              path;
    T_BT_PBAP_REPOSITORY repos;
} T_ROLESWAP_PBAP_INFO;

typedef struct t_roleswap_pbap_transact
{
    uint8_t              bd_addr[6];
    uint8_t              obex_state;
    uint8_t              pbap_state;
    uint8_t              path;
    T_BT_PBAP_REPOSITORY repos;
} T_ROLESWAP_PBAP_TRANSACT;

bool bt_pbap_roleswap_info_get(uint8_t               bd_addr[6],
                               T_ROLESWAP_PBAP_INFO *info);

bool bt_pbap_roleswap_info_set(uint8_t               bd_addr[6],
                               T_ROLESWAP_PBAP_INFO *info);

bool bt_pbap_roleswap_info_del(uint8_t bd_addr[6]);

bool bt_pbap_service_id_get(uint8_t *service_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_PBAP_INT_H_ */
