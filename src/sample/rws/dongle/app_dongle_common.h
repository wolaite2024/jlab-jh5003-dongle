#ifndef _APP_DONGLE_COMMON_H_
#define _APP_DONGLE_COMMON_H_

#include "app_link_util.h"

typedef enum
{
    ADV_INTERVAL_EVENT_TIMEOUT,
    ADV_INTERVAL_EVENT_BT_STREAMING_START,
    ADV_INTERVAL_EVENT_BT_STREAMING_STOP,
} T_APP_HEADSET_ADV_EVENT;

void app_dongle_common_init(void);

/**
    * @brief  Get connected dongle's link
    * @param  void
    * @return link info
    */
T_APP_BR_LINK *app_dongle_get_connected_dongle_link(void);

/**
    * @brief  Get connected lea phone link
    * @param  void
    * @return link info
    */
T_APP_LE_LINK *app_dongle_get_connected_lea_phone_link(void);

/**
    * @brief  Get connected phone link
    * @param  void
    * @return link info
    */
T_APP_BR_LINK *app_dongle_get_connected_phone_link(void);

/**
    * @brief  Get connected dongle's addr
    * @param  bd_addr bluetooth address
    * @return true/false
    */
bool app_dongle_get_connected_dongle_addr(uint8_t *addr);

/**
    * @brief  Check the input addr is dongle addr or not
    * @param  check_addr : addr wants to check
    * @return true if it is dongle addr
    */
bool app_dongle_is_dongle_addr(uint8_t *check_addr);

/**
    * @brief  Save addr as dongle addr
    * @param  bd_addr want to save
    * @return void
    */
void app_dongle_save_dongle_addr(uint8_t *addr);

/**
    * @brief  Start adv for dongle binding
    * @param  pairing new dongle or not
    * @return none
    */
void app_dongle_adv_start(bool enable_pairing);

/**
    * @brief  Stop adv for dongle binding
    * @param  void
    * @return none
    */
void app_dongle_adv_stop(void);

/**
    * @brief  handle headset adv state change
    * @param  event : adv event
    * @return none
    */
void app_dongle_handle_heaset_adv_interval(T_APP_HEADSET_ADV_EVENT event);


#if F_APP_LEA_SUPPORT
T_APP_LE_LINK *app_dongle_get_le_audio_dongle_link(void);
T_APP_LE_LINK *app_dongle_get_le_audio_phone_link(void);
#if F_APP_GAMING_DONGLE_SUPPORT
bool app_dongle_is_iso_data_from_dongle(uint16_t conn_handle);
void app_dongle_check_exit_pairing_state(uint8_t link_state, uint8_t *bd_addr);
#endif
#endif

#if F_APP_LEGACY_DONGLE_BINDING || F_APP_LEA_DONGLE_BINDING
/**
    * @brief  Get timer idx for change headset adv interal
    * @param  void
    * @return none
    */
uint8_t app_dongle_get_change_headset_adv_timer_idx(void);

#if F_APP_LEGACY_DONGLE_BINDING
/**
    * @brief  Get legacy adv handle for legacy dongle adv
    * @param  void
    * @return adv handle
    */
uint8_t app_dongle_get_legacy_adv_handle(void);
#endif

#if F_APP_LEA_DONGLE_BINDING
uint8_t app_dongle_lea_adv_get_conn_id(void);
#endif

extern bool dongle_pairing_non_intentionally;

#endif
#endif
