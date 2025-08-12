/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi3wire_demo.c
* @brief    This file provides demo code for three wire SPI comunication with PMW3610DM-SUDU mouse sensor.
* @details
* @author   elliot chen
* @date         2016-12-14
* @version  v1.0
*********************************************************************************************************
*/


/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_3wire_spi.h"
#include "platform_utils.h"
#include "trace.h"

/** @defgroup  SPI3WIRE_DEMO  SPI3WIRE
    * @brief  3WireSPI implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI3Wire_Demo_Exported_Macros SPI3Wire Demo Exported Macros
  * @brief
  * @{
  */
#define SPI_3WIRE_CLK_PIN       P0_0
#define SPI_3WIRE_DATA_PIN      P0_1
#define SPI_3WIRE_CS_PIN        P0_2
#define MOUSE_RESET_ADD         0x3a
#define MOUSE_RESET_DATA        0x5a
#define MOUSE_PROD_ID   0x00  /*mouse sensor PROD ID, default value 0x3e*/
#define MOUSE_REV_ID    0x01  /*mouse sensor REV ID, default value 0x01*/

/** @} */ /* End of group SPI3Wire_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup SPI3Wire_Demo_Exported_Functions SPI3Wire Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void Board_SPI3WIRE_Init(void)
{
    Pad_Config(SPI_3WIRE_CLK_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(SPI_3WIRE_DATA_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(SPI_3WIRE_CS_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);

    Pinmux_Config(SPI_3WIRE_CLK_PIN, SPI2W_CLK);
    Pinmux_Config(SPI_3WIRE_DATA_PIN, SPI2W_DATA);
    Pinmux_Config(SPI_3WIRE_CS_PIN, SPI2W_CS);
}

/**
  * @brief  Initialize SPI3WIRE peripheral.
  * @param   No parameter.
  * @return  void
  */
static void Driver_SPI3WIRE_Init(void)
{
    /* Enable SPI3WIRE clock */
    RCC_PeriphClockCmd(APBPeriph_SPI2W, APBPeriph_SPI2W_CLOCK, ENABLE);

    /* Initialize SPI3WIRE */
    SPI3WIRE_InitTypeDef SPI3WIRE_InitStruct;
    SPI3WIRE_StructInit(&SPI3WIRE_InitStruct);

    SPI3WIRE_InitStruct.SPI3WIRE_SysClock       = 20000000;
    SPI3WIRE_InitStruct.SPI3WIRE_Speed          = 800000;
    SPI3WIRE_InitStruct.SPI3WIRE_Mode           = SPI3WIRE_3WIRE_MODE;
    /* delay time = (SPI3WIRE_ReadDelay +1)/(2*SPI3WIRE_Speed). The delay time from the end of address phase to the start of read data phase */
    //delay time = (0x03 + 1)/(2 * speed) = 2.5us
    SPI3WIRE_InitStruct.SPI3WIRE_ReadDelay      = 0x3;
    SPI3WIRE_InitStruct.SPI3WIRE_OutputDelay    = SPI3WIRE_OE_DELAY_NONE;
    SPI3WIRE_InitStruct.SPI3WIRE_ExtMode        = SPI3WIRE_NORMAL_MODE;
    SPI3WIRE_Init(&SPI3WIRE_InitStruct);
}

/**
 * @brief read one byte through 3wire SPI perpherial .
 * @param  address: address of register which need to read .
 * @return value of register.
*/
static uint8_t Mouse_SingleRead(uint8_t address)
{
    uint8_t reg_value = 0;
    uint32_t timeout = 0;

    /* Check SPI busy or not */
    while (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_BUSY) == SET)
    {
        timeout++;
        if (timeout > 0x1ffff)
        {
            break;
        }
    }

    /* Clear Receive data length */
    SPI3WIRE_ClearRxDataLen();

    SPI3WIRE_StartRead(address, 1);

    timeout = 0;
    /* Wait for the end of communication */
    while (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_BUSY) == SET)
    {
        timeout++;
        if (timeout > 0x1ffff)
        {
            break;
        }
    }

    /* Get the length of received data */
    while (SPI3WIRE_GetRxDataLen() == 0);
    /* Read data */
    SPI3WIRE_ReadBuf(&reg_value, 1);

    return reg_value;
}

/**
 * @brief write one byte.
 * @param address: address of register which need to write data.
 * @param data: data which need to write.
 * @return TRUE: write success, FALSE: write failure.
*/
static bool Mouse_SingleWrite(uint8_t address, uint8_t data)
{
    uint32_t timeout = 0;

    /* Check SPI busy or not */
    while (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_BUSY) == SET)
    {
        timeout++;
        if (timeout > 0x1ffff)
        {
            return false;
        }
    }
    /* Write data */
    SPI3WIRE_StartWrite(address, data);

    timeout = 0;
    /* Wait communication to end */
    while (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_BUSY) == SET)
    {
        timeout++;
        if (timeout > 0x1ffff)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief reset mouse.
 * @param none.
 * @return none.
*/
static void Mouse_Reset(void)
{
    Mouse_SingleWrite(MOUSE_RESET_ADD, MOUSE_RESET_DATA);

    platform_delay_ms(3);
}

/**
 * @brief read mouse product ID.
 * @param  p_id, --pointer to production id buffer,buffer length should more than two.
 * @return ture.
*/
static bool Mouse_GetProductID(uint8_t *p_id)
{
    /* Read mouse sensor PROD ID */
    *p_id++ = Mouse_SingleRead(MOUSE_PROD_ID);

    /* Read mouse sensor REV ID */
    *p_id = Mouse_SingleRead(MOUSE_REV_ID);

    return true;
}

/**
  * @brief  demo code of SPI3WIRE communication.
  * @param   No parameter.
  * @return  void
  */
void spi3wire_demo(void)
{
    uint8_t id[2] = {0, 0};

    Board_SPI3WIRE_Init();
    Driver_SPI3WIRE_Init();

    /* Send resync time. Resync signal time = 2*1/(2*SPI3WIRE_Speed) = 1.25us */
    SPI3WIRE_SetResyncTime(2);
    SPI3WIRE_ResyncSignalCmd(ENABLE);
    while (SPI3WIRE_GetFlagStatus(SPI3WIRE_FLAG_RESYNC_BUSY) == SET);
    SPI3WIRE_ResyncSignalCmd(DISABLE);

    /* Enable SPI3WIRE to normal communication */
    SPI3WIRE_Cmd(ENABLE);

    Mouse_Reset();

    Mouse_GetProductID(&id[0]);
    IO_PRINT_TRACE2("spi3wire_demo: id[0] 0x%x, id[1] 0x%x", id[0], id[1]);

    if ((0x3e == id[0]) && (0x01 == id[1]))
    {
        IO_PRINT_INFO0("spi3wire_demo: Read mouse sensor ID success");
    }
    else if ((0x3e != id[0]) || (0x01 != id[1]))
    {
        IO_PRINT_ERROR0("spi3wire_demo: Read mouse sensor ID error");
    }
}

/** @} */ /* End of group SPI3Wire_Demo_Exported_Functions */
/** @} */ /* End of group SPI3WIRE_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

