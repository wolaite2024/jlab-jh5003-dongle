#if F_APP_GAMING_DONGLE_SUPPORT

#include <string.h>
#include <stdlib.h>
#include "os_sync.h"
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "audio.h"
#include "btm.h"
#include "bt_a2dp.h"
#include "audio_track.h"
#include "app_timer.h"
#include "app_gaming_sync.h"
#include "app_link_util.h"
#include "app_dongle_audio.h"
#include "app_dongle_dual_mode.h"
#include "app_dongle_common.h"
#include "app_bt_policy_api.h"

typedef enum
{
    APP_TIMER_INCREASE_A2DP_INTERVAL,
} T_APP_DONGLE_AUDIO_TIMER;

typedef struct dongle_a2dp_list
{
    struct dongle_a2dp_list *next;
    T_BT_EVENT_PARAM_A2DP_STREAM_DATA_IND data_ind;
} T_dongle_a2dp_LIST;

typedef struct
{
    bool     last_a2dp_seq_updated;
    uint8_t  pkt_loss_cnt;
    uint8_t  frame_num;
    uint8_t  last_rcv_frame_num;
    uint16_t frame_length;
    uint16_t last_seq;
    uint16_t samples_per_frame;
    uint32_t last_timestamp;
    uint32_t pkt_loss_samples;
} T_dongle_a2dp_INFO;

#define SEQ_PREQUEUE_THRESHOLD          3
#define SEQ_PREQUEUE_COUNT              4

/* increase a2dp interval if continuous plc happens exceeds this threshold */
#define PLC_CHECK_THRESHOLD             3
#define PLC_CHECK_INTERVAL              3000

static T_OS_QUEUE dongle_a2dp_list_queue;
static T_dongle_a2dp_INFO dongle_a2dp_info;

static bool     prequeue_dongle_a2dp = false;
static uint8_t  app_dongle_audio_timer_id = 0;
static uint8_t  timer_idx_increase_a2dp_interval = 0;
static uint8_t  plc_check_cnt = 0;

static void app_audio_dongle_a2dp_prequeue_clear(void)
{
    T_dongle_a2dp_LIST *dongle_a2dp = NULL;
    T_dongle_a2dp_LIST *p_next = NULL;

    while ((dongle_a2dp = os_queue_peek(&dongle_a2dp_list_queue, 0)) != NULL)
    {
        os_queue_out(&dongle_a2dp_list_queue);

#if F_APP_REDUCE_HEAP_USAGE
        os_mem_free(dongle_a2dp->data_ind.payload);
        os_mem_free(dongle_a2dp);
#else
        free(dongle_a2dp->data_ind.payload);
        free(dongle_a2dp);
#endif
    }
}

static void app_audio_dongle_a2dp_write(T_AUDIO_TRACK_HANDLE handle,
                                        T_BT_EVENT_PARAM_A2DP_STREAM_DATA_IND *a2dp_data_ind)
{
    uint16_t written_len = 0;
    uint16_t samples_per_frame = dongle_a2dp_info.samples_per_frame;

    if (dongle_a2dp_info.last_a2dp_seq_updated == false)
    {
        dongle_a2dp_info.last_a2dp_seq_updated = true;
    }
    else
    {
        uint8_t seq_diff = a2dp_data_ind->seq_num - dongle_a2dp_info.last_seq;

        if (a2dp_data_ind->seq_num != (dongle_a2dp_info.last_seq + 1))
        {
            dongle_a2dp_info.pkt_loss_samples = a2dp_data_ind->timestamp - dongle_a2dp_info.last_timestamp -
                                                (dongle_a2dp_info.last_rcv_frame_num * samples_per_frame);
            dongle_a2dp_info.pkt_loss_cnt = a2dp_data_ind->seq_num - dongle_a2dp_info.last_seq - 1;

            while (dongle_a2dp_info.pkt_loss_cnt > 0 || dongle_a2dp_info.pkt_loss_samples > 0)
            {
                uint16_t write_seq;
                uint8_t write_frame_num;

                if (dongle_a2dp_info.pkt_loss_cnt > 1)
                {
                    write_frame_num = dongle_a2dp_info.pkt_loss_samples / dongle_a2dp_info.pkt_loss_cnt /
                                      samples_per_frame;
                }
                else
                {
                    write_frame_num = dongle_a2dp_info.pkt_loss_samples / samples_per_frame;
                }

                write_seq = ++dongle_a2dp_info.last_seq;
                dongle_a2dp_info.pkt_loss_cnt--;
                dongle_a2dp_info.pkt_loss_samples -= (samples_per_frame * write_frame_num);

                audio_track_write(handle, a2dp_data_ind->bt_clock,
                                  write_seq,
                                  AUDIO_STREAM_STATUS_LOST,
                                  write_frame_num,
                                  a2dp_data_ind->payload,
                                  0,
                                  &written_len);
            }
        }
    }

    audio_track_write(handle, a2dp_data_ind->bt_clock,
                      a2dp_data_ind->seq_num,
                      AUDIO_STREAM_STATUS_CORRECT,
                      a2dp_data_ind->frame_num,
                      a2dp_data_ind->payload,
                      a2dp_data_ind->len,
                      &written_len);

    dongle_a2dp_info.last_rcv_frame_num = a2dp_data_ind->frame_num;
    dongle_a2dp_info.last_seq = a2dp_data_ind->seq_num;
    dongle_a2dp_info.last_timestamp = a2dp_data_ind->timestamp;
    dongle_a2dp_info.frame_num = a2dp_data_ind->frame_num;
}

static bool app_audio_dongle_a2dp_prequeue(T_APP_BR_LINK *p_link,
                                           T_BT_EVENT_PARAM_A2DP_STREAM_DATA_IND *a2dp_data_ind)
{
    bool ret = false;
    T_dongle_a2dp_LIST *dongle_a2dp = NULL;
    bool enqueue_a2dp = false;
    bool clear_a2dp = false;
    bool prequeue_done = false;
    uint8_t i = 0;

    if (prequeue_dongle_a2dp)
    {
        if (dongle_a2dp_list_queue.count == 0)
        {
            enqueue_a2dp = true;
        }
        else
        {
            dongle_a2dp = os_queue_peek(&dongle_a2dp_list_queue, dongle_a2dp_list_queue.count - 1);
            uint16_t seq_diff = a2dp_data_ind->seq_num - dongle_a2dp->data_ind.seq_num;

            if (seq_diff <= SEQ_PREQUEUE_THRESHOLD)
            {
                enqueue_a2dp = true;
            }
            else
            {
                clear_a2dp = true;
                enqueue_a2dp = true;
            }
        }

        APP_PRINT_TRACE5("app_audio_dongle_a2dp_prequeue: count %d enqueue %d clear %d seq %d len %d",
                         dongle_a2dp_list_queue.count, enqueue_a2dp, clear_a2dp,
                         a2dp_data_ind->seq_num,
                         a2dp_data_ind->len);

        /* clear exist a2dp data before enqueue */
        if (clear_a2dp)
        {
            app_audio_dongle_a2dp_prequeue_clear();
        }

        /* store new a2dp data */
        if (enqueue_a2dp)
        {
#if F_APP_REDUCE_HEAP_USAGE
            dongle_a2dp = os_mem_alloc(RAM_TYPE_BUFFER_ON, sizeof(T_dongle_a2dp_LIST));
#else
            dongle_a2dp = malloc(sizeof(T_dongle_a2dp_LIST));
#endif

            if (dongle_a2dp != NULL)
            {
                T_BT_EVENT_PARAM_A2DP_STREAM_DATA_IND *data_ind = &dongle_a2dp->data_ind;

#if F_APP_REDUCE_HEAP_USAGE
                data_ind->payload = os_mem_alloc(RAM_TYPE_BUFFER_ON, a2dp_data_ind->len);
#else
                data_ind->payload = malloc(a2dp_data_ind->len);
#endif

                if (data_ind->payload != NULL)
                {
                    memcpy(data_ind->bd_addr, a2dp_data_ind->bd_addr, 6);
                    data_ind->bt_clock  = a2dp_data_ind->bt_clock;
                    data_ind->seq_num   = a2dp_data_ind->seq_num;
                    data_ind->timestamp = a2dp_data_ind->timestamp;
                    data_ind->frame_num = a2dp_data_ind->frame_num;
                    data_ind->len       = a2dp_data_ind->len;
                    memcpy(data_ind->payload, a2dp_data_ind->payload, a2dp_data_ind->len);

                    os_queue_in(&dongle_a2dp_list_queue, dongle_a2dp);
                }
            }
        }

        if (dongle_a2dp_list_queue.count >= SEQ_PREQUEUE_COUNT)
        {
            for (i = 0; i < dongle_a2dp_list_queue.count; i++)
            {
                dongle_a2dp = os_queue_peek(&dongle_a2dp_list_queue, i);

                app_audio_dongle_a2dp_write(p_link->a2dp_track_handle, &dongle_a2dp->data_ind);
            }

            app_audio_dongle_a2dp_prequeue_clear();
            prequeue_dongle_a2dp = false;
        }

        ret = true;
    }

    return ret;
}

static void app_audio_dongle_a2dp_extract_sync_pkt(T_APP_BR_LINK *p_link,
                                                   T_BT_EVENT_PARAM_A2DP_STREAM_DATA_IND *a2dp_data_ind)
{
    uint16_t a2dp_payload_size = 0;
    uint8_t frame_num = a2dp_data_ind->frame_num;
    uint16_t frame_length = dongle_a2dp_info.frame_length;
    uint8_t *sync_pkt = NULL;
    uint16_t sync_pkt_size = 0;

    a2dp_payload_size = frame_length * frame_num;

    if (a2dp_data_ind->len > a2dp_payload_size)
    {
        sync_pkt_size = a2dp_data_ind->len - a2dp_payload_size;
        a2dp_data_ind->len -= sync_pkt_size;

        sync_pkt = a2dp_data_ind->payload + a2dp_data_ind->len;
        app_gaming_sync_disassemble_data(a2dp_data_ind->bd_addr, sync_pkt, sync_pkt_size);
    }

}

static void app_audio_dongle_a2dp_handle(T_APP_BR_LINK *p_link,
                                         T_BT_EVENT_PARAM_A2DP_STREAM_DATA_IND *a2dp_data_ind)
{
#if F_APP_LEGACY_DONGLE_BINDING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (dongle_ctrl_data.disallow_dongle_a2dp)
    {
        return;
    }
#endif

    app_audio_dongle_a2dp_extract_sync_pkt(p_link, a2dp_data_ind);

    if (prequeue_dongle_a2dp)
    {
        app_audio_dongle_a2dp_prequeue(p_link, a2dp_data_ind);
    }
    else
    {
        app_audio_dongle_a2dp_write(p_link->a2dp_track_handle, a2dp_data_ind);
    }

}

static void app_dongle_audio_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            if (app_link_check_dongle_link(param->a2dp_stream_start_ind.bd_addr))
            {
                dongle_a2dp_info.last_a2dp_seq_updated = false;
                app_audio_dongle_a2dp_prequeue_clear();

                headset_status.increase_a2dp_interval = false;
                app_dongle_sync_headset_status();

                plc_check_cnt = 0;
                app_stop_timer(&timer_idx_increase_a2dp_interval);
            }
        }
        break;

    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
            T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL *a2dp_config_cmpl = &param->a2dp_config_cmpl;

            if (app_link_check_dongle_link(a2dp_config_cmpl->bd_addr))
            {
                if (a2dp_config_cmpl->codec_type == BT_A2DP_CODEC_TYPE_LC3)
                {
                    uint8_t channels = (a2dp_config_cmpl->codec_info.lc3.channel_number == BT_A2DP_LC3_CHANNEL_NUM_1) ?
                                       1 : 2;

                    dongle_a2dp_info.frame_length = a2dp_config_cmpl->codec_info.lc3.frame_length * channels;

                    if (a2dp_config_cmpl->codec_info.lc3.sampling_frequency == BT_A2DP_LC3_SAMPLING_FREQUENCY_48KHZ)
                    {
                        if (a2dp_config_cmpl->codec_info.lc3.frame_duration == BT_A2DP_LC3_FRAME_DURATION_10MS)
                        {
                            dongle_a2dp_info.samples_per_frame = 48 * 10;
                        }
                        else // BT_A2DP_LC3_FRAME_DURATION_7_5MS
                        {
                            dongle_a2dp_info.samples_per_frame = 48 * 15 / 2;
                        }
                    }
                }
                else if (a2dp_config_cmpl->codec_type == BT_A2DP_CODEC_TYPE_SBC)
                {
                    uint8_t channel_mode = a2dp_config_cmpl->codec_info.sbc.channel_mode;
                    uint8_t subbands = a2dp_config_cmpl->codec_info.sbc.subbands == BT_A2DP_SBC_SUBBANDS_8 ? 8 : 4;
                    uint8_t channels = (channel_mode == BT_A2DP_SBC_CHANNEL_MODE_MONO) ? 1 : 2;
                    uint8_t block_length = 0;
                    uint8_t bitpool = 37;
                    uint8_t frame_len = 0;

                    if (a2dp_config_cmpl->codec_info.sbc.block_length == BT_A2DP_SBC_BLOCK_LENGTH_4)
                    {
                        block_length = 4;
                    }
                    else if (a2dp_config_cmpl->codec_info.sbc.block_length == BT_A2DP_SBC_BLOCK_LENGTH_8)
                    {
                        block_length = 8;
                    }
                    else if (a2dp_config_cmpl->codec_info.sbc.block_length == BT_A2DP_SBC_BLOCK_LENGTH_12)
                    {
                        block_length = 12;
                    }
                    else // BT_A2DP_SBC_BLOCK_LENGTH_16
                    {
                        block_length = 16;
                    }

                    if (channel_mode == BT_A2DP_SBC_CHANNEL_MODE_MONO ||
                        channel_mode == BT_A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL)
                    {
                        frame_len = 4 + (4 * subbands * channels) / 8 + (block_length * channels * bitpool + 4) / 8;
                    }
                    else
                    {
                        uint8_t join = (channel_mode == BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO) ? 1 : 0;

                        frame_len = 4 + (4 * subbands * channels) / 8 + (join * subbands + block_length * bitpool + 4) / 8;
                    }

                    dongle_a2dp_info.frame_length = frame_len;

                    if (a2dp_config_cmpl->codec_info.sbc.sampling_frequency == BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ)
                    {
                        dongle_a2dp_info.samples_per_frame = 48 * 8 / 3;
                    }

                    APP_PRINT_TRACE4("dongle a2dp cfg: samples_per_frame %d frame_length %d bitpool (%d %d)",
                                     dongle_a2dp_info.samples_per_frame, dongle_a2dp_info.frame_length,
                                     a2dp_config_cmpl->codec_info.sbc.min_bitpool,
                                     a2dp_config_cmpl->codec_info.sbc.max_bitpool);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_DATA_IND:
        {
            uint32_t s = os_lock();
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->a2dp_stream_data_ind.bd_addr);

            if (p_link && app_link_check_dongle_link(param->a2dp_stream_data_ind.bd_addr))
            {
                app_audio_dongle_a2dp_handle(p_link, &param->a2dp_stream_data_ind);

                app_bt_policy_set_first_connect_sync_default_vol_flag(false);
            }

            os_unlock(s);
        }
        break;

    default:
        break;
    }
}

static void app_dongle_audio_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    T_AUDIO_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_LATENCY_REPORTED:
        {
            T_AUDIO_EVENT_PARAM_TRACK_LATENCY_REPORTED *latency = (T_AUDIO_EVENT_PARAM_TRACK_LATENCY_REPORTED *)
                                                                  event_buf;
            T_APP_BR_LINK *p_dongle_link = app_dongle_get_connected_dongle_link();

            if (p_dongle_link &&
                latency->handle == p_dongle_link->a2dp_track_handle)
            {
                APP_PRINT_TRACE4("app_dongle_audio_cback: lat report preset %d instant %d dejitter %d plc %d",
                                 latency->preset_latency, latency->instant_latency, latency->dejitter_compensation,
                                 latency->dsp_plc_sum);
            }

            handle = true;
        }
        break;

    case AUDIO_EVENT_TRACK_DECODER_PLC_NOTIFY_REPORTED:
        {
            T_AUDIO_EVENT_PARAM_TRACK_DECODER_PLC_NOTIFY_REPORTED *plc_report =
                &param->track_decoder_plc_notify_reported;
            bool increase_a2dp_interval = false;

            APP_PRINT_TRACE3("plc report: %d %d %d", plc_report->plc_sample_num, plc_report->total_sample_num,
                             plc_report->continue_sample_num);

            /* plc mute flow */
            if (plc_report->continue_sample_num > 0)
            {
                audio_track_plc_notify_set(plc_report->handle, 0, 0, false);
                prequeue_dongle_a2dp = true;

                if (headset_status.increase_a2dp_interval == false)
                {
                    plc_check_cnt++;
                    if (timer_idx_increase_a2dp_interval == 0)
                    {
                        app_start_timer(&timer_idx_increase_a2dp_interval, "increase_a2dp_interval",
                                        app_dongle_audio_timer_id, APP_TIMER_INCREASE_A2DP_INTERVAL, 0, false,
                                        PLC_CHECK_INTERVAL);
                    }
                    else
                    {
                        if (plc_check_cnt >= PLC_CHECK_THRESHOLD)
                        {
                            app_stop_timer(&timer_idx_increase_a2dp_interval);

                            /* increase a2dp send interval */
                            headset_status.increase_a2dp_interval = true;
                            app_dongle_sync_headset_status();

                            app_dongle_adjust_gaming_latency();
                        }
                    }
                }
            }
        }
        break;

    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            T_APP_BR_LINK *p_dongle_link = app_dongle_get_connected_dongle_link();

            if (p_dongle_link && p_dongle_link->a2dp_track_handle == param->track_state_changed.handle)
            {
                if (param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED ||
                    param->track_state_changed.state == AUDIO_TRACK_STATE_RESTARTED)
                {
                    audio_track_plc_notify_set(p_dongle_link->a2dp_track_handle, 1000, 1536, true);
                    prequeue_dongle_a2dp = false;
                }
            }

            handle = true;
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_dongle_audio_cback: event 0x%04x", event_type);
    }
}

static void app_dongle_audio_timer_cback(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_INCREASE_A2DP_INTERVAL:
        {
            app_stop_timer(&timer_idx_increase_a2dp_interval);
            plc_check_cnt = 0;
        }
        break;

    default:
        break;
    }
}

void app_dongle_audio_init(void)
{
    os_queue_init(&dongle_a2dp_list_queue);

    bt_mgr_cback_register(app_dongle_audio_bt_cback);

    audio_mgr_cback_register(app_dongle_audio_cback);

    app_timer_reg_cb(app_dongle_audio_timer_cback, &app_dongle_audio_timer_id);
}
#endif
