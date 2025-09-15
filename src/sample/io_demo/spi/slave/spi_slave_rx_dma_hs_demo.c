/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_slave_rx_dma_hs_demo.c
* @brief    This file provides demo code of SPI salve receive data for high speed by dma mode.
* @details  In fullduplex read mode, spi slave clk can support to 10M.
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

#define SPI_SLAVE_RX_DMA_CHANNEL_NUM     spi_slave_rx_ch_num
#define SPI_SLAVE_RX_DMA_CHANNEL         DMA_CH_BASE(spi_slave_rx_ch_num)
#define SPI_SLAVE_RX_DMA_IRQ             DMA_CH_IRQ(spi_slave_rx_ch_num)
#define SPI_SLAVE_RX_DMA_VECTOR          DMA_CH_VECTOR(spi_slave_rx_ch_num)

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

#define DMA_Handshake_SPI_SLAVE_RX       GDMA_Handshake_SPI0_RX
/*============================================================================*
 *                              Variables
 *============================================================================*/

static uint8_t read_buf[TEST_SIZE];
static uint8_t spi_slave_rx_ch_num = 0x55;

/*============================================================================*
 *                              Functions
 *============================================================================*/

static void spi_slave_rx_dma_handler(void);

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
    RCC_PeriphClockCmd(APBPeriph_SPI, APBPeriph_SPI_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction          = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode               = SPI_Mode_Slave;
    SPI_InitStructure.SPI_DataSize           = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL               = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA               = SPI_CPHA_1Edge;

    SPI_InitStructure.SPI_RxWaterlevel       =
        31; //Adjust waterlevel due to MSIZE, rxwaterlevel = rxmsize - 1

    SPI_Init(SPI_SLAVE, &SPI_InitStructure);
    SPI_Cmd(SPI_SLAVE, ENABLE);
}

static void spi_slave_read_dma_init(void)
{
    for (uint16_t i = 0; i < TEST_SIZE; i++)
    {
        read_buf[i] = 0;
    }
    if (!GDMA_channel_request(&spi_slave_rx_ch_num, spi_slave_rx_dma_handler, true))
    {
        return;
    }
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    /*--------------DMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = SPI_SLAVE_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_PeripheralToMemory;
    GDMA_InitStruct.GDMA_BufferSize          = TEST_SIZE;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)SPI_SLAVE->DR;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)read_buf;
    GDMA_InitStruct.GDMA_SourceHandshake     = DMA_Handshake_SPI_SLAVE_RX;

    GDMA_Init(SPI_SLAVE_RX_DMA_CHANNEL, &GDMA_InitStruct);
    GDMA_INTConfig(SPI_SLAVE_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------DMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_SLAVE_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

static void spi_dma_rx_data(uint8_t *p_recv_buf, uint16_t len)
{
    GDMA_SetBufferSize(SPI_SLAVE_RX_DMA_CHANNEL, len);
    GDMA_SetDestinationAddress(SPI_SLAVE_RX_DMA_CHANNEL, (uint32_t)p_recv_buf);

    /* Enable SPI GDMA Rx Function */
    SPI_GDMACmd(SPI_SLAVE, SPI_GDMAReq_Rx, DISABLE);
    SPI_GDMACmd(SPI_SLAVE, SPI_GDMAReq_Rx, ENABLE);
    GDMA_Cmd(SPI_SLAVE_RX_DMA_CHANNEL_NUM, ENABLE);
}

void spi_slave_rx_dma_hs_demo(void)
{
    IO_PRINT_INFO0("spi_slave_rx_dma_demo");
    uint32_t required_mhz = 0;

    board_spi_init();
    driver_slave_spi_init();
    spi_slave_read_dma_init();

#ifdef TARGET_RTL87X3E
    /* need to adjust slow clk to 2.5MHz when spi master clk > 5MHz */
    if (SPI_MASTER_CLOCK_KHZ > 5000)
    {
        required_mhz = 2;
    }
#endif

    pm_cpu_slow_freq_set(required_mhz);

    /*-----------------start to read data-----------*/
    spi_dma_rx_data(read_buf, TEST_SIZE);
}

static void spi_slave_rx_dma_handler(void)
{
    uint16_t rx_len_all = GDMA_GetTransferLen(SPI_SLAVE_RX_DMA_CHANNEL);
    IO_PRINT_INFO1("spi_slave_rx_dma_handler! rx_len_all %d", rx_len_all);

    GDMA_INTConfig(SPI_SLAVE_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, DISABLE);
    GDMA_ClearINTPendingBit(SPI_SLAVE_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    /* It is recommended to post the os msg to the task thread for data processing. */

    for (uint16_t i = 0; i < TEST_SIZE / 50; i++)
    {
        IO_PRINT_INFO1("spi_slave_rx_dma_handler: read_buf %b", TRACE_BINARY(50, read_buf + 50 * i));
    }
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
