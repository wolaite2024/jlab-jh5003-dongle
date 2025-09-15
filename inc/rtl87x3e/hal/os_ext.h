/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     os_ext.h
* @brief    This file provides OS extern API for sdk customer.
* @details
* @author   huan_yang
* @date
* @version  v1.0
*********************************************************************************************************
*/
/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef OS_EXT_H
#define OS_EXT_H
/*============================================================================*
 *                               Header Files
*============================================================================*/

/** @defgroup  HAL_87x3e_OS_EXT    OS Extern
    * @brief OS extern API.
    * @{
    */
/*============================================================================*
 *                              Variables
*============================================================================*/
/** @defgroup HAL_87x3e_OS_EXT_Exported_Variables OS Extern Exported Variables
    * @brief
    * @{
    */
typedef long BaseType_t;
typedef void (*PendedFunction_t)(void *, uint32_t);

/** @} */ /* End of group HAL_87x3e_OS_EXT_Exported_Variables */

/*============================================================================*
 *                              Functions
*============================================================================*/
/** @defgroup HAL_87x3e_OS_EXT_Exported_Functions OS Extern Exported Functions
    * @brief
    * @{
    */
#ifdef __cplusplus
extern "C" {
#endif

/**
    * @brief Create an OS timer to monitor free heap and OS timer.
    * @param timeout_s Timeout time in seconds.
    * @return void
    */
void monitor_memory_and_timer(uint16_t heap_and_timer_monitor_timer_timeout_s);

/**
    * @brief Get OS timer handle with timer id.
    * @param id Timer id.
    * @return void
    */
void *os_timer_handle_get(uint8_t id);

/**
    * @brief Get timer index with timer handle.
    * @param timer Point to OS timer.
    * @return timer id
    */
uint8_t os_timer_index_get(void *timer);

/**
    * @brief Get OS timer state.
    * @param pp_handle Point to timer handle.
    * @param p_timer_state Timer state
    * @return       Timer state.
    * @retval true  The timer is activate.
    * @retval false The timer is inactive.
    */
bool os_timer_state_get(void **pp_handle, uint32_t *p_timer_state);

/**
    * @brief Do context switch or not before the interrupt exits.
    * @param x If x is set to 0 , a context switch should be requested before
    * the interrupt exits, otherwise it should not be requested.
    * @return void
    */
void os_portyield_from_isr(BaseType_t x);

/**
 * @brief Used from application interrupt service routines to defer the execution of a
 *        function to the RTOS daemon task.
 * \details Used from application interrupt service routines to defer the execution of a
 * function to the RTOS daemon task (the timer service task, hence this function
 * is implemented in timers.c and is prefixed with 'Timer').
 *
 * Ideally an interrupt service routine (ISR) is kept as short as possible, but
 * sometimes an ISR either has a lot of processing to do, or needs to perform
 * processing that is not deterministic.  In these cases
 * xTimerPendFunctionCallFromISR() can be used to defer processing of a function
 * to the RTOS daemon task.
 *
 * A mechanism is provided that allows the interrupt to return directly to the
 * task that will subsequently execute the pended callback function.  This
 * allows the callback function to execute contiguously in time with the
 * interrupt - just as if the callback had executed in the interrupt itself.
 *
 * @param xFunctionToPend The function to execute from the timer service/
 * daemon task.  The function must conform to the PendedFunction_t
 * prototype.
 *
 * @param pvParameter1 The value of the callback function's first parameter.
 * The parameter has a void * type to allow it to be used to pass any type.
 * For example, unsigned longs can be cast to a void *, or the void * can be
 * used to point to a structure.
 *
 * @param ulParameter2 The value of the callback function's second parameter.
 *
 * @param pxHigherPriorityTaskWoken As mentioned above, calling this function
 * will result in a message being sent to the timer daemon task.  If the
 * priority of the timer daemon task is higher than the priority of
 * the currently running task (the task the interrupt interrupted) then
 * *pxHigherPriorityTaskWoken will be set to true within
 * xTimerPendFunctionCallFromISR(), indicating that a context switch should be
 * requested before the interrupt exits.  For that reason
 * *pxHigherPriorityTaskWoken must be initialised to false.  See the
 * example code below.
 *
 * @return Whether the message is successfully sent to the timer daemon task.
 *
 * Example usage:
 * @verbatim
 *
 *  // The callback function that will execute in the context of the daemon task.
 *  // Note callback functions must all use this same prototype.
 *  void vProcessInterface( void *pvParameter1, uint32_t ulParameter2 )
 *  {
 *      BaseType_t xInterfaceToService;
 *
 *      // The interface that requires servicing is passed in the second
 *      // parameter.  The first parameter is not used in this case.
 *      xInterfaceToService = ( BaseType_t ) ulParameter2;
 *
 *      // ...Perform the processing here...
 *  }
 *
 *  // An ISR that receives data packets from multiple interfaces
 *  void vAnISR( void )
 *  {
 *      BaseType_t xInterfaceToService, xHigherPriorityTaskWoken;
 *
 *      // Query the hardware to determine which interface needs processing.
 *      xInterfaceToService = prvCheckInterfaces();
 *
 *      // The actual processing is to be deferred to a task.  Request the
 *      // vProcessInterface() callback function is executed, passing in the
 *      // number of the interface that needs processing.  The interface to
 *      // service is passed in the second parameter.  The first parameter is
 *      // not used in this case.
 *      xHigherPriorityTaskWoken = pdFALSE;
 *      xTimerPendFunctionCallFromISR( vProcessInterface, NULL, ( uint32_t ) xInterfaceToService, &xHigherPriorityTaskWoken );
 *
 *      // If xHigherPriorityTaskWoken is now set to pdTRUE then a context
 *      // switch should be requested.  The macro used is port specific and will
 *      // be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
 *      // the documentation page for the port being used.
 *      portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 *
 *  }
 * @endverbatim
 */
BaseType_t xTimerPendFunctionCallFromISR(PendedFunction_t xFunctionToPend, void *pvParameter1,
                                         uint32_t ulParameter2, BaseType_t *pxHigherPriorityTaskWoken);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
/** @} */ /* End of group HAL_87x3e_OS_EXT_Exported_Functions */
/** @} */ /* End of group HAL_87x3e_OS_EXT */
#endif /* __OS_EXT_H_ */

