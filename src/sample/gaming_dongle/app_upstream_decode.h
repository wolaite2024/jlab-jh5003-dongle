#ifndef __APP_UPSTREAM_DECODE_H__
#define __APP_UPSTREAM_DECODE_H__

#include "ring_buffer.h"
#include "bt_direct_msg.h"
#include "audio_pipe.h"
#include "section.h"
#include "codec_def.h"
#include "app_gaming_sync.h"

#define PCM_PREQUEUE_SIZE          1440

typedef enum
{
    ISO_LEFT_QUEUE,
    ISO_RIGHT_QUEUE,
} T_ISO_QUEUE_NUM;

typedef struct
{
    T_GAMING_CODEC codec_type;
    uint16_t frame_size;
    uint16_t mic_gain;
    uint16_t decoding_pcm_size;
    uint16_t expect_pcm_frame_len;
    bool pcm_prequeue_ready;
    bool mic_mute;
    bool us_pkt_xmitting;
    bool us_pkt_start_decode;
} T_US_PIPE_INFO;

/**
 *
 * @brief   drain pcm data in shm
 *
 * @param   void
 *
 * @return  void
*/
void us_drain_pcm_data(void);

/**
 *
 * @brief   decode us pkt from buf
 *
 * @param   void
 *
 * @return  void
*/
void us_decode_pkt_from_buf(void);

/**
 *
 * @brief   check pipe decode data in shm in case of interrupt miss
 *
 * @param   void
 *
 * @return  true if has pipe data wait for drain
*/
bool us_check_pipe_decode_data_in_shm(void);


#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
/**
 *
 * @brief   process received upstream pkt
 *
 * @param   buf: data buf
 * @param   len: data len
 *
 * @return  void
*/
void us_handle_rcv_pkt(uint8_t *buf, uint16_t len);
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
/**
 *
 * @brief   process received upstream lea audio data
 *
 * @param   p_iso: lea iso data to be handled
 *
 * @return  void
*/
void us_handle_lea_data(T_BT_DIRECT_ISO_DATA_IND *p_iso);
#endif

/**
 *
 * @brief   decode upstream pkt from buf
 *
 * @param   void
 *
 * @return  void
*/
void us_decode_pkt_in_buf(void);

/**
 *
 * @brief   set upstream encode gain
 *
 * @param   gain: gain value to set
 *
 * @return  true if set success
*/
bool us_pipe_gain_set(uint16_t gain);

/**
 *
 * @brief   set upstream mic mute/unmute
 *
 * @param   mute: mic mute/unmute
 *
 * @return  true if set success
*/
bool us_pipe_mute_set(bool mute);

/**
 *
 * @brief   set pcm prequeue ready status
 *
 * @param   value
 *
 * @return  void
*/
RAM_TEXT_SECTION void us_set_pcm_prequeue_ready(bool value);

/**
 *
 * @brief   check if us pkt start decode
 *
 * @param   void
 *
 * @return  true if upstream packet start decode
*/
RAM_TEXT_SECTION bool us_pkt_start_decode(void);

/**
 *
 * @brief   check if pcm prequeue ready before to xmit
 *
 * @param   void
 *
 * @return  true if prequeue ready
*/
RAM_TEXT_SECTION bool us_pcm_prequeue_ready(void);

void upstream_pipe_create(T_GAMING_CODEC src_codec);
void upstream_pipe_release(void);

#if TARGET_LE_AUDIO_GAMING_DONGLE
void iso_queue_clear(T_ISO_QUEUE_NUM queue_num);
void app_upstream_lea_pipe_create(T_CODEC_CFG *p_lea_codec);
#endif

/**
 *
 * @brief   init realtek talk profile process
 *
 * @param   void
 *
 * @return  void
*/
void us_process_init(void);

extern T_RING_BUFFER usb_us_pcm_ring_buf;

#endif
