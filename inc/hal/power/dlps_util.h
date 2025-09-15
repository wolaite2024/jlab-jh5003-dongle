/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      dlps_util.h
* @brief
* @details
* @author
* @date      2024-10-11
* @version   v1.1
* *********************************************************************************************************
*/


#ifndef _DLPS_DEBUG_
#define _DLPS_DEBUG_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "rtl876x.h"


/** @addtogroup DLPS_DEBUG DLPS Debug
  * @brief DLPS debug function module.
  * @{
  */

/** @defgroup DLPS_DEBUG_Exported_Constants DLPS Debug Exported Constants
  * @brief DLPS or power down wake up reason.
  * @{
  */

#if defined(CONFIG_SOC_SERIES_RTL87X3D) || (TARGET_RTL8773DO == 1) || (TARGET_RTL8773DFL == 1)

#define PLATFORM_DLPS_WAKEUP_UNKNOWN              0x0000 //!< Allowed to wake up from platform DLPS mode.
#define PLATFORM_DLPS_WAKEUP_USER                 0x0001
#define PLATFORM_DLPS_WAKEUP_OS                   0x0002
#define PLATFORM_DLPS_WAKEUP_PRE_SYSTEM_LEVEL     0x0003
#define PLATFORM_DLPS_WAKEUP_PF_RTC               0x0040
#define PLATFORM_DLPS_WAKEUP_ADSP                 0x0080
#define PLATFORM_DLPS_WAKEUP_POW                  0x0100
#define PLATFORM_DLPS_WAKEUP_MFB                  0x0200
#define PLATFORM_DLPS_WAKEUP_USB_RESUME           0x0400
#define PLATFORM_DLPS_WAKEUP_GPIO                 0x0800
#define PLATFORM_DLPS_WAKEUP_MAC                  0x1000
#define PLATFORM_DLPS_WAKEUP_RTC                  0x2000
#define PLATFORM_DLPS_WAKEUP_VAD                  0x4000
#define PLATFORM_DLPS_WAKEUP_VADBUF               0x8000

/* Power down wake up reason */
#define POWER_DOWN_WAKEUP_RTC                       BIT0
#define POWER_DOWN_WAKEUP_PAD                       BIT1
#define POWER_DOWN_WAKEUP_MFB                       BIT2
#define POWER_DOWN_WAKEUP_ADP                       BIT3
#define POWER_DOWN_WAKEUP_CTC                       BIT4 //!< rtl87x3d not support.

#else /* BB2 or BBLite2 */

#define PLATFORM_DLPS_WAKEUP_UNKNOWN              0x0000
#define PLATFORM_DLPS_WAKEUP_USER                 0x0001
#define PLATFORM_DLPS_WAKEUP_OS                   0x0002
#define PLATFORM_DLPS_WAKEUP_PRE_SYSTEM_LEVEL     0x0003
#define PLATFORM_DLPS_WAKEUP_PF_RTC               0x0100
#define PLATFORM_DLPS_WAKEUP_RTC                  0x0200
#define PLATFORM_DLPS_WAKEUP_MAC                  0x0400
#define PLATFORM_DLPS_WAKEUP_GPIO                 0x0800
#define PLATFORM_DLPS_WAKEUP_USB_RESUME           0x1000
#define PLATFORM_DLPS_WAKEUP_MFB                  0x2000
#define PLATFORM_DLPS_WAKEUP_POW                  0x4000
#define PLATFORM_DLPS_WAKEUP_CTC                  0x8000

/* Power down wake up reason */
#define POWER_DOWN_WAKEUP_RTC                       BIT0 //!< Must open 32k in power down.
#define POWER_DOWN_WAKEUP_PAD                       BIT1
#define POWER_DOWN_WAKEUP_MFB                       BIT2 //!< Must open 32k in power down.
#define POWER_DOWN_WAKEUP_ADP                       BIT3
#define POWER_DOWN_WAKEUP_CTC                       BIT4 //!< Must open 32k in power down.

#endif

/** End of Group DLPS_DEBUG_Exported_Constants
  * @}
  */

/** @defgroup DLPS_DEBUG_Exported_Types DLPS Debug Exported Types
  * @brief DLPS error code.
  * @{
  */

typedef enum
{
    BTMAC_POWERMODE_ERROR_NONE                         = 0x0,  //!< Allowed to enter bt mac low power mode.
    BTMAC_POWERMODE_ERROR_POWER_MODE                   = 0x1,
    BTMAC_POWERMODE_ERROR_LEGACY_SCAN                  = 0x2,
    BTMAC_POWERMODE_ERROR_ROLE_SWITCH                  = 0x3,
    BTMAC_POWERMODE_ERROE_BQB                          = 0x4,
    BTMAC_POWERMODE_ERROR_PSD                          = 0x5,
    BTMAC_POWERMODE_ERROR_CSB_ENABLE                   = 0x6,
    BTMAC_POWERMODE_ERROR_NOT_EMPTY_QUEUE_OF_LOWER     = 0x7,
    BTMAC_POWERMODE_ERROR_CONTROLLER_TO_HOST_BUSY      = 0x8,
    BTMAC_POWERMODE_ERROR_TX_BUSY                      = 0x9,
    BTMAC_POWERMODE_ERROR_LEGACY_NOT_IDLE              = 0xA,
    BTMAC_POWERMODE_ERROR_LE_REG_S_INST                = 0xB,
    BTMAC_POWERMODE_ERROR_ADV_STATE_NOT_IDLE           = 0xC,
    BTMAC_POWERMODE_ERROR_SCAN_STATE_NOT_IDLE          = 0xD,
    BTMAC_POWERMODE_ERROR_INITIATOR_UNIT_ENABLE        = 0xE,
    BTMAC_POWERMODE_ERROR_CHANNEL_MAP_UPDATE           = 0xF,
    BTMAC_POWERMODE_ERROR_CONNECTION_UPDATE            = 0x10,
    BTMAC_POWERMODE_ERROR_PHY_UPDATE                   = 0x11,
    BTMAC_POWERMODE_ERROR_CONN_STATE_NOT_IDLE          = 0x12,
    BTMAC_POWERMODE_ERROR_LE_SCHE_NOT_READY            = 0x13,
    BTMAC_POWERMODE_ERROR_INTERRUPT_PENDING            = 0x14,
    BTMAC_POWERMODE_ERROR_WAKEUP_TIME                  = 0x15,
    BTMAC_POWERMODE_ERROR_32K_CHECK_LOCK               = 0x16,
    BTMAC_POWERMODE_ERROR_HW_TIMER_RUNNING             = 0x17,
    BTMAC_POWERMODE_ERROR_LE_ISO_ACTIVE                = 0x18,
} T_BTMAC_POWERMODE_ERROR_CODE;

typedef enum
{
    PLATFORM_POWERMODE_ERROR_NONE                  = 0x0, //!< Allowed to enter platform low power mode.
    PLATFORM_POWERMODE_ERROR_POWER_MODE            = 0x1,
    PLATFORM_POWERMODE_ERROR_DISABLE_DLPS_TIME     = 0x2,
    PLATFORM_POWERMODE_ERROR_32K_CHECK_LOCK        = 0x3,
    PLATFORM_POWERMODE_ERROR_LOG_DMA_NOT_IDLE      = 0x4,
    PLATFORM_POWERMODE_ERROR_CALLBACK_CHECK        = 0x5,
    PLATFORM_POWERMODE_ERROR_INTERRUPT_OCCURRED    = 0x6,
    PLATFORM_POWERMODE_ERROR_WAKEUP_TIME           = 0x7,
    PLATFORM_POWERMODE_ERROR_FREE_BLOCK_STORE      = 0x8,
    PLATFORM_POWERMODE_ERROR_BUFFER_PRE_ALLOCATE   = 0x9,
    PLATFORM_POWERMODE_ERROR_DATA_UART             = 0xa, //!< rtl87x3g not support.
} T_PLATFORM_POWERMODE_ERROR_CODE;

/** End of Group DLPS_DEBUG_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup DLPS_DEBUG_Exported_Functions DLPS Debug Exported Functions
 * @{
 */

/**
 * @brief  Check current DLPS wake up timer.
 * @param  None.
 * @retval true: Find DLPS wake up timer success.
 * @retval false: Find DLPS wake up timer failed.
*/
bool dlps_check_os_timer(void);

/**
 * @brief  Check power down wake up reason, must open 32k in power down except PAD and ADP.
 * @param  None.
 * @return Return power down wake up reason.
*/
uint8_t power_down_check_wake_up_reason(void);

/**
 * @brief  Print the DLPS wake up status.
 * @param  None.
 * @return None.
*/
void dlps_utils_print_wake_up_info(void);

#if defined(CONFIG_SOC_SERIES_RTL87X3D) || (TARGET_RTL8773DO == 1) || (TARGET_RTL8773DFL == 1)
/**
 * @brief  Clear power down wake up status.
 * @param  None.
 * @return None.
*/
void power_down_wake_up_status_clear(void);
#endif

/**
 * dlps_util.h
 *
 * \brief   Get the bt mac sleep statics.
 *
 * \details Get the bt mac sleep time and count.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[out] wakeup_count: bt mac low power mode wake up count.
 * \param[out] last_wakeup_clk: bt mac last wake up clock.
 * \param[out] last_sleep_clk: bt mac last sleep clock.
 *
 * \return None.
 */
void dlps_utils_get_btmac_lpm_statics(uint32_t *wakeup_count, uint32_t *last_wakeup_clk,
                                      uint32_t *last_sleep_clk);

/**
 * dlps_util.h
 *
 * \brief   Get the bt mac low power mode error code.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \details Get the bt mac blocking reason.
 *
 * \param[out] None.
 *
 * \retval error_code: See the error code in details \ref T_BTMAC_POWERMODE_ERROR_CODE.
 */
T_BTMAC_POWERMODE_ERROR_CODE  dlps_util_get_btmac_error_code(void);

/**
 * dlps_util.h
 *
 * \brief   Get the platform low power mode refuse reason.
 *
 * \details When bt mac lpm error code is 0, and platform error code is \ref PLATFORM_POWERMODE_ERROR_CALLBACK_CHECK, check the
 *          refuse reason for DLPS blocking reason.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[out] None.
 *
 * \retval error_code: See the error code in details \ref T_PLATFORM_POWERMODE_ERROR_CODE.
 */
uint32_t *dlps_utils_get_platform_refuse_reason(void);

/**
 * dlps_util.h
 *
 * \brief   Get the platform low power mode error code.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \details Get the platform blocking reason.
 *
 * \param[out] None.
 *
 * \retval error_code: See the error code in details \ref T_PLATFORM_POWERMODE_ERROR_CODE.
 */
T_PLATFORM_POWERMODE_ERROR_CODE  dlps_util_get_platform_error_code(void);


/* for CPU sleep debug, temporarily unavailable for bblite2 */
/**
 * dlps_util.h
 *
 * \brief   Start monitor CPU sleep time.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 * \param[in] None.
 * \return    None.
 */
void dlps_util_cpu_sleep_monitor_start(void);

/**
 * dlps_util.h
 *
 * \brief   Stop monitor CPU sleep time.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 * \param[in] None.
 * \return    None.
 */
void dlps_util_cpu_sleep_monitor_stop(void);

/**
 * dlps_util.h
 *
 * \brief   Init DLPS debug timer for monitor.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in]  period_ms               The interval for print in miliseconds.
 *
 * \return           The monitor timer is created successfully or not.
 * \retval true      The monitor timer is created successfully.
 * \retval false     The monitor timer is failed to create.
 */
bool dlps_util_debug_init(uint32_t period_ms);

/**
 *
 * \brief   Enable print the timer or task block low power mode.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 * \note     If enabled, frequent printing behavior may occur.
 *
 */
void dlps_utils_enable_print_block_timer_task_info(void);

/**
 *
 * \brief   Disable print the timer or task block low power mode.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 */
void dlps_utils_disable_print_block_timer_task_info(void);

/** @} */ /* End of group DLPS_DEBUG_Exported_Functions */

#ifdef __cplusplus
}
#endif

#endif /* _DLPS_DEBUG_ */

/** @} */ /* End of group DLPS_DEBUG */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/

