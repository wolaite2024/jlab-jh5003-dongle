/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     i2s_master_send_dma_demo.c
* @brief    This file provides demo code of I2S master send data via DMA.
* @details
* @author   mh_chang
* @date     2022-10-25
* @version  v1.0
*********************************************************************************************************
*/

#include "rtl876x_i2s.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "vector_table.h"
#include "dma_channel.h"
#include "trace.h"

#define I2S_LRCK_PIN                    P0_0
#define I2S_BCLK_PIN                    P0_1
#define I2S_SDO_PIN                     P0_2

#define I2S_NUM                         I2S0
#define I2S_LRCK_PINMUX                 LRC_SPORT0
#define I2S_BCLK_PINMUX                 BCLK_SPORT0
#define I2S_SDO_PINMUX                  DACDAT_SPORT0

#define I2S_TX_DMA_CHANNEL_NUM          i2s_tx_dma_ch_num
#define I2S_TX_DMA_CHANNEL              DMA_CH_BASE(i2s_tx_dma_ch_num)
#define I2S_TX_DMA_VECTOR               DMA_CH_VECTOR(i2s_tx_dma_ch_num)
#define I2S_TX_DMA_IRQ                  DMA_CH_IRQ(i2s_tx_dma_ch_num)
#define I2S_DMA_HANDSHAKE               GDMA_Handshake_SPORT0_TX
#define I2S_TX_ADDR                     I2S0_TX_ADDR
#define I2S_DMA_BUF_LEN                 16

static uint8_t i2s_tx_dma_ch_num = 0xa5;
static int32_t i2s_send_buff[I2S_DMA_BUF_LEN];

/**
  * @brief  I2S TX DMA interrupt handler function.
  * @param  No parameter.
  * @return Void
  */
static void i2s_tx_dma_handler(void)
{
    GDMA_ClearAllTypeINT(I2S_TX_DMA_CHANNEL_NUM);

    GDMA_Cmd(I2S_TX_DMA_CHANNEL_NUM, ENABLE);
}

/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param  No parameter.
  * @return Void
  */
static void board_i2s_init(void)
{
    /* set PAD_SW_MODE & PAD_PULL_DOWN when I2S disable to prevent PAD floating */
    Pad_Config(I2S_BCLK_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(I2S_LRCK_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(I2S_SDO_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    Pinmux_Config(I2S_BCLK_PIN, I2S_BCLK_PINMUX);
    Pinmux_Config(I2S_LRCK_PIN, I2S_LRCK_PINMUX);
    Pinmux_Config(I2S_SDO_PIN, I2S_SDO_PINMUX);
}

/**
  * @brief  Initialize I2S peripheral.
  * @param  No parameter.
  * @return Void
  */
static void driver_i2s_init(void)
{
    I2S_InitTypeDef I2S_InitStruct;

    I2S_StructInit(&I2S_InitStruct);
    I2S_InitStruct.I2S_ClockSource      = I2S_CLK_XTAL;
    /* BCLK = 40MHz * (I2S_BClockNi / I2S_BClockMi), LRCK = BCLK / (I2S_BClockDiv + 1) */
    I2S_InitStruct.I2S_BClockMi         = 0x271;
    I2S_InitStruct.I2S_BClockNi         = 0x30;     /* BCLK = 3.072MHz */
    I2S_InitStruct.I2S_BClockDiv        = 0x3F;     /* LRCK = 48KHz */
    I2S_InitStruct.I2S_DeviceMode       = I2S_DeviceMode_Master;
    I2S_InitStruct.I2S_TxChannelType    = I2S_Channel_Mono;
    I2S_InitStruct.I2S_TxDataWidth      = I2S_Width_24Bits;
    I2S_InitStruct.I2S_TxDataFormat     = I2S_Mode;
    I2S_InitStruct.I2S_DMACmd           = I2S_DMA_ENABLE;
    I2S_Init(I2S_NUM, &I2S_InitStruct);

    /* data initialization for 24-bits format */
    for (uint32_t i = 0; i < I2S_DMA_BUF_LEN; i++)
    {
        i2s_send_buff[i] = (0x123400 + i) << 8;
    }
}

static void driver_gdma_init(void)
{
    GDMA_InitTypeDef gdma_init;
    NVIC_InitTypeDef nvic_init;

    if ((i2s_tx_dma_ch_num == 0xA5) &&
        (!GDMA_channel_request(&i2s_tx_dma_ch_num, NULL, false)))
    {
        IO_PRINT_ERROR0("driver_gdma_init: i2s tx dma request fail");

        return;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    GDMA_StructInit(&gdma_init);
    gdma_init.GDMA_BufferSize           = I2S_DMA_BUF_LEN;
    gdma_init.GDMA_SourceDataSize       = GDMA_DataSize_Word;
    gdma_init.GDMA_DestinationDataSize  = GDMA_DataSize_Word;
    gdma_init.GDMA_SourceMsize          = GDMA_Msize_4;
    gdma_init.GDMA_DestinationMsize     = GDMA_Msize_4;
    gdma_init.GDMA_ChannelNum           = I2S_TX_DMA_CHANNEL_NUM;
    gdma_init.GDMA_DIR                  = GDMA_DIR_MemoryToPeripheral;
    gdma_init.GDMA_SourceAddr           = (uint32_t)(i2s_send_buff);
    gdma_init.GDMA_DestinationAddr      = (uint32_t)I2S_TX_ADDR;
    gdma_init.GDMA_SourceInc            = DMA_SourceInc_Inc;
    gdma_init.GDMA_DestinationInc       = DMA_DestinationInc_Fix;
    gdma_init.GDMA_DestHandshake        = I2S_DMA_HANDSHAKE;
    GDMA_Init(I2S_TX_DMA_CHANNEL, &gdma_init);

    RamVectorTableUpdate(I2S_TX_DMA_VECTOR, i2s_tx_dma_handler);

    nvic_init.NVIC_IRQChannel           = I2S_TX_DMA_IRQ;
    nvic_init.NVIC_IRQChannelCmd        = (FunctionalState)ENABLE;
    nvic_init.NVIC_IRQChannelPriority   = 3;
    NVIC_Init(&nvic_init);
}

/**
  * @brief  Demo code of I2S send data via DMA continuously.
  * @param  No parameter.
  * @return Void
  */
static void i2s_send_data_dma(void)
{
    GDMA_INTConfig(I2S_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    GDMA_Cmd(I2S_TX_DMA_CHANNEL_NUM, ENABLE);
    I2S_Cmd(I2S_NUM, I2S_MODE_TX, ENABLE);
}

/**
  * @brief  Demo code of I2S communication.
  * @param  No parameter.
  * @return Void
*/
void i2s_master_send_dma_demo(void)
{
    board_i2s_init();
    driver_i2s_init();
    driver_gdma_init();
    i2s_send_data_dma();
}
/******************* (C) COPYRIGHT 2022 Realtek Semiconductor Corporation *****END OF FILE****/

