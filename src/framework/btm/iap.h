/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _IAP_H_
#define _IAP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_iap_msg
{
    IAP_MSG_CONN_IND           = 0x00,
    IAP_MSG_CONNECTED          = 0x01,
    IAP_MSG_CONN_FAIL          = 0x02,
    IAP_MSG_DISCONNTED         = 0x03,
    IAP_MSG_START_IDENT_REQ    = 0x04,
    IAP_MSG_AUTHEN_SUCCESS     = 0x05,
    IAP_MSG_AUTHEN_FAIL        = 0x06,
    IAP_MSG_DATA_IND           = 0x07,
    IAP_MSG_START_EAP_SESSION  = 0x08,
    IAP_MSG_STOP_EAP_SESSION   = 0x09,
    IAP_MSG_EAP_SESSION_STATUS = 0x0a,
    IAP_MSG_DATA_TRANSMITTED   = 0x0b,
    IAP_MSG_RESET              = 0x0c,
    IAP_MSG_CTRL_MSG_IND       = 0x0d,
} T_IAP_MSG;

typedef enum t_iap_eap_session_status
{
    IAP_EAP_SESSION_STATUS_OK     = 0x00,
    IAP_EAP_SESSION_STATUS_CLOSED = 0x01
} T_IAP_EAP_SESSION_STATUS;

typedef enum t_iap_app_launch_method
{
    IAP_APP_LAUNCH_WITH_USER_ALERT    = 0x00,
    IAP_APP_LAUNCH_WITHOUT_USER_ALERT = 0x01
} T_IAP_APP_LAUNCH_METHOD;

typedef struct t_iap_conn_ind
{
    uint16_t    rfc_frame_size;
} T_IAP_CONN_IND;

typedef struct t_iap_conn_cmpl_info
{
    uint16_t    max_data_len;
} T_IAP_CONN_CMPL_INFO;

typedef struct t_iap_disconn_info
{
    uint16_t    cause;
} T_IAP_DISCONN_INFO;

typedef struct t_iap_start_eap_session
{
    uint16_t    eap_session_id;
    uint8_t     eap_id;
} T_IAP_START_EAP_SESSION;

typedef struct t_iap_stop_eap_session
{
    uint16_t    eap_session_id;
} T_IAP_STOP_EAP_SESSION;

typedef struct t_iap_eap_session_status_info
{
    uint16_t                 eap_session_id;
    T_IAP_EAP_SESSION_STATUS eap_session_status;
} T_IAP_EAP_SESSION_STATUS_INFO;

typedef struct t_iap_data_ind
{
    uint8_t    *p_data;
    uint16_t    len;
    uint16_t    eap_session_id;
    uint8_t     dev_seq_num;
} T_IAP_DATA_IND;

typedef struct t_iap_data_transmitted
{
    uint16_t    eap_session_id;
    uint16_t    data_len;
    bool        success;
} T_IAP_DATA_TRANSMITTED;

typedef struct t_iap_ctrl_msg_ind
{
    uint16_t    msg_id;
    uint16_t    param_len;
    uint8_t    *p_param;
} T_IAP_CTRL_MSG_IND;

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
    uint8_t     dev_max_culumative_ack;
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

typedef void(*P_IAP_CB)(uint8_t    bd_addr[6],
                        T_IAP_MSG  type,
                        void      *p_data);

typedef enum
{
    IAP_PARAM_ACC_MAX_PKT_LEN         = 0x00,
    IAP_PARAM_ACC_RETRANS_TOUT        = 0x01,
    IAP_PARAM_ACC_CUMULATIVE_ACK_TOUT = 0x02,
    IAP_PARAM_ACC_MAX_OUT_PKT_NUM     = 0x03,
    IAP_PARAM_ACC_MAX_RETRANS_NUM     = 0x04,
    IAP_PARAM_ACC_MAX_CULUMATIVE_ACK  = 0x05,
    IAP_PARAM_ACC_START_SEQ_NUM       = 0x06,

    IAP_PARAM_TX_Q_ELEM_NUM           = 0x80,
} T_IAP_PARAM_TYPE;

bool iap_init(uint8_t  link_num,
              uint8_t  iap_chann_num,
              P_IAP_CB callback);

bool iap_set_param(T_IAP_PARAM_TYPE  type,
                   uint8_t           len,
                   void             *p_value);

bool iap_connect_req(uint8_t  bd_addr[6],
                     uint8_t  server_chann,
                     uint16_t frame_size,
                     uint8_t  init_credits);

bool iap_connect_cfm(uint8_t  bd_addr[6],
                     bool     accept,
                     uint16_t frame_size,
                     uint8_t  init_credits);

bool iap_disconnect_req(uint8_t bd_addr[6]);

uint8_t *iap2_prep_param(uint8_t  *p_param,
                         uint16_t  param_id,
                         void     *p_data,
                         uint16_t  data_len);

bool iap_send_ident_info(uint8_t   bd_addr[6],
                         uint8_t  *p_ident,
                         uint16_t  ident_len);

bool iap_eap_session_status_send(uint8_t                  bd_addr[6],
                                 uint16_t                 session_id,
                                 T_IAP_EAP_SESSION_STATUS status);

bool iap_launch_app(uint8_t                  bd_addr[6],
                    char                    *boundle_id,
                    uint8_t                  len_boundle_id,
                    T_IAP_APP_LAUNCH_METHOD  method);

bool iap_hid_start(uint8_t   bd_addr[6],
                   uint16_t  hid_component_id,
                   uint16_t  vid,
                   uint16_t  pid,
                   uint8_t  *hid_report_desc,
                   uint16_t  hid_report_desc_len);

bool iap_hid_send_report(uint8_t   bd_addr[6],
                         uint16_t  hid_component_id,
                         uint8_t  *hid_report,
                         uint16_t  hid_report_len);

bool iap_hid_stop(uint8_t  bd_addr[6],
                  uint16_t hid_component_id);

bool iap_send_bt_comp_info(uint8_t  bd_addr[6],
                           uint16_t comp_id,
                           bool     enable);

bool iap_send_start_bt_conn_update(uint8_t  bd_addr[6],
                                   uint16_t comp_id);

bool iap_send_start_comm_update(uint8_t  bd_addr[6],
                                uint16_t param_id);

bool iap_send_mute_status_update(uint8_t bd_addr[6],
                                 bool    status);

bool iap_send_cmd(uint8_t   bd_addr[6],
                  uint16_t  msg_type,
                  uint8_t  *p_param,
                  uint16_t  param_len);

bool iap_send_data(uint8_t   bd_addr[6],
                   uint16_t  session_id,
                   uint8_t  *p_data,
                   uint16_t  data_len);

bool iap_send_ack(uint8_t bd_addr[6],
                  uint8_t ack_seq);

bool iap_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_IAP_INFO *p_data);

bool iap_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_IAP_INFO *p_data);

bool iap_del_roleswap_info(uint8_t bd_addr[6]);

uint8_t iap_get_rfc_profile_idx(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IAP_H_ */
