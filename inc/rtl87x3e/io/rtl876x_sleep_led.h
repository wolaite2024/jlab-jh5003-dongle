/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_sleep_led.h
* @brief
* @details
* @author    howie_wang
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef _RTL876X_SLEEP_LED_H_
#define _RTL876X_SLEEP_LED_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"


/** @addtogroup 87x3e_LED SLEEP LED
  * @brief SLEEP LED driver module.
  * @{
  */

/** @defgroup 87x3e_SLEEP_LED_Exported_Types SLEEP LED Exported Types
  * @{
  */

/**
 * @brief SLEEP LED initialize parameters.
 */
typedef struct
{
    uint8_t mode;               /*!< Specifies the SLEEP LED working mode.
                                     This parameter can be a value of @ref x3e_SLEEP_LED_mode. */

    uint8_t polarity;           /*!< Specifies the SLEEP LED output polarity.
                                     This parameter can be a value of @ref x3e_SLEEP_LED_Output_polarity. */

    uint16_t prescale;          /*!< Specifies the SLEEP LED prescale value.
                                     This parameter can be a value of 0-1023.
                                     period_timetick = prescale / 32000. */

    uint8_t period_high[3];     /*!< Specifies the period high level count value in blink mode.
                                     This parameter can be a value of 0-255.
                                     Period high level time = period_high * period_timetick. */

    uint8_t period_low[3];      /*!< Specifies the period low level count value in blink mode.
                                     This parameter can be a value of 0-255.
                                     Period low level time = period_low * period_timetick. */

    uint8_t phase_uptate_rate[8];  /*!< Specifies the phase duty update rate in breathe mode.
                                        This parameter can be a value of 0-15.
                                        Duty update time = (phase_uptate_rate + 1) * phase_timetick. */

    uint16_t phase_phase_tick[8];    /*!< Specifies the duration of the whole phase in breathe mode.
                                          This parameter can be a value of 0-1023.
                                          Phase output time = phase_phase_tick * phase_timetick. */

    uint16_t phase_initial_duty[8]; /*!< Specifies the initial high level time in breathe mode.
                                         This parameter can be a value of 0-511.
                                         Initial high level time = phase_initial_duty / 32000. */

    uint8_t phase_increase_duty[8]; /*!< Specifies the direction of each change of duty in breathe mode.
                                         This parameter can be a value of 0 or 1.
                                         1: increase duty. 0: decrease duty. */

    uint8_t phase_duty_step[8];   /*!< Specifies duty step in breathe mode.
                                       This parameter can be a value of 0-63.
                                       Duty step time = phase_duty_step / 32000. */

} SLEEP_LED_InitTypeDef;

/** End of group 87x3e_SLEEP_LED_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_SLEEP_LED_Exported_Constants SLEEP LED Exported Constants
  * @{
  */

#define LED_BREATH_PHASE_NUM    8         //!< 8 phases per channel in breathe mode.

/** @defgroup 87x3e_SLEEP_LED_Prescale SLEEP LED Prescale
  * @{
  */
#define LED_TIME_TICK_SCALE     320     //!< SLEEP LED prescale value is 320.

/** End of group 87x3e_SLEEP_LED_Prescale
  * @}
  */

/** @defgroup 87x3e_SLEEP_LED_CHANNEL SLEEP LED Channel
  * @{
  */

typedef enum
{
    LED_CHANNEL_0 = 0x01,     //!< SLEEP LED channel 0.
    LED_CHANNEL_1 = 0x02,     //!< SLEEP LED channel 1.
    LED_CHANNEL_2 = 0x04      //!< SLEEP LED channel 2.
} SLEEP_LED_CHANNEL;
/** End of group 87x3e_SLEEP_LED_CHANNEL
  * @}
  */

/** @defgroup 87x3e_SLEEP_LED_mode SLEEP LED Working Mode
  * @{
  */

#define LED_BLINK_MODE              ((uint32_t)0x00000000)    //!< Blink mode.
#define LED_BREATHE_MODE            ((uint32_t)0x00000001)    //!< Breathe mode.
#define IS_LED_MODE(MODE)           (((MODE) == LED_BLINK_MODE) || \
                                     ((MODE) == LED_BREATHE_MODE)) //!< Check whether is the SLEEP LED mode.

/** End of group 87x3e_SLEEP_LED_mode
  * @}
  */

/** @defgroup 87x3e_SLEEP_LED_Output_polarity SLEEP LED Output Polarity
  * @{
  */

#define LED_OUTPUT_NORMAL           ((uint32_t)0x00000000)    //!< Non-inverting output.
#define LED_OUTPUT_INVERT           ((uint32_t)0x00000001)    //!< Inverting output.
#define IS_LED_OUTPUT_MODE(MODE)    (((MODE) == LED_OUTPUT_NORMAL) || \
                                     ((MODE) == LED_OUTPUT_INVERT)) //!< Check whether is the SLEEP LED output polarity.
/** End of group 87x3e_SLEEP_LED_Output_polarity
  * @}
  */

/** @defgroup 87x3e_SLEEP_LED_Idle_State  SLEEP LED Idle State
  * @{
  */

#define LED_IDLE_HIGH               ((uint8_t)0x01)     //!< SLEEP LED output is high in idle state.
#define LED_IDLE_LOW                ((uint8_t)0x00)     //!< SLEEP LED output is low in idle state.
#define IS_LED_IDLE_MODE(MODE)    (((MODE) == LED_IDLE_HIGH) || \
                                   ((MODE) == LED_IDLE_LOW)) //!< Check whether is the SLEEP LED idle state.
/** End of group 87x3e_SLEEP_LED_Idle_State
  * @}
  */

/** End of group 87x3e_SLEEP_LED_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_SLEEP_LED_Exported_Functions SLEEP LED Exported Functions
  * @{
  */

/**
 *
 * \brief   Restore all the SLEEP LED registers to their default values.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_led_init(void)
 * {
 *     SleepLed_Reset();
 * }
 * \endcode
 */
void SleepLed_Reset(void);


/**
 *
 * \brief   Restore the registers of the specified SLEEP LED channel to their default values.
 *
 * \param[in] channel: Select the SLEEP LED channel, refer to \ref x3e_SLEEP_LED_CHANNEL.
 *            - LED_CHANNEL_0: SLEEP LED channel 0.
 *            - LED_CHANNEL_1: SLEEP LED channel 1.
 *            - LED_CHANNEL_2: SLEEP LED channel 2.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void led_close(void)
 * {
 *     SleepLed_Cmd(LED_CHANNEL_0 | LED_CHANNEL_1 | LED_CHANNEL_2, DISABLE);
 *
 *     SleepLed_DeInit(LED_CHANNEL_0);
 *     SleepLed_DeInit(LED_CHANNEL_1);
 *     SleepLed_DeInit(LED_CHANNEL_2);
 * }
 * \endcode
 */
void SleepLed_DeInit(SLEEP_LED_CHANNEL channel);


/**
 *
 * \brief  Initializes the specified SLEEP LED channel according to the specified
 *         parameters in the LED_InitStruct.
 *
 * \param[in] channel: Select the SLEEP LED channel, refer to \ref x3e_SLEEP_LED_CHANNEL.
 *            - LED_CHANNEL_0: SLEEP LED channel 0.
 *            - LED_CHANNEL_1: SLEEP LED channel 1.
 *            - LED_CHANNEL_2: SLEEP LED channel 2.
 * \param[in] LED_InitStruct: Pointer to a SLEEP_LED_InitTypeDef structure that
 *            contains the configuration information for the specified SLEEP LED channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_led_init_blink(void)
 * {
 *     SleepLed_Reset();
 *     SLEEP_LED_InitTypeDef Led_Initsturcture;
 *     SleepLed_StructInit(&Led_Initsturcture);
 *     Led_Initsturcture.mode                        = LED_BLINK_MODE;
 *     Led_Initsturcture.polarity                    = LED_OUTPUT_NORMAL;
 *     Led_Initsturcture.prescale                    = 32;
 *     Led_Initsturcture.period_high[0]                = 200;
 *     Led_Initsturcture.period_low[0]                  = 100;
 *     SleepLed_Init(LED_CHANNEL_0, &Led_Initsturcture);
 *     SleepLed_Cmd(LED_CHANNEL_0, ENABLE);
 * }
 * \endcode
 */
void SleepLed_Init(SLEEP_LED_CHANNEL channel, SLEEP_LED_InitTypeDef *LED_InitStruct);


/**
 *
 * \brief   Fill each LED_InitStruct member with its default value.
 *
 * \note   The default settings for the LED_InitStruct member are shown in the following table:
 *         | LED_InitStruct Member  | Default Value            |
 *         |:----------------------:|:------------------------:|
 *         | mode                   | \ref LED_BREATHE_MODE    |
 *         | polarity               | \ref LED_OUTPUT_NORMAL   |
 *         | prescale               | \ref LED_TIME_TICK_SCALE |
 *         | period_high[3]         | 0                        |
 *         | period_low[3]          | 0                        |
 *         | phase_update_rate[8]   | 0                        |
 *         | phase_phase_tick[8]    | 0                        |
 *         | phase_initial_duty[8]  | 0                        |
 *         | phase_increase_duty[8] | 0                        |
 *         | phase_duty_step[8]     | 0                        |
 *
 * \param[in] LED_InitStruct: Pointer to a SLEEP_LED_InitTypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_led_init_breathe(void)
 * {
 *     SleepLed_Reset();
 *     SLEEP_LED_InitTypeDef Led_Initsturcture;
 *     SleepLed_StructInit(&Led_Initsturcture);
 *     Led_Initsturcture.mode                        = LED_BREATHE_MODE;
 *     Led_Initsturcture.polarity                    = LED_OUTPUT_NORMAL;
 *     Led_Initsturcture.prescale                    = 320;
 *     Led_Initsturcture.phase_uptate_rate[0]        = 0;
 *     Led_Initsturcture.phase_phase_tick[0]         = 50;
 *     Led_Initsturcture.phase_initial_duty[0]       = 0;
 *     Led_Initsturcture.phase_increase_duty[0]      = 1;
 *     Led_Initsturcture.phase_duty_step[0]          = 1;
 *     SleepLed_Init(LED_CHANNEL_0, &Led_Initsturcture);
 * }
 * \endcode
 */
void SleepLed_StructInit(SLEEP_LED_InitTypeDef *LED_InitStruct);


/**
 *
 * \brief   Enable or disable the specified LED channel.
 *
 * \param[in] channel: Select the SLEEP LED channel, refer to \ref x3e_SLEEP_LED_CHANNEL.
 *            - LED_CHANNEL_0: SLEEP LED channel 0.
 *            - LED_CHANNEL_1: SLEEP LED channel 1.
 *            - LED_CHANNEL_2: SLEEP LED channel 2.
 * \param[in] NewState: New state of the specified SLEEP LED channel.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified SLEEP LED channel.
 *            - DISABLE: Disable the specified SLEEP LED channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_led_init(void)
 * {
 *     SleepLed_Cmd(LED_CHANNEL_0, ENABLE);
 * }
 * \endcode
 */
void SleepLed_Cmd(uint8_t channel, FunctionalState NewState);


/**
 *
 * \brief   Set idle state of the specified SLEEP LED channel.
 *
  * \param[in] channel: Select the SLEEP LED channel, refer to \ref x3e_SLEEP_LED_CHANNEL.
 *            - LED_CHANNEL_0: SLEEP LED channel 0.
 *            - LED_CHANNEL_1: SLEEP LED channel 1.
 *            - LED_CHANNEL_2: SLEEP LED channel 2.
 * \param[in] State: Idle state of the specified SLEEP LED channel, refer to \ref x3e_SLEEP_LED_Idle_State.
 *            - LED_IDLE_HIGH: SLEEP LED output is high in idle state.
 *            - LED_IDLE_LOW: SLEEP LED output is low in idle state.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_led_init(void)
 * {
 *     SleepLed_SetIdleMode(LED_CHANNEL_0, LED_IDLE_LOW);
 * }
 * \endcode
 */
void SleepLed_SetIdleMode(SLEEP_LED_CHANNEL channel, uint8_t State);


/**
 *
 * \brief   Write data to SLEEP LED registers.
 *
 * \param[in] offset: The offset of SLEEP LED register.
 * \param[in] data: The data to be written.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void SleepLed_Reset(void)
 * {
 *     SleepLed_RegisterWrite(0x00, 0x0);
 * }
 * \endcode
 */
void SleepLed_RegisterWrite(uint32_t offset, uint32_t data);


/**
 *
 * \brief   Read SLEEP LED register.
 *
 * \param[in] addr: The address of SLEEP LED register.
 *
 * \return  The value read from the register.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void SleepLed_Cmd(void)
 * {
 *     uint32_t ctrl_value;
 *     uint32_t led_control_register;
 *     led_control_register = (uint32_t)(&(RTC_LED->S_LED_CR));
 *     ctrl_value = SleepLed_RegisterRead(led_control_register);
 * }
 * \endcode
 */
uint32_t SleepLed_RegisterRead(uint32_t addr);


/** @} */ /* End of group 87x3e_SLEEP_LED_Exported_Functions */
/** @} */ /* End of group 87x3e_LED */

#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_SLEEP_LED_H_ */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/

