/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     i2c_demo.c
* @brief    This file provides demo code of I2C in interrupt mode which as a master.
* @details
* @author   renee
* @date     2017-03-14
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_i2c.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "vector_table.h"
#include "trace.h"

/** @defgroup  I2C_DEMO_I2C  I2C DEMO
    * @brief  I2C work in master mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup I2C_Master_Demo_Exported_Macros I2C Master Demo Exported Macros
  * @brief
  * @{
  */

#define PIN_I2C0_SCL                P0_0
#define PIN_I2C0_SDA                P0_1

/** @} */ /* End of group I2C_Master_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup I2C_Master_Demo_Exported_Functions I2C Master Demo Exported Functions
  * @brief
  * @{
  */

static void i2c0_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_i2c0_init(void)
{
    Pinmux_Config(PIN_I2C0_SDA, I2C0_DAT);
    Pinmux_Config(PIN_I2C0_SCL, I2C0_CLK);
    Pad_Config(PIN_I2C0_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_I2C0_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
}

/**
  * @brief  Initialize I2C peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_i2c0_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);

    I2C_InitTypeDef  I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);

    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_InitStructure.I2C_DeviveMode = I2C_DeviveMode_Master;
    I2C_InitStructure.I2C_AddressMode = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_SlaveAddress = 0x50;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;

    I2C_Init(I2C0, &I2C_InitStructure);

    RamVectorTableUpdate(I2C0_VECTORn, i2c0_handler);

    /* detect stop signal */
    I2C_INTConfig(I2C0, I2C_INT_STOP_DET, ENABLE);
    /* Config I2C interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = I2C0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    I2C_Cmd(I2C0, ENABLE);
}

/**
  * @brief  demo code of operation about I2C.
  * @param   No parameter.
  * @return  void
  */
void i2c_demo(void)
{
    uint8_t I2C_WriteBuf[16] = {0xaa, 0xbb, 0x66, 0x68, 0x77, 0x88};
    uint8_t I2C_ReadBuf[16] = {0, 0, 0, 0};

    board_i2c0_init();
    driver_i2c0_init();

    //I2C_MasterWrite(I2C0, I2C_WriteBuf, 5);
    I2C_RepeatRead(I2C0, I2C_WriteBuf, 2, I2C_ReadBuf, 4);
}

/**
* @brief  I2C interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void i2c0_handler(void)
{
    if (I2C_GetINTStatus(I2C0, I2C_INT_STOP_DET) == SET)
    {
        // add user code here
        IO_PRINT_INFO0("i2c0_handler: I2C0 stop detect");
        // clear interrupt
        I2C_ClearINTPendingBit(I2C0, I2C_INT_STOP_DET);
    }
}

/** @} */ /* End of group I2C_Master_Demo_Exported_Functions */
/** @} */ /* End of group I2C_DEMO_I2C */

