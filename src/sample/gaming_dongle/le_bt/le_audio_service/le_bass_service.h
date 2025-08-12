#ifndef _LE_BASS_SERVICE_H_
#define _LE_BASS_SERVICE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "ble_audio_sync.h"
#include "ble_audio_def.h"
#include "gap_le_types.h"
#include "le_audio_service.h"
#include "base_data_parse.h"

#if LE_AUDIO_BASS_SUPPORT
void le_bass_start_adv(void);
void le_bass_stop_adv(void);

void le_bass_setup_sink_audio(void);
void le_bass_remove_sink_audio(void);
uint16_t le_audio_bass_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
void le_audio_bass_init(void);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
