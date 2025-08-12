/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include "trace.h"
#include "audio_track.h"
#include "app_audio_track.h"
#include "audio.h"
#include "app_cfg.h"
#include "audio_route.h"
#include "app_dsp_cfg.h"

typedef struct audio_track_device
{
    struct audio_track_device *next;
    /* Upper layer identification. */
    uint8_t                 uid;
    uint16_t                frame_len;
    uint32_t                device;
    T_AUDIO_TRACK_STATE     state;
    uint32_t                seq_num;
    void (*mgr_cback)(uint8_t id, uint8_t event, uint8_t *buf, uint32_t len);
    T_AUDIO_TRACK_HANDLE   handle;
} T_AUDIO_TRACK_DEVICE;

typedef struct
{
    uint8_t track_num;
    uint8_t enable;
    T_AUDIO_TRACK_DEVICE *p_head;
} T_APP_AUDIO_TRACK_MGR;

T_APP_AUDIO_TRACK_MGR g_app_audio_track;

static T_AUDIO_TRACK_DEVICE *get_app_audio_device_handle(uint32_t device)
{
    T_AUDIO_TRACK_DEVICE *p_device = g_app_audio_track.p_head;

    while (p_device)
    {
        if (p_device->device == device)
        {
            return p_device;
        }
        p_device = p_device->next;
    }

    return NULL;
}

static T_AUDIO_TRACK_DEVICE *get_device_by_handle(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK_DEVICE *p_device = g_app_audio_track.p_head;

    while (p_device)
    {
        if (p_device->handle == handle)
        {
            return p_device;
        }
        p_device = p_device->next;
    }

    return NULL;
}

static bool track_list_add(T_AUDIO_TRACK_DEVICE *p_device)
{
    if (p_device == NULL)
    {
        return false;
    }

    p_device->next = NULL;

    T_AUDIO_TRACK_DEVICE *p_node = g_app_audio_track.p_head;
    T_AUDIO_TRACK_DEVICE *p_prev = g_app_audio_track.p_head;

    while (p_node)
    {
        p_prev = p_node;
        p_node = p_node->next;
    }

    if (p_prev == NULL)     //head is null
    {
        g_app_audio_track.p_head = p_device;
    }
    else
    {
        p_prev->next = p_device;
    }
    return true;
}

static bool track_list_del(T_AUDIO_TRACK_DEVICE *p_device)
{
    if (p_device == NULL)
    {
        return false;
    }
    T_AUDIO_TRACK_DEVICE *p_prev = g_app_audio_track.p_head;
    T_AUDIO_TRACK_DEVICE *p_node = g_app_audio_track.p_head;

    while (p_node)
    {
        if (p_node != p_device)
        {
            p_prev = p_node;
            p_node = p_node->next;
        }
        else
        {
            if (p_node == p_prev)   // head
            {
                g_app_audio_track.p_head = p_node->next;
            }
            else
            {
                p_prev->next = p_node->next;
            }
            free(p_node);
            return true;
        }
    }
    return false;


}

bool app_audio_track_create(uint32_t device, T_AUDIO_FORMAT_INFO *info,
                            void (*mgr_cback)(uint8_t id, uint8_t event, uint8_t *buf, uint32_t len),
                            uint8_t uid)
{
    T_AUDIO_STREAM_TYPE    stream_type = AUDIO_STREAM_TYPE_RECORD;
    T_AUDIO_STREAM_MODE    mode;
    T_AUDIO_STREAM_USAGE   usage;

    uint8_t                volume_out;
    uint8_t                volume_in;

    if (device & 0xFFFF) //out device
    {
        stream_type = AUDIO_STREAM_TYPE_PLAYBACK;
    }
    else    // in device
    {
        stream_type = AUDIO_STREAM_TYPE_RECORD;
    }
    T_AUDIO_TRACK_DEVICE *p_device = NULL;

    p_device = get_app_audio_device_handle(device);
    if (p_device == NULL)
    {
        p_device = calloc(1, sizeof(T_AUDIO_TRACK_DEVICE));
        if (p_device == NULL)
        {
            APP_PRINT_ERROR0("app_audio_track_create calloc failed!");
            return false;
        }
        p_device->device = device;
        track_list_add(p_device);
    }

    p_device->mgr_cback = mgr_cback;
    p_device->uid = uid;


    usage = AUDIO_STREAM_USAGE_LOCAL;
    mode = AUDIO_STREAM_MODE_NORMAL;

    volume_out = app_dsp_cfg_vol.playback_volume_default;
    volume_in = app_dsp_cfg_vol.record_volume_default;

    APP_PRINT_INFO3("app_audio_track_create type %x vol_out %x vol_in %x", info->type, volume_out,
                    volume_in);
    if (info->type == AUDIO_FORMAT_TYPE_LC3)
    {
        APP_PRINT_INFO4("app_audio_track_create  sr %d, chann_location %x, len 0x%x, duration 0x%x",
                        info->attr.lc3.sample_rate,
                        info->attr.lc3.chann_location,
                        info->attr.lc3.frame_length,
                        info->attr.lc3.frame_duration);
        if ((stream_type == AUDIO_STREAM_TYPE_PLAYBACK) || (stream_type == AUDIO_STREAM_TYPE_RECORD))
        {
            mode = AUDIO_STREAM_MODE_DIRECT;
        }
    }

    APP_PRINT_INFO2("app_audio_track_create stream_type 0x%x mode 0x%x", stream_type, mode);
    T_AUDIO_TRACK_HANDLE p_track = audio_track_create(stream_type,
                                                      mode,
                                                      usage,
                                                      *info,
                                                      volume_out,
                                                      volume_in,
                                                      device,
                                                      NULL,
                                                      NULL);
    if (!p_track)
    {
        APP_PRINT_ERROR0("app_audio_track_create create failed!");
        return false;
    }
    //p_device->handle = p_track;

    return true;
}

void app_audio_track_release(uint32_t device)
{
    T_AUDIO_TRACK_DEVICE *p_device = get_app_audio_device_handle(device);
    if (p_device == NULL)
    {
        return;
    }

    audio_track_release(p_device->handle);
}

bool app_audio_track_change_uid(uint32_t device, uint8_t uid)
{
    T_AUDIO_TRACK_DEVICE *p_device = NULL;

    p_device = get_app_audio_device_handle(device);
    if (p_device)
    {
        p_device->uid = uid;
        return true;
    }

    return false;
}

bool app_audio_track_write_with_flag(uint32_t device, uint8_t *buf, uint8_t flag, uint16_t len,
                                     uint16_t frame_num, uint32_t timestamp)
{
    if ((buf == NULL) || (!len))
    {
        return false;
    }

    T_AUDIO_TRACK_DEVICE *p_device = NULL;
    bool retval;
    //uint16_t frame_num;
    uint16_t written_len;

    p_device = get_app_audio_device_handle(device);
    if (p_device == NULL)
    {
        APP_PRINT_ERROR1("app_audio_track_write_with_flag device %x, handle NULL", device);
        return false;
    }

    p_device->seq_num++;

    retval = audio_track_write(p_device->handle, timestamp, p_device->seq_num,
                               flag == 0 ? AUDIO_STREAM_STATUS_CORRECT : AUDIO_STREAM_STATUS_LOST,
                               frame_num, buf, len,
                               &written_len);
    APP_PRINT_INFO4("audio_track_write retval %d, flag %d, written_len %d, timestamp 0x%x", retval,
                    flag, written_len,
                    timestamp);
    return retval;
}

static void track_state_changed_proc(void *event_buf, uint16_t buf_len)
{
    T_AUDIO_TRACK_DEVICE *p_device = NULL;
    T_AUDIO_EVENT_PARAM_TRACK_STATE_CHANGED *p_state = event_buf;
    uint32_t device;
    void (*mgr_cback)(uint8_t id, uint8_t event, uint8_t *buf, uint32_t len) = NULL;
    uint8_t uid;

    if (audio_track_device_get(p_state->handle, &device) == false)
    {
        APP_PRINT_ERROR0("track_state_changed_proc, can not get device");
        p_device = get_device_by_handle(p_state->handle);
    }
    else
    {
        APP_PRINT_INFO1("get_app_audio_device_handle device %d", device);
        p_device = get_app_audio_device_handle(device);
    }

    if (p_device == NULL)
    {
        APP_PRINT_ERROR0("get_app_audio_device_handle, can not get device handle");
        return;
    }

    APP_PRINT_INFO1("track_state_changed_proc state %d", p_state->state);
    uid = p_device->uid;
    mgr_cback = p_device->mgr_cback;
    switch (p_state->state)
    {
    case AUDIO_TRACK_STATE_RELEASED:
        track_list_del(p_device);
        if (mgr_cback)
        {
            mgr_cback(uid, APP_AUDIO_TRACK_EVENT_RELEASED, NULL, 0);
        }
        break;
    case AUDIO_TRACK_STATE_CREATED:
        APP_PRINT_INFO1("AUDIO_TRACK_STATE_CREATED handle %x", p_state->handle);
        p_device->handle = p_state->handle;
        audio_track_latency_set(p_state->handle, A2DP_LATENCY_MS, true);
        audio_track_start(p_state->handle);
        if (mgr_cback)
        {
            mgr_cback(uid, APP_AUDIO_TRACK_EVENT_CREATED, NULL, 0);
        }
        break;
    case AUDIO_TRACK_STATE_STARTED:
        p_device->seq_num = 0;
        if (mgr_cback)
        {
            mgr_cback(uid, APP_AUDIO_TRACK_EVENT_STARTED, NULL, 0);
        }
        break;
    case AUDIO_TRACK_STATE_STOPPED:
        if (mgr_cback)
        {
            mgr_cback(uid, APP_AUDIO_TRACK_EVENT_STOPPED, NULL, 0);
        }
        break;
    case AUDIO_TRACK_STATE_PAUSED:
        break;
    case AUDIO_TRACK_STATE_RESTARTED:
        break;
    default:
        break;
    }


}

static void track_data_ind_proc(void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM_TRACK_DATA_IND *p_track_data_ind = event_buf;
    uint8_t *buf;
    uint32_t timestamp;
    uint16_t seq_num;
    uint8_t frame_num;
    uint16_t read_len;


    T_AUDIO_TRACK_DEVICE *p_device = get_device_by_handle(p_track_data_ind->handle);
    if (p_device == NULL)
    {
        APP_PRINT_ERROR0("track_data_ind_proc get device_by_handle failed!");
        return;
    }
    if (p_device->mgr_cback)
    {
        T_AUDIO_STREAM_STATUS status;

        buf = calloc(1, p_track_data_ind->len + 1);
        if (buf == NULL)
        {
            APP_PRINT_ERROR0("track_data_ind_proc calloc failed!");
            return;
        }

        if (audio_track_read(p_track_data_ind->handle,
                             &timestamp,
                             &seq_num,
                             &status,
                             &frame_num,
                             buf + 1,
                             p_track_data_ind->len,
                             &read_len) == true)
        {
            APP_PRINT_TRACE2("track_data_ind_proc p_track_data_ind length %d, read_len %d",
                             p_track_data_ind->len, read_len);
            buf[0] = frame_num;
            p_device->mgr_cback(p_device->uid, APP_AUDIO_TRACK_EVENT_DATA_IND,
                                buf, read_len + 1);
        }

        free(buf);
    }

}

static void track_buffer_low_proc(void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM_TRACK_BUFFER_LOW *p_track_low = event_buf;

    T_AUDIO_TRACK_DEVICE *p_device = get_device_by_handle(p_track_low->handle);
    if (!p_device)
    {
        APP_PRINT_ERROR1("track_buffer_low_proc get_device_by_handle NULL handle %x", p_track_low->handle);
        return;
    }

    if (p_device->mgr_cback)
    {
        p_device->mgr_cback(p_device->uid, APP_AUDIO_TRACK_EVENT_DATA_FILLED,
                            NULL, 0);
    }
}

static void app_audio_track_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    APP_PRINT_INFO1("app_audio_track_cback event %x", event_type);
    T_AUDIO_TRACK_DEVICE *p_device = NULL;
    T_AUDIO_EVENT_PARAM *p_param = (T_AUDIO_EVENT_PARAM *)event_buf;
    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        track_state_changed_proc(event_buf, buf_len);
        break;
    case AUDIO_EVENT_TRACK_DATA_IND:
        track_data_ind_proc(event_buf, buf_len);
        break;
    case AUDIO_EVENT_TRACK_BUFFER_LOW:
        track_buffer_low_proc(event_buf, buf_len);
        break;

    case AUDIO_EVENT_TRACK_VOLUME_OUT_CHANGED:
        {
            p_device = get_device_by_handle(p_param->track_volume_out_changed.handle);
            if (p_device != NULL)
            {
                if (p_device->mgr_cback)
                {
                    p_device->mgr_cback(p_device->uid, APP_AUDIO_TRACK_EVENT_VOL_CHG,
                                        (uint8_t *)&p_param->track_volume_out_changed.curr_volume, 1);
                }
            }
        }
        break;
    case AUDIO_EVENT_TRACK_VOLUME_OUT_MUTED:
        {
            p_device = get_device_by_handle(p_param->track_volume_out_muted.handle);
            if (p_device != NULL)
            {
                if (p_device->mgr_cback)
                {
                    p_device->mgr_cback(p_device->uid, APP_AUDIO_TRACK_EVENT_MUTED, (uint8_t *)p_device, 4);
                }
            }
        }
        break;
    case AUDIO_EVENT_TRACK_VOLUME_OUT_UNMUTED:
        {
            p_device = get_device_by_handle(p_param->track_volume_out_unmuted.handle);
            if (p_device != NULL)
            {
                if (p_device->mgr_cback)
                {
                    p_device->mgr_cback(p_device->uid, APP_AUDIO_TRACK_EVENT_UNMUTED, (uint8_t *)p_device, 4);
                }
            }
        }
        break;

    case AUDIO_EVENT_TRACK_VOLUME_IN_CHANGED:
        {
            p_device = get_device_by_handle(p_param->track_volume_in_changed.handle);
            if (p_device != NULL)
            {
                if (p_device->mgr_cback)
                {
                    p_device->mgr_cback(p_device->uid, APP_AUDIO_TRACK_EVENT_VOL_CHG,
                                        (uint8_t *)&p_param->track_volume_in_changed.curr_volume, 1);
                }
            }
        }
        break;
    case AUDIO_EVENT_TRACK_VOLUME_IN_MUTED:
        {
            p_device = get_device_by_handle(p_param->track_volume_in_muted.handle);
            if (p_device != NULL)
            {
                if (p_device->mgr_cback)
                {
                    p_device->mgr_cback(p_device->uid, APP_AUDIO_TRACK_EVENT_MUTED, (uint8_t *)p_device, 4);
                }
            }
        }
        break;
    case AUDIO_EVENT_TRACK_VOLUME_IN_UNMUTED:
        {
            p_device = get_device_by_handle(p_param->track_volume_in_unmuted.handle);
            if (p_device != NULL)
            {
                if (p_device->mgr_cback)
                {
                    p_device->mgr_cback(p_device->uid, APP_AUDIO_TRACK_EVENT_UNMUTED, (uint8_t *)p_device, 4);
                }
            }
        }
        break;

    default:
        break;
    }
}

bool app_audio_track_out_volume_change(uint32_t device, uint8_t value)
{
    T_AUDIO_TRACK_DEVICE *p_device = NULL;
    p_device = get_app_audio_device_handle(device);
    if (p_device == NULL)
    {
        return false;
    }
    return audio_track_volume_out_set(p_device->handle, value);
}

bool app_audio_track_out_volume_mute(uint32_t device, bool enable)
{
    T_AUDIO_TRACK_DEVICE *p_device = NULL;
    p_device = get_app_audio_device_handle(device);
    if (p_device == NULL)
    {
        return false;
    }
    if (enable)
    {
        return audio_track_volume_out_mute(p_device->handle);
    }
    else
    {
        return audio_track_volume_out_unmute(p_device->handle);
    }
}

bool app_audio_track_in_volume_up_down(uint32_t device, bool vol_up)
{
    T_AUDIO_TRACK_DEVICE *p_device = NULL;
    p_device = get_app_audio_device_handle(device);
    uint8_t gain_level;

    if (p_device == NULL)
    {
        APP_PRINT_ERROR0("app_audio_track_in_volume_up_down cannot find device");
        return false;
    }

    if (audio_track_volume_in_get(p_device->handle, &gain_level))
    {
        APP_PRINT_INFO1("app_audio_track_in_volume_up_down 0x%x", gain_level);
        if (vol_up)
        {
            audio_track_volume_in_set(p_device->handle, gain_level + 1);
        }
        else
        {
            audio_track_volume_in_set(p_device->handle, gain_level - 1);
        }
        return true;
    }
    else
    {
        APP_PRINT_ERROR0("app_audio_track_in_volume_change gain_level get failed");
        return false;
    }
}

bool app_audio_track_in_volume_mute(uint32_t device, bool enable)
{
    T_AUDIO_TRACK_DEVICE *p_device = NULL;
    p_device = get_app_audio_device_handle(device);
    if (p_device == NULL)
    {
        APP_PRINT_ERROR0("app_audio_track_in_mute can't find device");
        return false;
    }
    if (enable)
    {
        return audio_track_volume_in_mute(p_device->handle);
    }
    else
    {
        return audio_track_volume_in_unmute(p_device->handle);
    }
}

#if 0
const uint16_t app_audio_dac_gain_table[] =
{
    0x8001, 0xf280, 0xf380, 0xf480, 0xf580, 0xf680, 0xf780, 0xf880,
    0xf980, 0xfa80, 0xfb80, 0xfc80, 0xfd80, 0xfe80, 0xff80, 0x0000
};

const uint16_t app_audio_adc_gain_table[] =
{
    0x0000, 0x002f, 0x0037, 0x003f, 0x0047, 0x012f, 0x0137, 0x013f,
    0x0147, 0x022f, 0x0237, 0x023f, 0x0247, 0x032f, 0x0337, 0x033f
};
#endif

static bool app_audio_dac_gain_cback(T_AUDIO_CATEGORY category, uint32_t           level,
                                     T_AUDIO_ROUTE_DAC_GAIN *gain)
{
    APP_PRINT_TRACE2("app_audio_dac_gain_cback category 0x%x, level %d", category, level);
    if (category == AUDIO_CATEGORY_AUDIO)
    {
        gain->dac_gain = app_dsp_cfg_data->audio_dac_gain_table[level];
        //gain->dac_gain = app_audio_dac_gain_table[level];
        return true;
    }
    return false;
}

bool app_audio_adc_gain_cback(T_AUDIO_CATEGORY category, uint32_t level,
                              T_AUDIO_ROUTE_ADC_GAIN *gain)
{
    APP_PRINT_TRACE2("app_audio_adc_gain_cback category 0x%x, level %d", category, level);
    if (category == AUDIO_CATEGORY_RECORD)
    {
        gain->adc_gain = app_dsp_cfg_data->record_adc_gain_table[level];
        //gain->adc_gain = app_audio_adc_gain_table[level];
        return true;
    }
    return false;
}


bool app_audio_track_init(void)
{
    g_app_audio_track.enable = true;
    g_app_audio_track.track_num = 0;
    g_app_audio_track.p_head = NULL;
    audio_mgr_cback_register(app_audio_track_cback);
    audio_route_gain_register(AUDIO_CATEGORY_AUDIO,
                              app_audio_dac_gain_cback,
                              NULL);
    audio_route_gain_register(AUDIO_CATEGORY_RECORD,
                              NULL,
                              app_audio_adc_gain_cback);

    return true;
}

