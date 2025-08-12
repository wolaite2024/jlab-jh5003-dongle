/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "dsp_shm.h"
#include "dsp_ipc.h"
#include "audio_probe.h"
#include "codec_driver.h"
#include "audio_mgr.h"
#include "dsp_mgr.h"
#include "audio_pad.h"
#include "dsp_driver.h"

/* TODO Remove Start */
#include "sys_cfg.h"
#include "sys_mgr.h"
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-801*/
#include "errno.h"
#include "os_msg.h"
#include "os_sync.h"
#include "app_msg.h"
#endif
#include "dsp_loader.h"

static uint8_t  customer_chann_info = 0;
static uint8_t  cfg_chann_info = 0;
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-801*/
extern void *hEventQueueHandleAu;
#endif
/* TODO Remove End */

#define AUDIO_PROBE_DSP_SDK_DATA_H2D_CMD_LEN    512

static const T_AUDIO_PROBE_EVENT probe_map[] =
{
    PROBE_SCENARIO_STATE,
    PROBE_EAR_FIT_RESULT,
    PROBE_SDK_GENERAL_CMD,
    PROBE_SDK_BOOT_DONE,
    PROBE_HA_VER_INFO,
    PROBE_SEG_SEND_REQ_DATA,
    PROBE_SEG_SEND_ERROR,
    PROBE_SYNC_REF_REQUEST,
};

typedef struct t_audio_probe_cback_item
{
    struct t_audio_probe_cback_item     *p_next;
    P_AUDIO_PROBE_DSP_CABCK              cback;
} T_AUDIO_PROBE_CBACK_ITEM;

typedef struct t_audio_probe_dsp_evt_cback_item
{
    struct t_audio_probe_dsp_evt_cback_item     *p_next;
    P_AUDIO_PROBE_DSP_EVENT_CABCK               cback;
} T_AUDIO_PROBE_DSP_EVT_CBACK_ITEM;

typedef struct t_audio_prob_db
{
    bool register_ipc_handler;
    T_SYS_IPC_HANDLE            dsp_event;
    T_OS_QUEUE cback_list;
    T_OS_QUEUE dsp_event_cback_list;
} T_AODIO_PROBE_DB;

#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-801*/
#define ENGINE_Q_MAX_NUM    0x30
typedef enum
{
    ENGINE_EVT_START,
    ENGINE_EVT_STOP,
} T_ENGINE_EVT;

typedef struct t_engine_db
{
    void     *msg_queue;
    void     *sem;
} T_ENGINE_DB;

typedef struct
{
    uint8_t     msg_type;
    uint8_t     msg_sub_type;
    uint16_t    data_len;
    void        *p_data;
} T_ENGINE_MSG;

static T_ENGINE_DB engine_db  = {.msg_queue = NULL, .sem = NULL};
#endif

T_AODIO_PROBE_DB audio_prob_db =
{
    .register_ipc_handler = false,
    .dsp_event            = NULL,
};

static bool audio_probe_dsp_ipc_cback(T_DSP_IPC_EVENT event, uint32_t param)
{
    T_AUDIO_PROBE_CBACK_ITEM *elem;
    uint8_t i;

    switch (event)
    {
    case DSP_IPC_EVT_PROBE:
        {
            i = PROBE_SCENARIO_STATE;
        }
        break;

    case DSP_IPC_EVT_PROBE_EARFIT:
        {
            i = PROBE_EAR_FIT_RESULT;
        }
        break;

    case DSP_IPC_EVT_PROBE_SDK_GEN_CMD:
        {
            i = PROBE_SDK_GENERAL_CMD;
        }
        break;

    case DSP_IPC_EVT_PROBE_SDK_BOOT:
        {
            i = PROBE_SDK_BOOT_DONE;
        }
        break;

    case DSP_IPC_EVT_HA_VER_INFO:
        {
            i = PROBE_HA_VER_INFO;
        }
        break;

    case DSP_IPC_EVT_SEG_SEND_REQ_DATA:
        {
            i = PROBE_SEG_SEND_REQ_DATA;
        }
        break;

    case DSP_IPC_EVT_SEG_SEND_ERROR:
        {
            i = PROBE_SEG_SEND_ERROR;
        }
        break;

    case DSP_IPC_EVT_PROBE_SYNC_REF_REQUEST:
        {
            i = PROBE_SYNC_REF_REQUEST;
        }
        break;

    default:
        {
            return false;
        }
    }

    elem = os_queue_peek(&audio_prob_db.cback_list, 0);

    while (elem != NULL)
    {
        elem->cback(probe_map[i], (void *)param);
        elem = elem->p_next;
    }
    return true;
}

bool audio_probe_dsp_cback_register(P_AUDIO_PROBE_DSP_CABCK cback)
{
    if (!audio_prob_db.register_ipc_handler &&
        dsp_ipc_cback_register(audio_probe_dsp_ipc_cback) == true)
    {
        audio_prob_db.register_ipc_handler = true;
    }
    if (!audio_prob_db.register_ipc_handler)
    {
        return false;
    }
    T_AUDIO_PROBE_CBACK_ITEM *elem;

    elem = os_mem_zalloc2(sizeof(T_AUDIO_PROBE_CBACK_ITEM));
    if (elem != NULL)
    {
        elem->cback = cback;
        os_queue_in(&audio_prob_db.cback_list, elem);
        return true;
    }

    return false;
}

bool audio_probe_dsp_cback_unregister(P_AUDIO_PROBE_DSP_CABCK cback)
{
    bool ret = false;
    T_AUDIO_PROBE_CBACK_ITEM *elem;

    elem = os_queue_peek(&audio_prob_db.cback_list, 0);
    while (elem != NULL)
    {
        if (elem->cback == cback)
        {
            ret = os_queue_delete(&audio_prob_db.cback_list, elem);
            break;
        }

        elem = elem->p_next;
    }
    /*When remove all elems of audio_prob_db.cback_list,
    *should unregister interface of dsp ipc.
    */
    if (audio_prob_db.register_ipc_handler &&
        (audio_prob_db.cback_list.count == 0) &&
        dsp_ipc_cback_unregister(audio_probe_dsp_ipc_cback))
    {
        audio_prob_db.register_ipc_handler = false;
    }
    return ret;
}

bool audio_probe_dsp_send(uint8_t *buf, uint16_t len)
{
    uint16_t cmd_id = *(uint16_t *)buf;

    AUDIO_PRINT_TRACE2("audio_probe_dsp_send: buf %p, len 0x%04x",
                       buf, len);

    if ((cmd_id != H2D_ADC_GAIN) &&
        (cmd_id != H2D_APT_DAC_GAIN))
    {
        return h2d_cmd_send(buf, len, true);
    }

    return true;
}

bool audio_probe_codec_hw_eq_set(uint8_t eq_type, uint8_t eq_chann, uint8_t *buf, uint16_t len)
{
    return codec_drv_eq_data_set(eq_type, eq_chann, buf, len);
}

void audio_probe_set_voice_primary_mic(uint8_t mic_sel, uint8_t mic_type)
{
    sys_cfg_const.voice_primary_mic_sel = mic_sel;
    sys_cfg_const.voice_primary_mic_type = mic_type;
}

void auido_probe_set_voice_secondary_mic(uint8_t mic_sel, uint8_t mic_type)
{
    sys_cfg_const.voice_secondary_mic_sel = mic_sel;
    sys_cfg_const.voice_secondary_mic_type = mic_type;
}

uint8_t audio_probe_get_voice_primary_mic_sel(void)
{
    return sys_cfg_const.voice_primary_mic_sel;
}

uint8_t audio_probe_get_voice_primary_mic_type(void)
{
    return sys_cfg_const.voice_primary_mic_type;
}

uint8_t audio_probe_get_voice_secondary_mic_sel(void)
{
    return sys_cfg_const.voice_secondary_mic_sel;
}

uint8_t audio_probe_get_voice_secondary_mic_type(void)
{
    return sys_cfg_const.voice_secondary_mic_type;
}


bool audio_probe_dsp_event_cback(uint32_t event, void *msg)
{
    T_AUDIO_PROBE_DSP_EVT_CBACK_ITEM *elem;

    elem = os_queue_peek(&audio_prob_db.dsp_event_cback_list, 0);

    while (elem != NULL)
    {
        elem->cback(event, msg);
        elem = elem->p_next;
    }
    return true;
}

bool audio_probe_dsp_evt_cback_register(P_AUDIO_PROBE_DSP_EVENT_CABCK cback)
{
    if (audio_prob_db.dsp_event == NULL)
    {
        audio_prob_db.dsp_event = dsp_mgr_register_cback(audio_probe_dsp_event_cback);
    }

    T_AUDIO_PROBE_DSP_EVT_CBACK_ITEM *elem;
    elem = os_mem_zalloc2(sizeof(T_AUDIO_PROBE_DSP_EVT_CBACK_ITEM));
    if (elem != NULL)
    {
        elem->cback = cback;
        os_queue_in(&audio_prob_db.dsp_event_cback_list, elem);
        return true;
    }
    return false;
}

bool audio_probe_dsp_evt_cback_unregister(P_AUDIO_PROBE_DSP_EVENT_CABCK cback)
{
    bool ret = false;
    T_AUDIO_PROBE_DSP_EVT_CBACK_ITEM *elem;

    elem = os_queue_peek(&audio_prob_db.dsp_event_cback_list, 0);
    while (elem != NULL)
    {
        if (elem->cback == cback)
        {
            ret = os_queue_delete(&audio_prob_db.dsp_event_cback_list, elem);
            break;
        }

        elem = elem->p_next;
    }
    /*When remove all elems of audio_prob_db.dsp_event_cback_list,
    *should unregister interface of dsp evt.
    */
    if ((audio_prob_db.dsp_event != NULL) &&
        (audio_prob_db.dsp_event_cback_list.count == 0))
    {
        dsp_mgr_unregister_cback(audio_prob_db.dsp_event);
        audio_prob_db.dsp_event = NULL;
    }
    return ret;
}

bool audio_probe_send_dsp_sdk_data(uint8_t *p_data, uint16_t data_len)
{
    if (data_len <= AUDIO_PROBE_DSP_SDK_DATA_H2D_CMD_LEN)
    {
        return dsp_ipc_send_h2d_param(H2D_SCENARIO0_PARA_PARSING, p_data, data_len);
    }
    else
    {
        CODEC_PRINT_TRACE0("audio_probe_send_dsp_sdk_data: NO send");
        return false;
    }
}

bool audio_probe_send_lhdc_license(uint8_t *p_data, uint16_t data_len)
{
    if (data_len <= AUDIO_PROBE_DSP_SDK_DATA_H2D_CMD_LEN)
    {
        return dsp_ipc_send_h2d_param(H2D_LHDC_LICENSE_PARA, p_data, data_len);
    }
    else
    {
        CODEC_PRINT_TRACE0("audio_probe_send_dsp_sdk_data: NO send");
        return false;
    }
}

bool audio_probe_send_malleus_license(uint8_t *p_data, uint16_t data_len)
{
    if (data_len <= AUDIO_PROBE_DSP_SDK_DATA_H2D_CMD_LEN)
    {
        return dsp_ipc_send_h2d_param(H2D_MALLEUS_LICENSE_PARA, p_data, data_len);
    }
    else
    {
        CODEC_PRINT_TRACE0("audio_probe_send_dsp_sdk_data: NO send");
        return false;
    }
}

bool audio_probe_dsp_ipc_send_ha_param(uint8_t *payload_data, uint16_t payload_len)
{
    return dsp_ipc_send_h2d_param(H2D_HA_PARA, payload_data, payload_len);
}

void *audio_probe_media_buffer_malloc(uint16_t buf_size)
{
    return media_buffer_get(audio_db->playback_pool_handle, buf_size);
}

bool audio_probe_media_buffer_free(void *p_buf)
{
    return media_buffer_put(audio_db->playback_pool_handle, p_buf);
}

bool audio_probe_dsp_ipc_send_call_status(bool enable)
{
    uint32_t cmd_length = 0x01;
    uint32_t cmd_buf_len = 0;
    uint8_t cmd_buf[8] = {0};

    cmd_buf_len = ((cmd_length + 1) * 4);

    cmd_buf[0] = (uint8_t)(H2D_CALL_STATUS);
    cmd_buf[1] = (uint8_t)(H2D_CALL_STATUS >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (enable == true) ? 1 : 0;

    return h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

#define H2D_CUSTOMER_INFO_CONFIG_SIZE   1
bool dsp_ipc_send_sepc_channel_info(uint8_t param, bool flush)
{
    uint8_t cmd_buf[8];
    uint32_t cmd_buf_len = 0;
    DIPC_PRINT_ERROR1("dsp_ipc_send_sepc_channel_info: param %d", param);
    cmd_buf_len = ((H2D_CUSTOMER_INFO_CONFIG_SIZE + 1) * 4);
    memset(cmd_buf, 0, cmd_buf_len);
    cmd_buf[0] = (uint8_t)H2D_CUSTOMER_INFO_CONFIG;
    cmd_buf[1] = (uint8_t)(H2D_CUSTOMER_INFO_CONFIG >> 8);
    cmd_buf[2] = (uint8_t)(H2D_CUSTOMER_INFO_CONFIG_SIZE);
    cmd_buf[3] = (uint8_t)(H2D_CUSTOMER_INFO_CONFIG_SIZE >> 8);

    cmd_buf[4] = (uint8_t)param;
    cmd_buf[5] = 0;
    cmd_buf[6] = 0;
    cmd_buf[7] = 0;
    return h2d_cmd_send(cmd_buf, cmd_buf_len, flush);

}

bool dsp_ipc_send_sepc_info(uint8_t param, bool action, uint8_t para_chanel_out)
{
    customer_chann_info = param;
    if (action)
    {
        return dsp_ipc_send_sepc_channel_info(customer_chann_info, true);
    }
    else
    {
        cfg_chann_info = para_chanel_out;
    }
    return true;
}

bool audio_probe_send_sepc_info(uint8_t parameter, bool action, uint8_t para_chanel_out)
{
    return dsp_ipc_send_sepc_info(parameter, action, para_chanel_out);
}

void audio_probe_set_sepc_info(void)
{
    audio_volume_out_channel_set((T_AUDIO_VOLUME_CHANNEL_MASK)cfg_chann_info);
    dsp_ipc_send_sepc_channel_info(customer_chann_info, false);
}

void audio_probe_dsp_test_bin_set(bool enable)
{
    dsp_load_set_test_bin(enable);
}

void audio_probe_disable_dsp_powerdown(void)
{
    audio_path_lpm_set(false);
}

void audio_probe_codec_bias_mode_set(uint8_t mode)
{
    if ((mode == AUDIO_PROBE_CODEC_MGR_BIAS_ALWAYS_ON_MODE) ||
        (mode == AUDIO_PROBE_CODEC_MGR_BIAS_USER_MODE))
    {
        audio_path_lpm_set(false);
    }
    else
    {
        audio_path_lpm_set(true);
    }

    codec_drv_bias_mode_set(mode);
}

void audio_probe_codec_bias_pad_set(bool enable)
{
    audio_pad_bias_set(enable);
}

void audio_probe_codec_adda_loopback_set(uint8_t mic_sel)
{
    dsp_hal_codec_open();
    codec_drv_adda_loopback_set((T_CODEC_AMIC_CHANNEL_SEL) mic_sel);
}

#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-801*/
void engine_msg_handler(void)
{
    T_ENGINE_MSG msg;

    if (os_msg_recv(engine_db.msg_queue, &msg, 0) == true)
    {
        AUDIO_PRINT_INFO1("engine_msg_handler, evt:%d", msg.msg_type);
        switch (msg.msg_type)
        {
        case ENGINE_EVT_START:
            {
                bool *success = msg.p_data;
                if (dsp_mgr_power_on_check())
                {
                    dsp_mgr_dsp2_ref_increment();
                    *success = true;
                }
                else
                {
                    *success = false;
                }
                os_sem_give(engine_db.sem);
            }
            break;

        case ENGINE_EVT_STOP:
            {
                bool *success = msg.p_data;

                dsp_mgr_dsp2_ref_decrement();
                if (dsp_mgr_get_state() == DSP_STATE_IDLE)
                {
                    audio_path_power_off();
                }
                *success = true;
                os_sem_give(engine_db.sem);

            }
            break;

        default:
            break;
        }
    }
}

uint32_t engine_start(void)
{
    T_ENGINE_MSG msg = {.msg_type = ENGINE_EVT_START};
    bool success = false;
    uint8_t evt = EVENT_ENGINE_MSG;
    uint32_t instance = 0;

    if (engine_db.msg_queue == NULL)
    {
        if (os_msg_queue_create(&engine_db.msg_queue, "engineQ",
                                ENGINE_Q_MAX_NUM, sizeof(T_ENGINE_MSG)) == false)
        {
            return 0;
        }

        sys_mgr_event_register(EVENT_ENGINE_MSG, engine_msg_handler);
    }

    if (engine_db.sem == NULL)
    {
        if (os_sem_create(&engine_db.sem, "engine semaphore", 0, 1) == false)
        {
            return 0;
        }
    }

    msg.p_data = &success;
    msg.data_len = sizeof(&success);
    if (os_msg_send(engine_db.msg_queue, &msg, 0))
    {
        if (os_msg_send(hEventQueueHandleAu, &evt, 0))
        {
            os_sem_take(engine_db.sem, 2000);
        }
    }

    if (success)
    {
        instance = dsp_mgr_dsp2_ref_get();
    }
    AUDIO_PRINT_INFO2("engine_start:%d-%d", instance, success);
    return instance;
}

int32_t engine_stop(uint32_t instance)
{
    T_ENGINE_MSG msg = {.msg_type = ENGINE_EVT_STOP};
    bool success = false;
    uint8_t evt = EVENT_ENGINE_MSG;

    if (dsp_mgr_dsp2_ref_get() == 0)
    {
        AUDIO_PRINT_ERROR0("engine_stop failed:engine has not started");
        return -EPERM;
    }

    msg.p_data = &success;
    msg.data_len = sizeof(&success);
    if (os_msg_send(engine_db.msg_queue, &msg, 0))
    {
        if (os_msg_send(hEventQueueHandleAu, &evt, 0))
        {
            os_sem_take(engine_db.sem, 2000);
        }
    }

    AUDIO_PRINT_INFO2("engine_stop:%d-%d", dsp_mgr_dsp2_ref_get(), success);
    if (!success)
    {
        return -EPERM;
    }

    return 0;
}
#endif
