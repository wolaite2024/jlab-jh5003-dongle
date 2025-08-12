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
 *                      Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __APP_EXT_MIC_SWITCH__
#define __APP_EXT_MIC_SWITCH__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/

#include "platform_utils.h"

/*============================================================================*
 *                              Define
 *============================================================================*/

typedef enum
{
    APP_EXT_MIC_IC_BOOM_MIC = 0,
    APP_EXT_MIC_IC_INTERNAL_MIC,
} T_APP_EXT_MIC_IC_MIC_SEL;

typedef enum
{
    APP_EXT_MIC_IC_UNMUTE = 0,
    APP_EXT_MIC_IC_MUTE,
} T_APP_EXT_MIC_IC_MUTE;

/*============================================================================*
 *                              Functions
 *============================================================================*/

void app_ext_mic_detect_power_on_check(void);
void app_ext_mic_gpio_detect_init(void);
void app_ext_mic_gpio_board_init(void);
void app_ext_mic_gpio_driver_init(void);

void app_ext_mic_gpio_detect_enter_dlps_pad_set(void);
void app_ext_mic_gpio_detect_exit_dlps_pad_set(void);
void app_ext_mic_gpio_detect_handle_msg(T_IO_MSG *io_driver_msg_recv);
void app_ext_mic_switch_ic_mute(bool mute);
void app_ext_mic_switch_ic_mic_sel(bool internal_mic);

#ifdef __cplusplus
}
#endif

#endif
