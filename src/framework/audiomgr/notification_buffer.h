/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _NOTIFICATION_BUFFER_H_
#define _NOTIFICATION_BUFFER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_notification_buffer_type
{
    NOTIFICATION_BUFFER_TYPE_FRAME      = 0x00, /* buffer in frame type */
    NOTIFICATION_BUFFER_TYPE_STREAM     = 0x01, /* buffer in stream type */
} T_NOTIFICATION_BUFFER_TYPE;

typedef enum t_notification_buffer_event
{
    NOTIFICATION_BUFFER_EVENT_PREQUEUE      = 0x00, /* buffer under prequeue */
    NOTIFICATION_BUFFER_EVENT_PLAY          = 0x01, /* buffer prequeue completed and ready to play */
    NOTIFICATION_BUFFER_EVENT_LOW           = 0x02, /* buffer watermark under the low threshold */
    NOTIFICATION_BUFFER_EVENT_HIGH          = 0x03, /* buffer watermark above the upper threshold */
    NOTIFICATION_BUFFER_EVENT_F_PLAY        = 0x04, /* buffer watermark under the upper threshold */
    NOTIFICATION_BUFFER_EVENT_EMPTY         = 0x05, /* buffer empty */
    NOTIFICATION_BUFFER_EVENT_E_PLAY        = 0x06, /* buffer watermark above the low threshold */
    NOTIFICATION_BUFFER_EVENT_FULL          = 0x07, /* buffer full */
} T_NOTIFICATION_BUFFER_EVENT;

typedef bool (*P_NOTIFICATION_BUFFER_CBACK)(void                        *owner,
                                            T_NOTIFICATION_BUFFER_EVENT  event,
                                            uint32_t                     param);

bool notification_buffer_init(uint16_t pool_size);

void notification_buffer_deinit(void);

void notification_buffer_reset(void                        *owner,
                               P_NOTIFICATION_BUFFER_CBACK  cback);

void notification_buffer_config(uint32_t buffer_size,
                                uint8_t  upper_scaling,
                                uint8_t  low_scaling,
                                uint8_t  prequeue_scaling);

bool notification_buffer_prequeue(T_NOTIFICATION_BUFFER_TYPE type,
                                  uint32_t                   buf,
                                  uint32_t                   offset,
                                  uint32_t                   len);

bool notification_buffer_push(T_NOTIFICATION_BUFFER_TYPE type,
                              uint32_t                   buf,
                              uint32_t                   offset,
                              uint32_t                   len);

void *notification_buffer_peek(T_NOTIFICATION_BUFFER_TYPE  type,
                               uint32_t                   *len);

bool notification_buffer_flush(T_NOTIFICATION_BUFFER_TYPE type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NOTIFICATION_BUFFER_H_ */
