#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "app_usb_audio_buf.h"
#include "section.h"

void ringbuf_init(RINGBUF_T *p_ringbuf, uint16_t buf_len)
{
    if (p_ringbuf != NULL)
    {
        p_ringbuf->idx_r = 0;
        p_ringbuf->idx_w = 0;
        p_ringbuf->total_len = buf_len;
//        p_ringbuf->data_len = 0;
        p_ringbuf->buf = os_mem_zalloc(RAM_TYPE_DATA_ON, p_ringbuf->total_len);
    }
}

void ringbuf_deinit(RINGBUF_T *p_ringbuf)
{

    if (p_ringbuf != NULL && p_ringbuf->buf != NULL)
    {
        p_ringbuf->idx_r = 0;
        p_ringbuf->idx_w = 0;
        p_ringbuf->total_len = 0;
//        p_ringbuf->data_len = 0;

        os_mem_free(p_ringbuf->buf);
        p_ringbuf->buf = NULL;
    }
}
#if 0
bool is_ringbuf_empty(RINGBUF_T *p_ringbuf)
{
    if (p_ringbuf == NULL)
    {
        return false;
    }

    return (p_ringbuf->data_len == 0);
}

bool is_ringbuf_full(RINGBUF_T *p_ringbuf)
{
    if (p_ringbuf == NULL)
    {
        return false;
    }

    return (p_ringbuf->data_len == p_ringbuf->total_len);
}
#endif

RAM_TEXT_SECTION
uint16_t ringbuf_freespace(RINGBUF_T *p_ringbuf)
{
    if (p_ringbuf == NULL || p_ringbuf->buf == NULL)
    {
        return false;
    }
    return ((p_ringbuf->idx_r + p_ringbuf->total_len - p_ringbuf->idx_w - 1) % p_ringbuf->total_len);
}

RAM_TEXT_SECTION
uint16_t ringbuf_dataspace(RINGBUF_T *p_ringbuf)
{
    if (p_ringbuf == NULL || p_ringbuf->buf == NULL)
    {
        return false;
    }
    return ((p_ringbuf->idx_w + p_ringbuf->total_len - p_ringbuf->idx_r) % p_ringbuf->total_len);
}

RAM_TEXT_SECTION
bool ringbuf_read(RINGBUF_T *p_ringbuf, uint8_t *buf_r, uint16_t len)
{
    if (p_ringbuf == NULL || p_ringbuf->buf == NULL)
    {
        return false;
    }
#if 0
    if (len > p_ringbuf->data_len)
    {
        APP_PRINT_ERROR0("JIM:read: No enough data to read");
        return false;
    }

    p_ringbuf->data_len -= len;
#else
    if (len > ringbuf_dataspace(p_ringbuf))
    {
        //APP_PRINT_ERROR0("JIM:read: No enough data to read");
        return false;
    }
#endif
    if (p_ringbuf->idx_r + len <= p_ringbuf->total_len)
    {
        memcpy(buf_r, p_ringbuf->buf + p_ringbuf->idx_r, len);
        p_ringbuf->idx_r += len;
    }
    else
    {
        memcpy(buf_r, p_ringbuf->buf + p_ringbuf->idx_r, \
               p_ringbuf->total_len - p_ringbuf->idx_r);
        memcpy(buf_r + p_ringbuf->total_len - p_ringbuf->idx_r, \
               p_ringbuf->buf, len - (p_ringbuf->total_len - p_ringbuf->idx_r));
        p_ringbuf->idx_r = len - (p_ringbuf->total_len - p_ringbuf->idx_r);
    }
    p_ringbuf->idx_r %= p_ringbuf->total_len;

    return true;

}

RAM_TEXT_SECTION
bool ringbuf_write(RINGBUF_T *p_ringbuf, uint8_t *p_buf_w, uint16_t len)
{
    if (p_ringbuf == NULL || p_ringbuf->buf == NULL)
    {
        return false;
    }
#if 0
    if (p_ringbuf->data_len + len > p_ringbuf->total_len)
    {
        APP_PRINT_ERROR0("JIM:write: No enough space to write");
        return false;
    }
    p_ringbuf->data_len += len;
#else
    if (ringbuf_freespace(p_ringbuf) < len)
    {
        //APP_PRINT_ERROR0("JIM:write: No enough space to write");
        return false;
    }
#endif

    if (p_ringbuf->idx_w + len <= p_ringbuf->total_len)
    {
        memcpy(p_ringbuf->buf + p_ringbuf->idx_w, p_buf_w, len);
        p_ringbuf->idx_w += len;
    }
    else
    {
        memcpy(p_ringbuf->buf + p_ringbuf->idx_w, p_buf_w, \
               p_ringbuf->total_len - p_ringbuf->idx_w);
        memcpy(p_ringbuf->buf, p_buf_w + p_ringbuf->total_len - p_ringbuf->idx_w, \
               len - (p_ringbuf->total_len - p_ringbuf->idx_w));
        p_ringbuf->idx_w = len - (p_ringbuf->total_len - p_ringbuf->idx_w);
    }
    p_ringbuf->idx_w %= p_ringbuf->total_len;

    return true;
}

