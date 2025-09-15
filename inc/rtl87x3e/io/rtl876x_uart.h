/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_uart.h
* @brief     The header file of UART driver.
* @details
* @author    tifnan_ge
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _RTL876X_UART_H_
#define _RTL876X_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"
#include <stdbool.h>

/** @addtogroup 87x3e_UART UART
  * @brief UART driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup 87x3e_UART_Exported_Types UART Exported Types
  * @{
  */

/**
 * @brief UART initialize parameters.
 */
typedef struct
{
    //baudrate calibration
    uint16_t ovsr_adj;              /*!< Specifies the baudrate setting of ovsr_adj, please refer to UART_BaudRate_Table in API \ref UART_ComputeDiv.*/
    uint16_t div;                   /*!< Specifies the baudrate setting of div, please refer to UART_BaudRate_Table in API \ref UART_ComputeDiv .*/
    uint16_t ovsr;                  /*!< Specifies the baudrate setting of ovsr, please refer to UART_BaudRate_Table in API \ref UART_ComputeDiv .*/

    uint16_t wordLen;               /*!< Specifies the UART word length.
                                        This parameter can be a value of @ref x3e_UART_Word_Length. */
    uint16_t parity;                /*!< Specifies the UART parity.
                                        This parameter can be a value of @ref x3e_UART_Parity. */
    uint16_t stopBits;              /*!< Specifies the UART stop bits.
                                        This parameter can be a value of @ref x3e_UART_Stop_Bits. */
    uint16_t autoFlowCtrl;          /*!< Specifies the UART hardware auto flow control.
                                        This parameter can be a value of @ref x3e_UART_Hardware_Flow_Control. */

    uint16_t rxTriggerLevel;        /*!< Specifies the UART RX trigger Level.
                                        This parameter must range from 1 to 29.*/
    uint16_t dmaEn;                 /*!< Specifies the DMA mode.
                                        This parameter must be a value of DISABLE and ENABLE. */
    uint16_t idle_time;             /*!< Specifies the UART RX idle time.
                                        This parameter can be a value of @ref x3e_UART_Rx_idle_time. */

    uint8_t TxWaterlevel;           /*!< Specifies the DMA TX water level.
                                        This parameter must range from 1 to 15.*/
    uint8_t RxWaterlevel;           /*!< Specifies the DMA RX water level.
                                        This parameter must range from 1 to 31.*/
    uint16_t TxDmaEn;               /*!< Specifies the TX DMA mode.
                                        This parameter must be a value of DISABLE and ENABLE. */
    uint16_t RxDmaEn;               /*!< Specifies the RX DMA mode.
                                        This parameter must be a value of DISABLE and ENABLE. */

} UART_InitTypeDef;

/** End of group 87x3e_UART_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/

/** @defgroup 87x3e_UART_Exported_Constants UART Exported Constants
  * @{
  */
/** @defgroup 87x3e_UART_Declaration UART Declaration
  * @{
  */
#define UART0                           ((UART_TypeDef             *) UART0_REG_BASE) //!< The UART0 base address.
#define UART1                           ((UART_TypeDef             *) UART1_REG_BASE) //!< The UART1 base address.
#define UART2                           ((UART_TypeDef             *) UART2_REG_BASE) //!< The UART2 base address.

#define IS_UART_PERIPH(PERIPH) (((PERIPH) == UART0) || \
                                ((PERIPH) == UART1) || \
                                ((PERIPH) == UART2)) //!< UART peripherals can select UART0, UART1, or UART2.

#define UART_TX_FIFO_SIZE           16 //!< UART TX FIFO size is 16.
#define UART_RX_FIFO_SIZE           32 //!< UART RX FIFO size is 32.
/** End of group 87x3e_UART_Declaration
  * @}
  */

/** @defgroup 87x3e_UART_Interrupts_Definition UART Interrupts Definition
  * @{
  */

#define UART_INT_RD_AVA                             ((uint16_t)(1 << 0)) //!< Receive data avaliable interrupt, includes RX FIFO trigger level or RX timeout.
#define UART_INT_FIFO_EMPTY                         ((uint16_t)(1 << 1)) //!< TX FIFO empty interrupt.
#define UART_INT_LINE_STS                           ((uint16_t)(1 << 2)) //!< Receiver line status interrupt.
#define UART_INT_MODEM_STS                          ((uint16_t)(1 << 3)) //!< Modem status interrupt.
#define UART_INT_TX_DONE                            ((uint16_t)(1 << 4)) //!< TX done(TX FIFO empty and TX waveform sent done) interrupt.
#define UART_INT_TX_THD                             ((uint16_t)(1 << 5)) //!< TX FIFO threshold interrupt. TX FIFO level is less than or equal to TX FIFO threshold.
#define UART_INT_IDLE                               ((uint16_t)(1 << 7)) //!< Bus idle interrupt.

#define IS_UART_IT(IT) ((((IT) & 0xFFFFFFF0) == 0x00) && ((IT) != 0x00)) //!< Check if the input parameter is valid.

#define IS_UART_GET_IT(IT) ((IT) & (UART_INT_RD_AVA | UART_INT_FIFO_EMPTY | UART_INT_LINE_STS |\
                                    UART_INT_MODEM_STS | UART_INT_TX_DONE | UART_INT_TX_THD |\
                                    UART_INT_IDLE)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_Interrupts_Definition
  * @}
  */

/** @defgroup 87x3e_UART_Interrupt_Identifier UART Interrupt Identifier
  * @{
  */

#define UART_INT_ID_LINE_STATUS                     ((uint16_t)(0x03 << 1)) //!< RX line status interrupt identification.
#define UART_INT_ID_RX_LEVEL_REACH                  ((uint16_t)(0x02 << 1)) //!< RX trigger level reached interrupt identification.
#define UART_INT_ID_RX_TMEOUT                       ((uint16_t)(0x06 << 1)) //!< RX FIFO data timeout interrupt identification.
#define UART_INT_ID_TX_EMPTY                        ((uint16_t)(0x01 << 1)) //!< TX FIFO empty interrupt identification.
#define UART_INT_ID_MODEM_STATUS                    ((uint16_t)(0x00 << 1)) //!< Modem status interrupt identification.

#define IS_UART_IT_ID(ID) (((ID) == UART_INT_ID_LINE_STATUS) || ((ID) == UART_INT_ID_RX_LEVEL_REACH)\
                           || ((ID) == UART_INT_ID_RX_TMEOUT) || ((ID) == UART_INT_ID_TX_EMPTY)\
                           || ((ID) == UART_INT_ID_MODEM_STATUS)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_Interrupt_Identifier
  * @}
  */

/** @defgroup 87x3e_UART_Interrupts_Mask_Definition UART Interrupts Mask Definition
  * @{
 */

#define UART_INT_MASK_RD_AVA                    ((uint16_t)(1 << 0)) //!< Mask received data avaliable interrupt(RX fifo trigger level or timeout).
#define UART_INT_MASK_TX_FIFO_EMPTY             ((uint16_t)(1 << 1)) //!< Mask transmitter FIFO empty interrupt.
#define UART_INT_MASK_RX_LINE_STS               ((uint16_t)(1 << 2)) //!< Mask receiver line status interrupt.
#define UART_INT_MASK_RX_BREAK                  ((uint16_t)(1 << 4)) //!< Mask RX break interrupt.
#define UART_INT_MASK_RX_IDLE                   ((uint16_t)(1 << 5)) //!< Mask RX idle timeout interrupt.
#define UART_INT_MASK_TX_DONE                   ((uint16_t)(1 << 6)) //!< Mask TX done(TX shift register empty and TX FIFO empty) interrupt.
#define UART_INT_MASK_TX_THD                    ((uint16_t)(1 << 7)) //!< Mask TX FIFO threshold interrupt.

#define IS_UART_INT_MASK(INT) (((INT) == UART_INT_MASK_RD_AVA) || ((INT) == UART_INT_MASK_TX_FIFO_EMPTY)\
                               || ((INT) == UART_INT_MASK_RX_LINE_STS) || ((INT) == UART_INT_MASK_RX_IDLE)\
                               || ((INT) == UART_INT_MASK_TX_DONE) || ((INT) == UART_INT_MASK_TX_THD)\
                               || ((INT) == UART_INT_MASK_RX_BREAK)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_Interrupts_Mask_Definition
  * @}
  */

/** @defgroup 87x3e_UART_Flag UART Flag
  * @{
  */

#define UART_FLAG_INT_PEND                         ((uint16_t)(1 << 0)) //!< Interrupt pending indicator.
#define UART_FLAG_RX_DATA_RDY                      ((uint16_t)(1 << 0)) //!< RX FIFO data ready indicator. At least one character has been received and transferred into the receiver buffer register or the FIFO.
#define UART_FLAG_RX_OVERRUN                       ((uint16_t)(1 << 1)) //!< RX FIFO overrun error indicator. Indicates that data in the RX FIFO was not read by the CPU before the next character was transferred into the RX FIFO.
#define UART_FLAG_PARTY_ERR                        ((uint16_t)(1 << 2)) //!< Parity error indicator. Indicates that the received data character does not have the correct even or odd parity. 
#define UART_FLAG_FRAME_ERR                        ((uint16_t)(1 << 3)) //!< Framing error indicator. The received character at the top of the FIFO did not have a valid stop bit.
#define UART_FLAG_BREAK_ERR                        ((uint16_t)(1 << 4)) //!< Break interrupt indicator. Set to logic 1 whenever the received data input is held in the spacing (logic 0) state for a longer than a full word transmission time.
#define UART_FLAG_THR_EMPTY                        ((uint16_t)(1 << 5)) //!< Transmitter holding register (THR) empty indicator.
#define UART_FLAG_THR_TSR_EMPTY                    ((uint16_t)(1 << 6)) //!< Transmitter holding register (THR) and the transmitter shift register (TSR) are both empty.
#define UART_FLAG_RX_FIFO_ERR                      ((uint16_t)(1 << 7)) //!< At least one parity error, framing error or break indication in the FIFO.
#define UART_FLAG_RX_IDLE                          ((uint16_t)(1 << 8)) //!< Only to show difference cause the address of UART RX idle flag is isolate.
#define UART_FLAG_TX_DONE                          ((uint16_t)(1 << 10)) //!< TX done(TX FIFO empty and TX waveform sent done).

#define IS_UART_GET_FLAG(FLAG) (((FLAG) == UART_FLAG_RX_DATA_RDY)   || ((FLAG) == UART_FLAG_RX_OVERRUN)\
                                || ((FLAG) == UART_FLAG_PARTY_ERR) || ((FLAG) == UART_FLAG_FRAME_ERR)\
                                || ((FLAG) == UART_FLAG_BREAK_ERR) || ((FLAG) == UART_FLAG_THR_EMPTY)\
                                || ((FLAG) == UART_FLAG_THR_TSR_EMPTY) || ((FLAG) == UART_FLAG_RX_FIFO_ERR)\
                                || ((FLAG) == UART_FLAG_RX_IDLE) || ((FLAG) == UART_FLAG_TX_DONE)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_Flag
  * @}
  */

/** @defgroup 87x3e_UART_RX_FIFO_Level UART RX FIFO Level
  * @{
  */

#define UART_RX_FIFO_TRIGGER_LEVEL_1BYTE            ((uint16_t)(0x01)) //!< Receiver FIFO interrupt trigger level is 1 byte.
#define UART_RX_FIFO_TRIGGER_LEVEL_4BYTE            ((uint16_t)(0x04)) //!< Receiver FIFO interrupt trigger level is 4 bytes.
#define UART_RX_FIFO_TRIGGER_LEVEL_8BYTE            ((uint16_t)(0x08)) //!< Receiver FIFO interrupt trigger level is 8 bytes.
#define UART_RX_FIFO_TRIGGER_LEVEL_14BYTE           ((uint16_t)(0x0E)) //!< Receiver FIFO interrupt trigger level is 14 bytes.

#define IS_UART_RX_FIFO_TRIGGER_LEVEL(LEVEL) (((LEVEL) >= 1) && ((LEVEL) <= 29)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_RX_FIFO_Level
  * @}
  */


/** @defgroup 87x3e_UART_Rx_idle_time UART RX Idle Time
  * @{
  */

#define UART_RX_IDLE_1BYTE                 ((uint16_t)(0x00)) //!< RX idle timeout value is 8 bit time.
#define UART_RX_IDLE_2BYTE                 ((uint16_t)(0x01)) //!< RX idle timeout value is 16 bit time.
#define UART_RX_IDLE_4BYTE                 ((uint16_t)(0x02)) //!< RX idle timeout value is 32 bit time.
#define UART_RX_IDLE_8BYTE                 ((uint16_t)(0x03)) //!< RX idle timeout value is 64 bit time.
#define UART_RX_IDLE_16BYTE                ((uint16_t)(0x04)) //!< RX idle timeout value is 128 bit time.
#define UART_RX_IDLE_32BYTE                ((uint16_t)(0x05)) //!< RX idle timeout value is 256 bit time.
#define UART_RX_IDLE_64BYTE                ((uint16_t)(0x06)) //!< RX idle timeout value is 512 bit time.
#define UART_RX_IDLE_128BYTE               ((uint16_t)(0x07)) //!< RX idle timeout value is 1024 bit time.
#define UART_RX_IDLE_256BYTE               ((uint16_t)(0x08)) //!< RX idle timeout value is 2048 bit time.
#define UART_RX_IDLE_512BYTE               ((uint16_t)(0x09)) //!< RX idle timeout value is 4096 bit time.
#define UART_RX_IDLE_1024BYTE              ((uint16_t)(0x0A)) //!< RX idle timeout value is 8192 bit time.
#define UART_RX_IDLE_2048BYTE              ((uint16_t)(0x0B)) //!< RX idle timeout value is 16384 bit time.
#define UART_RX_IDLE_4096BYTE              ((uint16_t)(0x0C)) //!< RX idle timeout value is 32768 bit time.
#define UART_RX_IDLE_8192BYTE              ((uint16_t)(0x0D)) //!< RX idle timeout value is 65535 bit time.
#define UART_RX_IDLE_16384BYTE             ((uint16_t)(0x0E)) //!< RX idle timeout value is 131072 bit time.
#define UART_RX_IDLE_32768BYTE             ((uint16_t)(0x0F)) //!< RX idle timeout value is 262144 bit time.

#define IS_UART_IDLE_TIME(TIME) ((TIME) <= 0x0F) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_Rx_idle_time
  * @}
  */

/** @defgroup 87x3e_UART_Parity UART Parity
  * @{
  */

#define UART_PARITY_NO_PARTY                        ((uint16_t)(0x00 << 3)) //!< Select no parity.
#define UART_PARITY_ODD                             ((uint16_t)(0x01 << 3)) //!< Select odd parity.
#define UART_PARITY_EVEN                            ((uint16_t)(0x03 << 3)) //!< Select even parity.

#define IS_UART_PARITY(PARITY) (((PARITY) == UART_PARITY_NO_PARTY) || ((PARITY) == UART_PARITY_ODD)\
                                || ((PARITY) == UART_PARITY_EVEN)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_Parity
  * @}
  */

/** @defgroup 87x3e_UART_DMA UART DMA
  * @{
  */

#define UART_DMA_ENABLE                             ((uint16_t)(1 << 3)) //!< Enable UART DMA.
#define UART_DMA_DISABLE                            ((uint16_t)(0 << 3)) //!< Disable UART DMA.

#define IS_UART_DMA_CFG(CFG) (((CFG) == UART_DMA_ENABLE) || ((CFG) == UART_DMA_DISABLE)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_DMA
  * @}
  */

/** @defgroup 87x3e_UART_Hardware_Flow_Control UART Hardware Flow Control
  * @{
  */

#define UART_AUTO_FLOW_CTRL_EN                      ((uint16_t)((1 << 5) | (1 << 1))) //!< Enable auto flow control.
#define UART_AUTO_FLOW_CTRL_DIS                     ((uint16_t)0x00) //!< Disable auto flow control.

#define IS_UART_AUTO_FLOW_CTRL(CTRL) (((CTRL) == UART_AUTO_FLOW_CTRL_EN) || ((CTRL) == UART_AUTO_FLOW_CTRL_DIS)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_Hardware_Flow_Control
  * @}
  */

/** @defgroup 87x3e_UART_Word_Length UART Word Length
  * @{
  */

#define UART_WROD_LENGTH_7BIT                       ((uint16_t)(0 << 0)) //!< Data format is 7 bit word length.
#define UART_WROD_LENGTH_8BIT                       ((uint16_t)(1 << 0)) //!< Data format is 8 bit word length.

#define IS_UART_WORD_LENGTH(LEN) ((((LEN)) == UART_WROD_LENGTH_7BIT) || (((LEN)) == UART_WROD_LENGTH_8BIT)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_Word_Length
  * @}
  */

/** @defgroup 87x3e_UART_Stop_Bits UART Stop Bits
  * @{
  */

#define UART_STOP_BITS_1                           ((uint16_t)(0 << 2)) //!< 1-bit stop bits.
#define UART_STOP_BITS_2                           ((uint16_t)(1 << 2)) //!< 2-bit stop bits.

#define IS_UART_STOPBITS(STOP) (((STOP) == UART_STOP_BITS_1) || ((STOP) == UART_STOP_BITS_2)) //!< Check if the input parameter is valid.

/** End of group 87x3e_UART_Stop_Bits
  * @}
  */

/** @cond private
  * @defgroup 87x3e_Uart_Tx_Rx_FIFO_CLEAR_BIT Uart FIFO Clear Bits
  * @{
  */

#define FCR_CLEAR_RX_FIFO_Set           ((uint32_t)(1 << 1))
#define FCR_CLEAR_RX_FIFO_Reset         ((uint32_t)~(1 << 1))

#define FCR_CLEAR_TX_FIFO_Set           ((uint32_t)(1 << 2))
#define FCR_CLEAR_TX_FIFO_Reset         ((uint32_t)~(1 << 2))

/**
  * @}
  * @endcond
  */

/** End of group 87x3e_UART_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_UART_Exported_Functions UART Exported Functions
  * @{
  */

/**
 *
 * \brief   Initialize the selected UART peripheral according to the specified
 *          parameters in UART_InitStruct.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]   UART_InitStruct: Pointer to a \ref UART_InitTypeDef structure that
 *              contains the configuration information for the selected UART peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_uart_init(void)
 * {
 *     UART_DeInit(UART0);
 *
 *     RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);
 *
 *     UART_InitTypeDef uartInitStruct;
 *     UART_StructInit(&uartInitStruct);
 *     uartInitStruct.rxTriggerLevel = UART_RX_FIFO_TRIGGER_LEVEL_14BYTE;
 *    //Add other initialization parameters that need to be configured here.
 *
 *     UART_Init(UART, &uartInitStruct);
 * }
 * \endcode
 */
extern void (*UART_Init)(UART_TypeDef *UARTx, UART_InitTypeDef *UART_InitStruct);

/**
 *
 * \brief   Disable the UARTx peripheral clock, and restore registers to their default values.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_uart_init(void)
 * {
 *    UART_DeInit(UART0);
 * }
 * \endcode
 */
void UART_DeInit(UART_TypeDef *UARTx);

/**
 *
 * \brief   Fills each UART_InitStruct member with its default value.
 *
 * \note   The default settings for the UART_InitStruct member are shown in the following table:
 *         | UART_InitStruct Member | Default Value                |
 *         |:----------------------:|:----------------------------:|
 *         | div                    | 20                           |
 *         | ovsr                   | 12                           |
 *         | ovsr_adj               | 0x252                        |
 *         | parity                 | \ref UART_PARITY_NO_PARTY    |
 *         | stopBits               | \ref UART_STOP_BITS_1        |
 *         | wordLen                | \ref UART_WROD_LENGTH_8BIT   |
 *         | dmaEn                  | \ref UART_DMA_DISABLE        |
 *         | autoFlowCtrl           | \ref UART_AUTO_FLOW_CTRL_DIS |
 *         | rxTriggerLevel         | 16                           |
 *         | idle_time              | \ref UART_RX_IDLE_2BYTE      |
 *         | TxWaterlevel           | 15                           |
 *         | RxWaterlevel           | 1                            |
 *         | TxDmaEn                | DISABLE                      |
 *         | RxDmaEn                | DISABLE                      |
 *
 * \param[in]   UART_InitStruct: Pointer to an \ref UART_InitTypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_uart_init(void)
 * {
 *     UART_DeInit(UART0);
 *
 *     RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);
 *
 *     UART_InitTypeDef uartInitStruct;
 *     UART_StructInit(&uartInitStruct);
 *     uartInitStruct.rxTriggerLevel = UART_RX_FIFO_TRIGGER_LEVEL_14BYTE;
 *    //Add other initialization parameters that need to be configured here.
 *
 *     UART_Init(UART, &uartInitStruct);
 * }
 * \endcode
 */
void UART_StructInit(UART_InitTypeDef *UART_InitStruct);

/**
 *
 * \brief   Set baud rate of UART.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]   baud_rate: Baud rate to be set. The value can refer to \ref UartBaudRate_TypeDef.
 *
 * \return   Specified the UART baud rate that to be set or not.
 * \retval 0   The baud_rate was set successfully.
 * \retval 1   The selected baud_rate was not supported.
 *
 * <b>Example usage</b>
 * \code{.c}
  * void driver_uart_init(void)
 * {
 *     UART_SetBaudRate(UART0, BAUD_RATE_115200);
 * }
 * \endcode
 */
extern uint8_t (*UART_SetBaudRate)(UART_TypeDef *UARTx, UartBaudRate_TypeDef baud_rate);

/**
 *
 * \brief   Receive data from RX FIFO.
 * \param[in]  UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[out] outBuf: Buffer to store data which read from RX FIFO.
 * \param[in]  count: Length of data to be read.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     uint8_t data[32] = {10};
 *     UART_ReceiveData(UART0, data, 10);
 * }
 * \endcode
 */
void UART_ReceiveData(UART_TypeDef *UARTx, uint8_t *outBuf, uint16_t count);

/**
 *
 * \brief   Send data to TX FIFO directly.
 * \param[in] UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in] inBuf: Buffer of data to be written to TX FIFO. This parameter must range from 0x0 to 0xFF.
 * \param[in] count: Length of data to be written. This parameter must range from 0x1 to 0xFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     uint8_t data[] = "UART demo";
 *     UART_SendData(UART0, data, sizeof(data));
 * }
 * \endcode
 */
extern void UART_SendData(UART_TypeDef *UARTx, const uint8_t *inBuf, uint16_t count);

/**
 *
 * \brief   Send data to TX FIFO by polling mode.
 * \param[in] UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in] data: Buffer of data to be written to TX FIFO. This parameter must range from 0x0 to 0xFF.
 * \param[in] len: Length of data to be written. This parameter must range from 0x1 to 0xFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     uint8_t data[] = "UART demo";
 *     UART_TxData(UART0, data, sizeof(data));
 * }
 * \endcode
 */
void UART_TxData(UART_TypeDef *UARTx, uint8_t *data, uint32_t len);

/**
 *
 * \brief   Enables or disables the specified UART interrupts.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]   UART_IT: Specified the UART interrupt that to be enabled or disabled \ref x3e_UART_Interrupts_Definition.
 *      This parameter can be any combination of the following values:
 *      - UART_INT_RD_AVA: RX data avaliable interrupt.
 *      - UART_INT_FIFO_EMPTY: TX FIFO empty interrupt.
 *      - UART_INT_LINE_STS: Line status interrupt.
 *      - UART_INT_MODEM_STS: Modem status interrupt.
 *      - UART_INT_IDLE: Bus idle interrupt.
 *      - UART_INT_TX_DONE: TX done(TX FIFO empty and TX waveform sent done) interrupt.
 * \param[in] newState: New state of the specified UART interrupt.
 *      This parameter can be one of the following values:
 *      - ENABLE: Enable the specified UART interrupt.
 *      - DISABLE: Disable the specified UART interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_uart_init(void)
 * {
 *     UART_DeInit(UART0);
 *
 *     RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);
 *
 *     UART_InitTypeDef uartInitStruct;
 *     UART_StructInit(&uartInitStruct);
 *     uartInitStruct.rxTriggerLevel = UART_RX_FIFO_TRIGGER_LEVEL_14BYTE;
 *    //Add other initialization parameters that need to be configured here.
 *
 *     UART_Init(UART, &uartInitStruct);
 *
 *     UART_INTConfig(UART0, UART_INT_RD_AVA, ENABLE);
 * }
 * \endcode
 */
extern void UART_INTConfig(UART_TypeDef *UARTx, uint32_t UART_IT, FunctionalState newState);

/**
 *
 * \brief   Check whether the specified UART flag is set or not.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]   UART_FLAG: Specified UART flag to check \ref x3e_UART_Flag.
 *      This parameter can be one of the following values:
 *      - UART_FLAG_RX_DATA_RDY: RX FIFO data ready. At least one character has been received and transferred into the receiver buffer register or the FIFO.
 *      - UART_FLAG_RX_OVERRUN: RX FIFO overrun error. Indicates that data in the RX FIFO was not read by the CPU before the next character was transferred into the RX FIFO.
 *      - UART_FLAG_PARTY_ERR: Parity error. Indicates that the received data character does not have the correct even or odd parity.
 *      - UART_FLAG_FRAME_ERR: Framing error. The received character at the top of the FIFO did not have a valid stop bit.
 *      - UART_FLAG_BREAK_ERR: Break error. Set to logic 1 whenever the received data input is held in the spacing (logic 0) state for a longer than a full word transmission time.
 *      - UART_FLAG_THR_EMPTY: Transmitter holding register (THR) empty.
 *      - UART_FLAG_THR_TSR_EMPTY: Transmitter holding register (THR) and the transmitter shift register (TSR) are both empty.
 *      - UART_FLAG_RX_FIFO_ERR: At least one parity error, framing error or break indication in the FIFO.
 *      - UART_FLAG_RX_IDLE: RX idle timeout. Only to show difference cause the address of UART RX idle flag is isolate.
 *      - UART_FLAG_TX_DONE: TX done(TX FIFO empty and TX waveform sent done).
 *
 * \return   New state of UART flag.
 * \retval SET: The specified UART flag bit is set.
 * \retval RESET: The specified flag is not set.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_send_str(char *str, uint16_t str_len)
 * {
 *     uint8_t blk, remain, i;
 *     blk = str_len / UART_TX_FIFO_SIZE;
 *     remain = str_len % UART_TX_FIFO_SIZE;

 *     //send through UART
 *     for (i = 0; i < blk; i++)
 *     {
 *         UART_SendData(UART2, (uint8_t *)&str[16 * i], 16);
 *         while (UART_GetFlagState(UART2, UART_FLAG_THR_EMPTY) != SET);
 *     }

 *     UART_SendData(UART2, (uint8_t *)&str[16 * i], remain);
 *     while (UART_GetFlagState(UART2, UART_FLAG_THR_EMPTY) != SET);
 * }
 * \endcode
 */
FlagStatus UART_GetFlagState(UART_TypeDef *UARTx, uint32_t UART_FLAG);

/**
 *
 * \brief   Clear the specified UART flag. Only \ref UART_FLAG_RX_IDLE need to be cleared manually, other interrupt flag cannot be cleared.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]   UART_FLAG: Specified UART flag to check \ref x3e_UART_Flag.
 *      This parameter can be one of the following values:
 *      - UART_FLAG_RX_DATA_RDY: RX FIFO data ready. At least one character has been received and transferred into the receiver buffer register or the FIFO.
 *      - UART_FLAG_RX_OVERRUN: RX FIFO overrun error. Indicates that data in the RX FIFO was not read by the CPU before the next character was transferred into the RX FIFO.
 *      - UART_FLAG_PARTY_ERR: Parity error. Indicates that the received data character does not have the correct even or odd parity.
 *      - UART_FLAG_FRAME_ERR: Framing error. The received character at the top of the FIFO did not have a valid stop bit.
 *      - UART_FLAG_BREAK_ERR: Break error. Set to logic 1 whenever the received data input is held in the spacing (logic 0) state for a longer than a full word transmission time.
 *      - UART_FLAG_THR_EMPTY: Transmitter holding register (THR) empty.
 *      - UART_FLAG_THR_TSR_EMPTY: Transmitter holding register (THR) and the transmitter shift register (TSR) are both empty.
 *      - UART_FLAG_RX_FIFO_ERR: At least one parity error, framing error or break indication in the FIFO.
 *      - UART_FLAG_RX_IDLE: RX idle timeout. Only to show difference cause the address of UART RX idle flag is isolate.
 *      - UART_FLAG_TX_DONE: TX done(TX FIFO empty and TX waveform sent done).
 *
 * \return   New state of UART flag.
 * \retval SET: The specified UART flag bit is set.
 * \retval RESET: The specified flag is not set.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_handler(void)
 * {
 *     if(UART_GetFlagState(UART2, UART_FLAG_RX_IDLE) == SET)
 *     {
 *          UART_ClearINT(UART2, UART_FLAG_RX_IDLE);
 *     }
 * }
 * \endcode
 */
void UART_ClearINT(UART_TypeDef *UARTx, uint32_t UART_FLAG);

/**
 *
 * \brief   Config the specified UART loopback function.
 *
 * \param[in] UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in] NewState: New state of UART loopback function.
 *      This parameter can be one of the following values:
 *      - ENABLE: Enable the specified UART loopback function. In the loopback mode, data that is transmitted is immediately received.
 *      - DISABLE: Disable the specified UART loopback function, keep in normal operation.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     UART_LoopBackCmd(UART0, ENABLE);
 * }
 * \endcode
 */
void UART_LoopBackCmd(UART_TypeDef *UARTx, FunctionalState NewState);

/**
 *
 * \brief   Baudrate convert to UartBaudRate_TypeDef.
 *
 * \param[in]   baudrate: Select UART uint32_t baudrate. This parameter must range from 0x1 to 0xFFFFFFFF.
 *
 * \return   The converted UartBaudRate_TypeDef baud rate or 0xff.
 * \retval baudrate  UartBaudRate_TypeDef baudrate.
 * \retval 0xff      The selected baud_rate was not supported.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     UART_ConvUartBaudRate(115200);
 * }
 * \endcode
 */
UartBaudRate_TypeDef UART_ConvUartBaudRate(uint32_t baudrate);

/**
 *
 * \brief   UartBaudRate_TypeDef convert to baudrate.
 *
 * \param[in] baudrate: Select UART baudrate \ref UartBaudRate_TypeDef.
 *
 * \return   The converted UART uint32_t baud rate or 0xff.
 * \retval baudrate  UART uint32_t baudrate.
 * \retval 0         The selected baud_rate was not supported.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     UART_ConvRateValue(BAUD_RATE_115200);
 * }
 * \endcode
 */
uint32_t UART_ConvRateValue(UartBaudRate_TypeDef baudrate);

/**
 *
 * \brief   According to baudrate get UART param.
 *
 * \param[out]   div: The div for setting baudrate. This parameter ranges from 0x0 to 0xFFFF.
 * \param[out]   ovsr: The ovsr for setting baudrate. This parameter ranges from 0x0 to 0xFFFF.
 * \param[out]   ovsr_adj: The ovsr_adj for setting baudrate. This parameter ranges from 0x0 to 0xFFFF.
 * \param[in]   rate: Select UART baudrate \ref UartBaudRate_TypeDef.
 *
 * \return   UART param of specified the UART baud rate that to be get or not.
 * \retval true   The UART param was get from baudrate successfully.
 * \retval false  The UART param was failed to get due to unsupport baudrate.
 *
 * <b>Example usage</b>
 * \code{.c}
 * UART_BaudRate_Table
 * div/ovsr/ovsr_adj: These three parameters set the baud rate calibration parameters of UART.
    baudrate    |   div     |   ovsr    |   ovsr_adj
    ------------------------------------------------
    1200Hz      |   2589    |   7       |   0x7F7
    2400Hz      |   1200    |   8       |   0x3EF
    4800Hz      |   600     |   8       |   0x3EF
    9600Hz      |   271     |   10      |   0x24A
    14400Hz     |   271     |   5       |   0x222
    19200Hz     |   165     |   7       |   0x5AD
    28800Hz     |   110     |   7       |   0x5AD
    38400Hz     |   85      |   7       |   0x222
    57600Hz     |   55      |   7       |   0x5AD
    76800Hz     |   35      |   9       |   0x7EF
    115200Hz    |   20      |   12      |   0x252
    128000Hz    |   25      |   7       |   0x555
    153600Hz    |   15      |   12      |   0x252
    230400Hz    |   10      |   12      |   0x252
    460800Hz    |   5       |   12      |   0x252
    500000Hz    |   8       |   5       |   0
    921600Hz    |   4       |   5       |   0x3F7
    1000000Hz   |   4       |   5       |   0
    1382400Hz   |   2       |   9       |   0x2AA
    1444400Hz   |   2       |   8       |   0x5F7
    1500000Hz   |   2       |   8       |   0x492
    1843200Hz   |   2       |   5       |   0x3F7
    2000000Hz   |   2       |   5       |   0
    2100000Hz   |   1       |   14      |   0x400
    2764800Hz   |   1       |   9       |   0x2AA
    3000000Hz   |   1       |   8       |   0x492
    3250000Hz   |   1       |   7       |   0x112
    3692300Hz   |   1       |   5       |   0x5F7
    3750000Hz   |   1       |   5       |   0x36D
    4000000Hz   |   1       |   5       |   0
    6000000Hz   |   1       |   1       |   0x36D
    ------------------------------------------------
 * void uart_demo(void)
 * {
 *     UART_ComputeDiv(div, ovsr, ovsr_adj, BAUD_RATE_115200);
 * }
 * \endcode
 */
bool UART_ComputeDiv(uint16_t *div, uint16_t *ovsr, uint16_t *ovsr_adj, UartBaudRate_TypeDef rate);

/**
 *
 * \brief   UART idle interrupt config.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]   NewState: New state of the UART idle interrupt.
 *      This parameter can be one of the following values:
 *      - ENABLE: Enable UART idle interrupt.
 *      - DISABLE: Disable UART idle interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     UART_IdleIntConfig(UART0, ENABLE);
 * }
 * \endcode
 */
void UART_IdleIntConfig(UART_TypeDef *UARTx, FunctionalState newState);

/**
 *
 * \brief   UART one wire config.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]   is_enable: UART one wire config is set or not.
 *      This parameter can be one of the following values:
 *      - true: UART one wire config is set.
 *      - false: UART one wire config is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     UART_OneWireConfig(UART0, true);
 * }
 * \endcode
 */
void UART_OneWireConfig(UART_TypeDef *UARTx, bool is_enable);

/**
 *
 * \brief    Mask or unmask the specified UART interrupts.
 * \xrefitem Added_API_2_14_1_0 "Added Since 2.14.1.0" "Added API"
 *
 * \param[in]  UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]  UART_INT_MASK: Specifies the UART interrupts sources to be masked or unmasked \ref x3e_UART_Interrupts_Mask_Definition.
 *      This parameter can be any combination of the following values:
 *      - UART_INT_MASK_RD_AVA: Mask received data avaliable interrupt(RX fifo trigger level or timeout).
 *      - UART_INT_MASK_TX_FIFO_EMPTY: Mask transmitter FIFO empty interrupt.
 *      - UART_INT_MASK_RX_LINE_STS: Mask receiver line status interrupt.
 *      - UART_INT_MASK_RX_BREAK: Mask RX break interrupt.
 *      - UART_INT_MASK_RX_IDLE: Mask RX idle timeout interrupt.
 *      - UART_INT_MASK_TX_DONE: Mask TX done(TX shift register empty and TX FIFO empty) interrupt.
 *      - UART_INT_MASK_TX_THD: Mask TX FIFO threshold interrupt.
 * \param[in]  NewState: New state of the specified UART interrupts.
 *      This parameter can be: ENABLE or DISABLE.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     UART_MaskINTConfig(UART0, UART_INT_MASK_TX_FIFO_EMPTY, ENABLE);
 * }
 * \endcode
 */
void UART_MaskINTConfig(UART_TypeDef *UARTx, uint32_t UART_INT_MASK, FunctionalState NewState);

/**
 *
 * \brief   Send one byte of data to TX FIFO.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]   data: Byte data to send. This parameter must range from 0x0 to 0xFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     uint8_t data = 0x55;
 *     UART_SendByte(UART0, data);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void UART_SendByte(UART_TypeDef *UARTx, uint8_t data)
{
    /* Check the parameters */
    assert_param(IS_UART_PERIPH(UARTx));

    UARTx->RB_THR = data;

    return;
}

/**
 *
 * \brief   Read one byte of data from UART RX FIFO.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 *
 * \return   Which byte data has been read.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     uint8_t data = UART_ReceiveByte(UART0);
 * }
 * \endcode
 */
static __forceinline uint8_t UART_ReceiveByte(UART_TypeDef *UARTx)
{
    /* Check the parameters */
    assert_param(IS_UART_PERIPH(UARTx));

    return (uint8_t)(UARTx->RB_THR);
}

/**
 *
 * \brief   Get interrupt identifier of the selected UART peripheral.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 *
 * \return   The interrupt identifier value \ref x3e_UART_Interrupt_Identifier.
 *      This return value can be one or a combination of the following values:
 *      \retval UART_INT_ID_LINE_STATUS: RX line status interrupt ID.
 *      \retval UART_INT_ID_RX_LEVEL_REACH: RX trigger level reached interrupt ID.
 *      \retval UART_INT_ID_RX_TMEOUT: RX FIFO data timeout interrupt ID.
 *      \retval UART_INT_ID_TX_EMPTY: TX FIFO empty interrupt ID.
 *      \retval UART_INT_ID_MODEM_STATUS: Modem status interrupt ID.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void UART0_Handler()
 * {
 *     uint16_t rx_len = 0;
 *     uint8_t uart_rev_data[32];
 *
 *     //Get interrupt ID.
 *     uint32_t int_status = UART_GetIID(UART0);
 *
 *     //Disable interrupt.
 *     UART_INTConfig(UART0, UART_INT_RD_AVA, DISABLE);
 *
 *     if (UART_GetFlagStatus(UART0, UART_FLAG_RX_IDLE) == SET)
 *     {
 *         UART_INTConfig(UART0, UART_INT_RX_IDLE, DISABLE);
 *         //Add user code here.
 *         UART_ClearRxFIFO(UART0);
 *         UART_INTConfig(UART0, UART_INT_RX_IDLE, ENABLE);
 *     }
 *
 *     switch (int_status)
 *     {
 *     case UART_INT_ID_RX_TMEOUT:
 *         rx_len = UART_GetRxFIFODataLen(UART0);
 *         UART_ReceiveData(UART0, uart_rev_data, rx_len);
 *         //Add user code here.
 *         break;
 *
 *     case UART_INT_ID_LINE_STATUS:
 *         //Add user code here.
 *         break;
 *
 *     case UART_INT_ID_RX_LEVEL_REACH:
 *         rx_len = UART_GetRxFIFODataLen(UART0);
 *         UART_ReceiveData(UART0, uart_rev_data, rx_len);
 *         //Add user code here.
 *         break;
 *
 *     case UART_INT_ID_TX_EMPTY:
 *         //Add user code here.
 *         break;
 *
 *     case UART_INT_ID_MODEM_STATUS:
 *         //Add user code here.
 *         break;
 *
 *     default:
 *         break;
 *     }
 *     UART_INTConfig(UART0, UART_INT_RD_AVA, ENABLE);
 * }
 * \endcode
 */
static __forceinline uint16_t UART_GetIID(UART_TypeDef *UARTx)
{
    /* Check the parameters */
    assert_param(IS_UART_PERIPH(UARTx));

    return (uint16_t)(UARTx->INTID_FCR & (0x0000000E));
}

/**
 *
 * \brief   Get line status.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 *
 * \return   Line status.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     uint8_t line_status = UART_GetLineStatus(UART0);
 * }
 * \endcode
 */
static __forceinline uint8_t UART_GetLineStatus(UART_TypeDef *UARTx)
{
    /* Check the parameters */
    assert_param(IS_UART_PERIPH(UARTx));

    return (uint8_t)(UARTx->LSR & (0x000000FF));
}

/**
 *
 * \brief   Check line status.
 *
 * \param[in]   line_status: The line status.
 * \param[in]   line_status_flag: Specifies the flag to check with line status \ref x3e_UART_Flag.
 *      This parameter can be one of the following values:
 *      - UART_FLAG_RX_DATA_RDY: RX FIFO data ready. At least one character has been received and transferred into the receiver buffer register or the FIFO.
 *      - UART_FLAG_RX_OVERRUN: RX FIFO overrun error. Indicates that data in the RX FIFO was not read by the CPU before the next character was transferred into the RX FIFO.
 *      - UART_FLAG_PARTY_ERR: Parity error. Indicates that the received data character does not have the correct even or odd parity.
 *      - UART_FLAG_FRAME_ERR: Framing error. The received character at the top of the FIFO did not have a valid stop bit.
 *      - UART_FLAG_BREAK_ERR: Break error. Set to logic 1 whenever the received data input is held in the spacing (logic 0) state for a longer than a full word transmission time.
 *      - UART_FLAG_THR_EMPTY: Transmitter holding register (THR) empty.
 *      - UART_FLAG_THR_TSR_EMPTY: Transmitter holding register (THR) and the transmitter shift register (TSR) are both empty.
 *      - UART_FLAG_RX_FIFO_ERR: At least one parity error, framing error or break indication in the FIFO.
 *      - UART_FLAG_RX_IDLE: RX idle timeout. Only to show difference cause the address of UART RX idle flag is isolate.
 *      - UART_FLAG_TX_DONE: TX done(TX FIFO empty and TX waveform sent done).
 *
 * \return   New status of UART line_status flag.
 * \retval SET: The specified line_status flag bit is set.
 * \retval RESET: The specified line_status flag is not set.
 *
 * <b>Example usage</b>
 * \code{.c}
 * if (int_status == UART_INT_ID_LINE_STATUS)
 * {
 *      uint8_t line_status = UART_GetLineStatus(UART0);
 *
 *      if (UART_CheckLineStatus(line_status, UART_FLAG_BREAK_ERR))
 *      {
 *          //Add user code here.
 *      }
 * }
 * \endcode
 */
static __forceinline uint8_t UART_CheckLineStatus(uint8_t line_status, uint32_t line_status_flag)
{
    assert_param(IS_UART_GET_FLAG(line_status_flag));

    return (line_status & line_status_flag);
}

/**
 *
 * \brief   Clear TX FIFO of the selected UART peripheral.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     UART_ClearTxFIFO(UART0);
 * }
 * \endcode
 */
static __forceinline void UART_ClearTxFifo(UART_TypeDef *UARTx)
{
    /* Check the parameters */
    assert_param(IS_UART_PERIPH(UARTx));

    UARTx->INTID_FCR = (((UARTx->STSR & BIT24) >> 21) | ((UARTx->STSR & 0x7C000000) >> 18) | ((
                            UARTx->STSR & BIT25) >> 25) | FCR_CLEAR_TX_FIFO_Set);

    return;
}

/**
 *
 * \brief   Clear RX FIFO of the selected UART peripheral.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     UART_ClearRxFIFO(UART0);
 * }
 * \endcode
 */
static __forceinline void UART_ClearRxFifo(UART_TypeDef *UARTx)
{
    /* Check the parameters */
    assert_param(IS_UART_PERIPH(UARTx));

    UARTx->INTID_FCR = (((UARTx->STSR & BIT24) >> 21) | ((UARTx->STSR & 0x7C000000) >> 18) | ((
                            UARTx->STSR & BIT25) >> 25) | FCR_CLEAR_RX_FIFO_Set);

    return;
}

/**
 *
 * \brief   Get the data length in TX FIFO of the selected UART peripheral.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 *
 * \return   Data length in UART TX FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     uint8_t data_len = UART_GetTxFIFOLen(UART0);
 * }
 * \endcode
 */
static __forceinline uint8_t UART_GetTxFIFOLen(UART_TypeDef *UARTx)
{
    /* Check the parameters */
    assert_param(IS_UART_PERIPH(UARTx));

    return (uint8_t)(UARTx->FIFO_LEVEL & 0x1F);
}

/**
 *
 * \brief   Get the data length in RX FIFO of the selected UART peripheral.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 *
 * \return   Data length in UART RX FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void uart_demo(void)
 * {
 *     uint8_t data_len = UART_GetRxFIFOLen(UART0);
 * }
 * \endcode
 */
static __forceinline uint8_t UART_GetRxFIFOLen(UART_TypeDef *UARTx)
{
    /* Check the parameters */
    assert_param(IS_UART_PERIPH(UARTx));

    return (uint8_t)((UARTx->FIFO_LEVEL >> 8) & 0x3F);
}

/**
 *
 * \brief    Enable/Disable DMA mode on UART side.
 *
 * \param[in]   UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in]   newState: Enable or disable UART DMA mode.
 *      This parameter can be one of the following values:
 *      - ENABLE: Enable UART RX DMA mode, can use DMA to receive data.
 *      - DISABLE: Disable UART RX DMA mode, cannot use DMA to receive data.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_uart_init(void)
 * {
 *     UART_RxDmaCmd(UART0, DISABLE);
 * }
 * \endcode
 */
static __forceinline void UART_RxDmaCmd(UART_TypeDef *UARTx, FunctionalState newState)
{
    /* Check the parameters */
    assert_param(IS_UART_PERIPH(UARTx));

    if (newState != DISABLE)
    {
        UARTx->MISCR |= BIT2;
    }
    else
    {
        UARTx->MISCR &= ~BIT2;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_UART_H_ */

/** @} */ /* End of group 87x3e_UART_Exported_Functions */
/** @} */ /* End of group 87x3e_UART */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/
