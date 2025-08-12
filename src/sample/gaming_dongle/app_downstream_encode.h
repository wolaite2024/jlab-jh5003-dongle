#ifndef __APP_DOWNSTREAM_ENCODE_H__
#define __APP_DOWNSTREAM_ENCODE_H__

#include "ring_buffer.h"
#include "audio_pipe.h"
#include "app_gaming_sync.h"
#include "usb_audio_stream.h"

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "btm.h"
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
#include "codec_def.h"
#endif

typedef enum
{
    DS_TRANSMIT_LEA,
} T_DS_TRANSMIT_TYPE;

typedef enum
{
    DS_MSG_SUBTYPE_PCM_DECODE,
    DS_MSG_SUBTYPE_DS_STREAMING,
} T_DS_MSG_SUB_TYPE;

#if ENABLE_UAC2
#define USB_DS_PIPE_NUM     2
#else
#define USB_DS_PIPE_NUM     1
#endif

typedef struct
{
    T_AUDIO_PIPE_HANDLE pipe_handle;
    T_RING_BUFFER pcm_ring_buf;
    uint8_t *pcm_buf;
    bool pipe_ready;
    uint16_t fill_pcm_size;
    uint32_t snk_transport_address;
    uint16_t dac_gain;
    uint8_t  dac_gain_percentage;
    bool spk_mute;
} T_DS_PIPE_INFO;

typedef struct
{
    uint16_t last_send_seq;
    uint32_t time_diff_to_ap;
    uint32_t last_ap;
    uint32_t next_ap;
    uint32_t iso_interval;
} T_APP_ISO_TIME_INFO;

typedef enum
{
    A2DP_DS_STREAM_STATE_IDLE,
    A2DP_DS_STREAM_STATE_STARTING,
    A2DP_DS_STREAM_STATE_STARTED,
    A2DP_DS_STREAM_STATE_TRANSMITTING,
} T_APP_A2DP_DS_STREAM_STATE;

typedef enum
{
    DOWNSTREAM_MODE_NONE,
    DOWNSTREAM_MODE_LEGACY,
    DOWNSTREAM_MODE_LEA,
} T_APP_24G_DOWNSTREAM_MODE;

typedef struct
{
    T_GAMING_CODEC codec_type;
    T_DS_PIPE_INFO pipe_info[USB_AUDIO_STREAM_NUM];
    bool           create_pipe_after_feedback_detect;
    bool           mixing_pipe;
    bool           mixing_filling;
    bool           fill_time_fitting_to_ap;
    bool           increase_usb_pcm_input;
    bool           decrease_usb_pcm_input;
    int32_t        sample_freq_delta;
    int32_t        sample_delta_per_interval;
    uint16_t       pcm_bytes_per_interval;
    uint8_t        asrc_state;
    int32_t        asrc_ratio;
    uint8_t        asrc_detect_cnt;
    uint16_t       skip_pcm_interval;
#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    bool           a2dp_suspend;
    bool           a2dp_connected;
    bool           a2dp_opened;
    uint8_t        a2dp_frame_num;
    uint16_t       a2dp_seq;
    uint32_t       a2dp_timestamp;
    T_APP_A2DP_DS_STREAM_STATE a2dp_ds_stream_state;
    T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL a2dp_cfg;
#endif
#if TARGET_LE_AUDIO_GAMING_DONGLE
    T_CODEC_CFG      lea_codec_cfg;
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    T_APP_24G_DOWNSTREAM_MODE downstream_mode;
#endif
#endif
} T_USB_DS_INFO;

/**
 *
 * @brief   rcv downstream pcm data from usb
 *
 * @param   data: pcm data to be received
 * @param   len : rcv data len
 * @param   uac_label: data from which uac
 *
 * @return  return true if success
*/
bool app_usb_ds_rcv_pcm(uint8_t *data, uint16_t len, T_UAC_LABEL uac_label);

/**
 *
 * @brief   handle downstream start action
 *
 * @param   void
 *
 * @return  void
*/
void app_usb_ds_handle_stream_start(void);

/**
 *
 * @brief   handle downstream stop action
 *
 * @param   void
 *
 * @return  void
*/
void app_usb_ds_handle_stream_stop(void);

/**
 *
 * @brief   encode pcm data from buf
 *
 * @param   void
 *
 * @return  void
*/
void app_usb_ds_pcm_encode(void);

/**
 *
 * @brief   handle a2dp suspend msg from headset
 *
 * @param   suspend:  suspend stream to change codec
 *
 * @return  void
*/
void app_usb_ds_handle_a2dp_suspend(bool suspend);

/**
 *
 * @brief   set uac spk mute/unmute
 *
 * @param   mute: dac gain mute/unmute
 *
 * @return  true if success
*/
bool app_usb_ds_mute_set(T_UAC_LABEL uac_label, bool mute);

/**
 *
 * @brief   set downstream encode gain
 *
 * @param   uac_label: uac to be set
 * @param   gain: encode gain
 *
 * @return  true if success
*/
bool app_usb_ds_gain_set(T_UAC_LABEL uac_label, uint16_t gain);

/**
 *
 * @brief   set uac gain percentage
 *
 * @param   uac1_vol_percent: uac1 vol percentage
 * @param   uac2_vol_percent: uac2 vol percentage
 *
 * @return  void
*/
void app_usb_ds_set_gain_percentage(uint8_t uac1_vol_percent, uint8_t uac2_vol_percent);

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
/**
 *
 * @brief   create legacy audio pipe for usb downstream with a2dp
 *
 * @param   cfg: a2dp cfg
 *
 * @return  void
*/
void app_usb_ds_legacy_pipe_create(T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL cfg);
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
/**
 *
 * @brief   create audio pipe for usb downstream with lea
 *
 * @param   p_lea_codec: lea cfg
 *
 * @return  void
*/
void app_usb_ds_lea_pipe_create(T_CODEC_CFG *p_lea_codec);
#endif

/**
 *
 * @brief   streaming handle in app task to do the
 *          task need to be executed in app task
 *
 * @param   none
 *
 * @return  void
*/
void app_usb_ds_streaming_handle(void);

/**
 *
 * @brief   release exist create audio pipe for downstream
 *
 * @param   void
 *
 * @return  void
*/
void app_usb_ds_pipe_release(void);

/**
 *
 * @brief   init usb downstream handle
 *
 * @param   void
 *
 * @return  void
*/
void app_usb_ds_init(void);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
bool app_usb_ds_is_a2dp_stream(void);
#endif

void app_usb_ds_set_sbc_frame_num(uint8_t frame_num);
uint8_t app_usb_ds_get_sbc_frame_num(void);
void app_usb_ds_set_lc3_frame_num(uint8_t frame_num);
uint8_t app_usb_ds_get_lc3_frame_num(void);

#endif
