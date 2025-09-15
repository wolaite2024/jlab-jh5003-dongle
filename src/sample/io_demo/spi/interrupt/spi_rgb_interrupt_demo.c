/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_rgb_interrupt_demo.c
* @brief    This file provides demo code of using SPI as one data line to control LED by interrupt mode.
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
#include "vector_table.h"
#include "trace.h"

/** @defgroup  SPI_RGB_INTERRUPT_DEMO  SPI DEMO
    * @brief  Spi work in master mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Spi_Master_Interrupt_Demo_Exported_Macros Spi Master Interrupt Demo Exported Macros
  * @brief
  * @{
  */
#define PIN_SPI_MOSI               P1_0
#define SPI_MO_MASTER              SPI1_MO_MASTER

#define SPI_MASTER                 SPI1
#define APBPeriph_SPI_CLOCK        APBPeriph_SPI1_CLOCK
#define APBPeriph_SPI              APBPeriph_SPI1

#define SPI_VECTORn                SPI1_VECTORn
#define SPI_IRQn                   SPI1_IRQn

#define RGB_DATA_LEN                (24)  //R,G,B total 24 bits

static uint16_t SPI_WriteBuf[RGB_DATA_LEN] = {0};

/** @} */ /* End of group Spi_Master_Interrupt_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Spi_Master_Interrupt_Demo_Exported_Functions Spi Master Interrupt Demo Exported Functions
  * @brief
  * @{
  */
static void spi_tx_handler(void);

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
    SPI_InitStructure.SPI_TxThresholdLevel  = 0;

    SPI_Init(SPI_MASTER, &SPI_InitStructure);
    SPI_Cmd(SPI_MASTER, ENABLE);

    RamVectorTableUpdate(SPI_VECTORn, (IRQ_Fun)spi_tx_handler);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = SPI_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
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
  * @brief  demo code of operation about SPI RGB INTERRUPT.
  * @param   No parameter.
  * @return  void
  */
void spi_rgb_interrupt_demo(void)
{
    IO_PRINT_INFO0("spi_rgb_interrupt_demo");

    uint32_t rgb_data = 0xa5a5a5; //24bits RGB sample

    led_control_rgb_to_spi_buffer(rgb_data);

    board_spi_init();

    driver_spi_init();

    /*---------------send demo buffer--------------*/
    SPI_SendHalfWord(SPI_MASTER, SPI_WriteBuf, RGB_DATA_LEN);
    SPI_INTConfig(SPI_MASTER, SPI_INT_TXE, ENABLE);
}

static void spi_tx_handler(void)
{
    if (SPI_GetINTStatus(SPI_MASTER, SPI_INT_TXE) == SET)
    {
        SPI_INTConfig(SPI_MASTER, SPI_INT_TXE, DISABLE);
        IO_PRINT_INFO0("spi_tx_handler: SPI TX FIFO Empty");
        /* It is recommended to post the os msg to the task thread for data processing. */
    }
}

/** @} */ /* End of group Spi_Master_Interrupt_Demo_Exported_Functions */
/** @} */ /* End of group SPI_RGB_INTERRUPT_DEMO */

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
