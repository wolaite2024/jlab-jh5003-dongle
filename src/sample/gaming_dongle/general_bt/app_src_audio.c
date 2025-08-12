#include "app_src_audio.h"
#include "audio_type.h"
#include "trace.h"
#include "section.h"
#include "app_audio_path.h"
#include "app_general_policy.h"
#include "app_usb_audio_wrapper.h"

#include "app_cfg.h"

#ifdef LEGACY_BT_GENERAL
typedef enum
{
    SBC_DEFAULT
} T_APP_A2DP_ENCODE_FORMAT;

enum src_codec_type
{
    SRC_CODEC_SBCENC,
    SRC_CODEC_SBCDEC,
    SRC_CODEC_MSBCENC,
    SRC_CODEC_MSBCDEC,
    SRC_CODEC_MAX,
};

typedef struct
{
    bool is_voice_start;
    T_AUDIO_TRACK_HANDLE handle;
    T_AUDIO_FORMAT_INFO format_info;
} T_A2DP_RECORDER_INFO;

static T_AUDIO_FORMAT_INFO audio_codec_info[SRC_CODEC_MAX];

static T_A2DP_RECORDER_INFO a2dp_recorder_info = {0};

#define MAX_US_PKT_LOSS_COUNT   0x04


#define USB_DOWN_STREAM_MASK 0x01
#define USB_UP_STREAM_MASK   0x02
uint8_t usb_stream_state;

uint8_t music_sbc_enc_flag = false;
uint8_t hfp_msbc_enc_flag = false;
uint8_t hfp_msbc_dec_flag = false;

bool app_usb_audio_is_us_streaming(void)
{
    return (usb_stream_state & USB_UP_STREAM_MASK);
}

void app_src_uac_us_status(bool active)
{
    if (active)
    {
        usb_stream_state |= USB_UP_STREAM_MASK;
    }
    else
    {
        usb_stream_state &= ~USB_UP_STREAM_MASK;
    }

    app_notify_policy_us_audio_status(active);
}

static uint32_t start_sbc_record(T_A2DP_RECORDER_INFO *info, P_AUDIO_TRACK_ASYNC_IO async_read)
{
    if (info == NULL)
    {
        APP_PRINT_ERROR0("start_sbc_record: Parameter is NULL");
        return 1;
    }

    info->handle = audio_track_create(AUDIO_STREAM_TYPE_RECORD,
                                      AUDIO_STREAM_MODE_NORMAL,
                                      AUDIO_STREAM_USAGE_LOCAL,
                                      info->format_info,
                                      15,
                                      15,
                                      AUDIO_DEVICE_IN_AUX,
                                      NULL,
                                      async_read);

    audio_track_start(info->handle);

    return 0;
}

static uint32_t sbc_dsp_stop_capture(T_A2DP_RECORDER_INFO *info)
{
    if (info->handle == NULL)
    {
        APP_PRINT_ERROR0("Audio trace handle is null");
        return 1;
    }

    audio_track_stop(info->handle);
    audio_track_release(info->handle);

    return 0;
}

void app_src_sbc_voice_start_capture(T_AUDIO_FORMAT_INFO p_format_info,
                                     P_AUDIO_TRACK_ASYNC_IO async_read)
{
    APP_PRINT_INFO7("app_src_sbc_voice_start_capture: %d, %02x, %02x, %02x, %02x, %02x, is_start %d",
                    p_format_info.attr.sbc.sample_rate,
                    p_format_info.attr.sbc.chann_mode,
                    p_format_info.attr.sbc.block_length,
                    p_format_info.attr.sbc.subband_num,
                    p_format_info.attr.sbc.allocation_method,
                    p_format_info.attr.sbc.bitpool,
                    a2dp_recorder_info.is_voice_start);

    if (a2dp_recorder_info.is_voice_start == false)/*g_voice_data.is_voice_start == false8*/
    {
        a2dp_recorder_info.is_voice_start = true;

        memcpy(&a2dp_recorder_info.format_info, &p_format_info, sizeof(T_AUDIO_FORMAT_INFO));

        start_sbc_record(&a2dp_recorder_info, async_read);
    }
}

void app_src_sbc_voice_stop_capture(void)
{
    APP_PRINT_TRACE1("app_src_sbc_voice_stop_capture: is_voice_start %x !",
                     a2dp_recorder_info.is_voice_start);
    /* stop encoder*/
    if (a2dp_recorder_info.is_voice_start == true)//g_voice_data.is_voice_start == true
    {
        a2dp_recorder_info.is_voice_start = false;
        sbc_dsp_stop_capture(&a2dp_recorder_info);//au_dsp_stop_capture(&voice_capture_info);
    }
}


static void src_set_codec_fmt(uint8_t codec_type, T_AUDIO_FORMAT_INFO *info)
{
    if ((!info) || (codec_type >= SRC_CODEC_MAX))
    {
        return;
    }

    T_AUDIO_FORMAT_INFO *p_codec = &audio_codec_info[codec_type];
    memcpy(p_codec, info, sizeof(T_AUDIO_FORMAT_INFO));
}

static T_AUDIO_FORMAT_INFO *src_get_codec_fmt(uint8_t codec_type)
{
    if (codec_type >= SRC_CODEC_MAX)
    {
        return NULL;
    }

    T_AUDIO_FORMAT_INFO *p_codec = &audio_codec_info[codec_type];

    APP_PRINT_INFO7("src_get_codec_fmt codec %d, block %d, subband %d, sr %d, "
                    "bt %d, am %d, cm %d", codec_type,
                    p_codec->attr.sbc.block_length,
                    p_codec->attr.sbc.subband_num,
                    p_codec->attr.sbc.sample_rate,
                    p_codec->attr.sbc.bitpool,
                    p_codec->attr.sbc.allocation_method,
                    p_codec->attr.sbc.chann_mode);
    return p_codec;
}


static bool audio_path_enc_uapi_cback(uint8_t id, uint8_t event, void *buf,
                                      uint16_t len, uint16_t frm_num)
{
    if (event == EVENT_AUDIO_PATH_DATA_IND)
    {
        APP_PRINT_INFO1("audio_path_enc_uapi_cback get data len %d", len);
        usb_audio_data_cb(buf, len, frm_num);
        return true;
    }

    APP_PRINT_INFO2("audio_path_enc_uapi_cback: event %x id %d", event, id);
    switch (event)
    {
    case EVENT_AUDIO_PATH_STREAM_STARTED:
        //app_try_start_stream();
        break;
    case EVENT_AUDIO_PATH_STREAM_STOPPED:
        break;
    case EVENT_AUDIO_PATH_READY:
        break;
    case EVENT_AUDIO_PATH_RELEASED:
        break;
    }

    return true;
}

static bool audio_path_dec_uapi_cback(uint8_t id, uint8_t event, void *buf,
                                      uint16_t len, uint16_t frm_num)
{
    static uint32_t dec_count = 0;

    if (event == EVENT_AUDIO_PATH_DATA_IND)
    {

        APP_PRINT_INFO5("audio_path_dec_uapi_cback: id %u, data recv, len %u, "
                        "frmn %u, dec_count %u, buf %b", id, len, frm_num, dec_count, TRACE_BINARY(10, buf));

        dec_count++;

        if (len)
        {
            bool rc;

            rc = app_usb_audio_us_data_write(buf, len);
            if (!rc)
            {
                uint32_t n;

                n = app_audio_path_watermark(OT_UDEV_OUT1);
                APP_PRINT_ERROR4("audio_path_dec_uapi_cback: Write upstream err"
                                 " len %u, watermark %u/%u, upstream space %u",
                                 len, (n & 0xffff), (n >> 16) & 0xffff,
                                 app_usb_audio_us_get_remaining_pool_size());
            }
        }
        return true;
    }

    APP_PRINT_INFO2("audio_path_dec_uapi_cback: event %x id %d", event, id);
    switch (event)
    {
    case EVENT_AUDIO_PATH_STREAM_STARTED:
        break;
    case EVENT_AUDIO_PATH_STREAM_STOPPED:
        break;
    case EVENT_AUDIO_PATH_READY:
        break;
    case EVENT_AUDIO_PATH_RELEASED:
        break;
    }

    return true;

}

void app_usb_audio_music_create(T_AUDIO_FORMAT_INFO codec)
{
    struct path_iovec iv[2];
    uint8_t ids[2];
    uint8_t ivn;
    T_AUDIO_FORMAT_INFO iofmt;
    APP_PRINT_INFO1("app_usb_audio_music_create: enc_flag %d", music_sbc_enc_flag);

    if (music_sbc_enc_flag)
    {
        return;
    }
    music_sbc_enc_flag = true;
    memset(iv, 0, sizeof(iv));

    src_set_codec_fmt(SRC_CODEC_SBCENC, &codec);
    if (app_cfg_const.dongle_media_device == 1)
    {
        iv[0].it = IT_AUX;
    }
    else if (app_cfg_const.dongle_media_device == 2)
    {
        iv[0].it = IT_MIC;
    }
    else if (!app_cfg_const.dongle_media_device)
    {
        iv[0].it = IT_UDEV_IN1;
    }

    iv[0].ot = OT_SBC;
    iv[0].ident = "it_udev_ot_a2dp";
    iofmt.attr.pcm.sample_rate = 48000;
    iofmt.type = AUDIO_FORMAT_TYPE_PCM;
    iv[0].ifmt = &iofmt;
    iv[0].ofmt = src_get_codec_fmt(SRC_CODEC_SBCENC);
    iv[0].uapi_cback = audio_path_enc_uapi_cback;
    iv[0].priority = 0;
    ivn = 1;
    app_audio_path_createv(iv, ivn, ids);
}

void app_usb_audio_music_destroy(void)
{
    uint8_t  it = IT_UDEV_IN1;
    APP_PRINT_INFO1("app_usb_audio_music_destroy: enc_flag %d", music_sbc_enc_flag);
    if (!music_sbc_enc_flag)
    {
        return;
    }
    music_sbc_enc_flag = false;

    if (app_cfg_const.dongle_media_device == 0)
    {
        it = IT_UDEV_IN1;
    }
    else if (app_cfg_const.dongle_media_device == 1)
    {
        it = IT_AUX;
    }
    else if (app_cfg_const.dongle_media_device == 2)
    {
        it = IT_MIC;
    }

    app_audio_path_release_by_itot(it, OT_SBC);
}


void app_usb_audio_msbc_ds_create(void)
{
    struct path_iovec iv[2];
    uint8_t ids[2];
    uint8_t ivn;
    T_AUDIO_FORMAT_INFO ifmt, ofmt;

    APP_PRINT_INFO1("app_usb_audio_msbc_ds_create: enc_flag %d", hfp_msbc_enc_flag);
    if (hfp_msbc_enc_flag)
    {
        return;
    }
    memset(iv, 0, sizeof(iv));
    hfp_msbc_enc_flag = true;
    ifmt.type = AUDIO_FORMAT_TYPE_PCM;
    ifmt.attr.pcm.sample_rate = 48000;

    ofmt.type = AUDIO_FORMAT_TYPE_MSBC;
    ofmt.attr.msbc.sample_rate = 16000;
    ofmt.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
    ofmt.attr.msbc.block_length = 15;
    ofmt.attr.msbc.subband_num = 8;
    ofmt.attr.msbc.allocation_method = 0;
    ofmt.attr.msbc.bitpool = 26;
    src_set_codec_fmt(SRC_CODEC_MSBCENC, &ofmt);
    app_audio_path_flush(IT_UDEV_IN1);

    iv[0].it = IT_UDEV_IN1;
    iv[0].ot = OT_MSBC;
    iv[0].ident = "it_udev_ot_hfp";
    iv[0].ifmt = &ifmt;

    iv[0].ofmt = src_get_codec_fmt(SRC_CODEC_MSBCENC);
    iv[0].uapi_cback = audio_path_enc_uapi_cback;
    iv[0].priority = 0;
    ivn = 1;
    app_audio_path_createv(iv, ivn, ids);
}

void app_usb_audio_msbc_ds_destroy(void)
{
    APP_PRINT_INFO1("app_usb_audio_msbc_us_create: enc_flag %d", hfp_msbc_enc_flag);
    if (!hfp_msbc_enc_flag)
    {
        return;
    }
    hfp_msbc_enc_flag = false;
    app_audio_path_release_by_itot(IT_UDEV_IN1, OT_MSBC);
}

void app_usb_audio_msbc_us_create(void)
{
    struct path_iovec iv[2];
    uint8_t ids[2];
    uint8_t ivn;
    T_AUDIO_FORMAT_INFO ifmt, ofmt;

    APP_PRINT_INFO1("app_usb_audio_msbc_us_create: dec_flag %d", hfp_msbc_dec_flag);
    if (hfp_msbc_dec_flag)
    {
        return;
    }
    hfp_msbc_dec_flag = true;
    memset(iv, 0, sizeof(iv));

    ifmt.type = AUDIO_FORMAT_TYPE_MSBC;
    ifmt.attr.msbc.sample_rate = 16000;
    ifmt.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
    ifmt.attr.msbc.block_length = 15;
    ifmt.attr.msbc.subband_num = 8;
    ifmt.attr.msbc.allocation_method = 0;
    ifmt.attr.msbc.bitpool = 26;
    src_set_codec_fmt(SRC_CODEC_MSBCDEC, &ifmt);
    app_audio_path_flush(IT_MSBC);

    ofmt.type = AUDIO_FORMAT_TYPE_PCM;
    ofmt.attr.pcm.sample_rate = (USB_AUDIO_US_SAMPLE_RATE == UAC_SAM_RATE_48000) ? 48000 : 16000;

    iv[0].it = IT_MSBC;
    iv[0].ot = OT_UDEV_OUT1;
    iv[0].ident = "it_hfp_ot_udev1";
    iv[0].ifmt = src_get_codec_fmt(SRC_CODEC_MSBCDEC);

    iv[0].ofmt = &ofmt;
    iv[0].uapi_cback = audio_path_dec_uapi_cback;
    iv[0].priority = 0;
    ivn = 1;
    app_audio_path_createv(iv, ivn, ids);
}

void app_usb_audio_msbc_us_destroy(void)
{
    APP_PRINT_INFO1("app_usb_audio_msbc_us_destroy: dec_flag %d", hfp_msbc_dec_flag);
    if (!hfp_msbc_dec_flag)
    {
        return;
    }
    hfp_msbc_dec_flag = false;
    app_audio_path_release_by_itot(IT_MSBC, OT_UDEV_OUT1);
}

RAM_TEXT_SECTION
void app_usb_audio_msbc_fill_us(uint8_t *buf, uint16_t len, uint8_t flag)
{
    app_audio_path_fill(IT_MSBC, buf, len, flag, 0);
}
#endif
