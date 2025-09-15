/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_ADP_CMD_PARSE_H_
#define _APP_ADP_CMD_PARSE_H_

#include <stdbool.h>
#include "btm.h"
#include "sysm.h"

#ifdef __cplusplus
extern "C" {
#endif

#if F_APP_ADP_5V_CMD_SUPPORT
#define ADP_CMD_DBG                     0

#define MAX_BIT_CNT                     32
#define GUART_BIT_TIM_TOLERANCE         50000

/*usb in out */
#define ADP_USB_IN_PLAYLOAD         0x05
#define ADP_USB_OUT                 0x0a70
#define ADP_USB_IN_PRAR             0x0a05

typedef struct
{
    uint8_t cmd_bit_num;
    uint8_t cmd_bit_width;          // uint: ms
    uint32_t cmd_guard_time;        // uint: us
    uint32_t cmd_detect_timeout;    // uint: us
    uint32_t cmd_max_inaccuracy;    // uint: us
} T_ADP_CMD_PARSE_PARAMETER;

typedef struct _T_ADP_CMD_PARSE_STRUCT
{
    uint8_t hw_timer_started : 1;
    uint8_t tim_cnt_finish : 1;
    uint8_t usb_start_wait_en : 1;
    uint8_t usb_in_cmd_det : 1;
    uint8_t usb_started : 1;
    uint8_t usb_cmd_rec : 1;
    uint8_t rev3 : 2;
    uint8_t usb_start_wait_tim_cnt;
    uint8_t total_bit_pushed ;
    uint32_t tim_prev_value;
    uint32_t cmd_out;
} T_ADP_CMD_PARSE_STRUCT;

typedef struct
{
    uint32_t bit_data : 1;
    uint32_t tim_delta_value : 31;
} T_ADP_INT_DATA;

typedef enum
{
    CHARGER_BOX_CMD_SET_9BITS   = 0,
    CHARGER_BOX_CMD_SET_15BITS  = 1,
    CHARGER_BOX_CMD_SET_RSV1    = 2,
    CHARGER_BOX_CMD_SET_RSV2    = 3,
} CHARGER_BOX_CMD_SET;

typedef enum
{
    CHARGER_BOX_BIT_LENGTH_40MS = 0,
    CHARGER_BOX_BIT_LENGTH_20MS = 1,
} CHARGER_BOX_BIT_LENGTH;

typedef void (*P_ADP_CMD_RAW_DATA_PARSE_CBACK)(uint32_t cmd_data);

void app_adp_cmd_parse_handle_msg(uint32_t cmd_data);

void app_adp_cmd_parse_para_set(uint8_t cmd_bit_num, uint8_t cmd_bit_length,
                                P_ADP_CMD_RAW_DATA_PARSE_CBACK cback);

void app_adp_cmd_parse_init(P_ADP_CMD_RAW_DATA_PARSE_CBACK cmd_handle);
void app_adp_cmd_parse_int_handle(T_IO_MSG *io_driver_msg_recv);

void app_adp_cmd_parse_hw_timer_handler(void);

void app_adp_cmd_parse_protect(void);

void app_adp_cmd_parse_handle_usb_cmd(uint8_t cmd, uint8_t payload);

bool app_adp_cmd_parse_get_usb_cmd_rec(void);

bool app_adp_cmd_parse_process(uint32_t cmd_data, uint8_t *p_cmd_index, uint8_t *p_cmd_payload);

#endif

#ifdef __cplusplus
}
#endif

#endif /*_APP_ADP_CMD_PARSE_H_*/

