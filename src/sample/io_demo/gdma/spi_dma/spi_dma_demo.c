/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_dma_demo.c
* @brief    This file provides demo code of SPI send and receive data by dma mode.
* @details
* @author
* @date
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "dma_channel.h"
#include "trace.h"

/** @defgroup  SPI_DMA_DEMO  SPI DMA DEMO
    * @brief  Spi send and receive data by dma mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI_Demo_Exported_Macros SPI Demo Exported Macros
  * @brief
  * @{
  */

#define PIN_SPI1_SCK                P0_0
#define PIN_SPI1_MOSI               P1_0
#define PIN_SPI1_MISO               P1_1
#define PIN_SPI1_CS                 P0_1

#define TEST_SIZE               200

/** @} */ /* End of group SPI_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup SPI_DMA_Exported_Variables SPI data transfered by gdma Variables
    * @brief
    * @{
    */

static uint8_t sendbuf[TEST_SIZE];
static uint8_t readbuf[TEST_SIZE];
static uint8_t spi_tx_dma_ch_num = 0xa5;
static uint8_t spi_rx_dma_ch_num = 0xa5;

#define SPI_TX_DMA_CHANNEL_NUM     spi_tx_dma_ch_num
#define SPI_TX_DMA_CHANNEL         DMA_CH_BASE(spi_tx_dma_ch_num)
#define SPI_TX_DMA_IRQ             DMA_CH_IRQ(spi_tx_dma_ch_num)

#define SPI_RX_DMA_CHANNEL_NUM     spi_rx_dma_ch_num
#define SPI_RX_DMA_CHANNEL         DMA_CH_BASE(spi_rx_dma_ch_num)
#define SPI_RX_DMA_IRQ             DMA_CH_IRQ(spi_rx_dma_ch_num)

/** @} */ /* End of group SPI_GDMA_Exported_Variables */

static void spi_tx_dma_handler(void);
static void spi_rx_dma_handler(void);

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup SPI_Demo_Exported_Functions SPI Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_spi_init(void)
{
    Pinmux_Config(PIN_SPI1_SCK, SPI1_CLK_MASTER);
    Pinmux_Config(PIN_SPI1_MOSI, SPI1_MO_MASTER);
    Pinmux_Config(PIN_SPI1_MISO, SPI1_MI_MASTER);
    Pinmux_Config(PIN_SPI1_CS, SPI1_SS_N_0_MASTER);

    Pad_Config(PIN_SPI1_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI1_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI1_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI1_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
}

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_spi_init(void)
{
    /* turn on SPI clock */
    RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 100;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_TxWaterlevel        = 7;
    SPI_InitStructure.SPI_RxWaterlevel        = 1;

    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
}


/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
static void spi_send_dma_init(void)
{
    for (uint16_t i = 0; i < TEST_SIZE; i++)
    {
        sendbuf[i] = i;
    }


    if (!GDMA_channel_request(&spi_tx_dma_ch_num, spi_tx_dma_handler, true))
    {
        IO_PRINT_ERROR0("spi_send_dma_init: tx channel request fail");
        return;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    /*--------------GDMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_TX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToPeripheral;//GDMA_DIR_MemoryToPeripheral
    GDMA_InitStruct.GDMA_BufferSize      = TEST_SIZE;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)sendbuf;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)SPI1->DR;
    GDMA_InitStruct.GDMA_DestHandshake = GDMA_Handshake_SPI1_TX;

    GDMA_Init(SPI_TX_DMA_CHANNEL, &GDMA_InitStruct);

    GDMA_INTConfig(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_TX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
static void spi_receive_dma_init(void)
{
    for (uint16_t i = 0; i < TEST_SIZE; i++)
    {
        readbuf[i] = 0;
    }

    if (!GDMA_channel_request(&spi_rx_dma_ch_num, spi_rx_dma_handler, true))
    {
        IO_PRINT_ERROR0("spi_receive_dma_init: rx channel request fail");
        return;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;
    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;//GDMA_DIR_PeripheralToMemory
    GDMA_InitStruct.GDMA_BufferSize      = TEST_SIZE;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)SPI1->DR;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(readbuf);
    GDMA_InitStruct.GDMA_SourceHandshake = GDMA_Handshake_SPI1_RX;

    GDMA_Init(SPI_RX_DMA_CHANNEL, &GDMA_InitStruct);

    GDMA_INTConfig(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    SPI_GDMACmd(SPI1, SPI_GDMAReq_Rx, DISABLE);
    SPI_GDMACmd(SPI1, SPI_GDMAReq_Rx, ENABLE);

    /*-----------------start to receive data-----------*/
    GDMA_Cmd(SPI_RX_DMA_CHANNEL_NUM, ENABLE);
}

/**
  * @brief  demo code of operation about SPI.
  * @param   No parameter.
  * @return  void
  */
void spi_dma_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_spi_init();

    /* Initialize SPI peripheral */
    driver_spi_init();

    spi_send_dma_init();

    spi_receive_dma_init();

    SPI_GDMACmd(SPI1, SPI_GDMAReq_Tx, DISABLE);
    SPI_GDMACmd(SPI1, SPI_GDMAReq_Tx, ENABLE);
    /*-----------------start to send data-----------*/
    GDMA_Cmd(SPI_TX_DMA_CHANNEL_NUM, ENABLE);
}


/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void spi_tx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    /* It is recommended to post the os msg to the task thread for data processing. */
    IO_PRINT_TRACE0("spi_tx_dma_handler");
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void spi_rx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    /* It is recommended to post the os msg to the task thread for data processing. */
    IO_PRINT_TRACE0("spi_rx_dma_handler");
}


/** @} */ /* End of group SPI_Demo_Exported_Functions */
/** @} */ /* End of group SPI_DMA_DEMO */


