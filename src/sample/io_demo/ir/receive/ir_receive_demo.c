/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     ir_receive_demo.c
* @brief        This file provides IR demo code to receive data by interrupt.
* @details
* @author   elliot chen
* @date         2016-12-07
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_ir.h"
#include "ir_nec_protocol.h"
#include "vector_table.h"

/** @defgroup  IR_RECEIVE_DEMO  IR RECEIVE DEMO
    * @brief  Ir receive data implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup IR_Receive_Demo_Exported_Macros IR Receive Demo Exported Macros
  * @brief
  * @{
  */

#define IR_RECV_PIN                     P0_2
#define IR_RX_FIFO_THR_LEVEL            30

/** @} */ /* End of group IR_Receive_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup IR_Receive_Demo_Exported_Variables IR Receive Demo Exported Variables
  * @brief
  * @{
  */

/* Buffer which store receiving data */
static IR_DataTypeDef IR_DataStruct;

/* Number of data which has been sent */
static volatile uint16_t rx_count = 0;


/** @} */ /* End of group IR_Receive_Demo_Exported_Variables */

static void ir_handler(void);
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup IR_Receive_Demo_Exported_Functions IR Receive Demo Exported Functions
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
    Pad_Config(IR_RECV_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pinmux_Config(IR_RECV_PIN, IRDA_RX);
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

    IR_InitStruct.IR_Freq               = 38;/* IR carrier freqency is 38KHz */
    IR_InitStruct.IR_DutyCycle          = 2;/* Duty ratio = 1/IR_DutyCycle */
    IR_InitStruct.IR_Mode               = IR_MODE_RX;/* IR receiveing mode */
    IR_InitStruct.IR_RxStartMode        = IR_RX_AUTO_MODE;
    IR_InitStruct.IR_RxFIFOThrLevel     =
        IR_RX_FIFO_THR_LEVEL; /* Configure RX FIFO threshold level to trigger IR_INT_RF_LEVEL interrupt */
    IR_InitStruct.IR_RxFIFOFullCtrl     =
        IR_RX_FIFO_FULL_DISCARD_NEWEST;/* Discard the lastest received dta if RX FIFO is full */
    IR_InitStruct.IR_RxTriggerMode      = IR_RX_RISING_EDGE;/* Configure trigger type */
    IR_InitStruct.IR_RxFilterTime       =
        IR_RX_FILTER_TIME_50ns;/* If high to low or low to high transition time <= 50ns,Filter out it. */
    IR_InitStruct.IR_RxCntThrType       =
        IR_RX_Count_Low_Level;/* IR_RX_Count_Low_Level is counting low level */
    IR_InitStruct.IR_RxCntThr           =
        0x23a;/* Configure RX counter threshold.You can use it to decide to stop receiving IR data */
    IR_Init(&IR_InitStruct);

    RamVectorTableUpdate(IR_VECTORn, (IRQ_Fun)ir_handler);

    /* Enable IR threshold interrupt. when RX FIFO offset >= threshold value, trigger interrupt*/
    /* Enable IR counter threshold interrupt to stop receiving data */
    IR_INTConfig(IR_INT_RF_LEVEL | IR_INT_RX_CNT_THR, ENABLE);
    IR_MaskINTConfig(IR_INT_RF_LEVEL | IR_INT_RX_CNT_THR, DISABLE);

    /* Configure NVIC */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = IR_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 2;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    IR_ClearRxFIFO();
    IR_Cmd(IR_MODE_RX, ENABLE);

}

/**
  * @brief  demo code of IR send data.
  * @param   No parameter.
  * @return  void
  */
void ir_receive_demo(void)
{
    uint8_t address = 0;
    uint8_t cmd = 0;

    /* Initialize IR */
    board_ir_init();
    driver_ir_init();

    /* Wait for receiving the whole IR packets */
    /* This is just a demo. You can send message from IR interrupt handler */
    while (rx_count <= NEC_LENGTH - 5) {;};

    if (IR_SUCCEED != IR_NECDecode(38, &address, &cmd, &IR_DataStruct))
    {
        // Decoding error!
    }
    else
    {
        // Decoding success!
    }
}


/**
* @brief IR interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void ir_handler(void)
{
    uint16_t len = 0;

    /* Receive by interrupt */
    if (IR_GetINTStatus(IR_INT_RF_LEVEL) == SET)
    {
        IR_MaskINTConfig(IR_INT_RF_LEVEL, ENABLE);
        len = IR_GetRxDataLen();
        IR_ReceiveBuf(IR_DataStruct.irBuf + rx_count, len);
        IR_DataStruct.bufLen += len;
        rx_count += len;
        IR_ClearINTPendingBit(IR_INT_RF_LEVEL_CLR);
        IR_MaskINTConfig(IR_INT_RF_LEVEL, DISABLE);
    }

    /* Stop to receive IR data */
    if (IR_GetINTStatus(IR_INT_RX_CNT_THR) == SET)
    {
        IR_MaskINTConfig(IR_INT_RX_CNT_THR, ENABLE);
        /* Read remaining data */
        len = IR_GetRxDataLen();
        IR_ReceiveBuf(IR_DataStruct.irBuf + rx_count, len);
        IR_DataStruct.bufLen += len;
        rx_count += len;
        IR_ClearINTPendingBit(IR_INT_RX_CNT_THR_CLR);
        IR_MaskINTConfig(IR_INT_RX_CNT_THR, DISABLE);
    }
}

/** @} */ /* End of group IR_Receive_Demo_Exported_Functions */
/** @} */ /* End of group IR_RECEIVE_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

