/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */

#ifndef __APP_AUDIO_PATH_H__
#define __APP_AUDIO_PATH_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "audio_type.h"
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define IO_MSG_TYPE_APP_AUDIO_PATH  0xF4     /* audio path message type */

/**
 * @brief Audio path input terminal type
 *
 */
enum iterminal_type
{
    IT_SBC = 0x00,        /* SBC */
    IT_LC3FRM,            /* LC3 */
    IT_MIC,               /* MIC */
    IT_AUX,               /* AUX */
    IT_UDEV_IN1,          /* USB isoc out */
    IT_UDEV_IN2,          /* UAC2 */
    IT_RELAY_CODE1,       /* For relaying encoded frames from one path to another. */
    IT_MSBC,              /* MSBC */
    IT_MAX,
};

/**
 * @brief Audio path output terminal type
 *
 */
enum oterminal_type
{
    OT_SBC = 0x00,        /* SBC */
    OT_SBC2,              /* SBC2, used for dual uac */
    OT_LC3FRM,            /* LC3 */
    OT_LC3FRM2,           /* LC3 2, used for dual uac */
    OT_SPK,               /* SPEAKER */
    OT_AUX,               /* AUX */
    OT_UDEV_OUT1,         /* usb isoc in */
    OT_RELAY_CODE1,       /* For relaying decoded frames from one path to another. */
    OT_MSBC,              /* MSBC */
    OT_MAX,
};

/**
 * @brief Audio path state
 *
 */
enum uapi_audio_event
{
    EVENT_AUDIO_PATH_CREATED,            /* audio path is ready(low level audio pipe or track started) */
    EVENT_AUDIO_PATH_READY,            /* audio path is ready(low level audio pipe or track started) */
    EVENT_AUDIO_PATH_RELEASED,         /* audio path is released */
    EVENT_AUDIO_PATH_DATA_IND,         /* audio path received data(the encoded or decoded data is ready) */
    EVENT_AUDIO_PATH_STREAM_STARTED,   /* audio path stream started(received first audio pipe or track packet) */
    EVENT_AUDIO_PATH_STREAM_STOPPED,   /* audio path stream stopped */
};

/**
 * @brief Audio path codec type
 *
 */
enum app_audio_path_codec
{
    AUDIO_PATH_CODEC_NONE,      /* audio path none */
    AUDIO_PATH_CODEC_ENC,       /* audio path encode */
    AUDIO_PATH_CODEC_DEC,       /* audio path decode */
    AUDIO_PATH_CODEC_MAX,
};

/**
 * @brief Audio path monitor events, defined for user who wants to monitor the state of all audio paths.
 *
 */
enum audio_monitor_event
{
    EVENT_AUDIO_CREATED           = 0,       /* audio pipe or track created */
    EVENT_AUDIO_STARTED           = 1,       /* audio pipe or track started */
    EVENT_AUDIO_MUTED             = 2,       /* audio pipe or track muted */
    EVENT_AUDIO_UNMUTED           = 3,       /* audio pipe or track unmuted */
    EVENT_AUDIO_VOLUME_CHANGED    = 4,       /* audio pipe or tack volume changed */
    EVENT_AUDIO_STOPPED           = 5,       /* audio pipe or track stopped */
    EVENT_AUDIO_RELEASED          = 6,       /* audio pipe or track released */
};

/**
 * @brief  Message header associated with any above monitor event.
 *
 */
struct audio_monitor_msghdr
{
    uint8_t path_id;        /* path id */
    uint8_t it;             /* input terminal type */
    uint8_t ot;             /* output terminal type */
    uint8_t codec_type;     /* codec type: encode or decode */
};

/**
 * app_audio_path.h
 *
 * \brief  Define callback function for audio path state event.
 *
 */
typedef bool (*t_audio_uapi_usr_cback)(uint8_t id, uint8_t event, void *buf,
                                       uint16_t len, uint16_t frm_num);

/**
 * app_audio_path.h
 *
 * \brief  Define callback function for audio path monitor event.
 */
typedef void (*t_audio_monitor_cback)(uint8_t event, void *buf, uint16_t len);

/**
 * @brief  Define the parameters for creating a new audio path.
 *
 */
struct path_iovec
{
    uint8_t it;                          /* input terminal type */
    uint8_t ot;                          /* output terminal type */
    char *ident;                         /* A string describing it and ot */
    T_AUDIO_FORMAT_INFO *ifmt;           /* input audio format info */
    T_AUDIO_FORMAT_INFO *ofmt;           /* output audio format info */
    t_audio_uapi_usr_cback uapi_cback;   /* callback function for audio path state event. */
    uint8_t priority;                    /* priority */
    uint8_t mix;                         /* used in dual uac mode, to mix two audio into one. */
};

/**
 * @brief  Define the format for releasing an audio path.
 *
 */
struct rel_iovec
{
    uint8_t it;        /* input terminal type. */
    uint8_t ot;        /* output terminal type. */
};

/**
 * \brief   Initialize audio path module.
 *
 * \param[in]  evt_queue       audio event queue.
 * \param[in]  msg_queue       audio message queue.
 * \param[in]  ipc_ver         IPC version.
 *
 * \return                     The status of initializing audio path module.
 * \retval true                audio path module was initialized successfully.
 * \retval false               audio path module was failed to initialize.
 */
bool app_audio_path_init(void *evt_queue, void *msg_queue, uint8_t ipc_ver);

/**
 * \brief   Create one or more audio paths.
 *
 * \param[in]  iov       audio path parameters.
 * \param[in]  iovcnt    audio path count to be created.
 * \param[out] ids       path ids.
 *
 * \return               The result of creating audio paths.
 * \retval 0             Create audio path successfully.
 * \retval negative      Create audio path failed.
 */
int app_audio_path_createv(const struct path_iovec *iov, int iovcnt, uint8_t *ids);

/**
 * \brief   Release all audio paths with IT and OT matched.
 *
 * \param[in]  iov       audio path parameters.
 * \param[in]  count     the audio path count to be released.
 *
 * \return               void.
 */
void app_audio_path_releasev_itot(const struct rel_iovec *iov, uint8_t count);

/**
 * \brief   Release one audio path with IT and OT matched.
 *
 * \param[in]  it       input terminal type.
 * \param[in]  ot       output terminal ype.
 *
 * \return              void.
 */
void app_audio_path_release_by_itot(uint8_t it, uint8_t ot);

/**
 * \brief   Fill data into the audio path cyclic buffer.
 *
 * \param[in]  it            input terminal type.
 * \param[in]  buf           the data to be written into the cyclic buffer .
 * \param[in]  len           data length.
 * \param[in]  flag          packet status flag.
 * \param[in]  timestamp     time stamp.
 *
 * \return                   The result of filling data into the cyclic buffer.
 * \retval  positive         write cyclic buffer successfully.
 * \retval  negative         abnormal conditions.
 */
int app_audio_path_fill_cyclic(uint8_t it, uint8_t *buf, uint16_t len,
                               uint8_t flag, uint32_t timestamp);

/**
 * \brief   fter Filling data into the cyclic buffer, send message to notify APP. .
 *
 * \param[in]  it            input terminal type.
 *
 * \return                   send message result.
 * \retval  0                send message successfully.
 * \retval  negative         abnormal conditions.
 */
int app_audio_path_send_fill_msg(uint8_t it);

/**
 * \brief   Fill PCM data to the audio path which is identified by IT .
 *          Call this func when the context is different from  the app task
 *
 * \param[in]  it            input terminal type.
 * \param[in]  buf           the data to be filled into the audio path.
 * \param[in]  len           data length.
 * \param[in]  flag          packet status flag.
 * \param[in]  timestamp     time stamp.
 *
 * \return                   The result of filling data into the audio path.
 * \retval  0                filling data successfully.
 * \retval  negative         abnormal conditions.
 */
int app_audio_path_fill_async(uint8_t it, uint8_t *buf, uint16_t len,
                              uint8_t flag, uint32_t timestamp);

/**
 * \brief   For decoding, there is a space limit.
 *          The buffer for storing decoded data would not be as big as possible .
 *          So we provide a mechanism that monitors the watermark of the buffer
 *          and pauses or resumes the decoding subroutine.
 *          This function is used to decrease the watermark when the decoded data was fetched.
 *          It decreases the watermark and check if the decoder should be resumed
 *
 * \param[in]  ot            output terminal type.
 * \param[in]  data_sent     send data flag. 0 means no change on watermark,
 *                           so needn't to send signal to app task to resume decoding.
 *
 * \return                   void.
 */
void app_audio_path_resume_dec(uint8_t ot, uint16_t data_sent);

/**
 * \brief   Return the current watermark.
 *
 * \param[in]  ot            output terminal type.
 *
 * \return                   current watermark of the audio pipe.
 * \retval  0                ot != OT_UDEV_OUT1.
 * \retval  positive         current watermark.
 */
uint32_t app_audio_path_watermark(uint8_t ot);

/**
 * \brief   Similar functionality as the \ref app_audio_path_fill_async function.
 *          Call this func in the app task context
 *
 * \param[in]  it            input terminal type.
 * \param[in]  buf           the data to be filled into the audio path.
 * \param[in]  len           data length.
 * \param[in]  flag          packet status flag.
 * \param[in]  timestamp     time stamp.
 *
 * \return                   The result of filling data into the audio path.
 * \retval  0                filling data successfully.
 * \retval  -1               filling data error.
 */
int app_audio_path_fill(uint8_t it, uint8_t *buf, uint16_t len, uint8_t flag,
                        uint32_t timestamp);

/**
 * \brief   A wrapper function for handling audio path messages.
 *
 * \param[in]  msg       messages.
 *
 * \return               void.
 */
void app_audio_path_handle_msg(T_IO_MSG *msg);

/**
 * \brief   Flush audio path cyclic buffer with IT matched.
 *
 * \param[in]  it       input terminal type.
 *
 * \return              The result of flushing cyclic buffer.
 * \retval  -1          flush failed because of invalid input terminal type.
 * \retval  0           no need to flush.
 * \retval  positive    buffer flush size .
 */
int app_audio_path_flush(uint8_t it);

/**
 * \brief   Mute/Unmute an input terminal.
 *
 * \param[in]  iterminal       input terminal type.
 * \param[in]  mute            mute flag, true or false.
 *
 * \return                     The result of setting mute or unmute.
 * \retval  true               set mute/unmute successfully.
 * \retval  false              set mute/unmute failed.
 */
bool app_audio_path_set_input_mute(uint8_t iterminal, bool mute);

/**
 * \brief   Mute/Unmute the AUX OUT or Speaker.
 *
 * \param[in]  mute           mute flag, true or false.
 *
 * \return                    The result of setting mute or unmute.
 * \retval  true              set mute/unmute successfully.
 * \retval  false             set mute/unmute failed.
 */
bool app_audio_path_set_aux_mute(bool mute);

/**
 * \brief   Increase or decrease the AUX OUT or Speaker volume.
 *
 * \param[in]  volume         the volume to be set.
 *
 * \return                    The result of setting volume.
 * \retval  true              set volume successfully.
 * \retval  false             set volume failed.
 */
bool app_audio_path_set_aux_track_volume(uint8_t volume);

/**
 * \brief   Set the audio pipe gain. Currently we only support set lc3 encode gain.
 *
 * \param[in]  value      the gain to be set.
 *
 * \return                The result of setting gain.
 * \retval  true          set gain successfully.
 * \retval  false         set gain failed.
 */
bool app_audio_path_set_pipe_volume(uint16_t value);

/**
 * \brief   Set the audio pipe gain with IT and OT matched.
 *
 * \param[in]  it         input terminal type.
 * \param[in]  ot         output terminal type.
 * \param[in]  gain       the gain to be set.
 *
 * \return                The result of setting audio pipe gain.
 * \retval  true          set gain successfully.
 * \retval  false         set gain failed.
 */
bool app_audio_path_set_gain(uint8_t it, uint8_t ot, uint16_t gain);

/**
 * \brief   Register a monitor to get audio path state when the state of any audio path changes.
 *
 * \param[in]  cback      callback function for audio path monitor.
 *
 * \return                The result of registering audio path monitor.
 * \retval  0             register successfully.
 * \retval  -1            register failed.
 */
int app_audio_path_reg_monitor(t_audio_monitor_cback cback);

/**
 * \brief   Get the audio path format by id.
 *
 * \param[in]   id        audio path id.
 * \param[out]  ifmt      input audio format.
 * \param[out]  ofmt      output audio format.
 *
 * \return                The result of registering audio path monitor.
 * \retval  0             get format successfully.
 * \retval  -1            get format failed.
 */
int app_audio_path_format_by_id(uint8_t id, T_AUDIO_FORMAT_INFO **ifmt, T_AUDIO_FORMAT_INFO **ofmt);

bool app_audio_path_track_in_volume_mute(uint32_t device, bool enable);

bool app_audio_path_track_in_volume_up_down(uint32_t device, bool vol_up);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __APP_AUDIO_PATH_H__ */
