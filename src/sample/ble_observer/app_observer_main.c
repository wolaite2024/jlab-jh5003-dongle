/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      main.c
   * @brief     Source file for BLE observer project, mainly used for initialize modules
   * @author    danni
   * @date      2022-06-12
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "pm.h"
#include "os_sched.h"
#include "trace.h"
#include "gap_le.h"
#include "app_observer_gap.h"
#include "app_observer_task.h"

/** @defgroup  OB_DEMO_MAIN Observer Main
    * @brief Main file to initialize hardware and BT stack and start task scheduling
    * @{
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief board_init
          Contains the initialization of pinmux settings and pad settings
 * @note  All the pinmux settings and pad settings shall be initiated in this function,
 *        but if legacy driver is used, the initialization of pinmux setting and pad setting
 *        should be peformed with the IO initializing.
 * @return  void
 */
void board_init(void)
{

}

/**
 * @brief driver_init
          Contains the initialization of peripherals
 * @note  Both new architecture driver and legacy driver initialization method can be used
 * @return   void
 */
void driver_init(void)
{

}

/**
 * @brief pwr_mgr_init
          Contains the power mode settings
 * @return void
 */
void pwr_mgr_init(void)
{
#if F_DLPS_EN
    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    power_mode_set(POWER_DLPS_MODE);
#endif
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Observer APP, thus only one APP task is init here
 * @return   void
 */
void task_init(void)
{
    app_task_init();
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int main(void)
{
    board_init();
    le_gap_init(0);
    gap_lib_init();
    app_gap_init();
    pwr_mgr_init();
    task_init();
    os_sched_start();

    return 0;
}
/** @} */ /* End of group OB_DEMO_MAIN */


