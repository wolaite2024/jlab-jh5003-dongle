/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AVCTP_H_
#define _AVCTP_H_

#include <stdint.h>
#include <stdbool.h>

#include "avrcp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AVCTP_L2CAP_MTU_SIZE            0x0200 //AVRCP spec define max AV/C packet size: 512
#define AVCTP_BROWSING_L2CAP_MTU_SIZE   1017
#define AVCTP_NON_FRAG_HDR_LENGTH    3
#define AVCTP_START_HDR_LENGTH       4

#define AVCTP_MSG_TYPE_CMD     0
#define AVCTP_MSG_TYPE_RSP     1

#define AVCTP_PKT_TYPE_UNFRAG        0x0
#define AVCTP_PKT_TYPE_START         0x1
#define AVCTP_PKT_TYPE_CONTINUE      0x2
#define AVCTP_PKT_TYPE_END           0x3

typedef enum t_avctp_state
{
    AVCTP_STATE_DISCONNECTED  = 0x00,   /* no L2CAP connection */
    AVCTP_STATE_ALLOCATED     = 0x01,
    AVCTP_STATE_CONNECTING    = 0x02,
    AVCTP_STATE_CONNECTED     = 0x03,   /* L2CAP connected */
    AVCTP_STATE_DISCONNECTING = 0x04,
} T_AVCTP_STATE;

typedef struct t_avctp_recombine
{
    uint8_t         *p_buf;
    uint8_t         number_of_packets;
    uint16_t        write_index;
    uint16_t        profile_id;
    uint16_t        recombine_length;
} T_AVCTP_RECOMBINE;

typedef struct
{
    T_AVCTP_STATE     state;
    uint16_t          cid;
    uint16_t          remote_mtu;
    T_AVCTP_RECOMBINE recombine;
    uint16_t          ds_data_offset;
} T_AVCTP_CONTROL_CHANN;

typedef struct
{
    T_AVCTP_STATE     state;
    uint16_t          cid;
    uint16_t          remote_mtu;
    uint16_t          profile_id;
    uint16_t          ds_data_offset;
} T_AVCTP_BROWSING_CHANN;

typedef struct t_avctp_link
{
    uint8_t                   bd_addr[6];
    T_AVCTP_CONTROL_CHANN     control_chann;
    T_AVCTP_BROWSING_CHANN    browsing_chann;
} T_AVCTP_LINK;

typedef struct t_avctp_data_ind
{
    uint8_t    transact_label;
    uint8_t    crtype;
    uint16_t   length;
    uint8_t    *p_data;
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

typedef void(*P_AVCTP_CBACK)(uint16_t     cid,
                             T_AVCTP_MSG  msg_type,
                             void        *msg_buf);

bool avctp_init(uint8_t       link_num,
                P_AVCTP_CBACK cback);

bool avctp_connect_req(uint8_t bd_addr[6]);

bool avctp_disconnect_req(uint8_t bd_addr[6]);

bool avctp_connect_cfm(uint8_t bd_addr[6],
                       bool    accept);

#define avctp_send_data(bd_addr, avrcp_data, length, transact, crtype)  \
    avctp_send_data2buf(bd_addr, avrcp_data, length, NULL, 0, transact, crtype)

bool avctp_send_data2buf(uint8_t   bd_addr[6],
                         uint8_t  *p_avrcp_data,
                         uint16_t  avrcp_length,
                         uint8_t  *p_avrcp_data2,
                         uint16_t  length2,
                         uint8_t   transact,
                         uint8_t   crtype);

bool avctp_browsing_connect_req(uint8_t bd_addr[6]);

bool avctp_browsing_disconnect_req(uint8_t bd_addr[6]);

bool avctp_browsing_connect_cfm(uint8_t bd_addr[6],
                                bool    accept);

bool avctp_browsing_send_data2buf(uint8_t   bd_addr[6],
                                  uint8_t  *p_avrcp_data,
                                  uint16_t  avrcp_length,
                                  uint8_t   transact,
                                  uint8_t   crtype);

bool avctp_get_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info);

bool avctp_set_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info);

bool avctp_del_roleswap_info(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AVCTP_H_ */
