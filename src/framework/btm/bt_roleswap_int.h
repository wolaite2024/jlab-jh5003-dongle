/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_ROLESWAP_INT_H_
#define _BT_ROLESWAP_INT_H_

#include "gap_br.h"
#include "gap_handover_br.h"
#include "rfc.h"
#include "bt_mgr.h"
#include "bt_roleswap.h"
#include "bt_mgr_int.h"
#include "bt_a2dp_int.h"
#include "bt_hfp_int.h"
#include "bt_avrcp_int.h"
#include "bt_hid_int.h"
#include "bt_spp_int.h"
#include "bt_iap_int.h"
#include "bt_pbap_int.h"
#include "bt_rfc_int.h"
#include "bt_avp_int.h"
#include "bt_att_int.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ROLESWAP_MSG_HDR_LEN    4   /* module(1), submodule(1), len(2) */

#define UUID_IAP                            0xFFF0
#define UUID_BTRFC                          0xFFFF

typedef enum
{
    ROLESWAP_TYPE_ACL           = 0x00,
    ROLESWAP_TYPE_SCO           = 0x01,
    ROLESWAP_TYPE_SM            = 0x02,
    ROLESWAP_TYPE_L2C           = 0x03,
    ROLESWAP_TYPE_RFC_CTRL      = 0x04,
    ROLESWAP_TYPE_RFC_DATA      = 0x05,
    ROLESWAP_TYPE_A2DP          = 0x06,
    ROLESWAP_TYPE_AVRCP         = 0x07,
    ROLESWAP_TYPE_HFP           = 0x08,
    ROLESWAP_TYPE_SPP           = 0x09,
    ROLESWAP_TYPE_PBAP          = 0x0A,
    ROLESWAP_TYPE_HID_DEVICE    = 0x0B,
    ROLESWAP_TYPE_HID_HOST      = 0x0C,
    ROLESWAP_TYPE_IAP           = 0x0D,
    ROLESWAP_TYPE_AVP           = 0x0E,
    ROLESWAP_TYPE_BT_RFC        = 0x0F,
    ROLESWAP_TYPE_ATT           = 0x10,
    ROLESWAP_TYPE_ALL
} T_ROLESWAP_DATA_TYPE;

typedef enum
{
    ROLESWAP_MODULE_ACL          = 0x00,
    ROLESWAP_MODULE_SCO          = 0x01,
    ROLESWAP_MODULE_SM           = 0x02,
    ROLESWAP_MODULE_L2C          = 0x03,
    ROLESWAP_MODULE_RFC          = 0x04,
    ROLESWAP_MODULE_A2DP         = 0x05,
    ROLESWAP_MODULE_AVRCP        = 0x06,
    ROLESWAP_MODULE_HFP          = 0x07,
    ROLESWAP_MODULE_SPP          = 0x08,
    ROLESWAP_MODULE_PBAP         = 0x09,
    ROLESWAP_MODULE_HID_DEVICE   = 0x0A,
    ROLESWAP_MODULE_HID_HOST     = 0x0B,
    ROLESWAP_MODULE_IAP          = 0x0C,
    ROLESWAP_MODULE_AVP          = 0x0D,
    ROLESWAP_MODULE_CTRL         = 0x0E,
    ROLESWAP_MODULE_BT_RFC       = 0x0F,
    ROLESWAP_MODULE_ATT          = 0x10,
} T_ROLESWAP_MODULE;

typedef enum
{
    ROLESWAP_ACL_CONN,
    ROLESWAP_ACL_DISCONN,
    ROLESWAP_ACL_UPDATE,
} T_ROLESWAP_ACL_SUBMODULE;

typedef enum
{
    ROLESWAP_SCO_CONN,
    ROLESWAP_SCO_DISCONN
} T_ROLESWAP_SCO_SUBMODULE;

typedef enum
{
    ROLESWAP_SM_CONN,
    ROLESWAP_SM_DISCONN,
    ROLESWAP_SM_UPDATE
} T_ROLESWAP_SM_SUBMODULE;

typedef enum
{
    ROLESWAP_L2C_CONN,
    ROLESWAP_L2C_DISCONN
} T_ROLESWAP_L2C_SUBMODULE;

typedef enum
{
    ROLESWAP_RFC_CTRL_CONN,
    ROLESWAP_RFC_CTRL_DISCONN,
    ROLESWAP_RFC_DATA_CONN,
    ROLESWAP_RFC_DATA_DISCONN,
    ROLESWAP_RFC_DATA_TRANSACT
} T_ROLESWAP_RFC_SUBMODULE;

typedef enum
{
    ROLESWAP_SPP_CONN,
    ROLESWAP_SPP_DISCONN,
    ROLESWAP_SPP_TRANSACT,
} T_ROLESWAP_SPP_SUBMODULE;

typedef enum
{
    ROLESWAP_A2DP_CONN,
    ROLESWAP_A2DP_DISCONN,
    ROLESWAP_A2DP_STREAM_CONN,
    ROLESWAP_A2DP_STREAM_DISCONN,
    ROLESWAP_A2DP_TRANSACT,
} T_ROLESWAP_A2DP_SUBMODULE;

typedef enum
{
    ROLESWAP_AVRCP_CONN,
    ROLESWAP_AVRCP_DISCONN,
    ROLESWAP_AVRCP_TRANSACT,
} T_ROLESWAP_AVRCP_SUBMODULE;

typedef enum
{
    ROLESWAP_HFP_CONN,
    ROLESWAP_HFP_DISCONN,
    ROLESWAP_HFP_TRANSACT
} T_ROLESWAP_HFP_SUBMODULE;

typedef enum
{
    ROLESWAP_PBAP_CONN,
    ROLESWAP_PBAP_DISCONN,
    ROLESWAP_PBAP_TRANSACT
} T_ROLESWAP_PBAP_SUBMODULE;

typedef enum
{
    ROLESWAP_HID_DEVICE_CONN,
    ROLESWAP_HID_DEVICE_DISCONN,
    ROLESWAP_HID_DEVICE_TRANSACT,
} T_ROLESWAP_HID_DEVICE_SUBMODULE;

typedef enum
{
    ROLESWAP_HID_HOST_CONN,
    ROLESWAP_HID_HOST_DISCONN,
    ROLESWAP_HID_HOST_TRANSACT,
} T_ROLESWAP_HID_HOST_SUBMODULE;

typedef enum
{
    ROLESWAP_IAP_CONN,
    ROLESWAP_IAP_DISCONN,
    ROLESWAP_IAP_TRANSACT
} T_ROLESWAP_IAP_SUBMODULE;

typedef enum
{
    ROLESWAP_AVP_CONN,
    ROLESWAP_AVP_DISCONN,
    ROLESWAP_AVP_TRANSACT,
} T_ROLESWAP_AVP_SUBMODULE;

typedef enum
{
    ROLESWAP_ATT_CONN,
    ROLESWAP_ATT_DISCONN,
    ROLESWAP_ATT_TRANSACT,
} T_ROLESWAP_ATT_SUBMODULE;

typedef enum
{
    ROLESWAP_CTRL_TOKEN_REQ,
    ROLESWAP_CTRL_TOKEN_RSP,
    ROLESWAP_CTRL_DISCONN_SNIFFING_REQ,
} T_ROLESWAP_CTRL_SUBMODULE;

typedef enum
{
    ROLESWAP_BT_RFC_CONN,
    ROLESWAP_BT_RFC_DISCONN,
} T_ROLESWAP_BT_RFC_SUBMODULE;

typedef struct
{
    uint8_t     bd_addr[6];
    uint8_t     role;
    uint8_t     mode;
    uint16_t    link_policy;
    uint16_t    superv_tout;
    bool        authen_state;
    uint8_t     encrypt_state;
} T_ROLESWAP_ACL_TRANSACT;

typedef struct
{
    uint8_t     bd_addr[6];
    uint32_t    mode;
    uint8_t     state;
    uint8_t     sec_state;
    uint8_t     remote_authen;
    uint8_t     remote_io;
} T_ROLESWAP_SM_INFO;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    local_cid;
    uint16_t    remote_cid;

    uint16_t    local_mtu;
    uint16_t    remote_mtu;

    uint16_t    local_mps;
    uint16_t    remote_mps;

    uint16_t    psm;
    uint8_t     role;
    uint8_t     mode;
} T_ROLESWAP_L2C_INFO;

typedef struct t_roleswap_data
{
    struct t_roleswap_data *next;
    uint16_t                type;
    uint16_t                length;
    union
    {
        T_ROLESWAP_ACL_INFO         acl;
        T_ROLESWAP_SCO_INFO         sco;
        T_ROLESWAP_SM_INFO          sm;
        T_ROLESWAP_L2C_INFO         l2c;
        T_ROLESWAP_RFC_CTRL_INFO    rfc_ctrl;
        T_ROLESWAP_RFC_DATA_INFO    rfc_data;
        T_ROLESWAP_SPP_INFO         spp;
        T_ROLESWAP_A2DP_INFO        a2dp;
        T_ROLESWAP_AVRCP_INFO       avrcp;
        T_ROLESWAP_HFP_INFO         hfp;
        T_ROLESWAP_PBAP_INFO        pbap;
        T_ROLESWAP_HID_DEVICE_INFO  hid_device;
        T_ROLESWAP_HID_HOST_INFO    hid_host;
        T_ROLESWAP_IAP_INFO         iap;
        T_ROLESWAP_AVP_INFO         avp;
        T_ROLESWAP_BT_RFC_INFO      bt_rfc;
        T_ROLESWAP_ATT_INFO         att;
    } u;
} T_ROLESWAP_DATA;

typedef void (*P_BT_ROLESWAP_ACL_STATUS)(uint8_t   bd_addr[6],
                                         T_BT_MSG  msg,
                                         void     *buf);

typedef void (*P_BT_ROLESWAP_PROFILE_CONN)(uint8_t  bd_addr[6],
                                           uint32_t profile_mask,
                                           uint8_t  param);

typedef void (*P_BT_ROLESWAP_PROFILE_DISCONN)(uint8_t                           bd_addr[6],
                                              T_ROLESWAP_PROFILE_DISCONN_PARAM *param);

typedef void (*P_BT_ROLESWAP_SCO_DISCONN)(uint8_t  bd_addr[6],
                                          uint16_t cause);

typedef void (*P_BT_ROLESWAP_BT_RFC_CONN)(uint8_t bd_addr[6],
                                          uint8_t server_chann);

typedef void (*P_BT_ROLESWAP_BT_RFC_DISCONN)(uint8_t  bd_addr[6],
                                             uint8_t  server_chann,
                                             uint16_t cause);

typedef void (*P_BT_ROLESWAP_BT_AVP_CONN)(uint8_t bd_addr[6]);

typedef void (*P_BT_ROLESWAP_BT_AVP_DISCONN)(uint8_t  bd_addr[6],
                                             uint16_t cause);

typedef void (*P_BT_ROLESWAP_BT_ATT_CONN)(uint8_t bd_addr[6]);

typedef void (*P_BT_ROLESWAP_BT_ATT_DISCONN)(uint8_t  bd_addr[6],
                                             uint16_t cause);

typedef void (*P_BT_ROLESWAP_CRTL_CONN)(void);

typedef void (*P_BT_ROLESWAP_RECV)(uint8_t  *data,
                                   uint16_t  data_len);

typedef void (*P_BT_ROLESWAP_CBACK)(void                       *buf,
                                    T_GAP_BR_HANDOVER_MSG_TYPE  msg);

typedef bool (*P_BT_ROLESWAP_START)(uint8_t                      bd_addr[6],
                                    uint8_t                      context,
                                    bool                         stop_after_shadow,
                                    P_REMOTE_ROLESWAP_SYNC_CBACK cback);

typedef bool (*P_BT_ROLESWAP_CFM)(bool    accept,
                                  uint8_t context);

typedef bool (*P_BT_ROLESWAP_STOP)(uint8_t bd_addr[6]);

typedef bool (*P_BT_ROLESWAP_GET_PICONET_ID)(T_BT_CLK_REF  clk_ref,
                                             uint8_t      *pid,
                                             uint8_t      *role);

typedef T_BT_CLK_REF(*P_BT_ROLESWAP_GET_PICONET_CLK)(T_BT_CLK_REF  clk_ref,
                                                     uint32_t     *bb_clock_timer,
                                                     uint16_t     *bb_clock_us);

typedef struct
{
    P_BT_ROLESWAP_ACL_STATUS                acl_status;
    P_BT_ROLESWAP_PROFILE_CONN              profile_conn;
    P_BT_ROLESWAP_PROFILE_DISCONN           profile_disconn;
    P_BT_ROLESWAP_SCO_DISCONN               sco_disconn;
    P_BT_ROLESWAP_BT_RFC_CONN               bt_rfc_conn;
    P_BT_ROLESWAP_BT_RFC_DISCONN            bt_rfc_disconn;
    P_BT_ROLESWAP_BT_AVP_CONN               bt_avp_conn;
    P_BT_ROLESWAP_BT_AVP_DISCONN            bt_avp_disconn;
    P_BT_ROLESWAP_BT_ATT_CONN               bt_att_conn;
    P_BT_ROLESWAP_BT_ATT_DISCONN            bt_att_disconn;
    P_BT_ROLESWAP_CRTL_CONN                 ctrl_conn;
    P_BT_ROLESWAP_RECV                      recv;
    P_BT_ROLESWAP_CBACK                     cback;
    P_BT_ROLESWAP_START                     start;
    P_BT_ROLESWAP_CFM                       cfm;
    P_BT_ROLESWAP_STOP                      stop;
    P_BT_ROLESWAP_GET_PICONET_ID            get_piconet_id;
    P_BT_ROLESWAP_GET_PICONET_CLK           get_piconet_clk;
} T_BT_ROLESWAP_PROTO;

extern const T_BT_ROLESWAP_PROTO bt_relay_proto;
extern const T_BT_ROLESWAP_PROTO bt_sniffing_proto;

T_ROLESWAP_INFO *bt_roleswap_info_base_find(uint8_t bd_addr[6]);

T_ROLESWAP_INFO *bt_roleswap_info_base_alloc(uint8_t bd_addr[6]);

bool bt_roleswap_info_base_free(T_ROLESWAP_INFO *info);

bool bt_roleswap_sco_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_SCO_INFO *info);

bool bt_roleswap_info_send(uint8_t   module,
                           uint8_t   submodule,
                           uint8_t  *info,
                           uint16_t  len);

bool bt_roleswap_info_alloc(uint8_t   bd_addr[6],
                            uint8_t   type,
                            uint8_t  *info,
                            uint16_t  len);

void bt_roleswap_rfc_info_get(uint8_t  bd_addr[6],
                              uint8_t  dlci,
                              uint16_t uuid);

bool bt_roleswap_info_free(uint8_t          bd_addr[6],
                           T_ROLESWAP_DATA *data);

void bt_roleswap_spp_info_free(uint8_t bd_addr[6],
                               uint8_t local_server_chann);

void bt_roleswap_a2dp_info_free(uint8_t bd_addr[6]);

void bt_roleswap_avrcp_info_free(uint8_t bd_addr[6]);

void bt_roleswap_hfp_info_free(uint8_t bd_addr[6]);

void bt_roleswap_pbap_info_free(uint8_t bd_addr[6]);

void bt_roleswap_hid_device_info_free(uint8_t bd_addr[6]);

void bt_roleswap_hid_host_info_free(uint8_t bd_addr[6]);

void bt_roleswap_att_info_free(uint8_t bd_addr[6]);

void bt_roleswap_iap_info_free(uint8_t bd_addr[6]);

void bt_roleswap_avp_info_free(uint8_t bd_addr[6]);

void bt_roleswap_bt_rfc_info_free(uint8_t bd_addr[6],
                                  uint8_t local_server_chann);

void bt_roleswap_transfer(uint8_t bd_addr[6]);

void bt_roleswap_sync(uint8_t bd_addr[6]);

T_ROLESWAP_DATA *bt_roleswap_data_find(uint8_t bd_addr[6],
                                       uint8_t type);

T_ROLESWAP_DATA *bt_roleswap_l2c_find(uint8_t  bd_addr[6],
                                      uint16_t cid);

T_ROLESWAP_DATA *bt_roleswap_rfc_ctrl_find(uint8_t  bd_addr[6],
                                           uint16_t cid);

T_ROLESWAP_DATA *bt_roleswap_rfc_data_find(uint8_t bd_addr[6],
                                           uint8_t dlci);

T_ROLESWAP_DATA *bt_find_roleswap_rfc_data_by_cid(uint8_t  bd_addr[6],
                                                  uint16_t cid);

T_ROLESWAP_DATA *bt_roleswap_bt_rfc_find(uint8_t bd_addr[6],
                                         uint8_t local_server_chann);

T_ROLESWAP_DATA *bt_find_roleswap_bt_rfc_by_dlci(uint8_t bd_addr[6],
                                                 uint8_t dlci);

T_ROLESWAP_DATA *bt_roleswap_spp_find(uint8_t bd_addr[6],
                                      uint8_t local_server_chann);

T_ROLESWAP_DATA *bt_find_roleswap_spp_by_dlci(uint8_t bd_addr[6],
                                              uint8_t dlci);

bool bt_roleswap_l2c_cid_check(uint8_t  bd_addr[6],
                               uint16_t cid);

void bt_roleswap_l2c_info_free(uint8_t  bd_addr[6],
                               uint16_t cid);

bool bt_roleswap_acl_info_set(T_ROLESWAP_ACL_INFO *info);

bool bt_roleswap_sco_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_SCO_INFO *info);

bool bt_roleswap_rfc_ctrl_info_set(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_RFC_CTRL_INFO *info);

bool bt_roleswap_rfc_data_info_set(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_RFC_DATA_INFO *info);

#ifdef __cplusplus
}
#endif

#endif /*_BT_ROLESWAP_INT_H_*/
