/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _JITTER_BUFFER_H_
#define _JITTER_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    JITTER_BUFFER_EVT_ASRC_PID_ADJ,
    JITTER_BUFFER_EVT_ASRC_PID_ADJ_CANCEL,
    JITTER_BUFFER_EVT_ASRC_PID_ADJ_OPEN,
    JITTER_BUFFER_EVT_ASRC_PID_ADJ_CLOSE,
    JITTER_BUFFER_EVT_ASRC_LONGTERM_ADJ_REQ,
    JITTER_BUFFER_EVT_ASRC_ADJ_REQ,
    JITTER_BUFFER_EVT_LAT_ADJ_PLC_REQ,
    JITTER_BUFFER_EVT_LOW_LAT_ASRC_RESTORE_REQ,
    JITTER_BUFFER_EVT_LOW_LAT_ASRC_RESTORE_CANCEL,
} T_JITTER_BUFFER_EVT;
typedef struct
{
    int     asrc_ppm;
} T_JITTER_BUFFER_ASRC_ADJ;

typedef struct
{
    uint16_t diff_ms;
    bool accelerate;
} T_JITTER_BUFFER_EVT_LOW_LAT_ADJ_REQ;

typedef struct
{
    uint32_t sync_adj_clk;
    uint32_t restore_clk;
    uint16_t duration;
    uint16_t ratio;
    bool accelerate;
    uint8_t policy;
    uint8_t dec_frame_num;
} T_JITTER_BUFFER_EVT_LOW_LAT_ADJ;

typedef void *T_JITTER_BUFFER_HANDLE;
typedef void (*T_JITTER_BUFFER_EVT_CBACK)(T_JITTER_BUFFER_EVT event, void *param, void *owner);

bool jb_init(void);
void jb_deinit(void);
T_JITTER_BUFFER_HANDLE jitter_buffer_register(void *buffer_ent, T_JITTER_BUFFER_EVT_CBACK cback);
void jitter_buffer_unregister(T_JITTER_BUFFER_HANDLE handle);
void jitter_buffer_asrc_ratio_adjust(uint32_t bt_clk, int ratio_ppm);
void jitter_buffer_buffer_reset(T_JITTER_BUFFER_HANDLE handle);
bool jitter_buffer_asrc_pid_controller(T_JITTER_BUFFER_HANDLE handle, uint32_t latency_report,
                                       uint32_t fifo_report);
bool jitter_buffer_asrc_pid_init(T_JITTER_BUFFER_HANDLE handle);
void jitter_buffer_asrc_pid_reset(T_JITTER_BUFFER_HANDLE handle);
void jitter_buffer_asrc_pid_block(T_JITTER_BUFFER_HANDLE handle);
bool jitter_buffer_asrc_pid_is_block(T_JITTER_BUFFER_HANDLE handle);
void jitter_buffer_asrc_pid_unblock(T_JITTER_BUFFER_HANDLE handle);
void jitter_buffer_low_latency_keeper(T_JITTER_BUFFER_HANDLE handle, uint32_t latency_report_ms);
void jitter_buffer_low_latency_adjust_latency(T_JITTER_BUFFER_HANDLE handle,
                                              T_JITTER_BUFFER_EVT_LOW_LAT_ADJ *params);
void jitter_buffer_low_latency_asrc_restore(T_JITTER_BUFFER_HANDLE handle, uint32_t restore_clk);
int32_t jitter_buffer_compensation_get(T_JITTER_BUFFER_HANDLE handle);
int32_t jitter_buffer_asrc_ppm_get(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _JITTER_BUFFER_H_ */
