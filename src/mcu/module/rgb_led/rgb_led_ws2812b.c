/*
 * Copyright (c) 2023, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdlib.h>
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "dma_channel.h"
#include "rgb_led.h"
#include "trace.h"

#define PIN_SPI_MOSI               P1_0
#define SPI_MO_MASTER              SPI1_MO_MASTER

#define SPI_MASTER                 SPI1
#define APBPeriph_SPI_CLOCK        APBPeriph_SPI1_CLOCK
#define APBPeriph_SPI              APBPeriph_SPI1

#define SPI_DMA_CHANNEL_NUM        spi_dma_ch_num
#define SPI_DMA_CHANNEL            DMA_CH_BASE(spi_dma_ch_num)
#define SPI_DMA_IRQ                DMA_CH_IRQ(spi_dma_ch_num)
#define GDMA_Handshake_SPI_TX      GDMA_Handshake_SPI1_TX

#define RGB_DATA_LEN                (24) //R,G,B total 24 bits

static uint8_t spi_dma_ch_num = 0xa5;
static uint16_t *SPI_WriteBuf = NULL;
// static uint8_t lap[9] = {60, 48, 40, 32, 24, 16, 12, 8, 1}; //number of LEDs per circle

P_RGB_LED_CB user_rgb_cb = NULL;

static void spi_tx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SPI_DMA_CHANNEL_NUM, GDMA_INT_Transfer);

    if (user_rgb_cb != NULL)
    {
        user_rgb_cb();
    }
}

static void board_spi_init(void)
{
    Pinmux_Config(PIN_SPI_MOSI, SPI_MO_MASTER);
    Pad_Config(PIN_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
}

static void driver_spi_init(void)
{
    /* turn on SPI clock */
    RCC_PeriphClockCmd(APBPeriph_SPI, APBPeriph_SPI_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_12b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 4;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_InitStructure.SPI_TxWaterlevel = 46;

    SPI_Init(SPI_MASTER, &SPI_InitStructure);
    SPI_Cmd(SPI_MASTER, ENABLE);
}

static void spi_tx_dma_init(void)
{
    if ((spi_dma_ch_num == 0xa5) &&
        (!GDMA_channel_request(&spi_dma_ch_num, spi_tx_dma_handler, true)))
    {
        APP_PRINT_ERROR0("spi_tx_dma_init: GDMA_channel_request failed");
        return;
    }

    /* turn on GDMA clock */
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    /*---------------------GDMA init----------------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_InitStruct.GDMA_ChannelNum          = SPI_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_16;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_16;

    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)SPI_WriteBuf;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)SPI_MASTER->DR;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_SPI_TX;

    GDMA_Init(SPI_DMA_CHANNEL, &GDMA_InitStruct);

    GDMA_INTConfig(SPI_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------DMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

static void spi_tx_dma(uint16_t *buf, uint32_t len)
{
    GDMA_SetSourceAddress(SPI_DMA_CHANNEL, (uint32_t)buf);
    GDMA_SetBufferSize(SPI_DMA_CHANNEL, len);

    SPI_GDMACmd(SPI_MASTER, SPI_GDMAReq_Tx, DISABLE);
    SPI_GDMACmd(SPI_MASTER, SPI_GDMAReq_Tx, ENABLE);
    GDMA_Cmd(SPI_DMA_CHANNEL_NUM, ENABLE);
}

void rgb_led_data_send(uint32_t *buf, uint32_t rgb_data_num)
{
    for (uint32_t i = 0; i < rgb_data_num; i++)
    {
        for (uint8_t j = 0; j < RGB_DATA_LEN; j++)
        {
            if (buf[i] & BIT(RGB_DATA_LEN - j - 1))
            {
                SPI_WriteBuf[j + i * RGB_DATA_LEN] = 0xFF8; //HIGH
            }
            else
            {
                SPI_WriteBuf[j + i * RGB_DATA_LEN] = 0xE00; //LOW
            }
        }
    }

    spi_tx_dma(SPI_WriteBuf, RGB_DATA_LEN * rgb_data_num);
}

void rgb_led_init(uint32_t rgb_data_num, P_RGB_LED_CB user_rgb_cb)
{
    uint32_t data_len = RGB_DATA_LEN * rgb_data_num;

    if (SPI_WriteBuf == NULL)
    {
        SPI_WriteBuf = (uint16_t *)malloc(data_len * 2);
        if (SPI_WriteBuf == NULL)
        {
            APP_PRINT_ERROR0("rgb_led_init: SPI_WriteBuf malloc fail!");
            return ;
        }
    }

    board_spi_init();

    driver_spi_init();

    spi_tx_dma_init();
}
