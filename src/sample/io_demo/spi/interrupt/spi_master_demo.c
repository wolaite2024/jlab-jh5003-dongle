/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_master_demo.c
* @brief    This file provides demo code of SPI comunication in interrupt mode.
* @details
* @author   elliot chen
* @date     2015-07-03
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

/** @defgroup  SPI_MASTER_DEMO  SPI MASTER DEMO
    * @brief  Spi work in master mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI_Demo_Exported_Macros SPI Demo Exported Macros
  * @brief
  * @{
  */

#define PIN_SPI1_SCK                P0_0
#define PIN_SPI1_MOSI               P1_0
#define PIN_SPI1_MISO               P1_1
#define PIN_SPI1_CS                 P0_1

/** @} */ /* End of group SPI_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup SPI_Demo_Exported_Functions SPI Demo Exported Functions
  * @brief
  * @{
  */

static void spi1_handler(void);

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

    SPI_InitStructure.SPI_RxThresholdLevel  =
        0;/* cause SPI_INT_RXF interrupt if data length in receive FIFO  >= SPI_RxThresholdLevel + 1*/
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);

    RamVectorTableUpdate(SPI1_VECTORn, spi1_handler);
    /* detect receive data */
    SPI_INTConfig(SPI1, SPI_INT_RXF, ENABLE);
    /* Config SPI interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = SPI1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

}


/**
  * @brief  demo code of operation about SPI.
  * @param   No parameter.
  * @return  void
  */
void spi_master_demo(void)
{
    uint8_t SPI_WriteBuf[30] = {0, 0x01, 0x02, 0x00};

    /* Configure PAD and pinmux firstly! */
    board_spi_init();

    /* Initialize SPI peripheral */
    driver_spi_init();

    /*---------------send demo buffer--------------*/
    SPI_WriteBuf[0] = 0x9f;
    SPI_SendBuffer(SPI1, SPI_WriteBuf, 20);
}

/**
* @brief  SPI interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void spi1_handler(void)
{
    uint8_t len = 0;
    uint8_t idx = 0;
    uint8_t SPI_ReadINTBuf[16] = {0, 0, 0, 0};

    if (SPI_GetINTStatus(SPI1, SPI_INT_RXF) == SET)
    {
        len = SPI_GetRxFIFOLen(SPI1);
        for (idx = 0; idx < len; idx++)
        {
            /* must read all data in receive FIFO , otherwise cause SPI_INT_RXF interrupt again */
            SPI_ReadINTBuf[idx] = SPI_ReceiveData(SPI1);
            IO_PRINT_INFO2("spi1_handler: SPI_ReadINTBuf[%d] 0x%x", idx, SPI_ReadINTBuf[idx]);
        }
    }
}

/** @} */ /* End of group SPI_Demo_Exported_Functions */
/** @} */ /* End of group SPI_MASTER_DEMO */

