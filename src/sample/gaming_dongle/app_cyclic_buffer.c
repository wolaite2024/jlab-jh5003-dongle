/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "app_cyclic_buffer.h"
#include "section.h"

bool cyclic_buf_init(T_CYCLIC_BUF *cyclic, uint16_t len)
{
    if (!cyclic)
    {
        return false;
    }

    cyclic->read_idx = 0;
    cyclic->write_idx = 0;
    cyclic->len  = len;
    cyclic->buf  = calloc(1, cyclic->len);

    if (!cyclic->buf)
    {
        APP_PRINT_ERROR0("cyclic_buf_init: Cannot alloc mem for cyclic buf");
        return false;
    }

    return true;
}

void cyclic_buf_destroy(T_CYCLIC_BUF *cyclic)
{
    if (!cyclic)
    {
        return;
    }

    if (cyclic->buf)
    {
        free(cyclic->buf);
    }

    cyclic->read_idx = 0;
    cyclic->write_idx = 0;
    cyclic->len  = 0;
    cyclic->buf  = NULL;
}


void cyclic_buf_deinit(T_CYCLIC_BUF *cyclic)
{
    if (!cyclic)
    {
        return;
    }

    if (cyclic->buf)
    {
        free(cyclic->buf);
    }

    cyclic->read_idx = 0;
    cyclic->write_idx = 0;
    cyclic->len  = 0;
    cyclic->buf  = NULL;
}

RAM_TEXT_SECTION
uint16_t cyclic_buf_count(T_CYCLIC_BUF *cyclic)
{
    if (!cyclic || !cyclic->buf)
    {
        return 0;
    }

    return ((cyclic->write_idx + cyclic->len - cyclic->read_idx) % cyclic->len);
}

RAM_TEXT_SECTION
uint16_t cyclic_buf_room(T_CYCLIC_BUF *cyclic)
{
    if (!cyclic || !cyclic->buf)
    {
        return 0;
    }

    return ((cyclic->read_idx + cyclic->len - (cyclic->write_idx + 1)) % cyclic->len);
}

bool cyclic_buf_peek(T_CYCLIC_BUF *cyclic, uint8_t *buf, uint16_t len)
{
    if (!cyclic || !cyclic->buf)
    {
        return false;
    }

    if (len > cyclic_buf_count(cyclic))
    {
        APP_PRINT_ERROR2("cyclic_buf_peek: No enough data to peek, req %u, %u",
                         len, cyclic->len);
        return false;
    }

    if (cyclic->read_idx + len <= cyclic->len)
    {
        memcpy(buf, cyclic->buf + cyclic->read_idx, len);
    }
    else
    {
        uint16_t tlen = cyclic->len - cyclic->read_idx;

        memcpy(buf, cyclic->buf + cyclic->read_idx, tlen);
        memcpy(buf + tlen, cyclic->buf, len - tlen);
    }

    return true;
}

RAM_TEXT_SECTION
bool cyclic_buf_read(T_CYCLIC_BUF *cyclic, uint8_t *buf, uint16_t len)
{
    if (!cyclic || !cyclic->buf)
    {
        return false;
    }

    if (len > cyclic_buf_count(cyclic))
    {
        APP_PRINT_ERROR2("cyclic_buf_read: No enough data to read, req %u, %u",
                         len, cyclic->len);
        return false;
    }

    if (cyclic->read_idx + len <= cyclic->len)
    {
        memcpy(buf, cyclic->buf + cyclic->read_idx, len);
        cyclic->read_idx += len;
    }
    else
    {
        uint16_t tlen = cyclic->len - cyclic->read_idx;

        memcpy(buf, cyclic->buf + cyclic->read_idx, tlen);
        memcpy(buf + tlen, cyclic->buf, len - tlen);
        cyclic->read_idx = len - tlen;
    }

    cyclic->read_idx %= cyclic->len;

    return true;
}

RAM_TEXT_SECTION
bool cyclic_buf_drop(T_CYCLIC_BUF *cyclic, uint16_t len)
{
    if (!cyclic || !cyclic->buf)
    {
        return false;
    }

    if (len > cyclic_buf_count(cyclic))
    {
        APP_PRINT_ERROR2("cyclic_buf_drop: No enough data to drop, req %u, %u",
                         len, cyclic->len);
        len = cyclic_buf_count(cyclic);
    }

    if (cyclic->read_idx + len <= cyclic->len)
    {
        cyclic->read_idx += len;
    }
    else
    {
        uint16_t tlen = cyclic->len - cyclic->read_idx;

        cyclic->read_idx = len - tlen;
    }

    cyclic->read_idx %= cyclic->len;

    return true;
}

RAM_TEXT_SECTION
bool cyclic_buf_write(T_CYCLIC_BUF *cyclic, uint8_t *buf, uint16_t len)
{
    if (!cyclic || !cyclic->buf)
    {
        return false;
    }

    if (cyclic_buf_room(cyclic) < len)
    {
        return false;
    }

    if (cyclic->write_idx + len <= cyclic->len)
    {
        memcpy(cyclic->buf + cyclic->write_idx, buf, len);
        cyclic->write_idx += len;
    }
    else
    {
        uint16_t tlen = cyclic->len - cyclic->write_idx;

        memcpy(cyclic->buf + cyclic->write_idx, buf, tlen);
        memcpy(cyclic->buf, buf + tlen, len - tlen);
        cyclic->write_idx = len - tlen;
    }

    cyclic->write_idx %= cyclic->len;

    return true;
}
