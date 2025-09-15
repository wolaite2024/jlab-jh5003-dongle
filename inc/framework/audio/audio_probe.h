/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_PROBE_H_
#define _AUDIO_PROBE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_PROBE Audio Probe
 *
 * \brief   Test, analyze and make profiling on the Audio Subsystem.
 *
 * \details Audio Probe provides the testing or profiling interfaces, which application can analyze
 *          the Audio Subsystem.
 *
 * \note    Audio Probe interfaces are used only for testing and profiling. Application shall not
 *          use Audio Probe interfaces in any formal product scenarios.
 */

/**
 * \brief   Define Audio Probe event from DSP.
 *
 * \note    Only provide Audio Probe event for application using.
 *
 * \ingroup AUDIO_PROBE
 */
typedef enum
{
    PROBE_SCENARIO_STATE             = 0x00,
    PROBE_EAR_FIT_RESULT             = 0x01,
    PROBE_SDK_GENERAL_CMD            = 0x02,
    PROBE_SDK_BOOT_DONE              = 0x03,
    PROBE_HA_VER_INFO                = 0x04,
    PROBE_SEG_SEND_REQ_DATA          = 0x05,
    PROBE_SEG_SEND_ERROR             = 0x06,
    PROBE_SYNC_REF_REQUEST           = 0x07,
} T_AUDIO_PROBE_EVENT;

/**
 * \brief   Define Audio Probe event from DSP.
 *
 * \note    Only provide Audio Probe event for application using.
 *
 * \ingroup AUDIO_PROBE
 */
typedef enum
{
    AUDIO_PROBE_DSP_EVT_INIT_FINISH                     = 0x00,
    AUDIO_PROBE_DSP_EVT_POWER_OFF                       = 0x01,
    AUDIO_PROBE_DSP_EVT_POWER_ON                        = 0x02,
    AUDIO_PROBE_DSP_EVT_EXCEPTION                       = 0x03,
    AUDIO_PROBE_DSP_EVT_REQ                             = 0x04,
    AUDIO_PROBE_DSP_EVT_READY                           = 0x05,
    AUDIO_PROBE_DSP_EVT_REQUEST_EFFECT                  = 0x06,
    AUDIO_PROBE_DSP_EVT_FADE_OUT_FINISH                 = 0x07,
    AUDIO_PROBE_DSP_EVT_SPORT0_READY                    = 0x08,
    AUDIO_PROBE_DSP_EVT_SPORT0_STOP                     = 0x09,
    AUDIO_PROBE_DSP_EVT_SPORT1_READY                    = 0x0a,
    AUDIO_PROBE_DSP_EVT_SPORT1_STOP                     = 0x0b,
    AUDIO_PROBE_DSP_EVT_SPORT_STOP_FAKE                 = 0x0c,
    AUDIO_PROBE_DSP_EVT_SPORT_START_FAKE                = 0x0d,
    AUDIO_PROBE_DSP_EVT_CODEC_STATE                     = 0x0e,
    AUDIO_PROBE_DSP_MGR_EVT_FW_READY                    = 0x0f,
    AUDIO_PROBE_DSP_MGR_EVT_FW_STOP                     = 0x10,
    AUDIO_PROBE_DSP_EVT_MAILBOX_DSP_DATA                = 0x11,
    AUDIO_PROBE_DSP_MGR_EVT_NOTIFICATION_FINISH         = 0x12,
    AUDIO_PROBE_DSP_MGR_EVT_OPEN_AIR_AVC                = 0x21,
} T_AUDIO_PROBE_DSP_EVENT;

typedef enum
{
    PROBE_ALGO_PROPRIETARY_VOICE,    //00
    PROBE_ALGO_G711_ALAW,            //01
    PROBE_ALGO_CVSD,                 //02
    PROBE_ALGO_MSBC,                 //03
    PROBE_ALGO_OPUS_VOICE,           //04
    PROBE_ALGO_UHQ_VOICE,            //05
    PROBE_ALGO_USB_SPEECH,           //06
    PROBE_ALGO_SBC,                  //07
    PROBE_ALGO_AAC,                  //08
    PROBE_ALGO_PROPRIETARY_AUDIO1,   //09
    PROBE_ALGO_PROPRIETARY_AUDIO2,   //10
    PROBE_ALGO_MP3,                  //11
    PROBE_ALGO_USB_AUDIO,            //12
    PROBE_ALGO_SUB_WOOFER,           //13
    PROBE_ALGO_LDAC,                 //14 SONY
    PROBE_ALGO_UHQ,                  //15 Samsung
    PROBE_ALGO_LHDC,                 //16 Savitech
    PROBE_ALGO_OPUS_AUDIO,           //17
    PROBE_ALGO_PROPRIETARY_AUDIO3,   //18
    PROBE_ALGO_PURE_STREAM,          //19, ALGORITHM_PROPRIETARY_AUDIO4
    PROBE_ALGO_LINE_IN,              //20, MUST before ALGORITHM_END
    PROBE_ALGO_END                   //21
} T_ALGO_TYPE;

typedef struct
{
    uint8_t algo;
    uint8_t category;
} T_AUDIO_PROBE_DSP_EVENT_MSG;

typedef enum t_audio_probe_codec_mgr_bias_mode
{
    AUDIO_PROBE_CODEC_MGR_BIAS_NORMAL_MODE      = 0X00,
    AUDIO_PROBE_CODEC_MGR_BIAS_ALWAYS_ON_MODE   = 0X01,
    AUDIO_PROBE_CODEC_MGR_BIAS_USER_MODE        = 0X02,
} T_AUDIO_PROBE_CODEC_MGR_BIAS_MODE;

/**
 * \brief   Define Audio Probe command id for DSP.
 *
 * \note    Should be referenced to h2d_cmd.
 *
 * \ingroup AUDIO_PROBE
 */
typedef enum
{
    AUDIO_PROBE_TEST_MODE       = 0x02,
    AUDIO_PROBE_UPLINK_SYNCREF  = 0x5E,
    AUDIO_PROBE_SEG_SEND        = 0x63,
    AUDIO_PROBE_HA_PARA         = 0x70,
} T_AUDIO_PROBE_DSP_CMD;

/**
 * \brief Define Audio Probe callback prototype.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in]   event   The event for DSP \ref T_AUDIO_PROBE_EVENT.
 * \param[in]   buf     The buffer that holds the DSP probe data.
 *
 * \ingroup AUDIO_PROBE
 */
typedef void (*P_AUDIO_PROBE_DSP_CABCK)(T_AUDIO_PROBE_EVENT event, void *buf);

/**
 * \brief Define Audio Probe callback prototype of DSP status.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in]   event   The event for DSP \ref T_AUDIO_PROBE_DSP_EVENT.
 * \param[in]   msg     The message that holds the DSP status.
 *
 * \ingroup AUDIO_PROBE
 */
typedef void (*P_AUDIO_PROBE_DSP_EVENT_CABCK)(uint32_t event, void *msg);

typedef bool (*P_SYS_IPC_CBACK)(uint32_t id, void *msg);

typedef void *T_SYS_IPC_HANDLE;

/**
 * \brief   Register Audio Probe DSP callback.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in] cback The Audio Probe DSP callback \ref P_AUDIO_PROBE_DSP_CABCK.
 *
 * \return          The status of registering Audio Probe DSP callback.
 * \retval true     Audio Probe DSP callback was registered successfully.
 * \retval false    Audio Probe DSP callback was failed to register.
 *
 * \ingroup AUDIO_PROBE
 */
bool audio_probe_dsp_cback_register(P_AUDIO_PROBE_DSP_CABCK cback);

/**
 * \brief   Unregister Audio Probe DSP callback.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in] cback The Audio Probe DSP callback \ref P_AUDIO_PROBE_DSP_CABCK.
 *
 * \return          The status of unregistering Audio Probe DSP callback.
 * \retval true     Audio Probe DSP callback was unregistered successfully.
 * \retval false    Audio Probe DSP callback was failed to unregister.
 *
 * \ingroup AUDIO_PROBE
 */
bool audio_probe_dsp_cback_unregister(P_AUDIO_PROBE_DSP_CABCK cback);

/**
 * \brief   Send Audio Probe data to DSP.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in] buf   The probe data buffer address.
 * \param[in] len   The probe data buffer length.
 *
 * \return          The status of sending Audio Probe DSP data.
 * \retval true     Audio Probe DSP data was sent successfully.
 * \retval false    Audio Probe DSP data was failed to send.
 *
 * \ingroup AUDIO_PROBE
 */
bool audio_probe_dsp_send(uint8_t *buf, uint16_t len);

/**
 * \brief   Set Codec hardware Equalizer (EQ).
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in] eq_type   Hardware EQ type.
 * \param[in] eq_chann  Hardware EQ channel.
 * \param[in] buf       Hardware EQ band parameter buffer.
 * \param[in] len       Hardware EQ band parameter length.
 *
 * \return          The status of setting Codec hardware EQ.
 * \retval true     Codec hardware EQ was setting successfully.
 * \retval false    Codec hardware EQ was failed to set.
 *
 * \ingroup AUDIO_PROBE
 */
bool audio_probe_codec_hw_eq_set(uint8_t eq_type, uint8_t eq_chann, uint8_t *buf, uint16_t len);

bool audio_probe_send_dsp_sdk_data(uint8_t *p_data, uint16_t data_len);
bool audio_probe_send_lhdc_license(uint8_t *p_data, uint16_t data_len);
bool audio_probe_send_malleus_license(uint8_t *p_data, uint16_t data_len);

/**
 * \brief   Set Voice Primary MIC.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in] mic_sel   MIC selection (0: DMIC1, 1: DMIC2, 2: AMIC1, 3: AMIC2, 4: AMIC3, 5: AMIC4, 6: DMIC3, 7: MIC sel disable).
 * \param[in] mic_type  MIC type (0: single-end AMIC/falling DMIC, 1: differential AMIC/raising DMIC).
 *
 * \ingroup AUDIO_PROBE
 */
void audio_probe_set_voice_primary_mic(uint8_t mic_sel, uint8_t mic_type);

/**
 * \brief   Set Voice Secondary MIC.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in] mic_sel   MIC selection (0: DMIC1, 1: DMIC2, 2: AMIC1, 3: AMIC2, 4: AMIC3, 5: AMIC4, 6: DMIC3, 7: MIC sel disable).
 * \param[in] mic_type  MIC type (0: single-end AMIC/falling DMIC, 1: differential AMIC/raising DMIC).
 *
 * \ingroup AUDIO_PROBE
 */
void auido_probe_set_voice_secondary_mic(uint8_t mic_sel, uint8_t mic_type);

/**
 * \brief   Get Voice Primary MIC selection.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \ingroup AUDIO_PROBE
 */
uint8_t audio_probe_get_voice_primary_mic_sel(void);

/**
 * \brief   Get Voice Primary MIC type.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \ingroup AUDIO_PROBE
 */
uint8_t audio_probe_get_voice_primary_mic_type(void);

/**
 * \brief   Get Voice Secondary MIC selection.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \ingroup AUDIO_PROBE
 */
uint8_t audio_probe_get_voice_secondary_mic_sel(void);

/**
 * \brief   Get Voice Secondary MIC type.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \ingroup AUDIO_PROBE
 */
uint8_t audio_probe_get_voice_secondary_mic_type(void);

/**
 * \brief   Register Audio Probe DSP status callback.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in] cback The DSP probe callback \ref P_AUDIO_PROBE_DSP_EVENT_CABCK.
 *
 * \return          The status of registering Audio Probe DSP status callback.
 * \retval true     Audio Probe DSP status callback was registered successfully.
 * \retval false    Audio Probe DSP status callback was failed to register.
 *
 * \ingroup AUDIO_PROBE
 */
bool audio_probe_dsp_evt_cback_register(P_AUDIO_PROBE_DSP_EVENT_CABCK cback);

/**
 * \brief   Unregister Audio Probe DSP status callback.
 *
 * \note    Do <b>not</b> touch this interface in any product scenarios.
 *
 * \param[in] cback The DSP probe callback \ref P_AUDIO_PROBE_DSP_EVENT_CABCK.
 *
 * \return          The status of unregistering Audio Probe DSP status callback.
 * \retval true     Audio Probe DSP status callback was unregistered successfully.
 * \retval false    Audio Probe DSP status callback was failed to unregister.
 *
 * \ingroup AUDIO_PROBE
 */
bool audio_probe_dsp_evt_cback_unregister(P_AUDIO_PROBE_DSP_EVENT_CABCK cback);

bool audio_probe_dsp_ipc_send_ha_param(uint8_t *payload_data, uint16_t payload_len);

/**
 * \brief   Malloc RAM from playback buffer.
 *
 * \param[in] buf_size The buffer size need to malloc from playback buffer.
 *
 * \ingroup AUDIO_PROBE
 */
void *audio_probe_media_buffer_malloc(uint16_t buf_size);

/**
 * \brief   Free RAM to playback buffer.
 *
 * \return          The status of freeing playback buffer.
 * \retval true     The playback buffer was freed successfully.
 * \retval false    The playback buffer was failed to free.
 *
 * \ingroup AUDIO_PROBE
 */
bool audio_probe_media_buffer_free(void *p_buf);

bool audio_probe_dsp_ipc_send_call_status(bool enable);

bool audio_probe_send_sepc_info(uint8_t parameter, bool action, uint8_t para_chanel_out);

void audio_probe_set_sepc_info(void);

/**
 * \brief   Control loading DSP test bin.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] enable    Enable/disable loading DSP test bin.
 *
 * \ingroup AUDIO_PROBE
 */
void audio_probe_dsp_test_bin_set(bool enable);

/**
 * \brief   Power down DSP.
 *
 * \ingroup AUDIO_PROBE
 */
void audio_probe_disable_dsp_powerdown(void);

/**
 * \brief   Start the engine communication with DSP2.
 *
 * \brief   set codec bias mode.
 * \param[in] mode bias_mode(0:normal mode 1: always on mode 2:user mode)
 * \note
 *
 * \return
 * \retval  zero
 * \retval  non-zero
 *
 * \ingroup AUDIO_PROBE
 */
void audio_probe_codec_bias_mode_set(uint8_t mode);

/**
 * audio_probe.h
 *
 * \brief   set codec bias pad.
 * \param[in] enable (true: pull high false: pull low)
 * \note
 *
 * \return
 * \retval  zero
 * \retval  non-zero
 *
 * \ingroup AUDIO_PROBE
 */
void audio_probe_codec_bias_pad_set(bool enable);

/**
 * audio_probe.h
 *
 * \brief   set codec adda_loopback.
 * \param[in] mic_sel (0: amic1 1: amic2 2:amic3)
 * \note
 *
 * \return
 * \retval  zero
 * \retval  non-zero
 *
 * \ingroup AUDIO_PROBE
 */
void audio_probe_codec_adda_loopback_set(uint8_t mic_sel);

/**
 * audio_probe.h
 *
 * \brief   start the engine communication w/ DSP2.
 *
 * \note
 *
 * \return              The engine instance id.
 * \retval  zero        The engine was failed to start.
 * \retval  non-zero    The engine was started successfully.
 *
 * \ingroup AUDIO_PROBE
 */
uint32_t engine_start(void);

/**
 * \brief   Stop the engine communication with DSP2.
 *
 * \return
 * \retval  zero        The engine was stopped successfully.
 * \retval  non-zero    The engine was failed to stop.
 *
 * \ingroup AUDIO_PROBE
 */
int32_t engine_stop(uint32_t instance);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_PROBE_H_ */
