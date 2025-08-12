/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_observer_task.h
   * @brief     Routines to create App task and handle events & messages
   * @author    danni
   * @date      2022-06-02
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_OBSERVER_TASK_H_
#define _APP_OBSERVER_TASK_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** @defgroup OB_APP_TASK Observer App Task
  * @brief Observer App Task
  * @{
  */
extern void driver_init(void);

/**
 * @brief  Initialize App task
 * @return void
 */
void app_task_init(void);

/** End of OB_APP_TASK
* @}
*/

#ifdef __cplusplus
}
#endif
#endif

