/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_DEVICE_H_
#define _APP_DEVICE_H_
#include <stdbool.h>
#include <stdint.h>
#include <rtl876x_wdg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_DEVICE App Device
  * @brief App Device
  * @{
  */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_DEVICE_Exported_Macros App Device Macros
    * @{
    */

/** End of APP_DEVICE_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_DEVICE_Exported_Types App Device Types
    * @{
    */
typedef enum
{
    APP_DEVICE_TIMER_POWER_OFF_RESET,
    APP_DEVICE_TIMER_TOTAL
} T_APP_DEVICE_TIMER;

typedef enum
{
    BUD_COUPLE_STATE_IDLE      = 0x00,
    BUD_COUPLE_STATE_START     = 0x01,
    BUD_COUPLE_STATE_CONNECTED = 0x02
} T_BUD_COUPLE_STATE;

typedef enum
{
    APP_DEVICE_STATE_OFF       = 0x00,
    APP_DEVICE_STATE_ON        = 0x01,
    APP_DEVICE_STATE_OFF_ING   = 0x02,
} T_APP_DEVICE_STATE;

typedef enum
{
    AUTO_POWER_OFF_MASK_POWER_ON = 0x01,
    AUTO_POWER_OFF_MASK_SOURCE_LINK = 0x02,
    AUTO_POWER_OFF_MASK_IN_BOX = 0x04,
    AUTO_POWER_OFF_MASK_BUD_COUPLING = 0x08,
    AUTO_POWER_OFF_MASK_KEY = 0x10,
    AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF = 0x20,
    AUTO_POWER_OFF_MASK_PAIRING_MODE = 0x40,
    AUTO_POWER_OFF_MASK_ANC_APT_MODE = 0x80,
    AUTO_POWER_OFF_MASK_BLE_LINK_EXIST  = 0x00000100,
    AUTO_POWER_OFF_MASK_PLAYBACK_MODE   = 0x00000200,
    AUTO_POWER_OFF_MASK_LINE_IN = 0x00000400,
    AUTO_POWER_OFF_MASK_ANC_APT_MODE_WITH_PHONE_CONNECTED   = 0x00000800,
    AUTO_POWER_OFF_MASK_LINKBACK = 0x00002000,
    AUTO_POWER_OFF_MASK_AIRPLANE_MODE = 0x00001000,
    AUTO_POWER_OFF_MASK_DUT_MODE = 0x00004000,
    AUTO_POWER_OFF_MASK_USB_AUDIO_MODE = 0x00008000,
} T_AUTO_POWER_OFF_MASK;


typedef enum
{
    APP_TONE_VP_STOP      = 0x00,
    APP_TONE_VP_STARTED     = 0x01,
} T_APP_TONE_VP_STATE;

typedef struct
{
    T_APP_TONE_VP_STATE state;
    uint8_t     index;
} T_APP_TONE_VP_STARTED;


typedef struct test_equipment_info
{
    uint8_t oui[3];
    const char *name;
} T_APP_TEST_EQUIPMENT_INFO;

extern void (*app_power_on_hook)(void);
extern void (*app_power_off_hook)(void);

/** End of APP_DEVICE_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_DEVICE_Exported_Functions App Device Functions
    * @{
    */
/* @brief  app device module init.
*
* @param  void
* @return none
*/
void app_device_init(void);

/* @brief save dongle pairing id to NV
*
* @param  id:   pairing id
* @param  len : pairing id len
* @return true if success
*/
bool app_device_save_pairing_id(uint8_t *id, uint8_t len);

/* @brief  app device reboot.
*
* @param  timeout_ms timeout value to do system reboot
* @param  wdg_mode Watch Dog Mode
* @return none
*/
void app_device_reboot(uint32_t timeout_ms, T_WDG_MODE wdg_mode);

void app_device_factory_reset(void);

/* @brief  check device inside/outside box state
*
* @param  void
* @return true/false
*/
bool app_device_is_in_the_box(void);

/* @brief  change device state
*
* @param  state @ref T_APP_DEVICE_STATE
* @return none
*/
void app_device_state_change(T_APP_DEVICE_STATE state);

T_APP_DEVICE_STATE app_device_state_get(void);

/* @brief  handle remote relay msg
*
* @param  msg @ref T_APP_REMOTE_MSG
* @param  buf buffer
* @return true/false
*/
void app_device_handle_remote_msg(uint16_t msg, void *buf);

/* @brief  start up bt policy
*
* @param  at_once_trigger
* @return none
*/
void app_device_bt_policy_startup(bool at_once_trigger);
/** @} */ /* End of group APP_DEVICE_Exported_Functions */
/** End of APP_DEVICE
* @}
*/

/**
 * @brief whether the app is powered on or not
 *
 * @param  void
 * @return true/false
 */
bool app_device_is_power_on(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_DEVICE_H_ */
