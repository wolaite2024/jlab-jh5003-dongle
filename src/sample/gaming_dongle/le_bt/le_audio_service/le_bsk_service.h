#ifndef _LE_BSK_SERVICE_H_
#define _LE_BSK_SERVICE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio_sync.h"
#include "ble_audio_def.h"
#include "bass_client.h"
#include "gap_le_types.h"
#include "le_audio_service.h"
#include "base_data_parse.h"


void bsnk_handle_big_sync_state(void *p_buf);
void bsnk_handle_setup_data_path_cmplt(void *p_buf);
void bsnk_handle_rmv_data_path_cmplt(void *p_buf);

bool bsnk_big_establish(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3],
                        uint8_t num_bis, uint8_t *p_bis);
bool bsnk_big_terminate(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                        uint8_t broadcast_id[3]);
bool bsnk_big_release(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3]);
void le_bsnk_setup_sink_audio(void);
void le_bsnk_remove_sink_audio(void);
void bsnk_select_broadcast_src(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                               uint8_t broadcast_id[3], uint8_t *code);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
