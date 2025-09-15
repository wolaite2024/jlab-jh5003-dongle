/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _VAD_H_
#define _VAD_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_VAD Voice Activity Detection
 *
 * \brief   Enable and disable Voice Activity Detection.
 * \details The Voice Activity Detection (VAD) is a technique used in speech processing to detect
 *          the presence or absence of human speech. The main uses of VAD are in speech recognition
 *          and speech coding, e.g. hot words wakeup and voice recording. It can avoid unnecessary
 *          coding and transmission of silence packets to save on computation and network bandwidth.
 */

/**
 * \brief   Enable the Voice Activity Detection.
 *
 * \return          The status of enabling Voice Activity Detection.
 * \retval  true    Voice Activity Detection was enabled successfully.
 * \retval  false   Voice Activity Detection was failed to enable.
 *
 * \ingroup AUDIO_VAD
 */
bool vad_enable(void);

/**
 * \brief   Disable Voice Activity Detection.
 *
 * \return          The status of disabling Voice Activity Detection.
 * \retval  true    Voice Activity Detection was disabled successfully.
 * \retval  false   Voice Activity Detection was failed to disable.
 *
 * \ingroup AUDIO_VAD
 */
bool vad_disable(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _VAD_H_ */
