/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CHARGER_H_
#define _APP_CHARGER_H_

#include <stdint.h>
#include <stdbool.h>
#include "charger_api.h"
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_CHARGER App Charger
  * @brief App Charger
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_CHARGER_Exported_Macros App charger Macros
   * @{
   */

#define CHARGERBOX_ADAPTOR_DETECT_TIMER          500
#define SMART_CHARGERBOX_ADAPTOR_DETECT_TIMER    200

#define POWER_OFF_ENTER_DUT_MODE_INTERVAL 500//ms

/* Battery Status */
#define BATTERY_STATUS_EMPTY    0
#define BATTERY_STATUS_LOW      1
#define BATTERY_STATUS_NORMAL   2
#define BATTERY_STATUS_FULL     3

#define MULTIPLE_OF_TEN(x) ((x + 9) / 10 * 10)

#define LOW_BAT_WARNING_TO_MULTIPLIER_MINUTE   60
#define LOW_BAT_WARNING_TO_MULTIPLIER_SECOND   1

/** End of APP_CHARGER_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_CHARGER_Exported_Types App Charger Types
   * @{
   */
/**  @brief  Charger battery capacity,  percentage form */
typedef enum t_bat_capacity
{
    BAT_CAPACITY_0 = 0,
    BAT_CAPACITY_10 = 10,
    BAT_CAPACITY_20 = 20,
    BAT_CAPACITY_30 = 30,
    BAT_CAPACITY_40 = 40,
    BAT_CAPACITY_50 = 50,
    BAT_CAPACITY_60 = 60,
    BAT_CAPACITY_70 = 70,
    BAT_CAPACITY_80 = 80,
    BAT_CAPACITY_90 = 90,
    BAT_CAPACITY_100 = 100,

    BAT_CAPACITY_TOTAL
} T_BAT_CAPACITY;

typedef enum
{
    ADAPTOR_UNPLUG,
    ADAPTOR_PLUG
} T_ADAPTOR_STATUS;


/**  @brief  Charger control type. */
typedef enum
{
    CHARGER_CONTROL_BY_BOX_BATTERY,
    CHARGER_CONTROL_BY_ADP_VOLTAGE, /* charger auto enable must be false */
} T_CHARGER_CONTROL_TYPE;

typedef enum
{
    APP_CHARGER_STATE_CHARGING,
    APP_CHARGER_STATE_CHARGE_FINISH,
    APP_CHARGER_STATE_ERROR,
    APP_CHARGER_STATE_NO_CHARGE,
} T_APP_CHARGER_STATE;

typedef enum
{
    APP_CHARGER_EVENT_BATT_CHANGE           = 0x00,
    APP_CHARGER_EVENT_BATT_STATE_CHANGE     = 0x01,
    APP_CHARGER_EVENT_BATT_ROLESWAP_FLAG    = 0x02,
    APP_CHARGER_EVENT_BATT_CASE_LEVEL       = 0x03,
    APP_CHARGER_EVENT_BATT_REQ_LEVEL        = 0x04,

    APP_CHARGER_MSG_TOTAL                   = 0x05,
} T_APP_CHARGER_REMOTE_MSG;

/** End of APP_CHARGER_Exported_Types
    * @}
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_CHARGER_Exported_Functions App Charger Functions
    * @{
    */
/**
   * @brief  App charger init. App will register charger state callback and state of charge callback.
   *         And init charger three led type.
   *         Charger state include charger start, fast charge, charger finish etc.
   *         Charging status display charger battery current capacity, such as 10%, 50% etc.
   * @param  void
   * @return void
   */
void app_charger_init(void);
/**
    * @brief  discharger module init.
    * @param  void
    * @return void
    */
void app_discharger_init(void);

/**
    * @brief  All the state of charge change message will be handled in this function.
    *         App will do some action according to charger battery capacity.
    * @param  state_of_charge Charger battery state, such as %10, 50% etc
    * @param  first_power_on The first time power-on
    * @return void
    */
void app_handle_state_of_charge(uint8_t state_of_charge, uint8_t first_power_on);

/**
    * @brief  check whether local battery level is low or not.
    * @param  void
    * @return true if is low battery level, otherwise return false.
    */
bool app_charger_local_battery_status_is_low(void);

/**
    * @brief  check whether remote battery level is low or not.
    * @param  void
    * @return true if is low battery level, otherwise return false.
    */
bool app_charger_remote_battery_status_is_low(void);

/**
    * @brief  check whether adaptor is pluged or not.
    * @param  void
    * @return @ref T_ADAPTOR_STATUS.
    */
uint8_t get_adaptor_plug_state(void);

/**
    * @brief  adaptor plug msg process
    * @param  void
    * @return none
    */
void app_charger_plug_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  adaptor unplug msg process
    * @param  void
    * @return none
    */
void app_charger_unplug_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  handle charger state and battery level msg
    * @param  void
    * @return none
    */
void app_charger_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  check adaptor plug/unplug state
    * @param  void
    * @return none
    */
void audio_app_adp_detect(void);

/**
    * @brief  check charger plug debounce timer.
    *
    * @param  void
    * @return timer started if true
    */
bool app_charger_plug_debounce_timer_started(void);

/**
 * @brief update charger state and handle the state
 *
 * @param void
 * @return none
 */
void app_charger_update(void);


/** @} */ /* End of group APP_CHARGER_Exported_Functions */
/** End of APP_CHARGER
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_CHARGER_H_ */
