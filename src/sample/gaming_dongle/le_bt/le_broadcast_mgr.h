#ifndef _LE_BROADCAST_MGR_H_
#define _LE_BROADCAST_MGR_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "bap.h"
#include "gap_big_mgr.h"
#include "ble_audio_sync.h"
#include "gap_pa_sync.h"
#include "base_data_parse.h"
#include "bass_client.h"
#include "bass_def.h"
#include "ual_dev_mgr.h"
#include "broadcast_source_sm.h"

#define BAP_BA_BSNK_SYNC_BSRC_NONE      0

#define BAP_BSNK_PA_SYNC_BSRC_MSK      1
#define BAP_BA_PA_SYNC_BSRC_MSK        2

#define BAP_BSNK_BIG_SYNC_BSRC_MSK      1
#define BAP_BA_big_SYNC_BSRC_MSK        2

#if LE_AUDIO_BROADCAST_SOURCE_ROLE
typedef struct
{
    T_BROADCAST_SOURCE_STATE old_state;
    T_BROADCAST_SOURCE_STATE new_state;
} BAP_BS_SRC_STATE_CHANGE;

T_BROADCAST_SOURCE_HANDLE bap_bsrc_create_handle(void);
bool bap_bsrc_config(T_BLE_BD_TYPE local_addr_type, uint8_t adv_sid, uint8_t group_idx,
                     T_BROADCAST_SOURCE_HANDLE handle,
                     uint32_t ea_min, uint32_t ea_max,
                     uint16_t pa_min, uint16_t pa_max,
                     T_QOS_CFG_TYPE qos, uint8_t encrypted);
bool bap_bsrc_reconfig(uint8_t group_idx, uint8_t subgroup_idx, uint8_t metadata_len,
                       uint8_t *p_metadata);
bool bap_bsrc_metaupdate(uint8_t group_idx, uint8_t subgroup_idx, uint8_t metadata_len,
                         uint8_t *p_metadata);
bool bap_bsrc_establish(uint8_t group_idx, uint8_t  retransmission_number, uint32_t sdu_interval,
                        uint16_t max_sdu,  uint8_t phy, uint8_t packing,
                        uint8_t framing, uint16_t max_transport_latency, uint8_t *broadcast_code);
bool bap_bsrc_test_establish(uint8_t group_idx, uint8_t  retransmission_number,
                             uint32_t sdu_interval,
                             uint16_t iso_interval, uint8_t nse, uint16_t max_sdu,
                             uint16_t max_pdu, uint8_t phy, uint8_t packing,
                             uint8_t framing, uint8_t bn, uint8_t irc,
                             uint8_t pto, uint8_t *broadcast_code);
bool bap_bsrc_disable(uint8_t group_idx, uint8_t reason);
bool bap_bsrc_release(uint8_t group_idx);
bool bap_bsrc_get_adv_info(uint8_t group_idx, uint8_t *adv_sid, uint8_t *broadcast_id);
bool bap_bsrc_get_random_address(uint8_t group_idx, uint8_t *p_random_addr);

#endif

#if (LE_AUDIO_BROADCAST_ASSISTANT_ROLE | LE_AUDIO_BROADCAST_SINK_ROLE)
typedef struct t_bap_bc_src_info
{
    struct t_bap_bc_src_info    *p_next;
    bool                        selected;
    bool                        local_src;
    uint8_t                     broadcast_code[BROADCAST_CODE_LEN];
    uint8_t                     adv_addr[GAP_BD_ADDR_LEN];
    uint8_t                     adv_addr_type;
    uint8_t                     advertiser_sid;
    uint8_t                     broadcast_id[3];
    T_BLE_AUDIO_SYNC_HANDLE     sync_handle;
    T_GAP_PA_SYNC_STATE                       pa_sync_state;
    T_GAP_BIG_SYNC_RECEIVER_SYNC_STATE        big_sync_state;
    uint8_t                                   peri_sync_id;
    uint16_t                                  peri_sync_handle;
    T_LE_BIGINFO_ADV_REPORT_INFO              big_info;
    T_BASE_DATA_MAPPING                       *p_mapping;
    int                         scan_client_id;
    uint8_t                     pa_sync_mask;
    uint8_t                     big_sync_mask;
    uint8_t                     iso_num_bis;
    uint8_t                     iso_bis[GAP_BIG_MGR_MAX_BIS_NUM];
} T_BAP_BC_SRC_INFO;

typedef struct
{
    T_BAP_BC_SRC_INFO           *p_bc_source;

    uint8_t                     bis_idx;
    uint16_t                    bis_conn_handle;
    uint16_t                    cause;
} T_BAP_BC_SRC_SETUP_PATH_CMPLT;

typedef struct
{
    T_BAP_BC_SRC_INFO           *p_bc_source;

    uint8_t                     bis_idx;
    uint16_t                    bis_conn_handle;
    uint16_t                    cause;
} T_BAP_BC_SRC_RMV_PATH_CMPLT;

void bap_ba_bsnk_mgr_clear_src_dev_list(void);
bool bap_ba_bsnk_add_src_dev(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                             uint8_t broadcast_id[3], bool local_src);
bool bap_ba_bsnk_rmv_src_dev(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                             uint8_t broadcast_id[3]);
void bap_ba_bsnk_set_broadcast_code(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                                    uint8_t *code, uint8_t broadcast_id[3]);

T_BAP_BC_SRC_INFO *bap_ba_bsnk_find_bst_dev(uint8_t *bd_addr, uint8_t bd_type,
                                            uint8_t adv_sid, uint8_t broadcast_id[3]);

#if (LE_AUDIO_BROADCAST_SINK_ROLE)
bool bap_bsnk_pa_sync(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3],
                      uint16_t skip);
bool bap_bsnk_pa_terminate(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                           uint8_t broadcast_id[3]);
bool bap_bsnk_pa_sync_release(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                              uint8_t broadcast_id[3]);
bool bap_bsnk_sync_big_establish(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                                 uint8_t broadcast_id[3],
                                 uint8_t num_bis, uint8_t *p_bis);
bool bap_bsnk_sync_big_terminate(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                                 uint8_t broadcast_id[3]);
bool bap_bsnk_setup_data_path(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                              uint8_t broadcast_id[3],
                              uint8_t num_bis, uint8_t *p_bis);
bool bap_bsnk_remove_data_path(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                               uint8_t broadcast_id[3],
                               uint8_t num_bis, uint8_t *p_bis);
#endif

#if LE_AUDIO_BROADCAST_ASSISTANT_ROLE
void bap_ba_sync_broadcast_src(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                               uint8_t broadcast_id[3], uint8_t *code);
#endif

#if LE_AUDIO_BASS_CLIENT_SUPPORT
bool bap_ba_remote_scanning(uint16_t conn_handle, bool scan_enable);
bool bap_ba_add_src(uint16_t conn_handle, T_BASS_PA_SYNC pa_sync, uint32_t bis_array);
bool bap_ba_modify_src(uint16_t conn_handle, uint8_t source_id, T_BASS_PA_SYNC pa_sync,
                       uint32_t bis_array);
bool bap_ba_remove_src(uint16_t conn_handle, uint8_t source_id);
bool bap_ba_set_broadcast_code(uint16_t conn_handle, uint8_t *bd_addr, uint8_t bd_type,
                               uint8_t adv_sid,
                               uint8_t broadcast_id[3],
                               uint8_t source_id, uint8_t *code);
void bap_ba_handle_bass_sync_request(T_BASS_CLIENT_SYNC_INFO_REQ *sync_req_data);
void bap_ba_handle_bass_brs_state(T_BASS_CLIENT_BRS_DATA *brs_data);
#if LE_AUDIO_BROADCAST_SOURCE_ROLE
bool bap_ba_add_local_src(uint16_t conn_handle, uint8_t group_idx, T_BASS_PA_SYNC pa_sync,
                          uint32_t bis_array);
bool bap_ba_modify_local_src(uint16_t conn_handle, uint8_t group_idx, uint8_t source_id,
                             T_BASS_PA_SYNC pa_sync, uint32_t bis_array);
#endif

#endif

#endif

bool le_broadcat_mgr_init(uint8_t role_mask);

#endif
