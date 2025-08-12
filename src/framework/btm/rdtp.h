/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _RDTP_H_
#define _RDTP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    RDTP_MSG_CONN_REQ     = 0x00,
    RDTP_MSG_CONN_RSP     = 0x01,
    RDTP_MSG_CONNECTED    = 0x02,
    RDTP_MSG_CONN_FAIL    = 0x03,
    RDTP_MSG_DISCONNECTED = 0x04,
    RDTP_MSG_DATA_IND     = 0x05,
    RDTP_MSG_DATA_RSP     = 0x06,
} T_RDTP_MSG;

typedef struct
{
    uint8_t  *buf;
    uint16_t  len;
} T_RDTP_DATA_IND;

typedef struct
{
    uint16_t    cause;
} T_RDTP_DISCONN_INFO;

typedef void(*P_RDTP_CBACK)(uint8_t     bd_addr[6],
                            T_RDTP_MSG  msg_type,
                            void       *msg_buf);

bool rdtp_init(P_RDTP_CBACK cback);

bool rdtp_conn_req(uint8_t bd_addr[6]);

bool rdtp_disconn_req(uint8_t bd_addr[6]);

bool rdtp_data_send(uint8_t   bd_addr[6],
                    void     *buf,
                    uint16_t  len,
                    bool      flush);

bool rdtp_handle_roleswap(uint8_t  bd_addr[6],
                          uint8_t *curr_bd_addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RDTP_H_ */
