#include <stdlib.h>
#include "section.h"
#include "app_timer.h"
#include "app_cfg.h"
#include "trace.h"
#include "audio_pipe.h"
#include "usb_audio_config.h"
#include "app_downstream_encode.h"
#include "app_upstream_decode.h"
#include "app_src_policy.h"
#include "app_audio_pipe_mgr.h"

#if F_APP_USB_GIP_SUPPORT
#include "app_gip.h"
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
#include "le_audio_data.h"
#endif

#define US_CODEC_BUF_SIZE          2048
#define US_TMP_BUF_SIZE            1024
#define USB_US_PCM_BUF_SIZE        4096
#define LEA_DATA_TMP_BUF_SIZE      256

/* fill upstream packet according to pcm buf level */
#define FILL_US_PKT_THRESHOLD      1024
#define US_START_DECODE_FRAME_CNT  6
#define SPLIT_DECODE_PCM_FRAME_NUM 1

#define ISO_QUEUE_MAX_ELEM         10

typedef enum
{
    APP_TIMER_US_PKT_XMIT,
} T_APP_US_TIMER;

typedef struct
{
    T_AUDIO_FORMAT_INFO frame_info;
    uint8_t             fill_frame_num;
    /* transfer to corresponding pcm len */
    uint16_t            pcm_48k_frame_len;
    uint16_t            sbc_frame_len;
    bool                is_stereo;
} T_US_FRAME_INFO;

typedef struct iso_queue_elem
{
    struct iso_queue_elem *next;
    uint32_t timestamp;
    uint8_t *data;
    uint16_t size;
} T_ISO_QUEUE_ELEM;

static uint8_t *us_codec_buf = NULL;
static uint8_t *us_tmp_buf = NULL;
static uint8_t *usb_us_pcm_buf;

#if TARGET_LE_AUDIO_GAMING_DONGLE
static bool upstream_is_stereo = true;
static uint8_t *lea_data_tmp_buf = NULL;
static T_OS_QUEUE left_iso_queue;
static T_OS_QUEUE right_iso_queue;
#endif

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
static uint8_t timer_idx_us_pkt_xmit = 0;
#endif

static T_US_PIPE_INFO us_pipe_info;

static uint8_t app_us_decode_timer_id = 0;
static T_RING_BUFFER us_codec_ring_buf;
T_RING_BUFFER usb_us_pcm_ring_buf;

static void app_us_decode_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    case APP_TIMER_US_PKT_XMIT:
        {
            app_stop_timer(&timer_idx_us_pkt_xmit);

            us_pipe_info.us_pkt_xmitting = false;
            us_pipe_info.us_pkt_start_decode = false;

            uint16_t remain_codec_size = ring_buffer_get_data_count(&us_codec_ring_buf);

            if (remain_codec_size != 0)
            {
                ring_buffer_remove(&us_codec_ring_buf, remain_codec_size);
            }

            APP_PRINT_TRACE2("us pkt timeout: codec buf (%d->%d)", remain_codec_size,
                             ring_buffer_get_data_count(&us_codec_ring_buf));
        }
        break;
#endif

    default:
        break;
    }
}

static bool upstream_pipe_data_ind(uint8_t *data, uint16_t len)
{
    uint16_t pre_pcm_size = ring_buffer_get_data_count(&usb_us_pcm_ring_buf);
    uint16_t new_pcm_size = pre_pcm_size;
    uint16_t pre_decoding_size = us_pipe_info.decoding_pcm_size;

    us_pipe_info.decoding_pcm_size -= len;

    if (ring_buffer_get_remaining_space(&usb_us_pcm_ring_buf) < len)
    {
        ring_buffer_remove(&usb_us_pcm_ring_buf, len);

        APP_PRINT_TRACE0("upstream_pipe_data_ind: buf full");
    }

    ring_buffer_write(&usb_us_pcm_ring_buf, data, len);

    new_pcm_size = ring_buffer_get_data_count(&usb_us_pcm_ring_buf);

    if (us_pipe_info.pcm_prequeue_ready == false)
    {
        if (new_pcm_size >= PCM_PREQUEUE_SIZE)
        {
            us_set_pcm_prequeue_ready(true);
        }
    }

    new_pcm_size = ring_buffer_get_data_count(&usb_us_pcm_ring_buf);

    APP_PRINT_TRACE5("upstream_pipe_data_ind: len %d decoding (%d->%d) buf (%d->%d)", len,
                     pre_decoding_size, us_pipe_info.decoding_pcm_size,
                     pre_pcm_size, new_pcm_size);

    return true;
}

static void upstream_pipe_event_cback(T_APP_PIPE_EVENT event)
{
    APP_PRINT_TRACE1("upstream_pipe_event_cback: event %d", event);

    if (event == PIPE_EVENT_CREATED || event == PIPE_EVENT_STARTED)
    {
        us_pipe_info.decoding_pcm_size = 0;
    }
}

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
void us_handle_rcv_pkt(uint8_t *buf, uint16_t len)
{
    uint8_t frame_cnt = (buf[1] >> 3) & 0x07;
    uint16_t data_len = ((buf[2] & 0x03) << 8) | buf[3];
    uint16_t frame_size = data_len / frame_cnt;
    uint16_t pre_codec_size = ring_buffer_get_data_count(&us_codec_ring_buf);
    uint16_t new_codec_size = pre_codec_size;

    us_pipe_info.us_pkt_xmitting = true;
    app_start_timer(&timer_idx_us_pkt_xmit, "us_pkt_xmit",
                    app_us_decode_timer_id, APP_TIMER_US_PKT_XMIT,
                    0, false, 100);

    us_pipe_info.frame_size = frame_size;

    if (ring_buffer_get_remaining_space(&us_codec_ring_buf) < data_len)
    {
        APP_PRINT_ERROR0("us_handle_rcv_pkt: codec buf full");
        ring_buffer_remove(&us_codec_ring_buf, data_len);
    }

    ring_buffer_write(&us_codec_ring_buf, buf + 4, data_len);

    new_codec_size = ring_buffer_get_data_count(&us_codec_ring_buf);

    APP_PRINT_TRACE2("us_handle_rcv_pkt: (%d -> %d)", pre_codec_size, new_codec_size);

    if (us_pipe_info.us_pkt_start_decode == false)
    {
        if ((new_codec_size / frame_size) > US_START_DECODE_FRAME_CNT)
        {
            us_pipe_info.us_pkt_start_decode = true;
        }
    }

#if 0         /* driven pcm decode by usb periodical interrupt only */
    if (us_pipe_info.us_pkt_start_decode)
    {
        us_decode_pkt_from_buf();
    }
#endif
}
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
static T_ISO_CHNL_INFO_DB *get_lea_iso_elem(uint16_t conn_handle)
{
    T_LE_AUDIO_CO_DB *p_audio_co_db = le_audio_get_audio_db();
    T_ISO_CHNL_INFO_DB *iso_elem = NULL;
    uint8_t i = 0;

    for (i = 0; i < p_audio_co_db->output_path_queue.count; i++)
    {
        iso_elem = (T_ISO_CHNL_INFO_DB *)os_queue_peek(&p_audio_co_db->output_path_queue, i);

        if (iso_elem->iso_conn_handle == conn_handle)
        {
            break;
        }
    }

    return iso_elem;
}

static bool iso_queue_in(T_OS_QUEUE *iso_queue, uint8_t *data, uint16_t size, uint32_t timestamp)
{
    T_ISO_QUEUE_ELEM *iso_elem_in = NULL;
    T_ISO_QUEUE_ELEM *iso_elem_out = NULL;
    uint8_t cause = 0;

    iso_elem_in = calloc(1, sizeof(T_ISO_QUEUE_ELEM));
    if (iso_elem_in == NULL)
    {
        cause = 1;
        goto failed;
    }

    iso_elem_in->data = calloc(1, size);
    if (iso_elem_in->data == NULL)
    {
        cause = 2;
        goto failed;
    }

    memcpy(iso_elem_in->data, data, size);
    iso_elem_in->size = size;
    iso_elem_in->timestamp = timestamp;

    if (iso_queue->count > ISO_QUEUE_MAX_ELEM)
    {
        iso_elem_out = os_queue_out(iso_queue);

        free(iso_elem_out->data);
        free(iso_elem_out);
    }

    os_queue_in(iso_queue, iso_elem_in);

    return true;

failed:
    APP_PRINT_ERROR1("iso_queue_in failed: cause -%d", cause);

    return false;
}

static T_ISO_QUEUE_ELEM *iso_queue_out(T_OS_QUEUE *iso_queue)
{
    return os_queue_out(iso_queue);
}

static T_ISO_QUEUE_ELEM *iso_queue_peek_last(T_OS_QUEUE *iso_queue)
{
    T_ISO_QUEUE_ELEM *elem = NULL;

    if (iso_queue->count != 0)
    {
        elem = os_queue_peek(iso_queue, iso_queue->count - 1);
    }

    return elem;
}

static T_ISO_QUEUE_ELEM *iso_queue_search_by_time(T_OS_QUEUE *iso_queue, uint32_t timestamp)
{
    T_ISO_QUEUE_ELEM *iso_elem = NULL;
    bool found = false;

    for (uint8_t i = 0; i < iso_queue->count; i++)
    {
        iso_elem = os_queue_peek(iso_queue, i);

        if (iso_elem->timestamp == timestamp)
        {
            found = true;
            break;
        }
    }

    if (found == false)
    {
        iso_elem = NULL;
    }

    return iso_elem;
}

static void iso_queue_delete_elem_and_expired(T_OS_QUEUE *iso_queue, T_ISO_QUEUE_ELEM *iso_elem)
{
    uint8_t delete_num = 0;
    uint8_t i = 0;
    T_ISO_QUEUE_ELEM *iso_peek = NULL;

    for (i = 0; i < iso_queue->count; i++)
    {
        iso_peek = os_queue_peek(iso_queue, i);

        if (iso_peek == iso_elem)
        {
            delete_num = i + 1;
            break;
        }
    }

    while (delete_num > 0)
    {
        iso_peek = os_queue_out(iso_queue);

        if (iso_peek != NULL)
        {
            free(iso_peek->data);
            free(iso_peek);

            delete_num--;
        }
    }
}

void iso_queue_clear(T_ISO_QUEUE_NUM queue_num)
{
    T_OS_QUEUE *iso_queue;
    T_ISO_QUEUE_ELEM *iso_elem;

    APP_PRINT_TRACE1("iso_queue_clear: %d", queue_num);

    if (queue_num == ISO_LEFT_QUEUE)
    {
        iso_queue = &left_iso_queue;
    }
    else
    {
        iso_queue = &right_iso_queue;
    }

    while ((iso_elem = os_queue_out(iso_queue)) != NULL)
    {
        free(iso_elem->data);
        free(iso_elem);
    }
}

static bool lea_data_pre_handle(uint8_t *buf, uint16_t buf_size, T_BT_DIRECT_ISO_DATA_IND *p_iso,
                                uint16_t *fill_size)
{
    T_LE_AUDIO_CO_DB *p_audio_co_db = le_audio_get_audio_db();
    T_ISO_CHNL_INFO_DB *iso_db = get_lea_iso_elem(p_iso->conn_handle);
    bool is_valid_data = true;
    bool two_upstream_channel = false;
    uint8_t *data = p_iso->p_buf + p_iso->offset;
    uint8_t cause = 0;
    uint16_t frame_size = 0;
    bool ret = false;
    bool duplicate_frame = false;
    T_OS_QUEUE *iso_queue = NULL;
    T_OS_QUEUE *other_iso_queue = NULL;
    T_ISO_QUEUE_ELEM *iso_elem = NULL;
    T_ISO_QUEUE_ELEM *other_iso_elem = NULL;
    T_ISO_QUEUE_ELEM *iso_left = NULL;
    T_ISO_QUEUE_ELEM *iso_right = NULL;
    T_ISO_QUEUE_NUM queue_num;

    if (iso_db == NULL)
    {
        cause = 1;
        goto failed;
    }

    if ((p_audio_co_db->output_left_ready == false &&
         p_iso->conn_handle == p_audio_co_db->output_left_handle) ||
        (p_audio_co_db->output_right_ready == false &&
         p_iso->conn_handle == p_audio_co_db->output_right_handle))
    {
        cause = 2;
        goto failed;
    }

    if (p_iso->pkt_status_flag != ISOCH_DATA_PKT_STATUS_VALID_DATA || p_iso->iso_sdu_len == 0)
    {
        is_valid_data = false;
    }

    frame_size = iso_db->codec_data.octets_per_codec_frame;

    if (upstream_is_stereo && p_audio_co_db->output_left_ready && p_audio_co_db->output_right_ready)
    {
        two_upstream_channel = true;
    }

    if (upstream_is_stereo)
    {
        duplicate_frame = true;
    }

    if (duplicate_frame)
    {
        *fill_size = frame_size * 2;
    }
    else
    {
        *fill_size = frame_size;
    }

    if (buf_size < *fill_size)
    {
        cause = 3;
        goto failed;
    }

    if (is_valid_data == false)
    {
        memset(us_tmp_buf, 0, US_TMP_BUF_SIZE);
        data = us_tmp_buf;
    }

    if (p_iso->conn_handle == p_audio_co_db->output_left_handle)
    {
        iso_queue = &left_iso_queue;
        other_iso_queue = &right_iso_queue;
        queue_num = ISO_LEFT_QUEUE;
    }
    else
    {
        iso_queue = &right_iso_queue;
        other_iso_queue = &left_iso_queue;
        queue_num = ISO_RIGHT_QUEUE;
    }

    if (iso_queue_in(iso_queue, data, frame_size, p_iso->time_stamp) == false)
    {
        cause = 4;
        goto failed;
    }

    if (two_upstream_channel == false)
    {
        if ((iso_elem = iso_queue_out(iso_queue)) != NULL)
        {
            memcpy(buf, iso_elem->data, iso_elem->size);

            if (duplicate_frame)
            {
                memcpy(buf + iso_elem->size, iso_elem->data, iso_elem->size);
            }

            free(iso_elem->data);
            free(iso_elem);

            ret = true;
        }
    }
    else
    {
        if ((other_iso_elem = iso_queue_search_by_time(other_iso_queue, p_iso->time_stamp)) != NULL)
        {
            iso_elem = iso_queue_peek_last(iso_queue);
            if (iso_elem != NULL)
            {
                iso_left = (iso_queue == &left_iso_queue) ? iso_elem : other_iso_elem;
                iso_right = (iso_queue == &right_iso_queue) ? iso_elem : other_iso_elem;

                memcpy(buf, iso_left->data, iso_left->size);
                memcpy(buf + iso_left->size, iso_right->data, iso_right->size);

                iso_queue_delete_elem_and_expired(&left_iso_queue, iso_left);
                iso_queue_delete_elem_and_expired(&right_iso_queue, iso_right);

                ret = true;
            }
        }
    }

    APP_PRINT_TRACE5("lea_data_pre_handle: left_ready %d right_ready %d queue_num %d frame_size %d ret %d",
                     p_audio_co_db->output_left_ready, p_audio_co_db->output_right_ready,
                     queue_num, frame_size, ret);

    return ret;
failed:
    APP_PRINT_ERROR1("lea_data_pre_handle: failed -%d", cause);

    return false;
}

void us_handle_lea_data(T_BT_DIRECT_ISO_DATA_IND *p_iso)
{
    T_LE_AUDIO_CO_DB *p_audio_co_db = le_audio_get_audio_db();
    uint16_t fill_size = 0;
    uint8_t cause = 0;

    if (lea_data_pre_handle(lea_data_tmp_buf, LEA_DATA_TMP_BUF_SIZE, p_iso, &fill_size) == false)
    {
        return;
    }

    if (fill_size != app_pipe_get_codec_frame_size(us_pipe_info.codec_type))
    {
        cause = 3;

        APP_PRINT_ERROR2("us_handle_lea_data: frame_size %d not equal to codec frame size %d",
                         fill_size, app_pipe_get_codec_frame_size(us_pipe_info.codec_type));

        goto failed;
    }

    if (app_pipe_fill(AUDIO_PIPE_UPSTREAM, lea_data_tmp_buf, fill_size))
    {
        us_pipe_info.decoding_pcm_size += us_pipe_info.expect_pcm_frame_len;
    }

    return;

failed:

    APP_PRINT_ERROR1("us_handle_lea_data failed: cause %d", cause);
}
#endif

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
void us_decode_pkt_from_buf(void)
{
    uint16_t fill_size = us_pipe_info.frame_size;
    uint16_t pre_codec_size = ring_buffer_get_data_count(&us_codec_ring_buf);
    uint16_t new_codec_size = pre_codec_size;
    uint16_t remain_pcm_buf_size = ring_buffer_get_remaining_space(&usb_us_pcm_ring_buf);
    uint16_t remain_codec_data_size = ring_buffer_get_data_count(&us_codec_ring_buf);
    uint8_t cause = 0;

    if (fill_size == 0)
    {
        cause = 1;
        goto exit;
    }

    if (remain_codec_data_size < fill_size)
    {
        cause = 2;
        goto exit;
    }

    if (remain_pcm_buf_size < us_pipe_info.expect_pcm_frame_len + us_pipe_info.decoding_pcm_size)
    {
        cause = 3;
        goto exit;
    }

    ring_buffer_read(&us_codec_ring_buf, fill_size, us_tmp_buf);

    new_codec_size = ring_buffer_get_data_count(&us_codec_ring_buf);

    if (app_pipe_fill(AUDIO_PIPE_UPSTREAM, us_tmp_buf, fill_size))
    {
        us_pipe_info.decoding_pcm_size += us_pipe_info.expect_pcm_frame_len;
    }

exit:
    if (cause != 0)
    {
        APP_PRINT_TRACE4("us_decode_pkt_from_buf failed: cause %d (%d %d %d)", cause, remain_pcm_buf_size,
                         us_pipe_info.expect_pcm_frame_len,
                         us_pipe_info.decoding_pcm_size);
    }
    else
    {
        APP_PRINT_TRACE3("us_decode_pkt_from_buf: codec size (%d ->%d) remain_pcm_buf_size %d",
                         pre_codec_size, new_codec_size, remain_pcm_buf_size);
    }
}
#endif

void us_set_pcm_prequeue_ready(bool value)
{
    APP_PRINT_TRACE1("us_set_pcm_prequeue_ready: %d", value);

    us_pipe_info.pcm_prequeue_ready = value;

    if (value == false)
    {
        uint16_t remain_pcm_size = ring_buffer_get_data_count(&usb_us_pcm_ring_buf);

        if (remain_pcm_size != 0)
        {
            ring_buffer_remove(&usb_us_pcm_ring_buf, remain_pcm_size);
        }
    }
}

RAM_TEXT_SECTION bool us_pkt_start_decode(void)
{
    return us_pipe_info.us_pkt_start_decode;
}

RAM_TEXT_SECTION bool us_pcm_prequeue_ready(void)
{
    return us_pipe_info.pcm_prequeue_ready;
}

bool us_pipe_gain_set(uint16_t gain)
{
    bool ret = false;

    us_pipe_info.mic_gain = gain;

    if (us_pipe_info.mic_mute)
    {
        gain = UAC_MIC_VOL_MIN;
    }

    app_pipe_gain_set(AUDIO_PIPE_UPSTREAM, gain);

    return ret;
}

bool us_pipe_mute_set(bool mute)
{
    uint16_t gain = 0;

    us_pipe_info.mic_mute = mute;

    if (us_pipe_info.mic_mute)
    {
        gain = UAC_MIC_VOL_MIN;
    }
    else
    {
        gain = us_pipe_info.mic_gain;
    }

    app_pipe_gain_set(AUDIO_PIPE_UPSTREAM, gain);

    APP_PRINT_TRACE1("us_pipe_mute_set: mute %d", mute);

    return true;
}

void upstream_pipe_create(T_GAMING_CODEC src_codec)
{
    T_APP_PIPE_PARAM pipe_param = {0};
    T_GAMING_CODEC snk_codec = PCM_48K_16BIT_MONO_10MS;

#if (USB_AUDIO_US_SAMPLE_RATE == UAC_SAM_RATE_48000)
#if F_APP_USB_GIP_SUPPORT
    if (app_gip_get_switch_mode())
    {
        snk_codec = PCM_16K_16BIT_MONO_10MS;
    }
#endif
#elif (USB_AUDIO_US_SAMPLE_RATE == UAC_SAM_RATE_16000)
    snk_codec = PCM_16K_16BIT_MONO_10MS;
#endif

    pipe_param.src_codec   = src_codec;
    pipe_param.snk_codec   = snk_codec;
    pipe_param.data_cback  = upstream_pipe_data_ind;
    pipe_param.event_cback = upstream_pipe_event_cback;
    pipe_param.asrc_toggle = false;
    pipe_param.mode = AUDIO_STREAM_MODE_DIRECT;

    app_pipe_create(AUDIO_PIPE_UPSTREAM, &pipe_param);

    us_pipe_info.expect_pcm_frame_len = app_pipe_get_codec_frame_size(snk_codec);
    us_pipe_info.codec_type = src_codec;

    APP_PRINT_TRACE2("upstream_pipe_create: type %d expect_pcm_frame_len %d", src_codec,
                     us_pipe_info.expect_pcm_frame_len);
}

void upstream_pipe_release()
{
    app_pipe_release(AUDIO_PIPE_UPSTREAM);
}

#if TARGET_LE_AUDIO_GAMING_DONGLE
void app_upstream_lea_pipe_create(T_CODEC_CFG *p_lea_codec)
{
    if (p_lea_codec->frame_duration == FRAME_DURATION_CFG_10_MS)
    {
        if (upstream_is_stereo)
        {
            upstream_pipe_create(LC3_16K_16BIT_STEREO_10MS);
        }
        else
        {
            upstream_pipe_create(LC3_16K_16BIT_MONO_10MS);
        }
    }
    else if (p_lea_codec->frame_duration == FRAME_DURATION_CFG_7_5_MS)
    {
        if (upstream_is_stereo)
        {
            upstream_pipe_create(LC3_16K_16BIT_STEREO_7_5MS);
        }
        else
        {
            upstream_pipe_create(LC3_16K_16BIT_MONO_7_5MS);
        }
    }
}
#endif

void us_process_init(void)
{
    app_timer_reg_cb(app_us_decode_timeout_cb, &app_us_decode_timer_id);

    if (us_codec_buf == NULL)
    {
        us_codec_buf = malloc(US_CODEC_BUF_SIZE);

        ring_buffer_init(&us_codec_ring_buf, us_codec_buf, US_CODEC_BUF_SIZE);
    }

    if (us_tmp_buf == NULL)
    {
        us_tmp_buf = malloc(US_TMP_BUF_SIZE);
    }

    if (usb_us_pcm_buf == NULL)
    {
        usb_us_pcm_buf = malloc(USB_US_PCM_BUF_SIZE);

        ring_buffer_init(&usb_us_pcm_ring_buf, usb_us_pcm_buf, USB_US_PCM_BUF_SIZE);
    }

#if TARGET_LE_AUDIO_GAMING_DONGLE
    if (lea_data_tmp_buf == NULL)
    {
        lea_data_tmp_buf = malloc(LEA_DATA_TMP_BUF_SIZE);
    }

    os_queue_init(&left_iso_queue);
    os_queue_init(&right_iso_queue);
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE_FOR_STEREO
    upstream_is_stereo = false;
#endif
}

