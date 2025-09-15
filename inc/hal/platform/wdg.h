/**
*****************************************************************************************
*     Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    wdg.h
  * @brief   This file provides wdg API wrapper for bbpro compatibility.
  * @author  sandy_jiang
  * @date    2024-10-11
  * @version v1.1
  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

#ifndef __WDG_H_
#define __WDG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/** @defgroup  HAL_WDG_API    WDG API
    * @brief This file introduces the watch dog timer (WDG) APIs.
    * @{
    */

/*============================================================================*
 *                                 Types
 *============================================================================*/

/** @defgroup HAL_WDG_API_Exported_Types WDG API Exported Types
  * @{
  */

typedef enum _WDG_MODE
{
    INTERRUPT_CPU = 0,          /**< Interrupt CPU only. */
    RESET_ALL_EXCEPT_AON = 1,   /**< Reset all except RTC and some AON register. */
    RESET_CORE_DOMAIN = 2,      /**< Reset core domain. */
    RESET_ALL = 3               /**< Reset all. */
} T_WDG_MODE;


/** End of group HAL_WDG_API_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup HAL_WDG_API_Exported_Functions WDG API Exported Functions
 * @{
 */

/**
    * @brief  Get watch dog default mode.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  None.
    * @return Watch dog mode @ref T_WDG_MODE.
    */
T_WDG_MODE wdg_get_default_mode(void);

/**
    * @brief  Get watch dog current mode.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  None.
    * @return Watch dog mode @ref T_WDG_MODE.
    */
T_WDG_MODE wdg_get_mode(void);

/**
    * @brief  Get watch dog current timeout value.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  None.
    * @return The current timeout value, unit ms.
    */
uint32_t wdg_get_timeout_ms(void);

/**
    * @brief  Get watch dog default timeout value.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  None.
    * @return The default timeout value, unit ms.
    */
uint32_t wdg_get_default_timeout_ms(void);

/**
    * @brief  Change watch dog timeout period.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  timeout_ms: New timeout value.
    * @return None.
    */
void wdg_change_timeout_period(uint32_t timeout_ms);

/**
    * @brief  Change watch dog mode.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  mode: Watch dog mode @ref T_WDG_MODE.
    * @return None.
    */
void wdg_change_mode(T_WDG_MODE mode);

/**
    * @brief  Check if watch dog is enabled.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  None.
    * @retval true: Watch dog is enabled.
    * @retval false: Watch dog is disabled.
    */
bool wdg_is_enable(void);

/**
    * @brief  Set watch dog timeout period and mode.
    * @param  ms: New timeout value.
    * @param  wdg_mode: Watch dog mode @ref T_WDG_MODE.
    * @retval true: Set the period and mode success.
    * @retval false: Set the period and mode fail.
    */
extern bool (*wdg_start)(uint32_t ms, T_WDG_MODE  wdg_mode);
#define WDG_Start(ms, wdg_mode) wdg_start(ms, wdg_mode);

/**
    * @brief  Disable watch dog.
    * @param  None.
    * @return None.
    */
extern void (*wdg_disable)(void);
#define WDG_Disable() wdg_disable();

/**
    * @brief  Kick watch dog.
    * @param  None.
    * @return None.
    */
extern void (*wdg_kick)(void);
#define WDG_Kick() wdg_kick();

/**
    * @brief  Reset the MCU at specified mode.
    * @param  wdg_mode: Watch dog mode @ref T_WDG_MODE.
    * @return None.
    */
extern void wdt_reset(T_WDG_MODE wdg_mode, const char *function, uint32_t line);
#define chip_reset(mode)    wdt_reset(mode,  __FUNCTION__, __LINE__)

/**
    * @brief  Reset the MCU at specified mode with reset reason.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  wdg_mode: Watch dog mode @ref T_WDG_MODE.
    * @param  reset_reason: Reset reason.
    * @return None.
    */
void wdg_reset_with_reason(T_WDG_MODE wdg_mode, uint8_t reset_reason);

/**
    * @brief  Get watch dog reset reason.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  None.
    * @return The watch dog reset reason.
    */
uint8_t wdg_get_reset_reason(void);

#ifdef __cplusplus
}
#endif
/** @} */ /* End of group HAL_WDG_API_Exported_Functions */
/** @} */ /* End of group HAL_WDG_API */
#endif
