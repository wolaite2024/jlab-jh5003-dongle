/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _VOICE_PROMPT_H_
#define _VOICE_PROMPT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_VOICE_PROMPT Voice Prompt
 *
 * \brief   Play and control the Voice Prompt.
 * \details Voice Prompt (VP) plays voice files with the required languages. It is used when
 *          prompting for notification, alarm, user-confirmation, as well as other situations.
 */

/**
 * \brief Define the Voice Prompt modes.
 *
 * \details Voice Prompt has three modes which are audible mode, silent mode, and volume fixed mode.
 *          Audible mode is Voice Prompt's default mode that Voice Prompt can be played and stopped,
 *          and Voice Prompt volume can be adjusted, muted and unmuted. Silent mode will disable
 *          Voice Prompt and mute Voice Prompt volume that Voice Prompt cannot be played and its
 *          volume cannot be adjusted. Fixed mode will make Voice Prompt volume unadjustable.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
typedef enum t_voice_prompt_mode
{
    VOICE_PROMPT_MODE_AUDIBLE   = 0x00, /**< Voice Prompt is enabled and audible. */
    VOICE_PROMPT_MODE_SILENT    = 0x01, /**< Voice Prompt is disabled and silent. */
    VOICE_PROMPT_MODE_FIXED     = 0x02, /**< Voice Prompt is enabled, but volume is fixed and not adjustable. */
} T_VOICE_PROMPT_MODE;

/**
 * \brief   Get the current Voice Prompt mode.
 *
 * \return  The current Voice Prompt mode \ref T_VOICE_PROMPT_MODE.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
T_VOICE_PROMPT_MODE voice_prompt_mode_get(void);

/**
 * \brief   Set the current Voice Prompt mode.
 *
 * \param[in] mode  The Voice Prompt mode \ref T_VOICE_PROMPT_MODE.
 *
 * \return          The result of setting the Voice Prompt mode.
 * \retval true     The Voice Prompt mode was set successfully.
 * \retval false    The Voice Prompt mode was failed to set.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_mode_set(T_VOICE_PROMPT_MODE mode);

/**
 * \brief   Voice Prompt language list.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
typedef enum t_voice_prompt_language_id
{
    VOICE_PROMPT_LANGUAGE_ENGLISH       = 0x00,
    VOICE_PROMPT_LANGUAGE_CHINESE       = 0x01,
    VOICE_PROMPT_LANGUAGE_FRENCH        = 0x02,
    VOICE_PROMPT_LANGUAGE_PORTUGUESE    = 0x03,
    VOICE_PROMPT_LANGUAGE_INVALID       = 0x04,
} T_VOICE_PROMPT_LANGUAGE_ID;

/**
 * \brief   Set global Voice Prompt language.
 *
 * \param[in] language  The Language for global Voice Prompt \ref T_VOICE_PROMPT_LANGUAGE_ID.
 *
 * \return          The result of setting global Voice Prompt language.
 * \retval true     The global Voice Prompt language was set successfully.
 * \retval false    The global Voice Prompt language was failed to set.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_language_set(T_VOICE_PROMPT_LANGUAGE_ID language);

/**
 * \brief   Get current global Voice Prompt language.
 *
 * \return  The current global Voice Prompt language \ref T_VOICE_PROMPT_LANGUAGE_ID. If failed,
 *          VOICE_PROMPT_LANGUAGE_INVALID is returned.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
T_VOICE_PROMPT_LANGUAGE_ID voice_prompt_language_get(void);

/**
 * \brief   Get all supported Voice Prompt languages.
 *
 * \return  Bitmap for supported Voice Prompt languages \ref T_VOICE_PROMPT_LANGUAGE_ID.
 *          Bit value is 1 << \ref T_VOICE_PROMPT_LANGUAGE_ID.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
uint8_t voice_prompt_supported_languages_get(void);

/**
 * \brief   Roll the next Voice Prompt language.
 *
 * \return          The result of rolling Voice Prompt.
 * \retval true     The Voice Prompt language was rolled successfully.
 * \retval false    The Voice Prompt language was failed to roll.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_language_roll(void);

/**
 * \brief   Get the maximum volume level of the Voice Prompt.
 * \brief   Get the maximum volume level of the Voice Prompt.
 *
 * \return  The maximum volume level of the Voice Prompt.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
uint8_t voice_prompt_volume_max_get(void);

/**
 * \brief   Set the maximum volume level of the Voice Prompt.
 *
 * \param[in] volume    The maximum volume level of the Voice Prompt.
 *
 * \return          The result of setting the Voice Prompt maximum volume level.
 * \retval  true    The Voice Prompt maximum volume level was set successfully.
 * \retval  false   The Voice Prompt maximum volume level was failed to set.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_volume_max_set(uint8_t volume);

/**
 * \brief   Get the minimum volume level of the Voice Prompt.
 *
 * \return  The minimum volume level of the Voice Prompt.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
uint8_t voice_prompt_volume_min_get(void);

/**
 * \brief   Set the minimum volume level of the Voice Prompt.
 *
 * \param[in] volume    The minimum volume level of the Voice Prompt.
 *
 * \return          The result of setting the Voice Prompt minimum volume level.
 * \retval  true    The Voice Prompt minimum volume level was set successfully.
 * \retval  false   The Voice Prompt minimum volume level was failed to set.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_volume_min_set(uint8_t volume);

/**
 * \brief   Get the current volume level of the Voice Prompt.
 *
 * \return  The current volume level of the Voice Prompt.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
uint8_t voice_prompt_volume_get(void);

/**
 * \brief   Set the current volume level of the Voice Prompt.
 *
 * \details If the Voice Prompt volume was muted previously, setting the volume at any level excluding
 *          the minimum level \ref voice_prompt_volume_min_get() will unmute and activate the volume
 *          level. When the Voice Prompt volume was already muted, setting the volume at the minimum
 *          level will have no effect.
 *
 * \param[in] volume    The volume level to set. See \ref voice_prompt_volume_max_get() and
 *                      \ref voice_prompt_volume_min_get() for the valid volume level range.
 *
 * \return          The result of setting the Voice Prompt current volume level.
 * \retval  true    The Voice Prompt current volume level was set successfully.
 * \retval  false   The Voice Prompt current volume level was failed to set.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_volume_set(uint8_t volume);

/**
 * \brief   Mute the current volume level of the Voice Prompt.
 *
 * \return          The result of muting the Voice Prompt volume level.
 * \retval true     The Voice Prompt volume level was muted successfully.
 * \retval false    The Voice Prompt volume level was failed to mute.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_volume_mute(void);

/**
 * \brief   Unmute the current volume level of the Voice Prompt.
 *
 * \return          The result of muting the Voice Prompt volume level.
 * \retval true     The Voice Prompt volume level was unmuted successfully.
 * \retval false    The Voice Prompt volume level was failed to unmute.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_volume_unmute(void);

/**
 * \brief   Get the volume balance scale of the Voice Prompt.
 *
 * \details Volume balance scale ranges from -1.0 to +1.0. If the volume balance scale
 *          is 0.0, the left channel volume and right channel volume are identical; if
 *          the volume balance scale ranges from +0.0 to +1.0, the right channel volume
 *          remains unchanged but the left channel volume scales down to (1.0 - scale)
 *          ratio; if the volume balance scale ranges from -1.0 to -0.0, the left channel
 *          volume remains unchanged but the right channel volume scales down to (1.0 + scale)
 *          ratio.
 *
 * \return  The volume balance scale of the Voice Prompt.
 *          The valid returned values are from -1.0 to +1.0.
 *
 * \ingroup VOICE_PROMPT
 */
float voice_prompt_volume_balance_get(void);

/**
 * \brief   Set the volume balance scale of the Voice Prompt.
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
 * \return          The status of setting the Voice Prompt volume balance scale.
 * \retval true     The volume balance scale was set successfully.
 * \retval false    The volume balance scale was failed to set.
 *
 * \ingroup VOICE_PROMPT
 */
bool voice_prompt_volume_balance_set(float scale);

/**
 * \brief   Play a Voice Prompt.
 *
 * \details The Voice Prompt is first pushed into the notification queue inside Audio Subsystem.
 *          When there are no pending voice prompts or ringtones in the notification queue, this
 *          Voice Prompt will be played immediately, otherwise, it will be scheduled until all
 *          the preceding voice prompts or ringtones are popped from the notification queue. Upper
 *          layer will be notified by the audio event \ref AUDIO_EVENT_VOICE_PROMPT_STARTED when
 *          started, and by the audio event \ref AUDIO_EVENT_VOICE_PROMPT_STOPPED when stopped.
 *
 * \note    The notification queue inside Audio Subsystem is basically a first-in, fist-out (FIFO)
 *          queue, So all the voice prompts and ringtones pending in the notification queue will
 *          be played in sequence. Upper layer can use \ref voice_prompt_cancel to cancel these
 *          voice prompts with the required index, or \ref voice_prompt_flush to flush all the
 *          voice prompts pending in the notification queue.
 *          This action will be permitted only when the Voice Prompt mode is \ref VOICE_PROMPT_MODE_AUDIBLE
 *          or \ref VOICE_PROMPT_MODE_FIXED. See \ref T_VOICE_PROMPT_MODE for details.
 *
 * \param[in] index     The type of the playing Voice Prompt.
 * \param[in] language  Language for the playing Voice Prompt \ref T_VOICE_PROMPT_LANGUAGE_ID.
 * \param[in] relay     Relay the Voice Prompt to the peer identical device.
 *
 * \return          The result of playing Voice Prompt.
 * \retval true     The Voice Prompt was played successfully.
 * \retval false    The Voice Prompt was failed to play.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_play(uint8_t index, T_VOICE_PROMPT_LANGUAGE_ID language, bool relay,
                       uint32_t addr, uint32_t len);

/**
 * \brief   Stop the playing Voice Prompt.
 *
 * \details All the voice prompts pending in the notification queue inside Audio Subsystem
 *          will not be affected.
 *
 * \note    This action will be permitted only when the Voice Prompt mode is \ref VOICE_PROMPT_MODE_AUDIBLE
 *          or \ref VOICE_PROMPT_MODE_FIXED. See \ref T_VOICE_PROMPT_MODE for details.
 *
 * \return          The result of stopping Voice Prompt.
 * \retval true     The Voice Prompt was stopped successfully.
 * \retval false    The Voice Prompt was failed to stop.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_stop(void);

/**
 * \brief   Cancel the specific Voice Prompt in the queue.
 *
 * \details All the voice prompts with the required index pending in the notification queue
 *          inside Audio Subsystem will be canceled. Note that if there is a playing Voice
 *          Prompt with the same index, this playing Voice Prompt will also be stopped immediately.
 *
 * \note    This action will be permitted only when the Voice Prompt mode is \ref VOICE_PROMPT_MODE_AUDIBLE
 *          or \ref VOICE_PROMPT_MODE_FIXED. See \ref T_VOICE_PROMPT_MODE for details.
 *
 * \param[in] index     The index of the queued Voice Prompt.
 * \param[in] relay     Relay the Voice Prompt canceling action to the peer identical device.
 *
 * \return          The result of canceling Voice Prompt.
 * \retval true     The Voice Prompt was canceled successfully.
 * \retval false    The Voice Prompt was failed to cancel.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_cancel(uint8_t index, bool relay);

/**
 * \brief   Flush all the pending voice prompts in the queue.
 *
 * \details All the voice prompts pending in the notification queue inside Audio Subsystem
 *          will be flushed immediately. If there is a Voice Prompt currently under playing,
 *          it will not be affected by this flush operation.
 *
 * \note    This action will be permitted only when the Voice Prompt mode is \ref VOICE_PROMPT_MODE_AUDIBLE
 *          or \ref VOICE_PROMPT_MODE_FIXED. See \ref T_VOICE_PROMPT_MODE for details.
 *
 * \param[in] relay Relay the Voice Prompt flushing action to the peer identical device.
 *
 * \return          The result of flushing the voice prompts.
 * \retval true     The Voice Prompt was flushed successfully.
 * \retval false    The Voice Prompt was failed to flush.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
bool voice_prompt_flush(bool relay);

/**
 * \brief   Return the count remaining voice prompts.
 *
 * \return  The count of remaining voice prompts.
 *
 * \ingroup AUDIO_VOICE_PROMPT
 */
uint8_t voice_prompt_remaining_count_get(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _VOICE_PROMPT_H_ */
