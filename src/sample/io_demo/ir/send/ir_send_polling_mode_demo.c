/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     ir_send_polling_mode_demo.c
* @brief    This file provides IR demo code to send data by polling.
* @details
* @author   colin_lu
* @date     2023-02-01
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_ir.h"

/** @defgroup  IR_SEND_POLLING_MODE_DEMO  IR SEND POLLING MODE DEMO
    * @brief  Ir send data implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup IR_Send_POLLING_MODE_Demo_Exported_Macros IR_Send_POLLING_MODE_Demo_Exported_Macros
  * @brief
  * @{
  */

#define IR_SEND_PIN                     P2_2
#define IR_TX_FIFO_THR_LEVEL            4

/** @} */ /* End of group IR_Send_POLLING_MODE_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup IR_Send__POLLING_MODE_Demo_Exported_Variables IR_Send_POLLING_MODE_Demo_Exported_Variables
  * @brief
  * @{
  */

/* Buffer which to be sent */
static uint32_t buffer[64] = { 0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF,
                        0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF,
                        0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF,
                        0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF,
                        0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF,
                        0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF,
                        0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF,
                        0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF, 0x80000004, 0x000000FF};
/** @} */ /* End of group IR_Send_POLLING_MODE_Demo_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup IR_Send_POLLING_MODE_Demo_Exported_Functions IR_Send_POLLING_MODE_Demo_Exported_Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_ir_init(void)
{
    Pad_Config(IR_SEND_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pinmux_Config(IR_SEND_PIN, IRDA_TX);
}

/**
  * @brief  Initialize IR peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_ir_init(void)
{
    /* Enable IR clock */
    RCC_PeriphClockCmd(APBPeriph_IR, APBPeriph_IR_CLOCK, ENABLE);

    /* Initialize IR */
    IR_InitTypeDef IR_InitStruct;
    IR_StructInit(&IR_InitStruct);
    IR_InitStruct.IR_Freq           = 38;
    IR_InitStruct.IR_DutyCycle      = 2; /* !< 1/2 duty cycle */
    IR_InitStruct.IR_Mode           = IR_MODE_TX;
    IR_InitStruct.IR_TxInverse      = IR_TX_DATA_NORMAL;
    IR_InitStruct.IR_TxIdleLevel    = IR_IDLE_OUTPUT_HIGH;
    IR_InitStruct.IR_TxFIFOThrLevel = IR_TX_FIFO_THR_LEVEL;
    IR_Init(&IR_InitStruct);
}

/**
  * @brief  demo code of IR send data.
  * @param   No parameter.
  * @return  void
  */
void ir_send_polling_mode_demo(void)
{
    uint32_t total_data_cnt = 0;
    /* Initialize IR */
    board_ir_init();

    driver_ir_init();

    /* Make sure TX fifo is empty */
    IR_ClearTxFIFO();

    total_data_cnt = sizeof(buffer) / sizeof(buffer[0]);

    if(total_data_cnt >= IR_TX_FIFO_SIZE)
    {
        IR_SendBuf(buffer, IR_TX_FIFO_SIZE, DISABLE);
        total_data_cnt -= IR_TX_FIFO_SIZE;
    }
    else
    {
        IR_SendBuf(buffer, total_data_cnt, ENABLE);
        total_data_cnt = 0;
    }

    /* Must fill TX FIFO first */
    IR_Cmd(IR_MODE_TX, ENABLE);

    while(total_data_cnt)
    {
        if ((IR_GetFlagStatus(IR_FLAG_TX_RUN) == SET) && (IR_GetTxFIFOFreeLen() >= (IR_TX_FIFO_SIZE - IR_TX_FIFO_THR_LEVEL)))
        {
            if(total_data_cnt >= (IR_TX_FIFO_SIZE - IR_TX_FIFO_THR_LEVEL))
            {
                IR_SendBuf(buffer, (IR_TX_FIFO_SIZE - IR_TX_FIFO_THR_LEVEL), DISABLE);
                total_data_cnt -= (IR_TX_FIFO_SIZE - IR_TX_FIFO_THR_LEVEL);
            }
            else
            {
                IR_SendBuf(buffer, total_data_cnt, ENABLE);
                total_data_cnt = 0;
            }
        }
    }

    /* Make sure IR TX finish before disable IR */
    while((IR_GetFlagStatus(IR_FLAG_TF_EMPTY) == RESET) || (IR_GetFlagStatus(IR_FLAG_TX_RUN) == SET));

    /* Must disable IR TX before next package send */
    IR_Cmd(IR_MODE_TX, DISABLE);
}

/** @} */ /* End of group IR_Send_POLLING_MODE_Demo_Exported_Functions */
/** @} */ /* End of group IR_SEND_POLLING_MODE_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

