/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_AVP_INT_H_
#define _BT_AVP_INT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_roleswao_avp_info
{
    uint8_t     bd_addr[6];
    uint16_t    l2c_cid;
    uint8_t     state;
    uint8_t     data_offset;
} T_ROLESWAP_AVP_INFO;

typedef struct t_roleswap_avp_transact
{
    uint8_t     bd_addr[6];
    uint8_t     state;
} T_ROLESWAP_AVP_TRANSACT;

bool bt_avp_roleswap_info_del(uint8_t bd_addr[6]);

bool bt_avp_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_AVP_INFO *info);

bool bt_avp_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_AVP_INFO *info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_AVP_INT_H_ */
