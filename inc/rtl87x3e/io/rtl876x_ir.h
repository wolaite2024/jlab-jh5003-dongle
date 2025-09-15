/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_ir.h
* @brief     The header file of IR driver.
* @details
* @author    elliot chen
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _RTL876x_IR_H_
#define _RTL876x_IR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"

/* Peripheral: IR */
/* Description: IR register defines */

/* Register: TX_CONFIG -------------------------------------------------------*/
/* Description: IR TX CONFIG register. Offset: 0x04. Address: 0x40003004. */

/* TX_CONFIG[31] :IR_MODE_SEL. 0x1: RX mode. 0x0: TX mode. */
#define IR_MODE_SEL_Pos                 (31UL)
#define IR_MODE_SEL_Msk                 (0x1UL << IR_MODE_SEL_Pos)
#define IR_MODE_SEL_CLR                 (~IR_MODE_SEL_Msk)
/* TX_CONFIG[30] :IR_TX_START. 0x1: Start TX. 0x0: Stop TX. */
#define IR_TX_START_Pos                 (30UL)
#define IR_TX_START_Msk                 (0x1UL << IR_TX_START_Pos)
#define IR_TX_START_CLR                 (~IR_TX_START_Msk)
/* TX_CONFIG[16] :IR_TX_DUTY_NUM. Duty cycle setting for modulation frequency. */
/* Example : for 1/3 duty cycle, IR_DUTY_NUM = (IR_DIV_NUM+1)/3 -1. */
#define IR_TX_DUTY_NUM_Pos              (16UL)
#define IR_TX_DUTY_NUM_Msk              (0xFFFUL << IR_TX_DUTY_NUM_Pos)
/* TX_CONFIG[14] :IR_OUTPUT_INVERSE. 0x1: Inverse  active output. 0x0: Not inverse active output. */
#define IR_OUTPUT_INVERSE_Pos           (14UL)
#define IR_OUTPUT_INVERSE_Msk           (0x1UL << IR_OUTPUT_INVERSE_Pos)
#define IR_OUTPUT_INVERSE_CLR           (~IR_OUTPUT_INVERSE_Msk)
/* TX_CONFIG[13] :IR_FIFO_INVERSE. 0x1: Inverse FIFO define. 0x0: Not inverse FIFO define. */
#define IR_FIFO_INVERSE_Pos             (13UL)
#define IR_FIFO_INVERSE_Msk             (0x1UL << IR_FIFO_INVERSE_Pos)
#define IR_FIFO_INVERSE_CLR             (~IR_FIFO_INVERSE_Msk)
/* TX_CONFIG[8] :IR_TX_FIFO_THRESHOLD. TX FIFO interrupt threshold. when TX FIFO offset <= threshold value, trigger interrupt. */
#define IR_TX_FIFO_THRESHOLD_Pos        (8UL)
#define IR_TX_FIFO_THRESHOLD_Msk        (0x1fUL << IR_TX_FIFO_THRESHOLD_Pos)
#define IR_TX_FIFO_THRESHOLD_CLR        (~IR_TX_FIFO_THRESHOLD_Msk)
/* TX_CONFIG[6] :IR_TX_IDLE_STATE. TX output State in idle. 0x1: High. 0x0: Low. */
#define IR_TX_IDLE_STATE_Pos            (6UL)
#define IR_TX_IDLE_STATE_Msk            (0x1UL << IR_TX_IDLE_STATE_Pos)
#define IR_TX_IDLE_STATE_CLR            (~IR_TX_IDLE_STATE_Msk)
/* TX_CONFIG[5] :IR_TX_FIFO_OVER_INT_MASK. TX FIFO empty Interrupt. 0x1: Mask. 0x0: Unmask. */
#define IR_TX_FIFO_OVER_INT_MASK_Pos    (5UL)
#define IR_TX_FIFO_OVER_INT_MASK_Msk    (0x1UL << IR_TX_FIFO_OVER_INT_MASK_Pos)
#define IR_TX_FIFO_OVER_INT_MASK_CLR    (~IR_TX_FIFO_OVER_INT_MASK_Msk)
/* TX_CONFIG[4] :IR_TX_FIFO_OVER_INT_EN. TX FIFO overflow Interrupt. 0x1: Enable. 0x0: Disable. */
#define IR_TX_FIFO_OVER_INT_EN_Pos      (4UL)
#define IR_TX_FIFO_OVER_INT_EN_Msk      (0x1UL << IR_TX_FIFO_OVER_INT_EN_Pos)
#define IR_TX_FIFO_OVER_INT_EN_CLR      (~IR_TX_FIFO_OVER_INT_EN_Msk)
#define IR_TX_FIFO_OVER_MSK_TO_EN_Pos   (IR_TX_FIFO_OVER_INT_MASK_Pos - IR_TX_FIFO_OVER_INT_EN_Pos)
/* TX_CONFIG[3] :IR_TX_FIFO_LEVEL_INT_MASK. TX FIFO threshold Interrupt. 0x1: Mask. 0x0: Unmask. */
#define IR_TX_FIFO_LEVEL_INT_MASK_Pos   (3UL)
#define IR_TX_FIFO_LEVEL_INT_MASK_Msk   (0x1UL << IR_TX_FIFO_LEVEL_INT_MASK_Pos)
#define IR_TX_FIFO_LEVEL_INT_MASK_CLR   (~IR_TX_FIFO_LEVEL_INT_MASK_Msk)
/* TX_CONFIG[2] :IR_TX_FIFO_EMPTY_INT_MASK. TX FIFO empty Interrupt. 0x1: Mask. 0x0: Unmask. */
#define IR_TX_FIFO_EMPTY_INT_MASK_Pos   (2UL)
#define IR_TX_FIFO_EMPTY_INT_MASK_Msk   (0x1UL << IR_TX_FIFO_EMPTY_INT_MASK_Pos)
#define IR_TX_FIFO_EMPTY_INT_MASK_CLR   (~IR_TX_FIFO_EMPTY_INT_MASK_Msk)
/* TX_CONFIG[1] :IR_TX_FIFO_LEVEL_INT_EN. TX FIFO threshold Interrupt. 0x1: Enable. 0x0: Disable. */
#define IR_TX_FIFO_LEVEL_INT_EN_Pos     (1UL)
#define IR_TX_FIFO_LEVEL_INT_EN_Msk     (0x1UL << IR_TX_FIFO_LEVEL_INT_EN_Pos)
#define IR_TX_FIFO_LEVEL_INT_EN_CLR     (~IR_TX_FIFO_LEVEL_INT_EN_Msk)
/* TX_CONFIG[0] :IR_TX_FIFO_EMPTY_INT_EN. TX FIFO empty Interrupt. 0x1: Enable. 0x0: Disable. */
#define IR_TX_FIFO_EMPTY_INT_EN_Pos     (0UL)
#define IR_TX_FIFO_EMPTY_INT_EN_Msk     (0x1UL << IR_TX_FIFO_EMPTY_INT_EN_Pos)
#define IR_TX_FIFO_EMPTY_INT_EN_CLR     (~IR_TX_FIFO_EMPTY_INT_EN_Msk)
#define IR_TX_MSK_TO_EN_Pos             (IR_TX_FIFO_EMPTY_INT_MASK_Pos - IR_TX_FIFO_EMPTY_INT_EN_Pos)
#define IR_TX_STATUS_TO_EN_Pos          (IR_TX_FIFO_OVER_INT_EN_Pos - IR_TX_FIFO_EMPTY_INT_MASK_Pos)

/* Register: TX_SR -----------------------------------------------------------*/
/* Description: TX_SR register. Offset: 0x08. Address: 0x40003008. */

/* TX_SR[15] :IR_TX_FIFO_EMPTY. 0x1: empty. 0x0: not empty. */
#define IR_TX_FIFO_EMPTY_Pos            (15UL)
#define IR_TX_FIFO_EMPTY_Msk            (0x1UL << IR_TX_FIFO_EMPTY_Pos)
/* TX_SR[14] :IR_TX_FIFO_FULL. 0x1: full. 0x0: not full. */
#define IR_TX_FIFO_FULL_Pos             (14UL)
#define IR_TX_FIFO_FULL_Msk             (0x1UL << IR_TX_FIFO_FULL_Pos)
/* TX_SR[8] :IR_TX_FIFO_OFFSET. */
#define IR_TX_FIFO_OFFSET_Pos           (8UL)
#define IR_TX_FIFO_OFFSET_Msk           (0x3fUL << IR_TX_FIFO_OFFSET_Pos)
/* TX_SR[4] :IR_TX_STATUS. */
#define IR_TX_STATUS_Pos                (4UL)
#define IR_TX_STATUS_Msk                (0x1UL << IR_TX_STATUS_Pos)
/* TX_SR[2] :IR_TX_FIFO_OVER_INT_STATUS. */
#define IR_TX_FIFO_OVER_INT_STATUS_Pos  (2UL)
#define IR_TX_FIFO_OVER_INT_STATUS_Msk  (0x1UL << IR_TX_FIFO_OVER_INT_STATUS_Pos)
/* TX_SR[1] :IR_TX_FIFO_LEVEL_INT_STATUS. */
#define IR_TX_FIFO_LEVEL_INT_STATUS_Pos (1UL)
#define IR_TX_FIFO_LEVEL_INT_STATUS_Msk (0x1UL << IR_TX_FIFO_LEVEL_INT_STATUS_Pos)
/* TX_SR[0] :IR_TX_FIFO_EMPTY_INT_STATUS. */
#define IR_TX_FIFO_EMPTY_INT_STATUS_Pos (0UL)
#define IR_TX_FIFO_EMPTY_INT_STATUS_Msk (0x1UL << IR_TX_FIFO_EMPTY_INT_STATUS_Pos)

/* Register: TX_INT_CLR -----------------------------------------------------------*/
/* Description: TX_INT_CLR register. Offset: 0x10. Address: 0x40003010. */

/* TX_INT_CLR[3] :IR_TX_FIFO_OVER_INT_CLR. Write 1 clear. */
#define IR_TX_FIFO_OVER_INT_CLR_Pos     (3UL)
#define IR_TX_FIFO_OVER_INT_CLR_Msk     (0x1UL << IR_TX_FIFO_OVER_INT_CLR_Pos)
/* TX_INT_CLR[2] :IR_TX_FIFO_LEVEL_INT_CLR. Write 1 clear. */
#define IR_TX_FIFO_LEVEL_INT_CLR_Pos    (2UL)
#define IR_TX_FIFO_LEVEL_INT_CLR_Msk    (0x1UL << IR_TX_FIFO_LEVEL_INT_CLR_Pos)
/* TX_INT_CLR[1] :IR_TX_FIFO_EMPTY_INT_CLR. Write 1 clear. */
#define IR_TX_FIFO_EMPTY_INT_CLR_Pos    (1UL)
#define IR_TX_FIFO_EMPTY_INT_CLR_Msk    (0x1UL << IR_TX_FIFO_EMPTY_INT_CLR_Pos)
/* TX_INT_CLR[0] :IR_TX_FIFO_CLR. Write 1 clear. */
#define IR_TX_FIFO_CLR_Pos              (0UL)
#define IR_TX_FIFO_CLR_Msk              (0x1UL << IR_TX_FIFO_CLR_Pos)

#define IR_INT_ALL_CLR                  (IR_TX_FIFO_OVER_INT_CLR_Msk | \
                                         IR_TX_FIFO_LEVEL_INT_CLR_Msk | \
                                         IR_TX_FIFO_EMPTY_INT_CLR_Msk)

/* Register: TX_FIFO ---------------------------------------------------------*/
/* Description: TX_FIFO register. Offset: 0x14. Address: 0x40003014. */

/* TX_FIFO[31] :IR_DATA_TYPE. 0x1: active carrier. 0x0: incative carrier. */
#define IR_DATA_TYPE_Pos                (31UL)
#define IR_DATA_TYPE_Msk                (0x1UL << IR_DATA_TYPE_Pos)
#define IR_DATA_TYPE_CLR                (~IR_DATA_TYPE_Msk)
/* TX_FIFO[30] :IR_TX_LAST_PACKEET. 0x1: last packet. 0x0: normal packet. */
#define IR_TX_LAST_PACKEET_Pos          (30UL)
#define IR_TX_LAST_PACKEET_Msk          (0x1UL << IR_TX_LAST_PACKEET_Pos)
#define IR_TX_LAST_PACKEET_CLR          (~IR_TX_LAST_PACKEET_Msk)
/* TX_FIFO[29:28] :IR_LOW_PERIOD_COMPENSATION */
#define IR_LOW_PERIOD_COMPENSATION_Pos  (27UL)
#define IR_LOW_PERIOD_COMPENSATION_Msk  (0x3UL << IR_LOW_PERIOD_COMPENSATION_Pos)
#define IR_LOW_PERIOD_COMPENSATION_CLR  (~IR_LOW_PERIOD_COMPENSATION_Msk)

/* Register: RX_CONFIG ------------------------------------------------------*/
/* Description: IR RX CONFIG register. Offset: 0x18. Address: 0x40003018. */

/* RX_CONFIG[28] :IR_RX_START. 0x1: Run. 0x0: Stop. */
#define IR_RX_START_Pos                 (28UL)
#define IR_RX_START_Msk                 (0x1UL << IR_RX_START_Pos)
#define IR_RX_START_CLR                 (~IR_RX_START_Msk)
/* RX_CONFIG[27] :IR_RX_START_MODE. 0x1: auto mode. 0x0: manual mode. */
#define IR_RX_START_MODE_Pos            (27UL)
#define IR_RX_START_MODE_Msk            (0x1UL << IR_RX_START_MODE_Pos)
#define IR_RX_START_MODE_CLR            (~IR_RX_START_MODE_Msk)
/* RX_CONFIG[26] :IR_RX_MAN_START. 0x1: Start check waveform. */
#define IR_RX_MAN_START_Pos             (26UL)
#define IR_RX_MAN_START_Msk             (0x1UL << IR_RX_MAN_START_Pos)
#define IR_RX_MAN_START_CLR             (~IR_RX_MAN_START_Msk)
/* RX_CONFIG[24] :IR_TRIGGER_MODE. 0x1: Run. */
/* 0x0: high->low  trigger. 0x1: low->high trigger. 0x02: high->low  or low->high trigger. */
#define IR_TRIGGER_MODE_Pos             (24UL)
#define IR_TRIGGER_MODE_Msk             (0x3UL << IR_TRIGGER_MODE_Pos)
#define IR_TRIGGER_MODE_CLR             (~IR_TRIGGER_MODE_Msk)
/* RX_CONFIG[21] :IR_FILTER_TIME. */
#define IR_FILTER_TIME_Pos              (21UL)
#define IR_FILTER_TIME_Msk              (0x3UL << IR_FILTER_TIME_Pos)
#define IR_FILTER_TIME_CLR              (~IR_FILTER_TIME_Msk)
/* RX_CONFIG[19] :IR_RX_FIFO_ERROR_MASK_INT. 0x1: mask. 0x0: unmask.*/
#define IR_RX_FIFO_ERROR_MASK_INT_Pos   (19UL)
#define IR_RX_FIFO_ERROR_MASK_INT_Msk   (0x1UL << IR_RX_FIFO_ERROR_MASK_INT_Pos)
#define IR_RX_FIFO_ERROR_MASK_INT_CLR   (~IR_RX_FIFO_ERROR_MASK_INT_Msk)
/* RX_CONFIG[18] :IR_RX_CNT_THR_MASK_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_CNT_THR_MASK_INT_Pos      (18UL)
#define IR_RX_CNT_THR_MASK_INT_Msk      (0x1UL << IR_RX_CNT_THR_MASK_INT_Pos)
#define IR_RX_CNT_THR_MASK_INT_CLR      (~IR_RX_CNT_THR_MASK_INT_Msk)
/* RX_CONFIG[17] :IR_RX_FIFO_OF_MASK_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_FIFO_OF_MASK_INT_Pos      (17UL)
#define IR_RX_FIFO_OF_MASK_INT_Msk      (0x1UL << IR_RX_FIFO_OF_MASK_INT_Pos)
#define IR_RX_FIFO_OF_MASK_INT_CLR      (~IR_RX_FIFO_OF_MASK_INT_Msk)
/* RX_CONFIG[16] :IR_RX_CNT_OF_MASK_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_CNT_OF_MASK_INT_Pos       (16UL)
#define IR_RX_CNT_OF_MASK_INT_Msk       (0x1UL << IR_RX_CNT_OF_MASK_INT_Pos)
#define IR_RX_CNT_OF_MASK_INT_CLR       (~IR_RX_CNT_OF_MASK_INT_Msk)
/* RX_CONFIG[15] :IR_RX_FIFO_LEVEL_MASK_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_FIFO_LEVEL_MASK_INT_Pos   (15UL)
#define IR_RX_FIFO_LEVEL_MASK_INT_Msk   (0x1UL << IR_RX_FIFO_LEVEL_MASK_INT_Pos)
#define IR_RX_FIFO_LEVEL_MASK_INT_CLR   (~IR_RX_FIFO_LEVEL_MASK_INT_Msk)
/* RX_CONFIG[14] :IR_RX_FIFO_FULL_MASK_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_FIFO_FULL_MASK_INT_Pos    (14UL)
#define IR_RX_FIFO_FULL_MASK_INT_Msk    (0x1UL << IR_RX_FIFO_FULL_MASK_INT_Pos)
#define IR_RX_FIFO_FULL_MASK_INT_CLR    (~IR_RX_FIFO_FULL_MASK_INT_Msk)
/* RX_CONFIG[13] :IR_RX_FIFO_DISCARD_SET. 0x1: reject new data send to FIFO. 0x0: discard oldest data in FIFO.*/
#define IR_RX_FIFO_DISCARD_SET_Pos      (13UL)
#define IR_RX_FIFO_DISCARD_SET_Msk      (0x1UL << IR_RX_FIFO_DISCARD_SET_Pos)
#define IR_RX_FIFO_DISCARD_SET_CLR      (~IR_RX_FIFO_DISCARD_SET_Msk)
/* RX_CONFIG[8] :IR_RX_FIFO_LEVE. */
#define IR_RX_FIFO_LEVEL_Pos            (8UL)
#define IR_RX_FIFO_LEVEL_Msk            (0x1fUL << IR_RX_FIFO_LEVEL_Pos)
#define IR_RX_FIFO_LEVEL_CLR            (~IR_RX_FIFO_LEVEL_Msk)
/* RX_CONFIG[5] :IR_RX_FIFO_ERROR_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_FIFO_ERROR_INT_Pos        (5UL)
#define IR_RX_FIFO_ERROR_INT_Msk        (0x1UL << IR_RX_FIFO_ERROR_INT_Pos)
#define IR_RX_FIFO_ERROR_INT_CLR        (~IR_RX_FIFO_ERROR_INT_Msk)
/* RX_CONFIG[4] :IR_RX_CNT_THR_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_CNT_THR_INT_Pos           (4UL)
#define IR_RX_CNT_THR_INT_Msk           (0x1UL << IR_RX_CNT_THR_INT_Pos)
#define IR_RX_CNT_THR_INT_CLR           (~IR_RX_CNT_THR_INT_Msk)
/* RX_CONFIG[3] :IR_RX_FIFO_OF_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_FIFO_OF_INT_Pos           (3UL)
#define IR_RX_FIFO_OF_INT_Msk           (0x1UL << IR_RX_FIFO_OF_INT_Pos)
#define IR_RX_FIFO_OF_INT_CLR           (~IR_RX_FIFO_OF_INT_Msk)
/* RX_CONFIG[2] :IR_RX_CNT_OF_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_CNT_OF_INT_Pos            (2UL)
#define IR_RX_CNT_OF_INT_Msk            (0x1UL << IR_RX_CNT_OF_INT_Pos)
#define IR_RX_CNT_OF_INT_CLR            (~IR_RX_CNT_OF_INT_Msk)
/* RX_CONFIG[1] :IR_RX_FIFO_LEVEL_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_FIFO_LEVEL_INT_Pos        (1UL)
#define IR_RX_FIFO_LEVEL_INT_Msk        (0x1UL << IR_RX_FIFO_LEVEL_INT_Pos)
#define IR_RX_FIFO_LEVEL_INT_CLR        (~IR_RX_FIFO_LEVEL_INT_Msk)
/* RX_CONFIG[0] :IR_RX_FIFO_FULL_INT. 0x1: enable. 0x0: disable.*/
#define IR_RX_FIFO_FULL_INT_Pos         (0UL)
#define IR_RX_FIFO_FULL_INT_Msk         (0x1UL << IR_RX_FIFO_FULL_INT_Pos)
#define IR_RX_FIFO_FULL_INT_CLR         (~IR_RX_FIFO_FULL_INT_Msk)
#define IR_RX_MSK_TO_EN_Pos             (IR_RX_FIFO_FULL_MASK_INT_Pos - IR_RX_FIFO_FULL_INT_Pos)

#define IR_RX_MASK_ALL_INT              (IR_RX_FIFO_ERROR_MASK_INT_Msk | IR_RX_CNT_THR_MASK_INT_Msk | \
                                         IR_RX_FIFO_OF_MASK_INT_Msk | IR_RX_CNT_OF_MASK_INT_Msk | \
                                         IR_RX_FIFO_LEVEL_MASK_INT_Msk | IR_RX_FIFO_FULL_MASK_INT_Msk)

/* Register: RX_SR -----------------------------------------------------------*/
/* Description: RX_SR register. Offset: 0x1C. Address: 0x4000301C. */

/* RX_SR[17] :IR_TX_FIFO_EMPTY. 0x1: empty. 0x0: not empty. */
#define IR_RX_FIFO_EMPTY_Pos            (17UL)
#define IR_RX_FIFO_EMPTY_Msk            (0x1UL << IR_RX_FIFO_EMPTY_Pos)
/* RX_SR[8] :IR_RX_FIFO_OFFSET. */
#define IR_RX_FIFO_OFFSET_Pos           (8UL)
#define IR_RX_FIFO_OFFSET_Msk           (0x3fUL << IR_RX_FIFO_OFFSET_Pos)
#define IR_RX_FIFO_OFFSET_CLR           (~IR_RX_FIFO_OFFSET_Msk)
/* RX_SR[7] :IR_RX_STATUS. */
#define IR_RX_STATUS_Pos                (7UL)
#define IR_RX_STATUS_Msk                (0x1UL << IR_RX_STATUS_Pos)

/* Register: RX_INT_CLR -----------------------------------------------------------*/
/* Description: RX_INT_CLR register. Offset: 0x20. Address: 0x40003020. */

/* RX_INT_CLR[8] :IR_RX_FIFO_CLR. Write 1 clear. */
#define IR_RX_FIFO_CLR_Pos              (8UL)
#define IR_RX_FIFO_CLR_Msk              (0x1UL << IR_RX_FIFO_CLR_Pos)
/* RX_INT_CLR[5] :IR_RX_FIFO_ERROR_INT_CLR. Write 1 clear. */
#define IR_RX_FIFO_ERROR_INT_CLR_Pos    (5UL)
#define IR_RX_FIFO_ERROR_INT_CLR_Msk    (0x1UL << IR_RX_FIFO_ERROR_INT_CLR_Pos)
/* RX_INT_CLR[4] :IR_RX_CNT_THR_INT_CLR. Write 1 clear. */
#define IR_RX_CNT_THR_INT_CLR_Pos       (4UL)
#define IR_RX_CNT_THR_INT_CLR_Msk       (0x1UL << IR_RX_CNT_THR_INT_CLR_Pos)
/* RX_INT_CLR[3] :IR_RX_FIFO_OF_INT_CLR. Write 1 clear. */
#define IR_RX_FIFO_OF_INT_CLR_Pos       (3UL)
#define IR_RX_FIFO_OF_INT_CLR_Msk       (0x1UL << IR_RX_FIFO_OF_INT_CLR_Pos)
/* RX_INT_CLR[2] :IR_RX_CNT_OF_INT_CLR. Write 1 clear. */
#define IR_RX_CNT_OF_INT_CLR_Pos        (2UL)
#define IR_RX_CNT_OF_INT_CLR_Msk        (0x1UL << IR_RX_CNT_OF_INT_CLR_Pos)
/* RX_INT_CLR[1] :IR_RX_FIFO_LEVEL_INT_CLR. Write 1 clear. */
#define IR_RX_FIFO_LEVEL_INT_CLR_Pos    (1UL)
#define IR_RX_FIFO_LEVEL_INT_CLR_Msk    (0x1UL << IR_RX_FIFO_LEVEL_INT_CLR_Pos)
/* RX_INT_CLR[0] :IR_RX_FIFO_FULL_INT_CLR. Write 1 clear. */
#define IR_RX_FIFO_FULL_INT_CLR_Pos     (0UL)
#define IR_RX_FIFO_FULL_INT_CLR_Msk     (0x1UL << IR_RX_FIFO_FULL_INT_CLR_Pos)

#define IR_RX_INT_ALL_CLR               (IR_RX_FIFO_CLR_Msk | IR_RX_FIFO_ERROR_INT_CLR_Msk | \
                                         IR_RX_CNT_THR_INT_CLR_Msk | IR_RX_FIFO_OF_INT_CLR_Msk | \
                                         IR_RX_CNT_OF_INT_CLR_Msk | IR_RX_FIFO_LEVEL_INT_CLR_Msk | \
                                         IR_RX_FIFO_FULL_INT_CLR_Msk)

/* Register: RX_CNT_INT_SEL -------------------------------------------------*/
/* Description: IR RX counter interrupt setting register. Offset: 0x24. Address: 0x40003024. */

/* RX_CNT_INT_SEL[31] :IR_RX_CNT_THR_TRIGGER_LV. */
/* 0x1: high level couner >= threshold trigger interrupt. 0x0: low level couner >= threshold trigger interrupt. */
#define IR_RX_CNT_THR_TYPE_Pos          (31UL)
#define IR_RX_CNT_THR_TYPE_Msk          (0x1UL << IR_RX_CNT_THR_TYPE_Pos)
#define IR_RX_CNT_THR_TYPE_CLR          (~IR_RX_CNT_THR_TYPE_Msk)
/* RX_CNT_INT_SEL[0] :IR_RX_CNT_THR. */
#define IR_RX_CNT_THR_Pos               (0UL)
#define IR_RX_CNT_THR_Msk               (0x7fffffffUL << IR_RX_CNT_THR_Pos)
#define IR_RX_CNT_THR_CLR               (~IR_RX_CNT_THR_Msk)


/** @addtogroup 87x3e_IR IR
  * @brief IR driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/

/** @defgroup 87x3e_IR_Exported_Types IR Exported Types
  * @{
  */

/**
 * @brief IR initialize parameters.
 */
typedef struct
{
    uint32_t IR_Clock;              /*!< Specifies the IR clock source, default 40000000. */
    uint32_t IR_Freq;               /*!< Specifies the IR clock frequency. This parameter is the IR TX carrier frequency or RX sample clock, unit KHz.
                                              This parameter can be a value of @ref x3e_IR_Frequency. IR_CLK_DIV = (IR_Clock / IR_Freq * 1000) - 1 .  */
    uint32_t IR_DutyCycle;          /*!< Specifies the IR duty cycle. This parameter must range from 0x1 to 0x3FFF. IR_TX_DUTY_NUM = (IR_CLK_DIV + 1)/IR_DutyCycle - 1 . */
    uint32_t IR_Mode;               /*!< Specifies the IR mode.
                                              This parameter can be a value of @ref x3e_IR_Mode. */
    uint32_t IR_TxIdleLevel;        /*!< Specifies the IR TX output level in idle.
                                              This parameter can be a value of @ref x3e_IR_Idle_Status. */
    uint32_t IR_TxInverse;          /*!< Specifies inverse FIFO data or not in TX mode.
                                              This parameter can be a value of @ref x3e_IR_TX_Data_Type. */
    uint32_t IR_TxFIFOThrLevel;     /*!< Specifies TX FIFO threshold value in TX mode. When TX FIFO offset <= IR_TxFIFOThrLevel, trigger interrupt \ref IR_INT_TF_LEVEL.
                                              This parameter can be a value of @ref x3e_IR_Tx_Threshold. This parameter must range from 0 to 32. */
    uint32_t IR_RxStartMode;        /*!< Specifies the IR RX start mode.
                                              This parameter can be a value of @ref x3e_IR_Rx_Start_Mode. */
    uint32_t IR_RxFIFOThrLevel;     /*!< Specifies RX FIFO threshold value in RX mode. When RX FIFO offset >= IR_RxFIFOThrLevel, trigger interrupt \ref IR_INT_RF_LEVEL.
                                              This parameter can be a value of @ref x3e_IR_Rx_Threshold. This parameter must range from 0 to 32. */
    uint32_t IR_RxFIFOFullCtrl;     /*!< Specifies data discard mode in RX mode when RX FIFO is full and receiving new data.
                                              This parameter can be a value of @ref x3e_IR_RX_FIFO_DISCARD_SETTING. */
    uint32_t IR_RxTriggerMode;      /*!< Specifies IR RX trigger mode.
                                              This parameter can be a value of @ref x3e_IR_RX_Trigger_Mode. */
    uint32_t IR_RxFilterTime;       /*!< Specifies IR RX filter time.
                                              This parameter can be a value of @ref x3e_IR_RX_Filter_Time. */
    uint32_t IR_RxCntThrType;       /*!< Specifies counter level type when trigger @ref IR_INT_RX_CNT_THR interrupt in RX mode.
                                              This parameter can be a value of @ref x3e_IR_RX_COUNTER_THRESHOLD_TYPE. */
    uint32_t IR_RxCntThr;           /*!< Specifies counter threshold value when trigger @ref IR_INT_RX_CNT_THR interrupt in RX mode.
                                              This parameter must range from 0x0 to 0x7FFFFFFF. This parameter can be a value of @ref x3e_IR_Rx_Counter_Threshold. */
} IR_InitTypeDef;

/** End of group 87x3e_IR_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_IR_Exported_Constants IR Exported Constants
  * @{
  */

#define IS_IR_PERIPH(PERIPH) ((PERIPH) == IR) //!< I2C peripheral can select IR.
#define IR_TX_FIFO_SIZE                   32  //!< I2C TX FIFO SIZE is 32.
#define IR_RX_FIFO_SIZE                   32  //!< I2C RX FIFO SIZE is 32.

/** @defgroup 87x3e_IR_Frequency IR Frequency
  * @{
  */

#define IS_IR_FREQUENCY(F) (((F) >= 5) && ((F) <= 40000)) //!< IR TX carrier frequency is between 5 and 2000, RX sample clock max is 40000 (unit KHz).

/** End of group 87x3e_IR_Frequency
  * @}
  */

/** @defgroup 87x3e_IR_Mode IR Mode
  * @{
  */

#define IR_MODE_TX                                  ((uint32_t)((uint32_t)0x0 << IR_MODE_SEL_Pos)) //!< IR TX mode.
#define IR_MODE_RX                                  ((uint32_t)((uint32_t)0x1 << IR_MODE_SEL_Pos)) //!< IR RX mode.

#define IS_IR_MODE(MODE) (((MODE) == IR_MODE_TX) || ((MODE) == IR_MODE_RX)) //!< Check if the input parameter is valid.

/** End of group 87x3e_IR_Mode
  * @}
  */

/** @defgroup 87x3e_IR_Idle_Status  IR Idle Status
  * @{
  */

#define IR_IDLE_OUTPUT_HIGH                         ((uint32_t)(0x01 << IR_TX_IDLE_STATE_Pos)) //!< TX output high level in idle.
#define IR_IDLE_OUTPUT_LOW                          ((uint32_t)(0x00 << IR_TX_IDLE_STATE_Pos)) //!< TX output low level in idle.

#define IS_IR_IDLE_STATUS(LEVEL) (((LEVEL) == IR_IDLE_OUTPUT_HIGH) || ((LEVEL) == IR_IDLE_OUTPUT_LOW)) //!< Check if the input parameter is valid.

/** End of group 87x3e_IR_Idle_Status
  * @}
  */

/** @defgroup 87x3e_IR_TX_Data_Type IR TX Data Type
  * @{
  */

#define IR_TX_DATA_NORMAL                           ((uint32_t)(0 << IR_FIFO_INVERSE_Pos)) //!< Not inverse TX FIFO define.
#define IR_TX_DATA_INVERSE                          ((uint32_t)(1 << IR_FIFO_INVERSE_Pos)) //!< Inverse TX FIFO define.

#define IS_IR_TX_DATA_TYPE(TYPE) (((TYPE) == IR_TX_DATA_NORMAL) || ((TYPE) == IR_TX_DATA_INVERSE)) //!< Check if the input parameter is valid.

/** End of group 87x3e_IR_TX_Data_Type
  * @}
  */

/** @defgroup 87x3e_IR_Tx_Threshold IR TX Threshold
  * @{
  */

#define IS_IR_TX_THRESHOLD(THD)  ((THD) <= IR_TX_FIFO_SIZE) //!< IR TX Threshold value must range from 0 to 32.

/** End of group 87x3e_IR_Tx_Threshold
  * @}
  */

/** @defgroup 87x3e_IR_Rx_Start_Mode IR RX Start Mode
  * @{
  */

#define IR_RX_AUTO_MODE                             ((uint32_t)((0x1) << IR_RX_START_MODE_Pos)) //!< IR RX is started automatically, and starts when the RX trigger mode is met.
#define IR_RX_MANUAL_MODE                           ((uint32_t)((0x0) << IR_RX_START_MODE_Pos)) //!< IR RX is started by calling API \ref IR_StartManualRxTrigger.

#define IS_RX_START_MODE(MODE) (((MODE) == IR_RX_AUTO_MODE) || ((MODE) == IR_RX_MANUAL_MODE)) //!< Check if the input parameter is valid.

/** End of group 87x3e_IR_Rx_Start_Mode
  * @}
  */

/** @defgroup 87x3e_IR_Rx_Threshold IR RX Threshold
  * @{
  */

#define IS_IR_RX_THRESHOLD(THD) ((THD) <= IR_RX_FIFO_SIZE) //!< IR RX Threshold must range from 0 to 32.

/** End of group 87x3e_IR_Rx_Threshold
  * @}
  */

/** @defgroup 87x3e_IR_RX_Trigger_Mode IR RX Trigger Mode
  * @{
  */

#define IR_RX_FALL_EDGE                             ((uint32_t)((0x0) << IR_TRIGGER_MODE_Pos)) //!< IR RX is triggered by falling edge.
#define IR_RX_RISING_EDGE                           ((uint32_t)((0x1) << IR_TRIGGER_MODE_Pos)) //!< IR RX is triggered by rising edge.
#define IR_RX_DOUBLE_EDGE                           ((uint32_t)((0x2) << IR_TRIGGER_MODE_Pos)) //!< IR RX is triggered by double edge.

#define IS_RX_RX_TRIGGER_EDGE(EDGE) (((EDGE) == IR_RX_FALL_EDGE) || ((EDGE) == IR_RX_RISING_EDGE) || ((EDGE) == IR_RX_DOUBLE_EDGE)) //!< Check if the input parameter is valid.

/** End of group 87x3e_IR_RX_Trigger_Mode
  * @}
  */

/** @defgroup 87x3e_IR_RX_FIFO_DISCARD_SETTING IR RX FIFO Discard Setting
  * @{
  */

#define IR_RX_FIFO_FULL_DISCARD_OLDEST              ((uint32_t)(0 << IR_RX_FIFO_DISCARD_SET_Pos)) //!< Discard oldest data in FIFO when RX FIFO is full and new data send to FIFO.
#define IR_RX_FIFO_FULL_DISCARD_NEWEST              ((uint32_t)(1 << IR_RX_FIFO_DISCARD_SET_Pos)) //!< Reject new data data send to FIFO when RX FIFO is full.

#define IS_IR_RX_FIFO_FULL_CTRL(CTRL)  (((CTRL) == IR_RX_FIFO_FULL_DISCARD_NEWEST) || ((CTRL) == IR_RX_FIFO_FULL_DISCARD_OLDEST)) //!< Check if the input parameter is valid.

/** End of group 87x3e_IR_RX_FIFO_DISCARD_SETTING
  * @}
  */

/** @defgroup 87x3e_IR_RX_Filter_Time IR RX Filter Time
  * @{
  */

#define IR_RX_FILTER_TIME_50ns                      ((uint32_t)((0x0) << IR_FILTER_TIME_Pos)) //!< IR RX filter time is 50ns.
#define IR_RX_FILTER_TIME_75ns                      ((uint32_t)((0x2) << IR_FILTER_TIME_Pos)) //!< IR RX filter time is 75ns.
#define IR_RX_FILTER_TIME_100ns                     ((uint32_t)((0x3) << IR_FILTER_TIME_Pos)) //!< IR RX filter time is 100ns.
#define IR_RX_FILTER_TIME_125ns                     ((uint32_t)((0x4) << IR_FILTER_TIME_Pos)) //!< IR RX filter time is 125ns.
#define IR_RX_FILTER_TIME_150ns                     ((uint32_t)((0x5) << IR_FILTER_TIME_Pos)) //!< IR RX filter time is 150ns.
#define IR_RX_FILTER_TIME_175ns                     ((uint32_t)((0x6) << IR_FILTER_TIME_Pos)) //!< IR RX filter time is 175ns.
#define IR_RX_FILTER_TIME_200ns                     ((uint32_t)((0x7) << IR_FILTER_TIME_Pos)) //!< IR RX filter time is 200ns.

#define IS_IR_RX_FILTER_TIME_CTRL(CTRL)  (((CTRL) == IR_RX_FILTER_TIME_50ns) || \
                                          ((CTRL) == IR_RX_FILTER_TIME_75ns) || \
                                          ((CTRL) == IR_RX_FILTER_TIME_100ns) || \
                                          ((CTRL) == IR_RX_FILTER_TIME_125ns) || \
                                          ((CTRL) == IR_RX_FILTER_TIME_150ns) || \
                                          ((CTRL) == IR_RX_FILTER_TIME_175ns) || \
                                          ((CTRL) == IR_RX_FILTER_TIME_200ns)) //!< Check if the input parameter is valid.
/** End of group 87x3e_IR_RX_Filter_Time
  * @}
  */

/** @defgroup 87x3e_IR_RX_COUNTER_THRESHOLD_TYPE IR RX Counter Threshold Type
  * @{
  */

#define IR_RX_Count_Low_Level                       ((uint32_t)0 << IR_RX_CNT_THR_TYPE_Pos) //!< \ref IR_INT_RX_CNT_THR is triggered when low level couner >= threshold.
#define IR_RX_Count_High_Level                      ((uint32_t)1 << IR_RX_CNT_THR_TYPE_Pos) //!< \ref IR_INT_RX_CNT_THR is triggered when high level couner >= threshold.

#define IS_IR_RX_COUNT_LEVEL_CTRL(CTRL)  (((CTRL) == IR_RX_Count_Low_Level) || ((CTRL) == IR_RX_Count_High_Level)) //!< Check if the input parameter is valid.

/** End of group 87x3e_IR_RX_COUNTER_THRESHOLD_TYPE
  * @}
  */

/** @defgroup 87x3e_IR_Rx_Counter_Threshold IR RX Counter Threshold
  * @{
  */

#define IS_IR_RX_COUNTER_THRESHOLD(THD) ((THD) <= IR_RX_CNT_THR_Msk) //!< IR RX counter threshold value must range from 0x0 to 0x7FFFFFFF.

/** End of group 87x3e_IR_Rx_Counter_Threshold
  * @}
  */

/** @defgroup 87x3e_IR_Interrupts_Definition  IR Interrupts Definition
  * @{
  */

/* All interrupts in transmission mode */
#define IR_INT_TF_EMPTY                             ((uint32_t)IR_TX_FIFO_EMPTY_INT_EN_Msk) //!< When TX FIFO is empty, TX FIFO empty interrupt will be triggered.
#define IR_INT_TF_LEVEL                             ((uint32_t)IR_TX_FIFO_LEVEL_INT_EN_Msk) //!< When TX FIFO offset <= threshold value, trigger TX FIFO level interrupt.
#define IR_INT_TF_OF                                ((uint32_t)IR_TX_FIFO_OVER_INT_EN_Msk) //!< When TX FIFO is full, data continues to be wrote to TX FIFO, TX FIFO overflow interrupt will be triggered.
/* All interrupts in receiving mode */
#define IR_INT_RF_FULL                              ((uint32_t)IR_RX_FIFO_FULL_INT_Msk) //!< When RX FIFO offset = 32, RX FIFO full interrupt will be triggered.
#define IR_INT_RF_LEVEL                             ((uint32_t)IR_RX_FIFO_LEVEL_INT_Msk) //!< When RX FIFO offset >= threshold value, trigger RX FIFO Level Interrupt.
#define IR_INT_RX_CNT_OF                            ((uint32_t)IR_RX_CNT_OF_INT_Msk) //!< When RX counter is overflow, RX counter overflow interrupt will be triggered.
#define IR_INT_RF_OF                                ((uint32_t)IR_RX_FIFO_OF_INT_Msk) //!< When RX FIFO is full and continue to be wrote, RX FIFO overflow interrupt will be triggered.
#define IR_INT_RX_CNT_THR                           ((uint32_t)IR_RX_CNT_THR_INT_Msk) //!< When RX counter >= IR_RxCntThr, RX counter threshold interrupt will be triggered.
#define IR_INT_RF_ERROR                             ((uint32_t)IR_RX_FIFO_ERROR_INT_Msk) //!< When RX FIFO is empty and continue to be read, RX FIFO error read interrupt will be triggered.

#define IS_IR_TX_INT_CONFIG(CONFIG)   (((CONFIG) == IR_INT_TF_EMPTY)   || \
                                       ((CONFIG) == IR_INT_TF_LEVEL)   || \
                                       ((CONFIG) == IR_INT_TF_OF)) //!< Contains all IR TX interrupt.

#define IS_IR_RX_INT_CONFIG(CONFIG)   (((CONFIG) == IR_INT_RF_FULL)     || \
                                       ((CONFIG) == IR_INT_RF_LEVEL)   || \
                                       ((CONFIG) == IR_INT_RX_CNT_OF)  || \
                                       ((CONFIG) == IR_INT_RF_OF)      || \
                                       ((CONFIG) == IR_INT_RX_CNT_THR) || \
                                       ((CONFIG) == IR_INT_RF_ERROR)) //!< Contains all IR RX interrupt.
#define IS_IR_INT_CONFIG(CONFIG)      (IS_IR_TX_INT_CONFIG(CONFIG) || IS_IR_RX_INT_CONFIG(CONFIG)) //!< Contains all IR TX and RX interrupt.

/** End of group 87x3e_IR_Interrupts_Definition
  * @}
  */

/** @defgroup 87x3e_IR_Interrupts_Clear_Flag IR Interrupts Clear Flag
  * @{
  */

/* Clear all interrupts in transmission mode */
#define IR_INT_TF_EMPTY_CLR                                 ((uint32_t)IR_TX_FIFO_EMPTY_INT_CLR_Msk) //!< Clear TX FIFO empty interrupt status.
#define IR_INT_TF_LEVEL_CLR                                 ((uint32_t)IR_TX_FIFO_LEVEL_INT_CLR_Msk) //!< Clear TX FIFO threshold interrupt status.
#define IR_INT_TF_OF_CLR                                    ((uint32_t)IR_TX_FIFO_OVER_INT_CLR_Msk) //!< Clear TX FIFO overflow interrupt status.
/* Clear all interrupts in receiving mode */
#define IR_INT_RF_FULL_CLR                                  ((uint32_t)IR_RX_FIFO_FULL_INT_CLR_Msk) //!< Clear RX FIFO full interrupt status.
#define IR_INT_RF_LEVEL_CLR                                 ((uint32_t)IR_RX_FIFO_LEVEL_INT_CLR_Msk) //!< Clear RX FIFO threshold interrupt status.
#define IR_INT_RX_CNT_OF_CLR                                ((uint32_t)IR_RX_CNT_OF_INT_CLR_Msk) //!< Clear RX counter overflow interrupt status.
#define IR_INT_RF_OF_CLR                                    ((uint32_t)IR_RX_FIFO_OF_INT_CLR_Msk) //!< Clear RX FIFO overflow interrupt status.
#define IR_INT_RX_CNT_THR_CLR                               ((uint32_t)IR_RX_CNT_THR_INT_CLR_Msk) //!< Clear RX counter timeout interrupt status.
#define IR_INT_RF_ERROR_CLR                                 ((uint32_t)IR_RX_FIFO_ERROR_INT_CLR_Msk) //!< Clear RX FIFO error read interrupt status.
#define IR_RF_CLR                                           ((uint32_t)IR_RX_FIFO_CLR_Msk) //!< Clear RX FIFO.
#define IS_IR_INT_CLEAR(INT)            (((INT) == IR_INT_TF_EMPTY_CLR)  || ((INT) == IR_INT_TF_LEVEL_CLR)  || \
                                         ((INT) == IR_INT_TF_OF_CLR)      || ((INT) == IR_INT_RF_FULL_CLR)   || \
                                         ((INT) == IR_INT_RF_LEVEL_CLR)   || ((INT) == IR_INT_RX_CNT_OF_CLR) || \
                                         ((INT) == IR_INT_RF_OF_CLR)      || ((INT) == IR_INT_RX_CNT_THR_CLR)|| \
                                         ((INT) == IR_INT_RF_ERROR_CLR)) //!< Check if the input parameter is valid.

/** End of group 87x3e_IR_Interrupts_Clear_Flag
  * @}
  */

/** @defgroup 87x3e_IR_Flag IR Flag
  * @{
  */

#define IR_FLAG_TF_EMPTY                                    ((uint32_t)IR_TX_FIFO_EMPTY_Msk) //!< TX FIFO is empty or not.
#define IR_FLAG_TF_FULL                                     ((uint32_t)IR_TX_FIFO_FULL_Msk) //!< TX FIFO is full or not.
#define IR_FLAG_TX_RUN                                      ((uint32_t)IR_TX_STATUS_Msk) //!< TX state is running or idle.
#define IR_FLAG_RF_EMPTY                                    ((uint32_t)IR_RX_FIFO_EMPTY_Msk) //!< RX FIFO is empty or not.
#define IR_FLAG_RX_RUN                                      ((uint32_t)IR_RX_STATUS_Msk) //!< RX state is running or idle.

#define IS_IR_FLAG(FLAG)                (((FLAG) == IR_FLAG_TF_EMPTY) || ((FLAG) == IR_FLAG_TF_FULL) || \
                                         ((FLAG) == IR_FLAG_TX_RUN) || ((FLAG) == IR_FLAG_RF_EMPTY) || \
                                         ((FLAG) == IR_FLAG_RX_RUN)) //!< Check if the input parameter is valid.
/** End of group 87x3e_IR_Flag
  * @}
  */

/** @defgroup 87x3e_IR_Compensation_Flag IR Compensation Flag
  * @{
  */

typedef enum
{

    IR_COMPEN_FLAG_0_8_CARRIER      = ((uint32_t)(0 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< no compensation.
    IR_COMPEN_FLAG_1_8_CARRIER      = ((uint32_t)(1 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< 1/8 carrier cycle.
    IR_COMPEN_FLAG_1_4_CARRIER      = ((uint32_t)(2 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< 1/4 carrier cycle.
    IR_COMPEN_FLAG_2_8_CARRIER      = ((uint32_t)(2 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< 2/8 carrier cycle.
    IR_COMPEN_FLAG_3_8_CARRIER      = ((uint32_t)(3 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< 3/8 carrier cycle.
    IR_COMPEN_FLAG_1_2_CARRIER      = ((uint32_t)(4 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< 1/2 carrier cycle.
    IR_COMPEN_FLAG_4_8_CARRIER      = ((uint32_t)(4 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< 4/8 carrier cycle.
    IR_COMPEN_FLAG_5_8_CARRIER      = ((uint32_t)(5 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< 5/8 carrier cycle.
    IR_COMPEN_FLAG_6_8_CARRIER      = ((uint32_t)(6 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< 6/8 carrier cycle.
    IR_COMPEN_FLAG_7_8_CARRIER      = ((uint32_t)(7 << IR_LOW_PERIOD_COMPENSATION_Pos)), //!< 7/8 carrier cycle.

} IR_TX_COMPEN_TYPE;

/** End of group 87x3e_IR_Compensation_Flag
  * @}
  */

/** End of group IR_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_IR_Exported_Functions IR Exported Functions
  * @{
  */

/**
 *
 * \brief  Disable the IR peripheral clock, and restore registers to their default values.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_ir_init(void)
 * {
 *     IR_DeInit();
 * }
 * \endcode
 */
void IR_DeInit(void);


/**
 *
 * \brief   Initializes the IR peripheral according to the specified
 *          parameters in IR_InitStruct.
 *
 * \param[in] IR_InitStruct: Pointer to a \ref IR_InitTypeDef structure that
 *            contains the configuration information for the specified IR peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_ir_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_IR, APBPeriph_IR_CLOCK, ENABLE);
 *
 *     IR_InitTypeDef IR_InitStruct;
 *     IR_StructInit(&IR_InitStruct);
 *
 *     IR_InitStruct.IR_Freq               = 38;// IR carrier frequency is 38KHz
 *     IR_InitStruct.IR_DutyCycle          = 2; //Duty ratio = 1/IR_DutyCycle
 *     IR_InitStruct.IR_Mode               = IR_MODE_RX;// IR receiveing mode
 *     IR_InitStruct.IR_RxStartMode        = IR_RX_AUTO_MODE;
 *     IR_InitStruct.IR_RxFIFOThrLevel     =
           IR_RX_FIFO_THR_LEVEL; // Configure RX FIFO threshold level to trigger IR_INT_RF_LEVEL interrupt
 *     IR_InitStruct.IR_RxFIFOFullCtrl     =
 *         IR_RX_FIFO_FULL_DISCARD_NEWEST;// Discard the lastest received dta if RX FIFO is full
 *     IR_InitStruct.IR_RxFilterTime       =
           IR_RX_FILTER_TIME_50ns;// If high to low or low to high transition time <= 50ns,Filter out it.
 *     IR_InitStruct.IR_RxTriggerMode      = IR_RX_RISING_EDGE;// Configure trigger type
 *     IR_InitStruct.IR_RxCntThrType       =
           IR_RX_Count_Low_Level;// IR_RX_Count_Low_Level is counting low level
 *     IR_InitStruct.IR_RxCntThr           =
           0x23a;// Configure RX counter threshold.You can use it to decide to stop receiving IR data
 *     IR_Init(&IR_InitStruct);
 * }
 * \endcode
 */
void IR_Init(IR_InitTypeDef *IR_InitStruct);


/**
 *
 * \brief  Fills each IR_InitStruct member with its default value.
 *
 * \param[in] IR_InitStruct: Pointer to an \ref IR_InitTypeDef structure which will be initialized.
 *
 * \note   The default settings for the IR_InitStruct member are shown in the following table:
 *         | IR_InitStruct Member  | Default Value                            |
 *         |:---------------------:|:----------------------------------------:|
 *         | IR_Clock              | 40000000                                 |
 *         | IR_Freq               | 38                                       |
 *         | IR_DutyCycle          | 3                                        |
 *         | IR_Mode               | \ref IR_MODE_TX                          |
 *         | IR_TxIdleLevel        | \ref IR_IDLE_OUTPUT_LOW                  |
 *         | IR_TxInverse          | \ref IR_TX_DATA_NORMAL                   |
 *         | IR_TxFIFOThrLevel     | 0                                        |
 *         | IR_RxStartMode        | \ref IR_RX_AUTO_MODE                     |
 *         | IR_RxFIFOThrLevel     | 0                                        |
 *         | IR_RxFIFOFullCtrl     | \ref IR_RX_FIFO_FULL_DISCARD_NEWEST      |
 *         | IR_RxTriggerMode      | \ref IR_RX_FALL_EDGE                     |
 *         | IR_RxFilterTime       | \ref IR_RX_FILTER_TIME_50ns              |
 *         | IR_RxCntThrType       | \ref IR_RX_Count_Low_Level               |
 *         | IR_RxCntThr           | 0x23a                                    |
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_ir_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_IR, APBPeriph_IR_CLOCK, ENABLE);

 *     IR_InitTypeDef IR_InitStruct;
 *     IR_StructInit(&IR_InitStruct);

 *     IR_InitStruct.IR_Freq               = 38;// IR carrier frequency is 38KHz
 *     IR_InitStruct.IR_DutyCycle          = 2; // !< 1/2 duty cycle
 *     IR_InitStruct.IR_Mode               = IR_MODE_TX;
 *     IR_InitStruct.IR_TxInverse          = IR_TX_DATA_NORMAL;
 *     IR_InitStruct.IR_TxFIFOThrLevel     = IR_TX_FIFO_THR_LEVEL;
 *     IR_Init(&IR_InitStruct);
 * }
 * \endcode
 */
void IR_StructInit(IR_InitTypeDef *IR_InitStruct);


/**
 *
 * \brief   Enable or disable the selected IR mode.
 *
 * \param[in] mode: Selected IR operation mode \ref x3e_IR_Mode.
 *            This parameter can be one of the following values:
 *            - IR_MODE_TX: Transmission mode.
 *            - IR_MODE_RX: Receiving mode.
 * \param[in] NewState: New state of the operation mode.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the selected IR mode. IR controller switches to TX or RX mode, allowing it to begin data transfer operations.
 *            - DISABLE: Disable the selected IR mode. Stop IR TX or RX data transfer and enter idle state.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_send_demo_code(void)
 * {
 *     IR_Cmd(IR_MODE_TX, ENABLE);
 * }
 * \endcode
 */
void IR_Cmd(uint32_t mode, FunctionalState NewState);


/**
 *
 * \brief   Start trigger receive, only in manual receive mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     IR_StartManualRxTrigger();
 * }
 * \endcode
 */
void IR_StartManualRxTrigger(void);


/**
 *
 * \brief   Config counter threshold value in receiving mode. You can use it to stop receiving IR data.
 *
 * \param[in] IR_RxCntThrType: IR RX Count threshold type \ref x3e_IR_RX_COUNTER_THRESHOLD_TYPE.
 *            This parameter can be the following values:
 *            - IR_RX_Count_Low_Level: Low level counter value >= IR_RxCntThr, trigger \ref IR_INT_RX_CNT_THR interrupt.
 *            - IR_RX_Count_High_Level: High level counter value >= IR_RxCntThr, trigger \ref IR_INT_RX_CNT_THR interrupt.
 * \param[in] IR_RxCntThr: Configure IR RX counter threshold value which can be 0 to 0x7fffffffUL.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     IR_SetRxCounterThreshold(IR_RX_Count_Low_Level, 0x100);
 * }
 * \endcode
 */
void IR_SetRxCounterThreshold(uint32_t IR_RxCntThrType, uint32_t IR_RxCntThr);


/**
 *
 * \brief   Send data through IR.
 *
 * \param[in] pBuf: Data buffer to send. This parameter must range from 0x0 to 0xFFFFFFFF.
 * \param[in] len: The length of data to send. This parameter must range from 0x1 to 0xFFFFFFFF.
 * \param[in] IsLastPacket: Is it the last package of data.
 *            This parameter can be one of the following values:
 *            - ENABLE: The last data in IR packet and there is no continous data. In other words, an infrared data transmission is completed.
 *            - DISABLE: There is data to be transmitted continuously.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     uint32_t data_buf[80] = {0};
 *     IR_SendBuf(data_buf, 68, DISABLE);
 * }
 * \endcode
 */
void IR_SendBuf(uint32_t *pBuf, uint32_t len, FunctionalState IsLastPacket);


/**
 *
 * \brief  Send compensation data.
 *
 * \param[in] comp_type: Compensation data type \ref x3e_IR_Compensation_Flag.
 *            This parameter can be one of the following values:
 *            - IR_COMPEN_FLAG_0_8_CARRIER: no compensation.
 *            - IR_COMPEN_FLAG_1_8_CARRIER: 1/8 carrier cycle.
 *            - IR_COMPEN_FLAG_1_4_CARRIER: 1/4 carrier cycle.
 *            - IR_COMPEN_FLAG_1_2_CARRIER: 1/2 carrier cycle.
 *            - IR_COMPEN_FLAG_2_8_CARRIER: 2/8 carrier cycle.
 *            - IR_COMPEN_FLAG_3_8_CARRIER: 3/8 carrier cycle.
 *            - IR_COMPEN_FLAG_4_8_CARRIER: 4/8 carrier cycle.
 *            - IR_COMPEN_FLAG_5_8_CARRIER: 5/8 carrier cycle.
 *            - IR_COMPEN_FLAG_6_8_CARRIER: 6/8 carrier cycle.
 *            - IR_COMPEN_FLAG_7_8_CARRIER: 7/8 carrier cycle.
 * \param[in] pBuf: Data buffer to send. This parameter must range from 0x0 to 0xFFFFFFFF.
 * \param[in] len: The length of data to send. This parameter must range from 0x1 to 0xFFFFFFFF.
 * \param[in] IsLastPacket: Is it the last package of data.
 *            This parameter can be one of the following values:
 *            - ENABLE: The last data in IR packet and there is no continous data. In other words, an infrared data transmission is completed.
 *            - DISABLE: There is data to be transmitted continuously.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     uint32_t data_buf[80] = {0};
 *     IR_SendCompenBuf(IR_COMPEN_FLAG_1_2_CARRIER, data_buf, 68, DISABLE);
 * }
 * \endcode
 */
void IR_SendCompenBuf(IR_TX_COMPEN_TYPE comp_type, uint32_t *pBuf, uint32_t len,
                      FunctionalState IsLastPacket);

/**
 *
 * \brief   Read data from RX FIFO.
 *
 * \param[in] pBuf: Buffer address to receive data. This parameter must range from 0x0 to 0xFFFFFFFF.
 * \param[in] length: The length of data to read. This parameter must range from 0x1 to 0xFFFFFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     uint32_t data_buf[80] = {0};
 *     IR_ReceiveBuf(data_buf, 68);
 * }
 * \endcode
 */
void IR_ReceiveBuf(uint32_t *pBuf, uint32_t length);

/**
 *
 * \brief     Enable or disable the specified IR interrupt.
 *
 * \param[in] IR_INT: Specifies the IR interrupt to be enabled or disabled \ref x3e_IR_Interrupts_Definition.
 *            This parameter can be one of the following values:
 *            - IR_INT_TF_EMPTY: When TX FIFO is empty, TX FIFO empty interrupt will be triggered.
 *            - IR_INT_TF_LEVEL: When TX FIFO offset <= threshold value, trigger TX FIFO level interrupt.
 *            - IR_INT_TF_OF: When TX FIFO is full, data continues to be wrote to TX FIFO, TX FIFO overflow interrupt will be triggered.
 *            - IR_INT_RF_FULL: When RX FIFO offset = 32, RX FIFO full interrupt will be triggered.
 *            - IR_INT_RF_LEVEL: When RX FIFO offset >= threshold value, trigger RX FIFO Level Interrupt.
 *            - IR_INT_RX_CNT_OF: When RX counter is overflow, RX counter overflow interrupt will be triggered.
 *            - IR_INT_RF_OF: When RX FIFO is full and continue to be wrote, RX FIFO overflow interrupt will be triggered.
 *            - IR_INT_RX_CNT_THR: When RX counter >= IR_RxCntThr, RX counter threshold interrupt will be triggered.
 *            - IR_INT_RF_ERROR: When RX FIFO is empty and continue to be read, RX FIFO error read interrupt will be triggered.
 * \param[in] newState: New state of the specified IR interrupt.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified IR interrupt.
 *            - DISABLE: Disable the specified IR interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_send_demo_code(void)
 * {
 *     //Enable IR threshold interrupt. when TX FIFO offset <= threshold value, trigger interrupt.
 *     IR_INTConfig(IR_INT_TF_LEVEL, ENABLE);
 * }
 * \endcode
 */
void IR_INTConfig(uint32_t IR_INT, FunctionalState newState);


/**
 *
 * \brief     Mask or unmask the specified IR interrupt.
 *
 * \param[in] IR_INT: Specifies the IR interrupts to be mask or unmask \ref x3e_IR_Interrupts_Definition.
 *            This parameter can be one of the following values:
 *            - IR_INT_TF_EMPTY: When TX FIFO is empty, TX FIFO empty interrupt will be triggered.
 *            - IR_INT_TF_LEVEL: When TX FIFO offset <= threshold value, trigger TX FIFO level interrupt.
 *            - IR_INT_TF_OF: When TX FIFO is full, data continues to be wrote to TX FIFO, TX FIFO overflow interrupt will be triggered.
 *            - IR_INT_RF_FULL: When RX FIFO offset = 32, RX FIFO full interrupt will be triggered.
 *            - IR_INT_RF_LEVEL: When RX FIFO offset >= threshold value, trigger RX FIFO Level Interrupt.
 *            - IR_INT_RX_CNT_OF: When RX counter is overflow, RX counter overflow interrupt will be triggered.
 *            - IR_INT_RF_OF: When RX FIFO is full and continue to be wrote, RX FIFO overflow interrupt will be triggered.
 *            - IR_INT_RX_CNT_THR: When RX counter >= IR_RxCntThr, RX counter threshold interrupt will be triggered.
 *            - IR_INT_RF_ERROR: When RX FIFO is empty and continue to be read, RX FIFO error read interrupt will be triggered.
 * \param[in] newState: New state of the specified IR interrupts.
 *            This parameter can be one of the following values:
 *            - ENABLE: Mask the specified IR interrupt.
 *            - DISABLE: Unmask the specified IR interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void IR_Handler(void)
 * {
 *     if (IR_GetINTStatus(IR_INT_TF_LEVEL) == SET)
 *     {
 *         IR_MaskINTConfig(IR_INT_TF_LEVEL, ENABLE);
 *
 *         //add user code here.
 *
 *         IR_ClearINTPendingBit(IR_INT_TF_LEVEL_CLR);
 *         IR_MaskINTConfig(IR_INT_TF_LEVEL, DISABLE);
 *     }
 * }
 * \endcode
 */
void IR_MaskINTConfig(uint32_t IR_INT, FunctionalState newState);

/**
 *
 * \brief     Get the specified IR interrupt status.
 *
 * \param[in] IR_INT: The specified IR interrupts \ref x3e_IR_Interrupts_Definition.
 *            This parameter can be one of the following values:
 *            - IR_INT_TF_EMPTY: When TX FIFO is empty, TX FIFO empty interrupt will be triggered.
 *            - IR_INT_TF_LEVEL: When TX FIFO offset <= threshold value, trigger TX FIFO level interrupt.
 *            - IR_INT_TF_OF: When TX FIFO is full, data continues to be wrote to TX FIFO, TX FIFO overflow interrupt will be triggered.
 *            - IR_INT_RF_FULL: When RX FIFO offset = 32, RX FIFO full interrupt will be triggered.
 *            - IR_INT_RF_LEVEL: When RX FIFO offset >= threshold value, trigger RX FIFO Level Interrupt.
 *            - IR_INT_RX_CNT_OF: When RX counter is overflow, RX counter overflow interrupt will be triggered.
 *            - IR_INT_RF_OF: When RX FIFO is full and continue to be wrote, RX FIFO overflow interrupt will be triggered.
 *            - IR_INT_RX_CNT_THR: When RX counter >= IR_RxCntThr, RX counter threshold interrupt will be triggered.
 *            - IR_INT_RF_ERROR: When RX FIFO is empty and continue to be read, RX FIFO error read interrupt will be triggered.
 *
 * \return  The new state of IR_INT.
 * \retval SET: The specified IR interrupt flag is set.
 * \retval RESET: The specified IR interrupt flag is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void IR_Handler(void)
 * {
 *     if (IR_GetINTStatus(IR_INT_RF_LEVEL) == SET)
 *     {
 *         //add user code here.
 *     }
 * }
 * \endcode
 */
ITStatus IR_GetINTStatus(uint32_t IR_INT);

/**
 *
 * \brief     Clear the IR interrupt pending bit.
 *
 * \param[in] IR_CLEAR_INT: Specifies the interrupt pending bit to clear \ref x3e_IR_Interrupts_Clear_Flag.
 *            This parameter can be one or any combination of the following values:
 *            - IR_INT_TF_EMPTY_CLR: Clear TX FIFO empty interrupt.
 *            - IR_INT_TF_LEVEL_CLR: Clear TX FIFO threshold interrupt.
 *            - IR_INT_TF_OF_CLR: Clear TX FIFO overflow interrupt.
 *            - IR_INT_RF_FULL_CLR: Clear RX FIFO full interrupt.
 *            - IR_INT_RF_LEVEL_CLR: Clear RX FIFO threshold interrupt.
 *            - IR_INT_RX_CNT_OF_CLR: Clear RX counter overflow interrupt.
 *            - IR_INT_RF_OF_CLR: Clear RX FIFO overflow interrupt.
 *            - IR_INT_RX_CNT_THR_CLR: Clear RX counter timeout interrupt.
 *            - IR_INT_RF_ERROR_CLR: Clear RX FIFO error read interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void IR_Handler(void)
 * {
 *     if (IR_GetINTStatus(IR_INT_RF_LEVEL) == SET)
 *     {
 *         IR_MaskINTConfig(IR_INT_RF_LEVEL, ENABLE);
 *
 *         //add user code here.
 *         IR_ClearINTPendingBit(IR_INT_RF_LEVEL_CLR);
 *         IR_MaskINTConfig(IR_INT_RF_LEVEL, DISABLE);
 *     }
 * }
 * \endcode
 */
void IR_ClearINTPendingBit(uint32_t IR_CLEAR_INT);


/**
 *
 * \brief  Get free size of TX FIFO.
 *
 * \return The free size of TX FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     uint16_t data_len = IR_GetTxFIFOFreeLen();
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint16_t IR_GetTxFIFOFreeLen(void)
{
    return (uint16_t)(IR_TX_FIFO_SIZE - ((IR->TX_SR & (IR_TX_FIFO_OFFSET_Msk)) >> 8));
}


/**
 *
 * \brief   Get data size in RX FIFO.
 *
 * \return  Current data size in RX FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void IR_Handler(void)
 * {
 *     //Receive by interrupt.
 *     if (IR_GetINTStatus(IR_INT_RF_LEVEL) == SET)
 *     {
 *         len = IR_GetRxDataLen();
 *         IR_ReceiveBuf(IR_DataStruct.irBuf + rx_count, len);
 *         IR_DataStruct.bufLen += len;
 *         rx_count += len;
 *     }
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint16_t IR_GetRxDataLen(void)
{
    return ((uint16_t)(((IR->RX_SR) & IR_RX_FIFO_OFFSET_Msk) >> 8));
}


/**
 *
 * \brief   Send one data.
 *
 * \param[in] data: The data to send. This parameter must range from 0x0 to 0xFFFFFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     IR_SendData(0x80000100);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void IR_SendData(uint32_t data)
{
    IR->TX_FIFO = data;
}


/**
 *
 * \brief   Read one data.
 *
 * \return  Data which read from RX FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     uint32_t data = IR_ReceiveData();
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE uint32_t IR_ReceiveData(void)
{
    return IR->RX_FIFO;
}


/**
 *
 * \brief  Set TX threshold, when TX FIFO offset <= threshold value will trigger interrupt \ref IR_INT_TF_LEVEL.
 *
 * \param[in] thd: TX threshold. This parameter must range from 0 to 32.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void IR_Handler(void)
 * {
 *     //Configure TX threshold level to zero and trigger interrupt when TX FIFO is empty.
 *     IR_SetTxThreshold(0);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void IR_SetTxThreshold(uint8_t thd)
{
    IR->TX_CONFIG &= IR_TX_FIFO_THRESHOLD_CLR;
    IR->TX_CONFIG |= (thd << IR_TX_FIFO_THRESHOLD_Pos);
}


/**
 *
 * \brief   Set RX threshold, when RX FIFO offset >= threshold value will trigger interrupt \ref IR_INT_RF_LEVEL.
 *
 * \param[in] thd: RX threshold. This parameter must range from 0 to 32.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     IR_SetRxThreshold(2);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void IR_SetRxThreshold(uint8_t thd)
{
    IR->RX_CONFIG &= IR_RX_FIFO_LEVEL_CLR;
    IR->RX_CONFIG |= (thd << IR_RX_FIFO_LEVEL_Pos);
}


/**
 *
 * \brief  Clear IR TX FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     IR_ClearTxFIFO();
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void IR_ClearTxFIFO(void)
{
    IR->TX_INT_CLR = IR_TX_FIFO_CLR_Msk;
}


/**
 *
 * \brief   Clear IR RX FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     IR_ClearRxFIFO();
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void IR_ClearRxFIFO(void)
{
    IR->RX_INT_CLR = IR_RX_FIFO_CLR_Msk;
}


/**
 *
 * \brief  Check whether the specified IR flag is set.
 *
 * \param[in] IR_FLAG: Specifies the IR flag to check \ref x3e_IR_Flag.
 *            This parameter can be one of the following values:
 *            - IR_FLAG_TF_EMPTY: TX FIFO empty or not. If SET, TX FIFO is empty.
 *            - IR_FLAG_TF_FULL: TX FIFO full or not. If SET, TX FIFO is full.
 *            - IR_FLAG_TX_RUN: TX run or not. If SET, TX is running.
 *            - IR_FLAG_RF_EMPTY: RX FIFO empty or not. If SET, RX FIFO is empty.
 *            - IR_FLAG_RX_RUN: RX run or not. If SET, RX is running.
 *
 * \return  The new state of IR_FLAG.
 * \retval SET: The specified IR flag is set.
 * \retval RESET: The specified IR flag is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     //add user code here.
 *     while (IR_GetFlagStatus(IR_FLAG_TF_EMPTY) == RESET);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE FlagStatus IR_GetFlagStatus(uint32_t IR_FLAG)
{
    FlagStatus bitstatus = RESET;

    /* Check the parameters */
    assert_param(IS_IR_FLAG(IR_FLAG));

    if (IR->TX_CONFIG & IR_MODE_SEL_Msk)
    {
        if (IR->RX_SR & IR_FLAG)
        {
            bitstatus = SET;
        }
        return bitstatus;
    }
    else
    {
        if (IR->TX_SR & IR_FLAG)
        {
            bitstatus = SET;
        }
        return bitstatus;
    }
}


/**
 *
 * \brief   Set or reset TX waveform definition inverse.
 *
 * \param[in] NewState: This parameter can be: ENABLE or DISABLE.
 *            This parameter can be one of the following values:
 *            - ENABLE: IR TX waveform definition is inversed. Mark waveform changes to space, space waveform changes to mark.
 *            - DISABLE: IR TX waveform definition is not inversed.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     IR_SetTxInverse(ENABLE);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void IR_SetTxInverse(FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState == ENABLE)
    {
        IR->TX_CONFIG |= IR_FIFO_INVERSE_Msk;
    }
    else
    {
        IR->TX_CONFIG &= IR_FIFO_INVERSE_CLR;
    }
}


/**
 *
 * \brief  Enable or disable TX waveform inverse.
 *
 * \param[in] NewState: This parameter can be: ENABLE or DISABLE.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable TX waveform inverse. Mark: The change from high duty to low duty is now a change from low duty to high duty. Space output low changes to output high.
 *            - DISABLE: Disable TX waveform inverse.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void ir_demo(void)
 * {
 *     IR_TxOutputInverse(ENABLE);
 * }
 * \endcode
 */
__STATIC_ALWAYS_INLINE void IR_TxOutputInverse(FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState == ENABLE)
    {
        IR->TX_CONFIG |= IR_OUTPUT_INVERSE_Msk;
    }
    else
    {
        IR->TX_CONFIG &= IR_OUTPUT_INVERSE_CLR;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* _RTL876x_IR_H_ */

/** @} */ /* End of group 87x3e_IR_Exported_Functions */
/** @} */ /* End of group 87x3e_IR */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

