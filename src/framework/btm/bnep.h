/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BNEP_H_
#define _BNEP_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_bnep_msg_type
{
    BNEP_MSG_CONN_IND                  = 0x00,
    BNEP_MSG_CONN_RSP                  = 0x01,
    BNEP_MSG_CONN_CMPL                 = 0x02,
    BNEP_MSG_CONN_FAIL                 = 0x03,
    BNEP_MSG_DISCONN_CMPL              = 0x04,
    BNEP_MSG_SETUP_CONN_IND            = 0x05,
    BNEP_MSG_SETUP_CONN_RSP            = 0x06,
    BNEP_MSG_FILTER_NET_TYPE_SET_IND   = 0x07,
    BNEP_MSG_FILTER_NET_TYPE_SET_RSP   = 0x08,
    BNEP_MSG_FILTER_MULTI_ADDR_SET_IND = 0x09,
    BNEP_MSG_FILTER_MULTI_ADDR_SET_RSP = 0x0a,
    BNEP_MSG_ETHERNET_PACKET_IND       = 0x0b
} T_BNEP_MSG_TYPE;

typedef struct t_bnep_setup_conn_ind
{
    uint16_t   dst_uuid;
    uint16_t   src_uuid;
} T_BNEP_SETUP_CONN_IND;

typedef struct t_bnep_setup_conn_rsp
{
    uint16_t   rsp_msg;
} T_BNEP_SETUP_CONN_RSP;

typedef struct t_bnep_filter_net_type_set_ind
{
    uint16_t  filter_num;
    uint16_t *filter_start;
    uint16_t *filter_end;
} T_BNEP_FILTER_NET_TYPE_SET_IND;

typedef struct t_bnep_filter_net_type_set_rsp
{
    uint16_t  rsp_msg;
} T_BNEP_FILTER_NET_TYPE_SET_RSP;

typedef struct t_bnep_filter_multi_addr_set_ind
{
    uint16_t  filter_num;
    uint8_t (*filter_start)[6];
    uint8_t (*filter_end)[6];
} T_BNEP_FILTER_MULTI_ADDR_SET_IND;

typedef struct t_bnep_filter_multi_addr_set_rsp
{
    uint16_t  rsp_msg;
} T_BNEP_FILTER_MULTI_ADDR_SET_RSP;

typedef struct t_bnep_ethernet_packet_ind
{
    uint8_t  *buf;
    uint16_t  len;
} T_BNEP_ETHERNET_PACKET_IND;

typedef void (*P_BNEP_CBACK)(uint16_t         cid,
                             T_BNEP_MSG_TYPE  msg_type,
                             void            *msg);

bool bnep_init(P_BNEP_CBACK cback);

void bnep_deinit(void);

bool bnep_connect_req(uint8_t  local_addr[6],
                      uint8_t  remote_addr[6],
                      uint16_t src_uuid,
                      uint16_t dst_uuid);

bool bnep_disconnect_req(uint8_t bd_addr[6]);

bool bnep_connect_cfm(uint8_t local_addr[6],
                      uint8_t remote_addr[6],
                      bool    accept);

bool bnep_control_command_not_understood_send(uint8_t bd_addr[6],
                                              uint8_t control_type);

bool bnep_setup_connection_req(uint8_t  bd_addr[6],
                               uint16_t src_uuid,
                               uint16_t dst_uuid);

bool bnep_setup_connection_rsp(uint8_t  bd_addr[6],
                               uint16_t rsp_msg);

bool bnep_filter_net_type_set(uint8_t   bd_addr[6],
                              uint16_t  filter_num,
                              uint16_t *start_array,
                              uint16_t *end_array);

bool bnep_filter_net_type_rsp(uint8_t  bd_addr[6],
                              uint16_t rsp_msg);

bool bnep_filter_multi_addr_set(uint8_t  bd_addr[6],
                                uint16_t filter_num,
                                uint8_t  start_array[][6],
                                uint8_t  end_array[][6]);

bool bnep_filter_multi_addr_rsp(uint8_t  bd_addr[6],
                                uint16_t rsp_msg);

bool bnep_send(uint8_t   bd_addr[6],
               uint8_t  *buf,
               uint16_t  len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BNEP_H_ */
