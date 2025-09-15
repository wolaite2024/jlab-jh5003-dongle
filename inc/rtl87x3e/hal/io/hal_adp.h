/*
 * Copyright (c) 2024, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _HAL_ADP_
#define _HAL_ADP_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @addtogroup HAL_ADP HAL ADP
  * @brief HAL ADP function module.
  * @{
  */

/** @defgroup HAL_ADP_Exported_Types HAL ADP Exported Types
  * @{
  */

/** @defgroup T_ADP_DETECT ADP Detect Type
 * @{
 */

typedef enum
{
    ADP_DETECT_5V,
    ADP_DETECT_IO,
    ADP_DETECT_MAX,
} T_ADP_DETECT;

/** End of group T_ADP_DETECT
  * @}
  */

/** @defgroup T_ADP_PLUG_EVENT ADP Plug Event
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

/** @defgroup T_ADP_LEVEL ADP Level
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

/** @defgroup T_ADP_STATE ADP State
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


/** @defgroup T_ADP_IO_GPIO_STATE ADP IO GPIO State
 * @{
 */

typedef enum
{
    ADP_IO_GPIO_CLOSE,
    ADP_IO_GPIO_OPEN,
} T_ADP_IO_GPIO_STATE;

/** End of group T_ADP_IO_GPIO_STATE
  * @}
  */

/** @defgroup T_ADP_IO_DEBOUNCE_TIME ADP IO Debounce Time
 * @{
 */
typedef enum
{
    IO_DEBOUNCE_TIME_0MS,       /*!< ADP IO debounce time 0ms.   */
    IO_DEBOUNCE_TIME_10MS,      /*!< ADP IO debounce time 10ms.  */
    IO_DEBOUNCE_TIME_20MS,      /*!< ADP IO debounce time 20ms.  */
    IO_DEBOUNCE_TIME_40MS,      /*!< ADP IO debounce time 40ms.  */
    IO_DEBOUNCE_TIME_60MS,      /*!< ADP IO debounce time 60ms.  */
    IO_DEBOUNCE_TIME_80MS,      /*!< ADP IO debounce time 80ms.  */
    IO_DEBOUNCE_TIME_100MS,     /*!< ADP IO debounce time 100ms. */
    IO_DEBOUNCE_TIME_200MS,     /*!< ADP IO debounce time 200ms. */
    IO_DEBOUNCE_TIME_300MS,     /*!< ADP IO debounce time 300ms. */
    IO_DEBOUNCE_TIME_400MS,     /*!< ADP IO debounce time 400ms. */
    IO_DEBOUNCE_TIME_500MS,     /*!< ADP IO debounce time 500ms. */
    IO_DEBOUNCE_TIME_600MS,     /*!< ADP IO debounce time 600ms. */
    IO_DEBOUNCE_TIME_700MS,     /*!< ADP IO debounce time 700ms. */
    IO_DEBOUNCE_TIME_800MS,     /*!< ADP IO debounce time 800ms. */
    IO_DEBOUNCE_TIME_900MS,     /*!< ADP IO debounce time 900ms. */
    IO_DEBOUNCE_TIME_1000MS,    /*!< ADP IO debounce time 1000ms.*/
    IO_DEBOUNCE_TIME_MAX,
} T_ADP_IO_DEBOUNCE_TIME;

/** End of group T_ADP_IO_DEBOUNCE_TIME
  * @}
  */

/** @defgroup T_ADP_WAKE_UP_MODE ADP Wake Up Mode
 * @{
 */
typedef enum
{
    ADP_WAKE_UP_POWER_OFF,     /*!< Config ADP power off wake up. */
    ADP_WAKE_UP_GENERAL,       /*!< Config ADP DLPS or power down wake up. */
} T_ADP_WAKE_UP_MODE;

/** End of group T_ADP_WAKE_UP_MODE
  * @}
  */

/** @defgroup T_ADP_WAKE_UP_POL ADP Wake Up Polarity
 * @{
 */
typedef enum
{
    ADP_WAKE_UP_POL_HIGH,     /*!< Config ADP wake up high polarity. */
    ADP_WAKE_UP_POL_LOW,      /*!< Config ADP wake up low polarity. */
} T_ADP_WAKE_UP_POL;

/** End of group T_ADP_WAKE_UP_POL
  * @}
  */

/** End of Group HAL_ADP_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup HAL_ADP_Exported_Functions HAL ADP Exported Functions
  * @{
  */

/**
  * @brief  ADP interrupt callback function type.
  * @param  None.
  * @return None.
  */
typedef void (*P_ADP_ISR_CBACK)(void);

/**
  * @brief  ADP plug callback function type.
  * @param  event: ADP plug event.
  *     This parameter can be one of the following values:
  *     @arg ADP_EVENT_PLUG_IN: ADP plug in.
  *     @arg ADP_EVENT_PLUG_OUT: ADP plug out.
  * @param  user_data: user data.
  * @return None.
  */
typedef void (*P_ADP_PLUG_CBACK)(T_ADP_PLUG_EVENT event, void *user_data);

/**
  * @brief  Update ADP IO/5V interrupt callback.
  * @param  adp_type: use ADP 5V or ADP IO.
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use ADP 5V.
  *     @arg ADP_DETECT_IO: use ADP IO.
  * @param  cback: ADP interrupt callback function.
  * @retval true: Update ADP interrupt callback success.
  * @retval false: Update ADP interrupt callback failed.
  */
bool adp_update_isr_cb(T_ADP_DETECT adp_type, P_ADP_ISR_CBACK cback);

/**
  * @brief  Get ADP IO/5V current level.
  * @param  adp_type: use ADP 5V or ADP IO.
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use ADP 5V.
  *     @arg ADP_DETECT_IO: use ADP IO.
  * @return The ADP current level.
  */
T_ADP_LEVEL adp_get_level(T_ADP_DETECT adp_type);

/**
  * @brief  Register ADP IO/5V callback.
  * @param  adp_type: use ADP 5V or ADP IO.
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use ADP 5V.
  *     @arg ADP_DETECT_IO: use ADP IO.
  * @param  cback: ADP callback function.
  * @param  user_data: user data.
  * @retval true: Register ADP callback success.
  * @retval false: Register ADP callback failed.
  */
bool adp_register_state_change_cb(T_ADP_DETECT adp_type, P_ADP_PLUG_CBACK cback, void *user_data);

/**
  * @brief  Unregister ADP IO/5V callback.
  * @param  adp_type: use ADP 5V or ADP IO.
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use ADP 5V.
  *     @arg ADP_DETECT_IO: use ADP IO.
  * @param  cback: ADP callback function.
  * @retval true: Unregister ADP callback success.
  * @retval false: Unregister ADP callback failed.
  */
bool adp_unregister_state_change_cb(T_ADP_DETECT adp_type, P_ADP_PLUG_CBACK cback);

/**
  * @brief  Get ADP IO/5V current state.
  * @param  adp_type: use ADP 5V or ADP IO.
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use ADP 5V.
  *     @arg ADP_DETECT_IO: use ADP IO.
  * @return ADP current state.
  */
T_ADP_STATE adp_get_current_state(T_ADP_DETECT adp_type);

/**
  * @brief  Get ADP IO/5V function debounce time.
  * @param  adp_type: use ADP 5V or ADP IO.
  *     This parameter can be one of the following values:
  *     @arg ADP_DETECT_5V: use ADP 5V.
  *     @arg ADP_DETECT_IO: use ADP IO.
  * @param  io_in_debounce_time: ADP in debounce time.
  * @param  io_out_debounce_time: ADP out debounce time.
  * @retval true: get ADP IO debounce time success.
  * @retval false: get ADP IO debounce time failed.
  */
bool adp_get_debounce_time(T_ADP_DETECT adp_type, uint32_t *p_in_debounce_ms,
                           uint32_t *p_out_debounce_ms);

/**
  * @brief  Only can set ADP IO function debounce time.
  * @param  adp_type: only can be set @ref ADP_DETECT_IO.
  * @param  io_in_debounce_time: ADP IO in debounce time.
  * @param  io_out_debounce_time: ADP IO out debounce time.
  * @retval true: set ADP IO debounce success.
  * @retval false: ADP type error or debounce time overflow.
  */
bool adp_set_debounce_time(T_ADP_DETECT adp_type, T_ADP_IO_DEBOUNCE_TIME io_in_debounce_time,
                           T_ADP_IO_DEBOUNCE_TIME io_out_debounce_time);

/**
 * @brief  Close ADP function.
 * @param  adp_type: only can be set @ref ADP_DETECT_IO.
 * @retval true: close ADP IO function success.
 * @retval false: ADP type error.
 */
bool adp_close(T_ADP_DETECT adp_type);

/**
  * @brief  Open ADP function.
  * @param  adp_type: only can be set @ref ADP_DETECT_IO.
  * @retval true: open ADP IO function success.
  * @retval false: ADP type error.
  */
bool adp_open(T_ADP_DETECT adp_type);

/**
  * @brief  Adapter wake up function enable.
  * @param  mode: config power off or none power off.
  *     This parameter value is @ref ADP_WAKE_UP_POWER_OFF or @ref ADP_WAKE_UP_GENERAL.
  * @param  pol: polarity to wake up.
  *     This parameter ADP_WAKE_UP_POL_HIGH means high level to wakeup, ADP_WAKE_UP_POL_LOW means low level to wakeup.
  * @retval true: enable success.
  * @retval false: enable failed.
  */
bool adp_wake_up_enable(T_ADP_WAKE_UP_MODE mode, T_ADP_WAKE_UP_POL pol);

/**
  * @brief  Adapter wake up function disable.
  * @param  mode: config power off or none power off.
  *     This parameter value is @ref ADP_WAKE_UP_POWER_OFF or @ref ADP_WAKE_UP_GENERAL.
  * @return None.
  */
void adp_wake_up_disable(T_ADP_WAKE_UP_MODE mode);

/** @} */ /* End of group HAL_ADP_Exported_Functions */
/** @} */ /* End of group HAL_ADP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HAL_ADP_ */

