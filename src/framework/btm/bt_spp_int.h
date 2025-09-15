/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_SPP_INT_H_
#define _BT_SPP_INT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_bt_spp_conn_ind
{
    uint8_t     local_server_chann;
    uint16_t    frame_size;
} T_BT_SPP_CONN_IND;

typedef struct t_bt_spp_conn_cmpl
{
    uint8_t     local_server_chann;
    uint8_t     credit;
    uint16_t    frame_size;
} T_BT_SPP_CONN_CMPL;

typedef struct t_bt_spp_data_ind
{
    uint8_t     local_server_chann;
    uint8_t    *p_data;
    uint16_t    len;
} T_BT_SPP_DATA_IND;

typedef struct t_bt_spp_data_rsp
{
    uint8_t     local_server_chann;
} T_BT_SPP_DATA_RSP;

typedef struct t_bt_spp_credit_info
{
    uint8_t     local_server_chann;
    uint8_t     credit;
} T_BT_SPP_CREDIT_INFO;

typedef struct t_bt_spp_disconn_cmpl
{
    uint8_t     local_server_chann;
    uint16_t    cause;
} T_BT_SPP_DISCONN_CMPL;

typedef struct t_roleswap_spp_info
{
    uint8_t     bd_addr[6];
    uint16_t    l2c_cid;
    uint16_t    frame_size;
    uint8_t     rfc_dlci;
    uint8_t     local_server_chann;
    uint8_t     state;
    uint8_t     remote_credit;
} T_ROLESWAP_SPP_INFO;

typedef struct t_roleswap_spp_transact
{
    uint8_t     bd_addr[6];
    uint8_t     local_server_chann;
    uint8_t     state;
    uint8_t     remote_credit;
} T_ROLESWAP_SPP_TRANSACT;

bool bt_spp_service_id_get(uint8_t  local_server_chann,
                           uint8_t *service_id);

bool bt_spp_roleswap_info_get(uint8_t              bd_addr[6],
                              uint8_t              local_server_chann,
                              T_ROLESWAP_SPP_INFO *info);

bool bt_spp_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_SPP_INFO *info);

bool bt_spp_roleswap_info_del(uint8_t bd_addr[6],
                              uint8_t local_server_chann);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_SPP_INT_H_ */
