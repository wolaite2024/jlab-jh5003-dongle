/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    aon_wdg_ext.h
  * @brief   This file provides api wrapper for bbpro compatibility..
  * @author  huan_yang
  * @date    2023-4-23
  * @version v1.0
  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __AON_WDG_EXT_H_
#define __AON_WDG_EXT_H_


/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "rtl876x.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AON_WDG1_REG   ((BTAON_FAST_RTC_AON_WDT *)AON_WDG_REG_BASE)
#define AON_WDG2_REG   ((BTAON_FAST_RTC_AON_WDT *)(AON_WDG_REG_BASE + 0x0c))

#define AON_WDG1_REG_OFFSET   ((uint32_t)AON_WDG1_REG - RTC_REG_BASE)
#define AON_WDG2_REG_OFFSET   ((uint32_t)AON_WDG2_REG - RTC_REG_BASE)
/*AON_WDG1 is the same as charger aon wdg, and can not be used for the project with using charger*/
#define AON_WDG1 (BTAON_FAST_RTC_AON_WDT *)AON_WDG1_REG_OFFSET
#define AON_WDG2 (BTAON_FAST_RTC_AON_WDT *)AON_WDG2_REG_OFFSET

/**
    * @brief  enable aon wdg
    * \xrefitem Added_API_2_14_0_0 "Added Since 2.14.0.0" "Added API"
    * @param  wdg: @ref BTAON_FAST_RTC_AON_WDT
    * @return void
    */
void aon_wdg_enable(BTAON_FAST_RTC_AON_WDT *wdg);

/**
    * @brief  disable aon wdg
    * \xrefitem Added_API_2_14_0_0 "Added Since 2.14.0.0" "Added API"
    * @param  wdg: @ref BTAON_FAST_RTC_AON_WDT
    * @return none
    */
void aon_wdg_disable(BTAON_FAST_RTC_AON_WDT *wdg);

/**
    * @brief  restart aon wdg
    * \xrefitem Added_API_2_14_0_0 "Added Since 2.14.0.0" "Added API"
    * @param  wdg: @ref BTAON_FAST_RTC_AON_WDT
    * @return none
    */
void aon_wdg_kick(BTAON_FAST_RTC_AON_WDT *wdg);

/**
    * @brief  aon wdg is enabled or not
    * \xrefitem Added_API_2_14_0_0 "Added Since 2.14.0.0" "Added API"
    * @param  wdg: @ref BTAON_FAST_RTC_AON_WDT
    * @return true: enabled; false: diabled
    */
bool aon_wdg_is_enable(BTAON_FAST_RTC_AON_WDT *wdg);

#ifdef __cplusplus
}
#endif

#endif
