/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "os_sync.h"
#include "trace.h"
#include "audio_mgr.h"
#include "media_buffer.h"
#include "jitter_buffer.h"
#include "dsp_ipc.h"
#include "stdlib.h"

#define LONGTERM_ASRC_LIMITATION 40
#define AVERAGE_WINDOW 30
#define AVERAGE_CHECK_WINDOW 500

#define NORMAL_ASRC_PID_LIMITATION_PPM 2000
#define NORMAL_PID_ON_LOWER_THRESHOLD_ERROR_US 25000
#define NORMAL_PID_ON_UPPER_THRESHOLD_ERROR_US 25000
#define NORMAL_PID_OFF_LOWER_THRESHOLD_ERROR_US 5000
#define NORMAL_PID_OFF_UPPER_THRESHOLD_ERROR_US 5000
#define NORMAL_PID_KP 0.035
#define NORMAL_PID_KI 0.005
#define NORMAL_PID_KD 0.01
#define NORMAL_TGT_OFFSET 0

#define LOW_LAT_ASRC_PID_LIMITATION_PPM 2000
#define LOW_LAT_PID_ON_LOWER_THRESHOLD_ERROR_US 11000
#define LOW_LAT_PID_ON_UPPER_THRESHOLD_ERROR_US 11000
#define LOW_LAT_PID_OFF_LOWER_THRESHOLD_ERROR_US 5000
#define LOW_LAT_PID_OFF_UPPER_THRESHOLD_ERROR_US 5000
#define LOW_LAT_TGT_OFFSET 0

#define ULTRA_LOW_LAT_ASRC_PID_LIMITATION_PPM 2500
#define ULTRA_LOW_LAT_PID_ON_UPPER_THRESHOLD_ERROR_US1 1200
#define ULTRA_LOW_LAT_PID_ON_LOWER_THRESHOLD_ERROR_US1 500
#define ULTRA_LOW_LAT_PID_OFF_UPPER_THRESHOLD_ERROR_US1 300
#define ULTRA_LOW_LAT_PID_OFF_LOWER_THRESHOLD_ERROR_US1 200
#define ULTRA_LOW_LAT_PID_ON_LOWER_THRESHOLD_ERROR_US2 500
#define ULTRA_LOW_LAT_PID_ON_UPPER_THRESHOLD_ERROR_US2 1200
#define ULTRA_LOW_LAT_PID_OFF_LOWER_THRESHOLD_ERROR_US2 200
#define ULTRA_LOW_LAT_PID_OFF_UPPER_THRESHOLD_ERROR_US2 300
#define ULTRA_LOW_LAT_PID_KP 0.06
#define ULTRA_LOW_LAT_PID_KI 0.01
#define ULTRA_LOW_LAT_PID_KD 0.08
#define ULTRA_LOW_LAT_TGT_OFFSET 5000
#define ULTRA_LOW_LAT_TGT 10000

typedef enum
{
    JB_ASRC_SLOW,
    JB_ASRC_NORMAL,
    JB_ASRC_RAPID,
} T_JB_ASRC_MODE;

typedef enum
{
    JITTER_BUFFER_LAT_KEEPER_ASRC_RESTORE,
} T_JB_TIMER;
typedef struct
{
    T_OS_QUEUE  handles;
    uint8_t id;
    T_JB_ASRC_MODE asrc_mode;
    int32_t        current_asrc_ppm;
} T_JITTER_BUFFER_DB;

typedef struct
{
    float current_asrc_ppm;
    float last_excute_asrc_ppm;
    int   error;
    int   total_error;
    int   error_1;
    float kp, ki, kd;
    float T;
    int   asrc_limitation_ppm;
    int   pid_on_upper_threshold_error_us;
    int   pid_on_lower_threshold_error_us;
    int   pid_off_upper_threshold_error_us;
    int   pid_off_lower_threshold_error_us;
    int   offset;
    bool  is_on;
} T_ASRC_PID;

typedef struct t_jitter_buffer_buffer_db
{
    struct t_jitter_buffer_buffer_db   *p_next;
    T_MEDIA_BUFFER_ENTITY              *buffer_ent;
    T_JITTER_BUFFER_EVT_CBACK           cback;
    float       average_latency;
    uint16_t    average_count;
    bool        block_asrc_adjust;
    uint8_t     id;
    T_ASRC_PID  asrc_pid;
    void       *latency_keeper_asrc_resume_timer_handle;
    uint32_t    latency_keeper_asrc_restore_clk;
    uint16_t    latency_keeper_asrc_cancel_threshold;
    bool        latency_keeper_asrc_accelerate;
    bool        deviation_detected;
} T_JITTER_BUFFER_BUFFER_DB;

T_JITTER_BUFFER_DB *jb_db;

bool jb_init(void)
{
    int ret;

    jb_db = os_mem_zalloc2(sizeof(T_JITTER_BUFFER_DB));
    if (jb_db == NULL)
    {
        ret = 1;
        goto fail_alloc_jb_db;
    }

    jb_db->asrc_mode = JB_ASRC_NORMAL;
    os_queue_init(&jb_db->handles);

    return true;
fail_alloc_jb_db:
    AUDIO_PRINT_TRACE1("jb_init: failed %d", -ret);
    return false;
}

void jb_deinit(void)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db;

    if (jb_db)
    {
        buffer_jb_db = os_queue_out(&jb_db->handles);
        while (buffer_jb_db != NULL)
        {
            if (buffer_jb_db->latency_keeper_asrc_resume_timer_handle != NULL)
            {
                sys_timer_delete(buffer_jb_db->latency_keeper_asrc_resume_timer_handle);
                buffer_jb_db->latency_keeper_asrc_resume_timer_handle = NULL;
            }
            os_mem_free(buffer_jb_db);
            buffer_jb_db = os_queue_out(&jb_db->handles);
        }
        os_mem_free(jb_db);
        jb_db = NULL;
    }
}

T_JITTER_BUFFER_HANDLE jitter_buffer_register(void *buffer_ent, T_JITTER_BUFFER_EVT_CBACK cback)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db;
    buffer_jb_db = os_mem_zalloc2(sizeof(T_JITTER_BUFFER_BUFFER_DB));
    if (buffer_jb_db == NULL)
    {
        return NULL;
    }
    buffer_jb_db->buffer_ent = buffer_ent;
    buffer_jb_db->cback = cback;
    jitter_buffer_asrc_pid_init(buffer_jb_db);
    buffer_jb_db->id = jb_db->id;
    jb_db->id++;

    os_queue_in(&jb_db->handles, buffer_jb_db);

    return buffer_jb_db;
}

void jitter_buffer_unregister(T_JITTER_BUFFER_HANDLE handle)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;
    if (buffer_jb_db)
    {
        if (os_queue_search(&jb_db->handles, handle) == true)
        {
            os_queue_delete(&jb_db->handles, handle);
            os_mem_free(buffer_jb_db);
        }
    }
}

T_JITTER_BUFFER_HANDLE jitter_buffer_find_handle_by_id(uint8_t id)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = NULL;

    buffer_jb_db = os_queue_peek(&jb_db->handles, 0);

    while (buffer_jb_db)
    {
        if (buffer_jb_db->id != id)
        {
            buffer_jb_db = buffer_jb_db->p_next;
        }
        else
        {
            break;
        }
    }
    return buffer_jb_db;
}

bool jitter_bufferr_check_handle(T_JITTER_BUFFER_HANDLE handle)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;

    if (buffer_jb_db)
    {
        if (os_queue_search(&jb_db->handles, buffer_jb_db))
        {
            return true;
        }
    }
    return false;
}


void jitter_buffer_timeout_cback(T_SYS_TIMER_HANDLE handle)
{
    uint32_t timer_id;
    uint16_t event;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db;

    timer_id = sys_timer_id_get(handle);
    BTM_PRINT_TRACE2("jitter_buffer_timeout_cback: timer_id 0x%02X handle %p",
                     timer_id, handle);
    event = timer_id >> 16;

    buffer_jb_db = jitter_buffer_find_handle_by_id(timer_id & 0x0000ffff);
    if (buffer_jb_db == NULL)
    {
        return;
    }

    buffer_ent = buffer_jb_db->buffer_ent;

    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }

    switch (event)
    {
    case JITTER_BUFFER_LAT_KEEPER_ASRC_RESTORE:
        {
            sys_timer_delete(buffer_jb_db->latency_keeper_asrc_resume_timer_handle);
            buffer_jb_db->latency_keeper_asrc_resume_timer_handle = NULL;

            buffer_jb_db->cback(JITTER_BUFFER_EVT_LOW_LAT_ASRC_RESTORE_REQ,
                                &buffer_jb_db->latency_keeper_asrc_restore_clk, buffer_ent);
        }
        break;

    default:
        break;
    }
}

void jitter_buffer_asrc_ratio_adjust(uint32_t bt_clk, int ratio_ppm)
{
    uint32_t param[2] = {0};
    uint32_t jb_asrc_final_offset = 0x8000000 / 1000 * ratio_ppm / 1000;

    jb_db->current_asrc_ppm = ratio_ppm;
    param[0] = jb_asrc_final_offset;
    param[1] = bt_clk;
    AUDIO_PRINT_INFO3("jitter_buffer_asrc_ratio_adjust: clk 0x%x ppm %d jb_asrc_final_offset 0x%x",
                      bt_clk,
                      ratio_ppm, jb_asrc_final_offset);
    dsp_ipc_set_rws_asrc_ratio(param);
}

void jitter_buffer_buffer_reset(T_JITTER_BUFFER_HANDLE handle)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;

    if (jitter_bufferr_check_handle(handle) == false)
    {
        return;
    }

    if (buffer_jb_db->latency_keeper_asrc_resume_timer_handle != NULL)
    {
        sys_timer_delete(buffer_jb_db->latency_keeper_asrc_resume_timer_handle);
        buffer_jb_db->latency_keeper_asrc_resume_timer_handle = NULL;
        buffer_jb_db->latency_keeper_asrc_restore_clk = 0;
    }
}

bool jitter_buffer_asrc_pid_init(T_JITTER_BUFFER_HANDLE handle)
{
    T_AUDIO_STREAM_MODE mode;
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;
    T_ASRC_PID *asrc_pid;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;

    if (jitter_bufferr_check_handle(handle) == false)
    {
        return false;
    }

    buffer_ent = buffer_jb_db->buffer_ent;
    media_buffer_get_mode(buffer_ent, &mode);

    asrc_pid = &buffer_jb_db->asrc_pid;
    AUDIO_PRINT_TRACE0("jitter_buffer_asrc_pid_init");
    asrc_pid->error = 0;
    asrc_pid->error_1 = 0;
    asrc_pid->current_asrc_ppm = 0;
    buffer_jb_db->average_count = 0;
    buffer_jb_db->average_latency = 0;

    if (mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
    {
        asrc_pid->T = LATENCY_REPORT_PERIOD_MS / 1000;
        asrc_pid->asrc_limitation_ppm = ULTRA_LOW_LAT_ASRC_PID_LIMITATION_PPM;
        asrc_pid->kp = ULTRA_LOW_LAT_PID_KP;
        asrc_pid->ki = ULTRA_LOW_LAT_PID_KI;
        asrc_pid->kd = ULTRA_LOW_LAT_PID_KD;
        if (buffer_ent->p_cfg->latency == 0)
        {
            asrc_pid->pid_on_upper_threshold_error_us = ULTRA_LOW_LAT_PID_ON_UPPER_THRESHOLD_ERROR_US1;
            asrc_pid->pid_on_lower_threshold_error_us = ULTRA_LOW_LAT_PID_ON_LOWER_THRESHOLD_ERROR_US1;
            asrc_pid->pid_off_upper_threshold_error_us = ULTRA_LOW_LAT_PID_OFF_UPPER_THRESHOLD_ERROR_US1;
            asrc_pid->pid_off_lower_threshold_error_us = ULTRA_LOW_LAT_PID_OFF_LOWER_THRESHOLD_ERROR_US1;
        }
        else
        {
            asrc_pid->pid_on_upper_threshold_error_us = ULTRA_LOW_LAT_PID_ON_UPPER_THRESHOLD_ERROR_US2;
            asrc_pid->pid_on_lower_threshold_error_us = ULTRA_LOW_LAT_PID_ON_LOWER_THRESHOLD_ERROR_US2;
            asrc_pid->pid_off_upper_threshold_error_us = ULTRA_LOW_LAT_PID_OFF_UPPER_THRESHOLD_ERROR_US2;
            asrc_pid->pid_off_lower_threshold_error_us = ULTRA_LOW_LAT_PID_OFF_LOWER_THRESHOLD_ERROR_US2;
        }
        asrc_pid->offset = ULTRA_LOW_LAT_TGT_OFFSET;
    }
    else if (mode == AUDIO_STREAM_MODE_LOW_LATENCY)
    {
        asrc_pid->T = LATENCY_REPORT_PERIOD_MS / 1000;
        asrc_pid->asrc_limitation_ppm = LOW_LAT_ASRC_PID_LIMITATION_PPM;
        asrc_pid->kp = NORMAL_PID_KP;
        asrc_pid->ki = NORMAL_PID_KI;
        asrc_pid->kd = NORMAL_PID_KD;
        asrc_pid->pid_on_upper_threshold_error_us = LOW_LAT_PID_ON_UPPER_THRESHOLD_ERROR_US;
        asrc_pid->pid_on_lower_threshold_error_us = LOW_LAT_PID_ON_LOWER_THRESHOLD_ERROR_US;
        asrc_pid->pid_off_upper_threshold_error_us = LOW_LAT_PID_OFF_LOWER_THRESHOLD_ERROR_US;
        asrc_pid->pid_off_lower_threshold_error_us = LOW_LAT_PID_OFF_LOWER_THRESHOLD_ERROR_US;
        asrc_pid->offset = LOW_LAT_TGT_OFFSET;
    }
    else
    {
        asrc_pid->T = LATENCY_REPORT_PERIOD_MS / 1000;
        asrc_pid->asrc_limitation_ppm = NORMAL_ASRC_PID_LIMITATION_PPM;
        asrc_pid->kp = NORMAL_PID_KP;
        asrc_pid->ki = NORMAL_PID_KI;
        asrc_pid->kd = NORMAL_PID_KD;
        asrc_pid->pid_on_upper_threshold_error_us = NORMAL_PID_ON_UPPER_THRESHOLD_ERROR_US;
        asrc_pid->pid_on_lower_threshold_error_us = NORMAL_PID_ON_LOWER_THRESHOLD_ERROR_US;
        asrc_pid->pid_off_upper_threshold_error_us = NORMAL_PID_OFF_UPPER_THRESHOLD_ERROR_US;
        asrc_pid->pid_off_lower_threshold_error_us = NORMAL_PID_OFF_LOWER_THRESHOLD_ERROR_US;
        asrc_pid->offset = NORMAL_TGT_OFFSET;
    }

    asrc_pid->is_on = false;
    buffer_jb_db->block_asrc_adjust = false;
    return true;
}

void jitter_buffer_asrc_pid_reset(T_JITTER_BUFFER_HANDLE handle)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;
    T_ASRC_PID *asrc_pid;
    if (jitter_bufferr_check_handle(handle) == false)
    {
        return;
    }

    asrc_pid = &buffer_jb_db->asrc_pid;
    AUDIO_PRINT_TRACE0("jitter_buffer_asrc_pid_reset");
    asrc_pid->is_on = false;
    asrc_pid->error = 0;
    asrc_pid->total_error = 0;
    asrc_pid->error_1 = 0;
    asrc_pid->current_asrc_ppm = 0;
    asrc_pid->last_excute_asrc_ppm = 0;
    buffer_jb_db->average_count = 0;
    buffer_jb_db->average_latency = 0;
    buffer_jb_db->deviation_detected = false;
}

void jitter_buffer_asrc_pid_unblock(T_JITTER_BUFFER_HANDLE handle)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;
    if (jitter_bufferr_check_handle(handle) == false)
    {
        return;
    }
    AUDIO_PRINT_TRACE0("jitter_buffer_asrc_pid_unblock");
    buffer_jb_db->block_asrc_adjust = false;
}

void jitter_buffer_asrc_pid_block(T_JITTER_BUFFER_HANDLE handle)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;

    if (jitter_bufferr_check_handle(handle) == false)
    {
        return;
    }

    AUDIO_PRINT_TRACE0("jitter_buffer_asrc_pid_block");
    if (buffer_jb_db->block_asrc_adjust == false)
    {
        buffer_jb_db->block_asrc_adjust = true;
        if (buffer_jb_db->asrc_pid.is_on)
        {
            T_JITTER_BUFFER_ASRC_ADJ msg;

            msg.asrc_ppm = 0;
            jitter_buffer_asrc_pid_reset(buffer_jb_db);
            buffer_jb_db->cback(JITTER_BUFFER_EVT_ASRC_PID_ADJ_CANCEL, &msg, buffer_jb_db->buffer_ent);
        }
    }
}

bool jitter_buffer_asrc_pid_is_block(T_JITTER_BUFFER_HANDLE handle)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;
    if (jitter_bufferr_check_handle(handle) == false)
    {
        return false;
    }
    return buffer_jb_db->block_asrc_adjust;
}

bool jitter_buffer_asrc_pid_controller(T_JITTER_BUFFER_HANDLE handle, uint32_t latency_report,
                                       uint32_t fifo_report)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_ASRC_PID *asrc_pid;
    T_JITTER_BUFFER_ASRC_ADJ msg;
    float adjust_kp = 0;
    float adjust_ki = 0;
    float curdev = 0;
    int error;
    uint32_t target_latency;
    bool error_in_on_thres = false;
    bool error_in_off_thres = false;
    bool first_adjust = false;
    T_AUDIO_STREAM_MODE mode;


    if (jitter_bufferr_check_handle(handle) == false)
    {
        return false;
    }

    buffer_ent = buffer_jb_db->buffer_ent;
    asrc_pid = &buffer_jb_db->asrc_pid;
    media_buffer_get_mode(buffer_ent, &mode);

    if (mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
    {
        if (buffer_ent->p_cfg->latency == 0)
        {
            target_latency = ULTRA_LOW_LAT_TGT;
            latency_report = latency_report + fifo_report;
        }
        else
        {
            latency_report += fifo_report;
            target_latency = buffer_ent->p_cfg->latency * 1000 + asrc_pid->offset;
        }
    }
    else
    {
        latency_report += fifo_report;
        target_latency = buffer_ent->p_cfg->latency * 1000 + asrc_pid->offset;
    }

    error = latency_report - target_latency;

    if (error >= 0)
    {
        if (error > asrc_pid->pid_on_upper_threshold_error_us)
        {
            error_in_on_thres = false;
        }
        else
        {
            error_in_on_thres = true;
        }
        if (error > asrc_pid->pid_off_upper_threshold_error_us)
        {
            error_in_off_thres = false;
        }
        else
        {
            error_in_off_thres = true;
        }
    }
    else
    {
        if (-error > asrc_pid->pid_on_lower_threshold_error_us)
        {
            error_in_on_thres = false;
        }
        else
        {
            error_in_on_thres = true;
        }
        if (-error > asrc_pid->pid_off_lower_threshold_error_us)
        {
            error_in_off_thres = false;
        }
        else
        {
            error_in_off_thres = true;
        }
    }

    if (asrc_pid->is_on == false)
    {

        if (error_in_on_thres == false)
        {
            if (buffer_jb_db->deviation_detected == true)
            {
                buffer_jb_db->deviation_detected = false;
                jitter_buffer_asrc_pid_reset(handle);
                asrc_pid->is_on = true;
                first_adjust = true;
                buffer_ent->ignore_latency_report = true;
                buffer_jb_db->average_count = 0;
            }
            else
            {
                buffer_jb_db->deviation_detected = true;
                return false;
            }
        }
        else
        {
            buffer_jb_db->average_count++;
            buffer_jb_db->average_latency = buffer_jb_db->average_latency * (AVERAGE_WINDOW - 1) /
                                            AVERAGE_WINDOW +
                                            latency_report / AVERAGE_WINDOW;
            if (buffer_jb_db->average_count > AVERAGE_CHECK_WINDOW)
            {
                int check_error;
                buffer_jb_db->average_count = 0;
                check_error = buffer_jb_db->average_latency - target_latency;
                msg.asrc_ppm = check_error / (LATENCY_REPORT_PERIOD_MS * AVERAGE_CHECK_WINDOW / 1000);
                AUDIO_PRINT_INFO2("jitter_buffer_asrc_pid_func: longterm asrc adjust err %d ppm %d",
                                  check_error, msg.asrc_ppm);
                if (msg.asrc_ppm > LONGTERM_ASRC_LIMITATION)
                {
                    msg.asrc_ppm = LONGTERM_ASRC_LIMITATION;
                }
                else if (msg.asrc_ppm < -LONGTERM_ASRC_LIMITATION)
                {
                    msg.asrc_ppm = -LONGTERM_ASRC_LIMITATION;
                }
                if (buffer_jb_db->block_asrc_adjust == false)
                {
                    buffer_jb_db->cback(JITTER_BUFFER_EVT_ASRC_LONGTERM_ADJ_REQ, &msg, buffer_jb_db->buffer_ent);
                    return true;
                }

            }
            return false;
        }
    }

    asrc_pid->error = latency_report - target_latency;
    asrc_pid->total_error += asrc_pid->error;

    if (asrc_pid->error > 0)
    {
        if (asrc_pid->error > 2 * asrc_pid->pid_off_upper_threshold_error_us)
        {
            adjust_kp = asrc_pid->kp;
            adjust_ki = 0;
            asrc_pid->total_error = 0;
        }
        else
        {
            adjust_kp = asrc_pid->kp;
            adjust_ki = asrc_pid->ki;
        }
    }
    else
    {
        if (-asrc_pid->error > 2 * asrc_pid->pid_off_lower_threshold_error_us)
        {
            adjust_kp = asrc_pid->kp;
            adjust_ki = 0;
            asrc_pid->total_error = 0;
        }
        else
        {
            adjust_kp = asrc_pid->kp;
            adjust_ki = asrc_pid->ki;
        }
    }
    curdev = asrc_pid->kd * (asrc_pid->error - asrc_pid->error_1) / asrc_pid->T;
    asrc_pid->current_asrc_ppm = adjust_kp * asrc_pid->error +
                                 adjust_ki * asrc_pid->T * asrc_pid->total_error + curdev;

    if (abs((int)asrc_pid->current_asrc_ppm) > asrc_pid->asrc_limitation_ppm)
    {
        if (asrc_pid->current_asrc_ppm > 0)
        {
            asrc_pid->current_asrc_ppm = asrc_pid->asrc_limitation_ppm;
        }
        else
        {
            asrc_pid->current_asrc_ppm = -asrc_pid->asrc_limitation_ppm;
        }
    }

    AUDIO_PRINT_INFO4("jitter_buffer_asrc_pid_controller: ppm %d error %d latency_report %d tgt latency %d",
                      (int)(asrc_pid->current_asrc_ppm),
                      (int)(asrc_pid->error), latency_report, target_latency);

    /*
        AUDIO_PRINT_TRACE5("jitter_buffer_asrc_pid_func: error %d, error1 %d, total %d, error-error_1 %d, current_asrc_ppm %d, %d",
                            (int)(asrc_pid->error * 1000),
                            (int)(asrc_pid->error_1 * 1000),
                            (int)(asrc_pid->total_error * 1000),
                            (int)((asrc_pid->error-asrc_pid->error_1) * 1000),
                            asrc_pid->current_asrc_ppm);
    */

    asrc_pid->error_1 = asrc_pid->error;

    if ((int)asrc_pid->current_asrc_ppm == (int)asrc_pid->last_excute_asrc_ppm)
    {
        AUDIO_PRINT_TRACE0("jitter_buffer_asrc_pid_func: same ppm");
        return false;
    }
    //return 0x8000000 / 1000000 * asrc_pid->current_asrc_ppm;
    if (error_in_off_thres)
    {
        asrc_pid->is_on = false;
        jitter_buffer_asrc_pid_reset(buffer_jb_db);
        msg.asrc_ppm = 0;
        buffer_jb_db->cback(JITTER_BUFFER_EVT_ASRC_PID_ADJ_CLOSE, &msg, buffer_jb_db->buffer_ent);
        return false;
    }
    else
    {
        msg.asrc_ppm = asrc_pid->current_asrc_ppm;
        if (first_adjust)
        {
            buffer_jb_db->cback(JITTER_BUFFER_EVT_ASRC_PID_ADJ_OPEN, &msg, buffer_jb_db->buffer_ent);
        }
        else
        {
            buffer_jb_db->cback(JITTER_BUFFER_EVT_ASRC_PID_ADJ, &msg, buffer_jb_db->buffer_ent);
        }
        return true;
    }
}

void jitter_buffer_low_latency_asrc_restore(T_JITTER_BUFFER_HANDLE handle, uint32_t restore_clk)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;

    if (jitter_bufferr_check_handle(handle) == false)
    {
        return;
    }

    if (buffer_jb_db->latency_keeper_asrc_resume_timer_handle)
    {
        sys_timer_delete(buffer_jb_db->latency_keeper_asrc_resume_timer_handle);
        buffer_jb_db->latency_keeper_asrc_resume_timer_handle = NULL;
    }

    jitter_buffer_asrc_ratio_adjust(restore_clk, 0);
    buffer_jb_db->latency_keeper_asrc_restore_clk = 0xffffffff;
}

void jitter_buffer_low_latency_adjust_latency(T_JITTER_BUFFER_HANDLE handle,
                                              T_JITTER_BUFFER_EVT_LOW_LAT_ADJ *params)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;

    if (jitter_bufferr_check_handle(handle) == false)
    {
        return;
    }

    if (params->policy == LATENCY_KEEPER_POLICY_BY_PLC ||
        params->policy == LATENCY_KEEPER_POLICY_BY_BOTH)
    {
        if (params->dec_frame_num != 0)
        {
            buffer_jb_db->cback(JITTER_BUFFER_EVT_LAT_ADJ_PLC_REQ, &params->dec_frame_num,
                                buffer_jb_db->buffer_ent);
        }
    }
    if (params->policy == LATENCY_KEEPER_POLICY_BY_ASRC ||
        params->policy == LATENCY_KEEPER_POLICY_BY_BOTH)
    {
        buffer_jb_db->latency_keeper_asrc_accelerate = params->accelerate;
        if (params->sync_adj_clk == 0xffffffff)
        {

            if (params->accelerate)
            {
                jitter_buffer_asrc_ratio_adjust(0xffffffff, params->ratio);
            }
            else
            {
                jitter_buffer_asrc_ratio_adjust(0xffffffff, -params->ratio);
            }
            if (buffer_jb_db->latency_keeper_asrc_resume_timer_handle != NULL)
            {
                sys_timer_delete(buffer_jb_db->latency_keeper_asrc_resume_timer_handle);
            }

            buffer_jb_db->latency_keeper_asrc_resume_timer_handle =
                sys_timer_create("gaming_mode_asrc_restore",
                                 SYS_TIMER_TYPE_LOW_PRECISION,
                                 (JITTER_BUFFER_LAT_KEEPER_ASRC_RESTORE << 16) | buffer_jb_db->id,
                                 params->duration * 1000 - 100000,
                                 false,
                                 jitter_buffer_timeout_cback);
            if (buffer_jb_db->latency_keeper_asrc_resume_timer_handle != NULL)
            {
                sys_timer_start(buffer_jb_db->latency_keeper_asrc_resume_timer_handle);
            }
            buffer_jb_db->latency_keeper_asrc_restore_clk = 0xffffffff;
        }
        else
        {
            if (params->accelerate)
            {
                jitter_buffer_asrc_ratio_adjust(params->sync_adj_clk, params->ratio);
            }
            else
            {
                jitter_buffer_asrc_ratio_adjust(params->sync_adj_clk, -params->ratio);
            }
            if (buffer_jb_db->latency_keeper_asrc_resume_timer_handle != NULL)
            {
                sys_timer_delete(buffer_jb_db->latency_keeper_asrc_resume_timer_handle);
            }

            buffer_jb_db->latency_keeper_asrc_resume_timer_handle = sys_timer_create("gaming_mode_asrc_restore",
                                                                                     SYS_TIMER_TYPE_LOW_PRECISION,
                                                                                     (JITTER_BUFFER_LAT_KEEPER_ASRC_RESTORE << 16) | 0,
                                                                                     params->duration * 1000 - 100000,
                                                                                     false,
                                                                                     jitter_buffer_timeout_cback);
            if (buffer_jb_db->latency_keeper_asrc_resume_timer_handle != NULL)
            {
                sys_timer_start(buffer_jb_db->latency_keeper_asrc_resume_timer_handle);
            }
            buffer_jb_db->latency_keeper_asrc_restore_clk = params->restore_clk;
        }
    }
}

void jitter_buffer_low_latency_keeper(T_JITTER_BUFFER_HANDLE handle, uint32_t latency_report_us)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;
    T_JITTER_BUFFER_EVT_LOW_LAT_ADJ_REQ msg;
    uint16_t latency_report_ms = latency_report_us / 1000;
    uint16_t target_latency = 0;

    if (jitter_bufferr_check_handle(handle) == false)
    {
        return;
    }

    buffer_ent = buffer_jb_db->buffer_ent;

    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }


    if (buffer_ent->p_cfg->latency != 0)
    {
        target_latency = buffer_ent->p_cfg->latency;
    }
    else
    {
        target_latency = 7;
    }

    if (buffer_jb_db->latency_keeper_asrc_resume_timer_handle)
    {
        bool cancel_asrc = false;
        if (buffer_jb_db->latency_keeper_asrc_accelerate)
        {
            if (latency_report_ms < buffer_jb_db->latency_keeper_asrc_cancel_threshold)
            {
                cancel_asrc = true;
            }
        }
        else
        {
            if (latency_report_ms > buffer_jb_db->latency_keeper_asrc_cancel_threshold)
            {
                cancel_asrc = true;
            }
        }
        if (cancel_asrc)
        {
            buffer_jb_db->cback(JITTER_BUFFER_EVT_LOW_LAT_ASRC_RESTORE_CANCEL, &msg, buffer_jb_db->buffer_ent);
            AUDIO_PRINT_WARN2("AUDIO_MSG_STREAM_LATENCY_REPORT: latency reach cancel threshold %u in advance, target %u",
                              buffer_jb_db->latency_keeper_asrc_cancel_threshold, buffer_ent->p_cfg->latency);

            sys_timer_delete(buffer_jb_db->latency_keeper_asrc_resume_timer_handle);
            buffer_jb_db->latency_keeper_asrc_resume_timer_handle = NULL;
        }
        return;
    }
    else
    {
        if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
        {
            if (abs(latency_report_ms - target_latency) <= 5)
            {
                return;
            }
        }
        else
        {
            if (abs(latency_report_ms - target_latency) <= 10)
            {
                return;
            }
        }
    }

    uint16_t diff_time_ms = 0;
    bool accelerate = false;

    if (latency_report_ms > target_latency)
    {
        accelerate = true;
        diff_time_ms = latency_report_ms - target_latency;
    }
    else
    {
        accelerate = false;
        diff_time_ms = target_latency - latency_report_ms;
    }

    if (diff_time_ms)
    {
        if (accelerate)
        {
            buffer_jb_db->latency_keeper_asrc_cancel_threshold = target_latency +
                                                                 LOW_LATENCY_ASRC_PROTECTION_THRESHOLD / 2;
        }
        else
        {
            buffer_jb_db->latency_keeper_asrc_cancel_threshold = target_latency -
                                                                 LOW_LATENCY_ASRC_PROTECTION_THRESHOLD / 2;
        }
        msg.diff_ms = diff_time_ms;
        msg.accelerate = accelerate;
        buffer_jb_db->cback(JITTER_BUFFER_EVT_ASRC_ADJ_REQ, &msg, buffer_jb_db->buffer_ent);
    }
}

int32_t jitter_buffer_compensation_get(T_JITTER_BUFFER_HANDLE handle)
{
    T_JITTER_BUFFER_BUFFER_DB *buffer_jb_db = handle;

    if (buffer_jb_db)
    {
        return buffer_jb_db->asrc_pid.offset;
    }

    return 0;
}

int32_t jitter_buffer_asrc_ppm_get(void)
{
    return jb_db->current_asrc_ppm;
}

