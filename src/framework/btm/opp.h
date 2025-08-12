/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _OPP_H_
#define _OPP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_opp_notif_msg_data
{
    uint8_t    *p_data;
    uint16_t    data_len;
    bool        data_end;
} T_OPP_NOTIF_MSG_DATA;

typedef struct t_opp_notif_msg_data_headers
{
    uint8_t    *name;
    uint16_t    name_len;
    uint8_t    *type;
    uint16_t    type_len;
    uint32_t    total_len;
} T_OPP_NOTIF_MSG_DATA_HEADERS;

typedef enum t_opp_msg
{
    OPP_MSG_CONN_IND                    = 0x00,
    OPP_MSG_CONNECTED                   = 0x01,
    OPP_MSG_CONN_FAIL                   = 0x02,
    OPP_MSG_DISCONNECTED                = 0x03,
    OPP_MSG_DATA_HEADER_IND             = 0x04,
    OPP_MSG_DATA_IND                    = 0x05,
    OPP_MSG_DATA_RSP                    = 0x06,
} T_OPP_MSG;

typedef void(*P_OPP_CBACK)(uint8_t    bd_addr[6],
                           T_OPP_MSG  msg_type,
                           void      *msg_buf);

bool opp_init(uint8_t     link_num,
              P_OPP_CBACK cback,
              uint8_t     server_chann,
              uint16_t    l2c_psm);

bool opp_conn_over_l2c(uint8_t  bd_addr[6],
                       uint16_t l2c_psm);

bool opp_conn_over_rfc(uint8_t bd_addr[6],
                       uint8_t server_chann);

bool opp_connect_cfm(uint8_t bd_addr[6],
                     bool    accept);

bool opp_disconnect_req(uint8_t bd_addr[6]);

bool opp_push_data_header_req(uint8_t   bd_addr[6],
                              uint32_t  total_len,
                              uint8_t  *name,
                              uint16_t  name_len,
                              uint8_t  *type,
                              uint16_t  type_len,
                              bool      srm_enable);

bool opp_push_data_req(uint8_t   bd_addr[6],
                       uint8_t  *data,
                       uint16_t  data_len,
                       bool      more_data);

bool opp_send_rsp(uint8_t bd_addr[6],
                  uint8_t rsp_code);

bool opp_push_abort(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _OPP_H_ */
