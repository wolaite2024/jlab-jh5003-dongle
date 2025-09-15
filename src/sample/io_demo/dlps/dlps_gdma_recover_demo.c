/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dlps_gdma_recover_demo.c
* @brief    This file provides demo code of GDMA recover from DLPS mode.
* @details
* @author   Colin
* @date     2023-03-24
* @version  v1.0
*********************************************************************************************************
*/



/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "io_dlps.h"
#include "pm.h"
#include "dma_channel.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_uart.h"
#include "vector_table.h"
#include "section.h"

/** @defgroup  DLPS_GDMA_RECOVER_DEMO  DLPS GDMA RECOVER
    * @brief  DLPS GDMA recover implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup DLPS_GDMA_RECOVER_Exported_Macros DLPS GDMA recover Exported Macros
  * @brief
  * @{
  */
#define WAKE_UP_PIN             P0_0

#define UART_BUFFER_SIZE        1024
#define UART_TX_PIN             P3_1
#define UART_RX_PIN             P3_0

#define UART_RX_DMA_CHANNEL_NUM              uart_rx_dma_ch_num
#define UART_RX_DMA_CHANNEL                  DMA_CH_BASE(uart_rx_dma_ch_num)
#define UART_RX_DMA_IRQ                      DMA_CH_IRQ(uart_rx_dma_ch_num)

/** @} */ /* End of group DLPS_GDMA_RECOVER_Exported_Macros */

/* uart received data */
static uint8_t uart_receive_buf[UART_BUFFER_SIZE];

/* uart dma channel */
static uint8_t uart_rx_dma_ch_num = 0xa5;

/* uart allow enter dlps */
static bool uart_allow_enter_dlps = true;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup DLPS_GDMA_Recover_Exported_Functions
  * @brief
  * @{
  */

/**
* @brief  Data Uart interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void data_uart_handler(void)
{
    uint32_t int_status = UART_GetIID(UART0);
    uint16_t rx_count;

    /* disable interrupt */
    if (UART_GetFlagState(UART, UART_FLAG_RX_IDLE) == SET)
    {
        //clear Flag
        UART_INTConfig(UART, UART_INT_IDLE, DISABLE);

        if (GDMA_GetChannelStatus(UART_RX_DMA_CHANNEL_NUM))
            /*suspend at first to let DMA pop the fifo data*/
        {
            GDMA_SuspendCmd(UART_RX_DMA_CHANNEL, ENABLE);
        }
        /*should waiting or fifo_count will not right when rx one byte or three byte*/
        while (GDMA_GetFIFOStatus(UART_RX_DMA_CHANNEL) != SET);

        /*read DMA total transfer count*/
        rx_count = GDMA_GetTransferLen(UART_RX_DMA_CHANNEL);

        IO_PRINT_INFO1("data_uart_handler: rx_count %d", rx_count);

        /*abort DMA */
        GDMA_Cmd(UART_RX_DMA_CHANNEL_NUM, DISABLE);

        /* restart dma receive */
        GDMA_SetDestinationAddress(UART_RX_DMA_CHANNEL, (uint32_t)uart_receive_buf);
        GDMA_SuspendCmd(UART_RX_DMA_CHANNEL, DISABLE);
        GDMA_Cmd(UART_RX_DMA_CHANNEL_NUM, ENABLE);

        UART_INTConfig(UART0, UART_INT_IDLE, ENABLE);

        /* Uart receive finish and allow enter DLPS */
        uart_allow_enter_dlps = true;
    }

    switch (int_status)
    {

    /* tx fifo empty, not enable */
    case UART_INT_ID_TX_EMPTY:
        break;

    /* rx data valiable */
    case UART_INT_ID_RX_LEVEL_REACH:
        break;

    case UART_INT_ID_RX_TMEOUT:
        break;

    /* receive line status interrupt */
    case UART_INT_ID_LINE_STATUS:
        {
            UART_INTConfig(UART0, UART_INT_LINE_STS, DISABLE);
            IO_PRINT_ERROR1("data_uart_handler: Line status error, status 0x%x", UART_GetLineStatus(UART));
            UART_INTConfig(UART0, UART_INT_LINE_STS, ENABLE);
        }
        break;

    default:
        break;
    }

    return;
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void uart_rx_dma_handler(void)
{
    IO_PRINT_TRACE0("uart_rx_dma_handler");
    GDMA_ClearINTPendingBit(UART_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
}

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_dma_uart_init(void)
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
    UART_DeInit(UART0);
    RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);
    /* uart init */
    UART_InitTypeDef uartInitStruct;
    UART_StructInit(&uartInitStruct);

    /* change default rx trigger level */
    uartInitStruct.rxTriggerLevel = 29;
    /* Config the baudrate is 3M */
    uartInitStruct.div            = 1;
    uartInitStruct.ovsr           = 8;
    uartInitStruct.ovsr_adj       = 0x492;
    uartInitStruct.dmaEn = UART_DMA_ENABLE;
    uartInitStruct.RxWaterlevel = 1;
    uartInitStruct.RxDmaEn        = ENABLE;
    UART_Init(UART0, &uartInitStruct);

    //enable rx interrupt and line status interrupt
    UART_INTConfig(UART0, UART_INT_LINE_STS | UART_INT_IDLE, ENABLE);

    RamVectorTableUpdate(UART0_VECTORn, (IRQ_Fun)data_uart_handler);

    /*  Enable UART IRQ  */
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = UART0_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

static void uart_rx_gdma_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    GDMA_InitTypeDef GDMA_InitStruct;

    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = UART_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;//GDMA_DIR_PeripheralToMemory
    GDMA_InitStruct.GDMA_BufferSize      = UART_BUFFER_SIZE;//determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)(&(UART0->RB_THR));
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(uart_receive_buf);
    GDMA_InitStruct.GDMA_SourceHandshake    = GDMA_Handshake_UART0_RX;

    GDMA_Init(UART_RX_DMA_CHANNEL, &GDMA_InitStruct);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = UART_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_INTConfig(UART_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------start to receive data-----------*/
    GDMA_Cmd(UART_RX_DMA_CHANNEL_NUM, ENABLE);
}
/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_gdma_init(void)
{
    /* get free gdma channel */
    if (!GDMA_channel_request(&uart_rx_dma_ch_num, uart_rx_dma_handler, true))
    {
        return;
    }

    uart_rx_gdma_init();
}

RAM_TEXT_SECTION static bool uart_dlps_check_callback(void)
{
    return uart_allow_enter_dlps;
}

static void dlps_store(void)
{
    /* Config PAD pull up */
    Pad_PullUpOrDownValue(WAKE_UP_PIN, 1);
    /*set wakeup pin and polarity*/
    System_WakeUpPinEnable(WAKE_UP_PIN, PAD_WAKEUP_POL_LOW);

    DBG_DIRECT("dlps_store: enter dlps");
}

static void dlps_restore(void)
{
    /* Wait for uart receive data */
    uart_allow_enter_dlps = false;

    /* GDMA should reinitialize when exit from DLPS */
    if (uart_rx_dma_ch_num != 0xa5)
    {
        uart_rx_gdma_init();
    }

    IO_PRINT_TRACE0("dlps_restore: exit dlps");
}

static void dlps_gdma_recover_init(void)
{
    /*IO register  store restore callback register*/
    io_dlps_register();

    /* register of call back function */
    power_check_cb_register(uart_dlps_check_callback);

    /*user callback fuciton register*/
    io_dlps_register_enter_cb(dlps_store);
    io_dlps_register_exit_cb(dlps_restore);

    /*set btmac to dsm mode*/
    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    /*set platfrom as dlps mode*/
    power_mode_set(POWER_DLPS_MODE);
}

/**
  * @brief  demo code of operation about GPIO.
  * @param   No parameter.
  * @return  void
  */
void dlps_gdma_recover_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_dma_uart_init();

    /* Initialize UART peripheral */
    driver_uart_init();

    /* Initialize GDMA peripheral */
    driver_gdma_init();

    /* dlps gdma recover init */
    dlps_gdma_recover_init();
}
/** @} */ /* End of group DLPS_GDMA_Recover_Exported_Functions */
/** @} */ /* End of group DLPS_GDMA_RECOVER_DEMO */
