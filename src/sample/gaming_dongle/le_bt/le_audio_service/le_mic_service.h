#ifndef _LE_MIC_SERVICE_H_
#define _LE_MIC_SERVICE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "profile_server.h"
#include "mics_client.h"
#include "gap.h"
#include "ble_audio.h"

uint16_t le_mic_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
bool le_mic_set_remote_mute(uint8_t *addr, uint8_t mute_value);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
