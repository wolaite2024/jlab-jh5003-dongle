/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      pwm.h
* @brief
* @details
* @author
* @date
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef _PWM_H_
#define _PWM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/** @addtogroup PWM PWM
  * @brief PWM driver module.
  * @{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup PWM_Exported_Constants PWM Exported Constants
  * @{
  */

/**
  * @brief PWM clock source enumeration.
  */
typedef enum _T_PWM_CLOCK_SOURCE
{
    PWM_CLOCK_1M,       //!< PWM clock source is 1MHz.
    PWM_CLOCK_MAX,      //!< The maximum number of selectable clock sources for PWM.
} T_PWM_CLOCK_SOURCE;

/**
  * @brief PWM deadzone stop state enumeration.
  */
typedef enum _T_PWM_STOP_STATE
{
    PWM_DEAD_ZONE_STOP_LOW,     //!< PWM DeadZone stop at low level.
    PWM_DEAD_ZONE_STOP_HIGH     //!< PWM DeadZone stop at high level.
} T_PWM_STOP_STATE;

/**
  * @brief PWM pin function enumeration.
  */
typedef enum
{
    PWM_FUNC_DISABLE,     //!< Disable PWM function of pin.
    PWM_FUNC,             //!< Configure pin for PWM output function.
    PWM_FUNC_P,           //!< Configure pin for PWM_P output function.
    PWM_FUNC_N,           //!< Configure pin for PWM_N output function.
} T_PWM_PIN_FUNC;

/** End of group PWM_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup PWM_Exported_Types PWM Exported Types
  * @{
  */

/**
  * @brief  PWM handle definition.
  */
typedef void *T_PWM_HANDLE;

/**
  * @brief  PWM configure structure definition.
  */
typedef struct _T_HW_TIMER_PWM
{
    uint32_t pwm_deadzone_size;             /*!< Specify the PWM deadzone size.
                                                 This parameter must range from 0x1 to 0xff.
                                                 The calculation formula for deadzone time is as follows:
                                                 deadzone time = (pwm_deadzone_size) / 32000. */

    uint32_t pwm_high_count;                /*!< Specifies the PWM high count. */

    uint32_t pwm_low_count;                 /*!< Specifies the PWM low count. */

    bool pwm_deadzone_enable;               /*!< Enable or disable the PWM complementary output and deadzone.
                                                This parameter can be a value of ENABLE or DISABLE. */

    T_PWM_STOP_STATE pwm_p_stop_state;      /*!< Specifies the PWM P stop state.
                                                This parameter can be a value of @ref T_PWM_STOP_STATE. */

    T_PWM_STOP_STATE pwm_n_stop_state;      /*!< Specifies the PWM N stop state.
                                                This parameter can be a value of @ref T_PWM_STOP_STATE. */

    T_PWM_CLOCK_SOURCE clock_source;        /*!< Specifies the PWM clock source.
                                                This parameter can be a value of @ref T_PWM_CLOCK_SOURCE. */
} T_PWM_CONFIG;

/** End of group PWM_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup PWM_Exported_Functions PWM Exported Functions
  * @{
  */

/**
  * @brief  Create the PWM according to the specified parameters.
  * @param  name: Specifies the PWM name.
  * @param  high_period_us: Specifies the PWM high count.
  * @param  low_period_us: Specifies the PWM low count.
  * @param  deadzone_enable: Specifies whether PWM deadzone is enabled.
  *         - true: PWM deadzone is enabled.
  *         - false: PWM deadzone is disabled.
  * @return A handle by which the created PWM can be referenced.
  */
T_PWM_HANDLE pwm_create(const char *name, uint32_t high_period_us,
                        uint32_t low_period_us, bool deadzone_enable);

/**
  * @brief  Delete the specified PWM.
  * @param  handle: The handle of the PWM to be deleted.
  * @return The status of the PWM deletion.
  * @retval true     PWM was deleted successfully.
  * @retval false     PWM was failed to delete.
  */
bool pwm_delete(T_PWM_HANDLE handle);

/**
  * @brief  Start the specified PWM.
  * @param  handle: The handle of the PWM to be started.
  * @return The status of the PWM start.
  * @retval true     PWM was started successfully.
  * @retval false     PWM was failed to start.
  */
bool pwm_start(T_PWM_HANDLE handle);

/**
  * @brief  Stop the specified PWM.
  * @param  handle: The handle of the PWM to be stopped.
  * @return The status of the PWM stop.
  * @retval true     PWM was stopped successfully.
  * @retval false     PWM was failed to stop.
  */
bool pwm_stop(T_PWM_HANDLE handle);

/**
  * @brief  Create the PWM timeout callback function.
  * @param  handle: The handle of the PWM to be created timeout callback function.
  * @param  p_timer_callback: Pointer to a function that contains
  *         the contents executed in the timer interrupt.
  * @return The status of the PWM timeout callback creation.
  * @retval true     PWM timeout callback was created successfully.
  * @retval false     PWM timeout callback was failed to create.
  */
bool pwm_register_timeout_callback(T_PWM_HANDLE handle,
                                   void (*p_timer_callback)(T_PWM_HANDLE));

/**
  * @brief  Checks whether the specified PWM has been enabled or disabled.
  * @param  handle: The handle of the PWM to be checked.
  * @param  state: The state of the specified timer(ENABLE or DISABLE).
  * @return The result of the PWM state check.
  * @retval true     PWM state was checked successfully.
  * @retval false     PWM state was failed to check.
  */
bool pwm_is_active(T_PWM_HANDLE handle, bool *state);

/**
  * @brief  Configure the specified PWM according to the specified
  *         parameters in the @ref T_PWM_CONFIG structure.
  * @param  handle: The handle of the PWM to be configured.
  * @param  config_struct: Pointer to a @ref T_PWM_CONFIG structure
  *         that contains the configuration information for the
  *         specified PWM.
  * @return The status of the PWM configuration.
  * @retval true     PWM was configured successfully.
  * @retval false     PWM was failed to configure.
  */
bool pwm_config(T_PWM_HANDLE handle, T_PWM_CONFIG *config_struct);

/**
  * @brief  Configure PWM output pin.
  * @param  handle: The handle of the PWM to be configured.
  * @param  pin_index: Pin number.
  * @param  func: Pin function, can be a value of @ref T_PWM_PIN_FUNC.
  * @return The status of the PWM pin configuration.
  * @retval true     PWM pin was configured successfully.
  * @retval false     PWM pin was failed to configure.
  */
bool pwm_pin_config(T_PWM_HANDLE handle, uint8_t pin_index, T_PWM_PIN_FUNC func);

/**
  * @brief  Change the PWM frequency and duty cycle according high_count and low_count.
  * @param  handle: The handle of the PWM to be configured.
  * @param  high_count: Specifies the PWM high count.
  * @param  low_count: Specifies the PWM low count.
  * @return The status of the PWM configuration.
  * @retval true      PWM was configured successfully.
  * @retval false     PWM was failed to configure.
  */
bool pwm_change_duty_and_frequency(T_PWM_HANDLE handle, uint32_t high_count,
                                   uint32_t low_count);

#ifdef __cplusplus
}
#endif

#endif /* _PWM_H_ */

/** @} */ /* End of group PWM_Exported_Functions */
/** @} */ /* End of group PWM */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

