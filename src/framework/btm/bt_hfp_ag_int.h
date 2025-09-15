/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_HFP_AG_INT_H_
#define _BT_HFP_AG_INT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_bt_hfp_ag_sco_setting
{
    HFP_AG_SCO_SETTING_ANY  = 0x00,
    HFP_AG_SCO_SETTING_T2   = 0x01,
    HFP_AG_SCO_SETTING_T1   = 0x02,
    HFP_AG_SCO_SETTING_S4   = 0x03,
    HFP_AG_SCO_SETTING_S3   = 0x04,
    HFP_AG_SCO_SETTING_S2   = 0x05,
    HFP_AG_SCO_SETTING_S1   = 0x06,
    HFP_AG_SCO_SETTING_D1   = 0x07,
} T_BT_HFP_AG_SCO_SETTING;

typedef struct t_bt_hfp_ag_call_dtmf_code
{
    char dtmf_code;
} T_BT_HFP_AG_CALL_DTMF_CODE;

typedef struct t_bt_hfp_ag_dial_with_number
{
    char number[20];
} T_BT_HFP_AG_DIAL_WITH_NUMBER;

typedef struct t_bt_hfp_ag_dial_with_memory
{
    uint8_t num;
} T_BT_HFP_AG_DIAL_WITH_MEMORY;

typedef struct t_bt_hfp_ag_call_status_info
{
    uint8_t prev_call_status;
    uint8_t curr_call_status;
} T_BT_HFP_AG_CALL_STATUS_INFO;

typedef struct t_bt_hfp_ag_disconn_info
{
    uint16_t cause;
} T_BT_HFP_AG_DISCONN_INFO;

void bt_hfp_ag_handle_audio_conn_rsp(uint8_t bd_addr[6], uint16_t cause);

void bt_hfp_ag_handle_audio_conn_cmpl(uint8_t bd_addr[6], uint16_t cause);

void bt_hfp_ag_handle_audio_disconn(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_HFP_AG_INT_H_ */
