/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file      can_trx.c
* @brief     This file provides all the demo code for CAN bus firmware functions.
* @details
* @author
* @date      2024-02-26
* @version  v1.0
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

#define CAN_RX_FIFO_EN    0
#define CAN_TIME_STAMP_EN 0

#if CAN_TIME_STAMP_EN
#define CAN_TX_TRIGGER_EN 0
#endif

/*============================================================================*
 *                              Variables
 *============================================================================*/

/*============================================================================*
 *                              Functions
 *============================================================================*/
void can_trx_handler(void);

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
    CAN_DeInit();

    /* Enable rcc for CAN before initialize. */
    RCC_PeriphClockCmd(APBPeriph_CAN, APBPeriph_CAN_CLOCK, ENABLE);

    IO_PRINT_INFO1("can_driver_init: BUS state: %d, waiting...", CAN_GetBusState());

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

    init_struct.CAN_FdEn = ENABLE;

    /* can fd speed = 40mhz / ((brp + 1)*(1 + tseg1 + 1 + tseg2 + 1)) */
    /* Here config to 5mhz. */
    init_struct.CAN_FdBitTiming.b.can_fd_brp = 0;
    init_struct.CAN_FdBitTiming.b.can_fd_sjw = 1;
    init_struct.CAN_FdBitTiming.b.can_fd_tseg1 = 1;
    init_struct.CAN_FdBitTiming.b.can_fd_tseg2 = 4;
    init_struct.CAN_FdSspCal.b.can_fd_ssp_auto = ENABLE;
    init_struct.CAN_FdSspCal.b.can_fd_ssp_dco = \
                                                (init_struct.CAN_FdBitTiming.b.can_fd_tseg1 + 2) * (init_struct.CAN_FdBitTiming.b.can_fd_brp + 1);
    init_struct.CAN_FdSspCal.b.can_fd_ssp_min = 0;
    init_struct.CAN_FdSspCal.b.can_fd_ssp = 0;

#if CAN_TIME_STAMP_EN
    init_struct.CAN_TimeStamp.b.can_time_stamp_en = ENABLE;
    init_struct.CAN_TimeStamp.b.can_time_stamp_div = 200;
#endif

#if CAN_RX_FIFO_EN
    init_struct.CAN_RxFifoEn = ENABLE;
#endif

    RamVectorTableUpdate(CAN_VECTORn, (IRQ_Fun)can_trx_handler);

    CAN_Init(&init_struct);

    /* CAN enable */
    CAN_Cmd(ENABLE);

    /* CAN interrupts enable */
    CAN_INTConfig((CAN_BUS_OFF_INT | CAN_WAKE_UP_INT | CAN_ERROR_INT |
                   CAN_RX_INT | CAN_TX_INT), ENABLE);

    /* NVIC enable */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = CAN_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    /* polling CAN bus on status */
    while (CAN_GetBusState() != CAN_BUS_STATE_ON)
    {
        __asm volatile
        (
            "nop    \n"
        );
    }

    IO_PRINT_INFO1("can_driver_init: BUS ON %d", CAN_GetBusState());
}

void can_basic_tx(uint32_t buf_id, CANDataFrameSel_TypeDef frame_type, \
                  uint16_t frame_id, uint32_t ext_id, uint8_t *tx_data, uint8_t data_len)
{
    /* Set CAN Tx message buffer. */
    CANError_TypeDef tx_error;

    CANTxFrame_TypeDef tx_frame_type;

    tx_frame_type.msg_buf_id = buf_id;
    tx_frame_type.frame_type = frame_type;
    tx_frame_type.standard_frame_id = frame_id;
    tx_frame_type.frame_brs_bit = CAN_BRS_NO_SWITCH_BIT_TIMING;
    tx_frame_type.auto_reply_bit = DISABLE;

    switch (frame_type)
    {
    case CAN_EXT_DATA_FRAME:
    case CAN_EXT_REMOTE_FRAME:
        tx_frame_type.extend_frame_id = ext_id;
    case CAN_STD_DATA_FRAME:
    case CAN_STD_REMOTE_FRAME:
        break;
    case CAN_FD_EXT_DATA_FRAME:
        tx_frame_type.extend_frame_id = ext_id;
    case CAN_FD_STD_DATA_FRAME:
        tx_frame_type.frame_brs_bit = CAN_BRS_SWITCH_BIT_TIMING;
        break;
    }

    CAN_MBTxINTConfig(tx_frame_type.msg_buf_id, ENABLE);
    tx_error = CAN_SetMsgBufTxMode(&tx_frame_type, tx_data, data_len);

    while (CAN_GetRamState() != CAN_RAM_STATE_IDLE)
    {
        __asm volatile
        (
            "nop    \n"
        );
    }

    if (tx_error != CAN_NO_ERR)
    {
        IO_PRINT_INFO1("can_basic_tx: tx error %d", tx_error);
    }
}

void can_basic_rx(void)
{
    CANError_TypeDef rx_error;
    CANRxFrame_TypeDef rx_frame_type;
#if CAN_RX_FIFO_EN
    rx_frame_type.msg_buf_id = CAN_MESSAGE_FIFO_START_ID;
#else
    rx_frame_type.msg_buf_id = 7;
#endif

    /* Set 0 to filter related bit, and set 1 to mask related filter. */
    rx_frame_type.extend_frame_id = 0;
    rx_frame_type.standard_frame_id = 0;
    rx_frame_type.frame_rtr_mask = SET;
    rx_frame_type.frame_ide_mask = SET;
    rx_frame_type.frame_id_mask = CAN_FRAME_ID_MASK_MAX_VALUE;
    rx_frame_type.rx_dma_en = RESET;
    rx_frame_type.auto_reply_bit = RESET;
    rx_error = CAN_SetMsgBufRxMode(&rx_frame_type);

    CAN_MBRxINTConfig(rx_frame_type.msg_buf_id, ENABLE);

    while (CAN_GetRamState() != CAN_RAM_STATE_IDLE)
    {
        __asm volatile
        (
            "nop    \n"
        );
    }

    if (rx_error != CAN_NO_ERR)
    {
        IO_PRINT_INFO1("can_basic_rx: rx error %d", rx_error);
    }

    IO_PRINT_INFO0("can_basic_rx: waiting for rx...");
}

void can_trx_demo(void)
{
    IO_PRINT_INFO0("can_trx_demo: start!");

    can_board_init();

    can_driver_init();

    /* Send tx messages. */
    uint8_t tx_data[8];
    for (uint8_t i = 0; i < 8; ++i)
    {
        tx_data[i] = i;
    }

    /* Send standard data frame. */
    IO_PRINT_INFO0("can_trx_demo: send standard data frame");
    can_basic_tx(0, CAN_STD_DATA_FRAME, 0x123, 0, tx_data, 8);

    /* Send extend data frame. */
    IO_PRINT_INFO0("can_trx_demo: send extend data frame");
    can_basic_tx(1, CAN_EXT_DATA_FRAME, 0x123, 0x4567, tx_data, 8);

    /* Send standard remote frame. */
    IO_PRINT_INFO0("can_trx_demo: send standard remote frame");
    can_basic_tx(2, CAN_STD_REMOTE_FRAME, 0x7ff, 0, tx_data, 0);

    /* Send extend remote frame. */
    IO_PRINT_INFO0("can_trx_demo: send extend remote frame");
    can_basic_tx(3, CAN_EXT_REMOTE_FRAME, 0x7ff, 0x3ffff, tx_data, 0);

    /* Send standard remote frame. */
    IO_PRINT_INFO0("can_trx_demo: send FD standard data frame");
    can_basic_tx(4, CAN_FD_STD_DATA_FRAME, 0x321, 0, tx_data, 8);

    /* Send extend remote frame. */
    IO_PRINT_INFO0("can_trx_demo: send FD extend data frame");
    can_basic_tx(5, CAN_FD_EXT_DATA_FRAME, 0x321, 0x3ffff, tx_data, 8);

#if CAN_TX_TRIGGER_EN
    /* Send standard data frame in tx_trigger mode. */
    IO_PRINT_INFO0("can_trx_demo: send standard data frame in tx_trigger mode");

    uint16_t begin_ts = CAN_GetTimeStampCount();
    IO_PRINT_INFO1("can_trx_demo: time_stamp = %d", begin_ts);
    begin_ts += 1000;

    /* Enable Tx trigger function. */
    CAN_TxTriggerConfig(ENABLE, begin_ts, 100);

    can_basic_tx(6, CAN_STD_DATA_FRAME, 0x123, 0, tx_data, 8);
#endif

    can_basic_rx();

    /* Waiting for rx data to generate interrupt. */
}

void can_trx_handler(void)
{
#if CAN_TIME_STAMP_EN
    uint16_t time_stamp = CAN_GetTimeStampCount();
    IO_PRINT_INFO1("can_trx_handler: time_stamp = %d", time_stamp);
#endif

    if (SET == CAN_GetINTStatus(CAN_BUS_OFF_INT_FLAG))
    {
        IO_PRINT_INFO0("can_trx_handler: CAN BUS OFF");
        CAN_ClearINTPendingBit(CAN_BUS_OFF_INT_FLAG);

        /* Add user code. */
    }

    if (SET == CAN_GetINTStatus(CAN_WAKE_UP_INT_FLAG))
    {
        IO_PRINT_INFO0("can_trx_handler: CAN WAKE UP");
        CAN_ClearINTPendingBit(CAN_WAKE_UP_INT_FLAG);

        /* Add user code. */
    }

    if (SET == CAN_GetINTStatus(CAN_ERROR_INT_FLAG))
    {
        IO_PRINT_INFO0("can_trx_handler: CAN ERROR");
        CAN_ClearINTPendingBit(CAN_ERROR_INT_FLAG);

        if (SET == CAN_GetErrorStatus(CAN_ERROR_RX))
        {
            IO_PRINT_INFO0("can_trx_handler: CAN ERROR RX");
            CAN_CLearErrorStatus(CAN_ERROR_RX);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN_ERROR_TX))
        {
            IO_PRINT_INFO0("can_trx_handler: CAN ERROR TX");
            CAN_CLearErrorStatus(CAN_ERROR_TX);

            for (uint8_t index = 0; index < CAN_MESSAGE_BUFFER_MAX_CNT; index++)
            {
                if (SET == CAN_GetMBnTxErrorFlag(index))
                {
                    IO_PRINT_INFO1("can_trx_handler: CAN ERROR TX MB_%d", index);
                    CAN_ClearMBnTxErrorFlag(index);

                    /* Add user code. */
                }
            }
        }

        if (SET == CAN_GetErrorStatus(CAN_ERROR_ACK))
        {
            IO_PRINT_INFO0("can_trx_handler: CAN ERROR ACK");
            CAN_CLearErrorStatus(CAN_ERROR_ACK);
            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN_ERROR_STUFF))
        {
            IO_PRINT_INFO0("can_trx_handler: CAN ERROR STUFF");
            CAN_CLearErrorStatus(CAN_ERROR_STUFF);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN_ERROR_CRC))
        {
            IO_PRINT_INFO0("can_trx_handler: CAN_ERROR_CRC");
            CAN_CLearErrorStatus(CAN_ERROR_CRC);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN_ERROR_FORM))
        {
            IO_PRINT_INFO0("can_trx_handler: CAN ERROR FORM");
            CAN_CLearErrorStatus(CAN_ERROR_FORM);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN_ERROR_BIT1))
        {
            IO_PRINT_INFO0("can_trx_handler: CAN ERROR BIT1");
            CAN_CLearErrorStatus(CAN_ERROR_BIT1);

            /* Add user code. */
        }

        if (SET == CAN_GetErrorStatus(CAN_ERROR_BIT0))
        {
            IO_PRINT_INFO0("can_trx_handler: CAN ERROR BIT0");
            CAN_CLearErrorStatus(CAN_ERROR_BIT0);

            /* Add user code. */
        }
    }

    if (SET == CAN_GetINTStatus(CAN_RX_INT_FLAG))
    {
        CAN_ClearINTPendingBit(CAN_RX_INT_FLAG);

        for (uint8_t index = 0; index < CAN_MESSAGE_BUFFER_MAX_CNT; index++)
        {
            if (SET == CAN_GetMBnRxDoneFlag(index))
            {
                IO_PRINT_INFO1("can_trx_handler: MB_%d rx done", index);
                CAN_ClearMBnRxDoneFlag(index);

#if CAN_RX_FIFO_EN
                if (index == CAN_MESSAGE_FIFO_START_ID)
                {
                    CANFifoStatus_TypeDef fifo_status;
                    CAN_GetFifoStatus(&fifo_status);

                    while (fifo_status.fifo_msg_empty == RESET)
                    {
                        CANMsgBufInfo_TypeDef mb_info;
                        CAN_GetMsgBufInfo(index, &mb_info);

                        uint8_t rx_data[64];
                        memset(rx_data, 0, 64);
                        CAN_GetRamData(mb_info.data_length, rx_data);

                        CANDataFrameSel_TypeDef frame_type = CAN_CheckFrameType(mb_info.rtr_bit, mb_info.ide_bit,
                                                                                mb_info.edl_bit);

                        IO_PRINT_INFO3("can_trx_handler: frame_type %d, frame_id = 0x%x, ext_frame_id = 0x%x", \
                                       frame_type, mb_info.standard_frame_id, mb_info.extend_frame_id);

                        for (uint8_t index = 0; index < mb_info.data_length; index++)
                        {
                            IO_PRINT_INFO2("can_trx_handler: rx_fifo_data [%d] 0x%02X", index, rx_data[index]);
                        }

                        CAN_GetFifoStatus(&fifo_status);

                        /* Send back frame here. */
                        can_basic_tx(0, frame_type, mb_info.standard_frame_id, \
                                     mb_info.extend_frame_id, rx_data, mb_info.data_length);
                    }

                    /* Start rx next time. */
                    can_basic_rx();
                }
                else
#endif
                {
                    CANMsgBufInfo_TypeDef mb_info;
                    CAN_GetMsgBufInfo(index, &mb_info);

                    uint8_t rx_data[64];
                    memset(rx_data, 0, 64);
                    CAN_GetRamData(mb_info.data_length, rx_data);

                    CANDataFrameSel_TypeDef frame_type = CAN_CheckFrameType(mb_info.rtr_bit, mb_info.ide_bit,
                                                                            mb_info.edl_bit);

                    IO_PRINT_INFO3("[can_trx_handler: frame_type %d, frame_id = 0x%03x, ext_frame_id = 0x%05x", \
                                   frame_type, mb_info.standard_frame_id, mb_info.extend_frame_id);

                    for (uint8_t index = 0; index < mb_info.data_length; index++)
                    {
                        IO_PRINT_INFO2("can_trx_handler: rx_data [%d] 0x%02x", index, rx_data[index]);
                    }

                    /* Send back frame here. */
                    can_basic_tx(0, frame_type, mb_info.standard_frame_id, \
                                 mb_info.extend_frame_id, rx_data, mb_info.data_length);

                    /* Start rx next time. */
                    can_basic_rx();
                }
            }
        }
    }

    if (SET == CAN_GetINTStatus(CAN_TX_INT_FLAG))
    {
        CAN_ClearINTPendingBit(CAN_TX_INT_FLAG);

        for (uint8_t index = 0; index < CAN_MESSAGE_BUFFER_MAX_CNT; index++)
        {
            if (SET == CAN_GetMBnTxDoneFlag(index))
            {
                IO_PRINT_INFO1("can_trx_handler: MB_%d tx done", index);
                CAN_ClearMBnTxDoneFlag(index);

                /* Add user code. */
            }
        }
    }
}

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

