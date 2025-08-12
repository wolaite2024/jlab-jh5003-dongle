#ifndef __USB_AUDIO_WRAPPER_H__
#define __USB_AUDIO_WRAPPER_H__
#include "audio_type.h"
#include "usb_audio.h"
#include "usb_audio_stream.h"

/** @defgroup UAW USB Audio Wrapper
  * @brief module for universal usb audio process
  * @{
  */


/*============================================================================*
*                              Macros
*============================================================================*/

typedef enum
{
    UAC_SPK_VOL,
    UAC_MIC_VOL,
} T_UAC_VOL_TYPE;

typedef enum
{
    USB_FEEDBACK_DETECT,
    USB_FEEDBACK_SUPPORT,
    USB_FEEDBACK_NOT_SUPPORT,
} T_USB_AUDIO_FEEDBACK_STATE;

typedef void (*T_USB_FEEDBACK_CB)(T_USB_AUDIO_FEEDBACK_STATE state);

/* append silence data time after no usb audio stream */
#define USB_SEND_SILENCE_TIME          300

#if F_APP_USB_HIGH_SPEED_0_5MS || F_APP_PCM_SPLIT_0_5MS
#define PCM_DS_INTERVAL                500 //us
#else
#define PCM_DS_INTERVAL                (USB_AUDIO_DS_INTERVAL * 1000)
#endif


/**
 * app_usb_audio_wrapper.h
 *
 * \brief   mapping usb host gain value to avrcp gain range (0x00~0x7f)
 *
 * \param[in] vol_param: gain value from usb host
 *
 * \return  vol value for avrcp vol range (0x00~0x7f)
 *
 * \ingroup UAW
 */
uint16_t app_usb_audio_spk_vol_transform(uint16_t vol_param);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   notify stream active
 *
 * \param[in] label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] dir downstrea or upstream
 *
 * \return true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_active(uint32_t label, uint8_t dir);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   notify stream deactive
 *
 * label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] dir downstrea or upstream
 *
 * \return true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_deactive(uint32_t label, uint8_t dir);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   adjust sample freq for usb feedback
 *
 * \param[in] freq_diff sample freq diff
 *
 * \return void
 *
 * \ingroup UAW
 */
void app_usb_audio_sample_rate_adjust(int16_t freq_diff);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   donwn stream feedback
 *
 * \param[in] buf data buffer
 * \param[in] len length of data to write
 * \param[in] label the pipe of @ref T_USB_AUDIO_STREAM
 *
 * \return true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_feedback_ds(uint8_t *data, uint16_t length, uint32_t label);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   donwn stream data
 *
 * \param[in] buf data buffer
 * \param[in] len length of data to write
 * \param[in] label the pipe of @ref T_USB_AUDIO_STREAM
 *
 * \return true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_data_xmit_out(uint8_t *data, uint16_t length, T_UAC_LABEL label);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   donwn stream data msg in app task
 *
 * \param[in] label the pipe of @ref T_USB_AUDIO_STREAM
 *
 * \return true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_data_xmit_out_handle(uint32_t label);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   write usb audio stream data
 *          is called
 *
 * \param[in] buf data buffer
 * \param[in] len length of data to write
 *
 * \return actual length of written data
 *
 * \ingroup UAW
 */
bool app_usb_audio_us_data_write(uint8_t *data, uint16_t length);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   get usb audio upstream remaining pool size
 *
 * \return remaining pool size
 *
 * \ingroup UAW
 */
uint16_t app_usb_audio_us_get_remaining_pool_size(void);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   get usb audio upstream data size
 *
 * \return data size
 *
 * \ingroup UAW
 */
uint16_t app_usb_audio_us_get_data_len(void);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   usb audio upstream pool size is changing
 *
 * \param[in] data_space used data size
 * \param[in] free_space remaining pool size
 * \param[in] byte_out usb transfer data size
 *
 * \ingroup UAW
 */
void app_usb_audio_us_size_change(uint16_t data_space, uint16_t free_space,
                                  uint16_t byte_out);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   upstream data is empty handle
 *
 * \return  true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_us_data_empty(void);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   usb audio set speaker volume
 *
 * \param[in] label label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] vol volume value
 *
 * \ingroup UAW
 */
bool app_usb_audio_set_spk_vol(uint32_t label, uint16_t vol);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   usb audio set mic volume
 *
 * \param[in] label label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] vol volume value
 *
 * \ingroup UAW
 */
bool app_usb_audio_set_mic_vol(uint32_t label, uint16_t vol);

/**
 *
 * \brief     get uac silence streaming state
 *
 * \param[in] uac label
 *
 * \return    true if uac silence streaming
 *
 * \ingroup UAW
 */
bool app_usb_get_uac_silence_streaming_state(T_UAC_LABEL uac_label);

/**
 *
 * \brief     get pcm size per usb interval
 *
 * \param[in] void
 *
 * \return    pcm size
 *
 * \ingroup UAW
 */
uint16_t app_usb_get_usb_pcm_size(void);

/**
 *
 * \brief     get uac streaming state
 *
 * \param[in] uac label
 *
 * \return    true if uac streaming
 *
 * \ingroup UAW
 */
bool app_usb_get_uac_streaming_state(T_UAC_LABEL uac_label);

/**
 *
 * \brief     get dac gain value by level
 *
 * \param[in] vol level
 *
 * \return    dac gain value
 *
 * \ingroup UAW
 */
uint16_t app_usb_get_dac_gain_by_level(uint8_t level);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   set usb streaming status
 *
 * \param[in] usb streaming status
 *
 * \ingroup UAW
 */
void app_usb_set_usb_src_streaming(bool streaming);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   set usb streaming silence status
 *
 * \param[in] usb streaming silence status
 *
 * \ingroup UAW
 */
void app_usb_set_stream_silence_for_a_while(bool status);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   init usb audio stream
 *
 * \ingroup UAW
 */
void app_usb_audio_wrapper_init(void);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   usb audio set speaker mute
 *
 * \param[in] vol mute value
 *
 * \ingroup UAW
 */
bool app_usb_audio_set_spk_mute(uint32_t label, uint16_t vol);


/**
 * app_usb_audio_wrapper.h
 *
 * \brief   usb audio set vol
 *
 * \param[in] vol_type: spk or mic val
 * \param[in] label:    uac num
 * \param[in] vol: vol value to be set
 *
 * \ingroup UAW
 */
bool app_usb_audio_set_vol(T_UAC_VOL_TYPE vol_type, uint8_t label, uint16_t vol);

/**
 * app_usb_audio_wrapper.h
 *
 * \brief   sync vol info to headset
 *
 * \ingroup UAW
 */
bool app_usb_spk_vol_sync_to_headset(void);

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
void app_usb_audio_reg_feedback_state_cb(T_USB_FEEDBACK_CB callback);
void app_usb_audio_set_feedback_state(T_USB_AUDIO_FEEDBACK_STATE state);
void app_usb_feedback_detect_start(void);
void app_usb_feedback_detect_stop(void);
T_USB_AUDIO_FEEDBACK_STATE app_usb_audio_get_feedback_state(void);
#endif

/** @}*/
/** End of UAW
*/

#endif
