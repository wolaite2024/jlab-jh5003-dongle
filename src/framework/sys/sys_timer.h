/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _SYS_TIMER_H_
#define _SYS_TIMER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    SYS_TIMER_TYPE_LOW_PRECISION    = 0x00,
    SYS_TIMER_TYPE_HIGH_PRECISION   = 0x01,
} T_SYS_TIMER_TYPE;

typedef void *T_SYS_TIMER_HANDLE;

typedef void (*P_SYS_TIMER_CBACK)(T_SYS_TIMER_HANDLE handle);

bool sys_timer_init(void);

uint32_t sys_timer_id_get(T_SYS_TIMER_HANDLE handle);

T_SYS_TIMER_HANDLE sys_timer_create(const char *timer_name,
                                    T_SYS_TIMER_TYPE timer_type,
                                    uint32_t timer_id,
                                    uint32_t timeout_us,
                                    bool reload,
                                    P_SYS_TIMER_CBACK cback);

bool sys_timer_start(T_SYS_TIMER_HANDLE handle);

bool sys_timer_restart(T_SYS_TIMER_HANDLE handle, uint32_t timeout_us);

bool sys_timer_stop(T_SYS_TIMER_HANDLE handle);

bool sys_timer_delete(T_SYS_TIMER_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_TIMER_H_ */
