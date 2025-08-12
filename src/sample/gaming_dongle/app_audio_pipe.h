/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __APP_AUDIO_PIPE_H__
#define __APP_AUDIO_PIPE_H__

#include "audio_type.h"
#include "app_msg.h"
#include "app_cyclic_buffer.h"

/* TODO: avoid msg type collision */
#define MSG_TYPE_AUDIO_PIPE_CODEC        0x0E00    /* audio pipe codec message type */
#define MSG_TYPE_AUDIO_PIPE_RESUME_DEC   0x0E01    /* audio pipe resume decoding message type */

#define CODEC_SBCENC_TYPE           0x01         /* SBC encode */
#define CODEC_LC3ENC_TYPE           0x02         /* LC3 encode */
#define CODEC_LC3DEC_TYPE           0x03         /* LC3 decode */
#define CODEC_SBCDEC_TYPE           0x04         /* SBC decode */
#define CODEC_MSBCENC_TYPE          0x05         /* MSBC encode */
#define CODEC_MSBCDEC_TYPE          0x06         /* MSBC decode */
#define CODEC_SBCENC2_TYPE          0x07         /* SBC2 encode */
#define CODEC_LC3ENC2_TYPE          0x08         /* LC3 2 encode */
#define CODEC_MAX_TYPE              0x09

typedef enum
{
    APP_AUDIO_PIPE_EVENT_RELEASED,               /* audio pipe released event */
    APP_AUDIO_PIPE_EVENT_CREATED,                /* audio pipe created event */
    APP_AUDIO_PIPE_EVENT_STARTED,                /* audio pipe started event */
    APP_AUDIO_PIPE_EVENT_STOPPED,                /* audio pipe stoped event */
    APP_AUDIO_PIPE_EVENT_DATA_FILLED,            /* audio pipe filled data into DSP evnet */
    APP_AUDIO_PIPE_EVENT_DATA_IND,               /* audio pipe received data from DSP event */
} T_APP_AUDIO_PIPE_EVENT;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief   calcualte pkt loss info
 *
 * \param[in]  codec_tyep      codec type.
 * \param[in]  loss_frame_cnt  loss frame cnt.
 *
 * \return  true if cal successfully
 */
bool app_audio_calculate_loss_frame(uint8_t codec_type, uint8_t loss_frame_cnt);

/**
 * \brief   set rtp frame cnt
 *
 * \param[in]  cnt             rtp frame cnt in one packet
 *
 * \return  void
 */
void app_audio_pipe_set_rtp_frame_cnt(uint8_t cnt);

/**
 * \brief   set rtp frame duration
 *
 * \param[in]  duration        rtp frame duration.
 *
 * \return  void
 */
void app_audio_pipe_set_rtp_frame_duration(uint8_t duration);

/**
 * \brief   Initialize audio pipe module.
 *
 * \param[in]  evt_queue       audio event queue.
 * \param[in]  msg_queue       audio message queue.
 * \param[in]  io_msg_type     io message type.
 *
 * \return                     void.
 */
void app_audio_pipe_init(void *evt_queue, void *msg_queue, uint8_t io_msg_type);

/**
 * \brief   Create an audio pipe.
 *
 * \param[in]  codec_type      codec type.
 * \param[in]  config          audio format information.
 * \param[in]  mgr_cback       app audio pipe event callback function.
 * \param[in]  uid             Upper layer identification.
 * \param[in]  associated      mix flag, 0 means a single audio path.
 *
 * \return                     The result of creating an audio pipe.
 * \retval true                audio pipe created successfully.
 * \retval false               audio path created failed.
 */
bool app_audio_pipe_create(uint8_t codec_type, T_AUDIO_FORMAT_INFO *config,
                           void (*mgr_cback)(uint8_t id, uint8_t event, uint8_t *buf, uint32_t len),
                           uint32_t uid, uint8_t associated);

/**
 * \brief   Change audio pipe path id.
 *
 * \param[in]  codec_type      codec type.
 * \param[in]  uid             path id.
 *
 * \return                     The result of changing audio pipe path id.
 * \retval true                audio pipe path id changed successfully.
 * \retval false               audio pipe path id changed failed.
 */
bool app_audio_pipe_change_uid(uint8_t codec_type, uint32_t uid);

/**
 * \brief   fill pkt loss info to dsp to generate plc data
 *
 * \param[in]  codec_type      codec type.
 *
 * \return                     The result of fill pkt loss.
 * \retval true                fill pkt loss successfully.
 * \retval false               fill pkt loss failed.
 */
bool app_audio_pipe_fill_pkt_loss(uint8_t codec_type);

/**
 * \brief   Audio pipe fill data to encode or decode.
 *
 * \param[in]  codec_type      codec type.
 * \param[in]  cyclic          cyclic buffer.
 *
 * \return                     The result of filling data.
 * \retval true                filled data successfully.
 * \retval false               filled data failed.
 */
bool app_audio_pipe_fill(uint8_t codec_type, T_CYCLIC_BUF *cyclic);

/**
 * \brief   Release an audio pipe.
 *
 * \param[in]  codec_type      codec type.
 *
 * \return                     void.
 */
void app_audio_pipe_release(uint8_t codec_type);

/**
 * \brief   A wrapper function for handling audio pipe messages.
 *
 * \param[in]  msg       messages.
 *
 * \return               void.
 */
void app_audio_pipe_handle_msg(T_IO_MSG *msg);

/**
 * \brief   Set the audio pipe gain. Currently we only support set lc3 encode gain.
 *
 * \param[in]  value      the gain to be set.
 *
 * \return                The result of setting gain.
 * \retval  true          set gain successfully.
 * \retval  false         set gain failed.
 */
bool app_audio_pipe_set_volume(uint16_t value);

/**
 * \brief   Set the audio pipe gain with codec type matched.
 *
 * \param[in]  codec_type         codec type.
 * \param[in]  gain               the gain to be set.
 *
 * \return                        The result of setting audio pipe gain.
 * \retval  true                  set audio pipe gain successfully.
 * \retval  false                 set audio pipe gain failed.
 */
bool app_audio_pipe_set_gain(uint8_t codec_type, uint16_t gain);

/**
 * \brief   Return the current watermark.
 *
 * \return                   current watermark of the audio pipe.
 * \retval  positive         current watermark.
 */
uint32_t app_audio_pipe_watermark(void);

/**
 * \brief   Audio pipe increase decoder watermark.
 *
 * \param[in]  codec_type         codec type.
 * \param[in]  value              increased size.
 *
 * \return                        The result of increasing decoder watermark.
 * \retval  true                  decoder watermark increased successfully.
 * \retval  false                 decoder watermark increased failed.
 */
bool app_audio_pipe_add_watermark(uint8_t codec_type, uint16_t value);

/**
 * \brief   Calculating sbc frame size.
 *
 * \param[in]  chann_mode          channel mode for encoding or decoding.
 *                                 0: mono, 1: dual channels, 2: stereo, 3: joint stereo.
 * \param[in]  blocks              block length for encoding or decoding.
 *                                 The supported values are 4, 8, 12 and 16.
 * \param[in]  subbands            number of subbands for encoding or decoding.
 *                                 The supported values are 4 and 8.
 * \param[in]  bitpool             bitpool value for encoding or decoding.
 *                                 The supported ranges are from 2 to 250.
 *
 * \return                         frame size.
 */
uint16_t calc_sbc_frame_size(uint8_t chann_mode, uint8_t blocks,
                             uint8_t subbands, uint8_t bitpool);
void app_usb_downstream_pcm_format_init(T_AUDIO_FORMAT_INFO *info);
void app_usb_upstream_pcm_format_init(T_AUDIO_FORMAT_INFO *info);
void app_set_downstream_pcm_format(uint32_t sample_rate, uint8_t chan_num, uint8_t bit_width);
void app_set_upstream_pcm_format(uint32_t sample_rate, uint8_t chan_num, uint8_t bit_width);

#ifdef __cplusplus
}
#endif

#endif /* __APP_AUDIO_PIPE_H__ */
