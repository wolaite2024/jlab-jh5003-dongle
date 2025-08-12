/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_STATUS_REPORT_H_
#define _APP_STATUS_REPORT_H_

#if APP_DEBUG_REPORT
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint16_t    codec_up_frame_len;
    uint16_t    codec_down_frame_len;
    uint16_t    pcm_up_frame_len;
    uint16_t    pcm_down_frame_len;

    uint16_t    pcm_up_ringbuffer_bytes;
    uint16_t    pcm_down_ringbuffer_bytes;
    uint16_t    codec_up_ringbuffer_bytes;
    uint16_t    codec_down_ringbuffer_bytes;

    uint32_t    codec_up_drop_bytes;
    uint32_t    codec_down_drop_bytes;
    uint32_t    pcm_down_drop_bytes;

    uint16_t    codec_flight_in_dsp_bytes;
    uint16_t    pcm_flight_in_dsp_bytes;

    uint16_t    first_decode_pkt_seq;
    uint16_t    last_decode_pkt_seq;
    uint32_t    total_decode_pkts;
    uint32_t    decode_fill_num;
    uint32_t    decode_drain_num;
    uint32_t    decode_drain_lost;
} T_APP_STATUS_INFO;

extern T_APP_STATUS_INFO app_status_report;

void app_status_report_print(void);
void app_status_report_init(void);

#endif
#endif /* _APP_STATUS_REPORT_H_ */
