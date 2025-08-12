#ifndef _APP_DONGLE_DATA_CTRL_H_
#define _APP_DONGLE_DATA_CTRL_H_

#include "app_gaming_sync.h"

#if F_APP_COMMON_DONGLE_SUPPORT
bool app_dongle_send_cmd(T_APP_DONGLE_CMD cmd, uint8_t *data, uint8_t len);
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
void app_dongle_handle_le_data(uint8_t *data, uint16_t len);
void app_gaming_ctrl_data_rcv(uint8_t *data, uint16_t len);
bool app_gaming_ctrl_data_send_to_dongle(uint8_t *data, uint16_t size, bool flushable);
void app_gaming_handle_usb_spk_vol(void);
void app_dongle_send_vol_ctrl_cmd(uint8_t vol_cmd);
void app_dongle_data_ctrl_init(void);
#endif

#endif
