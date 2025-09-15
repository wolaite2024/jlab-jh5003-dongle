/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     i2s_master_send_demo.c
* @brief    This file provides demo code of I2S master send data.
* @details
* @author   mh_chang
* @date     2022-10-25
* @version  v1.0
*********************************************************************************************************
*/

#include "rtl876x_i2s.h"
#include "rtl876x_pinmux.h"

#define I2S_LRCK_PIN                    P0_0
#define I2S_BCLK_PIN                    P0_1
#define I2S_SDO_PIN                     P0_2

#define I2S_NUM                         I2S0
#define I2S_LRCK_PINMUX                 LRC_SPORT0
#define I2S_BCLK_PINMUX                 BCLK_SPORT0
#define I2S_SDO_PINMUX                  DACDAT_SPORT0

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
    I2S_InitStruct.I2S_TxChannelType    = I2S_Channel_Stereo;
    I2S_InitStruct.I2S_TxDataWidth      = I2S_Data_Width_16Bits;
    I2S_InitStruct.I2S_TxDataFormat     = I2S_Mode;
    I2S_InitStruct.I2S_DMACmd           = I2S_DMA_DISABLE;
    I2S_Init(I2S_NUM, &I2S_InitStruct);
    I2S_Cmd(I2S_NUM, I2S_MODE_TX, ENABLE);
}

/**
  * @brief  Demo code of I2S send data continuously.
  * @param  No parameter.
  * @return Void
  */
static void i2s_send_data(void)
{
    uint32_t pattern = 0x12345678;

    while (1)
    {
        if (I2S_GetTxFIFODepth(I2S_NUM))
        {
            /* in 16-bits format, low half word send first */
            I2S_SendData(I2S_NUM, pattern);

            /* increase high & low half word by 1 respectively */
            pattern += (BIT16 | BIT0);
        }
    }
}

/**
  * @brief  Demo code of I2S communication.
  * @param  No parameter.
  * @return Void
*/
void i2s_master_send_demo(void)
{
    board_i2s_init();
    driver_i2s_init();
    i2s_send_data();
}
/******************* (C) COPYRIGHT 2022 Realtek Semiconductor Corporation *****END OF FILE****/

