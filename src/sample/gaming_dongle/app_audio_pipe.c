/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "os_msg.h"
#include "trace.h"
#include "audio_pipe.h"
#include "app_audio_pipe.h"
#include "app_cyclic_buffer.h"
#include "ual_types.h"
#include "ual_list.h"
#include "section.h"
#if APP_DEBUG_REPORT
#include "app_status_report.h"
#endif
#include "section.h"
#include "app_cfg.h"

//#define APP_AUDIO_PIPE_DEBUG

/* pipe event inform to path by cback */
#define     AUDIO_PATH_ENABLE_PIPE_CBACK
/* FIXME: What is the biggest length of buf for draining ? */
#define PIPE_DRAIN_BUF_LEN          1024

/* FIXME: For decoding, we should avoid filling packets frequently.
 * Filling packets frequently might make dsp stick to decoding and it has no
 * time for encoding pcm data. This would make pop sound.
 * Application should fill some dummy data before filling the first code pkt
 * (sbcdec pkt, etc)
 * */
#define DECODING_THRESHOLD   (768 * 4)
/* Every X miliseconds, usb isoc interrupt routine would send a message to app
 * to try to resume decoding. */
#define USB_UPSTREAM_PERIOD_SIZE    384

static uint32_t pipe_associated = 0;

static uint8_t rtp_frame_cnt = 0;

enum audio_pipe_state
{
    AUDIO_PIPE_STATE_CREATED = 0x01,
    AUDIO_PIPE_STATE_STARTING,
    AUDIO_PIPE_STATE_STARTED,
    AUDIO_PIPE_STATE_STOPPED,
    AUDIO_PIPE_STATE_RELEASED,
};

typedef struct t_app_audio_pipe
{
    uint8_t              codec_type: 7;
    uint8_t              enc: 1;
    uint8_t              state;
    uint8_t              busy;
    uint8_t              normal_frame_decoding;
    uint8_t              plc_frame_decoding;
    T_AUDIO_PIPE_HANDLE  pipe;
    T_AUDIO_FORMAT_INFO  fmt;
    uint8_t              frame_duration;
    uint16_t             in_frame_len;
    uint8_t              *in_frame_buffer;
    uint16_t             pcm_frame_len;
    uint16_t             expect_drain_len;
    uint16_t             out_frame_len;
    uint16_t             total_plc_pcm;
    uint16_t             total_loss_pcm;
    uint16_t             seq_num;
    /* Upper layer identification */
    uint8_t                   uid;
    /* Save app audio upper layer callback.
     * id: indicates the app audio upper layer id
     * len: for data indication, it consists of buf len and frm num
     *      high 16-bit, frm num
     *      low  16-bit, len
     * */
    void (*mgr_cback)(uint8_t id, uint8_t event, uint8_t *buf, uint32_t len);
} T_APP_AUDIO_PIPE;

/* TODO: The item header in cyclic buf
 * BE CAUTIOUS, THIS STRUCTURE MUST BE SAME AS THE ONE IN app_audio_path.c
 * */
struct cyclic_ihdr
{
    uint16_t len;
    uint8_t  it;
    uint8_t  flag;
    uint32_t timestamp;
};
typedef struct t_usb_downstream_pcm_format
{
    uint32_t    sample_rate;
    uint8_t     chann_num;
    uint8_t     bit_width;
} T_APP_DS_PCM_FORMAT;

typedef struct t_usb_upstream_pcm_format
{
    uint32_t    sample_rate;
    uint8_t     chann_num;
    uint8_t     bit_width;
} T_APP_US_PCM_FORMAT;

static T_APP_AUDIO_PIPE sbcenc_pipe;
static T_APP_AUDIO_PIPE sbcenc2_pipe;
static T_APP_AUDIO_PIPE sbcdec_pipe;
static T_APP_AUDIO_PIPE lc3enc_pipe;
static T_APP_AUDIO_PIPE lc3enc2_pipe;
static T_APP_AUDIO_PIPE lc3dec_pipe;
static T_APP_AUDIO_PIPE msbcenc_pipe;
static T_APP_AUDIO_PIPE msbcdec_pipe;
static uint8_t *pipe_drain_buf;
static T_APP_AUDIO_PIPE *audio_pipes[CODEC_MAX_TYPE];
static uint8_t pipe_io_msg_type = 0xF4;
static void *audio_pipe_evt_q;
static void *audio_pipe_msg_q;

static T_APP_DS_PCM_FORMAT ds_pcm_format;
static T_APP_US_PCM_FORMAT us_pcm_format;

static void app_audio_data_ind_handle(T_APP_AUDIO_PIPE *pipe, uint8_t codec_type);
static inline T_APP_AUDIO_PIPE *app_audio_pipe_instance(uint8_t codec_type);

static uint32_t dec_watermark;

#if APP_DEBUG_REPORT
extern uint16_t usb_uac_get_up_stream_buff_data_space(void);
#endif

/* TODO: Might the func be called in other task context or interrupt context,*/
void app_audio_pipe_clear_dec_watermark(void)
{
    dec_watermark = 0;
}

RAM_TEXT_SECTION
void app_audio_pipe_clear_fake_dec_watermark(void)
{
    dec_watermark = 0;
}

void app_set_downstream_pcm_format(uint32_t sample_rate, uint8_t chan_num, uint8_t bit_width)
{
    ds_pcm_format.sample_rate = sample_rate;
    ds_pcm_format.chann_num = chan_num;
    ds_pcm_format.bit_width = bit_width;
}

void app_set_upstream_pcm_format(uint32_t sample_rate, uint8_t chan_num, uint8_t bit_width)
{
    us_pcm_format.sample_rate = sample_rate;
    us_pcm_format.chann_num = chan_num;
    us_pcm_format.bit_width = bit_width;
}

void app_usb_downstream_pcm_format_init(T_AUDIO_FORMAT_INFO *info)
{
    if (!info)
    {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->type = AUDIO_FORMAT_TYPE_PCM;
    info->frame_num = 1;
    info->attr.pcm.sample_rate = ds_pcm_format.sample_rate;
    info->attr.pcm.chann_num = ds_pcm_format.chann_num;
    info->attr.pcm.bit_width = ds_pcm_format.bit_width;
}

void app_usb_upstream_pcm_format_init(T_AUDIO_FORMAT_INFO *info)
{
    if (!info)
    {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->type = AUDIO_FORMAT_TYPE_PCM;
    info->frame_num = 1;
    info->attr.pcm.sample_rate = us_pcm_format.sample_rate;
    info->attr.pcm.chann_num = us_pcm_format.chann_num;
    info->attr.pcm.bit_width = us_pcm_format.bit_width;
}

static bool app_audio_pipe_signal(uint8_t codec_type,
                                  T_AUDIO_PIPE_HANDLE handle,
                                  T_AUDIO_PIPE_EVENT  event,
                                  uint32_t            param)
{
    uint8_t  evt;
    T_IO_MSG msg;
    T_APP_AUDIO_PIPE *pipe = NULL;

    if (!audio_pipe_evt_q || !audio_pipe_msg_q)
    {
        APP_PRINT_ERROR0("app_audio_pipe_signal: evt/msg queue is null");
        return false;
    }

    pipe = app_audio_pipe_instance(codec_type);
    if (!pipe)
    {
        APP_PRINT_ERROR1("app_audio_pipe_signal: Unsupported codec_type %u",
                         codec_type);
        return false;
    }

    evt = EVENT_IO_TO_APP;
    msg.type    = pipe_io_msg_type;
    msg.subtype = MSG_TYPE_AUDIO_PIPE_CODEC;
    msg.u.param = (event & 0xff) | (codec_type << 8) | ((param & 0xffff) << 16);

    /* In order to make sbcdec decoding consistently, we call drain directly.
     * */
    if ((pipe->enc || codec_type == CODEC_SBCDEC_TYPE) &&
        AUDIO_PIPE_EVENT_DATA_IND == event)
    {
        /* Don't clear pipe->busy. */

        /* Drain data */
        app_audio_data_ind_handle(pipe, codec_type);
        return true;
    }
    if (os_msg_send(audio_pipe_evt_q, &evt, 0))
    {
        if (os_msg_send(audio_pipe_msg_q, &msg, 0))
        {
            return true;
        }
        else
        {
            APP_PRINT_ERROR2("app_audio_pipe_signal: codec %u send msg %08x fail",
                             codec_type, msg.u.param);
        }
    }
    else
    {
        APP_PRINT_ERROR2("app_audio_pipe_signal: codec %u send evt fail, parm %08x",
                         codec_type, msg.u.param);
    }

    return false;
}

static bool app_audio_pipe_lc3enc_cback(T_AUDIO_PIPE_HANDLE handle,
                                        T_AUDIO_PIPE_EVENT  event,
                                        uint32_t            param)
{
    return app_audio_pipe_signal(CODEC_LC3ENC_TYPE, handle, event, param);
}

static bool app_audio_pipe_lc3enc2_cback(T_AUDIO_PIPE_HANDLE handle,
                                         T_AUDIO_PIPE_EVENT  event,
                                         uint32_t            param)
{
    return app_audio_pipe_signal(CODEC_LC3ENC2_TYPE, handle, event, param);
}

static bool app_audio_pipe_lc3dec_cback(T_AUDIO_PIPE_HANDLE handle,
                                        T_AUDIO_PIPE_EVENT  event,
                                        uint32_t            param)
{
    return app_audio_pipe_signal(CODEC_LC3DEC_TYPE, handle, event, param);
}

static bool app_audio_pipe_sbcenc_cback(T_AUDIO_PIPE_HANDLE handle,
                                        T_AUDIO_PIPE_EVENT  event,
                                        uint32_t            param)
{
    return app_audio_pipe_signal(CODEC_SBCENC_TYPE, handle, event, param);
}

static bool app_audio_pipe_sbcenc2_cback(T_AUDIO_PIPE_HANDLE handle,
                                         T_AUDIO_PIPE_EVENT  event,
                                         uint32_t            param)
{
    return app_audio_pipe_signal(CODEC_SBCENC2_TYPE, handle, event, param);
}

static bool app_audio_pipe_sbcdec_cback(T_AUDIO_PIPE_HANDLE handle,
                                        T_AUDIO_PIPE_EVENT  event,
                                        uint32_t            param)
{
    return app_audio_pipe_signal(CODEC_SBCDEC_TYPE, handle, event, param);
}

static bool app_audio_pipe_msbcenc_cback(T_AUDIO_PIPE_HANDLE handle,
                                         T_AUDIO_PIPE_EVENT  event,
                                         uint32_t            param)
{
    return app_audio_pipe_signal(CODEC_MSBCENC_TYPE, handle, event, param);
}

static bool app_audio_pipe_msbcdec_cback(T_AUDIO_PIPE_HANDLE handle,
                                         T_AUDIO_PIPE_EVENT  event,
                                         uint32_t            param)
{
    return app_audio_pipe_signal(CODEC_MSBCDEC_TYPE, handle, event, param);
}

static inline T_APP_AUDIO_PIPE *app_audio_pipe_instance(uint8_t codec_type)
{
    if (codec_type >= CODEC_MAX_TYPE)
    {
        return NULL;
    }

    return audio_pipes[codec_type];
}

static bool app_audio_fill_lc3_pipe_decode_data(T_APP_AUDIO_PIPE *pipe, T_CYCLIC_BUF *cyclic_buf)
{
    bool rc = false;
    uint16_t frame_len;
    uint8_t *buf;
    struct cyclic_ihdr *hdr;

    if (!cyclic_buf)
    {
        return false;
    }

    /* Don't worry. usb will try to resume decoding. */
    if (dec_watermark >= DECODING_THRESHOLD)
    {
        APP_PRINT_WARN2("app_audio_fill_lc3_pipe_data: dec_watermark %u : %d", dec_watermark,
                        DECODING_THRESHOLD);
        return false;
    }

    APP_PRINT_INFO1("app_audio_fill_lc3_pipe_data, len %x", pipe->in_frame_len);
    frame_len = pipe->in_frame_len + sizeof(*hdr);

    if (pipe->in_frame_buffer == NULL)
    {
        pipe->in_frame_buffer = calloc(1, frame_len);
    }

    if (!pipe->in_frame_buffer)
    {
        APP_PRINT_ERROR0("app_audio_fill_lc3_pipe_data: Cannot alloc frame mem for filling");
        return false;
    }

    rc = cyclic_buf_read(cyclic_buf, pipe->in_frame_buffer, frame_len);
    if (!rc)
    {
        APP_PRINT_ERROR0("app_audio_fill_lc3_pipe_data: Read buf error");
        goto done;
    }

    hdr = (struct cyclic_ihdr *)pipe->in_frame_buffer;
    buf = pipe->in_frame_buffer + sizeof(*hdr);

#if APP_DEBUG_REPORT
    app_status_report.codec_up_ringbuffer_bytes -= app_status_report.codec_up_frame_len;
#endif

    //FIX TODO
    if (hdr->flag == 0)
    {
        APP_PRINT_INFO1("app_audio_fill_lc3_pipe_data: fill %x", pipe->in_frame_len);
        rc = audio_pipe_fill(pipe->pipe, 0, pipe->seq_num, AUDIO_STREAM_STATUS_CORRECT, rtp_frame_cnt,
                             buf, pipe->in_frame_len);
    }
    else
    {
        APP_PRINT_ERROR0("app_audio_fill_lc3_pipe_data: lost data need plc");
        rc = audio_pipe_fill(pipe->pipe, 0, pipe->seq_num, AUDIO_STREAM_STATUS_DUMMY, rtp_frame_cnt, NULL,
                             0);
    }
    if (rc)
    {
        pipe->seq_num++;
        pipe->busy = 1;
        dec_watermark += pipe->pcm_frame_len;
#if APP_DEBUG_REPORT
        app_status_report.codec_flight_in_dsp_bytes += app_status_report.codec_up_frame_len;
        app_status_report.decode_fill_num++;
#endif
    }
    else
    {
        APP_PRINT_ERROR0("app_audio_fill_lc3_pipe_data: Pipe fill error");
#if APP_DEBUG_REPORT
        app_status_report.codec_up_drop_bytes += frame_len;
#endif
    }

done:
    return rc;
}

//static uint32_t lc3_pcm_idx = 0;
//static uint32_t lc3_encode_idx = 0;
/* Encoding */
static bool app_audio_pipe_fill_pcm(T_APP_AUDIO_PIPE *pipe, T_CYCLIC_BUF *cyclic)
{
    uint16_t frame_len;
    bool rc = false;

    frame_len = pipe->in_frame_len;

    if (cyclic_buf_count(cyclic) < frame_len)
    {
        return false;
    }

    if (pipe->in_frame_buffer == NULL)
    {
        pipe->in_frame_buffer = calloc(1, frame_len);
    }

    if (!pipe->in_frame_buffer)
    {
        APP_PRINT_ERROR0("pipe_fill: Cannot alloc frame mem for filling");
        return false;
    }

    rc = cyclic_buf_read(cyclic, pipe->in_frame_buffer, frame_len);
    if (!rc)
    {
        APP_PRINT_ERROR0("pipe_fill: Read buf error");
        goto done;
    }
#if APP_DEBUG_REPORT
    if (pipe->codec_type == CODEC_LC3ENC_TYPE)
    {
        app_status_report.pcm_down_ringbuffer_bytes -= app_status_report.pcm_down_frame_len;
    }
#endif

#ifdef APP_AUDIO_PIPE_DEBUG
    APP_PRINT_INFO2("pipe_fill: Fill %u codec_type %d", frame_len, pipe->codec_type);
#endif
    rc = audio_pipe_fill(pipe->pipe, 0, pipe->seq_num, AUDIO_STREAM_STATUS_CORRECT, 1,
                         pipe->in_frame_buffer, frame_len);
    if (rc)
    {
        pipe->seq_num++;
        pipe->busy = 1;
#if APP_DEBUG_REPORT
        if (pipe->codec_type == CODEC_LC3ENC_TYPE)
        {
            app_status_report.pcm_flight_in_dsp_bytes += app_status_report.pcm_down_frame_len;
            //lc3_pcm_idx++;
            //APP_PRINT_INFO1("lc3 endcode lc3_pcm_idx %d", lc3_pcm_idx);
        }
#endif
    }
    else
    {
        APP_PRINT_ERROR0("pipe_fill: Pipe fill error");
    }

done:
    return rc;
}

/* Decoding */
static bool app_audio_pipe_fill_code(T_APP_AUDIO_PIPE *pipe, T_CYCLIC_BUF *cyclic)
{
    uint16_t frame_len;
    bool rc = false;
    struct cyclic_ihdr hdr;

    if (cyclic_buf_count(cyclic) < sizeof(hdr))
    {
        return false;
    }

    frame_len = pipe->in_frame_len + sizeof(hdr);

    if (!cyclic_buf_peek(cyclic, (void *)&hdr, sizeof(hdr)))
    {
        return false;
    }

    /* This frame is not for this codec, remove it and process next frame.
     * Be careful, there is recursion.
     * */
    if (hdr.len != frame_len)
    {
        APP_PRINT_WARN3("app_audio_pipe_fill_code: frame len (%u!=%u) mismatch,"
                        " drop the frame, pipe codec_type %u", hdr.len, frame_len,
                        pipe->codec_type);
        if (cyclic_buf_drop(cyclic, hdr.len))
        {
            app_audio_pipe_fill_code(pipe, cyclic);
        }
        else
        {
            APP_PRINT_ERROR0("app_audio_pipe_fill_code: drop data err");
            return false;
        }
    }

    if (cyclic_buf_count(cyclic) < frame_len)
    {
        return false;
    }

    /* TODO: Why is lc3dec special? */
    if (pipe->codec_type == CODEC_LC3DEC_TYPE)
    {
        return app_audio_fill_lc3_pipe_decode_data(pipe, cyclic);
    }

    if (dec_watermark >= DECODING_THRESHOLD)
    {
        APP_PRINT_ERROR2("app_audio_pipe_fill_code: Watermark too high, %u/%u",
                         dec_watermark, DECODING_THRESHOLD);
        return false;
    }

    if (pipe->in_frame_buffer == NULL)
    {
        pipe->in_frame_buffer = calloc(1, frame_len);
    }

    if (!pipe->in_frame_buffer)
    {
        APP_PRINT_ERROR0("app_audio_pipe_fill_code: Cannot alloc frame mem for filling");
        return false;
    }

    rc = cyclic_buf_read(cyclic, pipe->in_frame_buffer, frame_len);
    if (!rc)
    {
        APP_PRINT_ERROR0("app_audio_pipe_fill_code: Read buf error");
        goto done;
    }

    frame_len -= sizeof(hdr);

#ifdef APP_AUDIO_PIPE_DEBUG
    APP_PRINT_INFO2("app_audio_pipe_fill_code: Fill %u codec_type %d", frame_len, pipe->codec_type);
#endif
    if (hdr.flag)
    {
        memset(pipe->in_frame_buffer + sizeof(hdr), 0, frame_len);
        rc = audio_pipe_fill(pipe->pipe, 0, pipe->seq_num, AUDIO_STREAM_STATUS_CORRECT, rtp_frame_cnt,
                             pipe->in_frame_buffer + sizeof(hdr), frame_len);
    }
    else
    {
        rc = audio_pipe_fill(pipe->pipe, 0, pipe->seq_num, AUDIO_STREAM_STATUS_CORRECT, rtp_frame_cnt,
                             pipe->in_frame_buffer + sizeof(hdr), frame_len);
    }

    if (rc)
    {
        if (pipe->codec_type == CODEC_SBCDEC_TYPE || pipe->codec_type == CODEC_LC3DEC_TYPE)
        {
            pipe->normal_frame_decoding = true;
        }

        pipe->seq_num++;
        pipe->busy = 1;
        /* TODO: If we come here, we are definitely in decoding. */
        dec_watermark += pipe->pcm_frame_len;
        /* APP_PRINT_INFO2("app_audio_pipe_fill_code: increase watermark, %u, %u",
         *                 dec_watermark, pipe->pcm_frame_len);
         */
    }
    else
    {
        APP_PRINT_ERROR0("app_audio_pipe_fill_code: Pipe fill error");
    }

done:
    return rc;
}

bool app_audio_pipe_fill_pkt_loss(uint8_t codec_type)
{
    T_APP_AUDIO_PIPE *pipe = NULL;
    uint8_t cause = 0;
    bool ret = true;
    uint8_t buf[52] = {0};

    uint8_t plc_frame_num = 0;
    uint16_t us_pcm_size_per_frame = 0;

    pipe = app_audio_pipe_instance(codec_type);
    if (!pipe)
    {
        cause = 1;
        goto exit;
    }

    if (pipe->normal_frame_decoding)
    {
        cause = 2;
        goto exit;
    }

    us_pcm_size_per_frame = (us_pcm_format.sample_rate / 1000) * pipe->frame_duration *
                            (us_pcm_format.chann_num) * (us_pcm_format.bit_width / 8);

    plc_frame_num = pipe->pcm_frame_len / us_pcm_size_per_frame;

    if (audio_pipe_fill(pipe->pipe, 0, 0, AUDIO_STREAM_STATUS_LOST, plc_frame_num,
                        buf, sizeof(buf)) == false)
    {
        cause = 3;
        goto exit;
    }

    pipe->plc_frame_decoding = true;

exit:
    if (cause)
    {
        ret = false;
    }

    APP_PRINT_TRACE3("app_audio_pipe_fill_pkt_loss: frame_num %d ret %d cause %d",
                     plc_frame_num, ret, cause);

    return ret;
}

bool app_audio_pipe_fill(uint8_t codec_type, T_CYCLIC_BUF *cyclic)
{
    T_APP_AUDIO_PIPE *pipe = NULL;

    if (!cyclic)
    {
        return false;
    }

    pipe = app_audio_pipe_instance(codec_type);
    if (!pipe)
    {
        APP_PRINT_ERROR1("pipe_fill: Unsupported codec_type %u", codec_type);
        return false;
    }

    if (!pipe->pipe || pipe->busy)
    {
        return false;
    }

    if (pipe->state != AUDIO_PIPE_STATE_STARTED)
    {
        return false;
    }

    if (!pipe->enc)
    {
        /* Don't worry. usb will try to resume decoding. */
        if (dec_watermark >= DECODING_THRESHOLD)
        {
            return false;
        }

        return app_audio_pipe_fill_code(pipe, cyclic);
    }
    else
    {
        return app_audio_pipe_fill_pcm(pipe, cyclic);
    }

}

// static bool app_audio_pipe_fill_retry(uint8_t codec_type)
// {
//     bool rc = false;
//     uint16_t frame_len;
//     T_APP_AUDIO_PIPE *pipe = NULL;

//     pipe = app_audio_pipe_instance(codec_type);
//     if (!pipe)
//     {
//         APP_PRINT_ERROR1("pipe_fill_retry: Unsupported codec %u", codec_type);
//         return false;
//     }

//     if (!pipe->pipe || pipe->busy)
//     {
//         return false;
//     }

//     if (pipe->state != AUDIO_PIPE_STATE_STARTED)
//     {
//         return false;
//     }

//     if (!pipe->in_frame_buffer)
//     {
//         APP_PRINT_ERROR0("app_audio_pipe_fill_retry: Cannot alloc frame mem is NULL");
//         return false;
//     }

//     frame_len = pipe->in_frame_len;
//     if (codec_type == CODEC_LC3DEC_TYPE)
//     {
//         if (pipe->in_frame_buffer[0] == 0)
//         {
//             rc = audio_pipe_fill(pipe->pipe, 0, pipe->seq_num, AUDIO_STREAM_STATUS_CORRECT, rtp_frame_cnt,
//                                  pipe->in_frame_buffer + 1, frame_len);
//         }
//         else
//         {
//             APP_PRINT_ERROR0("app_audio_pipe_fill_retry: lost data need plc");
//             rc = audio_pipe_fill(pipe->pipe, 0, pipe->seq_num, AUDIO_STREAM_STATUS_DUMMY, rtp_frame_cnt, NULL, 0);
//         }
//     }
//     else
//     {
//         rc = audio_pipe_fill(pipe->pipe, 0, pipe->seq_num, AUDIO_STREAM_STATUS_CORRECT, rtp_frame_cnt,
//                              pipe->in_frame_buffer, frame_len);
//     }
//     if (rc)
//     {
//         pipe->seq_num++;
//         pipe->busy = 1;
//     }
//     else
//     {
//         APP_PRINT_ERROR0("app_audio_pipe_fill_retry: Pipe fill error");
//     }

//     return rc;
// }

bool app_audio_calculate_loss_frame(uint8_t codec_type, uint8_t loss_frame_cnt)
{
    T_APP_AUDIO_PIPE *pipe = app_audio_pipe_instance(codec_type);;
    bool ret = false;
    uint8_t cause = 0;
    uint16_t us_pcm_size = 0;

    if (pipe == NULL)
    {
        cause = 1;
        goto exit;
    }

    if (pipe->total_plc_pcm)
    {
        us_pcm_size = pipe->frame_duration * loss_frame_cnt *
                      (us_pcm_format.sample_rate / 1000) * (us_pcm_format.bit_width / 8);

        pipe->total_loss_pcm += us_pcm_size;

        if (pipe->total_plc_pcm > pipe->total_loss_pcm)
        {
            pipe->total_plc_pcm -= pipe->total_loss_pcm;
            pipe->total_loss_pcm = 0;
        }
        else
        {
            pipe->total_loss_pcm -= pipe->total_plc_pcm;
            pipe->total_plc_pcm = 0;
        }
    }

exit:
    APP_PRINT_TRACE3("app_audio_calculate_loss_frame: loss %d frame, cause -%d us_pcm_size %d",
                     loss_frame_cnt, cause, us_pcm_size);

    return ret;
}

static void app_audio_data_ind_handle(T_APP_AUDIO_PIPE *pipe, uint8_t codec_type)
{
    uint16_t len = 0;
    bool rc;
    uint8_t  frame_num;
    uint32_t timestamp;
    uint16_t seq;
    T_AUDIO_STREAM_STATUS status;

    if (!pipe)
    {
        APP_PRINT_ERROR1("data_ind_handle: Unsupported codec_type %u", codec_type);
        return;
    }

    if (!pipe->pipe)
    {
        APP_PRINT_ERROR0("data_ind_handle: Data ind but pipe is null");
        return;
    }

    if (!pipe_drain_buf)
    {
        APP_PRINT_ERROR0("data_ind_handle: Drain buf is null");
        return;
    }

    uint8_t *drain_buf = pipe_drain_buf;

    rc = audio_pipe_drain(pipe->pipe,
                          &timestamp,
                          &seq,
                          &status,
                          &frame_num,
                          drain_buf,
                          &len);

    if (!rc)
    {
        APP_PRINT_ERROR1("data_ind_handle: Pipe drain error, len %u", len);
        /* FIXME: Is the len reliable when draining failed ? */
        if (!pipe->enc)
        {
            dec_watermark -= len;
        }
        return;
    }

    if (pipe->codec_type == CODEC_LC3DEC_TYPE || pipe->codec_type == CODEC_SBCDEC_TYPE)
    {
        if (pipe->plc_frame_decoding == false)
        {
            uint16_t total_pcm_len = pipe->frame_duration * rtp_frame_cnt * (us_pcm_format.sample_rate / 1000)
                                     * (us_pcm_format.bit_width / 8);

            pipe->out_frame_len += len;

            if (pipe->out_frame_len == total_pcm_len)
            {
                pipe->out_frame_len = 0;
                pipe->normal_frame_decoding = false;
            }

            /* drop incoming normal pcm data*/
            if (pipe->total_plc_pcm != 0)
            {
                uint16_t drop_len = 0;

                /* consider pkt loss */
                if (pipe->total_plc_pcm > pipe->total_loss_pcm)
                {
                    pipe->total_plc_pcm -= pipe->total_loss_pcm;
                    pipe->total_loss_pcm = 0;
                }
                else
                {
                    pipe->total_loss_pcm -= pipe->total_plc_pcm;
                    pipe->total_plc_pcm = 0;
                }

                /* decide the final drop len */
                if (pipe->total_plc_pcm < len)
                {
                    drop_len = pipe->total_plc_pcm;
                    pipe->total_plc_pcm = 0;
                }
                else
                {
                    drop_len = len;
                    pipe->total_plc_pcm -= drop_len;
                }

                if (drop_len == len)
                {
                    APP_PRINT_TRACE1("drop all %d normal pcm data", drop_len);

                    return;
                }
                else
                {
                    drain_buf += drop_len;
                    len -= drop_len;

                    APP_PRINT_TRACE1("drop %d normal pcm data", drop_len);
                }
            }
        }
        else
        {
            pipe->plc_frame_decoding = false;
            pipe->total_plc_pcm += len;
        }
    }

    /* APP_PRINT_INFO2("app_audio_data_ind_handle: codec_type %u, len %u",
     *                 codec_type, len);
     */

    if ((len % pipe->expect_drain_len) != 0)
    {
#if 0
        APP_PRINT_ERROR3("data_ind_handle: Pipe drain length error, frame %d len %u : %u", frame_num, len,
                         pipe->expect_drain_len);
#endif
#if APP_DEBUG_REPORT
        if (codec_type == CODEC_LC3DEC_TYPE)
        {
            app_status_report.decode_drain_lost += (pipe->expect_drain_len - (len % pipe->expect_drain_len));
        }
#endif
    }

    if (pipe->mgr_cback)
    {
        uint32_t rlen;

        /* frame num and len composes the return len. */
        rlen = (frame_num << 16) | len;
        pipe->mgr_cback(pipe->uid, APP_AUDIO_PIPE_EVENT_DATA_IND,
                        drain_buf, rlen);
    }

#if APP_DEBUG_REPORT
    if (codec_type == CODEC_LC3DEC_TYPE)
    {
        app_status_report.codec_flight_in_dsp_bytes -= app_status_report.codec_up_frame_len;
        app_status_report.decode_drain_num++;
    }
    else if (codec_type == CODEC_LC3ENC_TYPE)
    {
        app_status_report.pcm_flight_in_dsp_bytes -= app_status_report.pcm_down_frame_len * 2;    //FIX TODO
        //lc3_encode_idx += 2;
        //APP_PRINT_INFO1("lc3 endcode lc3_encode_idx %d", lc3_encode_idx);
    }
#endif

}

static bool pipe_starting(void)
{
    uint8_t i;

    for (i = 0; i < CODEC_MAX_TYPE; i++)
    {
        if (audio_pipes[i] && audio_pipes[i]->pipe &&
            audio_pipes[i]->state == AUDIO_PIPE_STATE_STARTING)
        {
            return true;
        }
    }

    return false;
}

static void app_start_pipes(void)
{
    uint8_t i;

    /*
     * We only start one pipe when receiving another pipe started msg.
     * The subsequent pipe starting is driven by the pipe started msg.
     */
    for (i = 0; i < CODEC_MAX_TYPE; i++)
    {
        if (audio_pipes[i] && audio_pipes[i]->pipe &&
            audio_pipes[i]->state == AUDIO_PIPE_STATE_CREATED)
        {
            APP_PRINT_INFO1("app_start_pipes: Start pipe codec_type (%u)", i);
            if (audio_pipe_start(audio_pipes[i]->pipe))
            {
                audio_pipes[i]->state = AUDIO_PIPE_STATE_STARTING;
            }
            else
            {
                APP_PRINT_ERROR1("app_start_pipes: Pipe codec_type %u start err", i);
            }
            return;
        }
    }
}

static void app_audio_pipe_handle(uint8_t codec_type, uint16_t event,
                                  uint32_t param)
{
    T_APP_AUDIO_PIPE *pipe = NULL;
    bool rc;
    static uint32_t audio_associated = 0;

    pipe = app_audio_pipe_instance(codec_type);
    if (!pipe)
    {
        APP_PRINT_ERROR1("pipe_handle: Unsupported codec_type %u", codec_type);
        return;
    }

    if ((event != AUDIO_PIPE_EVENT_DATA_IND) && (event != AUDIO_PIPE_EVENT_DATA_FILLED))
    {
        APP_PRINT_INFO2("pipe_handle: codec_type %x event %x", codec_type, event);
    }

    switch (event)
    {
    case AUDIO_PIPE_EVENT_CREATED:
        APP_PRINT_INFO1("pipe_handle: codec_type %u CREATED", codec_type);
        pipe->state = AUDIO_PIPE_STATE_CREATED;
        pipe->busy = 0;
        if (pipe_associated && (codec_type == CODEC_SBCENC_TYPE ||
                                codec_type == CODEC_SBCENC2_TYPE))
        {
            T_APP_AUDIO_PIPE *pipe_prime = NULL;
            T_APP_AUDIO_PIPE *pipe_aux = NULL;

            audio_associated |= (1 << codec_type);
            if (audio_associated != pipe_associated)
            {
                break;
            }

            if (codec_type == CODEC_SBCENC_TYPE)
            {
                pipe_prime = pipe;
                pipe_aux = app_audio_pipe_instance(CODEC_SBCENC2_TYPE);
                if (!pipe_aux)
                {
                    APP_PRINT_ERROR0("pipe_handle: No aux pipe");
                    break;
                }
            }
            else
            {
                pipe_aux = pipe;
                pipe_prime = app_audio_pipe_instance(CODEC_SBCENC_TYPE);
                if (!pipe_prime)
                {
                    APP_PRINT_ERROR0("pipe_handle: No prime pipe");
                    break;
                }
            }

            APP_PRINT_INFO0("pipe_handle: Set pre mix");
            if (!audio_pipe_pre_mix(pipe_prime->pipe, pipe_aux->pipe))
            {
                APP_PRINT_ERROR0("pipe_handle: set pre mix err");
                break;
            }

            if (pipe_prime->pipe && !pipe_starting())
            {
                rc = audio_pipe_start(pipe_prime->pipe);
                if (rc)
                {
                    pipe_prime->state = AUDIO_PIPE_STATE_STARTING;
                }
                else
                {
                    APP_PRINT_ERROR0("pipe_handle: Start pipe prime error");
                }
            }

            if (pipe_prime->mgr_cback)
            {
                pipe_prime->mgr_cback(pipe_prime->uid, APP_AUDIO_PIPE_EVENT_CREATED, NULL, 0);
            }
            if (pipe_aux->mgr_cback)
            {
                pipe_aux->mgr_cback(pipe_aux->uid, APP_AUDIO_PIPE_EVENT_CREATED, NULL, 0);
            }
            break;
        }

        if (pipe_associated && (codec_type == CODEC_LC3ENC_TYPE ||
                                codec_type == CODEC_LC3ENC2_TYPE))
        {
            T_APP_AUDIO_PIPE *pipe_prime = NULL;
            T_APP_AUDIO_PIPE *pipe_aux = NULL;

            audio_associated |= (1 << codec_type);
            if (audio_associated != pipe_associated)
            {
                break;
            }

            if (codec_type == CODEC_LC3ENC_TYPE)
            {
                pipe_prime = pipe;
                pipe_aux = app_audio_pipe_instance(CODEC_LC3ENC2_TYPE);
                if (!pipe_aux)
                {
                    APP_PRINT_ERROR0("pipe_handle: No aux pipe");
                    break;
                }
            }
            else
            {
                pipe_aux = pipe;
                pipe_prime = app_audio_pipe_instance(CODEC_LC3ENC_TYPE);
                if (!pipe_prime)
                {
                    APP_PRINT_ERROR0("pipe_handle: No prime pipe");
                    break;
                }
            }

            APP_PRINT_INFO0("pipe_handle: Set pre mix");
            if (!audio_pipe_pre_mix(pipe_prime->pipe, pipe_aux->pipe))
            {
                APP_PRINT_ERROR0("pipe_handle: set pre mix err");
                break;
            }

            if (pipe_prime->pipe && !pipe_starting())
            {
                rc = audio_pipe_start(pipe_prime->pipe);
                if (rc)
                {
                    pipe_prime->state = AUDIO_PIPE_STATE_STARTING;
                }
                else
                {
                    APP_PRINT_ERROR0("pipe_handle: Start pipe prime error");
                }
            }

            if (pipe_prime->mgr_cback)
            {
                pipe_prime->mgr_cback(pipe_prime->uid, APP_AUDIO_PIPE_EVENT_CREATED, NULL, 0);
            }
            if (pipe_aux->mgr_cback)
            {
                pipe_aux->mgr_cback(pipe_aux->uid, APP_AUDIO_PIPE_EVENT_CREATED, NULL, 0);
            }
            break;
        }

        if (pipe->pipe && !pipe_starting())
        {
            rc = audio_pipe_start(pipe->pipe);
            if (rc)
            {
                pipe->state = AUDIO_PIPE_STATE_STARTING;
            }
            else
            {
                APP_PRINT_ERROR0("pipe_handle: Start pipe error");
            }
        }
        if (pipe->mgr_cback)
        {
            pipe->mgr_cback(pipe->uid, APP_AUDIO_PIPE_EVENT_CREATED, NULL, 0);
        }
        break;
    case AUDIO_PIPE_EVENT_STARTED:
        APP_PRINT_INFO1("pipe_handle: codec_type %u STARTED", codec_type);
        pipe->busy = 0;
        pipe->state = AUDIO_PIPE_STATE_STARTED;
        if (codec_type == CODEC_SBCENC_TYPE || codec_type == CODEC_SBCENC2_TYPE)
        {
            audio_associated &= ~(1 << codec_type);
        }
        if (codec_type == CODEC_LC3ENC_TYPE || codec_type == CODEC_LC3ENC2_TYPE)
        {
            audio_associated &= ~(1 << codec_type);
        }
        app_start_pipes();
        if (pipe->mgr_cback)
        {
            pipe->mgr_cback(pipe->uid, APP_AUDIO_PIPE_EVENT_STARTED, NULL, 0);
        }
        break;
    case AUDIO_PIPE_EVENT_STOPPED:
        pipe->busy = 0;
        pipe->state = AUDIO_PIPE_STATE_STOPPED;
        if (pipe->mgr_cback)
        {
            pipe->mgr_cback(pipe->uid, APP_AUDIO_PIPE_EVENT_STOPPED, NULL, 0);
        }
        break;
    case AUDIO_PIPE_EVENT_DATA_IND:
#ifdef APP_AUDIO_PIPE_DEBUG
        APP_PRINT_INFO1("pipe_handle: codec_type %u DATA IND", codec_type);
#endif
        /* TODO: check param ?*/
        app_audio_data_ind_handle(pipe, codec_type);
        break;
//     case AUDIO_PIPE_EVENT_DATA_FILL_FAIL:
//         APP_PRINT_ERROR1("pipe_handle: codec_type %u DATA FILLED FAILED", codec_type);
//         pipe->busy = 0;
//         if (!app_audio_pipe_fill_retry(codec_type))
//         {
//             if (!pipe->enc)
//             {
//                 dec_watermark -= pipe->pcm_frame_len;
//             }
// #if APP_DEBUG_REPORT
//             if (codec_type == CODEC_LC3DEC_TYPE)
//             {
//                 app_status_report.codec_flight_in_dsp_bytes -= app_status_report.codec_up_frame_len;
//             }
//             else if (codec_type == CODEC_LC3ENC_TYPE)
//             {
//                 app_status_report.pcm_flight_in_dsp_bytes -= app_status_report.pcm_down_frame_len * 2;    //FIX TODO
//             }
// #endif
//         }
//         break;
    case AUDIO_PIPE_EVENT_DATA_FILLED:
#ifdef APP_AUDIO_PIPE_DEBUG
        APP_PRINT_INFO1("pipe_handle: codec_type %u DATA FILLED", codec_type);
#endif
        /* FIXME: For decoding, we should not fill pkts too frequently. This
         * would make decoding hold dsp too long. It might make a2dp latency
         * bigger.
         * */
        pipe->busy = 0;
        if (pipe->mgr_cback)
        {
            pipe->mgr_cback(pipe->uid, APP_AUDIO_PIPE_EVENT_DATA_FILLED, NULL, 0);
        }
        break;
    case AUDIO_PIPE_EVENT_RELEASED:
        pipe->pipe = NULL;
        pipe->busy = 0; /* FIXME: What's the real pipe state */
        pipe->state = AUDIO_PIPE_STATE_RELEASED;
        APP_PRINT_ERROR1("pipe_handle: codec_type %u RELEASED", codec_type);
        if (codec_type == CODEC_SBCENC_TYPE || codec_type == CODEC_SBCENC2_TYPE)
        {
            pipe_associated &= ~(1 << codec_type);
        }
        if (codec_type == CODEC_LC3ENC_TYPE || codec_type == CODEC_LC3ENC2_TYPE)
        {
            pipe_associated &= ~(1 << codec_type);
        }
        if (pipe->in_frame_buffer != NULL)
        {
            free(pipe->in_frame_buffer);
            pipe->in_frame_buffer = NULL;
        }
        if (pipe->mgr_cback)
        {
            pipe->mgr_cback(pipe->uid, APP_AUDIO_PIPE_EVENT_RELEASED, NULL, 0);
            //audio_pipes[codec_type] = NULL;
        }
        break;
    case AUDIO_PIPE_EVENT_MIXED:
        /* TODO */
        APP_PRINT_INFO0("pipe_handle: Received pipe mixed event");
        break;
    default:
        APP_PRINT_ERROR1("app_audio_pipe_handle: Unknown event %04x",
                         event);
        break;
    }
}

static inline void app_audio_pipe_dec_fill_once(void)
{
    uint8_t i;
    T_APP_AUDIO_PIPE *pipe = NULL;

    for (i = 0; i < CODEC_MAX_TYPE; i++)
    {
        pipe = audio_pipes[i];
        if (pipe && pipe->pipe && !pipe->enc)
        {
            /* FIXME: We send a fake filled event to upper layer. It makes
             * upper layer try to fill data to pipe.
             * */
            if (pipe->mgr_cback)
            {
                pipe->mgr_cback(pipe->uid, APP_AUDIO_PIPE_EVENT_DATA_FILLED, NULL, 0);
            }
            break;
        }
    }
}

void app_audio_pipe_handle_msg(T_IO_MSG *msg)
{
    uint16_t subtype;
    uint8_t codec_type;
    uint8_t pipe_event;
    uint16_t  param;

    subtype = msg->subtype;
    pipe_event = msg->u.param & 0xff;
    codec_type = (msg->u.param >> 8) & 0xff;
    param = (msg->u.param >> 16) & 0xffff;

    /* APP_PRINT_INFO3("app_audio_pipe_handle_msg: subtype %04x, pipe evt %u, "
     *                 "codec_type %u", subtype, pipe_event, codec_type);
     */

    if (subtype == MSG_TYPE_AUDIO_PIPE_CODEC)
    {
        app_audio_pipe_handle(codec_type, pipe_event, param);
        return;
    }

    switch (subtype)
    {
    case MSG_TYPE_AUDIO_PIPE_RESUME_DEC:
        {
            /* Don't worry if the length of sent data is zero.
             * The fill function will check the watermark and determine if data
             * should be filled to pipe.
             * And we would drop the evt previously if the param is zero.
             * */
            if (dec_watermark >= param)
            {
                dec_watermark -= param;
            }
            else
            {
                APP_PRINT_ERROR2("MSG_TYPE_AUDIO_PIPE_RESUME_DEC: MIS-Match %u : %u",
                                 dec_watermark, param);
            }

            /* Currently we use this msg to drive sbcdec at regular intervals.
             * It seems good to balance encode and decode.
             * */
            if (dec_watermark >= DECODING_THRESHOLD)
            {
                break;
            }

            app_audio_pipe_dec_fill_once();
        }
        break;
    default:
        APP_PRINT_ERROR1("app_audio_pipe_handle_msg: Unknown subtype %04x",
                         subtype);
        break;
    }
}

uint16_t calc_sbc_frame_size(uint8_t chann_mode, uint8_t blocks,
                             uint8_t subbands, uint8_t bitpool)
{
    uint8_t channel_num = 0;
    uint16_t frame_size = 0;
    uint8_t joint = 0;
    uint16_t temp = 0;

    APP_PRINT_INFO4("calc_sbc_frame_size: ch %u, bl %u, subb %u, bitpool %u",
                    chann_mode, blocks, subbands, bitpool);

    switch (chann_mode)
    {
    case 3: /* joint-stereo */
        joint = 1;
        channel_num = 2;
        break;
    case 2: /* stereo */
        channel_num = 2;
        break;
    case 1: /* dual channels */
        channel_num = 2;
        break;
    case 0: /* mono */
        channel_num = 1;
        break;
    default:
        break;
    }

    if (chann_mode == 0 || chann_mode == 1)
    {
        //mono or dual channel
        temp = blocks * channel_num * bitpool;

        frame_size = 4 + subbands * channel_num / 2 + temp / 8;

        if (temp % 8 != 0)
        {
            frame_size++;
        }
    }
    else if (chann_mode == 2 || chann_mode == 3)
    {
        //stereo or joint stereo
        temp = joint * subbands + blocks * bitpool;

        frame_size = 4 + subbands * channel_num / 2 + temp / 8;

        if (temp % 8 != 0)
        {
            frame_size++;
        }
    }

    APP_PRINT_INFO1("calc_sbc_frame_size: frame size %d", frame_size);

    return frame_size;
}

static uint8_t count_uint32_t_bits(uint32_t value)
{
    uint8_t count = 0;

    while (value)
    {
        if (value & 0x01)
        {
            count++;
        }
        value >>= 1;
    }
    return count;
}

/* 1. the max pcm buf is 1k, provide by fwk limit, so we should cut down pcm len
 * 2. the param named pcm_frame_length actually is the point num of pcm,
 *  nothing to do with chann_num and bit_witch, the formula is
 *  pcm_frame_length = sample_rate * duration,
 *
 * 3. formula of pcm len which is inputed to dsp or outputed by dsp is
 *  pcm_len = pcm_frame_length * chan_num * bit_width;
 */
static uint16_t pipe_calc_pcm_frame_length(uint32_t sample_rate, uint8_t duration,
                                           uint8_t duration_div, uint8_t chan_num, uint8_t bit_width)
{
    if ((!duration_div) || (bit_width % 8))
    {
        APP_PRINT_ERROR2("pipe_calc_pcm_frame_length duration_div %d bit_width %d", duration_div,
                         bit_width);
        return 0;
    }

    uint32_t buf_size_limit = 1000;
    uint32_t pcm_frame_length = 0;
    uint32_t pcm_len = 0;
    uint8_t div_cnt = 1;

    /*pcm_frame_len = sample_rate * frame_duration * bit_width*/
    /*cause of fwk cache buf size (<1K), div 2*/
    /*in_frame_len = pcm_frame_len * chann_num */
    sample_rate = sample_rate / 1000;
    pcm_frame_length = sample_rate * duration / duration_div * bit_width / 8;
    pcm_len = pcm_frame_length * chan_num;

    while (pcm_len > buf_size_limit)
    {
        pcm_frame_length = pcm_frame_length / 2;
        pcm_len = pcm_len / 2;
        div_cnt = div_cnt * 2;
    }
    APP_PRINT_INFO6("pipe_calc_pcm_frame_length sr %d duration %d div %d cn %d bw %d pcm_len %d",
                    sample_rate, duration, duration_div, chan_num, bit_width, pcm_frame_length);
    return pcm_frame_length;
}

/*
 * If application invokes releasing func and creating func continuously.
 * Application would receive RELEASED event prior to CREATED event.
 * We do nothing in RELEASED event handler to avoid messing up pipe.
 */

bool app_audio_pipe_create(uint8_t codec_type, T_AUDIO_FORMAT_INFO *config,
                           void (*mgr_cback)(uint8_t id, uint8_t event, uint8_t *buf, uint32_t len),
                           uint32_t uid, uint8_t associated)
{
    T_AUDIO_FORMAT_INFO src;
    T_AUDIO_FORMAT_INFO snk;
    T_APP_AUDIO_PIPE *pipe = NULL;
    uint16_t frame_len;
    uint8_t duration;
    uint8_t duration_div;
    bool (*pipe_cback)(T_AUDIO_PIPE_HANDLE handle,
                       T_AUDIO_PIPE_EVENT  event,
                       uint32_t            param);

    if (!config)
    {
        APP_PRINT_ERROR1("pipe_create: Pipe (codec_type %u) config is null", codec_type);
        return false;
    }

    if (codec_type >= CODEC_MAX_TYPE)
    {
        APP_PRINT_ERROR2("pipe_create: codec_type %u exceeds max val %u",
                         codec_type, CODEC_MAX_TYPE);
        return false;
    }

    if (codec_type == CODEC_SBCDEC_TYPE || codec_type == CODEC_LC3DEC_TYPE)
    {
        rtp_frame_cnt = 0;
    }

    APP_PRINT_INFO3("app_audio_pipe_create codec_type %x playback %x record %x",
                    codec_type, app_cfg_const.dongle_usb_playback_sample_rate,
                    app_cfg_const.dongle_usb_record_sample_rate);
    switch (codec_type)
    {
    case CODEC_SBCENC_TYPE:
        pipe = &sbcenc_pipe;
        pipe->codec_type = codec_type;
        pipe->enc = 1;
        pipe_cback = app_audio_pipe_sbcenc_cback;

        memcpy(&sbcenc_pipe.fmt, config, sizeof(*config));

        /* FIXME: Set src.attr.pcm.sample_rate ? */
        app_usb_downstream_pcm_format_init(&src);
        /*pcm_frame_len = sample_rate * frame_duration * bit_width */
        /*in_frame_len = pcm_frame_len * chann_num */
        src.attr.pcm.frame_length = 256; /* FIXME: */

        snk.type = config->type;
        snk.frame_num = 1;
        snk.attr.sbc.sample_rate  = config->attr.sbc.sample_rate;
        snk.attr.sbc.chann_mode   = (T_AUDIO_SBC_CHANNEL_MODE)AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
        snk.attr.sbc.block_length = config->attr.sbc.block_length;
        snk.attr.sbc.subband_num  = config->attr.sbc.subband_num;
        snk.attr.sbc.bitpool      = config->attr.sbc.bitpool;
        snk.attr.sbc.allocation_method = 0x00;

        APP_PRINT_INFO5("pipe_create: sbcenc sf %u, bl %02x, cm %02x, a %u, s %u",
                        snk.attr.sbc.sample_rate,
                        snk.attr.sbc.block_length,
                        snk.attr.sbc.chann_mode,
                        snk.attr.sbc.allocation_method,
                        snk.attr.sbc.subband_num);

        /*in_frame_len = pcm_frame_len * chann_num */
        pipe->in_frame_len = src.attr.pcm.frame_length * src.attr.pcm.chann_num;
        pipe->in_frame_buffer = calloc(1, pipe->in_frame_len);
        if (pipe->in_frame_buffer == NULL)
        {
            APP_PRINT_WARN2("pipe_create: Pipe (%u) alloc %d fail",
                            codec_type, pipe->in_frame_len);
        }

        if (associated)
        {
            pipe_associated |= (1 << codec_type);
        }

        break;
    case CODEC_SBCENC2_TYPE:
        pipe = &sbcenc2_pipe;
        pipe->codec_type = codec_type;
        pipe->enc = 1;
        pipe_cback = app_audio_pipe_sbcenc2_cback;

        memcpy(&sbcenc2_pipe.fmt, config, sizeof(*config));

        /* FIXME: Set src.attr.pcm.sample_rate ? */
        app_usb_downstream_pcm_format_init(&src);
        /*pcm_frame_len = sample_rate * frame_duration * bit_width */
        /*in_frame_len = pcm_frame_len * chann_num */
        src.attr.pcm.frame_length = 256; /* FIXME: */

        snk.type = config->type;
        snk.frame_num = 1;
        snk.attr.sbc.sample_rate  = config->attr.sbc.sample_rate;
        snk.attr.sbc.chann_mode   = (T_AUDIO_SBC_CHANNEL_MODE)AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
        snk.attr.sbc.block_length = config->attr.sbc.block_length;
        snk.attr.sbc.subband_num  = config->attr.sbc.subband_num;
        snk.attr.sbc.bitpool      = config->attr.sbc.bitpool;
        snk.attr.sbc.allocation_method = 0x00;

        APP_PRINT_INFO5("pipe_create: sbcenc2 sf %u, bl %02x, cm %02x, a %u, s %u",
                        snk.attr.sbc.sample_rate,
                        snk.attr.sbc.block_length,
                        snk.attr.sbc.chann_mode,
                        snk.attr.sbc.allocation_method,
                        snk.attr.sbc.subband_num);

        /*in_frame_len = pcm_frame_len * chann_num */
        pipe->in_frame_len = src.attr.pcm.frame_length * src.attr.pcm.chann_num;
        pipe->in_frame_buffer = calloc(1, pipe->in_frame_len);
        if (pipe->in_frame_buffer == NULL)
        {
            APP_PRINT_WARN2("pipe_create: Pipe (codec_type %u) alloc %d fail",
                            codec_type, pipe->in_frame_len);
        }

        if (associated)
        {
            pipe_associated |= (1 << codec_type);
        }

        break;
    case CODEC_SBCDEC_TYPE:
        pipe = &sbcdec_pipe;
        pipe->codec_type = codec_type;
        pipe_cback = app_audio_pipe_sbcdec_cback;

        memcpy(&sbcdec_pipe.fmt, config, sizeof(*config));

        src.type = AUDIO_FORMAT_TYPE_SBC;
        src.frame_num = 1;
        src.attr.sbc.sample_rate       = config->attr.sbc.sample_rate;
        src.attr.sbc.allocation_method = config->attr.sbc.allocation_method;
        src.attr.sbc.block_length      = config->attr.sbc.block_length;
        src.attr.sbc.chann_mode        = config->attr.sbc.chann_mode;
        src.attr.sbc.bitpool           = config->attr.sbc.bitpool;
        src.attr.sbc.subband_num       = config->attr.sbc.subband_num;

        APP_PRINT_INFO5("pipe_create: sbcdec sf %u, bl %02x, cm %02x, a %u, s %u",
                        src.attr.sbc.sample_rate,
                        src.attr.sbc.block_length,
                        src.attr.sbc.chann_mode,
                        src.attr.sbc.allocation_method,
                        src.attr.sbc.subband_num);

        app_usb_upstream_pcm_format_init(&snk);
        /* FIXME: How to determine the pcm sample rate? */
        /*pcm_frame_len = sample_rate * frame_duration * bit_width */
        /*in_frame_len = pcm_frame_len * chann_num */
        if (snk.attr.pcm.sample_rate == 48000)
        {
            /* FIXME: How long will it be suitable for getting one data indication
            * after filling a packet.
            * */
            snk.attr.pcm.frame_length = 128 * 3 * snk.attr.pcm.bit_width / 8;
        }
        else if (snk.attr.pcm.sample_rate == 32000)
        {
            //current muc cfg only support choose 16K/48k
            snk.attr.pcm.frame_length = 128 * 2 * snk.attr.pcm.bit_width / 8;
        }
        else
        {
            snk.attr.pcm.frame_length = 128 * snk.attr.pcm.bit_width / 8;
        }
        /*in_frame_len = pcm_frame_len * chann_num */
        pipe->pcm_frame_len = snk.attr.pcm.frame_length * snk.attr.pcm.chann_num;

        frame_len = calc_sbc_frame_size(src.attr.sbc.chann_mode,
                                        src.attr.sbc.block_length,
                                        src.attr.sbc.subband_num,
                                        src.attr.sbc.bitpool);
        if (src.attr.sbc.sample_rate == 32000)
        {
            pipe->in_frame_len = 6 * frame_len;
        }
        else if (src.attr.sbc.sample_rate == 48000)
        {
            pipe->in_frame_len = 3 * frame_len;
        }
        else
        {
            pipe->in_frame_len = frame_len;
        }

        pipe->frame_duration = (src.attr.sbc.block_length * src.attr.sbc.subband_num) * 1000 /
                               src.attr.sbc.sample_rate;

        app_audio_pipe_clear_dec_watermark();
        /* TODO: cyclic_ihdr + N bytes frame len */
        pipe->in_frame_buffer = calloc(1, pipe->in_frame_len + sizeof(struct cyclic_ihdr));
        if (pipe->in_frame_buffer == NULL)
        {
            APP_PRINT_WARN2("pipe_create: Pipe (%u) alloc %d fail",
                            codec_type, pipe->in_frame_len + 1);
        }

        APP_PRINT_INFO2("pipe_create: bitpool %u, size %u",
                        src.attr.sbc.bitpool, pipe->in_frame_len);

        break;
    case CODEC_LC3ENC_TYPE:
        pipe = &lc3enc_pipe;
        pipe->codec_type = codec_type;
        pipe->enc = 1;
        pipe_cback = app_audio_pipe_lc3enc_cback;

        snk = *config;
        snk.frame_num = 1;

        app_usb_downstream_pcm_format_init(&src);
        /*pcm_frame_len = sample_rate * frame_duration * bit_width*/
        /*cause of fwk cache buf size (<1K), div 2*/
        /*in_frame_len = pcm_frame_len * chann_num */
        if (snk.attr.lc3.frame_duration == AUDIO_LC3_FRAME_DURATION_7_5_MS)
        {
            src.attr.pcm.frame_length = (src.attr.pcm.sample_rate * 15 / 1000) / 4 *
                                        src.attr.pcm.bit_width / 8;
        }
        else
        {
            src.attr.pcm.frame_length = (src.attr.pcm.sample_rate * 10 / 1000) / 2 *
                                        src.attr.pcm.bit_width / 8;
        }

        /* FIXME: Calculate the in_frame_len with frame duration and sample
         * rate.
         */
        /*in_frame_len = pcm_frame_len * chann_num */
        pipe->in_frame_len = src.attr.pcm.frame_length * src.attr.pcm.chann_num;

        pipe->in_frame_buffer = calloc(1, pipe->in_frame_len);
        if (pipe->in_frame_buffer == NULL)
        {
            APP_PRINT_WARN2("pipe_create: Pipe (%u) alloc %d fail",
                            codec_type, pipe->in_frame_len);
        }

        pipe->expect_drain_len = snk.attr.lc3.frame_length;
        APP_PRINT_INFO5("pipe_create: lc3enc sr %u, loc %08x, dur %u, len %u, ilen %u",
                        snk.attr.lc3.sample_rate,
                        snk.attr.lc3.chann_location,
                        snk.attr.lc3.frame_duration,
                        snk.attr.lc3.frame_length,
                        pipe->in_frame_len);
#if APP_DEBUG_REPORT
        app_status_report.codec_down_frame_len = snk.attr.lc3.frame_length;
        app_status_report.pcm_down_frame_len = 960;
#endif
        if (associated)
        {
            pipe_associated |= (1 << codec_type);
        }
        break;
    case CODEC_LC3ENC2_TYPE:
        pipe = &lc3enc2_pipe;
        pipe->codec_type = codec_type;
        pipe->enc = 1;
        pipe_cback = app_audio_pipe_lc3enc2_cback;
        snk = *config;
        snk.frame_num = 1;

        app_usb_downstream_pcm_format_init(&src);
        /*pcm_frame_len = sample_rate * frame_duration * bit_width*/
        /*cause of fwk cache buf size (<1K), div 2*/
        /*in_frame_len = pcm_frame_len * chann_num */
        if (snk.attr.lc3.frame_duration == AUDIO_LC3_FRAME_DURATION_7_5_MS)
        {
            src.attr.pcm.frame_length = (src.attr.pcm.sample_rate * 15 / 1000) / 4 *
                                        src.attr.pcm.bit_width / 8;
        }
        else
        {
            src.attr.pcm.frame_length = (src.attr.pcm.sample_rate * 10 / 1000) / 2 *
                                        src.attr.pcm.bit_width / 8;
        }

        /* FIXME: Calculate the in_frame_len with frame duration and sample
         * rate.
         */
        /*in_frame_len = pcm_frame_len * chann_num */
        pipe->in_frame_len = src.attr.pcm.frame_length * src.attr.pcm.chann_num;

        pipe->in_frame_buffer = calloc(1, pipe->in_frame_len);
        if (pipe->in_frame_buffer == NULL)
        {
            APP_PRINT_WARN2("pipe_create: Pipe (%u) alloc %d fail",
                            codec_type, pipe->in_frame_len);
        }

        pipe->expect_drain_len = snk.attr.lc3.frame_length;
        APP_PRINT_INFO5("pipe_create: lc3enc2 sr %u, loc %08x, dur %u, len %u, ilen %u",
                        snk.attr.lc3.sample_rate,
                        snk.attr.lc3.chann_location,
                        snk.attr.lc3.frame_duration,
                        snk.attr.lc3.frame_length,
                        pipe->in_frame_len);
#if APP_DEBUG_REPORT
        app_status_report.codec_down_frame_len = snk.attr.lc3.frame_length;
        app_status_report.pcm_down_frame_len = 960;
#endif
        if (associated)
        {
            pipe_associated |= (1 << codec_type);
        }
        break;
    case CODEC_LC3DEC_TYPE:
        {
            pipe = &lc3dec_pipe;
            pipe->codec_type = codec_type;
            pipe_cback = app_audio_pipe_lc3dec_cback;

            src = *config;
            src.frame_num = 1;

            app_usb_upstream_pcm_format_init(&snk); /* one chann usb upstream, mix by dsp */

            uint8_t chnl_count = src.attr.lc3.chann_location == AUDIO_CHANNEL_LOCATION_MONO ? 1 :
                                 count_uint32_t_bits(src.attr.lc3.chann_location);
            pipe->in_frame_len = src.attr.lc3.frame_length * chnl_count;
            APP_PRINT_INFO2("CODEC_LC3DEC_TYPE playback %x, record %x",
                            app_cfg_const.dongle_usb_playback_sample_rate,
                            app_cfg_const.dongle_usb_record_sample_rate);

            if (src.attr.lc3.frame_duration == AUDIO_LC3_FRAME_DURATION_7_5_MS)
            {
                duration = 15;
                duration_div = 2;
            }
            else
            {
                duration = 10;
                duration_div = 1;
            }
            snk.attr.pcm.frame_length = pipe_calc_pcm_frame_length(snk.attr.pcm.sample_rate, duration,
                                                                   duration_div,
                                                                   snk.attr.pcm.chann_num,
                                                                   snk.attr.pcm.bit_width);
            pipe->pcm_frame_len = snk.attr.pcm.frame_length * snk.attr.pcm.chann_num;
            pipe->expect_drain_len = pipe->pcm_frame_len;

            /* TODO: cyclic_ihdr + N bytes frame len */
            pipe->in_frame_buffer = calloc(1, pipe->in_frame_len + sizeof(struct cyclic_ihdr));
            if (pipe->in_frame_buffer == NULL)
            {
                APP_PRINT_WARN2("pipe_create: Pipe (%u) alloc %d fail",
                                codec_type, pipe->in_frame_len + 1);
            }
            APP_PRINT_INFO1("pipe_create watermark %d", dec_watermark);
            app_audio_pipe_clear_dec_watermark();
            APP_PRINT_INFO5("pipe_create: lc3dec sr %u, loc %08x, dur %u, len %u, ilen %u",
                            src.attr.lc3.sample_rate,
                            src.attr.lc3.chann_location,
                            src.attr.lc3.frame_duration,
                            src.attr.lc3.frame_length,
                            pipe->in_frame_len);
#if APP_DEBUG_REPORT
            app_status_report.codec_up_frame_len = src.attr.lc3.frame_length;
            app_status_report.pcm_up_frame_len = 960;
#endif
        }
        break;
    case CODEC_MSBCENC_TYPE:
        {
            pipe = &msbcenc_pipe;
            pipe->codec_type = codec_type;
            pipe->enc = 1;
            pipe_cback = app_audio_pipe_msbcenc_cback;

            memcpy(&msbcenc_pipe.fmt, config, sizeof(*config));

            /* FIXME: Set src.attr.pcm.sample_rate ? */
            app_usb_downstream_pcm_format_init(&src);
            /*pcm_frame_len = sample_rate * frame_duration * bit_width*/
            /*cause of fwk cache buf size (<1K), div 2*/
            /*in_frame_len = pcm_frame_len * chann_num */
            src.attr.pcm.frame_length = src.attr.pcm.sample_rate / 1000 * 15 / 4 *
                                        (src.attr.pcm.bit_width / 8);

            snk.type = config->type;
            snk.frame_num = 1;
            snk.attr.msbc.sample_rate  = config->attr.msbc.sample_rate;
            snk.attr.msbc.chann_mode   = (T_AUDIO_SBC_CHANNEL_MODE)AUDIO_SBC_CHANNEL_MODE_MONO;
            snk.attr.msbc.block_length = config->attr.msbc.block_length;
            snk.attr.msbc.subband_num  = config->attr.msbc.subband_num;
            snk.attr.msbc.bitpool      = config->attr.msbc.bitpool;
            snk.attr.msbc.allocation_method = 0x00;

            APP_PRINT_INFO5("pipe_create: msbcenc sf %u, bl %02x, cm %02x, a %u, s %u",
                            snk.attr.msbc.sample_rate,
                            snk.attr.msbc.block_length,
                            snk.attr.msbc.chann_mode,
                            snk.attr.msbc.allocation_method,
                            snk.attr.msbc.subband_num);

            pipe->in_frame_len = src.attr.pcm.frame_length * src.attr.pcm.chann_num;
            pipe->in_frame_buffer = calloc(1, pipe->in_frame_len);
            if (pipe->in_frame_buffer == NULL)
            {
                APP_PRINT_WARN2("pipe_create: Pipe (%u) alloc %d fail",
                                codec_type, pipe->in_frame_len);
            }
        }
        break;
    case CODEC_MSBCDEC_TYPE:
        {
            pipe = &msbcdec_pipe;
            pipe->codec_type = codec_type;
            pipe_cback = app_audio_pipe_msbcdec_cback;

            memcpy(&msbcdec_pipe.fmt, config, sizeof(*config));

            src.type = config->type;
            src.frame_num = 1;
            src.attr.msbc.sample_rate       = config->attr.msbc.sample_rate;
            src.attr.msbc.allocation_method = config->attr.msbc.allocation_method;
            src.attr.msbc.block_length      = config->attr.msbc.block_length;
            src.attr.msbc.chann_mode        = config->attr.msbc.chann_mode;
            src.attr.msbc.bitpool           = config->attr.msbc.bitpool;
            src.attr.msbc.subband_num       = config->attr.msbc.subband_num;

            APP_PRINT_INFO5("pipe_create: msbcdec sf %u, bl %02x, cm %02x, a %u, s %u",
                            src.attr.msbc.sample_rate,
                            src.attr.msbc.block_length,
                            src.attr.msbc.chann_mode,
                            src.attr.msbc.allocation_method,
                            src.attr.msbc.subband_num);
            pipe->in_frame_len = calc_sbc_frame_size(src.attr.msbc.chann_mode,
                                                     src.attr.msbc.block_length,
                                                     src.attr.msbc.subband_num,
                                                     src.attr.msbc.bitpool);

            /* FIXME: How to determine the pcm sample rate? */
            app_usb_upstream_pcm_format_init(&snk);

            if (snk.attr.pcm.sample_rate == 48000)
            {
                snk.attr.pcm.frame_length = 360 * snk.attr.pcm.bit_width / 8;
            }
            else if (snk.attr.pcm.sample_rate == 32000)
            {
                //current muc cfg only support choose 16K/48k
                snk.attr.pcm.frame_length = 120 * 2 * snk.attr.pcm.bit_width / 8;
            }
            else
            {
                snk.attr.pcm.frame_length = 120 * snk.attr.pcm.bit_width / 8;
            }
            pipe->pcm_frame_len = snk.attr.pcm.frame_length * snk.attr.pcm.chann_num;
            /* TODO: cyclic_ihdr + N bytes frame len */
            pipe->in_frame_buffer = calloc(1, pipe->in_frame_len + sizeof(struct cyclic_ihdr));
            if (pipe->in_frame_buffer == NULL)
            {
                APP_PRINT_WARN2("pipe_create: Pipe (%u) alloc %d fail",
                                codec_type, pipe->in_frame_len + 1);
            }
            app_audio_pipe_clear_dec_watermark();
            APP_PRINT_INFO2("pipe_create: bitpool %u, size %u",
                            src.attr.sbc.bitpool, pipe->in_frame_len);

        }
        break;
    default:
        APP_PRINT_ERROR1("pipe_create: Unsupported pipe codec_type %u", codec_type);
        return false;
    }

    if (pipe->pipe)
    {
        APP_PRINT_WARN1("pipe_create: Pipe (codec_type %u) is already created",
                        codec_type);
        return false;
    }

    pipe->pipe = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL, src, snk, 0, pipe_cback);
    pipe->mgr_cback = mgr_cback;
    pipe->uid = uid;

    if (!pipe->pipe)
    {
        APP_PRINT_ERROR1("pipe_create: Cannot create codec_type (%u)", codec_type);
        pipe->mgr_cback = NULL;
        return false;
    }
    pipe->busy = 1;
    audio_pipes[codec_type] = pipe;

    APP_PRINT_INFO2("pipe_create: pipe %p, codec_type %u", pipe->pipe, codec_type);

    return true;
}

void app_audio_pipe_release(uint8_t codec_type)
{
    T_APP_AUDIO_PIPE *pipe = NULL;

    pipe = app_audio_pipe_instance(codec_type);
    if (!pipe)
    {
        return;
    }

    if (pipe->pipe)
    {
        APP_PRINT_INFO1("pipe_release: Pipe (codec_type %u) is releasing ...", codec_type);
        audio_pipe_release(pipe->pipe);
    }
    else
    {
        APP_PRINT_WARN1("pipe_release: Pipe (codec_type %u) is not created", codec_type);
    }
#if APP_DEBUG_REPORT
    //FIX TODO
    app_status_report_init();
#endif
}

bool app_audio_pipe_change_uid(uint8_t codec_type, uint32_t uid)
{
    T_APP_AUDIO_PIPE *pipe = NULL;

    pipe = app_audio_pipe_instance(codec_type);
    if (!pipe)
    {
        return false;
    }

    pipe->uid = uid;

    return true;
}

/* FIXME: We must fill the destination buffer and then add the watermark
 * explicitly.
 * */
bool app_audio_pipe_add_watermark(uint8_t codec_type, uint16_t value)
{
    T_APP_AUDIO_PIPE *pipe = NULL;

    if (codec_type != CODEC_SBCDEC_TYPE && codec_type != CODEC_LC3DEC_TYPE &&
        codec_type != CODEC_MSBCDEC_TYPE)
    {
        APP_PRINT_ERROR1("app_audio_pipe_add_watermark: Invalid codec_type %u",
                         codec_type);
        return false;
    }

    pipe = app_audio_pipe_instance(codec_type);
    if (!pipe)
    {
        APP_PRINT_ERROR1("app_audio_pipe_add_watermark: No pipe, codec_type %u",
                         codec_type);
        return false;
    }

    dec_watermark += value;

    return true;
}

RAM_TEXT_SECTION uint32_t app_audio_pipe_watermark(void)
{
    return (DECODING_THRESHOLD << 16) | dec_watermark;
}

bool app_audio_pipe_set_volume(uint16_t value)
{
    if (lc3enc_pipe.pipe)
    {
        return audio_pipe_gain_set(lc3enc_pipe.pipe, value, value);
    }
    return false;
}

bool app_audio_pipe_set_gain(uint8_t codec_type, uint16_t gain)
{
    T_APP_AUDIO_PIPE *pipe = NULL;

    pipe = app_audio_pipe_instance(codec_type);
    if (!pipe)
    {
        APP_PRINT_ERROR1("app_audio_pipe_set_gain: No instance for type (codec_type %u)",
                         codec_type);
        return false;
    }

    APP_PRINT_INFO2("app_audio_pipe_set_gain: Set gain %04x for type (codec_type %d)",
                    gain, codec_type);
    return audio_pipe_gain_set(pipe->pipe, gain, gain);
}

void app_audio_pipe_set_rtp_frame_cnt(uint8_t cnt)
{
    rtp_frame_cnt = cnt;
}

void app_audio_pipe_init(void *evt_queue, void *msg_queue, uint8_t io_msg_type)
{
    APP_PRINT_INFO0("app_audio_pipe_init(): ++");

    audio_pipe_evt_q = evt_queue;
    audio_pipe_msg_q = msg_queue;
    pipe_io_msg_type = io_msg_type;

    /* TODO: We don't know the exact draining data length */
    pipe_drain_buf = calloc(1, PIPE_DRAIN_BUF_LEN);
    if (!pipe_drain_buf)
    {
        APP_PRINT_ERROR0("app_audio_pipe_init: Cannot alloc mem for draining");
    }
}
