/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _RFC_H_
#define _RFC_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define RFC_DEFAULT_MTU                 330     /**<Max Packet Size */
#define RFC_DEFAULT_CREDIT              7       /**<Credits for flow control*/

#define RFC_ACCEPT                      0
#define RFC_REJECT                      1

typedef enum t_rfc_msg_type
{
    RFC_CONN_IND        = 0x00,      /**< rfcomm connect indicate */
    RFC_CONN_CMPL       = 0x01,      /**< rfcomm connect successful */
    RFC_AUTHOR_IND      = 0x02,      /**< rfcomm based service authorization indicate */
    RFC_DISCONN_CMPL    = 0x03,      /**< rfcomm connect successful */
    RFC_DATA_IND        = 0x04,      /**< rfcomm data indicate */
    RFC_DATA_RSP        = 0x05,      /**< rfcomm data repsone with ack flag set */
    RFC_CREDIT_INFO     = 0x06,      /**< rfcomm remote credits infomation */
    RFC_SEC_REG_RSP     = 0x07,
    RFC_DLCI_CHANGE     = 0x08
} T_RFC_MSG_TYPE;

typedef struct t_rfc_conn_ind
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
    uint16_t    frame_size;
} T_RFC_CONN_IND;

typedef struct t_rfc_conn_cmpl
{
    uint8_t  bd_addr[6];
    uint8_t  dlci;
    uint8_t  service_id;
    uint16_t frame_size;
    uint8_t  remain_credits;
} T_RFC_CONN_CMPL;

typedef struct t_rfc_disc_cmpl
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
    uint16_t    cause;
} T_RFC_DISCONN_CMPL;

typedef struct t_rfc_data_ind
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
    uint8_t     remain_credits;
    uint16_t    length;
    uint8_t    *buf;
} T_RFC_DATA_IND;

typedef struct t_rfc_data_rsp
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
} T_RFC_DATA_RSP;

typedef struct t_rfc_credit_info
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
    uint8_t     remain_credits;
} T_RFC_CREDIT_INFO;

typedef struct t_rfc_dlci_change_info
{
    uint8_t     bd_addr[6];
    uint8_t     prev_dlci;
    uint8_t     curr_dlci;
} T_RFC_DLCI_CHANGE_INFO;

typedef struct t_rfc_sec_reg_rsp
{
    uint8_t  server_chann;
    uint8_t  active;
    uint16_t cause;
} T_RFC_SEC_REG_RSP;

typedef void (*P_RFC_SERVICE_CB)(T_RFC_MSG_TYPE  msg_type,
                                 void          *msg);

typedef struct t_roleswap_rfc_ctrl_info
{
    uint8_t     bd_addr[6];
    uint16_t    l2c_cid;
    uint16_t    mtu_size;
    bool        link_initiator;
    bool        initiator;
    uint8_t     data_offset;
} T_ROLESWAP_RFC_CTRL_INFO;

typedef struct t_roleswap_rfc_data_info
{
    uint8_t     bd_addr[6];
    uint16_t    uuid;
    uint16_t    frame_size;
    uint16_t    l2c_cid;
    uint8_t     dlci;
    uint8_t     remote_remain_credits;
    uint8_t     given_credits;
    uint8_t     init_credits;
} T_ROLESWAP_RFC_DATA_INFO;

typedef struct t_roleswap_rfc_transact
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
    uint8_t     remote_remain_credits;
    uint8_t     given_credits;
} T_ROLESWAP_RFC_TRANSACT;

bool rfc_cback_register(uint8_t           server_chann,
                        P_RFC_SERVICE_CB  cback,
                        uint8_t          *service_id);

bool rfc_reg_sec(uint8_t service_id,
                 uint8_t server_chann,
                 uint8_t active,
                 uint8_t sec);

bool rfc_set_ertm_mode(bool enable);

bool rfc_flow_ctrl_req(uint8_t bd_addr[6],
                       uint8_t dlci,
                       uint8_t flow_status,
                       uint8_t sbreak);

bool rfc_conn_req(uint8_t   bd_addr[6],
                  uint8_t   server_chann,
                  uint8_t   service_id,
                  uint16_t  frame_size,
                  uint8_t   max_credits,
                  uint8_t  *dlci);

bool rfc_disconn_req(uint8_t bd_addr[6],
                     uint8_t dlci);

bool rfc_data_req(uint8_t   bd_addr[6],
                  uint8_t   dlci,
                  uint8_t  *buffer,
                  uint16_t  len,
                  bool      ack);

bool rfc_data_cfm(uint8_t bd_addr[6],
                  uint8_t dlci,
                  uint8_t rsp_num);

void rfc_conn_cfm(uint8_t  bd_addr[6],
                  uint8_t  dlci,
                  uint16_t status,
                  uint16_t frame_size,
                  uint8_t  max_credits);

bool rfc_get_cid(uint8_t   bd_addr[6],
                 uint8_t   dlci,
                 uint16_t *cid);

bool rfc_ctrl_roleswap_info_get(uint8_t                   bd_addr[6],
                                uint16_t                  cid,
                                T_ROLESWAP_RFC_CTRL_INFO *ctrl);

bool rfc_data_roleswap_info_get(uint8_t                   bd_addr[6],
                                uint8_t                   dlci,
                                T_ROLESWAP_RFC_DATA_INFO *data);

bool rfc_ctrl_roleswap_info_set(uint8_t                   bd_addr[6],
                                T_ROLESWAP_RFC_CTRL_INFO *info);

bool rfc_data_roleswap_info_set(uint8_t                   bd_addr[6],
                                uint8_t                   service_id,
                                T_ROLESWAP_RFC_DATA_INFO *info);

bool rfc_ctrl_roleswap_info_del(uint8_t bd_addr[6]);

bool rfc_data_roleswap_info_del(uint8_t  dlci,
                                uint16_t cid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RFC_H_ */
