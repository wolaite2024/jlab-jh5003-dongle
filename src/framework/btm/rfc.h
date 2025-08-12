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

typedef enum
{
    RFC_CONN_IND        = 0x00,      /**< rfcomm connect indicate */
    RFC_CONN_CMPL       = 0x01,      /**< rfcomm connect successful */
    RFC_AUTHOR_IND      = 0x02,      /**< rfcomm based profile authorization indicate */
    RFC_DISCONN_CMPL    = 0x03,      /**< rfcomm connect successful */
    RFC_DATA_IND        = 0x04,      /**< rfcomm data indicate */
    RFC_DATA_RSP        = 0x05,      /**< rfcomm data repsone with ack flag set */
    RFC_CREDIT_INFO     = 0x06,      /**< rfcomm remote credits infomation */
    RFC_SEC_REG_RSP     = 0x07,
    RFC_DLCI_CHANGE     = 0x08
} T_RFC_MSG_TYPE;

typedef struct
{
    uint16_t    frame_size;
    uint8_t     dlci;
    uint8_t     bd_addr[6];
} T_RFC_CONN_IND;

typedef struct
{
    uint16_t    frame_size;
    uint8_t     remain_credits;
    uint8_t     dlci;
    uint8_t     bd_addr[6];
    uint8_t     profile_index;
} T_RFC_CONN_CMPL;

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
    uint16_t    cause;
} T_RFC_DISCONN_CMPL;

typedef struct
{
    uint8_t    *buf;
    uint16_t    length;
    uint8_t     remain_credits;
    uint8_t     dlci;
    uint8_t     bd_addr[6];
} T_RFC_DATA_IND;

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
} T_RFC_DATA_RSP;

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
    uint8_t     remain_credits;
} T_RFC_CREDIT_INFO;

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     pre_dlci;
    uint8_t     curr_dlci;
} T_RFC_DLCI_CHANGE_INFO;

typedef struct
{
    uint16_t    server_chann;
    uint16_t    uuid;
    uint16_t    cause;
    uint8_t     active;
} T_RFC_SEC_REG_RSP;

typedef void (* P_RFC_PROFILE_CB)(T_RFC_MSG_TYPE  msg_type,
                                  void           *p_msg);

/** @brief  protocol description */
typedef struct
{
    P_RFC_PROFILE_CB    cb;                 /**< point to callback function */
    uint8_t             server_chann;       /**< protocol service multiplexer */
} T_RFC_PROFILE;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    l2c_cid;
    uint16_t    mtu_size;
    bool        link_initiator;
    bool        initiator;
    uint8_t     data_offset;
} T_ROLESWAP_RFC_CTRL_INFO;

typedef struct
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

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     dlci;
    uint8_t     remote_remain_credits;
    uint8_t     given_credits;
} T_ROLESWAP_RFC_TRANSACT;

/*APIs provided to upstream*/
bool rfc_reg_cb(uint8_t           chann_num,
                P_RFC_PROFILE_CB  callback,
                uint8_t          *p_idx);

bool rfc_reg_sec(uint8_t  active,
                 uint16_t server_chann,
                 uint8_t  profile_idx,
                 uint8_t  sec);

bool rfc_set_ertm_mode(bool enable);

bool rfc_flow_ctrl_req(uint8_t bd_addr[6],
                       uint8_t dlci,
                       uint8_t flow_status,
                       uint8_t sbreak);

bool rfc_conn_req(uint8_t   bd_addr[6],
                  uint8_t   server_chann,
                  uint16_t  frame_size,
                  uint8_t   max_credits,
                  uint8_t   profile_index,
                  uint8_t  *p_dlci);

bool rfc_disconn_req(uint8_t bd_addr[6],
                     uint8_t dlci);

bool rfc_data_req(uint8_t   bd_addr[6],
                  uint8_t   dlci,
                  uint8_t  *p_buffer,
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
                 uint16_t *p_cid);

bool rfc_get_roleswap_info(uint8_t                   bd_addr[6],
                           uint8_t                   dlci,
                           T_ROLESWAP_RFC_DATA_INFO *p_data,
                           T_ROLESWAP_RFC_CTRL_INFO *p_ctrl);

bool rfc_set_ctrl_roleswap_info(uint8_t                   bd_addr[6],
                                T_ROLESWAP_RFC_CTRL_INFO *p_info);

bool rfc_set_data_roleswap_info(uint8_t                   bd_addr[6],
                                uint8_t                   profile_idx,
                                T_ROLESWAP_RFC_DATA_INFO *p_info);

bool rfc_del_ctrl_roleswap_info(uint8_t bd_addr[6]);

bool rfc_del_data_roleswap_info(uint8_t  dlci,
                                uint16_t cid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RFC_H_ */
