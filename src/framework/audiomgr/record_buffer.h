/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _RECORD_BUFFER_H_
#define _RECORD_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const T_MEDIABUFFER_OP record_buffer_ops;

void *record_buffer_peek(int index, uint16_t *len, uint32_t *timestamp);

uint16_t record_buffer_shrink(uint16_t len);

bool record_buffer_write(void *buf, uint16_t len, void *param);

uint16_t record_buffer_read(void *buf, uint16_t len, uint32_t *timestamp);

bool record_buffer_flush(uint16_t cnt, void *param);

void record_buffer_reset(void);

void record_buffer_dump(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RECORD_BUFFER_H_ */
