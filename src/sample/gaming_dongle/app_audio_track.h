/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __APP_AUDIO_TRACK_H__
#define __APP_AUDIO_TRACK_H__


#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "audio_type.h"
#include "app_msg.h"

typedef enum
{
    APP_AUDIO_TRACK_EVENT_RELEASED,          /* audio track released event */
    APP_AUDIO_TRACK_EVENT_CREATED,           /* audio track created event */
    APP_AUDIO_TRACK_EVENT_STARTED,           /* audio track started event */
    APP_AUDIO_TRACK_EVENT_STOPPED,           /* audio track stopped event */
    APP_AUDIO_TRACK_EVENT_DATA_FILLED,       /* audio track filled data into DSP event */
    APP_AUDIO_TRACK_EVENT_DATA_IND,          /* audio track received data from DSP evet */
    APP_AUDIO_TRACK_EVENT_MUTED,             /* audio track volume muted event */
    APP_AUDIO_TRACK_EVENT_UNMUTED,           /* audio track volume unmuted event */
    APP_AUDIO_TRACK_EVENT_VOL_CHG,           /* audio track volume changed event */
} T_APP_AUDIO_TRACK_EVENT;


#define    IO_MSG_TYPE_AUDIO_TRACK 0xf3      /* audio track message type */

#define     MSG_TYPE_TRACK_ENC_RAW_RECV 1
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief   Initialize audio track module.
 *
 * \return                   The status of initializing audio track module.
 * \retval true              audio track module was initialized successfully.
 */
bool app_audio_track_init(void);

/**
 * \brief   Create an audio track.
 *
 * \param[in]  device        input/output terminal type.
 * \param[in]  info          audio format information.
 * \param[in]  mgr_cback     app audio track event callback function.
 * \param[in]  uid           Upper layer identification.
 *
 * \return                   The result of creating an audio track.
 * \retval true              audio track created successfully.
 * \retval false             audio track created failed.
 */
bool app_audio_track_create(uint32_t device, T_AUDIO_FORMAT_INFO *info,
                            void (*mgr_cback)(uint8_t id, uint8_t event, uint8_t *buf, uint32_t len),
                            uint8_t uid);

/**
 * \brief   Change audio track path id.
 *
 * \param[in]  device          input/output terminal type.
 * \param[in]  uid             path id.
 *
 * \return                     The result of changing audio track path id.
 * \retval true                audio track path id changed successfully.
 * \retval false               audio track path id changed failed.
 */
bool app_audio_track_change_uid(uint32_t device, uint8_t uid);

/**
 * \brief   Fill data into audio track .
 *
 * \param[in]  device          input/output terminal type.
 * \param[in]  buf             buffer write location.
 * \param[in]  flag            packet status flag.
 * \param[in]  len             data length.
 * \param[in]  frame_num       frame number.
 * \param[in]  timestamp       time stamp.
 *
 * \return                     The result of filling data into audio track.
 * \retval true                audio track filled data successfully.
 * \retval false               audio track filled data failed.
 */
bool app_audio_track_write_with_flag(uint32_t device, uint8_t *buf, uint8_t flag, uint16_t len,
                                     uint16_t frame_num, uint32_t timestamp);

/**
 * \brief   Release an audio track.
 *
 * \param[in]  device         input/output terminal type.
 *
 * \return                    void.
 */
void app_audio_track_release(uint32_t device);

/**
 * \brief   Set the audio track out volume.
 *
 * \param[in]  device         input/output terminal type.
 * \param[in]  value          the volume to be set.
 *
 * \return                    The result of setting audio track out volume.
 * \retval  true              set volume successfully.
 * \retval  false             set volume failed.
 */
bool app_audio_track_out_volume_change(uint32_t device, uint8_t value);

/**
 * \brief   Mute/Unmute audio track out.
 *
 * \param[in]  device         input/output terminal type.
 * \param[in]  enable         true: mute, false: unmute.
 *
 * \return                    The result of setting mute or unmute.
 * \retval  true              set audio track mute/unmute successfully.
 * \retval  false             set audio track mute/unmute failed.
 */
bool app_audio_track_out_volume_mute(uint32_t device, bool enable);

/**
 * \brief   Mute/Unmute audio track in.
 *
 * \param[in]  device         input/output terminal type.
 * \param[in]  enable         true: mute, false: unmute.
 *
 * \return                    The result of setting mute or unmute.
 * \retval  true              set audio track mute/unmute successfully.
 * \retval  false             set audio track mute/unmute failed.
 */
bool app_audio_track_in_volume_mute(uint32_t device, bool enable);

/**
 * \brief   vol up/down audio track i.
 *
 * \param[in]  device         input terminal type.
 * \param[in]  vol_up         true: up, false: down.
 *
 * \return                    The result of setting vol.
 * \retval  true              set audio track vol up/down successfully.
 * \retval  false             set audio track vol up/down failed.
 */
bool app_audio_track_in_volume_up_down(uint32_t device, bool vol_up);

#ifdef __cplusplus
}
#endif

#endif /* __APP_AUDIO_TRACK_H__ */
