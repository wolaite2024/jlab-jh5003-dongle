#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "section.h"
#include "ring_buffer.h"
#include "audio_type.h"
#include "audio_pipe.h"
#include "usb_audio_config.h"
#include "usb_audio_stream.h"
#include "app_io_msg.h"
#include "app_timer.h"
#include "app_downstream_encode.h"
#include "app_usb_audio_wrapper.h"
#include "app_src_policy.h"
#include "app_link_util.h"
#include "app_usb_uac.h"
#include "app_upstream_decode.h"
#include "app_cfg.h"
#include "ble_audio_def.h"
#include "os_sync.h"
#include "app_ctrl_pkt_policy.h"
#if F_APP_USB_GIP_SUPPORT
#include "app_gip.h"
#endif

#include "app_audio_pipe_mgr.h"

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "bt_a2dp.h"
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "app_adapter_service.h"
#endif
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
#include "gap_iso_data.h"
#include "syncclk_driver.h"
#include "bt_syscall.h"
#include "le_audio_data.h"
#include "le_media_player.h"
#include "app_le_audio.h"
#endif

#define DOWNSTREAM_DEBUG            0

#define DONGLE_STREAMING_DETECT_TIMEOUT     600
#define DONGLE_STREAMING_DETECT_INTERVAL    300

#define USB_DS_PCM_BUF_SIZE         4096
#define USB_DS_CODEC_BUF_SIZE       1024
#define USB_DS_SYNC_DATA_BUF_SIZE   1024
#define USB_DS_TMP_BUF_SIZE         1920

#define ASRC_DETECT_THRESHOLD       1
#define DS_PIPE_ASRC_RATIO          (500)

#define FILL_TIME_DIFF_MIN   300
#if (USB_AUDIO_DS_INTERVAL == 2)
#define FILL_TIME_DIFF_MAX   2700
#else
#define FILL_TIME_DIFF_MAX   1700
#endif
#define FILL_TIME_DIFF_MIN2  500
#if (USB_AUDIO_DS_INTERVAL == 2)
#define FILL_TIME_DIFF_MAX2  3000
#else
#define FILL_TIME_DIFF_MAX2  2000
#endif

#define ASRC_TIME_DIFF_MIN   500
#define ASRC_TIME_DIFF_MAX   (ASRC_TIME_DIFF_MIN + PCM_DS_INTERVAL + 300)

#define ASRC_TIME_DIFF_MIN2  500
#define ASRC_TIME_DIFF_MAX2  (ASRC_TIME_DIFF_MIN2 + (2 * PCM_DS_INTERVAL) + 300)

typedef enum
{
    NORMAL_MODE_2_5MS,
    NORMAL_MODE_5MS,
    NORMAL_MODE_7_5MS,
    NORMAL_MODE_10MS,
    HIRES_MODE_10MS,
} T_LEA_FRAME_DURATION_IDX;

typedef enum
{
    TIMER_STOP_CIS_STREAM,
    TIMER_STREAMING_DETECT,
} T_APP_USB_DS_TIMER;

typedef enum
{
    NONE_TRIGGER_ENCODE,
    UAC1_TRIGGER_ENCODE,
    UAC2_TRIGGER_ENCODE,
} T_APP_TRIGGER_ENCODE_UAC;

typedef enum
{
    ASRC_NORMAL_STATE,
    ASRC_NORMAL_DETECT_STATE,
    ASRC_SLOW_DETECT_STATE,
    ASRC_SLOW_STATE,
    ASRC_FAST_DETECT_STATE,
    ASRC_FAST_STATE,
} T_APP_ASRC_STATE;

typedef struct
{
    int32_t asrc_ratio;
} T_APP_ASRC_INFO;

typedef struct
{
    int16_t freq_diff;
} T_APP_UAC_FREQ_INFO;

typedef struct
{
    int16_t pcm_left;
    int16_t pcm_right;
} T_PCM_SAMPLE_16BIT;

typedef struct
{
    T_APP_AUDIO_PIPE_TYPE type;
} T_APP_UAC_PIPE_MAPPING;

typedef struct
{
    uint16_t frame_duration;
    uint32_t time_diff_to_ap_fill_min;
    uint32_t time_diff_to_ap_fill_max;
    uint32_t time_diff_to_ap_asrc_min;
    uint32_t time_diff_to_ap_asrc_max;
} T_LEA_ISO_TIME_DIFF;

#if TARGET_LE_AUDIO_GAMING_DONGLE
static const T_LEA_ISO_TIME_DIFF lea_iso_time_diff[] =
{
    [NORMAL_MODE_2_5MS] = {
        .frame_duration = 2500,
        .time_diff_to_ap_fill_min = FILL_TIME_DIFF_MIN,
        .time_diff_to_ap_fill_max = FILL_TIME_DIFF_MAX,
        .time_diff_to_ap_asrc_min = ASRC_TIME_DIFF_MIN,
        .time_diff_to_ap_asrc_max = ASRC_TIME_DIFF_MAX,
    },

    [NORMAL_MODE_5MS]   = {
        .frame_duration = 5000,
        .time_diff_to_ap_fill_min = FILL_TIME_DIFF_MIN,
        .time_diff_to_ap_fill_max = FILL_TIME_DIFF_MAX,
        .time_diff_to_ap_asrc_min = ASRC_TIME_DIFF_MIN,
        .time_diff_to_ap_asrc_max = ASRC_TIME_DIFF_MAX,
    },

    [NORMAL_MODE_7_5MS] = {
        .frame_duration = 7500,
        .time_diff_to_ap_fill_min = FILL_TIME_DIFF_MIN,
        .time_diff_to_ap_fill_max = FILL_TIME_DIFF_MAX,
        .time_diff_to_ap_asrc_min = ASRC_TIME_DIFF_MIN,
        .time_diff_to_ap_asrc_max = ASRC_TIME_DIFF_MAX,
    },

    [NORMAL_MODE_10MS]  = {
        .frame_duration = 10000,
        .time_diff_to_ap_fill_min = FILL_TIME_DIFF_MIN,
        .time_diff_to_ap_fill_max = FILL_TIME_DIFF_MAX,
        .time_diff_to_ap_asrc_min = ASRC_TIME_DIFF_MIN,
        .time_diff_to_ap_asrc_max = ASRC_TIME_DIFF_MAX,
    },

    [HIRES_MODE_10MS]  = {
        .frame_duration = 10000,
        .time_diff_to_ap_fill_min = FILL_TIME_DIFF_MIN2,
        .time_diff_to_ap_fill_max = FILL_TIME_DIFF_MAX2,
        .time_diff_to_ap_asrc_min = ASRC_TIME_DIFF_MIN2,
        .time_diff_to_ap_asrc_max = ASRC_TIME_DIFF_MAX2,
    },
};
#endif

static const T_APP_UAC_PIPE_MAPPING uac_pipe_mapping[] =
{
    [USB_AUDIO_STREAM_LABEL_1] = {.type = AUDIO_PIPE_DOWNSTREAM_UAC1},
    [USB_AUDIO_STREAM_LABEL_2] = {.type = AUDIO_PIPE_DOWNSTREAM_UAC2},
};

#if TARGET_LE_AUDIO_GAMING_DONGLE && F_APP_HANDLE_DS_PIPE_IN_ISR
static const T_APP_ASRC_INFO asrc_info[] =
{
    [ASRC_NORMAL_STATE] = {0},
    [ASRC_SLOW_STATE]   = {-DS_PIPE_ASRC_RATIO},
    [ASRC_FAST_STATE]   = {DS_PIPE_ASRC_RATIO},
};

static const T_APP_UAC_FREQ_INFO uac_freq_info[] =
{
    [ASRC_NORMAL_STATE] = {0},
    [ASRC_SLOW_STATE]   = {30},
    [ASRC_FAST_STATE]   = {-30},
};
#endif

static T_USB_DS_INFO usb_ds_info =
{
    .pipe_info[0].dac_gain_percentage = 100,
#if ENABLE_UAC2
    .pipe_info[1].dac_gain_percentage = 100,
#endif
};
static T_RING_BUFFER usb_ds_codec_ring_buf;
static uint8_t *usb_ds_codec_buf;
static uint8_t *usb_ds_tmp_buf;
static uint8_t app_usb_ds_timer_id = 0;
static uint8_t timer_idx_streaming_detect = 0;
static uint8_t lc3_frame_num = GAMING_LC3_FRAME_NUM;
static uint8_t sbc_frame_num = GAMING_SBC_FRAME_NUM;

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
static uint16_t codec_data_len = 0;
#endif

#if F_APP_HANDLE_DS_PIPE_IN_ISR
#define A2DP_DETECT_INTERVAL    20

static uint8_t *usb_ds_sync_data_buf;
static T_RING_BUFFER usb_ds_sync_data_ring_buf;
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
#define TIME_DIFF_MONITOR_CNT   200

static uint8_t timer_idx_stop_cis_stream = 0;
#endif

static void streaming_detect_timer_start(void)
{
    if (timer_idx_streaming_detect == 0)
    {
        dongle_status.streaming_to_peer = true;
        app_gaming_sync_dongle_status();
    }

    app_start_timer(&timer_idx_streaming_detect,
                    "streaming_detect",
                    app_usb_ds_timer_id, TIMER_STREAMING_DETECT, 0, false,
                    DONGLE_STREAMING_DETECT_TIMEOUT);
}

static void dongle_streaming_detect(void)
{
    static uint32_t last_detect_time = 0;
    uint32_t curr_time = sys_timestamp_get();
    bool restart_timer = false;

    if (timer_idx_streaming_detect == 0)
    {
        /* dongle streaming start */
        last_detect_time = 0;
    }

    if (app_gaming_ctrl_check_timer_need_to_restart(curr_time, last_detect_time,
                                                    DONGLE_STREAMING_DETECT_INTERVAL))
    {
        restart_timer = true;
    }

    if (restart_timer)
    {
#if F_APP_HANDLE_DS_PIPE_IN_ISR
        T_IO_MSG io_msg;
        io_msg.type = IO_MSG_TYPE_DS_PCM_ENCODE;
        io_msg.subtype = DS_MSG_SUBTYPE_DS_STREAMING;
        app_io_msg_send(&io_msg);
#else
        streaming_detect_timer_start();
#endif

        last_detect_time = curr_time;
    }
}

#if F_APP_HANDLE_DS_PIPE_IN_ISR
void app_usb_ds_streaming_handle(void)
{
    streaming_detect_timer_start();

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (usb_ds_info.a2dp_ds_stream_state == A2DP_DS_STREAM_STATE_TRANSMITTING)
    {
        uint16_t sync_pkt_size = app_gaming_sync_get_data_len_for_streaming();

        app_gaming_set_sync_data_in_streaming();

        /* read gaming sync data */
        if (sync_pkt_size != 0)
        {
            if (ring_buffer_get_remaining_space(&usb_ds_sync_data_ring_buf) >= sync_pkt_size)
            {
                uint8_t *tmp = malloc(sync_pkt_size);

                if (tmp != NULL)
                {
                    app_gaming_sync_append_data_to_streaming(tmp, sync_pkt_size);

                    ring_buffer_write(&usb_ds_sync_data_ring_buf, tmp, sync_pkt_size);

                    free(tmp);
                }
            }
        }
    }
#endif
}
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
static void usb_feedback_callback(T_USB_AUDIO_FEEDBACK_STATE state)
{
    APP_PRINT_TRACE1("usb_feedback_callback: state %d", state);

    if (state != USB_FEEDBACK_DETECT)
    {
        if (usb_ds_info.create_pipe_after_feedback_detect)
        {
            usb_ds_info.create_pipe_after_feedback_detect = false;

            app_usb_ds_lea_pipe_create(&usb_ds_info.lea_codec_cfg);
        }
    }
}
#endif

static void lea_downstream_start(void)
{
    if (timer_idx_stop_cis_stream != 0)
    {
        app_stop_timer(&timer_idx_stop_cis_stream);
    }

    app_le_audio_set_downstream_active(true);
}

static void lea_downstream_stop(void)
{
    APP_PRINT_TRACE0("lea_downstream_stop");

    app_le_audio_set_downstream_active(false);
}

RAM_TEXT_SECTION static bool get_ref_ap_info(uint16_t conn_handle,
                                             T_BT_LE_ISO_SYNC_REF_AP_INFO *ap_info)
{
    bool ret = false;

    ap_info->dir = 1;
    ap_info->conn_handle = conn_handle;
    ret = bt_get_le_iso_sync_ref_ap_info(ap_info);

    return ret;
}

RAM_TEXT_SECTION static bool get_iso_time_info(T_APP_ISO_TIME_INFO *iso_time_info,
                                               uint16_t conn_handle)
{
    T_SYNCCLK_LATCH_INFO_TypeDef *p_latch_info = synclk_drv_time_get(SYNCCLK_ID4);
    uint32_t curr_clock = p_latch_info->exp_sync_clock;
    uint32_t next_ap = 0;
    T_BT_LE_ISO_SYNC_REF_AP_INFO ap_info;
    T_LE_AUDIO_CO_DB *p_audio_co_db = le_audio_get_audio_db();
    bool ret = false;

    if (get_ref_ap_info(conn_handle, &ap_info))
    {
        next_ap = ap_info.group_anchor_point;

        /* find next send ap */
        while (next_ap < curr_clock)
        {
            if ((0xFFFFFFFF - curr_clock + next_ap) <= ap_info.iso_interval_us)
            {
                break;
            }

            next_ap += ap_info.iso_interval_us;
        }

        if (next_ap >= curr_clock)
        {
            iso_time_info->time_diff_to_ap = next_ap - curr_clock;
        }
        else
        {
            iso_time_info->time_diff_to_ap = next_ap + 0xFFFFFFFF - curr_clock;
        }

        iso_time_info->last_ap = ap_info.group_anchor_point;
        iso_time_info->next_ap = next_ap;
        iso_time_info->iso_interval = ap_info.iso_interval_us;
        iso_time_info->last_send_seq = ap_info.sdu_seq_num;

        ret = true;
    }

    return ret;
}

RAM_TEXT_SECTION void handle_iso_data_send(T_ISO_CHNL_INFO_DB *iso_elem, uint8_t *data,
                                           uint16_t len, uint16_t pkt_seq, uint32_t time_diff_to_ap)
{
    uint8_t chnl_cnt = 0;
    T_SYNCCLK_LATCH_INFO_TypeDef *p_latch_info = synclk_drv_time_get(SYNCCLK_ID4);
    uint32_t timestamp = p_latch_info->exp_sync_clock;

    if (iso_elem != NULL)
    {
        if (iso_elem->codec_data.audio_channel_allocation == AUDIO_LOCATION_MONO)
        {
            chnl_cnt = 1;
        }
        else
        {
            chnl_cnt = __builtin_popcount(iso_elem->codec_data.audio_channel_allocation);
        }

        if (chnl_cnt == 2)
        {
            if (gap_iso_send_data(data, iso_elem->iso_conn_handle,
                                  len, true, timestamp, pkt_seq) == GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_TRACE4("handle_iso_data_send L/R: seq %d len %d time_diff_to_tp %d data %b",
                                 pkt_seq, len / 2,
                                 time_diff_to_ap, TRACE_BINARY(3, data));
            }
        }
        else // chnl_cnt == 1
        {
            if (iso_elem->codec_data.audio_channel_allocation & (AUDIO_LOCATION_FL | AUDIO_LOCATION_SIL))
            {
                if (gap_iso_send_data(data, iso_elem->iso_conn_handle,
                                      len / 2, true, timestamp, pkt_seq) == GAP_CAUSE_SUCCESS)
                {
                    APP_PRINT_TRACE4("handle_iso_data_send L: seq %d len %d time_diff_to_tp %d data %b",
                                     pkt_seq, len / 2,
                                     time_diff_to_ap, TRACE_BINARY(3, data));
                }
            }
            else
            {
                if (gap_iso_send_data(data + (len / 2), iso_elem->iso_conn_handle,
                                      len / 2, true, timestamp, pkt_seq) == GAP_CAUSE_SUCCESS)
                {
                    APP_PRINT_TRACE4("handle_iso_data_send R: seq %d len %d time_diff_to_tp %d data %b",
                                     pkt_seq, len / 2,
                                     time_diff_to_ap, TRACE_BINARY(3, data + len / 2));
                }
            }
        }
    }
}

RAM_TEXT_SECTION static bool iso_need_to_resync(T_APP_ISO_TIME_INFO *iso_time_info,
                                                uint16_t curr_send_seq)
{
    uint16_t last_send_seq = iso_time_info->last_send_seq;
    uint16_t seq_diff;

    if (iso_time_info->next_ap > iso_time_info->last_ap)
    {
        uint16_t ap_diff = 0;

        ap_diff = iso_time_info->next_ap - iso_time_info->last_ap;

        /* consider pkt send during last seq sending */
        if (ap_diff != iso_time_info->iso_interval)
        {
            last_send_seq += (ap_diff - iso_time_info->iso_interval) / iso_time_info->iso_interval;
        }
    }

    seq_diff = curr_send_seq - last_send_seq;

    if (seq_diff != 1)
    {
        APP_PRINT_TRACE2("iso_need_to_resync: curr_send_seq 0x%x last_send_seq 0x%x", curr_send_seq,
                         last_send_seq);

        return true;
    }

    return false;
}

static T_LEA_FRAME_DURATION_IDX get_lea_time_diff_idx(void)
{
    uint8_t frame_duration = usb_ds_info.lea_codec_cfg.frame_duration;
    T_LEA_FRAME_DURATION_IDX idx = NORMAL_MODE_7_5MS;

    if (frame_duration == FRAME_DURATION_CFG_7_5_MS)
    {
        idx = NORMAL_MODE_7_5MS;
    }
    else if (frame_duration == FRAME_DURATION_CFG_10_MS)
    {
        idx = NORMAL_MODE_10MS;
    }

    return idx;
}

#if F_APP_HANDLE_DS_PIPE_IN_ISR
static void downstream_asrc_state_machine(uint32_t time_diff_to_ap)
{
    static T_APP_ASRC_STATE pre_state;
    T_APP_ASRC_STATE new_state = pre_state;
    static uint8_t detect_cnt;
    T_LEA_FRAME_DURATION_IDX time_diff_idx = get_lea_time_diff_idx();
    uint16_t asrc_time_diff_min = lea_iso_time_diff[time_diff_idx].time_diff_to_ap_asrc_min;
    uint16_t asrc_time_diff_max = lea_iso_time_diff[time_diff_idx].time_diff_to_ap_asrc_max;
    int32_t new_asrc = 0;
    bool new_state_is_detect = false;

    switch (pre_state)
    {
    case ASRC_NORMAL_DETECT_STATE:
        {
            if (time_diff_to_ap > asrc_time_diff_max)
            {
                new_state = ASRC_FAST_DETECT_STATE;
            }
            else if (time_diff_to_ap < asrc_time_diff_min)
            {
                new_state = ASRC_SLOW_DETECT_STATE;
            }
            else
            {
                detect_cnt++;

                if (detect_cnt >= ASRC_DETECT_THRESHOLD)
                {
                    new_state = ASRC_NORMAL_STATE;
                }
            }
        }
        break;

    case ASRC_NORMAL_STATE:
        {
            if (time_diff_to_ap > asrc_time_diff_max)
            {
                new_state = ASRC_FAST_DETECT_STATE;
            }
            else if (time_diff_to_ap < asrc_time_diff_min)
            {
                new_state = ASRC_SLOW_DETECT_STATE;
            }
        }
        break;

    case ASRC_FAST_DETECT_STATE:
        {
            if (time_diff_to_ap > asrc_time_diff_max)
            {
                detect_cnt++;

                if (detect_cnt >= ASRC_DETECT_THRESHOLD)
                {
                    new_state = ASRC_FAST_STATE;
                }
            }
            else if (time_diff_to_ap < asrc_time_diff_min)
            {
                new_state = ASRC_SLOW_DETECT_STATE;
            }
            else
            {
                new_state = ASRC_NORMAL_DETECT_STATE;
            }
        }
        break;

    case ASRC_SLOW_DETECT_STATE:
        {
            if (time_diff_to_ap > asrc_time_diff_max)
            {
                new_state = ASRC_FAST_DETECT_STATE;
            }
            else if (time_diff_to_ap < asrc_time_diff_min)
            {
                detect_cnt++;

                if (detect_cnt >= ASRC_DETECT_THRESHOLD)
                {
                    new_state = ASRC_SLOW_STATE;
                }
            }
            else
            {
                new_state = ASRC_NORMAL_DETECT_STATE;
            }
        }
        break;

    case ASRC_FAST_STATE:
        {
            if (time_diff_to_ap < asrc_time_diff_min)
            {
                new_state = ASRC_SLOW_DETECT_STATE;
            }
            else if (time_diff_to_ap < asrc_time_diff_max)
            {
                new_state = ASRC_NORMAL_DETECT_STATE;
            }
        }
        break;

    case ASRC_SLOW_STATE:
        {
            if (time_diff_to_ap > asrc_time_diff_max)
            {
                new_state = ASRC_FAST_DETECT_STATE;
            }
            else if (time_diff_to_ap > asrc_time_diff_min)
            {
                new_state = ASRC_NORMAL_DETECT_STATE;
            }
        }
        break;
    }

    if (new_state == ASRC_SLOW_DETECT_STATE || new_state == ASRC_FAST_DETECT_STATE ||
        new_state == ASRC_NORMAL_DETECT_STATE)
    {
        new_state_is_detect = true;
    }

    if (pre_state != new_state || new_state_is_detect)
    {
        APP_PRINT_TRACE3("downstream_asrc_state_machine: time_diff %d state (%d->%d)",
                         time_diff_to_ap, pre_state, new_state);

        if (pre_state != new_state && new_state_is_detect)
        {
            detect_cnt = 0;
        }

        pre_state = new_state;
    }


    if (new_state_is_detect == false)
    {
#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
        if (app_usb_audio_get_feedback_state() == USB_FEEDBACK_SUPPORT)
        {
            app_usb_audio_sample_rate_adjust(uac_freq_info[new_state].freq_diff);
        }
        else
#endif
        {
            new_asrc = asrc_info[new_state].asrc_ratio;

            if (app_pipe_get_asrc_ratio(AUDIO_PIPE_DOWNSTREAM_UAC1) != new_asrc)
            {
                app_pipe_asrc_set(AUDIO_PIPE_DOWNSTREAM_UAC1, new_asrc);
            }
        }
    }
}
#endif

RAM_TEXT_SECTION static void send_audio_iso_data(uint8_t *data, uint16_t len)
{
    uint8_t i = 0;
    T_LE_AUDIO_CO_DB *p_audio_co_db = le_audio_get_audio_db();
    uint8_t curr_path_cnt = p_audio_co_db->input_path_queue.count;
    T_ISO_CHNL_INFO_DB *iso_elem = NULL;
    T_LEA_FRAME_DURATION_IDX time_diff_idx = get_lea_time_diff_idx();
    uint32_t min_fill_iso_time_diff = lea_iso_time_diff[time_diff_idx].time_diff_to_ap_fill_min;
    uint32_t max_fill_iso_time_diff = lea_iso_time_diff[time_diff_idx].time_diff_to_ap_fill_max;
    uint32_t time_diff_to_ap = 0;
    static uint16_t send_iso_seq = 0;
    uint8_t send_idx = 0;
    bool resync_iso = false;

    for (i = 0; i < curr_path_cnt; i++)
    {
        T_APP_ISO_TIME_INFO iso_time_info;
        uint8_t failed_cause = 0;

        iso_elem = (T_ISO_CHNL_INFO_DB *)os_queue_peek(&p_audio_co_db->input_path_queue, i);

        if (iso_elem != NULL)
        {
            if (get_iso_time_info(&iso_time_info, iso_elem->iso_conn_handle) == false)
            {
                failed_cause = 1;
            }
            else if (iso_elem->first_iso_is_send == false)
            {
                if (iso_time_info.time_diff_to_ap < min_fill_iso_time_diff ||
                    iso_time_info.time_diff_to_ap > max_fill_iso_time_diff)
                {
                    failed_cause = 2;

                    usb_ds_info.skip_pcm_interval = PCM_DS_INTERVAL;
                }
            }
            else if (iso_elem->first_iso_is_send)
            {
                if (iso_need_to_resync(&iso_time_info, send_iso_seq))
                {
                    APP_PRINT_TRACE1("iso resync: %d", i);

                    resync_iso = true;
                    failed_cause = 3;
                    break;
                }
            }

            time_diff_to_ap = iso_time_info.time_diff_to_ap;

            if (failed_cause == 0)
            {
                send_idx |= (1 << i);
            }
            else
            {
                APP_PRINT_TRACE1("send_audio_iso_data failed: cause %d", failed_cause);
            }
        }
    }

    if (resync_iso)
    {
        send_idx = 0;
        send_iso_seq++;

        for (i = 0; i < curr_path_cnt; i++)
        {
            iso_elem = (T_ISO_CHNL_INFO_DB *)os_queue_peek(&p_audio_co_db->input_path_queue, i);

            if (iso_elem != NULL)
            {
                iso_elem->first_iso_is_send = false;
            }
        }
    }

    if (send_idx != 0)
    {
        for (i = 0; i < curr_path_cnt; i++)
        {
            if (send_idx & (1 << i))
            {
                iso_elem = (T_ISO_CHNL_INFO_DB *)os_queue_peek(&p_audio_co_db->input_path_queue, i);

                if (iso_elem != NULL)
                {
                    handle_iso_data_send(iso_elem, data, len, send_iso_seq, time_diff_to_ap);
                    iso_elem->first_iso_is_send = true;
                }
            }
        }

        send_iso_seq++;

        dongle_streaming_detect();
#if F_APP_HANDLE_DS_PIPE_IN_ISR
        downstream_asrc_state_machine(time_diff_to_ap);
#endif
    }
}
#endif

static void usb_ds_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
#if TARGET_LE_AUDIO_GAMING_DONGLE
    case TIMER_STOP_CIS_STREAM:
        {
            app_stop_timer(&timer_idx_stop_cis_stream);

            lea_downstream_stop();
        }
        break;
#endif

    case TIMER_STREAMING_DETECT:
        {
            app_stop_timer(&timer_idx_streaming_detect);

            dongle_status.streaming_to_peer = false;
            app_gaming_sync_dongle_status();
        }
        break;

    default:
        break;
    }
}

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
static void update_a2dp_stream_state(T_APP_A2DP_DS_STREAM_STATE state)
{
    T_APP_A2DP_DS_STREAM_STATE pre_state = usb_ds_info.a2dp_ds_stream_state;

    usb_ds_info.a2dp_ds_stream_state = state;

    if (pre_state != state)
    {
        APP_PRINT_TRACE2("update_a2dp_stream_state: (%d -> %d)", pre_state, state);
    }
}

static uint8_t get_sbc_subbands(uint8_t subband_mask)
{
    uint8_t sbs = 0;

    switch (subband_mask)
    {
    case 0x08:
        sbs = 4;
        break;

    case 0x04:
        sbs = 8;
        break;

    default:
        APP_PRINT_WARN1("get_sbc_subbands: unknown mask %02x", subband_mask);
        break;
    }

    return sbs;
}

static uint8_t get_sbc_block_length(uint8_t bl_mask)
{
    uint8_t bl = 0;

    switch (bl_mask)
    {
    case 0x80:
        bl = 4;
        break;

    case 0x40:
        bl = 8;
        break;

    case 0x20:
        bl = 12;
        break;

    case 0x10:
        bl = 16;
        break;

    default:
        APP_PRINT_WARN1("get_sbc_block_length: unknown mask %02x", bl_mask);
        break;
    }

    return bl;
}

static uint16_t get_lc3_sample_rate(uint8_t sample_freq)
{
    uint16_t sample_rate = 0;

    switch (sample_freq)
    {
    case BT_A2DP_LC3_SAMPLING_FREQUENCY_8KHZ:
        sample_rate = 8000;
        break;
    case BT_A2DP_LC3_SAMPLING_FREQUENCY_16KHZ:
        sample_rate = 16000;
        break;
    case BT_A2DP_LC3_SAMPLING_FREQUENCY_24KHZ:
        sample_rate = 24000;
        break;
    case BT_A2DP_LC3_SAMPLING_FREQUENCY_32KHZ:
        sample_rate = 32000;
        break;
    case BT_A2DP_LC3_SAMPLING_FREQUENCY_44_1KHZ:
        sample_rate = 44100;
        break;
    case BT_A2DP_LC3_SAMPLING_FREQUENCY_48KHZ:
        sample_rate = 48000;
        break;
    default:
        break;
    }

    return sample_rate;
}

static uint16_t get_lc3_frame_duration(uint8_t duration)
{
    uint16_t frame_duration = 0;

    switch (duration)
    {
    case BT_A2DP_LC3_FRAME_DURATION_7_5MS:
        frame_duration = 7500;
        break;

    case BT_A2DP_LC3_FRAME_DURATION_10MS:
        frame_duration = 10000;
        break;

    default:
        break;
    }

    return frame_duration;
}

static void send_a2dp_data(void)
{
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    uint8_t *remote_addr = app_cfg_nv.public_bud_addr;
#else
    uint8_t *remote_addr = sink_dev_list[0].remote_info.bd_addr;
#endif
    uint8_t cause = 0;
    uint16_t a2dp_pkt_size = 0;
    uint8_t frame_num = 0;
    uint16_t remove_size = 0;
    bool send_a2dp_ret = false;
    uint16_t a2dp_seq = usb_ds_info.a2dp_seq;
    uint16_t sync_pkt_size = 0;

    if (usb_ds_info.a2dp_connected == false)
    {
        cause = 2;
        goto exit;
    }

    if (usb_ds_info.a2dp_suspend)
    {
        cause = 3;
        goto exit;
    }

    if (usb_ds_info.a2dp_opened == false)
    {
        cause = 4;
        goto exit;
    }

    if (usb_ds_info.a2dp_ds_stream_state == A2DP_DS_STREAM_STATE_IDLE)
    {
        bt_a2dp_stream_start_req(remote_addr);
        update_a2dp_stream_state(A2DP_DS_STREAM_STATE_STARTING);
    }

    if (usb_ds_info.a2dp_ds_stream_state != A2DP_DS_STREAM_STATE_STARTED &&
        usb_ds_info.a2dp_ds_stream_state != A2DP_DS_STREAM_STATE_TRANSMITTING)
    {
        cause = 5;
        goto exit;
    }

    if (usb_ds_info.a2dp_cfg.codec_type == BT_A2DP_CODEC_TYPE_SBC)
    {
        frame_num = sbc_frame_num;
    }
    else
    {
        frame_num = lc3_frame_num;
    }

    a2dp_pkt_size = frame_num * codec_data_len;

    if (ring_buffer_get_data_count(&usb_ds_codec_ring_buf) < a2dp_pkt_size)
    {
        cause = 6;
        goto exit;
    }

    if (usb_ds_info.a2dp_ds_stream_state != A2DP_DS_STREAM_STATE_TRANSMITTING)
    {
        /* first send: only keep latest a2dp to shortest latency */
        remove_size = ring_buffer_get_data_count(&usb_ds_codec_ring_buf) -
                      a2dp_pkt_size;

        if (remove_size)
        {
            ring_buffer_remove(&usb_ds_codec_ring_buf, a2dp_pkt_size);
        }
    }

    ring_buffer_read(&usb_ds_codec_ring_buf, a2dp_pkt_size, usb_ds_tmp_buf);

#if F_APP_HANDLE_DS_PIPE_IN_ISR
    sync_pkt_size = ring_buffer_get_data_count(&usb_ds_sync_data_ring_buf);

    if (sync_pkt_size > 0 && ((sync_pkt_size + a2dp_pkt_size) <= USB_DS_TMP_BUF_SIZE))
    {
        ring_buffer_read(&usb_ds_sync_data_ring_buf, sync_pkt_size, usb_ds_tmp_buf + a2dp_pkt_size);
        a2dp_pkt_size += sync_pkt_size;
    }
#else
    if (app_gaming_set_sync_data_in_streaming())
    {
        sync_pkt_size = app_gaming_sync_get_data_len_for_streaming();

        if (sync_pkt_size != 0)
        {
            app_gaming_sync_append_data_to_streaming(usb_ds_tmp_buf + a2dp_pkt_size, sync_pkt_size);
            a2dp_pkt_size += sync_pkt_size;
        }
    }
#endif

    send_a2dp_ret = bt_a2dp_stream_data_send(remote_addr, a2dp_seq,
                                             usb_ds_info.a2dp_timestamp,
                                             frame_num, usb_ds_tmp_buf, a2dp_pkt_size);

    dongle_streaming_detect();

    update_a2dp_stream_state(A2DP_DS_STREAM_STATE_TRANSMITTING);
    usb_ds_info.a2dp_seq++;

    if (usb_ds_info.a2dp_cfg.codec_type == BT_A2DP_CODEC_TYPE_SBC)
    {
        uint8_t subbands = get_sbc_subbands(usb_ds_info.a2dp_cfg.codec_info.sbc.subbands);
        uint16_t block_length = get_sbc_block_length(usb_ds_info.a2dp_cfg.codec_info.sbc.block_length);

        usb_ds_info.a2dp_timestamp += frame_num * subbands * block_length;
    }
    else if (usb_ds_info.a2dp_cfg.codec_type == BT_A2DP_CODEC_TYPE_LC3)
    {
        uint16_t sample_rate = get_lc3_sample_rate(usb_ds_info.a2dp_cfg.codec_info.lc3.sampling_frequency);
        uint16_t frame_duration = get_lc3_frame_duration(
                                      usb_ds_info.a2dp_cfg.codec_info.lc3.frame_duration);

        usb_ds_info.a2dp_timestamp += frame_num * sample_rate * frame_duration / 1000 / 1000;
    }

exit:
    APP_PRINT_TRACE5("send_a2dp_data: cause %d a2dp_seq %d size %d send_a2dp_ret %d timestamp %d",
                     cause,
                     a2dp_seq, a2dp_pkt_size,
                     send_a2dp_ret, usb_ds_info.a2dp_timestamp);
}

static void handle_a2dp_data_send(uint8_t *buf, uint16_t len)
{
    if (ring_buffer_get_remaining_space(&usb_ds_codec_ring_buf) < len)
    {
        ring_buffer_remove(&usb_ds_codec_ring_buf, len);
    }

    ring_buffer_write(&usb_ds_codec_ring_buf, buf, len);

    send_a2dp_data();
}

void app_usb_ds_handle_a2dp_suspend(bool suspend)
{
    APP_PRINT_TRACE1("app_usb_ds_handle_a2dp_suspend: suspend %d", suspend);

    if (suspend)
    {
        usb_ds_info.a2dp_suspend = true;

        if (usb_ds_info.a2dp_ds_stream_state != A2DP_DS_STREAM_STATE_IDLE)
        {
            app_usb_ds_handle_stream_stop();
        }
    }
    else
    {
        usb_ds_info.a2dp_suspend = false;
    }
}

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
bool app_usb_ds_is_a2dp_stream(void)
{
    return (usb_ds_info.a2dp_ds_stream_state == A2DP_DS_STREAM_STATE_TRANSMITTING);
}
#endif
#endif

static bool usb_ds_handle_gain_set(T_DS_PIPE_INFO *pipe_info, T_UAC_LABEL uac_label, uint16_t gain)
{
    uint16_t pipe_dac_gain = 0;
    bool ret = false;

    if (pipe_info->spk_mute)
    {
        gain = UAC_SPK_VOL_MIN;
    }

    /* 0x00~0x7f : for avrcp bt volume */
    uint16_t vol_level = app_usb_audio_spk_vol_transform(gain);

    if (pipe_info->dac_gain_percentage != 100)
    {
        vol_level = vol_level * pipe_info->dac_gain_percentage / 100;
    }

    if (uac_get_pipe_dac_gain(vol_level, &pipe_dac_gain))
    {
        ret = app_pipe_gain_set(uac_pipe_mapping[uac_label].type, pipe_dac_gain);
    }

    APP_PRINT_TRACE3("app_usb_ds_handle_gain_set: vol_level %d gain 0x%04x pipe_dac_gain 0x%04x",
                     vol_level, gain, pipe_dac_gain);

    return ret;
}

#if F_APP_SUPPORT_PCM_MIXING_FOR_ONE_CIS && (UAC_SPK_BIT_RES == UAC_BIT_RES_16)
RAM_TEXT_SECTION static void app_usb_ds_mixing_pcm(uint8_t *data, uint16_t len)
{
    uint16_t i = 0;
    T_PCM_SAMPLE_16BIT *pcm_sample = NULL;
    int32_t pcm_tmp;

    for (i = 0; i < len; i += sizeof(T_PCM_SAMPLE_16BIT))
    {
        pcm_sample = (T_PCM_SAMPLE_16BIT *)&data[i];

        pcm_tmp = (pcm_sample->pcm_left + pcm_sample->pcm_right) / 2;

        pcm_sample->pcm_left = pcm_sample->pcm_right = pcm_tmp;
    }
}
#endif

RAM_TEXT_SECTION bool app_usb_ds_rcv_pcm(uint8_t *data, uint16_t len, T_UAC_LABEL uac_label)
{
    T_DS_PIPE_INFO *pipe_info = &usb_ds_info.pipe_info[uac_label];
    uint16_t pre_pcm_size = ring_buffer_get_data_count(&pipe_info->pcm_ring_buf);
    static uint32_t pcm_cnt[USB_AUDIO_STREAM_NUM] = {0};

    pcm_cnt[uac_label]++;

    if (app_pipe_is_ready(AUDIO_PIPE_DOWNSTREAM_UAC1) == false
#if ENABLE_UAC2
        || app_pipe_is_ready(AUDIO_PIPE_DOWNSTREAM_UAC2) == false
#endif
       )
    {
        if (pcm_cnt[uac_label] % 100 == 0)
        {
            APP_PRINT_ERROR0("app_usb_ds_rcv_pcm: pipe not ready!");
        }

        return false;
    }

    if (usb_ds_info.skip_pcm_interval > 0)
    {
        APP_PRINT_ERROR1("app_usb_ds_rcv_pcm: skip pcm %d", usb_ds_info.skip_pcm_interval);

        if (usb_ds_info.skip_pcm_interval > PCM_DS_INTERVAL)
        {
            usb_ds_info.skip_pcm_interval -= PCM_DS_INTERVAL;
        }
        else
        {
            usb_ds_info.skip_pcm_interval = 0;
        }

        return false;
    }

#if F_APP_SUPPORT_PCM_MIXING_FOR_ONE_CIS && (UAC_SPK_BIT_RES == UAC_BIT_RES_16)
    T_LE_AUDIO_CO_DB *p_audio_co_db = le_audio_get_audio_db();

    if (p_audio_co_db->input_path_queue.count == 1)
    {
        /* mixing left and right when only one buds connected */
        app_usb_ds_mixing_pcm(data, len);
    }
#endif

    if (len != usb_ds_info.pcm_bytes_per_interval)
    {
        APP_PRINT_TRACE2("app_usb_ds_rcv_pcm: len not match %d pcm_bytes_per_interval %d", len,
                         usb_ds_info.pcm_bytes_per_interval);
    }

    if (ring_buffer_get_remaining_space(&pipe_info->pcm_ring_buf) < len)
    {
        APP_PRINT_ERROR0("app_usb_ds_rcv_pcm: buf full");
        ring_buffer_remove(&pipe_info->pcm_ring_buf, len);
    }

    ring_buffer_write(&pipe_info->pcm_ring_buf, data, len);

    if (pcm_cnt[uac_label] % 500 == 0)
    {
        APP_PRINT_TRACE6("app_usb_ds_rcv_pcm: uac_label %d %d %d len %d (%d->%d)",
                         uac_label,
                         usb_ds_info.increase_usb_pcm_input, usb_ds_info.decrease_usb_pcm_input, len,
                         pre_pcm_size, ring_buffer_get_data_count(&pipe_info->pcm_ring_buf));
    }

#if F_APP_HANDLE_DS_PIPE_IN_ISR
    app_usb_ds_pcm_encode();
#else
    T_IO_MSG io_msg;

    io_msg.type = IO_MSG_TYPE_DS_PCM_ENCODE;
    io_msg.subtype = DS_MSG_SUBTYPE_PCM_DECODE;

    app_io_msg_send(&io_msg);
#endif

    return true;
}

RAM_TEXT_SECTION void app_usb_ds_pcm_encode(void)
{
    uint8_t i = 0;
    T_DS_PIPE_INFO *pipe_info = NULL;
    uint16_t pre_pcm_size = 0;
    uint32_t lock = 0;
    uint8_t loop_cnt = 0;

    lock = os_lock();

#if ENABLE_UAC2
    bool fill_pcm = true;

    if (app_usb_get_uac_streaming_state(USB_AUDIO_STREAM_LABEL_1) &&
        app_usb_get_uac_streaming_state(USB_AUDIO_STREAM_LABEL_2))
    {
        for (i = 0; i < USB_DS_PIPE_NUM; i++)
        {
            pipe_info = &usb_ds_info.pipe_info[i];

            if (ring_buffer_get_data_count(&pipe_info->pcm_ring_buf) < pipe_info->fill_pcm_size)
            {
                fill_pcm = false;
                break;
            }
        }
    }
    else if (app_usb_get_uac_streaming_state(USB_AUDIO_STREAM_LABEL_1))
    {
        if (ring_buffer_get_data_count(&usb_ds_info.pipe_info[0].pcm_ring_buf) <
            usb_ds_info.pipe_info[0].fill_pcm_size)
        {
            fill_pcm = false;
        }
    }
    else if (app_usb_get_uac_streaming_state(USB_AUDIO_STREAM_LABEL_2))
    {
        if (ring_buffer_get_data_count(&usb_ds_info.pipe_info[1].pcm_ring_buf) <
            usb_ds_info.pipe_info[0].fill_pcm_size)
        {
            fill_pcm = false;
        }
    }
    else
    {
        fill_pcm = false;
    }

    if (fill_pcm == false)
    {
        goto exit;
    }
#endif

    for (i = 0; i < USB_DS_PIPE_NUM; i++)
    {
        bool fill_result = false;
        bool (*fill_func)(T_APP_AUDIO_PIPE_TYPE type, void *buf, uint16_t len);

        pipe_info = &usb_ds_info.pipe_info[i];

#if F_APP_HANDLE_DS_PIPE_IN_ISR
        fill_func = app_pipe_fill_in_isr;
#else
        fill_func = app_pipe_fill;
#endif

        do
        {
            loop_cnt++;

            pre_pcm_size = ring_buffer_get_data_count(&pipe_info->pcm_ring_buf);

            if (pre_pcm_size >= pipe_info->fill_pcm_size)
            {
                ring_buffer_peek(&pipe_info->pcm_ring_buf, pipe_info->fill_pcm_size, usb_ds_tmp_buf);

                fill_result = fill_func(i == 0 ? AUDIO_PIPE_DOWNSTREAM_UAC1 : AUDIO_PIPE_DOWNSTREAM_UAC2,
                                        usb_ds_tmp_buf, pipe_info->fill_pcm_size);

                if (fill_result)
                {
                    ring_buffer_remove(&pipe_info->pcm_ring_buf, pipe_info->fill_pcm_size);
                }
            }
#if ENABLE_UAC2
            else
            {
                memset(usb_ds_tmp_buf, 0, USB_DS_TMP_BUF_SIZE);

                fill_func(i == 0 ? AUDIO_PIPE_DOWNSTREAM_UAC1 : AUDIO_PIPE_DOWNSTREAM_UAC2,
                          usb_ds_tmp_buf, pipe_info->fill_pcm_size);
            }
#endif

#if DOWNSTREAM_DEBUG
            APP_PRINT_TRACE7("app_usb_ds_pcm_encode: loop_cnt %d i %d (%d -> %d) fill_result %d mixing_pipe %d fill_pcm_size %d",
                             loop_cnt, i,
                             pre_pcm_size,
                             ring_buffer_get_data_count(&pipe_info->pcm_ring_buf), fill_result, usb_ds_info.mixing_pipe,
                             pipe_info->fill_pcm_size);
#endif
        }
        while ((ring_buffer_get_data_count(&pipe_info->pcm_ring_buf) >= pipe_info->fill_pcm_size) &&
               (fill_result == true));
    }

#if ENABLE_UAC2
exit:
#endif
    os_unlock(lock);

    return;
}

void app_usb_ds_handle_stream_start(void)
{
    APP_PRINT_TRACE0("app_usb_ds_handle_stream_start");

#if TARGET_LE_AUDIO_GAMING_DONGLE
    lea_downstream_start();
#endif
}

void app_usb_ds_handle_stream_stop(void)
{
    uint8_t i = 0;
    T_DS_PIPE_INFO *pipe_info = NULL;
    uint16_t remain_codec_size = 0;
    uint16_t remain_pcm_size = 0;

    APP_PRINT_TRACE0("app_usb_ds_handle_stream_stop");

#if TARGET_LEGACY_GAMING_DONGLE
    bt_a2dp_stream_suspend_req(sink_dev_list[0].remote_info.bd_addr);
    update_a2dp_stream_state(A2DP_DS_STREAM_STATE_IDLE);
#elif F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    bt_a2dp_stream_suspend_req(app_cfg_nv.public_bud_addr);
    update_a2dp_stream_state(A2DP_DS_STREAM_STATE_IDLE);
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
    uint32_t timeout = 10000;

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    timeout = 100;
#endif

    app_start_timer(&timer_idx_stop_cis_stream, "stop_cis_stream",
                    app_usb_ds_timer_id, TIMER_STOP_CIS_STREAM,
                    0, false, timeout);
#endif

    usb_ds_info.fill_time_fitting_to_ap = false;

    remain_codec_size = ring_buffer_get_data_count(&usb_ds_codec_ring_buf);

    if (remain_codec_size)
    {
        ring_buffer_remove(&usb_ds_codec_ring_buf, remain_codec_size);
    }

    for (i = 0; i < USB_DS_PIPE_NUM; i++)
    {
        pipe_info = &usb_ds_info.pipe_info[i];

        remain_pcm_size = ring_buffer_get_data_count(&pipe_info->pcm_ring_buf);

        if (remain_pcm_size > 0)
        {
            ring_buffer_remove(&pipe_info->pcm_ring_buf, remain_pcm_size);
        }
    }
}

bool app_usb_ds_mute_set(T_UAC_LABEL uac_label, bool mute)
{
    bool ret = false;
    T_DS_PIPE_INFO *pipe_info = &usb_ds_info.pipe_info[uac_label];

    pipe_info->spk_mute = mute;

    ret = usb_ds_handle_gain_set(pipe_info, uac_label, pipe_info->dac_gain);

    return ret;
}

bool app_usb_ds_gain_set(T_UAC_LABEL uac_label, uint16_t gain)
{
    bool ret = false;
    T_DS_PIPE_INFO *pipe_info = &usb_ds_info.pipe_info[uac_label];

    pipe_info->dac_gain = gain;

    ret = usb_ds_handle_gain_set(pipe_info, uac_label, gain);

    return ret;
}

#if ENABLE_UAC2
void app_usb_ds_set_gain_percentage(uint8_t uac1_vol_percent, uint8_t uac2_vol_percent)
{
    T_DS_PIPE_INFO *pipe_info = NULL;

    usb_ds_info.pipe_info[0].dac_gain_percentage = uac1_vol_percent;
    usb_ds_info.pipe_info[1].dac_gain_percentage = uac2_vol_percent;

    APP_PRINT_TRACE2("app_usb_ds_set_gain_percentage: uac1_vol_percent %d uac2_vol_percent %d",
                     uac1_vol_percent, uac2_vol_percent);

    pipe_info = &usb_ds_info.pipe_info[0];
    usb_ds_handle_gain_set(pipe_info, USB_AUDIO_STREAM_LABEL_1, pipe_info->dac_gain);

    pipe_info = &usb_ds_info.pipe_info[1];
    usb_ds_handle_gain_set(pipe_info, USB_AUDIO_STREAM_LABEL_2, pipe_info->dac_gain);
}
#endif

static bool downstream_pipe_data_ind(uint8_t *data, uint16_t len)
{
#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    codec_data_len = len;
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
    bool send_by_lea = true;

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (adapter_a2dp_is_connected())
    {
        send_by_lea = false;
    }
#endif

    if (send_by_lea)
    {
        send_audio_iso_data(data, len);
    }
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    else
    {
        handle_a2dp_data_send(data, len);
    }
#endif
#else
    handle_a2dp_data_send(data, len);
#endif

    return true;
}

static bool downstream_pipe_create(T_GAMING_CODEC codec, bool asrc_toggle)
{
    T_APP_PIPE_PARAM param = {0};

#if F_APP_USB_AUDIO_96K_24BIT && (F_APP_USB_HIGH_SPEED_0_5MS || F_APP_PCM_SPLIT_0_5MS)
    param.src_codec = PCM_96K_24BIT_STEREO_0_5MS;
#else

#if USB_AUDIO_DS_INTERVAL == 2
#if UAC_SPK_BIT_RES == UAC1_BIT_RES_24
    param.src_codec = PCM_48K_24BIT_STEREO_2MS;
#else
    param.src_codec = PCM_48K_16BIT_STEREO_2MS;
#endif

#elif USB_AUDIO_DS_INTERVAL == 1

#if F_APP_PCM_SPLIT_0_5MS
#if UAC_SPK_BIT_RES == UAC1_BIT_RES_24
    param.src_codec = PCM_48K_24BIT_STEREO_0_5MS;
#else
    param.src_codec = PCM_48K_16BIT_STEREO_0_5MS;
#endif
#else
#if UAC_SPK_BIT_RES == UAC1_BIT_RES_24
    param.src_codec = PCM_48K_24BIT_STEREO_1MS;
#else
    param.src_codec = PCM_48K_16BIT_STEREO_1MS;
#endif
#endif

#else
#error "not defined"
#endif

#endif

    param.snk_codec = codec;
    param.data_cback = downstream_pipe_data_ind;
    param.asrc_toggle = asrc_toggle;
    param.event_cback = NULL;
    /* crb-13 dsp not support ultra low latency mode w/o ack yet */
    param.mode = AUDIO_STREAM_MODE_DIRECT;

#if F_APP_HANDLE_DS_PIPE_IN_ISR
    param.handle_data_in_isr = true;
#endif

#if ENABLE_UAC2
    param.is_mixed_pipe = true;
#endif
    app_pipe_create(AUDIO_PIPE_DOWNSTREAM_UAC1, &param);
    usb_ds_info.pipe_info[0].fill_pcm_size = app_pipe_get_codec_frame_size(param.src_codec);

#if ENABLE_UAC2
    param.is_mixed_pipe = true;

    app_pipe_create(AUDIO_PIPE_DOWNSTREAM_UAC2, &param);
    usb_ds_info.pipe_info[1].fill_pcm_size = app_pipe_get_codec_frame_size(param.src_codec);
#endif

    APP_PRINT_TRACE3("downstream_pipe_create: src %d snk %d pcm_size %d", param.src_codec,
                     param.snk_codec, usb_ds_info.pipe_info[0].fill_pcm_size);

    return true;
}

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
void app_usb_ds_legacy_pipe_create(T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL cfg)
{
    T_GAMING_CODEC codec;
    bool ret = false;

    memcpy(&usb_ds_info.a2dp_cfg, &cfg, sizeof(T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL));

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    usb_ds_info.downstream_mode = DOWNSTREAM_MODE_LEGACY;
#endif

    if (cfg.codec_type == BT_A2DP_CODEC_TYPE_SBC)
    {
        codec = SBC_48K_16BIT_STEREO;
        usb_ds_info.a2dp_frame_num = 2;
    }
    else
    {
        codec = LC3_48K_16BIT_STEREO_10MS;
        usb_ds_info.a2dp_frame_num = 1;
    }
    usb_ds_info.codec_type = codec;

    ret = downstream_pipe_create(codec, false);

    APP_PRINT_TRACE2("app_usb_ds_legacy_pipe_create: type %d ret %d", cfg.codec_type, ret);
}
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
void app_usb_ds_lea_pipe_create(T_CODEC_CFG *p_lea_codec)
{
    T_GAMING_CODEC snk_codec;
    bool ret = false;
    uint8_t cause = 0;
    bool asrc_toggle = false;

    memcpy(&usb_ds_info.lea_codec_cfg, p_lea_codec, sizeof(T_CODEC_CFG));

    if (p_lea_codec->frame_duration == FRAME_DURATION_CFG_10_MS)
    {
        snk_codec = LC3_48K_16BIT_STEREO_10MS;
    }
    else if (p_lea_codec->frame_duration == FRAME_DURATION_CFG_7_5_MS)
    {
        snk_codec = LC3_48K_16BIT_STEREO_7_5MS;
    }
    else
    {
        cause = 1;
        goto exit;
    }

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
    if (app_usb_audio_get_feedback_state() == USB_FEEDBACK_DETECT)
    {
        usb_ds_info.create_pipe_after_feedback_detect = true;
        cause = 2;
        goto exit;
    }
    else if (app_usb_audio_get_feedback_state() == USB_FEEDBACK_SUPPORT)
    {
        asrc_toggle = false;
    }
    else
#endif
    {
        asrc_toggle = true;
    }

    ret = downstream_pipe_create(snk_codec, asrc_toggle);

    usb_ds_info.codec_type = snk_codec;

exit:
    APP_PRINT_TRACE5("app_usb_ds_lea_pipe_create: %d %d %d ret %d cause %d",
                     p_lea_codec->frame_duration,
                     p_lea_codec->sample_frequency,
                     p_lea_codec->octets_per_codec_frame, ret, cause);
}
#endif

void app_usb_ds_pipe_release(void)
{
    usb_ds_info.create_pipe_after_feedback_detect = false;

    app_pipe_release(AUDIO_PIPE_DOWNSTREAM_UAC1);

#if ENABLE_UAC2
    app_pipe_release(AUDIO_PIPE_DOWNSTREAM_UAC2);
#endif
}

static void app_usb_ds_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = false;

    switch (event_type)
    {
#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    case BT_EVENT_A2DP_CONN_CMPL:
        {
            usb_ds_info.a2dp_connected = true;
            handle = true;
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            usb_ds_info.a2dp_opened = true;
            handle = true;
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            /* due to a2dp will be send in isr, we need to clean timestamp before stream state started */
            uint32_t s = os_lock();

            usb_ds_info.a2dp_seq = 0;
            usb_ds_info.a2dp_timestamp = 0;
            update_a2dp_stream_state(A2DP_DS_STREAM_STATE_STARTED);

            os_unlock(s);

            uint16_t remain_codec_size = ring_buffer_get_data_count(&usb_ds_codec_ring_buf);

            if (remain_codec_size)
            {
                ring_buffer_remove(&usb_ds_codec_ring_buf, remain_codec_size);
            }

            handle = true;
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
    case BT_EVENT_A2DP_STREAM_STOP:
    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            uint32_t s = os_lock();

            update_a2dp_stream_state(A2DP_DS_STREAM_STATE_IDLE);
            usb_ds_info.a2dp_seq = 0;
            usb_ds_info.a2dp_timestamp = 0;

            os_unlock(s);

            if (event_type == BT_EVENT_A2DP_STREAM_CLOSE)
            {
                usb_ds_info.a2dp_opened = false;
            }

            if (event_type == BT_EVENT_A2DP_DISCONN_CMPL)
            {
                usb_ds_info.a2dp_connected = false;
                usb_ds_info.a2dp_opened = false;
                usb_ds_info.a2dp_suspend = false;
            }

            handle = true;
        }
        break;
#endif

    default:
        break;
    }

    if (handle)
    {
        APP_PRINT_TRACE1("app_usb_ds_bt_cback: event 0x%04x", event_type);
    }
}

void app_usb_ds_set_sbc_frame_num(uint8_t frame_num)
{
    sbc_frame_num = frame_num;
}

uint8_t app_usb_ds_get_sbc_frame_num(void)
{
    return sbc_frame_num;
}

void app_usb_ds_set_lc3_frame_num(uint8_t frame_num)
{
    lc3_frame_num = frame_num;
}

uint8_t app_usb_ds_get_lc3_frame_num(void)
{
    return lc3_frame_num;
}

void app_usb_ds_init(void)
{
    uint8_t i = 0;
    T_DS_PIPE_INFO *pipe_info = NULL;
    uint32_t sample_freq = USB_AUDIO_DS_SAMPLE_RATE;

#if F_APP_HANDLE_DS_PIPE_IN_ISR
    if (usb_ds_sync_data_buf == NULL)
    {
        usb_ds_sync_data_buf = malloc(USB_DS_SYNC_DATA_BUF_SIZE);

        ring_buffer_init(&usb_ds_sync_data_ring_buf, usb_ds_sync_data_buf, USB_DS_SYNC_DATA_BUF_SIZE);
    }
#endif

    if (usb_ds_codec_buf == NULL)
    {
        usb_ds_codec_buf = malloc(USB_DS_CODEC_BUF_SIZE);

        ring_buffer_init(&usb_ds_codec_ring_buf, usb_ds_codec_buf, USB_DS_CODEC_BUF_SIZE);
    }

    for (i = 0; i < USB_DS_PIPE_NUM; i++)
    {
        pipe_info = &usb_ds_info.pipe_info[i];

        if (pipe_info->pcm_buf == NULL)
        {
            pipe_info->pcm_buf = malloc(USB_DS_PCM_BUF_SIZE);

            ring_buffer_init(&pipe_info->pcm_ring_buf, pipe_info->pcm_buf, USB_DS_PCM_BUF_SIZE);
        }
    }

    if (usb_ds_tmp_buf == NULL)
    {
        usb_ds_tmp_buf = malloc(USB_DS_TMP_BUF_SIZE);
    }

    uint8_t uac_spk_bit_res = UAC_SPK_BIT_RES;

#if F_APP_USB_GIP_SUPPORT
    if (app_gip_get_switch_mode())
    {
        uac_spk_bit_res = UAC1_SPK_BIT_RES_GIP;
    }
#endif

    usb_ds_info.pcm_bytes_per_interval = PCM_DS_INTERVAL * (sample_freq / 1000) *
                                         (uac_spk_bit_res / 8) * UAC_SPK_CHAN_NUM / 1000;

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
    app_usb_audio_reg_feedback_state_cb(usb_feedback_callback);
#endif

    app_timer_reg_cb(usb_ds_timeout_cb, &app_usb_ds_timer_id);

    bt_mgr_cback_register(app_usb_ds_bt_cback);

}

