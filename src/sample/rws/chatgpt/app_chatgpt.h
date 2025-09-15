/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CHATGPT_H_
#define _APP_CHATGPT_H_

#include <string.h>
#include <stdlib.h>
#include "sysm.h"
#include "trace.h"
#include "board.h"
#include "os_mem.h"
#include "os_sched.h"
#include "storage.h"
#include "gap_conn_le.h"
#include "crc16btx.h"
#include "audio.h"
#include "audio_track.h"
#include "transmit_service.h"
#include "pm.h"

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
#include "profile_server_ext.h"
#else
#include "profile_server.h"
#endif

#include "app_chatgpt_transport.h"
#include "app_chatgpt_ble.h"
#include "app_chatgpt_process.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void app_chatgpt_start(void);
void app_chatgpt_stop(void);
void app_chatgpt_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
