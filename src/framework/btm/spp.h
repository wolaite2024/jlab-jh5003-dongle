/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _SPP_H_
#define _SPP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    SPP_MSG_CONN_IND     = 0x00,
    SPP_MSG_CONN_CMPL    = 0x01,
    SPP_MSG_CONN_FAIL    = 0x02,
    SPP_MSG_DISCONN_CMPL = 0x03,
    SPP_MSG_DATA_IND     = 0x04,
    SPP_MSG_DATA_RSP     = 0x05,
    SPP_MSG_CREDIT_INFO  = 0x06,
} T_SPP_MSG;

typedef struct
{
    uint8_t     local_server_chann;
    uint16_t    frame_size;
} T_SPP_CONN_IND;

typedef struct
{
    uint8_t     local_server_chann;
    uint8_t     credit;
    uint16_t    frame_size;
} T_SPP_CONN_CMPL;

typedef struct
{
    uint8_t     local_server_chann;
    uint8_t    *p_data;
    uint16_t    len;
} T_SPP_DATA_IND;

typedef struct
{
    uint8_t     local_server_chann;
} T_SPP_DATA_RSP;

typedef struct
{
    uint8_t     local_server_chann;
    uint8_t     credit;
} T_SPP_CREDIT_INFO;

typedef struct
{
    uint8_t     local_server_chann;
    uint16_t    cause;
} T_SPP_DISCONN_CMPL;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    l2c_cid;
    uint16_t    frame_size;
    uint8_t     rfc_dlci;
    uint8_t     role;
    uint8_t     local_server_chann;     //used to distinguish which spp is used
    uint8_t     state;
    uint8_t     remote_credit;
} T_ROLESWAP_SPP_INFO;

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     local_server_chann;
    uint8_t     state;
    uint8_t     remote_credit;
} T_ROLESWAP_SPP_TRANSACT;

// local server chann is used for upper layer to distinguish different spp service if using only one callback
typedef void(*P_SPP_CBACK)(uint8_t    bd_addr[6],
                           T_SPP_MSG  msg_type,
                           void      *msg_buf);

bool spp_init(uint8_t link_num,
              uint8_t service_num);

bool spp_reg_cb(uint8_t     server_chann,
                P_SPP_CBACK callback);

bool spp_connect_req(uint8_t  bd_addr[6],
                     uint8_t  rmt_server_chann,
                     uint16_t frame_size,
                     uint8_t  credits,
                     uint8_t  local_server_chann);

bool spp_connect_cfm(uint8_t  bd_addr[6],
                     uint8_t  local_server_chann,
                     bool     accept,
                     uint16_t frame_size,
                     uint8_t  credits);

bool spp_send_data(uint8_t   bd_addr[6],
                   uint8_t   local_server_chann,
                   uint8_t  *p_data,
                   uint16_t  len,
                   bool      ack);

bool spp_disconnect_req(uint8_t bd_addr[6],
                        uint8_t local_server_chann);

void spp_disconnect_all(uint8_t bd_addr[6]);

bool spp_send_credit(uint8_t bd_addr[6],
                     uint8_t local_server_chann,
                     uint8_t credits);

bool spp_set_ertm_mode(bool enable);

bool spp_get_roleswap_info(uint8_t              bd_addr[6],
                           uint8_t              local_server_chann,
                           T_ROLESWAP_SPP_INFO *p_info);

bool spp_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_SPP_INFO *p_info);

bool spp_del_roleswap_info(uint8_t bd_addr[6],
                           uint8_t local_server_chann);

bool spp_get_rfc_profile_idx(uint8_t  local_server_chann,
                             uint8_t *p_rfc_idx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SPP_H_ */
