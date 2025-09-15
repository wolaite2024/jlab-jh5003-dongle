/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_tim.h
* @brief
* @details
* @author    elliot chen
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef __RTL876X_TIM_H
#define __RTL876X_TIM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"
#include <stdbool.h>

#define TIMER0_LOAD_COUNT2 *((volatile uint32_t *)0x400250b0UL)
#define TIMER1_LOAD_COUNT2 *((volatile uint32_t *)0x400250b4UL)

#define TIMER2_LOAD_COUNT2 *((volatile uint32_t *)0x400250b8UL)
#define TIMER3_LOAD_COUNT2 *((volatile uint32_t *)0x400250bCUL)
#define TIMER4_LOAD_COUNT2 *((volatile uint32_t *)0x400250C0UL)
#define TIMER5_LOAD_COUNT2 *((volatile uint32_t *)0x400250c4UL)
#define TIMER6_LOAD_COUNT2 *((volatile uint32_t *)0x400250c8UL)
#define TIMER7_LOAD_COUNT2 *((volatile uint32_t *)0x400250ccUL)

/* TIM Private Defines */
#define TIMER_PWM2_CR *((volatile uint32_t *)0x40000364UL)
#define TIMER_PWM3_CR *((volatile uint32_t *)0x40000368UL)

/** @addtogroup 87x3e_TIM TIM
  * @brief TIM driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup 87x3e_TIM_Exported_Types TIM Exported Types
  * @{
  */

/**
  * @brief  TIM Init structure definition.
  */
typedef struct
{
    uint16_t TIM_ClockSrc;          /*!< This is a deprecated parameter. */

    uint16_t TIM_DIV;               /*!< This is a deprecated parameter. */

    uint16_t TIM_SOURCE_DIV;        /*!< Specifies the clock source divider.
                                            This parameter can be a value of @ref x3e_TIM_Clock_Divider. */

    uint16_t TIM_SOURCE_DIV_En;     /*!< This is a deprecated parameter. */

    uint16_t TIM_Mode;              /*!< Specifies the counter mode.
                                        This parameter can be a value of @ref x3e_TIM_Mode. */

    uint16_t TIM_PWM_En;            /*!< Enable or disable the PWM mode.
                                        This parameter can be a value of @ref x3e_TIM_PWM_En. */

    uint32_t TIM_Period;            /*!< Specifies the period value to be loaded into the active
                                         auto-reload register at the next update event.
                                        This parameter must range from 0x0000 to 0xFFFFFFFF.  */

    uint32_t TIM_PWM_High_Count;    /*!< Specify the PWM high count.
                                         This parameter must range from 0x0 to 0xFFFFFFFF.
                                         PWM high period = (TIM_PWM_High_Count + 1) * TIM clock period.*/

    uint32_t TIM_PWM_Low_Count;     /*!< Specify the PWM low count.
                                         This parameter must range from 0x0 to 0xFFFFFFFF.
                                         PWM low period = (TIM_PWM_Low_Count + 1) * TIM clock period. */

    uint32_t TIM_EventMode;         /*!< This is a deprecated parameter. */

    uint32_t TIM_EventIndex;        /*!< This is a deprecated parameter. */

    uint32_t TIM_EventDuration;     /*!< This is a deprecated parameter. */

    uint8_t  ClockDepend;           /*!< Specifies TIM Source depend.
                                        Timer3 depend timer2, timer5 depend timer4, timer7 depend timer6.
                                        This parameter can be a value of true or false. */

    uint32_t PWM_Deazone_Size;      /*!< Specify the PWM deadzone size.
                                         This parameter must range from 0x1 to 0xff.
                                         The calculation formula for deadzone time is as follows:
                                         Deadzone time = (PWM_Deazone_Size) / deadzone clock. */

    uint16_t PWMDeadZone_En;        /*!< Enable or disable the PWM complementary output and deadzone.
                                         This parameter can be a value of ENABLE or DISABLE. */

    uint16_t PWM_Stop_State_P;      /*!< Specifies the PWM P stop state.
                                        This parameter can be a value of @ref x3e_PWMDeadZone_Stop_state. */

    uint16_t PWM_Stop_State_N;      /*!< Specifies the PWM N stop state.
                                        This parameter can be a value of @ref x3e_PWMDeadZone_Stop_state. */
} TIM_TimeBaseInitTypeDef;

/** End of group 87x3e_TIM_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_TIM_Exported_Constants TIM Exported Constants
  * @{
  */

#define IS_TIM_ALL_PERIPH(PERIPH) (((PERIPH) == TIM0) || \
                                   ((PERIPH) == TIM1) || \
                                   ((PERIPH) == TIM2) || \
                                   ((PERIPH) == TIM3) || \
                                   ((PERIPH) == TIM4) || \
                                   ((PERIPH) == TIM5) || \
                                   ((PERIPH) == TIM6) || \
                                   ((PERIPH) == TIM7))    //!< TIM peripherals can select TIM0 to TIM7.


/** @defgroup 87x3e_TIM_Clock_Divider TIM Clock Divider
  * @{
  */


#define TIM_CLOCK_DIVIDER_1                           ((uint16_t)0x0)   //!< The clock divider is 1.
#define TIM_CLOCK_DIVIDER_2                           ((uint16_t)0x4)   //!< The clock divider is 2.
#define TIM_CLOCK_DIVIDER_4                           ((uint16_t)0x5)   //!< The clock divider is 4.
#define TIM_CLOCK_DIVIDER_8                           ((uint16_t)0x6)   //!< The clock divider is 8.
#define TIM_CLOCK_DIVIDER_40                          ((uint16_t)0x7)   //!< The clock divider is 40.
#define IS_TIM_SOURCE_DIVIDER(DIV)              (((DIV) == TIM_CLOCK_DIVIDER_1) || \
                                                 ((DIV) == TIM_CLOCK_DIVIDER_2) || \
                                                 ((DIV) == TIM_CLOCK_DIVIDER_4) || \
                                                 ((DIV) == TIM_CLOCK_DIVIDER_8) ||\
                                                 ((DIV) == TIM_CLOCK_DIVIDER_40)) //!< Check whether is the TIM clock divider.

/** End of group 87x3e_TIM_Clock_Divider
  * @}
  */

/** @defgroup 87x3e_TIM_Mode TIM Mode
  * @{
  */

#define TIM_Mode_FreeRun                    ((uint16_t)0x0000)    //!< Select the TIM mode as free-running mode. In free-running mode, the TIM counts down from 0xFFFFFFFF.
#define TIM_Mode_UserDefine                 ((uint16_t)0x0001)    //!< Select the TIM mode as user-defined mode. In user-defined mode, the TIM counts down from the value set by TIM_Period in \ref TIM_TimeBaseInitTypeDef.
#define IS_TIM_MODE(mode) (((mode) == TIM_Mode_FreeRun) || \
                           ((mode) == TIM_Mode_UserDefine))  //!< Check whether is the TIM mode.
/** End of group 87x3e_TIM_Mode
  * @}
  */

/** @defgroup 87x3e_TIM_PWM_En PWM Mode Enable
  * @{
  */

#define PWM_ENABLE                        ((uint16_t)0x1)   //!< Enable the PWM mode.
#define PWM_DISABLE                       ((uint16_t)0x0)   //!< Disable the PWM mode.
#define IS_TIM_PWM_En(mode) (((mode) == PWM_ENABLE) || \
                             ((mode) == PWM_DISABLE))  //!< Check whether is the status of PWM mode.
/** End of group 87x3e_TIM_PWM_En
  * @}
  */

/** @defgroup 87x3e_TIM_Event_Duration TIM Event Duration
  * @{
  */

#define TIM_EventDuration_32us                    ((uint16_t)0x0000)    //!< The TIM event duration is 32us.
#define TIM_EventDuration_64us                    ((uint16_t)0x0001)    //!< The TIM event duration is 64us.
#define TIM_EventDuration_128us                   ((uint16_t)0x0002)    //!< The TIM event duration is 128us.
#define TIM_EventDuration_256us                   ((uint16_t)0x0003)    //!< The TIM event duration is 256us.

#define IS_TIM_Event_DURATION(duration) (((duration) == TIM_EventDuration_32us) || \
                                         ((duration) == TIM_EventDuration_64us) || \
                                         ((duration) == TIM_EventDuration_128us) || \
                                         ((duration) == TIM_EventDuration_256us))  //!< Check whether is the TIM event duration.
/** End of group 87x3e_TIM_Event_Duration
  * @}
  */

/** @defgroup 87x3e_PWMDeadZone_En PWM Dead Zone Enable
* @{
*/

#define DEADZONE_ENABLE                        ((uint16_t)0x1)      //!< Enable the PWM complementary output and deadzone.
#define DEADZONE_DISABLE                       ((uint16_t)0x0)      //!< Disable the PWM complementary output and deadzone.
#define IS_TIM_PWM_DeadZone_En(mode) (((mode) == DEADZONE_ENABLE) || \
                                      ((mode) == DEADZONE_DISABLE)) //!< Check whether is the status of PWM deadzone.
/** End of group 87x3e_PWMDeadZone_En
  * @}
  */

/** @defgroup 87x3e_PWMDeadZone_Stop_state PWM Dead Zone Stop State
* @{
*/

#define PWM_STOP_AT_HIGH                        ((uint16_t)0x1)   //!< PWM DeadZone stop at high level.
#define PWM_STOP_AT_LOW                         ((uint16_t)0x0)   //!< PWM DeadZone stop at low level.

/** End of group 87x3e_PWMDeadZone_Stop_state
  * @}
  */

/** @defgroup 87x3e_PWMDeadZone_Peripheral PWM Dead Zone Peripheral
* @{
*/

#define IS_PWM_ALL_PERIPH(PERIPH) (((PERIPH) == PWM0_PN) || \
                                   ((PERIPH) == PWM1_PN))       //!< PWM Dead Zone peripherals can select PWM0_PN or PWM1_PN.

/** End of group 87x3e_PWMDeadZone_Peripheral
  * @}
  */

/** @defgroup 87x3e_TIM_Declaration TIM Declaration
  * @{
  */
#define TIM0                            ((TIM_TypeDef              *) TIM0_REG_BASE)  //!< The TIM0 base address.
#define TIM1                            ((TIM_TypeDef              *) TIM1_REG_BASE)  //!< The TIM1 base address.
#define TIM2                            ((TIM_TypeDef              *) TIM2_REG_BASE)  //!< The TIM2 base address.
#define TIM3                            ((TIM_TypeDef              *) TIM3_REG_BASE)  //!< The TIM3 base address.
#define TIM4                            ((TIM_TypeDef              *) TIM4_REG_BASE)  //!< The TIM4 base address.
#define TIM5                            ((TIM_TypeDef              *) TIM5_REG_BASE)  //!< The TIM5 base address.
#define TIM6                            ((TIM_TypeDef              *) TIM6_REG_BASE)  //!< The TIM6 base address.
#define TIM7                            ((TIM_TypeDef              *) TIM7_REG_BASE)  //!< The TIM7 base address.
/** End of group 87x3e_TIM_Declaration
  * @}
  */

/** @defgroup 87x3e_PWM_Declaration PWM Dead Zone Declaration
  * @{
  */
#define PWM0_PN                         ((PWM_TypeDef              *) PWM0_REG_BASE)  //!< The PWM0_PN base address.
#define PWM1_PN                         ((PWM_TypeDef              *) PWM1_REG_BASE)  //!< The PWM1_PN base address.
/** End of group 87x3e_PWM_Declaration
  * @}
  */

/** End of group 87x3e_TIM_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_TIM_Exported_Functions TIM Exported Functions
  * @{
  */


/**
 *
 * \brief   Initialize the TIMx peripheral according to
 *          the specified parameters in TIM_TimeBaseInitStruct.
 *
 * \param[in] TIMx: Where x can be 0 to 7 to select the TIM peripheral. \ref x3e_TIM_Declaration.
 * \param[in] TIM_TimeBaseInitStruct: Pointer to a TIM_TimeBaseInitTypeDef
 *            structure that contains the configuration information for the specified TIM peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void tim_driver_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_TIMERA, APBPeriph_TIMERA_CLOCK, ENABLE);
 *
 *     TIM_TimeBaseInitTypeDef TIM_InitStruct;
 *     TIM_StructInit(&TIM_InitStruct);
 *
 *     TIM_InitStruct.TIM_PWM_En = PWM_DISABLE;
 *     TIM_InitStruct.TIM_Period = 2000 * 10 - 1;
 *     TIM_InitStruct.TIM_Mode = TIM_Mode_UserDefine;
 *     TIM_TimeBaseInit(TIM6, &TIM_InitStruct);
 * }
 * \endcode
 */
void TIM_TimeBaseInit(TIM_TypeDef *TIMx, TIM_TimeBaseInitTypeDef *TIM_TimeBaseInitStruct);


/**
 *
 * \brief   Fills each TIM_TimeBaseInitStruct member with its default value.
 *
 * \note    The default settings for the TIM_TimeBaseInitStruct member are shown in the following table:
 *          | TIM_TimeBaseInitStruct Member | Default Value               |
 *          |:-----------------------------:|:---------------------------:|
 *          | TIM_Mode                      | \ref TIM_Mode_UserDefine    |
 *          | TIM_Period                    | 0xfff                       |
 *          | TIM_SOURCE_DIV                | \ref TIM_CLOCK_DIVIDER_1    |
 *          | ClockDepend                   | false                       |
 *          | PWM_Stop_State_N              | \ref PWM_STOP_AT_HIGH       |
 *          | PWM_Stop_State_P              | \ref PWM_STOP_AT_LOW        |
 *          | TIM_PWM_En                    | \ref PWM_DISABLE            |
 *          | PWMDeadZone_En                | \ref DEADZONE_DISABLE       |
 *
 * \param[in] TIM_TimeBaseInitStruct: Pointer to a TIM_TimeBaseInitTypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void tim_driver_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_TIMERA, APBPeriph_TIMERA_CLOCK, ENABLE);
 *
 *     TIM_TimeBaseInitTypeDef TIM_InitStruct;
 *     TIM_StructInit(&TIM_InitStruct);
 *
 *     TIM_InitStruct.TIM_PWM_En = PWM_DISABLE;
 *     TIM_InitStruct.TIM_Period = 2000 * 10 - 1;
 *     TIM_InitStruct.TIM_Mode = TIM_Mode_UserDefine;
 *     TIM_TimeBaseInit(TIM6, &TIM_InitStruct);
 * }
 * \endcode
 */
void TIM_StructInit(TIM_TimeBaseInitTypeDef *TIM_TimeBaseInitStruct);


/**
 *
 * \brief   Enable or disable the specified TIM peripheral.
 *
 * \param[in] TIMx: Where x can be 0 to 7 to select the TIMx peripheral. \ref x3e_TIM_Declaration.
 * \param[in] NewState: New state of the TIMx peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified TIM peripheral.
 *            - DISABLE: Disable the specified TIM peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void tim_driver_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_TIMERA, APBPeriph_TIMERA_CLOCK, ENABLE);
 *
 *     TIM_TimeBaseInitTypeDef TIM_InitStruct;
 *     TIM_StructInit(&TIM_InitStruct);
 *
 *     TIM_InitStruct.TIM_PWM_En = PWM_DISABLE;
 *     TIM_InitStruct.TIM_Period = 2000 * 10 - 1;
 *     TIM_InitStruct.TIM_Mode = TIM_Mode_UserDefine;
 *     TIM_TimeBaseInit(TIM6, &TIM_InitStruct);
 *     TIM_Cmd(TIM6, DISABLE);
 * }
 * \endcode
 */
void TIM_Cmd(TIM_TypeDef *TIMx, FunctionalState NewState);


/**
 *
 * \brief       Change the specified TIM period value.
 *
 * \param[in]   TIMx: Where x can be 0 to 7 to select the TIMx peripheral. \ref x3e_TIM_Declaration.
 * \param[in]   period: Period value to be changed. This parameter must range from 0x0 to 0xFFFFFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void timer_demo(void)
 * {
 *     uint32_t new_period = 2000 * 10 - 1;
 *     TIM_Cmd(TIM6, DISABLE);
 *     TIM_ChangePeriod(TIM6, new_period);
 *     TIM_Cmd(TIM6, ENABLE);
 * }
 * \endcode
 */
void TIM_ChangePeriod(TIM_TypeDef *TIMx, uint32_t period);


/**
 *
 * \brief     Enable or disable the specified TIMx interrupt.
 *
 * \param[in] TIMx: Where x can be 0 to 7 to select the TIM peripheral. \ref x3e_TIM_Declaration.
 * \param[in] NewState: New state of the specified TIM interrupt.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the timeout interrupt.
 *            - DISABLE: Disable the timeout interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void tim_driver_init(void)
 * {
 *     TIM_ClearINT(TIM6);
 *     TIM_INTConfig(TIM6, ENABLE);
 *     RamVectorTableUpdate(Timer6_VECTORn, (IRQ_Fun)tim_handler);
 *
 *     //Enable TIM6 IRQ
 *     NVIC_InitTypeDef NVIC_InitStruct;
 *     NVIC_InitStruct.NVIC_IRQChannel = TIM6_IRQn;
 *     NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
 *     NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
 *     NVIC_Init(&NVIC_InitStruct);
 * }
 * \endcode
 */
void TIM_INTConfig(TIM_TypeDef *TIMx, FunctionalState NewState);


/**
 *
 * \brief   Change the PWM frequency and duty cycle of the specified TIM according to high_count and low_count.
 *
 * \param[in] TIMx: Where x can be 0 to 7 to select the TIM peripheral. \ref x3e_TIM_Declaration.
 * \param[in] high_count: Set the PWM high count value. This parameter must range from 0~0xFFFFFFFF.
 * \param[in] low_count: Set the PWM low count value. This parameter must range from be 0~0xFFFFFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void timer_demo(void)
 * {
 *     uint32_t high_count = 500;
 *     uint32_t low_count = 500;
 *     TIM_PWMChangeFreqAndDuty(TIM6, high_count, low_count);
 * }
 * \endcode
 */
void TIM_PWMChangeFreqAndDuty(TIM_TypeDef *TIMx, uint32_t high_count, uint32_t low_count);

/**
 *
 * \brief   Configure the PWM deadzone clock source, enable or disable the PWM deadzone function and clock.
 *
 * \param[in]  TIMx: Where x can be 2 to 3 to select the TIMx peripheral. \ref x3e_TIM_Declaration.
 * \param[in]  clock_source: Specify the PWM deadzone clock source.
 *             This parameter can be one of the following values:
 *             - 0: The clock source is 32KHz.
 *             - 1: The clock source is 5MHz.
 * \param[in]  NewState: New state of the PWM deadzone function and clock.
 *             This parameter can be one of the following values:
 *             - ENABLE: Enable the PWM deadzone function and clock.
 *             - DISABLE: Disable the PWM deadzone function and clock.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void timer_demo(void)
 * {
 *    TIM_PWMClockConfig(TIM2, 0, ENABLE);
 * }
 * \endcode
 */
void TIM_PWMClockConfig(TIM_TypeDef *TIMx, bool clock_source, FunctionalState NewState);


/**
 *
 * \brief   Get the specified TIM current value when timer is running.
 *
 * \param[in]   TIMx: Where x can be 0 to 7 to select the TIM peripheral. \ref x3e_TIM_Declaration.
 *
 * \return  The current counter value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void timer_demo(void)
 * {
 *     uint32_t cur_value = TIM_GetCurrentValue(TIM6);
 * }
 * \endcode
 */
__forceinline uint32_t TIM_GetCurrentValue(TIM_TypeDef *TIMx)
{
    /* Check the parameters */
    assert_param(IS_TIM_ALL_PERIPH(TIMx));

    return TIMx->CurrentValue;
}


/**
 *
 * \brief  Get the specified TIM elapsed value when timer is running.
 *
 * \param[in]  TIMx: Where x can be 0 to 7 to select the TIM peripheral. \ref x3e_TIM_Declaration.
 *
 * \return The elapsed counter value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void timer_demo(void)
 * {
 *     uint32_t value = TIM_GetElapsedValue(TIM6);
 * }
 * \endcode
 */
__forceinline uint32_t TIM_GetElapsedValue(TIM_TypeDef *TIMx)
{
    /* Check the parameters */
    assert_param(IS_TIM_ALL_PERIPH(TIMx));

    return (TIMx->LoadCount - TIMx->CurrentValue);
}


/**
 *
 * \brief   Get the specified TIM operation status.
 *
 * \param[in]   TIMx: Where x can be 0 to 7 to select the TIM peripheral. \ref x3e_TIM_Declaration.
 *
 * \return The new state of the specified TIM.
 * \retval SET: The timer is running.
 * \retval RESET: The timer is not running.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void timer_demo(void)
 * {
 *    if (TIM_GetStatus(TIM6) == ENABLE)
 *    {
 *        // add user code here
 *    }
 * }
 * \endcode
 */
__forceinline FunctionalState TIM_GetStatus(TIM_TypeDef *TIMx)
{
    /* Check the parameters */
    assert_param(IS_TIM_ALL_PERIPH(TIMx));

    if (TIMx->ControlReg & BIT(0))
    {
        return ENABLE;
    }
    else
    {
        return DISABLE;
    }
}


/**
 *
 * \brief   Get the specified TIM interrupt status.
 *
 * \param[in]   TIMx: Where x can be 0 to 7 to select the TIM peripheral. \ref x3e_TIM_Declaration.
 *
 * \return The new state of the specified TIM interrupt.
 * \retval SET: The TIM interrupt has occurred.
 * \retval RESET: The TIM interrupt has not occurred.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void tim_handler(void)
 * {
 *    if (TIM_GetINTStatus(TIM6) == SET)
 *    {
 *        TIM_ClearINT(TIM6);
 *        TIM_Cmd(TIM6, DISABLE);
 *        // add user code here
 *    }
 * }
 * \endcode
 */
__forceinline ITStatus TIM_GetINTStatus(TIM_TypeDef *TIMx)
{
    ITStatus bitstatus = RESET;
    uint16_t itstatus = (uint16_t)TIMx->IntStatus;

    /* Check the parameters */
    assert_param(IS_TIM_ALL_PERIPH(TIMx));

    if (itstatus != (uint16_t)RESET)
    {
        bitstatus = SET;
    }

    return bitstatus;
}


/**
 *
 * \brief   Clear the specified TIM interrupt.
 *
 * \param[in]   TIMx: Where x can be 0 to 7 to select the TIM peripheral. \ref x3e_TIM_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void tim_handler(void)
 * {
 *    if (TIM_GetINTStatus(TIM6) == SET)
 *    {
 *        TIM_ClearINT(TIM6);
 *        // add user code here
 *    }
 * }
 * \endcode
 */
__forceinline void TIM_ClearINT(TIM_TypeDef *TIMx)
{
    /* Check the parameters */
    assert_param(IS_TIM_ALL_PERIPH(TIMx));
    /* Clear the IT */
    TIMx->EOI;
}


/**
 *
 * \brief   PWM deadzone mode emergency stop.
 *
 * \param[in]  PWMx: Where x can be 0_PN to 1_PN to select the PWM dead zone peripheral. \ref x3e_PWM_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void test_timer_pwmdeadzone(void)
 * {
 *    PWM_Deadzone_EMStop(PWM0_PN);
 * }
 * \endcode
 */
__forceinline void PWM_Deadzone_EMStop(PWM_TypeDef *PWMx)
{
    /* Check the parameters */
    assert_param(IS_PWM_ALL_PERIPH(PWMx));

    PWMx->CR |= BIT(8);
}

#ifdef __cplusplus
}
#endif

#endif /*__RTL876X_TIM_H*/

/** @} */ /* End of group 87x3e_TIM_Exported_Functions */
/** @} */ /* End of group 87x3e_TIM */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

