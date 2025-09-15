/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_i2c.h
* @brief
* @details
* @author    elliot chen
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef __RTL876X_I2C_H
#define __RTL876X_I2C_H

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"

#include "compiler_abstraction.h"

/** @addtogroup 87x3e_I2C I2C
  * @brief I2C driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup 87x3e_I2C_Exported_Types I2C Exported Types
  * @{
  */

/**
  * @brief  I2C Init structure definition.
  */

typedef struct
{
    uint32_t I2C_Clock;           /*!< Specifies the I2C clock source frequency, default 40000000.
                                                  This parameter can be set with @ref x3e_I2C_Clock_Divider. I2C_Clock = 40000000 / I2C_Clock_Divider. */

    uint32_t I2C_ClockSpeed;      /*!< Specifies the I2C clock speed.
                                                  This parameter must be set to a value lower than 1MHz. */

    uint16_t I2C_DeviveMode;      /*!< Specifies the I2C device mode.
                                                  This parameter can be a value of @ref x3e_I2C_device_mode. */

    uint16_t I2C_AddressMode;     /*!< Specifies the I2C address mode.
                                                                This parameter can be a value of @ref x3e_I2C_address_mode. */

    uint16_t I2C_SlaveAddress;    /*!< Specifies the I2C slave address.
                                                  This parameter can be a 7-bit or 10-bit address, must range from 0x0 to 0x3FF. */

    uint16_t I2C_Ack;             /*!< Enables or disables the acknowledgement only in slave mode.
                                                  This parameter can be a value of @ref x3e_I2C_acknowledgement. */

    uint32_t I2C_TxThresholdLevel;  /*!< Specifies the transmit FIFO threshold to trigger interrupt \ref I2C_INT_TX_EMPTY.
                                                  This parameter can be a value less than 24. */

    uint32_t I2C_RxThresholdLevel;  /*!< Specifies the receive FIFO Threshold to trigger interrupt \ref I2C_INT_RX_FULL.
                                                  This parameter can be a value less than 40. */

    uint16_t I2C_TxDmaEn;               /*!< Specifies the I2C TX DMA mode.
                                                  This parameter can be a value of ENABLE or DISABLE. */

    uint16_t I2C_RxDmaEn;               /*!< Specifies the I2C RX DMA mode.
                                                  This parameter can be a value of ENABLE or DISABLE. */

    uint8_t  I2C_TxWaterlevel;          /*!< Specifies the DMA TX water level. This parameter must range from 1 to 23.
                                                  I2C_TxWaterlevel = I2C TX FIFO depth - I2C TX GDMA MSize. */

    uint8_t  I2C_RxWaterlevel;          /*!< Specifies the DMA RX water level. This parameter must range from 1 to 39.
                                                  I2C_RxWaterlevel = I2C RX GDMA MSize - 1. */

    uint8_t  I2C_RisingTimeNs;            /*!< Specifies the I2C SDA/SCL rising time.
                                                  The unit is ns and must be an integer multiple of clock src period. This parameter must range from 0x1 to 0xff. */
} I2C_InitTypeDef;

/** End of group 87x3e_I2C_Exported_Types
  * @}
  */


/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_I2C_Exported_Constants I2C Exported Constants
  * @{
  */

/** @defgroup 87x3e_I2C_Declaration I2C Declaration
  * @{
  */
#define I2C0                            ((I2C_TypeDef              *) I2C0_REG_BASE) //!< The I2C0 base address.
#define I2C1                            ((I2C_TypeDef              *) I2C1_REG_BASE) //!< The I2C1 base address.
#define I2C2                            ((I2C_TypeDef              *) I2C2_REG_BASE) //!< The I2C2 base address.

#define IS_I2C_ALL_PERIPH(PERIPH) (((PERIPH) == I2C0) || \
                                   ((PERIPH) == I2C1) || \
                                   ((PERIPH) == I2C2)) //!< I2C peripherals can select I2C0, I2C1, or I2C2.
/** End of group 87x3e_I2C_Declaration
  * @}
  */

/** @defgroup 87x3e_I2C_clock_speed I2C Clock Speed
  * @{
  */

#define IS_I2C_CLOCK_SPEED(SPEED) (((SPEED) >= 0x01) && ((SPEED) <= 1000000)) //!< I2C clock speed is between 1 and 1000000.

/** End of group 87x3e_I2C_clock_speed
  * @}
  */

/** @defgroup 87x3e_I2C_device_mode I2C Device Mode
  * @{
  */

#define I2C_DeviveMode_Master                   ((uint16_t)0x0041) //!< I2C device operating mode as master.
#define I2C_DeviveMode_Slave                    ((uint16_t)0x0000) //!< I2C device operating mode as slave.

/** End of group 87x3e_I2C_device_mode
  * @}
  */

/** @defgroup 87x3e_I2C_address_mode I2C Address Mode
  * @{
  */

#define I2C_AddressMode_7BIT                    ((uint16_t)0x0000) //!< I2C address mode using 7-bit addressing.
#define I2C_AddressMode_10BIT                   ((uint16_t)0x0001) //!< I2C address mode using 10-bit addressing.

/** End of group 87x3e_I2C_address_mode
  * @}
  */

/** @defgroup 87x3e_I2C_acknowledgement I2C Acknowledgement
  * @{
  */

#define I2C_Ack_Enable                          ((uint16_t)0x0001) //!< I2C acknowledgment is enabled.
#define I2C_Ack_Disable                         ((uint16_t)0x0000) //!< I2C acknowledgment is disabled.

/** End of group 87x3e_I2C_acknowledgement
  * @}
  */

/** @defgroup 87x3e_I2C_flags_definition I2C Flags Definition
  * @{
  */

#define I2C_FLAG_SLV_ACTIVITY                   ((uint32_t)0x00000040) //!< Slave FSM(finite state machine) activity status.
#define I2C_FLAG_MST_ACTIVITY                   ((uint32_t)0x00000020) //!< Master FSM(finite state machine) activity status.
#define I2C_FLAG_RFF                            ((uint32_t)0x00000010) //!< Receive FIFO completely full. 
#define I2C_FLAG_RFNE                           ((uint32_t)0x00000008) //!< Receive FIFO not empty. 
#define I2C_FLAG_TFE                            ((uint32_t)0x00000004) //!< Transmit FIFO completely empty.
#define I2C_FLAG_TFNF                           ((uint32_t)0x00000002) //!< Transmit FIFO not full. 
#define I2C_FLAG_ACTIVITY                       ((uint32_t)0x00000001) //!< I2C activity status.

#define IS_I2C_GET_FLAG(FLAG) (((FLAG) == I2C_FLAG_SLV_ACTIVITY) || ((FLAG) == I2C_FLAG_MST_ACTIVITY) || \
                               ((FLAG) == I2C_FLAG_RFF) || ((FLAG) == I2C_FLAG_RFNE) || \
                               ((FLAG) == I2C_FLAG_TFE) || ((FLAG) == I2C_FLAG_TFNF) || \
                               ((FLAG) == I2C_FLAG_ACTIVITY)) //!< Check if the input parameter is valid.
/** End of group 87x3e_I2C_flags_definition
  * @}
  */

/** @defgroup 87x3e_I2C_transmit_Abort_Source I2C Transmit Abort Source
  * @{
  */

#define ABRT_SLVRD_INTX                         ((uint32_t)BIT(15)) //!< When the processor side responds to a slave mode request for data to be transmitted to a remote master and user send read command.
#define ABRT_SLV_ARBLOST                        ((uint32_t)BIT(14)) //!< Slave lost the bus while transmitting data to a remote master.
#define ABRT_SLVFLUSH_TXFIFO                    ((uint32_t)BIT(13)) //!< Slave has received a read command and some data exists in the TX FIFO so the slave issues TX abort interrupt to flush old data in TX FIFO.
#define ARB_LOST                                ((uint32_t)BIT(12)) //!< Master has lost arbitration or the slave transmitter has lost arbitration.
#define ABRT_MASTER_DIS                         ((uint32_t)BIT(11)) //!< User tries to initiate a master operation with the master mode disabled.
#define ABRT_10B_RD_NORSTRT                     ((uint32_t)BIT(10)) //!< The restart is disabled and the master sends a read command in 10-bit addressing mode.
#define ABRT_SBYTE_NORSTRT                      ((uint32_t)BIT(9))  //!< The restart is disabled and the user is trying to send a start byte.
#define ABRT_HS_NORSTRT                         ((uint32_t)BIT(8))  //!< The restart is disabled and the user is trying to use the master to transfer data in high speed mode.
#define ABRT_SBYTE_ACKDET                       ((uint32_t)BIT(7))  //!< Master has sent a start byte and the start byte was acknowledged (wrong behavior).
#define ABRT_HS_ACKDET                          ((uint32_t)BIT(6))  //!< Master is in high speed mode and the high speed master code was acknowledged (wrong behavior).
#define ABRT_GCALL_READ                         ((uint32_t)BIT(5))  //!< I2C in master mode sent a general call but the user programmed the byte following the general call to be a read from the bus.
#define ABRT_GCALL_NOACK                        ((uint32_t)BIT(4))  //!< I2C in master mode sent a general call and no slave on the bus acknowledged the general call.
#define ABRT_TXDATA_NOACK                       ((uint32_t)BIT(3))  //!< This is a master-mode only bit. master has received an acknowledgement for the address, but when it sent data byte(s) following the address, it did not receive an acknowledge from the remote slave(s).
#define ABRT_10ADDR2_NOACK                      ((uint32_t)BIT(2))  //!< Master is in 10-bit address mode and the second address byte of the 10-bit address was not acknowledged by any slave.
#define ABRT_10ADDR1_NOACK                      ((uint32_t)BIT(1))  //!< Master is in 10-bit address mode and the first 10-bit address byte was not acknowledged by any slave.
#define ABRT_7B_ADDR_NOACK                      ((uint32_t)BIT(0))  //!< Master is in 7-bit address mode and the address sent was not acknowledged by any slave.

#define MS_ALL_ABORT                            (ARB_LOST | ABRT_MASTER_DIS | ABRT_TXDATA_NOACK |\
                                                 ABRT_10ADDR2_NOACK | ABRT_10ADDR1_NOACK | ABRT_7B_ADDR_NOACK) //!< Contains all master abort sources.

#define IS_I2C_EVENT(EVENT) (((EVENT) == ABRT_SLVRD_INTX) || \
                             ((EVENT) == ABRT_SLV_ARBLOST) || \
                             ((EVENT) == ABRT_SLVFLUSH_TXFIFO) || \
                             ((EVENT) == ARB_LOST) || \
                             ((EVENT) == ABRT_MASTER_DIS) || \
                             ((EVENT) == ABRT_10B_RD_NORSTRT) || \
                             ((EVENT) == ABRT_SBYTE_NORSTRT) || \
                             ((EVENT) == ABRT_HS_NORSTRT) || \
                             ((EVENT) == ABRT_SBYTE_ACKDET) || \
                             ((EVENT) == ABRT_HS_ACKDET) || \
                             ((EVENT) == ABRT_GCALL_READ) || \
                             ((EVENT) == ABRT_GCALL_NOACK) || \
                             ((EVENT) == ABRT_TXDATA_NOACK) || \
                             ((EVENT) == ABRT_10ADDR2_NOACK) || \
                             ((EVENT) == ABRT_10ADDR1_NOACK) || \
                             ((EVENT) == ABRT_7B_ADDR_NOACK)) //!< Check if the input parameter is valid.
/** End of group 87x3e_I2C_transmit_Abort_Source
  * @}
  */

/** @defgroup 87x3e_I2C_interrupts_definition I2C Interrupts Definition
  * @{
  */

#define I2C_INT_GEN_CALL                                ((uint32_t)BIT(11)) //!< I2C general call interrupt. When a general call address is received and it is acknowledged.
#define I2C_INT_START_DET                               ((uint32_t)BIT(10)) //!< I2C start detect interrupt. When a start or restart condition has occurred on the I2C interface.
#define I2C_INT_STOP_DET                                ((uint32_t)BIT(9))  //!< I2C stop detect interrupt. When a stop condition has occurred on the I2C interface.
#define I2C_INT_ACTIVITY                                ((uint32_t)BIT(8))  //!< I2C activity interrupt. When I2C is activity on the bus.
#define I2C_INT_RX_DONE                                 ((uint32_t)BIT(7))  //!< I2C slave RX done interrupt. When the I2C is acting as a slave-transmitter and the master does not acknowledge a transmitted byte. This occurs on the last byte of the transmission, indicating that the transmission is done.
#define I2C_INT_TX_ABRT                                 ((uint32_t)BIT(6))  //!< I2C TX abort interrupt. When an I2C transmitter is unable to complete the intended actions on the contents of the transmit FIFO.
#define I2C_INT_RD_REQ                                  ((uint32_t)BIT(5))  //!< I2C slave RX request interrupt. When I2C is acting as a slave and another I2C master is attempting to read data from I2C.
#define I2C_INT_TX_EMPTY                                ((uint32_t)BIT(4))  //!< I2C TX FIFO empty interrupt. When the transmit buffer is at or below the threshold value.
#define I2C_INT_TX_OVER                                 ((uint32_t)BIT(3))  //!< I2C TX FIFO overflow interrupt. When transmit buffer is filled to 24 and the processor attempts to issue another I2C command.
#define I2C_INT_RX_FULL                                 ((uint32_t)BIT(2))  //!< I2C RX FIFO full interrupt. When the receive buffer reaches or goes above the RX FIFO threshold value.
#define I2C_INT_RX_OVER                                 ((uint32_t)BIT(1))  //!< I2C RX FIFO overflow interrupt. When the receive buffer is completely filled to 40 and an additional byte is received from an external I2C device.
#define I2C_INT_RX_UNDER                                ((uint32_t)BIT(0))  //!< I2C RX FIFO underflow interrupt. When the processor attempts to read the receive buffer when it is empty.

#define I2C_GET_INT(INT)    (((INT) == I2C_INT_GEN_CALL) || ((INT) == I2C_INT_START_DET) || \
                             ((INT) == I2C_INT_STOP_DET) || ((INT) == I2C_INT_ACTIVITY) || \
                             ((INT) == I2C_INT_RX_DONE)  || ((INT) == I2C_INT_TX_ABRT) || \
                             ((INT) == I2C_INT_RD_REQ)   || ((INT) == I2C_INT_TX_EMPTY) || \
                             ((INT) == I2C_INT_TX_OVER)  || ((INT) == I2C_INT_RX_FULL) || \
                             ((INT) == I2C_INT_RX_OVER)  || ((INT) == I2C_INT_RX_UNDER)) //!< Check if the input parameter is valid.
/** End of group 87x3e_I2C_interrupts_definition
  * @}
  */

/** @defgroup 87x3e_I2C_GDMA_transfer_requests  I2C GDMA Transfer Requests
  * @{
  */

#define I2C_GDMAReq_Tx               ((uint16_t)0x0002) //!< TX buffer GDMA transfer request.
#define I2C_GDMAReq_Rx               ((uint16_t)0x0001) //!< RX buffer GDMA transfer request.
#define IS_I2C_GDMAREQ(GDMAREQ) ((((GDMAREQ) & (uint16_t)0xFFFC) == 0x00) && ((GDMAREQ) != 0x00)) //!< Check if the input parameter is valid.

/** End of group 87x3e_I2C_GDMA_transfer_requests
  * @}
  */

/** @defgroup 87x3e_I2C_send_command  I2C Send Command
  * @{
  */

#define I2C_WRITE_CMD       0     //!< I2C write command.
#define I2C_READ_CMD        BIT8  //!< I2C read command.
#define IS_I2C_CMD(CMD) (((CMD) == I2C_WRITE_CMD) || ((CMD) == I2C_READ_CMD)) //!< Check if the input parameter is valid.

/** End of group 87x3e_I2C_send_command
  * @}
  */

/** @defgroup 87x3e_I2C_send_stop  I2C Send Stop
  * @{
  */

#define I2C_STOP_ENABLE     BIT9  //!< I2C stop will be issued.
#define I2C_STOP_DISABLE    0     //!< I2C stop will not be issued.
#define IS_I2C_STOP(CMD) (((CMD) == I2C_STOP_ENABLE) || ((CMD) == I2C_STOP_DISABLE)) //!< Check if the input parameter is valid.

/** End of group 87x3e_I2C_send_stop
  * @}
  */

/** @defgroup 87x3e_I2C_Status  I2C Status
  * @{
  */

typedef enum
{
    I2C_Success,              //!< I2C success.
    I2C_ARB_LOST,             //!< Master or slave transmitter losed arbitration.
    I2C_ABRT_MASTER_DIS,      //!< User tried to initiate a master operation with the master mode disabled.
    I2C_ABRT_TXDATA_NOACK,    //!< Master sent data byte(s) following the address, but it did not receive an acknowledge from the remote slave.
    I2C_ABRT_10ADDR2_NOACK,   //!< Master is in 10-bit address mode and the second address byte of the 10-bit address was not acknowledged by any slave.
    I2C_ABRT_10ADDR1_NOACK,   //!< Master is in 10-bit address mode and the first 10-bit address byte was not acknowledged by any slave.
    I2C_ABRT_7B_ADDR_NOACK,   //!< Master is in 7-bit address mode and the address sent was not acknowledged by any slave.
    I2C_TIMEOUT,              //!< I2C timeout.
} I2C_Status;


/** End of group 87x3e_I2C_Status
  * @}
  */

/** End of group 87x3e_I2C_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup 87x3e_I2C_Exported_Functions I2C Exported Functions
 * @{
 */

/**
 *
 * \brief   Disable the I2Cx peripheral clock, and restore registers to their default values.
 *
 * \param[in]  I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2c0_init(void)
 * {
 *     I2C_DeInit(I2C0);
 * }
 * \endcode
 */
void I2C_DeInit(I2C_TypeDef *I2Cx);

/**
 *
 * \brief   Initializes the I2Cx peripheral according to the specified
 *          parameters in the I2C_InitStruct.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] I2C_InitStruct: Pointer to a \ref I2C_InitTypeDef structure that
 *            contains the configuration information for the specified I2C peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2c0_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);
 *
 *     I2C_InitTypeDef  I2C_InitStruct;
 *     I2C_StructInit(&I2C_InitStruct);
 *
 *     I2C_InitStruct.I2C_ClockSpeed    = 100000;
 *     I2C_InitStruct.I2C_DeviveMode    = I2C_DeviveMode_Master;
 *     I2C_InitStruct.I2C_AddressMode   = I2C_AddressMode_7BIT;
 *     I2C_InitStruct.I2C_SlaveAddress  = 0x50;
 *     I2C_InitStruct.I2C_Ack           = I2C_Ack_Enable;
 *
 *     I2C_Init(I2C0, &I2C_InitStruct);
 * }
 * \endcode
 */
void I2C_Init(I2C_TypeDef *I2Cx, I2C_InitTypeDef *I2C_InitStruct);

/**
 *
 * \brief  Enable or disable the specified I2C peripheral.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] NewState: New state of the I2Cx peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified I2C peripheral, allowing it to begin data transfer operations.
 *            - DISABLE: Disable the specified I2C peripheral, TX and RX FIFOs are held in an erased state.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2c0_init(void)
 * {
 *     I2C_Cmd(I2C0, ENABLE);
 * }
 * \endcode
 */
void I2C_Cmd(I2C_TypeDef *I2Cx, FunctionalState NewState);

/**
 *
 * \brief   Fills each I2C_InitStruct member with its default value.
 *
 * \note   The default settings for the I2C_InitStruct member are shown in the following table:
 *         | I2C_InitStruct Member | Default Value                    |
 *         |:---------------------:|:--------------------------------:|
 *         | I2C_Clock             | 40000000                         |
 *         | I2C_ClockSpeed        | 400000                           |
 *         | I2C_DeviveMode        | \ref I2C_DeviveMode_Master       |
 *         | I2C_AddressMode       | \ref I2C_AddressMode_7BIT        |
 *         | I2C_SlaveAddress      | 0                                |
 *         | I2C_Ack               | \ref I2C_Ack_Enable              |
 *         | I2C_TxThresholdLevel  | 0x00                             |
 *         | I2C_RxThresholdLevel  | 0x00                             |
 *         | I2C_TxDmaEn           | DISABLE                          |
 *         | I2C_RxDmaEn           | DISABLE                          |
 *         | I2C_RxWaterlevel      | 1                                |
 *         | I2C_TxWaterlevel      | 15                               |
 *         | I2C_RisingTimeNs      | 50                               |
 *
 * \param[in] I2C_InitStruct: Pointer to a \ref I2C_InitTypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2c0_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);
 *
 *     I2C_InitTypeDef  I2C_InitStruct;
 *     I2C_StructInit(&I2C_InitStruct);
 *
 *     I2C_InitStruct.I2C_ClockSpeed    = 100000;
 *     I2C_InitStruct.I2C_DeviveMode    = I2C_DeviveMode_Master;
 *     I2C_InitStruct.I2C_AddressMode   = I2C_AddressMode_7BIT;
 *     I2C_InitStruct.I2C_SlaveAddress  = 0x50;
 *     I2C_InitStruct.I2C_Ack           = I2C_Ack_Enable;
 *
 *     I2C_Init(I2C0, &I2C_InitStruct);
 * }
 * \endcode
 */
void I2C_StructInit(I2C_InitTypeDef *I2C_InitStruct);

/**
 *
 * \brief   Send data in master mode through the I2Cx peripheral.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] pBuf: Byte to be transmitted. This parameter must range from 0x0 to 0xFF.
 * \param[in] len: Data length to send. This parameter must range from 0x1 to 0xFFFF.
 *
 * \return I2C status, please refer to \ref x3e_I2C_Status.
 * \retval I2C_Success: I2C success.
 * \retval I2C_ARB_LOST: Master or slave transmitter losed arbitration.
 * \retval I2C_ABRT_MASTER_DIS: User tried to initiate a master operation with the master mode disabled.
 * \retval I2C_ABRT_TXDATA_NOACK: Master sent data byte(s) following the address, but it did not receive an acknowledge from the remote slave.
 * \retval I2C_ABRT_10ADDR2_NOACK: Master is in 10-bit address mode and the second address byte of the 10-bit address was not acknowledged by any slave.
 * \retval I2C_ABRT_10ADDR1_NOACK: Master is in 10-bit address mode and the first 10-bit address byte was not acknowledged by any slave.
 * \retval I2C_ABRT_7B_ADDR_NOACK: Master is in 7-bit address mode and the address sent was not acknowledged by any slave.
 * \retval I2C_TIMEOUT: I2C timeout.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c0_demo(void)
 * {
 *     uint8_t data[10] = {0x01, x0x02, 0x03, 0x04};
 *     I2C_MasterWrite(I2C0, data, 4);
 * }
 * \endcode
 */
I2C_Status I2C_MasterWrite(I2C_TypeDef *I2Cx, uint8_t *pBuf, uint16_t len);

/**
 *
 * \brief   Read data in master mode through the I2Cx peripheral.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] pBuf: Data buffer to receive data. This parameter must range from 0x0 to 0xFF.
 * \param[in] len: Read data length. This parameter must range from 0x1 to 0xFFFF.
 *
 * \return I2C status, please refer to \ref x3e_I2C_Status.
 * \retval I2C_Success: I2C success.
 * \retval I2C_ARB_LOST: Master or slave transmitter losed arbitration.
 * \retval I2C_ABRT_MASTER_DIS: User tried to initiate a master operation with the master mode disabled.
 * \retval I2C_ABRT_TXDATA_NOACK: Master sent data byte(s) following the address, but it did not receive an acknowledge from the remote slave.
 * \retval I2C_ABRT_10ADDR2_NOACK: Master is in 10-bit address mode and the second address byte of the 10-bit address was not acknowledged by any slave.
 * \retval I2C_ABRT_10ADDR1_NOACK: Master is in 10-bit address mode and the first 10-bit address byte was not acknowledged by any slave.
 * \retval I2C_ABRT_7B_ADDR_NOACK: Master is in 7-bit address mode and the address sent was not acknowledged by any slave.
 * \retval I2C_TIMEOUT: I2C timeout.
 *
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c0_demo(void)
 * {
 *     uint8_t data[10] = {0};
 *     I2C_MasterRead(I2C0, data, 10);
 * }
 * \endcode
 */
I2C_Status I2C_MasterRead(I2C_TypeDef *I2Cx, uint8_t *pBuf, uint16_t len);

/**
 *
 * \brief   Sends data and read data in master mode through the I2Cx peripheral.
 *          Attention:Read data with time out mechanism.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] pWriteBuf: Data buffer to send before read. This parameter must range from 0x0 to 0xFF.
 * \param[in] Writelen: Data length to send. This parameter must range from 0x1 to 0xFFFF.
 * \param[in] pReadBuf: Data buffer to receive. This parameter must range from 0x0 to 0xFF.
 * \param[in] Readlen: Data length to receive. This parameter must range from 0x1 to 0xFFFF.
 *
 * \return I2C status, please refer to \ref x3e_I2C_Status.
 * \retval I2C_Success: I2C success.
 * \retval I2C_ARB_LOST: Master or slave transmitter losed arbitration.
 * \retval I2C_ABRT_MASTER_DIS: User tried to initiate a master operation with the master mode disabled.
 * \retval I2C_ABRT_TXDATA_NOACK: Master sent data byte(s) following the address, but it did not receive an acknowledge from the remote slave.
 * \retval I2C_ABRT_10ADDR2_NOACK: Master is in 10-bit address mode and the second address byte of the 10-bit address was not acknowledged by any slave.
 * \retval I2C_ABRT_10ADDR1_NOACK: Master is in 10-bit address mode and the first 10-bit address byte was not acknowledged by any slave.
 * \retval I2C_ABRT_7B_ADDR_NOACK: Master is in 7-bit address mode and the address sent was not acknowledged by any slave.
 * \retval I2C_TIMEOUT: I2C timeout.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c0_demo(void)
 * {
 *     uint8_t tx_data[10] = {0x01,x0x02,0x03,0x04};
 *     uint8_t rx_data[10] = {0};
 *     I2C_RepeatRead(I2C0, tx_data, 4, rx_data, 10);
 * }
 * \endcode
 */
I2C_Status I2C_RepeatRead(I2C_TypeDef *I2Cx, uint8_t *pWriteBuf, uint16_t Writelen,
                          uint8_t *pReadBuf, uint16_t Readlen);

/**
 *
 * \brief     Mask the specified I2C interrupt or not.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] I2C_IT: Specified the I2C interrupt \ref x3e_I2C_interrupts_definition.
 *            This parameter can be one of the following values:
 *            - I2C_INT_GEN_CALL: I2C general call interrupt. When a general call address is received and it is acknowledged.
 *            - I2C_INT_START_DET: I2C start detect interrupt. When a start or restart condition has occurred on the I2C interface.
 *            - I2C_INT_STOP_DET: I2C stop detect interrupt. When a stop condition has occurred on the I2C interface.
 *            - I2C_INT_ACTIVITY: I2C activity interrupt. When I2C is activity on the bus.
 *            - I2C_INT_RX_DONE: I2C slave RX done interrupt. When the I2C is acting as a slave-transmitter and the master does not acknowledge a transmitted byte. This occurs on the last byte of the transmission, indicating that the transmission is done.
 *            - I2C_INT_TX_ABRT: I2C TX abort interrupt. When an I2C transmitter is unable to complete the intended actions on the contents of the transmit FIFO.
 *            - I2C_INT_RD_REQ: I2C slave RX request interrupt. When I2C is acting as a slave and another I2C master is attempting to read data from I2C.
 *            - I2C_INT_TX_EMPTY: I2C TX FIFO empty interrupt. When the transmit buffer is at or below the threshold value.
 *            - I2C_INT_TX_OVER: I2C TX FIFO overflow interrupt. When transmit buffer is filled to 24 and the processor attempts to issue another I2C command.
 *            - I2C_INT_RX_FULL: I2C RX FIFO full interrupt. When the receive buffer reaches or goes above the RX FIFO threshold value.
 *            - I2C_INT_RX_OVER: I2C RX FIFO overflow interrupt. When the receive buffer is completely filled to 40 and an additional byte is received from an external I2C device.
 *            - I2C_INT_RX_UNDER: I2C RX FIFO underflow interrupt. When the processor attempts to read the receive buffer when it is empty.
 * \param[in] NewState: Mask the specified I2C interrupt or not.
 *            This parameter can be one of the following values:
 *            - ENABLE: Mask the specified I2C interrupt.
 *            - DISABLE: Unmask the specified I2C interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2c0_init(void)
 * {
 *     I2C_INTConfig(I2C0, I2C_INT_STOP_DET | I2C_INT_RX_FULL, ENABLE);
 *     RamVectorTableUpdate(I2C0_VECTORn, (IRQ_Fun)I2C0_Handler);
 *
 *     NVIC_InitTypeDef NVIC_InitStruct;
 *     NVIC_InitStruct.NVIC_IRQChannel = I2C0_IRQn;
 *     NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
 *     NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
 *     NVIC_Init(&NVIC_InitStruct);
 * }
 * \endcode
 */
void I2C_INTConfig(I2C_TypeDef *I2Cx, uint16_t I2C_IT, FunctionalState NewState);

/**
 *
 * \brief   Clear the specified I2C interrupt pending bit.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] I2C_IT: Specified the I2C interrupt \ref x3e_I2C_interrupts_definition.
 *            This parameter can be one of the following values:
 *            - I2C_INT_GEN_CALL: I2C general call interrupt. When a general call address is received and it is acknowledged.
 *            - I2C_INT_START_DET: I2C start detect interrupt. When a start or restart condition has occurred on the I2C interface.
 *            - I2C_INT_STOP_DET: I2C stop detect interrupt. When a stop condition has occurred on the I2C interface.
 *            - I2C_INT_ACTIVITY: I2C activity interrupt. When I2C is activity on the bus.
 *            - I2C_INT_RX_DONE: I2C slave RX done interrupt. When the I2C is acting as a slave-transmitter and the master does not acknowledge a transmitted byte. This occurs on the last byte of the transmission, indicating that the transmission is done.
 *            - I2C_INT_TX_ABRT: I2C TX abort interrupt. When an I2C transmitter is unable to complete the intended actions on the contents of the transmit FIFO.
 *            - I2C_INT_RD_REQ: I2C slave RX request interrupt. When I2C is acting as a slave and another I2C master is attempting to read data from I2C.
 *            - I2C_INT_TX_EMPTY: I2C TX FIFO empty interrupt. When the transmit buffer is at or below the threshold value.
 *            - I2C_INT_TX_OVER: I2C TX FIFO overflow interrupt. When transmit buffer is filled to 24 and the processor attempts to issue another I2C command.
 *            - I2C_INT_RX_FULL: I2C RX FIFO full interrupt. When the receive buffer reaches or goes above the RX FIFO threshold value.
 *            - I2C_INT_RX_OVER: I2C RX FIFO overflow interrupt. When the receive buffer is completely filled to 40 and an additional byte is received from an external I2C device.
 *            - I2C_INT_RX_UNDER: I2C RX FIFO underflow interrupt. When the processor attempts to read the receive buffer when it is empty.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void I2C0_Handler(void)
 * {
 *     if (I2C_GetINTStatus(I2C0, I2C_INT_STOP_DET) == SET)
 *     {
 *         //Add user code here.
 *         I2C_ClearINTPendingBit(I2C0, I2C_INT_STOP_DET);
 *     }
 * }
 * \endcode
 */
void I2C_ClearINTPendingBit(I2C_TypeDef *I2Cx, uint16_t I2C_IT);

/**
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"

 * \brief   Set the I2C clock speed, the function need to be called when I2C disabled.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] I2C_ClockSpeed: Specified the I2C clock speed. This parameter must range from 1 to 1000000 (1MHz).
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c_demo(void)
 * {
 *     I2C_SetClockSpeed(I2C0, 400000);
 * }
 * \endcode
 */
void I2C_SetClockSpeed(I2C_TypeDef *I2Cx, uint32_t I2C_ClockSpeed);

/**
 *
 * \brief     Set slave device address.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] Address: Specifies the slave address which will be transmitted. This parameter must range from 0x0 to 0xFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c0_demo(void)
 * {
 *     uint16_t slave_address = 0x55;
 *     I2C_SetSlaveAddress(I2C0, slave_address);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void I2C_SetSlaveAddress(I2C_TypeDef *I2Cx, uint16_t Address)
{
    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));

    /* Mask target address */
    I2Cx->IC_TAR &= ~((uint16_t)0x3ff);
    /* Configure new target address */
    I2Cx->IC_TAR |= Address & 0x3ff;
}

/**
 *
 * \brief   Write command through the I2Cx peripheral.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] command: Command of write or read \ref x3e_I2C_send_command.
 *            - I2C_READ_CMD: Read command. Data which want to receive can be 0 in this situation.
 *            - I2C_WRITE_CMD: Write command. Data which want to transmit can be 1 in this situation.
 * \param[in] data: Data which to be transmitted. This parameter must range from 0x0 to 0xFF.
 * \param[in] StopState: Whether a Stop is issued after the byte is sent or received \ref x3e_I2C_send_stop.
 *            - I2C_STOP_ENABLE: Send stop signal.
 *            - I2C_STOP_DISABLE: Do not send stop signal.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c0_demo(void)
 * {
 *     I2C_SendCmd(I2C0, I2C_WRITE_CMD, 0xaa, I2C_STOP_DISABLE);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void I2C_SendCmd(I2C_TypeDef *I2Cx, uint16_t command, uint8_t data,
                                        uint16_t StopState)
{
    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));
    assert_param(IS_I2C_CMD(command));
    assert_param(IS_I2C_STOP(StopState));

    I2Cx->IC_DATA_CMD = data | command | StopState;
}

/**
 *
 * \brief  Return the most recent received data by the I2Cx peripheral.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 *
 * \return The value of the received data.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t rx_count = 0;
 * uint8_t read_buf = 0;
 *
 * void I2C1_Handler(void)
 * {
 *     if (I2C_GetINTStatus(I2C1, I2C_INT_RX_FULL) == SET)
 *     {
 *         rx_count = I2C_GetRxFIFOLen(I2C1);
 *         for(uint8_t i = 0; i < rx_count; i++)
 *         {
 *             read_buf = I2C_ReceiveData(I2C1);
 *             APP_PRINT_INFO2("I2C1 rx_count:%d, read_buf:%d", rx_count, read_buf);
 *         }
 *         I2C_ClearINTPendingBit(I2C1, I2C_INT_RX_FULL);
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint8_t I2C_ReceiveData(I2C_TypeDef *I2Cx)
{
    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));

    /* Return the data in the DR register */
    return (uint8_t)I2Cx->IC_DATA_CMD;
}

/**
 *
 * \brief   Get data length in RX FIFO through the I2Cx peripheral.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t rx_count = 0;
 *
 * void I2C1_Handler(void)
 * {
 *     rx_count = I2C_GetRxFIFOLen(I2C1);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint8_t I2C_GetRxFIFOLen(I2C_TypeDef *I2Cx)
{
    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));

    return (uint8_t)I2Cx->IC_RXFLR;
}

/**
 *
 * \brief   Get data length in TX FIFO through the I2Cx peripheral.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c0_demo(void)
 * {
 *     uint8_t data_len = I2C_GetTxFIFOLen(I2C0);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint8_t I2C_GetTxFIFOLen(I2C_TypeDef *I2Cx)
{
    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));

    return (uint8_t)I2Cx->IC_TXFLR;
}

/**
 *
 * \brief   Clear all I2C interrupt.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c0_demo(void)
 * {
 *     I2C_ClearAllINT(I2C0);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void I2C_ClearAllINT(I2C_TypeDef *I2Cx)
{
    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));

    I2Cx->IC_CLR_INTR;
}

/**
 *
 * \brief  Check whether the specified I2C flag is set or not.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] I2C_FLAG: Specifies the flag to check \ref x3e_I2C_flags_definition.
 *            This parameter can be one of the following values:
 *            - I2C_FLAG_SLV_ACTIVITY: Slave FSM activity status.
 *            - I2C_FLAG_MST_ACTIVITY: Master FSM activity status.
 *            - I2C_FLAG_RFF: Receive FIFO is completely full.
 *            - I2C_FLAG_RFNE: Receive FIFO is not empty.
 *            - I2C_FLAG_TFE: Transmit FIFO is completely empty.
 *            - I2C_FLAG_TFNF: Transmit FIFO is not full.
 *            - I2C_FLAG_ACTIVITY: I2C activity status.
 *
 * \return The new state of I2C_FLAG.
 * \retval SET: The specified I2C flag is set.
 * \retval RESET: The specified I2C flag is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c_test_code(void)
 * {
 *     uint8_t cnt = 0;
 *     for (cnt = 0; cnt < TEST_SIZE; cnt++)
 *     {
 *         I2C_SendCmd(I2C1, I2C_READ_CMD, 0, I2C_STOP_DISABLE);
 *     }
 *     while (I2C_GetFlagState(I2C1, I2C_FLAG_TFNF) == RESET);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE FlagStatus I2C_GetFlagState(I2C_TypeDef *I2Cx, uint32_t I2C_FLAG)
{
    FlagStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));
    assert_param(IS_I2C_GET_FLAG(I2C_FLAG));

    if ((I2Cx->IC_STATUS & I2C_FLAG) != (uint32_t)RESET)
    {
        /* I2C_FLAG is set */
        bit_status = SET;
    }

    /* Return the I2C_FLAG status */
    return  bit_status;
}

/**
 *
 * \brief  Check whether the last I2Cx event is equal to the one passed as parameter.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] I2C_EVENT: Specifies the event to be checked about I2C Transmit Abort Status Register \ref x3e_I2C_transmit_Abort_Source.
 *      This parameter can be one of the following values:
 *      - ABRT_SLVRD_INTX: When the processor side responds to a slave mode request for data to be transmitted to a remote master and user send read command.
 *      - ABRT_SLV_ARBLOST: Slave lost the bus while transmitting data to a remote master.
 *      - ABRT_SLVFLUSH_TXFIFO: Slave has received a read command and some data exists in the TX FIFO so the slave issues a TX abort interrupt to flush old data in TX FIFO.
 *      - ARB_LOST: Master has lost arbitration or the slave transmitter has lost arbitration.
 *      - ABRT_MASTER_DIS: User tries to initiate a master operation with the master mode disabled.
 *      - ABRT_10B_RD_NORSTRT: The restart is disabled and the master sends a read command in 10-bit addressing mode.
 *      - ABRT_SBYTE_NORSTRT: The restart is disabled and the user is trying to send a start byte.
 *      - ABRT_HS_NORSTRT: The restart is disabled and the user is trying to use the master to transfer data in high speed mode.
 *      - ABRT_SBYTE_ACKDET: Master has sent a start byte and the start byte was acknowledged (wrong behavior).
 *      - ABRT_HS_ACKDET: Master is in high speed mode and the high speed master code was acknowledged (wrong behavior).
 *      - ABRT_GCALL_READ: Sent a general call but the user programmed the byte following the general call to be a read from the bus.
 *      - ABRT_GCALL_NOACK: Sent a general call and no slave on the bus acknowledged the general call.
 *      - ABRT_TXDATA_NOACK: Master sent data byte(s) following the address, it did not receive an acknowledge from the remote slave.
 *      - ABRT_10ADDR2_NOACK: Master is in 10-bit address mode and the second address byte of the 10-bit address was not acknowledged by any slave.
 *      - ABRT_10ADDR1_NOACK: Master is in 10-bit address mode and the first 10-bit address byte was not acknowledged by any slave.
 *      - ABRT_7B_ADDR_NOACK: Master is in 7-bit addressing mode and the address sent was not acknowledged by any slave.
 *
 * \return  An ErrorStatus enumeration value.
 * \retval  SUCCESS: Last event is equal to the I2C_EVENT.
 * \retval  ERROR: Last event is different from the I2C_EVENT.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c_repeatread_demo(void)
 * {
 *     if (I2C_Success != I2C_RepeatRead(I2C1, I2C_WriteBuf, 2, I2C_ReadBuf, 4))
 *     {
 *          APP_PRINT_ERROR0("Send failed");
 *
 *          if (I2C_CheckEvent(I2C1, ABRT_7B_ADDR_NOACK) == SET)
 *          {
 *              APP_PRINT_ERROR0("Wrong addr");
 *          }
 *          if (I2C_CheckEvent(I2C1, ABRT_GCALL_NOACK) == SET)
 *          {
 *              APP_PRINT_ERROR0("General call nack");
 *          }
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE FlagStatus I2C_CheckEvent(I2C_TypeDef *I2Cx, uint32_t I2C_EVENT)
{
    FlagStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));
    assert_param(IS_I2C_EVENT(I2C_EVENT));

    if ((I2Cx->IC_TX_ABRT_SOURCE & I2C_EVENT) != (uint32_t)RESET)
    {

        bit_status = SET;
    }

    /* Return the I2C event status */
    return  bit_status;
}

/**
 *
 * \brief   Get the specified I2C interrupt status.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] I2C_IT: Specified the I2C interrupt \ref x3e_I2C_interrupts_definition.
 *            This parameter can be one of the following values:
 *            - I2C_INT_GEN_CALL: I2C general call interrupt. When a general call address is received and it is acknowledged.
 *            - I2C_INT_START_DET: I2C start detect interrupt. When a start or restart condition has occurred on the I2C interface.
 *            - I2C_INT_STOP_DET: I2C stop detect interrupt. When a stop condition has occurred on the I2C interface.
 *            - I2C_INT_ACTIVITY: I2C activity interrupt. When I2C is activity on the bus.
 *            - I2C_INT_RX_DONE: I2C slave RX done interrupt. When the I2C is acting as a slave-transmitter and the master does not acknowledge a transmitted byte. This occurs on the last byte of the transmission, indicating that the transmission is done.
 *            - I2C_INT_TX_ABRT: I2C TX abort interrupt. When an I2C transmitter is unable to complete the intended actions on the contents of the transmit FIFO.
 *            - I2C_INT_RD_REQ: I2C slave RX request interrupt. When I2C is acting as a slave and another I2C master is attempting to read data from I2C.
 *            - I2C_INT_TX_EMPTY: I2C TX FIFO empty interrupt. When the transmit buffer is at or below the threshold value.
 *            - I2C_INT_TX_OVER: I2C TX FIFO overflow interrupt. When transmit buffer is filled to 24 and the processor attempts to issue another I2C command.
 *            - I2C_INT_RX_FULL: I2C RX FIFO full interrupt. When the receive buffer reaches or goes above the RX FIFO threshold value.
 *            - I2C_INT_RX_OVER: I2C RX FIFO overflow interrupt. When the receive buffer is completely filled to 40 and an additional byte is received from an external I2C device.
 *            - I2C_INT_RX_UNDER: I2C RX FIFO underflow interrupt. When the processor attempts to read the receive buffer when it is empty.
 *
 * \return The new state of I2C_IT.
 * \retval SET: The specified I2C flag is set.
 * \retval RESET: The specified I2C flag is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void I2C0_Handler(void)
 * {
 *     if (I2C_GetINTStatus(I2C0, I2C_INT_STOP_DET) == SET)
 *     {
 *         //Add user code here.
 *         I2C_ClearINTPendingBit(I2C0, I2C_INT_STOP_DET);
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE ITStatus I2C_GetINTStatus(I2C_TypeDef *I2Cx, uint32_t I2C_IT)
{
    ITStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));
    assert_param(I2C_GET_INT(I2C_IT));

    if ((I2Cx->IC_INTR_STAT & I2C_IT) != (uint32_t)RESET)
    {

        bit_status = SET;
    }

    /* Return the I2C_IT status */
    return  bit_status;
}

/**
 *
 * \brief   Enable or disable the I2Cx GDMA interface.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] I2C_GDMAReq: Specifies the I2C GDMA transfer request to be enabled or disabled \ref x3e_I2C_GDMA_transfer_requests.
 *            This parameter can be one of the following values:
 *            - I2C_GDMAReq_Tx: TX buffer GDMA transfer request.
 *            - I2C_GDMAReq_Rx: RX buffer GDMA transfer request.
 * \param[in] NewState: New state of the selected I2C GDMA transfer request.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the I2Cx GDMA interface.
 *            - DISABLE: Disable the I2Cx GDMA interface.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c0_demo(void)
 * {
 *     I2C_GDMACmd(I2C0, I2C_GDMAReq_Tx, ENABLE);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void I2C_GDMACmd(I2C_TypeDef *I2Cx, uint16_t I2C_GDMAReq,
                                        FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    assert_param(IS_I2C_GDMAREQ(I2C_GDMAReq));

    if (NewState != DISABLE)
    {
        /* Enable the selected I2C GDMA request */
        I2Cx->IC_DMA_CR |= I2C_GDMAReq;
    }
    else
    {
        /* Disable the selected I2C GDMA request */
        I2Cx->IC_DMA_CR &= (uint16_t)~(I2C_GDMAReq);
    }
}

#ifdef __cplusplus
}
#endif

#endif /*__RTL8762X_I2C_H*/

/** @} */ /* End of group 87x3e_I2C_Exported_Functions */
/** @} */ /* End of group 87x3e_I2C */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

