/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_SWITCH_DETECT_H_
#define _APP_SWITCH_DETECT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "app_io_msg.h"

#define AUX_SWITCH_TX 1
#define AUX_SWITCH_RX 2
#define AUX_DETECT_IN 1
#define AUX_DETECT_OUT 2

void app_switch_detect_init(void);
void gpio_aux_switch_intr_handler(void);
void gpio_aux_detect_intr_handler(void);
void app_switch_detect_handle_msg(T_IO_MSG *io_driver_msg_recv);

//app_aux_switch_state_get return value:
//AUX_SWITCH_TX for Tx , AUX_SWITCH_RX for Rx , 0 for not support
uint8_t app_aux_switch_state_get(void);
//app_aux_detect_state_get return value:
//AUX_DETECT_IN for in, AUX_DETECT_OUT for out , 0 for not support
uint8_t app_aux_detect_state_get(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SWITCH_DETECT_H_ */
