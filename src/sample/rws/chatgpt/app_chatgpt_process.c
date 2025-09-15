/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if F_APP_CHATGPT_SUPPORT
#include "app_main.h"
#include "app_chatgpt.h"
#include "app_cfg.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_bond.h"
#include "app_timer.h"
#include "app_dsp_cfg.h"

#define OPUS_DATA_LEN                       40
#define OPUS_DATA_CNTS                      4

#define VOICE_LEVEL_HIGH                    1000  //high level  ms
#define VOICE_LEVEL_LOW                     300  //low  level  ms

#define VOICE_MIDDLE                        (VOICE_LEVEL_HIGH-VOICE_LEVEL_LOW)/2
#define VOICE_LANTENCY                      (VOICE_LEVEL_LOW + VOICE_MIDDLE)

#define VOICE_END                           0
#define VOICE_CANCEL                        1

static uint8_t transport_data[256] = {0};
static uint16_t transport_msg_id = 0x00;
static uint8_t chatgpt_msg_id = 0x00;
static T_CHATGPT_STATUS chatgpt_status = CHATGPT_STATUS_IDLE;

static uint8_t app_chatgpt_timer_id = 0;
static uint8_t timer_idx_write_release = 0;

typedef enum
{
    APP_TIMER_WRITE_RELEASE,
} T_APP_CHATGPT_TIMER;

static T_AUDIO_TRACK_HANDLE voice_handle = NULL;
static uint16_t voice_frame_len = 0, voice_seq_num = 0;

static T_AUDIO_TRACK_HANDLE opus_handle = NULL;
static uint8_t opus_data[OPUS_DATA_LEN * OPUS_DATA_CNTS + OPUS_DATA_CNTS] = {0};
static uint8_t opus_data_cnts = 0;

static uint16_t app_chatgpt_process_get_frame_ctrl_nums(uint16_t frame_len)
{
    return (8 * VOICE_MIDDLE / frame_len);
}

static void app_chatgpt_process_opus_rst(void)
{
    opus_data_cnts = 0;
    memset(opus_data, 0, sizeof(opus_data));
    chatgpt_status = CHATGPT_STATUS_IDLE;
}

static void app_chatgpt_process_write_buffer_data(uint8_t *data, uint16_t len)
{
    uint8_t frame_num = len / voice_frame_len;
    uint16_t written_len = 0;

    if (data != NULL)
    {
        audio_track_write(voice_handle, 0xFFFFFFFF,
                          voice_seq_num++,
                          AUDIO_STREAM_STATUS_CORRECT,
                          frame_num,
                          data,
                          len,
                          &written_len);
    }
}

static void app_chatgpt_process_transmit(uint8_t key, uint8_t *pbuf, uint16_t len)
{
    T_CHATGPT_TRANS_L2_HEADER header = {0};

    if ((len + TRANS_L2_HEADER_LEN) > sizeof(transport_data))
    {
        APP_PRINT_WARN1("app_chatgpt_process_transmit: len is over %d", len + TRANS_L2_HEADER_LEN);
        return;
    }

    header.opcode = TRANS_CMD_CHATGPT;
    header.version = 0;
    header.key = key;
    header.len_high = (len >> 8) & 0xff;
    header.len_low = (len >> 0) & 0xff;

    memcpy(transport_data, (uint8_t *)&header, TRANS_L2_HEADER_LEN);
    memcpy(transport_data + TRANS_L2_HEADER_LEN, pbuf, len);

    app_chatgpt_transport_transmit(key, transport_data, (len + TRANS_L2_HEADER_LEN));
}

static void app_chatgpt_process_opus_write(uint8_t *data, uint16_t len)
{
    uint8_t offset = opus_data_cnts * (OPUS_DATA_LEN + 1);

    if (len != OPUS_DATA_LEN)
    {
        APP_PRINT_WARN1("app_chatgpt_process_opus_write: len is err %d", len);
        return;
    }

    opus_data[offset] = len;
    memcpy(&opus_data[offset + 1], data, len);

    opus_data_cnts++;

    if (opus_data_cnts >= OPUS_DATA_CNTS)
    {
        opus_data_cnts = 0;
        app_chatgpt_process_transmit(CHATGPT_CMD_VOICE_DATA, opus_data, sizeof(opus_data));
        memset(opus_data, 0, sizeof(opus_data));
    }
}

static bool app_chatgpt_process_track_async_read(T_AUDIO_TRACK_HANDLE   handle,
                                                 uint32_t              *timestamp,
                                                 uint16_t              *seq_num,
                                                 T_AUDIO_STREAM_STATUS *status,
                                                 uint8_t               *frame_num,
                                                 void                  *buf,
                                                 uint16_t               required_len,
                                                 uint16_t              *actual_len)
{
    chatgpt_status = CHATGPT_STATUS_OPUS_ENCODE_START;
    app_chatgpt_process_opus_write(buf, required_len);
    *actual_len = required_len;

    return true;
}

static void app_chatgpt_process_opus_create(void)
{
    T_AUDIO_FORMAT_INFO opus_info;

    if (opus_handle != NULL)
    {
        return;
    }

    opus_info.type = AUDIO_FORMAT_TYPE_OPUS;
    opus_info.attr.opus.sample_rate = 16 * 1000;
    opus_info.attr.opus.chann_num = 1;
    opus_info.attr.opus.cbr = 1;
    opus_info.attr.opus.cvbr = 0;
    opus_info.attr.opus.mode = 3;
    opus_info.attr.opus.complexity = 3;
    opus_info.attr.opus.frame_duration = AUDIO_OPUS_FRAME_DURATION_20_MS;
    opus_info.attr.opus.bitrate = 16 * 1000;

    bt_power_mode_set(BTPOWER_ACTIVE);
    opus_handle = audio_track_create(AUDIO_STREAM_TYPE_RECORD,
                                     AUDIO_STREAM_MODE_NORMAL,
                                     AUDIO_STREAM_USAGE_LOCAL,
                                     opus_info,
                                     0,
                                     app_dsp_cfg_vol.record_volume_default,
                                     AUDIO_DEVICE_IN_MIC,
                                     NULL,
                                     app_chatgpt_process_track_async_read);

    audio_track_start(opus_handle);
}

static void app_chatgpt_process_opus_release(void)
{
    if (opus_handle == NULL)
    {
        return;
    }

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    audio_track_stop(opus_handle);
    audio_track_release(opus_handle);
    opus_handle = NULL;
}

void app_chatgpt_process_start(void)
{
    T_CHATGPT_START_VOICE start_voice = {0};
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    /*opus*/
    start_voice.codec_type = 0;

    start_voice.msg_id = ++chatgpt_msg_id;
    start_voice.channel = 1;

    /*16k*/
    start_voice.sample_rate[0] = 0;
    start_voice.sample_rate[1] = 0;
    start_voice.sample_rate[2] = 0x3E;
    start_voice.sample_rate[3] = 0x80;

    if (app_db.br_link[active_a2dp_idx].connected_profile & A2DP_PROFILE_MASK)
    {
        start_voice.a2dp = 1;
    }
    else
    {
        start_voice.ble = 1;
    }

    chatgpt_status = CHATGPT_STATUS_OPUS_START;
    transport_msg_id = app_chatgpt_transport_get_seq();
    app_chatgpt_process_transmit(CHATGPT_CMD_VOICE_START, (uint8_t *)&start_voice, sizeof(start_voice));
}

void app_chatgpt_process_stop(void)
{
    T_CHATGPT_STOP_VOICE stop_voice = {0};

    stop_voice.voice = VOICE_END;

    chatgpt_status = CHATGPT_STATUS_OPUS_STOP;
    transport_msg_id = app_chatgpt_transport_get_seq();
    app_chatgpt_process_opus_release();
    app_chatgpt_process_transmit(CHATGPT_CMD_VOICE_END, (uint8_t *)&stop_voice, sizeof(stop_voice));
}

void app_chatgpt_process_frame_ctrl(bool increase, uint16_t nums)
{
    T_CHATGPT_FRAME_CTRL frame_ctrl = {0};

    frame_ctrl.method = increase ? 1 : 2;
    frame_ctrl.frame_high = (nums >> 8) & 0xff;
    frame_ctrl.frame_low = (nums >> 0) & 0xff;

    app_chatgpt_process_transmit(CHATGPT_EVT_RX_BUFFER_CTRL, (uint8_t *)&frame_ctrl,
                                 sizeof(frame_ctrl));
}

void app_chatgpt_process_ack_check(uint16_t seq)
{
    if (seq == transport_msg_id)
    {
        if (chatgpt_status == CHATGPT_STATUS_OPUS_START)
        {
            chatgpt_status = CHATGPT_STATUS_OPUS_CREATE;
            app_chatgpt_process_opus_create();
        }
        if (chatgpt_status == CHATGPT_STATUS_OPUS_STOP)
        {
            app_chatgpt_process_opus_rst();
        }
    }
}

void app_chatgpt_process_event_handle(uint8_t *data, uint16_t len, uint8_t *need_ack)
{
    T_CHATGPT_TRANS_L2_HEADER l2_header = {0};
    T_CHATGPT_EVT_DAT *evt_data = (T_CHATGPT_EVT_DAT *)(data + TRANS_L2_HEADER_LEN);
    uint16_t data_len = 0;

    memcpy((uint8_t *)&l2_header, data, TRANS_L2_HEADER_LEN);
    data_len = (l2_header.len_high << 8) + l2_header.len_low;

    switch (l2_header.key)
    {
    case CHATGPT_EVT_VOICE_DATA_START:
        {
            uint8_t active_hf_idx = app_hfp_get_active_idx();
            uint8_t pair_idx = 0;

            chatgpt_status = CHATGPT_STATUS_VOICE_START;

            app_bond_get_pair_idx_mapping(app_db.br_link[active_hf_idx].bd_addr, &pair_idx);

            if (data_len == sizeof(T_CHATGPT_VOICE_DAT_START))
            {
                voice_frame_len = 60;

                T_AUDIO_FORMAT_INFO format_info;
                format_info.type = AUDIO_FORMAT_TYPE_SBC;
                format_info.attr.sbc.sample_rate = 16 * 1000;
                format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
                format_info.attr.sbc.block_length = 16;
                format_info.attr.sbc.subband_num = 8;
                format_info.attr.sbc.bitpool = 26;
                format_info.attr.sbc.allocation_method = 0;

                voice_handle = audio_track_create(AUDIO_STREAM_TYPE_PLAYBACK,
                                                  AUDIO_STREAM_MODE_NORMAL,
                                                  AUDIO_STREAM_USAGE_LOCAL,
                                                  format_info,
                                                  //  app_cfg_nv.audio_gain_level[pair_idx],
                                                  15,
                                                  0,
                                                  AUDIO_DEVICE_OUT_DEFAULT,
                                                  NULL,
                                                  NULL);

                if (voice_handle != NULL)
                {
                    audio_track_latency_set(voice_handle, VOICE_LANTENCY, true);
                    audio_track_threshold_set(voice_handle, VOICE_LEVEL_HIGH, VOICE_LEVEL_LOW);
                    audio_track_start(voice_handle);
                }
            }
        }
        break;

    case CHATGPT_EVT_VOICE_DATA:
        {
            uint8_t msg_id = data[TRANS_L2_HEADER_LEN];

            if (chatgpt_msg_id == msg_id)
            {
                app_chatgpt_process_write_buffer_data(data + TRANS_L2_HEADER_LEN + 1,
                                                      len - TRANS_L2_HEADER_LEN - 1);
            }

            *need_ack = 0;
        }
        break;

    case CHATGPT_EVT_VOICE_DATA_STOP:
        {
            chatgpt_status = CHATGPT_STATUS_VOICE_STOP;
        }
        break;

    default:
        break;
    }
}

static void app_chatgpt_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_chatgpt_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_WRITE_RELEASE:
        {
            chatgpt_status = CHATGPT_STATUS_IDLE;
            app_stop_timer(&timer_idx_write_release);
            if (voice_handle != NULL)
            {
                audio_track_stop(voice_handle);
                audio_track_release(voice_handle);
            }
        }
        break;

    default:
        break;
    }
}

static void app_chatgpt_process_audio_cback(T_AUDIO_EVENT event_type, void *event_buf,
                                            uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    if (param->track_state_changed.handle != voice_handle)
    {
        return;
    }

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_BUFFER_LOW:
        {
            if (chatgpt_status == CHATGPT_STATUS_VOICE_STOP)
            {
                app_start_timer(&timer_idx_write_release, "write_release",
                                app_chatgpt_timer_id, APP_TIMER_WRITE_RELEASE, 0, false, VOICE_LEVEL_LOW);
            }
            else
            {
                app_chatgpt_process_frame_ctrl(true, app_chatgpt_process_get_frame_ctrl_nums(voice_frame_len));
            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_HIGH:
        {
            app_chatgpt_process_frame_ctrl(false, app_chatgpt_process_get_frame_ctrl_nums(voice_frame_len));

        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_chatgpt_process_audio_cback: event_type 0x%04x", event_type);
    }
}

void app_chatgpt_process_init(void)
{
    audio_mgr_cback_register(app_chatgpt_process_audio_cback);
    app_timer_reg_cb(app_chatgpt_timeout_cb, &app_chatgpt_timer_id);
}
#endif
