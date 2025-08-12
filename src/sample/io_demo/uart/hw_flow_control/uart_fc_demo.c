/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     uart_fc_demo.c
* @brief    uart demo-- auto hardware flow control
* @details
* @author   renee
* @date     2017-07-10
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "rtl876x_uart.h"
#include "rtl876x_rcc.h"
#include "os_task.h"
#include "os_msg.h"
#include "trace.h"
#include "vector_table.h"

/** @defgroup  UART_FC_DEMO  UART FLOW CONTROL DEMO
    * @brief  Uart work in hw flow control mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup UART_FlowControl_Demo_Exported_Macros UART FlowControl Demo Exported Macros
  * @brief
  * @{
  */

#define UART_TX_PIN         P3_1
#define UART_RX_PIN         P3_0
#define UART_CTS_PIN        P0_0
#define UART_RTS_PIN        P0_1

#define IO_DEMO_EVENT_QUEUE_SIZE                    0x10
#define IO_DEMO_EVENT_UART_RX                       0x01

/** @} */ /* End of group UART_FlowControl_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup UART_FlowControl_Demo_Exported_Variables UART FlowControl Demo Exported Variables
  * @brief
  * @{
  */
static void *io_queue_handle;
static void *iodemo_app_task_handle;

static uint8_t RxBuffer[100];
static uint8_t DemoStrBuffer[100];
static uint8_t RxCount = 0;

/** @} */ /* End of group UART_FlowControl_Demo_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup UART_FlowControl_Demo_Exported_Functions UART FlowControl Demo Exported Functions
  * @brief
  * @{
  */
static void data_uart_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_uart_init(void)
{
    Pad_Config(UART_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(UART_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(UART_CTS_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(UART_RTS_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);

    Pinmux_Config(UART_TX_PIN, UART0_TX);
    Pinmux_Config(UART_RX_PIN, UART0_RX);
    Pinmux_Config(UART_CTS_PIN, UART0_CTS);
    Pinmux_Config(UART_RTS_PIN, UART0_RTS);
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

    /* change default rx trigger level */
    uartInitStruct.rxTriggerLevel = UART_RX_FIFO_TRIGGER_LEVEL_14BYTE;
    uartInitStruct.autoFlowCtrl = UART_AUTO_FLOW_CTRL_EN;

    UART_Init(UART, &uartInitStruct);

    //enable rx interrupt and line status interrupt
    UART_INTConfig(UART, UART_INT_RD_AVA | UART_INT_LINE_STS | UART_INT_IDLE, ENABLE);

    RamVectorTableUpdate(UART0_VECTORn, (IRQ_Fun)data_uart_handler);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = UART0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  IO_Demo Task Handle.
  * @param   No parameter.
  * @return  void
  */
static void io_demo_task(void *param)
{
    uint16_t  strLen = 0;
    uint16_t  remainder = 0;
    uint16_t  blkcount = 0;
    uint16_t  i = 0;
    uint8_t event = 0;

    /* Configure PAD and pinmux firstly! */
    board_uart_init();

    /* Initialize UART peripheral */
    driver_uart_init();

    char *demoStr = "### Uart demo--Auto Hardware Flow Contrl ###\r\n";
    strLen = strlen(demoStr);
    memcpy(DemoStrBuffer, demoStr, strLen);

    /* send demo tips */
    blkcount = strLen / UART_TX_FIFO_SIZE;
    remainder = strLen % UART_TX_FIFO_SIZE;

    /* send block bytes(16 bytes) */
    for (i = 0; i < blkcount; i++)
    {
        UART_SendData(UART, &DemoStrBuffer[16 * i], 16);
        /* wait tx fifo empty */
        while (UART_GetFlagState(UART, UART_FLAG_THR_TSR_EMPTY) != SET);
    }

    /* send left bytes */
    UART_SendData(UART, &DemoStrBuffer[16 * i], remainder);
    /* wait tx fifo empty */
    while (UART_GetFlagState(UART, UART_FLAG_THR_TSR_EMPTY) != SET);

    /* loop rx and tx */
    while (1)
    {
        /* rx end */
        if (os_msg_recv(io_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == IO_DEMO_EVENT_UART_RX)
            {
                /* 16 is uart rx FIFO size */
                blkcount = RxCount / 16;
                remainder = RxCount % 16;

                /* send block bytes(16 bytes) */
                for (i = 0; i < blkcount; i++)
                {
                    UART_SendData(UART, &RxBuffer[16 * i], 16);
                    /* wait tx fifo empty */
                    while (UART_GetFlagState(UART, UART_FLAG_THR_TSR_EMPTY) != SET);
                }

                /* send left bytes */
                UART_SendData(UART, &RxBuffer[16 * i], remainder);
                /* wait tx fifo empty */
                while (UART_GetFlagState(UART, UART_FLAG_THR_TSR_EMPTY) != SET);

                /* return */
                UART_SendByte(UART, '\r');
                UART_SendByte(UART, '\n');
                while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);

                RxCount = 0;
            }
        }
    }
}

/**
  * @brief  demo code of operation about UART.
  * @param   No parameter.
  * @return  void
  */
void uart_fc_demo(void)
{
    os_task_create(&iodemo_app_task_handle, "app", io_demo_task, NULL, 384 * 4, 2);

    os_msg_queue_create(&io_queue_handle, "uartRxQ", IO_DEMO_EVENT_QUEUE_SIZE, sizeof(uint8_t));
}

/**
* @brief  UART interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void data_uart_handler(void)
{
    uint8_t event = IO_DEMO_EVENT_UART_RX;
    uint32_t int_status = 0;
    uint8_t recv_len;
    uint8_t line_status = 0;

    if (UART_GetFlagState(UART, UART_FLAG_RX_IDLE) == SET)
    {
        UART_INTConfig(UART, UART_INT_IDLE, DISABLE);
        if (os_msg_send(io_queue_handle, &event, 0) == false)
        {
            IO_PRINT_ERROR0("data_uart_handler: Send Queue Error");
        }
        //user code here

        UART_INTConfig(UART, UART_INT_IDLE, ENABLE);
    }

    /* read interrupt id */
    int_status = UART_GetIID(UART);

    switch (int_status)
    {
    /* tx fifo empty, not enable */
    case UART_INT_ID_TX_EMPTY:
        break;

    /* rx data valiable */
    case UART_INT_ID_RX_LEVEL_REACH:
        recv_len = UART_GetRxFIFOLen(UART);
        UART_ReceiveData(UART, &RxBuffer[RxCount], recv_len);
        RxCount += recv_len;
        break;

    case UART_INT_ID_RX_TMEOUT:
        recv_len = UART_GetRxFIFOLen(UART);
        UART_ReceiveData(UART, &RxBuffer[RxCount], recv_len);
        RxCount += recv_len;

        break;

    /* receive line status interrupt */
    case UART_INT_ID_LINE_STATUS:
        line_status = UART_GetLineStatus(UART);
        IO_PRINT_ERROR1("data_uart_handler: line_status 0x%x", line_status);
        UART_SendByte(UART, line_status);
        break;

    default:
        break;
    }

    return;
}

/** @} */ /* End of group UART_FlowControl_Demo_Exported_Functions */
/** @} */ /* End of group UART_FC_DEMO */


