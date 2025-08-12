/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_rgb_dma_demo.c
* @brief    This file provides demo code of using SPI as one data line to control LED by dma mode.
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

/** @defgroup  SPI_RGB_DMA_DEMO  SPI DEMO
    * @brief  Spi work in master mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Spi_Master_Dma_Demo_Exported_Macros Spi Master Dma Demo Exported Macros
  * @brief
  * @{
  */
#define PIN_SPI_MOSI               P1_0
#define SPI_MO_MASTER              SPI1_MO_MASTER

#define SPI_MASTER                 SPI1
#define APBPeriph_SPI_CLOCK        APBPeriph_SPI1_CLOCK
#define APBPeriph_SPI              APBPeriph_SPI1

#define SPI_DMA_CHANNEL_NUM        spi_dma_ch_num
#define SPI_DMA_CHANNEL            DMA_CH_BASE(spi_dma_ch_num)
#define SPI_DMA_IRQ                DMA_CH_IRQ(spi_dma_ch_num)
#define GDMA_Handshake_SPI_TX      GDMA_Handshake_SPI1_TX

#define RGB_DATA_LEN                (24)  //R,G,B total 24 bits

static uint8_t spi_dma_ch_num = 0xa5;
static uint16_t SPI_WriteBuf[RGB_DATA_LEN] = {0};

/** @} */ /* End of group Spi_Master_Dma_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Spi_Master_Dma_Demo_Exported_Functions Spi Master Dma Demo Exported Functions
  * @brief
  * @{
  */
static void spi_tx_dma_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_spi_init(void)
{
    Pinmux_Config(PIN_SPI_MOSI, SPI_MO_MASTER);
    Pad_Config(PIN_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
}

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
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

/**
  * @brief  Initialize DMA peripheral.
  * @param   No parameter.
  * @return  void
  */
static void spi_tx_dma_init(void)
{
    if (!GDMA_channel_request(&spi_dma_ch_num, spi_tx_dma_handler, true))
    {
        IO_PRINT_ERROR0("spi_tx_dma_init: GDMA_channel_request failed");
        return;
    }

    /* turn on GDMA clock */
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    /*---------------------GDMA init----------------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_InitStruct.GDMA_ChannelNum          = SPI_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize          = RGB_DATA_LEN;
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

/**
  * @brief  SPI master send data.
  * @param   No parameter.
  * @return  void
  */
static void spi_tx_dma(uint16_t *buf, uint8_t len)
{
    GDMA_SetSourceAddress(SPI_DMA_CHANNEL, (uint32_t)buf);
    GDMA_SetBufferSize(SPI_DMA_CHANNEL, len);

    SPI_GDMACmd(SPI_MASTER, SPI_GDMAReq_Tx, DISABLE);
    SPI_GDMACmd(SPI_MASTER, SPI_GDMAReq_Tx, ENABLE);
    GDMA_Cmd(SPI_DMA_CHANNEL_NUM, ENABLE);
}

/**
  * @brief  write send buffer according to RGB data.
  * @param   No parameter.
  * @return  void
  */
static void led_control_rgb_to_spi_buffer(uint32_t data)
{
    for (uint32_t i = 0; i < RGB_DATA_LEN; i++)
    {
        if (data & BIT(RGB_DATA_LEN - i - 1))
        {
            SPI_WriteBuf[i] = 0xFF8; //HIGH
        }
        else
        {
            SPI_WriteBuf[i] = 0xE00; //LOW
        }
    }
}

/**
  * @brief  demo code of operation about SPI RGB DMA.
  * @param   No parameter.
  * @return  void
  */
void spi_rgb_dma_demo(void)
{
    IO_PRINT_INFO0("spi_rgb_dma_demo");

    uint32_t rgb_data = 0xa5a5a5; //24bits RGB sample

    led_control_rgb_to_spi_buffer(rgb_data);

    board_spi_init();

    driver_spi_init();

    spi_tx_dma_init();

    spi_tx_dma(SPI_WriteBuf, RGB_DATA_LEN);
}

static void spi_tx_dma_handler(void)
{
    IO_PRINT_INFO0("spi_tx_dma_handler");
    GDMA_INTConfig(SPI_DMA_CHANNEL_NUM, GDMA_INT_Transfer, DISABLE);
    GDMA_ClearINTPendingBit(SPI_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
}

/** @} */ /* End of group Spi_Master_Dma_Demo_Exported_Functions */
/** @} */ /* End of group SPI_RGB_DMA_DEMO */

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
