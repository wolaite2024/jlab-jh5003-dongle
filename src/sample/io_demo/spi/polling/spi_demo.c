/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_demo.c
* @brief    This file provides demo code of SPI comunication.
* @details
* @author   elliot chen
* @date     2015-06-04
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

#define PIN_SPI1_SCK                P1_2
#define PIN_SPI1_MOSI               P1_3
#define PIN_SPI1_MISO               P1_4
#define PIN_SPI1_CS                 P1_5

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
    Pinmux_Config(PIN_SPI1_SCK, SPI1_CLK_MASTER);
    Pinmux_Config(PIN_SPI1_MOSI, SPI1_MO_MASTER);
    Pinmux_Config(PIN_SPI1_MISO, SPI1_MI_MASTER);
    Pinmux_Config(PIN_SPI1_CS, SPI1_SS_N_0_MASTER);

    Pad_Config(PIN_SPI1_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI1_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI1_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI1_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);

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
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 100;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);

}

/**
  * @brief  demo code of operation about ADC.
  * @param   No parameter.
  * @return  void
  */
void spi_demo(void)
{
    uint8_t SPI_WriteBuf[16] = {0, 0x01, 0x02, 0x00};
    uint8_t SPI_ReadBuf[16] = {0, 0, 0, 0};
    uint8_t idx = 0;
    uint8_t len = 0;


    /* Configure PAD and pinmux firstly! */
    board_spi_init();

    /* Initialize ADC peripheral */
    driver_spi_init();

    /*---------------read flash ID--------------*/
    SPI_WriteBuf[0] = 0x9f;
    SPI_SendBuffer(SPI1, SPI_WriteBuf, 4);

    /*Waiting for SPI data transfer to end*/
    while (SPI_GetFlagState(SPI1, SPI_FLAG_BUSY));

    /*read ID number of flash GD25Q20*/
    len = SPI_GetRxFIFOLen(SPI1);
    for (idx = 0; idx < len; idx++)
    {
        SPI_ReadBuf[idx] = SPI_ReceiveData(SPI1);
        IO_PRINT_INFO2("spi_demo: SPI_ReadBuf[%d] 0x%x", idx, SPI_ReadBuf[idx]);
    }
}

/** @} */ /* End of group Spi_Master_Polling_Demo_Exported_Functions */
/** @} */ /* End of group SPI_POLLING_DEMO */


