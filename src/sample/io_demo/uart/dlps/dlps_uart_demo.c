/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dlps_uart_demo.c
* @brief    This file provides demo code of uart work in dlps mode.
* @details
* @author   renee
* @date     2017-02-27
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "string.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "rtl876x_uart.h"
#include "rtl876x_rcc.h"
#include "os_task.h"
#include "os_msg.h"
#include "trace.h"
#include "pm.h"
#include "io_dlps.h"
#include "vector_table.h"

/** @defgroup  DLPS_UART_DEMO  UART DEMO DLPS
    * @brief  Uart work in system dlps mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup UART_Dlps_Demo_Exported_Macros UART Dlps Demo Exported Macros
  * @brief
  * @{
  */
#define UART_TX_PIN             P3_1
#define UART_RX_PIN             P3_0

/* queue size */
#define IO_DEMO_EVENT_QUEUE_SIZE                    0x10

#define IO_DEMO_EVENT_UART_RX                       0x01

/** @} */ /* End of group UART_Dlps_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup UART_Dlps_Demo_Exported_Variables UART Dlps Demo Exported Variables
  * @brief
  * @{
  */
/* task handle & queue handle */
static void *io_queue_handle;
static void *iodemo_app_task_handle;

/* global */
static uint8_t RxBuffer[600];
static uint32_t RxCount = 0;
static bool  allowedSystemEnterDlps = true;

/** @} */ /* End of group UART_Dlps_Demo_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup UART_Dlps_Demo_Exported_Functions UART Dlps Demo Exported Functions
  * @brief
  * @{
  */

static void io_demo_task(void *param);
static void uart2_handler(void);


/*============================================================================*
 *                         Public   Functions
 *============================================================================*/
/**
  * @brief  Initialize peripheral Task and IO message queue, call this function to demonstrate
  *         uart functions in dlps mode.
  * @param   No parameter.
  * @return  void
  */
void dlps_uart_demo(void)
{
    os_task_create(&iodemo_app_task_handle, "app", io_demo_task, NULL, 384 * 4, 2);

    os_msg_queue_create(&io_queue_handle, "uartRxQ", IO_DEMO_EVENT_QUEUE_SIZE, sizeof(uint8_t));
}

/*============================================================================*
 *                         Private   Functions
 *============================================================================*/

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_uart_init(void)
{
    Pad_Config(UART_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(UART_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(UART_TX_PIN, UART2_TX);
    Pinmux_Config(UART_RX_PIN, UART2_RX);
}

/**
  * @brief  Initialize UART peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_uart_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_UART2, APBPeriph_UART2_CLOCK, ENABLE);
    /* uart init */
    UART_InitTypeDef uartInitStruct;
    UART_StructInit(&uartInitStruct);

    /* default baudrate is 115200 */
    UART_Init(UART2, &uartInitStruct);

    RamVectorTableUpdate(UART2_VECTORn, (IRQ_Fun)uart2_handler);

    //enable rx interrupt and line status interrupt
    UART_INTConfig(UART2, UART_INT_RD_AVA | UART_INT_IDLE, ENABLE);

    /*  Enable UART IRQ  */
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = UART2_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

/**
  * @brief  uart send string buffer.
  * @param   No parameter.
  * @return  void
  */
static void uart_send_str(char *str, uint16_t str_len)
{
    uint8_t blk, remain, i;
    blk = str_len / UART_TX_FIFO_SIZE;
    remain = str_len % UART_TX_FIFO_SIZE;

    //send through uart
    for (i = 0; i < blk; i++)
    {
        UART_SendData(UART2, (uint8_t *)&str[16 * i], 16);
        while (UART_GetFlagState(UART2, UART_FLAG_THR_EMPTY) != SET);
    }

    UART_SendData(UART2, (uint8_t *)&str[16 * i], remain);
    while (UART_GetFlagState(UART2, UART_FLAG_THR_EMPTY) != SET);
}

/**
  * @brief  IO Enter dlps call back function.
  * @param   No parameter.
  * @return  void
  */
static void uart_dlps_enter(void)
{
    /* switch pad to Software mode */
    Pad_ControlSelectValue(UART_TX_PIN, PAD_SW_MODE);
    Pad_ControlSelectValue(UART_RX_PIN, PAD_SW_MODE);

    System_WakeUpPinEnable(UART_RX_PIN, PAD_WAKEUP_POL_LOW);
    /* To Debug */
    DBG_DIRECT("uart_dlps_enter");
}

/**
  * @brief  IO Exit dlps call back function.
  * @param   No parameter.
  * @return  void
  */
static void uart_dlps_exit(void)
{
    /* switch pad to Pinmux mode */
    Pad_ControlSelectValue(UART_TX_PIN, PAD_PINMUX_MODE);
    Pad_ControlSelectValue(UART_RX_PIN, PAD_PINMUX_MODE);
    if (System_WakeUpInterruptValue(UART_RX_PIN) == SET)
    {
        allowedSystemEnterDlps = false;
    }
    IO_PRINT_INFO0("uart_dlps_exit");
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
static bool io_dlps_check(void)
{
    return allowedSystemEnterDlps;
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
static void power_uart_init(void)
{
    power_check_cb_register(io_dlps_check);
    io_dlps_register();

    power_stage_cb_register(uart_dlps_enter, POWER_STAGE_STORE);
    power_stage_cb_register(uart_dlps_exit, POWER_STAGE_RESTORE);

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    power_mode_set(POWER_DLPS_MODE);
}

/**
  * @brief  IO_Demo Task Handle.
  * @param   No parameter.
  * @return  void
  */
static void io_demo_task(void *param)
{
    uint8_t event = 0;
    uint8_t strLen = 0;
    uint16_t index = 0;

    /* Pinmux & Pad Config */
    board_uart_init();

    /* Initialize UART peripheral */
    driver_uart_init();

    /* Power Setting */
    power_uart_init();


    /* Send demo buffer */
    char *demoStr = "### Welcome to use RealTek Bumblebee ###\r\n";
    strLen = strlen(demoStr);
    uart_send_str(demoStr, strLen);

    while (1)
    {
        if (os_msg_recv(io_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == IO_DEMO_EVENT_UART_RX)
            {
                uart_send_str((char *)RxBuffer, RxCount);

                for (index = 0; index < 600; index++)
                {
                    RxBuffer[index] = 0;
                }

                RxCount = 0;
            }

        }
    }
}

/**
* @brief  Data Uart interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void uart2_handler(void)
{
    uint8_t event = IO_DEMO_EVENT_UART_RX;
    uint32_t int_status = 0;
    uint8_t rxfifocnt = 0;
    int_status = UART_GetIID(UART2);

    if (UART_GetFlagState(UART2, UART_FLAG_RX_IDLE) == SET)
    {
        //clear Flag
        UART_INTConfig(UART2, UART_INT_IDLE, DISABLE);
        if (os_msg_send(io_queue_handle, &event, 0) == false)
        {
            IO_PRINT_ERROR0("uart2_handler: Send Queue Error");
        }
        allowedSystemEnterDlps = true;
        UART_INTConfig(UART2, UART_INT_IDLE, ENABLE);
    }

    switch (int_status)
    {
    /* tx fifo empty, not enable */
    case UART_INT_ID_TX_EMPTY:
        break;

    /* rx data valiable */
    case UART_INT_ID_RX_LEVEL_REACH:
        rxfifocnt = UART_GetRxFIFOLen(UART2);
        UART_ReceiveData(UART2, &RxBuffer[RxCount], rxfifocnt);
        RxCount += rxfifocnt;
        break;

    case UART_INT_ID_RX_TMEOUT:
        rxfifocnt = UART_GetRxFIFOLen(UART2);
        UART_ReceiveData(UART2, &RxBuffer[RxCount], rxfifocnt);
        RxCount += rxfifocnt;
        break;

    /* receive line status interrupt */
    case UART_INT_ID_LINE_STATUS:
        {
            IO_PRINT_ERROR1("uart2_handler: Line status error, Status 0x%x", UART_GetLineStatus(UART2));
        }
        break;

    default:
        break;
    }

    return;
}

/** @} */ /* End of group UART_Dlps_Demo_Exported_Functions */
/** @} */ /* End of group DLPS_UART_DEMO */
