/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AVCTP_H_
#define _AVCTP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AVCTP_L2CAP_MTU_SIZE            0x0200 /* AVRCP spec defines max AV/C packet size as 512 bytes. */
#define AVCTP_BROWSING_L2CAP_MTU_SIZE   1017
#define AVCTP_NON_FRAG_HDR_LENGTH       3
#define AVCTP_START_HDR_LENGTH          4

#define AVCTP_MSG_TYPE_CMD              0
#define AVCTP_MSG_TYPE_RSP              1

#define AVCTP_PKT_TYPE_UNFRAG           0x0
#define AVCTP_PKT_TYPE_START            0x1
#define AVCTP_PKT_TYPE_CONTINUE         0x2
#define AVCTP_PKT_TYPE_END              0x3

typedef struct t_avctp_data_ind
{
    uint8_t     transact_label;
    uint8_t     crtype;
    uint16_t    length;
    uint8_t    *data;
} T_AVCTP_DATA_IND;

typedef enum t_avctp_msg
{
    AVCTP_MSG_CONN_IND               = 0x00,
    AVCTP_MSG_CONN_RSP               = 0x01,
    AVCTP_MSG_CONN_CMPL_IND          = 0x02,
    AVCTP_MSG_CONN_FAIL              = 0x03,
    AVCTP_MSG_DATA_IND               = 0x04,
    AVCTP_MSG_DISCONN_IND            = 0x05,

    AVCTP_MSG_BROWSING_CONN_IND      = 0x06,
    AVCTP_MSG_BROWSING_CONN_RSP      = 0x07,
    AVCTP_MSG_BROWSING_CONN_CMPL_IND = 0x08,
    AVCTP_MSG_BROWSING_DATA_IND      = 0x09,
    AVCTP_MSG_BROWSING_DISCONN_IND   = 0x0a,
} T_AVCTP_MSG;

typedef struct t_roleswap_avctp_info
{
    uint16_t l2c_cid;
    uint16_t remote_mtu;
    uint8_t  data_offset;
    uint8_t  avctp_state;
} T_ROLESWAP_AVCTP_INFO;

typedef void(*T_AVCTP_CBACK)(uint16_t     cid,
                             T_AVCTP_MSG  msg_type,
                             void        *msg_buf);

bool avctp_init(T_AVCTP_CBACK cback);

bool avctp_connect_req(uint8_t bd_addr[6]);

bool avctp_disconnect_req(uint8_t bd_addr[6]);

bool avctp_connect_cfm(uint8_t bd_addr[6],
                       bool    accept);

bool avctp_data_send(uint8_t   bd_addr[6],
                     uint8_t  *data,
                     uint16_t  len,
                     uint8_t  *vendor_data,
                     uint16_t  vendor_len,
                     uint8_t   transact,
                     uint8_t   crtype);

bool avctp_browsing_connect_req(uint8_t bd_addr[6]);

bool avctp_browsing_disconnect_req(uint8_t bd_addr[6]);

bool avctp_browsing_connect_cfm(uint8_t bd_addr[6],
                                bool    accept);

bool avctp_browsing_data_send(uint8_t   bd_addr[6],
                              uint8_t  *data,
                              uint16_t  length,
                              uint8_t   transact,
                              uint8_t   crtype);

bool avctp_roleswap_info_get(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVCTP_INFO *info);

bool avctp_roleswap_info_set(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVCTP_INFO *info);

bool avctp_roleswap_info_del(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AVCTP_H_ */
