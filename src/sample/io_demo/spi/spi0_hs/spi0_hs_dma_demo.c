/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi0_hs_dma_demo.c
* @brief    This file provides dma demo code of spi0 master high speed mode for rtl87x3d.
* @details  Because the SPI speed is relatively fast, if the bus speed is not enough, or CPU busy, the SPI speed will also slow down.
*           Therefore, DMA mode is recommended at high speed.
* @author   colin_lu
* @date     2022-10-9
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "pll.h"
#include "pm.h"
#include "rtl876x_gdma.h"
#include "dma_channel.h"
#include "rtl876x_nvic.h"
#include "vector_table.h"
/** @defgroup  SPI0_HS_DEMO  SPI0 HS Demo
    * @brief  SPI0 hs implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI_Demo_Exported_Macros SPI Demo Exported Macros
  * @brief
  * @{
  */

/* If use SPI0_HS, recommend to use these pin */
#define SPI0_HS_SCK                P1_3
#define SPI0_HS_MOSI               P1_4
#define SPI0_HS_MISO               P1_5
#define SPI0_HS_CS                 P1_2

#define DMA_BUFFER_SIZE            1024
#define SEND_BUF_LEN               DMA_BUFFER_SIZE
#define READ_BUF_LEN               DMA_BUFFER_SIZE
static uint8_t sendbuf[DMA_BUFFER_SIZE];
static uint8_t readbuf[DMA_BUFFER_SIZE];
static uint8_t spi_tx_dma_ch_num = 0;  /* SPI0_HS use dma channel 0 due to high performance */
static uint8_t spi_rx_dma_ch_num = 1;  /* SPI0_HS use dma channel 1 due to high performance */

#define SPI_TX_DMA_CHANNEL_NUM     spi_tx_dma_ch_num
#define SPI_TX_DMA_CHANNEL         DMA_CH_BASE(spi_tx_dma_ch_num)
#define SPI_TX_DMA_IRQ             DMA_CH_IRQ(spi_tx_dma_ch_num)
#define SPI_TX_DMA_VECTOR          DMA_CH_VECTOR(spi_tx_dma_ch_num)

#define SPI_RX_DMA_CHANNEL_NUM     spi_rx_dma_ch_num
#define SPI_RX_DMA_CHANNEL         DMA_CH_BASE(spi_rx_dma_ch_num)
#define SPI_RX_DMA_IRQ             DMA_CH_IRQ(spi_rx_dma_ch_num)
#define SPI_RX_DMA_VECTOR          DMA_CH_VECTOR(spi_rx_dma_ch_num)

/** @} */ /* End of group SPI_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/

/** @defgroup SPI_Demo_Exported_Functions SPI transfer Exported Functions
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
    /* SPI0 */
    Pinmux_Config(SPI0_HS_SCK, SPI0_CLK_MASTER);
    Pinmux_Config(SPI0_HS_MOSI, SPI0_MO_MASTER);
    Pinmux_Config(SPI0_HS_MISO, SPI0_MI_MASTER);
    Pinmux_Config(SPI0_HS_CS, SPI0_SS_N_0_MASTER);

    Pad_Config(SPI0_HS_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(SPI0_HS_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(SPI0_HS_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(SPI0_HS_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

static void spi_tx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    IO_PRINT_INFO0("spi_tx_dma_handler");
}

static void spi_rx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    IO_PRINT_INFO0("spi_rx_dma_handler");
    for (uint16_t i = 0; i < READ_BUF_LEN; i++)
    {
        IO_PRINT_INFO2("spi_rx_dma_handler: readbuf[%d] 0x%x", i, readbuf[i]);
    }
    GDMA_Cmd(SPI_RX_DMA_CHANNEL_NUM, ENABLE);
}

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_spi_init(void)
{
    SPI_InitTypeDef SPI_InitStruct;
    uint32_t actual_mhz;
    int32_t ret;

    /*
        If spi0_hs clock set 40M, cpu freq must greater than 120M.
        If spi0_hs clock set 20M, cpu freq must greater than 80M.
    */
    ret = pm_cpu_freq_set(120, &actual_mhz);
    IO_PRINT_TRACE2("driver_spi_init: spi hs improve cpu freq, ret %d, actual_mhz %d", ret, actual_mhz);

    /* If choose pll as spi0 clock source, enable pll4 in 160M */
    pll4_enable_cko1();

    /* SPI0 clock source switch to pll clock */
    RCC_SPIClkSourceSwitch(SPI0, SPI_CLOCK_SOURCE_PLL);

    /* Open SPI0 clock */
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

    /* SPI0_HS only support master*/
    SPI_StructInit(&SPI_InitStruct);
    SPI_InitStruct.SPI_Direction = SPI_Direction_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_BaudRatePrescaler = 4;       /* SPI0_HS max clock : 160 / 4 = 40M */
    SPI_InitStruct.SPI_RxThresholdLevel = 0;
    SPI_InitStruct.SPI_FrameFormat = SPI_Frame_Motorola;
    /* If spi0 clock greater than 10M, SPI_RxSampleDly need to be set 1*/
    SPI_InitStruct.SPI_RxSampleDly = 1;

    SPI_InitStruct.SPI_TxDmaEn             = ENABLE;
    SPI_InitStruct.SPI_RxDmaEn             = ENABLE;
    SPI_InitStruct.SPI_TxWaterlevel        =
        25;     //Adjust waterlevel due to MSIZE, waterlevel < tx fifo size - msize
    SPI_InitStruct.SPI_RxWaterlevel        =
        31;     //Adjust waterlevel due to MSIZE, waterlevel = msize - 1

    /* detect receive data */
    SPI_Init(SPI0_HS, &SPI_InitStruct);

    SPI_Cmd(SPI0_HS, ENABLE);

    for (uint16_t i = 0; i < SEND_BUF_LEN; i++)
    {
        sendbuf[i] = i;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    /*--------------GDMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_TX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize      = DMA_BUFFER_SIZE;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)sendbuf;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)SPI0_HS->DR;
    GDMA_InitStruct.GDMA_DestHandshake = GDMA_Handshake_SPI0_TX;

    GDMA_Init(SPI_TX_DMA_CHANNEL, &GDMA_InitStruct);

    RamVectorTableUpdate(SPI_TX_DMA_VECTOR, spi_tx_dma_handler);
    GDMA_INTConfig(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_TX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    for (uint16_t i = 0; i < READ_BUF_LEN; i++)
    {
        readbuf[i] = 0;
    }

    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;//GDMA_DIR_PeripheralToMemory
    GDMA_InitStruct.GDMA_BufferSize      = DMA_BUFFER_SIZE;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)SPI0_HS->DR;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(readbuf);
    GDMA_InitStruct.GDMA_SourceHandshake = GDMA_Handshake_SPI0_RX;

    GDMA_Init(SPI_RX_DMA_CHANNEL, &GDMA_InitStruct);

    RamVectorTableUpdate(SPI_RX_DMA_VECTOR, spi_rx_dma_handler);
    GDMA_INTConfig(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------GDMA IRQ init-------------------*/
    nvic_init_struct.NVIC_IRQChannel         = SPI_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    /*-----------------start to receive data-----------*/
    GDMA_Cmd(SPI_RX_DMA_CHANNEL_NUM, ENABLE);
    GDMA_Cmd(SPI_TX_DMA_CHANNEL_NUM, ENABLE); //SEND DATA
}

/**
  * @brief  demo code of operation about SPI.
  * @param   No parameter.
  * @return  void
  */
void spi0_hs_dma_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_spi_init();

    /* Initialize SPI peripheral */
    driver_spi_init();
}

/** @} */ /* End of group SPI_Demo_Exported_Functions */
/** @} */ /* End of group SPI_DEMO */

