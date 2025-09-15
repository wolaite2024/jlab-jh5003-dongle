#ifndef _LE_AUDIO_SERVICE_H_
#define _LE_AUDIO_SERVICE_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include <gap_conn_le.h>
#include <profile_client.h>
#include "app_gaming_sync.h"
#include "app_usb_uac.h"
#include "csis_rsi.h"
#include "ble_audio_def.h"
#include "vector.h"
#include "os_queue.h"
#include "csis_client.h"
#include "profile_server_def.h"
#include "ual_bluetooth.h"
#include "ble_audio_group.h"
#include "bass_client.h"
#include "bap.h"
#include "ble_audio.h"
#include "app_ctrl_pkt_policy.h"

#define LE_AUDIO_PACS_FLAG         0x0001
#define LE_AUDIO_ASCS_FLAG         0x0002
#define LE_AUDIO_VCS_FLAG          0x0008
#define LE_AUDIO_VOCS_FLAG         0x0010
#define LE_AUDIO_CSIS_FLAG         0x0020
#define LE_AUDIO_BASS_FLAG         0x0040
#define LE_AUDIO_MCS_SERV_FLAG     0x0080
#define LE_AUDIO_OTS_SERV_FLAG     0x0100
#define LE_AUDIO_MICS_FLAG         0x0200
#define LE_AUDIO_TBS_FLAG          0x0400
#define LE_AUDIO_TMAS_FLAG         0x0800

#define LE_AUDIO_PACS_CLIENT_FLAG  0x0001
#define LE_AUDIO_ASCS_CLIENT_FLAG  0x0002
#define LE_AUDIO_VCS_CLIENT_FLAG   0x0004
#define LE_AUDIO_VOCS_CLIENT_FLAG  0x0008
#define LE_AUDIO_AICS_CLIENT_FLAG  0x0010
#define LE_AUDIO_CSIS_CLIENT_FLAG  0x0020
#define LE_AUDIO_BASS_CLIENT_FLAG  0x0040
#define LE_AUDIO_MCS_CLIENT_FLAG   0x0080
#define LE_AUDIO_OTS_CLIENT_FLAG   0x0100
#define LE_AUDIO_MICS_CLIENT_FLAG  0x0200
#define LE_AUDIO_TBS_CLIENT_FLAG   0x0400
#define LE_AUDIO_TMAP_CLIENT_FLAG  0x0800
/*============================================================================*
 *                              Constants
 *============================================================================*/
#define LE_AUDIO_APP_ID                11
#define LE_AUDIO_CSIS_CLIENT_APP_ID    12

#define BASS_BRS_NUM_MAX    2

#define LE_AUDIO_SINK_NONE            0
#define LE_AUDIO_SINK_FROM_LOCAL      1
#define LE_AUDIO_SINK_FROM_BASS       2

#define BROADCAST_NAME_MAX          32

#define APP_CONTENT_CONTROL_ID_CHAR_VALUE_GTBS  0x01

#define APP_CONTENT_CONTROL_ID_CHAR_VALUE_GMCS  0x20

typedef enum
{
    AUDIO_ROLE_UNKNOWN = 0x00,
    AUDIO_ROLE_SINK = 0x01,
    AUDIO_ROLE_SOURCE = 0x02,
    AUDIO_ROLE_BOTH = 0x03,
} T_AUDIO_ROLE;

typedef struct t_brs_info
{
    uint8_t                     source_id;
    uint8_t                     adv_addr[BD_ADDR_LEN];
    uint8_t                     adv_addr_type;
    uint8_t                     advertiser_sid;
    uint8_t                     broadcast_id[3];
    T_PA_SYNC_STATE             pa_sync_state;
    uint32_t                    bis_sync_state;
    bool                        need_remove;
} T_BRS_INFO;

typedef struct t_brs_sync_info
{
    bool                    pending;
    uint8_t                 bd_type;
    uint8_t                 bd_addr[BD_ADDR_LEN];
    uint32_t                bis_array;
} T_BRS_SYNC_INFO;


typedef struct
{
    bool                used;
    T_BT_CONN_STATE     state;
    uint8_t             audio_state;
    uint8_t             conn_id;
    uint16_t            conn_handle;
    uint16_t            mtu_size;
    uint16_t            remote_serv_sup;
    uint8_t             target_latency;
#if LE_AUDIO_BASS_CLIENT_SUPPORT
    uint8_t             bass_brs_num;
    T_BRS_INFO          brs_cb[BASS_BRS_NUM_MAX];
    T_BRS_SYNC_INFO     brs_pending_sync;
#endif
#if LE_AUDIO_PACS_CLIENT_SUPPORT
    uint16_t            snk_sup_context;
    uint16_t            src_sup_context;
    uint16_t            snk_avail_context;
    uint16_t            src_avail_context;
    uint8_t             snk_pac_num;
    uint8_t             src_pac_num;
    uint32_t            snk_audio_loc;
    uint32_t            src_audio_loc;
#endif

#if LE_AUDIO_ASCS_CLIENT_SUPPORT
    uint8_t             snk_ase_num;
    uint8_t             src_ase_num;
#endif

#if LE_AUDIO_VCS_CLIENT_SUPPORT
    bool                vcs_marked;
#endif

#if LE_AUDIO_VOCS_CLIENT_SUPPORT
    uint8_t             srv_total_num;
#endif

#if LE_AUDIO_AICS_CLIENT_SUPPORT
    uint8_t             aics_num;
#endif

#if LE_AUDIO_MICS_CLIENT_SUPPORT
    uint8_t             mic_mute;
#endif

#if LE_AUDIO_CSIS_CLIENT_SUPPORT
    uint8_t             csis_disc_done;
    uint8_t             csis_size;
    uint8_t             csis_ready_result;
#endif

    uint16_t            supported_services;
    T_UALIST_HEAD       list;
    uint8_t             cap_disc_done;
    uint8_t             bap_disc_all_done;
#if DONGLE_TRANSMIT_CLIENT_SUPPORT
    uint8_t                tx_event_seqn;
    uint8_t                link_id;
    T_EARBUD_SIDE          bud_side;
#endif
} T_LE_AUDIO;

typedef struct
{
    uint8_t                 bis_index;
    uint8_t                 sample_frequency;
    uint32_t                chnnl_allocation;
} T_BROADCAST_SRC_BIS_PARAMS;


typedef struct
{
    uint8_t                         subgroup_idx;
    uint8_t                         num_bis;
    uint8_t                         metadata_len;
    uint8_t                         metadata[40];      //FIX TODO
    T_BROADCAST_SRC_BIS_PARAMS      bis_param[4];   //FIX TODO
} T_BROADCAST_SRC_GRP_PARAMS;

typedef struct
{
    uint8_t                     adv_addr[BD_ADDR_LEN];
    uint8_t                     adv_addr_type;
    uint8_t                     advertiser_sid;
    uint8_t                     num_subgroups;
    uint8_t                     broadcast_id[3];
    int8_t                      rssi;
    uint8_t                     feature;
    uint8_t                     pbp_len;
    uint8_t                    *pbp_ltv;
    uint8_t                     broadcast_name_len;
    uint8_t                     broadcast_name[BROADCAST_NAME_MAX];
    T_BROADCAST_SRC_GRP_PARAMS  bst_groups[4];           //FIX TODO
} T_BROADCAST_SRC_INFO;


T_LE_AUDIO *ble_audio_find_by_conn_id(uint8_t conn_id);
T_LE_AUDIO *ble_audio_find_by_conn_handle(uint16_t conn_handle);

#if DONGLE_TRANSMIT_CLIENT_SUPPORT
T_LE_AUDIO *ble_audio_find_by_link_id(uint8_t link_id);
T_LE_AUDIO *ble_audio_find_by_bud_side(uint8_t bud_side);
#endif
void ble_audio_get_pacs_info(T_BAP_DIS_ALL_DONE *p_data);
void ble_audio_update_pacs_info(T_BAP_PACS_NOTIFY *p_data);

void ble_audio_conn_state_change(uint8_t conn_id, T_BT_CONN_STATE new_state);
void ble_update_brs_state(T_LE_AUDIO *p_ble_audio, T_BASS_CLIENT_BRS_DATA *brs_data);
void ble_audio_set_broadcast_mode(bool enable, T_AUDIO_ROLE role);
void ble_audio_set_unicast_mode(bool enable, T_AUDIO_ROLE role);
uint8_t ble_audio_get_mode(void);
void ble_audio_route_sink_dev(uint8_t sink_from, bool enable);
void ble_audio_handle_conn_update_for_headset_stream_status_change(T_HEADSET_STATUS
                                                                   *headset_status);
void ble_audio_mgr_init(void *evt_queue, void *io_queue);
T_UALIST_HEAD *ble_audio_get_conn_list(void);
uint16_t le_audio_handle_cap_msg(T_LE_AUDIO_MSG msg, void *buf);
#endif
