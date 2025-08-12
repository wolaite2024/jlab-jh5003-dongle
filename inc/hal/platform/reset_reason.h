/**
*****************************************************************************************
*     Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    reset_reason.h
  * @brief   This file provides wdg reset reason..
  * @author  colin
  * @date    2023-11-22
  * @version v1.0
  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2023 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

/** @defgroup  WDG_RESET_REASON    WDG reset reason
    * @brief This file introduces the watchdog reset reason
    * @{
    */

/*============================================================================*
 *               Constants
 *============================================================================*/
#ifndef __WDG_RESET_REASON_
#define __WDG_RESET_REASON_

#ifdef __cplusplus
extern "C" {
#endif

#define RESET_REASON_HW             0x0
#define RESET_REASON_MAX            0x3F


#ifdef __cplusplus
}
#endif
/** @} */ /* End of group WDG_RESET_REASON */
#endif
