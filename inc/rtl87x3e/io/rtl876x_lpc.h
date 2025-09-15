/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_lpc.h
* @brief
* @details
* @author    elliot chen
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef __RTL876X_LPC_H
#define __RTL876X_LPC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"
#include "rtl876x_rtc.h"

/* Defines -------------------------------------------------------------------*/

/* Peripheral: LPC */
/* Description: Low Power Comparator register defines */

/* Register: LPC_CR0 */
/* Description: Control Register 0 */

/* LPC_CR0[15] :LPC_Threshold. Specified the threshold value of comparator voltage */
#define LPC_THRESHOLD_Pos (15UL)
#define LPC_THRESHOLD_Msk (0xfUL << LPC_THRESHOLD_Pos)

/* LPC_CR0[12] :LPC_Channel. Specified the input pin */
#define LPC_CH_NUM_Pos (12UL)
#define LPC_CH_NUM_Msk (0x7UL << LPC_CH_NUM_Pos)

/* LPC_CR0[11] :LPC_Edge. Specified the cmp output edge */
#define LPC_POSEDGE_Pos (11UL)
#define LPC_POSEDGE_Msk (0x1UL << LPC_POSEDGE_Pos)

/* LPC_CR0[9] :LPC_SRC_INT_EN. 1: Enable lpcomp out sync signal to CPU interrupt  */
#define LPC_SRC_INT_EN_Pos (9UL)
#define LPC_SRC_INT_EN_Msk (0x1UL << LPC_SRC_INT_EN_Pos)

/* LPC_CR0[8] :LPC_EVENT_EN. 1: Enable or disable LPCOMP event */
#define LPC_EVENT_EN_Pos (8UL)
#define LPC_EVENT_EN_Msk (0x1UL << LPC_EVENT_EN_Pos)

/* LPC_CR0[3] : LPC_POWER. 1: Enable power of low power cmp */
#define LPC_POWER_EN_Pos (10UL)
#define LPC_POWER_EN_Msk (0x1UL << LPC_POWER_EN_Pos)

/* LPC_CR0[2] :LPC_FLAG_CLEAR. 1: Clear Event Status of LPCOMP */
#define LPC_FLAG_CLEAR_Pos (2UL)
#define LPC_FLAG_CLEAR_Msk (0x1UL << LPC_FLAG_CLEAR_Pos)

/* LPC_CR0[1] :LPC_COUNTER_START. 1: Start LPCOMP counter. */
#define LPC_COUNTER_START_Pos (1UL)
#define LPC_COUNTER_START_Msk (0x1UL << LPC_COUNTER_START_Pos)

/* LPC_CR0[0] :LPC_COUNTER_RESET. 1: Reset LPCOMP Counter */
#define LPC_COUNTER_RESET_Pos (0UL)
#define LPC_COUNTER_RESET_Msk (0x1UL << LPC_COUNTER_RESET_Pos)

/* Register: LPC_SR */
/* Description: Status Register */

/* LPC_SR[0] :LPC_FLAG. Event Status of LPCOMP */
#define LPC_FLAG_Pos (0UL) /*!< Position of */
#define LPC_TFLAG_Msk (0xfUL << LPC_FLAG_Pos) /*!< Bit mask of  */


/** @addtogroup 87x3e_LPC LPC
  * @brief Low power comparator driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup 87x3e_LPC_Exported_Types LPC Exported Types
  * @{
  */

/**
  * @brief  LPC Init structure definition.
  */
typedef struct
{
    uint16_t LPC_Channel;          /*!< Specifies the input LPC channel.
                                                    This parameter can be a value of ADC_0 to ADC_7. */

    uint32_t LPC_Edge;             /*!< Specifies the comparator output edge. This parameter can be a value of \ref x3e_LPC_Edge.*/

    uint32_t LPC_Threshold;        /*!< Specifies the threshold value of comparator voltage. This parameter can be a value of \ref x3e_LPC_Threshold. */

} LPC_InitTypeDef;

/** End of group 87x3e_LPC_Exported_Types
      * @}
      */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_LPC_Exported_Constants LPC Exported Constants
  * @{
  */

/** @defgroup 87x3e_LPC_Threshold LPC Threshold
  * @{
  */
#define LPC_200_mV                 ((uint32_t)0x0000) //!< LPC threshold value is 200mV.
#define LPC_400_mV                 ((uint32_t)0x0001) //!< LPC threshold value is 400mV.
#define LPC_600_mV                 ((uint32_t)0x0002) //!< LPC threshold value is 600mV.
#define LPC_800_mV                 ((uint32_t)0x0003) //!< LPC threshold value is 800mV.
#define LPC_1000_mV                ((uint32_t)0x0004) //!< LPC threshold value is 1000mV.
#define LPC_1200_mV                ((uint32_t)0x0005) //!< LPC threshold value is 1200mV.
#define LPC_1400_mV                ((uint32_t)0x0006) //!< LPC threshold value is 1400mV.
#define LPC_1600_mV                ((uint32_t)0x0007) //!< LPC threshold value is 1600mV.
#define LPC_1800_mV                ((uint32_t)0x0008) //!< LPC threshold value is 1800mV.
#define LPC_2000_mV                ((uint32_t)0x0009) //!< LPC threshold value is 2000mV.
#define LPC_2200_mV                ((uint32_t)0x000a) //!< LPC threshold value is 2200mV.
#define LPC_2400_mV                ((uint32_t)0x000b) //!< LPC threshold value is 2400mV.
#define LPC_2600_mV                ((uint32_t)0x000c) //!< LPC threshold value is 2600mV.
#define LPC_2800_mV                ((uint32_t)0x000d) //!< LPC threshold value is 2800mV.
#define LPC_3000_mV                ((uint32_t)0x000e) //!< LPC threshold value is 3000mV.
#define LPC_3200_mV                ((uint32_t)0x000f) //!< LPC threshold value is 3200mV.

#define IS_LPC_THRESHOLD(THRESHOLD) (((THRESHOLD) == LPC_200_mV) || \
                                     ((THRESHOLD) == LPC_400_mV) || \
                                     ((THRESHOLD) == LPC_600_mV) || \
                                     ((THRESHOLD) == LPC_800_mV) || \
                                     ((THRESHOLD) == LPC_1000_mV) || \
                                     ((THRESHOLD) == LPC_1200_mV) || \
                                     ((THRESHOLD) == LPC_1400_mV) || \
                                     ((THRESHOLD) == LPC_1600_mV) || \
                                     ((THRESHOLD) == LPC_1800_mV) || \
                                     ((THRESHOLD) == LPC_2000_mV) || \
                                     ((THRESHOLD) == LPC_2200_mV) || \
                                     ((THRESHOLD) == LPC_2400_mV) || \
                                     ((THRESHOLD) == LPC_2800_mV) || \
                                     ((THRESHOLD) == LPC_3000_mV) || \
                                     ((THRESHOLD) == LPC_3200_mV)) //!< Check if the input parameter is valid.

/** End of group 87x3e_LPC_Threshold
      * @}
      */

/** @defgroup 87x3e_LPC_Channel LPC Channel
  * @{
  */
#define IS_LPC_CHANNEL(CHANNEL) ((CHANNEL) <= 0x07) //!< LPC channel is less than or equal to 0x7.

/** End of group 87x3e_LPC_Channel
      * @}
      */

/** @defgroup 87x3e_LPC_Edge LPC Output Edge
  * @{
  */
#define LPC_Vin_Below_Vth               ((uint32_t)0x0000) //!< The input voltage is below threshold voltage.
#define LPC_Vin_Over_Vth                ((uint32_t)0x0001) //!< The input voltage is over threshold voltage.

#define IS_LPC_EDGE(EDGE) (((EDGE) == LPC_Vin_Below_Vth) || \
                           ((EDGE) == LPC_Vin_Over_Vth)) //!< Check if the input parameter is valid.

/** End of group 87x3e_LPC_Edge
      * @}
      */

/** @defgroup 87x3e_LPC_interrupts_definition LPC Interrupts Definition
      * @{
      */
#define LPC_INT_VOLTAGE_COMP                (LPC_SRC_INT_EN_Msk) //!< LPC voltage detection interrupt. Triggered when the input voltage is higher or lower than the threshold voltage, depending on the setting of \ref x3e_LPC_Edge.
#define LPC_INT_COUNT_COMP                  (LPC_EVENT_EN_Msk) //!< LPC counter comparator interrupt. Triggered when the LPC counter value reaches the LPC comparison value.

#define IS_LPC_CONFIG_INT(INT) (((INT) == LPC_INT_VOLTAGE_COMP) || \
                                ((INT) == LPC_INT_COUNT_COMP)) //!< Check if the input parameter is valid.
#define IS_LPC_CLEAR_INT(INT) ((INT) == LPC_INT_COUNT_COMP) //!< Check if the input parameter is valid.

/** End of group 87x3e_LPC_interrupts_definition
      * @}
      */


/** End of group 87x3e_LPC_Exported_Constants
      * @}
      */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_LPC_Exported_Functions LPC Exported Functions
  * @{
  */


/**
 *
 * \brief   Initializes LPC peripheral according to
 *          the specified parameters in LPC_InitStruct.
 *
 * \param[in] LPC_InitStruct: Pointer to a \ref LPC_InitTypeDef structure that contains
 *            the configuration information for the specified LPC peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LPC_CAPTURE_PIN         ADC_0 //ADC_0 as input
 * void driver_lpc_init(void)
 * {
 *     LPC_InitTypeDef LPC_InitStruct;
 *
 *     LPC_StructInit(&LPC_InitStruct);
 *     LPC_InitStruct.LPC_Channel   = LPC_CAPTURE_PIN;
 *     LPC_InitStruct.LPC_Edge      = LPC_Vin_Over_Vth;
 *     LPC_InitStruct.LPC_Threshold = LPC_400_mV;
 *     LPC_Init(&LPC_InitStruct);
 * }
 * \endcode
 */
void LPC_Init(LPC_InitTypeDef *LPC_InitStruct);


/**
 *
 * \brief  Enable or disable LPC peripheral.
 *
 * \param[in] NewState: New state of LPC peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: Power on the LPC peripheral.
 *            - DISABLE: Power off the LPC peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_lpc_init(void)
 * {
 *     LPC_Cmd(ENABLE);
 * }
 * \endcode
 */
void LPC_Cmd(FunctionalState NewState);


/**
 *
 * \brief   Start or stop the LPC counter.
 *
 * \param[in] NewState: New state of the LPC counter.
 *            This parameter can be one of the following values:
 *            - ENABLE: Start LPCOMP counter.
 *            - DISABLE: Stop LPCOMP counter.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lpc_init(void)
 * {
 *     LPC_CounterCmd(ENABLE);
 * }
 * \endcode
 */
void LPC_CounterCmd(FunctionalState NewState);


/**
 *
 * \brief   Reset the LPC counter.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LPC_COMP_VALUE          0x4
 * void driver_lpc_init(void)
 * {
 *     LPC_CounterReset();
 *     LPC_WriteComparator(LPC_COMP_VALUE);
 *     LPC_CounterCmd(ENABLE);
 * }
 * \endcode
 */
void LPC_CounterReset(void);


/**
 *
 * \brief  Fills each LPC_InitStruct member with its default value.
 *
 * \note   The default settings for the LPC_InitStruct member are shown in the following table:
 *         | LPC_InitStruct Member | Default Value            |
 *         |:---------------------:|:------------------------:|
 *         | LPC_Channel           | \ref ADC_0               |
 *         | LPC_Edge              | \ref LPC_Vin_Below_Vth   |
 *         | LPC_Threshold         | \ref LPC_200_mV          |
 *
 * \param[in]  LPC_InitStruct : Pointer to a \ref LPC_InitTypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LPC_CAPTURE_PIN         ADC_0 //ADC_0 as input
 * void driver_lpc_init(void)
 * {
 *     LPC_InitTypeDef LPC_InitStruct;
 *
 *     LPC_StructInit(&LPC_InitStruct);
 *     LPC_InitStruct.LPC_Channel   = LPC_CAPTURE_PIN;
 *     LPC_InitStruct.LPC_Edge      = LPC_Vin_Over_Vth;
 *     LPC_InitStruct.LPC_Threshold = LPC_400_mV;
 *     LPC_Init(&LPC_InitStruct);
 * }
 * \endcode
 */
void LPC_StructInit(LPC_InitTypeDef *LPC_InitStruct);


/**
 *
 * \brief  Configure LPCOMP counter's comparator value.
 *
 * \param[in]  data: LPCOMP counter's comparator value, which can be 0 to 0xfff.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LPC_COMP_VALUE          0x4
 * void driver_lpc_init(void)
 * {
 *     LPC_WriteComparator(LPC_COMP_VALUE);
 *     LPC_CounterCmd(ENABLE);
 * }
 * \endcode
 */
void LPC_WriteComparator(uint32_t data);


/**
 *
 * \brief  Read LPCOMP comparator value.
 *
 * \return  LPCOMP comparator value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LPC_COMP_VALUE          0x4
 * void LPC_Handler(void)
 * {
 *     if (LPC_GetINTStatus(LPC_INT_COUNT_COMP) == SET)
 *     {
 *         LPC_WriteComparator(LPC_ReadComparator() + LPC_COMP_VALUE);
 *     }
 * }
 * \endcode
 */
uint16_t LPC_ReadComparator(void);


/**
 *
 * \brief  Read LPC counter value.
 *
 * \return  LPCOMP comparator value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LPC_COMP_VALUE          0x4
 * void lpc_rtc_handler(void)
 * {
 *     if (LPC_GetINTStatus(LPC_INT_COUNT_COMP) == SET)
 *     {
 *         lpc_counter_value = LPC_ReadCounter();
 *         LPC_WriteComparator(lpc_counter_value + LPC_COMP_VALUE);
 *         LPC_ClearINTPendingBit(LPC_INT_COUNT_COMP);
 *     }
 * }
 * \endcode
 */
uint16_t LPC_ReadCounter(void);


/**
 *
 * \brief  Enables or disables the specified LPC interrupts.
 *
 * \param[in] LPC_INT: Specifies the LPC interrupt to be enabled or disabled \ref x3e_LPC_interrupts_definition.
 *            This parameter can be one of the following values:
 *            - LPC_INT_VOLTAGE_COMP: LPC voltage detection interrupt. Triggered when the input voltage is higher or lower than the threshold voltage, depending on the setting of \ref x3e_LPC_Edge.
 *            - LPC_INT_COUNT_COMP: LPC counter comparator interrupt. Triggered when the LPC counter value reaches the LPC comparison value.
 * \param[in] NewState: New state of the specified LPC interrupt.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified LPC interrupts.
 *            - DISABLE: Disable the specified LPC interrupts.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LPC_CAPTURE_PIN         ADC_0 //ADC_0 as input
 * #define LPC_COMP_VALUE          0x4
 * void driver_lpc_init(void)
 * {
 *     LPC_INTConfig(LPC_INT_COUNT_COMP, ENABLE);
 *     RTC_CpuNVICEnable(ENABLE);
 *     RamVectorTableUpdate(RTC_VECTORn, lpc_rtc_handler);
 *
 *     NVIC_InitTypeDef NVIC_InitStruct;
 *     NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
 *     NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
 *     NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
 *     NVIC_Init(&NVIC_InitStruct);
 * }
 * \endcode
 */
void LPC_INTConfig(uint32_t LPC_INT, FunctionalState NewState);


/**
 *
 * \brief   Clear the specified LPC interrupt pending bit.
 *
 * \param[in] LPC_INT: Specifies the LPC interrupt to clear \ref x3e_LPC_interrupts_definition.
 *            This parameter can be one of the following values:
 *            - LPC_INT_VOLTAGE_COMP: LPC voltage detection interrupt. Triggered when the input voltage is higher or lower than the threshold voltage, depending on the setting of \ref x3e_LPC_Edge.
 *            - LPC_INT_COUNT_COMP: LPC counter comparator interrupt. Triggered when the LPC counter value reaches the LPC comparison value.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LPC_COMP_VALUE          0x4
 * void lpc_rtc_handler(void)
 * {
 *     if (LPC_GetINTStatus(LPC_INT_COUNT_COMP) == SET)
 *     {
 *         LPC_ClearINTPendingBit(LPC_INT_COUNT_COMP);
 *     }
 * }
 * \endcode
 */
void LPC_ClearINTPendingBit(uint32_t LPC_INT);


/**
 *
 * \brief  Checks whether the specified LPC interrupt is set or not.
 *
 * \param[in] LPC_INT: Specifies the LPC interrupt to check \ref x3e_LPC_interrupts_definition.
 *            This parameter can be one of the following values:
 *            - LPC_INT_COUNT_COMP: LPC counter comparator interrupt. Triggered when the LPC counter value reaches the LPC comparison value.
 *
 * \return The new state of LPC_INT.
 * \retval SET: The LPC interrupt status has been set.
 * \retval RESET: The LPC interrupt status has been reset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LPC_COMP_VALUE          0x4
 * void lpc_rtc_handler(void)
 * {
 *     if (LPC_GetINTStatus(LPC_INT_COUNT_COMP) == SET)
 *     {
 *         //add user code here.
 *     }
 * }
 * \endcode
 */
ITStatus LPC_GetINTStatus(uint32_t LPC_INT);

/**
 *
 * \brief  Configure LPC trigger edge.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  LPC_Edge: Specifies the LPC trigger edge \ref x3e_LPC_Edge.
 *            This parameter can be one of the following values:
 *            - LPC_Vin_Below_Vth: The input voltage is below the threshold voltage.
 *            - LPC_Vin_Over_Vth: The input voltage is above the threshold voltage.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lpc_init(void)
 * {
 *     LPC_SetTriggerEdge(LPC_Vin_Below_Vth);
 * }
 * \endcode
 */
void LPC_SetTriggerEdge(uint32_t LPC_Edge);

#ifdef __cplusplus
}
#endif

#endif /*__RTL876X_LPC_H*/

/** @} */ /* End of group 87x3e_LPC_Exported_Functions */
/** @} */ /* End of group 87x3e_LPC */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

