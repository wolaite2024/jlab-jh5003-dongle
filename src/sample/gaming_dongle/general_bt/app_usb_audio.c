/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "app_usb_audio.h"
#include "app_main.h"
#include "app_io_msg.h"
#include "app_usb_audio_buf.h"
#include "usb_lib_ext.h"
//#include "audio_codec.h"
#include "os_mem.h"
#include "app_src_policy.h"
#include "rtl876x_pinmux.h"
#include "section.h"
#include "app_vendor_cfg.h"
#ifdef AUDIO_DUMMY_DATA_TEST
#include "os_sched.h"
#endif

#ifdef AUDIO_DELAY_TEST_SCO_US
static bool first_sco_us_enc_pkt = true;
static bool first_sco_us_dec_pkt = true;
static bool first_sco_us_usb_pkt = true;
#endif

#ifdef AUDIO_DELAY_TEST_SCO_DS
static bool first_sco_ds_usb_pkt = true;
static bool first_sco_ds_dec_pkt = true;
static bool first_sco_ds_enc_pkt = true;
#endif

#ifdef AUDIO_DELAY_TEST_MUSIC
static bool first_music_usb_pkt = true;
static bool first_music_dec_pkt = true;
static bool first_music_enc_pkt = true;
#endif

#define APP_USB_AUDIO_DEBUG         0

#define     CHAN_MODE_MONO          0
#define     CHAN_MODE_STEREO        1

#define     DS_RING_BUF_LEN         1*1024
#define     US_RING_BUF_LEN         1*1024

#define     DATA_IND_TYPE_DEC       0x08
#define     DATA_IND_TYPE_ENC       0x09
#define     DATA_ACK_TYPE_DEC       0x0A
#define     DATA_ACK_TYPE_ENC       0x0B

static RINGBUF_T    usb_audio_buf_ds;
static RINGBUF_T    usb_audio_buf_us;

static uint16_t     sf_idx_ds = 4;  //48k
static uint16_t     sf_idx_us = 4;  //48k
static uint16_t     frame_size_ds;
static uint16_t     frame_size_us;

static uint16_t     enc_frame_len = 0;
static uint8_t      usb_connected = 0;


#define SBC_ENC_FRAME_SIZE          512
#define MSBC_ENC_FRAME_SIZE         (240 * 2)
typedef struct app_uac_codec
{
    void                *handle;
    bool                is_busy;
} T_APP_UAC_CODEC;

T_APP_UAC_CODEC app_audio_codec = {.handle = NULL, .is_busy = false};

T_APP_UAC_CODEC app_audio_codec_msbc_us = {.handle = NULL, .is_busy = false};

T_APP_UAC_CODEC app_audio_codec_msbc_ds = {.handle = NULL, .is_busy = false};


#define USB_AUDIO_DS_MASK   0x01
#define USB_AUDIO_US_MASK   0x02

#define MAX_US_PKT_LOSS_COUNT   0x04

static uint8_t audio_stream_mask = 0;

static bool src_1st_mSBC_received = false;
static uint8_t src_consecutive_pkt_loss_count = 0;

static const uint16_t frame_size_tbl[2][2][8] =
{
    // 16 bit
    {
        {16, 32, 64, 90, 96, 176, 192, 384},      // frame_size_2[0][0][0] = 8k, 1mono, 16 bits
        {32, 64, 128, 180, 192, 352, 384, 768}   // frame_size_2[0][1][0] = 8k, stereo, 16 bits
    },
    // 24 bit -> represet as 32 bit
    {
        {24, 48, 96, 135, 144, 264, 288, 576},      // frame_size_2[1][0][0] = 8k, 1mono, 24 bits
        {48, 96, 192, 270, 288, 528, 576, 1152}   // frame_size_2[1][1][0] = 8k, stereo, 24 bits
    }
};

typedef enum
{
    MSG_TYPE_PLUG = 0,
    MSG_TYPE_UNPLUG,
    MSG_TYPE_UAC_ACTIVE,
    MSG_TYPE_UAC_INACTIVE,
    MSG_TYPE_UAC_DATA_TRANS_DS,
    MSG_TYPE_UAC_DATA_TRANS_US,
    MSG_TYPE_UAC_SET_VOL,
    MSG_TYPE_UAC_GET_VOL,

    MSG_TYPE_UAC_CODEC,

    MSG_TYPE_NONE = 0xff,
} USB_MSG_TYPE_T;

static bool usb_audio_plug(uint32_t param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB_UAC;
    gpio_msg.subtype = MSG_TYPE_PLUG;
    gpio_msg.u.param = param;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("app_usb_audio_plug: msg send fail");
        return false;
    }

    return true;
}

static bool usb_audio_unplug(void)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB_UAC;
    gpio_msg.subtype = MSG_TYPE_UNPLUG;
    gpio_msg.u.param = 0;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("app_usb_audio_plug: msg send fail");
        return false;
    }

    return true;
}

static bool usb_audio_active(uint8_t audio_path, uint8_t bit_res, uint8_t sf, uint8_t chann_mode)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB_UAC;
    gpio_msg.subtype = MSG_TYPE_UAC_ACTIVE;
    gpio_msg.u.param = (audio_path | bit_res << 8 | sf << 16);
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("app_usb_audio_plug: msg send fail");
        return false;
    }

    return true;
}

static bool usb_audio_inactive(uint8_t audio_path)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB_UAC;
    gpio_msg.subtype = MSG_TYPE_UAC_INACTIVE;
    gpio_msg.u.param = audio_path;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("app_usb_audio_unplug: msg send fail");
        return false;
    }

    return true;
}
#ifdef AUDIO_DUMMY_DATA_TEST
extern int test_pcm_data_len;
extern unsigned char const test_pcm_data[];
static int test_pcm_idx = 0;
#endif

RAM_TEXT_SECTION
static bool usb_audio_data_trans(void *buf, uint8_t audio_path)
{
    static uint16_t us_null_counter = 0;
    static uint16_t ds_full_counter = 0;
#ifdef AUDIO_DUMMY_DATA_TEST
    static uint32_t last_timer = 0;
    static uint32_t max_interval = 0, times = 0;;
#endif
    bool ret = false;

    if (audio_path == DOWN_STREAM)
    {
        T_IO_MSG gpio_msg;

        gpio_msg.type = IO_MSG_TYPE_USB_UAC;
        gpio_msg.subtype = MSG_TYPE_UAC_DATA_TRANS_DS;
        gpio_msg.u.buf = (uint8_t *)buf;
#ifdef AUDIO_DUMMY_DATA_TEST
        {
            uint32_t current_timer = os_sys_time_get();
            times ++;
            if (last_timer == 0)
            {
                last_timer = current_timer;
            }
            if (((current_timer - last_timer) > 50) || ((times % 200) == 0))
            {
                APP_PRINT_INFO4("usb_audio_data_trans last_timer:%d current_timer:%d interval: %d(%d)",
                                last_timer, current_timer, (current_timer - last_timer), max_interval);
            }
            if ((current_timer - last_timer) > max_interval)
            {
                APP_PRINT_INFO4("usb_audio_data_trans last_timer:%d current_timer:%d interval: %d(%d)",
                                last_timer, current_timer, (current_timer - last_timer), max_interval);
                max_interval = (current_timer - last_timer);
            }
            last_timer = current_timer;
        }
#endif
        if (app_io_msg_send(&gpio_msg) == false)
        {
            APP_PRINT_ERROR0("app_usb_audio_data_trans: msg send fail");
            return false;
        }

        if (usb_audio_buf_ds.buf != NULL)
        {

#ifdef AUDIO_DUMMY_DATA_TEST
            if ((test_pcm_idx + frame_size_ds) <= test_pcm_data_len)
            {
                memcpy((unsigned char *)buf, &test_pcm_data[test_pcm_idx], frame_size_ds);
                test_pcm_idx += frame_size_ds;
                test_pcm_idx %= test_pcm_data_len;
            }
            else
            {
                int i = test_pcm_data_len - test_pcm_idx;
                unsigned char *p = buf;
                memcpy(p, &test_pcm_data[test_pcm_idx], i);
                memcpy(&p[i], &test_pcm_data[0], frame_size_ds - i);
                test_pcm_idx = frame_size_ds - i;
            }
#endif
            if (!ringbuf_write(&usb_audio_buf_ds, buf, frame_size_ds))
            {
                ds_full_counter += 16;

                if (ds_full_counter == 0)
                {
                    APP_PRINT_INFO0("usb_audio_data_trans: ds full");
                }
            }
            else
            {
                ds_full_counter = 0;
            }
        }
    }
    else if (audio_path == UP_STREAM)
    {
        if ((usb_audio_buf_us.buf != NULL) &&
#if 0
            (usb_audio_buf_us.data_len >= frame_size_us))
#else
            (ringbuf_dataspace(&usb_audio_buf_us) >= frame_size_us))
#endif
        {
#ifdef AUDIO_DELAY_TEST_SCO_US
            if (first_sco_us_usb_pkt)
            {
                Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                first_sco_us_usb_pkt = false;
            }
#endif

            us_null_counter = 0;

            ret = ringbuf_read(&usb_audio_buf_us, buf, frame_size_us);

            return ret;
        }
        else
        {
            us_null_counter += 16;

            //if (us_null_counter == 0)
            {
                APP_PRINT_INFO0("usb_audio_data_trans: us no data");
            }

            memset(buf, 0, frame_size_us);

            return false;
        }
    }

    return true;
}

static bool usb_audio_set_vol(uint32_t param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB_UAC;
    gpio_msg.subtype = MSG_TYPE_UAC_SET_VOL;
    gpio_msg.u.param = param;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("app_usb_audio_plug: msg send fail");
        return false;
    }

    return true;
}

static bool usb_audio_get_vol(uint32_t param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB_UAC;
    gpio_msg.subtype = MSG_TYPE_UAC_GET_VOL;
    gpio_msg.u.param = param;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("app_usb_audio_plug: msg send fail");
        return false;
    }

    return true;
}

static uint16_t usb_audio_get_frame_size(uint8_t sf_idx, uint8_t bit_res, uint8_t chan_mode)
{
    return frame_size_tbl[bit_res][chan_mode][sf_idx];
}

#define         PARAM_MASK              (0xffff0000)
#define         EVENT_MASK              (~PARAM_MASK)
#define         PARAM_BITS              (16)

RAM_TEXT_SECTION
bool audio_codec_callback(T_AUDIO_CODEC_PIPE_HANDLE handle, \
                          T_AUDIO_CODEC_PIPE_EVENT  event, \
                          uint32_t  param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB_UAC;
    gpio_msg.subtype = MSG_TYPE_UAC_CODEC;
    gpio_msg.u.param = (event & EVENT_MASK) | ((param << PARAM_BITS)&PARAM_MASK);
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("audio_codec_callback msg send fail");
    }
    return true;
}

static void app_usb_handle_msg_plug(void *buf)
{
    //app_led_set_mode(APP_LED_MODE_FAIL_OR_NO_CONNECTION_WITH_TEAMS);
}


static void app_usb_handle_msg_unplug(void *buf)
{

}

static void app_usb_handle_msg_uac_active(void *buf)
{
    uint32_t param = (uint32_t)buf;
    uint8_t audio_path;
    uint8_t bit_res;

    audio_path = (uint8_t)param;
    bit_res = (uint8_t)(param >> 8);

    if (audio_path == DOWN_STREAM)
    {
        sf_idx_ds = (uint8_t)(param >> 16);
        frame_size_ds = usb_audio_get_frame_size(sf_idx_ds, bit_res, CHAN_MODE_STEREO);
        audio_stream_mask |= USB_AUDIO_DS_MASK;
    }
    else if (audio_path == UP_STREAM)
    {
        sf_idx_us = (uint8_t)(param >> 16);
        frame_size_us = usb_audio_get_frame_size(sf_idx_us, bit_res, CHAN_MODE_MONO);
        audio_stream_mask |= USB_AUDIO_US_MASK;

        app_notify_policy_us_audio_status(true);
    }
    APP_PRINT_INFO6("app_usb_handle_msg_uac_active:%x-%x-%x-%x-%x-%x", param, audio_path, bit_res,
                    sf_idx_ds, \
                    sf_idx_us, frame_size_us);

}

static bool created = false;
static uint8_t app_usb_audio_get_block_length_from_config(uint8_t block_length)
{
    uint8_t bl = 0;

    switch (block_length)
    {
    case 4:
        bl = 0x00;
        break;

    case 8:
        bl = 0x01;
        break;

    case 12:
        bl = 0x02;
        break;

    case 16:
        bl = 0x03;
        break;

    default:
        APP_PRINT_WARN1("app_usb_audio_get_block_length_from_config: unknown block length %02x",
                        block_length);
        break;
    }

    return bl;
}

static uint8_t app_usb_audio_get_channel_mode_from_config(T_AUDIO_SBC_CHANNEL_MODE chann_mode)
{
    uint8_t cm = 0;

    switch (chann_mode)
    {
    case AUDIO_SBC_CHANNEL_MODE_MONO:
        cm = 0x00;
        break;

    case AUDIO_SBC_CHANNEL_MODE_DUAL:
        cm = 0x01;
        break;

    case AUDIO_SBC_CHANNEL_MODE_STEREO:
        cm = 0x02;
        break;

    case AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO:
        cm = 0x03;
        break;

    default:
        APP_PRINT_WARN1("app_usb_audio_get_channel_mode_from_config: unknown channel mode %02x",
                        chann_mode);
        break;
    }

    return cm;
}

static uint8_t app_usb_audio_get_subbands_from_config(uint8_t subband_num)
{
    uint8_t sbs = 0;

    switch (subband_num)
    {
    case 4:
        sbs = 0;
        break;

    case 8:
        sbs = 1;
        break;

    default:
        APP_PRINT_WARN1("app_usb_audio_get_subbands_from_config: unknown subbands %02x",
                        subband_num);
        break;
    }

    return sbs;
}


void app_usb_audio_music_create(T_AUDIO_FORMAT_INFO cfg_snk_info)
{
    if (app_audio_codec.handle == NULL)
    {
#ifdef AUDIO_DELAY_TEST_MUSIC
        first_music_usb_pkt = true;
        first_music_dec_pkt = true;
        first_music_enc_pkt = true;
#endif
        APP_PRINT_INFO0("app_usb_audio_music_create");

        T_AUDIO_FORMAT_INFO src_info;
        T_AUDIO_FORMAT_INFO snk_info;
        //char sbc_param[2] = {0xbd, 30};
        src_info.type = AUDIO_FORMAT_TYPE_PCM;
        //src_info.attr.pcm.sample_rate = sf_idx_ds;

        src_info.attr.pcm.sample_rate = 48000;
        src_info.attr.pcm.frame_length = 512;
        src_info.attr.pcm.chann_num = 2;
        src_info.attr.pcm.bit_width = 16;

        snk_info.type = cfg_snk_info.type;
        snk_info.attr.sbc.sample_rate = src_info.attr.pcm.sample_rate;
        snk_info.attr.sbc.allocation_method = cfg_snk_info.attr.sbc.allocation_method;
        snk_info.attr.sbc.bitpool = cfg_snk_info.attr.sbc.bitpool;

        snk_info.attr.sbc.block_length =
            app_usb_audio_get_block_length_from_config(cfg_snk_info.attr.sbc.block_length);

        snk_info.attr.sbc.chann_mode =
            (T_AUDIO_SBC_CHANNEL_MODE)app_usb_audio_get_channel_mode_from_config(
                cfg_snk_info.attr.sbc.chann_mode);

        snk_info.attr.sbc.subband_num =
            app_usb_audio_get_subbands_from_config(cfg_snk_info.attr.sbc.subband_num);

        enc_frame_len = SBC_ENC_FRAME_SIZE;

        /*
                snk_info.type = AUDIO_FORMAT_TYPE_SBC;
                snk_info.attr.sbc.allocation_method = (sbc_param[0] >> 1) & 0x01;
                snk_info.attr.sbc.block_length = (sbc_param[0] >> 4) & 0x03;
                snk_info.attr.sbc.chann_mode = (T_AUDIO_SBC_CHANNEL_MODE)((sbc_param[0] >> 2) & 0x03);
                snk_info.attr.sbc.bitpool = sbc_param[1];
                snk_info.attr.sbc.sample_rate = src_info.attr.pcm.sample_rate;
                snk_info.attr.sbc.subband_num = sbc_param[0] & 0x01;

                enc_frame_len = SBC_ENC_FRAME_SIZE;
        */
        app_audio_codec.handle = audio_codec_pipe_create(src_info, snk_info, audio_codec_callback);
        app_audio_codec.is_busy = true;
    }
    else
    {
        APP_PRINT_WARN0("app_usb_audio_music_create: already created");
    }
}

void app_usb_audio_music_destroy(void)
{
    if (app_audio_codec.handle != NULL)
    {
        APP_PRINT_INFO0("app_usb_audio_music_destroy");

        audio_codec_pipe_release(app_audio_codec.handle);
        app_audio_codec.handle = NULL;
        app_audio_codec.is_busy = false;
        enc_frame_len = 0;

        created = false;
    }
    else
    {
        APP_PRINT_WARN0("app_usb_audio_music_destroy: not created!");
    }
}

void app_usb_audio_msbc_ds_create(void)
{
    if (app_audio_codec_msbc_ds.handle == NULL)
    {
#ifdef AUDIO_DELAY_TEST_SCO_DS
        first_sco_ds_usb_pkt = true;
        first_sco_ds_dec_pkt = true;
        first_sco_ds_enc_pkt = true;
#endif
        APP_PRINT_INFO0("app_usb_audio_msbc_ds_create");

        T_AUDIO_FORMAT_INFO src_info;
        T_AUDIO_FORMAT_INFO snk_info;
        //char sbc_param[2] = {0xbd, 35};
        src_info.type = AUDIO_FORMAT_TYPE_PCM;
        //src_info.attr.pcm.sample_rate = sf_idx_ds;
        src_info.attr.pcm.sample_rate = 48000;
        src_info.attr.pcm.frame_length = 512;
        src_info.attr.pcm.chann_num = 2;
        src_info.attr.pcm.bit_width = 16;

        snk_info.type = AUDIO_FORMAT_TYPE_MSBC;
        snk_info.attr.msbc.sample_rate = src_info.attr.pcm.sample_rate;
        snk_info.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;

        enc_frame_len = MSBC_ENC_FRAME_SIZE;

        app_audio_codec_msbc_ds.handle = audio_codec_pipe_create(src_info, snk_info, audio_codec_callback);
        app_audio_codec_msbc_ds.is_busy = true;
    }
    else
    {
        APP_PRINT_WARN0("app_usb_audio_msbc_ds_create: already created!");
    }
}

void app_usb_audio_msbc_ds_destroy(void)
{
    if (app_audio_codec_msbc_ds.handle != NULL)
    {
        APP_PRINT_INFO0("app_usb_audio_msbc_ds_destroy");

        audio_codec_pipe_release(app_audio_codec_msbc_ds.handle);
        app_audio_codec_msbc_ds.handle = NULL;
        app_audio_codec_msbc_ds.is_busy = false;
        enc_frame_len = 0;

        created = false;
    }
    else
    {
        APP_PRINT_WARN0("app_usb_audio_msbc_ds_destroy: not created!");
    }
}

void app_usb_audio_msbc_us_create(void)
{
    T_AUDIO_FORMAT_INFO src_info;
    T_AUDIO_FORMAT_INFO snk_info;

    if (app_audio_codec_msbc_us.handle == NULL)
    {
#ifdef AUDIO_DELAY_TEST_SCO_US
        first_sco_us_enc_pkt = true;
        first_sco_us_dec_pkt = true;
        first_sco_us_usb_pkt = true;
#endif
        APP_PRINT_INFO0("app_usb_audio_msbc_us_create");

        src_info.type = AUDIO_FORMAT_TYPE_MSBC;
        //src_info.attr.msbc.sample_rate = 1;//ICODEC_SR_16K
        src_info.attr.msbc.sample_rate = sf_idx_us;
        src_info.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;

        snk_info.type = AUDIO_FORMAT_TYPE_PCM;
        snk_info.attr.pcm.sample_rate = 1;

        app_audio_codec_msbc_us.handle = audio_codec_pipe_create(src_info, snk_info, audio_codec_callback);
        app_audio_codec_msbc_us.is_busy = true;

        src_1st_mSBC_received = false;
        src_consecutive_pkt_loss_count = 0;
    }
    else
    {
        APP_PRINT_WARN0("app_usb_audio_msbc_us_create: already created!");
    }
}

void app_usb_audio_msbc_us_destroy(void)
{
    if (app_audio_codec_msbc_us.handle != NULL)
    {
        APP_PRINT_INFO0("app_usb_audio_msbc_us_destroy");

        audio_codec_pipe_release(app_audio_codec_msbc_us.handle);
        app_audio_codec_msbc_us.handle = NULL;
        app_audio_codec_msbc_us.is_busy = false;

        created = false;
    }
    else
    {
        APP_PRINT_WARN0("app_usb_audio_msbc_us_destroy: not created!");
    }
}


RAM_TEXT_SECTION
void app_usb_audio_msbc_fill_us(uint8_t *buf, uint16_t len)
{
    static uint32_t fill_seq = 0;

#if 1
    uint8_t msbc_pkt[57] = {0};
#else
    uint8_t msbc_pkt[57] = {0xad, 0x00, 0x00, 0xc9, 0x22, 0x10, 0x10, 0x10,
                            0x5c, 0xfb, 0x6a, 0x1d, 0x28, 0xe0, 0x85, 0xaf,
                            0xc5, 0xa9, 0x54, 0x8f, 0x37, 0x9e, 0xfa, 0xe1,
                            0xe0, 0x44, 0xf7, 0x77, 0xee, 0xc5, 0x99, 0x93,
                            0xce, 0x75, 0x5d, 0x34, 0xd6, 0x0d, 0xb5, 0x84,
                            0x95, 0x27, 0x39, 0xe1, 0x35, 0xf0, 0x59, 0x4b,
                            0x0d, 0x9c, 0x8e, 0xb4, 0xf4, 0xb3, 0xb0, 0xaa,
                            0x64
                           };
#endif
    //APP_PRINT_INFO4("app_usb_audio_fill_us: %p, %d, %d, %d",
    //                buf, len, app_audio_codec_msbc_us.is_busy,
    //                app_audio_codec_msbc_us.cur_scene);

    if ((buf == NULL) || (len != 57))
    {
        return;
    }

    if ((app_audio_codec_msbc_us.handle != NULL) &&
        (app_audio_codec_msbc_us.is_busy == false))
    {
#ifdef AUDIO_DELAY_TEST_SCO_US
        if (first_sco_us_enc_pkt)
        {
            Pad_Config(P2_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            Pad_Config(P2_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
            first_sco_us_enc_pkt = false;
        }
#endif

        //check sync word
        if (buf[0] == 0xad)
        {
            fill_seq++;

            //APP_PRINT_INFO1("fill seq: %d", fill_seq);

#if 1
            memcpy(msbc_pkt, buf, 57);
#endif
            audio_codec_pipe_fill(app_audio_codec_msbc_us.handle, msbc_pkt, 57);
            app_audio_codec_msbc_us.is_busy = true;

            if (!src_1st_mSBC_received)
            {
                src_1st_mSBC_received = true;
            }

            src_consecutive_pkt_loss_count = 0;
        }
        else
        {
            memset(msbc_pkt, 0, 57);

            if (!src_1st_mSBC_received)
            {
                audio_codec_pipe_zero(app_audio_codec_msbc_us.handle, msbc_pkt, 57);
            }
            else
            {
                src_consecutive_pkt_loss_count++;

                if (src_consecutive_pkt_loss_count > MAX_US_PKT_LOSS_COUNT)
                {
                    audio_codec_pipe_zero(app_audio_codec_msbc_us.handle, msbc_pkt, 57);
                }
                else
                {
                    audio_codec_pipe_loss(app_audio_codec_msbc_us.handle, msbc_pkt, 57);
                }
            }

            app_audio_codec_msbc_us.is_busy = true;
        }
    }
    else
    {
        APP_PRINT_ERROR2("app_usb_audio_msbc_fill_us: error, %p, %d",
                         app_audio_codec_msbc_us.handle, app_audio_codec_msbc_us.is_busy);
    }
}


RAM_TEXT_SECTION
static void app_fill_data_to_codec(void)
{
#ifdef AUDIO_DUMMY_DATA_TEST
    static uint32_t last_timer = 0;
    static uint32_t max_interval = 0;
    static uint32_t times = 0;
#endif
    if (ringbuf_dataspace(&usb_audio_buf_ds) >= enc_frame_len)

    {
#ifdef AUDIO_DUMMY_DATA_TEST
        {
            uint32_t current_timer = os_sys_time_get();
            times ++;
            if (last_timer == 0)
            {
                last_timer = current_timer;
            }
            if (((current_timer - last_timer) > 50) || ((times % 200) == 0))
            {
                APP_PRINT_INFO4("AUDIO_DUMMY_DATA_TEST:app_fill_data_to_codec last_timer:%d current_timer:%d interval: %d(%d)",
                                last_timer, current_timer, (current_timer - last_timer), max_interval);
            }
            if ((current_timer - last_timer) > max_interval)
            {
                APP_PRINT_INFO4("AUDIO_DUMMY_DATA_TEST:app_fill_data_to_codec last_timer:%d current_timer:%d interval: %d(%d)",
                                last_timer, current_timer, (current_timer - last_timer), max_interval);
                max_interval = (current_timer - last_timer);
            }
            last_timer = current_timer;
        }
#endif
        if (app_audio_codec.handle != NULL &&
            app_audio_codec.is_busy == false)
        {
#if defined(APP_USB_AUDIO_DEBUG) && (APP_USB_AUDIO_DEBUG == 1)
            APP_PRINT_INFO5("app_usb_handle_msg_data_trans_ds:%d-%d-%d-%x-%x",
                            created,
                            usb_audio_buf_ds.data_len,
                            enc_frame_len,
                            app_audio_codec.handle,
                            app_audio_codec.is_busy);
#endif

#ifdef AUDIO_DELAY_TEST_MUSIC
            if (first_music_dec_pkt)
            {
                Pad_Config(P2_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                Pad_Config(P2_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                first_music_dec_pkt = false;
            }
#endif

            uint8_t *buf = os_mem_alloc(RAM_TYPE_DTCM0, enc_frame_len);
            if (buf != NULL)
            {
                ringbuf_read(&usb_audio_buf_ds, buf, enc_frame_len);
                extern int8_t app_src_os_spk_is_mute;
                if (app_src_os_spk_is_mute)
                {
                    memset(buf, 0, enc_frame_len);
                }
                audio_codec_pipe_fill(app_audio_codec.handle, buf, enc_frame_len);
                os_mem_free(buf);
                app_audio_codec.is_busy = true;
            }
            else
            {
                APP_PRINT_ERROR2("%s:%d os_mem_alloc return NULL", __FUNCTION__, __LINE__);
            }
        }
        else if (app_audio_codec_msbc_ds.handle != NULL &&
                 app_audio_codec_msbc_ds.is_busy == false)
        {
#if defined(APP_USB_AUDIO_DEBUG) && (APP_USB_AUDIO_DEBUG == 1)
            APP_PRINT_INFO5("app_usb_handle_msg_data_trans_ds:%d-%d-%d-%x-%x",
                            created,
                            usb_audio_buf_ds.data_len,
                            enc_frame_len,
                            app_audio_codec_msbc_ds.handle,
                            app_audio_codec_msbc_ds.is_busy);
#endif

#ifdef AUDIO_DELAY_TEST_SCO_DS
            if (first_sco_ds_dec_pkt)
            {
                Pad_Config(P2_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                Pad_Config(P2_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                first_sco_ds_dec_pkt = false;
            }
#endif

            uint8_t *buf = os_mem_alloc(RAM_TYPE_DTCM0, enc_frame_len);
            if (buf != NULL)
            {
                ringbuf_read(&usb_audio_buf_ds, buf, enc_frame_len);
                extern int8_t app_src_os_spk_is_mute;
                if (app_src_os_spk_is_mute)
                {
                    memset(buf, 0, enc_frame_len);
                }
                audio_codec_pipe_fill(app_audio_codec_msbc_ds.handle, buf, enc_frame_len);
                os_mem_free(buf);
                app_audio_codec_msbc_ds.is_busy = true;
            }
            else
            {
                APP_PRINT_ERROR2("%s:%d os_mem_alloc return NULL", __FUNCTION__, __LINE__);
            }
        }
    }
}

RAM_TEXT_SECTION
static void app_usb_handle_msg_data_trans_ds(void *buf)
{
//    uint8_t *data = (uint8_t *)buf;
//    bool res = false;
    //static uint16_t enc_frame_len = 0;

    //uint8_t data[192] = {0};

    static uint32_t seq = 0;

    if (created)
    {
        seq++;

        if (usb_audio_buf_ds.buf != NULL)
        {
#ifdef AUDIO_DELAY_TEST_SCO_DS
            if (first_sco_ds_usb_pkt && (app_audio_codec_msbc_ds.handle != NULL))
            {
                Pad_Config(P1_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                Pad_Config(P1_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                first_sco_ds_usb_pkt = false;
            }
#endif

#ifdef AUDIO_DELAY_TEST_MUSIC
            if (first_music_usb_pkt && (app_audio_codec.handle != NULL))
            {
                Pad_Config(P1_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                Pad_Config(P1_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                first_music_usb_pkt = false;
            }
#endif

//            res = ringbuf_write(&usb_audio_buf_ds, data, frame_size_ds);
//            if (!res)
//            {
//                APP_PRINT_ERROR2("%s:%d os_mem_alloc ringbuf_write false", __FUNCTION__, __LINE__);
//            }

            app_fill_data_to_codec();
        }

    }
    else
    {
        if ((audio_stream_mask == USB_AUDIO_DS_MASK) &&
            (app_audio_codec.handle == NULL))
        {
            //notify app_src_policy to do a2dp start
            app_handle_music_data_notify();
            return;
        }
        else if ((audio_stream_mask & USB_AUDIO_US_MASK) &&
                 (app_audio_codec_msbc_ds.handle == NULL))
        {
            app_notify_policy_us_audio_status(true);
        }
    }
}

static void app_usb_handle_msg_uac_inactive(void *buf)
{
    uint32_t param = (uint32_t)buf;
    uint8_t audio_path = (uint8_t)param;

    app_db.usb_audio_path &= ~audio_path;
    APP_PRINT_INFO2("app_usb_handle_msg_uac_inactive: %d, %d",
                    audio_path, app_db.usb_audio_path);

    if (audio_path == DOWN_STREAM)
    {
        audio_stream_mask &= ~USB_AUDIO_DS_MASK;
    }
    else if (audio_path == UP_STREAM)
    {
        audio_stream_mask &= ~USB_AUDIO_US_MASK;
        app_notify_policy_us_audio_status(false);
    }
}

bool app_usb_audio_is_us_streaming(void)
{
    return (audio_stream_mask & USB_AUDIO_US_MASK);
}

bool app_usb_audio_is_ds_streaming(void)
{
    return (audio_stream_mask & USB_AUDIO_DS_MASK);
}

#ifdef AUDIO_DUMMY_DATA_TEST
extern unsigned char const test_msbc_cache[];
extern int test_msbc_cache_len;
extern int test_msbc_cache_index;
#endif

RAM_TEXT_SECTION
static void app_usb_handle_msg_codec(void *buf)
{
    uint16_t event = ((uint32_t)buf) & EVENT_MASK;
    uint16_t param = ((uint32_t)buf & PARAM_MASK) >> PARAM_BITS;

    //static uint32_t offset = 0;

#if defined(APP_USB_AUDIO_DEBUG) && (APP_USB_AUDIO_DEBUG == 1)
    APP_PRINT_INFO2("app_usb_handle_msg_codec, event:0x%x, param:%d", event, param);
#endif

    switch (event)
    {
    case AUDIO_CODEC_PIPE_EVENT_CREATED:
        {
            if (param == 1)
            {
                if (app_audio_codec.handle != NULL)
                {
                    app_audio_codec.is_busy = false;
                    created = true;
                    APP_PRINT_INFO0("app_usb_handle_msg_codec: music codec ready");
                }
                else if (app_audio_codec_msbc_ds.handle != NULL)
                {
                    app_audio_codec_msbc_ds.is_busy = false;
                }
            }
            else if (param == 2)
            {
                app_audio_codec_msbc_us.is_busy = false;
            }

            if ((app_audio_codec_msbc_ds.handle != NULL) &&
                (app_audio_codec_msbc_ds.is_busy == false) &&
                (app_audio_codec_msbc_us.handle != NULL) &&
                (app_audio_codec_msbc_us.is_busy == false))
            {
                created = true;
                APP_PRINT_INFO0("app_usb_handle_msg_codec: voice codec ready");
                //audio_codec_pipe_fill(app_audio_codec_msbc_us.handle, msbc_pkt, 57);
                //app_audio_codec_msbc_us.is_busy = true;
            }
        }
        break;
    case AUDIO_CODEC_PIPE_EVENT_DATA_IND:
        {
            uint16_t pkt_type = ((uint32_t)buf & PARAM_MASK) >> PARAM_BITS;

            uint8_t *buf = NULL;
            uint16_t len = 0;
            uint16_t frame_num = 0;


            if (pkt_type == DATA_IND_TYPE_ENC)
            {
                if (app_audio_codec.handle != NULL)
                {
#ifdef AUDIO_DELAY_TEST_MUSIC
                    if (first_music_enc_pkt)
                    {
                        Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                        Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                        first_music_enc_pkt = false;
                    }
#endif

                    audio_codec_pipe_drain(app_audio_codec.handle, (void **)&buf, &len, &frame_num);
                }
                else if (app_audio_codec_msbc_ds.handle != NULL)
                {
#ifdef AUDIO_DELAY_TEST_SCO_DS
                    if (first_sco_ds_enc_pkt)
                    {
                        Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                        Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                        first_sco_ds_enc_pkt = false;
                    }
#endif
                    audio_codec_pipe_drain(app_audio_codec_msbc_ds.handle, (void **)&buf, &len, &frame_num);
#ifdef AUDIO_DUMMY_DATA_TEST_AFTER_MSBC_ENC
                    memcpy(buf, &test_msbc_cache[test_msbc_cache_index], 57);
                    test_msbc_cache_index += 57;
                    if (test_msbc_cache_index >= test_msbc_cache_len)
                    {
                        test_msbc_cache_index = 0;
                    }
#endif
                }
                usb_audio_data_cb(buf, len, frame_num);

                //APP_PRINT_INFO2("AUDIO_CODEC_PIPE_EVENT_DATA_IND: len %d, p_data %b",
                //            len, TRACE_BINARY(len, buf));

#if defined(APP_USB_AUDIO_DEBUG) && (APP_USB_AUDIO_DEBUG == 1)
                APP_PRINT_INFO2("DATA_IND_TYPE_ENC: len %d, frame_num %d", len, frame_num);
#endif
            }
            else if (pkt_type == DATA_IND_TYPE_DEC)
            {
#ifdef AUDIO_DELAY_TEST_SCO_US
                if (first_sco_us_dec_pkt)
                {
                    Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                    Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                    first_sco_us_dec_pkt = false;
                }
#endif

                audio_codec_pipe_drain(app_audio_codec_msbc_us.handle, (void **)&buf, &len, &frame_num);

#if defined(APP_USB_AUDIO_DEBUG) && (APP_USB_AUDIO_DEBUG == 1)
                APP_PRINT_INFO2("DATA_IND_TYPE_DEC: len %d, frame_num %d", len, frame_num);
#endif
//                uint32_t s;

//                s = os_lock();
                if (usb_audio_buf_us.buf != NULL && buf != NULL)
                {
#if 0
                    memset(buf, 0, len);
#endif
                    ringbuf_write(&usb_audio_buf_us, buf, len);
                    //if (offset + len > pcm_data_len)
                    //{
                    //    offset = 0;
                    //}

                    //ringbuf_write(&usb_audio_buf_us, offset + ac06241816_audacity, len);

                    //offset += len;
                }
//                os_unlock(s);
            }
        }
        break;
    case AUDIO_CODEC_PIPE_EVENT_DATA_FILLED:
        {
            uint16_t pkt_type = ((uint32_t)buf & PARAM_MASK) >> PARAM_BITS;
            if (pkt_type == DATA_ACK_TYPE_ENC)
            {
                if (app_audio_codec.handle != NULL)
                {
                    app_audio_codec.is_busy = false;
                }
                else if (app_audio_codec_msbc_ds.handle != NULL)
                {
                    app_audio_codec_msbc_ds.is_busy = false;
                }

                app_fill_data_to_codec();
            }
            else if (pkt_type == DATA_ACK_TYPE_DEC)
            {
                app_audio_codec_msbc_us.is_busy = false;
                //audio_codec_pipe_fill(app_audio_codec_msbc_us.handle, msbc_pkt, 57);
            }
        }
        break;
    case AUDIO_CODEC_PIPE_EVENT_RELEASED:
        {
        }
        break;
    }

}

static BOOL_USB_FUNC app_usb_cbs[CB_IDX_MAX] =
{
    [CB_IDX_PLUG] = (BOOL_USB_FUNC)usb_audio_plug,
    [CB_IDX_UNPLUG] = (BOOL_USB_FUNC)usb_audio_unplug,
    [CB_IDX_UAC_ACTIVE] = (BOOL_USB_FUNC)usb_audio_active,
    [CB_IDX_UAC_INACTIVE] = (BOOL_USB_FUNC)usb_audio_inactive,
    [CB_IDX_UAC_DATA_TRANS] = (BOOL_USB_FUNC)usb_audio_data_trans,
    [CB_IDX_UAC_SET_VOL] = (BOOL_USB_FUNC)usb_audio_set_vol,
    [CB_IDX_UAC_GET_VOL] = (BOOL_USB_FUNC)usb_audio_get_vol,
};

void app_usb_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    T_IO_MSG *usb_msg = io_driver_msg_recv;

    uint16_t usb_msg_type = usb_msg->subtype;

#if defined(APP_USB_AUDIO_DEBUG) && (APP_USB_AUDIO_DEBUG == 1)
    APP_PRINT_INFO1("app_usb_handle_msg,msg:%x", usb_msg_type);
#endif

    switch (usb_msg_type)
    {
    case MSG_TYPE_PLUG:
        {
            app_usb_handle_msg_plug(usb_msg->u.buf);
            usb_connected = 1;
            //app_mmi_handle_action(0xfc);
        }
        break;
    case MSG_TYPE_UNPLUG:
        {
            app_usb_handle_msg_unplug(usb_msg->u.buf);
            usb_connected = 0;
            //app_mmi_handle_action(0xfd);
        }
        break;
    case MSG_TYPE_UAC_ACTIVE:
        {
            app_usb_handle_msg_uac_active(usb_msg->u.buf);
        }
        break;
    case MSG_TYPE_UAC_INACTIVE:
        {
            app_usb_handle_msg_uac_inactive(usb_msg->u.buf);
        }
        break;
    case MSG_TYPE_UAC_DATA_TRANS_DS:
        {
            app_usb_handle_msg_data_trans_ds(usb_msg->u.buf);
        }
        break;

    case MSG_TYPE_UAC_SET_VOL:
        {
            extern void app_usb_hid_handle_uac_set_volume(uint32_t vol);
            app_usb_hid_handle_uac_set_volume(usb_msg->u.param);

        }
        break;
    case MSG_TYPE_UAC_GET_VOL:
        {
            APP_PRINT_INFO0("MSG_TYPE_UAC_GET_VOL not assigned");
        }
        break;
    case MSG_TYPE_UAC_CODEC:
        {
            app_usb_handle_msg_codec(usb_msg->u.buf);
        }
        break;
    default:
        break;

    }
}

bool app_usb_connected(void)
{
    APP_PRINT_INFO1("app_usb_connected: %d", usb_connected);

    return (usb_connected == 1);
}


void app_usb_audio_init(void)
{
    memset(&app_audio_codec, 0, sizeof(T_APP_UAC_CODEC));

    ringbuf_init(&usb_audio_buf_us, US_RING_BUF_LEN);
    ringbuf_init(&usb_audio_buf_ds, DS_RING_BUF_LEN);

    usb_register_cb(app_usb_cbs);

    T_USB_PARAM para =
    {
        .idVendor = app_cfg_const.app_dongle_usb_vid,
        .idProduct = app_cfg_const.app_dongle_usb_pid,
        .fEnableFunction = USB_FUNCTION_BIT_UAC | USB_FUNCTION_BIT_HID,
        // other null means not changed default;
    };

    usb_init(&para);
}

void app_usb_audio_start(void)
{
    usb_start(NULL);
}

void app_usb_audio_stop(void)
{
    usb_stop(NULL);
}
