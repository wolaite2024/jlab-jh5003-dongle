/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_IAP_INT_H_
#define _BT_IAP_INT_H_

#include <stdint.h>
#include "iap.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_iap_link_data
{
    uint16_t    rfc_frame_size;
    uint16_t    max_data_len;
    uint16_t    eap_session_id;
    uint8_t     eap_id;
    uint8_t     dev_seq_num;
} T_IAP_LINK_DATA;

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
    uint16_t                 eap_session_id;
    T_IAP_EAP_SESSION_STATUS eap_session_status;
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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_IAP_INT_H_ */
