/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_PASSTHROUGH_H_
#define _AUDIO_PASSTHROUGH_H_

#include <stdint.h>
#include <stdbool.h>

#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_PASSTHROUGH Audio Passthrough
 *
 * \brief   Enable, disable and control Audio Passthrough instance.
 * \details Audio Passthrough, also known as Ambient Sound Mode, uses built-in microphones to
 *          capture surrounding noise and channel it into your headphones. In real-world situations,
 *          being able to hear ambient sounds even while wearing headphones is crucial for detecting
 *          potentially dangerous situations.
 */

/**
 * \brief Define the Audio Passthrough mode.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
typedef enum t_audio_passthrough_mode
{
    AUDIO_PASSTHROUGH_MODE_NORMAL       = 0x00,
    AUDIO_PASSTHROUGH_MODE_LOW_LATENCY  = 0x01,
} T_AUDIO_PASSTHROUGH_MODE;


/**
 * \brief Define the Audio Passthrough states.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
typedef enum t_audio_passthrough_state
{
    AUDIO_PASSTHROUGH_STATE_RELEASED     = 0x00, /**< Audio Passthrough that was released. */
    AUDIO_PASSTHROUGH_STATE_CREATED      = 0x01, /**< Audio Passthrough that was created. */
    AUDIO_PASSTHROUGH_STATE_STARTING     = 0x02, /**< Audio Passthrough that was starting. */
    AUDIO_PASSTHROUGH_STATE_STARTED      = 0x03, /**< Audio Passthrough that was started. */
    AUDIO_PASSTHROUGH_STATE_STOPPING     = 0x04, /**< Audio Passthrough that was stopping. */
    AUDIO_PASSTHROUGH_STATE_STOPPED      = 0x05, /**< Audio Passthrough that was stopped. */
} T_AUDIO_PASSTHROUGH_STATE;

/**
 * \brief   Create an Audio Passthrough instance.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  sample_rate    The sample rate for the Audio Passthrough instance. The supported values are 48000 and 96000.
 *
 * \note Audio Passthrough is a singleton design that the application shall destroy the instance before creating a new instance.
 *
 * \return          The status of creating Audio Passthrough instance.
 * \retval  true    The Audio Passthrough instance was created successfully.
 * \retval  false   The Audio Passthrough instance was failed to create.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_create(uint32_t sample_rate);

/**
 * \brief   Destroy the Audio Passthrough instance.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \return          The status of destroying Audio Passthrough instance.
 * \retval  true    The Audio Passthrough instance was destroyed successfully.
 * \retval  false   The Audio Passthrough instance was failed to destory.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_destroy(void);

/**
 * \brief   Enable Audio Passthrough instance.
 *
 * \param[in]   mode              The Audio Passthrough mode  \ref T_AUDIO_PASSTHROUGH_MODE
 * \param[in]   llapt_scenario_id The Low Latency Audio Passthrough scenario index
 *
 * \return          The status of enabling Audio Passthrough instance.
 * \retval  true    The Audio Passthrough instance was enabled successfully.
 * \retval  false   The Audio Passthrough instance was failed to enable.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_enable(T_AUDIO_PASSTHROUGH_MODE mode, uint8_t llapt_scenario_id);

/**
 * \brief   Disable Audio Passthrough instance.
 *
 * \return          The status of disabling Audio Passthrough mode.
 * \retval  true    The Audio Passthrough instance was disabled successfully.
 * \retval  false   The Audio Passthrough instance was failed to disable.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_disable(void);

/**
 * \brief   Get the state of the Audio Passthrough stream.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \return  The state \ref T_AUDIO_PASSTHROUGH_STATE of the Audio Passthrough stream.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
T_AUDIO_PASSTHROUGH_STATE audio_passthrough_state_get(void);

/**
 * \brief   Get the maximum volume out level of the Audio Passthrough stream.
 *
 * \return  The maximum volume out level.
 *
 * \note  The maximum volume out level is configured by \ref audio_passthrough_volume_out_max_set,
 *        or the default value provided by the Audio Subsystem.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
uint8_t audio_passthrough_volume_out_max_get(void);

/**
 * \brief   Set the maximum volume out level of the Audio Passthrough stream.
 *
 * \param[in]   level   The maximum volume out level to set.
 *
 * \return  The status of setting the maximum volume out level.
 * \retval  true    The maximum volume out level was set successfully.
 * \retval  false   The maximum volume out level was failed to set.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_out_max_set(uint8_t level);

/**
 * \brief   Get the minimum volume out level of the Audio Passthrough stream.
 *
 * \return  The minimum volume out level.
 *
 * \note  The minimum volume out level is configured by \ref audio_passthrough_volume_out_min_set,
 *        or the default value provided by the Audio Subsystem.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
uint8_t audio_passthrough_volume_out_min_get(void);

/**
 * \brief   Set the minimum volume out level of the Audio Passthrough stream.
 *
 * \param[in]   level   The minimum volume out level to set.
 *
 * \return  The status of setting the minimum volume out level.
 * \retval  true    The minimum volume out level was set successfully.
 * \retval  false   The minimum volume out level was failed to set.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_out_min_set(uint8_t level);

/**
 * \brief   Get the current volume out level of the Audio Passthrough stream.
 *
 * \return  The current volume out level.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
uint8_t audio_passthrough_volume_out_get(void);

/**
 * \brief   Set the current volume out level of the Audio Passthrough stream.
 *
 * \param[in]   level   The volume out level to set.
 *
 * \return  The status of setting volume out level.
 * \retval  true    The volume out level was set successfully.
 * \retval  false   The volume out level was failed to set.
 *
 * \note  The current volume out level shall be set between \ref audio_passthrough_volume_out_min_get
 *        and \ref audio_passthrough_volume_out_max_get.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_out_set(uint8_t level);

/**
 * \brief   Mute the current volume out level of the Audio Passthrough stream.
 *
 * \return          The result of muting the Audio Passthrough current volume out level.
 * \retval true     The Audio Passthrough current volume out level was muted successfully.
 * \retval false    The Audio Passthrough current volume out level was failed to mute.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_out_mute(void);

/**
 * \brief   Unmute the current volume out level of the Audio Passthrough stream.
 *
 * \return          The result of unmuting the Audio Passthrough current volume out level.
 * \retval true     The Audio Passthrough current volume out level was unmuted successfully.
 * \retval false    The Audio Passthrough current volume out level was failed to unmute.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_out_unmute(void);

/**
 * \brief   Get the current volume out mute status of the Audio Passthrough stream.
 *
 * \return          The volume out mute status of the Audio Passthrough stream.
 * \retval true     The Audio Passthrough current volume out was muted.
 * \retval false    The Audio Passthrough current volume out was unmuted.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_out_is_muted(void);

/**
 * \brief   Get the maximum volume in level of the Audio Passthrough stream.
 *
 * \return  The maximum volume in level.
 *
 * \note  The maximum volume in level is configured by \ref audio_passthrough_volume_in_max_set,
 *        or the default value provided by the Audio subsystem.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
uint8_t audio_passthrough_volume_in_max_get(void);

/**
 * \brief   Set the maximum volume in level of the Audio Passthrough stream.
 *
 * \param[in]   level   The maximum volume in level to set.
 *
 * \return  The status of setting the maximum volume in level.
 * \retval  true    The maximum volume in level was set successfully.
 * \retval  false   The maximum volume in level was failed to set.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_in_max_set(uint8_t level);

/**
 * \brief   Get the minimum volume in level of the Audio Passthrough stream.
 *
 * \return  The minimum volume in level.
 *
 * \note  The minimum volume in level is configured by \ref audio_passthrough_volume_in_min_set,
 *        or the default value provided by the Audio subsystem.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
uint8_t audio_passthrough_volume_in_min_get(void);

/**
 * \brief   Set the minimum volume in level of the Audio Passthrough stream.
 *
 * \param[in]   level   The minimum volume in level to set.
 *
 * \return  The status of setting the minimum volume in level.
 * \retval  true    The minimum volume in level was set successfully.
 * \retval  false   The minimum volume in level was failed to set.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_in_min_set(uint8_t level);

/**
 * \brief   Get the current volume in level of the Audio Passthrough stream.
 *
 * \return  The current volume in level.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
uint8_t audio_passthrough_volume_in_get(void);

/**
 * \brief   Set the current volume in level of the Audio Passthrough stream.
 *
 * \param[in]   level   The volume in level to set.
 *
 * \return  The status of setting volume in level.
 * \retval  true    The Volume in level was set successfully.
 * \retval  false   The Volume in level was failed to set.
 *
 * \note  The current volume in level shall be set between \ref audio_passthrough_volume_in_min_get
 *        and \ref audio_passthrough_volume_in_max_get.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_in_set(uint8_t level);

/**
 * \brief   Mute the current volume in level of the Audio Passthrough stream.
 *
 * \return          The result of muting the Audio Passthrough current volume in level.
 * \retval true     The Audio Passthrough current volume in level was muted successfully.
 * \retval false    The Audio Passthrough current volume in level was failed to mute.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_in_mute(void);

/**
 * \brief   Unmute the current volume in level of the Audio Passthrough stream.
 *
 * \return          The result of unmuting the Audio Passthrough current volume in level.
 * \retval true     The Audio Passthrough current volume in level was unmuted successfully.
 * \retval false    The Audio Passthrough current volume in level was failed to unmute.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_in_unmute(void);

/**
 * \brief   Get the current volume in mute status of the Audio Passthrough stream.
 *
 * \return          The volume in mute status of the Audio Passthrough stream.
 * \retval true     The Audio Passthrough current volume in was muted.
 * \retval false    The Audio Passthrough current volume in was unmuted.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_in_is_muted(void);

/**
 * \brief   Get the volume balance scale of the Audio Passthrough stream.
 *
 * \details The volume balance scale ranges from -1.0 to +1.0. If the volume balance scale
 *          is 0.0, the left channel volume and right channel volume are identical; if
 *          the volume balance scale ranges from +0.0 to +1.0, the right channel volume
 *          remains unchanged but the left channel volume scales down to (1.0 - scale)
 *          ratio; if the volume balance scale ranges from -1.0 to -0.0, the left channel
 *          volume remains unchanged but the right channel volume scales down to (1.0 + scale)
 *          ratio.
 *
 * \return  The volume balance scale of the Audio Passthrough stream.
 *          The valid returned values are from -1.0 to +1.0.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
float audio_passthrough_volume_balance_get(void);

/**
 * \brief   Set the volume balance scale of the Audio Passthrough stream.
 *
 * \details The volume balance scale ranges from -1.0 to +1.0. If the volume balance scale
 *          is 0.0, the left channel volume and right channel volume are identical; if
 *          the volume balance scale ranges from +0.0 to +1.0, the right channel volume
 *          remains unchanged but the left channel volume scales down to (1.0 - scale)
 *          ratio; if the volume balance scale ranges from -1.0 to -0.0, the left channel
 *          volume remains unchanged but the right channel volume scales down to (1.0 + scale)
 *          ratio.
 *
 * \param[in] scale The volume balance scale ranges from -1.0 to +1.0.
 *
 * \return          The status of setting the Audio Passthrough stream volume balance scale.
 * \retval true     The volume balance scale was set successfully.
 * \retval false    The volume balance scale was failed to set.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_volume_balance_set(float scale);

/**
 * \brief   Set the brightness strength of the Audio Passthrough stream.
 *
 * \param[in] strength  The brightness strength to set.
 *
 * \return  The status of setting brightness strength.
 * \retval  true    The brightness strength was set successfully.
 * \retval  false   The brightness strength was failed to set.
 *
 * \note  The brightness strength shall be set between 0.0 and 1.0.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_brightness_set(float strength);

/**
 * \brief   Enable the Own Voice Processing (OVP) of the Audio Passthrough stream.
 *
 * \details The OVP separates your own voice from the rest of the soundscape and processes
 *          it in a way that makes it sound as natural as possible.
 *
 * \param[in] level The initial aggressiveness level of own voice volume.
 *
 * \return  The status of enabling the OVP.
 * \retval  true    OVP was enabled successfully.
 * \retval  false   OVP was failed to enable.
 *
 * \note  The OVP aggressiveness level shall be set between 0 and 15. The own voice
 *        volume is decreasing when the OVP aggressiveness level is increasing.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_ovp_enable(uint8_t level);

/**
 * \brief   Set the Own Voice Processing (OVP) aggressiveness level of the Audio Passthrough stream.
 *
 * \details The OVP separates your own voice from the rest of the soundscape and processes
 *          it in a way that makes it sound as natural as possible.
 *
 * \param[in] level The aggressiveness level of own voice volume to set.
 *
 * \return  The status of setting the OVP aggressiveness level.
 * \retval  true    OVP aggressiveness level was set successfully.
 * \retval  false   OVP aggressiveness level was failed to set.
 *
 * \note  The OVP aggressiveness level shall be set between 0 and 15. The own voice
 *        volume is decreasing when the OVP aggressiveness level is increasing.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_ovp_set(uint8_t level);

/**
 * \brief   Disable the Own Voice Processing (OVP) of the Audio Passthrough stream.
 *
 * \details The OVP separates your own voice from the rest of the soundscape and processes
 *          it in a way that makes it sound as natural as possible.
 *
 * \return  The status of disabling the OVP.
 * \retval  true    OVP was disabled successfully.
 * \retval  false   OVP was failed to disable.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_ovp_disable(void);

/**
 * \brief   Enable the Beamforming effect of the Audio Passthrough stream.
 *
 * \details Beamforming technique in the acoustic field is applied to enhance signals from
 *          the desired direction or angle, and suppress noises from other directions.
 *
 * \param[in] direction The desired direction for Beamforming processing.
 *
 * \return  The status of enabling the Beamforming effect.
 * \retval  true    The Beamforming effect was enabled successfully.
 * \retval  false   The Beamforming effect was failed to enable.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_beamforming_enable(uint8_t direction);

/**
 * \brief   Set the Beamforming direction of the Audio Passthrough stream.
 *
 * \details Beamforming technique in the acoustic field is applied to enhance signals from
 *          the desired direction or angle, and suppress noises from other directions.
 *
 * \param[in] direction The desired direction for Beamforming processing.
 *
 * \return  The status of setting the Beamforming direction.
 * \retval  true    The Beamforming direction was set successfully.
 * \retval  false   The Beamforming direction was failed to set.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_beamforming_set(uint8_t direction);

/**
 * \brief   Disable the Beamforming effect of the Audio Passthrough stream.
 *
 * \details Beamforming technique in the acoustic field is applied to enhance signals from
 *          the desired direction or angle, and suppress noises from other directions.
 *
 * \return  The status of disabling the Beamforming effect.
 * \retval  true    The Beamforming effect was disabled successfully.
 * \retval  false   The Beamforming effect was failed to disable.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_beamforming_disable(void);

/**
 * \brief   Attach the Audio Effect instance to the Audio Passthrough stream.
 *
 * \param[in] instance  The Audio Effect instance \ref T_AUDIO_EFFECT_INSTANCE.
 *
 * \return  The status of attaching the Audio Effect instance.
 * \retval  true    The Audio Effect instance was attached successfully.
 * \retval  false   The Audio Effect instance was failed to attach.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_effect_attach(T_AUDIO_EFFECT_INSTANCE instance);

/**
 * \brief   Detach the Audio Effect instance from the Audio Passthrough stream.
 *
 * \param[in] instance  The Audio Effect instance \ref T_AUDIO_EFFECT_INSTANCE.
 *
 * \return  The status of detaching the Audio Effect instance.
 * \retval  true    The Audio Effect instance was detached successfully.
 * \retval  false   The Audio Effect instance was failed to detach.
 *
 * \ingroup AUDIO_PASSTHROUGH
 */
bool audio_passthrough_effect_detach(T_AUDIO_EFFECT_INSTANCE instance);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_PASSTHROUGH_H_ */
