/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_RFC_INT_H_
#define _BT_RFC_INT_H_

#include <stdint.h>
#include <stdbool.h>
#include "bt_rfc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BT_RFC_DLCI_NUM     11
#define BT_RFC_PROFILE_NUM  6

typedef struct
{
    P_BT_RFC_PROFILE_CBACK    cback;                 /**< point to callback function */
    uint8_t                   server_chann;
    uint8_t                   profile_index;
} BT_RFC_PROFILE;

typedef struct
{
    bool        used;
    uint8_t     bd_addr[6];
    uint8_t     local_server_chann;
    uint8_t     dlci;
} BT_RFC_CHANN;

typedef struct
{
    uint8_t      bd_addr[6];
    uint8_t      local_server_chann;
    uint8_t      dlci;
} T_ROLESWAP_BT_RFC_INFO;

bool bt_rfc_find_profile_idx_by_server_chann(uint8_t  local_server_chann,
                                             uint8_t *profile_idx);

bool bt_rfc_get_roleswap_info(uint8_t                 bd_addr[6],
                              uint8_t                 server_chann,
                              T_ROLESWAP_BT_RFC_INFO *p_info);

bool bt_rfc_set_roleswap_info(T_ROLESWAP_BT_RFC_INFO *p_info);

bool bt_rfc_del_roleswap_info(uint8_t bd_addr[6],
                              uint8_t server_chann);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_RFC_INT_H_ */
