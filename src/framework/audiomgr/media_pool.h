/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _MEDIA_POOL_H_
#define _MEDIA_POOL_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void *T_MEDIA_POOL_HANDLE;

T_MEDIA_POOL_HANDLE media_pool_create(uint16_t size);

void media_pool_destory(T_MEDIA_POOL_HANDLE handle);

bool media_pool_reset(T_MEDIA_POOL_HANDLE handle);

void *media_buffer_get(T_MEDIA_POOL_HANDLE handle, uint16_t buf_size);

bool media_buffer_put(T_MEDIA_POOL_HANDLE handle, void *p_buf);

void media_pool_dump(T_MEDIA_POOL_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MEDIA_POOL_H_ */
