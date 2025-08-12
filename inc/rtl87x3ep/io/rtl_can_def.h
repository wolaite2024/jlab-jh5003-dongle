/**
*****************************************************************************************
*     Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* \file    rtl_can_def.h
* \brief   CAN related definitions for RTL87x3e.
* \author
* \date    2023-11-16
* \version v1.0
* *************************************************************************************
*/

#ifndef RTL_CAN_DEF_H
#define RTL_CAN_DEF_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "rtl876x.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ================================================================================ */
/* ================          CAN Registers_Structures        ================ */
/* ================================================================================ */
/**
  * @brief Analog to digital converter. (CAN)
  */
typedef struct            /*!< CAN Structure */
{
    __IO uint32_t  CAN_CTL;              //!<0X00
    __IO uint32_t  CAN_STS;              //!<0X04
    __IO uint32_t  CAN_FIFO_STS;         //!<0X08
    __IO uint32_t  CAN_BIT_TIMING;       //!<0X0C
    __IO uint32_t  CAN_FD_BIT_TIMING;    //!<0X10
    __IO uint32_t  CAN_FD_SSP_CAL;       //!<0X14
    __IO uint32_t  CAN_INT_EN;           //!<0X18
    __IO uint32_t  CAN_MB_RXINT_EN;      //!<0X1C
    __IO uint32_t  CAN_MB_TXINT_EN;      //!<0X20
    __IO uint32_t  CAN_INT_FLAG;         //!<0X24
    __IO uint32_t  CAN_ERR_STATUS;       //!<0X28
    __IO uint32_t  CAN_ERR_CNT_CTL;      //!<0X2C
    __IO uint32_t  CAN_ERR_CNT_STS;      //!<0X30
    __IO uint32_t  CAN_TX_ERROR_FLAG;    //!<0X34
    __IO uint32_t  CAN_TX_DONE;          //!<0X38
    __IO uint32_t  CAN_RX_DONE;          //!<0X3C
    __IO uint32_t  CAN_TIME_STAMP;       //!<0X40
    __IO uint32_t  CAN_MB_TRIGGER;       //!<0X44
    __IO uint32_t  CAN_RXDMA_MSIZE;      //!<0X48
    __IO uint32_t  CAN_RX_DMA_DATA;      //!<0X4C
    __IO uint32_t  CAN_SLEEP_MODE;       //!<0X50
    __IO uint32_t  CAN_TEST;             //!<0X54
    __IO uint32_t  CAN_MB0_15_STS;       //!<0X100:0x13C
    __IO uint32_t  CAN_MB0_15_CTRL;      //!<0X200:0x23C
    __IO uint32_t  CAN_MB_BA_END;        //!<0X2F0
    __IO uint32_t  CAN_RAM_DATA15_0;     //!<0X300:0x33C
    __IO uint32_t  CAN_RAM_ARB;          //!<0X340
    __IO uint32_t  CAN_RAM_MASK;         //!<0X344
    __IO uint32_t  CAN_RAM_CS;           //!<0X348
    __IO uint32_t  CAN_RAM_CMD;          //!<0X34C
} CAN_TypeDef;

/* ================================================================================ */
/* ================          CAN Register           ================ */
/* ================================================================================ */
/**
  * @brief Analog to digital converter. (CAN)
  */
/* 0x000        CAN_CTL
    0       R/W    can_en                  0x0
    1       R/W/ES bus_on_req              0x0
    2       R/W    can_tri_sample          0x0
    3       R/W    auto_re_tx_en           0x0
    4       R/W    test_mode_en            0x0
    5       R/W    rx_fifo_en              0x0
    6       R/W    can_fd_en               0x0
    7       R/W    can_rxdma_en            0x0
    8       R/W    can_fd_crc_mode         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_en: 1;
        __IO uint32_t bus_on_req: 1;
        __IO uint32_t can_tri_sample: 1;
        __IO uint32_t auto_re_tx_en: 1;
        __IO uint32_t test_mode_en: 1;
        __IO uint32_t rx_fifo_en: 1;
        __IO uint32_t can_fd_en: 1;
        __IO uint32_t can_rxdma_en: 1;
        __IO uint32_t can_fd_crc_mode: 1;
    } b;
} CAN_0x00_TYPE_TypeDef;

/* 0x004        CAN_STS
    0       R      bus_on_state            0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t bus_on_state: 1;
    } b;
} CAN_0x04_TYPE_TypeDef;

/* 0x008        CAN_FIFO_STS
    0       R      fifo_msg_full           0x0
    1       R      fifo_msg_empty          0x0
    2       R      fifo_msg_overflow       0x0
    6:4     R      fifo_msg_lvl            0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t fifo_msg_full: 1;
        __I uint32_t fifo_msg_empty: 1;
        __I uint32_t fifo_msg_overflow: 1;
        __I uint32_t RESERVED_0:  1;
        __I uint32_t fifo_msg_lvl: 3;
    } b;
} CAN_0x08_TYPE_TypeDef;

/* 0x00C        CAN_BIT_TIMING
    7:0     R/W    can_tseg1               0x0
    15:8    R/W    can_tseg2               0x0
    18:16   R/W    can_sjw                 0x0
    31:24   R/W    can_brp                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_tseg1: 8;
        __IO uint32_t can_tseg2: 8;
        __IO uint32_t can_sjw: 3;
        __IO uint32_t RESERVED_0:  5;
        __IO uint32_t can_brp: 8;
    } b;
} CAN_0x0C_TYPE_TypeDef;

/* 0x010        CAN_FD_BIT_TIMING
    7:0     R/W    can_fd_tseg1            0x0
    15:8    R/W    can_fd_tseg2            0x0
    18:16   R/W    can_fd_sjw              0x0
    31:24   R/W    can_fd_brp              0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_fd_tseg1: 8;
        __IO uint32_t can_fd_tseg2: 8;
        __IO uint32_t can_fd_sjw: 3;
        __IO uint32_t RESERVED_0:  5;
        __IO uint32_t can_fd_brp: 8;
    } b;
} CAN_0x10_TYPE_TypeDef;

/* 0x014        CAN_FD_SSP_CAL
    7:0     R/W/ES can_fd_ssp              0x0
    15:8    R/W    can_fd_ssp_min          0x0
    23:16   R/W    can_fd_ssp_dco          0x0
    24      R/W    can_fd_ssp_auto         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_fd_ssp: 8;
        __IO uint32_t can_fd_ssp_min: 8;
        __IO uint32_t can_fd_ssp_dco: 8;
        __IO uint32_t can_fd_ssp_auto: 1;
    } b;
} CAN_0x14_TYPE_TypeDef;

/* 0x018        CAN_INT_EN
    0       R/W    tx_int_en               0x0
    1       R/W    rx_int_en               0x0
    2       R/W    error_int_en            0x0
    3       R/W    wakeup_int_en           0x0
    4       R/W    busoff_int_en           0x0
    5       R/W    ram_move_done_int_en    0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_int_en: 1;
        __IO uint32_t rx_int_en: 1;
        __IO uint32_t error_int_en: 1;
        __IO uint32_t wakeup_int_en: 1;
        __IO uint32_t busoff_int_en: 1;
        __IO uint32_t ram_move_done_int_en: 1;
    } b;
} CAN_0x18_TYPE_TypeDef;

/* 0x01C        CAN_MB_RXINT_EN
    15:0    R/W    can_mb_rxint_en         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_mb_rxint_en: 16;
    } b;
} CAN_0x1C_TYPE_TypeDef;

/* 0x020        CAN_MB_TXINT_EN
    15:0    R/W    can_mb_txint_en         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_mb_txint_en: 16;
    } b;
} CAN_0x20_TYPE_TypeDef;

/* 0x024        CAN_INT_FLAG
    0       RW1C   tx_int_flag             0x0
    1       RW1C   rx_int_flag             0x0
    2       RW1C   error_int_flag          0x0
    3       RW1C   wakeup_int_flag         0x0
    4       RW1C   busoff_int_flag         0x0
    5       RW1C   ram_move_done_int_flag  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_int_flag: 1;
        __IO uint32_t rx_int_flag: 1;
        __IO uint32_t error_int_flag: 1;
        __IO uint32_t wakeup_int_flag: 1;
        __IO uint32_t busoff_int_flag: 1;
        __IO uint32_t ram_move_done_int_flag: 1;
    } b;
} CAN_0x24_TYPE_TypeDef;

/* 0x028        CAN_ERR_STATUS
    0       RW1C   can_error_bit0          0x0
    1       RW1C   can_error_bit1          0x0
    2       RW1C   can_error_form          0x0
    3       RW1C   can_error_crc           0x0
    4       RW1C   can_error_stuff         0x0
    5       RW1C   can_error_ack           0x0
    8       RW1C   can_error_tx            0x0
    9       RW1C   can_error_rx            0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_error_bit0: 1;
        __IO uint32_t can_error_bit1: 1;
        __IO uint32_t can_error_form: 1;
        __IO uint32_t can_error_crc: 1;
        __IO uint32_t can_error_stuff: 1;
        __IO uint32_t can_error_ack: 1;
        __IO uint32_t RESERVED_0:  2;
        __IO uint32_t can_error_tx: 1;
        __IO uint32_t can_error_rx: 1;
    } b;
} CAN_0x28_TYPE_TypeDef;

/* 0x02C        CAN_ERR_CNT_CTL
    0       WA0    tx_err_cnt_clr          0x0
    1       WA0    rx_err_cnt_clr          0x0
    16:8    R/W    can_error_warn_th       0x60
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_err_cnt_clr: 1;
        __IO uint32_t rx_err_cnt_clr: 1;
        __IO uint32_t RESERVED_0:  6;
        __IO uint32_t can_error_warn_th: 9;
    } b;
} CAN_0x2C_TYPE_TypeDef;

/* 0x030        CAN_ERR_CNT_STS
    8:0     R      can_tec                 0x0
    24:16   R      can_rec                 0x0
    28      R      can_error_passive       0x0
    29      R      can_error_busoff        0x0
    30      R      can_error_warning       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t can_tec: 9;
        __I uint32_t RESERVED_1:  7;
        __I uint32_t can_rec: 9;
        __I uint32_t RESERVED_0:  3;
        __I uint32_t can_error_passive: 1;
        __I uint32_t can_error_busoff: 1;
        __I uint32_t can_error_warning: 1;
    } b;
} CAN_0x30_TYPE_TypeDef;

/* 0x034        CAN_TX_ERROR_FLAG
    15:0    RW1CB  can_tx_error_flag       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_tx_error_flag: 16;
    } b;
} CAN_0x34_TYPE_TypeDef;

/* 0x038        CAN_TX_DONE
    15:0    RW1CB  can_tx_done             0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_tx_done: 16;
    } b;
} CAN_0x38_TYPE_TypeDef;

/* 0x03C        CAN_RX_DONE
    15:0    RW1CB  can_rx_done             0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_rx_done: 16;
    } b;
} CAN_0x3C_TYPE_TypeDef;

/* 0x040        CAN_TIME_STAMP
    15:0    R      can_time_stamp          0x0
    23:16   R/W    can_time_stamp_div      0x0
    31      R/W    can_time_stamp_en       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t can_time_stamp: 16;
        __IO uint32_t can_time_stamp_div: 8;
        __IO uint32_t RESERVED_0:  7;
        __IO uint32_t can_time_stamp_en: 1;
    } b;
} CAN_0x40_TYPE_TypeDef;

/* 0x044        CAN_MB_TRIGGER
    15:0    R/W    tx_trigger_begin        0x0
    23:16   R/W    tx_trigger_close_offset 0x0
    24      R/W    tx_trigger_en           0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_trigger_begin: 16;
        __IO uint32_t tx_trigger_close_offset: 8;
        __IO uint32_t tx_trigger_en: 1;
    } b;
} CAN_0x44_TYPE_TypeDef;

/* 0x048        CAN_RXDMA_MSIZE
    13:0    R      can_rxdma_msize         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t can_rxdma_msize: 14;
    } b;
} CAN_0x48_TYPE_TypeDef;

/* 0x04C        CAN_RX_DMA_DATA
    31:0    RP     can_rx_dma_data         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_rx_dma_data: 32;
    } b;
} CAN_0x4C_TYPE_TypeDef;

/* 0x050        CAN_SLEEP_MODE
    7:0     R/W    can_wakepin_flt_length  0x0
    8       R/W    can_wakepin_flt_en      0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_wakepin_flt_length: 8;
        __IO uint32_t can_wakepin_flt_en: 1;
    } b;
} CAN_0x50_TYPE_TypeDef;

/* 0x054        CAN_TEST
    0       R/W    can_lpb_en              0x0
    1       R/W    can_silence_en          0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_lpb_en: 1;
        __IO uint32_t can_silence_en: 1;
    } b;
} CAN_0x54_TYPE_TypeDef;

/* 0x100:0x13C        CAN_MB0_STS:CAN_MB15_STS
    0       R      can_msg_tx_req         0x0
    1       R      can_msg_tx_done        0x0
    2       R      can_msg_rx_rdy         0x0
    3       R      can_msg_rx_vld         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t can_msg_tx_req: 1;
        __I uint32_t can_msg_tx_done: 1;
        __I uint32_t can_msg_rx_rdy: 1;
        __I uint32_t can_msg_rx_vld: 1;
    } b;
} CAN_0x100_0x13C_TYPE_TypeDef;

/* 0x200:0x23C        CAN_MB0_CTRL:CAN_MB15_CTRL
    8:0     R/W    can_msg@_ba             0x0
    24      R/W/ES can_msg@_rxdma_en       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_msg_ba: 9;
        __IO uint32_t RESERVED_0:  15;
        __IO uint32_t can_msg_rxdma_en: 1;
    } b;
} CAN_0x200_0x23C_TYPE_TypeDef;

/* 0x300:0x33C        CAN_RAM_FDDATA_15:CAN_RAM_DATA_0
    7:0     R/W/ES can_ram_data0          0x0
    15:8    R/W/ES can_ram_data1          0x0
    23:16   R/W/ES can_ram_data2          0x0
    31:24   R/W/ES can_ram_data3          0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_ram_data0: 8;
        __IO uint32_t can_ram_data1: 8;
        __IO uint32_t can_ram_data2: 8;
        __IO uint32_t can_ram_data3: 8;
    } b;
} CAN_0x300_0x33C_TYPE_TypeDef;

/* 0x340        CAN_RAM_ARB
    28:0    R/W/ES can_ram_id              0x0
    29      R/W/ES can_ram_ide             0x0
    30      R/W/ES can_ram_rtr             0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_ram_id: 29;
        __IO uint32_t can_ram_ide: 1;
        __IO uint32_t can_ram_rtr: 1;
    } b;
} CAN_0x340_TYPE_TypeDef;

/* 0x344        CAN_RAM_MASK
    28:0    R/W/ES can_ram_id_mask         0x0
    29      R/W/ES can_ram_ide_mask        0x0
    30      R/W/ES can_ram_rtr_mask        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_ram_id_mask: 29;
        __IO uint32_t can_ram_ide_mask: 1;
        __IO uint32_t can_ram_rtr_mask: 1;
    } b;
} CAN_0x344_TYPE_TypeDef;

/* 0x348        CAN_RAM_CS
    3:0     R/W/ES can_ram_dlc             0x0
    4       R/W/ES can_ram_lost            0x0
    5       R/W/ES can_ram_rxtx            0x0
    6       R/W/ES can_ram_autoreply       0x0
    8       R/W/ES can_ram_edl             0x0
    9       R/W/ES can_ram_brs             0x0
    10      R/W/ES can_ram_esi             0x0
    31:16   R/W/ES can_ram_timestamp       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_ram_dlc: 4;
        __IO uint32_t can_ram_lost: 1;
        __IO uint32_t can_ram_rxtx: 1;
        __IO uint32_t can_ram_autoreply: 1;
        __IO uint32_t RESERVED_1:  1;
        __IO uint32_t can_ram_edl: 1;
        __IO uint32_t can_ram_brs: 1;
        __IO uint32_t can_ram_esi: 1;
        __IO uint32_t RESERVED_0:  5;
        __IO uint32_t can_ram_timestamp: 16;
    } b;
} CAN_0x348_TYPE_TypeDef;

/* 0x34C        CAN_RAM_CMD
    7:0     R/W    can_ram_acc_num         0x0
    8       R/W    can_ram_acc_mask        0x0
    9       R/W    can_ram_acc_cs          0x0
    10      R/W    can_ram_acc_arb         0x0
    11:26   R/W    can_ram_acc_data       0x0
    29      R/W    can_ram_buffer_en       0x0
    30      R/W    can_ram_dir             0x0
    31      R/W/ES can_ram_start           0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t can_ram_acc_num: 8;
        __IO uint32_t can_ram_acc_mask: 1;
        __IO uint32_t can_ram_acc_cs: 1;
        __IO uint32_t can_ram_acc_arb: 1;
        __IO uint32_t can_ram_acc_data: 16;
        __IO uint32_t RESERVED_0:  2;
        __IO uint32_t can_ram_buffer_en: 1;
        __IO uint32_t can_ram_dir: 1;
        __IO uint32_t can_ram_start: 1;
    } b;
} CAN_0x34C_TYPE_TypeDef;


typedef struct
{
    union
    {
        __IO uint32_t CAN_CTL;
        CAN_0x00_TYPE_TypeDef BITS_00;
    } u_00;

    union
    {
        __IO uint32_t CAN_STS;
        CAN_0x04_TYPE_TypeDef BITS_04;
    } u_04;

    union
    {
        __IO uint32_t CAN_FIFO_STS;
        CAN_0x08_TYPE_TypeDef BITS_08;
    } u_08;

    union
    {
        __IO uint32_t CAN_BIT_TIMING;
        CAN_0x0C_TYPE_TypeDef BITS_0C;
    } u_0C;

    union
    {
        __IO uint32_t CAN_FD_BIT_TIMING;
        CAN_0x10_TYPE_TypeDef BITS_10;
    } u_10;

    union
    {
        __IO uint32_t CAN_FD_SSP_CAL;
        CAN_0x14_TYPE_TypeDef BITS_14;
    } u_14;

    union
    {
        __IO uint32_t CAN_INT_EN;
        CAN_0x18_TYPE_TypeDef BITS_18;
    } u_18;

    union
    {
        __IO uint32_t CAN_MB_RXINT_EN;
        CAN_0x1C_TYPE_TypeDef BITS_1C;
    } u_1C;

    union
    {
        __IO uint32_t CAN_MB_TXINT_EN;
        CAN_0x20_TYPE_TypeDef BITS_20;
    } u_20;

    union
    {
        __IO uint32_t CAN_INT_FLAG;
        CAN_0x24_TYPE_TypeDef BITS_24;
    } u_24;

    union
    {
        __IO uint32_t CAN_ERR_STATUS;
        CAN_0x28_TYPE_TypeDef BITS_28;
    } u_28;

    union
    {
        __IO uint32_t CAN_ERR_CNT_CTL;
        CAN_0x2C_TYPE_TypeDef BITS_2C;
    } u_2C;

    union
    {
        __IO uint32_t CAN_ERR_CNT_STS;
        CAN_0x30_TYPE_TypeDef BITS_30;
    } u_30;

    union
    {
        __IO uint32_t CAN_TX_ERROR_FLAG;
        CAN_0x34_TYPE_TypeDef BITS_34;
    } u_34;

    union
    {
        __IO uint32_t CAN_TX_DONE;
        CAN_0x38_TYPE_TypeDef BITS_38;
    } u_38;

    union
    {
        __IO uint32_t CAN_RX_DONE;
        CAN_0x3C_TYPE_TypeDef BITS_3C;
    } u_3C;

    union
    {
        __IO uint32_t CAN_TIME_STAMP;
        CAN_0x40_TYPE_TypeDef BITS_40;
    } u_40;

    union
    {
        __IO uint32_t CAN_MB_TRIGGER;
        CAN_0x44_TYPE_TypeDef BITS_44;
    } u_44;

    union
    {
        __IO uint32_t CAN_RXDMA_MSIZE;
        CAN_0x48_TYPE_TypeDef BITS_48;
    } u_48;

    union
    {
        __IO uint32_t CAN_RX_DMA_DATA;
        CAN_0x4C_TYPE_TypeDef BITS_4C;
    } u_4C;

    union
    {
        __IO uint32_t CAN_SLEEP_MODE;
        CAN_0x50_TYPE_TypeDef BITS_50;
    } u_50;

    union
    {
        __IO uint32_t CAN_TEST;
        CAN_0x54_TYPE_TypeDef BITS_54;
    } u_54;
} CAN_BASE_TypeDef;

typedef struct
{
    union
    {
        __IO uint32_t CAN_MB0_STS;
        CAN_0x100_0x13C_TYPE_TypeDef u_100;
    } u_100;

    union
    {
        __IO uint32_t CAN_MB1_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_104;
    } u_104;

    union
    {
        __IO uint32_t CAN_MB2_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_108;
    } u_108;

    union
    {
        __IO uint32_t CAN_MB3_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_10C;
    } u_10C;

    union
    {
        __IO uint32_t CAN_MB4_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_110;
    } u_110;

    union
    {
        __IO uint32_t CAN_MB5_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_114;
    } u_114;

    union
    {
        __IO uint32_t CAN_MB6_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_118;
    } u_118;

    union
    {
        __IO uint32_t CAN_MB7_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_11C;
    } u_11C;

    union
    {
        __IO uint32_t CAN_MB8_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_120;
    } u_120;

    union
    {
        __IO uint32_t CAN_MB9_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_124;
    } u_124;

    union
    {
        __IO uint32_t CAN_MB10_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_128;
    } u_128;

    union
    {
        __IO uint32_t CAN_MB11_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_12C;
    } u_12C;

    union
    {
        __IO uint32_t CAN_MB12_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_130;
    } u_130;

    union
    {
        __IO uint32_t CAN_MB13_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_134;
    } u_134;

    union
    {
        __IO uint32_t CAN_MB14_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_138;
    } u_138;

    union
    {
        __IO uint32_t CAN_MB15_STS;
        CAN_0x100_0x13C_TYPE_TypeDef BITS_13C;
    } u_13C;
} CAN_MB_STS_TypeDef;

typedef struct
{
    union
    {
        __IO uint32_t CAN_MB0_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_200;
    } u_200;

    union
    {
        __IO uint32_t CAN_MB1_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_204;
    } u_204;

    union
    {
        __IO uint32_t CAN_MB2_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_208;
    } u_208;

    union
    {
        __IO uint32_t CAN_MB3_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_20C;
    } u_20C;

    union
    {
        __IO uint32_t CAN_MB4_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_210;
    } u_210;

    union
    {
        __IO uint32_t CAN_MB5_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_214;
    } u_214;

    union
    {
        __IO uint32_t CAN_MB6_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_218;
    } u_218;

    union
    {
        __IO uint32_t CAN_MB7_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_21C;
    } u_21C;

    union
    {
        __IO uint32_t CAN_MB8_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_220;
    } u_220;

    union
    {
        __IO uint32_t CAN_MB9_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_224;
    } u_224;

    union
    {
        __IO uint32_t CAN_MB10_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_228;
    } u_228;

    union
    {
        __IO uint32_t CAN_MB11_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_22C;
    } u_22C;

    union
    {
        __IO uint32_t CAN_MB12_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_230;
    } u_230;

    union
    {
        __IO uint32_t CAN_MB13_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_234;
    } u_234;

    union
    {
        __IO uint32_t CAN_MB14_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_238;
    } u_238;

    union
    {
        __IO uint32_t CAN_MB15_CTRL;
        CAN_0x200_0x23C_TYPE_TypeDef BITS_23C;
    } u_23C;
} CAN_MB_CTRL_TypeDef;

typedef struct
{
    __IO uint32_t CAN_RAM_DATA[16];

    union
    {
        __IO uint32_t CAN_RAM_ARB;
        CAN_0x340_TYPE_TypeDef BITS_340;
    } u_340;

    union
    {
        __IO uint32_t CAN_RAM_MASK;
        CAN_0x344_TYPE_TypeDef BITS_344;
    } u_344;

    union
    {
        __IO uint32_t CAN_RAM_CS;
        CAN_0x348_TYPE_TypeDef BITS_348;
    } u_348;

    union
    {
        __IO uint32_t CAN_RAM_CMD;
        CAN_0x34C_TYPE_TypeDef BITS_34C;
    } u_34C;
} CAN_RAM_TypeDef;

/* CAN reg */
/*
    because CAN register must access by word
    in order to prevent misoperating,
    we use the similar write approach as fast aon registers
*/
#define CAN_REG_00                0x000
#define CAN_REG_04                0x004
#define CAN_REG_08                0x008
#define CAN_REG_0C                0x00C
#define CAN_REG_10                0x010
#define CAN_REG_14                0x014
#define CAN_REG_18                0x018
#define CAN_REG_20                0x020
#define CAN_REG_24                0x024
#define CAN_REG_28                0x028
#define CAN_REG_2C                0x02C
#define CAN_REG_30                0x030
#define CAN_REG_34                0x034
#define CAN_REG_38                0x038
#define CAN_REG_3C                0x03C
#define CAN_REG_40                0x040
#define CAN_REG_44                0x044
#define CAN_REG_48                0x048
#define CAN_REG_4C                0x04C
#define CAN_REG_50                0x050
#define CAN_REG_54                0x054
#define CAN_REG_100               0x100
#define CAN_REG_200               0x200
#define CAN_REG_2F0               0x2F0
#define CAN_REG_300               0x300
#define CAN_REG_33C               0x33C
#define CAN_REG_340               0x340
#define CAN_REG_344               0x344
#define CAN_REG_348               0x348
#define CAN_REG_34C               0x34C
#define CAN_REG_3FC               0x3FC


#define CAN                       ((CAN_BASE_TypeDef        *) CAN_BASE)
#define CAN_RAM                   ((CAN_RAM_TypeDef     *) (CAN_BASE + CAN_REG_300))

#define CAN_RX_DMA_DATA           (CAN_BASE + CAN_REG_4C)
#define CAN_MB_STS                (CAN_BASE + CAN_REG_100)
#define CAN_MB_CTRL               (CAN_BASE + CAN_REG_200)
#define CAN_MB_BA_END             (CAN_BASE + CAN_REG_2F0)
#define CAN_RAM_DATA0             (CAN_BASE + CAN_REG_33C)

#define can_reg_read32(offset)           HAL_READ32(CAN_BASE, offset)
#define can_reg_write32(offset, value)   HAL_WRITE32(CAN_BASE, offset, value)

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* RTL_CAN_DEF_H */

