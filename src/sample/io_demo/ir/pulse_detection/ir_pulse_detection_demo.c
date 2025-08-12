/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file         ir_pulse_detection_demo.c
* @brief        This file provides IR demo code to pulse detection.
* @details
* @author       colin_lu
* @date         2022-12-20
* @version      v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_ir.h"
#include "vector_table.h"
#include "trace.h"
/** @defgroup  IR_PULSE_DETECTION_DEMO  IR PULSE DETECTION DEMO
    * @brief  Ir pulse detection implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup IR_PULSE_DETECTION_Demo_Exported_Macros IR Pulse Detection Demo Exported Macros
  * @brief
  * @{
  */

#define IR_RECV_PIN                     P2_1
#define IR_RX_FIFO_THR_LEVEL            2

/** @} */ /* End of group IR_PULSE_DETECTION_Demo_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup IR_PULSE_DETECTION_Demo_Exported_Variables IR Pulse Detection Demo Exported Variables
  * @brief
  * @{
  */

/* Buffer which store receiving data */
static uint32_t IR_DataStruct[50];

/* Number of data which has been sent */
static uint16_t rx_count = 0;
static uint8_t rx_finish_flag = 0;

/** @} */ /* End of group IR_PULSE_DETECTION_Demo_Exported_Variables */

static void ir_handler(void);
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup IR_PULSE_DETECTION_Demo_Exported_Functions IR Pulse Detection Demo Exported Functions
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

    IR_InitStruct.IR_Freq               =
        40000;                               /* IR carrier freqency is 40KHz */
    IR_InitStruct.IR_Mode               = IR_MODE_RX;                       /* IR receiveing mode */
    IR_InitStruct.IR_RxStartMode        = IR_RX_AUTO_MODE;
    IR_InitStruct.IR_RxFIFOThrLevel     =
        IR_RX_FIFO_THR_LEVEL;             /* Configure RX FIFO threshold level to trigger IR_INT_RF_LEVEL interrupt */
    IR_InitStruct.IR_RxFIFOFullCtrl     =
        IR_RX_FIFO_FULL_DISCARD_NEWEST;   /* Discard the lastest received dta if RX FIFO is full */
    IR_InitStruct.IR_RxTriggerMode      = IR_RX_DOUBLE_EDGE;                /* Configure trigger type */
    IR_InitStruct.IR_RxFilterTime       =
        IR_RX_FILTER_TIME_50ns;           /* If high to low or low to high transition time <= 50ns,Filter out it. */
    IR_InitStruct.IR_RxCntThrType       =
        IR_RX_Count_Low_Level;            /* RX level timeout trigger level */
    IR_InitStruct.IR_RxCntThr           =
        2000000;                             /* 2000000 * 0.000025ms = 50ms, RX level timeout cnt.You can use it to decide to stop receiving IR data */
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
void ir_pulse_detection_demo(void)
{
    uint32_t high_cnt = 0;
    uint32_t low_cnt = 0;
    uint32_t freq = 0;
    /* Initialize IR */
    board_ir_init();
    driver_ir_init();

    while (1)
    {
        if (rx_finish_flag)
        {
            for (uint8_t i = 0; i < rx_count; i++)
            {
                if (IR_DataStruct[i] & BIT(31))
                {
                    /* High level time cnt */
                    high_cnt = (IR_DataStruct[i] & 0x7FFFFFFF) + 1;
                }
                else
                {
                    /* Low level time cnt */
                    low_cnt = IR_DataStruct[i] + 1;
                }
            }

            freq = 40000000 / (high_cnt + low_cnt);   //Hz
            IO_PRINT_INFO3("ir_pulse_detection_demo: freq %d, high_cnt %d, low_cnt %d", freq, high_cnt,
                           low_cnt);
            rx_finish_flag = 0;
            IR_ClearRxFIFO();
            IR_Cmd(IR_MODE_RX, ENABLE);
        }
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
        IR_ReceiveBuf(IR_DataStruct + rx_count, len);
        rx_count += len;
        rx_finish_flag = 1;
        IR_ClearINTPendingBit(IR_INT_RF_LEVEL_CLR);
        IR_MaskINTConfig(IR_INT_RF_LEVEL, DISABLE);
        IR_Cmd(IR_MODE_RX, DISABLE);
    }

    /* Stop to receive IR data */
    if (IR_GetINTStatus(IR_INT_RX_CNT_THR) == SET)
    {
        IR_MaskINTConfig(IR_INT_RX_CNT_THR, ENABLE);
        /* Read remaining data */
        len = IR_GetRxDataLen();
        IR_ReceiveBuf(IR_DataStruct + rx_count, len);
        rx_count += len;
        rx_finish_flag = 1;
        IR_ClearINTPendingBit(IR_INT_RX_CNT_THR_CLR);
        IR_MaskINTConfig(IR_INT_RX_CNT_THR, DISABLE);
        IR_Cmd(IR_MODE_RX, DISABLE);
    }
}

/** @} */ /* End of group IR_PULSE_DETECTION_Demo_Exported_Functions */
/** @} */ /* End of group IR_PULSE_DETECTION_DEMO */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

