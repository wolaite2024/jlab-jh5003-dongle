/**
************************************************************************************************************
*            Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file      rtl876x_captouch.h
* @brief     CapTouch implementation header file.
* @author    js.lin
* @date      2024-07-18
* @version   v1.0
*************************************************************************************************************
*/
#ifndef __RTL876X_CAP_TOUCH__
#define __RTL876X_CAP_TOUCH__
#include <stdint.h>
#include <stdbool.h>
#include "rtl876x.h"

/** @addtogroup 87x3e_CAPTOUCH CapTouch
  * @brief CapTouch driver module.
  * @{
  */

/** @defgroup 87x3e_CapTouch_Exported_Types CapTouch Exported Types
  * @{
  */


/** @defgroup 87x3e_CapTouch_CHANNEL CapTouch Channel
  * @{
  */

typedef enum
{
    CTC_CH0 = 0,    //!< CapTouch channel 0.
    CTC_CH1 = 1,    //!< CapTouch channel 1.
    CTC_CH2 = 2,    //!< CapTouch channel 2.
    CTC_CH3 = 3     //!< CapTouch channel 3.
} CTC_CH_TYPE;

/** End of group 87x3e_CapTouch_CHANNEL
  * @}
  */

/** @defgroup 87x3e_CapTouch_Interrupts_Definition CapTouch Interrupts Definition
  * @{
  */

typedef enum
{
    CTC_TOUCH_PRESS_INT     = (1UL << 0),   //!< CapTouch press interrupt.
    CTC_TOUCH_RELEASE_INT   = (1UL << 8),   //!< CapTouch release interrupt.
    CTC_FALSE_TOUCH_INT     = (1UL << 19)   //!< CapTouch false touch interrupt.
} CTC_CH_INT_TYPE;

/** End of Group 87x3e_CapTouch_Interrupts_Definition
  * @}
  */

/** @defgroup 87x3e_CapTouch_Noise_Interrupts_Definition CapTouch Noise Interrupts Definition
  * @{
  */

typedef enum
{
    CTC_OVER_P_NOISE_INT    = (1UL << 16),    //!< CapTouch positive noise interrupt.
    CTC_OVER_N_NOISE_INT    = (1UL << 18)     //!< CapTouch negative noise interrupt.
} CTC_NOISE_INT_TYPE;

/** End of Group 87x3e_CapTouch_Noise_Interrupts_Definition
  * @}
  */

/** @defgroup 87x3e_CapTouch_Mode CapTouch Mode
  * @{
  */

typedef enum
{
    CTC_SLOW_MODE = 0,      //!< CapTouch slow mode.
    CTC_FAST_MODE = 1       //!< CapTouch fast mode
} CTC_MODE_TYPE;

/** End of Group 87x3e_CapTouch_Mode
  * @}
  */

/** End of group 87x3e_CapTouch_Exported_Types
  * @}
  */

/** @defgroup 87x3e_CapTouch_Exported_Functions CapTouch Exported Functions
  * @{
  */

/**
 *
 * \brief  Reset CapTouch system and clear all setting.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_SystemEnable(true);
 *     CapTouch_SysReset();
 * }
 * \endcode
 */
void CapTouch_SysReset(void);

/**
 *
 * \brief     Enables or disables the specified CapTouch interrupts of specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] INT_to_config: Specifies the CapTouch interrupt sources to be enabled or disabled, refer to \ref x3e_CapTouch_Interrupts_Definition.
 *            This parameter can be one or combinations of the following values:
 *            - CTC_TOUCH_PRESS_INT: CapTouch press interrupt.
 *            - CTC_TOUCH_RELEASE_INT: CapTouch release interrupt.
 *            - CTC_FALSE_TOUCH_INT: CapTouch false touch interrupt.
 * \param[in] new_state: New state of the specified CapTouch interrupts.
 *            This parameter can be one of the following values:
 *             - ENABLE: Enable the specified CapTouch interrupts.
 *             - DISABLE: Disable the specified CapTouch interrupts.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     RamVectorTableUpdate(TOUCH_VECTORn, (IRQ_Fun)captouch_handler);
 *     CapTouch_ChCmd(CTC_CH0, ENABLE);
 *     CapTouch_ChINTConfig(CTC_CH0, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT), ENABLE);
 * }
 * \endcode
 */
void CapTouch_ChINTConfig(CTC_CH_TYPE channel, CTC_CH_INT_TYPE INT_to_config,
                          FunctionalState new_state);

/**
 *
 * \brief     Enables or disables the specified CapTouch noise interrupts.
 *
 * \param[in] INT_to_config: Specifies the CapTouch interrupt sources to be enabled or disabled, refer to \ref x3e_CapTouch_Noise_Interrupts_Definition.
 *         This parameter can be one or combinations of the following values:
 *            - CTC_OVER_P_NOISE_INT: CapTouch positive noise interrupt.
 *            - CTC_OVER_N_NOISE_INT: CapTouch negative noise interrupt.
 * \param[in] new_state: New state of the specified CapTouch noise interrupts.
 *            This parameter can be one of the following values:
 *             - ENABLE: Enable the specified CapTouch noise interrupts.
 *             - DISABLE: Disable the specified CapTouch noise interrupts.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     RamVectorTableUpdate(TOUCH_VECTORn, (IRQ_Fun)captouch_handler);
 *     CapTouch_NoiseINTConfig(CTC_OVER_P_NOISE_INT, ENABLE);
 * }
 * \endcode
 */
void CapTouch_NoiseINTConfig(CTC_NOISE_INT_TYPE INT_to_config, FunctionalState new_state);

/**
 *
 * \brief     Check whether the specific interrupt of specific channel is triggered.
 *
 * \param[in] int_status: The interrupt status.
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] INT_source: Specifies the CapTouch interrupts to be check, refer to \ref x3e_CapTouch_Interrupts_Definition.
 *            This parameter can be one or combinations of the following values:
 *            - CTC_TOUCH_PRESS_INT: CapTouch press interrupt.
 *            - CTC_TOUCH_RELEASE_INT: CapTouch release interrupt.
 *            - CTC_FALSE_TOUCH_INT: CapTouch false touch interrupt.
 *
 * \return The specific interrupt of specific channel is triggered or not.
 * \retval true: The specific interrupt of specific channel is triggered.
 * \retval false: The specific interrupt of specific channel is not triggered.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void captouch_handler(void)
 * {
 *     uint32_t int_status = 0;
 *     int_status = CapTouch_GetINTStatus();
 *     if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_TOUCH_PRESS_INT))
 *     {
 *         CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_TOUCH_PRESS_INT);
 *     }
 * }
 * \endcode
 */
bool CapTouch_IsChINTTriggered(uint32_t int_status, CTC_CH_TYPE channel,
                               CTC_CH_INT_TYPE INT_source);

/**
 *
 * \brief     Check whether the specific noise interrupt is triggered.
 *
 * \param[in] int_status: The interrupt status.
 * \param[in] INT_source: Specifies the CapTouch interrupts to be check, refer to \ref x3e_CapTouch_Noise_Interrupts_Definition.
 *            This parameter can be one or combinations of the following values:
 *            - CTC_OVER_P_NOISE_INT: CapTouch positive noise interrupt.
 *            - CTC_OVER_N_NOISE_INT: CapTouch negative noise interrupt.
 *
 * \return The specific noise interrupt is triggered or not.
 * \retval true: The specific noise interrupt is triggered.
 * \retval false: The specific noise interrupt is not triggered.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void captouch_handler(void)
 * {
 *     uint32_t int_status = 0;
 *     int_status = CapTouch_GetINTStatus();
 *     if (CapTouch_IsNoiseINTTriggered(int_status, CTC_OVER_N_NOISE_INT))
 *     {
 *         CapTouch_NoiseINTClearPendingBit(CTC_OVER_N_NOISE_INT);
 *     }
 * }
 * \endcode
 */
bool CapTouch_IsNoiseINTTriggered(uint32_t int_status, CTC_NOISE_INT_TYPE INT_source);

/**
 *
 * \brief     Clears the specified CapTouch interrupt pending bits of specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] INT_to_clear: Specifies the CapTouch interrupts to be cleared, refer to \ref x3e_CapTouch_Interrupts_Definition.
 *            This parameter can be one or combinations of the following values:
 *            - CTC_TOUCH_PRESS_INT: CapTouch press interrupt.
 *            - CTC_TOUCH_RELEASE_INT: CapTouch release interrupt.
 *            - CTC_FALSE_TOUCH_INT: CapTouch false touch interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void captouch_handler(void)
 * {
 *     if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_TOUCH_PRESS_INT))
 *     {
 *         CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_TOUCH_PRESS_INT);
 *     }
 * }
 * \endcode
 */
void CapTouch_ChINTClearPendingBit(CTC_CH_TYPE channel, CTC_CH_INT_TYPE INT_to_clear);

/**
 *
 * \brief     Clears the specified CapTouch noise interrupt pending bits.
 *
 * \param[in] INT_to_clear: Specifies the CapTouch interrupts to be cleared, refer to \ref x3e_CapTouch_Noise_Interrupts_Definition.
 *            This parameter can be one or combinations of the following values:
 *            - CTC_OVER_P_NOISE_INT: CapTouch positive noise interrupt.
 *            - CTC_OVER_N_NOISE_INT: CapTouch negative noise interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void captouch_handler(void)
 * {
 *     if (CapTouch_IsNoiseINTTriggered(int_status, CTC_OVER_N_NOISE_INT))
 *     {
 *         CapTouch_NoiseINTClearPendingBit(CTC_OVER_N_NOISE_INT);
 *     }
 * }
 * \endcode
 */
void CapTouch_NoiseINTClearPendingBit(CTC_NOISE_INT_TYPE INT_to_clear);

/**
 *
 * \brief     Clears all CapTouch interrupt pending bits.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void captouch_handler(void)
 * {
 *     CapTouch_INTClearAllPendingBit();
 * }
 * \endcode
 */
void CapTouch_INTClearAllPendingBit(void);

/**
 *
 * \brief     Enables or disables the specified CapTouch peripheral.
 *
 * \param[in] new_state: New state of the CapTouch peripheral.
 *            This parameter can be one of the following values:
 *             - ENABLE: Enable the specified CapTouch peripheral.
 *             - DISABLE: Disable the specified CapTouch peripheral.
 * \param[in] baseline_init: Enable or disable baseline initial function.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable baseline initial function.
 *            - DISABLE: Disable baseline initial function.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_Cmd(ENABLE, ENABLE);
 * }
 * \endcode
 */
void CapTouch_Cmd(FunctionalState new_state, FunctionalState baseline_init);

/**
 *
 * \brief     Enables or disables the specified channel of CapTouch.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] new_state: New state of the specified channel.
 *            This parameter can be one of the following values:
 *             - ENABLE: Enable the specified channel.
 *             - DISABLE: Disable the specified channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_ChCmd(CTC_CH0, ENABLE);
 *     CapTouch_ChCmd(CTC_CH1, ENABLE);
 * }
 * \endcode
 */
void CapTouch_ChCmd(CTC_CH_TYPE channel, FunctionalState new_state);

/**
 *
 * \brief     Enable or disable specified channel wakeup function.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] new_state: New state of the specified channel wakeup function.
 *            This parameter can be one of the following values:
 *             - ENABLE: Enable the specified channel wakeup function.
 *             - DISABLE: Disable the specified channel wakeup function.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_ChWakeupCmd(CTC_CH0, (FunctionalState)ENABLE);
 *     CapTouch_ChWakeupCmd(CTC_CH1, (FunctionalState)ENABLE);
 * }
 * \endcode
 */
void CapTouch_ChWakeupCmd(CTC_CH_TYPE channel, FunctionalState new_state);

/**
 *
 * \brief     Set CapTouch scan interval for slow mode or fast mode.
 *
 * \param[in] interval: Scan interval in units of 1s/1.024KHz (= 0.9765625 ms).
 *            Configurable range: 0x0~0xFFF (0~4095).
 *            Note that slow mode interval should not be lower then fast mode interval,
 *            fast mode interval should be greater then total guard time for AUXADC.
 * \param[in] mode: CapTouch mode, refer to \ref x3e_CapTouch_Mode.
 *            This parameter can be one of the following values:
 *            - CTC_SLOW_MODE: Set CapTouch scan interval for slow mode.
 *            - CTC_FAST_MODE: Set CapTouch scan interval for fast mode.
 *
 * \return Set CapTouch Scan interval is success or fail.
 * \retval true: Set CapTouch scan interval successfully.
 * \retval false: Set CapTouch scan interval failed. Scan interval is out of range.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *    if (!CapTouch_SetScanInterval(0x3B, CTC_SLOW_MODE))
 *    {
 *       IO_PRINT_WARN0("cap_touch_demo: Slow mode scan interval overange");
 *    }
 * }
 * \endcode
 */
bool CapTouch_SetScanInterval(uint16_t interval, CTC_MODE_TYPE mode);

/**
 *
 * \brief     Set baseline to specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] baseline: The baseline value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_SetChBaseline(CTC_CH0, baseline);
 * }
 * \endcode
 */
void CapTouch_SetChBaseline(CTC_CH_TYPE channel, uint16_t baseline);

/**
 *
 * \brief     Set relative touch threshold for related channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] threshold: The related threshold value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_SetChDiffThres(CTC_CH0, threshold);
 * }
 * \endcode
 */
void CapTouch_SetChDiffThres(CTC_CH_TYPE channel, uint16_t threshold);

/**
 *
 * \brief     Set P noise threshold for related channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] p_noise_threshold: P noise threshold value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_SetChPNoiseThres(CTC_CH0, p_noise_threshold);
 * }
 * \endcode
 */
void CapTouch_SetChPNoiseThres(CTC_CH_TYPE channel, uint8_t p_noise_threshold);

/**
 *
 * \brief     Set N noise threshold for related channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] n_noise_threshold: N noise Threshold value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_SetChNNoiseThres(CTC_CH2, n_noise_threshold);
 * }
 * \endcode
 */
void CapTouch_SetChNNoiseThres(CTC_CH_TYPE channel, uint8_t n_noise_threshold);

/**
 *
 * \brief     Set Mbias current for specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 * \param[in] mbias: Mbias value, relate current = 0.25*mbias.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_SetChMbias(CTC_CH3, mbias);
 * }
 * \endcode
 */
void CapTouch_SetChMbias(CTC_CH_TYPE channel, uint8_t mbias);

/**
 *
 * \brief     Get CapTouch interrupt status.
 *
 * \return    The interrupt status.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void captouch_handler(void)
 * {
 *     uint32_t int_status = 0;
 *     int_status = CapTouch_GetINTStatus();
 *     if (CapTouch_IsNoiseINTTriggered(int_status, CTC_OVER_N_NOISE_INT))
 *     {
 *         CapTouch_NoiseINTClearPendingBit(CTC_OVER_N_NOISE_INT);
 *     }
 * }
 * \endcode
 */
uint32_t CapTouch_GetINTStatus(void);

/**
 *
 * \brief     Get status from specified CapTouch channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 *
 * \return The channel enabled status.
 * \retval true: The specified  CapTouch channel is enabled.
 * \retval false: The specified CapTouch channel is disabled.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void captouch_handler(void)
 * {
 *     bool status = 0;
 *     status = CapTouch_GetChStatus(CTC_CH3);
 * }
 * \endcode
 */
bool CapTouch_GetChStatus(CTC_CH_TYPE channel);

/**
 *
 * \brief     Read baseline data from specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 *
 * \return    The baseline value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint16_t baseline = CapTouch_GetChBaseline(CTC_CH0);
 * }
 * \endcode
 */
uint16_t CapTouch_GetChBaseline(CTC_CH_TYPE channel);

/**
 *
 * \brief     Read average data from the specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 *
 * \return    The average data from the specified channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint16_t data = CapTouch_GetChAveData(CTC_CH0);
 * }
 * \endcode
 */
uint16_t CapTouch_GetChAveData(CTC_CH_TYPE channel);

/**
 *
 * \brief     Get the difference threshold of specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 *
 * \return    The difference threshold value from the specified channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint16_t threshold = CapTouch_GetChDiffThres(CTC_CH0);
 * }
 * \endcode
 */
uint16_t CapTouch_GetChDiffThres(CTC_CH_TYPE channel);

/**
 *
 * \brief     Get Mbias current setting from specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 *
 * \return    The mbias value, unit 0.25uA.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint8_t curr_mbias = CapTouch_GetChMbias(CTC_CH2);
 * }
 * \endcode
 */
uint8_t CapTouch_GetChMbias(CTC_CH_TYPE channel);

/**
 *
 * \brief     Read touch active count from specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 *
 * \return    The touch active count.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint8_t touch_cnt = CapTouch_GetChTouchCnt(CTC_CH2);
 * }
 * \endcode
 */
uint16_t CapTouch_GetChTouchCnt(CTC_CH_TYPE channel);

/**
 *
 * \brief     Read CapTouch status under fast mode.
 *
 * \return    The CapTouch status of all channel.
 *            bit4~bit0 each bit represent one channel, 0: un-tounch, 1: tounch.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     IO_PRINT_INFO2("CapTouch_status: fs %d, TouchStatus %d", CapTouch_IsFastMode(), CapTouch_GetChTouchStatus());
 * }
 * \endcode
 */
uint8_t CapTouch_GetChTouchStatus(void);

/**
 *
 * \brief     Get number of enabled channels.
 *
 * \return    The number of enabled channels.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint8_t number = CapTouch_GetNumOfEnabledChs();
 * }
 * \endcode
 */
uint8_t CapTouch_GetNumOfEnabledChs(void);

/**
 *
 * \brief     Read CapTouch mode status.
 *
 * \return The CapTouch mode status.
 * \retval true: CapTouch mode is in fast mode.
 * \retval false: CapTouch mode is in slow mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint8_t mode = CapTouch_IsFastMode();
 *     IO_PRINT_INFO1("CapTouch_status: fast mode = %d", mode);
 * }
 * \endcode
 */
bool CapTouch_IsFastMode(void);

/**
 *
 * \brief     Check legality of specified channel.
 *
 * \param[in] channel: Select the CapTouch channel, refer to \ref x3e_CapTouch_CHANNEL.
 *            This parameter can be one of the following values:
 *            - CTC_CH0: CapTouch channel 0.
 *            - CTC_CH1: CapTouch channel 1.
 *            - CTC_CH2: CapTouch channel 2.
 *            - CTC_CH3: CapTouch channel 3.
 *
 * \return The legality of specified channel.
 * \retval true: The specified CapTouch channel is allowed.
 * \retval false: The specified CapTouch channel is not allowed.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint8_t result = CapTouch_IsChAllowed(CTC_CH2);
 * }
 * \endcode
 */
bool CapTouch_IsChAllowed(CTC_CH_TYPE channel);

/**
 *
 * \brief     Read CapTouch enabled status.
 *
 * \return The CapTouch enabled status.
 * \retval true: The CapTouch is running.
 * \retval false: The CapTouch is not running.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CTC_PRINT_INFO1("app_cap_touch_enable: IsRunning %d", CapTouch_IsRunning());
 * }
 * \endcode
 */
bool CapTouch_IsRunning(void);

/**
 *
 * \brief     Read system enabled status.
 *
 * \return The system is enabled or not.
 * \retval true: The system is enabled.
 * \retval false: The system is disabled.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint8_t result = CapTouch_IsSystemEnable();
 * }
 * \endcode
 */
bool CapTouch_IsSystemEnable(void);

/**
 *
 * \brief     Enable or disable the system.
 *
 * \param[in] status: Enable or disable the system.
 *            - true: Enable the system.
 *            - false: Disable the system.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_SystemEnable(true);
 *     CapTouch_SysReset();
 * }
 * \endcode
 */
void CapTouch_SystemEnable(bool status);

/**
 *
 * \brief     Get the max channel of CapTouch.
 *
 * \return The max channel of CapTouch.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     CapTouch_SystemEnable(true);
 *     CapTouch_SysReset();
 *     IO_PRINT_INFO2("cap_touch_demo: Start, max_channel %d, cap_touch_enable %d", CapTouch_GetMaxChannel(), CapTouch_IsSystemEnable());
 * }
 * \endcode
 */
uint8_t CapTouch_GetMaxChannel(void);

/**
 *
 * \brief     Get 32k clock power domain. CapTouch wakeup from powerdown 32K clock power domain should be AON_DOMAIN or ALWAYS_ACTIVE.
 *
 * \return The power domain of 32k clock.
 * \retval 0: 32k clock power domain is ALWAYS_ACTIVE.
 * \retval 1: 32k clock power domain is AON_DOMAIN.
 * \retval 2: 32k clock power domain is PON_DOMAIN.
 * \retval 4: 32k clock power domain is CORE_DOMAIN.
 * \retval 6: 32k clock power domain is ALWAYS_INACTIVE.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void cap_touch_demo(void)
 * {
 *     uint32_t 32k_power_domain = Captouch_Get32KClkPowerDomain();
 * }
 * \endcode
 */
uint32_t Captouch_Get32KClkPowerDomain(void);

/** @} */ /* End of group 87x3e_CapTouch_Exported_Functions */
/** @} */ /* End of group 87x3e_CAPTOUCH */
#endif
