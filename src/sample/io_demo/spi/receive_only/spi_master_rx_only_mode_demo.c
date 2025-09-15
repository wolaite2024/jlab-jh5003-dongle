/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_master_rx_only_mode_demo.c
* @brief    This file provides demo code of SPI master read  data by receive only mode.
* @details
* @author   justin
* @date     2023-01-10
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"

#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "vector_table.h"
#include "rtl876x_gdma.h"
#include "rtl876x_pinmux.h"
#include "dma_channel.h"
/*============================================================================*
 *                              Macros
 *============================================================================*/

#define TEST_SIZE                        16

#define SPI_MASTER_RX_DMA_CHANNEL_NUM    spi_master_rx_dma_ch_num
#define SPI_MASTER_RX_DMA_CHANNEL        DMA_CH_BASE(spi_master_rx_dma_ch_num)
#define SPI_MASTER_RX_DMA_IRQ            DMA_CH_IRQ(spi_master_rx_dma_ch_num)
#define SPI_MASTER_RX_DMA_VECTOR         DMA_CH_VECTOR(spi_master_rx_dma_ch_num)

#define PIN_SPI_SCK                      P0_0
#define PIN_SPI_MOSI                     P0_1
#define PIN_SPI_MISO                     P0_2
#define PIN_SPI_CS                       P0_3

#define SPI_CLK_MASTER                   SPI1_CLK_MASTER
#define SPI_MO_MASTER                    SPI1_MO_MASTER
#define SPI_MI_MASTER                    SPI1_MI_MASTER
#define SPI_SS_N_0_MASTER                SPI1_SS_N_0_MASTER

#define SPI_MST                          SPI1
#define SPI_MST_VECTORn                  SPI1_VECTORn
#define SPI_MST_IRQn                     SPI1_IRQn
#define APBPeriph_SPI_CLOCK              APBPeriph_SPI1_CLOCK
#define APBPeriph_SPI                    APBPeriph_SPI1

#define DMA_Handshake_SPI_MST_RX         GDMA_Handshake_SPI1_RX
/*============================================================================*
 *                              Variables
 *============================================================================*/

static uint8_t read_buf[TEST_SIZE];
static uint8_t spi_master_rx_dma_ch_num = 0x55;

/*============================================================================*
 *                              Functions
 *============================================================================*/

static void spi_master_rx_dma_handler(void);

static void board_spi_init(void)
{
    Pinmux_Config(PIN_SPI_SCK, SPI_CLK_MASTER);
    Pinmux_Config(PIN_SPI_MOSI, SPI_MO_MASTER);
    Pinmux_Config(PIN_SPI_MISO, SPI_MI_MASTER);
    Pinmux_Config(PIN_SPI_CS, SPI_SS_N_0_MASTER);

    Pad_Config(PIN_SPI_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

static void driver_master_spi_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_SPI, APBPeriph_SPI_CLOCK, ENABLE);
    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction   = SPI_Direction_RxOnly;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 100;
    SPI_InitStructure.SPI_NDF =  TEST_SIZE;
    SPI_InitStructure.SPI_RxWaterlevel       = 1;

    SPI_Init(SPI_MST, &SPI_InitStructure);
    SPI_Cmd(SPI_MST, ENABLE);
}

static void spi_master_read_dma_init(void)
{
    for (uint16_t i = 0; i < TEST_SIZE; i++)
    {
        read_buf[i] = 0;
    }
    if (!GDMA_channel_request(&spi_master_rx_dma_ch_num, spi_master_rx_dma_handler, true))
    {
        return;
    }
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    /*--------------DMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = SPI_MASTER_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_PeripheralToMemory;
    GDMA_InitStruct.GDMA_BufferSize          = TEST_SIZE;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)SPI_MST->DR;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)read_buf;
    GDMA_InitStruct.GDMA_SourceHandshake     = DMA_Handshake_SPI_MST_RX;

    GDMA_Init(SPI_MASTER_RX_DMA_CHANNEL, &GDMA_InitStruct);
    GDMA_INTConfig(SPI_MASTER_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------DMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_MASTER_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

static void spi_dma_rx_data(uint8_t *p_recv_buf, uint16_t len)
{
    uint8_t send_buf[4] = {1, 2, 3, 4};

    /* Configure length of data which want to read */
    SPI_SetReadLen(SPI_MST, len);
    /* Configure GDMA transmission size */
    GDMA_SetBufferSize(SPI_MASTER_RX_DMA_CHANNEL, len);
    /* Configure GDMA destination address */
    GDMA_SetDestinationAddress(SPI_MASTER_RX_DMA_CHANNEL, (uint32_t)p_recv_buf);
    /* Enable SPI GDMA Rx Function */
    SPI_GDMACmd(SPI_MST, SPI_GDMAReq_Rx, DISABLE);
    SPI_GDMACmd(SPI_MST, SPI_GDMAReq_Rx, ENABLE);
    /* Enable GDMA to transmission */
    GDMA_Cmd(SPI_MASTER_RX_DMA_CHANNEL_NUM, ENABLE);

    /* When the transfer mode is receive only a serial transfer is started
    by writing one dummy data word into the transmit FIFO when a serial slave is selected.*/
    SPI_SendBuffer(SPI_MST, send_buf, 1);
}

void spi_master_rx_only_mode_demo(void)
{
    IO_PRINT_INFO0("spi_master_rx_only_mode_demo");

    board_spi_init();
    driver_master_spi_init();
    spi_master_read_dma_init();
    /*-----------------start to read data-----------*/
    spi_dma_rx_data(read_buf, TEST_SIZE);
}

static void spi_master_rx_dma_handler(void)
{
    IO_PRINT_INFO0("spi_master_rx_dma_handler");
    GDMA_ClearINTPendingBit(SPI_MASTER_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);

    /* It is recommended to post the os msg to the task thread for data processing. */
    for (uint16_t idx = 0; idx < TEST_SIZE; idx++)
    {
        IO_PRINT_INFO2("spi_master_rx_dma_handler: DMA master read_buf[%d] 0x%x", idx, read_buf[idx]);
    }
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
