#ifndef _APP_VENDOR_CFG_H_
#define _APP_VENDOR_CFG_H_

#include <stdint.h>
#include <stdbool.h>

void app_vendor_cfg_get_capability(uint8_t cap[32]);

bool app_vendor_cfg_is_audio_hid(void);

void app_vendor_cfg_load(void);

#endif
