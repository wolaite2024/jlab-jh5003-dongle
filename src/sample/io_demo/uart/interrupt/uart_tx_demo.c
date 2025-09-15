/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     uart_tx_demo.c
* @brief    This file provides demo code of uart tx data by interrupt.
* @details
* @author
* @date
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "rtl876x_nvic.h"
#include "rtl876x_uart.h"
#include "rtl876x_rcc.h"
#include "vector_table.h"
#include "rtl876x_pinmux.h"

/** @defgroup  UART_DEMO  UART DEMO
    * @brief  Uart implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup UART_Interrupt_Exported_Macros UART Interrupt Exported Macros
  * @brief
  * @{
  */

#define UART_TX_PIN         P3_1
#define UART_RX_PIN         P3_0

/** @} */ /* End of group UART_Interrupt_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup UART_Interrupt_Exported_Variables UART Interrupt Exported Variables
  * @brief
  * @{
  */

static uint8_t DemoStrBuffer[100];
static uint8_t *uart_tx_curr_addr;
static uint32_t uart_tx_len;

/** @} */ /* End of group UART_Interrupt_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup UART_Interrupt_Exported_Functions UART Interrupt Exported Functions
  * @brief
  * @{
  */

static void uart0_interrupt_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   void
  * @return  void
  */
static void board_uart_init(void)
{
    Pad_Config(UART_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(UART_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(UART_TX_PIN, UART0_TX);
    Pinmux_Config(UART_RX_PIN, UART0_RX);
}

/**
  * @brief  Initialize UART peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_uart_init(void)
{
    /* turn on UART clock */
    RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);

    /* uart init */
    UART_InitTypeDef uartInitStruct;
    UART_StructInit(&uartInitStruct);

    UART_Init(UART0, &uartInitStruct);

    RamVectorTableUpdate(UART0_VECTORn, uart0_interrupt_handler);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = UART0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

static void uart_send_data(uint8_t *buf, uint32_t len)
{
    uint32_t tx_len = len > UART_TX_FIFO_SIZE ? UART_TX_FIFO_SIZE : len;

    UART_SendData(UART0, buf, tx_len);

    uart_tx_curr_addr = buf + tx_len;
    uart_tx_len = len - tx_len;

    UART_INTConfig(UART0, UART_INT_FIFO_EMPTY, ENABLE);
}

/**
  * @brief  demo code of operation about UART.
  * @param   No parameter.
  * @return  void
  */
void uart_tx_demo(void)
{
    uint16_t  strLen = 0;

    /* Configure PAD and pinmux firstly! */
    board_uart_init();

    /* Initialize UART peripheral */
    driver_uart_init();

    char *demoStr = "### Welcome to use RealTek Bumblebee ###\r\n";
    strLen = strlen(demoStr);
    memcpy(DemoStrBuffer, demoStr, strLen);

    uart_tx_curr_addr = DemoStrBuffer;
    uart_tx_len = strLen;

    uart_send_data(uart_tx_curr_addr, uart_tx_len);
}

static void uart0_interrupt_handler(void)
{
    uint32_t int_status = 0;

    /* read interrupt id */
    int_status = UART_GetIID(UART0);

    switch (int_status)
    {
    /* tx fifo empty */
    case UART_INT_ID_TX_EMPTY:
        UART_INTConfig(UART0, UART_INT_FIFO_EMPTY, DISABLE);
        UART_GetIID(UART0);
        if (uart_tx_len)
        {
            uart_send_data(uart_tx_curr_addr, uart_tx_len);
        }
        break;

    /* rx data valiable */
    case UART_INT_ID_RX_LEVEL_REACH:
        break;

    case UART_INT_ID_RX_TMEOUT:
        break;

    /* receive line status interrupt */
    case UART_INT_ID_LINE_STATUS:
        break;

    default:
        break;
    }
    return;
}

/** @} */ /* End of group UART_Interrupt_Exported_Functions */
/** @} */ /* End of group UART_DEMO */
