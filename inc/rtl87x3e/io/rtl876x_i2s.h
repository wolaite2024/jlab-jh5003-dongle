/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl876x_i2s.h
* \brief    The header file of the peripheral I2S driver.
* \details  This file provides all I2S firmware functions.
* \author   mh_chang
* \date     2024-07-18
* \version  v1.0
* *********************************************************************************************************
*/
#ifndef _RTL876x_I2S_H
#define _RTL876x_I2S_H
#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                         Includes
 *============================================================================*/
#include "rtl876x.h"
#include "string.h"

#define I2S0_REG_BASE                   0x40020000UL
#define I2S1_REG_BASE                   0x40021000UL

#define TX_FIFO_WR_ADDR_OFFSET          0x0
#define RX_FIFO_RD_ADDR_OFFSET          0x10

#define I2S0_TX_ADDR                    I2S0_REG_BASE + TX_FIFO_WR_ADDR_OFFSET
#define I2S0_RX_ADDR                    I2S0_REG_BASE + RX_FIFO_RD_ADDR_OFFSET
#define I2S1_TX_ADDR                    I2S1_REG_BASE + TX_FIFO_WR_ADDR_OFFSET
#define I2S1_RX_ADDR                    I2S1_REG_BASE + RX_FIFO_RD_ADDR_OFFSET

/** @defgroup 87x3e_I2S_Declaration I2S Declaration
  * @{
  */
#define I2S0                            ((I2S_TypeDef*) I2S0_REG_BASE) //!< The I2S0 base address.
#define I2S1                            ((I2S_TypeDef*) I2S1_REG_BASE) //!< The I2S1 base address.
/** End of group 87x3e_I2S_Declaration
  * @}
  */

/** @addtogroup 87x3e_I2S I2S
  * @brief Manage the I2S peripheral functions.
  * @{
  */
/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup 87x3e_I2S_Exported_Types I2S Exported Types
  * @{
  */
/**
 * \brief       I2S initialize parameters.
 *
 */
typedef struct _X3E_I2S_InitTypeDef
{
    uint32_t I2S_ClockSource;           /*!< Specifies the I2S clock source.
                                             This parameter can be a value of @ref x3e_I2S_Clock_Source. */

    uint32_t I2S_BClockMi;              /*!< Specifies the BLCK clock speed. BCLK = 40MHz * (I2S_BClockNi / I2S_BClockMi).
                                             This parameter must range from 1 to 0xffff. */

    uint32_t I2S_BClockNi;              /*!< Specifies the BLCK clock speed.
                                             This parameter must range from 1 to 0x7FFF. */

    uint32_t I2S_BClockDiv;             /*!< Specifies the BLCK divider ratio. LRCK = BCLK / (I2S_BClockDiv + 1).
                                             This parameter must range from 1 to 0xff. */

    uint32_t I2S_DeviceMode;            /*!< Specifies the I2S device mode.
                                             This parameter can be a value of @ref x3e_I2S_Device_Mode. */

    uint32_t I2S_TxChannelType;         /*!< Specifies the channel type used for the I2S communication.
                                             This parameter can be a value of @ref x3e_I2S_Channel_Type. */

    uint32_t I2S_RxChannelType;         /*!< Specifies the channel type used for the I2S communication.
                                             This parameter can be a value of @ref x3e_I2S_Channel_Type. */

    uint32_t I2S_TxDataFormat;          /*!< Specifies the I2S Data format mode.
                                             This parameter can be a value of @ref x3e_I2S_Format_Mode. */

    uint32_t I2S_RxDataFormat;          /*!< Specifies the I2S Data format mode.
                                             This parameter can be a value of @ref x3e_I2S_Format_Mode. */

    uint32_t I2S_TxBitSequence;         /*!< Specifies the I2S Data bits sequences.
                                             This parameter can be a value of @ref x3e_I2S_Tx_Bit_Sequence. */

    uint32_t I2S_RxBitSequence;         /*!< Specifies the I2S Data bits sequences.
                                             This parameter can be a value of @ref x3e_I2S_Rx_Bit_Sequence. */

    uint32_t I2S_TxDataWidth;           /*!< Specifies the I2S Data width.
                                             This parameter can be a value of @ref x3e_I2S_Data_Width. */

    uint32_t I2S_RxDataWidth;           /*!< Specifies the I2S Data width.
                                             This parameter can be a value of @ref x3e_I2S_Data_Width. */

    uint32_t I2S_TxChannelWidth;        /*!< Specifies the I2S Channel width.
                                             This parameter can be a value of @ref x3e_I2S_Channel_Width. */

    uint32_t I2S_RxChannelWidth;        /*!< Specifies the I2S Channel width.
                                             This parameter can be a value of @ref x3e_I2S_Channel_Width. */

    uint32_t I2S_TxTdmMode;             /*!< Specifies the I2S TDM mode.
                                             This parameter can be a value of @ref x3e_I2S_TDM_Mode. */

    uint32_t I2S_RxTdmMode;             /*!< Specifies the I2S TDM mode.
                                             This parameter can be a value of @ref x3e_I2S_TDM_Mode. */

    uint32_t I2S_DMACmd;                /*!< Specifies the I2S DMA control.
                                             This parameter can be a value of @ref x3e_I2S_DMA_Cmd. */

    uint32_t I2S_TxWaterlevel;          /*!< Specifies the DMA watermark level in transmit mode.
                                             This parameter must range from 1 to 63. */

    uint32_t I2S_RxWaterlevel;          /*!< Specifies the DMA watermark level in receive mode.
                                             This parameter must range from 1 to 63. */

} I2S_InitTypeDef;

/**
  * @brief I2S registers.
  */
typedef struct _X3E_I2S_TypeDef
{
    __O uint32_t TX_FIFO_0_WR;          /**<0x00. */
    __IO uint32_t CTL_REG0;             /**<0x04. */
    __IO uint32_t CTL_REG1;             /**<0x08. */
    __IO uint32_t INT_CTLR;             /**<0x0C. */
    __I uint32_t RX_FIFO_0_RD;          /**<0x10. */
    __I uint32_t DEPTH_CNT;             /**<0x14. */
    __I uint32_t ERR_CNT;               /**<0x18. */
    __IO uint32_t BCLK;                 /**<0x1C. */
    __IO uint32_t BCLK_DIV;             /**<0x20. */
    __I uint32_t STATUS_IND;            /**<0x24. */
    __IO uint32_t CTL_REG2;             /**<0x28. */
    __IO uint32_t RX_BCLK;              /**<0x2C. */
    __IO uint32_t INT_CLR_1;            /**<0x30. */
    __I uint32_t READ_CNT;              /**<0x34. */
    __IO uint32_t RSVD_0;               /**<0x38. */
    __IO uint32_t TX_CH_DATA_SEL;       /**<0x3C. */
    __IO uint32_t RSVD_1;               /**<0x40. */
    __IO uint32_t RSVD_2;               /**<0x44. */
    __IO uint32_t RSVD_3;               /**<0x48. */
    __IO uint32_t RSVD_4;               /**<0x4C. */
    __IO uint32_t RSVD_5;               /**<0x50. */
    __IO uint32_t RX_FIFO_REG0;         /**<0x54. */
    __IO uint32_t RSVD_6;               /**<0x58. */
    __IO uint32_t RX_CNT_CTRL;          /**<0x5C. */
    __I uint32_t RX_CNT_VAL;            /**<0x60. */
    __I uint32_t FIFO_CNT_LATCH;        /**<0x64. */
} I2S_TypeDef;

/** End of Group 87x3e_I2S_Exported_Types
  * @}
  */
/*============================================================================*
 *                         Constants
 *============================================================================*/
/** @defgroup 87x3e_I2S_Exported_Constants  I2S Exported Constants
  * @{
  */

#define IS_I2S_ALL_PERIPH(PERIPH)                   (((PERIPH) == I2S0) || \
                                                     ((PERIPH) == I2S1))    //!< Check whether is the I2S peripheral.

/** @defgroup 87x3e_I2S_Clock_Source I2S Clock Source
  * @{
  */
#define I2S_CLK_XTAL                                ((uint32_t)0x04)        //!< Configure the I2S clock source as XTAL.

#define IS_I2S_CLK_SOURCE(CLK)                      (((CLK) == I2S_CLK_XTAL))   //!< Check whether is the I2S clock source.
/** End of group 87x3e_I2S_Clock_Source
  * @}
  */

/** @defgroup 87x3e_I2S_Master_Clock_SEL I2S Master Clock Select
  * @{
  */
#define I2S_MCLK_DIV4                               ((uint32_t)0x00)        //!< Configure the master clock to be divided by 4.
#define I2S_MCLK_DIV2                               ((uint32_t)0x01)        //!< Configure the master clock to be divided by 2.
#define I2S_MCLK_DIV_NONE                           ((uint32_t)0x02)        //!< Configure the master clock to be divided by 1.

#define IS_I2S_MCLK_DIV(SEL)                        (((SEL) == I2S_MCLK_DIV4) || \
                                                     ((SEL) == I2S_MCLK_DIV2) || \
                                                     ((SEL) == I2S_MCLK_DIV_NONE))  //!< Check whether is the I2S MCLK divider select.
/** End of group 87x3e_I2S_Master_Clock_SEL
  * @}
  */

/** @defgroup 87x3e_I2S_Device_Mode I2S Device Mode
  * @{
  */
#define I2S_DeviceMode_Master                       ((uint32_t)0x00)        //!< Configure the device mode as I2S master.
#define I2S_DeviceMode_Slave                        ((uint32_t)0x01)        //!< Configure the device mode as I2S slave.

#define IS_I2S_DEVICE_MODE(DEVICE)                  (((DEVICE) == I2S_DeviceMode_Master) || \
                                                     ((DEVICE) == I2S_DeviceMode_Slave))    //!< Check whether is the I2S device mode.
/** End of group 87x3e_I2S_Device_Mode
  * @}
  */

/** @defgroup 87x3e_I2S_Mode I2S Mode
  * @{
  */
#define I2S_MODE_TX                                 ((uint32_t)0x01)                    //!< Configure the I2S TX.
#define I2S_MODE_RX                                 ((uint32_t)0x02)                    //!< Configure the I2S RX.
#define I2S_MODE_TRX                                ((I2S_MODE_TX) | (I2S_MODE_RX))     //!< Configure the I2S TX and RX.

#define IS_I2S_MODE(MODE)                           (((MODE) == I2S_MODE_TX) || \
                                                     ((MODE) == I2S_MODE_RX))   //!< Check whether is the I2S mode.
/** End of group 87x3e_I2S_Mode
  * @}
  */

/** @defgroup 87x3e_I2S_Channel_Type I2S Channel Type
  * @{
  */
#define I2S_Channel_Mono                            ((uint32_t)0x01)        //!< Configure the channel type as mono mode.
#define I2S_Channel_Stereo                          ((uint32_t)0x00)        //!< Configure the channel type as stereo mode.

#define IS_I2S_CHANNEL_TYPE(TYPE)                   (((TYPE) == I2S_Channel_Mono) || \
                                                     ((TYPE) == I2S_Channel_Stereo))    //!< Check whether is the I2S channel type.
/** End of group 87x3e_I2S_Channel_Type
  * @}
  */

/** @defgroup 87x3e_I2S_Format_Mode I2S Format Mode
  * @{
  */
#define I2S_Mode                                    ((uint32_t)0x00)        //!< Configure the format mode as I2S mode.
#define Left_Justified_Mode                         ((uint32_t)0x01)        //!< Configure the format mode as left justified mode.
#define PCM_Mode_A                                  ((uint32_t)0x02)        //!< Configure the format mode as PCM mode A.
#define PCM_Mode_B                                  ((uint32_t)0x03)        //!< Configure the format mode as PCM mode B.

#define IS_I2S_DATA_FORMAT(FORMAT)                  (((FORMAT) == I2S_Mode)             || \
                                                     ((FORMAT) == Left_Justified_Mode)  || \
                                                     ((FORMAT) == PCM_Mode_A)           || \
                                                     ((FORMAT) == PCM_Mode_B))  //!< Check whether is the I2S data format.
/** End of group 87x3e_I2S_Format_Mode
  * @}
  */

/** @defgroup 87x3e_I2S_Tx_Bit_Sequence I2S TX Bit Sequence
  * @{
  */
#define I2S_TX_MSB_First                            ((uint32_t)0x00)        //!< Configure the TX bit sequence as MSB first.
#define I2S_TX_LSB_First                            ((uint32_t)0x01)        //!< Configure the TX bit sequence as LSB first.

#define IS_I2S_TX_BIT_SEQ(SEQ)                      (((SEQ) == I2S_TX_MSB_First) || \
                                                     ((SEQ) == I2S_TX_LSB_First))   //!< Check whether is the I2S TX bit sequence.
/** End of group 87x3e_I2S_Tx_Bit_Sequence
  * @}
  */

/** @defgroup 87x3e_I2S_Rx_Bit_Sequence I2S RX Bit Sequence
  * @{
  */
#define I2S_RX_MSB_First                            ((uint32_t)0x00)        //!< Configure the RX bit sequence as MSB first.
#define I2S_RX_LSB_First                            ((uint32_t)0x01)        //!< Configure the RX bit sequence as LSB first.

#define IS_I2S_RX_BIT_SEQ(SEQ)                      (((SEQ) == I2S_RX_MSB_First) || \
                                                     ((SEQ) == I2S_RX_LSB_First))   //!< Check whether is the I2S RX bit sequence.
/** End of group 87x3e_I2S_Rx_Bit_Sequence
  * @}
  */

/** @defgroup 87x3e_I2S_Data_Width I2S Data Width
  * @{
  */
#define I2S_Data_Width_16Bits                       ((uint32_t)0x00)        //!< Configure the data width as 16 bits.
#define I2S_Data_Width_20Bits                       ((uint32_t)0x01)        //!< Configure the data width as 20 bits.
#define I2S_Data_Width_24Bits                       ((uint32_t)0x02)        //!< Configure the data width as 24 bits.
#define I2S_Data_Width_8Bits                        ((uint32_t)0x03)        //!< Configure the data width as 8 bits.
#define I2S_Data_Width_32Bits                       ((uint32_t)0x04)        //!< Configure the data width as 32 bits.

#define IS_I2S_DATA_WIDTH(WIDTH)                    (((WIDTH) == I2S_Data_Width_16Bits) || \
                                                     ((WIDTH) == I2S_Data_Width_24Bits) || \
                                                     ((WIDTH) == I2S_Data_Width_8Bits)  || \
                                                     ((WIDTH) == I2S_Data_Width_20Bits) || \
                                                     ((WIDTH) == I2S_Data_Width_32Bits))    //!< Check whether is the I2S data width.
/** End of group 87x3e_I2S_Data_Width
  * @}
  */

/** @defgroup 87x3e_I2S_Channel_Width I2S Channel Width
  * @{
  */
#define I2S_Channel_Width_16Bits                    ((uint32_t)0x00)        //!< Configure the data width as 16 bits.
#define I2S_Channel_Width_20Bits                    ((uint32_t)0x01)        //!< Configure the data width as 20 bits.
#define I2S_Channel_Width_24Bits                    ((uint32_t)0x02)        //!< Configure the data width as 24 bits.
#define I2S_Channel_Width_8Bits                     ((uint32_t)0x03)        //!< Configure the data width as 8 bits.
#define I2S_Channel_Width_32Bits                    ((uint32_t)0x04)        //!< Configure the data width as 32 bits.

#define IS_I2S_CHANNEL_WIDTH(WIDTH)                 (((WIDTH) == I2S_Channel_Width_16Bits) || \
                                                     ((WIDTH) == I2S_Channel_Width_24Bits) || \
                                                     ((WIDTH) == I2S_Channel_Width_8Bits)  || \
                                                     ((WIDTH) == I2S_Channel_Width_20Bits) || \
                                                     ((WIDTH) == I2S_Channel_Width_32Bits)) //!< Check whether is the I2S channel width.
/** End of group 87x3e_I2S_Channel_Width
  * @}
  */

/** @defgroup 87x3e_I2S_DMA_Cmd I2S DMA CMD
  * @{
  */
#define I2S_DMA_ENABLE                              ((uint32_t)0x00)        //!< Configure the I2S GDMA handshake is enabled.
#define I2S_DMA_DISABLE                             ((uint32_t)0x01)        //!< Configure the I2S GDMA handshake is disabled.

#define IS_I2S_DMA_CMD(CMD)                         (((CMD) == I2S_DMA_ENABLE) || \
                                                     ((CMD) == I2S_DMA_DISABLE))    //!< Check whether is the I2S DMA CMD.
/** End of group 87x3e_I2S_DMA_Cmd
  * @}
  */

/** @defgroup 87x3e_I2S_TDM_Mode I2S TDM Mode
  * @{
  */
#define I2S_TDM_DISABLE                             ((uint32_t)0x00)        //!< Configure the TDM mode as non-TDM.
#define I2S_TDM_MODE_4                              ((uint32_t)0x01)        //!< Configure the TDM mode as TDM4.

#define IS_I2S_TDM_MODE(MODE)                       (((MODE) == I2S_TDM_DISABLE) || \
                                                     ((MODE) == I2S_TDM_MODE_4))    //!< Check whether is the I2S TDM mode.
/** End of group 87x3e_I2S_TDM_Mode
  * @}
  */

/** @defgroup 87x3e_I2S_Interrupt I2S Interrupt
  * @{
  */
#define I2S_MCU_INT_TX_IDLE                         BIT22       //!< I2S interrupt for TX is working, but FIFO_0 is empty.
#define I2S_MCU_INT_RX_EMPTY                        BIT21       //!< I2S interrupt for RX FIFO_0 is empty (MIC path).
#define I2S_MCU_INT_TX_EMPTY                        BIT20       //!< I2S interrupt for TX FIFO_0 is empty (SPK path).
#define I2S_MCU_INT_RX_FULL                         BIT19       //!< I2S interrupt for RX FIFO_0 is full (MIC path).
#define I2S_MCU_INT_TX_FULL                         BIT18       //!< I2S interrupt for TX FIFO_0 is full (SPK path).
#define I2S_MCU_INT_RX_READY                        BIT17       //!< I2S interrupt for ready to receive data (MIC path).
#define I2S_MCU_INT_TX_READY                        BIT16       //!< I2S interrupt for ready to send data out (SPK path).

#define IS_I2S_MCU_INT_CONFIG(INT)                  (((INT) == I2S_MCU_INT_TX_IDLE)  || \
                                                     ((INT) == I2S_MCU_INT_RX_EMPTY) || \
                                                     ((INT) == I2S_MCU_INT_TX_EMPTY) || \
                                                     ((INT) == I2S_MCU_INT_RX_FULL)  || \
                                                     ((INT) == I2S_MCU_INT_TX_FULL)  || \
                                                     ((INT) == I2S_MCU_INT_RX_READY) || \
                                                     ((INT) == I2S_MCU_INT_TX_READY))   //!< Check whether is the I2S interrupt.
/** End of group 87x3e_I2S_Interrupt
  * @}
  */

/** @defgroup 87x3e_I2S_Clear_Interrupt I2S Clear Interrupt
  * @{
  */
#define I2S_CLEAR_INT_RX_READY                      BIT1        //!< Clear the I2S interrupt for ready to receive data (MIC path).
#define I2S_CLEAR_INT_TX_READY                      BIT0        //!< Clear the I2S interrupt for ready to send data out (SPK path).
/** End of group 87x3e_I2S_Clear_Interrupt
  * @}
  */

/** End of Group 87x3e_I2S_Exported_Constants
  * @}
  */


/*============================================================================*
 *                         Functions
 *============================================================================*/
/**
 * \defgroup    87x3e_I2S_Exported_Functions I2S Exported Functions
 * \{
 */

/**
 * \brief   Deinitializes the I2S peripheral registers to their default values.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2s_init(void)
 * {
 *     I2S_DeInit(I2S0);
 * }
 * \endcode
 */
void I2S_DeInit(I2S_TypeDef *I2Sx);

/**
 * \brief   Initializes the I2S peripheral according to the specified
 *          parameters in the I2S_InitStruct.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \param[in] I2S_InitStruct: Pointer to a I2S_InitTypeDef structure that
 *            contains the configuration information for the specified I2S peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2s_init(void)
 * {
 *     RCC_PeriphClockCmd(APB_I2S, APB_I2S_CLOCK, ENABLE);
 *
 *     I2S_InitTypeDef I2S_InitStruct;
 *
 *     I2S_StructInit(&I2S_InitStruct);
 *     I2S_InitStruct.I2S_ClockSource      = I2S_CLK_40M;
 *     I2S_InitStruct.I2S_BClockMi         = 0x271;
 *     I2S_InitStruct.I2S_BClockNi         = 0x10;
 *     I2S_InitStruct.I2S_DeviceMode       = I2S_DeviceMode_Master;
 *     I2S_InitStruct.I2S_ChannelType      = I2S_Channel_Stereo;
 *     I2S_InitStruct.I2S_DataWidth        = I2S_Width_16Bits;
 *     I2S_InitStruct.I2S_DataFormat       = I2S_Mode;
 *     I2S_InitStruct.I2S_DMACmd           = I2S_DMA_DISABLE;
 *     I2S_Init(I2S0, &I2S_InitStruct);
 *     I2S_Cmd(I2S0, I2S_MODE_TX, ENABLE);
 * }
 * \endcode
 */
void I2S_Init(I2S_TypeDef *I2Sx, I2S_InitTypeDef *I2S_InitStruct);

/**
 * \brief   Fills each I2S_InitStruct member with its default value.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \note   The default settings for the I2S_InitStruct member are shown in the following table:
 *         | I2S_InitStruct Member | Default Value                |
 *         |:---------------------:|:----------------------------:|
 *         | I2S_ClockSource       | \ref I2S_CLK_XTAL            |
 *         | I2S_BClockMi          | 0x271                        |
 *         | I2S_BClockNi          | 0x30                         |
 *         | I2S_BClockDiv         | 0x3F                         |
 *         | I2S_DeviceMode        | \ref I2S_DeviceMode_Master   |
 *         | I2S_TxChannelType     | \ref I2S_Channel_Mono        |
 *         | I2S_RxChannelType     | \ref I2S_Channel_Mono        |
 *         | I2S_TxDataFormat      | \ref I2S_Mode                |
 *         | I2S_RxDataFormat      | \ref I2S_Mode                |
 *         | I2S_TxBitSequence     | \ref I2S_TX_MSB_First        |
 *         | I2S_RxBitSequence     | \ref I2S_RX_MSB_First        |
 *         | I2S_TxDataWidth       | \ref I2S_Width_16Bits        |
 *         | I2S_RxDataWidth       | \ref I2S_Width_16Bits        |
 *         | I2S_TxChannelWidth    | \ref I2S_Width_32Bits        |
 *         | I2S_RxChannelWidth    | \ref I2S_Width_32Bits        |
 *         | I2S_TxTdmMode         | \ref I2S_TDM_DISABLE         |
 *         | I2S_RxTdmMode         | \ref I2S_TDM_DISABLE         |
 *         | I2S_DMACmd            | \ref I2S_DMA_ENABLE          |
 *         | I2S_TxWaterlevel      | 16                           |
 *         | I2S_RxWaterlevel      | 16                           |
 *
 * \param[in] I2S_InitStruct: Pointer to an I2S_InitTypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2s_init(void)
 * {
 *     RCC_PeriphClockCmd(APB_I2S, APB_I2S_CLOCK, ENABLE);
 *
 *     I2S_InitTypeDef I2S_InitStruct;
 *
 *     I2S_StructInit(&I2S_InitStruct);
 *     I2S_InitStruct.I2S_ClockSource      = I2S_CLK_40M;
 *     I2S_InitStruct.I2S_BClockMi         = 0x271;
 *     I2S_InitStruct.I2S_BClockNi         = 0x10;
 *     I2S_InitStruct.I2S_DeviceMode       = I2S_DeviceMode_Master;
 *     I2S_InitStruct.I2S_ChannelType      = I2S_Channel_Stereo;
 *     I2S_InitStruct.I2S_DataWidth        = I2S_Width_16Bits;
 *     I2S_InitStruct.I2S_DataFormat       = I2S_Mode;
 *     I2S_InitStruct.I2S_DMACmd           = I2S_DMA_DISABLE;
 *     I2S_Init(I2S0, &I2S_InitStruct);
 *     I2S_Cmd(I2S0, I2S_MODE_TX, ENABLE);
 * }
 * \endcode
 */
void I2S_StructInit(I2S_InitTypeDef *I2S_InitStruct);

/**
 * \brief   Enable or disable the selected I2S mode.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \param[in] mode: Selected I2S operation mode.
 *      This parameter can be the following values:
 *      - I2S_MODE_TX: Transmission mode.
 *      - I2S_MODE_RX: Receiving mode.
 *      - I2S_MODE_TRX: Transmission & Receiving mode.
 * \param[in] NewState: New state of the operation mode.
 *      This parameter can be one of the following values:
 *      - ENABLE: Enable I2S transmission or receiving.
 *      - DISABLE: Disable I2S transmission or receiving.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2s_init(void)
 * {
 *     RCC_PeriphClockCmd(APB_I2S, APB_I2S_CLOCK, ENABLE);
 *
 *     I2S_InitTypeDef I2S_InitStruct;
 *
 *     I2S_StructInit(&I2S_InitStruct);
 *     I2S_InitStruct.I2S_ClockSource      = I2S_CLK_40M;
 *     I2S_InitStruct.I2S_BClockMi         = 0x271;
 *     I2S_InitStruct.I2S_BClockNi         = 0x10;
 *     I2S_InitStruct.I2S_DeviceMode       = I2S_DeviceMode_Master;
 *     I2S_InitStruct.I2S_ChannelType      = I2S_Channel_Stereo;
 *     I2S_InitStruct.I2S_DataWidth        = I2S_Width_16Bits;
 *     I2S_InitStruct.I2S_DataFormat       = I2S_Mode;
 *     I2S_InitStruct.I2S_DMACmd           = I2S_DMA_DISABLE;
 *     I2S_Init(I2S_NUM, &I2S_InitStruct);
 *     I2S_Cmd(I2S_NUM, I2S_MODE_TX, ENABLE);
 * }
 * \endcode
 */
void I2S_Cmd(I2S_TypeDef *I2Sx, uint32_t mode, FunctionalState NewState);

/**
 * \brief   Enable or disable the specified I2S interrupt source.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \param[in] I2S_INT: Specifies the I2S interrupt source to be enable or disable.
 *      This parameter can be the following values:
 *      - I2S_INT_TX_IDLE: Transmit idle interrupt source.
 *      - I2S_INT_RF_EMPTY: Receive FIFO empty interrupt source.
 *      - I2S_INT_TF_EMPTY: Transmit FIFO empty interrupt source.
 *      - I2S_INT_RF_FULL: Receive FIFO full interrupt source.
 *      - I2S_INT_TF_FULL: Transmit FIFO full interrupt source.
 *      - I2S_INT_RX_READY: Ready to receive interrupt source.
 *      - I2S_INT_TX_READY: Ready to transmit interrupt source.
 * \param[in]  NewState: New state of the specified I2S interrupt.
 *      This parameter can be one of the following values:
 *      - ENABLE: Enable the specified I2S interrupt.
 *      - DISABLE: Disable the specified I2S interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2s_demo(void)
 * {
 *     I2S_INTConfig(I2S0, I2S_INT_TF_EMPTY, ENABLE);
 * }
 * \endcode
 */
void I2S_INTConfig(I2S_TypeDef *I2Sx, uint32_t I2S_MCU_INT, FunctionalState NewState);

/**
 * \brief   Get the specified I2S interrupt status.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \param[in] I2S_INT: The specified I2S interrupt.
 *      This parameter can be one of the following values:
 *      - I2S_INT_TX_IDLE: Transmit idle interrupt.
 *      - I2S_INT_RF_EMPTY: Receive FIFO empty interrupt.
 *      - I2S_INT_TF_EMPTY: Transmit FIFO empty interrupt.
 *      - I2S_INT_RF_FULL: Receive FIFO full interrupt.
 *      - I2S_INT_TF_FULL: Transmit FIFO full interrupt.
 *      - I2S_INT_RX_READY: Ready to receive interrupt.
 *      - I2S_INT_TX_READY: Ready to transmit interrupt.
 * \return The new state of I2S_INT (SET or RESET).
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2s_demo(void)
 * {
 *     ITStatus int_status = I2S_GetINTStatus(I2S0, I2S_INT_TF_EMPTY);
 * }
 * \endcode
 */
ITStatus I2S_GetINTStatus(I2S_TypeDef *I2Sx, uint32_t I2S_INT);

/**
  * \brief  Clears the I2S interrupt pending bits.
  * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \param[in] I2S_CLEAR_INT: Specifies the interrupt pending bit to clear.
 *   This parameter can be any combination of the following values:
 *     - I2S_CLEAR_INT_RX_READY: Clear ready to receive interrupt.
 *     - I2S_CLEAR_INT_TX_READY: Clear ready to transmit interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void I2S_RX_Handler(void)
 * {
 *     if (I2S_GetINTStatus(I2S_NUM, I2S_MCU_INT_RX_READY))
 *     {
 *        //add user code here
 *        I2S_ClearINTPendingBit(I2S_NUM, I2S_CLEAR_INT_RX_READY);
 *     }
 *
 * }
 * \endcode
 */
void I2S_ClearINTPendingBit(I2S_TypeDef *I2Sx, uint32_t I2S_CLEAR_INT);

/**
 * \brief  Select I2S connect to external I2S peripheral or internal codec.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \param[in] NewState: New state of the I2S bridge selection.
 *      This parameter can be one of the following values:
 *      - ENABLE: Enable the I2Sx connect to internal codec.
 *      - DISABLE: Disable the I2Sx connect to internal codec.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2s_demo(void)
 * {
 *     I2S_CodecSelInternal(I2S0, ENABLE);
 * }
 * \endcode
 */
void I2S_CodecSelInternal(I2S_TypeDef *I2Sx, FunctionalState newState);

/**
  * \brief  Transmits a Data through the I2Sx peripheral.
  * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \param[in] Data: Data to be transmitted.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2s_send_data(void)
 * {
 *     uint32_t pattern = 0x12345678;
 *     I2S_SendData(I2S0, pattern);
 *
 * }
 * \endcode
 */
__STATIC_INLINE void I2S_SendData(I2S_TypeDef *I2Sx, uint32_t Data)
{
    /* Check the parameters */
    assert_param(IS_I2S_ALL_PERIPH(I2Sx));

    I2Sx->TX_FIFO_0_WR = Data;
}

/**
  * \brief  Returns the most recent received data by the I2Sx peripheral.
  * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \return The value of the received data.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void I2S_RX_Handler(void)
 * {
 *     if (I2S_GetINTStatus(I2S0, I2S_MCU_INT_RX_READY))
 *     {
 *        uint8_t len = I2S_GetRxFIFOLen(I2S0);
 *        for (uint8_t i = 0; i < len; i++)
 *        {
 *           data = I2S_ReceiveData(I2S0);
 *        }
 *     }
 *
 * }
 * \endcode
 */
__STATIC_INLINE uint32_t I2S_ReceiveData(I2S_TypeDef *I2Sx)
{
    /* Check the parameters */
    assert_param(IS_I2S_ALL_PERIPH(I2Sx));

    return I2Sx->RX_FIFO_0_RD;
}

/**
  * \brief  Returns the transmit FIFO free length by the I2Sx peripheral.
  * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \return The free length of the transmit FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2s_send_data(void)
 * {
 *     if (I2S_GetTxFIFODepth(I2S0))
 *     {
 *        //add user code here
 *        I2S_SendData(I2S_NUM, pattern);
 *     }
 *
 * }
 * \endcode
 */
__STATIC_INLINE uint8_t I2S_GetTxFIFODepth(I2S_TypeDef *I2Sx)
{
    /* Check the parameters */
    assert_param(IS_I2S_ALL_PERIPH(I2Sx));

    return ((I2Sx->DEPTH_CNT & 0x3F));
}

/**
  * \brief  Returns the receive FIFO data length by the I2Sx peripheral.
  * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] I2Sx: Selected I2S peripheral, where x can be 0 or 1 \ref x3e_I2S_Declaration.
 * \return The data length of the receive FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void I2S_RX_Handler(void)
 * {
 *     if (I2S_GetINTStatus(I2S0, I2S_MCU_INT_RX_READY))
 *     {
 *        uint8_t len = I2S_GetRxFIFOLen(I2S0);
 *        for (uint8_t i = 0; i < len; i++)
 *        {
 *           data = I2S_ReceiveData(I2S0);
 *        }
 *     }
 *
 * }
 * \endcode
 */
__STATIC_INLINE uint8_t I2S_GetRxFIFOLen(I2S_TypeDef *I2Sx)
{
    /* Check the parameters */
    assert_param(IS_I2S_ALL_PERIPH(I2Sx));

    return ((I2Sx->DEPTH_CNT & 0x3F00) >> 8);
}

/** \} */ /*End of group 87x3e_I2S_Exported_Functions */
/** @} */ /* End of group 87x3e_I2S */

#ifdef __cplusplus
}
#endif
#endif /* _RTL876x_I2S_H_ */
/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

