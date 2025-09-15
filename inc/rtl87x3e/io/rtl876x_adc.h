/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_adc.h
* @brief     The header file of ADC driver.
* @details
* @author    renee
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _RTL876X_ADC_H_
#define _RTL876X_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"
#include "platform_utils.h"
#include "compiler_abstraction.h"

/** @addtogroup 87x3e_ADC ADC
  * @brief ADC driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup 87x3e_ADC_Exported_Types ADC Exported Types
  * @{
  */


/**
 * @brief ADC initialize parameters.
 *
 */
typedef struct
{

    uint8_t adcClock;                      /*!< Specifies the ADC sample clock. ADC sample period = (adcClock+1) cycles from 10MHz.
                                                    This parameter can be a value of 0x1~0xff. */

    uint8_t adcConvertTimePeriod;          /*!< Specifies the ADC sample convert time.
                                                    This parameter can be a value of @ref x3e_ADC_CONVERT_TIME. */

    uint8_t adcFifoThd;                     /*!< Specifies the ADC FIFO threshold to trigger interrupt @ref ADC_INT_FIFO_TH.
                                                    This parameter can be a value of 0 to 32. */

    uint8_t adcBurstSize;                   /*!< Specifies the ADC FIFO burst size to trigger GDMA.
                                                    This parameter can be a value of 0 to 32. */

    uint16_t adcFifoOverWritEn;             /*!< Specifies if over write FIFO when FIFO overflow.
                                                    This parameter can be a value of @ref  x3e_ADC_over_write_enable. */

    uint16_t dataLatchEdge;                 /*!< Specifies ADC data latch mode.
                                                    This parameter can be a value of @ref x3e_ADC_Latch_Data_Edge. */

    uint16_t fifoOREn;                      /*!< This is a deprecated parameter. */

    uint16_t schIndex[16];                  /*!< Specifies ADC mode and channel for schedule table.
                                                    This parameter can be a value of @ref x3e_ADC_schedule_table. */

    uint16_t bitmap;                        /*!< Specify whether the schedule table is enabled.
                                                  Each bit in the 16-bit bitmap corresponds to a schedule index from schIndex[0] to schIndex[15].
                                                  If a bit is set to 1, the corresponding schedule index is enabled; if it's 0, the schedule index is disabled.
                                                  Given bitmap 0x0003, in binary it is 16'b0000000000000011. This means:
                                                  - The schIndex[0] is enabled (bit 0 is 1).
                                                  - The schIndex[1] is enabled (bit 1 is 1).
                                                  - The schIndex[2] to schIndex[15] are disabled (bits 2 to 15 are 0). */

    uint8_t timerTriggerEn;                 /*!< To control whether the TIM7 peripheral triggers ADC one-shot mode sampling.
                                                    This parameter can be a value of ENABLE or DISABLE. */

    uint32_t dataAligned;                   /*!< Specifies ADC data storage format.
                                                   This parameter can be a value of @ref x3e_ADC_Data_Align. */

    uint8_t dataWriteToFifo;                /*!< Enable or disable the function that assign ADC output data write into FIFO in one shot mode.
                                                     This parameter can be a value of ENABLE or DISABLE. */

    uint32_t dataMinusEn;                   /*!< Enable or disable the function that data minuses the offset before writes to register/FIFO.
                                                    This parameter can be a value of @ref x3e_ADC_Data_Minus. */

    uint8_t dataMinusOffset;                /*!< Offset to be minused from ADC raw data latched. This parameter can be a value of 0 to 4095. */

    uint32_t pwrmode;                       /*!< Specifies ADC power on mode.
                                                    This parameter can be a value of @ref x3e_ADC_Power_mode. */

    uint16_t datalatchDly;                  /*!< Specifiy the delay time interval that postpone the ADC controller to latched the output code. The time unit is based on ADC controller system clock (40MHz).
                                                    This parameter can be a value of 0x1 to 0x7. */

    uint16_t adcRG2X0Dly;                   /*!< Specifies the power on delay time selection of RG2X_AUXADC[0].
                                                    This parameter can be a value of @ref x3e_ADC_RG2X_0_Delay_Time. */

    uint16_t adcRG0X1Dly;                   /*!< Specifies the power on delay time selection of RG0X_AUXADC[1].
                                                    This parameter can be a value of @ref x3e_ADC_RG0X_1_Delay_Time. */

    uint16_t adcRG0X0Dly;                   /*!< Specifies the power on delay time selection of RG0X_AUXADC[0].
                                                    This parameter can be a value of @ref x3e_ADC_RG0X_0_Delay_Time. */

    uint32_t adc_fifo_stop_write;           /*!< Stop FIFO from writing data. This bit will be asserted automatically as FIFO overflow,
                                                    (not automatically when ADC_FIFO_OVER_WRITE_ENABLE), need to be cleared in order to write
                                                    data again. This will not stop overwrite mode.
                                                        This parameter can be a value of @ref x3e_ADC_FIFO_Stop_Write */

    uint32_t adcPowerAlwaysOnCmd;           /*!< Enable or disable the power always on function.
                                                    This parameter can be a value of @ref x3e_ADC_Power_Always_On_Cmd. */

} ADC_InitTypeDef;

/** End of Group 87x3e_ADC_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_ADC_Exported_Constants ADC Exported Constants
  * @{
  */

#define IS_ADC_PERIPH(PERIPH) ((PERIPH) == ADC) //!< ADC peripherals can select ADC.

#define IS_ADC_CHANNEL(ch)      (ch<=ADC_3) //!< ADC external channel can select ADC_0, ADC_1, ADC_2, ADC_3.

/** @defgroup 87x3e_ADC_Interrupts_Definition ADC Interrupts Definition
  * @{
  */

#define ADC_INT_FIFO_RD_REQ                             ((uint32_t)(1 << 0)) //!< ADC GDMA request interrupt: When the FIFO data level reaches the GDMA threshold level (adcBurstSize), this interrupt is triggered.
#define ADC_INT_FIFO_RD_ERR                             ((uint32_t)(1 << 1)) //!< ADC FIFO read error interrupt: This interrupt is triggered when an attempt is made to read from an empty FIFO.
#define ADC_INT_FIFO_TH                                 ((uint32_t)(1 << 2)) //!< ADC FIFO threshold interrupt: When the FIFO data number is greater than or equal to the threshold level (adcFifoThd), this interrupt is triggered.
#define ADC_INT_FIFO_OVERFLOW                           ((uint32_t)(1 << 3)) //!< ADC FIFO full interrupt: When the FIFO is full, this interrupt is triggered.
#define ADC_INT_ONE_SHOT_DONE                           ((uint32_t)(1 << 4)) //!< ADC one shot mode done interrupt: When the ADC conversion is done, this interrupt is triggered.

#define IS_ADC_IT(IT) ((((IT) & 0xFFFFFFE0) == 0x00) && ((IT) != 0x00)) //!< Check whether is the ADC interrupt.

#define IS_ADC_GET_IT(IT) (((IT) == ADC_INT_FIFO_RD_REQ) || ((IT) == ADC_INT_FIFO_RD_ERR)\
                           || ((IT) == ADC_INT_FIFO_TH) || ((IT) == ADC_INT_FIFO_OVERFLOW)\
                           || ((IT) == ADC_INT_ONE_SHOT_DONE)) //!< Check whether is the ADC interrupt.
/** End of Group 87x3e_ADC_Interrupts_Definition
  * @}
  */
/** @defgroup 87x3e_ADC_CONVERT_TIME   ADC Convert Time
  * @{
  */
#define ADC_CONVERT_TIME_500NS          0 //!< ADC convert time is 500ns.
#define ADC_CONVERT_TIME_700NS          1 //!< ADC convert time is 700ns.
#define ADC_CONVERT_TIME_900NS          2 //!< ADC convert time is 900ns.
#define ADC_CONVERT_TIME_1100NS         3 //!< ADC convert time is 1100ns.
#define IS_ADC_CONVERT_TIME(TIME) (((TIME) == ADC_CONVERT_TIME_500NS)\
                                   || ((TIME) == ADC_CONVERT_TIME_700NS)\
                                   || ((TIME) == ADC_CONVERT_TIME_900NS)\
                                   || ((TIME) == ADC_CONVERT_TIME_1100NS)) //!< Check whether is the ADC convert time.

/** End of Group 87x3e_ADC_CONVERT_TIME
  * @}
  */

/** @defgroup 87x3e_ADC_operation_Mode   ADC Operation Mode
  * @{
  */
#define ADC_Continuous_Mode                         ((uint16_t)(1 << 0)) //!< ADC continuous mode.
#define ADC_One_Shot_Mode                           ((uint16_t)(1 << 1)) //!< ADC one shot mode.

#define IS_ADC_MODE(MODE) (((MODE) == ADC_Continuous_Mode) || ((MODE) == ADC_One_Shot_Mode)) //!< Check whether is the ADC operation mode.

/** End of Group 87x3e_ADC_operation_Mode
  * @}
  */

/** @defgroup 87x3e_ADC_Latch_Data_Edge ADC Latch Data Edge
  * @{
  */

#define ADC_Latch_Data_Positive                           ((uint16_t)(0 << 2)) //!< ADC latch adc data at positive clock edge.
#define ADC_Latch_Data_Negative                           ((uint16_t)(1 << 2)) //!< ADC latch adc data at negative clock edge.

#define IS_ADC_LATCH_MODE(MODE) (((MODE) == ADC_Latch_Data_Positive) || ((MODE) == ADC_Latch_Data_Negative)) //!< Check whether is the ADC latch data edge.

/** End of Group 87x3e_ADC_Latch_Data_Edge
  * @}
  */

/** @defgroup 87x3e_ADC_schedule_table ADC Channel and Mode
  * @{
  */
#define SCHEDULE_TABLE(index)           index //!< ADC schedule table index.

#define EXT_SINGLE_ENDED(index)                     ((uint16_t)((0x00 << 3) | (index))) /**< External single-ended mode. */

#define INTERNAL_VBAT_MODE                          ((uint16_t)((0x02 << 3) | 0x00)) //!< Internal VBAT mode.
#define INTERNAL_VADPIN_MODE                        ((uint16_t)((0x02 << 3) | 0x01)) //!< Internal VADPIN mode.

#define IS_ADC_SCHEDULE_INDEX_CONFIG(CONFIG) ((((CONFIG) & 0xFFE0) ==0) && ((((~(CONFIG)) & (0x18)) !=0)\
                                                                            || ((CONFIG) == INTERNAL_VBAT_MODE) || ((CONFIG) == INTERNAL_VADPIN_MODE))) //!< Check whether is the ADC channel and mode.

/** End of Group 87x3e_ADC_schedule_table
  * @}
  */

/** @defgroup 87x3e_ADC_Data_Align ADC Data Align
  * @{
  */

#define ADC_DATA_LSB                                 (uint32_t)(0 << 30) //!< ADC data storage format is LSB.
#define ADC_DATA_MSB                                 (uint32_t)(1 << 30) //!< ADC data storage format is MSB.

#define IS_ADC_DATA_ALIGN(DATA_ALIGN) (((DATA_ALIGN) == ADC_DATA_LSB) || ((DATA_ALIGN) == ADC_DATA_MSB)) //!< Check whether is the ADC data align.
/** End of Group 87x3e_ADC_Data_Align
  * @}
  */

/** @defgroup 87x3e_ADC_Clock_Config ADC Sample Clock
  * @brief ADC sample clock frequency default value as follow, other vaule (0x1~0xff) is also support if user wanted.
  * @{
  */

#define ADC_CLK_625K                                 (0x0f) //!< ADC sample clock frequency is 625KHz.
#define ADC_CLK_312_5K                               (0x1f) //!< ADC sample clock frequency is 312.5KHz.
#define ADC_CLK_156_25K                              (0x3f) //!< ADC sample clock frequency is 156.25KHz.
#define ADC_CLK_78_125K                              (0x7f) //!< ADC sample clock frequency is 78.125KHz.
#define ADC_CLK_39K                                  (0xff) //!< ADC sample clock frequency is 39KHz.

/** End of Group 87x3e_ADC_Clock_Config
  * @}
  */

/** @defgroup 87x3e_ADC_AVG_Select ADC Averaged Select
  * @{
  */

/**
 * @brief ADC hardware averaged select.
 */
typedef enum _T_ADC_DTAT_AVG_SEL
{
    ADC_DTAT_AVG_SEL_BY2, //!< ADC data averaged by 2.
    ADC_DTAT_AVG_SEL_BY4, //!< ADC data averaged by 4.
    ADC_DTAT_AVG_SEL_BY8, //!< ADC data averaged by 8.
    ADC_DTAT_AVG_SEL_BY16, //!< ADC data averaged by 16.
    ADC_DTAT_AVG_SEL_BY32, //!< ADC data averaged by 32.
    ADC_DTAT_AVG_SEL_BY64, //!< ADC data averaged by 64.
    ADC_DTAT_AVG_SEL_BY128, //!< ADC data averaged by 128.
    ADC_DTAT_AVG_SEL_BY256, //!< ADC data averaged by 256.
    ADC_DTAT_AVG_SEL_MAX, //!< The maximum value that can be selected for ADC data average times.
} T_ADC_DTAT_AVG_SEL;

/** End of Group 87x3e_ADC_AVG_Select
  * @}
  */

/** @defgroup 87x3e_ADC_Data_Minus ADC Data Minus
  * @brief ADC data minuses the offset before writes to register/FIFO.
  * @{
  */

#define ADC_DATA_MINUS_DIS                                 (uint32_t)(0 << 31) //!< Disable the function that data minuses the offset before writing to register/FIFO.
#define ADC_DATA_MINUS_EN                                  ((uint32_t)1 << 31) //!< Enable the function that data minuses the offset before writing to register/FIFO.

#define IS_ADC_DATA_MINUS(DATA_MINUS) (((DATA_MINUS) == ADC_DATA_MINUS_DIS) || ((DATA_MINUS) == ADC_DATA_MINUS_EN)) //!< Check whether is the ADC data minus.


/** End of Group 87x3e_ADC_Data_Minus
  * @}
  */

/** @defgroup 87x3e_ADC_Burst_Size ADC Burst Size
  * @{
  */

#define IS_ADC_BURST_SIZE_CONFIG(CONFIG) ((CONFIG) <= 0x20) //!< ADC burst size is between 0 and 0x20.

/** End of Group 87x3e_ADC_Burst_Size
  * @}
  */

/** @defgroup 87x3e_ADC_FIFO_Threshold ADC FIFO Threshold
  * @{
  */

#define IS_ADC_FIFO_THRESHOLD(THD) ((THD) <= 0x20) //!< ADC FIFO threshold is between 0 and 0x20.

/** End of Group 87x3e_ADC_FIFO_Threshold
  * @}
  */

/** @defgroup 87x3e_ADC_Power_mode ADC Power Mode
  * @{
  */

#define ADC_POWER_MANNUAL                             ((uint32_t)(1 << 19)) //!<  ADC power on is manually controlled.
#define ADC_POWER_AUTO                                ((uint32_t)(0 << 19)) //!<  ADC power on is automatically controlled.

#define IS_ADC_POWER_MODE(MODE) (((MODE) == ADC_POWER_MANNUAL) || ((MODE) == ADC_POWER_AUTO)) //!< Check whether is the ADC power mode.

/** End of Group 87x3e_ADC_Power_mode
  * @}
  */

/** @defgroup 87x3e_ADC_RG2X_0_Delay_Time ADC Power On Delay Time of RG2X_AUXADC[0]
  * @{
  */

#define ADC_RG2X_0_DELAY_10_US                      ((uint32_t)(0 << 4)) //!< ADC power on delay time of RG2X_AUXADC[0] is 30us.
#define ADC_RG2X_0_DELAY_20_US                      ((uint32_t)(1 << 4)) //!< ADC power on delay time of RG2X_AUXADC[0] is 60us.
#define ADC_RG2X_0_DELAY_40_US                      ((uint32_t)(2 << 4)) //!< ADC power on delay time of RG2X_AUXADC[0] is 120us.
#define ADC_RG2X_0_DELAY_80_US                      ((uint32_t)(3 << 4)) //!< ADC power on delay time of RG2X_AUXADC[0] is 240us.

#define IS_ADC_RG2X_0_DELAY_TIME(TIME) (((TIME) == ADC_RG2X_0_DELAY_10_US) || ((TIME) == ADC_RG2X_0_DELAY_20_US)\
                                        || ((TIME) == ADC_RG2X_0_DELAY_40_US) || ((TIME) == ADC_RG2X_0_DELAY_80_US)) //!< Check whether is the ADC power on delay time of RG2X_AUXADC[0].

/** End of Group 87x3e_ADC_RG2X_0_Delay_Time
  * @}
  */

/** @defgroup 87x3e_ADC_RG0X_1_Delay_Time ADC Power On Delay Time of RG0X_AUXADC[1]
  * @{
  */

#define ADC_RG0X_1_DELAY_40_US                      ((uint16_t)(0 << 2)) //!< ADC power on delay time of RG0X_AUXADC[1] is 30us.
#define ADC_RG0X_1_DELAY_80_US                      ((uint16_t)(1 << 2)) //!< ADC power on delay time of RG0X_AUXADC[1] is 60us.
#define ADC_RG0X_1_DELAY_160_US                     ((uint16_t)(2 << 2)) //!< ADC power on delay time of RG0X_AUXADC[1] is 120us.
#define ADC_RG0X_1_DELAY_320_US                     ((uint16_t)(3 << 2)) //!< ADC power on delay time of RG0X_AUXADC[1] is 240us.

#define IS_ADC_RG0X_1_DELAY_TIME(TIME) (((TIME) == ADC_RG0X_1_DELAY_40_US) || ((TIME) == ADC_RG0X_1_DELAY_80_US)\
                                        || ((TIME) == ADC_RG0X_1_DELAY_160_US) || ((TIME) == ADC_RG0X_1_DELAY_320_US)) //!< Check whether is the ADC power on delay time of RG0X_AUXADC[1].

/** End of Group 87x3e_ADC_RG0X_1_Delay_Time
  * @}
  */

/** @defgroup 87x3e_ADC_RG0X_0_Delay_Time ADC Power On Delay Time of RG0X_AUXADC[0]
  * @{
  */

#define ADC_RG0X_0_DELAY_30_US                       ((uint16_t)(0 << 0)) //!< ADC power on delay time of RG0X_AUXADC[0] is 30us.
#define ADC_RG0X_0_DELAY_60_US                       ((uint16_t)(1 << 0)) //!< ADC power on delay time of RG0X_AUXADC[0] is 60us.
#define ADC_RG0X_0_DELAY_120_US                      ((uint16_t)(2 << 0)) //!< ADC power on delay time of RG0X_AUXADC[0] is 120us.
#define ADC_RG0X_0_DELAY_240_US                      ((uint16_t)(3 << 0)) //!< ADC power on delay time of RG0X_AUXADC[0] is 240us.

#define IS_ADC_RG0X_0_DELAY_TIME(TIME) (((TIME) == ADC_RG0X_0_DELAY_30_US) || ((TIME) == ADC_RG0X_0_DELAY_60_US)\
                                        || ((TIME) == ADC_RG0X_0_DELAY_120_US) || ((TIME) == ADC_RG0X_0_DELAY_240_US)) //!< Check whether is the ADC power on delay time of RG0X_AUXADC[0].

/** End of Group 87x3e_ADC_RG0X_0_Delay_Time
  * @}
  */

/** @defgroup 87x3e_ADC_one_shot_fifo ADC One Shot FIFO
  * @{
  */

#define ADC_ONE_SHOT_FIFO_ENABLE                             ((uint32_t)(1 << 27)) //!< Enable the function that assigns ADC output data to be written into FIFO in one shot mode.

/** End of Group 87x3e_ADC_one_shot_fifo
  * @}
  */

/** @defgroup 87x3e_ADC_over_write_enable ADC FIFO Over Write
  * @{
  */

#define ADC_FIFO_OVER_WRITE_ENABLE                             ((uint32_t)(1 << 13)) //!< Enable ADC FIFO overwrite function.
#define ADC_FIFO_OVER_WRITE_DISABLE                            ((uint32_t)(0 << 13)) //!< Disable ADC FIFO overwrite function.

#define IS_ADC_OVERWRITE_MODE(MODE) (((MODE) == ADC_FIFO_OVER_WRITE_ENABLE) || ((MODE) == ADC_FIFO_OVER_WRITE_DISABLE)) //!< Check whether is the status of ADC overwrite function.

/** End of Group 87x3e_ADC_over_write_enable
  * @}
  */


/** @defgroup 87x3e_ADC_FIFO_Stop_Write  ADC FIFO Stop Write
  * @{
  */

#define ADC_FIFO_STOP_WRITE                             ((uint32_t)(1 << 21)) //!< Stop FIFO from writing data. This bit will be asserted automatically as FIFO overflow, (not automatically when ADC_FIFO_OVER_WRITE_ENABLE), need to be cleared in order to write data again. This will not stop overwrite mode.

/** End of  87x3e_ADC_FIFO_Stop_Write
  * @}
  */
/** @defgroup 87x3e_ADC_Power_Always_On_Cmd ADC Power Always on CMD
  * @{
  */

#define ADC_POWER_ALWAYS_ON_ENABLE                          ((uint32_t)(1 << 15)) //!< Enable the power always on function, which power on ADC with the power on sequence and disable auto power on/off function.
#define ADC_POWER_ALWAYS_ON_DISABLE                         ((uint32_t)(0)) //!< Disable the power always on function.

#define IS_ADC_POWER_ALWAYS_ON(CMD) (((CMD) == ADC_POWER_ALWAYS_ON_ENABLE) || ((CMD) == ADC_POWER_ALWAYS_ON_DISABLE)) //!< Check whether is the status of ADC the power always on function.

/** End of Group 87x3e_ADC_Power_Always_On_Cmd
  * @}
  */


/** End of Group 87x3e_ADC_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_ADC_Exported_Functions ADC Exported Functions
  * @{
  */

/**
 *
 * \brief  Initializes the ADC peripheral according to the specified
 *         parameters in the ADC_InitStruct.
 *
 * \param[in]  ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in]  ADC_InitStruct: Pointer to a ADC_InitTypeDef structure that
 *             contains the configuration information for the specified ADC peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);
 *
 *     ADC_InitTypeDef adc_init_struct;
 *     ADC_StructInit(&adc_init_struct);
 *     adc_init_struct.adcClock = ADC_CLK_39K;
 *     adc_init_struct.schIndex[0] = EXT_SINGLE_ENDED(1);
 *     adc_init_struct.schIndex[1] = INTERNAL_VBAT_MODE;
 *     adc_init_struct.schIndex[2] = INTERNAL_VADPIN_MODE;
 *     adc_init_struct.bitmap = 0x07;
 *     ADC_Init(ADC, &adc_init_struct);
 * }
 * \endcode
 */
extern void (*ADC_Init)(ADC_TypeDef *ADCx, ADC_InitTypeDef *ADC_InitStruct);


/**
 *
 * \brief   Disable the ADCx peripheral clock, and restore registers to their default values.
 *
 * \param[in]  ADCx: Selected ADC peripheral, which can be ADC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     ADC_DeInit(ADC);
 * }
 * \endcode
 */
extern void ADC_DeInit(ADC_TypeDef *ADCx);

/**
 *
 * \brief  Fills each ADC_InitStruct member with its default value.
 *
 * \note   The default settings for the ADC_InitStruct member are shown in the following table:
 *         | ADC_InitStruct Member   | Default Value                   |
 *         |:-----------------------:|:-------------------------------:|
 *         | adcConvertTimePeriod    | \ref ADC_CONVERT_TIME_500NS     |
 *         | adcClock                | \ref ADC_CLK_78_125K            |
 *         | dataLatchEdge           | \ref ADC_Latch_Data_Positive    |
 *         | adcFifoOverWritEn       | \ref ADC_FIFO_OVER_WRITE_ENABLE |
 *         | timerTriggerEn          | DISABLE                         |
 *         | dataWriteToFifo         | DISABLE                         |
 *         | dataAligned             | \ref ADC_DATA_LSB               |
 *         | dataMinusEn             | \ref ADC_DATA_MINUS_DIS         |
 *         | dataMinusOffset         | 0                               |
 *         | schIndex[16]            | 0                               |
 *         | bitmap                  | 0x0                             |
 *         | adcFifoThd              | 0x06                            |
 *         | adcBurstSize            | 0x1                             |
 *         | adc_fifo_stop_write     | 0                               |
 *         | pwrmode                 | \ref ADC_POWER_AUTO             |
 *         | datalatchDly            | 0x1                             |
 *         | adcRG2X0Dly             | \ref ADC_RG2X_0_DELAY_40_US     |
 *         | adcRG0X1Dly             | \ref ADC_RG0X_1_DELAY_40_US     |
 *         | adcRG0X0Dly             | \ref ADC_RG0X_0_DELAY_30_US     |
 *         | adcPowerAlwaysOnCmd     | \ref ADC_POWER_ALWAYS_ON_DISABLE|
 *
 * \param[in]  ADC_InitStruct: Pointer to a ADC_InitTypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);
 *
 *     ADC_InitTypeDef adcInitStruct;
 *     ADC_StructInit(&adcInitStruct);
 *     adcInitStruct.schIndex[0]         = EXT_SINGLE_ENDED(0);;
 *     adcInitStruct.schIndex[1]         = EXT_SINGLE_ENDED(1);
 *     adcInitStruct.schIndex[2]         = INTERNAL_VBAT_MODE;
 *     adcInitStruct.schIndex[3]         = INTERNAL_VADPIN_MODE;
 *     adcInitStruct.bitmap              = 0x0f;
 *     ADC_Init(ADC, &adcInitStruct);
 * }
 * \endcode
 */
extern void (*ADC_StructInit)(ADC_InitTypeDef *ADC_InitStruct);


/**
 *
 * \brief  Enable or disable the specified ADC interrupts.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] ADC_IT: Specifies the ADC interrupt sources to be enabled or disabled, refer to \ref x3e_ADC_Interrupts_Definition.
 *            This parameter can be one of the following values:
 *            - ADC_INT_ONE_SHOT_DONE: ADC one shot mode done interrupt.
 *            - ADC_INT_FIFO_OVERFLOW: ADC FIFO full interrupt.
 *            - ADC_INT_FIFO_RD_REQ: ADC GDMA request interrupt.
 *            - ADC_INT_FIFO_RD_ERR: ADC FIFO read error interrupt.
 *            - ADC_INT_FIFO_TH: ADC FIFO threshold interrupt.
 * \param[in] newState: New state of the specified ADC interrupts.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified ADC interrupts.
 *            - DISABLE: Disable the specified ADC interrupts.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     ADC_INTConfig(ADC, ADC_INT_ONE_SHOT_DONE, ENABLE);
 *
 *     NVIC_InitTypeDef NVIC_InitStruct;
 *     NVIC_InitStruct.NVIC_IRQChannel = ADC_IRQn;
 *     NVIC_InitStruct.NVIC_IRQChannelPriority = 2;
 *     NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
 *     NVIC_Init(&NVIC_InitStruct);
 *
 *     ADC_Cmd(ADC, ADC_One_Shot_Mode, ENABLE);
 * }
 * \endcode
 */
extern void ADC_INTConfig(ADC_TypeDef *ADCx, uint32_t ADC_IT, FunctionalState newState);


/**
 *
 * \brief  Read ADC data according to specific channel.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] ScheduleIndex: Schedule table index, the value can be 0 ~ 15.
 *
 * \return The 12-bit converted ADC data.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void adc_polling_demo(void)
 * {
 *     uint16_t data[3];
 *
 *     //ADC one shot sampling mode, read data from schedule table.
 *     data[0] = ADC_Read(ADC, 0);
 *     data[1] = ADC_Read(ADC, 1);
 *     data[2] = ADC_Read(ADC, 2);
 *     APP_PRINT_INFO3("ADC data[0] = %d, ADC data[1] =%d ADC data[2] =%d",
 *                     data[0], data[1], data[2]);
 * }
 * \endcode
 */
extern uint16_t (*ADC_Read)(ADC_TypeDef *ADCx, uint8_t ScheduleIndex);


/**
 *
 * \brief  Enable or disable the specified ADC peripheral.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] adcMode: The ADC work mode to select, refer to \ref x3e_ADC_operation_Mode.
 *            This parameter can be one of the following values:
 *            - ADC_One_Shot_Mode: ADC one shot mode.
 *            - ADC_Continuous_Mode: ADC continuous mode.
 * \param[in] NewState: New state of the specified ADC peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified ADC peripheral to start sampling.
 *            - DISABLE: Disable the specified ADC peripheral to stop sampling.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     ADC_Cmd(ADC, ADC_One_Shot_Mode, ENABLE);
 * }
 * \endcode
 */
extern void (*ADC_Cmd)(ADC_TypeDef *ADCx, uint8_t adcMode, FunctionalState NewState);


/**
 *
 * \brief  Config ADC schedule table.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] Index: The index of schedule table, the value can be 0 ~ 15.
 * \param[in] adcMode: The ADC channel and mode to select, refer to \ref x3e_ADC_schedule_table.
 *            This parameter can be one of the following values:
 *            - EXT_SINGLE_ENDED(index): Single-ended mode, the input is external channel index.
 *            - INTERNAL_VBAT_MODE: The input is internal battery voltage detection channel.
 *            - INTERNAL_VADPIN_MODE: The input is internal adapter voltage detection channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     uint16_t index = 0;
 *     ADC_SchTableConfig(ADC, index, EXT_SINGLE_ENDED(index));
 * }
 * \endcode
 */
extern void ADC_SchTableConfig(ADC_TypeDef *ADCx, uint16_t Index,
                               uint8_t adcMode);


/**
 *
 * \brief  Get data from ADC FIFO.
 *
 * \param[in]  ADCx: Selected ADC peripheral, which can be ADC.
 * \param[out] outBuf: Buffer to save data read from ADC FIFO.
 * \param[in]  count: Number of data to be read.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     uint8_t fifo_count = 0;
 *     uint16_t data[33] = {0, 0, 0};
 *
 *     fifo_count = ADC_GetFifoLen(ADC);
 *     ADC_GetFifoData(ADC, data, fifo_count);
 * }
 * \endcode
 */
extern void ADC_GetFifoData(ADC_TypeDef *ADCx, uint16_t *outBuf, uint16_t count);


/**
 *
 * \brief  Config ADC high bypass resistance mode.
 *
 * \note   Channels using bypass mode cannot over 0.9V!
 *
 * \param[in] channelNum: The external channel number, which can be 0 ~ 3.
 * \param[in] NewState: New state of the ADC bypass resistor.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the ADC high bypass resistance mode.
 *            - DISABLE: Disable the ADC high bypass resistance mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     ADC_HighBypassCmd(1, ENABLE);
 * }
 * \endcode
 */
extern void (*ADC_HighBypassCmd)(uint8_t channelNum, FunctionalState NewState);


/**
 *
 * \brief  Get ADC conversion result.
 *
 * \param[in] RawData: ADC raw data.
 * \param[in] adcMode: The ADC channel and mode to select, refer to \ref x3e_ADC_schedule_table.
 *            This parameter can be one of the following values:
 *            - EXT_SINGLE_ENDED(index): Single-ended mode, the input is external channel index.
 *            - INTERNAL_VBAT_MODE: The input is internal battery voltage detection channel.
 *            - INTERNAL_VADPIN_MODE: The input is internal adapter voltage detection channel.
 *
 * \return ADC result.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     uint16_t data[3];
 *     int32_t res[3];
 *
 *     data[0] = ADC_Read(ADC, 0);
 *     data[1] = ADC_Read(ADC, 1);
 *     data[2] = ADC_Read(ADC, 2);
 *
 *     //Get conversion results based on data. The unit of the result is mV.
 *     res[0] = ADC_GetRes(data[0], EXT_SINGLE_ENDED(1));
 *     res[1] = ADC_GetRes(data[1], INTERNAL_VBAT_MODE);
 *     res[2] = ADC_GetRes(data[2], INTERNAL_VADPIN_MODE);
 *     APP_PRINT_INFO3("ADC Result[0] = %d, ADC Result[1] =%d Result[2] =%d",
 *                     res[0], res[1], res[2]);
 * }
 * \endcode
 */
extern int32_t (*ADC_GetRes)(uint16_t RawData, uint8_t adcMode);


/**
 *
 * \brief  Get ADC result in high bypass resistance mode.
 *
 * \param[in] RawData: ADC raw data.
 * \param[in] adcMode: The ADC channel and mode to select, refer to \ref x3e_ADC_schedule_table.
 *            This parameter can be one of the following values:
 *            - EXT_SINGLE_ENDED(index): Single-ended mode, the input is external channel index.
 *
 * \return ADC result.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ADC_Handler(void)
 * {
 *     int32_t data[2];
 *     int32_t res[2];
 *
 *     data[0] = ADC_Read(ADC, 0);
 *     data[1] = ADC_Read(ADC, 1);
 *     res[0] = ADC_GetRes(data[0], EXT_SINGLE_ENDED(0));
 *     res[1] = ADC_GetHighBypassRes(data[1], EXT_SINGLE_ENDED(1));
 *
 * }
 * \endcode
 */
extern int32_t (*ADC_GetHighBypassRes)(uint16_t RawData, uint8_t adcMode);


/**
 *
 * \brief  Enable or disable the ADC hardware average mode.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] NewState: New state of the ADC hardware average mode.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the ADC hardware average mode.
 *            - DISABLE: Disable the ADC hardware average mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     //When hardware average function is enabled, ADC can only use schedule table 0.
 *     adc_init_struct.schIndex[0] = EXT_SINGLE_ENDED(1);
 *     adc_init_struct.bitmap = 0x01;
 *
 *     //ADC hardware average function can only be used in one-shot mode
 *     //and only one schedule can be used.
 *     ADC_HwEvgEn(ADC, ENABLE);
 *     ADC_HwEvgSel(ADC, ADC_DTAT_AVG_SEL_BY32);
 *
 *     ADC_INTConfig(ADC, ADC_INT_ONE_SHOT_DONE, ENABLE);
 *     ADC_Cmd(ADC, ADC_One_Shot_Mode, ENABLE);
 * }
 * \endcode
 */
void ADC_HwEvgEn(ADC_TypeDef *ADCx, FunctionalState Newstate);


/**
 *
 * \brief  Set the hardware average number of times.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] data_sel_by: The hardware average number of times, refer to \ref x3e_ADC_AVG_Select.
 *            - ADC_DTAT_AVG_SEL_BY2: ADC data averaged by 2.
 *            - ADC_DTAT_AVG_SEL_BY4: ADC data averaged by 4.
 *            - ADC_DTAT_AVG_SEL_BY8: ADC data averaged by 8.
 *            - ADC_DTAT_AVG_SEL_BY16: ADC data averaged by 16.
 *            - ADC_DTAT_AVG_SEL_BY32: ADC data averaged by 32.
 *            - ADC_DTAT_AVG_SEL_BY64: ADC data averaged by 64.
 *            - ADC_DTAT_AVG_SEL_BY128: ADC data averaged by 128.
 *            - ADC_DTAT_AVG_SEL_BY256: ADC data averaged by 256.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     //Set the hardware average number of times.
 *     ADC_HwEvgSel(ADC, ADC_DTAT_AVG_SEL_BY32);
 * }
 * \endcode
 */
void ADC_HwEvgSel(ADC_TypeDef *ADCx, T_ADC_DTAT_AVG_SEL data_sel_by);


/**
 *
 * \brief  Read the data after ADC turn on hardware average function.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 *
 * \return The converted data after ADC turn on hardware average.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ADC_Handler(void)
 * {
 *     uint16_t data;
 *     int32_t result;
 *     if (ADC_GetIntFlagStatus(ADC, ADC_INT_ONE_SHOT_DONE) == SET)
 *     {
 *         ADC_ClearINTPendingBit(ADC, ADC_INT_ONE_SHOT_DONE);
 *         data = ADC_HwEvgRead(ADC);
 *         result = ADC_GetRes(data, EXT_SINGLE_ENDED(1));
 *         APP_PRINT_INFO1("ADC Result = %d", result);
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint16_t ADC_HwEvgRead(ADC_TypeDef *ADCx)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));

    return (uint16_t)((ADCx->SCHD0 >> 2) & 0xFFF);
}


/**
 *
 * \brief  Clear the ADC interrupt pending bits.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] ADC_IT: Specifies the interrupt pending bit to clear, refer to \ref x3e_ADC_Interrupts_Definition.
 *            This parameter can be one of the following values:
 *            - ADC_INT_ONE_SHOT_DONE: ADC one shot mode done interrupt.
 *            - ADC_INT_FIFO_OVERFLOW: ADC FIFO full interrupt.
 *            - ADC_INT_FIFO_RD_REQ: ADC GDMA request interrupt.
 *            - ADC_INT_FIFO_RD_ERR: ADC FIFO read error interrupt.
 *            - ADC_INT_FIFO_TH: ADC FIFO threshold interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ADC_Handler(void)
 * {
 *     if (ADC_GetIntFlagStatus(ADC, ADC_INT_ONE_SHOT_DONE) == SET)
 *     {
 *         ADC_ClearINTPendingBit(ADC, ADC_INT_ONE_SHOT_DONE);
 *         //add user code here.
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void ADC_ClearINTPendingBit(ADC_TypeDef *ADCx, uint32_t ADC_IT)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_ADC_IT(ADC_IT));

    ADCx->INTCR |= (ADC_IT << 8);

    return;
}


/**
 *
 * \brief  Check whether the specified ADC interrupt status flag is set.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] ADC_INT_FLAG: Specifies the interrupt status flag to check, refer to \ref x3e_ADC_Interrupts_Definition.
 *            This parameter can be one of the following values:
 *            - ADC_INT_ONE_SHOT_DONE: ADC one shot mode done interrupt flag.
 *            - ADC_INT_FIFO_OVERFLOW: ADC FIFO full interrupt flag.
 *            - ADC_INT_FIFO_RD_REQ: ADC GDMA request interrupt flag.
 *            - ADC_INT_FIFO_RD_ERR: ADC FIFO read error interrupt flag.
 *            - ADC_INT_FIFO_TH: ADC FIFO threshold interrupt flag.
 *
 * \return  The new state of ADC interrupt status flag.
 * \retval SET: The specified ADC interrupt status flag is set.
 * \retval RESET: The specified ADC interrupt status flag is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void adc_demo(void)
 * {
 *     while (ADC_GetIntFlagStatus(ADC, ADC_INT_ONE_SHOT_DONE) == RESET);
 *     ADC_ClearINTPendingBit(ADC, ADC_INT_ONE_SHOT_DONE);
 *     //add user code here.
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE FlagStatus ADC_GetIntFlagStatus(ADC_TypeDef *ADCx, uint32_t ADC_INT_FLAG)
{
    FlagStatus bitstatus = RESET;

    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_ADC_IT(ADC_INT_FLAG));

    if ((ADCx->INTCR & (ADC_INT_FLAG << 16)) != 0)
    {
        bitstatus = SET;
    }

    return bitstatus;
}


/**
 *
 * \brief  Enable or Disable setting ADC schedule table.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] channelMap: Specify whether the schedule table is enabled or disabled.
 * \param[in] NewState: New state of the ADC peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable setting the ADC schedule table.
 *            - DISABLE: Disable setting the ADC schedule table.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void adc_demo(void)
 * {
 *     ADC_SchTableSet(ADC, 0x1, ENABLE);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void ADC_SchTableSet(ADC_TypeDef *ADCx, uint16_t channelMap,
                                            FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState == ENABLE)
    {
        ADCx->SCHCR |= channelMap;
    }
    else
    {
        ADCx->SCHCR &= (~channelMap);
    }

    return;
}


/**
 *
 * \brief  Get one data from ADC FIFO.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 *
 * \return  The data of ADC FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void adc_handler(void)
 * {
 *     uint16_t data_fifo = 0;
 *     data_fifo = ADC_ReadFifoData(ADC);
 *     result_fifo = ADC_GetRes(data_fifo, EXT_SINGLE_ENDED(0));
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint16_t ADC_ReadFifoData(ADC_TypeDef *ADCx)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));

    return (uint16_t)((ADCx->FIFO) & 0xfff);
}


/**
 *
 * \brief  Get the length of ADC FIFO.
 *
 * \param[in]  ADCx: Selected ADC peripheral, which can be ADC.
 *
 * \return  Current data length of ADC FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_adc_init(void)
 * {
 *     uint8_t fifo_size = 0;
 *     fifo_size = ADC_GetFifoLen(ADC);
 *     IO_PRINT_TRACE1("fifo_size:%d", fifo_size);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint8_t ADC_GetFifoLen(ADC_TypeDef *ADCx)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));

    return ((uint8_t)(((ADCx->SCHCR) >> 16) & 0x3F));
}


/**
 *
 * \brief   Get all ADC interrupt status.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 *
 * \return The new state of all ADC interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void oneshot_int_handler(void)
 * {
 *     uint8_t int_status = 0;
 *     int_status = ADC_GetIntStatus(ADC);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint8_t ADC_GetIntStatus(ADC_TypeDef *ADCx)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));

    return ((uint8_t)(((ADCx->INTCR) & (0x1f << 16)) >> 16));
}


/**
 *
 * \brief  Clear ADC FIFO.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void adc_handler(void)
 * {
 *     ADC_ClearFifo(ADC);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void ADC_ClearFifo(ADC_TypeDef *ADCx)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));

    ADCx->CR |= BIT26;

    return;
}


/**
 *
 * \brief  Clear ADC_FIFO_STOP_WRITE status. For ADC_FIFO_STOP_WRITE bit will be asserted automatically
 *         as FIFO overflow, need to be cleared in order to write data again.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void adc_demo(void)
 * {
 *     ADC_StopwriteFifoStatusClear(ADC);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void ADC_StopwriteFifoStatusClear(ADC_TypeDef *ADCx)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));

    ADCx->PWRDLY &= ~ADC_FIFO_STOP_WRITE;

    return;
}


/**
 *
 * \brief  Enable or disable the function that assign ADC output data write into FIFO in one shot mode.
 *
 * \param[in] ADCx: Selected ADC peripheral, which can be ADC.
 * \param[in] NewState: New state of the ADC peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the function that assign ADC output data write into FIFO in one shot mode.
 *            - DISABLE: Disable the function that assign ADC output data write into FIFO in one shot mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void adc_demo(void)
 * {
 *     ADC_OneshotFifoSet(ADC, ENABLE);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void ADC_OneshotFifoSet(ADC_TypeDef *ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState == ENABLE)
    {
        ADCx->CR |= ADC_ONE_SHOT_FIFO_ENABLE;
    }
    else
    {
        ADCx->CR &= ~(ADC_ONE_SHOT_FIFO_ENABLE);
    }
    return;
}


/** @cond private
  * @defgroup 87x3e_ADC_Delay_Function ADC Delay.
  * @{
  */
__STATIC_ALWAYS_INLINE void ADC_delayUS(uint32_t t)
{
    platform_delay_us(t);
}

/**
  * @}
  * @endcond
  */


/**
*
* \brief  Configure ADC power on mode.
*
* \param[in] ADCx: Selected ADC peripheral, which can be ADC.
* \param[in] NewState: New state of the ADC power on mode.
*            This parameter can be one of the following values:
*            - ENABLE: ADC power on is manually controlled.
*            - DISABLE: ADC power on is automatically controlled.
*
* <b>Example usage</b>
* \code{.c}
*
* void driver_adc_init(void)
* {
*     ADC_PowerAlwaysOnCmd(ADC, ENABLE);
* }
* \endcode
*/
__STATIC_ALWAYS_INLINE void ADC_PowerAlwaysOnCmd(ADC_TypeDef *ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_PERIPH(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState == ENABLE)
    {
        ADCx->PWRDLY |= BIT19;
    }
    else
    {
        ADCx->PWRDLY &= ~(BIT19);
    }
}

/** @} */ /* End of group 87x3e_ADC_Exported_Functions */
/** @} */ /* End of group 87x3e_ADC */


#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_ADC_H_ */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/



