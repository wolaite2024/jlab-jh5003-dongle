/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _ATT_BR_H_
#define _ATT_BR_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    ATT_MSG_CONNECTED      = 0x00,
    ATT_MSG_CONN_FAIL      = 0x01,
    ATT_MSG_DISCONNECTED   = 0x02,
    ATT_MSG_DATA_IND       = 0x03,
} T_ATT_MSG;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    l2c_cid;
    uint8_t     state;
    uint8_t     data_offset;
    uint16_t    remote_mtu;
    uint16_t    local_mtu;
} T_ROLESWAP_ATT_INFO;

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     state;
} T_ROLESWAP_ATT_TRANSACT;

typedef void(*P_ATT_CBACK)(uint8_t    bd_addr[6],
                           T_ATT_MSG  msg_type,
                           void      *msg_buf);

bool att_init(P_ATT_CBACK cback);

bool att_conn_req(uint8_t bd_addr[6]);

bool att_disconn_req(uint8_t bd_addr[6]);

bool att_del_roleswap_info(uint8_t bd_addr[6]);

bool att_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_ATT_INFO *p_info);

bool att_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_ATT_INFO *p_info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
