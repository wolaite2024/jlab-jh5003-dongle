/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_ATT_INT_H_
#define _BT_ATT_INT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_roleswap_att_info
{
    uint8_t     bd_addr[6];
    uint16_t    l2c_cid;
    uint8_t     state;
    uint8_t     data_offset;
    uint16_t    remote_mtu;
    uint16_t    local_mtu;
    uint16_t    conn_handle;
} T_ROLESWAP_ATT_INFO;

typedef struct t_roleswap_att_transact
{
    uint8_t     bd_addr[6];
    uint8_t     state;
} T_ROLESWAP_ATT_TRANSACT;

bool bt_att_roleswap_info_del(uint8_t bd_addr[6]);

bool bt_att_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_ATT_INFO *info);

bool bt_att_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_ATT_INFO *info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
