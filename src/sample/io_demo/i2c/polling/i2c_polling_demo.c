/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     i2c_polling_demo.c
* @brief    This file provides demo code of I2C repeat read data which as a master.
* @details
* @author   renee
* @date     2017-03-13
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"
#include "rtl876x_i2c.h"
#include "string.h"
#include "trace.h"

/** @defgroup  I2C_REPEAT_READ_DEMO  I2C REPEAT READ DEMO
    * @brief  I2C Repeat read data implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup I2C_RepeatRead_Exported_Macros I2C Pepeat Read Exported Macros
  * @brief
  * @{
  */

#define PIN_I2C1_SCL                P0_0
#define PIN_I2C1_SDA                P0_1

#define ADDR              0x08

/** @} */ /* End of group I2C_RepeatRead_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup I2C_RepeatRead_Exported_Functions I2C Pepeat Read Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_i2c_init(void)
{
    Pinmux_Config(PIN_I2C1_SDA, I2C1_DAT);
    Pinmux_Config(PIN_I2C1_SCL, I2C1_CLK);
    Pad_Config(PIN_I2C1_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_I2C1_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
}

/**
  * @brief  Initialize ADC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_i2c_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_I2C1, APBPeriph_I2C1_CLOCK, ENABLE);

    I2C_InitTypeDef  I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);

    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_DeviveMode = I2C_DeviveMode_Master;
    I2C_InitStructure.I2C_AddressMode = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_SlaveAddress = ADDR;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;

    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);
}

/**
  * @brief  demo code of operation about I2C.
  * @param   No parameter.
  * @return  void
  */
void i2c_polling_demo(void)
{
    uint8_t I2C_WriteBuf[16] = {0xaa, 0xbb, 0x66, 0x68, 0x77, 0x88};
    uint8_t I2C_ReadBuf[16] = {0, 0, 0, 0};

    board_i2c_init();

    driver_i2c_init();

    if (I2C_Success != I2C_RepeatRead(I2C1, I2C_WriteBuf, 2, I2C_ReadBuf, 4))
    {
        IO_PRINT_ERROR0("i2c_polling_demo: Send failed");

        //Check Event
        if (I2C_CheckEvent(I2C1, ABRT_7B_ADDR_NOACK) == SET)
        {
            IO_PRINT_ERROR0("i2c_polling_demo: Wrong addr");
        }
        if (I2C_CheckEvent(I2C1, ABRT_GCALL_NOACK) == SET)
        {
            IO_PRINT_ERROR0("i2c_polling_demo: General call nack");
        }
    }
}
/** @} */ /* End of group I2C_RepeatRead_Exported_Functions */
/** @} */ /* End of group I2C_REPEAT_READ_DEMO */
