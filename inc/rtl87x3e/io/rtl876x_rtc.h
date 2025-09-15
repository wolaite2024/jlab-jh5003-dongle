/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_rtc.h
* @brief
* @details
* @author    justin
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef _RTL876X_RTC_H_
#define _RTL876X_RTC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"


/* define ------------------------------------------------------------------*/
typedef enum _INT_STATUS_POS
{
    RTC_TICK_CLR = 0,
    RTC_CNT_OV_CLR,
    RTC_PRECMP_CLR,
    RTC_PRECMP_CMP3_CLR,
    RTC_CMP0GT_CLR,           /*4*/
    RTC_CMP1GT_CLR,
    RTC_CMP2GT_CLR,
    RTC_CMP3GT_CLR,
    RTC_CMP0_NV_CLR,        /*8*/
    RTC_CMP1_NV_CLR,
    RTC_CMP2_NV_CLR,
    RTC_CMP3_NV_CLR,
    RTC_CMP0_WK_CLR,         /*12*/
    RTC_CMP1_WK_CLR,
    RTC_CMP2_WK_CLR,
    RTC_CMP3_WK_CLR,
    RTC_PF_CMP0_GT_CLR,      /*16*/
    RTC_PF_CMP1_GT_CLR,
    RTC_PF_CMP0_NV_CLR,
    RTC_PF_CMP1_NV_CLR,
    RTC_PF_CMP0_WK_CLR,     /*20*/
    RTC_PF_CMP1_WK_CLR,

} INT_STATUS_POS;

/* Register: CR0 bit29~31 */
#define RTC_START_EN          BIT(0)
#define RTC_START_CLR         (~(RTC_START_EN))
/*compatible  bbpro*/
#define RTC_START_Msk                     RTC_START_EN

#define RTC_COUNTER_RST_EN  BIT(1)
/*compatible  bbpro*/
#define RTC_COUNTER_RST_Msk          RTC_COUNTER_RST_EN

#define RTC_DIV_COUNTER_RST_EN    BIT(2)
/*compatible  bbpro*/
#define RTC_DIV_COUNTER_RST_Msk          RTC_DIV_COUNTER_RST_EN

#define RTC_WAKEUP_EN      BIT(29)
#define RTC_NV_IE_EN_GLB   BIT(30)
#define RTC_REST_EN        BIT(31)


/* Peripheral: RTC */
/* Description: Real time counter register defines */

/* Register: sleep control global register0 */
#define RTC_SLEEP_CR                    (0x40000100UL)

/* Clear all interrupt */
#define RTC_ALL_INT_CLR_SET             0x3fffff

#define RTC_ALL_INT_CLR_RESET           0xffc00000

/* RTC internal function: Fast write RTC register. */
void RTC_WriteReg(uint32_t offset, uint32_t data);
void RTC_WriteReg_Safe(uint32_t offset, uint32_t data);


/** @addtogroup 87x3e_RTC RTC
  * @brief RTC driver module.
  * @{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_RTC_Exported_Constants RTC Exported Constants
  * @{
  */

/** @defgroup 87x3e_RTC_Comparator_Index RTC Comparator Index
  * @{
  */
/**
 * @brief The RTC comparator index.
 */
typedef enum _COMPX_INDEX
{
    COMP0_INDEX = 0,            //!< RTC comparator index0.
    COMP1_INDEX,                //!< RTC comparator index1.
    COMP2_INDEX,                //!< RTC comparator index2.
    COMP3_INDEX,                //!< RTC comparator index3.
    COMP0GT_INDEX,              //!< RTC comparator GT index0.
    COMP1GT_INDEX,              //!< RTC comparator GT index1.
    COMP2GT_INDEX,              //!< RTC comparator GT index2.
    COMP3GT_INDEX,              //!< RTC comparator GT index3.
    BACKUP0,                    //!< Backup0.
    BACKUP1,                    //!< Backup1.
    PRESCALER_COMP_INDEX,       //!< Prescaler comparator index.
} COMPX_INDEX_t;

/** End of group 87x3e_RTC_Comparator_Index
  * @}
  */

/** @defgroup 87x3e_RTC_Interrupts RTC Interrupts
  * @{
  */
/**
 * @brief The RTC interrupts.
 */
typedef enum _RTC_INT
{
    RTC_INT_TICK        = BIT(8),   //!< RTC tick interrupt.
    RTC_INT_OVF         = BIT(9),   //!< RTC overflow interrupt.
    RTC_PRECMP_INT      = BIT(10),  //!< RTC PRECOMP interrupt.
    RTC_PRECMP_CMP3_INT = BIT(11),  //!< RTC PRECOMP&CMP3 interrupt.

    RTC_CMP0GT_INT      = BIT(12),  //!< RTC CMP0 GT interrupt.
    RTC_CMP1GT_INT      = BIT(13),  //!< RTC CMP1 GT interrupt.
    RTC_CMP2GT_INT      = BIT(14),  //!< RTC CMP2 GT interrupt.
    RTC_CMP3GT_INT      = BIT(15),  //!< RTC CMP3 GT interrupt.
    RTC_CMP0_NV_INT     = BIT(16),     //!< RTC CMP0 interrupt.
    RTC_INT_CMP0        = BIT(16),     //!< RTC CMP0 interrupt.

    RTC_CMP1_NV_INT     = BIT(17),     //!< RTC CMP1 interrupt.
    RTC_INT_CMP1        = BIT(17),     //!< RTC CMP1 interrupt.

    RTC_CMP2_NV_INT     = BIT(18),     //!< RTC CMP2 interrupt.
    RTC_INT_CMP2        = BIT(18),     //!< RTC CMP2 interrupt.

    RTC_CMP3_NV_INT     = BIT(19),     //!< RTC CMP3 interrupt.
    RTC_INT_CMP3        = BIT(19),     //!< RTC CMP3 interrupt.

    RTC_CMP0_WK_INT     = BIT(20),  //!< RTC CMP0 GT wakeup interrupt.
    RTC_CMP1_WK_INT     = BIT(21),  //!< RTC CMP1 GT wakeup interrupt.
    RTC_CMP2_WK_INT     = BIT(22),  //!< RTC CMP2 GT wakeup interrupt.
    RTC_CMP3_WK_INT     = BIT(23),  //!< RTC CMP3 GT wakeup interrupt.

} RTC_INT_t;

#define IS_RTC_CONFIG_INT(INT) (((INT) == RTC_INT_TICK) || \
                                ((INT) == RTC_INT_OVF) || \
                                ((INT) == RTC_PRECMP_INT) || \
                                ((INT) == RTC_PRECMP_CMP3_INT) || \
                                ((INT) == RTC_CMP0_NV_INT) || \
                                ((INT) == RTC_CMP1_NV_INT) || \
                                ((INT) == RTC_CMP2_NV_INT) || \
                                ((INT) == RTC_CMP3_NV_INT) || \
                                ((INT) == RTC_CMP0GT_INT) || \
                                ((INT) == RTC_CMP1GT_INT) || \
                                ((INT) == RTC_CMP2GT_INT) || \
                                ((INT) == RTC_CMP3GT_INT)) //!< Check if RTC interrupt is valid.

#define IS_RTC_COMP_INT(INT) ( ((INT) == RTC_CMP0_NV_INT) || \
                               ((INT) == RTC_CMP1_NV_INT) || \
                               ((INT) == RTC_CMP2_NV_INT) || \
                               ((INT) == RTC_CMP3_NV_INT)) //!< Check if RTC CMPX interrupt is valid.

/** End of group 87x3e_RTC_Interrupts
  * @}
  */

/** @defgroup 87x3e_RTC_sleep_mode_clock_definition RTC Sleep Mode Clock Definition
  * @{
  */

#define RTC_EXTERNAL_CLK            EXTERNAL_CLK //!< RTC sleep mode clock is BTAON (xtal or ring osc).
#define RTC_INTERNAL_CLK            INTERNAL_CLK //!< RTC sleep mode clock is 32k sdm(internal 32K after calibration).
#define IS_RTC_SLEEP_CLK(CLK) ( ((CLK) == EXTERNAL_CLK) || \
                                ((CLK) == INTERNAL_CLK)) //!< Check if the input parameter is valid.

/** End of group 87x3e_RTC_sleep_mode_clock_definition
  * @}
  */

/** End of group 87x3e_RTC_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_RTC_Exported_Functions RTC Exported Functions
  * @{
  */


/**
 *
 * \brief     Deinitializes the RTC peripheral registers to their default reset values(turn off clock).
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_PRESCALER_VALUE     320
 * #define RTC_COMP_INDEX          1
 * #define RTC_INT_CMP_1      RTC_INT_CMP1
 * #define RTC_COMP_VALUE          (1000)
 *
 * void driver_rtc_init(void)
 * {
 *     RTC_DeInit();
 *     RTC_SetPrescaler(RTC_PRESCALER_VALUE);
 *     RTC_SetComp(RTC_COMP_INDEX, RTC_COMP_VALUE);
 *     RTC_MaskINTConfig(RTC_INT_CMP_1, DISABLE);
 *     RTC_CompINTConfig(RTC_INT_CMP_1, ENABLE);
 *
 *     //Start RTC
 *     RTC_SystemWakeupConfig(ENABLE);
 *     RTC_RunCmd(ENABLE);
 * }
 * \endcode
 */
void RTC_DeInit(void);


/**
 *
 * \brief     Set RTC prescaler value.
 *
 * \param[in] value: The prescaler value to be set. Should be no more than 12 bits!
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_PRESCALER_VALUE     320
 *
 * void driver_rtc_init(void)
 * {
 *     RTC_SetPrescaler(RTC_PRESCALER_VALUE);
 * }
 * \endcode
 */
void RTC_SetPrescaler(uint32_t value);


/**
 *
 * \brief     Set RTC comparator value.
 *
 * \param[in] index: The comparator number to be set, please refer to \ref x3e_RTC_Comparator_Index.
 * \param[in] value: The comparator value to be set. Should be no more than 24 bits!
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_COMP_INDEX          1
 * #define RTC_COMP_VALUE          (1000)
 *
 * void driver_rtc_init(void)
 * {
 *     RTC_SetComp(RTC_COMP_INDEX, RTC_COMP_VALUE);
 * }
 * \endcode
 */
void RTC_SetComp(uint8_t index, uint32_t value);


/**
 *
 * \brief     Start or stop RTC peripheral.
 *
 * \param[in] NewState: New state of RTC peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: RTC count start.
 *            - DISABLE: RTC count stop and clear count value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_rtc_init(void)
 * {
 *     //Start RTC
 *     RTC_SystemWakeupConfig(ENABLE);
 *     RTC_RunCmd(ENABLE);
 * }
 * \endcode
 */
void RTC_RunCmd(FunctionalState NewState);


/**
 *
 * \brief     Enable or disable interrupt signal to CPU NVIC.
 *
 * \param[in] NewState: New state of interrupt signal to CPU NVIC.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable interrupt signal to CPU NVIC.
 *            - DISABLE: Disable interrupt signal to CPU NVIC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_rtc_init(void)
 * {
 *     RTC_CpuNVICEnable(ENABLE);
 * }
 * \endcode
 */
void RTC_CpuNVICEnable(FunctionalState NewState);


/**
 *
 * \brief     Enable or disable the specified RTC interrupt.
 *
 * \param[in] RTC_INT: Specifies the RTC interrupt, please refer to \ref x3e_RTC_Interrupts.
 *            This parameter can be any combination of the following values:
 *            - RTC_INT_TICK: RTC tick interrupt.
 *            - RTC_INT_OVF: RTC counter overflow interrupt.
 *            - RTC_PRECMP_INT: RTC prescale comparator interrupt.
 *            - RTC_CMPx_WK_INT: RTC comparator x wake up interrupt, x =0~3.
 *            - RTC_CMPx_NV_INT: RTC comparator x interrupt, x =0~3.
 *            - RTC_CMPxGT_INT: RTC comparator GT x interrupt, x =0~3.
 *            - RTC_PRECMP_CMP3_INT: RTC prescale & comparator 3 interrupt.
 *
 * \param[in] NewState: New state of the specified RTC interrupt.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified interrupt of RTC.
 *            - DISABLE: Disable the specified interrupt of RTC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_INT_CMP_1      RTC_INT_CMP1
 * void driver_rtc_init(void)
 * {
 *     RTC_INTEnableConfig(RTC_CMP0_WK_INT, ENABLE);
 *
 *     RTC_MaskINTConfig(RTC_INT_CMP_1, DISABLE);
 *     RTC_CompINTConfig(RTC_INT_CMP_1, ENABLE);
 * }
 * \endcode
 */
void RTC_INTEnableConfig(uint32_t RTC_INT, FunctionalState NewState);


/**
 *
 * \brief     Mask or unmask the selected RTC interrupts.
 *
 * \param[in] RTC_INT: Specifies the RTC interrupt, please refer to \ref x3e_RTC_Interrupts.
 *            This parameter can be any combination of the following values:
 *            - RTC_INT_TICK: RTC tick interrupt.
 *            - RTC_INT_OVF: RTC counter overflow interrupt.
 *            - RTC_PRECMP_INT: RTC prescale comparator interrupt.
 *            - RTC_CMPx_WK_INT: RTC comparator x wake up interrupt, x =0~3.
 *            - RTC_CMPx_NV_INT: RTC comparator x interrupt, x =0~3.
 *            - RTC_CMPxGT_INT: RTC comparator GT x interrupt, x =0~3.
 *            - RTC_PRECMP_CMP3_INT: RTC prescale & comparator 3 interrupt.
 *
 * \param[in] NewState: New state of the specified RTC interrupt.
 *            This parameter can be one of the following values:
 *            - ENABLE: Mask the selected RTC interrupt.
 *            - DISABLE: Unmask the selected RTC interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_INT_CMP_1      RTC_INT_CMP1
 *
 * void driver_rtc_init(void)
 * {
 *     RTC_MaskINTConfig(RTC_INT_CMP_1, DISABLE);
 *     RTC_CompINTConfig(RTC_INT_CMP_1, ENABLE);
 * }
 * \endcode
 */
void RTC_MaskINTConfig(uint32_t RTC_INT, FunctionalState NewState);
#define RTC_EN_INTConfig     RTC_MaskINTConfig //!< Please refer to \ref RTC_MaskINTConfig.

/**
 *
 * \brief     Enable or disable the specified RTC interrupts for comparator.
 *
 * \param[in] RTC_INT: Specifies the RTC comparator interrupt, please refer to \ref x3e_RTC_Interrupts.
 *            This parameter can be any combination of the following values:
 *            - RTC_INT_TICK: RTC tick interrupt.
 *            - RTC_INT_OVF: RTC counter overflow interrupt.
 *            - RTC_PRECMP_INT: RTC prescale comparator interrupt.
 *            - RTC_CMPx_WK_INT: RTC comparator x wake up interrupt, x =0~3.
 *            - RTC_CMPx_NV_INT: RTC comparator x interrupt, x =0~3.
 *            - RTC_CMPxGT_INT: RTC comparator GT x interrupt, x =0~3.
 *            - RTC_PRECMP_CMP3_INT: RTC prescale & comparator 3 interrupt.
 *
 * \param[in] NewState: New state of the specified RTC interrupt.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the selected RTC comparator interrupt.
 *            - DISABLE: Disable the selected RTC comparator interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_INT_CMP_1      RTC_INT_CMP1
 *
 * void driver_rtc_init(void)
 * {
 *     RTC_CompINTConfig(RTC_INT_CMP_1, ENABLE);
 * }
 * \endcode
 */
void RTC_CompINTConfig(uint32_t RTC_INT, FunctionalState NewState);


/**
 *
 * \brief     Enable or disable RTC tick interrupt.
 *
 * \param[in] NewState: New state of RTC tick interrupt.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable RTC tick interrupt.
 *            - DISABLE: Disable RTC tick interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_PRESCALER_VALUE     0
 *
 * void driver_rtc_init(void)
 * {
 *     RTC_SetPrescaler(RTC_PRESCALER_VALUE);
 *     RTC_MaskINTConfig(RTC_INT_TICK, DISABLE);
 *     RTC_TickINTConfig(ENABLE);
 * }
 * \endcode
 */
void RTC_TickINTConfig(FunctionalState NewState);


/**
 *
 * \brief  Check whether the specified RTC interrupt is set or not.
 *
 * \param[in] RTC_INT: Specifies the RTC interrupt, please refer to \ref x3e_RTC_Interrupts.
 *            This parameter can be any combination of the following values:
 *            - RTC_INT_TICK: RTC tick interrupt.
 *            - RTC_INT_OVF: RTC counter overflow interrupt.
 *            - RTC_PRECMP_INT: RTC prescale comparator interrupt.
 *            - RTC_CMPx_WK_INT: RTC comparator x wake up interrupt, x =0~3.
 *            - RTC_CMPx_NV_INT: RTC comparator x interrupt, x =0~3.
 *            - RTC_CMPxGT_INT: RTC comparator GT x interrupt, x =0~3.
 *            - RTC_PRECMP_CMP3_INT: RTC prescale & comparator 3 interrupt.
 *
 * \return The new state of RTC interrupt.
 * \retval SET: The specified RTC interrupt has occurred.
 * \retval RESET: The specified RTC interrupt has not occurred.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void rtc_dlps_restore(void)
 * {
 *    RTC_SystemWakeupConfig(DISABLE);
 *    RTC_RunCmd(DISABLE);
 *    if (RTC_GetINTStatus(RTC_CMP0_WK_INT) == SET)
 *    {
 *        RTC_ClearINTStatus(RTC_CMP0_WK_INT);
 *        RTC_INTEnableConfig(RTC_CMP0_WK_INT, DISABLE);
 *    }
 *    power_mode_set(POWER_ACTIVE_MODE);
 *    APP_PRINT_INFO0("RTC EXIT DLPS");
 * }
 * \endcode
 */
ITStatus RTC_GetINTStatus(uint32_t RTC_INT);


/**
 *
 * \brief     Enable or disable system wake up function of RTC.
 *
 * \param[in] NewState: New state of the RTC wake up function.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable system wake up function of RTC.
 *            - DISABLE: Disable system wake up function of RTC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_rtc_init(void)
 * {
 *     RTC_DeInit();
 *     RTC_SetPrescaler(RTC_PRESCALER_VALUE);
 *     RTC_SetComp(RTC_COMP_INDEX, RTC_COMP_VALUE);
 *     RTC_MaskINTConfig(RTC_INT_CMP_1, DISABLE);
 *     RTC_CompINTConfig(RTC_INT_CMP_1, ENABLE);
 *
 *     //Start RTC
 *     RTC_SystemWakeupConfig(ENABLE);
 *     RTC_RunCmd(ENABLE);
 * }
 * \endcode
 */
void RTC_SystemWakeupConfig(FunctionalState NewState);


/**
 *
 * \brief  Clear interrupt status of RTC.
 *
 * \param[in] RTC_INT: Specifies the RTC interrupt, please refer to \ref x3e_RTC_Interrupts.
 *            This parameter can be any combination of the following values:
 *            - RTC_INT_TICK: RTC tick interrupt.
 *            - RTC_INT_OVF: RTC counter overflow interrupt.
 *            - RTC_PRECMP_INT: RTC prescale comparator interrupt.
 *            - RTC_CMPx_WK_INT: RTC comparator x wake up interrupt, x =0~3.
 *            - RTC_CMPx_NV_INT: RTC comparator x interrupt, x =0~3.
 *            - RTC_CMPxGT_INT: RTC comparator GT x interrupt, x =0~3.
 *            - RTC_PRECMP_CMP3_INT: RTC prescale & comparator 3 interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void rtc_dlps_restore(void)
 * {
 *    if (RTC_GetINTStatus(RTC_CMP0_WK_INT) == SET)
 *    {
 *        RTC_ClearINTStatus(RTC_CMP0_WK_INT);
 *        RTC_INTEnableConfig(RTC_CMP0_WK_INT, DISABLE);
 *    }
 * }
 * \endcode
 */
void RTC_ClearINTStatus(uint32_t RTC_INT);


/**
 *
 * \brief     Reset prescaler counter value of RTC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void rtc_demo(void)
 * {
 *     RTC_ResetPrescalerCounter();
 *     RTC_RunCmd(ENABLE);
 * }
 * \endcode
 */
static __forceinline void RTC_ResetPrescalerCounter(void)
{
    RTC_WriteReg((uint32_t)(&(RTC->CR0)), (RTC->CR0) | RTC_DIV_COUNTER_RST_EN);
    __NOP();
    __NOP();
    RTC_WriteReg((uint32_t)(&(RTC->CR0)), (RTC->CR0) & (~(RTC_DIV_COUNTER_RST_EN)));
}


/**
 *
 * \brief     Get counter value of RTC.
 *
 * \return    The counter value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_COMP_INDEX          1
 * #define RTC_COMP_VALUE          (1000)
 * #define RTC_INT_CMP_1      RTC_INT_CMP1
 *
 * void RTC_Handler(void)
 * {
 *     if (RTC_GetINTStatus(RTC_INT_CMP_1) == SET)
 *    {
 *         RTC_SetComp(RTC_COMP_INDEX, RTC_GetCounter() + RTC_COMP_VALUE);
 *         RTC_ClearCompINT(RTC_COMP_INDEX);
 *    }
 * }
 * \endcode
 */
static __forceinline uint32_t RTC_GetCounter(void)
{
    return RTC->CNT;
}


/**
 *
 * \brief     Reset counter value of RTC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void rtc_demo(void)
 * {
 *     RTC_ResetCounter();
 *     RTC_RunCmd(ENABLE);
 * }
 * \endcode
 */
static __forceinline void RTC_ResetCounter(void)
{
    RTC_WriteReg((uint32_t)(&(RTC->CR0)), (RTC->CR0) | RTC_COUNTER_RST_Msk);
    __NOP();
    __NOP();
    RTC_WriteReg((uint32_t)(&(RTC->CR0)), (RTC->CR0) & (~(RTC_COUNTER_RST_EN)));
}


/**
 *
 * \brief     Get RTC comparator value.
 *
 * \param[in] index: The comparator number to be set, please refer to \ref x3e_RTC_Comparator_Index.
 *
 * \return    The RTC comparator value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void rtc_demo(void)
 * {
 *     uint32_t CompareValue = RTC_GetComp(0);
 * }
 * \endcode
 */
static __forceinline uint32_t RTC_GetComp(uint8_t index)
{
    return *((volatile uint32_t *)(&(RTC->COMP0) + index));
}


/**
 *
 * \brief     Clear the interrupt pending bit of the select comparator of RTC.
 *
 * \param[in] index: The comparator number to be set, please refer to \ref x3e_RTC_Comparator_Index.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_COMP_INDEX          1
 * #define RTC_COMP_VALUE          (1000)
 * #define RTC_INT_CMP_1      RTC_INT_CMP1
 *
 * void RTC_Handler(void)
 * {
 *     if (RTC_GetINTStatus(RTC_INT_CMP_1) == SET)
 *     {
 *         RTC_ClearCompINT(RTC_COMP_INDEX);
 *     }
 * }
 * \endcode
 */
static __forceinline void RTC_ClearCompINT(uint8_t index)
{
    RTC_WriteReg((uint32_t)(&(RTC->INT_MASK)), BIT(RTC_CMP0_NV_CLR + index));
    __NOP();
    __NOP();
    RTC_WriteReg((uint32_t)(&(RTC->INT_MASK)), 0);

}


/**
 *
 * \brief     Clear wakeup interrupt of the select comparator of RTC.
 *
 * \param[in] index: The comparator number to be set, please refer to \ref x3e_RTC_Comparator_Index.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define RTC_COMP_INDEX          1
 * #define RTC_INT_CMP_1      RTC_INT_CMP1
 *
 * void RTC_Handler(void)
 * {
 *     if (RTC_GetINTStatus(RTC_INT_CMP_1) == SET)
 *    {
 *         //add user here.
 *         RTC_ClearCompINT(RTC_COMP_INDEX);
 *         RTC_ClearCompWkINT(RTC_COMP_INDEX);
 *    }
 * }
 * \endcode
 */
static __forceinline void RTC_ClearCompWkINT(uint8_t index)
{
    RTC_WriteReg((uint32_t)(&(RTC->INT_MASK)), BIT(RTC_CMP0_WK_CLR + index));
    __NOP();
    __NOP();
    RTC_WriteReg((uint32_t)(&(RTC->INT_MASK)), 0);
}


/**
 *
 * \brief     Clear the overflow interrupt pending bit of RTC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void RTC_Handler(void)
 * {
 *     //RTC overflow interrupt handle
 *     if (RTC_GetINTStatus(RTC_INT_OVF) == SET)
 *    {
 *         // Add application code here
 *         RTC_ClearOverFlowINT();
 *    }
 * }
 * \endcode
 */
static __forceinline void RTC_ClearOverFlowINT(void)
{
    RTC_WriteReg((uint32_t)(&(RTC->INT_MASK)), BIT(RTC_CNT_OV_CLR));
    __NOP();
    __NOP();
    RTC_WriteReg((uint32_t)(&(RTC->INT_MASK)), 0);
}


/**
 *
 * \brief     Clear the tick interrupt pending bit of RTC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void RTC_Handler(void)
 * {
 *     //RTC tick interrupt handle
 *     if (RTC_GetINTStatus(RTC_INT_TICK) == SET)
 *    {
 *         // Add application code here
 *         RTC_ClearTickINT();
 *    }
 * }
 * \endcode
 */
static __forceinline void RTC_ClearTickINT(void)
{
    RTC_WriteReg((uint32_t)(&(RTC->INT_MASK)),  BIT(RTC_TICK_CLR));
    __NOP();
    __NOP();
    RTC_WriteReg((uint32_t)(&(RTC->INT_MASK)), 0);
}


/**
 *
 * \brief     Config the clock of sleep mode.
 *
 * \param[in] clock: The sleep clock source.
 *            This parameter can be the following values:
 *            - RTC_EXTERNAL_CLK: From BTAON (xtal or ring osc).
 *            - RTC_INTERNAL_CLK: From 32k sdm(internal 32K after calibration).
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void rtc_driver_init(void)
 * {
 *     RTC_SleepModeClkConfig(RTC_INTERNAL_CLK);
 * }
 * \endcode
 */
static __forceinline void RTC_SleepModeClkConfig(uint32_t clock)
{
    /* Check the parameters */
    assert_param(IS_RTC_SLEEP_CLK(clock));

    // RTC_WriteReg(RTC_SLEEP_CR, *(volatile uint32_t *)(RTC_SLEEP_CR) & clock);
}

/** @} */ /* End of group 87x3e_RTC_Exported_Functions */
/** @} */ /* End of group 87x3e_RTC */

#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_RTC_H_ */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/

