/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stdlib.h>
#include "trace.h"
#include "app_usb_uac.h"
#include "rtl876x_pinmux.h"
#include "section.h"
#include "app_io_msg.h"
#include "le_vcs_service.h"
#include "le_media_player.h"
#include "le_audio_data.h"
#include "le_audio_service.h"
#include "hw_tim.h"

#if APP_DEBUG_REPORT
#include "app_status_report.h"
#endif


#if F_APP_USB_UAC_SUPPORT && F_APP_USB_SUPPORT
#ifdef AUDIO_DUMMY_DATA_TEST
#include "os_sched.h"
#endif
#include "app_audio_path.h"
#include "app_dongle_vol.h"
#include "gaming_bt.h"
#include "app_timer.h"
#include "app_usb_uac.h"
#if DONGLE_LE_AUDIO
#include "app_le_audio.h"
#endif
#ifdef LEGACY_BT_GENERAL
#include "app_general_policy.h"
#endif

#include "app_src_policy.h"
#include "app_cfg.h"
#include "app_usb_audio_wrapper.h"

#include "app_usb_hid.h"
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
#include "uac_silence_detect.h"
#endif
#include "usb_audio_config.h"
#include "usb_audio_stream.h"
#include "mcp_def.h"

#if F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
#include "app_dongle_transmit_client.h"
#include "app_ctrl_pkt_policy.h"
#endif

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
#include "le_unicast_src_service.h"
#endif

#if F_APP_USB_GIP_SUPPORT
#include "app_gip.h"
#endif

#include "app_upstream_decode.h"

#include "app_downstream_encode.h"

#define UAC1_SET_VOL_TIMEOUT    30
#define UAC2_SET_VOL_TIMEOUT    30
#define UAC1_VOL_FLAG_SPK_SET_WAKEUP    (1 << 0)
#define UAC1_VOL_FLAG_SPK_MANIPULATING  (1 << 1)
#define UAC1_VOL_FLAG_MIC_SET_WAKEUP    (1 << 2)
#define UAC1_VOL_FLAG_MIC_MANIPULATING  (1 << 3)
#define UAC2_VOL_FLAG_SPK_SET_WAKEUP    (1 << 4)
#define UAC2_VOL_FLAG_SPK_MANIPULATING  (1 << 5)

uint8_t dongle_chnl_state = 0;

static volatile uint32_t uac1_vol_flag = 0;
static uint8_t uac1_vol = 64; /* intermediate vol */
static uint8_t uac1_mic_vol = 0;
static uint8_t timer_idx_uac1_set_vol = 0;
static uint8_t timer_idx_uac1_set_mic = 0;
static bool usb_stream_ready_to_tx = false;

#define MEM_TEST_TIMER_INTERVAL     3000
#if TARGET_RTL8773DO
#else
static uint8_t timer_idx_mem_test = 0;
#endif
extern size_t xMinimumEverFreeBytesRemaining[RAM_TYPE_NUM];

#if TARGET_RTL8773DO
#else
static size_t xPortGetMinimumEverFreeHeapSize_ext(RAM_TYPE ramType)
{
    return xMinimumEverFreeBytesRemaining[ramType];
}
#endif

#ifdef ENABLE_UAC2
#ifdef UAC_LATENCY_TEST
#define UAC_TEST_BUF_SIZE   (192 * 4)
static uint32_t uac2_test_count = 0;
static uint32_t uac1_test_count = 0;
static uint8_t *audio_test_buf;
/* One cycle of sin wave */
static const unsigned char audio_sample[192] =
{
    0x01, 0x00, 0xff, 0xff, 0x5d, 0x0d, 0x60, 0x0d, 0x7f, 0x1a, 0x7e, 0x1a,
    0x33, 0x27, 0x32, 0x27, 0x30, 0x33, 0x32, 0x33, 0x59, 0x3e, 0x56, 0x3e,
    0x68, 0x48, 0x6a, 0x48, 0x3b, 0x51, 0x3b, 0x51, 0xb0, 0x58, 0xb1, 0x58,
    0x9c, 0x5e, 0x97, 0x5e, 0xe6, 0x62, 0xef, 0x62, 0x8a, 0x65, 0x7e, 0x65,
    0x62, 0x66, 0x70, 0x66, 0x8b, 0x65, 0x7c, 0x65, 0xe5, 0x62, 0xf1, 0x62,
    0x9e, 0x5e, 0x97, 0x5e, 0xac, 0x58, 0xaf, 0x58, 0x3f, 0x51, 0x3e, 0x51,
    0x68, 0x48, 0x68, 0x48, 0x56, 0x3e, 0x54, 0x3e, 0x33, 0x33, 0x38, 0x33,
    0x30, 0x27, 0x2b, 0x27, 0x81, 0x1a, 0x86, 0x1a, 0x5d, 0x0d, 0x59, 0x0d,
    0x02, 0x00, 0x03, 0x00, 0xa0, 0xf2, 0xa1, 0xf2, 0x81, 0xe5, 0x80, 0xe5,
    0xcd, 0xd8, 0xd1, 0xd8, 0xd1, 0xcc, 0xcb, 0xcc, 0xa5, 0xc1, 0xac, 0xc1,
    0x9d, 0xb7, 0x94, 0xb7, 0xbd, 0xae, 0xc7, 0xae, 0x57, 0xa7, 0x4d, 0xa7,
    0x60, 0xa1, 0x69, 0xa1, 0x1b, 0x9d, 0x15, 0x9d, 0x76, 0x9a, 0x7a, 0x9a,
    0x9d, 0x99, 0x99, 0x99, 0x77, 0x9a, 0x7b, 0x9a, 0x1a, 0x9d, 0x15, 0x9d,
    0x62, 0xa1, 0x68, 0xa1, 0x55, 0xa7, 0x4f, 0xa7, 0xbf, 0xae, 0xc4, 0xae,
    0x9b, 0xb7, 0x98, 0xb7, 0xa7, 0xc1, 0xa8, 0xc1, 0xcf, 0xcc, 0xd0, 0xcc,
    0xcf, 0xd8, 0xcb, 0xd8, 0x7f, 0xe5, 0x87, 0xe5, 0xa4, 0xf2, 0x9a, 0xf2
};
#endif /* UAC_LATENCY_TEST */
#endif /* ENABLE_UAC2 */


#define UAC_DEBUG_SUPPORT   1
typedef enum
{
    UAC_TIME_ID_DS_MONITER,
    UAC_TIME_ID_DS_PAUSE,
    UAC1_VOL_SET_TIMER_ID,
    UAC2_VOL_SET_TIMER_ID,
    UAC1_VOL_MIC_TIMER_ID,
    UAC_TIMER_DELAY_USB_TX_WHEN_BT_PLAY,
    UAC_TIMER_CHECK_USB_STREAM,
    MEM_TEST_TIMER_ID,
    UAC_TIME_ID_DS_EXIST,
} T_UAC_TIME_ID;

#if (LEA_BIS_DUAL_UAC_SUPPORT == 1)
static uint32_t ds_uac1_trans_cnt = 0;
static uint32_t ds_uac2_trans_cnt = 0;
#endif

static uint32_t ds_data_trans_cnt = 0;

#define FL_ACTIVE_UAC1_DS       (1 << 0)
#define FL_ACTIVE_UAC2_DS       (1 << 1)
static uint8_t uac_ds_active = 0;

typedef struct
{
    uint8_t         uac_chnl;
    uint8_t         ds_streaming;
    uint8_t         us_streaming;
    uint8_t         ds_trans_enable;
    uint16_t        spk_vol;
    uint16_t        mic_vol;
    uint16_t        mic_gain_level;
    DONGLE_UAC_CB_F dongle_uac_cb_func;
#if UAC_DEBUG_SUPPORT
    uint32_t        uac_drop_ds_count;
#endif
} T_APP_UAC;

T_APP_UAC g_app_uac;

#define DS_MONITOR_TIMEOUT      (50)        //50ms
#define DS_DATA_TRANS_IDLE_MAX  (10)        //500ms(2s)
#define DS_EXIST_TIMEOUT        (50)        //50ms
#define DS_PAUSE_INTERVAL_TIMEOUT        (1000)      //2s

static uint8_t timer_idx_ds_moniter = 0;
static uint8_t timer_idx_ds_pause = 0;
static uint8_t timer_idx_ds_exist = 0;
static uint8_t usb_uac_timer_id = 0;

/* uac control status due to headset connected with phone */
T_UAC_CONTROL_STATUS uac_ctrl_status;

#if F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
/* delay usb tx when bt is playing */
#define DELAY_USB_TX_TIME         3000
static uint8_t timer_idx_delay_usb_tx = 0;
#endif

static uint8_t ds_moniter_trig_cnt = 0;

static const uint16_t app_audio_dac_gain_table[] =
{
    0x8001, 0xf280, 0xf380, 0xf480, 0xf580, 0xf680, 0xf780, 0xf880,
    0xf980, 0xfa80, 0xfb80, 0xfc80, 0xfd80, 0xfe80, 0xff80, 0x0000
};

#if ENABLE_UAC2 || F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME || \
   (LEGACY_BT_GAMING && (TARGET_LE_AUDIO_GAMING_DONGLE == 0))
#define MIC_GAIN_HIGHEST_LEVEL  100
#define MIC_GAIN_LOWEST_LEVEL   0
#endif

#define DAC_GAIN_HIGHEST_LEVEL  127
#define DAC_GAIN_LOWEST_LEVEL   0

#if ENABLE_UAC2 || F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME
static uint16_t uac1_spk_mute = 0;
#endif

#ifdef ENABLE_UAC2

static uint8_t uac2_vol = 64; /* intermediate vol */
static uint16_t uac2_spk_mute = 0;

#if 0
static const uint16_t app_audio_adc_gain_table[] =
{
    0x0000, 0x002f, 0x0037, 0x003f, 0x0047, 0x012f, 0x0137, 0x013f,
    0x0147, 0x022f, 0x0237, 0x023f, 0x0247, 0x032f, 0x0337, 0x033f
};
#endif

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof(a[0]))
#define DAC_GAIN_INTERVAL       ((DAC_GAIN_HIGHEST_LEVEL - DAC_GAIN_LOWEST_LEVEL + 1) / ARRAY_SIZE(app_audio_dac_gain_table))
#if 0
#define ADC_GAIN_HIGHEST_LEVEL  127
#define ADC_GAIN_LOWEST_LEVEL   0
#define ADC_GAIN_INTERVAL       ((ADC_GAIN_HIGHEST_LEVEL - ADC_GAIN_LOWEST_LEVEL + 1) / ARRAY_SIZE(app_audio_adc_gain_table))
#endif

static volatile uint32_t uac2_vol_flag = 0;
static uint8_t timer_idx_uac2_set_vol = 0;
static volatile uint8_t uac_downstream_state = 0;
#ifdef HW_TIMER_ISR_TIME_CONSUMPTION
static uint32_t stat_count = 0;
static uint32_t stat_accum = 0;
#endif

T_HW_TIMER_HANDLE uac1_timer_handle = NULL;
T_HW_TIMER_HANDLE uac2_timer_handle = NULL;

/* Timer load count interval 1ms ->1.5ms; 2ms ->(2+0.5)ms. */
#define UAC1_HW_TIMER_LOADCOUNT     (2000 + 500 - 1)
#define UAC2_HW_TIMER_LOADCOUNT     (2000 + 500 - 1)

//static uint8_t *dummy_buf;
#define UAC1_PCM_SIZE_FOR_CODING    512
#define UAC2_PCM_SIZE_FOR_CODING    512
#define UAC1_ISOC_BUF_SIZE  (1024 * 2)
#define UAC2_ISOC_BUF_SIZE  (1024 * 2)

#define UAC1_PCM_SIZE_FOR_LC3_CODING    960
#define UAC2_PCM_SIZE_FOR_LC3_CODING    960

/* TODO: Get the initial volume percentage from device record? */
static uint8_t game_vol_percentage = 100;
static uint8_t chat_vol_percentage = 100;

extern bool gaming_current_codec_is_lc3(void);

#endif /* end of ENABLE_UAC2 */

static void ds_moniter_timer_start(void)
{
    //APP_PRINT_INFO0("ds_moniter_timer_start");
    app_start_timer(&timer_idx_ds_moniter, "ds_moniter",
                    usb_uac_timer_id, UAC_TIME_ID_DS_MONITER, 0, false,
                    DS_MONITOR_TIMEOUT);
}

static void ds_moniter_timer_stop(void)
{
    //APP_PRINT_INFO0("ds_moniter_timer_stop");
    app_stop_timer(&timer_idx_ds_moniter);
}

void ds_pause_timer_start(void)
{
    //uac_ds_try_stream = false;
    APP_PRINT_INFO0("ds_pause_timer_start set ds_trans_enable false ");
    app_start_timer(&timer_idx_ds_pause, "ds_pause",
                    usb_uac_timer_id, UAC_TIME_ID_DS_PAUSE, 0, false,
                    DS_PAUSE_INTERVAL_TIMEOUT);
    g_app_uac.ds_trans_enable = false;
    g_app_uac.ds_streaming = false;
#if UAC_DEBUG_SUPPORT
    g_app_uac.uac_drop_ds_count = 0;
#endif
}

void ds_pause_timer_stop(void)
{
    APP_PRINT_INFO0("ds_pause_timer_stop set ds_trans_enable true ");
    g_app_uac.ds_trans_enable = true;
    app_stop_timer(&timer_idx_ds_pause);
}

void app_usb_uac_mic_vol_chg_handle(uint16_t vol)
{
    uac1_mic_vol = vol;

    uac1_vol_flag |= UAC1_VOL_FLAG_MIC_SET_WAKEUP;
    if (uac1_vol_flag & UAC1_VOL_FLAG_MIC_MANIPULATING)
    {
        return;
    }
    uac1_vol_flag |= UAC1_VOL_FLAG_MIC_MANIPULATING;

    APP_PRINT_INFO2("app_usb_uac_mic_vol_chg: vol %u, media_device:0x%x", vol,
                    app_cfg_const.dongle_media_device);

    if (app_cfg_const.dongle_media_device)
    {
        return;
    }

    app_start_timer(&timer_idx_uac1_set_mic, "uac1_set_mic",
                    usb_uac_timer_id, UAC1_VOL_MIC_TIMER_ID, 0, false,
                    UAC1_SET_VOL_TIMEOUT);
}

/* Just for testing, reading data from a PCM file. */
/* #define PCM_DATA_LEN    192
 * #include "48k16bit_123456_pcm.c"
 * static uint32_t pcm_frame_index = 0;
 * static uint8_t pcm_data_buf[PCM_DATA_LEN];
 */
RAM_TEXT_SECTION
bool app_usb_uac_cback_msg_ds_data_trans(uint8_t *data, uint16_t length, T_UAC_LABEL uac_label)
{
    if (app_cfg_const.dongle_media_device)
    {
        APP_PRINT_INFO1("app_usb_uac_cback_msg_ds_data_trans: media_device %x",
                        app_cfg_const.dongle_media_device);
        return true;
    }

    /* if (pcm_frame_index + PCM_DATA_LEN >= __48k16bit_123456_pcm_len)
     *     pcm_frame_index = 0;
     * memcpy(pcm_data_buf, __48k16bit_123456_pcm + pcm_frame_index, PCM_DATA_LEN);
     * pcm_frame_index += PCM_DATA_LEN;
     * app_audio_path_fill_async(IT_UDEV_IN1, 0, 0, pcm_data_buf, PCM_DATA_LEN);
     */
    if (!g_app_uac.ds_trans_enable)
    {
#if UAC_DEBUG_SUPPORT
        g_app_uac.uac_drop_ds_count++;
        APP_PRINT_INFO1("app_usb_uac_cback_msg_ds_data_trans drop count %x", g_app_uac.uac_drop_ds_count);
#endif
        return true;
    }

#if (UAC_SILENCE_DETECT_SUPPORT == 1)
    uac_silence_detect_proc(data, length);
#endif
    ds_data_trans_cnt ++;
    if (ds_data_trans_cnt % 100 == 0)
    {
        APP_PRINT_INFO1("app_usb_uac_cback_msg_ds_data_trans %d", ds_data_trans_cnt);
    }

#if LEGACY_BT_GAMING && (TARGET_LE_AUDIO_GAMING_DONGLE == 0)
    if (gaming_a2dp_is_force_suspend())
    {
        APP_PRINT_INFO0("app_usb_uac_cback_msg_ds_data_trans: force suspend,drop data!");
        return true;
    }
#endif

    /* TODO: Sometimes, downstream is active but there might be no data
     * transmitting from host. So we send a notification to app task as an
     * indication of stream start.
     * */
    usb_audio_stream_data_trans_msg(USB_AUDIO_STREAM_LABEL_1);

    /* only for loopback test */
    /* usb_uac_up_stream_data_write(data, 128); */

    return true;
}

bool uac_get_pipe_dac_gain(uint8_t level, uint16_t *gain)
{
    int16_t gain_val;
    int16_t j = app_audio_dac_gain_table[15] - app_audio_dac_gain_table[1];

    if (!gain)
    {
        return false;
    }

    if (level > DAC_GAIN_HIGHEST_LEVEL)
    {
        APP_PRINT_WARN2("uac_get_pipe_dac_gain: Level %u exceeds %u", level,
                        DAC_GAIN_HIGHEST_LEVEL);
        *gain = 0;
        return false;
    }

    if (level == DAC_GAIN_LOWEST_LEVEL)
    {
        *gain = app_audio_dac_gain_table[0];
    }
    else
    {
        gain_val = level * j / 127  + app_audio_dac_gain_table[1];
        *gain = (uint16_t)gain_val;
    }

    APP_PRINT_INFO2("uac2_get_dac_gain:, level %u, gain %04x",
                    level, *gain);

    return true;
}

#if ENABLE_UAC2 || F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME
#if 0
static bool uac2_get_adc_gain(uint8_t level, uint16_t *gain)
{
    uint8_t index = 0;

    if (!gain)
    {
        return false;
    }

    if (level > ADC_GAIN_HIGHEST_LEVEL)
    {
        APP_PRINT_WARN2("uac2_get_adc_gain: Level %u exceeds %u", level,
                        ADC_GAIN_HIGHEST_LEVEL);
        *gain = 0;
        return false;
    }

    index = (level + 1) / ADC_GAIN_INTERVAL;
    if (index > ARRAY_SIZE(app_audio_adc_gain_table) - 1)
    {
        APP_PRINT_ERROR2("uac2_get_adc_gain: Index %u exceeds %u", index,
                         ARRAY_SIZE(app_audio_adc_gain_table));
        return false;
    }

    *gain = app_audio_adc_gain_table[index];
    APP_PRINT_INFO3("uac2_get_adc_gain: index %u, level %u, gain %04x", index,
                    level, *gain);

    return true;
}
#endif

static bool app_usb_uac_set_gain(uint8_t it, uint8_t level)
{
    uint8_t ret = 0;
    uint16_t gain;

    if (it != IT_UDEV_IN1 && it != IT_UDEV_IN2)
    {
        ret = 1;
        goto ERR_FAIL;
    }

    if (level > 127)
    {
        ret = 2;
        goto ERR_FAIL;
    }

    if (!uac_get_pipe_dac_gain(level, &gain))
    {
        ret = 3;
        goto ERR_FAIL;
    }

ERR_FAIL:
    APP_PRINT_ERROR3("app_usb_uac_set_gain: failed, " "ret %d, level %u for it %u", ret, level, it);
    return false;
}

void app_handle_uac1_pipe_gain_init(void)
{
    if (!timer_idx_uac1_set_vol)
    {
        app_start_timer(&timer_idx_uac1_set_vol, "uac1_set_vol",
                        usb_uac_timer_id, UAC1_VOL_SET_TIMER_ID, 0, false,
                        UAC1_SET_VOL_TIMEOUT);
    }
}

#endif /* end of ENABLE_UAC2 */

void app_handle_uac_spk_vol_chg(uint16_t vol)
{
    DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);
    g_app_uac.spk_vol = vol;

    if (p_cb_func->dongle_uac_cback_spk_vol)
    {
        p_cb_func->dongle_uac_cback_spk_vol(vol);
    }
}

static void app_handle_uac_mic_vol_chg(uint16_t vol)
{
    DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);

    g_app_uac.mic_vol = vol;
    if (p_cb_func->dongle_uac_cback_mic_vol)
    {
        p_cb_func->dongle_uac_cback_mic_vol(vol);
    }
}

void app_handle_uac_active(uint32_t param)
{
    uint8_t dir = param & 0xFF;

    APP_PRINT_INFO2("app_handle_uac_active: uac active, dir %u, media_device 0x%x",
                    dir, app_cfg_const.dongle_media_device);
    if (app_cfg_const.dongle_media_device)
    {
        return;
    }

    if (dir == USB_AUDIO_STREAM_TYPE_IN)
    {
        g_app_uac.uac_chnl |= UAC_US_ACTIVE;
        g_app_uac.us_streaming = true;
        us_set_pcm_prequeue_ready(false);
    }
    else if (dir == USB_AUDIO_STREAM_TYPE_OUT)
    {
        g_app_uac.uac_chnl |= UAC_DS_ACTIVE;
        uac_ds_active |= FL_ACTIVE_UAC1_DS;
#ifdef ENABLE_UAC2
        uac_downstream_state |= FL_ACTIVE_UAC1_DS;
#endif
    }

    DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);

    if (p_cb_func->dongle_uac_cback_chnl_state)
    {
        p_cb_func->dongle_uac_cback_chnl_state(g_app_uac.uac_chnl);
    }

}

void app_handle_uac_inactive(uint32_t param)
{
    uint8_t dir = param & 0xFF;

    APP_PRINT_INFO2("app_handle_uac_inactive: uac inactive, dir %u, media_device 0x%x",
                    dir, app_cfg_const.dongle_media_device);
    if (app_cfg_const.dongle_media_device)
    {
        return;
    }

    if (dir == USB_AUDIO_STREAM_TYPE_IN)
    {
        g_app_uac.uac_chnl &= (~UAC_US_ACTIVE);
        g_app_uac.us_streaming = false;
    }
    else if (dir == USB_AUDIO_STREAM_TYPE_OUT)
    {
        g_app_uac.uac_chnl &= (~UAC_DS_ACTIVE);
        uac_ds_active &= ~FL_ACTIVE_UAC1_DS;
#ifdef ENABLE_UAC2
        uac_downstream_state &= ~FL_ACTIVE_UAC1_DS;
#endif
    }

    DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);

    if (p_cb_func->dongle_uac_cback_chnl_state)
    {
        p_cb_func->dongle_uac_cback_chnl_state(g_app_uac.uac_chnl);
    }

}

static void app_handle_uac_data_trans_ds(void)
{
    DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);
    ds_moniter_trig_cnt = 0;

#if ((UAC_SILENCE_DETECT_SUPPORT == 1) && (UAC_SILENCE_PROCESS == 1))
    if (uac_get_silence_state() == UAC_SILENCE_STATE_UNMUTE)
    {
#endif
        if ((g_app_uac.ds_streaming == false)
            && (g_app_uac.ds_trans_enable == true))
        {
            g_app_uac.ds_streaming = true;
            APP_PRINT_INFO0("ds_moniter_timeout_handle: set ds_streaming true");
            if (p_cb_func->dongle_uac_cback_stream_state)
            {
                p_cb_func->dongle_uac_cback_stream_state(true);
            }
#ifdef LEGACY_BT_GENERAL
            app_start_timer(&timer_idx_ds_exist, "ds_exist",
                            usb_uac_timer_id, UAC_TIME_ID_DS_EXIST, 0, false,
                            DS_EXIST_TIMEOUT);
#endif
        }
#if ((UAC_SILENCE_DETECT_SUPPORT == 1) && (UAC_SILENCE_PROCESS == 1))
    }
#endif
}

void uac1_handle_spk_vol_chg(uint16_t vol)
{
    static uint16_t spk_vol = 64;
    if (vol > 0x81)
    {
        vol = 0x81;
    }

    /* uac1_vol: 0 ~ 127
     * spk_vol: 0 ~ 127
     * */
    if (vol > 1)
    {
        uac1_vol = vol - 2;
        /* We should save the vol for later mute and unmute. */
        spk_vol = vol - 2;
    }
    else if (vol == 1)
    {
        /* Mute */
        uac1_vol = 0;
    }
    else
    {
        /* Unmute */
        uac1_vol = spk_vol;
    }

    uac1_vol_flag |= UAC1_VOL_FLAG_SPK_SET_WAKEUP;
    if (uac1_vol_flag & UAC1_VOL_FLAG_SPK_MANIPULATING)
    {
        return;
    }
    uac1_vol_flag |= UAC1_VOL_FLAG_SPK_MANIPULATING;
    app_start_timer(&timer_idx_uac1_set_vol,
                    "uac1_set_vol",
                    usb_uac_timer_id,
                    UAC1_VOL_SET_TIMER_ID,
                    0,
                    false,
                    UAC1_SET_VOL_TIMEOUT);
}

void uac_handle_spk_mute_chg_msg(uint16_t vol)
{
    uint16_t mute = vol;
#if ENABLE_UAC2 || F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME
    uac1_spk_mute = mute;
    uac1_handle_spk_vol_chg(uac1_spk_mute);
#else
    update_dongle_spk_mute_status(mute);
#endif
}

#ifdef ENABLE_UAC2
void app_handle_uac_2_active(uint32_t param)
{
    uint8_t dir = param & 0xFF;

    APP_PRINT_INFO1("app_handle_uac_2_active: uac active, dir %u", dir);

    if (dir == USB_AUDIO_STREAM_TYPE_IN)
    {

    }
    else if (dir == USB_AUDIO_STREAM_TYPE_OUT)
    {
        uac_downstream_state |= FL_ACTIVE_UAC2_DS;
        uac_ds_active |= FL_ACTIVE_UAC2_DS;
        ds_moniter_timer_start();
        app_start_timer(&timer_idx_mem_test, "mem test timer", usb_uac_timer_id,
                        MEM_TEST_TIMER_ID, 0, false,
                        MEM_TEST_TIMER_INTERVAL);
    }
}

void app_handle_uac_2_inactive(uint32_t param)
{
    uint8_t dir = param & 0xFF;
    APP_PRINT_INFO1("app_handle_uac_2_inactive: uac inactive, dir %u", dir);

    if (dir == USB_AUDIO_STREAM_TYPE_IN)
    {

    }
    else if (dir == USB_AUDIO_STREAM_TYPE_OUT)
    {
        uac_downstream_state &= ~FL_ACTIVE_UAC2_DS;
        uac_ds_active &= ~FL_ACTIVE_UAC2_DS;
    }
}

static void uac2_handle_spk_vol_chg(uint16_t vol)
{
    static uint16_t spk_vol = 64;
    if (vol > 0x81)
    {
        vol = 0x81;
    }

    /* uac2_vol: 0 ~ 127
     * spk_vol: 0 ~ 127
     * */
    if (vol > 1)
    {
        uac2_vol = vol - 2;
        spk_vol = vol - 2;
    }
    else if (vol == 1)
    {
        uac2_vol = 0;
    }
    else
    {
        uac2_vol = spk_vol;
    }

    uac2_vol_flag |= UAC2_VOL_FLAG_SPK_SET_WAKEUP;
    if (uac2_vol_flag & UAC2_VOL_FLAG_SPK_MANIPULATING)
    {
        return;
    }
    uac2_vol_flag |= UAC2_VOL_FLAG_SPK_MANIPULATING;

    app_start_timer(&timer_idx_uac2_set_vol,
                    "uac2_set_vol",
                    usb_uac_timer_id,
                    UAC2_VOL_SET_TIMER_ID,
                    0,
                    false,
                    UAC2_SET_VOL_TIMEOUT);
}

void uac2_handle_spk_vol_chg_msg_handle(uint16_t vol)
{
    uac2_handle_spk_vol_chg(vol);
}

void uac2_handle_spk_mute_chg_msg_handle(uint16_t vol)
{
    uint16_t mute = vol;
    uac2_spk_mute = mute;
    uac2_handle_spk_vol_chg(uac2_spk_mute);
}
#endif

void app_usb_uac_data_trans_ds_handle(void)
{
#ifdef LEGACY_BT_GAMING
#if ((UAC_SILENCE_DETECT_SUPPORT == 1) && (UAC_SILENCE_PROCESS == 1))
    if (uac_get_silence_state() == UAC_SILENCE_STATE_UNMUTE)
    {
#endif
        gaming_bt_try_start_stream();
#if ((UAC_SILENCE_DETECT_SUPPORT == 1) && (UAC_SILENCE_PROCESS == 1))
    }
#endif
#endif
    app_handle_uac_data_trans_ds();
}

#ifdef ENABLE_UAC2
void app_usb_uac_2_data_trans_ds_handle(void)
{
#ifdef LEGACY_BT_GAMING
#if ((UAC_SILENCE_DETECT_SUPPORT == 1) && (UAC_SILENCE_PROCESS == 1))
    if (uac2_get_silence_state() == UAC_SILENCE_STATE_UNMUTE)
    {
#endif
        gaming_bt_try_start_stream();
#if ((UAC_SILENCE_DETECT_SUPPORT == 1) && (UAC_SILENCE_PROCESS == 1))
    }
#endif
#endif
    app_handle_uac_data_trans_ds();
}
#endif

void app_usb_uac_ds_stream_data_stop_handle(void)
{
    APP_PRINT_INFO0("uac_handle_ds_stream_data_stop");
    ds_moniter_trig_cnt = 0;
    g_app_uac.ds_streaming = false;
    ds_moniter_timer_stop();

    DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);
    if (p_cb_func->dongle_uac_cback_stream_state)
    {
        p_cb_func->dongle_uac_cback_stream_state(false);
    }
}

static void uac_handle_ds_moniter_timeout(void)
{
    DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);

    ds_moniter_trig_cnt ++;
    if (ds_moniter_trig_cnt >= DS_DATA_TRANS_IDLE_MAX)
    {
        APP_PRINT_INFO0("ds_moniter_timeout_handle: set ds_streaming false");

        app_usb_uac_ds_stream_data_stop_handle();
        if (p_cb_func->dongle_uac_cback_stream_state)
        {
            p_cb_func->dongle_uac_cback_stream_state(false);
        }
    }
    else
    {
        ds_moniter_timer_start();
    }

}

void uac_handle_pause_interval_timeout(void)
{
    /* restart moniter timer */
    APP_PRINT_INFO0("uac_handle_pause_interval_timeout");
    app_usb_uac_ds_stream_data_stop_handle();
    ds_pause_timer_stop();
}

static void usb_uac_timeout_cb(uint8_t timer_evt, uint16_t param)
{

    switch (timer_evt)
    {
    case UAC_TIME_ID_DS_MONITER:
        {
            uac_handle_ds_moniter_timeout();
        }
        break;
    case UAC_TIME_ID_DS_PAUSE:
        {
            uac_handle_pause_interval_timeout();
        }
        break;
    case UAC1_VOL_SET_TIMER_ID:
#if ENABLE_UAC2 || F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME
        {
            bool rc = false;
            uint8_t vol;

            uac1_vol_flag &= ~UAC1_VOL_FLAG_SPK_SET_WAKEUP;
            app_stop_timer(&timer_idx_uac1_set_vol);
            timer_idx_uac1_set_vol = 0;
#if ENABLE_UAC2
            vol = (uint32_t)chat_vol_percentage * uac1_vol / 100;
#else
            vol = uac1_vol;
#endif
            if (uac1_spk_mute)
            {
                vol = 0;
            }
            rc = app_usb_uac_set_gain(IT_UDEV_IN1, vol);
            if (!rc)
            {
                APP_PRINT_ERROR1("uac_vol_set_timer_cback: Set uac1 gain %u err",
                                 uac1_vol);
            }
            uac1_vol_flag &= ~UAC1_VOL_FLAG_SPK_MANIPULATING;
            if (uac1_vol_flag & UAC1_VOL_FLAG_SPK_SET_WAKEUP)
                app_start_timer(&timer_idx_uac1_set_vol, "uac1_set_vol",
                                usb_uac_timer_id, UAC1_VOL_SET_TIMER_ID, 0, false,
                                UAC1_SET_VOL_TIMEOUT);
        }
#else
        {
            uac1_vol_flag &= ~UAC1_VOL_FLAG_SPK_SET_WAKEUP;
            app_stop_timer(&timer_idx_uac1_set_vol);
            timer_idx_uac1_set_vol = 0;
            app_handle_uac_spk_vol_chg(uac1_vol + 2);
            uac1_vol_flag &= ~UAC1_VOL_FLAG_SPK_MANIPULATING;
            if (uac1_vol_flag & UAC1_VOL_FLAG_SPK_SET_WAKEUP)
                app_start_timer(&timer_idx_uac1_set_vol, "uac1_set_vol",
                                usb_uac_timer_id, UAC1_VOL_SET_TIMER_ID, 0, false,
                                UAC1_SET_VOL_TIMEOUT);
        }
#endif
        break;
    case UAC1_VOL_MIC_TIMER_ID:
        uac1_vol_flag &= ~UAC1_VOL_FLAG_MIC_SET_WAKEUP;
        app_stop_timer(&timer_idx_uac1_set_mic);
        timer_idx_uac1_set_mic = 0;
        app_handle_uac_mic_vol_chg(uac1_mic_vol);
        uac1_vol_flag &= ~UAC1_VOL_FLAG_MIC_MANIPULATING;
        if (uac1_vol_flag & UAC1_VOL_FLAG_MIC_SET_WAKEUP)
            app_start_timer(&timer_idx_uac1_set_mic, "uac1_set_mic",
                            usb_uac_timer_id, UAC1_VOL_MIC_TIMER_ID, 0, false,
                            UAC1_SET_VOL_TIMEOUT);
        break;
#ifdef ENABLE_UAC2
    case UAC2_VOL_SET_TIMER_ID:
        {
            bool rc = false;
            uint8_t vol;

            uac2_vol_flag &= ~UAC2_VOL_FLAG_SPK_SET_WAKEUP;
            app_stop_timer(&timer_idx_uac2_set_vol);
            timer_idx_uac2_set_vol = 0;
            vol = (uint32_t)game_vol_percentage * uac2_vol / 100;
            if (uac2_spk_mute)
            {
                vol = 0;
            }
            rc = app_usb_uac_set_gain(IT_UDEV_IN2, vol);
            if (!rc)
            {
                APP_PRINT_ERROR1("uac_vol_set_timer_cback: Set uac2 gain %u err",
                                 uac2_vol);
            }
            uac2_vol_flag &= ~UAC2_VOL_FLAG_SPK_MANIPULATING;
            if (uac2_vol_flag & UAC2_VOL_FLAG_SPK_SET_WAKEUP)
                app_start_timer(&timer_idx_uac2_set_vol, "uac2_set_vol",
                                usb_uac_timer_id, UAC2_VOL_SET_TIMER_ID, 0, false,
                                UAC2_SET_VOL_TIMEOUT);
        }
        break;
#endif
    case MEM_TEST_TIMER_ID:
#if TARGET_RTL8773DO
#else
        APP_PRINT_INFO3("ITCM1 min %u, DSPSHARE min %u, BUFFER_ON %u",
                        xPortGetMinimumEverFreeHeapSize_ext(RAM_TYPE_DATA_ON),
                        xPortGetMinimumEverFreeHeapSize_ext(RAM_TYPE_DSPSHARE),
                        xPortGetMinimumEverFreeHeapSize_ext(RAM_TYPE_BUFFER_ON));
        app_start_timer(&timer_idx_mem_test, "mem test timer",
                        usb_uac_timer_id, MEM_TEST_TIMER_ID, 0, false,
                        MEM_TEST_TIMER_INTERVAL);
#endif
        break;

#if F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
    case UAC_TIMER_DELAY_USB_TX_WHEN_BT_PLAY:
        {
            app_stop_timer(&timer_idx_delay_usb_tx);
            uac_ctrl_status.delay_usb_tx_when_bt_play = false;
        }
        break;
#endif

    case UAC_TIME_ID_DS_EXIST:
        {
            DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);
            if (g_app_uac.ds_streaming == true)
            {
                if (p_cb_func->dongle_uac_cback_stream_state)
                {
                    APP_PRINT_INFO0("ds_exist_timeout_handle: set ds_streaming true");
                    p_cb_func->dongle_uac_cback_stream_state(true);
                }
            }
            else
            {
                app_stop_timer(&timer_idx_ds_exist);
            }
        }
        break;

    default:
        break;
    }
}

#if ENABLE_UAC2
void app_usb_uac_game_chat_balance(uint8_t game_vol,
                                   uint8_t chat_vol)
{
    if ((game_vol > 100) || (chat_vol > 100))
    {
        APP_PRINT_ERROR2("app_usb_uac_game_chat_balance: Incorrect "
                         "game vol %u percentage, chat vol %u percentage", game_vol, chat_vol);
    }
    if (game_vol > 100)
    {
        game_vol = 100;
    }
    if (chat_vol > 100)
    {
        chat_vol = 100;
    }

    game_vol_percentage = game_vol;
    chat_vol_percentage = chat_vol;

    if (!timer_idx_uac1_set_vol)
        app_start_timer(&timer_idx_uac1_set_vol, "uac1_set_vol",
                        usb_uac_timer_id, UAC1_VOL_SET_TIMER_ID, 0, false,
                        UAC1_SET_VOL_TIMEOUT);

    if (!timer_idx_uac2_set_vol)
        app_start_timer(&timer_idx_uac2_set_vol, "uac2_set_vol",
                        usb_uac_timer_id, UAC2_VOL_SET_TIMER_ID, 0, false,
                        UAC2_SET_VOL_TIMEOUT);
}

#endif /* ENABLE_UAC2 */

//FIX ME: sometimes only one chnl status change, but dynamic register should pass all chnl status
void dongle_uac_cback_chnl_state(uint8_t chnl)
{
    APP_PRINT_INFO2("dongle_uac_cback_chnl_state: dongle_chnl_state %x, chnl %u", dongle_chnl_state,
                    chnl);

    if ((chnl & UAC_US_ACTIVE) != (dongle_chnl_state & UAC_US_ACTIVE))
    {
#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
        uint8_t open_mic = false;
#endif

        /* us active */
        if (chnl & UAC_US_ACTIVE)
        {
#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
            open_mic = true;
#endif

#if LEGACY_BT_GAMING && ((TARGET_LE_AUDIO_GAMING_DONGLE == 0) || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT)
            gaming_bt_audio_capture_switch(true);
#endif

#if DONGLE_LE_AUDIO
            app_le_audio_set_upstream_active(true);
#endif
#ifdef LEGACY_BT_GENERAL
            app_src_uac_us_status(true);
#endif
        }
        else    /* us inactive */
        {
#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
            open_mic = false;
#endif

#if LEGACY_BT_GAMING && ((TARGET_LE_AUDIO_GAMING_DONGLE == 0) || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT)
            gaming_bt_audio_capture_switch(false);
#endif

#if DONGLE_LE_AUDIO
            app_le_audio_set_upstream_active(false);
#endif
#ifdef LEGACY_BT_GENERAL
            app_src_uac_us_status(false);
#endif
        }

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
        app_cmd_send_by_le(DONGLE_CMD_REQ_OPEN_MIC, &open_mic, 1, EARBUD_SIDE_ANY);
#endif
    }
    dongle_chnl_state = chnl;
}

void dongle_uac_cback_stream_state(uint8_t streaming)
{
    APP_PRINT_TRACE1("dongle_uac_cback_stream_state: streaming %d", streaming);

    if (streaming)
    {
#ifdef LEGACY_BT_GENERAL
        app_handle_music_data_notify();
#endif
#if LE_AUDIO_MCP_SERVER_SUPPORT
        /* FIXME: conversation mode not support mcp play */
        if ((dongle_chnl_state & UAC_US_ACTIVE) == 0)
        {
            le_media_player_set_state(MCS_MEDIA_STATE_PLAYING);
        }
#endif
#if DONGLE_LE_AUDIO
        app_le_audio_set_downstream_active(true);
#endif
    }
    else
    {
#if LE_AUDIO_MCP_SERVER_SUPPORT
        le_media_player_set_state(MCS_MEDIA_STATE_PAUSED);
#endif
#if DONGLE_LE_AUDIO
        app_le_audio_set_downstream_active(false);
#endif
    }
}

void dongle_uac_cback_spk_vol(uint16_t vol)
{
    app_dongle_handle_u2a_set_vol(vol);
}

void dongle_uac_cback_mic_vol(uint16_t vol)
{
#if LE_AUDIO_VCS_SUPPORT
    le_vcs_set_in_volume(vol);
#endif
}

#if F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
void app_usb_delay_usb_tx_start(void)
{
    app_start_timer(&timer_idx_delay_usb_tx,
                    "delay_usb_tx_start",
                    usb_uac_timer_id,
                    UAC_TIMER_DELAY_USB_TX_WHEN_BT_PLAY,
                    0,
                    false,
                    DELAY_USB_TX_TIME);
}
#endif

void app_usb_uac_clear_headset_status(void)
{
    memset(&headset_status, 0, sizeof(headset_status));

    ble_audio_handle_conn_update_for_headset_stream_status_change(&headset_status);
}

void app_usb_set_stream_state_to_tx(bool streaming)
{
    APP_PRINT_TRACE1("app_usb_set_stream_state_to_tx: %d", streaming);

    if (usb_stream_ready_to_tx == false && streaming)
    {
        app_usb_ds_handle_stream_start();
    }

    if (usb_stream_ready_to_tx && streaming == false)
    {
        app_usb_uac_stream_stop_handle();

        app_usb_ds_handle_stream_stop();
    }

    usb_stream_ready_to_tx = streaming;
}

RAM_TEXT_SECTION bool app_usb_stream_xmit_out(uint8_t *data, uint16_t len, T_UAC_LABEL uac_label)
{
    app_usb_ds_rcv_pcm(data, len, uac_label);

    return true;
}

void app_usb_uac_register_cbs(DONGLE_UAC_CB_F *p_func)
{
    DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);
    p_cb_func->dongle_uac_cback_chnl_state = p_func->dongle_uac_cback_chnl_state;
    p_cb_func->dongle_uac_cback_stream_state = p_func->dongle_uac_cback_stream_state;
    p_cb_func->dongle_uac_cback_spk_vol = p_func->dongle_uac_cback_spk_vol;
    p_cb_func->dongle_uac_cback_mic_vol = p_func->dongle_uac_cback_mic_vol;
    //update uac status

    if (p_cb_func->dongle_uac_cback_chnl_state)
    {
        p_cb_func->dongle_uac_cback_chnl_state(g_app_uac.uac_chnl);
    }

    if (p_cb_func->dongle_uac_cback_stream_state)
    {
        p_cb_func->dongle_uac_cback_stream_state(g_app_uac.ds_streaming);
    }

    if (p_cb_func->dongle_uac_cback_spk_vol)
    {
        p_cb_func->dongle_uac_cback_spk_vol(g_app_uac.spk_vol);
    }

    if (p_cb_func->dongle_uac_cback_mic_vol)
    {
        p_cb_func->dongle_uac_cback_mic_vol(g_app_uac.mic_vol);
    }
}

void app_usb_uac_unregister_cbs(void)
{
    DONGLE_UAC_CB_F *p_cb_func = &(g_app_uac.dongle_uac_cb_func);
    p_cb_func->dongle_uac_cback_chnl_state = NULL;
    p_cb_func->dongle_uac_cback_stream_state = NULL;
    p_cb_func->dongle_uac_cback_spk_vol = NULL;
    p_cb_func->dongle_uac_cback_mic_vol = NULL;
}


uint8_t app_get_usb_ds_state(void)
{
    return g_app_uac.ds_streaming;
}

RAM_TEXT_SECTION uint8_t app_get_usb_us_state(void)
{
    return g_app_uac.us_streaming;
}

RAM_TEXT_SECTION uint8_t app_get_usb_stream_ready_to_tx(void)
{
    return usb_stream_ready_to_tx;
}

void app_usb_uac_stream_stop_handle(void)
{
    APP_PRINT_TRACE0("app_usb_uac_stream_stop_handle");

#if F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
    app_stop_timer(&timer_idx_delay_usb_tx);
    uac_ctrl_status.delay_usb_tx_when_bt_play = false;
#endif

#ifdef LEGACY_BT_GENERAL
    src_handle_a2dp_stream_suspend();
#endif

}

void app_usb_uac_init(void)
{

    app_usb_audio_wrapper_init();

    memset(&g_app_uac, 0, sizeof(T_APP_UAC));
    g_app_uac.ds_trans_enable = true;

    DONGLE_UAC_CB_F uac_cb_func =
    {
        dongle_uac_cback_chnl_state,
        dongle_uac_cback_stream_state,
        dongle_uac_cback_spk_vol,
        dongle_uac_cback_mic_vol,
    };

    app_usb_uac_register_cbs(&uac_cb_func);
    app_timer_reg_cb(usb_uac_timeout_cb, &usb_uac_timer_id);

#if F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME || F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME || (F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET == 0)
    dongle_status.volume_sync_to_headset = false;
#else
    dongle_status.volume_sync_to_headset = true;
#endif

    return;
}
#endif
