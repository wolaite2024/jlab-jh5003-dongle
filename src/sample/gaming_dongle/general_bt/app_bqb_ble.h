#ifndef _APP_BQB_BLE_H_
#define _APP_BQB_BLE_H_

#include <stdint.h>
#include <string.h>

#include "app_msg.h"
#include "gap.h"


void app_bqb_ble_handle_gap_msg(T_IO_MSG *io_msg);

void app_bqb_ble_init(void);

void app_handle_hogp_cmd(uint8_t action, uint8_t *bd_addr);

#endif
