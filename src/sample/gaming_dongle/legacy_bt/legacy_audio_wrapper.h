/*
 *  Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 */

#ifndef __LEGACY_AUDIO_WRAPPER_H__
#define __LEGACY_AUDIO_WRAPPER_H__

#define PTR_TO_UINT(p)  ((uint32_t)((void *)(p)))
#define UINT_TO_PTR(u)  ((void *)((uint32_t *)(u)))

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup Legacy_Audio_Wrapper Legacy Audio Wrapper
  * @brief Legcy audio wrapper API
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Legacy_Audio_Wrapper_Exported_Functions Legacy Audio Wrapper Functions
  * @{
  */

/**
 * legacy_audio_weapper.h
 *
 * \brief   Legacy audio enter.
 *
 * \return The status of legacy audio enter.
 * \retval 0            Legacy audio enter successfully.
 */
int legacy_audio_enter(void);

/**
 * legacy_audio_weapper.h
 *
 * \brief   Legacy audio exit.
 */
void legacy_audio_exit(void);

/**
 * legacy_audio_weapper.h
 *
 * \brief   Legacy audio init.
 *
 * \param[in] *evt_queue    Audio evt queue handle
 * \param[in] *msg_queue    Audio io queue handle
 *
 * \return The status of legacy audio init.
 * \retval 0            Legacy audio init successfully.
 */
int legacy_audio_init(void *evt_queue, void *msg_queue);

/**
 * legacy_audio_weapper.h
 *
 * \brief   Legacy audio mmi execution.
 *
 * \param[in] mmi_action    MMI action.
 * \param[in] *params       Parameter.
 *
 * \return The result of mmi execution.
 * \retval 0            MMI execution succeeded.
 * \retval others       MMI execution failed.
 */
int legacy_audio_mmi(uint8_t mmi_action, void *params);

uint8_t gaming_get_status(uint8_t index);

/** @} */ /* End of group Legacy_Audio_Wrapper_Exported_Functions */

/** End of Legacy_Audio_Wrapper
* @}
*/

#ifdef __cplusplus
}
#endif

#endif /* __LEGACY_AUDIO_WRAPPER_H__ */
