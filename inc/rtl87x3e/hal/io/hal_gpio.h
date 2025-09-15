/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      hal_gpio.h
* @brief
* @details
* @author
* @date
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup HAL_GPIO_H_ HAL GPIO
  * @brief HAL GPIO device driver module.
  * @{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/

/** @defgroup HAL_GPIO_Exported_Constants HAL GPIO Exported Constants
  * @{
  */

typedef enum
{
    GPIO_STATUS_ERROR              = -3,        /**< The GPIO function failed to execute.*/
    GPIO_STATUS_ERROR_PIN          = -2,        /**< Invalid input pin number. */
    GPIO_STATUS_INVALID_PARAMETER  = -1,        /**< Invalid input parameter. */
    GPIO_STATUS_OK                 = 0,         /**< The GPIO function executed successfully. */
} T_GPIO_STATUS;

typedef enum
{
    GPIO_TYPE_AUTO      = 0,    /**< The GPIO function switch automatically, use core GPIO in active mode,
                                    use aon GPIO in low power mode.*/
    GPIO_TYPE_CORE      = 1,    /**< The GPIO function in core domain, only works when active mode.
                                    Support all the features. */
    GPIO_TYPE_AON       = 2,    /**< The GPIO function in aon domain, works in DLPS/power down,
                                    much slower than CORE mode. Reading level and interrupt are not supported for GPIO_TYPE_AON.
                                    If gpio type set to GPIO_TYPE_AON in input mode, hal_gpio will switch to GPIO_TYPE_AUTO. */
} T_GPIO_TYPE;

typedef enum
{
    GPIO_DIR_INPUT     = 0,
    GPIO_DIR_OUTPUT    = 1,
} T_GPIO_DIRECTION;

typedef enum
{
    GPIO_LEVEL_LOW     = 0,
    GPIO_LEVEL_HIGH    = 1,
    GPIO_LEVEL_UNKNOWN = 2,
} T_GPIO_LEVEL;

typedef enum
{
    GPIO_IRQ_EDGE,
    GPIO_IRQ_LEVEL,
} T_GPIO_IRQ_MODE;

typedef enum
{
    GPIO_IRQ_ACTIVE_LOW,
    GPIO_IRQ_ACTIVE_HIGH,
} T_GPIO_IRQ_POLARITY;

typedef enum
{
    GPIO_PULL_DOWN,   // band   PAD_PULL_DOWN,
    GPIO_PULL_UP,     //        PAD_PULL_UP,
    GPIO_PULL_NONE    //        PAD_PULL_NONE
} T_GPIO_PULL_VALUE;

/** End of group HAL_GPIO_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup HAL_GPIO_Exported_Types HAL GPIO Exported Types
  * @brief  GPIO callback.
  * @{
  */

typedef void (*P_GPIO_CBACK)(uint32_t context);

/** End of group HAL_GPIO_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup HAL_GPIO_Exported_Functions HAL GPIO Exported Functions
  * @{
  */

/**
 * hal_gpio.h
 *
 * \brief   Initialize a pin to GPIO mode.
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \param[in]  type         The TYPE of GPIO mode to be used, could be auto, core and aon type.
 *
 * \param[in]  direction    The direction of GPIO is set to, could be output mode or input mode.
 *
 * \param[in]  pull_value   The GPIO pull value is set to , could be pull up, down or none.
 *
 * \return The status of the GPIO pin initialization.
 * \retval GPIO_STATUS_OK        The GPIO pin was initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN The GPIO pin was failed to initialized due to invalid pin number.
 * \retval GPIO_STATUS_ERROR     The GPIO pin was failed to initialized due to malloc failure.
 * \retval GPIO_STATUS_INVALID_PARAMETER  The GPIO pin was failed to initialized due to invalid input paramter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_init_pin(TEST_PIN, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT ,GPIO_PULL_UP );
 *     hal_gpio_init_pin(TEST_PIN_2, GPIO_TYPE_CORE, GPIO_DIR_INPUT ,GPIO_PULL_UP);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_STATUS hal_gpio_init_pin(uint8_t pin_index, T_GPIO_TYPE type, T_GPIO_DIRECTION direction,
                                T_GPIO_PULL_VALUE pull_value);

/**
 * hal_gpio.h
 *
 * \brief    Change GPIO direction.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \param[in]  direction    The direction GPIO is set to, could be output mode or input mode.
 *
 * \return                       The status of changing direction of the GPIO pin.
 * \retval GPIO_STATUS_OK        The GPIO pin changes direction successfully.
 * \retval GPIO_STATUS_ERROR_PIN The GPIO pin was failed to change direction due to invalid pin number.
 * \retval GPIO_STATUS_ERROR     The GPIO pin was failed to change direction due to malloc failure.
 * \retval GPIO_STATUS_INVALID_PARAMETER  The GPIO pin was failed to change direction due to invalid input paramter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_change_direction(TEST_PIN, GPIO_DIR_OUTPUT);
 *     hal_gpio_change_direction(TEST_PIN_2, GPIO_DIR_INPUT);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_STATUS hal_gpio_change_direction(uint8_t pin_index, T_GPIO_DIRECTION direction);

/**
 * hal_gpio.h
 *
 * \brief   Set the GPIO pin pull value.
 *
 * \xrefitem Added_API_2_11_1_0 "Added Since 2.11.1.0" "Added API"
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \param[in]  pull_value        The value for the specific pin to pull up, down or none.
 *
 * \return                   The status of setting pull level of the specific GPIO pin.
 * \retval GPIO_STATUS_OK        The GPIO pin set pull level successfully.
 * \retval GPIO_STATUS_ERROR_PIN The GPIO pin was failed to set pull level due to invalid pin number.
 * \retval GPIO_STATUS_ERROR     The GPIO pin was failed to set pull level due to malloc failure.
 * \retval GPIO_STATUS_INVALID_PARAMETER  The GPIO pin was failed to set pull level due to invalid input paramter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_set_pull_value(TEST_PIN_2, GPIO_PULL_UP);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_STATUS hal_gpio_set_pull_value(uint8_t pin_index, T_GPIO_PULL_VALUE pull_value);

/**
 * hal_gpio.h
 *
 * \brief   Set the GPIO pin output level.
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \param[in]  level        The level for the specific pin to output, can be set to high or low level.
 *
 * \return                   The status of setting the output level of the specific GPIO pin.
 * \retval GPIO_STATUS_OK        The GPIO pin set the output level successfully.
 * \retval GPIO_STATUS_ERROR_PIN The GPIO pin was failed to set the output level due to invalid pin number.
 * \retval GPIO_STATUS_ERROR     The GPIO pin was failed to set the output level due to malloc failure.
 * \retval GPIO_STATUS_INVALID_PARAMETER  The GPIO pin was failed to set the output level due to invalid input paramter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_init_pin(TEST_PIN, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT);
 *     hal_gpio_set_level(TEST_PIN_2, GPIO_LEVEL_HIGH);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_STATUS hal_gpio_set_level(uint8_t pin_index, T_GPIO_LEVEL level);

/**
 * hal_gpio.h
 *
 * \brief   Get the pin current level.
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \return                   The current pin level of the specific GPIO pin.
 * \retval GPIO_LEVEL_LOW    The pin is low level.
 * \retval GPIO_LEVEL_HIGH   The pin is high level.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     T_GPIO_LEVEL gpio_level = hal_gpio_get_input_level(TEST_PIN);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_LEVEL hal_gpio_get_input_level(uint8_t pin_index);

/**
 * hal_gpio.h
 *
 * \brief   Set up the IRQ trigger mode of the specified pin.
 *
 * \param[in]  pin_index          The pin index, please refer to rtl876x.h 'Pin_Number' part.
 * \param[in]  mode               The interrupt mode to be set to, can be level or edge trigger.
 * \param[in]  polarity           The polarity for the interrupt to be set to, can be high or low active.
 * \param[in]  debounce_enable    Enable or Disable the pin hardware debounce feature.
 *
 * \return                   The status of setting up IRQ trigger mode of the specific GPIO pin.
 * \retval GPIO_STATUS_OK        The GPIO pin set up IRQ trigger mode successfully.
 * \retval GPIO_STATUS_ERROR_PIN The GPIO pin was failed to set up IRQ trigger mode due to invalid pin number.
 * \retval GPIO_STATUS_ERROR     The GPIO pin was failed to set up IRQ trigger mode due to malloc failure.
 * \retval GPIO_STATUS_INVALID_PARAMETER  The GPIO pin was failed to set up IRQ trigger mode due to invalid input paramter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_init_pin(TEST_PIN, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_UP);
 *     hal_gpio_set_up_irq(TEST_PIN, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_STATUS hal_gpio_set_up_irq(uint8_t pin_index, T_GPIO_IRQ_MODE mode,
                                  T_GPIO_IRQ_POLARITY polarity,
                                  bool debounce_enable);

/**
 * hal_gpio.h
 *
 * \brief   Enable the interrupt of the specified pin.
 *
 * \param[in]  pin_index          The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \return                   The status of enable interrupt of the specific GPIO pin.
 * \retval GPIO_STATUS_OK        The GPIO pin enables interrupt successfully.
 * \retval GPIO_STATUS_ERROR_PIN The GPIO pin was failed to enable interrupt due to invalid pin number.
 * \retval GPIO_STATUS_ERROR     The GPIO pin was failed to enable interrupt due to malloc failure.
 * \retval GPIO_STATUS_INVALID_PARAMETER  The GPIO pin was failed to enable interrupt due to invalid input paramter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_set_up_irq(TEST_PIN, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
 *     hal_gpio_register_isr_callback(TEST_PIN, gpio_isr_cb, GPIO_DEMO_INPUT_PIN0);
 *     hal_gpio_irq_enable(TEST_PIN);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_STATUS hal_gpio_irq_enable(uint8_t pin_index);


/**
 * hal_gpio.h
 *
 * \brief   Disable the interrupt of the specified pin.
 *
 * \param[in]  pin_index          The pin index, please refer to rtl876x.h 'Pin_Number' part.
 *
 * \return                   The status of disable interrupt of the specific GPIO pin.
 * \retval GPIO_STATUS_OK        The GPIO pin disables interrupt successfully.
 * \retval GPIO_STATUS_ERROR_PIN The GPIO pin was failed to disable interrupt due to invalid pin number.
 * \retval GPIO_STATUS_ERROR     The GPIO pin was failed to disable interrupt due to malloc failure.
 * \retval GPIO_STATUS_INVALID_PARAMETER  The GPIO pin was failed to disable interrupt due to invalid input paramter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_irq_disable(TEST_PIN);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_STATUS hal_gpio_irq_disable(uint8_t pin_index);

/**
 * hal_gpio.h
 *
 * \brief   Set the hardware debounce time. The interrupt could be triggered after 1 to 2 of debounce time.
 *
 * \param[in]  ms          The debounce time, unit ms, can be 1~64 ms.
 *
 * \return                   The status of setting debounce time.
 * \retval GPIO_STATUS_OK        The GPIO pin set debounce time successfully.
 * \retval GPIO_STATUS_ERROR_PIN The GPIO pin was failed to set debounce time due to invalid pin number.
 * \retval GPIO_STATUS_ERROR     The GPIO pin was failed to set debounce time due to malloc failure.
 * \retval GPIO_STATUS_INVALID_PARAMETER  The GPIO pin was failed to set debounce time due to invalid input paramter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_init();
 *     hal_gpio_int_init();
 *     hal_gpio_set_debounce_time(30);
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_STATUS hal_gpio_set_debounce_time(uint8_t ms);

/**
 * hal_gpio.h
 *
 * \brief   Change the interrupt polarity of GPIO current mode.
 *
 * \param[in]  pin_index          The pin index, please refer to rtl876x.h 'Pin_Number' part.
 * \param[in]  polarity           The polarity for the interrupt to be set to, can be high or low active.
 *
 * \return                   The status of change interrupt polarity of the specific GPIO pin.
 * \retval GPIO_STATUS_OK        The GPIO pin changes interrupt polarity successfully.
 * \retval GPIO_STATUS_ERROR_PIN The GPIO pin was failed to change interrupt polarity due to invalid pin number.
 * \retval GPIO_STATUS_ERROR     The GPIO pin was failed to change interrupt polarity due to malloc failure.
 * \retval GPIO_STATUS_INVALID_PARAMETER  The GPIO pin was failed to change interrupt polarity due to invalid input paramter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     if (gpio_level == GPIO_LEVEL_LOW)
 *     {
 *        hal_gpio_irq_change_polarity(pin_index, GPIO_IRQ_ACTIVE_HIGH);
 *     }
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
T_GPIO_STATUS hal_gpio_irq_change_polarity(uint8_t pin_index, T_GPIO_IRQ_POLARITY polarity);


/**
 * hal_gpio.h
 *
 * \brief   Init the HAL GPIO module, GPIO clock would be enabled, and HAL GPIO function could be used after this function called.
 *
 * \param   None.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_init();
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
void hal_gpio_init(void);

/**
 * hal_gpio.h
 *
 * \brief   De-Init the HAL GPIO module, GPIO clock would be disabled, and HAL GPIO function could not be used after this function called.
 *
 * \param   None.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     hal_gpio_deinit();
 * }
 * \endcode
 *
 * \ingroup  HAL_GPIO_Exported_Functions
 */
void hal_gpio_deinit(void);


#ifdef __cplusplus
}
#endif

#endif

/** @} */ /* End of group HAL_GPIO_Exported_Functions */
/** @} */ /* End of group HAL_GPIO_H_ */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/
