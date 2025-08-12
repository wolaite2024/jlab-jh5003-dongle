/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _MULTIPRO_CTRLOR_H_
#define _MULTIPRO_CTRLOR_H_


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "multitopology_if.h"
/*============================================================================*
 *                              Constants
 *============================================================================*/
typedef enum
{
    MTC_BIS_TMR_SCAN,
    MTC_BIS_TMR_RESYNC,
    MTC_BIS_TMR_MAX,
} T_MTC_BIS_TMR;

typedef enum
{
    T_MTC_OTHER_TMR_LINKBACK_TOTAL,
    T_MTC_OTHER_TMR_MAX,
} T_MTC_OTHER_TMR;

typedef enum
{
    MTC_TMR_BIS,
    MTC_TMR_OTHER,

    MTC_TMR_MAX,
} T_MTC_TMR;

typedef enum
{
    LE_AUDIO_NO    = 0x00,
    LE_AUDIO_CIS     = 0x01,
    LE_AUDIO_BIS   = 0x02,
    LE_AUDIO_ALL   = 0x03,
} T_MTC_AUDIO_MODE;

typedef enum t_mtc_pro_beep
{
    MTC_PRO_BEEP_NONE,
    MTC_PRO_BEEP_PROMPTLY,
    MTC_PRO_BEEP_DELAY,
} T_MTC_PRO_BEEP;

typedef enum t_mtc_pro_key
{
    MULTI_PRO_BT_BREDR,
    MULTI_PRO_BT_CIS,
    MULTI_PRO_BT_BIS,
} T_MTC_BT_MODE;

typedef enum t_mtc_resume
{
    MULTI_RESUME_NONE,
    MULTI_RESUME_A2DP,
    MULTI_RESUME_CIS,
    MULTI_RESUME_BIS,
} T_MTC_RESUME_MODE;

typedef enum t_multi_pro_sniffing_ststus
{
    MULTI_PRO_SNIFI_NOINVO = 0x0,
    MULTI_PRO_SNIFI_INVO,
} T_MTC_SNIFI_STATUS;

typedef enum
{
    MTC_BUD_STATUS_NONE      = 0x00,
    MTC_BUD_STATUS_SNIFF      = 0x01,
    MTC_BUD_STATUS_ACTIVE      = 0x02,

    MTC_BUD_STATUS_TOTAL,
} T_MTC_BUD_STATUS;

typedef enum
{
    MULTI_PRO_REMOTE_RELAY_STATUS_ASYNC_RCVD          = 0x00,
    MULTI_PRO_REMOTE_RELAY_STATUS_ASYNC_LOOPBACK      = 0x01,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_RCVD           = 0x02,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_TOUT           = 0x03,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_EXPIRED        = 0x04,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_LOOPBACK       = 0x06,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_REF_CHANGED    = 0x07,
    MULTI_PRO_REMOTE_RELAY_STATUS_SYNC_SENT_OUT       = 0x08,
    MULTI_PRO_REMOTE_RELAY_STATUS_ASYNC_SENT_OUT      = 0x09,
    MULTI_PRO_REMOTE_RELAY_STATUS_SEND_FAILED         = 0x0A,
} T_MULTI_PRO_REMOTE_RELAY_STATUS;

typedef enum
{
    MTC_GAP_VENDOR_NONE    = 0x00,
    MTC_GAP_VENDOR_INIT    = 0x01,
    MTC_GAP_VENDOR_ADV,
    MTC_GAP_VENDOR_TOTAL,

} T_MTC_GAP_VENDOR_MODE;

typedef enum
{
    MTC_EVENT_BIS_SCAN_TO                       = 0x00,
    MTC_EVENT_BIS_RESYNC_TO,
    MTC_EVENT_LEGACY_PAIRING_TO,
    MTC_EVENT_MAX,
} T_MTC_EVENT;

typedef enum
{
    MTC_TOPO_EVENT_A2DP_START      = 0x00,
    MTC_TOPO_EVENT_CIS_ENABLE      = 0x01,
    MTC_TOPO_EVENT_HFP_CALL        = 0x02,
    MTC_TOPO_EVENT_CCP_CALL        = 0x03,
    MTC_TOPO_EVENT_LEA_CONN        = 0x04,
    MTC_TOPO_EVENT_LEA_DISCONN     = 0x05,
    MTC_TOPO_EVENT_LEGACY_CONN     = 0x06,
    MTC_TOPO_EVENT_LEGACY_DISCONN  = 0x07,
    MTC_TOPO_EVENT_BIS_START       = 0x08,
    MTC_TOPO_EVENT_BIS_STOP        = 0x09,
    MTC_TOPO_EVENT_LEA_ADV_STOP    = 0x0A,
    MTC_TOPO_EVENT_BR_ROLE_SWAP    = 0x0B,
    MTC_TOPO_EVENT_CIS_STREAMING   = 0x0C,
    MTC_TOPO_EVENT_SET_BR_MODE     = 0x0D,
    MTC_TOPO_EVENT_CIS_TERMINATE   = 0x0E,
    MTC_TOPO_EVENT_BIS_STREAMING   = 0x0F,
    MTC_TOPO_EVENT_TOTAL,
} T_MTC_TOPO_EVENT;

typedef enum
{
    MTC_LINK_ACT_LEA_PRIO_HIGH     = 0x00,
    MTC_LINK_ACT_LEGACY_PRIO_HIGH  = 0x01,
    MTC_LINK_ACT_LAST_ONE          = 0x02,
    MTC_LINK_ACT_NONE              = 0x03,
    MTC_LINK_ACTIVE_TOTAL,

} T_MTC_LINK_ACT;

typedef enum
{
    MTC_MODE_IDLE                   = 0x00,
    MTC_MODE_BREDR_IDLE             = 0x01,
    MTC_MODE_BREDR_PLAYBACK         = 0x02,
    MTC_MODE_BREDR_VOICE            = 0x04,
    MTC_MODE_CIS_IDLE               = 0x08,
    MTC_MODE_CIS_PLAYBACK           = 0x10,
    MTC_MODE_CIS_VOICE              = 0x20,
    MTC_MODE_BIS_IDLE               = 0x40,
    MTC_MODE_BIS_PLAYBACK           = 0x80,
} T_MTC_ACTIVE_MODE;

#define MTC_MODE_GROUP_BREDR    (MTC_MODE_BREDR_IDLE|MTC_MODE_BREDR_PLAYBACK|MTC_MODE_BREDR_VOICE)
#define MTC_MODE_GROUP_CIS      (MTC_MODE_CIS_IDLE|MTC_MODE_CIS_PLAYBACK|MTC_MODE_CIS_VOICE)
#define MTC_MODE_GROUP_BIS      (MTC_MODE_BIS_IDLE|MTC_MODE_BIS_PLAYBACK)
#define MTC_MODE_GROUP_BIS_SD   (MTC_MODE_CIS_IDLE|MTC_MODE_BIS_PLAYBACK)

#define MTC_MODE_STREAM_TYPE_IDLE       (MTC_MODE_BREDR_IDLE|MTC_MODE_CIS_IDLE|MTC_MODE_BIS_IDLE)
#define MTC_MODE_STREAM_TYPE_PLAYBACK   (MTC_MODE_BREDR_PLAYBACK|MTC_MODE_CIS_PLAYBACK|MTC_MODE_BIS_PLAYBACK)
#define MTC_MODE_STREAM_TYPE_VOICE      (MTC_MODE_BREDR_VOICE|MTC_MODE_CIS_VOICE)



typedef struct
{
    uint8_t msg_type;
    uint8_t *buf;
    uint16_t len;
    T_MULTI_PRO_REMOTE_RELAY_STATUS status;
} T_RELAY_PARSE_PARA;


/*============================================================================*
 *                              Functions
 *============================================================================*/
typedef void (*P_MTC_RELAY_PARSE_CB)(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                     T_MULTI_PRO_REMOTE_RELAY_STATUS status);
typedef uint8_t MTC_GAP_HANDLER;

void mtc_start_timer(T_MTC_TMR tmt, uint8_t opt, uint32_t timeout);
void mtc_stop_timer(T_MTC_TMR tmt);
bool mtc_exist_handler(T_MTC_TMR tmt);

T_MTC_ACTIVE_MODE mtc_get_active_mode(void);

void mtc_state_bredr_transit_handler(T_MTC_ACTIVE_MODE state, uint8_t *param);
void mtc_state_cis_transit_handler(T_MTC_ACTIVE_MODE state, uint8_t *param);
void mtc_state_bis_transit_handler(T_MTC_ACTIVE_MODE state, uint8_t *param);

T_MTC_BUD_STATUS mtc_get_b2d_sniff_status(void);
T_MTC_BUD_STATUS mtc_get_b2s_sniff_status(void);
void mtc_set_pending(T_MTC_AUDIO_MODE action);
void mtc_set_resume_a2dp_idx(uint8_t index);
bool mtc_stream_switch(bool iscall);
uint8_t mtc_get_resume_a2dp_idx(void);
void mtc_pro_hook(uint8_t hook_point, T_RELAY_PARSE_PARA *info);
void mtc_set_beep(T_MTC_PRO_BEEP para);
T_MTC_PRO_BEEP mtc_get_beep(void);
void mtc_set_btmode(T_MTC_BT_MODE para);
void mtc_set_active_bt_mode(T_MTC_BT_MODE para);
T_MTC_BT_MODE mtc_get_btmode(void);
T_MTC_SNIFI_STATUS mtc_get_sniffing(void);
void  mtc_set_sniffing(T_MTC_SNIFI_STATUS para);
void mtc_set_dev(uint8_t para);
bool mtc_term_sd_sniffing(void);
bool mtc_check_call_sate(void);
void mtc_check_reopen_mic(void);

void mtc_legacy_update_call_status(uint8_t *call_status_old);
bool mtc_is_lea_cis_stream(uint8_t *set_mcp);
bool mtc_device_poweroff_check(T_MTC_EVENT event);
bool mtc_topology_dm(uint8_t event);

uint8_t mtc_gap_set_pri(T_MTC_GAP_VENDOR_MODE para);
uint8_t mtc_gap_callback(uint8_t cb_type, void *p_cb_data);
void mtc_gap_handle_state_evt_callback(uint8_t new_state, uint16_t cause);
bool mtc_gap_is_ready(void);
bool mtc_check_lea_le_link(void);
bool mtc_ase_release(void);
void mtc_cis_audio_conext_change(bool enable);
void mtc_init(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
