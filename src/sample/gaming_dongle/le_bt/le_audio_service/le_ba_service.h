#ifndef _LE_BA_SERVICE_H_
#define _LE_BA_SERVICE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio_sync.h"
#include "ble_audio_def.h"
#include "bass_client.h"
#include "gap_le_types.h"
#include "le_audio_service.h"
#include "base_data_parse.h"

typedef struct t_bc_src_info
{
    struct t_bc_src_info        *p_next;
    uint8_t                     adv_addr[BD_ADDR_LEN];
    uint8_t                     adv_addr_type;
    uint8_t                     advertiser_sid;
    uint8_t                     broadcast_id[3];
} T_BC_SRC_INFO;

void ba_bsnk_handle_bst_src_info(void *p_source);
void ba_bsnk_handle_src_sync_state(void *p_source);
bool ba_pa_sync(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3]);
bool ba_pa_terminate(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3]);
void ba_select_broadcast_src(uint8_t *bd_addr, uint8_t bd_type,
                             uint8_t adv_sid, uint8_t broadcast_id[3], uint8_t *code);
bool ba_add_device(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                   uint8_t broadcast_id[3]);
bool ba_start_remote_sync(uint8_t remote_addr_type, uint8_t *remote_addr, uint32_t bis_array);
uint8_t ba_start_le_scan(void);
void ba_stop_le_scan(void);
void ba_service_init(void);
void ba_handle_brs_state(T_LE_AUDIO *p_ble_audio, T_BASS_CLIENT_BRS_DATA *p_brs_data);
bool ba_stop_remote_sync(uint8_t *remote_addr, uint8_t reciver_id);
bool ba_modify_remote_sync(uint8_t *remote_addr, uint8_t reciver_id, uint32_t bis_array);

#if UAL_CONSOLE_PRINT
T_BC_SRC_INFO *ba_bsnk_find_bst_dev_by_idx(uint8_t idx);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
