/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_DLPS_H_
#define _APP_DLPS_H_

#include <stdint.h>
#include <stdbool.h>
#include "rtl876x_pinmux.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_DLPS App Dlps
  * @brief this file handle device dlps mode related process
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_DLPS_Exported_Macros App Dlps Macros
   * @{
   */
#define APP_DLPS_ENTER_CHECK_UART_RX            0x00000001
#define APP_DLPS_ENTER_CHECK_UART_TX            0x00000002
#define APP_DLPS_ENTER_CHECK_GPIO               0x00000004
#define APP_DLPS_ENTER_CHECK_APP                0x00000008
#define APP_DLPS_ENTER_CHECK_MFB_KEY            0x00000010
#define APP_DLPS_ENTER_CHECK_CHARGER            0x00000020
#define APP_DLPS_ENTER_CHECK_RSV                0x00000040
#define APP_DLPS_ENTER_CHECK_LED                0x00000080
#define APP_DLPS_ENTER_CHECK_ADAPTOR            0x00000100
#define APP_DLPS_ENTER_CHECK_MPTEST             0x00000200
#define APP_DLPS_ENTER_CHECK_RSV3               0x00000400
#define APP_DLPS_ENTER_CHECK_PLAYBACK           0x00000800
#define APP_DLPS_ENTER_CHECK_USB                0x00001000
#define APP_DLPS_ENTER_CHECK_LINEIN             0x00002000
#define APP_DLPS_ENTER_CHECK_ADP_VOLTAGE        0x00004000
#define APP_DLPS_ENTER_CHECK_WAIT_RESET         0x00008000
#define APP_DLPS_ENTER_CHECK_OTA_TOOLING_PARK   0x00010000

/** End of APP_DLPS_Exported_Macros
    * @}
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_DLPS_Exported_Functions App Dlps Functions
    * @{
    */
/* @brief  enable dlps by clear specific bit mask
*
* @param  bit one bit that show whether dlps is allowed or not by specific function uint
* @return none
*/
void app_dlps_enable(uint32_t bit);

/* @brief  disable dlps by set specific bit mask
*
* @param  bit one bit that show whether dlps is allowed or not by specific function uint
* @return none
*/
void app_dlps_disable(uint32_t bit);

/**
    * @brief  When app is about to enter dlps mode, you need to check via this callback first.
    * @param  void
    * @return bool
    */
bool app_dlps_check_callback(void);

/**
    * @brief   Need to handle message in this callback function,when App enter dlps mode
    * @param  void
    * @return void
    */
void app_dlps_enter_callback(void);

/**
    * @brief  Need to handle message in this callback function,when App exit dlps mode.
    * @param  void
    * @return void
    */
void app_dlps_exit_callback(void);

/**
    * @brief  dlps related process when power off.
    * @param  void
    * @return void
    */
void app_dlps_power_off(void);

/**
    * @brief  stop power down timer and enable already poweroff mask.
    * @param  void
    * @return void
    */
void app_dlps_enable_auto_poweroff_stop_wdg_timer(void);

/**
    * @brief  stop power down timer.
    * @param  void
    * @return void
    */
void app_dlps_stop_power_down_wdg_timer(void);

/**
    * @brief  start power down timer.
    * @param  void
    * @return void
    */
void app_dlps_start_power_down_wdg_timer(void);

/**
    * @brief  check whether do direct power on or not when wakeup from power down mode.
    * @param  void
    * @return bool
    */
bool app_dlps_check_short_press_power_on(void);

void app_dlps_restore_pad(uint8_t pinmux);
void app_dlps_set_pad_wake_up(uint8_t pinmux,
                              PAD_WAKEUP_POL_VAL wake_up_val);
/**
    * @brief  dlps module init.
    * @param  void
    * @return void
    */
void app_dlps_init(void);

#if F_APP_BB2_EXTERNAL_PA_SUPPORT
bool app_dlps_is_support_external_pa(void);
#endif

/** @} */ /* End of group APP_DLPS_Exported_Functions */
/** End of APP_DLPS
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_DLPS_H_ */
