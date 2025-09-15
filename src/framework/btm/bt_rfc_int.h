/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_RFC_INT_H_
#define _BT_RFC_INT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
{
    uint8_t      bd_addr[6];
    uint8_t      local_server_chann;
    uint8_t      dlci;
} T_ROLESWAP_BT_RFC_INFO;

bool bt_rfc_service_id_get(uint8_t  local_server_chann,
                           uint8_t *service_id);

bool bt_rfc_get_roleswap_info(uint8_t                 bd_addr[6],
                              uint8_t                 server_chann,
                              T_ROLESWAP_BT_RFC_INFO *info);

bool bt_rfc_set_roleswap_info(T_ROLESWAP_BT_RFC_INFO *info);

bool bt_rfc_del_roleswap_info(uint8_t bd_addr[6],
                              uint8_t server_chann);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_RFC_INT_H_ */
