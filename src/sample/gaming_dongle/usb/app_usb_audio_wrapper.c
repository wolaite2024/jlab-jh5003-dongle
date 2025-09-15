#if F_APP_USB_AUDIO_SUPPORT | F_APP_USB_UAC_SUPPORT && F_APP_USB_SUPPORT
#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "ring_buffer.h"
#include "section.h"
#include "app_timer.h"
#include "app_usb_audio_wrapper.h"
#include "usb_audio_stream.h"
#include "usb_audio_config.h"
#include "app_cfg.h"
#include "app_gaming_sync.h"
#include "app_usb_uac.h"
#include "app_src_policy.h"
#include "app_ctrl_pkt_policy.h"
#include "app_dongle_vol.h"

#if DONGLE_LE_AUDIO
#include "app_le_audio.h"
#endif

#include "app_downstream_encode.h"

#include "app_upstream_decode.h"

#define PCM_48K_UPSTREAM_PREQUEUE_LEN   (768 * 3) // 8ms unit

#if F_APP_USB_GIP_SUPPORT
#include "app_gip.h"
#endif

#include <stdlib.h>
#include "app_msg.h"
#include "app_io_msg.h"
#include "hw_tim.h"
#include "usb_audio_config.h"
#include "app_usb.h"
#include "app_usb_vol_control.h"
#include "usb_host_detect.h"
#include "gaming_bt.h"
#include "app_dongle_transmit_client.h"

/* detect usb stream before send out */
#if F_APP_USB_HIGH_SPEED_0_5MS
#define USB_STREAM_DETECT_TIMEOUT      300
#else
#define USB_STREAM_DETECT_TIMEOUT      800
#endif

#define USB_STREAM_DETECT_INTERVAL     (PCM_DS_INTERVAL + USB_STREAM_DETECT_TIMEOUT)
#define COMPENSATION_NUM    2

#if F_APP_USB_SILENCE_STREAM_DETECT
#define PCM_SILENCE_TIME     3000 //ms

static uint32_t silence_pkt_cnt = 0;
static bool silence_pcm_detect = false;
#endif

static uint8_t *silence_buf = NULL;
static uint8_t *pcm_tmp_buf = NULL;

static T_HW_TIMER_HANDLE uac1_stream_timer_handle = NULL;
static T_HW_TIMER_HANDLE uac2_stream_timer_handle = NULL;

static bool uac1_streaming = false;
static bool uac2_streaming = false;
static uint16_t usb_zero_cnt[USB_AUDIO_STREAM_NUM] = {0};

/* pcm data size per usb interval */
static uint16_t usb_pcm_size_per_interval = 0;

#if F_APP_USB_SILENCE_STREAM_DETECT
#define UAC_SILENCE_STREAM_THRESHOLD  0

#define ABSTRACT(x)         (((x) > 0) ? (x) : ((~(x)) + 1))
#endif

typedef bool (*T_APP_USB_DS_HANDLE)(uint8_t *, uint16_t, uint8_t);

typedef struct app_usb_audio_db
{
    uint32_t playback_id;
    uint32_t capture_id;
    uint32_t playback_id_2;
    uint8_t  host_type;
} T_APP_USB_AUDIO_DB;
static T_APP_USB_AUDIO_DB usb_audio_wrap_db;

typedef enum
{
    USB_AUDIO_UAC1_SET_SPK_VOL,
    USB_AUDIO_UAC1_SET_MIC_VOL,
    USB_AUDIO_UAC2_SET_SPK_VOL,
    USB_AUDIO_UAC2_SET_MIC_VOL,
    USB_AUDIO_FEEDBACK_DETECT,
} T_APP_USB_AUDIO_TIMER_ID;

typedef struct
{
    T_UAC_LABEL     uac_label;
    T_UAC_VOL_TYPE  vol_type;
    uint16_t        vol_value;
    uint8_t         timer_idx;
    const char      *timer_string;
    T_APP_USB_AUDIO_TIMER_ID timer_id;
} T_UAC_VOL_INFO;

static T_UAC_VOL_INFO uac_vol_info[] =
{
    {USB_AUDIO_STREAM_LABEL_1, UAC_SPK_VOL, UAC_SPK_VOL_CUR, 0, "uac1_set_spk_vol", USB_AUDIO_UAC1_SET_SPK_VOL},
    {USB_AUDIO_STREAM_LABEL_1, UAC_MIC_VOL, UAC_MIC_VOL_CUR, 0, "uac1_set_mic_vol", USB_AUDIO_UAC1_SET_MIC_VOL},
    {USB_AUDIO_STREAM_LABEL_2, UAC_SPK_VOL, UAC_SPK_VOL_CUR, 0, "uac2_set_spk_vol", USB_AUDIO_UAC2_SET_SPK_VOL},
    {USB_AUDIO_STREAM_LABEL_2, UAC_MIC_VOL, UAC_MIC_VOL_CUR, 0, "uac2_set_mic_vol", USB_AUDIO_UAC2_SET_MIC_VOL},
};

static uint8_t app_usb_audio_timer_queue_id = 0;

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
static int16_t uac_sample_freq_diff;
static T_USB_AUDIO_FEEDBACK_STATE uac_feedback_state;
static T_USB_FEEDBACK_CB uac_feeback_cb;
static uint8_t feedback_detect_cnt;
static uint8_t timer_idx_usb_feedback_detect;
#endif

#if USB_VOL_RANGE_CHANGE_SUPPORT
#define UAC_DAC_MAX_LEVEL      100
static bool is_ios_system = false;
static const uint16_t usb_audio_dac_new_gain_table[] =
{
    0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0008, 0x0009, 0x000A, 0x000C, 0x000D,
    0x000E, 0x000F, 0x0011, 0x0012, 0x0013, 0x0014, 0x0016, 0x0017, 0x0018, 0x0019,
    0x001B, 0x001C, 0x001D, 0x001E, 0x0020, 0x0021, 0x0022, 0x0023, 0x0025, 0x0026,
    0x0027, 0x0028, 0x002A, 0x002B, 0x002C, 0x002D, 0x002F, 0x0030, 0x0031, 0x0033,
    0x0034, 0x0035, 0x0036, 0x0038, 0x0039, 0x003A, 0x003B, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0042, 0x0043, 0x0044, 0x0046, 0x0047, 0x0048, 0x0049, 0x004B, 0x004C,
    0x004D, 0x004E, 0x0050, 0x0051, 0x0052, 0x0054, 0x0055, 0x0056, 0x0057, 0x0059,
    0x005A, 0x005B, 0x005D, 0x005E, 0x005F, 0x0060, 0x0062, 0x0063, 0x0064, 0x0065,
    0x0067, 0x0068, 0x0069, 0x006B, 0x006C, 0x006D, 0x006E, 0x0070, 0x0071, 0x0072,
    0x0074, 0x0075, 0x0076, 0x0077, 0x0079, 0x007A, 0x007B, 0x007D, 0x007E, 0x007F,
    0x0081
};
static const int16_t usb_audio_dac_dig_gain_table[] =
{
    0xb300, 0xc161, 0xc8a2, 0xcd86, 0xd138, 0xd430, 0xd6ac, 0xd8ce, 0xdaad, 0xdc57,
    0xddd7, 0xdf35, 0xe076, 0xe19e, 0xe2b2, 0xe3b3, 0xe4a4, 0xe587, 0xe65e, 0xe729,
    0xe7eb, 0xe8a3, 0xe953, 0xe9fb, 0xea9c, 0xeb37, 0xebcc, 0xec5b, 0xece6, 0xed6b,
    0xedec, 0xee69, 0xeee2, 0xef58, 0xefca, 0xf039, 0xf0a4, 0xf10d, 0xf173, 0xf1d7,
    0xf238, 0xf296, 0xf2f3, 0xf34d, 0xf3a5, 0xf3fb, 0xf450, 0xf4a2, 0xf4f3, 0xf542,
    0xf590, 0xf5dc, 0xf626, 0xf670, 0xf6b8, 0xf6fe, 0xf744, 0xf788, 0xf7cb, 0xf80c,
    0xf84d, 0xf88d, 0xf8cb, 0xf909, 0xf946, 0xf981, 0xf9bc, 0xf9f6, 0xfa2f, 0xfa68,
    0xfa9f, 0xfad6, 0xfb0c, 0xfb41, 0xfb75, 0xfba9, 0xfbdc, 0xfc0f, 0xfc41, 0xfc72,
    0xfca2, 0xfcd2, 0xfd02, 0xfd30, 0xfd5f, 0xfd8c, 0xfdba, 0xfde6, 0xfe12, 0xfe3e,
    0xfe69, 0xfe94, 0xfebe, 0xfee8, 0xff11, 0xff3a, 0xff63, 0xff8b, 0xffb2, 0xffda,
    0x0000
};
static const int16_t usb_audio_mic_gain_table[] =
{
    0xad00, 0xbf58, 0xc76a, 0xcca9, 0xd08d, 0xd3a6, 0xd638, 0xd86a, 0xda56, 0xdc0a,
    0xdd93, 0xdef7, 0xe03d, 0xe16b, 0xe282, 0xe387, 0xe47b, 0xe561, 0xe63a, 0xe708,
    0xe7cb, 0xe885, 0xe937, 0xe9e1, 0xea83, 0xeb1f, 0xebb5, 0xec46, 0xecd1, 0xed58,
    0xedda, 0xee58, 0xeed2, 0xef48, 0xefbb, 0xf02a, 0xf096, 0xf100, 0xf166, 0xf1ca,
    0xf22c, 0xf28b, 0xf2e8, 0xf342, 0xf39b, 0xf3f1, 0xf446, 0xf499, 0xf4ea, 0xf53a,
    0xf588, 0xf5d4, 0xf61f, 0xf669, 0xf6b1, 0xf6f8, 0xf73d, 0xf782, 0xf7c5, 0xf807,
    0xf848, 0xf888, 0xf8c6, 0xf904, 0xf941, 0xf97d, 0xf9b8, 0xf9f2, 0xfa2c, 0xfa64,
    0xfa9c, 0xfad3, 0xfb09, 0xfb3e, 0xfb73, 0xfba7, 0xfbda, 0xfc0c, 0xfc3e, 0xfc70,
    0xfca0, 0xfcd0, 0xfd00, 0xfd2f, 0xfd5d, 0xfd8b, 0xfdb8, 0xfde5, 0xfe11, 0xfe3d,
    0xfe68, 0xfe93, 0xfebd, 0xfee7, 0xff11, 0xff3a, 0xff62, 0xff8a, 0xffb2, 0xffda,
    0x0000
};
static uint16_t app_usb_audio_get_volume_level(uint16_t vol_param, bool spk_vol)
{
    int16_t gain_value = vol_param;
    int16_t gain_table_val = usb_audio_dac_dig_gain_table[0];
    uint16_t l_level = 0;
    uint16_t r_level = UAC_DAC_MAX_LEVEL;
    uint16_t mid_level = l_level + r_level / 2;
    uint8_t cnt = 0;
    while (l_level <= r_level)
    {
        cnt++;
        mid_level = (r_level + l_level + 1) / 2;
        if (spk_vol)
        {
            gain_table_val = usb_audio_dac_dig_gain_table[mid_level];
        }
        else
        {
            gain_table_val = usb_audio_mic_gain_table[mid_level];
        }
        if ((mid_level == l_level) || (mid_level == r_level))
        {
            if (spk_vol)
            {
                if (gain_value == usb_audio_dac_dig_gain_table[l_level])
                {
                    mid_level = l_level;
                }
                else if (gain_value == usb_audio_dac_dig_gain_table[r_level])
                {
                    mid_level = r_level;
                }
            }
            else
            {
                if (gain_value == usb_audio_mic_gain_table[l_level])
                {
                    mid_level = l_level;
                }
                else if (gain_value == usb_audio_mic_gain_table[r_level])
                {
                    mid_level = r_level;
                }
            }
            break;
        }
        if (gain_value == gain_table_val)
        {
            break;
        }
        else if (gain_table_val > gain_value)
        {
            r_level = mid_level - 1;
        }
        else
        {
            l_level = mid_level + 1;
        }
    }
    return mid_level;
}

uint16_t app_usb_audio_spk_vol_transform(uint16_t vol_param)
{
    uint16_t volume;
    uint16_t level = 66;

    if (vol_param == UAC_SPK_VOL_MAX) //MAX_VOL
    {
        level = 100;
    }
    else if (vol_param == UAC_SPK_VOL_MIN) //MIN_VOL
    {
        level = 0;
    }
    else
    {
        if (is_ios_system == false)
        {
            level = app_usb_audio_get_volume_level(vol_param, true);
        }
    }
    if ((vol_param != (uint16_t)usb_audio_dac_dig_gain_table[level]) && (is_ios_system == false))
    {
        APP_PRINT_INFO2("app_usb_audio_spk_vol_transform vol_param 0x%x ,table_vol 0x%x", vol_param,
                        usb_audio_dac_dig_gain_table[level]);
        is_ios_system = true;
    }
    if (is_ios_system == true)
    {
        int16_t j = (int16_t)(UAC_SPK_VOL_MAX - UAC_SPK_VOL_MIN);
        int16_t i = (int16_t)(vol_param - UAC_SPK_VOL_MIN);
        level = i * 0x64 / j;
    }

    volume = usb_audio_dac_new_gain_table[level];

    if (volume >= usb_audio_dac_new_gain_table[0])
    {
        /* change range to 0x00 ~ 0x7f for avrcp volume */
        volume -= usb_audio_dac_new_gain_table[0];
    }

    APP_PRINT_INFO4("app_usb_audio_spk_vol_transform, vol 0x%x, level = %d, volume 0x%x is_ios_system %d",
                    vol_param,
                    level, volume, is_ios_system);
    return volume;
}

#endif

bool app_usb_audio_active(uint32_t label, uint8_t dir)
{
    APP_PRINT_INFO2("app_usb_audio_active: label %d, dir %d", label, dir);

    if (label == USB_AUDIO_STREAM_LABEL_1)
    {
        app_handle_uac_active(dir);
    }
    else if (label == USB_AUDIO_STREAM_LABEL_2)
    {
#ifdef ENABLE_UAC2
        app_handle_uac_2_inactive(dir);
#endif
    }

    return true;
}

bool app_usb_audio_deactive(uint32_t label, uint8_t dir)
{
    APP_PRINT_INFO2("app_usb_audio_deactive: label %d, dir %d", label, dir);

    if (label == USB_AUDIO_STREAM_LABEL_1)
    {
        app_handle_uac_inactive(dir);
    }
    else if (label == USB_AUDIO_STREAM_LABEL_2)
    {
#ifdef ENABLE_UAC2
        app_handle_uac_2_inactive(dir);
#endif
    }

    return true;
}

RAM_TEXT_SECTION static void uac_silence_stream_handle(T_UAC_LABEL uac_label)
{
    uint32_t period = PCM_DS_INTERVAL;
    T_HW_TIMER_HANDLE usb_stream_timer_handle = NULL;

    if (uac_label == USB_AUDIO_STREAM_LABEL_1)
    {
        usb_stream_timer_handle = uac1_stream_timer_handle;
    }
    else
    {
        usb_stream_timer_handle = uac2_stream_timer_handle;
    }

    if (usb_zero_cnt[uac_label] < (USB_SEND_SILENCE_TIME * 1000 / PCM_DS_INTERVAL))
    {
        usb_zero_cnt[uac_label]++;

        /* compensation for the empty usb stream data */
        if (usb_zero_cnt[uac_label] < (COMPENSATION_NUM + 2))
        {
            period -= (USB_STREAM_DETECT_TIMEOUT / COMPENSATION_NUM);
        }

        hw_timer_restart(usb_stream_timer_handle, period);

        APP_PRINT_TRACE4("send silence pkt to label %d: cnt %d size %d period %d",
                         uac_label,
                         usb_zero_cnt[uac_label],
                         usb_pcm_size_per_interval, period);

        app_usb_stream_xmit_out(silence_buf, usb_pcm_size_per_interval, uac_label);
    }
    else
    {
        APP_PRINT_TRACE1("send silence pkt stop for label %d!", uac_label);
        usb_zero_cnt[uac_label] = 0;

        hw_timer_stop(usb_stream_timer_handle);

        if (uac_label == USB_AUDIO_STREAM_LABEL_1)
        {
            uac1_streaming = false;
        }
        else
        {
            uac2_streaming = false;
        }

        if (uac1_streaming == false && uac2_streaming == false)
        {
            app_usb_other_trigger_evt(USB_OTHER_EVT_AUDIO_USB_STREAM_READY_TO_TX, false);
        }
    }
}

RAM_TEXT_SECTION static void uac_stream_timeout_handle(T_UAC_LABEL uac_label)
{
    bool *uac_streaming = (uac_label == USB_AUDIO_STREAM_LABEL_1) ? &uac1_streaming : &uac2_streaming;
    bool other_uac_streaming = (uac_label == USB_AUDIO_STREAM_LABEL_1) ? uac2_streaming :
                               uac1_streaming;
    T_HW_TIMER_HANDLE usb_stream_timer_handle = (uac_label == USB_AUDIO_STREAM_LABEL_1) ?
                                                uac1_stream_timer_handle : uac2_stream_timer_handle;;

    if (other_uac_streaming == false)
    {
        uac_silence_stream_handle(uac_label);
    }
    else
    {
        *uac_streaming = false;
        hw_timer_stop(usb_stream_timer_handle);
    }
}

RAM_TEXT_SECTION static void app_uac1_stream_timer_isr_cb(T_HW_TIMER_HANDLE handle)
{
    APP_PRINT_TRACE2("app_uac1_stream_timer_isr_cb (%d %d)", uac1_streaming, uac2_streaming);

    uac_stream_timeout_handle(USB_AUDIO_STREAM_LABEL_1);
}

#if ENABLE_UAC2
RAM_TEXT_SECTION static void app_uac2_stream_timer_isr_cb(T_HW_TIMER_HANDLE handle)
{
    APP_PRINT_TRACE2("app_uac2_stream_timer_isr_cb (%d %d)", uac1_streaming, uac2_streaming);

    uac_stream_timeout_handle(USB_AUDIO_STREAM_LABEL_2);
}
#endif

/**
* @brief usb silence TIMx interrupt handler function.
*
*/

RAM_TEXT_SECTION static void app_usb_stream_detect(T_UAC_LABEL uac_label)
{
    T_HW_TIMER_HANDLE usb_stream_timer_handle = NULL;

    if (uac_label == USB_AUDIO_STREAM_LABEL_1)
    {
        uac1_streaming = true;
    }
    else
    {
        uac2_streaming = true;
    }

    if (uac_label == USB_AUDIO_STREAM_LABEL_1)
    {
        usb_stream_timer_handle = uac1_stream_timer_handle;
    }
    else // USB_AUDIO_STREAM_LABEL_2
    {
        usb_stream_timer_handle = uac2_stream_timer_handle;
    }

    hw_timer_restart(usb_stream_timer_handle, USB_STREAM_DETECT_INTERVAL);

}

RAM_TEXT_SECTION static bool app_usb_check_disallow_usb_stream(void)
{
    bool ret = false;
    uint8_t cause = 0;

    if (0)
    {}
#if F_APP_USB_SILENCE_STREAM_DETECT
    else if (silence_pcm_detect && !app_get_usb_us_state())
    {
        ret = true;
        cause = 3;
    }
#endif
    else if (headset_status.audio_mixing_support)
    {
        /* for this case, don't need to disallow non-silence usb stream */
        return false;
    }
#if F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
    else if (uac_ctrl_status.delay_usb_tx_when_bt_play)
    {
        ret = true;
        cause = 1;
    }
#endif
    else if (((headset_status.phone_status == PHONE_STREAM_HFP)
              || (headset_status.phone_status == PHONE_STREAM_LEA_CALL))
#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
             && (app_get_usb_us_state() == false)
#endif
            )
    {
        ret = true;
        cause = 2;
    }
#if TARGET_LE_AUDIO_GAMING_DONGLE
    else if (headset_status.headset_source == HEADSET_SOURCE_BT)
    {
        ret = true;
        cause = 4;
    }
#endif
#if TARGET_LE_AUDIO_GAMING_DONGLE
    else if (app_le_audio_restarting())
    {
        ret = true;
        cause = 6;
    }
#endif

    if (ret)
    {
        APP_PRINT_TRACE2("app_usb_check_disallow_usb_stream: ret %d cause %d", ret, -cause);
    }

    return ret;
}

#if F_APP_USB_SILENCE_STREAM_DETECT
RAM_TEXT_SECTION
static uint32_t app_usb_calc_average_after_abs(uint8_t *data, uint16_t length)
{
    if ((!data) || (!length))
    {
        return 0;
    }

    uint32_t ret = 0;
    uint16_t len = 0;
    int16_t pcm_data = 0;

    len = MIN(192, length) >> 1;

    if (len == 0)
    {
        return 0;
    }

    for (uint8_t i = 0; i < len; i++)
    {
        pcm_data = (int16_t)(data[2 * i + 1] << 8 | data[2 * i]);
        ret += ABSTRACT(pcm_data);
    }
    ret /= len;
    return ret;
}
#endif

RAM_TEXT_SECTION static bool app_usb_audio_pre_handle(T_UAC_LABEL label, uint8_t *data,
                                                      uint16_t length)
{
    bool ret = true;

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
    if (uac_feedback_state == USB_FEEDBACK_DETECT)
    {
        APP_PRINT_TRACE0("app_usb_audio_data_xmit_out: feedback detect");

        if (length != usb_pcm_size_per_interval)
        {
            feedback_detect_cnt++;

            if (feedback_detect_cnt >= 3)
            {
                app_usb_other_trigger_evt(USB_OTHER_EVT_FEEDBACK_SUPPORT, 0);
            }
        }

        return false;
    }
#endif

#if F_APP_USB_SILENCE_STREAM_DETECT
    uint32_t pcm_abs = app_usb_calc_average_after_abs(data, length);
    bool prev_pcm_silence = silence_pcm_detect;

    if (pcm_abs <= UAC_SILENCE_STREAM_THRESHOLD)
    {
        silence_pkt_cnt++;

        if ((silence_pkt_cnt * PCM_DS_INTERVAL) > (PCM_SILENCE_TIME * 1000))
        {
            silence_pcm_detect = true;
        }
    }
    else
    {
        silence_pkt_cnt = 0;
        silence_pcm_detect = false;
    }

    if (prev_pcm_silence != silence_pcm_detect)
    {
        if (silence_pcm_detect)
        {
            APP_PRINT_TRACE0("silence stream detect");
        }
        else
        {
            APP_PRINT_TRACE0("silence stream stop");
        }

        app_usb_other_trigger_evt(USB_OTHER_EVT_SILENCE_DETECT, silence_pcm_detect);
    }
#endif

    if (uac1_streaming == false && uac2_streaming == false)
    {
#if F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
        if (headset_status.phone_status == PHONE_STREAM_A2DP &&
            uac_ctrl_status.delay_usb_tx_when_bt_play == false)
        {
            uac_ctrl_status.delay_usb_tx_when_bt_play = true;

            app_usb_other_trigger_evt(USB_OTHER_EVT_DELAY_USB_TX_START, 0);
        }
#endif
    }

    if (app_usb_check_disallow_usb_stream())
    {
        ret = false;
    }

    return ret;
}

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
RAM_TEXT_SECTION
void app_usb_audio_sample_rate_adjust(int16_t freq_diff)
{
    if (freq_diff != 0)
    {
        APP_PRINT_TRACE1("app_usb_audio_sample_rate_adjust: %d", freq_diff);
    }

    uac_sample_freq_diff = freq_diff;
}

RAM_TEXT_SECTION
bool app_usb_audio_feedback_ds(uint8_t *data, uint16_t length, uint32_t label)
{
    bool ret = true;
    uint32_t freq = USB_AUDIO_DS_SAMPLE_RATE;

    if (app_usb_audio_get_feedback_state() == USB_FEEDBACK_DETECT)
    {
        freq += 100;
    }
    else
    {
        freq += uac_sample_freq_diff;
    }

    memcpy(data, &freq, length);

    if (freq != USB_AUDIO_DS_SAMPLE_RATE)
    {
        APP_PRINT_TRACE3("app_usb_audio_feedback_ds, len %d, label %d, freq %d",
                         length, label, freq);
    }

    return ret;
}

void app_usb_audio_reg_feedback_state_cb(T_USB_FEEDBACK_CB callback)
{
    uac_feeback_cb = callback;
}

void app_usb_audio_set_feedback_state(T_USB_AUDIO_FEEDBACK_STATE state)
{
    APP_PRINT_TRACE1("app_usb_audio_set_feedback_state: state %d", state);

    uac_feedback_state = state;

    if (state == USB_FEEDBACK_DETECT)
    {
        feedback_detect_cnt = 0;
    }

    if (uac_feeback_cb)
    {
        uac_feeback_cb(state);
    }
}

T_USB_AUDIO_FEEDBACK_STATE app_usb_audio_get_feedback_state(void)
{
    return uac_feedback_state;
}

void app_usb_feedback_detect_start(void)
{
    app_start_timer(&timer_idx_usb_feedback_detect, "usb_feedback_detect",
                    app_usb_audio_timer_queue_id, USB_AUDIO_FEEDBACK_DETECT, 0, false,
                    600);
}

void app_usb_feedback_detect_stop(void)
{
    app_stop_timer(&timer_idx_usb_feedback_detect);
}
#endif

RAM_TEXT_SECTION
bool app_usb_audio_data_xmit_out(uint8_t *data, uint16_t length, T_UAC_LABEL label)
{
    bool ret = false;
    bool pcm_length_not_correct = false;
#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
    uint8_t bytes_per_sample = (UAC_SPK_BIT_RES / 8) * UAC_SPK_CHAN_NUM;
#endif

    uint32_t curr_time = sys_timestamp_get();
    static uint32_t last_time;

    if (curr_time - last_time > (USB_SRC_STREAMING_DETECT_TIMEOUT - 200))
    {
        app_usb_other_trigger_evt(USB_OTHER_EVT_USB_SRC_STREAMING, 0);
        last_time = curr_time;
    }

    if (length != usb_pcm_size_per_interval
#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
        && abs(length - usb_pcm_size_per_interval) > (bytes_per_sample * 20)
#endif
       )
    {
        APP_PRINT_ERROR2("app_usb_audio_data_xmit_out: pcm length not correct (%d %d)", length,
                         usb_pcm_size_per_interval);

        pcm_length_not_correct = true;

        memset(pcm_tmp_buf, 0, usb_pcm_size_per_interval);

        if (length <= usb_pcm_size_per_interval)
        {
            memcpy(pcm_tmp_buf, data, length);
        }
        else
        {
            memcpy(pcm_tmp_buf, data, usb_pcm_size_per_interval);
        }
    }

    /* workaround to fix pcm length not correct issue when bit-width with 24bit */
    if (pcm_length_not_correct)
    {
        data = pcm_tmp_buf;
        length = usb_pcm_size_per_interval;
    }

    if (app_usb_audio_pre_handle(label, data, length) == false)
    {
        return false;
    }

    app_usb_stream_detect(label);

    if (app_get_usb_stream_ready_to_tx() == false)
    {
        app_usb_other_trigger_evt(USB_OTHER_EVT_AUDIO_USB_STREAM_READY_TO_TX, true);
    }

    usb_zero_cnt[label] = 0;

    ret = app_usb_stream_xmit_out(data, length, label);

    return ret;
}

bool app_usb_audio_data_xmit_out_handle(uint32_t label)
{
    if (label == USB_AUDIO_STREAM_LABEL_1)
    {
        app_usb_uac_data_trans_ds_handle();
    }
    else if (label == USB_AUDIO_STREAM_LABEL_2)
    {
#ifdef ENABLE_UAC2
        app_usb_uac_2_data_trans_ds_handle();
#endif
    }
    return true;
}

bool app_usb_audio_us_data_write(uint8_t *data, uint16_t length)
{
    return false;
}

static T_UAC_VOL_INFO *app_usb_get_vol_info(T_UAC_VOL_TYPE type, uint8_t label)
{
    uint8_t i = 0;
    T_UAC_VOL_INFO *vol_info = NULL;

    for (i = 0; i < sizeof(uac_vol_info) / sizeof(T_UAC_VOL_INFO); i++)
    {
        if (uac_vol_info[i].vol_type  == type &&
            uac_vol_info[i].uac_label == label)
        {
            vol_info = &uac_vol_info[i];
            break;
        }
    }

    return vol_info;
}

static int16_t app_usb_spk_vol_convert_to_db(uint16_t vol)
{
    return ((int16_t)vol / 256);
}

bool app_usb_spk_vol_sync_to_headset(void)
{
    bool ret = false;
    T_USB_SPK_INFO usb_vol;
    usb_vol.uac_spk_vol_gain = app_usb_spk_vol_convert_to_db(uac_vol_info[0].vol_value);
    usb_vol.host_type = usb_audio_wrap_db.host_type;
    if ((uac_vol_info[0].vol_value == UAC_SPK_VOL_MIN) ||
        (uac_vol_info[0].vol_value == UAC_SPK_VOL_MAX))
    {
        usb_vol.is_max_min_vol = true;
    }
    else
    {
        usb_vol.is_max_min_vol = false;
    }

#if TARGET_LEGACY_GAMING_DONGLE
    if (app_spp_cmd_send(0, DONGLE_CMD_TRANS_SPK_VOL_INFO, (uint8_t *)&usb_vol, sizeof(T_USB_SPK_INFO)))
    {
        ret = true;
    }
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
    if (app_cmd_send_by_le(DONGLE_CMD_TRANS_SPK_VOL_INFO, (uint8_t *)&usb_vol, sizeof(T_USB_SPK_INFO),
                           EARBUD_SIDE_ANY))
    {
        ret = true;
    }
#endif
    return ret;
}

static T_UAC_VOL_INFO *app_usb_get_vol_info_by_timer_id(T_APP_USB_AUDIO_TIMER_ID timer_id)
{
    uint8_t i = 0;
    T_UAC_VOL_INFO *vol_info = NULL;

    for (i = 0; i < sizeof(uac_vol_info) / sizeof(T_UAC_VOL_INFO); i++)
    {
        if (uac_vol_info[i].timer_id  == timer_id)
        {
            vol_info = &uac_vol_info[i];
            break;
        }
    }

    return vol_info;
}

static void app_usb_audio_timer_cback(uint8_t timer_id, uint16_t timer_chann)
{
    switch (timer_id)
    {
    case USB_AUDIO_UAC1_SET_SPK_VOL:
    case USB_AUDIO_UAC2_SET_SPK_VOL:
    case USB_AUDIO_UAC1_SET_MIC_VOL:
    case USB_AUDIO_UAC2_SET_MIC_VOL:
        {
            T_UAC_VOL_INFO *vol_info = app_usb_get_vol_info_by_timer_id((T_APP_USB_AUDIO_TIMER_ID)timer_id);

            if (vol_info != NULL)
            {
                app_stop_timer(&vol_info->timer_idx);

                if (vol_info->vol_type == UAC_SPK_VOL)
                {
#if F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME || F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME
                    app_usb_ds_gain_set(vol_info->uac_label, vol_info->vol_value);
#else
#if F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
                    if (vol_info->uac_label == USB_AUDIO_STREAM_LABEL_1)
                    {
                        app_usb_spk_vol_sync_to_headset();
                    }
#else
                    uint16_t vol = app_usb_audio_spk_vol_transform(vol_info->vol_value);

                    app_dongle_handle_u2a_set_vol(vol);
#endif
#endif
                }
                else
                {
                    us_pipe_gain_set(vol_info->vol_value);
                }
            }
        }
        break;

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
    case USB_AUDIO_FEEDBACK_DETECT:
        {
            app_stop_timer(&timer_idx_usb_feedback_detect);

            app_usb_audio_set_feedback_state(USB_FEEDBACK_NOT_SUPPORT);
        }
        break;
#endif

    default:
        break;
    }
}

bool app_usb_audio_set_vol(T_UAC_VOL_TYPE vol_type, uint8_t label, uint16_t vol)
{
    bool ret = false;
    T_UAC_VOL_INFO *vol_info = app_usb_get_vol_info(vol_type, label);

    if (vol_info == NULL)
    {
        return false;
    }

    vol_info->vol_value = vol;

    if (vol_info->timer_idx == 0)
    {
        app_start_timer(&vol_info->timer_idx, vol_info->timer_string,
                        app_usb_audio_timer_queue_id, vol_info->timer_id, 0, false,
                        300);
    }

    return ret;
}

RAM_TEXT_SECTION uint16_t app_usb_get_usb_pcm_size(void)
{
    return usb_pcm_size_per_interval;
}

RAM_TEXT_SECTION bool app_usb_get_uac_streaming_state(T_UAC_LABEL uac_label)
{
    return (uac_label == USB_AUDIO_STREAM_LABEL_1) ? uac1_streaming : uac2_streaming;
}

uint16_t app_usb_get_dac_gain_by_level(uint8_t level)
{
    return usb_audio_dac_dig_gain_table[level];
}

void app_usb_set_usb_src_streaming(bool streaming)
{
    if (dongle_status.usb_is_streaming != streaming)
    {
        APP_PRINT_TRACE1("app_usb_set_usb_src_streaming: %d", streaming);

        dongle_status.usb_is_streaming = streaming;

#if F_APP_LEA_DONGLE_BINDING
        app_gaming_sync_dongle_status();
#endif

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
        if (streaming)
        {
            if (app_usb_audio_get_feedback_state() == USB_FEEDBACK_DETECT)
            {
                app_usb_feedback_detect_start();
            }
        }
        else
        {
            app_usb_feedback_detect_stop();
        }
#endif
    }
}

void app_usb_set_stream_silence_for_a_while(bool status)
{
    bool prev_status = dongle_status.silence_stream_for_a_while;

    APP_PRINT_TRACE2("app_usb_set_stream_silence_for_a_while: (%d->%d)", prev_status, status);

    if (status != prev_status)
    {
        dongle_status.silence_stream_for_a_while = status;

#if F_APP_LEA_DONGLE_BINDING
        app_gaming_sync_dongle_status();
#endif
    }
}

static void app_usb_host_type_sync_cback(T_OS_TYPE type)
{
    usb_audio_wrap_db.host_type = type;

    app_dongle_host_type_check(type);

#if F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
    app_usb_spk_vol_sync_to_headset();
#endif
}

void app_usb_audio_wrapper_init(void)
{

    app_timer_reg_cb(app_usb_audio_timer_cback, &app_usb_audio_timer_queue_id);

    memset(&usb_audio_wrap_db, 0, sizeof(usb_audio_wrap_db));
    usb_audio_stream_init();

    app_usb_vol_host_type_sync_register(app_usb_host_type_sync_cback);

    usb_audio_wrap_db.playback_id = usb_audio_stream_ual_bind(USB_AUDIO_STREAM_TYPE_OUT,
                                                              USB_AUDIO_STREAM_LABEL_1);
    usb_audio_wrap_db.capture_id = usb_audio_stream_ual_bind(USB_AUDIO_STREAM_TYPE_IN,
                                                             USB_AUDIO_STREAM_LABEL_1);

#ifdef ENABLE_UAC2
    usb_audio_wrap_db.playback_id_2 = usb_audio_stream_ual_bind(USB_AUDIO_STREAM_TYPE_OUT,
                                                                USB_AUDIO_STREAM_LABEL_2);
#endif

    uint8_t spk_bit_res = UAC_SPK_BIT_RES;

#if F_APP_USB_GIP_SUPPORT
    if (app_gip_get_switch_mode())
    {
        // xbox gip mode only support 16 bit res.
        spk_bit_res = UAC1_SPK_BIT_RES_GIP;
    }
#endif

    usb_pcm_size_per_interval = PCM_DS_INTERVAL * (USB_AUDIO_DS_SAMPLE_RATE / 1000) * UAC_SPK_CHAN_NUM
                                * (spk_bit_res / 8) / 1000;

    uac1_stream_timer_handle = hw_timer_create("uac1_stream_timer_handle", PCM_DS_INTERVAL, false,
                                               app_uac1_stream_timer_isr_cb);

#if ENABLE_UAC2
    uac2_stream_timer_handle = hw_timer_create("uac2_stream_timer_handle", PCM_DS_INTERVAL, false,
                                               app_uac2_stream_timer_isr_cb);
#endif

    silence_buf = calloc(1, usb_pcm_size_per_interval);
    pcm_tmp_buf = calloc(1, usb_pcm_size_per_interval);
}
#endif
