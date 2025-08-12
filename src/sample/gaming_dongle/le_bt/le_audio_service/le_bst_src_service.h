#ifndef _LE_BST_SRC_SERVICE_H_
#define _LE_BST_SRC_SERVICE_H_

#include "ble_audio_def.h"
#include "bass_client.h"
#include "gap_le_types.h"
#include "broadcast_source_sm.h"
#include "ual_bluetooth.h"
#include "ual_dev_mgr.h"
#include "le_audio_service.h"
#include "codec_qos.h"
#include "app_cfg.h"

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define PRIMARY_ADV_INTERVAL_MIN          0x000020        //change ext adv interval to 30ms, BB2BUG-192
#define PRIMARY_ADV_INTERVAL_MAX          0xFFFFFF
#define DEFAULT_PRIMARY_ADV_INTERVAL_MIN  0x000030
#define DEFAULT_PRIMARY_ADV_INTERVAL_MAX  0x000030

#define PERIODIC_ADV_INTERVAL_MIN         0x000020
#define PERIODIC_ADV_INTERVAL_MAX         0xFFFFFF
#define DEFAULT_PERIODIC_ADV_INTERVAL_MIN 0x0000A0
#define DEFAULT_PERIODIC_ADV_INTERVAL_MAX 0x0000A0

#define PERIODIC_ADV_PROP                 0
#define SECONDARY_ADV_MAX_SKIP            0

#define BRS_AUDIO_CHAN_1_BIS_NUM        1
#define BRS_AUDIO_CHAN_2_BIS_NUM        2
#define BRS_AUDIO_CHAN_3_BIS_NUM        3
#define BRS_AUDIO_CHAN_4_BIS_NUM        4

#define AUDIO_LOCATION_LEFT               0x5451451
#define AUDIO_LOCATION_RIGHT              0xA8A28A2


void bs_handle_setup_data_path(T_BROADCAST_SOURCE_SETUP_DATA_PATH *p_data);
void bs_handle_rmv_data_path(T_BROADCAST_SOURCE_REMOVE_DATA_PATH *p_data);
void bs_handle_state_change(T_BROADCAST_SOURCE_STATE state);
bool bs_reconfig(uint16_t metadata);
bool bs_metaupdate(uint16_t metadata);
bool bs_establish(bool test_mode);
bool bs_disable(void);
bool bs_release(void);
bool bs_start_broadcast(uint8_t *code);
bool bs_stop_broadcast(void);
void bs_release_broadcast(void);
bool bs_rmv_snk_from_group(uint8_t *addr, bool remove_src);
bool bs_add_snk_to_group(uint8_t *addr);
bool bs_set_device_conf(uint8_t *addr, uint32_t chnl_location);
bool bs_set_track_codec(uint8_t index, uint8_t qos_type);
bool bs_start_sync(uint8_t *addr, uint8_t past, uint8_t prefer);
bool bs_stop_sync(uint8_t *addr);
void bs_update_brs_state(T_LE_AUDIO *p_le_audio, T_BASS_CLIENT_BRS_DATA *brs_data);
uint8_t bs_get_play_info(uint8_t *p_qos_type);
uint8_t le_audio_get_brs_state(void);
void bs_init(void);
bool bs_is_in_idle(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
