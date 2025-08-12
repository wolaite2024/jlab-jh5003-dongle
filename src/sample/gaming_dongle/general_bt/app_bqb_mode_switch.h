#ifndef _APP_BQB_MODE_SWITCH_H_
#define _APP_BQB_MODE_SWITCH_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_cfg.h"

#ifdef BTDONGLE_BQB_MODE_ENABLE

bool app_is_bqb_mode(void);

void app_bqb_mode_load(void);

void app_enable_bqb_mode(void);

void app_disable_bqb_mode(void);

#endif
#endif
