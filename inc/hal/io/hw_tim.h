/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      hw_tim.h
* @brief
* @details
* @author
* @date
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef _HW_TIM_
#define _HW_TIM_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/** @addtogroup HW_TIM HW TIM
  * @brief HW TIM driver module.
  * @{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup HW_TIM_Exported_Constants HW TIM Exported Constants
  * @{
  */

/**
  * @brief HW Timer ID enumeration.
  */
typedef enum
{
    HW_TIMER_ID_EMPTY,    //!< Timer ID is 0.
    HW_TIMER_ID_1,        //!< Timer ID is 1.
    HW_TIMER_ID_2,        //!< Timer ID is 2.
    HW_TIMER_ID_3,        //!< Timer ID is 3.
    HW_TIMER_ID_4,        //!< Timer ID is 4.
    HW_TIMER_ID_5,        //!< Timer ID is 5.
    HW_TIMER_ID_6,        //!< Timer ID is 6.
    HW_TIMER_ID_7,        //!< Timer ID is 7.
    HW_TIMER_ID_MAX,      //!< The total number of timer.
} T_HW_TIMER_ID;

/** End of group HW_TIM_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup HW_TIM_Exported_Types HW TIM Exported Types
  * @{
  */

/**
  * @brief  Timer handle definition.
  */
typedef void *T_HW_TIMER_HANDLE;

/**
  * @brief  Timer callback definition.
  */
typedef void (*P_HW_TIMER_CBACK)(T_HW_TIMER_HANDLE handle);

/** End of group HW_TIM_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup HW_TIM_Exported_Functions HW TIM Exported Functions
  * @{
  */

/**
  * @brief  Create the timer according to the specified parameters.
  * @param  name: Specifies the timer name.
  * @param  period_us: Specifies the period value to be loaded into
  *         the active auto-reload register at the next update event.
  *         The unit of the period value is microseconds.
  * @param  reload: Used to set the timer as a periodic or one-shot timer.
  *         - true: Create a periodic timer.
  *         - false: Create a one-shot timer.
  * @param  cback: Pointer to a P_HW_TIMER_CBACK function that contains
  *         the contents executed in the timer interrupt. If cback is
  *         NULL, disable timer interrupt.
  * @return A handle by which the created timer can be referenced.
  */
T_HW_TIMER_HANDLE hw_timer_create(const char *name, uint32_t period_us, bool reload,
                                  P_HW_TIMER_CBACK cback);

/**
  * @brief  Create the timer(support DMA handshake) according to the specified parameters.
  * @param  name: Specifies the timer name.
  * @param  period_us: Specifies the period value to be loaded into
  *         the active auto-reload register at the next update event.
  *         The unit of the period value is microseconds.
  * @param  reload: Used to set the timer as a periodic or one-shot timer.
  *         - true: Create a periodic timer.
  *         - false: Create a one-shot timer.
  * @param  cback: Pointer to a P_HW_TIMER_CBACK function that contains
  *         the contents executed in the timer interrupt. If cback is
  *         NULL, disable timer interrupt.
  * @return A handle by which the created timer can be referenced.
  */
T_HW_TIMER_HANDLE hw_timer_create_dma_mode(const char *name, uint32_t period_us, bool reload,
                                           P_HW_TIMER_CBACK cback);

/**
  * @brief  Get the DMA handshake of the specified timer.
  * @param  handle: The handle of the timer to be got DMA handshake.
  * @return Timer DMA handshake.
  */
uint8_t hw_timer_get_dma_handshake(T_HW_TIMER_HANDLE handle);

/**
  * @brief  Get the name of the specified timer.
  * @param  handle: The handle of the timer to be got name.
  * @return Timer name.
  */
const char *hw_timer_get_name(T_HW_TIMER_HANDLE handle);

/**
  * @brief  Get the id of the specified timer.
  * @param  handle: The handle of the timer to be got id.
  * @return Timer id.
  */
T_HW_TIMER_ID hw_timer_get_id(T_HW_TIMER_HANDLE handle);

/**
  * @brief  Delete the specified timer.
  * @param  handle: The handle of the timer to be deleted.
  * @return The status of the timer deletion.
  * @retval true      Timer was deleted successfully.
  * @retval false     Timer was failed to delete.
  */
bool hw_timer_delete(T_HW_TIMER_HANDLE handle);

/**
  * @brief  Start the specified timer.
  * @param  handle: The handle of the timer to be started.
  * @return The status of the timer start.
  * @retval true      Timer was started successfully.
  * @retval false     Timer was failed to start.
  */
bool hw_timer_start(T_HW_TIMER_HANDLE handle);

/**
  * @brief  Stop the specified timer.
  * @param  handle: The handle of the timer to be stopped.
  * @return The status of the timer stop.
  * @retval true      Timer was stopped successfully.
  * @retval false     Timer was failed to stop.
  */
bool hw_timer_stop(T_HW_TIMER_HANDLE handle);

/**
  * @brief  Restart the specified timer.
  * @param  handle: The handle of the timer to be restarted.
  * @param  period_us: The period value of the timer after restart.
  *         The unit of period value is microseconds.
  * @return The status of the timer restart.
  * @retval true      Timer was restarted successfully.
  * @retval false     Timer was failed to restart.
  */
bool hw_timer_restart(T_HW_TIMER_HANDLE handle, uint32_t period_us);

/**
  * @brief  Get current value when the specified timer is running.
  * @param  handle: The handle of the timer to be got counter value.
  * @param  count: The counter value.
  * @return The result of the timer counter value check.
  * @retval true      Timer current value was got successfully.
  * @retval false     Timer current value was failed to get.
  */
bool hw_timer_get_current_count(T_HW_TIMER_HANDLE handle, uint32_t *count);

/**
  * @brief  Get elapsed value when the specified timer is running.
  * @param  handle: The handle of the timer to be got elapsed value.
  * @param  time: The elapsed counter value.
  * @return The result of the timer elapsed value check.
  * @retval true      Timer elapsed value was got successfully.
  * @retval false     Timer elapsed value was failed to get.
  */
bool hw_timer_get_elapsed_time(T_HW_TIMER_HANDLE handle, uint32_t *time);

/**
  * @brief  Get period value of the specified timer.
  * @param  handle: The handle of the timer to be got period value.
  * @param  period_us: The period value.
  * @return The result of the timer period value check.
  * @retval true      Timer period value was got successfully.
  * @retval false     Timer period value was failed to get.
  */
bool hw_timer_get_period(T_HW_TIMER_HANDLE handle, uint32_t *period_us);

/**
  * @brief  Checks whether the specified timer has been enabled or disabled.
  * @param  handle: The handle of the timer to be checked.
  * @param  is_active: The state of the specified timer(ENABLE or DISABLE).
  * @return The result of the timer state check.
  * @retval true      Timer state was checked successfully.
  * @retval false     Timer state was failed to check.
  */
bool hw_timer_is_active(T_HW_TIMER_HANDLE handle, bool *is_active);

/**
  * @brief  Get free timer number.
  * @return The free timer number.
  */
uint8_t hw_timer_get_free_timer_number(void);

/**
  * @brief  Print all used timer name.
  */
void hw_timer_print_all_users(void);

/**
  * @brief  Configure hw timer low power mode support.
  * @param  handle: The handle of the timer to be checked.
  * @param  enable: Choose to support hw timer low power mode or not.
  *         - true: Allow hw timer to enter low power mode when active. The accuracy could be affected.
  *         - false: It will block low power mode when hw timer handle active.
  * @return The result of the timer state check.
  * @retval true      hw timer low power mode support set successfully.
  * @retval false     hw timer low power mode support was failed to set.
  */
bool hw_timer_lpm_set(T_HW_TIMER_HANDLE handle, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* _HW_TIM_ */

/** @} */ /* End of group HW_TIM_Exported_Functions */
/** @} */ /* End of group HW_TIM */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

