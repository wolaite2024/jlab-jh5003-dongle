/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _ANC_H_
#define _ANC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_ANC Active Noise Cancellation
 *
 * \brief   Enable and disable Active Noise Cancellation (ANC).
 * \details Active Noise Cancellation (ANC), or Active Noise Reduction (ANR), uses noise-cancelling
 *          speakers to reduce unwanted background noise. ANC will help you get away from the noise
 *          world when enjoying music or just quiet your mind. While enabling ANC will consume more
 *          battery power.
 */

/**
 * \brief   Enable Active Noise Cancellation (ANC).
 *
 * \param[in] scenario_id   ANC scenario ID to set.
 *
 * \return  The result of enabling ANC.
 * \retval  true    ANC was enabled successfully.
 * \retval  false   ANC was failed to enable.
 *
 * \ingroup AUDIO_ANC
 */
bool anc_enable(uint8_t scenario_id);

/**
 * \brief   Disable Active Noise Cancellation (ANC).
 *
 * \return  The result of disabling ANC.
 * \retval  true    ANC was disabled successfully.
 * \retval  false   ANC was failed to disable.
 *
 * \ingroup AUDIO_ANC
 */
bool anc_disable(void);

/**
 * \brief   Start Active Noise Cancellation (ANC) adaptive filter.
 *
 * \return  The result of starting ANC adaptive filter.
 * \retval  true    ANC adaptive filter was started successfully.
 * \retval  false   ANC adaptive filter was failed to start.
 *
 * \ingroup AUDIO_ANC
 */
bool anc_adaptive_filter_start(void);

/**
 * \brief   Pause Active Noise Cancellation (ANC) adaptive filter.
 *
 * \return  The result of pausing ANC adaptive filter.
 * \retval  true    ANC adaptive filter was pauseed successfully.
 * \retval  false   ANC adaptive filter was failed to pause.
 *
 * \ingroup AUDIO_ANC
 */
bool anc_adaptive_filter_pause(void);

/**
 * \brief   Stop Active Noise Cancellation (ANC) adaptive filter.
 *
 * \return  The result of stopping ANC adaptive filter.
 * \retval  true    ANC adaptive filter was stopped successfully.
 * \retval  false   ANC adaptive filter was failed to stop.
 *
 * \ingroup AUDIO_ANC
 */
bool anc_adaptive_filter_stop(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ANC_H_ */
