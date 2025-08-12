/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_LINE_IN_H_
#define _APP_LINE_IN_H_

#include <stdbool.h>
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void app_line_in_driver_init(void);
void app_line_in_power_on_check(void);
void app_line_in_power_off_check(void);
void app_line_in_detect_intr_callback(uint32_t param);
void app_line_in_detect_msg_handler(T_IO_MSG *io_io_driver_msg_recv);
bool app_line_in_is_playing(void);
void app_line_in_call_status_update(bool is_idle);

bool app_line_in_plug_in(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_LINE_IN_H_ */
