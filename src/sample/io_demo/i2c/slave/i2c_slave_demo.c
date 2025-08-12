/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     i2c_slave_demo.c
* @brief    This file provides demo code of I2C in interrupt mode which as a slave.
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

/** @defgroup  I2C_SLAVE_DEMO  I2C SLAVE DEMO
    * @brief  I2C work in slave mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup I2C_Slave_Demo_Exported_Macros I2C Slave Demo Exported Macros
  * @brief
  * @{
  */

#define PIN_I2C1_SCL                P0_0
#define PIN_I2C1_SDA                P0_1

/** @} */ /* End of group I2C_Slave_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup I2C_Slave_Demo_Exported_Functions I2C Slave Demo Exported Functions
  * @brief
  * @{
  */

static void i2c1_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_i2c1_init(void)
{
    Pinmux_Config(PIN_I2C1_SDA, I2C1_DAT);
    Pinmux_Config(PIN_I2C1_SCL, I2C1_CLK);
    Pad_Config(PIN_I2C1_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_I2C1_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
}

/**
  * @brief  Initialize I2C peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_i2c1_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_I2C1, APBPeriph_I2C1_CLOCK, ENABLE);

    I2C_InitTypeDef  I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);

    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_InitStructure.I2C_DeviveMode = I2C_DeviveMode_Slave;
    I2C_InitStructure.I2C_AddressMode = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_SlaveAddress = 0x50;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;

    I2C_Init(I2C1, &I2C_InitStructure);

    RamVectorTableUpdate(I2C1_VECTORn, i2c1_handler);

    /* Config I2C interrupt */
    I2C_INTConfig(I2C1, I2C_INT_RD_REQ | I2C_INT_RX_FULL | I2C_INT_STOP_DET, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = I2C1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    I2C_Cmd(I2C1, ENABLE);
}


/**
  * @brief  demo code of operation about I2C.
  * @param   No parameter.
  * @return  void
  */
void i2c_slave_demo(void)
{
    board_i2c1_init();
    driver_i2c1_init();
}

/**
* @brief  I2C1 interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void i2c1_handler(void)
{
    uint8_t rx_count = 0;
    uint8_t read_buf = 0;

    // any master want to read data will result this type interrupt.
    if (I2C_GetINTStatus(I2C1, I2C_INT_RD_REQ) == SET)
    {
        // add user code here
        //write data and not generate a stop signal. attention: slave have no right to generate stop signal. If ENABLE, it will cause stop signal which is not allowed .
        I2C_SendCmd(I2C1, I2C_WRITE_CMD, 0x66, I2C_STOP_DISABLE);

        // clear interrupt
        I2C_ClearINTPendingBit(I2C1, I2C_INT_RD_REQ);
    }

    if (I2C_GetINTStatus(I2C1, I2C_INT_RX_FULL) == SET)
    {
        rx_count = I2C_GetRxFIFOLen(I2C1);

        for (uint8_t i = 0; i < rx_count; i++)
        {
            read_buf = I2C_ReceiveData(I2C1);
            IO_PRINT_INFO2("i2c1_handler: rx_count %d, read_buf %d", rx_count, read_buf);
        }

        // clear interrupt
        I2C_ClearINTPendingBit(I2C1, I2C_INT_RX_FULL);
    }

    if (I2C_GetINTStatus(I2C1, I2C_INT_STOP_DET) == SET)
    {
        // add user code here
        IO_PRINT_INFO0("i2c1_handler: I2C1 stop detect");
        // clear interrupt
        I2C_ClearINTPendingBit(I2C1, I2C_INT_STOP_DET);
    }
}

/** @} */ /* End of group I2C_Slave_Demo_Exported_Functions */
/** @} */ /* End of group I2C_SLAVE_DEMO */

