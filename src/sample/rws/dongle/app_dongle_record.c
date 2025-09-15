#if F_APP_GAMING_DONGLE_SUPPORT
#include "app_main.h"
#include "app_multilink.h"
#include "app_dongle_record.h"
#include "app_dongle_spp.h"
#include "app_report.h"
#include "audio_track.h"
#include "ring_buffer.h"
#include "stdlib.h"
#include "trace.h"
#include "app_cfg.h"
#include "app_dsp_cfg.h"
#include "app_roleswap_control.h"
#include "app_sniff_mode.h"
#include "app_audio_policy.h"
#include "app_hfp.h"
#include "app_auto_power_off.h"
#include "bt_a2dp.h"
#include "app_dongle_common.h"
#include "app_dongle_dual_mode.h"
#include "gap_br.h"
#include "app_transfer.h"
#include "nrec.h"
#include "app_nrec.h"

#if F_APP_REDUCE_HEAP_USAGE
#include "os_mem.h"
#endif

#if F_APP_GAMING_CONTROLLER_SUPPORT
#include "app_dongle_controller.h"
#endif

#if F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE
#include "app_slide_switch.h"
#endif

#if F_APP_SIDETONE_SUPPORT
#include "app_sidetone.h"
#endif

#if F_APP_MUTLILINK_SOURCE_PRIORITY_UI
#include "app_multilink_customer.h"
#endif

#define AUDIO_LC3_16K_FRAME_LEN  40
#define AUDIO_LC3_32K_FRAME_LEN  80
#define AUDIO_LC3_48K_FRAME_LEN  120

/* start, type(3 bits; other reserved), (including seq and len msb two bits), len, voice_data, len */
#define RTP_VOICE_HEADER_LEN     5

#define DONGLE_RECORD_DATA_DBG 0

#define RTP_SBC_32K_FRAME_CNT   6
#define RTP_SBC_16K_FRAME_CNT   1
#define RTP_LC3_48K_FRAME_CNT   4
#define RTP_LC3_32K_FRAME_CNT   6
#define RTP_LC3_16K_FRAME_CNT   1

typedef enum
{
    RECORD_TYPE_LC3_MONO_16K,
    RECORD_TYPE_LC3_MONO_32K,
    RECORD_TYPE_LC3_MONO_48K,
    RECORD_TYPE_MAX_NUM,
} T_DONGLE_RECORD_TYPE;

typedef struct
{
    bool is_start;
    T_AUDIO_TRACK_HANDLE handle;
    uint8_t num_voice_buf;
    uint8_t *p_buf;
    T_RING_BUFFER voice_buf;
    T_AUDIO_EFFECT_INSTANCE sidetone_instance;
    T_AUDIO_EFFECT_INSTANCE eq_instance;
    T_AUDIO_EFFECT_INSTANCE nrec_instance;
    T_RECORD_SAMPLE_RATE sample_rate;
} APP_DONGLE_RECORD;

static void app_dongle_record_start(T_RECORD_SAMPLE_RATE sample_rate);

static APP_DONGLE_RECORD dongle_record = {.is_start = false, .handle = NULL};
static uint8_t rtp_seq = 0;

static const uint16_t record_sample_rate[] =
{
    [RECORD_SAMPLE_RATE_16K] = 16000,
    [RECORD_SAMPLE_RATE_32K] = 32000,
    [RECORD_SAMPLE_RATE_48K] = 48000,
};

static const uint8_t record_frame_size[] =
{
    [RECORD_SAMPLE_RATE_16K] = AUDIO_LC3_16K_FRAME_LEN,
    [RECORD_SAMPLE_RATE_32K] = AUDIO_LC3_32K_FRAME_LEN,
    [RECORD_SAMPLE_RATE_48K] = AUDIO_LC3_48K_FRAME_LEN,
};

static const uint8_t record_frame_cnt[] =
{
    [RECORD_SAMPLE_RATE_16K] = RTP_LC3_16K_FRAME_CNT,
    [RECORD_SAMPLE_RATE_32K] = RTP_LC3_32K_FRAME_CNT,
    [RECORD_SAMPLE_RATE_48K] = RTP_LC3_48K_FRAME_CNT,
};

static const T_AUDIO_FORMAT_INFO dongle_record_fmt[] =
{
    [RECORD_TYPE_LC3_MONO_16K] =
    {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 16000,
        .attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO,
        .attr.lc3.frame_length = AUDIO_LC3_16K_FRAME_LEN,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS,
    },

    [RECORD_TYPE_LC3_MONO_32K] =
    {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 32000,
        .attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO,
        .attr.lc3.frame_length = AUDIO_LC3_32K_FRAME_LEN,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS,
    },

    [RECORD_TYPE_LC3_MONO_48K] =
    {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 48000,
        .attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO,
        .attr.lc3.frame_length = AUDIO_LC3_48K_FRAME_LEN,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS,
    },
};

#if DONGLE_RECORD_DATA_DBG
static void dongle_dump_record_data(const char *title, uint8_t *record_data_buf, uint32_t data_len)
{
    const uint32_t bat_num = 8;
    uint32_t times = data_len / bat_num;
    uint32_t residue = data_len % bat_num;
    uint8_t *residue_buf = record_data_buf + times * bat_num;

    APP_PRINT_TRACE3("dongle_dump_record_data0: data_len %d, times %d, residue %d", data_len,
                     times, residue);
    APP_PRINT_TRACE2("dongle_dump_record_data1: record_data_buf is 0x%08x, residue_buf is 0x%08x\r\n",
                     (uint32_t)record_data_buf,
                     (uint32_t)residue_buf);

    for (int32_t i = 0; i < times; i++)
    {
        APP_PRINT_TRACE8("dongle_dump_record_data2: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         record_data_buf[i * bat_num], record_data_buf[i * bat_num + 1], record_data_buf[i * bat_num + 2],
                         record_data_buf[i * bat_num + 3],
                         record_data_buf[i * bat_num + 4], record_data_buf[i * bat_num + 5],
                         record_data_buf[i * bat_num + 6],
                         record_data_buf[i * bat_num + 7]);
    }

    switch (residue)
    {
    case 1:
        APP_PRINT_TRACE1("dongle_dump_record_data3: 0x%02x\r\n", residue_buf[0]);
        break;
    case 2:
        APP_PRINT_TRACE2("dongle_dump_record_data4: 0x%02x, 0x%02x\r\n", residue_buf[0], residue_buf[1]);
        break;
    case 3:
        APP_PRINT_TRACE3("dongle_dump_record_data5: 0x%02x, 0x%02x, 0x%02x\r\n", residue_buf[0],
                         residue_buf[1],
                         residue_buf[2]);
        break;
    case 4:
        APP_PRINT_TRACE4("dongle_dump_record_data6: 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n", residue_buf[0],
                         residue_buf[1], residue_buf[2], residue_buf[3]);
        break;
    case 5:
        APP_PRINT_TRACE5("dongle_dump_record_data7: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4]);
        break;
    case 6:
        APP_PRINT_TRACE6("dongle_dump_record_data8: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4], residue_buf[5]);
        break;
    case 7:
        APP_PRINT_TRACE7("dongle_dump_record_data9: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4], residue_buf[5],
                         residue_buf[6]);
        break;

    default:
        break;
    }
}
#endif

bool app_dongle_get_record_state(void)
{
    return dongle_record.is_start;
}

static void app_dongle_set_record_state(bool record_status)
{
    if (dongle_record.is_start != record_status)
    {
        dongle_record.is_start = record_status;
#if F_APP_MUTLILINK_SOURCE_PRIORITY_UI
        app_multilink_customer_update_dongle_record_status(dongle_record.is_start);
#endif
    }
}

uint16_t app_dongle_assemble_rtp_voice(uint8_t *buf, uint16_t buf_size, uint16_t voice_data_len)
{
    T_APP_DONGLE_DATA_TYPE data_type = DONGLE_TYPE_UPSTREAM_VOICE;
    uint8_t rtp_frame_cnt = record_frame_cnt[dongle_record.sample_rate];

    if (buf_size < voice_data_len + RTP_VOICE_HEADER_LEN)
    {
        APP_PRINT_ERROR0("app_dongle_assemble_rtp_voice failed");

        return 0;
    }

    buf[0] = DONGLE_FORMAT_START_BIT;
    buf[1] = ((rtp_frame_cnt & 0x07) << 3) | (data_type & 0x07); // rsv MSB 2 bit
    buf[2] = ((rtp_seq & 0x3f) << 2) | ((voice_data_len >> 8) & 0x03);
    buf[3] = voice_data_len & 0xff;

    rtp_seq++;
    if (rtp_seq > 0x3f)
    {
        rtp_seq = 0;
    }

    uint32_t actual_len = ring_buffer_read(&dongle_record.voice_buf,
                                           voice_data_len,
                                           buf + 4);

    buf[4 + voice_data_len] = DONGLE_FORMAT_STOP_BIT;

    return actual_len + RTP_VOICE_HEADER_LEN;
}


uint16_t app_dongle_get_voice_data_len(void)
{
    return record_frame_size[dongle_record.sample_rate] * record_frame_cnt[dongle_record.sample_rate];
}

void app_dongle_mic_data_report(void *data, uint16_t required_len)
{
    if (!app_link_find_br_link(app_cfg_nv.dongle_addr))
    {
        APP_PRINT_ERROR1("app_dongle_mic_data_report: bd_addr %b has no link",
                         TRACE_BDADDR(app_cfg_nv.dongle_addr));
        return;
    }

    if (ring_buffer_get_remaining_space(&dongle_record.voice_buf) < required_len)
    {
        APP_PRINT_ERROR0("app_dongle_mic_data_report: buf not enough");
        return;
    }

    ring_buffer_write(&dongle_record.voice_buf, data, required_len);
    dongle_record.num_voice_buf++;

    if (dongle_record.num_voice_buf == record_frame_cnt[dongle_record.sample_rate])
    {
        uint16_t voice_data_len = app_dongle_get_voice_data_len();
        uint16_t malloc_len = voice_data_len + RTP_VOICE_HEADER_LEN;
        uint16_t sync_data_len = 0;

        if (app_cfg_const.spp_voice_smaple_rate == RECORD_SAMPLE_RATE_16K)
        {
            if (app_gaming_set_sync_data_in_streaming())
            {
                sync_data_len = app_gaming_sync_get_data_len_for_streaming();
            }
        }

        malloc_len += sync_data_len;

        uint8_t *p_data_to_send = malloc(malloc_len);

        if (p_data_to_send)
        {
            uint16_t voice_pkt_len = app_dongle_assemble_rtp_voice(p_data_to_send, malloc_len, voice_data_len);

            if (voice_pkt_len == (voice_data_len + RTP_VOICE_HEADER_LEN)
                && app_roleswap_ctrl_get_status() == APP_ROLESWAP_STATUS_IDLE)
            {
                if (sync_data_len)
                {
                    app_gaming_sync_append_data_to_streaming(p_data_to_send + RTP_VOICE_HEADER_LEN + voice_data_len,
                                                             sync_data_len);
                }

#if F_APP_GAMING_CONTROLLER_SUPPORT
                if (app_dongle_controller_is_transmitting())
                {
                    app_dongle_controller_save_voice_data(p_data_to_send, malloc_len);
                }
                else
#endif
                {
                    app_dongle_send_fix_channel_data(app_cfg_nv.dongle_addr, FIX_CHANNEL_CID, p_data_to_send,
                                                     malloc_len, true);
                }
            }

            free(p_data_to_send);
            dongle_record.num_voice_buf = 0;
        }
    }
}

bool app_dongle_record_read_cb(T_AUDIO_TRACK_HANDLE   handle,
                               uint32_t              *timestamp,
                               uint16_t              *seq_num,
                               T_AUDIO_STREAM_STATUS *status,
                               uint8_t               *frame_num,
                               void                  *buf,
                               uint16_t               required_len,
                               uint16_t              *actual_len)
{
    bool report_mic_data = false;

    APP_PRINT_TRACE2("app_dongle_record_read_cb: buf 0x%08x, required_len %d", buf, required_len);

#if DONGLE_RECORD_DATA_DBG
    dongle_dump_record_data("app_dongle_record_read_cb", buf, required_len);
#endif

    if ((app_db.remote_is_dongle) && (app_db.dongle_is_enable_mic)
        && (app_roleswap_ctrl_get_status() == APP_ROLESWAP_STATUS_IDLE))
    {
        report_mic_data = true;
    }

    if (report_mic_data)
    {
        app_dongle_mic_data_report(buf, required_len);

    }

    *actual_len = required_len;

    return true;
}

/**
    * @brief        This function can stop the record.
    * @return       void
    */
void app_dongle_stop_recording(void)
{
    headset_status.upstream_enable = false;
    app_dongle_sync_headset_status();

    if (app_dongle_get_record_state() != true)
    {
        APP_PRINT_ERROR0("dongle_voice_stop_capture: already stopped!");
        return;
    }

    APP_PRINT_TRACE0("app_dongle_stop_recording");
    app_dongle_set_record_state(false);
    app_transfer_queue_reset(CMD_PATH_SPP);

    if (app_audio_is_mic_mute()
#if F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE
        && !app_slide_switch_mic_mute_toggle_support()
#endif
       )
    {
        app_audio_set_mic_mute_status(0);
        app_dongle_volume_in_unmute();
    }

    audio_track_stop(dongle_record.handle);

    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_SPP_RECORD);

    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE) && (app_cfg_const.enable_multi_link))
    {
        T_APP_BR_LINK *p_link;
        p_link = app_dongle_get_connected_dongle_link();

        app_bt_policy_b2s_tpoll_update(app_cfg_nv.dongle_addr, BP_TPOLL_EVENT_DONGLE_SPP_STOP);

        if (p_link == NULL)
        {
            app_hfp_update_call_status();

            app_multi_handle_sniffing_link_disconnect_event(0xff);
        }
        else
        {
            app_multi_handle_sniffing_link_disconnect_event(p_link->id);
        }
    }
}

/**
    * @brief        This function can start the record.
    * @return       void
    */
void app_dongle_start_recording(void)
{
    if (app_dongle_get_record_state() != false)/*g_voice_data.is_voice_start == false8*/
    {
        APP_PRINT_ERROR0("app_dongle_start_recording: already started");
        return;
    }

    app_dongle_record_start(dongle_record.sample_rate);

    APP_PRINT_INFO0("app_dongle_start_recording");
    app_dongle_set_record_state(true);

    app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_SPP_RECORD);

    uint8_t dongle_addr[6];
    if (app_dongle_get_connected_dongle_addr(dongle_addr))
    {
        app_bt_policy_b2s_tpoll_update(dongle_addr, BP_TPOLL_EVENT_DONGLE_SPP_START);
    }

#if F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE
    if (app_slide_switch_mic_mute_toggle_support())
    {
        app_hfp_mute_ctrl();
    }
#endif
}

void app_dongle_volume_in_mute(void)
{
    audio_track_volume_in_mute(dongle_record.handle);
    app_audio_tone_type_play(TONE_MIC_MUTE_ON, false, true);
}

void app_dongle_volume_in_unmute(void)
{
    audio_track_volume_in_unmute(dongle_record.handle);
    app_audio_tone_type_play(TONE_MIC_MUTE_OFF, false, true);
}

bool app_dongle_get_record_status(void)
{
    bool rtn  = false;

    audio_track_volume_in_is_muted(dongle_record.handle, &rtn);

    APP_PRINT_TRACE1("app_dongle_get_record_status %d", rtn);
    return rtn;
}

T_RECORD_SAMPLE_RATE app_dongle_get_record_sample_rate(void)
{
    return dongle_record.sample_rate;
}

static void app_dongle_record_start(T_RECORD_SAMPLE_RATE sample_rate)
{
    T_AUDIO_FORMAT_INFO format_info = {};
    T_DONGLE_RECORD_TYPE record_type;

    if (sample_rate > app_cfg_const.spp_voice_smaple_rate)
    {
        APP_PRINT_ERROR2("app_dongle_record_start: sample_rate %u exceeds max %u (heap not sufficient)",
                         sample_rate, app_cfg_const.spp_voice_smaple_rate);
        return;
    }

    APP_PRINT_TRACE1("app_dongle_record_start: sample_rate %d", sample_rate);

    /* check if we need to create a different track handle */
    if (dongle_record.handle != NULL)
    {
        if (sample_rate != dongle_record.sample_rate)
        {
            app_nrec_detach(dongle_record.handle, dongle_record.nrec_instance);
            app_sidetone_detach(dongle_record.handle, dongle_record.sidetone_instance);
            eq_release(dongle_record.eq_instance);
            audio_track_release(dongle_record.handle);
            dongle_record.handle = NULL;
        }
    }

    if (dongle_record.handle == NULL)
    {
        if (sample_rate == RECORD_SAMPLE_RATE_16K)
        {
            headset_status.upstream_codec = LC3_16K_16BIT_MONO_10MS;
            record_type = RECORD_TYPE_LC3_MONO_16K;
        }
        else if (sample_rate == RECORD_SAMPLE_RATE_32K)
        {
            headset_status.upstream_codec = LC3_32K_16BIT_MONO_10MS;
            record_type = RECORD_TYPE_LC3_MONO_32K;
        }
        else
        {
            headset_status.upstream_codec = LC3_48K_16BIT_MONO_10MS;
            record_type = RECORD_TYPE_LC3_MONO_48K;
        }
        memcpy(&format_info, &dongle_record_fmt[record_type], sizeof(T_AUDIO_FORMAT_INFO));

        dongle_record.handle = audio_track_create(AUDIO_STREAM_TYPE_RECORD,
                                                  AUDIO_STREAM_MODE_NORMAL,
                                                  AUDIO_STREAM_USAGE_LOCAL,
                                                  format_info,
                                                  0,
                                                  app_dsp_cfg_vol.record_volume_default,
                                                  AUDIO_DEVICE_IN_MIC,
                                                  NULL,
                                                  app_dongle_record_read_cb);

        if (dongle_record.handle)
        {
            dongle_record.eq_instance = app_eq_create(EQ_CONTENT_TYPE_RECORD, EQ_STREAM_TYPE_RECORD, MIC_SW_EQ,
                                                      VOICE_MIC_MODE, 0);

            if (dongle_record.eq_instance)
            {
                eq_enable(dongle_record.eq_instance);
                audio_track_effect_attach(dongle_record.handle, dongle_record.eq_instance);
            }

            dongle_record.sidetone_instance = app_sidetone_attach(dongle_record.handle, app_dsp_cfg_sidetone);

            dongle_record.nrec_instance = nrec_create(NREC_CONTENT_TYPE_RECORD, NREC_MODE_HIGH_SOUND_QUALITY,
                                                      0);
            if (dongle_record.nrec_instance)
            {
                nrec_enable(dongle_record.nrec_instance);
                audio_track_effect_attach(dongle_record.handle, dongle_record.nrec_instance);
            }
        }
    }

    rtp_seq = 0;
    dongle_record.num_voice_buf = 0;
    ring_buffer_clear(&dongle_record.voice_buf);

    audio_track_start(dongle_record.handle);

    dongle_record.sample_rate = sample_rate;

    headset_status.upstream_enable = true;
    app_dongle_sync_headset_status();
}

void app_dongle_record_init(void)
{
    dongle_record.sample_rate = (T_RECORD_SAMPLE_RATE)app_cfg_const.spp_voice_smaple_rate;

    uint8_t frame_size = record_frame_size[dongle_record.sample_rate];
    uint8_t frame_cnt = record_frame_cnt[dongle_record.sample_rate];

#if F_APP_REDUCE_HEAP_USAGE
    dongle_record.p_buf = os_mem_alloc(RAM_TYPE_BUFFER_ON,
                                       frame_size * frame_cnt + 1);
#else
    dongle_record.p_buf = malloc(frame_size * frame_cnt + 1);
#endif

    ring_buffer_init(&dongle_record.voice_buf, dongle_record.p_buf,
                     frame_size * frame_cnt + 1);
}

#endif
