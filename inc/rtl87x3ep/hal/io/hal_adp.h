/*
 * Copyright (c) 2021, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _HAL_ADP_
#define _HAL_ADP_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup 87x3e_HAL_ADP HAL ADP
 * @{
 */

/** @defgroup T_ADP_DETECT adp detect type
 * @{
 */

typedef enum
{
    ADP_DETECT_5V,
    ADP_DETECT_MAX,
} T_ADP_DETECT;

/** End of group T_ADP_DETECT
  * @}
  */

/** @defgroup T_ADP_PLUG_EVENT adp plug event
 * @{
 */

typedef enum
{
    ADP_EVENT_PLUG_IN,
    ADP_EVENT_PLUG_OUT,
    ADP_EVENT_MAX,
} T_ADP_PLUG_EVENT;

/** End of group T_ADP_PLUG_EVENT
  * @}
  */

/** @defgroup T_ADP_LEVEL adp level
 * @{
 */

typedef enum
{
    ADP_LEVEL_LOW,
    ADP_LEVEL_HIGH,
    ADP_LEVEL_UNKNOWN,
} T_ADP_LEVEL;

/** End of group T_ADP_LEVEL
  * @}
  */

/** @defgroup T_ADP_STATE adp state
 * @{
 */

typedef enum
{
    ADP_STATE_DETECTING,
    ADP_STATE_IN,
    ADP_STATE_OUT,
    ADP_STATE_UNKNOWN,
} T_ADP_STATE;

/** End of group T_ADP_STATE
  * @}
  */

/** @defgroup T_ADP_WAKE_UP_MODE adp wake up mode
 * @{
 */
typedef enum
{
    ADP_WAKE_UP_POWER_OFF,     /*!< Config adp power off wake up. */
    ADP_WAKE_UP_GENERAL,       /*!< Config adp dlps or power down wake up. */
} T_ADP_WAKE_UP_MODE;

/** End of group T_ADP_WAKE_UP_MODE
  * @}
  */

/** @defgroup T_ADP_WAKE_UP_POL adp wake up polarity
 * @{
 */
typedef enum
{
    ADP_WAKE_UP_POL_HIGH,     /*!< Config adp wake up high polarity. */
    ADP_WAKE_UP_POL_LOW,      /*!< Config adp wake up low polarity. */
} T_ADP_WAKE_UP_POL;

/** End of group T_ADP_WAKE_UP_POL
  * @}
  */

/**
  * @brief  Adp interrupt callback function type
  * @param  None
  * @retval None
  */
typedef void (*P_ADP_ISR_CBACK)(void);

/**
  * @brief  Adp plug callback function type
  * @param  event: adp plug event
  *     This parameter can be one of the following values:
  *     @arg ADP_EVENT_PLUG_IN: adp plug in.
  *     @arg ADP_EVENT_PLUG_OUT: adp plug out.
  * @param  user_data: user data
  * @retval None
  */
typedef void (*P_ADP_PLUG_CBACK)(T_ADP_PLUG_EVENT event, void *user_data);

/**
  * @brief  Update adp 5V interrupt callback
  * @param  adp_type: use adp 5v
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use adp 5v.
  * @param  cback: adp interrupt callback function
  * @retval true: Update adp interrupt callback success
  * @retval false: Update adp interrupt callback failed
  */
bool adp_update_isr_cb(T_ADP_DETECT adp_type, P_ADP_ISR_CBACK cback);

/**
  * @brief  Get adp 5V current level
  * @param  adp_type: use adp 5v
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use adp 5v.
  * @retval return adp current level
  */
T_ADP_LEVEL adp_get_level(T_ADP_DETECT adp_type);

/**
  * @brief  Register adp 5V callback
  * @param  adp_type: use adp 5v
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use adp 5v.
  * @param  cback: adp callback function
  * @param  user_data: user data
  * @retval true: Register adp callback success
  * @retval false: Register adp callback failed
  */
bool adp_register_state_change_cb(T_ADP_DETECT adp_type, P_ADP_PLUG_CBACK cback, void *user_data);

/**
  * @brief  Unregister adp 5V callback
  * @param  adp_type: use adp 5v
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use adp 5v.
  * @param  cback: adp callback function
  * @retval true: Unregister adp callback success
  * @retval false: Unregister adp callback failed
  */
bool adp_unregister_state_change_cb(T_ADP_DETECT adp_type, P_ADP_PLUG_CBACK cback);

/**
  * @brief  Get adp 5V current state
  * @param  adp_type: use adp 5v
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use adp 5v.
  * @retval return adp current state
  */
T_ADP_STATE adp_get_current_state(T_ADP_DETECT adp_type);

/**
  * @brief  Get adp 5V function debounce time
  * @param  adp_type: use adp 5v
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use adp 5v.
  * @param  io_in_debounce_time: adp in debounce time
  * @param  io_out_debounce_time: adp out debounce time
  * @retval true: get adp io debounce time success
  * @retval false: get adp io debounce time failed
  */
bool adp_get_debounce_time(T_ADP_DETECT adp_type, uint32_t *p_in_debounce_ms,
                           uint32_t *p_out_debounce_ms);

/**
  * @brief  adapter wake up function enable
  * @param  mode: config power off or none power off
  *     This parameter value is ADP_WAKE_UP_POWER_OFF or ADP_WAKE_UP_GENERAL.
  * @param  pol: polarity to wake up
  *     This parameter ADP_WAKE_UP_POL_HIGH means high level to wakeup, ADP_WAKE_UP_POL_LOW means low level to wakeup.
  * @retval true: enable success
  * @retval false: enable failed
  */
bool adp_wake_up_enable(T_ADP_WAKE_UP_MODE mode, T_ADP_WAKE_UP_POL pol);

/**
  * @brief  adapter wake up function disable
  * @param  mode: config power off or none power off
  *     This parameter value is ADP_WAKE_UP_POWER_OFF or ADP_WAKE_UP_GENERAL.
  * @retval none
  */
void adp_wake_up_disable(T_ADP_WAKE_UP_MODE mode);

/** End of group 87x3e_HAL_ADP
  * @}
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HAL_ADP_ */

