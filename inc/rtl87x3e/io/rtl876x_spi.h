/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_spi.h
* @brief
* @details
* @author    elliot chen
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RTL876X_SPI_H
#define __RTL876X_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"


/** @addtogroup 87x3e_SPI SPI
  * @brief SPI driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup 87x3e_SPI_Exported_Types SPI Exported Types
  * @{
  */

/**
  * @brief  SPI Init structure definition.
  */

typedef struct
{
    uint16_t SPI_Direction;         /*!< Specifies the SPI unidirectional or bidirectional data mode.
                                                    This parameter can be a value of @ref x3e_SPI_data_direction. */

    uint16_t SPI_Mode;              /*!< Specifies the SPI operating mode.
                                                    This parameter can be a value of @ref x3e_SPI_mode. */

    uint16_t SPI_DataSize;          /*!< Specifies the SPI data size.
                                                    This parameter can be a value of @ref x3e_SPI_data_size. */

    uint16_t SPI_CPOL;              /*!< Specifies the serial clock steady state.
                                                    This parameter can be a value of @ref x3e_SPI_Clock_Polarity. */

    uint16_t SPI_CPHA;              /*!< Specifies the clock active edge for the bit capture.
                                                    This parameter can be a value of @ref x3e_SPI_Clock_Phase. */

    uint32_t SPI_SwapTxBitEn;              /*!< Specifies whether to swap SPI TX data bit. Only SPI0 supports bit swap. This parameter can be a value of @ref x3e_SPI_Swap_En. */

    uint32_t SPI_SwapRxBitEn;              /*!< Specifies whether to swap SPI RX data bit. Only SPI0 supports bit swap. This parameter can be a value of @ref x3e_SPI_Swap_En. */

    uint32_t SPI_SwapTxByteEn;              /*!< Specifies whether to swap SPI TX data byte. Only SPI0 supports byte swap. This parameter can be a value of @ref x3e_SPI_Swap_En. */

    uint32_t SPI_SwapRxByteEn;              /*!< Specifies whether to swap SPI RX data byte. Only SPI0 supports byte swap. This parameter can be a value of @ref x3e_SPI_Swap_En. */

    uint32_t SPI_ToggleEn;            /*!< Specifies whether CS to toggle between successive frames, default is DISABLE. This parameter can be a value of DISABLE or ENABLE.
                                        ENABLE: CS toggle between successive frames.
                                        DISABLE: CS does not toggle between successive frames.
                                      Enable SPI_ToggleEn should make sure SPI_CPHA = 0(SPI_CPHA_1Edge). */

    uint32_t SPI_BaudRatePrescaler;        /*!< Specifies the speed of SCK clock. This parameter should be any even value between 2 and 65534.
                                                SPI Clock Speed = clk source/SPI_ClkDIV/SPI_BaudRatePrescaler,
                                                clk source is 40Mhz by default,
                                                SPI_ClkDIV refer to @ref x3e_SPI_Clock_Divider, set by API \ref RCC_SPIClkDivConfig.
                                                @note The communication clock is derived from the master
                                                clock. The slave clock does not need to be set. */
    uint16_t SPI_FrameFormat;       /*!< Specifies which serial protocol transfers the data.
                                                    This parameter can be a value of @ref x3e_SPI_frame_format. */

    uint32_t SPI_TxThresholdLevel;  /*!<  Specifies the transmit FIFO Threshold to trigger interrupt @ref SPI_INT_TXE. This parameter can be a value less than TX FIFO depth, with SPI0 being 64 and SPI1/2 being 16. */

    uint32_t SPI_RxThresholdLevel;  /*!< Specifies the receive FIFO Threshold to trigger interrupt @ref SPI_INT_RXF. This parameter can be a value less than RX FIFO depth, with SPI0 being 64 and SPI1/2 being 16. */

    uint32_t SPI_NDF;               /*!< Specifies the number of data frames in EEPROM and RX only mode.
                                                    This parameter should be the value of the length of read data. This parameter must range from 0x1 to 0xFFFFFFFF. */

    uint16_t SPI_TxDmaEn;               /*!< Specifies the TX DMA mode. This parameter can be a value of DISABLE or ENABLE. */

    uint16_t SPI_RxDmaEn;               /*!< Specifies the RX DMA mode. This parameter can be a value of DISABLE or ENABLE. */

    uint8_t SPI_TxWaterlevel;           /*!< Specifies the DMA TX water level. This parameter must range from 1 to 63. It is recommended to set SPI_TxWaterlevel < SPI TX FIFO depth - SPI TX GDMA MSize. */

    uint8_t SPI_RxWaterlevel;           /*!< Specifies the DMA RX water level. This parameter must range from 1 to 63. It is recommended to set SPI_RxWaterlevel = SPI RX GDMA MSize - 1. */

} SPI_InitTypeDef;

/** End of group 87x3e_SPI_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_SPI_Exported_Constants SPI Exported Constants
  * @{
  */

/** @defgroup 87x3e_SPI_Declaration SPI Declaration
  * @{
  */
#define SPI0                            ((SPI_TypeDef              *) SPI0_REG_BASE) //!< The SPI0 base address.
#define SPI1                            ((SPI_TypeDef              *) SPI1_REG_BASE) //!< The SPI1 base address.
#define SPI2                            ((SPI_TypeDef              *) SPI2_REG_BASE) //!< The SPI2 base address.

#define IS_SPI_ALL_PERIPH(PERIPH) (((PERIPH) == SPI0) || \
                                   ((PERIPH) == SPI1) || \
                                   ((PERIPH) == SPI2)) //!< SPI peripherals can select SPI0, SPI1, or SPI2.
/** End of group 87x3e_SPI_Declaration
  * @}
  */

/** @defgroup 87x3e_SPI_clock_speed SPI Clock Speed
  * @{
  */

#define IS_SPI_CLOCK_SPEED(SPEED) (((SPEED) >= 0x01) && ((SPEED) <= 50000000)) //!< SPI clock speed is between 1 and 50000000.

/** End of group 87x3e_SPI_clock_speed
  * @}
  */

/** @defgroup 87x3e_SPI_data_direction SPI Data Direction
  * @{
  */

#define SPI_Direction_FullDuplex        ((uint16_t)0x0000) //!< Data can be transmitted and received at the same time.
#define SPI_Direction_TxOnly            ((uint16_t)0x0001) //!< Data can only be transmitted at a time.
#define SPI_Direction_RxOnly            ((uint16_t)0x0002) //!< Data can only be received at a time.
#define SPI_Direction_EEPROM            ((uint16_t)0x0003) //!< Send data first to read target numbers of data.

#define IS_SPI_DIRECTION_MODE(MODE) (((MODE) == SPI_Direction_FullDuplex) || \
                                     ((MODE) == SPI_Direction_RxOnly) || \
                                     ((MODE) == SPI_Direction_TxOnly) || \
                                     ((MODE) == SPI_Direction_EEPROM)) //!< Check if the input parameter is valid.

/** End of group 87x3e_SPI_data_direction
  * @}
  */

/** @defgroup 87x3e_SPI_mode SPI Mode
  * @{
  */

#define SPI_Mode_Master                         ((uint16_t)0x0104) //!< SPI device operating mode as master.
#define SPI_Mode_Slave                          ((uint16_t)0x0000) //!< SPI device operating mode as slave.
#define IS_SPI_MODE(MODE) (((MODE) == SPI_Mode_Master) || \
                           ((MODE) == SPI_Mode_Slave)) //!< Check if the input parameter is valid.

/** End of group 87x3e_SPI_mode
  * @}
  */

/** @defgroup 87x3e_SPI_data_size SPI Data Size
  * @{
  */

#define SPI_DataSize_4b                 ((uint16_t)0x0003) //!< The data frame size is programmed to 4bits.
#define SPI_DataSize_5b                 ((uint16_t)0x0004) //!< The data frame size is programmed to 5bits.
#define SPI_DataSize_6b                 ((uint16_t)0x0005) //!< The data frame size is programmed to 6bits.
#define SPI_DataSize_7b                 ((uint16_t)0x0006) //!< The data frame size is programmed to 7bits.
#define SPI_DataSize_8b                 ((uint16_t)0x0007) //!< The data frame size is programmed to 8bits.
#define SPI_DataSize_9b                 ((uint16_t)0x0008) //!< The data frame size is programmed to 9bits.
#define SPI_DataSize_10b                 ((uint16_t)0x0009) //!< The data frame size is programmed to 10bits.
#define SPI_DataSize_11b                 ((uint16_t)0x000a) //!< The data frame size is programmed to 11bits.
#define SPI_DataSize_12b                 ((uint16_t)0x000b) //!< The data frame size is programmed to 12bits.
#define SPI_DataSize_13b                 ((uint16_t)0x000c) //!< The data frame size is programmed to 13bits.
#define SPI_DataSize_14b                 ((uint16_t)0x000d) //!< The data frame size is programmed to 14bits.
#define SPI_DataSize_15b                 ((uint16_t)0x000e) //!< The data frame size is programmed to 15bits.
#define SPI_DataSize_16b                 ((uint16_t)0x000f) //!< The data frame size is programmed to 16bits.
#define SPI_DataSize_17b                 ((uint16_t)0x0010) //!< The data frame size is programmed to 17bits.
#define SPI_DataSize_18b                 ((uint16_t)0x0011) //!< The data frame size is programmed to 18bits.
#define SPI_DataSize_19b                 ((uint16_t)0x0012) //!< The data frame size is programmed to 19bits.
#define SPI_DataSize_20b                 ((uint16_t)0x0013) //!< The data frame size is programmed to 20bits.
#define SPI_DataSize_21b                 ((uint16_t)0x0014) //!< The data frame size is programmed to 21bits.
#define SPI_DataSize_22b                 ((uint16_t)0x0015) //!< The data frame size is programmed to 22bits.
#define SPI_DataSize_23b                 ((uint16_t)0x0016) //!< The data frame size is programmed to 23bits.
#define SPI_DataSize_24b                 ((uint16_t)0x0017) //!< The data frame size is programmed to 24bits.
#define SPI_DataSize_25b                 ((uint16_t)0x0018) //!< The data frame size is programmed to 25bits.
#define SPI_DataSize_26b                 ((uint16_t)0x0019) //!< The data frame size is programmed to 26bits.
#define SPI_DataSize_27b                 ((uint16_t)0x001A) //!< The data frame size is programmed to 27bits.
#define SPI_DataSize_28b                 ((uint16_t)0x001B) //!< The data frame size is programmed to 28bits.
#define SPI_DataSize_29b                 ((uint16_t)0x001C) //!< The data frame size is programmed to 29bits.
#define SPI_DataSize_30b                 ((uint16_t)0x001D) //!< The data frame size is programmed to 30bits.
#define SPI_DataSize_31b                 ((uint16_t)0x001E) //!< The data frame size is programmed to 31bits.
#define SPI_DataSize_32b                 ((uint16_t)0x001F) //!< The data frame size is programmed to 32bits.
#define IS_SPI_DATASIZE(DATASIZE) (((DATASIZE) == SPI_DataSize_4b)  || \
                                   ((DATASIZE) == SPI_DataSize_5b)  || \
                                   ((DATASIZE) == SPI_DataSize_6b)  || \
                                   ((DATASIZE) == SPI_DataSize_7b)  || \
                                   ((DATASIZE) == SPI_DataSize_8b)  || \
                                   ((DATASIZE) == SPI_DataSize_9b)  || \
                                   ((DATASIZE) == SPI_DataSize_10b) || \
                                   ((DATASIZE) == SPI_DataSize_11b) || \
                                   ((DATASIZE) == SPI_DataSize_12b) || \
                                   ((DATASIZE) == SPI_DataSize_13b) || \
                                   ((DATASIZE) == SPI_DataSize_14b) || \
                                   ((DATASIZE) == SPI_DataSize_15b) || \
                                   ((DATASIZE) == SPI_DataSize_16b) || \
                                   ((DATASIZE) == SPI_DataSize_17b) || \
                                   ((DATASIZE) == SPI_DataSize_18b) || \
                                   ((DATASIZE) == SPI_DataSize_19b) || \
                                   ((DATASIZE) == SPI_DataSize_20b) || \
                                   ((DATASIZE) == SPI_DataSize_21b) || \
                                   ((DATASIZE) == SPI_DataSize_22b) || \
                                   ((DATASIZE) == SPI_DataSize_23b) || \
                                   ((DATASIZE) == SPI_DataSize_24b) || \
                                   ((DATASIZE) == SPI_DataSize_25b) || \
                                   ((DATASIZE) == SPI_DataSize_26b) || \
                                   ((DATASIZE) == SPI_DataSize_27b) || \
                                   ((DATASIZE) == SPI_DataSize_28b) || \
                                   ((DATASIZE) == SPI_DataSize_29b) || \
                                   ((DATASIZE) == SPI_DataSize_30b) || \
                                   ((DATASIZE) == SPI_DataSize_31b) || \
                                   ((DATASIZE) == SPI_DataSize_32b)) //!< Check if the input parameter is valid.

/** End of group 87x3e_SPI_data_size
  * @}
  */

/** @defgroup 87x3e_SPI_Clock_Polarity SPI Clock Polarity
  * @{
  */

#define SPI_CPOL_Low                    ((uint16_t)0x0000) //!< Inactive state of serial clock is low.
#define SPI_CPOL_High                   ((uint16_t)0x0001) //!< Inactive state of serial clock is high.
#define IS_SPI_CPOL(CPOL) (((CPOL) == SPI_CPOL_Low) || \
                           ((CPOL) == SPI_CPOL_High)) //!< Check if the input parameter is valid.

/** End of group 87x3e_SPI_Clock_Polarity
  * @}
  */

/** @defgroup 87x3e_SPI_Clock_Phase SPI Clock Phase
  * @{
  */

#define SPI_CPHA_1Edge                  ((uint16_t)0x0000) //!< Serial clock toggles in middle of first data bit.
#define SPI_CPHA_2Edge                  ((uint16_t)0x0001) //!< Serial clock toggles at start of first data bit.
#define IS_SPI_CPHA(CPHA) (((CPHA) == SPI_CPHA_1Edge) || \
                           ((CPHA) == SPI_CPHA_2Edge)) //!< Check if the input parameter is valid.

/** End of group 87x3e_SPI_Clock_Phase
  * @}
  */

/** @defgroup 87x3e_SPI_frame_format SPI Frame Format
  * @{
  */

#define SPI_Frame_Motorola              ((uint16_t)0x0000) //!< Standard SPI frame format.
#define SPI_Frame_TI_SSP                ((uint16_t)0x0001) //!< Texas instruments SSP frame format.
#define SPI_Frame_NS_MICROWIRE          ((uint16_t)0x0002) //!< National microwire frame format.
#define SPI_Frame_Reserve               ((uint16_t)0x0003) //!< Reserved value.
#define IS_SPI_FRAME_FORMAT(FRAME) (((FRAME) == SPI_Frame_Motorola) || \
                                    ((FRAME) == SPI_Frame_TI_SSP) || \
                                    ((FRAME) == SPI_Frame_NS_MICROWIRE) || \
                                    ((FRAME) == SPI_Frame_Reserve)) //!< Check if the input parameter is valid.

/** End of group 87x3e_SPI_frame_format
  * @}
  */

/** @defgroup 87x3e_SPI_flags_definition SPI Flags Definition
  * @{
  */

#define SPI_FLAG_BUSY                   ((uint16_t)0x0001)  /**< SPI Busy flag. Set if it is actively transferring data. Reset if it is idle or disabled. */
#define SPI_FLAG_TFNF                   ((uint16_t)0x0002)  /**< Transmit FIFO not full flag. Set if transmit FIFO is not full. */
#define SPI_FLAG_TFE                    ((uint16_t)0x0004)  /**< Transmit FIFO empty flag. Set if transmit FIFO is empty. */
#define SPI_FLAG_RFNE                   ((uint16_t)0x0008)  /**< Receive FIFO not empty flag. Set if receive FIFO is not empty. */
#define SPI_FLAG_RFF                    ((uint16_t)0x0010)  /**< Receive FIFO full flag. Set if the receive FIFO is completely full. */
#define SPI_FLAG_TXE                    ((uint16_t)0x0020)  /**< Transmission error flag. Set if the transmit FIFO is empty when a transfer is started in slave mode. */
#define SPI_FLAG_DCOL                   ((uint16_t)0x0040)  /**< Data collision error flag. Set if it is actively transmitting in master mode when another master selects this device as a slave. */

#define IS_SPI_GET_FLAG(FLAG)   (((FLAG) == SPI_FLAG_DCOL) || ((FLAG) == SPI_FLAG_TXE) || \
                                 ((FLAG) == SPI_FLAG_RFF) || ((FLAG) == SPI_FLAG_RFNE) || \
                                 ((FLAG) == SPI_FLAG_TFE) || ((FLAG) == SPI_FLAG_TFNF) || \
                                 ((FLAG) == SPI_FLAG_BUSY)) //!< Check if the input parameter is valid.

/** End of group 87x3e_SPI_flags_definition
  * @}
  */

/** @defgroup 87x3e_SPI_interrupts_definition SPI Interrupts Definition
  * @{
  */

#define SPI_INT_TXE                  ((uint8_t)BIT(0))  /**< Transmit FIFO empty interrupt. The TX FIFO is equal to or below its threshold value and requires service to prevent an under-run. */
#define SPI_INT_TXO                  ((uint8_t)BIT(1))  /**< Transmit FIFO overflow interrupt. An APB access attempts to write into the TX FIFO after it has been completely filled. When set, data written from the APB is discarded. */
#define SPI_INT_RXU                  ((uint8_t)BIT(2))  /**< Receive FIFO underflow interrupt. An APB access attempts to read from the RX FIFO when it is empty. When set, zeros are read back from the RX FIFO. */
#define SPI_INT_RXO                  ((uint8_t)BIT(3))  /**< Receive FIFO overflow interrupt. The receive logic attempts to place data into the RX FIFO after it has been completely filled. When set, newly received data are discarded. */
#define SPI_INT_RXF                  ((uint8_t)BIT(4))  /**< Receive FIFO full interrupt. RX FIFO is equal to or above its threshold value plus 1 and requires service to prevent an overflow. */
#define SPI_INT_MST                  ((uint8_t)BIT(5))  /**< Multi-Master contention interrupt, only for master. The interrupt is set when another serial master on the serial bus selects the SPI master as a serial-slave device and is actively transferring data. */
#define SPI_INT_FAE                  ((uint8_t)BIT(5))  /**< TX frame alignment interrupt, only for slave. The data received by the slave does not match data frame size. */
#define SPI_INT_TUF                  ((uint8_t)BIT(6))  /**< Transmit FIFO underflow interrupt, only for slave. Clocks sent by the master when the slave is at the empty FIFO level. */
#define SPI_INT_RIG                  ((uint8_t)BIT(7))  /**< CS rising edge detect interrupt, only for slave. CS line has a rising edge. */

#define IS_SPI_CONFIG_IT(IT) (((IT) == SPI_INT_TXE) || \
                              ((IT) == SPI_INT_TXO) || \
                              ((IT) == SPI_INT_RXU) || \
                              ((IT) == SPI_INT_RXO) || \
                              ((IT) == SPI_INT_RXF) || \
                              ((IT) == SPI_INT_MST) || \
                              ((IT) == SPI_INT_TUF) || \
                              ((IT) == SPI_INT_RIG) ) //!< Check if the input parameter is valid.
/** End of group 87x3e_SPI_interrupts_definition
  * @}
  */

/** @defgroup 87x3e_SPI_GDMA_transfer_requests  SPI GDMA Transfer Requests
  * @{
  */

#define SPI_GDMAReq_Tx               ((uint16_t)0x0002) //!< TX buffer GDMA transfer request.
#define SPI_GDMAReq_Rx               ((uint16_t)0x0001) //!< RX buffer GDMA transfer request.
#define IS_SPI_GDMAREQ(GDMAREQ) ((((GDMAREQ) & (uint16_t)0xFFFC) == 0x00) && ((GDMAREQ) != 0x00)) //!< Check if the input parameter is valid.

/** End of group 87x3e_SPI_GDMA_transfer_requests
  * @}
  */

/** @defgroup 87x3e_SPI_Swap_En  SPI Swap Enable
  * @{
  */

#define SPI_SWAP_ENABLE                ((uint32_t)0x0001) //!< Order of transmit/receive bit/byte does not swap.
#define SPI_SWAP_DISABLE               ((uint32_t)0x0000) //!< Order of transmit/receive bit/byte does swap.
#define IS_SPI_SWAPMODE(mode) (((mode) == SPI_SWAP_ENABLE) || \
                               ((mode) == SPI_SWAP_DISABLE)) //!< Check if the input parameter is valid.

/** End of group 87x3e_SPI_Swap_En
  * @}
  */

/** End of group 87x3e_SPI_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_SPI_Exported_Functions SPI Exported Functions
 * @{
 */

/**
 *
 * \brief   Disable the SPIx peripheral clock, and restore registers to their default values.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_spi_init(void)
 * {
 *     SPI_DeInit(SPI0);
 * }
 * \endcode
 */
void SPI_DeInit(SPI_TypeDef *SPIx);

/**
 *
 * \brief   Initializes the SPIx peripheral according to the specified
 *          parameters in the SPI_InitStruct.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] SPI_InitStruct: Pointer to a \ref SPI_InitTypeDef structure that
 *            contains the configuration information for the specified SPI peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_spi_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
 *
 *     SPI_InitTypeDef  SPI_InitStruct;
 *     SPI_StructInit(&SPI_InitStruct);
 *
 *     SPI_InitStruct.SPI_Direction   = SPI_Direction_EEPROM;
 *     SPI_InitStruct.SPI_Mode        = SPI_Mode_Master;
 *     SPI_InitStruct.SPI_DataSize    = SPI_DataSize_8b;
 *     SPI_InitStruct.SPI_CPOL        = SPI_CPOL_High;
 *     SPI_InitStruct.SPI_CPHA        = SPI_CPHA_2Edge;
 *     SPI_InitStruct.SPI_BaudRatePrescaler  = 100;
 *     SPI_InitStruct.SPI_RxThresholdLevel  = 1 - 1;
 *     SPI_InitStruct.SPI_NDF               = 1 - 1;
 *     SPI_InitStruct.SPI_FrameFormat = SPI_Frame_Motorola;
 *
 *     SPI_Init(SPI0, &SPI_InitStruct);
 * }
 * \endcode
 */
void SPI_Init(SPI_TypeDef *SPIx, SPI_InitTypeDef *SPI_InitStruct);


/**
 *
 * \brief  Fills each SPI_InitStruct member with its default value.
 *
 * \note   The default settings for the SPI_InitStruct member are shown in the following table:
 *         | SPI_InitStruct Member  | Default Value                  |
 *         |:----------------------:|:------------------------------:|
 *         | SPI_Mode               | \ref SPI_Mode_Master           |
 *         | SPI_DataSize           | \ref SPI_DataSize_8b           |
 *         | SPI_FrameFormat        | \ref SPI_Frame_Motorola        |
 *         | SPI_Direction          | \ref SPI_Direction_FullDuplex  |
 *         | SPI_CPOL               | \ref SPI_CPOL_High             |
 *         | SPI_CPHA               | \ref SPI_CPHA_2Edge            |
 *         | SPI_BaudRatePrescaler  | 80                             |
 *         | SPI_TxThresholdLevel   | 1                              |
 *         | SPI_RxThresholdLevel   | 0                              |
 *         | SPI_NDF                | 1                              |
 *         | SPI_SwapRxBitEn        | \ref SPI_SWAP_DISABLE          |
 *         | SPI_SwapTxBitEn        | \ref SPI_SWAP_DISABLE          |
 *         | SPI_SwapRxByteEn       | \ref SPI_SWAP_DISABLE          |
 *         | SPI_SwapTxByteEn       | \ref SPI_SWAP_DISABLE          |
 *         | SPI_ToggleEn           | DISABLE                        |
 *         | SPI_RxDmaEn            | DISABLE                        |
 *         | SPI_TxDmaEn            | DISABLE                        |
 *         | SPI_RxWaterlevel       | 1                              |
 *         | SPI_TxWaterlevel       | 35                             |
 *
 * \param[in]  SPI_InitStruct: Pointer to a \ref SPI_InitTypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_spi_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
 *
 *     SPI_InitTypeDef  SPI_InitStruct;
 *     SPI_StructInit(&SPI_InitStruct);
 *
 *     SPI_InitStruct.SPI_Direction   = SPI_Direction_EEPROM;
 *     SPI_InitStruct.SPI_Mode        = SPI_Mode_Master;
 *     SPI_InitStruct.SPI_DataSize    = SPI_DataSize_8b;
 *     SPI_InitStruct.SPI_CPOL        = SPI_CPOL_High;
 *     SPI_InitStruct.SPI_CPHA        = SPI_CPHA_2Edge;
 *     SPI_InitStruct.SPI_BaudRatePrescaler  = 100;
 *     SPI_InitStruct.SPI_RxThresholdLevel  = 1 - 1;
 *     SPI_InitStruct.SPI_NDF               = 1 - 1;
 *     SPI_InitStruct.SPI_FrameFormat = SPI_Frame_Motorola;
 *
 *     SPI_Init(SPI0, &SPI_InitStruct);
 * }
 * \endcode
 */
void SPI_StructInit(SPI_InitTypeDef *SPI_InitStruct);


/**
 *
 * \brief     Set SPI prescaler value.
 *
 * \note      Fsclk_out = Fssi_clk/prescaler use this API to get SPI clk. SPI should be disabled first before use this API.
 * \note      The communication clock is derived from the master clock. The slave clock does not need to be set.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] prescaler: This parameter can be any even value is between 2 and 65534, means the prescaler of SPI clk source, clk source is 40Mhz by default.
 *            Specifies the speed of SCK clock. SPI Clock Speed = clk source/prescaler.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define SPI_PRESCALER_VALUE     100
 * void driver_rtc_init(void)
 * {
 *     SPI_SetPrescaler(SPI0, SPI_PRESCALER_VALUE);
 * }
 * \endcode
 */
void SPI_SetPrescaler(SPI_TypeDef *SPIx, uint16_t prescaler);


/**
 *
 * \brief  Enables or disables the selected SPI peripheral.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] NewState: New state of the SPIx peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the selected SPI peripheral, allowing it to begin data transfer operations.
 *            - DISABLE: Disable the selected SPI peripheral, all serial transfers are halted immediately.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_spi_init(void)
 * {
 *     SPI_Cmd(SPI0, ENABLE);
 * }
 * \endcode
 */
void SPI_Cmd(SPI_TypeDef *SPIx, FunctionalState NewState);

/**
 *
 * \brief  Transmits a number of bytes through the SPIx peripheral.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] pBuf: Bytes to be transmitted. This parameter must range from 0x0 to 0xFF.
 * \param[in] len: Byte length to be transmitted. This parameter must range from 0x1 to 0xFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     uint8_t data_buf[] = {0x01,0x02,0x03};
 *     SPI_SendBuffer(SPI0, data_buf, sizeof(data_buf));
 * }
 * \endcode
 */
void SPI_SendBuffer(SPI_TypeDef *SPIx, uint8_t *pBuf, uint16_t len);


/**
 *
 * \brief  Transmits a number of words through the SPIx peripheral.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] pBuf: Words to be transmitted. This parameter must range from 0x0 to 0xFFFFFFFF.
 * \param[in] len: Word length to be transmitted. This parameter must range from 0x1 to 0xFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     uint32_t data_buf[] = {0x01020304,0x02030405,0x03040506};
 *     SPI_SendWord(SPI0, data_buf, sizeof(data_buf));
 * }
 * \endcode
 */
void SPI_SendWord(SPI_TypeDef *SPIx, uint32_t *pBuf, uint16_t len);


/**
 *
 * \brief  Transmits a number of halfwords through the SPIx peripheral.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] pBuf: Halfwords to be transmitted. This parameter must range from 0x0 to 0xFFFF.
 * \param[in] len: Halfwords length to be transmitted. This parameter must range from 0x1 to 0xFFFF.
 *
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     uint16_t data_buf[] = {0x0102,0x0203,0x0304};
 *     SPI_SendHalfWord(SPI0, data_buf, sizeof(data_buf));
 * }
 * \endcode
  */
void SPI_SendHalfWord(SPI_TypeDef *SPIx, uint16_t *pBuf, uint16_t len);


/**
 *
 * \brief  Enable or disable the specified SPI interrupt.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] SPI_IT: Specifies the SPI interrupt to be enabled or disabled \ref x3e_SPI_interrupts_definition.
 *            This parameter can be one of the following values:
 *            - SPI_INT_TXE: Transmit FIFO empty interrupt. The TX FIFO is equal to or below its threshold value and requires service to prevent an under-run.
 *            - SPI_INT_TXO: Transmit FIFO overflow interrupt. An APB access attempts to write into the TX FIFO after it has been completely filled. When set, data written from the APB is discarded.
 *            - SPI_INT_RXU: Receive FIFO underflow interrupt. An APB access attempts to read from the RX FIFO when it is empty. When set, zeros are read back from the RX FIFO.
 *            - SPI_INT_RXO: Receive FIFO overflow interrupt. The receive logic attempts to place data into the RX FIFO after it has been completely filled. When set, newly received data are discarded.
 *            - SPI_INT_RXF: Receive FIFO full interrupt. RX FIFO is equal to or above its threshold value plus 1 and requires service to prevent an overflow.
 *            - SPI_INT_MST: Multi-Master contention interrupt, only for master. The interrupt is set when another serial master on the serial bus selects the SPI master as a serial-slave device and is actively transferring data.
 *            - SPI_INT_FAE: TX frame alignment interrupt, only for slave. The data received by the slave does not match data frame size.
 *            - SPI_INT_TUF: Transmit FIFO underflow interrupt, only for slave. Clocks sent by the master when the slave is at the empty FIFO level.
 *            - SPI_INT_RIG: CS rising edge detect interrupt, only for slave. CS line has a rising edge.
 * \param[in] NewState: New state of the specified SPI interrupt.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified SPI interrupt.
 *            - DISABLE: Disable the specified SPI interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_spi_init(void)
 * {
 *     SPI_INTConfig(SPI0, SPI_INT_RXF, ENABLE);
 *     RamVectorTableUpdate(SPI0_VECTORn, (IRQ_Fun)SPI0_Handler);
 * }
 * \endcode
 */
void SPI_INTConfig(SPI_TypeDef *SPIx, uint8_t SPI_IT, FunctionalState NewState);


/**
 *
 * \brief  Clear the specified SPI interrupt pending bit.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] SPI_IT: Specifies the SPI interrupt to clear \ref x3e_SPI_interrupts_definition.
 *            This parameter can be one of the following values:
 *            - SPI_INT_TXE: Transmit FIFO empty interrupt. The TX FIFO is equal to or below its threshold value and requires service to prevent an under-run.
 *            - SPI_INT_TXO: Transmit FIFO overflow interrupt. An APB access attempts to write into the TX FIFO after it has been completely filled. When set, data written from the APB is discarded.
 *            - SPI_INT_RXU: Receive FIFO underflow interrupt. An APB access attempts to read from the RX FIFO when it is empty. When set, zeros are read back from the RX FIFO.
 *            - SPI_INT_RXO: Receive FIFO overflow interrupt. The receive logic attempts to place data into the RX FIFO after it has been completely filled. When set, newly received data are discarded.
 *            - SPI_INT_RXF: Receive FIFO full interrupt. RX FIFO is equal to or above its threshold value plus 1 and requires service to prevent an overflow.
 *            - SPI_INT_MST: Multi-Master contention interrupt, only for master. The interrupt is set when another serial master on the serial bus selects the SPI master as a serial-slave device and is actively transferring data.
 *            - SPI_INT_FAE: TX frame alignment interrupt, only for slave. The data received by the slave does not match data frame size.
 *            - SPI_INT_TUF: Transmit FIFO underflow interrupt, only for slave. Clocks sent by the master when the slave is at the empty FIFO level.
 *            - SPI_INT_RIG: CS rising edge detect interrupt, only for slave. CS line has a rising edge.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_handler(void)
 * {
 *     SPI_ClearINTPendingBit(SPI0, SPI_INT_RXF);
 * }
 * \endcode
 */
void SPI_ClearINTPendingBit(SPI_TypeDef *SPIx, uint16_t SPI_IT);

/**
 *
 * \brief   Change SPI data size.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 *
 * \param[in] SPI_DataSize: The parameter of data frame size, please refer to \ref x3e_SPI_data_size.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     SPI_DataSizeConfig(SPI0, SPI_DataSize_8b);
 * }
 * \endcode
 */
void SPI_DataSizeConfig(SPI_TypeDef *SPIx, uint16_t SPI_DataSize);

/**
 *
 * \brief   Enables or disables SPI TX byte swap function.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[in] SPIx: Where x can be 0 to select the SPI peripheral.
 * \param[in] NewState: Specifies whether to swap SPI TX data byte. This parameter can be: ENABLE or DISABLE.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     SPI_SwapTxByteCmd(SPI0, ENABLE);
 * }
 * \endcode
 */
void SPI_SwapTxByteCmd(SPI_TypeDef *SPIx, FunctionalState NewState);

/**
 *
 * \brief  Transmits a data through the SPIx peripheral.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] Data: Data to be transmitted. This parameter must range from 0x0 to 0xFFFFFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     uint32_t data = 0x01020304;
 *     SPI_SendData(SPI0, data);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void SPI_SendData(SPI_TypeDef *SPIx, uint32_t Data)
{
    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));

    SPIx->DR[0] = Data;
    while (!(SPIx->SR & BIT(1)));
}

/**
 *
 * \brief   Returns the most recent received data by the SPIx/I2Sx peripheral.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 *
 * \return  The value of the received data.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_handler(void)
 * {
 *     if (SPI_GetINTStatus(SPI0, SPI_INT_RXF) == SET)
 *     {
 *         len = SPI_GetRxFIFOLen(SPI0);
 *         for (idx = 0; idx < len; idx++)
 *         {
 *             SPI_ReadINTBuf[idx] = SPI_ReceiveData(SPI0);
 *         }
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint32_t SPI_ReceiveData(SPI_TypeDef *SPIx)
{
    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));

    return (uint32_t)SPIx->DR[0];
}


/**
 *
 * \brief   Read data length in RX FIFO through the SPIx peripheral.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 *
 * \return  Data length in RX FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_handler(void)
 * {
 *     if (SPI_GetINTStatus(SPI0, SPI_INT_RXF) == SET)
 *     {
 *         len = SPI_GetRxFIFOLen(SPI0);
 *         //add user code here.
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint8_t SPI_GetRxFIFOLen(SPI_TypeDef *SPIx)
{
    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));
    return (uint8_t)SPIx->RXFLR;
}

/**
 *
 * \brief   Read data length in TX FIFO through the SPIx peripheral.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 *
 * \return  Data length in TX FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     uint8_t data_len = SPI_GetTxFIFOLen(SPI0);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint8_t SPI_GetTxFIFOLen(SPI_TypeDef *SPIx)
{
    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));

    return (uint8_t)SPIx->TXFLR;
}


/**
 *
 * \brief   Change SPI direction mode.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 *
 * \param[in] dir: The parameter of direction mode, please refer to \ref x3e_SPI_data_direction.
 *            This parameter can be one of the following values:
 *            - SPI_Direction_FullDuplex: Data can be transmitted and received at the same time.
 *            - SPI_Direction_TxOnly: Data can only be transmitted at a time.
 *            - SPI_Direction_RxOnly: Data can only be received at a time.
 *            - SPI_Direction_EEPROM: Send data first to read target numbers of data.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     SPI_ChangeDirection(SPI0, SPI_Direction_EEPROM);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void SPI_ChangeDirection(SPI_TypeDef *SPIx, uint16_t dir)
{
    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));
    assert_param(IS_SPI_DIRECTION_MODE(dir));

    /* Disable the selected SPI peripheral */
    SPIx->SSIENR &= ~0x01;

    /* Change SPI direction mode */
    SPIx->CTRLR0 &= ~(0x03 << 8);
    SPIx->CTRLR0 |= dir << 8;

    /* Enable the selected SPI peripheral */
    SPIx->SSIENR |= 0x01;
}


/**
 *
 * \brief   Set read Data length in EEPROM mode and RX only mode through the SPIx peripheral, which
            enables you to receive up to 64 KB of data in a continuous transfer.

 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] len: Length of read data. This parameter must range from 0x1 to 0xFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     SPI_SetReadLen(SPI0, 100);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void SPI_SetReadLen(SPI_TypeDef *SPIx, uint16_t len)
{
    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));

    /* Disable the selected SPI peripheral */
    SPIx->SSIENR &= ~0x01;
    /* set read length in SPI EEPROM mode */
    SPIx->CTRLR1 = len - 1;
    /* Enable the selected SPI peripheral */
    SPIx->SSIENR |= 0x01;
}


/**
 *
 * \brief   Set CS number through the SPIx peripheral, only support for master.
 *          Please make sure SPIx has been initialized before calling this function.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] number: If SPIx is SPI0 or SPI1, number can be 0 to 2. If SPIx is SPI2, number must be 0.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     SPI_SetCSNumber(SPI1, 0);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void SPI_SetCSNumber(SPI_TypeDef *SPIx, uint8_t number)
{
    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));

    /* set cs number */
    SPIx->SER = BIT(number);
}


/**
 *
 * \brief  Check whether the specified SPI interrupt is set.
 *
 * \param[in]  SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in]  SPI_IT: Specifies the SPI interrupt to check \ref x3e_SPI_interrupts_definition.
 *            This parameter can be one of the following values:
 *            - SPI_INT_TXE: Transmit FIFO empty interrupt. The TX FIFO is equal to or below its threshold value and requires service to prevent an under-run.
 *            - SPI_INT_TXO: Transmit FIFO overflow interrupt. An APB access attempts to write into the TX FIFO after it has been completely filled. When set, data written from the APB is discarded.
 *            - SPI_INT_RXU: Receive FIFO underflow interrupt. An APB access attempts to read from the RX FIFO when it is empty. When set, zeros are read back from the RX FIFO.
 *            - SPI_INT_RXO: Receive FIFO overflow interrupt. The receive logic attempts to place data into the RX FIFO after it has been completely filled. When set, newly received data are discarded.
 *            - SPI_INT_RXF: Receive FIFO full interrupt. RX FIFO is equal to or above its threshold value plus 1 and requires service to prevent an overflow.
 *            - SPI_INT_MST: Multi-Master contention interrupt, only for master. The interrupt is set when another serial master on the serial bus selects the SPI master as a serial-slave device and is actively transferring data.
 *            - SPI_INT_FAE: TX frame alignment interrupt, only for slave. The data received by the slave does not match data frame size.
 *            - SPI_INT_TUF: Transmit FIFO underflow interrupt, only for slave. Clocks sent by the master when the slave is at the empty FIFO level.
 *            - SPI_INT_RIG: CS rising edge detect interrupt, only for slave. CS line has a rising edge.
 *
 * \return The new state of SPI_IT.
 * \retval SET: The specified SPI interrupt is set.
 * \retval RESET: The specified SPI interrupt is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_handler(void)
 * {
 *     if (SPI_GetINTStatus(SPI0, SPI_INT_RXF) == SET)
 *     {
 *         //add user code here.
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE ITStatus SPI_GetINTStatus(SPI_TypeDef *SPIx, uint32_t SPI_IT)
{
    ITStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));
    assert_param(IS_SPI_CONFIG_IT(SPI_IT));

    if ((SPIx->ISR & SPI_IT) != (uint32_t)RESET)
    {
        bit_status = SET;
    }

    /* Return the SPI_IT status */
    return  bit_status;
}


/**
 *
 * \brief  Check whether the specified SPI flag is set or not.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] SPI_FLAG: Specifies the SPI flag to check \ref x3e_SPI_flags_definition.
 *            This parameter can be one of the following values:
 *            - SPI_FLAG_DCOL: Data Collision Error flag. Set if it is actively transmitting in master mode when another master selects this device as a slave.
 *            - SPI_FLAG_TXE: Transmission error flag. Set if the transmit FIFO is empty when a transfer is started in slave mode.
 *            - SPI_FLAG_RFF: Receive FIFO full flag. Set if the receive FIFO is completely full.
 *            - SPI_FLAG_RFNE: Receive FIFO Not Empty flag. Set if receive FIFO is not empty.
 *            - SPI_FLAG_TFE: Transmit FIFO Empty flag. Set if transmit FIFO is empty.
 *            - SPI_FLAG_TFNF: Transmit FIFO Not Full flag. Set if transmit FIFO is not full.
 *            - SPI_FLAG_BUSY: SPI Busy flag. Set if it is actively transferring data.reset if it is idle or disabled.
 *
 * \return The new state of SPI_FLAG.
 * \retval SET: The specified SPI flag is set.
 * \retval RESET: The specified SPI flag is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     SPI_WriteBuf[0] = 0x9f;
 *     SPI_SendBuffer(SPI1, SPI_WriteBuf, 4);
 *
 *     //Waiting for SPI data transfer to end
 *     while (SPI_GetFlagState(SPI1, SPI_FLAG_BUSY));
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE FlagStatus SPI_GetFlagState(SPI_TypeDef *SPIx, uint8_t SPI_FLAG)
{
    FlagStatus bitstatus = RESET;

    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));
    assert_param(IS_SPI_GET_FLAG(SPI_FLAG));

    /* Check the status of the specified SPI flag */
    if ((SPIx->SR & SPI_FLAG) != (uint8_t)RESET)
    {
        /* SPI_FLAG is set */
        bitstatus = SET;
    }

    /* Return the SPI_FLAG status */
    return  bitstatus;
}


/**
 *
 * \brief   Enable or disable the SPIx GDMA interface.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] SPI_GDMAReq: Specifies the SPI GDMA transfer request to be enabled or disabled \ref x3e_SPI_GDMA_transfer_requests.
 *            This parameter can be one of the following values:
 *            - SPI_GDMAReq_Tx: TX buffer DMA transfer request.
 *            - SPI_GDMAReq_Rx: RX buffer DMA transfer request.
 * \param[in]  NewState: New state of the selected SPI GDMA transfer request.
 *             This parameter can be one of the following values:
 *             - ENABLE: Enable the SPIx GDMA interface.
 *             - DISABLE: Disable the SPIx GDMA interface.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void spi_demo(void)
 * {
 *     SPI_GDMACmd(SPI0, SPI_GDMAReq_Tx, ENABLE);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void SPI_GDMACmd(SPI_TypeDef *SPIx, uint16_t SPI_GDMAReq,
                                        FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_SPI_ALL_PERIPH(SPIx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    assert_param(IS_SPI_GDMAREQ(SPI_GDMAReq));

    if (NewState != DISABLE)
    {
        /* Enable the selected SPI GDMA request */
        SPIx->DMACR |= SPI_GDMAReq;
    }
    else
    {
        /* Disable the selected SPI GDMA request */
        SPIx->DMACR &= (uint16_t)~(SPI_GDMAReq);
    }
}


#ifdef __cplusplus
}
#endif

#endif /*__RTL876X_SPI_H*/

/** @} */ /* End of group 87x3e_SPI_Exported_Functions */
/** @} */ /* End of group 87x3e_SPI */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

