/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>

#include "os_queue.h"
#include "os_mem.h"
#include "os_msg.h"
#include "trace.h"
#include "section.h"
#include "notification_buffer.h"

/* TODO Remove Start */
#include "sys_mgr.h"
#include "app_msg.h"
#include "fmc_api.h"
extern void *hEventQueueHandleAu;
/* TODO Remove End */

#define NOTIFICATION_MSG_MAX_NUM        0x08
#define NOTIFICATION_MSG_TYPE_FRAME     0x0000
#define NOTIFICATION_MSG_TYPE_STREAM    0x0001

typedef struct t_notification_queue_msg
{
    uint8_t     type;
    uint8_t     subtype;
    uint16_t    param1;
    uint32_t    param2;
} T_NOTIFICATION_QUEUE_MSG;

#define NOTIFICATION_POOL_SIZE      (1024 * 8)

#define NOTIFICATION_FRAME_LEN_SIZE (2)

typedef struct t_notification_frame_head
{
    struct t_notification_frame_head *next;
    uint32_t                          frame_size;
} T_NOTIFICATION_FRAME_HEAD;

typedef enum t_notification_buffer_state
{
    NOTIFICATION_BUFFER_STATE_IDLE      = 0x00, /* buffer initial state */
    NOTIFICATION_BUFFER_STATE_PREQUEUE  = 0x01, /* buffer under the state transition from Idle to Play */
    NOTIFICATION_BUFFER_STATE_EMPTY     = 0x02, /* buffer empty state */
    NOTIFICATION_BUFFER_STATE_PLAY      = 0x03, /* buffer running state */
    NOTIFICATION_BUFFER_STATE_HIGH      = 0x04, /* buffer high state when reaching the high watermark */
} T_NOTIFICATION_BUFFER_STATE;

typedef struct t_notification_buffer_db
{
    void                        *owner;
    P_NOTIFICATION_BUFFER_CBACK  cback;
    T_OS_QUEUE                   queue;
    uint8_t                     *pool_buf;
    uint32_t                     pool_size;
    uint32_t                     pool_head;
    uint32_t                     pool_tail;
    uint32_t                     upper_threshold;
    uint32_t                     low_threshold;
    uint32_t                     prequeue_threshold;
    uint32_t                     cached_size;
    void                        *msg_queue;
    T_NOTIFICATION_BUFFER_STATE  state;
} T_NOTIFICATION_BUFFER_DB;

static T_NOTIFICATION_BUFFER_DB *buffer_db = NULL;

static void *notification_buffer_frame_peek(uint32_t *len)
{
    T_NOTIFICATION_FRAME_HEAD *p_head;
    void                      *p_frame = NULL;

    p_head = os_queue_peek(&buffer_db->queue, 0);
    if (p_head != NULL)
    {
        p_frame = (uint8_t *)p_head + sizeof(T_NOTIFICATION_FRAME_HEAD);
        *len    = p_head->frame_size;
    }

    return p_frame;
}

static bool notification_buffer_frame_write(void *data, uint32_t len)
{
    T_NOTIFICATION_FRAME_HEAD *p_head;
    uint32_t                   total_len;

    /* 4 bytes alignment */
    total_len = (sizeof(T_NOTIFICATION_FRAME_HEAD) + len + 0x03) & (~0x03);

    if (buffer_db->pool_tail + total_len > buffer_db->pool_size - 1)
    {
        if (total_len <= buffer_db->pool_head)
        {
            p_head = (T_NOTIFICATION_FRAME_HEAD *)buffer_db->pool_buf;
            memcpy(buffer_db->pool_buf + sizeof(T_NOTIFICATION_FRAME_HEAD), data, len);
            buffer_db->pool_tail = total_len;
            p_head->frame_size   = len;
            os_queue_in(&buffer_db->queue, p_head);
        }
        else
        {
            if (buffer_db->cback)
            {
                buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_FULL, buffer_db->cached_size);
            }

            return false;
        }
    }
    else
    {
        if (buffer_db->pool_tail >= buffer_db->pool_head ||
            buffer_db->pool_tail + total_len <= buffer_db->pool_head)
        {
            p_head = (T_NOTIFICATION_FRAME_HEAD *)(buffer_db->pool_buf + buffer_db->pool_tail);
            memcpy(buffer_db->pool_buf + buffer_db->pool_tail + sizeof(T_NOTIFICATION_FRAME_HEAD), data, len);
            buffer_db->pool_tail += total_len;
            p_head->frame_size    = len;
            os_queue_in(&buffer_db->queue, p_head);
        }
        else
        {
            if (buffer_db->cback)
            {
                buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_FULL, buffer_db->cached_size);
            }

            return false;
        }
    }

    buffer_db->cached_size += len;

    switch (buffer_db->state)
    {
    case NOTIFICATION_BUFFER_STATE_IDLE:
        if (buffer_db->cached_size > 0)
        {
            buffer_db->state = NOTIFICATION_BUFFER_STATE_PREQUEUE;
        }
        break;

    case NOTIFICATION_BUFFER_STATE_PREQUEUE:
        if (buffer_db->cached_size >= buffer_db->prequeue_threshold)
        {
            buffer_db->state = NOTIFICATION_BUFFER_STATE_PLAY;

            if (buffer_db->cback)
            {
                buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_PLAY, buffer_db->cached_size);
            }
        }
        break;

    case NOTIFICATION_BUFFER_STATE_EMPTY:
        if (buffer_db->cached_size >= buffer_db->low_threshold)
        {
            buffer_db->state = NOTIFICATION_BUFFER_STATE_PLAY;

            if (buffer_db->cback)
            {
                buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_E_PLAY, buffer_db->cached_size);
            }
        }
        break;

    case NOTIFICATION_BUFFER_STATE_PLAY:
        if (buffer_db->cached_size >= buffer_db->upper_threshold)
        {
            buffer_db->state = NOTIFICATION_BUFFER_STATE_HIGH;

            if (buffer_db->cback)
            {
                buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_HIGH, buffer_db->cached_size);
            }
        }
        break;

    case NOTIFICATION_BUFFER_STATE_HIGH:
        break;
    }

    return true;
}

static bool notification_buffer_frame_flush(void)
{
    T_NOTIFICATION_FRAME_HEAD *head;

    head = os_queue_out(&buffer_db->queue);
    if (head != NULL)
    {
        uint32_t frame_size = head->frame_size;

        head = os_queue_peek(&buffer_db->queue, 0);
        if (head != NULL)
        {
            buffer_db->pool_head = (uint32_t)head - (uint32_t)buffer_db->pool_buf;
        }
        else
        {
            buffer_db->pool_head = buffer_db->pool_tail;
        }

        buffer_db->cached_size -= frame_size;

        switch (buffer_db->state)
        {
        case NOTIFICATION_BUFFER_STATE_PLAY:
            if (buffer_db->cached_size == 0)
            {
                buffer_db->state = NOTIFICATION_BUFFER_STATE_EMPTY;

                if (buffer_db->cback)
                {
                    buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_EMPTY, buffer_db->cached_size);
                }
            }
            else if (buffer_db->cached_size < buffer_db->low_threshold)
            {
                if (buffer_db->cback)
                {
                    buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_LOW, buffer_db->cached_size);
                }
            }
            break;

        case NOTIFICATION_BUFFER_STATE_HIGH:
            if (buffer_db->cached_size < buffer_db->upper_threshold)
            {
                buffer_db->state = NOTIFICATION_BUFFER_STATE_PLAY;

                if (buffer_db->cback)
                {
                    buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_F_PLAY, buffer_db->cached_size);
                }
            }
            break;

        default:
            break;
        }
    }

    return true;
}

static bool notification_buffer_frame_push(uint32_t buf, uint32_t offset, uint16_t len)
{
    uint8_t *frame;

    AUDIO_PRINT_TRACE4("notification_buffer_frame_push: state 0x%02x, buf 0x%08x, offset 0x%08x, len 0x%04x",
                       buffer_db->state, buf, offset, len);

    frame = (uint8_t *)buf + offset;

    if ((len == 2) && (frame[0] == 0xFF) && (frame[1] == 0xFF))
    {
        if (buffer_db->state == NOTIFICATION_BUFFER_STATE_HIGH)
        {
            if (notification_buffer_frame_write(frame, len) == false)
            {
                notification_buffer_frame_flush();
            }
        }
        else if (buffer_db->state == NOTIFICATION_BUFFER_STATE_EMPTY)
        {
            if (notification_buffer_frame_write(frame, len))
            {
                buffer_db->state = NOTIFICATION_BUFFER_STATE_PLAY;

                if (buffer_db->cback)
                {
                    buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_E_PLAY, 0);
                }

                return true;
            }
            else
            {
                return false;
            }
        }
    }

    return notification_buffer_frame_write(frame, len);
}

static void *notification_buffer_stream_peek(uint32_t *len)
{
    uint32_t  pool_head;
    uint32_t  pool_tail;
    void     *p_frame = NULL;
    uint16_t  frame_len = 0;
    uint32_t  frame_size = 0;

    pool_head = buffer_db->pool_head;
    pool_tail = buffer_db->pool_tail;

    if (pool_head + NOTIFICATION_FRAME_LEN_SIZE <= pool_tail)
    {
        frame_len = (buffer_db->pool_buf[pool_head] << 8) | buffer_db->pool_buf[pool_head + 1];
        if (frame_len == 0xFFFF)
        {
            p_frame    = buffer_db->pool_buf + pool_head;
            frame_size = NOTIFICATION_FRAME_LEN_SIZE;
        }
        else if (pool_head + frame_len + NOTIFICATION_FRAME_LEN_SIZE <= pool_tail)
        {
            p_frame    = buffer_db->pool_buf + pool_head;
            frame_size = frame_len + NOTIFICATION_FRAME_LEN_SIZE;
        }
    }

    *len = frame_size;

    AUDIO_PRINT_TRACE5("notification_buffer_stream_peek: pool_head 0x%08x, pool_tail 0x%08x, p_frame %p, frame_len 0x%08x, frame_size 0x%08x",
                       pool_head, pool_tail, p_frame, frame_len, frame_size);

    return p_frame;
}

RAM_TEXT_SECTION static void notification_buffer_stream_write(void)
{
    uint8_t                  event;
    T_NOTIFICATION_QUEUE_MSG msg;

    event       = EVENT_NOTIFICATION_MSG;
    msg.type    = NOTIFICATION_MSG_TYPE_STREAM;
    msg.subtype = 0;
    msg.param1  = 0;
    msg.param2  = 0;

    AUDIO_PRINT_TRACE0("notification_buffer_stream_write");

    if (hEventQueueHandleAu != NULL)
    {
        if (os_msg_send(buffer_db->msg_queue, &msg, 0) == true)
        {
            os_msg_send(hEventQueueHandleAu, &event, 0);
        }
    }
}

static bool notification_buffer_stream_flush(void)
{
    uint32_t pool_head;
    uint32_t pool_tail;
    uint16_t frame_len;
    uint32_t frame_size = 0;

    pool_head = buffer_db->pool_head;
    pool_tail = buffer_db->pool_tail;

    if (pool_head + NOTIFICATION_FRAME_LEN_SIZE <= pool_tail)
    {
        frame_len = (buffer_db->pool_buf[pool_head] << 8) | buffer_db->pool_buf[pool_head + 1];
        if (frame_len == 0xFFFF)
        {
            frame_size = NOTIFICATION_FRAME_LEN_SIZE;
            buffer_db->pool_head += frame_size;
        }
        else if (pool_head + frame_len + NOTIFICATION_FRAME_LEN_SIZE <= pool_tail)
        {
            frame_size = frame_len + NOTIFICATION_FRAME_LEN_SIZE;
            buffer_db->pool_head += frame_size;
        }
    }

    if (pool_head != buffer_db->pool_head)
    {
        pool_head = buffer_db->pool_head;

        if (pool_head + NOTIFICATION_FRAME_LEN_SIZE > pool_tail)
        {
            buffer_db->state = NOTIFICATION_BUFFER_STATE_EMPTY;

            if (buffer_db->cback)
            {
                buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_EMPTY, pool_head);
            }
        }
        else
        {
            frame_len = (buffer_db->pool_buf[pool_head] << 8) | buffer_db->pool_buf[pool_head + 1];
            if (frame_len != 0xFFFF &&
                pool_head + frame_len + NOTIFICATION_FRAME_LEN_SIZE > pool_tail)
            {
                buffer_db->state = NOTIFICATION_BUFFER_STATE_EMPTY;

                if (buffer_db->cback)
                {
                    buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_EMPTY, pool_head);
                }
            }
        }

        return true;
    }

    return false;
}

static bool notification_buffer_stream_push(uint32_t buf, uint32_t offset, uint32_t len)
{
    uint32_t load_len;

    load_len = (len + 0x03) & (~0x03);
    if (load_len > buffer_db->pool_size)
    {
        load_len = buffer_db->pool_size;
    }

    buffer_db->pool_head = offset;
    buffer_db->pool_tail = load_len;

    AUDIO_PRINT_TRACE4("notification_buffer_stream_push: pool_buf %p, pool_head 0x%08x, pool_tail 0x%08x, load_len 0x%08x",
                       buffer_db->pool_buf, buffer_db->pool_head, buffer_db->pool_tail, load_len);

    if (fmc_flash_nor_auto_dma_read(buf,
                                    (uint32_t)buffer_db->pool_buf,
                                    load_len,
                                    notification_buffer_stream_write) == true)
    {
        if (buffer_db->state == NOTIFICATION_BUFFER_STATE_IDLE)
        {
            buffer_db->state = NOTIFICATION_BUFFER_STATE_PREQUEUE;
        }

        return true;
    }

    return false;
}

void notification_msg_handler(void)
{
    T_NOTIFICATION_QUEUE_MSG msg;

    if (os_msg_recv(buffer_db->msg_queue, &msg, 0) == true)
    {
        switch (msg.type)
        {
        case NOTIFICATION_MSG_TYPE_FRAME:
            break;

        case NOTIFICATION_MSG_TYPE_STREAM:
            {
                if (buffer_db->state == NOTIFICATION_BUFFER_STATE_PREQUEUE)
                {
                    buffer_db->state = NOTIFICATION_BUFFER_STATE_PLAY;

                    if (buffer_db->cback)
                    {
                        buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_PLAY, 0);
                    }
                }
                else if (buffer_db->state == NOTIFICATION_BUFFER_STATE_EMPTY)
                {
                    buffer_db->state = NOTIFICATION_BUFFER_STATE_PLAY;

                    if (buffer_db->cback)
                    {
                        buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_E_PLAY, 0);
                    }
                }
            }
            break;

        default:
            break;
        }
    }
}

bool notification_buffer_init(uint16_t pool_size)
{
    int32_t ret = 0;

    buffer_db = os_mem_zalloc2(sizeof(T_NOTIFICATION_BUFFER_DB));
    if (buffer_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    buffer_db->pool_buf = os_mem_alloc2(pool_size);
    if (buffer_db->pool_buf == NULL)
    {
        ret = 2;
        goto fail_alloc_pool;
    }

    if (os_msg_queue_create(&buffer_db->msg_queue,
                            "notif_q",
                            NOTIFICATION_MSG_MAX_NUM,
                            sizeof(T_NOTIFICATION_QUEUE_MSG)) == false)
    {
        ret = 3;
        goto fail_create_queue;
    }

    sys_mgr_event_register(EVENT_NOTIFICATION_MSG, notification_msg_handler);

    buffer_db->pool_size    = pool_size;
    buffer_db->pool_head    = 0;
    buffer_db->pool_tail    = 0;
    buffer_db->cached_size  = 0;
    buffer_db->state        = NOTIFICATION_BUFFER_STATE_IDLE;
    buffer_db->cback        = NULL;
    buffer_db->owner        = NULL;
    os_queue_init(&buffer_db->queue);

    return true;

fail_create_queue:
    os_mem_free(buffer_db->pool_buf);
fail_alloc_pool:
    os_mem_free(buffer_db);
    buffer_db = NULL;
fail_alloc_db:
    AUDIO_PRINT_ERROR1("notification_buffer_init: failed %d", -ret);
    return false;
}

void notification_buffer_deinit(void)
{
    if (buffer_db != NULL)
    {
        if (buffer_db->pool_buf != NULL)
        {
            os_mem_free(buffer_db->pool_buf);
        }

        os_mem_free(buffer_db);
        buffer_db = NULL;
    }
}

void notification_buffer_reset(void *owner, P_NOTIFICATION_BUFFER_CBACK cback)
{
    os_queue_init(&buffer_db->queue);

    buffer_db->pool_head    = 0;
    buffer_db->pool_tail    = 0;
    buffer_db->cached_size  = 0;
    buffer_db->state        = NOTIFICATION_BUFFER_STATE_IDLE;
    buffer_db->owner        = owner;
    buffer_db->cback        = cback;
}

void notification_buffer_config(uint32_t buffer_size,
                                uint8_t  upper_scaling,
                                uint8_t  low_scaling,
                                uint8_t  prequeue_scaling)
{
    uint32_t size;

    if (buffer_size > buffer_db->pool_size)
    {
        size = buffer_db->pool_size;
    }
    else
    {
        size = buffer_size;
    }

    buffer_db->upper_threshold    = size * upper_scaling / 100;
    buffer_db->low_threshold      = size * low_scaling / 100;
    buffer_db->prequeue_threshold = size * prequeue_scaling / 100;

    AUDIO_PRINT_TRACE4("notification_buffer_config: size 0x%08x, low_threshold 0x%08X, "
                       "upper_threshold 0x%08X, prequeue_threshold 0x%08X",
                       size, buffer_db->low_threshold, buffer_db->upper_threshold, buffer_db->prequeue_threshold);
}

bool notification_buffer_prequeue(T_NOTIFICATION_BUFFER_TYPE type,
                                  uint32_t                   buf,
                                  uint32_t                   offset,
                                  uint32_t                   len)
{
    bool ret = false;

    AUDIO_PRINT_TRACE5("notification_buffer_prequeue: type %u, state %u, buf 0x%08x, offset 0x%08x, len 0x%08x",
                       type, buffer_db->state, buf, offset, len);

    if (type == NOTIFICATION_BUFFER_TYPE_FRAME)
    {
        uint16_t  frame_len;
        uint8_t   frame_hdr[2];
        uint8_t  *frame;

        while (buffer_db->state != NOTIFICATION_BUFFER_STATE_PLAY &&
               buffer_db->state != NOTIFICATION_BUFFER_STATE_HIGH)
        {
            if (fmc_flash_nor_read(buf + offset, frame_hdr, NOTIFICATION_FRAME_LEN_SIZE) == true)
            {
                /* big endian for frame len */
                frame_len = (frame_hdr[0] << 8) | frame_hdr[1];
                if (frame_len == 0xFFFF)
                {
                    if (buffer_db->cback)
                    {
                        buffer_db->cback(buffer_db->owner, NOTIFICATION_BUFFER_EVENT_PREQUEUE, frame_len);
                    }

                    ret = notification_buffer_frame_push((uint32_t)frame_hdr, 0, NOTIFICATION_FRAME_LEN_SIZE);
                    break;
                }
                else
                {
                    frame = os_mem_alloc2(frame_len + NOTIFICATION_FRAME_LEN_SIZE);
                    if (frame != NULL)
                    {
                        if (fmc_flash_nor_read(buf + offset, frame, frame_len + NOTIFICATION_FRAME_LEN_SIZE))
                        {
                            if (buffer_db->cback)
                            {
                                buffer_db->cback(buffer_db->owner,
                                                 NOTIFICATION_BUFFER_EVENT_PREQUEUE,
                                                 frame_len + NOTIFICATION_FRAME_LEN_SIZE + offset);
                            }

                            buf += frame_len + NOTIFICATION_FRAME_LEN_SIZE + offset;
                            len -= frame_len + NOTIFICATION_FRAME_LEN_SIZE + offset;
                            offset = 0;

                            ret = notification_buffer_frame_push((uint32_t)frame, 0, frame_len + NOTIFICATION_FRAME_LEN_SIZE);
                        }

                        os_mem_free(frame);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
    else
    {
        if (buffer_db->state == NOTIFICATION_BUFFER_STATE_IDLE ||
            buffer_db->state == NOTIFICATION_BUFFER_STATE_EMPTY)
        {
            ret = notification_buffer_stream_push(buf, offset, len);
        }
    }

    return ret;
}

bool notification_buffer_push(T_NOTIFICATION_BUFFER_TYPE type,
                              uint32_t                   buf,
                              uint32_t                   offset,
                              uint32_t                   len)
{
    bool ret = false;

    if (type == NOTIFICATION_BUFFER_TYPE_FRAME)
    {
        ret = notification_buffer_frame_push(buf, offset, len);
    }
    else
    {
        ret = notification_buffer_stream_push(buf, offset, len);
    }

    return ret;
}

void *notification_buffer_peek(T_NOTIFICATION_BUFFER_TYPE  type,
                               uint32_t                   *len)
{
    void *buf;

    if (type == NOTIFICATION_BUFFER_TYPE_FRAME)
    {
        buf = notification_buffer_frame_peek(len);
    }
    else
    {
        buf = notification_buffer_stream_peek(len);
    }

    return buf;
}

bool notification_buffer_flush(T_NOTIFICATION_BUFFER_TYPE type)
{
    bool ret = false;

    if (type == NOTIFICATION_BUFFER_TYPE_FRAME)
    {
        ret = notification_buffer_frame_flush();
    }
    else
    {
        ret = notification_buffer_stream_flush();
    }

    return ret;
}
