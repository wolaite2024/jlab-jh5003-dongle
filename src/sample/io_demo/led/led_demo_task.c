/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     led_demo_task.c
* @brief    This file provides demo code of uart loop back rx data to tx by interrupt.
* @details
* @author   renee
* @date     2022-06-25
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>

#include "os_task.h"
#include "os_msg.h"
#include "trace.h"
#include "sw_led_demo.h"
#include "app_timer.h"

/** @defgroup  LED_TASK_DEMO LED Task Demo
    * @brief   implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup  Exported Macros
  * @brief
  * @{
  */
#define     MAX_NUMBER_OF_APP_TIMER_MODULE          0x30    //!< indicate app timer module size
#define     EVENT_APP_TIMER_MSG                                 0x30



/** @} */ /* End of group  */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup   Exported Variables
  * @brief
  * @{
  */
static void *io_demo_app_task_handle;
static void *io_evt_queue_handle;

/** @} */ /* End of group  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup   Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  IO_Demo Task Handle.
  * @param   No parameter.
  * @return  void
  */
static void io_demo_task(void *param)
{
    uint8_t event;

    IO_PRINT_INFO0("io_demo_task: running");
    led_breath_demo();

    while (true)
    {
        if (os_msg_recv(io_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            IO_PRINT_INFO1("io_demo_task: event 0x%x", event);
            if (event == EVENT_APP_TIMER_MSG)
            {
                app_timer_handle_msg(event);
            }
        }
    }
}

/**
  * @brief  task demo code interface , call by main.c.
  * @param   void
  * @return  void
  */
void led_demo_task(void)
{
    os_msg_queue_create(&io_evt_queue_handle, "evtQ", MAX_NUMBER_OF_APP_TIMER_MODULE,
                        sizeof(unsigned char));
    app_init_timer(io_evt_queue_handle, MAX_NUMBER_OF_APP_TIMER_MODULE);
    os_task_create(&io_demo_app_task_handle, "app", io_demo_task, NULL, 384 * 4, 2);
}




/** @} */ /* End of group  */
/** @} */ /* End of group LED_TASK_DEMO */
