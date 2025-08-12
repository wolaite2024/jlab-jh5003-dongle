/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_ROLESWAP_H_
#define _BT_ROLESWAP_H_

#include "gap_br.h"
#include "gap_handover_br.h"
#include "rfc.h"
#include "spp.h"
#include "a2dp.h"
#include "avrcp.h"
#include "hfp.h"
#include "pbap.h"
#include "iap.h"
#include "avp.h"
#include "att_br.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ROLESWAP_A2DP_PARAM_SINGAL = 0x00,
    ROLESWAP_A2DP_PARAM_STREAM = 0x01,
} T_ROLESWAP_A2DP_PARAM;

typedef enum
{
    ROLESWAP_HID_PARAM_CONTROL = 0x00,
    ROLESWAP_HID_PARAM_INTERRUPT = 0x01,
} T_ROLESWAP_HID_PARAM;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    handle;
    uint8_t     bd_type;
    uint8_t     conn_type;
    uint8_t     role;
    uint8_t     mode;
    uint16_t    link_policy;
    uint16_t    superv_tout;
    bool        authen_state;
    uint8_t     encrypt_state;
} T_ROLESWAP_ACL_INFO;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    handle;
    uint8_t     type;
    uint8_t     air_mode;
    uint8_t     pkt_len;
} T_ROLESWAP_SCO_INFO;

typedef struct
{
    uint32_t    profile_mask;
    uint16_t    cause;
    uint16_t    param;
} T_ROLESWAP_PROFILE_DISCONN_PARAM;

typedef struct
{
    uint16_t    cause;
    void       *p_param;
} T_ROLESWAP_RECOVERY_CONN_PARAM;

bool bt_roleswap_init(void);

void bt_roleswap_handle_acl_status(uint8_t   bd_addr[6],
                                   T_BT_MSG  msg,
                                   void     *buf);

void bt_roleswap_handle_link_policy(uint8_t bd_addr[6]);

void bt_roleswap_handle_profile_conn(uint8_t  bd_addr[6],
                                     uint32_t profile_mask,
                                     uint8_t  param);

void bt_roleswap_handle_profile_disconn(uint8_t                           bd_addr[6],
                                        T_ROLESWAP_PROFILE_DISCONN_PARAM *p_param);

void bt_roleswap_handle_sco_conn(uint8_t bd_addr[6]);

void bt_roleswap_handle_sco_disconn(uint8_t  bd_addr[6],
                                    uint16_t cause);

void bt_roleswap_handle_bt_rfc_conn(uint8_t bd_addr[6],
                                    uint8_t server_chann);

void bt_roleswap_handle_bt_rfc_disconn(uint8_t  bd_addr[6],
                                       uint8_t  server_chann,
                                       uint16_t cause);

void bt_roleswap_handle_bt_avp_control_conn(uint8_t bd_addr[6]);

void bt_roleswap_handle_bt_avp_control_disconn(uint8_t  bd_addr[6],
                                               uint16_t cause);

void bt_roleswap_handle_bt_avp_audio_conn(uint8_t bd_addr[6]);

void bt_roleswap_handle_bt_avp_audio_disconn(uint8_t  bd_addr[6],
                                             uint16_t cause);

void bt_roleswap_handle_bt_att_conn(uint8_t bd_addr[6]);

void bt_roleswap_handle_bt_att_disconn(uint8_t  bd_addr[6],
                                       uint16_t cause);

void bt_roleswap_handle_ctrl_conn(void);

void bt_roleswap_recv(uint8_t  *p_data,
                      uint16_t  data_len);

T_BT_CLK_REF bt_roleswap_get_piconet_clk(T_BT_CLK_REF  clk,
                                         uint32_t     *bb_clock_timer,
                                         uint16_t     *bb_clock_us);

bool bt_roleswap_get_piconet_id(T_BT_CLK_REF  clk_ref,
                                uint8_t      *clk_index,
                                uint8_t      *role);

bool bt_roleswap_conn_sniffing_link(uint8_t bd_addr[6]);

bool bt_roleswap_disconn_sniffing_link(uint8_t bd_addr[6]);

bool bt_roleswap_conn_audio_recovery(uint8_t  bd_addr[6],
                                     uint16_t interval,
                                     uint16_t flush_tout,
                                     uint8_t  rsvd_slot,
                                     uint8_t  idle_slot,
                                     uint8_t  idle_skip);

bool bt_roleswap_disconn_audio_recovery(uint8_t bd_addr[6],
                                        uint8_t reason);

bool bt_roleswap_cfg_audio_recovery(uint8_t  bd_addr[6],
                                    uint16_t interval,
                                    uint16_t flush_tout,
                                    uint8_t  rsvd_slot,
                                    uint8_t  idle_slot,
                                    uint8_t  idle_skip);

bool bt_roleswap_conn_voice_recovery(uint8_t bd_addr[6]);

bool bt_roleswap_disconn_voice_recovery(uint8_t bd_addr[6],
                                        uint8_t reason);

#ifdef __cplusplus
}
#endif

#endif /*_BT_ROLESWAP_H_*/
