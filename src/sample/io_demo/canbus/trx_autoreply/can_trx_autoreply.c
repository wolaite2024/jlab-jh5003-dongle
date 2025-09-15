/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file      can_trx_autoreply.c
* @brief     This file provides all the demo code for CAN bus firmware functions.
* @details
* @author
* @date      2024-02-28
* @version   v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "string.h"
#include "rtl_can.h"
#include "trace.h"
#include "vector_table.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"

/*============================================================================*
 *                              Macros
 *============================================================================*/

#define CAN_TX_PIN          P4_0
#define CAN_RX_PIN          P4_1

/*============================================================================*
 *                              Variables
 *============================================================================*/

/*============================================================================*
 *                              Functions
 *============================================================================*/
void can_trx_autoreply_handler(void);

static void can_board_init(void)
{
    /* Config pinmux and pad for CAN. */
    Pinmux_Config(CAN_TX_PIN, CAN_TX);
    Pinmux_Config(CAN_RX_PIN, CAN_RX);

    Pad_Config(CAN_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(CAN_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

static void can_driver_init(void)
{
    CAN_DeInit(CAN0);

    /* Enable rcc for CAN before initialize. */
    RCC_PeriphClockCmd(APBPeriph_CAN0, APBPeriph_CAN0_CLOCK, ENABLE);

    IO_PRINT_INFO1("can_driver_init: BUS state: %d, waiting...", CAN_GetBusState(CAN0));

    /* Initialize CAN. */
    CAN_InitTypeDef init_struct = {0};
    CAN_StructInit(&init_struct);
    init_struct.CAN_AutoReTxEn = DISABLE;

    /* can speed = 40mhz / ((brp + 1)*(1 + tseg1 + 1 + tseg2 + 1)) */
    /* Here config to 500khz. */
    init_struct.CAN_BitTiming.b.can_brp = 3;
    init_struct.CAN_BitTiming.b.can_sjw = 3;
    init_struct.CAN_BitTiming.b.can_tseg1 = 13;
    init_struct.CAN_BitTiming.b.can_tseg2 = 4;

    init_struct.CAN_FdEn = DISABLE;

    RamVectorTableUpdate(CAN_VECTORn, (IRQ_Fun)can_trx_autoreply_handler);

    CAN_Init(CAN0, &init_struct);

    /* CAN enable */
    CAN_Cmd(CAN0, ENABLE);

    /* CAN interrupts enable */
    CAN_INTConfig(CAN0, (CAN_BUS_OFF_INT | CAN_ERROR_INT |
                         CAN_RX_INT | CAN_TX_INT), ENABLE);

    /* NVIC enable */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = CAN_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    /* polling CAN bus on status */
    while (CAN_GetBusState(CAN0) != CAN_BUS_STATE_ON)
    {
        __asm volatile
        (
            "nop    \n"
        );
    }

    IO_PRINT_INFO1("can_driver_init: BUS ON %d", CAN_GetBusState(CAN0));
}

void can_receive_remote_auto_tx_reply(void)
{
    CANError_TypeDef tx_error;
    CANTxFrame_TypeDef tx_frame_type;

    uint8_t tx_data[8];
    for (uint8_t index = 0; index < 8; index++)
    {
        tx_data[index] = index;
    }

    tx_frame_type.msg_buf_id = 0;
    tx_frame_type.frame_brs_bit = CAN_BRS_NO_SWITCH_BIT_TIMING;
    tx_frame_type.frame_type = CAN_STD_DATA_FRAME;
    tx_frame_type.standard_frame_id = 0x01;
    tx_frame_type.extend_frame_id = 0x01;
    tx_frame_type.auto_reply_bit = SET;
    CAN_MBTxINTConfig(CAN0, tx_frame_type.msg_buf_id, ENABLE);
    tx_error = CAN_SetMsgBufTxMode(CAN0, &tx_frame_type, tx_data, 8);

    while (CAN_GetRamState(CAN0) != CAN_RAM_STATE_IDLE)
    {
        __asm volatile
        (
            "nop    \n"
        );
    }

    if (tx_error != CAN_NO_ERR)
    {
        IO_PRINT_INFO1("can_receive_remote_auto_tx_reply: tx error %d", tx_error);
    }

    IO_PRINT_INFO1("can_receive_remote_auto_tx_reply: tx auto reply remote frame id: 0x%x",
                   tx_frame_type.standard_frame_id);
}

void can_send_remote_auto_receive_reply(void)
{
    CANError_TypeDef rx_error;
    CANRxFrame_TypeDef rx_frame_type;

    rx_frame_type.msg_buf_id = 10;
    rx_frame_type.extend_frame_id = 0;
    rx_frame_type.standard_frame_id = 0x10;
    rx_frame_type.frame_ide_bit = CAN_IDE_STANDARD_FORMAT;
    rx_frame_type.frame_rtr_bit = CAN_RTR_DATA_FRAME;
    rx_frame_type.frame_rtr_mask = RESET;
    rx_frame_type.frame_ide_mask = RESET;
    rx_frame_type.frame_id_mask = 0;
    rx_frame_type.rx_dma_en = RESET;
    rx_frame_type.auto_reply_bit = SET;
    rx_error = CAN_SetMsgBufRxMode(CAN0, &rx_frame_type);

    CAN_MBRxINTConfig(CAN0, rx_frame_type.msg_buf_id, ENABLE);

    while (CAN_GetRamState(CAN0) != CAN_RAM_STATE_IDLE)
    {
        __asm volatile
        (
            "nop    \n"
        );
    }

    if (rx_error != CAN_NO_ERR)
    {
        IO_PRINT_INFO1("can_send_remote_auto_receive_reply: rx error %d", rx_error);
    }

    IO_PRINT_INFO1("can_send_remote_auto_receive_reply: rx auto receive data frame id: 0x%x",
                   rx_frame_type.standard_frame_id);
}

void can_trx_autoreply_demo(void)
{
    IO_PRINT_INFO0("can_trx_autoreply_demo: start!");

    can_board_init();

    can_driver_init();

    can_receive_remote_auto_tx_reply();
    can_send_remote_auto_receive_reply();

    /* Waiting for rx data to generate interrupt. */
}

void can_trx_autoreply_handler(void)
{
    if (SET == CAN_GetINTStatus(CAN0, CAN_BUS_OFF_INT_FLAG))
    {
        IO_PRINT_INFO0("can_trx_autoreply_handler: CAN BUS OFF");
        CAN_ClearINTPendingBit(CAN0, CAN_BUS_OFF_INT_FLAG);

        /* Add user code. */
    }

    if (SET == CAN_GetINTStatus(CAN0, CAN_ERROR_INT_FLAG))
    {
        IO_PRINT_INFO0("can_trx_autoreply_handler: CAN ERROR");
        CAN_ClearINTPendingBit(CAN0, CAN_ERROR_INT_FLAG);

        if (SET == CAN_GetErrorStatus(CAN0, CAN_ERROR_RX))
        {
            IO_PRINT_INFO0("can_trx_autoreply_handler: CAN ERROR RX");
            CAN_CLearErrorStatus(CAN0, CAN_ERROR_RX);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN0, CAN_ERROR_TX))
        {
            IO_PRINT_INFO0("can_trx_autoreply_handler: CAN ERROR TX");
            CAN_CLearErrorStatus(CAN0, CAN_ERROR_TX);

            for (uint8_t index = 0; index < CAN_MESSAGE_BUFFER_MAX_CNT; index++)
            {
                if (SET == CAN_GetMBnTxErrorFlag(CAN0, index))
                {
                    IO_PRINT_INFO1("can_trx_autoreply_handler: CAN ERROR TX MB_%d", index);
                    CAN_ClearMBnTxErrorFlag(CAN0, index);

                    /* Add user code. */
                }
            }
        }

        if (SET == CAN_GetErrorStatus(CAN0, CAN_ERROR_ACK))
        {
            IO_PRINT_INFO0("can_trx_autoreply_handler: CAN ERROR ACK");
            CAN_CLearErrorStatus(CAN0, CAN_ERROR_ACK);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN0, CAN_ERROR_STUFF))
        {
            IO_PRINT_INFO0("can_trx_autoreply_handler: CAN ERROR STUFF");
            CAN_CLearErrorStatus(CAN0, CAN_ERROR_STUFF);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN0, CAN_ERROR_CRC))
        {
            IO_PRINT_INFO0("can_trx_autoreply_handler: CAN_ERROR_CRC");
            CAN_CLearErrorStatus(CAN0, CAN_ERROR_CRC);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN0, CAN_ERROR_FORM))
        {
            IO_PRINT_INFO0("can_trx_autoreply_handler: CAN ERROR FORM");
            CAN_CLearErrorStatus(CAN0, CAN_ERROR_FORM);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN0, CAN_ERROR_BIT1))
        {
            IO_PRINT_INFO0("can_trx_autoreply_handler: CAN ERROR BIT1");
            CAN_CLearErrorStatus(CAN0, CAN_ERROR_BIT1);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN0, CAN_ERROR_BIT0))
        {
            IO_PRINT_INFO0("can_trx_autoreply_handler: CAN ERROR BIT0");
            CAN_CLearErrorStatus(CAN0, CAN_ERROR_BIT0);

            /* Add user code. */
        }
    }

    if (SET == CAN_GetINTStatus(CAN0, CAN_RX_INT_FLAG))
    {
        IO_PRINT_INFO0("can_trx_autoreply_handler: CAN RX INT");
        CAN_ClearINTPendingBit(CAN0, CAN_RX_INT_FLAG);

        for (uint8_t index = 0; index < CAN_MESSAGE_BUFFER_MAX_CNT; index++)
        {
            if (SET == CAN_GetMBnRxDoneFlag(CAN0, index))
            {
                IO_PRINT_INFO1("can_trx_autoreply_handler: MB_%d rx done", index);
                CAN_ClearMBnRxDoneFlag(CAN0, index);

                CANMsgBufInfo_TypeDef mb_info;
                CAN_GetMsgBufInfo(CAN0, index, &mb_info);

                uint8_t rx_data[64];
                memset(rx_data, 0, 64);
                CAN_GetRamData(CAN0, mb_info.data_length, rx_data);

                CANDataFrameSel_TypeDef frame_type = CAN_CheckFrameType(mb_info.rtr_bit, mb_info.ide_bit,
                                                                        mb_info.edl_bit);

                IO_PRINT_INFO3("can_trx_autoreply_handler: frame_type %d, frame_id = 0x%03x, ext_frame_id = 0x%05x",
                               \
                               frame_type, mb_info.standard_frame_id, mb_info.extend_frame_id);

                for (uint8_t index = 0; index < mb_info.data_length; index++)
                {
                    IO_PRINT_INFO2("can_trx_autoreply_handler: rx_data [%d] 0x%02x", index, rx_data[index]);
                }

                /* Start rx next time. */
                can_send_remote_auto_receive_reply();
            }
        }
    }

    if (SET == CAN_GetINTStatus(CAN0, CAN_TX_INT_FLAG))
    {
        CAN_ClearINTPendingBit(CAN0, CAN_TX_INT_FLAG);
        IO_PRINT_INFO0("can_trx_autoreply_handler: CAN TX INT");

        /* Enable autoreply next time. */
        can_receive_remote_auto_tx_reply();

        /* Add user code. */
    }
}

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

