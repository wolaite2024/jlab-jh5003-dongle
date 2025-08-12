/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _SYS_MGR_H_
#define _SYS_MGR_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SYS_IPC_TOPIC "SYS_TOPIC"

typedef enum
{
    SYS_IPC_POWER_ON        = 0x00000000,
    SYS_IPC_POWER_OFF       = 0x00000001,
} T_SYS_IPC_ID;

bool sys_mgr_event_register(uint8_t event, void (*entry)(void));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_MGR_H_ */
