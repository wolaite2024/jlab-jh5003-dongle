#ifndef _APP_USB_AUDIO_BUF_
#define _APP_USB_AUDIO_BUF_

#include <stdbool.h>
#include <stdint.h>


typedef struct
{
    uint16_t idx_r;
    uint16_t idx_w;
    uint16_t total_len;
//    uint16_t data_len;

    uint8_t  *buf;
} RINGBUF_T;

void ringbuf_init(RINGBUF_T *p_ringbuf, uint16_t buf_len);
void ringbuf_deinit(RINGBUF_T *p_ringbuf);
#if 0
bool is_ringbuf_empty(RINGBUF_T *p_ringbuf);
bool is_ringbuf_full(RINGBUF_T *p_ringbuf);
#endif
uint16_t ringbuf_freespace(RINGBUF_T *p_ringbuf);
uint16_t ringbuf_dataspace(RINGBUF_T *p_ringbuf);
bool ringbuf_read(RINGBUF_T *p_ringbuf, uint8_t *p_buf_r, uint16_t len);
bool ringbuf_write(RINGBUF_T *p_ringbuf, uint8_t *p_buf_w, uint16_t len);

#endif
