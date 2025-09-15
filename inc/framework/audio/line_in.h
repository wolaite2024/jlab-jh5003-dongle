/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _LINE_IN_H_
#define _LINE_IN_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    LINE_IN Line-in
 *
 * \brief   Start, stop and control the line-in playback.
 * \details Line-in is either a digital or an analog jack that can be connected to another audio
 *          output device. Line-in is used to record, relay, or amplify the incoming audio stream.
 */

/**
 * \brief   Create a Line-in playback instance.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  device         The device in and out bitmask \ref AUDIO_DEVICE_BITMASK of the Line-in session.
 * \param[in]  sample_rate    The sample rate for the Line-in playback instance. The supported values are 48000 and 96000.
 *
 * \note Line-in is a singleton design that the application shall destroy the instance before creating a new instance.
 *
 * \return          The status of creating a Line-in playback instance.
 * \retval  true    The Line-in playback instance was created successfully.
 * \retval  false   The Line-in playback instance was failed to create.
 *
 * \ingroup LINE_IN
 */
bool line_in_create(uint32_t device,
                    uint32_t sample_rate);

/**
 * \brief   Destroy the Line-in playback instance.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \return          The status of destroying Line-in playback instance.
 * \retval  true    The Line-in playback instance was destroyed successfully.
 * \retval  false   The Line-in playback instance was failed to destroy.
 *
 * \ingroup LINE_IN
 */
bool line_in_destroy(void);

/**
 * \brief   Start the Line-in playback instance.
 *
 * \return          The status of starting Line-in playback instance.
 * \retval  true    The Line-in playback instance was started successfully.
 * \retval  false   The Line-in playback instance was failed to start.
 *
 * \ingroup LINE_IN
 */
bool line_in_start(void);

/**
 * \brief   Stop the Line-in playback instance.
 *
 * \return          The status of stopping Line-in playback instance.
 * \retval  true    The Line-in playback instance was stopped successfully.
 * \retval  false   The Line-in playback instance was failed to stop.
 *
 * \ingroup LINE_IN
 */
bool line_in_stop(void);

/**
 * \brief   Get the maximum volume out level of the Line-in stream.
 *
 * \return  The maximum volume out level.
 *
 * \ingroup LINE_IN
 */
uint8_t line_in_volume_out_max_get(void);

/**
 * \brief   Set the maximum volume out level of the Line-in stream.
 *
 * \param[in]   level   The maximum volume out level to set.
 *
 * \return          The status of setting the maximum volume out level.
 * \retval  true    The maximum volume out level was set successfully.
 * \retval  false   The maximum volume out level was failed to set.
 *
 * \ingroup LINE_IN
 */
bool line_in_volume_out_max_set(uint8_t level);

/**
 * \brief   Get the minimum volume out level of the Line-in stream.
 *
 * \return  The minimum volume out level.
 *
 * \ingroup LINE_IN
 */
uint8_t line_in_volume_out_min_get(void);

/**
 * \brief   Set the minimum volume out level of the Line-in stream.
 *
 * \param[in]   volume  The minimum volume out level to set.
 *
 * \return          The status of setting the minimum volume out level.
 * \retval  true    The minimum volume out level was set successfully.
 * \retval  false   The minimum volume out level was failed to set.
 *
 * \ingroup LINE_IN
 */
bool line_in_volume_out_min_set(uint8_t volume);

/**
 * \brief   Get the current volume out level of the Line-in stream.
 *
 * \return  The current volume out level.
 *
 * \ingroup LINE_IN
 */
uint8_t line_in_volume_out_get(void);

/**
 * \brief   Set the current volume out level of the Line-in stream.
 *
 * \param[in]   volume  The volume out level to set.
 *
 * \return          The status of setting volume out level.
 * \retval  true    The volume out level was set successfully.
 * \retval  false   The volume out level was failed to set.
 *
 * \ingroup LINE_IN
 */
bool line_in_volume_out_set(uint8_t volume);

/**
 * \brief   Get the maximum volume in level of the Line-in stream.
 *
 * \return  The maximum volume in level.
 *
 * \ingroup LINE_IN
 */
uint8_t line_in_volume_in_max_get(void);

/**
 * \brief   Set the maximum volume in level of the Line-in stream.
 *
 * \param[in]   volume  The maximum volume in level to set.
 *
 * \return          The status of setting the maximum volume in level.
 * \retval  true    The maximum volume in level was set successfully.
 * \retval  false   The maximum volume in level was failed to set.
 *
 * \ingroup LINE_IN
 */
bool line_in_volume_in_max_set(uint8_t volume);

/**
 * \brief   Get the minimum volume in level of the Line-in stream.
 *
 * \return  The minimum volume in level.
 *
 * \ingroup LINE_IN
 */
uint8_t line_in_volume_in_min_get(void);

/**
 * \brief   Set the minimum volume in level of the Line-in stream.
 *
 * \param[in]   volume  The minimum volume in level to set.
 *
 * \return          The status of setting the minimum volume in level.
 * \retval  true    The minimum volume in level was set successfully.
 * \retval  false   The minimum volume in level was failed to set.
 *
 * \ingroup LINE_IN
 */
bool line_in_volume_in_min_set(uint8_t volume);

/**
 * \brief   Get the current volume in level of the Line-in stream.
 *
 * \return  The current volume in level.
 *
 * \ingroup LINE_IN
 */
uint8_t line_in_volume_in_get(void);

/**
 * \brief   Set the current volume in level of the Line-in stream.
 *
 * \param[in]   volume  The volume in level to set.
 *
 * \return          The status of setting volume in level.
 * \retval  true    The volume in level was set successfully.
 * \retval  false   The volume in level was failed to set.
 *
 * \ingroup LINE_IN
 */
bool line_in_volume_in_set(uint8_t volume);

/**
 * \brief  Start monitoring the signal out strength of the Line-in instance.
 *
 * \param[in] refresh_interval  The refresh interval in milliseconds.
 *
 * \return          The status of starting monitoring the signal out strength.
 * \retval  true    The monitoring of the signal out strength was started successfully.
 * \retval  false   The monitoring of the signal out strength was failed to started.
 *
 * \ingroup LINE_IN
 */
bool line_in_signal_out_monitoring_start(uint16_t refresh_interval);

/**
 * \brief  Stop monitoring the signal out strength of the Line-in instance.
 *
 * \return          The status of stopping monitoring the signal out strength.
 * \retval  true    The monitoring of the signal out strength was stopped successfully.
 * \retval  false   The monitoring of the signal out strength was failed to stopped.
 *
 * \ingroup LINE_IN
 */
bool line_in_signal_out_monitoring_stop(void);

/**
 * \brief  Start monitoring the signal in strength of the Line-in instance.
 *
 * \param[in] refresh_interval  The refresh interval in milliseconds.
 *
 * \return          The status of starting monitoring the signal in strength.
 * \retval  true    The monitoring of the signal in strength was started successfully.
 * \retval  false   The monitoring of the signal in strength was failed to started.
 *
 * \ingroup LINE_IN
 */
bool line_in_signal_in_monitoring_start(uint16_t refresh_interval);

/**
 * \brief  Stop monitoring the signal in strength of the Line-in instance.
 *
 * \return          The status of stopping monitoring the signal in strength.
 * \retval  true    The monitoring of the signal in strength was stopped successfully.
 * \retval  false   The monitoring of the signal in strength was failed to stopped.
 *
 * \ingroup LINE_IN
 */
bool line_in_signal_in_monitoring_stop(void);

/**
 * \brief   Get the volume balance scale of the Line-in stream.
 *
 * \details Volume balance scale ranges from -1.0 to +1.0. If the volume balance scale
 *          is 0.0, the left channel volume and right channel volume are identical; if
 *          the volume balance scale ranges from +0.0 to +1.0, the right channel volume
 *          remains unchanged but the left channel volume scales down to (1.0 - scale)
 *          ratio; if the volume balance scale ranges from -1.0 to -0.0, the left channel
 *          volume remains unchanged but the right channel volume scales down to (1.0 + scale)
 *          ratio.
 *
 * \return  The volume balance scale of the Line-in stream.
 *          The valid returned values are from -1.0 to +1.0.
 *
 * \ingroup LINE_IN
 */
float line_in_volume_balance_get(void);

/**
 * \brief   Set the volume balance scale of the Line-in stream.
 *
 * \details Volume balance scale ranges from -1.0 to +1.0. If the volume balance scale
 *          is 0.0, the left channel volume and right channel volume are identical; if
 *          the volume balance scale ranges from +0.0 to +1.0, the right channel volume
 *          remains unchanged but the left channel volume scales down to (1.0 - scale)
 *          ratio; if the volume balance scale ranges from -1.0 to -0.0, the left channel
 *          volume remains unchanged but the right channel volume scales down to (1.0 + scale)
 *          ratio.
 *
 * \param[in] scale The volume balance scale ranges from -1.0 to +1.0.
 *
 * \return          The status of setting the Line-in stream volume balance scale.
 * \retval true     The volume balance scale was set successfully.
 * \retval false    The volume balance scale was failed to set.
 *
 * \ingroup LINE_IN
 */
bool line_in_volume_balance_set(float scale);

/**
 * \brief   Attach the Audio Effect instance to the Line-in stream.
 *
 * \param[in] instance  The Audio Effect instance \ref T_AUDIO_EFFECT_INSTANCE.
 *
 * \return          The status of attaching the Audio Effect instance.
 * \retval  true    The Audio Effect instance was attached successfully.
 * \retval  false   The Audio Effect instance was failed to attach.
 *
 * \ingroup LINE_IN
 */
bool line_in_effect_attach(T_AUDIO_EFFECT_INSTANCE instance);

/**
 * \brief   Detach the Audio Effect instance from the Line-in stream.
 *
 * \param[in] instance  The Audio Effect instance \ref T_AUDIO_EFFECT_INSTANCE.
 *
 * \return          The status of detaching the Audio Effect instance.
 * \retval  true    The Audio Effect instance was detached successfully.
 * \retval  false   The Audio Effect instance was failed to detach.
 *
 * \ingroup LINE_IN
 */
bool line_in_effect_detach(T_AUDIO_EFFECT_INSTANCE instance);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LINE_IN_H_ */
