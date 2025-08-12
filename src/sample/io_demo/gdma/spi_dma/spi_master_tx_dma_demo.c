/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_master_tx_dma_demo.c
* @brief    This file provides demo code of SPI master send data by dma mode.
* @details
* @author
* @date     2024-3-13
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
#include "vector_table.h"
#include "trace.h"
#include "rtl876x_gdma.h"
#include "dma_channel.h"
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI_Demo_Exported_Macros SPI Demo Exported Macros
  * @brief
  * @{
  */

/* Master Macros*/
#define SPI0_SCK                P2_1
#define SPI0_MOSI               P2_2
#define SPI0_MISO               P2_3
#define SPI0_CS                 P0_1

#define SPI_MASTER_TX_DMA_CHANNEL_NUM     spi_master_tx_ch_num
#define SPI_MASTER_TX_DMA_CHANNEL         DMA_CH_BASE(spi_master_tx_ch_num)
#define SPI_MASTER_TX_DMA_IRQ             DMA_CH_IRQ(spi_master_tx_ch_num)
#define SPI_MASTER_TX_DMA_VECTOR          DMA_CH_VECTOR(spi_master_tx_ch_num)
#define DMA_Handshake_SPI_MASTER_TX       GDMA_Handshake_SPI0_TX

static uint8_t spi_master_tx_ch_num = 0x55;

#define SPI_DATA_LEN               60

uint8_t SPI_TX_Buf[SPI_DATA_LEN] = {0};

static void spi_master_tx_dma_handler(void);

void board_spi_init(void)
{
    Pinmux_Config(SPI0_SCK, SPI0_CLK_MASTER);
    Pinmux_Config(SPI0_MOSI, SPI0_MO_MASTER);
    Pinmux_Config(SPI0_MISO, SPI0_MI_MASTER);
    Pinmux_Config(SPI0_CS, SPI0_SS_N_0_MASTER);

    Pad_Config(SPI0_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

void spi_master_init(void)
{
    /* turn on SPI clock */
    SPI_DeInit(SPI0);
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 100;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_InitStructure.SPI_TxWaterlevel       = 7;

    SPI_Init(SPI0, &SPI_InitStructure);
    SPI_Cmd(SPI0, ENABLE);
}

static void spi_master_tx_dma_init(void)
{
    for (uint16_t i = 0; i < SPI_DATA_LEN; i++)
    {
        SPI_TX_Buf[i] = 0xAA;
    }

    if (!GDMA_channel_request(&spi_master_tx_ch_num, spi_master_tx_dma_handler, true))
    {
        IO_PRINT_INFO0("spi_master_tx_dma_init: tx channel request fail");
        return;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    /*--------------GDMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_MASTER_TX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize      = SPI_DATA_LEN;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)SPI_TX_Buf;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)SPI0->DR;
    GDMA_InitStruct.GDMA_DestHandshake = GDMA_Handshake_SPI0_TX;

    GDMA_Init(SPI_MASTER_TX_DMA_CHANNEL, &GDMA_InitStruct);
    GDMA_INTConfig(SPI_MASTER_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_MASTER_TX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

}

void spi_master_tx_dma_demo(void)
{
    board_spi_init();
    spi_master_init();
    spi_master_tx_dma_init();

    SPI_GDMACmd(SPI0, SPI_GDMAReq_Tx, DISABLE);
    SPI_GDMACmd(SPI0, SPI_GDMAReq_Tx, ENABLE);
    /*---------------start to send data-----------*/
    GDMA_Cmd(SPI_MASTER_TX_DMA_CHANNEL_NUM, ENABLE);
}

static void spi_master_tx_dma_handler(void)
{
    uint16_t tx_len_all = GDMA_GetTransferLen(SPI_MASTER_TX_DMA_CHANNEL);
    IO_PRINT_INFO1("spi_master_tx_dma_handler! tx_len_all %d", tx_len_all);

    GDMA_INTConfig(SPI_MASTER_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, DISABLE);
    GDMA_ClearINTPendingBit(SPI_MASTER_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);

    GDMA_INTConfig(SPI_MASTER_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
}
