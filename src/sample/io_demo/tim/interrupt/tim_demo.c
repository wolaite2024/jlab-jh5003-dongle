/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     tim_demo.c
* @brief    tim interrupt demo
* @details
* @author   renee
* @date     2020-05-23
* @version  v0.1
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "trace.h"
#include "hw_tim.h"
#include "section.h"

static T_HW_TIMER_HANDLE demo_timer_handle = NULL;
/** @defgroup  TIM_DEMO_TIM  TIM DEMO
    * @brief  Tim implementation demo code
    * @{
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup TIM_Demo_Exported_Functions TIM Demo Exported Functions
  * @brief
  * @{
  */

/**
* @brief  HW timer test callback function.
* @param   No parameter.
* @return  void
*/
RAM_TEXT_SECTION
static void demo_hw_timer_callback(T_HW_TIMER_HANDLE handle)
{
    //Add User code here
    IO_PRINT_TRACE0("demo_hw_timer_callback");
}

/**
  * @brief  Initialize TIM peripheral.
  * @param   No parameter.
  * @return  void
  */
static void tim_driver_init(void)
{
    demo_timer_handle = hw_timer_create("demo_hw_timer", 3000 * 1000, true, demo_hw_timer_callback);
    if (demo_timer_handle == NULL)
    {
        IO_PRINT_ERROR0("tim_driver_init: fail to create hw timer, check hw timer usage");
        return;
    }

    IO_PRINT_TRACE1("tim_driver_init: create hw timer instance successfully, id %d",
                    hw_timer_get_id(demo_timer_handle));

    hw_timer_start(demo_timer_handle);
}

/**
  * @brief  demo code of operation about tim.
  * @param   No parameter.
  * @return  void
  */
void tim_demo(void)
{
    /* Initialize tim peripheral */
    tim_driver_init();
}


/** @} */ /* End of group TIM_Demo_Exported_Functions */
/** @} */ /* End of group TIM_DEMO_TIM */
