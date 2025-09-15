/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      hal_gpio_int.h
* @brief     This file provides all the GPIO HAL interrupt functions.
* @details
* @author
* @date
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef _HAL_GPIO_INT_
#define _HAL_GPIO_INT_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup GPIO_INT_H_ HAL GPIO Internal
  * @brief HAL GPIO interrupt driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/

/** @defgroup HAL_GPIO_INTERNAL_Exported_Types HAL GPIO Internal Exported Types
  * @brief  GPIO callback.
  * @{
  */

typedef void (*P_GPIO_CBACK)(uint32_t context);

/** End of group HAL_GPIO_INTERNAL_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup HAL_GPIO_INTERNAL_Exported_Functions HAL GPIO Internal Exported Functions
  * @{
  */

/**
 * hal_gpio_int.h
 *
 * \brief   Register the GPIO interrupt callback for the specific pin.
 *
 * \param[in]  pin_index     The pin index, please refer to rtl876x.h 'Pin_Number' part.
 * \param[in]  callback      The callback to be called when the specific interrupt happened.
 * \param[in]  context       The user data when callback is called.
 *
 * \return                   The status of register interrupt functions.
 * \retval true              The callback is registered successfully.
 * \retval false             The callback is failed to register due to invalid pin number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void gpio_test(void)
 * {
 *     hal_gpio_set_up_irq(TEST_PIN, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
 *     hal_gpio_register_isr_callback(TEST_PIN, gpio_isr_cb, GPIO_DEMO_INPUT_PIN0);
 *     hal_gpio_irq_enable(TEST_PIN);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_INTERNAL_Exported_Functions
 */
bool hal_gpio_register_isr_callback(uint8_t pin_index, P_GPIO_CBACK callback, uint32_t context);

/**
 * hal_gpio_int.h
 *
 * \brief   Get the registered callback for the specific pin.
 *
 * \param[in]  pin_index     The pin index, please refer to rtl876x.h 'Pin_Number' part.
 * \param[in]  p_callback    The pointer for the callback to get.
 * \param[in]  p_context     The pointer for the context to get.
 *
 * \return                   The status of getting registered callback.
 * \retval true              The registered callback is get successfully.
 * \retval false             The registered callback is failed to get due to invalid pin number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void gpio_test(void)
 * {
 *     P_GPIO_CBACK cb = NULL;
 *     uint32_t context = NULL;
 *     hal_gpio_get_isr_callback(TEST_PIN, &cb, &context);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_INTERNAL_Exported_Functions
 */
bool hal_gpio_get_isr_callback(uint8_t pin_index, P_GPIO_CBACK *p_callback, uint32_t *p_context);

/**
 * hal_gpio_int.h
 *
 * \brief   Initialize the HAL GPIO interrupt module.
 *
 * \param[in]  None.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void gpio_test(void)
 * {
 *     hal_gpio_init();
 *     hal_gpio_int_init();
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_INTERNAL_Exported_Functions
 */
void hal_gpio_int_init(void);

#ifdef __cplusplus
}
#endif

#endif

/** @} */ /* End of group HAL_GPIO_INTERNAL_Exported_Functions */
/** @} */ /* End of group GPIO_INT_H_ */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/
