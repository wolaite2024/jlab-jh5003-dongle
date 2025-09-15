/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file
   * @brief
   * @details
   * @author
   * @date
   * @version
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/

#include "os_sched.h"
#include "section.h"
#include "trace.h"
#include "wdg.h"
#include "rtl876x.h"

/** @defgroup  IO_DEMO_MAIN  IO DEMO MAIN
    * @brief
    * @{
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup IO_DEMO_MAIN_Exported_Functions IO_DEMO MAIN Exported Functions
   * @{
   */

/**
    * @brief main is the entry of user code.
    * @param void
    * @return void
    */
RAM_TEXT_SECTION
int main(void)
{
    __enable_irq();
    WDG_Disable();

    IO_PRINT_INFO0("Hello io demo !");

    extern void gpio_int_demo(void);
    gpio_int_demo();

    os_sched_start();
}


/** End of IO_DEMO_MAIN_Exported_Functions
    * @}
    */


/** End of IO_DEMO_MAIN
    * @}
    */



