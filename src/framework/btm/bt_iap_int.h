/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_IAP_INT_H_
#define _BT_IAP_INT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_bt_iap_conn_ind
{
    uint16_t    rfc_frame_size;
} T_BT_IAP_CONN_IND;

typedef struct t_bt_iap_conn_info
{
    uint16_t    max_data_len;
} T_BT_IAP_CONN_INFO;

typedef struct t_bt_iap_disconn_info
{
    uint16_t    cause;
} T_BT_IAP_DISCONN_INFO;

typedef struct t_bt_iap_start_eap_session
{
    uint16_t    eap_session_id;
    uint8_t     eap_id;
} T_BT_IAP_START_EAP_SESSION;

typedef struct t_bt_iap_stop_eap_session
{
    uint16_t    eap_session_id;
} T_BT_IAP_STOP_EAP_SESSION;

typedef struct t_bt_iap_eap_session_status_info
{
    uint16_t    eap_session_id;
    uint8_t     eap_session_status;
} T_BT_IAP_EAP_SESSION_STATUS_INFO;

typedef struct t_bt_iap_data_ind
{
    uint8_t    *p_data;
    uint16_t    len;
    uint16_t    eap_session_id;
    uint8_t     dev_seq_num;
} T_BT_IAP_DATA_IND;

typedef struct t_bt_iap_data_transmitted
{
    uint16_t    eap_session_id;
    bool        success;
} T_BT_IAP_DATA_TRANSMITTED;

typedef struct t_bt_iap_ctrl_msg_ind
{
    uint16_t    msg_id;
    uint16_t    param_len;
    uint8_t    *p_param;
} T_BT_IAP_CTRL_MSG_IND;

typedef struct t_roleswap_iap_info
{
    uint8_t     bd_addr[6];
    uint16_t    rfc_frame_size;
    uint8_t     dlci;
    uint8_t     remote_credit;

    uint8_t     state;
    uint16_t    dev_max_pkt_len;
    uint16_t    dev_retrans_tout;
    uint16_t    dev_cumulative_ack_tout;

    uint8_t     acc_pkt_seq;
    uint8_t     acked_seq;
    uint8_t     dev_pkt_seq;
    uint8_t     dev_max_out_pkt;
    uint8_t     dev_max_retrans;
    uint8_t     dev_max_cumulative_ack;
} T_ROLESWAP_IAP_INFO;

typedef struct t_roleswap_iap_transact
{
    uint8_t     bd_addr[6];
    uint8_t     remote_credit;
    uint8_t     state;
    uint8_t     acc_pkt_seq;
    uint8_t     acked_seq;
    uint8_t     dev_pkt_seq;
} T_ROLESWAP_IAP_TRANSACT;

bool bt_iap_service_id_get(uint8_t *service_id);

bool bt_iap_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_IAP_INFO *info);

bool bt_iap_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_IAP_INFO *info);

bool bt_iap_roleswap_info_del(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_IAP_INT_H_ */
