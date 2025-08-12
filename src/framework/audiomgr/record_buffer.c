/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "os_mem.h"
#include "os_queue.h"
#include "os_sched.h"
#include "trace.h"
#include "audio_mgr.h"
#include "media_buffer.h"
#include "record_buffer.h"

typedef struct t_media_record_header
{
    struct t_media_record_header *p_next;
    uint32_t                      timestamp;
    uint16_t                      seq;
    uint16_t                      len;
    uint16_t                      offset;
    uint16_t                      rsvd[2];
    uint8_t                       data[0];
} T_MEDIA_RECORD_HEADER;


static T_OS_QUEUE record_queue;

void *record_buffer_peek(int index, uint16_t *len, uint32_t *timestamp)
{
    T_MEDIA_RECORD_HEADER *record_buf;
    void                  *buf = NULL;

    record_buf = os_queue_peek(&record_queue, index);
    if (record_buf != NULL)
    {
        buf = record_buf->data + record_buf->offset;

        if (len != NULL)
        {
            *len = record_buf->len - record_buf->offset;
        }

        if (timestamp != NULL)
        {
            *timestamp = record_buf->timestamp;
        }
    }

    return buf;
}

uint16_t record_buffer_shrink(uint16_t len)
{
    T_MEDIA_RECORD_HEADER *record_buf;
    uint16_t               remaining_len;
    uint16_t               shrink_len = 0;

    record_buf = os_queue_peek(&record_queue, 0);
    if (record_buf != NULL)
    {
        remaining_len = record_buf->len - record_buf->offset;
        if (remaining_len >= len)
        {
            record_buf->offset += len;
            shrink_len          = len;
        }
        else
        {
            record_buf->offset += remaining_len;
            shrink_len          = remaining_len;
        }

        if (remaining_len == shrink_len)
        {
            os_queue_delete(&record_queue, record_buf);
            media_buffer_put(audio_db->record_pool_handle, record_buf);
        }
    }

    return shrink_len;
}

bool record_buffer_write(void *buf, uint16_t len, void *param)
{
    T_MEDIA_RECORD_HEADER *record_buf;

    record_buf = media_buffer_get(audio_db->record_pool_handle, len + sizeof(T_MEDIA_RECORD_HEADER));
    if (record_buf != NULL)
    {
        record_buf->timestamp = os_sys_time_get();
        record_buf->seq       = 0; /* TODO increase */
        record_buf->len       = len;
        record_buf->offset    = 0;
        memcpy(record_buf->data, buf, len);

        os_queue_in(&record_queue, record_buf);
        return true;
    }

    return false;
}

uint16_t record_buffer_read(void *buf, uint16_t len, uint32_t *timestamp)
{
    T_MEDIA_RECORD_HEADER *record_buf;
    uint16_t               remaining_len;
    uint16_t               read_len = 0;

    record_buf = os_queue_peek(&record_queue, 0);
    if (record_buf != NULL)
    {
        if (timestamp != NULL)
        {
            *timestamp = record_buf->timestamp;
        }

        remaining_len = record_buf->len - record_buf->offset;
        if (remaining_len >= len)
        {
            memcpy(buf, record_buf->data + record_buf->offset, len);
            record_buf->offset += len;
            read_len            = len;
        }
        else
        {
            memcpy(buf, record_buf->data + record_buf->offset, remaining_len);
            record_buf->offset += remaining_len;
            read_len            = remaining_len;
        }

        if (remaining_len == read_len)
        {
            os_queue_delete(&record_queue, record_buf);
            media_buffer_put(audio_db->record_pool_handle, record_buf);
        }
    }
    else
    {
        if (timestamp != NULL)
        {
            *timestamp = os_sys_time_get();
        }
    }

    return read_len;
}

bool record_buffer_flush(uint16_t cnt, void *param)
{
    T_MEDIA_RECORD_HEADER *record_buf;
    uint16_t               i;

    for (i = 0; i < cnt; i++)
    {
        record_buf = os_queue_out(&record_queue);
        if (record_buf != NULL)
        {
            media_buffer_put(audio_db->record_pool_handle, record_buf);
        }
        else
        {
            /* Record queue is empty */
            break;
        }
    }

    return true;
}

void record_buffer_reset(void)
{
    record_buffer_flush(record_queue.count, NULL);
}

void record_buffer_dump(void)
{
    /* TODO */
}

const T_MEDIABUFFER_OP record_buffer_ops =
{
    .buffer_type    = MEDIA_BUFFER_TYPE_RECORD,
    .peek_fun       = record_buffer_peek,
    .shrink_fun     = record_buffer_shrink,
    .write_fun      = record_buffer_write,
    .read_fun       = record_buffer_read,
    .flush_fun      = record_buffer_flush,
    .reset_fun      = record_buffer_reset,
    .dump_fun       = record_buffer_dump,
};
