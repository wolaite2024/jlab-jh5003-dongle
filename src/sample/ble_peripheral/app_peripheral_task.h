/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_task.h
   * @brief     Routines to create App task and handle events & messages
   * @author    danni
   * @date      2017-06-02
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_PERIPHERAL_TASK_H_
#define _APP_PERIPHERAL_TASK_H_


/** @defgroup PERIPH_APP_TASK Peripheral App Task
  * @brief Peripheral App Task
  * @{
  */
/**
 * @brief driver_init
 * Contains the initialization of peripherals.
 * @note Both new architecture driver and legacy driver initialization method can be used
 * @return void
 */
void driver_init(void);

/**
 * @brief app_task_init
 * Initialize App task
 * @return void
 */
void app_task_init(void);

/** End of PERIPH_APP_TASK
* @}
*/

#endif
