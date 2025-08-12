/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_rgb_polling_demo.c
* @brief    This file provides demo code of using SPI as one data line to control LED by polling mode.
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
#include "trace.h"

/** @defgroup  SPI_POLLING_DEMO  SPI DEMO
    * @brief  Spi work in master mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Spi_Master_Polling_Demo_Exported_Macros Spi Master Polling Demo Exported Macros
  * @brief
  * @{
  */

#define PIN_SPI1_MOSI               P1_0
#define RGB_DATA_LEN                (24)  //R,G,B total 24 bits
#define RESET_LEN                   (67)  //reset time = 12bits * 0.1us * 67 = 80.4us
#define DATA_LEN                    (RGB_DATA_LEN + RESET_LEN)

uint16_t SPI_WriteBuf[DATA_LEN] = {0};

/** @} */ /* End of group Spi_Master_Polling_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Spi_Master_Polling_Demo_Exported_Functions Spi Master Polling Demo Exported Functions
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
    Pinmux_Config(PIN_SPI1_MOSI, SPI1_MO_MASTER);

    Pad_Config(PIN_SPI1_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
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
    RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_12b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 4; //spi clock = 10MHz
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);

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
        if (data & BIT(RGB_DATA_LEN - i - 1)) //1bit
        {
            SPI_WriteBuf[i] = 0xFF8;
        }
        else
        {
            SPI_WriteBuf[i] = 0xE00;
        }
    }
}

/**
  * @brief  demo code of operation about SPI.
  * @param   No parameter.
  * @return  void
  */
void spi_rgb_polling_demo(void)
{
    IO_PRINT_INFO0("spi_rgb_polling_demo");

    uint32_t rgb_data = 0xa5a5a5; //24bits RGB sample
    led_control_rgb_to_spi_buffer(rgb_data);

    /* Configure PAD and pinmux firstly! */
    board_spi_init();

    /* Initialize SPI peripheral */
    driver_spi_init();

    /* Send out the control format. If timing required, please add critical sections */
    SPI_SendHalfWord(SPI1, SPI_WriteBuf, DATA_LEN);

    /* Polling mode. Interrupt or DMA mode is also supported */
    while (SPI_GetFlagState(SPI1, SPI_FLAG_BUSY));

}

/** @} */ /* End of group Spi_Master_Polling_Demo_Exported_Functions */
/** @} */ /* End of group SPI_POLLING_DEMO */
