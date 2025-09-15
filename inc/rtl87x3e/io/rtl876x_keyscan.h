/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_keyscan.h
* @brief     The header file of KeyScan driver.
* @details
* @author    tifnan_ge
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _RTL876X_KEYSCAN_H_
#define _RTL876X_KEYSCAN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"

/** @cond private
  * @defgroup 87x3e_Keyscan_FIFO_AVALIABLE_MASK
  * @{
  */

#define STATUS_FIFO_DATA_NUM_MASK           ((uint32_t)(0x3F << 4))

/**
  * @}
  * @endcond
  */

/** @addtogroup 87x3e_KeyScan KeyScan
  * @brief KeyScan driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/

/** @defgroup 87x3e_KeyScan_Exported_Types KeyScan Exported Types
  * @{
  */

/**
 * @brief KeyScan initialize parameters.
 */
typedef struct
{
    uint16_t        rowSize;             /*!< Specifies KeyScan row size.
                                              This parameter can be set to a value of 12 or less. */

    uint16_t        colSize;            /*!< Specifies KeyScan column size.
                                             This parameter can be set to a value of 20 or less. */

    uint16_t        rowMap;             /*!< This is a deprecated parameter. */

    uint16_t        colMap;             /*!< This is a deprecated parameter. */

    uint32_t        detectPeriod;       /*!< This is a deprecated parameter. */

    uint16_t        timeout;            /*!< This is a deprecated parameter. */

    uint16_t        scanInterval;       /*!< Specifies KeyScan scan interval.
                                             Scan interval time = delay clock * scanInterval.
                                             This parameter can be a value of 0 ~ 0x1FF. */

    uint32_t        debounceEn;         /*!< Enable or disable debounce function.
                                             This parameter can be a value of @ref x3e_Keyscan_Debounce_Config. */

    uint32_t        scantimerEn;        /*!< Enable or disable scan interval timer.
                                             This parameter can be a value of @ref x3e_Keyscan_scan_interval_en. */

    uint32_t        detecttimerEn;      /*!< Enable or disable release detect timer.
                                             This parameter can be a value of @ref x3e_Keyscan_release_detect_timer_en. */

    uint16_t        debounceTime;       /*!< This is a deprecated parameter. */

    uint32_t        detectMode;         /*!< Specify key detection mode of KeyScan.
                                             This parameter can be a value of @ref x3e_Keyscan_Press_Detect_Mode. */

    uint32_t        fifoOvrCtrl;        /*!< Specifies KeyScan FIFO over flow control.
                                             This parameter can be a value of @ref x3e_Keyscan_Fifo_Overflow_Control. */

    uint16_t        maxScanData;        /*!< This is a deprecated parameter. */

    uint32_t        scanmode;           /*!< Specifies KeyScan scan mode.
                                             This parameter can be a value of @ref x3e_Keyscan_scan_mode. */

    uint16_t        clockdiv;           /*!< Specifies KeyScan clock divider.
                                             Scan clock = system clock/(clockdiv+1). */

    uint8_t         delayclk;           /*!< Specifies KeyScan delay clock divider.
                                             Delay clock = scan clock/(delayclk+1). */

    uint16_t
    fifotriggerlevel;   /*!< Specifies KeyScan FIFO threshold to trigger interrupt @ref KEYSCAN_INT_THRESHOLD.
                                             This parameter can be a value of 0 ~ 26. */

    uint8_t         debouncecnt;        /*!< Specifies KeyScan debounce count.
                                             Debounce time = delay clock * debouncecnt.
                                             This parameter can be a value of 0 ~ 0x1FF. */

    uint8_t         releasecnt;         /*!< Specifies KeyScan release count.
                                             Release time = delay clock * releasecnt.
                                             This parameter can be a value of 0 ~ 0x1FF. */

    uint8_t         keylimit;           /*!< Specify the maximum allowable scan data for each scan.
                                             This parameter can be a value of 0 ~ 26. */
} KEYSCAN_InitTypeDef;

/** End of group 87x3e_KeyScan_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_KeyScan_Exported_Constants KeyScan Exported Constants
  * @{
  */

#define IS_KeyScan_PERIPH(PERIPH) ((PERIPH) == KEYSCAN)     //!< KeyScan peripherals can select KeyScan.

/** @defgroup 87x3e_Keyscan_Row_Number KeyScan Row Number
  * @{
  */

#define IS_KEYSCAN_ROW_NUM(ROW) ((ROW) <= 12)              //!< The row number of the KeyScan need less than or equal to 12.

/** End of group 87x3e_Keyscan_Row_Number
  * @}
  */

/** @defgroup 87x3e_Keyscan_Column_Number KeyScan Column Number
  * @{
  */

#define IS_KEYSCAN_COL_NUM(COL) ((COL) <= 20)             //!< The column number of the KeyScan need less than or equal to 20.

/** End of group 87x3e_Keyscan_Column_Number
  * @}
  */

/** @defgroup 87x3e_Keyscan_scan_mode KeyScan Scan Mode
  * @{
  */

#define KeyScan_Manual_Scan_Mode        ((uint32_t)(0x0 << 30))       //!< The KeyScan manual scan mode.
#define KeyScan_Auto_Scan_Mode          ((uint32_t)(0x1 << 30))       //!< The KeyScan auto scan mode.

#define IS_KEYSCAN_SCAN_MODE(MODE)    (((MODE) == KeyScan_Detect_Mode_Edge) || ((MODE) == KeyScan_Detect_Mode_Level))  //!< Check whether is the KeyScan scan mode.

/** End of group 87x3e_Keyscan_scan_mode
  * @}
  */

/** @defgroup 87x3e_Keyscan_Fifo_Overflow_Control KeyScan FIFO Overflow Control
  * @{
  */

#define KeyScan_FIFO_OVR_CTRL_DIS_ALL   ((uint32_t)(0x0 << 28))   //!< Discard the new scan data when FIFO is full.
#define KeyScan_FIFO_OVR_CTRL_DIS_LAST  ((uint32_t)(0x1 << 28))   //!< Discard the oldest scan data when FIFO is full.

#define IS_KEYSCAN_FIFO_OVR_CTRL(CTRL)  (((CTRL) == KeyScan_FIFO_OVR_CTRL_DIS_ALL) || ((CTRL) == KeyScan_FIFO_OVR_CTRL_DIS_LAST)) //!< Check whether is the KeyScan FIFO overflow control.

/** End of group 87x3e_Keyscan_Fifo_Overflow_Control
  * @}
  */


/** @defgroup 87x3e_Keyscan_Debounce_Config KeyScan Debounce Config
  * @{
  */

#define KeyScan_Debounce_Enable              ((uint32_t)0x1 << 31)      //!< Enable debounce function.
#define KeyScan_Debounce_Disable             ((uint32_t)0x0 << 31)      //!< Disable debounce function.
#define IS_KEYSCAN_DEBOUNCE_EN(EN) (((EN) == KeyScan_Debounce_Enable) || ((EN) == KeyScan_Debounce_Disable)) //!< Check whether is the status of KeyScan debounce function.

/** End of group 87x3e_Keyscan_Debounce_Config
  * @}
  */

/** @defgroup 87x3e_Keyscan_scan_interval_en KeyScan Scan Interval Timer Enable
  * @{
  */

#define KeyScan_ScanInterval_Enable              ((uint32_t)(0x1 << 30))    //!< Enable scan interval timer.
#define KeyScan_ScanInterval_Disable             ((uint32_t)(0x0 << 30))    //!< Disable scan interval timer.
#define IS_KEYSCAN_SCANINTERVAL_EN(EN) (((EN) == KeyScan_ScanInterval_Enable) || ((EN) == KeyScan_ScanInterval_Disable)) //!< Check whether is the status of KeyScan scan interval timer.

/** End of group 87x3e_Keyscan_scan_interval_en
  * @}
  */

/** @defgroup 87x3e_Keyscan_release_detect_timer_en KeyScan Release Detect Timer Enable
  * @{
  */

#define KeyScan_Release_Detect_Enable              ((uint32_t)(0x1 << 29))    //!< Enable release detect timer.
#define KeyScan_Release_Detect_Disable             ((uint32_t)(0x0 << 29))    //!< Disable release detect timer.
#define IS_KEYSCAN_RELEASE_DETECT_EN(EN) (((EN) == KeyScan_Release_Detect_Enable) || ((EN) == KeyScan_Release_Detect_Disable)) //!< Check whether is the status of KeyScan release detect timer.

/** End of group 87x3e_Keyscan_release_detect_timer_en
  * @}
  */

/** @defgroup 87x3e_Keyscan_Press_Detect_Mode KeyScan Press Detect Mode
  * @{
  */

#define KeyScan_Detect_Mode_Edge        ((uint32_t)(0x0 << 29))     //!< The key detection mode is edge-triggered.
#define KeyScan_Detect_Mode_Level       ((uint32_t)(0x1 << 29))     //!< The key detection mode is level-triggered.

#define IS_KEYSCAN_DETECT_MODE(MODE)    (((MODE) == KeyScan_Detect_Mode_Edge) || ((MODE) == KeyScan_Detect_Mode_Level)) //!< Check whether is the KeyScan detection mode.

/** End of group 87x3e_Keyscan_Press_Detect_Mode
  * @}
  */


/** @defgroup 87x3e_Keyscan_Interrupt_Definition KeyScan Interrupt Definition
  * @{
  */

#define KEYSCAN_INT_THRESHOLD                    ((uint16_t)(0x1 << 4))     //!< KeyScan FIFO threshold interrupt. when data in the FIFO reaches the threshold level, the interrupt is triggered.
#define KEYSCAN_INT_OVER_READ                    ((uint16_t)(0x1 << 3))     //!< KeyScan FIFO over read interrupt. When there is no data in the FIFO, reading the FIFO will trigger this interrupt to prevent over-reading.
#define KEYSCAN_INT_SCAN_END                     ((uint16_t)(0x1 << 2))     //!< KeyScan finish interrupt. Whether the key value is scanned or not, the interrupt will be triggered as long as the scanning action is completed.
#define KEYSCAN_INT_FIFO_NOT_EMPTY               ((uint16_t)(0x1 << 1))     //!< KeyScan FIFO not empty interrupt. If there is data in the FIFO, the interrupt will be triggered.
#define KEYSCAN_INT_ALL_RELEASE                  ((uint16_t)(0x1 << 0))     //!< KeyScan all release interrupt. When the release time count reaches the set value, if no key is pressed, the interrupt is triggered.

#define IS_KEYSCAN_CONFIG_IT(IT) ((((IT) & (uint32_t)0xFFF8) == 0x00) && ((IT) != 0x00)) //!< Check whether is the KeyScan interrupt.

/** End of group 87x3e_Keyscan_Interrupt_Definition
  * @}
  */

/**
  * @defgroup  87x3e_Keyscan_Flags KeyScan Flags
  * @{
  */
#define KEYSCAN_FLAG_FIFOLIMIT                       ((uint32_t)(0x1 << 20))    //!< When data filtering occurs, this bit will be set to 1.
#define KEYSCAN_INT_FLAG_THRESHOLD                   ((uint32_t)(0x1 << 19))    //!< FIFO threshold interrupt status.
#define KEYSCAN_INT_FLAG_OVER_READ                   ((uint32_t)(0x1 << 18))    //!< FIFO over read interrupt status.
#define KEYSCAN_INT_FLAG_SCAN_END                    ((uint32_t)(0x1 << 17))    //!< Scan finish interrupt status.
#define KEYSCAN_INT_FLAG_FIFO_NOT_EMPTY              ((uint32_t)(0x1 << 16))    //!< FIFO not empty interrupt status.
#define KEYSCAN_INT_FLAG_ALL_RELEASE                 ((uint32_t)(0x1 << 15))    //!< All release interrupt status.
#define KEYSCAN_FLAG_DATAFILTER                      ((uint32_t)(0x1 << 3))     //!< FIFO data filter status.
#define KEYSCAN_FLAG_OVR                             ((uint32_t)(0x1 << 2))     //!< FIFO overflow status.
#define KEYSCAN_FLAG_FULL                            ((uint32_t)(0x1 << 1))     //!< FIFO full status.
#define KEYSCAN_FLAG_EMPTY                           ((uint32_t)(0x1 << 0))     //!< FIFO empty status.

#define IS_KEYSCAN_FLAG(FLAG)       ((((FLAG) & (uint32_t)0x01FF) == 0x00) && ((FLAG) != (uint32_t)0x00))  //!< Check whether is the KeyScan flag.
#define IS_KEYSCAN_CLEAR_FLAG(FLAG) ((((FLAG) & (uint32_t)0x00C0) == 0x00) && ((FLAG) != (uint32_t)0x00))  //!< Check whether is the definition of KeyScan flag clear.

/** End of group 87x3e_Keyscan_Flags
  * @}
  */

/** End of group 87x3e_KeyScan_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_KeyScan_Exported_Functions KeyScan Exported Functions
 * @{
 */


/**
 *
 * \brief   Initializes the KeyScan peripheral according to the specified
 *          parameters in the KeyScan_InitStruct.
 *
 * \param[in]  KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[in]  KeyScan_InitStruct: Pointer to a KEYSCAN_InitTypeDef structure that
 *             contains the configuration information for the specified KeyScan peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_keyscan_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_KEYSCAN, APBPeriph_KEYSCAN_CLOCK, ENABLE);
 *
 *     KEYSCAN_InitTypeDef KEYSCAN_InitStruct;
 *     KeyScan_StructInit(&KEYSCAN_InitStruct);
 *
 *     KEYSCAN_InitStruct.rowSize  = KEYBOARD_ROW_SIZE;
 *     KEYSCAN_InitStruct.colSize  = KEYBOARD_COLUMN_SIZE;
 *     KEYSCAN_InitStruct.scanmode     = KeyScan_Manual_Scan_Mode;
 *     KEYSCAN_InitStruct.debounceEn   = vDebounce_En;
 *     KeyScan_Init(KEYSCAN, &KEYSCAN_InitStruct);
 * }
 * \endcode
 */
extern void KeyScan_Init(KEYSCAN_TypeDef *KeyScan, KEYSCAN_InitTypeDef *KeyScan_InitStruct);

/**
 *
 * \brief  Disable the KeyScan peripheral clock, and restore registers to their default values.
 *
 * \param[in]  KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_keyscan_init(void)
 * {
 *     KeyScan_DeInit(KEYSCAN);
 * }
 * \endcode
 */
extern void KeyScan_DeInit(KEYSCAN_TypeDef *KeyScan);

/**
 *
 * \brief  Fills each Keyscan_InitStruct member with its default value.
 *
 * \note   The default settings for the KeyScan_InitStruct member are shown in the following table:
 *         | KeyScan_InitStruct Member | Default Value                       |
 *         |:-------------------------:|:-----------------------------------:|
 *         | colSize                   | 2                                   |
 *         | rowSize                   | 2                                   |
 *         | scanmode                  | \ref KeyScan_Auto_Scan_Mode         |
 *         | detectMode                | \ref KeyScan_Detect_Mode_Level      |
 *         | clockdiv                  | 0x1f8                               |
 *         | delayclk                  | 0x01                                |
 *         | fifotriggerlevel          | 1                                   |
 *         | fifoOvrCtrl               | \ref KeyScan_FIFO_OVR_CTRL_DIS_LAST |
 *         | debounceEn                | \ref KeyScan_Debounce_Enable        |
 *         | scantimerEn               | \ref KeyScan_ScanInterval_Enable    |
 *         | detecttimerEn             | \ref KeyScan_Release_Detect_Enable  |
 *         | scanInterval              | 0x10                                |
 *         | debouncecnt               | 0x10                                |
 *         | releasecnt                | 0x1                                 |
 *         | keylimit                  | 0x03                                |
 *
 * \param[in]  KeyScan_InitStruct: Pointer to a KEYSCAN_InitTypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_keyscan_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_KEYSCAN, APBPeriph_KEYSCAN_CLOCK, ENABLE);
 *
 *     KEYSCAN_InitTypeDef KEYSCAN_InitStruct;
 *     KeyScan_StructInit(&KEYSCAN_InitStruct);
 *
 *     KEYSCAN_InitStruct.rowSize  = KEYBOARD_ROW_SIZE;
 *     KEYSCAN_InitStruct.colSize  = KEYBOARD_COLUMN_SIZE;
 *     KEYSCAN_InitStruct.scanmode     = KeyScan_Manual_Scan_Mode;
 *     KEYSCAN_InitStruct.debounceEn   = vDebounce_En;
 *     KeyScan_Init(KEYSCAN, &KEYSCAN_InitStruct);
 * }
 * \endcode
 */
extern void KeyScan_StructInit(KEYSCAN_InitTypeDef *KeyScan_InitStruct);


/**
 *
 * \brief  Enable or disable the specified KeyScan interrupts.
 *
 * \param[in]  KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[in]  KeyScan_IT: Specifies the KeyScan interrupt sources to be enabled or disabled, refer to \ref x3e_Keyscan_Interrupt_Definition.
 *             This parameter can be any combination of the following values:
 *             - KEYSCAN_INT_OVER_READ: KeyScan FIFO over read interrupt. When there is no data in the FIFO, reading the FIFO will trigger this interrupt to prevent over-reading.
 *             - KEYSCAN_INT_THRESHOLD: KeyScan FIFO threshold interrupt. when data in the FIFO reaches the threshold level, the interrupt is triggered.
 *             - KEYSCAN_INT_SCAN_END: KeyScan finish interrupt. Whether the key value is scanned or not, the interrupt will be triggered as long as the scanning action is completed.
 *             - KEYSCAN_INT_FIFO_NOT_EMPTY: KeyScan FIFO not empty interrupt. If there is data in the FIFO, the interrupt will be triggered.
 *             - KEYSCAN_INT_ALL_RELEASE: KeyScan all release interrupt. When the release time count reaches the set value, if no key is pressed, the interrupt is triggered.
 * \param[in]  newState: New state of the specified KeyScan interrupts.
 *             This parameter can be one of the following values:
 *             - ENABLE: Enable the specified KeyScan interrupts.
 *             - DISABLE: Disable the specified KeyScan interrupts.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_keyscan_init(void)
 * {
 *     KeyScan_INTConfig(KEYSCAN, KEYSCAN_INT_SCAN_END | KEYSCAN_INT_ALL_RELEASE, ENABLE);
 *     KeyScan_Cmd(KEYSCAN, ENABLE);
 * }
 * \endcode
 */
extern void KeyScan_INTConfig(KEYSCAN_TypeDef *KeyScan, uint32_t KeyScan_IT,
                              FunctionalState newState);

/**
 *
 * \brief  Mask or unmask the specified KeyScan interrupts.
 *
 * \param[in]  KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[in]  KeyScan_IT: Specifies the KeyScan interrupt sources, refer to \ref x3e_Keyscan_Interrupt_Definition.
 *             This parameter can be any combination of the following values:
 *             - KEYSCAN_INT_OVER_READ: KeyScan FIFO over read interrupt. When there is no data in the FIFO, reading the FIFO will trigger this interrupt to prevent over-reading.
 *             - KEYSCAN_INT_THRESHOLD: KeyScan FIFO threshold interrupt. when data in the FIFO reaches the threshold level, the interrupt is triggered.
 *             - KEYSCAN_INT_SCAN_END: KeyScan finish interrupt. Whether the key value is scanned or not, the interrupt will be triggered as long as the scanning action is completed.
 *             - KEYSCAN_INT_FIFO_NOT_EMPTY: KeyScan FIFO not empty interrupt. If there is data in the FIFO, the interrupt will be triggered.
 *             - KEYSCAN_INT_ALL_RELEASE: KeyScan all release interrupt. When the release time count reaches the set value, if no key is pressed, the interrupt is triggered.
 * \param[in]  newState: New state of the specified KeyScan interrupt mask.
 *             This parameter can be one of the following values:
 *             - ENABLE: Enable the interrupt mask of KeyScan.
 *             - DISABLE: Disable the interrupt mask of KeyScan.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void KeyScan_Handler(void)
 * {
 *     if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_INT_FLAG_ALL_RELEASE) == SET)
 *     {
 *         //add user code here.
 *         KeyScan_ClearINTPendingBit(KEYSCAN, KEYSCAN_INT_ALL_RELEASE);
 *         KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_ALL_RELEASE, DISABLE);
 *     }
 * }
 * \endcode
 */
extern void KeyScan_INTMask(KEYSCAN_TypeDef *KeyScan, uint32_t KeyScan_IT,
                            FunctionalState newState);

/**
 *
 * \brief  Read data from KeyScan FIFO.
 *
 * \param[in]  KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[out] outBuf: Buffer to save data read from KeyScan FIFO.
 * \param[in]  count: Data length to be read.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void keyscan_demo(void)
 * {
 *     uint16_t data[3] = {0};
 *     KeyScan_Read(KEYSCAN, data, 3);
 * }
 * \endcode
 */
extern void KeyScan_Read(KEYSCAN_TypeDef *KeyScan, uint16_t *outBuf, uint16_t count);

/**
 *
 * \brief   Enable or disable the KeyScan peripheral.
 *
 * \param[in] KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[in] NewState: New state of the KeyScan peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the KeyScan peripheral.
 *            - DISABLE: Disable the KeyScan peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_keyscan_init(void)
 * {
 *     KeyScan_Cmd(KEYSCAN, ENABLE);
 * }
 * \endcode
 */
extern void KeyScan_Cmd(KEYSCAN_TypeDef *KeyScan, FunctionalState NewState);

/**
 *
 * \brief   Set filter data.
 *
 * \param[in] KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[in] data: Config the data to be filtered. This parameter should not be more than 9 bits.
 * \param[in] NewState: New state of the KeyScan filtering.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable data filtering.
 *            - DISABLE: Disable data filtering.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void keyscan_demo(void)
 * {
 *     KeyScan_FilterDataConfig(KEYSCAN, 0x01, ENABLE);
 * }
 * \endcode
 */
extern void KeyScan_FilterDataConfig(KEYSCAN_TypeDef *KeyScan, uint16_t data,
                                     FunctionalState NewState);

/**
 *
 * \brief   Config the KeyScan debounce time.
 *
 * \param[in] KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[in] time: KeyScan hardware debounce time. Debounce time = delay clock * time.
 * \param[in] NewState: New state of the KeyScan debounce function.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable KeyScan debounce function.
 *            - DISABLE: Disable KeyScan debounce function.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void keyscan_demo(void)
 * {
 *     KeyScan_debounceConfig(KEYSCAN, 10, ENABLE);
 *
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void KeyScan_debounceConfig(KEYSCAN_TypeDef *KeyScan, uint8_t time,
                                                   FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_KeyScan_PERIPH(KeyScan));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    KeyScan->TIMERCR &= ~((0x1ff << 18) | BIT31);
    KeyScan->TIMERCR |= ((NewState << 31) | time << 18);

}

/**
 *
 * \brief   Get KeyScan FIFO data number.
 *
 * \param[in]  KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 *
 * \return  Data length in FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void KeyScan_Handler(void)
 * {
 *     if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_INT_FLAG_SCAN_END) == SET)
 *     {
 *         KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_SCAN_END, ENABLE);
 *
 *         //KeyScan FIFO not empty
 *         if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_FLAG_EMPTY) != SET)
 *         {
 *             uint8_t data_len = KeyScan_GetFifoDataNum(KEYSCAN);
 *             KeyScan_Read(KEYSCAN, data, data_len);
 *             //add user code here.
 *         }
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint16_t KeyScan_GetFifoDataNum(KEYSCAN_TypeDef *KeyScan)
{
    assert_param(IS_KeyScan_PERIPH(KeyScan));

    return (uint16_t)((KeyScan->STATUS & STATUS_FIFO_DATA_NUM_MASK) >> 4);
}

/**
 *
 * \brief  Clear the KeyScan interrupt pending bit.
 *
 * \param[in]  KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[in]  KeyScan_IT: Specifies the interrupt pending bit to clear, refer to \ref x3e_Keyscan_Interrupt_Definition.
 *             This parameter can be any combination of the following values:
 *             - KEYSCAN_INT_OVER_READ: KeyScan FIFO over read interrupt. When there is no data in the FIFO, reading the FIFO will trigger this interrupt to prevent over-reading.
 *             - KEYSCAN_INT_THRESHOLD: KeyScan FIFO threshold interrupt. when data in the FIFO reaches the threshold level, the interrupt is triggered.
 *             - KEYSCAN_INT_SCAN_END: KeyScan finish interrupt. Whether the key value is scanned or not, the interrupt will be triggered as long as the scanning action is completed.
 *             - KEYSCAN_INT_FIFO_NOT_EMPTY: KeyScan FIFO not empty interrupt. If there is data in the FIFO, the interrupt will be triggered.
 *             - KEYSCAN_INT_ALL_RELEASE: KeyScan all release interrupt. When the release time count reaches the set value, if no key is pressed, the interrupt is triggered.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void KeyScan_Handler(void)
 * {
 *     if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_INT_FLAG_ALL_RELEASE) == SET)
 *     {
 *         //clear KeyScan interrupt
 *         KeyScan_ClearINTPendingBit(KEYSCAN, KEYSCAN_INT_ALL_RELEASE);
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void KeyScan_ClearINTPendingBit(KEYSCAN_TypeDef *KeyScan,
                                                       uint32_t KeyScan_IT)
{
    /* Check the parameters */
    assert_param(IS_KeyScan_PERIPH(KeyScan));
    assert_param(IS_KEYSCAN_CONFIG_IT(KeyScan_IT));

    KeyScan->INTCLR |= KeyScan_IT;

    return;
}

/**
 *
 * \brief   Clear the specified KeyScan flags.
 *
 * \param[in] KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[in] KeyScan_FLAG: Specifies the KeyScan flag to clear, refer to \ref x3e_Keyscan_Flags.
 *            This parameter can be one of the following values:
 *            - KEYSCAN_FLAG_FIFOLIMIT: When data filtering occurs, this bit will be set to 1.
 *            - KEYSCAN_FLAG_DATAFILTER: FIFO data filter status.
 *            - KEYSCAN_FLAG_OVR: FIFO overflow status.
 *
 * \note    KEYSCAN_FLAG_FULL and KEYSCAN_FLAG_EMPTY can't be cleared manually.
 *          They are cleared by hardware automatically.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void keyscan_demo(void)
 * {
 *     KeyScan_ClearFlags(KEYSCAN, KEYSCAN_FLAG_FIFOLIMIT);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void KeyScan_ClearFlags(KEYSCAN_TypeDef *KeyScan, uint32_t KeyScan_FLAG)
{
    /* Check the parameters */
    assert_param(IS_KeyScan_PERIPH(KeyScan));
    assert_param(IS_KEYSCAN_CLEAR_FLAG(KeyScan_FLAG));

    if (KeyScan_FLAG & KEYSCAN_FLAG_FIFOLIMIT)
    {
        KeyScan->INTCLR |= BIT8;
    }
    if (KeyScan_FLAG & KEYSCAN_FLAG_DATAFILTER)
    {
        KeyScan->INTCLR |= BIT7;
    }
    if (KeyScan_FLAG & KEYSCAN_FLAG_OVR)
    {
        KeyScan->INTCLR |= BIT5;
    }

    return;
}

/**
 *
 * \brief   Get the specified KeyScan flag status.
 *
 * \param[in] KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 * \param[in] KeyScan_FLAG: Specifies the KeyScan flag to check, refer to \ref x3e_Keyscan_Flags.
 *            This parameter can be one of the following values:
 *            - KEYSCAN_FLAG_FIFOLIMIT: When data filtering occurs, this bit will be set to 1.
 *            - KEYSCAN_INT_FLAG_THRESHOLD: FIFO threshold interrupt status.
 *            - KEYSCAN_INT_FLAG_OVER_READ: FIFO over read interrupt status.
 *            - KEYSCAN_INT_FLAG_SCAN_END: Scan finish interrupt status.
 *            - KEYSCAN_INT_FLAG_FIFO_NOT_EMPTY: FIFO not empty interrupt status.
 *            - KEYSCAN_INT_FLAG_ALL_RELEASE: All release interrupt status.
 *            - KEYSCAN_FLAG_DATAFILTER: FIFO data filter status.
 *            - KEYSCAN_FLAG_OVR: FIFO overflow status.
 *            - KEYSCAN_FLAG_FULL: FIFO full status.
 *            - KEYSCAN_FLAG_EMPTY: FIFO empty status.
 *
 * \return  The status of KeyScan flag.
 * \retval SET: The specified KeyScan flag is set.
 * \retval RESET: The specified KeyScan flag is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void KeyScan_Handler(void)
 * {
 *     if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_INT_FLAG_ALL_RELEASE) == SET)
 *     {
 *         //add user code here.
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE FlagStatus KeyScan_GetFlagState(KEYSCAN_TypeDef *KeyScan,
                                                       uint32_t KeyScan_FLAG)
{
    FlagStatus bitstatus = RESET;

    /* Check the parameters */
    assert_param(IS_KeyScan_PERIPH(KeyScan));
    assert_param(IS_KEYSCAN_FLAG(KeyScan_FLAG));

    if ((KeyScan->STATUS & KeyScan_FLAG) != 0)
    {
        bitstatus = SET;
    }

    return bitstatus;
}

/**
 *
 * \brief  Read KeyScan FIFO data.
 *
 * \param[in] KeyScan: Selected KeyScan peripheral, which can be KEYSCAN.
 *
 * \return KeyScan FIFO data.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void keyscan_demo(void)
 * {
 *     uint16_t data = KeyScan_ReadFifoData(KEYSCAN);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint16_t KeyScan_ReadFifoData(KEYSCAN_TypeDef *KeyScan)
{
    /* Check the parameters */
    assert_param(IS_KeyScan_PERIPH(KeyScan));

    return (uint16_t)(KeyScan->FIFODATA);
}

#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_KEYSCAN_H_ */

/** @} */ /* End of group 87x3e_KeyScan_Exported_Functions */
/** @} */ /* End of group 87x3e_KeyScan */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/


