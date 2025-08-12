/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     i2c_dma_demo.c
* @brief    This file provides demo code of i2c slave tx gdma + master rx gdma.
* @details
* @author   renee
* @date     2017-01-23
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "platform_utils.h"
#include "dma_channel.h"
#include "vector_table.h"
#include "rtl876x_gdma.h"
#include "rtl876x_i2c.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "trace.h"

/** @defgroup  I2C_GDMA_DEMO I2C GDMA Demo
    * @brief  I2C slave send data by gdma, and I2C master receive data by gdma implmentaion
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup I2C_GDMA_Exported_Macros I2C data transfered by gdma Macros
    * @brief
    * @{
    */

#define I2C0_SDA                P1_0
#define I2C0_SCL                P1_1
#define I2C1_SDA                P0_0
#define I2C1_SCL                P0_1

#define TEST_SIZE               255

/** @} */ /* End of group I2C_GDMA_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup I2C_GDMA_Exported_Variables I2C data transfered by gdma Variables
    * @brief
    * @{
    */

static uint16_t sendbuf[TEST_SIZE];
static uint8_t readbuf[TEST_SIZE];
static uint8_t i2c_tx_dma_ch_num = 0xa5;
static uint8_t i2c_rx_dma_ch_num = 0xa5;

#define I2C_TX_DMA_CHANNEL_NUM     i2c_tx_dma_ch_num
#define I2C_TX_DMA_CHANNEL         DMA_CH_BASE(i2c_tx_dma_ch_num)
#define I2C_TX_DMA_IRQ             DMA_CH_IRQ(i2c_tx_dma_ch_num)

#define I2C_RX_DMA_CHANNEL_NUM     i2c_rx_dma_ch_num
#define I2C_RX_DMA_CHANNEL         DMA_CH_BASE(i2c_rx_dma_ch_num)
#define I2C_RX_DMA_IRQ             DMA_CH_IRQ(i2c_rx_dma_ch_num)

/** @} */ /* End of group I2C_GDMA_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup I2C_GDMA_Exported_Functions I2C data transfered by gdma Functions
    * @brief
    * @{
    */
static void i2c_tx_dma_handler(void);
static void i2c_rx_dma_handler(void);
static void i2c0_handler(void);
static void i2c1_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_dma_i2c_init(void)
{
    Pad_Config(I2C0_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(I2C0_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(I2C1_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(I2C1_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pinmux_Config(I2C0_SDA, I2C0_DAT);
    Pinmux_Config(I2C0_SCL, I2C0_CLK);
    Pinmux_Config(I2C1_SDA, I2C1_DAT);
    Pinmux_Config(I2C1_SCL, I2C1_CLK);
}

/**
  * @brief  Initialize I2C0 peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_i2c0_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);
    I2C_InitTypeDef  I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_DeviveMode = I2C_DeviveMode_Slave;
    I2C_InitStructure.I2C_AddressMode = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_SlaveAddress = 0x50;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_TxDmaEn           = ENABLE;
    I2C_InitStructure.I2C_TxWaterlevel      = 16;
    I2C_Init(I2C0, &I2C_InitStructure);

    RamVectorTableUpdate(I2C0_VECTORn, i2c0_handler);

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = I2C0_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    /* configure interrupt type, please reference to I2C document about All interrupt type description */
    I2C_INTConfig(I2C0, I2C_INT_RD_REQ | I2C_INT_RX_FULL, ENABLE);
    I2C_Cmd(I2C0, ENABLE);
}

/**
  * @brief  Initialize I2C1 peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_i2c1_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_I2C1, APBPeriph_I2C1_CLOCK, ENABLE);
    I2C_InitTypeDef  I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_DeviveMode = I2C_DeviveMode_Master;
    I2C_InitStructure.I2C_AddressMode = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_SlaveAddress = 0x50;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_RxDmaEn           = ENABLE;
    I2C_InitStructure.I2C_RxWaterlevel      = 3;
    I2C_Init(I2C1, &I2C_InitStructure);

    RamVectorTableUpdate(I2C1_VECTORn, i2c1_handler);

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = I2C1_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    I2C_INTConfig(I2C1, I2C_INT_RX_UNDER | I2C_INT_STOP_DET, ENABLE);

    I2C_Cmd(I2C1, ENABLE);
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
static void i2c_send_dma_init(void)
{
    if (!GDMA_channel_request(&i2c_tx_dma_ch_num, i2c_tx_dma_handler, true))
    {
        return;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    /*--------------GDMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = I2C_TX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToPeripheral;//GDMA_DIR_MemoryToPeripheral
    GDMA_InitStruct.GDMA_BufferSize      = TEST_SIZE;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)sendbuf;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(&(I2C0->IC_DATA_CMD));
    GDMA_InitStruct.GDMA_DestHandshake = GDMA_Handshake_I2C0_TX;

    GDMA_Init(I2C_TX_DMA_CHANNEL, &GDMA_InitStruct);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = I2C_TX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_INTConfig(I2C_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
static void i2c_recv_bydma(void)
{
    if (!GDMA_channel_request(&i2c_rx_dma_ch_num, i2c_rx_dma_handler, true))
    {
        return;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;
    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = I2C_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;//GDMA_DIR_PeripheralToMemory
    GDMA_InitStruct.GDMA_BufferSize      = TEST_SIZE;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t) & (I2C1->IC_DATA_CMD);
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(readbuf);
    GDMA_InitStruct.GDMA_SourceHandshake = GDMA_Handshake_I2C1_RX;

    GDMA_Init(I2C_RX_DMA_CHANNEL, &GDMA_InitStruct);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = I2C_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_INTConfig(I2C_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------start to receive data-----------*/
    GDMA_Cmd(I2C_RX_DMA_CHANNEL_NUM, ENABLE);
}

static void i2c_test_code(void)
{
    for (uint16_t i = 0; i < TEST_SIZE; i++)
    {
        sendbuf[i] = i;
    }

    for (uint16_t i = 0; i < TEST_SIZE; i++)
    {
        readbuf[i] = 0;
    }

    /*send read command*/
    uint8_t cnt = 0;
    for (cnt = 0; cnt < TEST_SIZE; cnt++)
    {
        if (cnt >= TEST_SIZE - 1)
        {
            /* generate stop singal */
            I2C_SendCmd(I2C1, I2C_READ_CMD, 0, I2C_STOP_ENABLE);
        }
        else
        {
            I2C_SendCmd(I2C1, I2C_READ_CMD, 0, I2C_STOP_DISABLE);
        }
        while (I2C_GetFlagState(I2C1, I2C_FLAG_TFNF) == RESET);
    }
}

/**
  * @brief  demo code of operation about i2c + gdma.
  * @param   No parameter.
  * @return  void
  */
void i2c_dma_demo(void)
{
    board_dma_i2c_init();
    driver_i2c0_init();
    i2c_send_dma_init();
    driver_i2c1_init();
    i2c_recv_bydma();
    i2c_test_code();
}

/**
* @brief  I2C0 interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void i2c0_handler(void)
{
    if (I2C_GetINTStatus(I2C0, I2C_INT_RD_REQ) == SET)
    {
        GDMA_Cmd(I2C_TX_DMA_CHANNEL_NUM, ENABLE);
        //Add user code here
        IO_PRINT_INFO0("i2c0_handler: I2C_INT_RD_REQ");
        // clear interrupt
        I2C_ClearINTPendingBit(I2C0, I2C_INT_RD_REQ);
        I2C_INTConfig(I2C0, I2C_INT_RD_REQ, DISABLE);
    }

    if (I2C_GetINTStatus(I2C0, I2C_INT_RX_FULL) == SET)
    {
        // add user code here
        IO_PRINT_INFO0("i2c0_handler: I2C_INT_RX_FULL");
        I2C_INTConfig(I2C0, I2C_INT_RX_FULL, DISABLE);
        // clear interrupt,read for clear
        I2C_ClearINTPendingBit(I2C0, I2C_INT_RX_FULL);
    }
}

/**
* @brief  I2C1 interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void i2c1_handler(void)
{
    if (I2C_GetINTStatus(I2C1, I2C_INT_RX_UNDER) == SET)
    {
        // add user code here
        IO_PRINT_ERROR0("i2c1_handler: I2C_INT_RX_UNDER");
        // clear interrupt
        I2C_ClearINTPendingBit(I2C1, I2C_INT_RX_UNDER);
    }

    if (I2C_GetINTStatus(I2C1, I2C_INT_STOP_DET) == SET)
    {
        // add user code here
        IO_PRINT_INFO0("i2c1_handler: I2C1 stop detect");
        // clear interrupt
        I2C_ClearINTPendingBit(I2C1, I2C_INT_STOP_DET);
    }
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void i2c_tx_dma_handler(void)
{
    IO_PRINT_TRACE0("i2c_tx_dma_handler");
    GDMA_INTConfig(I2C_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, DISABLE);
    GDMA_ClearINTPendingBit(I2C_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    GDMA_channel_release(I2C_TX_DMA_CHANNEL_NUM);
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void i2c_rx_dma_handler(void)
{
    IO_PRINT_TRACE0("i2c_rx_dma_handler");
    GDMA_INTConfig(I2C_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, DISABLE);
    GDMA_ClearINTPendingBit(I2C_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    GDMA_channel_release(I2C_RX_DMA_CHANNEL_NUM);
}

/** @} */ /* End of group I2C_GDMA_Exported_Functions */
/** @} */ /* End of group I2C_GDMA_DEMO */

