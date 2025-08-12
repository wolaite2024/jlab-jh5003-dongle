#ifndef _LE_VCS_SERVICE_H_
#define _LE_VCS_SERVICE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "profile_server.h"
#include "gap.h"
#include "ble_audio.h"

#if LE_AUDIO_VCS_SUPPORT
void le_vcs_set_aux_record_state(bool enable);
void le_vcs_set_aux_track_state(bool enable);
void le_vcs_set_bluetooth_out_state(bool enable);
void le_vcs_usb_set_out_volume(uint16_t value);
void le_vcs_set_in_volume(uint16_t value);
uint16_t le_vcs_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
uint16_t le_vocs_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
uint16_t le_aics_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
uint16_t le_mics_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
void le_vcs_audio_set_input_status(uint8_t iterminal, bool enable);
void le_vcs_audio_set_input_mute(uint8_t iterminal, bool enable);
void le_vcs_audio_set_output_mute(uint8_t oterminal, bool enable);
void le_vcs_audio_set_output_vol_value(uint8_t oterminal, uint8_t volume);

void le_vcs_init_path_gain(void);

void le_vcs_init(void);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
