/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     ir_send_interrupt_demo.c
* @brief    This file provides IR demo code to send data by interrupt.
* @details
* @author   elliot chen
* @date     2016-12-07
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "os_task.h"
#include "os_msg.h"
#include "trace.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_ir.h"
#include "ir_nec_protocol.h"
#include "vector_table.h"

/** @defgroup  IR_SEND_INTERRUPT_DEMO  IR SEND INTERRUPT DEMO
    * @brief  Ir send data implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup IR_Send_INTERRUPT_Demo_Exported_Macros IR_Send_INTERRUPT_Demo_Exported_Macros
  * @brief
  * @{
  */

#define IR_SEND_PIN                     P2_2
#define IR_TX_FIFO_THR_LEVEL            2

#define IO_DEMO_EVENT_QUEUE_SIZE        0x10
#define IO_DEMO_EVENT_IR_TX             0x01

/** @} */ /* End of group IR_Send_INTERRUPT_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup IR_Send_INTERRUPT_Demo_Exported_Variables IR_Send_INTERRUPT_Demo_Exported_Variables
  * @brief
  * @{
  */

/* Buffer which store encoded data */
static IR_DataTypeDef IR_DataStruct;
/* Number of data which has been sent */
static uint8_t tx_count = 0;

static void *io_queue_handle;
static void *iodemo_app_task_handle;
/** @} */ /* End of group IR_Send_INTERRUPT_Demo_Exported_Variables */

static void ir_handler(void);
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup IR_Send_INTERRUPT_Demo_Exported_Functions IR_Send_INTERRUPT_Demo_Exported_Functions
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
    IR_InitStruct.IR_TxIdleLevel    = IR_IDLE_OUTPUT_LOW;
    IR_InitStruct.IR_TxFIFOThrLevel = IR_TX_FIFO_THR_LEVEL;
    IR_Init(&IR_InitStruct);

    RamVectorTableUpdate(IR_VECTORn, (IRQ_Fun)ir_handler);

    /* Configure NVIC */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = IR_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 2;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  demo code of IR send data.
  * @param   No parameter.
  * @return  void
  */
static void io_demo_task(void *param)
{
    uint8_t event = 0;

    /* Initialize IR */
    board_ir_init();
    driver_ir_init();

    /* Data to send */
    uint8_t ir_code[2] = {0x16, 0x28};

    /* Make sure TX fifo is empty */
    IR_ClearTxFIFO();

    /* Encode by NEC protocol */
    IR_NECEncode(38, ir_code[0], ir_code[1], &IR_DataStruct);

    /* Start to send first bytes data of encoded data */
    IR_SendBuf(IR_DataStruct.irBuf, IR_TX_FIFO_SIZE, DISABLE);

    /* Record number which has been sent */
    tx_count = IR_TX_FIFO_SIZE;

    /* Enable IR threshold interrupt. when TX FIFO offset <= threshold value, trigger interrupt*/
    IR_INTConfig(IR_INT_TF_LEVEL, ENABLE);

    /* Must fill TX FIFO first */
    IR_Cmd(IR_MODE_TX, ENABLE);

    while (1)
    {
        if (os_msg_recv(io_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == IO_DEMO_EVENT_IR_TX)
            {
                /* Make sure IR TX finish before disable IR */
                while((IR_GetFlagStatus(IR_FLAG_TX_RUN) == SET));

                /* Must disable IR TX before next package send */
                IR_Cmd(IR_MODE_TX, DISABLE);
            }
        }
    }
}

/**
  * @brief   demo code of IR send data.
  * @param   void
  * @return  void
  */
void ir_send_interrupt_demo(void)
{
    os_task_create(&iodemo_app_task_handle, "app", io_demo_task, NULL, 384 * 4, 2);

    os_msg_queue_create(&io_queue_handle, "IRRxQ", IO_DEMO_EVENT_QUEUE_SIZE, sizeof(uint8_t));
}

/**
* @brief IR interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void ir_handler(void)
{
    /* Continue to send by interrupt */
    if (IR_GetINTStatus(IR_INT_TF_LEVEL) == SET)
    {
        IR_MaskINTConfig(IR_INT_TF_LEVEL, ENABLE);
        /* The remaining data is larger than the TX FIFO length */
        if ((NEC_LENGTH - tx_count) >= IR_TX_FIFO_SIZE)
        {
            IR_SendBuf(IR_DataStruct.irBuf + tx_count, (IR_TX_FIFO_SIZE - IR_TX_FIFO_THR_LEVEL), DISABLE);
            tx_count += (IR_TX_FIFO_SIZE - IR_TX_FIFO_THR_LEVEL);
        }
        else if ((NEC_LENGTH - tx_count) > 0)
        {
            /* The remaining data is less than the TX FIFO length */

            /*  Configure TX threshold level to zero and trigger interrupt when TX FIFO is empty */
            IR_SetTxThreshold(0);
            IR_SendBuf(IR_DataStruct.irBuf + tx_count, NEC_LENGTH - tx_count, ENABLE);
            tx_count += (NEC_LENGTH - tx_count);
        }
        else
        {
            /* Tx completed */
            uint8_t event = IO_DEMO_EVENT_IR_TX;
            /* Disable IR tx empty interrupt */
            IR_INTConfig(IR_INT_TF_LEVEL, DISABLE);
            tx_count = 0;
            if (os_msg_send(io_queue_handle, &event, 0) == false)
            {
                IO_PRINT_ERROR0("ir_handler: Send queue error");
            }
        }

        /* Clear threshold interrupt */
        IR_ClearINTPendingBit(IR_INT_TF_LEVEL_CLR);
        /* Unmask IR interrupt */
        IR_MaskINTConfig(IR_INT_TF_LEVEL, DISABLE);
    }
}

/** @} */ /* End of group IR_Send_INTERRUPT_Demo_Exported_Functions */
/** @} */ /* End of group IR_SEND_INTERRUPT_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

