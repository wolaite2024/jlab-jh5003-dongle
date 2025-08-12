#ifndef _APP_SRC_AUDIO_H_
#define _APP_SRC_AUDIO_H_

#include "audio_track.h"

void app_src_uac_us_status(bool active);

void app_src_sbc_voice_start_capture(T_AUDIO_FORMAT_INFO p_format_info,
                                     P_AUDIO_TRACK_ASYNC_IO async_read);

void app_src_sbc_voice_stop_capture(void);

void app_usb_audio_msbc_fill_us(uint8_t *buf, uint16_t len, uint8_t flag);

bool app_usb_audio_is_us_streaming(void);

void app_usb_audio_music_create(T_AUDIO_FORMAT_INFO codec);

void app_usb_audio_music_destroy(void);

void app_usb_audio_msbc_ds_create(void);

void app_usb_audio_msbc_ds_destroy(void);

void app_usb_audio_msbc_us_create(void);

void app_usb_audio_msbc_us_destroy(void);

#endif
