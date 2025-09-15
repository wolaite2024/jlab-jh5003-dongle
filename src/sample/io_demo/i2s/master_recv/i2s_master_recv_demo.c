/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     i2s_master_recv_demo.c
* @brief    This file provides demo code of I2S master receive data.
* @details
* @author   mh_chang
* @date     2022-10-25
* @version  v1.0
*********************************************************************************************************
*/

#include "rtl876x.h"
#include "rtl876x_i2s.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "trace.h"
#include "vector_table.h"

#define I2S_LRCK_PIN                    P0_0
#define I2S_BCLK_PIN                    P0_1
#define I2S_SDI_PIN                     P0_3

#define I2S_NUM                         I2S1
#define I2S_LRCK_PINMUX                 LRC_SPORT1
#define I2S_BCLK_PINMUX                 BCLK_SPORT1
#define I2S_SDI_PINMUX                  ADCDAT_SPORT1

#define I2S_RX_IRQ                      SPORT1_RX_IRQn
#define I2S_RX_VECTOR                   SPORT1_RX_VECTORn

static void I2S_RX_Handler(void);

/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param  No parameter.
  * @return Void
  */
static void board_i2s_init(void)
{
    /* set PAD_SW_MODE & PAD_PULL_DOWN when I2S disable to prevent PAD floating */
    Pad_Config(I2S_LRCK_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(I2S_BCLK_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(I2S_SDI_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    Pinmux_Config(I2S_BCLK_PIN, I2S_BCLK_PINMUX);
    Pinmux_Config(I2S_LRCK_PIN, I2S_LRCK_PINMUX);
    Pinmux_Config(I2S_SDI_PIN, I2S_SDI_PINMUX);
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
    I2S_InitStruct.I2S_RxChannelType    = I2S_Channel_Stereo;
    I2S_InitStruct.I2S_RxDataWidth      = I2S_Data_Width_24Bits;
    I2S_InitStruct.I2S_RxDataFormat     = I2S_Mode;
    I2S_InitStruct.I2S_DMACmd           = I2S_DMA_DISABLE;
    I2S_Init(I2S_NUM, &I2S_InitStruct);

    RamVectorTableUpdate(I2S_RX_VECTOR, (IRQ_Fun)I2S_RX_Handler);
    I2S_INTConfig(I2S_NUM, I2S_MCU_INT_RX_READY, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = I2S_RX_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 5;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    I2S_Cmd(I2S_NUM, I2S_MODE_RX, ENABLE);
}

/**
  * @brief  Demo code of I2S communication.
  * @param  No parameter.
  * @return Void
*/
void i2s_master_recv_demo(void)
{
    board_i2s_init();
    driver_i2s_init();
}

/**
  * @brief  I2S_RX_Handler.
  * @param  No parameter.
  * @return void
*/
static void I2S_RX_Handler(void)
{
    uint32_t data;

    if (I2S_GetINTStatus(I2S_NUM, I2S_MCU_INT_RX_READY))
    {
        uint8_t len = I2S_GetRxFIFOLen(I2S_NUM);

        for (uint8_t i = 0; i < len; i++)
        {
            data = I2S_ReceiveData(I2S_NUM);
            IO_PRINT_TRACE2("I2S_RX_Handler: i %d, data 0x%x", i, data);
            /* do something */
        }

        I2S_ClearINTPendingBit(I2S_NUM, I2S_CLEAR_INT_RX_READY);
    }
}

/******************* (C) COPYRIGHT 2022 Realtek Semiconductor Corporation *****END OF FILE****/

