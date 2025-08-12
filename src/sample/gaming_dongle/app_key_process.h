/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_KEY_PROCESS_H_
#define _APP_KEY_PROCESS_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_KEY_PROCESS App Key Process
  * @brief App Key Process
  * @{
  */

/**
    * @brief  key process initial.
    * @param  void
    * @return void
    */
void app_key_init(void);

/**
    * @brief  judge if slide switch on.
    * @param  void
    * @return switch on or off
    */
bool app_key_is_slide_switch_on(void);

/**
    * @brief  judge if now going to pairing mode by long press key.
    * @param  void
    * @return is pairing mode or other mode
    */
bool app_key_is_enter_pairing(void);

/**
    * @brief  judge if now do factory reset by long  press key.
    * @param  void
    * @return do factory reset by long press key or not
    */
bool app_key_is_enter_factory_reset(void);

/**
    * @brief  set volume by press key.
    * @param  volume be set by press key;
    * @return void
    */
void app_key_set_volume_status(bool volume_status);

/**
    * @brief  judge if set volume by press key.
    * @param  void
    * @return set volume by press key or not
    */
bool app_key_is_set_volume(void);

/**
    * @brief  judge if set vol change mmi.
    * @param  void
    * @return void
    */
void app_key_check_vol_mmi(void);

/**
    * @brief  judge if set vol change mmi.
    * @param  void
    * @return set vol change or not
    */
bool app_key_is_set_vol_mmi(void);

/**
    * @brief  disallow sync play vol changed tone.
    * @param  disallow sync
    * @return void
    */
void app_key_set_sync_play_vol_changed_tone_disallow(bool disallow_sync);

/**
    * @brief  disallow sync play vol changed tone.
    * @param  void
    * @return sync play or not
    */
bool app_key_is_sync_play_vol_changed_tone_disallow(void);

/**
    * @brief  key single click process.
    * @param  key:This parameter is from KEY0_MASK to KEY7_MASK
    * @return void
    */
void app_key_single_click(uint8_t key);

/**
    * @brief  key single click process.
    * @param  key:This parameter is from KEY0_MASK to KEY7_MASK
    * @param  clicks:click counts
    * @return void
    */
void app_key_hybrid_multi_clicks(uint8_t key, uint8_t clicks);

/**
    * @brief  let key module know, now state is power on or not.
    *         it should be called by other module.
    * @param  on power on flag
    * @return void
    */
void app_key_set_power_on(bool on);

/**
    * @brief  key module handle message.
    *         when app_io_handle_msg recv msg IO_MSG_GPIO_KEY, it will be called.
    * @param  void
    * @return void
    */
void app_key_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  check key index is combine key power on off.
    * @param  key number
    * @return key index is combine key or not
    */
bool app_key_is_combinekey_power_on_off(uint8_t key);

/**
    * @brief  check gpio index is combine key power on off.
    * @param  key number
    * @return gpio index is combine key or not
    */
bool app_key_is_gpio_combinekey_power_on_off(uint8_t gpio_index);

/**
    * @brief  Read current status of Hall Switch sensor.
    * @param  void
    * @return status
    */
uint8_t key_read_hall_switch_status(void);

/**
    * @brief  power onoff combinekey process before enter dlps.
    * @param  void
    * @return void
    */
void app_power_onoff_combinekey_dlps_process(void);
void key_execute_action(uint8_t action);

/**
    * @brief  Execute the corresponding action with Hall Switch status.
    *         Once Hall Switch status changed. It required a stable timer for debouncing.
    *         After the debouncing process, related action will be executed.
    *         Note that action only be executed in the correct status.
    * @param  hall_switch_action Hall Switch high or Hall Switch Low.
    * @return void
    */
void key_execute_hall_switch_action(uint8_t hall_switch_action);

/**
    * @brief  App parses if there are hall switch function definitions in the hybrid key behaviors.
    *         Hall switch function is defined as one type of hybrid key.
    *         Execute this function to check if there's hall switch function used.
    *         Hall switch function actions are different from general hybrid key actions.
    * @param  void
    * @return void
    */
void hybird_key_parse_hall_switch_setting(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_KEY_PROCESS_H_ */
