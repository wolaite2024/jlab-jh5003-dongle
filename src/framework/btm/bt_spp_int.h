/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_SPP_INT_H_
#define _BT_SPP_INT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_spp_link_data
{
    uint8_t iap_authen_flag;
    uint8_t spp_ref;
} T_SPP_LINK_DATA;

typedef struct
{
    uint8_t     uuid[16];
    uint8_t     server_chann;
} T_SPP_SERVICE;

typedef struct
{
    uint8_t     local_server_chann;
    uint16_t    frame_size;
} T_BT_SPP_CONN_IND;

typedef struct
{
    uint8_t     local_server_chann;
    uint8_t     credit;
    uint16_t    frame_size;
} T_BT_SPP_CONN_CMPL;

typedef struct
{
    uint8_t     local_server_chann;
    uint8_t    *p_data;
    uint16_t    len;
} T_BT_SPP_DATA_IND;

typedef struct
{
    uint8_t     local_server_chann;
} T_BT_SPP_DATA_RSP;

typedef struct
{
    uint8_t     local_server_chann;
    uint8_t     credit;
} T_BT_SPP_CREDIT_INFO;

typedef struct
{
    uint8_t     local_server_chann;
    uint16_t    cause;
} T_BT_SPP_DISCONN_CMPL;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_SPP_INT_H_ */
