/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file      can_loopback.c
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
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"

/*============================================================================*
 *                              Variables
 *============================================================================*/

/*============================================================================*
 *                              Functions
 *============================================================================*/
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

    init_struct.CAN_TestModeSel = CAN_TEST_MODE_LOOP_BACK;

    CAN_Init(&init_struct);

    /* CAN enable */
    CAN_Cmd(ENABLE);

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

void can_loopback(uint32_t buf_id, CANDataFrameSel_TypeDef frame_type, \
                  uint16_t frame_id, uint32_t ext_id, uint8_t *tx_data, uint8_t data_len)
{
    /* Set rx message buffer. */
    CANError_TypeDef rx_error;
    CANRxFrame_TypeDef rx_frame_type;

    rx_frame_type.msg_buf_id = 15;

    /* Set 0 to filter related bit, and set 1 to mask related filter. */
    rx_frame_type.frame_rtr_mask = SET;
    rx_frame_type.frame_ide_mask = SET;
    rx_frame_type.frame_id_mask = CAN_FRAME_ID_MASK_MAX_VALUE;
    rx_frame_type.rx_dma_en = RESET;
    rx_frame_type.auto_reply_bit = RESET;

    rx_error = CAN_SetMsgBufRxMode(&rx_frame_type);

    while (CAN_GetRamState() != CAN_RAM_STATE_IDLE)
    {
        __asm volatile
        (
            "nop    \n"
        );
    }

    if (rx_error != CAN_NO_ERR)
    {
        IO_PRINT_INFO1("can_loopback: rx error %d", rx_error);
    }

    /* Set tx message buffer. */
    CANError_TypeDef tx_error = CAN_NO_ERR;

    CANTxFrame_TypeDef tx_frame_type;

    tx_frame_type.msg_buf_id = buf_id;
    tx_frame_type.frame_type = frame_type;
    tx_frame_type.standard_frame_id = frame_id;
    tx_frame_type.extend_frame_id = 0;
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
        IO_PRINT_INFO1("can_loopback: tx error %d", tx_error);
    }

    /* Polling tx done. */
    while (SET != CAN_GetMBnTxDoneFlag(tx_frame_type.msg_buf_id))
    {
        __asm volatile
        (
            "nop    \n"
        );
    }

    IO_PRINT_INFO0("can_loopback: BUS TX done");

    /* Polling rx done. */
    while (SET != CAN_GetMBnRxDoneFlag(rx_frame_type.msg_buf_id))
    {
        __asm volatile
        (
            "nop    \n"
        );
    }

    /* Receive rx data. */
    CANMsgBufInfo_TypeDef mb_info;
    CAN_GetMsgBufInfo(rx_frame_type.msg_buf_id, &mb_info);

    uint8_t rx_data[64];
    memset(rx_data, 0, 64);
    CAN_GetRamData(mb_info.data_length, rx_data);

    CANDataFrameSel_TypeDef get_frame_type = CAN_CheckFrameType(mb_info.rtr_bit, mb_info.ide_bit,
                                                                mb_info.edl_bit);

    IO_PRINT_INFO1("can_loopback: rx frame_type %d", get_frame_type);
    IO_PRINT_INFO2("can_loopback: rx frame_id = 0x%x, ext_frame_id = 0x%x", mb_info.standard_frame_id,
                   mb_info.extend_frame_id);

    for (uint8_t index = 0; index < mb_info.data_length; index++)
    {
        IO_PRINT_INFO2("can_loopback: rx_data [%d] 0x%02X", index, rx_data[index]);
    }

    IO_PRINT_INFO0("can_loopback: BUS RX done");
}

void can_loopback_demo(void)
{
    IO_PRINT_INFO0("can_loopback_demo: start!");

    can_driver_init();

    /* Send tx messages. */
    /* Set CAN tx message buffer */
    uint8_t tx_data[64];
    for (uint8_t i = 0; i < 64; ++i)
    {
        tx_data[i] = i;
    }

    /* Send standard data frame. */
    IO_PRINT_INFO0("can_loopback_demo: send standard data frame");
    can_loopback(0, CAN_STD_DATA_FRAME, 0x123, 0, tx_data, 8);

    /* Send extend data frame. */
    IO_PRINT_INFO0("can_loopback_demo: send extend data frame");
    can_loopback(1, CAN_EXT_DATA_FRAME, 0x123, 0x4567, tx_data, 8);

    /* Send standard remote frame. */
    IO_PRINT_INFO0("can_loopback_demo: send standard remote frame");
    can_loopback(2, CAN_STD_REMOTE_FRAME, 0x7ff, 0, tx_data, 0);

    /* Send extend remote frame. */
    IO_PRINT_INFO0("can_loopback_demo: send extend remote frame");
    can_loopback(3, CAN_EXT_REMOTE_FRAME, 0x7ff, 0x3ffff, tx_data, 0);

    /* Send fd standard data frame. */
    IO_PRINT_INFO0("can_loopback_demo: send fd standard data frame");
    can_loopback(4, CAN_FD_STD_DATA_FRAME, 0x123, 0, tx_data, 64);

    /* Send fd extend data frame. */
    IO_PRINT_INFO0("can_loopback_demo: send fd extend data frame");
    can_loopback(5, CAN_FD_EXT_DATA_FRAME, 0x123, 0x4567, tx_data, 64);

    IO_PRINT_INFO0("can_loopback_demo: end of can loopback demo");
}

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

