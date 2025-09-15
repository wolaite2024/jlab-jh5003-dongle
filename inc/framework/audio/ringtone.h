/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _RINGTONE_H_
#define _RINGTONE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_RINGTONE Ringtone
 *
 * \brief   Play and stop the Ringtone.
 * \details Ringtone plays the sine-wave synthesized sound. It is used as a signal
 *          for notification, alarm, prompt, or other situations.
 */

/**
 * \brief Define the Ringtone modes.
 *
 * \details Ringtone has three modes which are audible mode, silent mode, and volume fixed mode.
 *          Audible mode is Ringtone's default mode that Ringtone can be played and stopped, and
 *          Ringtone volume can be adjusted, muted and unmuted. Silent mode will disable Ringtone
 *          and mute Ringtone volume that Ringtone cannot be played and its volume cannot be
 *          adjusted. Fixed mode will make Ringtone volume unadjustable.
 *
 * \ingroup AUDIO_RINGTONE
 */
typedef enum t_ringtone_mode
{
    RINGTONE_MODE_AUDIBLE   = 0x00, /**< Ringtone is enabled and audible. */
    RINGTONE_MODE_SILENT    = 0x01, /**< Ringtone is disabled and silent. */
    RINGTONE_MODE_FIXED     = 0x02, /**< Ringtone is enabled, but volume is fixed and not adjustable. */
} T_RINGTONE_MODE;

/**
 * \brief   Get the current Ringtone mode.
 *
 * \return  The current Ringtone mode \ref T_RINGTONE_MODE.
 *
 * \ingroup AUDIO_RINGTONE
 */
T_RINGTONE_MODE ringtone_mode_get(void);

/**
 * \brief   Set the current Ringtone mode.
 *
 * \param[in] mode  The Ringtone mode \ref T_RINGTONE_MODE.
 *
 * \return          The result of setting the Ringtone mode.
 * \retval true     The Ringtone mode was set successfully.
 * \retval false    The Ringtone mode was failed to set.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_mode_set(T_RINGTONE_MODE mode);

/**
 * \brief   Get the maximum volume level of the Ringtone.
 *
 * \return  The maximum volume level of the Ringtone.
 *
 * \ingroup AUDIO_RINGTONE
 */
uint8_t ringtone_volume_max_get(void);

/**
 * \brief   Set the maximum volume level of the Ringtone.
 *
 * \param[in] volume    The maximum volume level of the Ringtone.
 *
 * \return          The result of setting the Ringtone maximum volume level.
 * \retval  true    The Ringtone maximum volume level was set successfully.
 * \retval  false   The Ringtone maximum volume level was failed to set.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_volume_max_set(uint8_t volume);

/**
 * \brief   Get the minimum volume level of the Ringtone.
 *
 * \return  The minimum volume level of the Ringtone.
 *
 * \ingroup AUDIO_RINGTONE
 */
uint8_t ringtone_volume_min_get(void);

/**
 * \brief   Set the minimum volume level of the Ringtone.
 *
 * \param[in] volume    The minimum volume level of the Ringtone.
 *
 * \return          The result of setting the Ringtone minimum volume level.
 * \retval  true    The Ringtone minimum volume level was set successfully.
 * \retval  false   The Ringtone minimum volume level was failed to set.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_volume_min_set(uint8_t volume);

/**
 * \brief   Get the current volume level of the Ringtone.
 *
 * \return  The current volume level of the Ringtone.
 *
 * \ingroup AUDIO_RINGTONE
 */
uint8_t ringtone_volume_get(void);

/**
 * \brief   Set the current volume level of the Ringtone.
 *
 * \details If the Ringtone volume was muted previously, setting the volume at any level excluding
 *          the minimum level \ref ringtone_volume_min_get() will unmute and activate the volume level.
 *          When the Ringtone volume was already muted, setting the volume at the minimum level will
 *          have no effect.
 *
 * \note    This action will be permitted only when the Ringtone mode is \ref RINGTONE_MODE_AUDIBLE.
 *          See \ref T_RINGTONE_MODE for details.
 *
 * \param[in] volume    The volume level to set. See \ref ringtone_volume_max_get() and \ref
 *                      ringtone_volume_min_get() for the valid volume level range.
 *
 * \return          The result of setting the Ringtone current volume level.
 * \retval  true    The Ringtone current volume level was set successfully.
 * \retval  false   The Ringtone current volume level was failed to set.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_volume_set(uint8_t volume);

/**
 * \brief   Mute the current volume level of the Ringtone.
 *
 * \note    This action will be permitted only when the Ringtone mode is \ref RINGTONE_MODE_AUDIBLE.
 *          See \ref T_RINGTONE_MODE for details.
 *
 * \return          The result of muting the Ringtone volume level.
 * \retval true     The Ringtone volume level was muted successfully.
 * \retval false    The Ringtone volume level was failed to mute.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_volume_mute(void);

/**
 * \brief   Unmute the current volume level of the Ringtone.
 *
 * \note    This action will be permitted only when the Ringtone mode is \ref RINGTONE_MODE_AUDIBLE.
 *          See \ref T_RINGTONE_MODE for details.
 *
 * \return          The result of unmuting the Ringtone volume level.
 * \retval true     The Ringtone volume level was unmuted successfully.
 * \retval false    The Ringtone volume level was failed to unmute.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_volume_unmute(void);

/**
 * \brief   Get the volume balance scale of the Ringtone.
 *
 * \details Volume balance scale ranges from -1.0 to +1.0. If the volume balance scale
 *          is 0.0, the left channel volume and right channel volume are identical; if
 *          the volume balance scale ranges from +0.0 to +1.0, the right channel volume
 *          remains unchanged but the left channel volume scales down to (1.0 - scale)
 *          ratio; if the volume balance scale ranges from -1.0 to -0.0, the left channel
 *          volume remains unchanged but the right channel volume scales down to (1.0 + scale)
 *          ratio.
 *
 * \return  The volume balance scale of the Ringtone.
 *          The valid returned values are from -1.0 to +1.0.
 *
 * \ingroup RINGTONE
 */
float ringtone_volume_balance_get(void);

/**
 * \brief   Set the volume balance scale of the Ringtone.
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
 * \return          The status of setting the Ringtone volume balance scale.
 * \retval true     The volume balance scale was set successfully.
 * \retval false    The volume balance scale was failed to set.
 *
 * \ingroup RINGTONE
 */
bool ringtone_volume_balance_set(float scale);

/**
 * \brief   Play a Ringtone.
 *
 * \details The Ringtone is first pushed into the notification queue inside Audio Subsystem. When
 *          there are no pending ringtones or voice prompts in the notification queue, this Ringtone
 *          will be played immediately, otherwise, it will be scheduled until all the preceding
 *          ringtones or voice prompts are popped from the notification queue. Upper layer will be
 *          notified by the audio event \ref AUDIO_EVENT_RINGTONE_STARTED when started, and by the
 *          audio event \ref AUDIO_EVENT_RINGTONE_STOPPED when stopped.
 *
 * \note    The notification queue inside Audio Subsystem is basically a first-in, fist-out (FIFO)
 *          queue, So all the ringtones and voice prompts pending in the notification queue will
 *          be played in sequence. Upper layer can use \ref ringtone_cancel to cancel these
 *          ringtones with the required index, or \ref ringtone_flush to flush all the ringtones
 *          pending in the notification queue.
 *          This action will be permitted only when the Ringtone mode is \ref RINGTONE_MODE_AUDIBLE
 *          or \ref RINGTONE_MODE_FIXED. See \ref T_RINGTONE_MODE for details.
 *
 * \param[in] index    The index of the playing Ringtone.
 * \param[in] relay    Relay the Ringtone to the peer identical device.
 *
 * \return          The result of playing a Ringtone.
 * \retval true     The Ringtone was played successfully.
 * \retval false    The Ringtone was failed to play.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_play(uint8_t index, bool relay);

/**
 * \brief   Stop the playing Ringtone.
 *
 * \details All the ringtones pending in the notification queue inside Audio Subsystem will
 *          not be affected.
 *
 * \note    This action will be permitted only when the Ringtone mode is \ref RINGTONE_MODE_AUDIBLE
 *          or \ref RINGTONE_MODE_FIXED. See \ref T_RINGTONE_MODE for details.
 *
 * \return          The result of stopping the Ringtone.
 * \retval true     The Ringtone was stopped successfully.
 * \retval false    The Ringtone was failed to stop.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_stop(void);

/**
 * \brief   Cancel the specific Ringtone in the queue.
 *
 * \details All the ringtones with the required index pending in the notification queue inside
 *          Audio Subsystem will be canceled. Note that if there is a playing Ringtone with the
 *          same index, this playing Ringtone will also be stopped immediately.
 *
 * \note    This action will be permitted only when the Ringtone mode is \ref RINGTONE_MODE_AUDIBLE
 *          or \ref RINGTONE_MODE_FIXED. See \ref T_RINGTONE_MODE for details.
 *
 * \param[in] index The index of the queued Ringtone.
 * \param[in] relay Relay the Ringtone canceling action to the peer identical device.
 *
 * \return          The result of canceling Ringtone.
 * \retval true     The Ringtone was canceled successfully.
 * \retval false    The Ringtone was failed to cancel.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_cancel(uint8_t index, bool relay);

/**
 * \brief   Flush all ringtones in the queue.
 *
 * \details All the ringtones pending in the notification queue inside Audio Subsystem will
 *          be flushed immediately. If there is a Ringtone currently under playing, it will
 *          not be affected by this flush operation.
 *
 * \note    This action will be permitted only when the Ringtone mode is \ref RINGTONE_MODE_AUDIBLE
 *          or \ref RINGTONE_MODE_FIXED. See \ref T_RINGTONE_MODE for details.
 *
 * \param[in] relay Relay the Ringtone flushing action to the peer identical device.
 *
 * \return          The result of flush the ringtones.
 * \retval true     The Ringtone was flushed successfully.
 * \retval false    The Ringtone was failed to flush.
 *
 * \ingroup AUDIO_RINGTONE
 */
bool ringtone_flush(bool relay);

/**
 * \brief   Return the count remaining ringtones.
 *
 * \return  The count of remaining ringtones.
 *
 * \ingroup AUDIO_RINGTONE
 */
uint8_t ringtone_remaining_count_get(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RINGTONE_H_ */
