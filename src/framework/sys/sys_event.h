/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _SYS_EVENT_H_
#define _SYS_EVENT_H_

#include <stdint.h>
#include <stdbool.h>

#include "os_queue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    SYS_EVENT_FLAG_OPT_CLEAR = 0x00,
    SYS_EVENT_FLAG_OPT_SET   = 0x01,
} T_SYS_EVENT_FLAG_OPT;

typedef enum
{
    SYS_EVENT_FLAG_TYPE_CLEAR_AND    = 0x00,
    SYS_EVENT_FLAG_TYPE_CLEAR_OR     = 0x01,
    SYS_EVENT_FLAG_TYPE_SET_AND      = 0x02,
    SYS_EVENT_FLAG_TYPE_SET_OR       = 0x03,
} T_SYS_EVENT_FLAG_TYPE;

typedef void (*P_SYS_EVENT_NODE_CBACK)(void *handle);
typedef void *T_SYS_EVENT_GROUP_HANDLE;
typedef void *T_SYS_EVENT_NODE_HANDLE;

bool sys_event_init(void);

void sys_event_deinit(void);

T_SYS_EVENT_GROUP_HANDLE sys_event_group_create(uint32_t flags);

bool sys_event_group_delete(T_SYS_EVENT_GROUP_HANDLE handle);

T_SYS_EVENT_NODE_HANDLE sys_event_flag_wait(T_SYS_EVENT_GROUP_HANDLE handle,
                                            P_SYS_EVENT_NODE_CBACK  cback,
                                            void                     *owner,
                                            uint32_t                 flags,
                                            T_SYS_EVENT_FLAG_TYPE    type);

bool sys_event_flag_abort(T_SYS_EVENT_NODE_HANDLE handle);

bool sys_event_flag_check(T_SYS_EVENT_GROUP_HANDLE handle, uint32_t flags);

uint32_t sys_event_flag_post(T_SYS_EVENT_GROUP_HANDLE handle,
                             T_SYS_EVENT_FLAG_OPT     opt,
                             uint32_t              flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_EVENT_H_ */
