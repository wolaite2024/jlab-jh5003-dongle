/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_wdg.h
* @brief     The header file of watch dog driver.
* @details
* @author    Lory_xu
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef _RTL876X_WDG_H_
#define _RTL876X_WDG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl876x.h"
#include "wdg.h"

/** @addtogroup 87x3e_WATCH_DOG Watch Dog
  * @brief Watch dog module.
  * @{
  */


/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup 87x3e_WATCH_DOG_Exported_Types Watch Dog Exported Types
  * @{
  */

/**
  * @brief   Watch Dog timer register definition.
  */
typedef union
{
    /** \name Union members */
    /** @{ */
    uint32_t d32;                  //!< The word member in the union.
    /** @} */
    /** \name Bit field structure */
    /** @{ */
    struct
    {
        uint32_t div_factor  : 16; //!< R/W Dividing factor[15:0](at least set 1).
        uint32_t en_byte     : 8;  //!< R/WSet 0xA5 to enable watch dog timer[23:16].
        uint32_t clear       : 1;  //!< W Write 1 to clear timer[24].
        uint32_t cnt_limit   : 4;  //!< R/W [28:25] 0: 0x001 1: 0x003 .... 10: 0x7FF 11~15: 0xFFF.
        uint32_t wdg_mode    : 2;  //!< R/W Select watch dog mode[29:30].
        uint32_t timeout     : 1;  //!< R/W1C Watch dog timer timeout[31] 1 cycle pulse.
    } b;                           //!< The struct member in the union.
    /** @} */
} T_WATCH_DOG_TIMER_REG;

/**
  * @brief   Watch Dog Type definition.
  */
typedef enum
{
    WDG_TYPE_DISABLE,       //!< Disable watch dog.
    WDG_TYPE_ENABLE,        //!< Enable watch dog.
    WDG_TYPE_RESTART,       //!< Restart watch dog.
} T_WDG_TYPE;

/** End of group 87x3e_WATCH_DOG_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup 87x3e_WATCH_DOG_Exported_Functions Watch Dog Exported Functions
  * @{
  */

/**
 *
 * \brief  Enable core watch dog clock.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void wdg_demo(void)
 * {
 *     WDG_ClockEnable();
 *     WDG_Start_Core(4000, RESET_ALL);
 * }
 * \endcode
 */
void WDG_ClockEnable(void);

/**
 *
 * \brief  Config core watch dog.
 *
 * \param[in] div_factor: The div of watch dog, can be a value of 1 to 65535.
 * \param[in] cnt_limit: The cnt of watch dog.
 *          This parameter can be one of the following values:
 *                - 0: 0x001
 *                - 1: 0x003
 *                - 2: 0x007
 *                - 3: 0x00F
 *                - 4: 0x01F
 *                - 5: 0x03F
 *                - 6: 0x07F
 *                - 7: 0x0FF
 *                - 8: 0x1FF
 *                - 9: 0x3FF
 *                - 10: 0x7FF
 *                - 11~15: 0xFFF
 * \param[in] wdg_mode: Watch dog mode \ref T_WDG_MODE.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void wdg_demo(void)
 * {
 *     WDG_Config(1, 15, RESET_ALL);
 * }
 * \endcode
 */
void WDG_Config(uint16_t div_factor, uint8_t  cnt_limit, T_WDG_MODE  wdg_mode);

/**
 *
 * \brief  Calculate div and cnt through timeout.
 *
 * \param[in] timeout_ms: Time to be calculated, can be a value of 0 to 8192000.
 * \param[out] div_factor: The div calculated by time.
 * \param[out] cnt_limit: The cnt_limit calculated by div and cnt.
 *          This parameter can be one of the following values:
 *                - 0: 0x001
 *                - 1: 0x003
 *                - 2: 0x007
 *                - 3: 0x00F
 *                - 4: 0x01F
 *                - 5: 0x03F
 *                - 6: 0x07F
 *                - 7: 0x0FF
 *                - 8: 0x1FF
 *                - 9: 0x3FF
 *                - 10: 0x7FF
 *                - 11~15: 0xFFF
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void wdg_demo(void)
 * {
 *     uint16_t div_factor = 0;
 *     uint8_t cnt_limit = 0;
 *     WDG_Calculate_Timeout_ms(4000, &div_factor, &cnt_limit);
 * }
 * \endcode
 */
bool WDG_Calculate_Config(uint32_t timeout_ms, uint16_t *div_factor, uint8_t *cnt_limit);

/**
 *
 * \brief  Set core watch dog.
 *
 * \param[in] type: The type of watch dog \ref T_WDG_TYPE.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void wdg_demo(void)
 * {
 *     WDG_Set_Core(WDG_TYPE_ENABLE);
 * }
 * \endcode
 */
void WDG_Set_Core(T_WDG_TYPE type);
#define WDG_Enable()  WDG_Set_Core(WDG_TYPE_ENABLE)     //!< Enable core watch dog.

/**
 *
 * \brief  Start core watch dog.
 *
 * \param[in] ms: The timeout of watch dog, can be a value of 0 to 8192000.
 * \param[in] wdg_mode: Watch dog mode \ref T_WDG_MODE.
 *
 * \return The result of start core watch dog.
 * \retval true: Start watch dog success.
 * \retval false: Start watch dog failed.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void wdg_demo(void)
 * {
 *     WDG_ClockEnable();
 *     WDG_Start_Core(4000, RESET_ALL);
 * }
 * \endcode
 */
bool WDG_Start_Core(uint32_t ms, T_WDG_MODE wdg_mode);

/**
 *
 * \brief  Disable core watch dog.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void wdg_demo(void)
 * {
 *     WDG_Disable_Core();
 * }
 * \endcode
 */
void WDG_Disable_Core(void);

/**
 *
 * \brief  Kick core watch dog.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void wdg_demo(void)
 * {
 *     WDG_Kick_Core();
 * }
 * \endcode
 */
void WDG_Kick_Core(void);

/**
 *
 * \brief  Trigger system reset by core watch dog.
 *
 * \param[in] wdg_mode: The mode of watch dog \ref T_WDG_MODE.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void wdg_demo(void)
 * {
 *     WDG_SystemReset(RESET_ALL);
 * }
 * \endcode
 */
void WDG_SystemReset(T_WDG_MODE wdg_mode);

/**
 *
 * \brief  Trigger system reset with debug by core watch dog.
 *
 * \param[in] wdg_mode: The mode of watch dog \ref T_WDG_MODE.
 * \param[in] dump_size: The size of debug to be dump.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void wdg_demo(void)
 * {
 *     WDG_SystemReset_Dump(RESET_ALL, 100);
 * }
 * \endcode
 */
void WDG_SystemReset_Dump(T_WDG_MODE wdg_mode, uint32_t dump_size);

/** @} */ /* End of group 87x3e_WATCH_DOG_Exported_Functions */
/** @} */ /* End of group 87x3e_WATCH_DOG */

#ifdef __cplusplus
}
#endif

#endif //_RTL876X_WDG_H_
