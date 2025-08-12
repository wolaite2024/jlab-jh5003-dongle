/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_broadcaster_task.h
   * @brief     Routines to create App task and handle events & messages
   * @author    danni
   * @date      2012-06-02
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_BROADCASTER_TASK_H_
#define _APP_BROADCASTER_TASK_H_

/** @defgroup BROADCASTER_APP_TASK Broadcaster App Task
  * @brief Broadcaster App Task
  * @{
  */

extern void driver_init(void);

/**
 * @brief  Initialize App task
 * @return void
 */
void app_task_init(void);

/** End of BROADCASTER_APP_TASK
* @}
*/
#endif
