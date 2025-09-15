/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_HFP_INT_H_
#define _BT_HFP_INT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_bt_hfp_network_operator_ind
{
    uint8_t mode;
    uint8_t format;
    char    name[17];
} T_BT_HFP_NETWORK_OPERATOR_IND;

typedef struct t_bt_hfp_subscriber_number_ind
{
    char    number[20];
    uint8_t type;
    uint8_t service;
} T_BT_HFP_SUBSCRIBER_NUMBER_IND;

typedef struct t_bt_hfp_caller_id_ind
{
    char    number[20];
    uint8_t type;
} T_BT_HFP_CALLER_ID_IND;

typedef struct t_bt_hfp_call_waiting_ind
{
    char    number[20];
    uint8_t type;
} T_BT_HFP_CALL_WAITING_IND;

typedef struct t_bt_hfp_clcc_ind
{
    uint8_t bd_addr[6];
    uint8_t call_idx;
    uint8_t dir_incoming;
    uint8_t status;
    uint8_t mode;
    uint8_t mpty;
    char    number[20];
    uint8_t type;
} T_BT_HFP_CLCC_IND;

typedef struct t_bt_hfp_call_status_info
{
    uint8_t prev_call_status;
    uint8_t curr_call_status;
} T_BT_HFP_CALL_STATUS_INFO;

typedef struct t_roleswap_hfp_info
{
    uint8_t    bd_addr[6];
    uint16_t   l2c_cid;
    uint16_t   uuid;
    uint8_t    rfc_dlci;
    uint8_t    indicator_cnt;
    uint8_t    indicator_type[20];
    uint8_t    indicator_range[20];
    uint16_t   cpbs;
    uint16_t   frame_size;
    uint8_t    state;
    uint8_t    batt_report_type;
    uint8_t    at_cmd_credits;
    uint8_t    curr_call_status;
    uint8_t    prev_call_status;
    uint8_t    service_status;
    uint8_t    supported_features;
    uint8_t    codec_type;
} T_ROLESWAP_HFP_INFO;

typedef struct t_roleswap_hfp_transact
{
    uint8_t    bd_addr[6];
    uint8_t    state;
    uint8_t    batt_report_type;
    uint8_t    at_cmd_credits;
    uint8_t    curr_call_status;
    uint8_t    prev_call_status;
    uint8_t    service_status;
    uint8_t    supported_features;
    uint8_t    codec_type;
} T_ROLESWAP_HFP_TRANSACT;

void bt_hfp_handle_audio_conn_cmpl(uint8_t bd_addr[6]);

void bt_hfp_handle_audio_disconn(uint8_t bd_addr[6]);

bool bt_hfp_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_HFP_INFO *info);

bool bt_hfp_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_HFP_INFO *info);

bool bt_hfp_roleswap_info_del(uint8_t  bd_addr[6],
                              uint16_t cause);

bool bt_hfp_service_id_get(uint16_t  uuid,
                           uint8_t  *service_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_HFP_INT_H_ */
