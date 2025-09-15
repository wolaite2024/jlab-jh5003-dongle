/*
 * Copyright (c) 2024, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _HAL_PINMUX_H_
#define _HAL_PINMUX_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "hal_def.h"

/** @addtogroup HAL_PINMUX HAL PINMUX
  * @brief HAL PINMUX driver module.
  * @{
  */

/*============================================================================*
*                         Types
*============================================================================*/
/** @defgroup HAL_PINMUX_Exported_Types HAL PINMUX Exported Types
  * @{
  */
typedef void (*P_PIN_WAKEUP_CALLBACK)(uint32_t context);

typedef enum
{
    HAL_WAKE_UP_POLARITY_LOW,
    HAL_WAKE_UP_POLARITY_HIGH,
    HAL_WAKE_UP_POLARITY_MAX
} T_HAL_WAKE_UP_POLARITY;

/** End of group HAL_PINMUX_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup HAL_PINMUX_Exported_Functions HAL PINMUX Exported Functions
  * @{
  */

/**
 * hal_pinmux.h
 *
 * \brief   Enable PAD wake up function.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \param[in]  polarity     The polarity for the wake up to be set to.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_pad_wake_up_enable(TEST_PIN, HAL_WAKE_UP_POLARITY_HIGH);
 * }
 * \endcode
 *
 */
void hal_pad_wake_up_enable(uint8_t pin_index, T_HAL_WAKE_UP_POLARITY polarity);
/**
 * hal_pinmux.h
 *
 * \brief   Disable PAD wake up function.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_pad_wake_up_disable(TEST_PIN);
 * }
 * \endcode
 *
 */
void hal_pad_wake_up_disable(uint8_t pin_index);
/**
 * hal_pinmux.h
 *
 * \brief   Register PAD wake up callback.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \param[in]  callback     The callback to be called when the specific wake up happened.
 *
 * \param[in]  context      The user data when callback is called.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_pad_register_pin_wake_callback(TEST_PIN, callback, context);
 * }
 * \endcode
 *
 */
void hal_pad_register_pin_wake_callback(uint8_t pin_index, P_PIN_WAKEUP_CALLBACK callback,
                                        uint32_t context);
/**
 * hal_pinmux.h
 *
 * \brief   Unregister PAD wake up callback.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_pad_unregister_wake_callback(TEST_PIN);
 * }
 * \endcode
 *
 */
void hal_pad_unregister_wake_callback(uint8_t pin_index);

/** @} */ /* End of group HAL_PINMUX_Exported_Functions */
/** @} */ /* End of group HAL_PINMUX */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HAL_PINMUX_H_ */
