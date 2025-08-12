/*
 * Copyright (c) 2020, Realsil Semiconductor Corporation. All rights reserved.
 */
#if (CONFIG_REALTEK_LISTEN_WHILE_SAVE_SUPPORT == 1)
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "media_buffer.h"
#include "listen_save_buffer.h"


T_OS_QUEUE listen_save_queue;
static uint32_t listen_save_buf_total_size = 0;
T_MEDIA_POOL_HANDLE listen_save_pool_handle;

void *listen_save_buffer_peek(int index, uint16_t *len, uint32_t *timestamp)
{
    T_MEDIA_LISTEN_SAVE_HEADER *listen_save_buf;

    listen_save_buf = os_queue_peek(&listen_save_queue, index);
    if (listen_save_buf != NULL)
    {
        if (len != NULL)
        {
            *len = listen_save_buf->len;
        }
    }

    return listen_save_buf;
}

bool listen_save_buffer_write(void *buf, uint16_t len, void *param)
{
    T_MEDIA_LISTEN_SAVE_HEADER *listen_save_buf;
    T_LISTEN_SAVE_SYNC_INFO *p_pkt_header = (T_LISTEN_SAVE_SYNC_INFO *)param;

    listen_save_buf = media_buffer_get(listen_save_pool_handle,
                                       len + sizeof(T_MEDIA_LISTEN_SAVE_HEADER));
    if (listen_save_buf != NULL)
    {
        listen_save_buf->seq       = p_pkt_header->seq_num;
        listen_save_buf->len       = p_pkt_header->len;
        listen_save_buf->frame_num = p_pkt_header->frame_num;
        memcpy(listen_save_buf->data, buf, len);

        listen_save_buf_total_size += len + sizeof(T_MEDIA_LISTEN_SAVE_HEADER);
        APP_PRINT_TRACE4("listen_save_buffer_write seq %d, len %d, frame_num %d, buf_total_size 0x%x",
                         listen_save_buf->seq, listen_save_buf->len, listen_save_buf->frame_num, listen_save_buf_total_size);
        os_queue_in(&listen_save_queue, listen_save_buf);
        return true;
    }

    return false;
}


bool listen_save_buffer_flush(uint16_t cnt, void *param)
{
    T_MEDIA_LISTEN_SAVE_HEADER *listen_save_buf;
    uint16_t               i;

    for (i = 0; i < cnt; i++)
    {
        listen_save_buf = os_queue_out(&listen_save_queue);
        if (listen_save_buf != NULL)
        {
            media_buffer_put(listen_save_pool_handle, listen_save_buf);
            listen_save_buf_total_size -= listen_save_buf->len + sizeof(T_MEDIA_LISTEN_SAVE_HEADER);
        }
        else
        {
            /* Record queue is empty */
            break;
        }
    }

    return true;
}

void listen_save_buffer_reset(void)
{
    listen_save_buffer_flush(listen_save_queue.count, NULL);
    listen_save_buf_total_size = 0;
}
#if 0
void listen_save_buffer_dump(void)
{
    /* TODO */
}

const T_MEDIABUFFER_OP listen_save_buffer_ops =
{
    .buffer_type    = MEDIA_BUFFER_TYPE_RECORD,
    .peek_fun       = listen_save_buffer_peek,
//    .shrink_fun     = record_buffer_shrink,
    .write_fun      = listen_save_buffer_write,
//    .read_fun       = record_buffer_read,
    .flush_fun      = listen_save_buffer_flush,
    .reset_fun      = listen_save_buffer_reset,
    .dump_fun       = listen_save_buffer_dump,
};
#endif

void listen_save_media_buffer_init(void)
{
    listen_save_pool_handle = media_pool_create(1024 * 6); /* TODO lemon */
    if (listen_save_pool_handle == NULL)
    {
        APP_PRINT_ERROR0("listen_save_media_buffer_init FAIL");
    }
    os_queue_init(&listen_save_queue);
    listen_save_buf_total_size = 0;
}
void listen_save_media_buffer_deinit(void)
{
    if (listen_save_pool_handle != NULL)
    {
        media_pool_destory(listen_save_pool_handle);
    }
}
#if 0
bool listen_save_write(uint16_t              seq_num,
                       uint8_t               frame_num,
                       void                 *buf,
                       uint16_t              len,
                       uint16_t             *written_len)
{
    bool res;
    T_LISTEN_SAVE_SYNC_INFO pkt_header;
    pkt_header.frame_num = frame_num;
    pkt_header.len = len;
    pkt_header.seq_num = seq_num;
    res = listen_save_buffer_write(buf, len, &pkt_header);
    return res;
}
#endif

uint16_t listen_save_get_pkt_count(void)
{
    return listen_save_queue.count;
}
#endif
