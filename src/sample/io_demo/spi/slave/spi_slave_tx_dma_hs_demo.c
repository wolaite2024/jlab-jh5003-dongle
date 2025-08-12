/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_slave_tx_dma_hs_demo.c
* @brief    This file provides demo code of SPI salve send data for high speed by dma mode.
* @details  The spi slave clk can support to 10M when adjust slow clk.
* @author
* @date     2023-10-31
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
#include "pm.h"

/*============================================================================*
 *                              Macros
 *============================================================================*/

#define TEST_SIZE                        1000
#define SPI_MASTER_CLOCK_KHZ             10000 //10MHz

#define SPI_SLAVE_TX_DMA_CHANNEL_NUM     spi_slave_tx_dma_ch_num
#define SPI_SLAVE_TX_DMA_CHANNEL         DMA_CH_BASE(spi_slave_tx_dma_ch_num)
#define SPI_SLAVE_TX_DMA_IRQ             DMA_CH_IRQ(spi_slave_tx_dma_ch_num)
#define SPI_SLAVE_TX_DMA_VECTOR          DMA_CH_VECTOR(spi_slave_tx_dma_ch_num)

#define PIN_SPI_SCK                      P0_0
#define PIN_SPI_MOSI                     P1_0
#define PIN_SPI_MISO                     P0_1
#define PIN_SPI_CS                       P1_1

#define SPI_CLK_SLAVE                    SPI0_CLK_SLAVE
#define SPI_MO_SLAVE                     SPI0_SI_SLAVE
#define SPI_MI_SLAVE                     SPI0_SO_SLAVE
#define SPI_SS_N_0_SLAVE                 SPI0_SS_N_0_SLAVE

#define SPI_SLAVE                        SPI0
#define SPI_SLAVE_VECTORn                SPI0_VECTORn
#define SPI_SLAVE_IRQn                   SPI0_IRQn
#define APBPeriph_SPI_CLOCK              APBPeriph_SPI0_CLOCK
#define APBPeriph_SPI                    APBPeriph_SPI0

#define GDMA_Handshake_SPI_SLAVE_TX      GDMA_Handshake_SPI0_TX
/*============================================================================*
 *                              Variables
 *============================================================================*/

static uint8_t sendbuf[TEST_SIZE];
static uint8_t spi_slave_tx_dma_ch_num = 0x55;

/*============================================================================*
 *                              Functions
 *============================================================================*/

static void spi_slave_tx_dma_handler(void);
static void spi_slave_handler(void);

static void board_spi_init(void)
{
    Pinmux_Config(PIN_SPI_SCK, SPI_CLK_SLAVE);
    Pinmux_Config(PIN_SPI_MOSI, SPI_MO_SLAVE);
    Pinmux_Config(PIN_SPI_MISO, SPI_MI_SLAVE);
    Pinmux_Config(PIN_SPI_CS, SPI_SS_N_0_SLAVE);

    Pad_Config(PIN_SPI_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

static void driver_slave_spi_init(void)
{
    /* turn on SPI clock */
    RCC_PeriphClockCmd(APBPeriph_SPI, APBPeriph_SPI_CLOCK, ENABLE);

    RamVectorTableUpdate(SPI_SLAVE_VECTORn, (IRQ_Fun)spi_slave_handler);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction         = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Slave;
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_FrameFormat       = SPI_Frame_Motorola;

    SPI_InitStructure.SPI_TxWaterlevel      =
        46; //Adjust waterlevel due to MSIZE, txwaterlevel < tx fifo size - txmsize

    SPI_Init(SPI_SLAVE, &SPI_InitStructure);
    SPI_Cmd(SPI_SLAVE, ENABLE);

    SPI_INTConfig(SPI_SLAVE, SPI_INT_TXO | SPI_INT_TUF, ENABLE);

    /* Config SPI interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel         = SPI_SLAVE_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd      = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

static void spi_slave_send_dma_init(void)
{
    for (uint16_t i = 0; i < TEST_SIZE; i++)
    {
        sendbuf[i] = i & 0xff;
    }
    if (!GDMA_channel_request(&spi_slave_tx_dma_ch_num, spi_slave_tx_dma_handler, true))
    {
        IO_PRINT_ERROR0("spi_slave_send_dma_init: GDMA_channel_request failed");
        return;
    }
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    /*--------------DMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_SLAVE_TX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize      = TEST_SIZE;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_16;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_16;
    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)sendbuf;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)SPI_SLAVE->DR;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_SPI_SLAVE_TX;

    GDMA_Init(SPI_SLAVE_TX_DMA_CHANNEL, &GDMA_InitStruct);

    GDMA_INTConfig(SPI_SLAVE_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------DMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_SLAVE_TX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

void spi_slave_tx_dma_hs_demo(void)
{
    IO_PRINT_INFO0("spi_slave_tx_dma_demo");
    uint32_t required_mhz = 0;

    board_spi_init();
    driver_slave_spi_init();
    spi_slave_send_dma_init();

#ifdef TARGET_RTL87X3E
    /* need to adjust slow clk when spi master clk > 1.25MHz */
    if (SPI_MASTER_CLOCK_KHZ > 5000)
    {
        required_mhz = 4;
    }
    else if (SPI_MASTER_CLOCK_KHZ > 4000)
    {
        required_mhz = 2;
    }
    else if (SPI_MASTER_CLOCK_KHZ > 1250)
    {
        required_mhz = 1;
    }
#else
    /* need to adjust slow clk when spi master clk > 4MHz */
    if (SPI_MASTER_CLOCK_KHZ > 5000)
    {
        required_mhz = 5;
    }
    else if (SPI_MASTER_CLOCK_KHZ > 4000)
    {
        required_mhz = 2;
    }
#endif

    pm_cpu_slow_freq_set(required_mhz);

    /*-----------------start to send data-----------*/
    SPI_GDMACmd(SPI_SLAVE, SPI_GDMAReq_Tx, DISABLE);
    SPI_GDMACmd(SPI_SLAVE, SPI_GDMAReq_Tx, ENABLE);
    GDMA_Cmd(SPI_SLAVE_TX_DMA_CHANNEL_NUM, ENABLE);
}

static void spi_slave_tx_dma_handler(void)
{
    uint16_t tx_len_all = GDMA_GetTransferLen(SPI_SLAVE_TX_DMA_CHANNEL);
    DBG_DIRECT("spi_slave_tx_dma_handler! tx_len_all %d", tx_len_all);

    GDMA_INTConfig(SPI_SLAVE_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, DISABLE);
    GDMA_ClearINTPendingBit(SPI_SLAVE_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
}

static void spi_slave_handler(void)
{
    IO_PRINT_INFO0("spi_slave_handler");

    if (SPI_GetINTStatus(SPI_SLAVE, SPI_INT_TUF) == SET)
    {
        SPI_INTConfig(SPI_SLAVE, SPI_INT_TUF, DISABLE);
        IO_PRINT_INFO0("spi_slave_handler: spi tx fifo underflow!");
    }

    if (SPI_GetINTStatus(SPI_SLAVE, SPI_INT_TXO) == SET)
    {
        SPI_INTConfig(SPI_SLAVE, SPI_INT_TXO, DISABLE);
        IO_PRINT_INFO0("spi_slave_handler: spi tx fifo overflow!");
    }
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
