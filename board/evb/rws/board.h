/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file     board.h
* @brief    Peripheral definitions for the project
* @details
* @author   Chuanguo Xue
* @date     2015-4-7
* @version  v1.0
* *********************************************************************************************************
*/

#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_NULL                0
#define KEY0_MASK               0x01
#define KEY1_MASK               0x02
#define KEY2_MASK               0x04
#define KEY3_MASK               0x08
#define KEY4_MASK               0x10
#define KEY5_MASK               0x20
#define KEY6_MASK               0x40
#define KEY7_MASK               0x80

#define KEY_RELEASE             0
#define KEY_PRESS               1

#define MAX_KEY_NUM             8
#define HYBRID_KEY_NUM          8
#define RWS_KEY_NUM             8

#define LED_NUM                 3 //MAX: 3 LED
#define LED_DEMO_NUM            6 //MAX: 6 LED
#define SLEEP_LED_PINMUX_NUM    10

#define DSP_TIMER               TIM7 //DSP HW fixed at timer7

#define UART_RX_BUFFER_SIZE         1024
#define RX_GDMA_START_ADDR          UART_RX_BUFFER_SIZE
#define RX_GDMA_BUFFER_SIZE         300


/** @defgroup IO Driver Config
  * @note user must config it firstly!! Do not change macro names!!
  * @{
  */

/**
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /* _BOARD_H_ */

