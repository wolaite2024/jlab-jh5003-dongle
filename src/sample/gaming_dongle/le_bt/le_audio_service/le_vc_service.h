#ifndef _LE_VC_SERVICE_H_
#define _LE_VC_SERVICE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "profile_server.h"
#include "vocs_client.h"
#include "vcs_client.h"
#include "gap.h"
#include "ble_audio.h"

bool le_audio_handle_remote_volume_change(uint16_t conn_handle, uint8_t volume_setting,
                                          uint8_t mute);
bool le_audio_handle_remote_volume_offset_change(uint16_t conn_handle,
                                                 T_VOCS_SRV_DATA *p_vocs_data);
bool le_audio_set_device_volume_offset(uint8_t *bd_addr, uint8_t idx, int16_t offset);
bool le_audio_set_remote_volume(uint8_t *bd_addr, uint8_t volume_setting);
bool le_audio_set_remote_mute(uint8_t *bd_addr, uint8_t mute);
uint16_t le_audio_aics_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
bool le_audio_set_device_input_gain(uint8_t *bd_addr, uint8_t idx, int8_t gai_setting);
bool le_audio_set_device_input_mute(uint8_t *bd_addr, uint8_t idx, uint8_t mute);
bool le_audio_set_device_input_gain_mode(uint8_t *bd_addr, uint8_t idx, uint8_t gain_mode);
uint16_t le_audio_vocs_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
uint16_t le_audio_vcs_handle_msg(T_LE_AUDIO_MSG msg, void *buf);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
