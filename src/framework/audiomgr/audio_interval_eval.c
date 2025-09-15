/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "os_mem.h"
#include "trace.h"
#include "audio_interval_eval.h"
#include "sys_timer.h"
#include "sys_ipc.h"
#include "audio_mgr.h"

#define STD_WINDOW 16
#define Z_SOCRE_LEVEL 5
#define INTERVAL_EVAL_DEBUG 0
#define SENS_LEVEL_MAX 4
#define VAR_CHECK_TIME1 60000000UL
#define VAR_CHECK_TIME2 3000000UL

typedef struct
{
    T_OS_QUEUE  handles;
    uint8_t id;
} T_AUDIO_INTERVAL_EVAL_DB;

typedef struct t_interval_queue
{
    float *intervals;
    float  avg;
    float  val;
    uint8_t  num;
    uint8_t  idx;
} T_INTERVAL_QUEUE;

typedef struct t_audio_interval_buffer_db
{
    struct t_audio_interval_buffer_db *p_next;
    T_MEDIA_BUFFER_ENTITY           *buffer_ent;
    T_AUDIO_INTERVAL_EVAL_EVT_CBACK cback;
    void *stable_dejitter_timer_handle;
    void *interval_variance_check_timer_handle;
    float avg_long_term;
    float val_long_term;
    uint32_t last_clk;
    uint32_t count;
    uint32_t var_check_time;
    float z_thres;
    bool eval_ready;
    bool interval_unstable;
    uint8_t sensitivity_level;
    uint8_t id;
    bool stable_dejitter_timer_start;
    bool var_check_timer_start;
    T_INTERVAL_QUEUE interval_queue;
} T_AUDIO_INTERVAL_BUFFER_DB;

static const float z_sen_sens_level_table[SENS_LEVEL_MAX] =
{
    /* SEN_LEVEL1: 0;   SEN_LEVEL2: 1;      SEN_LEVEL3: 2;      SEN_LEVEL4: 3. */
    2.7f,               2.5f,               2.45f,              2.4f,
};

static const uint32_t dijitter_timer_sens_level_table[SENS_LEVEL_MAX] =
{
    /* SEN_LEVEL1: 0;   SEN_LEVEL2: 1;      SEN_LEVEL3: 2.     SEN_LEVEL4: 3. */
    3000000,            5000000,            7000000,           9000000,
};

T_AUDIO_INTERVAL_EVAL_DB *audio_interval_eval_db;

bool audio_interval_check_handle(T_AUDIO_INTERVAL_EVAL_HANDLE handle)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;
    if (buffer_eval_db)
    {
        if (os_queue_search(&audio_interval_eval_db->handles, buffer_eval_db))
        {
            return true;
        }
    }
    return false;
}

static int float2Hint(float x)
{
    return (int)(x * 100);
}

static long *float2long(void *x)
{
    return (long *)x;
}

static float *long2float(void *x)
{
    return (float *)x;
}

static float invSqrt(float x)
{
    float xhalf = 0.5f * x;
    long i = *float2long(&x);//get bits for floating value

    i = 0x5f3759df - (i >> 1);//gives initial guess
    x = *long2float(&i); //convert bits back to float
    x = x * (1.5f - xhalf * x * x);//Newton step
    return x;
}

static void mean_window_update(T_INTERVAL_QUEUE *interval_queue)
{
    float mean = 0;
    float c = STD_WINDOW;
    uint8_t i;
    if (interval_queue->num == STD_WINDOW)
    {
        for (i = 0; i < STD_WINDOW; i++)
        {
            mean = mean + interval_queue->intervals[i];
        }
        mean = mean / c;
        interval_queue->avg = mean;
    }
}

static void std_window_update(T_INTERVAL_QUEUE *interval_queue)
{
    uint8_t i;
    float temp = 0;
    float c = STD_WINDOW - 1;

    if (interval_queue->num == STD_WINDOW)
    {
        for (i = 0; i < STD_WINDOW; i++)
        {
            temp += (interval_queue->intervals[i] - interval_queue->avg) * ((interval_queue->intervals)[i] -
                                                                            interval_queue->avg);
        }

        interval_queue->val = temp / c;
    }
}

static float z_score(T_INTERVAL_QUEUE *interval_queue, float x)
{
    return (x - interval_queue->avg) * invSqrt(interval_queue->val);
}
/*
static float z_score2(float x)
{
    return (x - buffer_eval_db->avg_long_term) * invSqrt(interval_queue.val);
}
*/
static void param_long_term_update(T_AUDIO_INTERVAL_EVAL_HANDLE handle, float x)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;

    if (audio_interval_check_handle(handle) == false)
    {
        return;
    }

    uint32_t n = buffer_eval_db->count;
    if (n == 1)
    {
        buffer_eval_db->val_long_term = 0;
        buffer_eval_db->avg_long_term = x;
    }
    else
    {
        buffer_eval_db->val_long_term = (n - 2) * buffer_eval_db->val_long_term / (n - 1) +
                                        (x - buffer_eval_db->avg_long_term) * (x - buffer_eval_db->avg_long_term) / n;
        buffer_eval_db->avg_long_term = buffer_eval_db->avg_long_term +
                                        (x - buffer_eval_db->avg_long_term) / buffer_eval_db->count;
    }

}

static void interval_queue_update(T_INTERVAL_QUEUE *interval_queue, float x)
{

    (interval_queue->intervals)[interval_queue->idx] = x;

    if (interval_queue->num < STD_WINDOW)
    {
        interval_queue->num += 1;
    }
    interval_queue->idx = (interval_queue->idx + 1) & (STD_WINDOW - 1); //(idx+1)%STD_WINDOW

    mean_window_update(interval_queue);
    std_window_update(interval_queue);
}

T_AUDIO_INTERVAL_EVAL_HANDLE audio_eval_find_handle_by_id(uint8_t id)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = NULL;

    buffer_eval_db = os_queue_peek(&audio_interval_eval_db->handles, 0);

    while (buffer_eval_db)
    {
        if (buffer_eval_db->id != id)
        {
            buffer_eval_db = buffer_eval_db->p_next;
        }
        else
        {
            break;
        }
    }
    return buffer_eval_db;
}

void variance_check_timer_cback(T_SYS_TIMER_HANDLE handle)
{
    uint32_t timer_id;
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;

    timer_id = sys_timer_id_get(handle);
    AUDIO_PRINT_TRACE2("Start check var: timer_id 0x%02X handle %p",
                       timer_id, handle);

    buffer_eval_db = audio_eval_find_handle_by_id(timer_id);
    if (buffer_eval_db == NULL)
    {
        return;
    }

    buffer_eval_db->var_check_timer_start = false;
}

void stable_dejitter_timer_cback(T_SYS_TIMER_HANDLE handle)
{
    uint32_t timer_id;
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;

    timer_id = sys_timer_id_get(handle);
    AUDIO_PRINT_TRACE2("stable_dejitter_timer_cback: timer_id 0x%02X handle %p",
                       timer_id, handle);

    buffer_eval_db = audio_eval_find_handle_by_id(timer_id);
    if (buffer_eval_db == NULL)
    {
        return;
    }

    buffer_eval_db->stable_dejitter_timer_start = false;
    if (buffer_eval_db->eval_ready)
    {
        buffer_eval_db->interval_unstable = false;
        buffer_eval_db->cback(AUDIO_INTERVAL_EVAL_EVT_STABLE, NULL, buffer_eval_db->buffer_ent);
    }
}

bool audio_interval_eval_init(void)
{
    int ret;

    audio_interval_eval_db = os_mem_zalloc2(sizeof(T_AUDIO_INTERVAL_EVAL_DB));
    if (audio_interval_eval_db == NULL)
    {
        ret = -1;
        goto fail_alloc_audio_latency_db;
    }

    os_queue_init(&audio_interval_eval_db->handles);

    return true;
fail_alloc_audio_latency_db:
    AUDIO_PRINT_TRACE1("audio_latency_mgr_init: failed %d", ret);
    return false;
}

void audio_interval_eval_deinit(void)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db;

    if (audio_interval_eval_db)
    {
        buffer_eval_db = os_queue_out(&audio_interval_eval_db->handles);
        while (buffer_eval_db != NULL)
        {
            if (buffer_eval_db->stable_dejitter_timer_handle)
            {
                sys_timer_delete(buffer_eval_db->stable_dejitter_timer_handle);
            }
            if (buffer_eval_db->interval_variance_check_timer_handle)
            {
                sys_timer_delete(buffer_eval_db->interval_variance_check_timer_handle);
            }
            os_mem_free(buffer_eval_db->interval_queue.intervals);
            os_mem_free(buffer_eval_db);
            buffer_eval_db = os_queue_out(&audio_interval_eval_db->handles);
        }
        os_mem_free(audio_interval_eval_db);
        audio_interval_eval_db = NULL;
    }
}

T_AUDIO_INTERVAL_EVAL_HANDLE audio_interval_evaluator_register(void *buffer_ent,
                                                               T_AUDIO_INTERVAL_EVAL_EVT_CBACK cback)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db;

    buffer_eval_db = os_mem_zalloc2(sizeof(T_AUDIO_INTERVAL_BUFFER_DB));
    if (buffer_eval_db == NULL)
    {
        return NULL;
    }
    buffer_eval_db->interval_queue.intervals = os_mem_zalloc2(sizeof(float) * STD_WINDOW);
    if (buffer_eval_db->interval_queue.intervals == NULL)
    {
        return NULL;
    }
    buffer_eval_db->buffer_ent = buffer_ent;
    buffer_eval_db->cback = cback;
    buffer_eval_db->id = audio_interval_eval_db->id;
    audio_interval_eval_db->id++;
    buffer_eval_db->sensitivity_level = 0;
    buffer_eval_db->var_check_time = VAR_CHECK_TIME2;
    buffer_eval_db->z_thres = 2.7;

    buffer_eval_db->stable_dejitter_timer_handle = sys_timer_create("stable_dejitter",
                                                                    SYS_TIMER_TYPE_LOW_PRECISION,
                                                                    buffer_eval_db->id,
                                                                    3000000,
                                                                    false,
                                                                    stable_dejitter_timer_cback);

    buffer_eval_db->interval_variance_check_timer_handle = sys_timer_create("variance_check",
                                                                            SYS_TIMER_TYPE_LOW_PRECISION,
                                                                            buffer_eval_db->id,
                                                                            buffer_eval_db->var_check_time,
                                                                            false,
                                                                            variance_check_timer_cback);

    os_queue_in(&audio_interval_eval_db->handles, buffer_eval_db);

    return buffer_eval_db;
}

void audio_interval_evaluator_unregister(T_AUDIO_INTERVAL_EVAL_HANDLE handle)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;
    if (buffer_eval_db)
    {
        if (os_queue_search(&audio_interval_eval_db->handles, buffer_eval_db) == true)
        {
            os_queue_delete(&audio_interval_eval_db->handles, buffer_eval_db);
            if (buffer_eval_db->stable_dejitter_timer_handle)
            {
                sys_timer_delete(buffer_eval_db->stable_dejitter_timer_handle);
            }
            if (buffer_eval_db->interval_variance_check_timer_handle)
            {
                sys_timer_delete(buffer_eval_db->interval_variance_check_timer_handle);
            }
            os_mem_free(buffer_eval_db->interval_queue.intervals);
            os_mem_free(buffer_eval_db);
        }
    }
}

void audio_interval_evaluator_reset(T_AUDIO_INTERVAL_EVAL_HANDLE handle)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;
    T_INTERVAL_QUEUE *interval_queue;

    if (audio_interval_check_handle(handle) == false)
    {
        return;
    }

    interval_queue  = &buffer_eval_db->interval_queue;
    memset(interval_queue->intervals, 0, sizeof(uint32_t)*STD_WINDOW);
    interval_queue->num = 0;
    interval_queue->idx = 0;
    interval_queue->avg = 0;
    interval_queue->val = 0;
    sys_timer_stop(buffer_eval_db->stable_dejitter_timer_handle);
    sys_timer_stop(buffer_eval_db->interval_variance_check_timer_handle);
    buffer_eval_db->avg_long_term = 0;
    buffer_eval_db->val_long_term = 0;;
    buffer_eval_db->last_clk = 0;
    buffer_eval_db->count = 0;
    buffer_eval_db->eval_ready = false;
    buffer_eval_db->interval_unstable = false;
    buffer_eval_db->stable_dejitter_timer_start = false;
    buffer_eval_db->var_check_timer_start = false;
}

bool audio_interval_evaluator_change_sensitivity(T_AUDIO_INTERVAL_EVAL_HANDLE handle,
                                                 uint8_t sens_level)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;
    if (audio_interval_check_handle(handle) == false)
    {
        return false;
    }

    audio_interval_evaluator_reset(handle);

    buffer_eval_db->sensitivity_level = sens_level;
    buffer_eval_db->var_check_time = VAR_CHECK_TIME1;
    buffer_eval_db->z_thres = z_sen_sens_level_table[sens_level];

    if (buffer_eval_db->stable_dejitter_timer_handle)
    {
        sys_timer_delete(buffer_eval_db->stable_dejitter_timer_handle);
        buffer_eval_db->stable_dejitter_timer_handle = sys_timer_create("stable_dejitter",
                                                                        SYS_TIMER_TYPE_LOW_PRECISION,
                                                                        buffer_eval_db->id,
                                                                        dijitter_timer_sens_level_table[sens_level],
                                                                        false,
                                                                        stable_dejitter_timer_cback);
    }
    if (buffer_eval_db->interval_variance_check_timer_handle)
    {
        sys_timer_delete(buffer_eval_db->interval_variance_check_timer_handle);
        buffer_eval_db->interval_variance_check_timer_handle = sys_timer_create("variance_check",
                                                                                SYS_TIMER_TYPE_LOW_PRECISION,
                                                                                0,
                                                                                buffer_eval_db->var_check_time,
                                                                                false,
                                                                                variance_check_timer_cback);
    }
    return true;
}

bool audio_inteval_evaluator_get_var(T_AUDIO_INTERVAL_EVAL_HANDLE handle, float *var)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;
    if (audio_interval_check_handle(handle) == false)
    {
        return false;
    }

    if (var != NULL && buffer_eval_db->eval_ready)
    {
        *var = buffer_eval_db->val_long_term;
        return true;
    }
    else
    {
        return false;
    }
}

void audio_inteval_evaluator_update(T_AUDIO_INTERVAL_EVAL_HANDLE handle, uint16_t seq, uint32_t clk)
{
    float interval;
    float z;

    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;
    if (audio_interval_check_handle(handle) == false)
    {
        return;
    }

    if (buffer_eval_db->last_clk == 0)
    {
        buffer_eval_db->last_clk = clk;
        return;
    }

    interval = (((clk << 4) - (buffer_eval_db->last_clk << 4)) >> 4) * 625;
    interval = interval / 2000;
    buffer_eval_db->last_clk = clk;

    if (interval > 500.0f)
    {
        AUDIO_PRINT_INFO0("audio_inteval_evaluator_update: interval too large");
        audio_interval_evaluator_reset(handle);
        return;
    }

    interval_queue_update(&buffer_eval_db->interval_queue, interval);

    buffer_eval_db->count++;
    param_long_term_update(handle, interval);
    if (buffer_eval_db->eval_ready)
    {
        z = z_score(&buffer_eval_db->interval_queue, interval);
    }
    else
    {
        z = 0 ;
    }
#if INTERVAL_EVAL_DEBUG

    AUDIO_PRINT_TRACE7("bt_inteval_evaluator_update: count %u, z %i, interval %i, val %i, avg %i, val_lt %i, avg_lt %i",
                       buffer_eval_db->count,
                       float2Hint(z),
                       float2Hint(interval),
                       float2Hint(interval_queue.val),
                       float2Hint(interval_queue.avg),
                       float2Hint(buffer_eval_db->val_long_term),
                       float2Hint(buffer_eval_db->avg_long_term));

    /*
    DBG_DIRECT("audio_inteval_evaluator_update: count %u, z %.3f, interval %.3f, val %.3f, avg %.3f, val_lt %.3f, avg_lt %.3f",
               buffer_eval_db->count, z, interval, interval_queue.val, interval_queue.avg,
               buffer_eval_db->val_long_term, buffer_eval_db->avg_long_term);
    */
#else
    AUDIO_PRINT_TRACE3("bt_inteval_evaluator_update: z %i var_check %u unstable %u", float2Hint(z),
                       buffer_eval_db->var_check_timer_start, buffer_eval_db->interval_unstable);
#endif

    if (buffer_eval_db->eval_ready == false)
    {
        if (buffer_eval_db->count > 3 * STD_WINDOW)
        {
            buffer_eval_db->eval_ready = true;
            buffer_eval_db->interval_unstable = false;
            buffer_eval_db->cback(AUDIO_INTERVAL_EVAL_EVT_STABLE, NULL, buffer_eval_db->buffer_ent);
        }
        return;
    }

    if (buffer_eval_db->count == 0xffffffff)
    {
        audio_interval_evaluator_reset(handle);
        return;
    }

    if (buffer_eval_db->var_check_timer_start == false)
    {
        if (z > buffer_eval_db->z_thres)
        {
            buffer_eval_db->interval_unstable = true;
            buffer_eval_db->cback(AUDIO_INTERVAL_EVAL_EVT_UNSTABLE, NULL, buffer_eval_db->buffer_ent);
            sys_timer_stop(buffer_eval_db->stable_dejitter_timer_handle);
            buffer_eval_db->stable_dejitter_timer_start = false;
            sys_timer_restart(buffer_eval_db->interval_variance_check_timer_handle,
                              buffer_eval_db->var_check_time);
            buffer_eval_db->var_check_timer_start = true;
            AUDIO_PRINT_TRACE1("audio_inteval_evaluator_update: interval unstable %i", float2Hint(z));
        }
        else
        {
            if (buffer_eval_db->interval_unstable &&
                buffer_eval_db->interval_queue.val < buffer_eval_db->val_long_term &&
                buffer_eval_db->stable_dejitter_timer_start == false)
            {
                buffer_eval_db->stable_dejitter_timer_start = true;
                sys_timer_start(buffer_eval_db->stable_dejitter_timer_handle);
                AUDIO_PRINT_TRACE0("audio_inteval_evaluator_update: interval stable dejitter");
            }
        }
    }
}

bool audio_interval_get_stable_state(T_AUDIO_INTERVAL_EVAL_HANDLE handle, bool *stable)
{
    T_AUDIO_INTERVAL_BUFFER_DB *buffer_eval_db = handle;
    if (audio_interval_check_handle(handle) == false)
    {
        return false;
    }

    if (buffer_eval_db->eval_ready)
    {
        *stable = !buffer_eval_db->interval_unstable;
        return true;
    }
    else
    {
        return false;
    }
}

