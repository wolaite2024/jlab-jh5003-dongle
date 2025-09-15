/**
*****************************************************************************************
*     Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    reset_reason.h
  * @brief   This file provides wdg reset reason.
  * @author  colin
  * @date    2024-10-11
  * @version v1.1
  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2023 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

/** @defgroup  WDG_RESET_REASON    WDG Reset Reason
    * @brief This file introduces the watch dog reset reason.
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

/** @defgroup WDG_RESET_REASON_Exported_Constants WDG Reset Reason Exported Constants
  * @{
  */

#define RESET_REASON_HW             0x0
#define RESET_REASON_MAX            0x3F

/** End of group WDG_RESET_REASON_Exported_Constants
  * @}
  */

#ifdef __cplusplus
}
#endif
/** @} */ /* End of group WDG_RESET_REASON */
#endif
