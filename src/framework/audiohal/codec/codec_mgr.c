/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "os_mem.h"
#include "os_queue.h"
#include "os_msg.h"
#include "trace.h"
#include "sys_cfg.h"
#include "codec_mgr.h"
#include "codec_driver.h"
#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)
#include "anc_driver.h"
#endif
#include "audio_route.h"
#include "audio_pad.h"

/* TODO Remove Start */
#include "dsp_mgr.h"
#include "sport_driver.h"
#include "dsp_driver.h"
#include "fmc_api.h"
#include "patch_header_check.h"
/* TODO Remove End */

#define FSM_STATE_NULL                (-1)
#define FSM_EVENT_NULL                (-1)
#define FSM_INVALID_TRANS_TABLE_INDEX (-1)

#define CODEC_MSG_MAX_NUM           (0x10)

typedef struct t_codec_mgr_config
{
    uint32_t dac_sample_rate;
    uint32_t adc_sample_rate;
    uint8_t adc_gain_level;
    uint8_t dac_gain_level;
    T_AUDIO_CATEGORY category;
} T_CODEC_MGR_CONFIG;

typedef enum t_codec_mgr_op_event
{
    CODEC_MGR_OP_EVENT_START,
    CODEC_MGR_OP_EVENT_STOP,
} T_CODEC_MGR_OP_EVENT;

typedef enum t_codec_mgr_op_state
{
    CODEC_MGR_OP_STATE_IDLE,
    CODEC_MGR_OP_STATE_BUSY,
} T_CODEC_MGR_OP_STATE;

typedef enum t_codec_mgr_session_event
{
    CODEC_MGR_SESSION_EVENT_ENABLE,
    CODEC_MGR_SESSION_EVENT_ENABLE_COMPLETE,
    CODEC_MGR_SESSION_EVENT_DISABLE,
    CODEC_MGR_SESSION_EVENT_DISABLING,
    CODEC_MGR_SESSION_EVENT_DISABLE_COMPLETE,
} T_CODEC_MGR_SESSION_EVENT;

typedef int8_t (*P_TRANSITION_FUNC)(void *context);
typedef void (*P_ENTER_FUNC)(void *context);

typedef struct t_fsm_transition_table
{
    int8_t enter_state;
    int8_t event;
    P_TRANSITION_FUNC trans_func;
    int8_t next_state;
    P_ENTER_FUNC enter_func;
} T_FSM_TRANSITION_TABLE;

typedef struct t_fsm_handle
{
    int8_t state;
    const char *name;
    const T_FSM_TRANSITION_TABLE *trans_table;
    int8_t *index_table;
    int8_t event_max;
    int8_t state_max;
    void *context;
} T_FSM_HANDLE;

typedef struct t_codec_mgr_session
{
    struct t_codec_mgr_session *p_next;
    T_OS_QUEUE device_queue;
    T_FSM_HANDLE *session_fsm;
    T_CODEC_MGR_EVENT_PARAM event_param;
    void *context;
    T_CODEC_MGR_CONFIG config;
} T_CODEC_MGR_SESSION;

typedef struct t_codec_mgr_operation
{
    struct t_codec_mgr_operation *p_next;
    T_CODEC_MGR_SESSION *session;
    T_CODEC_MGR_SESSION_EVENT event;
} T_CODEC_MGR_OPERATION;

typedef struct t_codec_mgr_loopback
{
    uint16_t enable      : 1;
    uint16_t dac_ref_cnt : 3;
    uint16_t adc_ref_cnt : 3;
    uint16_t adc_ch      : 3;
    uint16_t adc_loopback: 1;
} T_CODEC_MGR_LOOPBACK;

typedef struct t_dac_ref
{
    uint8_t count_5m;
    uint8_t count_fs;
} T_DAC_REF;

typedef struct t_adc_ref
{
    uint8_t count;
} T_ADC_REF;

typedef struct t_spk_ref
{
    uint8_t count;
} T_SPK_REF;

typedef struct t_amic_ref
{
    uint8_t count;
} T_AMIC_REF;

typedef struct t_dmic_ref
{
    uint8_t count;
} T_DMIC_REF;

typedef struct t_adc_status
{
    uint8_t count;
    uint8_t total;
} T_ADC_STATUS;

typedef struct t_codec_config_header
{
    uint16_t sync_word;
    uint16_t reserved1;
    uint32_t tool_version;
    uint16_t user_version;
    uint16_t reserved2;
    uint32_t algo_block_offset;
    uint32_t eq_cmd_block_offset;
    uint32_t gain_table_block_offset;
    uint32_t vad_param_block_offset;
    uint32_t eq_extend_info_offset;
    uint32_t hw_eq_block_offset;
    uint8_t  reserved[8];
    uint32_t package_features;
    uint32_t voice_stream_feature_bit;
    uint32_t audio_stream_feature_bit;
} T_CODEC_CONFIG_HEADER;

typedef struct t_hw_eq_param_header
{
    uint8_t  eq_type;
    uint8_t  eq_chann;
    uint16_t param_length;
    uint32_t param_offset;
    uint32_t sample_rate;
} T_HW_EQ_PARAM_HEADER;

typedef struct t_hw_eq_block
{
    uint16_t sync_word;
    uint16_t reserved1;
    uint32_t eq_block_len;
    uint16_t eq_num;
    uint16_t reserved2;
    T_HW_EQ_PARAM_HEADER *param_header;
} T_HW_EQ_BLOCK;

typedef struct t_codec_mgr_db
{
    T_OS_QUEUE session_queue;
    T_OS_QUEUE operation_queue;
    T_FSM_HANDLE *operation_fsm;
    T_FSM_HANDLE *codec_fsm;
    T_CODEC_MGR_OPERATION *operation;
    T_CODEC_MGR_EVENT_PARAM event_param;
    P_CODEC_MGR_CBACK cback;
    T_DAC_REF dac_ref[DAC_CHANNEL_MAX];
    T_ADC_REF adc_ref[ADC_CHANNEL_MAX];
    T_SPK_REF spk_ref[SPK_CHANNEL_MAX];
    T_AMIC_REF amic_ref[AMIC_CHANNEL_MAX];
    T_DMIC_REF dmic_ref[DMIC_CHANNEL_MAX];
    T_CODEC_MGR_LOOPBACK loopback_chann[AUDIO_ROUTE_DAC_CHANNEL_NUM];
    bool sidetone_status;
    bool wait_sidetone_off;
    T_ADC_STATUS adc_status[AUDIO_CATEGORY_NUMBER];
    uint32_t eq_block_offset;
    T_HW_EQ_BLOCK eq_block;
} T_CODEC_MGR_DB;

typedef void (*P_ADC_GAIN_SET_ACTION)(T_CODEC_MGR_SESSION *session,
                                      T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path,
                                      bool set_immediately);

void *hCODECQueueHandleAu;
static T_CODEC_MGR_DB *codec_mgr_db = NULL;

static void codec_mgr_session_event_post(T_CODEC_MGR_SESSION_EVENT event);
static void codec_mgr_op_event_post(T_CODEC_MGR_OP_EVENT event);
static void codec_mgr_event_post(T_CODEC_MGR_EVENT event);
static void codec_mgr_session_route(T_CODEC_MGR_SESSION_EVENT event);
static int8_t codec_mgr_session_cfg_set(void *context);
static int8_t codec_mgr_session_cfg_clear(void *context);

/*audio route and codec mic tye define is different*/
const uint8_t codec_mgr_mic_type_table[] =
{
    [AUDIO_ROUTE_MIC_SINGLE_END]   = MIC_TYPE_SINGLE_END_AMIC,
    [AUDIO_ROUTE_MIC_DIFFERENTIAL] = MIC_TYPE_DIFFERENTIAL_AMIC,
    [AUDIO_ROUTE_MIC_FALLING]      = MIC_TYPE_FALLING_DMIC,
    [AUDIO_ROUTE_MIC_RAISING]      = MIC_TYPE_RAISING_DMIC,
};

static const T_CODEC_I2S_CH_LEN_SEL chann_len_map[] =
{
    [AUDIO_ROUTE_SPORT_CH_LEN_8_BIT]  = CODEC_I2S_CH_LEN_8,
    [AUDIO_ROUTE_SPORT_CH_LEN_16_BIT] = CODEC_I2S_CH_LEN_16,
    [AUDIO_ROUTE_SPORT_CH_LEN_20_BIT] = CODEC_I2S_CH_LEN_32,
    [AUDIO_ROUTE_SPORT_CH_LEN_24_BIT] = CODEC_I2S_CH_LEN_24,
    [AUDIO_ROUTE_SPORT_CH_LEN_32_BIT] = CODEC_I2S_CH_LEN_32,
};

static const T_CODEC_I2S_DATALEN_SEL data_len_map[] =
{
    [AUDIO_ROUTE_SPORT_DATA_LEN_8_BIT]  = I2S_DATALEN_8,
    [AUDIO_ROUTE_SPORT_DATA_LEN_16_BIT] = I2S_DATALEN_16,
    [AUDIO_ROUTE_SPORT_DATA_LEN_20_BIT] = I2S_DATALEN_MAX,
    [AUDIO_ROUTE_SPORT_DATA_LEN_24_BIT] = I2S_DATALEN_24,
    [AUDIO_ROUTE_SPORT_DATA_LEN_32_BIT] = I2S_DATALEN_MAX,
};

/* Depend on IC */
static const T_AUDIO_ROUTE_DAC_CHANNEL loopback_map [AUDIO_ROUTE_ADC_CHANNEL_NUM] =
{
    [AUDIO_ROUTE_ADC_CHANNEL0] = AUDIO_ROUTE_DAC_CHANNEL0,
    [AUDIO_ROUTE_ADC_CHANNEL1] = AUDIO_ROUTE_DAC_CHANNEL1,
    [AUDIO_ROUTE_ADC_CHANNEL2] = AUDIO_ROUTE_DAC_CHANNEL0,
    [AUDIO_ROUTE_ADC_CHANNEL3] = AUDIO_ROUTE_DAC_CHANNEL1,
    [AUDIO_ROUTE_ADC_CHANNEL4] = AUDIO_ROUTE_DAC_CHANNEL0,
    [AUDIO_ROUTE_ADC_CHANNEL5] = AUDIO_ROUTE_DAC_CHANNEL1,
};

void codec_mgr_loopback_enable(void)
{
    T_CODEC_ADC_CONFIG adc_config;
    T_CODEC_ADC_CHANNEL_SEL ch_sel;
    T_CODEC_MGR_LOOPBACK *loopback;
    uint8_t i;

    for (i = 0; i < AUDIO_ROUTE_DAC_CHANNEL_NUM; ++i)
    {
        loopback = &(codec_mgr_db->loopback_chann[i]);

        if (loopback->adc_loopback == false)
        {
            if (loopback->enable == true)
            {
                ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(loopback->adc_ch);

                codec_drv_config_init(CODEC_CONFIG_SEL_ADC, (void *)&adc_config);

                adc_config.loopback = CODEC_ADC_DECI_SEL_MUSIC;

                codec_drv_adc_config_set(ch_sel, &adc_config, false);

                loopback->adc_loopback = true;
            }
        }

        CODEC_PRINT_TRACE6("codec_mgr_loopback_enable: dac_ch %d, enable %d, adc_ref_cnt %d, dac_ref_cnt %d, adc_ch %d, adc_loopback %d",
                           i, loopback->enable, loopback->adc_ref_cnt, loopback->dac_ref_cnt, loopback->adc_ch,
                           loopback->adc_loopback);
    }
}

void codec_mgr_loopback_disable(void)
{
    T_CODEC_ADC_CONFIG adc_config;
    T_CODEC_ADC_CHANNEL_SEL ch_sel;
    T_CODEC_MGR_LOOPBACK *loopback;
    uint8_t i;

    for (i = 0; i < AUDIO_ROUTE_DAC_CHANNEL_NUM; ++i)
    {
        loopback = &(codec_mgr_db->loopback_chann[i]);

        if (loopback->adc_loopback == true)
        {
            if (loopback->enable == false)
            {
                ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(loopback->adc_ch);

                codec_drv_config_init(CODEC_CONFIG_SEL_ADC, (void *)&adc_config);

                adc_config.loopback = CODEC_ADC_DECI_SEL_AMIC;

                codec_drv_adc_config_set(ch_sel, &adc_config, false);

                loopback->adc_loopback = false;
            }
        }

        CODEC_PRINT_TRACE6("codec_mgr_loopback_disable: dac_ch %d, enable %d, adc_ref_cnt %d, dac_ref_cnt %d, adc_ch %d, adc_loopback %d",
                           i, loopback->enable, loopback->adc_ref_cnt, loopback->dac_ref_cnt, loopback->adc_ch,
                           loopback->adc_loopback);
    }
}

bool codec_mgr_handle_check(T_CODEC_MGR_SESSION_HANDLE handle)
{
    T_CODEC_MGR_SESSION *session = os_queue_peek(&(codec_mgr_db->session_queue), 0);
    while (session != NULL)
    {
        if (session == (T_CODEC_MGR_SESSION *)handle)
        {
            return true;
        }
        session = session->p_next;
    }

    return false;
}


static T_CODEC_MGR_OPERATION *codec_mgr_op_create(T_CODEC_MGR_SESSION_HANDLE handle,
                                                  T_CODEC_MGR_SESSION_EVENT event)
{
    T_CODEC_MGR_OPERATION *operation;
    operation = os_mem_alloc2(sizeof(T_CODEC_MGR_OPERATION));
    if (operation != NULL)
    {
        operation->session = (T_CODEC_MGR_SESSION *)handle;
        operation->event = event;
    }

    return operation;
}

static void codec_mgr_op_queue_in(T_CODEC_MGR_OPERATION *operation)
{
    os_queue_in(&(codec_mgr_db->operation_queue), operation);
}

static T_CODEC_MGR_OPERATION *codec_mgr_op_queue_peek(void)
{
    return os_queue_peek(&(codec_mgr_db->operation_queue), 0);
}

static T_CODEC_MGR_OPERATION *codec_mgr_op_queue_out(void)
{
    return os_queue_out(&(codec_mgr_db->operation_queue));
}

static void codec_mgr_op_destroy(T_CODEC_MGR_OPERATION *operation)
{
    if (operation != NULL)
    {
        os_mem_free(operation);
    }
}

static int8_t fsm_get_trans_table_index(T_FSM_HANDLE *handle, int8_t event)
{
    int8_t enter_state = handle->state;
    if ((event > handle->event_max) ||
        (enter_state > handle->state_max))
    {
        return FSM_INVALID_TRANS_TABLE_INDEX;
    }

    return handle->index_table[enter_state * (handle->event_max + 1) + event];
}

void fsm_run(T_FSM_HANDLE *handle, uint32_t event)
{
    int8_t next_state;
    int8_t index = FSM_INVALID_TRANS_TABLE_INDEX;

    int8_t enter_state = handle->state;
    const T_FSM_TRANSITION_TABLE *trans_table = handle->trans_table;
    if (trans_table == NULL)
    {
        return;
    }

    index = fsm_get_trans_table_index(handle, event);
    if (index != FSM_INVALID_TRANS_TABLE_INDEX)
    {
        if (trans_table[index].trans_func)
        {
            next_state = trans_table[index].trans_func(handle->context);
            if (trans_table[index].next_state != FSM_STATE_NULL)
            {
                handle->state = trans_table[index].next_state;
            }
            else
            {
                handle->state = next_state;
            }
        }
        else
        {
            handle->state = trans_table[index].next_state;
        }

        if (trans_table[index].enter_func)
        {
            trans_table[index].enter_func(handle->context);
        }
    }

    CODEC_PRINT_TRACE4("fsm_run: name %s, enter_state %d, event %d, next_state %d",
                       TRACE_STRING(handle->name), enter_state, event, handle->state);

}

static bool fsm_create_trans_index_table(T_FSM_HANDLE *handle,
                                         const T_FSM_TRANSITION_TABLE *trans_table)
{
    uint8_t i = 0;
    uint8_t state_max = 0;
    uint8_t event_max = 0;
    while (trans_table[i].enter_state != FSM_STATE_NULL)
    {
        if (trans_table[i].enter_state > state_max)
        {
            state_max = trans_table[i].enter_state;
        }
        if (trans_table[i].event > event_max)
        {
            event_max = trans_table[i].event;
        }
        i++;
    }
    handle->event_max = event_max;
    handle->state_max = state_max;

    handle->index_table = os_mem_alloc2((state_max + 1) * (event_max + 1));
    if (handle->index_table == NULL)
    {
        return false;
    }
    memset(handle->index_table, FSM_INVALID_TRANS_TABLE_INDEX, (state_max + 1) * (event_max + 1));

    i = 0;
    uint8_t enter_state = 0;
    uint8_t event = 0;
    while (trans_table[i].enter_state != FSM_STATE_NULL)
    {
        enter_state = trans_table[i].enter_state;
        event = trans_table[i].event;
        handle->index_table[enter_state * (handle->event_max + 1) + event] = i;
        i++;
    }

    return true;
}

T_FSM_HANDLE *fsm_init(const T_FSM_TRANSITION_TABLE *trans_table, int8_t init_state,
                       const char *name, void *context)
{
    T_FSM_HANDLE *handle;
    int32_t ret = 0;

    handle = os_mem_alloc2(sizeof(T_FSM_HANDLE));
    if (handle == NULL)
    {
        ret = 1;
        goto fail_alloc_handle;
    }

    handle->name        = name;
    handle->trans_table = trans_table;
    handle->state       = init_state;
    handle->context     = context;

    if (!fsm_create_trans_index_table(handle, trans_table))
    {
        ret = 2;
        goto fail_create_trans_index_table;
    }

    return handle;

fail_create_trans_index_table:
    os_mem_free(handle);
fail_alloc_handle:
    CODEC_PRINT_TRACE1("fsm_init: failed %d", -ret);
    return NULL;
}

static void fsm_deinit(T_FSM_HANDLE *handle)
{
    if (handle != NULL)
    {
        os_mem_free(handle->index_table);
        os_mem_free(handle);
    }
}

static void codec_mgr_op_enter_busy(void *context)
{
    T_CODEC_MGR_SESSION_EVENT event = codec_mgr_db->operation->event;
    codec_mgr_session_event_post(event);
}

static int8_t codec_mgr_op_start_action(void *context)
{
    T_CODEC_MGR_OPERATION *operation = codec_mgr_op_queue_peek();
    codec_mgr_db->operation = operation;

    return FSM_STATE_NULL;
}

static int8_t codec_mgr_op_stop_action(void *context)
{
    T_CODEC_MGR_OPERATION *operation = codec_mgr_op_queue_out();
    codec_mgr_op_destroy(operation);

    return FSM_STATE_NULL;
}

static void codec_mgr_op_enter_idle(void *context)
{
    T_CODEC_MGR_OPERATION *operation = os_queue_peek(&(codec_mgr_db->operation_queue), 0);
    if (operation != NULL)
    {
        codec_mgr_op_event_post(CODEC_MGR_OP_EVENT_START);
    }
}

const static T_FSM_TRANSITION_TABLE operation_fsm_trans_table[] =
{
    {CODEC_MGR_OP_STATE_IDLE, CODEC_MGR_OP_EVENT_START, codec_mgr_op_start_action, CODEC_MGR_OP_STATE_BUSY, codec_mgr_op_enter_busy},
    {CODEC_MGR_OP_STATE_BUSY, CODEC_MGR_OP_EVENT_STOP, codec_mgr_op_stop_action, CODEC_MGR_OP_STATE_IDLE, codec_mgr_op_enter_idle},
    {FSM_STATE_NULL, FSM_EVENT_NULL, NULL, FSM_STATE_NULL, NULL},
};

static void codec_mgr_op_event_post(T_CODEC_MGR_OP_EVENT event)
{
    fsm_run(codec_mgr_db->operation_fsm, event);
}

static T_CODEC_MGR_SESSION_STATE codec_mgr_get_main_session_state(void)
{
    T_CODEC_MGR_SESSION *session = os_queue_peek(&(codec_mgr_db->session_queue), 0);
    T_CODEC_MGR_SESSION_STATE state = CODEC_MGR_SESSION_STATE_DISABLED;
    while (session != NULL)
    {
        if (session->session_fsm->state > state)
        {
            state = (T_CODEC_MGR_SESSION_STATE)session->session_fsm->state;
            if (state == CODEC_MGR_SESSION_STATE_ENABLING)
            {
                break;
            }
        }
        session = session->p_next;
    }

    return state;
}

static void codec_mgr_enter_enabling(void *context)
{
    codec_drv_enable();
}

static void codec_mgr_enter_enabled(void *context)
{
    codec_mgr_session_event_post(CODEC_MGR_SESSION_EVENT_ENABLE_COMPLETE);
}

static void codec_mgr_enter_muting(void *context)
{
    codec_drv_set_mute();
}

static void codec_mgr_enter_muted(void *context)
{
    codec_mgr_session_event_post(CODEC_MGR_SESSION_EVENT_DISABLE_COMPLETE);
}

static void codec_mgr_enter_disabling(void *context)
{
    codec_drv_disable();
}

static void codec_mgr_enter_disabled(void *context)
{
    (codec_mgr_db->event_param).context = NULL;
    (codec_mgr_db->event_param).state = CODEC_MGR_STATE_POWER_OFF;
    dsp_send_msg(DSP_MSG_CODEC_STATE, 0, &(codec_mgr_db->event_param), 0);
}

const static T_FSM_TRANSITION_TABLE codec_mgr_fsm_trans_table[] =
{
    {CODEC_MGR_STATE_POWER_OFF, CODEC_MGR_EVENT_ENABLE, NULL, CODEC_MGR_STATE_WAIT_ACTIVE, codec_mgr_enter_enabling},
    {CODEC_MGR_STATE_WAIT_ACTIVE, CODEC_MGR_EVENT_ENABLE_COMPLETE, NULL, CODEC_MGR_STATE_ACTIVED, codec_mgr_enter_enabled},
    {CODEC_MGR_STATE_ACTIVED, CODEC_MGR_EVENT_MUTE, NULL, CODEC_MGR_STATE_WAIT_MUTE, codec_mgr_enter_muting},
    {CODEC_MGR_STATE_ACTIVED, CODEC_MGR_EVENT_ENABLE, NULL, CODEC_MGR_STATE_WAIT_ACTIVE, codec_mgr_enter_enabling},
    {CODEC_MGR_STATE_WAIT_MUTE, CODEC_MGR_EVENT_MUTE_COMPLETE, NULL, CODEC_MGR_STATE_MUTED, codec_mgr_enter_muted},
    {CODEC_MGR_STATE_MUTED, CODEC_MGR_EVENT_DISABLE, NULL, CODEC_MGR_STATE_WAIT_POWER_OFF, codec_mgr_enter_disabling},
    {CODEC_MGR_STATE_MUTED, CODEC_MGR_EVENT_ENABLE, NULL, CODEC_MGR_STATE_WAIT_ACTIVE, codec_mgr_enter_enabling},
    {CODEC_MGR_STATE_WAIT_POWER_OFF, CODEC_MGR_EVENT_DISABLE_COMPLETE, NULL, CODEC_MGR_STATE_POWER_OFF, codec_mgr_enter_disabled},
    {CODEC_MGR_STATE_WAIT_POWER_OFF, CODEC_MGR_EVENT_ENABLE, NULL, CODEC_MGR_STATE_WAIT_ACTIVE, codec_mgr_enter_enabling},
    {FSM_STATE_NULL, FSM_EVENT_NULL, NULL, FSM_STATE_NULL, NULL},
};

static void codec_mgr_event_post(T_CODEC_MGR_EVENT event)
{
    fsm_run(codec_mgr_db->codec_fsm, event);
}

static void codec_mgr_session_route(T_CODEC_MGR_SESSION_EVENT event)
{
    T_CODEC_MGR_SESSION_STATE state = codec_mgr_get_main_session_state();
    CODEC_PRINT_TRACE2("codec_mgr_session_route: state %d, event %d", state, event);
    switch (state)
    {
    case CODEC_MGR_SESSION_STATE_ENABLING:
        {
            if (event == CODEC_MGR_SESSION_EVENT_ENABLE)
            {
                codec_mgr_event_post(CODEC_MGR_EVENT_ENABLE);
            }
        }
        break;

    case CODEC_MGR_SESSION_STATE_DISABLING:
        {
            if (event == CODEC_MGR_SESSION_EVENT_DISABLE)
            {
                codec_mgr_event_post(CODEC_MGR_EVENT_MUTE);
            }
        }
        break;

    case CODEC_MGR_SESSION_STATE_ENABLED:
        {
            if (event == CODEC_MGR_SESSION_EVENT_DISABLE)
            {
                codec_mgr_session_event_post(CODEC_MGR_SESSION_EVENT_DISABLE_COMPLETE);
            }
        }
        break;

    default:
        break;
    }
}

static void codec_mgr_session_event_post(T_CODEC_MGR_SESSION_EVENT event)
{
    T_CODEC_MGR_SESSION *session = codec_mgr_db->operation->session;
    fsm_run(session->session_fsm, event);
}

static void codec_mgr_session_enter_enabling(void *context)
{
    codec_mgr_session_route(CODEC_MGR_SESSION_EVENT_ENABLE);
}

static void codec_mgr_session_enter_enabled(void *context)
{
    T_CODEC_MGR_SESSION *session = (T_CODEC_MGR_SESSION *)context;
    (session->event_param).context = session->context;
    (session->event_param).state = CODEC_MGR_SESSION_STATE_ENABLED;
    dsp_send_msg(DSP_MSG_CODEC_STATE, 0, &(session->event_param), 0);
    codec_mgr_op_event_post(CODEC_MGR_OP_EVENT_STOP);
}

static void codec_mgr_session_enter_disabling(void *context)
{
    codec_mgr_session_route(CODEC_MGR_SESSION_EVENT_DISABLE);
}

static void codec_mgr_session_enter_disabled(void *context)
{
    T_CODEC_MGR_SESSION *session = (T_CODEC_MGR_SESSION *)context;
    (session->event_param).context = session->context;
    (session->event_param).state = CODEC_MGR_SESSION_STATE_DISABLED;
    dsp_send_msg(DSP_MSG_CODEC_STATE, 0, &(session->event_param), 0);
    codec_mgr_op_event_post(CODEC_MGR_OP_EVENT_STOP);
}


static void codec_mgr_session_nop(void *context)
{
    codec_mgr_op_event_post(CODEC_MGR_OP_EVENT_STOP);
}

static void codec_mgr_session_enter_fading_out(void *context)
{
    T_CODEC_MGR_SESSION *session = (T_CODEC_MGR_SESSION *)context;

    if (codec_mgr_db->sidetone_status)
    {
        if ((session->config.category == AUDIO_CATEGORY_APT) ||
            (session->config.category == AUDIO_CATEGORY_VOICE) ||
            (session->config.category == AUDIO_CATEGORY_RECORD))
        {
            T_CODEC_MGR_SESSION *sidetone_session = os_queue_peek(&(codec_mgr_db->session_queue), 0);
            T_CODEC_MGR_SESSION_STATE state = CODEC_MGR_SESSION_STATE_DISABLED;

            while (sidetone_session != NULL)
            {
                if (sidetone_session->config.category == AUDIO_CATEGORY_SIDETONE)
                {
                    state = (T_CODEC_MGR_SESSION_STATE)sidetone_session->session_fsm->state;
                    break;
                }

                sidetone_session = sidetone_session->p_next;
            }

            if (state == CODEC_MGR_SESSION_STATE_ENABLED)
            {
                codec_mgr_session_event_post(CODEC_MGR_SESSION_EVENT_DISABLING);
            }
            else
            {
                codec_mgr_sidetone_set(0, 0, false);
                codec_mgr_db->wait_sidetone_off = true;
            }
        }
        else
        {
            codec_mgr_session_event_post(CODEC_MGR_SESSION_EVENT_DISABLING);
        }
    }
    else
    {
        codec_mgr_session_event_post(CODEC_MGR_SESSION_EVENT_DISABLING);
    }
}

const static T_FSM_TRANSITION_TABLE session_fsm_trans_table[] =
{
    {CODEC_MGR_SESSION_STATE_DISABLED, CODEC_MGR_SESSION_EVENT_ENABLE, codec_mgr_session_cfg_set, CODEC_MGR_SESSION_STATE_ENABLING, codec_mgr_session_enter_enabling},
    {CODEC_MGR_SESSION_STATE_DISABLED, CODEC_MGR_SESSION_EVENT_DISABLE, codec_mgr_session_cfg_clear, CODEC_MGR_SESSION_STATE_DISABLED, codec_mgr_session_nop},
    {CODEC_MGR_SESSION_STATE_ENABLING, CODEC_MGR_SESSION_EVENT_ENABLE_COMPLETE, NULL, CODEC_MGR_SESSION_STATE_ENABLED, codec_mgr_session_enter_enabled},
    {CODEC_MGR_SESSION_STATE_ENABLED, CODEC_MGR_SESSION_EVENT_DISABLE, NULL, CODEC_MGR_SESSION_STATE_DISABLING, codec_mgr_session_enter_fading_out},
    {CODEC_MGR_SESSION_STATE_DISABLING, CODEC_MGR_SESSION_EVENT_DISABLING, codec_mgr_session_cfg_clear, CODEC_MGR_SESSION_STATE_DISABLING, codec_mgr_session_enter_disabling},
    {CODEC_MGR_SESSION_STATE_ENABLED, CODEC_MGR_SESSION_EVENT_ENABLE, codec_mgr_session_cfg_set, CODEC_MGR_SESSION_STATE_ENABLING, codec_mgr_session_enter_enabling},
    {CODEC_MGR_SESSION_STATE_DISABLING, CODEC_MGR_SESSION_EVENT_DISABLE_COMPLETE, NULL, CODEC_MGR_SESSION_STATE_DISABLED, codec_mgr_session_enter_disabled},
    {FSM_STATE_NULL, FSM_EVENT_NULL, NULL, FSM_STATE_NULL, NULL},
};

bool codec_mgr_session_control(T_CODEC_MGR_SESSION_HANDLE handle,
                               T_CODEC_MGR_SESSION_EVENT event)
{
    int8_t ret = 0;
    T_CODEC_MGR_OPERATION *operation = NULL;

    CODEC_PRINT_TRACE2("codec_mgr_session_control: handle %p, event %d", handle, event);

    if (!codec_mgr_handle_check(handle))
    {
        ret = 1;
        goto fail_handle_invalid;
    }

    operation = codec_mgr_op_create(handle, event);
    if (operation == NULL)
    {
        ret = 2;
        goto fail_create_operation;
    }

    codec_mgr_op_queue_in(operation);
    codec_mgr_op_event_post(CODEC_MGR_OP_EVENT_START);

    return true;

fail_handle_invalid:
fail_create_operation:
    CODEC_PRINT_ERROR1("codec_mgr_session_control: fail, ret = %d", -ret);
    return false;
}

bool codec_mgr_session_enable(T_CODEC_MGR_SESSION_HANDLE handle)
{
    return codec_mgr_session_control(handle, CODEC_MGR_SESSION_EVENT_ENABLE);
}

bool codec_mgr_session_disable(T_CODEC_MGR_SESSION_HANDLE handle)
{
    return codec_mgr_session_control(handle, CODEC_MGR_SESSION_EVENT_DISABLE);
}

T_CODEC_MGR_SESSION_STATE codec_mgr_session_state_get(T_CODEC_MGR_SESSION_HANDLE handle)
{
    T_CODEC_MGR_SESSION_STATE state = CODEC_MGR_SESSION_STATE_DISABLED;
    T_CODEC_MGR_SESSION *session = (T_CODEC_MGR_SESSION *)handle;

    if (codec_mgr_handle_check(handle))
    {
        state = (T_CODEC_MGR_SESSION_STATE)session->session_fsm->state;
    }

    return state;
}

void codec_mgr_power_off(void)
{
    CODEC_PRINT_TRACE0("codec_mgr_power_off");

    codec_mgr_event_post(CODEC_MGR_EVENT_DISABLE);
}

T_CODEC_MGR_SESSION_HANDLE codec_mgr_session_create(T_AUDIO_CATEGORY category,
                                                    uint32_t         dac_sample_rate,
                                                    uint32_t         adc_sample_rate,
                                                    uint8_t          dac_gain_level,
                                                    uint8_t          adc_gain_level,
                                                    void            *context)
{
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP physical_path_group;
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path;
    T_AUDIO_ROUTE_SPORT_CFG tx;
    T_AUDIO_ROUTE_SPORT_CFG rx;
    T_AUDIO_ROUTE_SPORT_IDX sport_id;
    T_CODEC_MGR_SESSION *session;
    int32_t ret = 0;

    if (codec_mgr_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    session = os_mem_alloc2(sizeof(T_CODEC_MGR_SESSION));
    if (session == NULL)
    {
        ret = 2;
        goto fail_alloc_session;
    }

    session->session_fsm = fsm_init(session_fsm_trans_table, CODEC_MGR_SESSION_STATE_DISABLED,
                                    "codec_mgr_session_fsm", session);
    if (session->session_fsm == NULL)
    {
        ret = 3;
        goto fail_init_fsm;
    }

    session->context = context;
    session->config.category = category;
    session->config.adc_gain_level = adc_gain_level;
    session->config.dac_gain_level = dac_gain_level;
    session->config.adc_sample_rate = adc_sample_rate;
    session->config.dac_sample_rate = dac_sample_rate;

    physical_path_group = audio_route_physical_path_take(category);
    if (physical_path_group.physical_path_num != 0)
    {
        physical_path = physical_path_group.physical_path;
        for (uint8_t i = 0; i < physical_path_group.physical_path_num; ++i)
        {
            sport_id = (T_AUDIO_ROUTE_SPORT_IDX)physical_path[i].sport_idx;
            if (physical_path[i].logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER)
            {
                tx = audio_route_sport_cfg_get(sport_id, AUDIO_ROUTE_SPORT_CH_DIR_TX);
                if (tx.sample_rate)
                {
                    session->config.dac_sample_rate = tx.sample_rate;
                }
            }
            else if ((physical_path[i].logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_MIC) ||
                     (physical_path[i].logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_AUX_IN))
            {
                rx = audio_route_sport_cfg_get(sport_id, AUDIO_ROUTE_SPORT_CH_DIR_RX);
                if (rx.sample_rate)
                {
                    session->config.adc_sample_rate = rx.sample_rate;
                }
            }
        }

        audio_route_physical_path_give(&physical_path_group);
    }

    os_queue_init(&(session->device_queue));
    os_queue_in(&(codec_mgr_db->session_queue), session);

    return (T_CODEC_MGR_SESSION_HANDLE)session;

fail_init_fsm:
    os_mem_free(session);
fail_alloc_session:
fail_invalid_db:
    CODEC_PRINT_ERROR1("codec_mgr_session_create: failed %d", -ret);
    return NULL;
}

bool codec_mgr_session_destroy(T_CODEC_MGR_SESSION_HANDLE handle)
{
    T_CODEC_MGR_SESSION *session = (T_CODEC_MGR_SESSION *)handle;

    if (!codec_mgr_handle_check(handle))
    {
        return false;
    }

    os_queue_delete(&(codec_mgr_db->session_queue), session);
    fsm_deinit(session->session_fsm);
    os_mem_free(session);

    return true;
}

bool codec_mgr_cback(T_CODEC_CB_EVENT event, uint32_t param)
{
    CODEC_PRINT_TRACE1("codec_mgr_cback: event %d", event);

    switch (event)
    {
    case CODEC_CB_STATE_MSG:
        {
            T_CODEC_STATE state = (T_CODEC_STATE)(*((uint8_t *)param));

            switch (state)
            {
            case CODEC_STATE_OFF:
                {
                    codec_mgr_event_post(CODEC_MGR_EVENT_DISABLE_COMPLETE);
                }
                break;

            case CODEC_STATE_MUTE:
                {
                    codec_mgr_event_post(CODEC_MGR_EVENT_MUTE_COMPLETE);
                }
                break;

            case CODEC_STATE_ON:
                {
                    codec_mgr_event_post(CODEC_MGR_EVENT_ENABLE_COMPLETE);
                }
                break;

            default:
                break;
            }
        }
        break;

    case CODEC_CB_CLEAR_SIDETONE:
        {
            T_CODEC_MGR_OPERATION *operation = codec_mgr_op_queue_peek();
            int8_t state = operation->session->session_fsm->state;

            codec_drv_sidetone_config_clear(SIDETONE_CHANNEL_L, false);
            codec_drv_sidetone_config_clear(SIDETONE_CHANNEL_R, true);

            if (state == CODEC_MGR_SESSION_STATE_DISABLING && codec_mgr_db->wait_sidetone_off)
            {
                codec_mgr_session_event_post(CODEC_MGR_SESSION_EVENT_DISABLING);
                codec_mgr_db->wait_sidetone_off = false;
            }

            codec_mgr_db->cback(CODEC_MGR_EVENT_SIDETONE_DISABLED);
        }
        break;

    case CODEC_CB_SET_SIDETONE:
        {
            codec_mgr_db->cback(CODEC_MGR_EVENT_SIDETONE_ENABLED);
        }
        break;

    default:
        break;
    }

    return true;
}

T_CODEC_DMIC_CLK_SEL codec_mgr_get_codec_dmic_clk(uint8_t mic_sel)
{
    uint8_t dmic_clk = DMIC_CLK_5MHZ;

    if (mic_sel == MIC_SEL_DMIC_1)
    {
        dmic_clk = sys_cfg_const.dmic1_clock;
    }
    else if (mic_sel == MIC_SEL_DMIC_2)
    {
        dmic_clk = sys_cfg_const.dmic2_clock;
    }
    else if (mic_sel == MIC_SEL_DMIC_3)
    {
        dmic_clk = sys_cfg_const.dmic3_clock;
    }
    return (T_CODEC_DMIC_CLK_SEL)dmic_clk;
}


#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)
static T_ANC_DRV_MIC_SEL codec_mgr_get_anc_mic(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    T_ANC_DRV_MIC_SEL anc_mic;

    anc_mic = ANC_DRV_MIC_SEL_MAX;

    switch (physical_path->logic_io_type)
    {
    case AUDIO_ROUTE_LOGIC_INTERNAL_MIC_LEFT:
        {
            anc_mic = ANC_DRV_MIC_SEL_INT_L;
        }
        break;

    case AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_LEFT:
        {
            anc_mic = ANC_DRV_MIC_SEL_EXT_L;
        }
        break;

    case AUDIO_ROUTE_LOGIC_INTERNAL_MIC_RIGHT:
        {
            anc_mic = ANC_DRV_MIC_SEL_INT_R;
        }
        break;

    case AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_RIGHT:
        {
            anc_mic = ANC_DRV_MIC_SEL_EXT_R;
        }
        break;
    }

    return anc_mic;
}
#endif

static bool codec_mgr_is_anc_mic(T_AUDIO_ROUTE_LOGIC_IO_TYPE logical_io)
{
    if ((logical_io == AUDIO_ROUTE_LOGIC_INTERNAL_MIC_LEFT) ||
        (logical_io == AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_LEFT) ||
        (logical_io == AUDIO_ROUTE_LOGIC_INTERNAL_MIC_RIGHT) ||
        (logical_io == AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_RIGHT))
    {
        return true;
    }

    return false;
}

static bool codec_mgr_is_audio_mic(T_AUDIO_ROUTE_LOGIC_IO_TYPE logical_io)
{
    if ((logical_io == AUDIO_ROUTE_LOGIC_PRIMARY_MIC) ||
        (logical_io == AUDIO_ROUTE_LOGIC_SECONDARY_MIC) ||
        (logical_io == AUDIO_ROUTE_LOGIC_FUSION_MIC) ||
        (logical_io == AUDIO_ROUTE_LOGIC_BONE_MIC) ||
        (logical_io == AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC) ||
        (logical_io == AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC))
    {
        return true;
    }

    return false;
}

static void codec_mgr_eq_set(T_CODEC_HW_EQ_TYPE eq_type, uint8_t eq_chann, uint32_t sample_rate)
{
    uint16_t  i;
    uint32_t  param_offset;
    uint16_t  param_length;
    uint8_t  *param_buffer;

    for (i = 0; i < codec_mgr_db->eq_block.eq_num; i++)
    {
        if (codec_mgr_db->eq_block.param_header[i].eq_type == eq_type &&
            codec_mgr_db->eq_block.param_header[i].eq_chann == eq_chann &&
            codec_mgr_db->eq_block.param_header[i].sample_rate == sample_rate)
        {
            CODEC_PRINT_TRACE3("codec_mgr_eq_set: eq_type 0x%02x, eq_chann 0x%02x, sample_rate %d",
                               codec_mgr_db->eq_block.param_header[i].eq_type,
                               codec_mgr_db->eq_block.param_header[i].eq_chann,
                               codec_mgr_db->eq_block.param_header[i].sample_rate);

            param_offset = codec_mgr_db->eq_block_offset + codec_mgr_db->eq_block.param_header[i].param_offset;
            param_length = codec_mgr_db->eq_block.param_header[i].param_length;

            param_buffer = os_mem_alloc2(param_length);
            if (param_buffer != NULL)
            {
                fmc_flash_nor_read(flash_dsp_cfg_addr_get() + DSP_PARAM_OFFSET +
                                   param_offset,
                                   param_buffer, param_length);

                codec_drv_eq_data_set(eq_type, eq_chann, param_buffer, param_length);
                os_mem_free(param_buffer);
            }

            break;
        }
    }
}

static void codec_mgr_adc_config_set(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path,
                                     T_CODEC_MGR_CONFIG *codec_config, bool set_immediately)
{
    T_CODEC_ADC_CHANNEL_SEL ch_sel;
    T_CODEC_AMIC_CONFIG amic_config;
    T_CODEC_DMIC_CONFIG dmic_config;
    T_CODEC_ADC_CONFIG adc_config;
    T_CODEC_ADC_CONFIG ref_adc_config;
    uint8_t mic_type, mic_class;
    uint8_t codec_mic_sel, mic_src, mic_src_sel;
    T_CODEC_I2S_CHANNEL_SEL i2s_sel;
    T_AUDIO_ROUTE_DAC_CHANNEL dac_ch;

    dac_ch = AUDIO_ROUTE_DAC_CHANNEL_NUM;

    /*To be optimized */
    mic_type = codec_mgr_mic_type_table[physical_path->attr.mic_attr.mic_type];
    mic_class = physical_path->attr.mic_attr.mic_class;
    codec_mic_sel = codec_drv_get_mic_ch_sel(physical_path->attr.mic_attr.mic_sel, &mic_src);
    mic_src_sel = codec_drv_get_mic_src_sel(physical_path->attr.mic_attr.mic_sel, mic_type);
    ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(physical_path->attr.mic_attr.adc_ch);
    i2s_sel = (T_CODEC_I2S_CHANNEL_SEL)(physical_path->sport_idx);

    codec_drv_config_init(CODEC_CONFIG_SEL_ADC, (void *)&adc_config);
    adc_config.ad_src_sel = (T_CODEC_AD_SRC_SEL)mic_src;
    adc_config.mic_sel = mic_src_sel;

    if (codec_mgr_is_audio_mic(physical_path->logic_io_type))
    {
        if ((physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC) ||
            (physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC))
        {
            dac_ch = loopback_map[physical_path->attr.mic_attr.adc_ch];

            if (dac_ch < AUDIO_ROUTE_DAC_CHANNEL_NUM)
            {
                (codec_mgr_db->loopback_chann[dac_ch].adc_ref_cnt) ++;
                codec_mgr_db->loopback_chann[dac_ch].adc_ch = physical_path->attr.mic_attr.adc_ch;
                if ((codec_mgr_db->loopback_chann[dac_ch].dac_ref_cnt > 0) &&
                    (codec_mgr_db->loopback_chann[dac_ch].adc_ref_cnt > 0) &&
                    (codec_mgr_db->loopback_chann[dac_ch].adc_ch < AUDIO_ROUTE_ADC_CHANNEL_NUM))
                {
                    codec_mgr_db->loopback_chann[dac_ch].enable = true;
                }
            }

            codec_drv_config_init(CODEC_CONFIG_SEL_ADC, (void *)&ref_adc_config);
            ref_adc_config.i2s_sel = i2s_sel;
            ref_adc_config.asrc_en = 0;
            ref_adc_config.equalizer_en = physical_path->attr.mic_attr.equalizer_en;
            ref_adc_config.sample_rate = codec_config->adc_sample_rate;
            codec_drv_adc_config_set(ch_sel, &ref_adc_config, set_immediately);
            (codec_mgr_db->adc_ref[ch_sel].count)++;
        }
        else
        {
            if (codec_config->category != AUDIO_CATEGORY_SIDETONE)
            {
                adc_config.sample_rate = codec_config->adc_sample_rate;
            }

            adc_config.enable = 1;
            adc_config.i2s_sel = i2s_sel;
            adc_config.asrc_en = 0;
            adc_config.equalizer_en = physical_path->attr.mic_attr.equalizer_en;
            codec_drv_adc_config_set(ch_sel, &adc_config, set_immediately);
            (codec_mgr_db->adc_ref[ch_sel].count)++;

            if (mic_src == AD_SRC_AMIC)
            {
                codec_drv_config_init(CODEC_CONFIG_SEL_AMIC, (void *)&amic_config);
                amic_config.enable = 1;
                amic_config.ch_sel = (T_CODEC_AMIC_CHANNEL_SEL)codec_mic_sel;
                amic_config.mic_type = mic_type;
                amic_config.input_dev = INPUT_DEV_MIC;
                amic_config.mic_class = mic_class;
                codec_drv_amic_config_set((T_CODEC_AMIC_CHANNEL_SEL)codec_mic_sel, &amic_config, set_immediately);

                codec_mgr_db->amic_ref[codec_mic_sel].count++;

            }
            else if (mic_src == AD_SRC_DMIC)
            {
                codec_drv_config_init(CODEC_CONFIG_SEL_DMIC, (void *)&dmic_config);
                dmic_config.enable = 1;
                dmic_config.ch_sel = (T_CODEC_DMIC_CHANNEL_SEL)codec_mic_sel;
                dmic_config.mic_type = mic_type;
                dmic_config.dmic_clk_sel = codec_mgr_get_codec_dmic_clk(physical_path->attr.mic_attr.mic_sel);
                codec_drv_dmic_config_set((T_CODEC_DMIC_CHANNEL_SEL)codec_mic_sel, &dmic_config, set_immediately);

                codec_mgr_db->dmic_ref[codec_mic_sel].count++;
            }
        }
    }

    if (codec_mgr_is_anc_mic(physical_path->logic_io_type))
    {
        if (mic_src == AD_SRC_AMIC)
        {
            codec_drv_config_init(CODEC_CONFIG_SEL_AMIC, (void *)&amic_config);
            amic_config.enable = 1;
            amic_config.ch_sel = (T_CODEC_AMIC_CHANNEL_SEL)codec_mic_sel;
            amic_config.mic_type = mic_type;
            amic_config.input_dev = INPUT_DEV_MIC;
            amic_config.mic_class = mic_class;
            codec_drv_amic_config_set((T_CODEC_AMIC_CHANNEL_SEL)codec_mic_sel, &amic_config, set_immediately);

            codec_mgr_db->amic_ref[codec_mic_sel].count++;

        }
        else if (mic_src == AD_SRC_DMIC)
        {
            codec_drv_config_init(CODEC_CONFIG_SEL_DMIC, (void *)&dmic_config);
            dmic_config.enable = 1;
            dmic_config.ch_sel = (T_CODEC_DMIC_CHANNEL_SEL)codec_mic_sel;
            dmic_config.mic_type = mic_type;
            dmic_config.dmic_clk_sel = codec_mgr_get_codec_dmic_clk(physical_path->attr.mic_attr.mic_sel);
            codec_drv_dmic_config_set((T_CODEC_DMIC_CHANNEL_SEL)codec_mic_sel, &dmic_config, set_immediately);

            codec_mgr_db->dmic_ref[codec_mic_sel].count++;
        }
    }

    codec_mgr_db->adc_status[codec_config->category].count++;
}

static void codec_mgr_mic_enable(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path,
                                 T_CODEC_MGR_CONFIG *codec_config)
{
    T_CODEC_ADC_CHANNEL_SEL ch_sel;
    T_CODEC_I2S_CHANNEL_SEL i2s_sel;
    T_CODEC_I2S_CONFIG i2s_config;
    uint8_t i2s_rx_ch;
    T_CODEC_I2S_DATALEN_SEL data_length;
    T_CODEC_I2S_CH_LEN_SEL chann_length;
    T_SPORT_TDM_MODE_SEL tdm_mode;
    T_AUDIO_ROUTE_SPORT_CFG rtx_cfg;

    if (codec_mgr_is_audio_mic(physical_path->logic_io_type))
    {
        ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(physical_path->attr.mic_attr.adc_ch);
        i2s_sel = (T_CODEC_I2S_CHANNEL_SEL)(physical_path->sport_idx);

        codec_mgr_adc_config_set(physical_path, codec_config, false);

        rtx_cfg = audio_route_sport_cfg_get(physical_path->sport_idx, physical_path->sport_ch_dir);

        data_length = data_len_map[rtx_cfg.data_len];
        chann_length = chann_len_map[rtx_cfg.chann_len];
        tdm_mode = (T_SPORT_TDM_MODE_SEL)rtx_cfg.mode;
        i2s_rx_ch = physical_path->sport_ch;
        codec_drv_config_init(CODEC_CONFIG_SEL_I2S, (void *)&i2s_config);
        i2s_config.rx_data_format = (T_SPORT_DATA_FORMAT_SEL)rtx_cfg.format;
        i2s_config.rx_tdm_mode = tdm_mode;
        i2s_config.rx_data_len = data_length;
        i2s_config.rx_channel_len = chann_length;
        i2s_config.rx_data_ch_en[i2s_rx_ch] = 1;
        i2s_config.rx_data_ch_sel[i2s_rx_ch] = (T_CODEC_I2S_RX_CH_SEL)ch_sel;
        codec_drv_i2s_config_set(i2s_sel, &i2s_config, false);

        codec_mgr_eq_set(CODEC_MIC_HW_EQ, ch_sel, codec_config->adc_sample_rate);
    }

    if (codec_mgr_is_anc_mic(physical_path->logic_io_type))
    {
#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)
        uint8_t mic_type;
        T_ANC_DRV_MIC_SEL anc_mic;
        uint8_t mic_class;

        /*To be optimized */
        mic_type = codec_mgr_mic_type_table[physical_path->attr.mic_attr.mic_type];
        mic_class = physical_path->attr.mic_attr.mic_class;

        anc_mic = codec_mgr_get_anc_mic(physical_path);

        codec_mgr_adc_config_set(physical_path, codec_config, false);

        anc_drv_mic_src_setting(anc_mic,
                                physical_path->attr.mic_attr.mic_sel,
                                mic_type,
                                mic_class);
#endif
    }
}

static void codec_mgr_mic_disable(T_CODEC_MGR_SESSION *session,
                                  T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    T_CODEC_ADC_CHANNEL_SEL ch_sel;
    uint8_t mic_sel, mic_src;

    ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(physical_path->attr.mic_attr.adc_ch);
    mic_sel = codec_drv_get_mic_ch_sel(physical_path->attr.mic_attr.mic_sel, &mic_src);

    if (codec_mgr_db->adc_status[session->config.category].count > 0)
    {
        if (codec_mgr_is_audio_mic(physical_path->logic_io_type))
        {
            T_AUDIO_ROUTE_DAC_CHANNEL dac_ch;

            if ((physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC) ||
                (physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC))
            {
                dac_ch = loopback_map[physical_path->attr.mic_attr.adc_ch];
                codec_mgr_db->loopback_chann[dac_ch].adc_ch = AUDIO_ROUTE_ADC_CHANNEL_NUM;

                if ((codec_mgr_db->loopback_chann[dac_ch].adc_ref_cnt) != 0)
                {
                    (codec_mgr_db->loopback_chann[dac_ch].adc_ref_cnt)--;
                }

                if ((codec_mgr_db->loopback_chann[dac_ch].adc_ref_cnt) == 0)
                {
                    codec_mgr_db->loopback_chann[dac_ch].enable = false;
                }
            }
            else
            {
                if (mic_src == AD_SRC_AMIC)
                {
                    if (codec_mgr_db->amic_ref[mic_sel].count != 0)
                    {
                        codec_mgr_db->amic_ref[mic_sel].count--;
                    }

                    if (codec_mgr_db->amic_ref[mic_sel].count == 0)
                    {
                        codec_drv_amic_config_clear((T_CODEC_AMIC_CHANNEL_SEL)mic_sel, true);
                    }
                }
                else if (mic_src == AD_SRC_DMIC)
                {
                    if (codec_mgr_db->dmic_ref[mic_sel].count != 0)
                    {
                        codec_mgr_db->dmic_ref[mic_sel].count--;
                    }

                    if (codec_mgr_db->dmic_ref[mic_sel].count == 0)
                    {
                        codec_drv_dmic_config_clear((T_CODEC_DMIC_CHANNEL_SEL)mic_sel, true);
                    }
                }
            }

            if (codec_mgr_db->adc_ref[ch_sel].count != 0)
            {
                codec_mgr_db->adc_ref[ch_sel].count--;
            }

            if (codec_mgr_db->adc_ref[ch_sel].count == 0)
            {
                codec_drv_adc_config_clear(ch_sel, true);
                codec_drv_eq_data_clear(CODEC_EQ_CONFIG_PATH_ADC, ch_sel);
            }
        }

        if (codec_mgr_is_anc_mic(physical_path->logic_io_type))
        {
#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)
            T_ANC_DRV_MIC_SEL anc_mic;
            uint8_t mic_class = 0;

            if (mic_src == AD_SRC_AMIC)
            {
                if (codec_mgr_db->amic_ref[mic_sel].count != 0)
                {
                    codec_mgr_db->amic_ref[mic_sel].count--;
                }

                if (codec_mgr_db->amic_ref[mic_sel].count == 0)
                {
                    codec_drv_amic_config_clear((T_CODEC_AMIC_CHANNEL_SEL)mic_sel, true);
                }
            }
            else if (mic_src == AD_SRC_DMIC)
            {
                if (codec_mgr_db->dmic_ref[mic_sel].count != 0)
                {
                    codec_mgr_db->dmic_ref[mic_sel].count--;
                }

                if (codec_mgr_db->dmic_ref[mic_sel].count == 0)
                {
                    codec_drv_dmic_config_clear((T_CODEC_DMIC_CHANNEL_SEL)mic_sel, true);
                }
            }

            anc_mic = codec_mgr_get_anc_mic(physical_path);
            anc_drv_mic_src_setting(anc_mic, ANC_DRV_DONT_CARE_8, ANC_DRV_DONT_CARE_8, mic_class);
#endif
        }

        codec_mgr_db->adc_status[session->config.category].count--;
    }
}

static void codec_mgr_mix_enable(void)
{
    T_CODEC_DAC_CONFIG dac_config;

    if (sys_cfg_const.dac_0_mix_point != 0)
    {
        codec_drv_config_init(CODEC_CONFIG_SEL_DAC, (void *)&dac_config);
        dac_config.ana_power_en = 1;
        dac_config.dig_power_en = 1;
        dac_config.downlink_mix = (T_CODEC_DOWNLINK_MIX)sys_cfg_const.dac_0_mix_point;
        dac_config.music_mute_en = 0;

        if (codec_mgr_db->dac_ref[DAC_CHANNEL_L].count_fs == 0)
        {
            dac_config.dig_gain = 0;
        }

        codec_mgr_db->dac_ref[DAC_CHANNEL_L].count_5m++;
        codec_drv_dac_config_set(DAC_CHANNEL_L, &dac_config, false);
    }

    if (sys_cfg_const.dac_1_mix_point != 0)
    {
        codec_drv_config_init(CODEC_CONFIG_SEL_DAC, (void *)&dac_config);
        dac_config.ana_power_en = 1;
        dac_config.dig_power_en = 1;
        dac_config.downlink_mix = (T_CODEC_DOWNLINK_MIX)sys_cfg_const.dac_1_mix_point;
        dac_config.music_mute_en = 0;

        if (codec_mgr_db->dac_ref[DAC_CHANNEL_R].count_fs == 0)
        {
            dac_config.dig_gain = 0;
        }

        codec_mgr_db->dac_ref[DAC_CHANNEL_R].count_5m++;
        codec_drv_dac_config_set(DAC_CHANNEL_R, &dac_config, false);
    }
}

static void codec_mgr_spk_enable(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path,
                                 T_CODEC_MGR_CONFIG *codec_config)
{
    T_CODEC_SPK_CONFIG spk_config;
    T_CODEC_DAC_CONFIG dac_config;
    T_CODEC_I2S_CONFIG i2s_config;
    T_CODEC_I2S_CHANNEL_SEL i2s_sel;
    uint32_t spk_sample_rate = 0;
    T_CODEC_I2S_DATALEN_SEL data_length;
    T_CODEC_I2S_CH_LEN_SEL chann_length;
    T_AUDIO_ROUTE_DAC_CHANNEL dac_ch;
    T_AUDIO_ROUTE_SPK_SEL spk_sel;
    T_AUDIO_ROUTE_SPORT_CFG rtx_cfg;
    dac_ch = physical_path->attr.spk_attr.dac_ch;
    spk_sel = physical_path->attr.spk_attr.spk_sel;

    codec_drv_config_init(CODEC_CONFIG_SEL_DAC, (void *)&dac_config);
    // spk setting
    codec_drv_config_init(CODEC_CONFIG_SEL_SPK, (void *)&spk_config);

    if ((physical_path->logic_io_type != AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER) &&
        (physical_path->logic_io_type != AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER))
    {
        codec_mgr_db->spk_ref[spk_sel].count++;
        spk_config.power_en = 1;
        spk_config.spk_type = physical_path->attr.spk_attr.spk_type;
        spk_config.spk_class = physical_path->attr.spk_attr.spk_class;
        codec_drv_spk_config_set((T_CODEC_SPK_CHANNEL_SEL)spk_sel, &spk_config, false);
    }

    if ((physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER) ||
        (physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER))
    {
        (codec_mgr_db->loopback_chann[dac_ch].dac_ref_cnt) ++;

        if ((codec_mgr_db->loopback_chann[dac_ch].adc_ref_cnt > 0) &&
            (codec_mgr_db->loopback_chann[dac_ch].dac_ref_cnt > 0) &&
            (codec_mgr_db->loopback_chann[dac_ch].adc_ch < AUDIO_ROUTE_ADC_CHANNEL_NUM))
        {
            codec_mgr_db->loopback_chann[dac_ch].enable = true;
        }
    }

    if ((physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER) ||
        (physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER))
    {
        spk_sample_rate = codec_config->dac_sample_rate;

        if (codec_mgr_db->dac_ref[dac_ch].count_fs != 0)
        {
            spk_sample_rate = 0xFFFFFFFF;
        }

        if ((codec_config->category == AUDIO_CATEGORY_ANC) ||
            (codec_config->category == AUDIO_CATEGORY_LLAPT))
        {
            codec_mgr_db->dac_ref[dac_ch].count_5m++;
            dac_config.anc_mute_en = 0;
        }
        else if (codec_config->category == AUDIO_CATEGORY_SIDETONE)
        {
            codec_mgr_db->dac_ref[dac_ch].count_5m++;
            dac_config.music_mute_en = 0;
        }
        else
        {
            codec_mgr_db->dac_ref[dac_ch].count_5m++;
            codec_mgr_db->dac_ref[dac_ch].count_fs++;
            dac_config.music_mute_en = 0;
            dac_config.i2s_sel = (T_CODEC_I2S_CHANNEL_SEL)physical_path->sport_idx;
        }

        dac_config.equalizer_en = physical_path->attr.spk_attr.equalizer_en;
        dac_config.ana_power_en = 1;
        dac_config.dig_power_en = 1;
        dac_config.sample_rate = spk_sample_rate;
        dac_config.asrc_en = 0;

        codec_drv_dac_config_set((T_CODEC_DAC_CHANNEL_SEL)dac_ch, &dac_config, false);
    }

    // i2s setting
    rtx_cfg = audio_route_sport_cfg_get(physical_path->sport_idx, physical_path->sport_ch_dir);
    data_length = data_len_map[rtx_cfg.data_len];
    chann_length = chann_len_map[rtx_cfg.chann_len];
    i2s_sel = (T_CODEC_I2S_CHANNEL_SEL)(physical_path->sport_idx);
    codec_drv_config_init(CODEC_CONFIG_SEL_I2S, (void *)&i2s_config);
    i2s_config.tx_data_format = (T_SPORT_DATA_FORMAT_SEL)rtx_cfg.format;
    i2s_config.tx_data_len = data_length;
    i2s_config.tx_channel_len = chann_length;
    codec_drv_i2s_config_set(i2s_sel, &i2s_config, false);

    codec_mgr_eq_set(CODEC_SPK_HW_EQ, dac_ch, spk_sample_rate);
}

static bool codec_mgr_is_idle(void)
{
    uint32_t ref_count;
    uint8_t i;

    ref_count = 0;

    for (i = 0; i < DAC_CHANNEL_MAX; ++i)
    {
        ref_count += codec_mgr_db->dac_ref[i].count_5m;
        ref_count += codec_mgr_db->dac_ref[i].count_fs;
    }

    for (i = 0; i < SPK_CHANNEL_MAX; ++i)
    {
        ref_count += codec_mgr_db->spk_ref[i].count;
    }

    return (ref_count == 0) ? true : false;
}

static void codec_mgr_mix_disable(void)
{
    if (sys_cfg_const.dac_0_mix_point != 0)
    {
        codec_mgr_db->dac_ref[DAC_CHANNEL_L].count_5m--;

        if ((codec_mgr_db->dac_ref[DAC_CHANNEL_L].count_5m == 0) &&
            (codec_mgr_db->dac_ref[DAC_CHANNEL_L].count_fs == 0))
        {
            codec_drv_dac_config_clear((T_CODEC_DAC_CHANNEL_SEL)DAC_CHANNEL_L, true);
            codec_drv_eq_data_clear(CODEC_EQ_CONFIG_PATH_DAC, DAC_CHANNEL_L);
        }

        if (codec_mgr_is_idle())
        {
            codec_drv_spk_config_clear(SPK_CHANNEL_L, true);
        }
    }

    if (sys_cfg_const.dac_1_mix_point != 0)
    {
        codec_mgr_db->dac_ref[DAC_CHANNEL_R].count_5m--;

        if ((codec_mgr_db->dac_ref[DAC_CHANNEL_R].count_5m == 0) &&
            (codec_mgr_db->dac_ref[DAC_CHANNEL_R].count_fs == 0))
        {
            codec_drv_dac_config_clear((T_CODEC_DAC_CHANNEL_SEL)DAC_CHANNEL_R, true);
            codec_drv_eq_data_clear(CODEC_EQ_CONFIG_PATH_DAC, DAC_CHANNEL_R);
        }

        if (codec_mgr_is_idle())
        {
            codec_drv_spk_config_clear(SPK_CHANNEL_R, true);
        }
    }
}

static void codec_mgr_spk_disable(T_CODEC_MGR_SESSION *session,
                                  T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    T_AUDIO_ROUTE_DAC_CHANNEL dac_ch;
    T_CODEC_DAC_CONFIG dac_config;
    T_AUDIO_ROUTE_SPK_SEL spk_sel;

    dac_ch = physical_path->attr.spk_attr.dac_ch;
    spk_sel = physical_path->attr.spk_attr.spk_sel;

    if ((physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER) ||
        (physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER))
    {
        if (codec_mgr_db->spk_ref[spk_sel].count != 0)
        {
            codec_mgr_db->spk_ref[spk_sel].count--;
        }

        if (codec_mgr_db->dac_ref[dac_ch].count_5m != 0)
        {
            codec_mgr_db->dac_ref[dac_ch].count_5m--;
        }

        if ((session->config.category == AUDIO_CATEGORY_ANC) ||
            (session->config.category == AUDIO_CATEGORY_LLAPT))
        {
            T_CODEC_DAC_CONFIG dac_config;
            codec_drv_config_init(CODEC_CONFIG_SEL_DAC, (void *)&dac_config);
            dac_config.anc_mute_en = 1;
            codec_drv_dac_config_set((T_CODEC_DAC_CHANNEL_SEL)dac_ch, &dac_config, true);
        }
        else if (codec_mgr_db->dac_ref[dac_ch].count_fs != 0)
        {
            if (session->config.category != AUDIO_CATEGORY_SIDETONE)
            {
                (codec_mgr_db->dac_ref[dac_ch].count_fs)--;
            }
        }

        if ((codec_mgr_db->dac_ref[dac_ch].count_5m == 0) &&
            (codec_mgr_db->dac_ref[dac_ch].count_fs == 0))
        {
            codec_drv_dac_config_clear((T_CODEC_DAC_CHANNEL_SEL)dac_ch, true);
            codec_drv_eq_data_clear(CODEC_EQ_CONFIG_PATH_DAC, dac_ch);
        }
        else if ((codec_mgr_db->dac_ref[dac_ch].count_fs == 0))
        {
            codec_drv_config_init(CODEC_CONFIG_SEL_DAC, (void *)&dac_config);
            dac_config.dig_gain = 0;
            codec_drv_dac_config_set((T_CODEC_DAC_CHANNEL_SEL)dac_ch, &dac_config, true);
        }

        if (codec_mgr_is_idle())
        {
            codec_drv_spk_config_clear((T_CODEC_SPK_CHANNEL_SEL)spk_sel, true);
        }
    }
    else if ((physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER) ||
             (physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER))
    {
        if ((codec_mgr_db->loopback_chann[dac_ch].dac_ref_cnt) != 0)
        {
            (codec_mgr_db->loopback_chann[dac_ch].dac_ref_cnt)--;
        }

        if (codec_mgr_db->loopback_chann[dac_ch].dac_ref_cnt == 0)
        {
            codec_mgr_db->loopback_chann[dac_ch].enable = 0;
        }
    }
}

static void codec_mgr_aux_in_enable(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path,
                                    T_CODEC_MGR_CONFIG *codec_config)
{
    T_CODEC_AMIC_CONFIG amic_config;
    T_CODEC_ADC_CONFIG adc_config;
    T_CODEC_ADC_CHANNEL_SEL ch_sel;
    T_CODEC_I2S_CHANNEL_SEL i2s_sel;
    uint8_t codec_mic_sel, mic_src;
    uint8_t mic_type, mic_class;
    T_CODEC_I2S_CONFIG i2s_config;
    uint8_t i2s_rx_ch;
    T_CODEC_I2S_DATALEN_SEL data_length;
    T_CODEC_I2S_CH_LEN_SEL chann_length;
    T_SPORT_TDM_MODE_SEL tdm_mode;
    T_AUDIO_ROUTE_SPORT_CFG rtx_cfg;

    codec_mic_sel = codec_drv_get_mic_ch_sel(physical_path->attr.aux_in_attr.mic_sel, &mic_src);
    ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(physical_path->attr.aux_in_attr.adc_ch);
    i2s_sel = (T_CODEC_I2S_CHANNEL_SEL)(physical_path->sport_idx);
    mic_type = codec_mgr_mic_type_table[physical_path->attr.aux_in_attr.mic_type];
    mic_class = physical_path->attr.aux_in_attr.mic_class;

    codec_drv_config_init(CODEC_CONFIG_SEL_AMIC, (void *)&amic_config);
    amic_config.enable = 1;
    amic_config.ch_sel = (T_CODEC_AMIC_CHANNEL_SEL)codec_mic_sel;
    amic_config.mic_type = mic_type;
    amic_config.input_dev = INPUT_DEV_AUX;
    amic_config.mic_class = mic_class;
    codec_drv_amic_config_set((T_CODEC_AMIC_CHANNEL_SEL)codec_mic_sel, &amic_config, false);

    codec_drv_config_init(CODEC_CONFIG_SEL_ADC, (void *)&adc_config);
    adc_config.mic_sel = codec_mic_sel;
    adc_config.enable = 1;
    adc_config.i2s_sel = i2s_sel;
    adc_config.asrc_en = 0;
    adc_config.equalizer_en = physical_path->attr.aux_in_attr.equalizer_en;
    adc_config.ad_src_sel = (T_CODEC_AD_SRC_SEL)mic_src;
    adc_config.sample_rate = codec_config->adc_sample_rate;

    codec_drv_adc_config_set(ch_sel, &adc_config, false);

    rtx_cfg = audio_route_sport_cfg_get(physical_path->sport_idx, physical_path->sport_ch_dir);

    data_length = data_len_map[rtx_cfg.data_len];
    chann_length = chann_len_map[rtx_cfg.chann_len];
    tdm_mode = (T_SPORT_TDM_MODE_SEL)rtx_cfg.mode;

    i2s_rx_ch = physical_path->sport_ch;
    codec_drv_config_init(CODEC_CONFIG_SEL_I2S, (void *)&i2s_config);
    i2s_config.rx_data_format = (T_SPORT_DATA_FORMAT_SEL)rtx_cfg.format;
    i2s_config.rx_tdm_mode = tdm_mode;
    i2s_config.tx_data_len = data_length;
    i2s_config.tx_channel_len = chann_length;
    i2s_config.rx_data_len = data_length;
    i2s_config.rx_channel_len = chann_length;
    i2s_config.rx_data_ch_en[i2s_rx_ch] = 1;
    i2s_config.rx_data_ch_sel[i2s_rx_ch] = (T_CODEC_I2S_RX_CH_SEL)ch_sel;
    codec_drv_i2s_config_set(i2s_sel, &i2s_config, false);

    codec_mgr_eq_set(CODEC_MIC_HW_EQ, ch_sel, codec_config->adc_sample_rate);
}

static void codec_mgr_aux_in_disable(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    T_CODEC_ADC_CHANNEL_SEL ch_sel;
    uint8_t mic_sel, mic_src;

    ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(physical_path->attr.aux_in_attr.adc_ch);

    mic_sel = codec_drv_get_mic_ch_sel(physical_path->attr.aux_in_attr.mic_sel, &mic_src);
    codec_drv_adc_config_clear(ch_sel, true);
    codec_drv_amic_config_clear((T_CODEC_AMIC_CHANNEL_SEL)mic_sel, true);
}

static void codec_mgr_adc_gain_mute_action(T_CODEC_MGR_SESSION *session,
                                           T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path,
                                           bool set_immediately)
{
    T_CODEC_ADC_CHANNEL_SEL ch_sel;

    ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(physical_path->attr.mic_attr.adc_ch);

    CODEC_PRINT_TRACE2("codec_mgr_adc_gain_mute_action: ch_sel %d, physical_io_type %d",
                       ch_sel, physical_path->physical_io_type);

    if ((physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_MIC) ||
        (physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_AUX_IN))
    {
        if (codec_mgr_is_audio_mic(physical_path->logic_io_type))
        {
            T_AUDIO_ROUTE_DAC_CHANNEL dac_ch;
            T_CODEC_MGR_LOOPBACK *loopback;

            dac_ch = loopback_map[physical_path->attr.mic_attr.adc_ch];

            loopback = &(codec_mgr_db->loopback_chann[dac_ch]);

            if ((loopback->adc_loopback == true) && (loopback->adc_ch == ch_sel))
            {
                loopback->adc_loopback = false;
            }

            if (codec_mgr_db->sidetone_status == false)
            {
                codec_mgr_mic_disable(session, physical_path);
            }
        }
    }
}

static void codec_mgr_adc_gain_set_action(T_CODEC_MGR_SESSION *session,
                                          T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path,
                                          bool set_immediately)
{
    uint8_t mic_sel, mic_src;
    T_CODEC_AMIC_CONFIG amic_config;
    T_CODEC_ADC_CONFIG adc_config;
    T_CODEC_ADC_CHANNEL_SEL ch_sel;
    T_CODEC_MGR_CONFIG *codec_config;

    codec_config = &(session->config);

    mic_sel = codec_drv_get_mic_ch_sel(physical_path->attr.mic_attr.mic_sel, &mic_src);

    if (physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_MIC)
    {
        ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(physical_path->attr.mic_attr.adc_ch);

        codec_drv_config_init(CODEC_CONFIG_SEL_ADC, (void *)&adc_config);

        if (codec_mgr_is_audio_mic(physical_path->logic_io_type))
        {
            if (codec_mgr_db->adc_status[codec_config->category].count !=
                codec_mgr_db->adc_status[codec_config->category].total)
            {
                codec_mgr_adc_config_set(physical_path, codec_config, set_immediately);
                codec_mgr_loopback_enable();
            }

            if ((physical_path->logic_io_type != AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC) &&
                (physical_path->logic_io_type != AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC))
            {
                if (mic_src == AD_SRC_AMIC)
                {
                    codec_drv_config_init(CODEC_CONFIG_SEL_AMIC, (void *)&amic_config);
                    amic_config.ana_gain = (T_CODEC_ADC_ANA_GAIN)physical_path->attr.mic_attr.ana_gain;
                    codec_drv_amic_config_set((T_CODEC_AMIC_CHANNEL_SEL)mic_sel, &amic_config, set_immediately);
                }
            }

            codec_drv_config_init(CODEC_CONFIG_SEL_ADC, (void *)&adc_config);

            adc_config.dig_gain = physical_path->attr.mic_attr.dig_gain;
            adc_config.boost_gain = (T_CODEC_ADC_BOOST_GAIN)physical_path->attr.mic_attr.dig_boost_gain;
            codec_drv_adc_config_set(ch_sel, &adc_config, set_immediately);
        }

        if (codec_mgr_is_anc_mic(physical_path->logic_io_type))
        {
            if (mic_src == AD_SRC_AMIC)
            {
                codec_drv_config_init(CODEC_CONFIG_SEL_AMIC, (void *)&amic_config);
                amic_config.ana_gain = (T_CODEC_ADC_ANA_GAIN)physical_path->attr.mic_attr.ana_gain;
                codec_drv_amic_config_set((T_CODEC_AMIC_CHANNEL_SEL)mic_sel, &amic_config, set_immediately);
            }
        }
    }
    else if (physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_AUX_IN)
    {
        ch_sel = (T_CODEC_ADC_CHANNEL_SEL)(physical_path->attr.aux_in_attr.adc_ch);

        if (codec_mgr_db->adc_status[codec_config->category].count !=
            codec_mgr_db->adc_status[codec_config->category].total)
        {
            codec_mgr_adc_config_set(physical_path, codec_config, set_immediately);
            codec_mgr_loopback_enable();
        }
        codec_drv_config_init(CODEC_CONFIG_SEL_AMIC, (void *)&amic_config);
        amic_config.ana_gain = (T_CODEC_ADC_ANA_GAIN)physical_path->attr.aux_in_attr.ana_gain;
        codec_drv_amic_config_set((T_CODEC_AMIC_CHANNEL_SEL)mic_sel, &amic_config, set_immediately);

        codec_drv_config_init(CODEC_CONFIG_SEL_ADC, (void *)&adc_config);
        adc_config.dig_gain = physical_path->attr.aux_in_attr.dig_gain;
        adc_config.boost_gain = (T_CODEC_ADC_BOOST_GAIN)physical_path->attr.aux_in_attr.dig_boost_gain;
        codec_drv_adc_config_set(ch_sel, &adc_config, set_immediately);
    }
    else
    {
        return ;
    }

    CODEC_PRINT_TRACE5("codec_mgr_adc_gain_set_action:ch_sel %d, physical_io_type %d, dig_gain 0x%02x, "
                       "boost_gain 0x%02x, ana_gain 0x%02x", ch_sel, physical_path->physical_io_type,
                       adc_config.dig_gain, adc_config.boost_gain, amic_config.ana_gain);
}

static void codec_mgr_adc_gain_config(T_CODEC_MGR_SESSION_HANDLE handle,
                                      P_ADC_GAIN_SET_ACTION action)
{
    T_CODEC_MGR_SESSION *session = (T_CODEC_MGR_SESSION *)handle;
    uint8_t physical_path_num;
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path;
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP physical_path_group;
    T_AUDIO_CATEGORY category;
    bool set_immediately = false;

    if (codec_mgr_session_state_get(handle) == CODEC_MGR_SESSION_STATE_ENABLED)
    {
        set_immediately = true;
    }

    category = session->config.category;

    physical_path_group = audio_route_physical_path_take(category);
    physical_path = physical_path_group.physical_path;
    physical_path_num = physical_path_group.physical_path_num;
    if (physical_path != NULL)
    {
        for (uint8_t i = 0; i < physical_path_num; i++)
        {
            action(session, physical_path, set_immediately);

            physical_path++;
        }

        audio_route_physical_path_give(&physical_path_group);
    }
}

void codec_mgr_adc_gain_mute(T_CODEC_MGR_SESSION_HANDLE handle)
{
    codec_mgr_adc_gain_config(handle, codec_mgr_adc_gain_mute_action);
}

void codec_mgr_adc_gain_set(T_CODEC_MGR_SESSION_HANDLE handle,
                            uint8_t level)
{
    codec_mgr_adc_gain_config(handle, codec_mgr_adc_gain_set_action);
}

void codec_mgr_dac_gain_set(T_CODEC_MGR_SESSION_HANDLE handle,
                            uint8_t level)
{
    T_CODEC_MGR_SESSION *session = (T_CODEC_MGR_SESSION *)handle;
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path;
    uint8_t physical_path_num;
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP pysical_path_group;
    T_CODEC_DAC_CONFIG dac_config;
    T_AUDIO_ROUTE_DAC_GAIN dac_gain;
    T_AUDIO_ROUTE_DAC_CHANNEL dac_ch;
    T_AUDIO_CATEGORY category;
    bool set_immediately = false;

    if (codec_mgr_session_state_get(handle) == CODEC_MGR_SESSION_STATE_ENABLED)
    {
        set_immediately = true;
    }

    category = session->config.category;
    if ((category == AUDIO_CATEGORY_ANC) || (category == AUDIO_CATEGORY_LLAPT))
    {
        return;
    }

    pysical_path_group = audio_route_physical_path_take(category);
    physical_path = pysical_path_group.physical_path;
    physical_path_num = pysical_path_group.physical_path_num;
    if (physical_path != NULL)
    {
        for (uint8_t i = 0; i < physical_path_num; i++)
        {
            if ((physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER) ||
                (physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER))
            {
                // dac setting
                if (audio_route_dac_gain_get(category, level, &dac_gain) == true)
                {
                    dac_ch = physical_path->attr.spk_attr.dac_ch;
                    codec_drv_config_init(CODEC_CONFIG_SEL_DAC, (void *)&dac_config);
                    dac_config.ana_gain = (T_CODEC_DAC_ANA_GAIN)physical_path->attr.spk_attr.ana_gain;
                    dac_config.dig_gain = physical_path->attr.spk_attr.dig_gain;
                    codec_drv_dac_config_set((T_CODEC_DAC_CHANNEL_SEL)dac_ch, &dac_config, set_immediately);
                }
            }
            physical_path++;
        }

        audio_route_physical_path_give(&pysical_path_group);
    }
}

bool codec_mgr_phy_path_enable(T_CODEC_MGR_SESSION *session)
{
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path;
    uint8_t physical_path_num;
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP pysical_path_group;
    T_AUDIO_CATEGORY category;
    T_CODEC_MGR_CONFIG *codec_config;

    category = session->config.category;
    codec_config = &(session->config);
    pysical_path_group = audio_route_physical_path_take(category);
    physical_path = pysical_path_group.physical_path;
    physical_path_num = pysical_path_group.physical_path_num;

    CODEC_PRINT_TRACE4("codec_mgr_phy_path_enable: category 0x%02x, physical_path_num %d, adc_sample_rate %d, dac_sample_rate %d",
                       category, physical_path_num, codec_config->adc_sample_rate, codec_config->dac_sample_rate);

    if (physical_path != NULL)
    {
        for (uint8_t i = 0; i < physical_path_num; i++)
        {
            if (physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_MIC)
            {
                if (physical_path->attr.mic_attr.mic_external == false)
                {
                    codec_mgr_mic_enable(physical_path, codec_config);
                    codec_mgr_db->adc_status[category].total++;
                }
            }
            else if (physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_SPEAKER)
            {
                if (physical_path->attr.spk_attr.spk_external == false)
                {
                    codec_mgr_spk_enable(physical_path, codec_config);
                }
            }
            else if (physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_AUX_IN)
            {
                if (physical_path->attr.aux_in_attr.aux_in_external == false)
                {
                    codec_mgr_aux_in_enable(physical_path, codec_config);
                }
            }
            physical_path++;
        }

        audio_route_physical_path_give(&pysical_path_group);
    }

    codec_mgr_mix_enable();
    codec_mgr_loopback_enable();

    return true;
}

bool codec_mgr_phy_path_disable(T_CODEC_MGR_SESSION *session)
{
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path;
    uint8_t physical_path_num;
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP pysical_path_group;
    T_AUDIO_CATEGORY category;
    bool ret = false;

    category = session->config.category;
    pysical_path_group = audio_route_physical_path_take(category);
    physical_path = pysical_path_group.physical_path;
    physical_path_num = pysical_path_group.physical_path_num;

    if (physical_path != NULL)
    {
        for (uint8_t i = 0; i < physical_path_num; i++)
        {
            if (physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_MIC)
            {
                codec_mgr_mic_disable(session, physical_path);
            }
            else if (physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_SPEAKER)
            {
                codec_mgr_spk_disable(session, physical_path);
            }
            else if (physical_path->physical_io_type == AUDIO_ROUTE_PHYSICAL_IO_AUX_IN)
            {
                codec_mgr_aux_in_disable(physical_path);
            }
            physical_path++;
        }

        audio_route_physical_path_give(&pysical_path_group);
        ret = true;
    }

    codec_mgr_mix_disable();
    codec_mgr_loopback_disable();

    codec_mgr_db->adc_status[category].total = 0;

    CODEC_PRINT_TRACE6("codec_mgr_phy_path_disable: category 0x%02x, physical_path_num %d, "
                       "adc0_ref_cnt %d, adc1_ref_cnt %d, "
                       "adc2_ref_cnt %d, adc3_ref_cnt %d",
                       category, physical_path_num, codec_mgr_db->adc_ref[0].count,
                       codec_mgr_db->adc_ref[1].count, codec_mgr_db->adc_ref[2].count,
                       codec_mgr_db->adc_ref[3].count);

    CODEC_PRINT_TRACE4("codec_mgr_phy_path_disable: dac0_ref_cnt_fs %d, dac0_ref_cnt_5m %d, "
                       " dac1_ref_cnt_fs %d, dac1_ref_cnt_5m %d",
                       codec_mgr_db->dac_ref[0].count_fs,
                       codec_mgr_db->dac_ref[0].count_5m,
                       codec_mgr_db->dac_ref[1].count_fs,
                       codec_mgr_db->dac_ref[1].count_5m);

    return ret;
}

void codec_mgr_bias_setting(void)
{
    T_CODEC_BIAS_CONFIG bias_config;

    codec_drv_config_init(CODEC_CONFIG_SEL_BIAS, &bias_config);
    bias_config.gpio_mode = audio_pad_check_bias_gpio_mode(sys_cfg_const.avcc_drv_voltage_sel,
                                                           sys_cfg_const.micbias_voltage_sel);

    bias_config.micbias_voltage = (T_CODEC_MICBIAS_VOL)sys_cfg_const.micbias_voltage_sel;
    codec_drv_bias_config_set(&bias_config, false);
}

T_CODEC_MGR_STATE codec_mgr_get_state(void)
{
    T_CODEC_MGR_STATE state = CODEC_MGR_STATE_POWER_OFF;

    state = (T_CODEC_MGR_STATE)codec_mgr_db->codec_fsm->state;

    return state;
}

static bool codec_mgr_eq_load(void)
{
    T_CODEC_CONFIG_HEADER *codec_cfg;
    T_HW_EQ_BLOCK *block;
    uint32_t data_offset;
    int32_t ret = 0;

    codec_cfg = os_mem_alloc2(sizeof(T_CODEC_CONFIG_HEADER));
    if (codec_cfg == NULL)
    {
        ret = 1;
        goto fail_alloc_config;
    }

    fmc_flash_nor_read(flash_dsp_cfg_addr_get() + DSP_PARAM_OFFSET,
                       codec_cfg, sizeof(T_CODEC_CONFIG_HEADER));
    if (codec_cfg->sync_word != APP_DATA_SYNC_WORD)
    {
        ret = 2;
        goto fail_load_config;
    }

    codec_mgr_db->eq_block_offset = codec_cfg->hw_eq_block_offset;
    data_offset = DSP_PARAM_OFFSET + codec_mgr_db->eq_block_offset;
    block = &codec_mgr_db->eq_block;

    fmc_flash_nor_read(flash_dsp_cfg_addr_get() + data_offset,
                       block, sizeof(T_HW_EQ_BLOCK) - sizeof(T_HW_EQ_PARAM_HEADER *));
    if (block->sync_word != APP_DATA_SYNC_WORD)
    {
        ret = 3;
        goto fail_load_block;
    }

#if F_APP_EXT_MIC_SWITCH_SUPPORT
    static T_HW_EQ_PARAM_HEADER *prev_param_header = NULL;

    if (prev_param_header == NULL)
    {
        block->param_header = calloc(block->eq_num, sizeof(T_HW_EQ_PARAM_HEADER));

        if (block->param_header != NULL)
        {
            prev_param_header = block->param_header;
        }
    }
    else
    {
        block->param_header = prev_param_header;
    }
#else
    block->param_header = calloc(block->eq_num, sizeof(T_HW_EQ_PARAM_HEADER));
#endif

    if (block->param_header == NULL)
    {
        ret = 4;
        goto fail_alloc_header;
    }

    data_offset += sizeof(T_HW_EQ_BLOCK) - sizeof(T_HW_EQ_PARAM_HEADER *);
    if (fmc_flash_nor_read(flash_dsp_cfg_addr_get() + data_offset,
                           block->param_header, block->eq_num * sizeof(T_HW_EQ_PARAM_HEADER)) == false)
    {
        ret = 5;
        goto fail_load_header;
    }

    CODEC_PRINT_TRACE2("codec_mgr_eq_load: eq_block_len %d, eq_num %d",
                       block->eq_block_len, block->eq_num);

    os_mem_free(codec_cfg);
    return true;

fail_load_header:
    os_mem_free(block->param_header);
    block->param_header = NULL;
fail_alloc_header:
fail_load_block:
fail_load_config:
    os_mem_free(codec_cfg);
fail_alloc_config:
    CODEC_PRINT_ERROR1("codec_mgr_eq_load: failed %d", -ret);
    return false;
}

bool codec_mgr_init(P_CODEC_MGR_CBACK cback)
{
    T_CODEC_FEATURE_MAP codec_feature_map;
    int32_t ret = 0;

    codec_mgr_db = os_mem_zalloc2(sizeof(T_CODEC_MGR_DB));
    if (codec_mgr_db == NULL)
    {
        ret = 1;
        goto fail_alloc_codec_mgr_db;
    }

    codec_mgr_db->operation_fsm = fsm_init(operation_fsm_trans_table, CODEC_MGR_OP_STATE_IDLE,
                                           "codec_mgr_operation_fsm", NULL);
    if (codec_mgr_db->operation_fsm == NULL)
    {
        ret = 2;
        goto fail_init_codec_mgr_operation_fsm;
    }

    codec_mgr_db->codec_fsm = fsm_init(codec_mgr_fsm_trans_table, CODEC_MGR_STATE_POWER_OFF,
                                       "codec_mgr_fsm", NULL);
    if (codec_mgr_db->codec_fsm == NULL)
    {
        ret = 3;
        goto fail_init_codec_mgr_fsm;
    }

    codec_mgr_db->cback = cback;

    os_msg_queue_create(&hCODECQueueHandleAu, "CODEC_Q", CODEC_MSG_MAX_NUM, sizeof(T_CODEC_SCHED_MSG));

    codec_feature_map.d32 = 0;
    codec_feature_map.avcc_drv_volt_sel = sys_cfg_const.avcc_drv_voltage_sel;

    if (codec_drv_init(codec_feature_map, codec_mgr_cback) == false)
    {
        ret = 4;
        goto fail_init_codec;
    }

    if (codec_mgr_eq_load() == false)
    {
        ret = 5;
        goto fail_load_eq;
    }

    os_queue_init(&(codec_mgr_db->session_queue));
    os_queue_init(&(codec_mgr_db->operation_queue));

    codec_mgr_db->loopback_chann[AUDIO_ROUTE_DAC_CHANNEL0].adc_ch = AUDIO_ROUTE_ADC_CHANNEL_NUM;
    codec_mgr_db->loopback_chann[AUDIO_ROUTE_DAC_CHANNEL1].adc_ch = AUDIO_ROUTE_ADC_CHANNEL_NUM;
    codec_mgr_db->loopback_chann[AUDIO_ROUTE_DAC_CHANNEL2].adc_ch = AUDIO_ROUTE_ADC_CHANNEL_NUM;
    codec_mgr_db->loopback_chann[AUDIO_ROUTE_DAC_CHANNEL3].adc_ch = AUDIO_ROUTE_ADC_CHANNEL_NUM;

    return true;

fail_load_eq:
    codec_drv_deinit();
fail_init_codec:
    fsm_deinit(codec_mgr_db->codec_fsm);
fail_init_codec_mgr_fsm:
    fsm_deinit(codec_mgr_db->operation_fsm);
fail_init_codec_mgr_operation_fsm:
    os_mem_free(codec_mgr_db);
    codec_mgr_db = NULL;
fail_alloc_codec_mgr_db:
    CODEC_PRINT_ERROR1("codec_mgr_init: failed %d", -ret);
    return false;
}

void codec_mgr_deinit(void)
{
    T_CODEC_MGR_SESSION *session = os_queue_out(&(codec_mgr_db->session_queue));
    while (session != NULL)
    {
        fsm_deinit(session->session_fsm);
        os_mem_free(session);
        session = os_queue_out(&(codec_mgr_db->session_queue));
    }

    fsm_deinit(codec_mgr_db->codec_fsm);
    fsm_deinit(codec_mgr_db->operation_fsm);
    os_mem_free(codec_mgr_db);
    codec_mgr_db = NULL;
}

bool codec_mgr_sidetone_set(int16_t gain, uint8_t level, uint8_t enable)
{
    uint8_t boost_gain, dig_gain;
    T_CODEC_SIDETONE_CONFIG sidetone_config;

    codec_drv_config_init(CODEC_CONFIG_SEL_SIDETONE, (void *)&sidetone_config);
    codec_drv_sidetone_gain_convert(CODEC_SIDETONE_TYPE_5M, gain, &boost_gain, &dig_gain);

    CODEC_PRINT_TRACE5("codec_mgr_sidetone_set: gain %d, level %d, "
                       "boost_gain: 0x%02x, dig_gain: 0x%02x, enable 0x%x", gain,
                       level, boost_gain, dig_gain, enable);

    if (enable)
    {
        codec_mgr_db->sidetone_status = true;

        sidetone_config.enable = 1;
        sidetone_config.dig_gain.val = dig_gain;
        sidetone_config.boost_gain = (T_CODEC_SIDETONE_BOOST_GAIN)boost_gain;
        sidetone_config.src = (T_CODEC_ADC_CHANNEL_SEL)sys_cfg_const.voice_primary_mic_adc_ch;
        sidetone_config.type = CODEC_SIDETONE_TYPE_5M;
        sidetone_config.eq_en = 0;
        sidetone_config.hpf_en = 1;
        sidetone_config.hpf_fc_sel = level;
        sidetone_config.input = CODEC_SIDETONE_INPUT_L;
        codec_drv_sidetone_config_set(SIDETONE_CHANNEL_L, &sidetone_config, true);
        codec_drv_sidetone_config_set(SIDETONE_CHANNEL_R, &sidetone_config, true);

        codec_drv_sidetone_enable();
    }
    else
    {
        codec_mgr_db->sidetone_status = false;

        sidetone_config.enable = 0;
        codec_drv_sidetone_config_set(SIDETONE_CHANNEL_L, &sidetone_config, true);
        codec_drv_sidetone_config_set(SIDETONE_CHANNEL_R, &sidetone_config, true);

        codec_drv_sidetone_disable();
    }

    return true;
}

void codec_mgr_sidetone_gain_set(int16_t gain)
{
    uint8_t boost_gain, dig_gain;
    T_CODEC_SIDETONE_CONFIG sidetone_config;

    codec_drv_config_init(CODEC_CONFIG_SEL_SIDETONE, (void *)&sidetone_config);
    codec_drv_sidetone_gain_convert(CODEC_SIDETONE_TYPE_5M, gain, &boost_gain, &dig_gain);

    CODEC_PRINT_TRACE3("codec_mgr_sidetone_gain_set: gain %d,"
                       "boost_gain: 0x%02x, dig_gain: 0x%02x, enable 0x%x", gain,
                       boost_gain, dig_gain);

    if (codec_mgr_db->sidetone_status)
    {
        sidetone_config.dig_gain.val = dig_gain;
        sidetone_config.boost_gain = (T_CODEC_SIDETONE_BOOST_GAIN)boost_gain;
        codec_drv_sidetone_config_set(SIDETONE_CHANNEL_L, &sidetone_config, true);
        codec_drv_sidetone_config_set(SIDETONE_CHANNEL_R, &sidetone_config, true);
        codec_drv_sidetone_enable();
    }
}

static int8_t codec_mgr_session_cfg_set(void *context)
{
    T_CODEC_MGR_SESSION *session = (T_CODEC_MGR_SESSION *)context;
    uint8_t adc_gain_level = session->config.adc_gain_level;
    uint8_t dac_gain_level = session->config.dac_gain_level;

    codec_mgr_bias_setting();
    codec_mgr_phy_path_enable(session);
    codec_mgr_adc_gain_set(session, adc_gain_level);
    codec_mgr_dac_gain_set(session, dac_gain_level);
    return CODEC_MGR_SESSION_STATE_ENABLING;
}

static int8_t codec_mgr_session_cfg_clear(void *context)
{
    T_CODEC_MGR_SESSION *session = (T_CODEC_MGR_SESSION *)context;

    codec_mgr_phy_path_disable(session);

    return CODEC_MGR_SESSION_STATE_DISABLING;
}
