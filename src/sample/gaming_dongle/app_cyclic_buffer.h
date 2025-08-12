/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef __APP_CYCLIC_BUFFER_H__
#define __APP_CYCLIC_BUFFER_H__

#include <stdbool.h>
#include <stdint.h>
#include "os_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_cyclic_buf
{
    uint16_t read_idx;
    uint16_t write_idx;
    uint16_t len;
    uint8_t  *buf;
} T_CYCLIC_BUF;

bool cyclic_buf_init(T_CYCLIC_BUF *cyclic, uint16_t len);
void cyclic_buf_deinit(T_CYCLIC_BUF *cyclic);
uint16_t cyclic_buf_room(T_CYCLIC_BUF *cyclic);
uint16_t cyclic_buf_count(T_CYCLIC_BUF *cyclic);
bool cyclic_buf_peek(T_CYCLIC_BUF *cyclic, uint8_t *buf, uint16_t len);
bool cyclic_buf_read(T_CYCLIC_BUF *cyclic, uint8_t *buf, uint16_t len);
bool cyclic_buf_drop(T_CYCLIC_BUF *cyclic, uint16_t len);
bool cyclic_buf_write(T_CYCLIC_BUF *cyclic, uint8_t *buf, uint16_t len);
void cyclic_buf_destroy(T_CYCLIC_BUF *cyclic);

#ifdef __cplusplus
}
#endif

#endif /* __APP_CYCLIC_BUFFER_H__ */
