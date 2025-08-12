/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_AM_AUDIO_PIPE_SUPPORT == 1)
#include <string.h>
#include <stdint.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "dsp_mgr.h"
#include "audio_type.h"
#include "audio_codec.h"
#include "audio_pipe.h"
#include "section.h"

/* TODO Remove Start */
#include "dsp_ipc.h"
#include "dsp_shm.h"
#include "audio_path.h"
/* TODO Remove End */

#define AUDIO_CODEC_DEBUG 0

#define MAX_MIXED_PIPE_NUM              (2)

#define AUDIO_CODEC_SNK_BUF_SIZE        (1024)

#define AUDIO_PIPE_IPC_BUF_SIZE         (2048)

#define AUDIO_PIPE_PEEK_BUF_SIZE        (2048)

typedef enum t_audio_pipe_state
{
    AUDIO_PIPE_STATE_RELEASED,
    AUDIO_PIPE_STATE_CREATING,
    AUDIO_PIPE_STATE_CREATED,
    AUDIO_PIPE_STATE_STARTING,
    AUDIO_PIPE_STATE_STARTED,
    AUDIO_PIPE_STATE_STOPPING,
    AUDIO_PIPE_STATE_STOPPED,
    AUDIO_PIPE_STATE_RELEASING,
} T_AUDIO_PIPE_STATE;

typedef enum t_audio_pipe_action
{
    AUDIO_PIPE_ACTION_NONE,
    AUDIO_PIPE_ACTION_CREATE,
    AUDIO_PIPE_ACTION_START,
    AUDIO_PIPE_ACTION_STOP,
    AUDIO_PIPE_ACTION_RELEASE,
} T_AUDIO_PIPE_ACTION;

typedef struct t_coder_instance
{
    uint8_t  coder_id;
    uint8_t  frame_num;
    uint16_t format_size;
    union
    {
        T_DIPC_PCM_CODER_FORMAT     pcm;
        T_DIPC_CVSD_CODER_FORMAT    cvsd;
        T_DIPC_MSBC_CODER_FORMAT    msbc;
        T_DIPC_SBC_CODER_FORMAT     sbc;
        T_DIPC_AAC_CODER_FORMAT     aac;
        T_DIPC_OPUS_CODER_FORMAT    opus;
        T_DIPC_MP3_CODER_FORMAT     mp3;
        T_DIPC_LC3_CODER_FORMAT     lc3;
        T_DIPC_LDAC_CODER_FORMAT    ldac;
        T_DIPC_FLAC_CODER_FORMAT    flac;
        T_DIPC_LHDC_CODER_FORMAT    lhdc;
        T_DIPC_G729_CODER_FORMAT    g729;
    } format;
} T_CODER_INSTANCE;

typedef struct t_audio_pipe
{
    struct t_audio_pipe      *p_next;
    T_CODER_INSTANCE          decoder;
    T_CODER_INSTANCE          encoder;
    T_DSP_MGR_SESSION_HANDLE  dsp_session;
    T_AUDIO_PIPE_HANDLE       mixed_pipes[MAX_MIXED_PIPE_NUM];
    P_AUDIO_PIPE_CBACK        cback;
    uint32_t                  src_transport_address;
    uint32_t                  src_transport_size;
    uint32_t                  snk_transport_address;
    uint32_t                  snk_transport_size;
    uint16_t                  gain_left;
    uint16_t                  gain_right;
    T_AUDIO_PIPE_STATE        state;
    T_AUDIO_PIPE_ACTION       action;
    uint8_t                   mixed_pipe_num;
    uint8_t                   credits;
    T_AUDIO_STREAM_MODE       mode;
} T_AUDIO_PIPE;

typedef struct t_audio_pipe_db
{
    T_OS_QUEUE               pipes;
    T_SYS_IPC_HANDLE         dsp_event;
} T_AUDIO_PIPE_DB;

typedef struct t_dipc_data_header
{
    uint32_t     sync_word;
    uint32_t     session_id;
    uint32_t     timestamp;
    uint16_t     seq_num;
    uint16_t     frame_count;
    uint8_t      frame_num;
    uint8_t      status;
    uint16_t     payload_length;
    uint32_t     tail;
} T_DIPC_DATA_HEADER;

typedef uint8_t (*P_FORMAT_INIT)(T_CODER_INSTANCE *coder_inst, void *coder_attr);

static bool audio_pipe_state_set(T_AUDIO_PIPE *audio_pipe, T_AUDIO_PIPE_STATE state);

static T_AUDIO_PIPE_DB *audio_pipe_db = NULL;
static uint8_t *audio_pipe_ipc_buf = NULL;
static uint8_t *audio_pipe_peek_buf = NULL;

static uint8_t audio_pipe_pcm_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    T_DIPC_PCM_CODER_FORMAT *pcm_format;
    T_AUDIO_PCM_ATTR *pcm_attr;

    coder_inst->coder_id = DIPC_CODER_ID_PCM;
    coder_inst->format_size = sizeof(T_DIPC_PCM_CODER_FORMAT);
    pcm_format = &(coder_inst->format.pcm);
    pcm_attr = (T_AUDIO_PCM_ATTR *)coder_attr;

    pcm_format->sample_rate = pcm_attr->sample_rate;
    pcm_format->frame_size = audio_codec_frame_size_get(AUDIO_FORMAT_TYPE_PCM, coder_attr);
    pcm_format->chann_num = pcm_attr->chann_num;
    pcm_format->bit_width = pcm_attr->bit_width;

    return 0;
}

static uint8_t audio_pipe_cvsd_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    T_DIPC_CVSD_CODER_FORMAT *cvsd_format;
    T_AUDIO_CVSD_ATTR *cvsd_attr;

    coder_inst->coder_id = DIPC_CODER_ID_CVSD;
    coder_inst->format_size = sizeof(T_DIPC_CVSD_CODER_FORMAT);
    cvsd_format = &(coder_inst->format.cvsd);
    cvsd_attr = (T_AUDIO_CVSD_ATTR *)coder_attr;

    cvsd_format->sample_rate = cvsd_attr->sample_rate;
    cvsd_format->frame_size = audio_codec_frame_size_get(AUDIO_FORMAT_TYPE_CVSD, cvsd_attr);
    cvsd_format->chann_num = cvsd_attr->chann_num;
    cvsd_format->bit_width = 0x10;

    return 0;
}

static uint8_t audio_pipe_msbc_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    T_DIPC_MSBC_CODER_FORMAT *msbc_format;
    T_AUDIO_MSBC_ATTR *msbc_attr;

    coder_inst->coder_id = DIPC_CODER_ID_MSBC;
    coder_inst->format_size = sizeof(T_DIPC_MSBC_CODER_FORMAT);
    msbc_format = &(coder_inst->format.msbc);
    msbc_attr = (T_AUDIO_MSBC_ATTR *)coder_attr;

    msbc_format->sample_rate = msbc_attr->sample_rate;
    msbc_format->frame_size = msbc_attr->block_length * msbc_attr->subband_num; //need confirm
    msbc_format->chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_MSBC, msbc_attr);
    msbc_format->bit_width = 0x10;//need confirm
    msbc_format->chann_mode = msbc_attr->chann_mode;
    msbc_format->block_length = msbc_attr->block_length;
    msbc_format->subband_num = msbc_attr->subband_num;
    msbc_format->allocation_method = msbc_attr->allocation_method;
    msbc_format->bitpool = msbc_attr->bitpool;

    return 0;
}

static uint8_t audio_pipe_sbc_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    T_DIPC_SBC_CODER_FORMAT *sbc_format;
    T_AUDIO_SBC_ATTR *sbc_attr;

    coder_inst->coder_id = DIPC_CODER_ID_SBC;
    coder_inst->format_size = sizeof(T_DIPC_SBC_CODER_FORMAT);
    sbc_format = &(coder_inst->format.sbc);
    sbc_attr = (T_AUDIO_SBC_ATTR *)coder_attr;

    sbc_format->sample_rate = sbc_attr->sample_rate;
    sbc_format->frame_size = sbc_attr->block_length * sbc_attr->subband_num;  //need confirm
    sbc_format->chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_SBC, sbc_attr);
    sbc_format->bit_width = 0x10;  //need confirm
    sbc_format->chann_mode = sbc_attr->chann_mode;
    sbc_format->block_length = sbc_attr->block_length;
    sbc_format->subband_num = sbc_attr->subband_num;
    sbc_format->allocation_method = sbc_attr->allocation_method;
    sbc_format->bitpool = sbc_attr->bitpool;

    return 0;
}

static uint8_t audio_pipe_aac_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    T_DIPC_AAC_CODER_FORMAT *aac_format;
    T_AUDIO_AAC_ATTR *aac_attr;

    coder_inst->coder_id = DIPC_CODER_ID_AAC;
    coder_inst->format_size = sizeof(T_DIPC_AAC_CODER_FORMAT);
    aac_attr = (T_AUDIO_AAC_ATTR *)coder_attr;
    aac_format = &(coder_inst->format.aac);

    aac_format->sample_rate = aac_attr->sample_rate;
    aac_format->frame_size = 1024;
    aac_format->chann_num = aac_attr->chann_num;
    aac_format->bit_width = 0x10; /* default 16 bit, need confirm */
    aac_format->transport_format = aac_attr->transport_format;
    aac_format->object_type = aac_attr->object_type;
    aac_format->vbr = aac_attr->vbr;
    aac_format->bitrate = aac_attr->bitrate;

    return 0;
}

static uint8_t audio_pipe_opus_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    T_DIPC_OPUS_CODER_FORMAT *opus_format;
    T_AUDIO_OPUS_ATTR *opus_attr;

    coder_inst->coder_id = DIPC_CODER_ID_OPUS;
    coder_inst->format_size = sizeof(T_DIPC_OPUS_CODER_FORMAT);
    opus_attr = (T_AUDIO_OPUS_ATTR *)coder_attr;
    opus_format = &(coder_inst->format.opus);

    opus_format->sample_rate = opus_attr->sample_rate;
    opus_format->frame_size = audio_codec_frame_size_get(AUDIO_FORMAT_TYPE_OPUS, opus_attr);
    opus_format->chann_num = opus_attr->chann_num;
    opus_format->bit_width = 0x10; /* default 16 bit, need confirm */
    opus_format->cbr = opus_attr->cbr;
    opus_format->cvbr = opus_attr->cvbr;
    opus_format->mode = opus_attr->mode;
    opus_format->complexity = opus_attr->complexity;
    opus_format->bitrate = opus_attr->bitrate;

    return 0;
}

static uint8_t audio_pipe_flac_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    return 0;
}

static uint8_t audio_pipe_mp3_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    T_DIPC_MP3_CODER_FORMAT *mp3_format;
    T_AUDIO_MP3_ATTR *mp3_attr;

    coder_inst->coder_id = DIPC_CODER_ID_MP3;
    coder_inst->format_size = sizeof(T_DIPC_MP3_CODER_FORMAT);
    mp3_format = &(coder_inst->format.mp3);
    mp3_attr = (T_AUDIO_MP3_ATTR *)coder_attr;

    mp3_format->sample_rate = mp3_attr->sample_rate;
    mp3_format->frame_size = audio_codec_frame_size_get(AUDIO_FORMAT_TYPE_MP3, mp3_attr);
    mp3_format->chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_MP3, mp3_attr);
    mp3_format->bit_width = 0x10; /* default 16 bit, need comfirm */
    mp3_format->chann_mode = mp3_attr->chann_mode;
    mp3_format->version = mp3_attr->version;
    mp3_format->layer = mp3_attr->layer;
    mp3_format->bitrate = mp3_attr->bitrate;

    return 0;
}

static uint8_t audio_pipe_lc3_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    T_DIPC_LC3_CODER_FORMAT *lc3_format;
    T_AUDIO_LC3_ATTR *lc3_attr;

    coder_inst->coder_id = DIPC_CODER_ID_LC3;
    coder_inst->format_size = sizeof(T_DIPC_LC3_CODER_FORMAT);
    lc3_format = &(coder_inst->format.lc3);
    lc3_attr = (T_AUDIO_LC3_ATTR *)coder_attr;

    lc3_format->sample_rate = lc3_attr->sample_rate;
    lc3_format->frame_size = audio_codec_frame_size_get(AUDIO_FORMAT_TYPE_LC3, lc3_attr);
    lc3_format->chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_LC3, lc3_attr);
    lc3_format->bit_width = 0x10; /* default 16-bit, need confirm */
    lc3_format->frame_length = lc3_attr->frame_length;
    lc3_format->chann_location = lc3_attr->chann_location;
    lc3_format->presentation_delay = lc3_attr->presentation_delay;

    return 0;
}

static uint8_t audio_pipe_ldac_format_init(T_CODER_INSTANCE *coder_inst, void *coder_attr)
{
    T_DIPC_LDAC_CODER_FORMAT *ldac_format;
    T_AUDIO_LDAC_ATTR *ldac_attr;

    coder_inst->coder_id = DIPC_CODER_ID_LDAC;
    coder_inst->format_size = sizeof(T_DIPC_LDAC_CODER_FORMAT);
    ldac_attr = (T_AUDIO_LDAC_ATTR *)coder_attr;
    ldac_format = &(coder_inst->format.ldac);

    ldac_format->sample_rate = ldac_attr->sample_rate;
    ldac_format->frame_size = audio_codec_frame_size_get(AUDIO_FORMAT_TYPE_LDAC, ldac_attr);
    ldac_format->chann_num = audio_codec_chann_num_get(AUDIO_FORMAT_TYPE_LDAC, ldac_attr);
    ldac_format->bit_width = 0x10;
    ldac_format->chann_mode = ldac_attr->chann_mode;

    return 0;
}

///TODO: move to dsp_mgr
static const P_FORMAT_INIT format_init[] =
{
    [AUDIO_FORMAT_TYPE_PCM ] = audio_pipe_pcm_format_init,
    [AUDIO_FORMAT_TYPE_CVSD] = audio_pipe_cvsd_format_init,
    [AUDIO_FORMAT_TYPE_MSBC] = audio_pipe_msbc_format_init,
    [AUDIO_FORMAT_TYPE_SBC ] = audio_pipe_sbc_format_init,
    [AUDIO_FORMAT_TYPE_AAC ] = audio_pipe_aac_format_init,
    [AUDIO_FORMAT_TYPE_OPUS] = audio_pipe_opus_format_init,
    [AUDIO_FORMAT_TYPE_FLAC] = audio_pipe_flac_format_init,
    [AUDIO_FORMAT_TYPE_MP3 ] = audio_pipe_mp3_format_init,
    [AUDIO_FORMAT_TYPE_LC3 ] = audio_pipe_lc3_format_init,
    [AUDIO_FORMAT_TYPE_LDAC] = audio_pipe_ldac_format_init,
};

static bool audio_pipe_dsp_cback(uint32_t event, void *msg)
{
    T_AUDIO_PIPE *audio_pipe;

    switch (event)
    {
    case DSP_MGR_EVT_INIT_FINISH:
        {
            if (dsp_mgr_power_on_check() == false)
            {
                dsp_mgr_power_on();
            }

            audio_pipe = os_queue_peek(&audio_pipe_db->pipes, 0);
            while (audio_pipe != NULL)
            {
                if (audio_pipe->state == AUDIO_PIPE_STATE_CREATING)
                {
                    dsp_mgr_session_enable(audio_pipe->dsp_session);
                }

                audio_pipe = audio_pipe->p_next;
            }
        }
        break;

    case DSP_MGR_EVT_POWER_OFF:
        {
            audio_pipe = os_queue_peek(&audio_pipe_db->pipes, 0);
            while (audio_pipe != NULL)
            {
                if (audio_pipe->state == AUDIO_PIPE_STATE_RELEASED)
                {
                    audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_CREATING);
                }

                audio_pipe = audio_pipe->p_next;
            }
        }
        break;

    default:
        break;
    }

    return true;
}

bool audio_pipe_init(void)
{
    audio_pipe_db = os_mem_alloc2(sizeof(T_AUDIO_PIPE_DB));
    if (audio_pipe_db == NULL)
    {
        return false;
    }

    os_queue_init(&audio_pipe_db->pipes);

    audio_pipe_db->dsp_event = dsp_mgr_register_cback(audio_pipe_dsp_cback);

    return true;
}

void audio_pipe_deinit(void)
{
    T_AUDIO_PIPE *audio_pipe;
    T_AUDIO_PIPE *p_next;

    if (audio_pipe_db != NULL)
    {
        dsp_mgr_unregister_cback(audio_pipe_db->dsp_event);

        audio_pipe = os_queue_peek(&audio_pipe_db->pipes, 0);
        while (audio_pipe)
        {
            p_next = audio_pipe->p_next;

            os_queue_delete(&audio_pipe_db->pipes, audio_pipe);
            os_mem_free(audio_pipe);

            audio_pipe = p_next;
        }

        audio_pipe_db = NULL;
    }
}

void audio_pipe_dsp_session_cb(void *handle, T_DSP_MGR_EVENT event, uint32_t param)
{
    T_AUDIO_PIPE *audio_pipe;

    audio_pipe = (T_AUDIO_PIPE *)handle;

    if (audio_pipe == NULL)
    {
        return;
    }

#if (AUDIO_CODEC_DEBUG == 0)
    if (event != DSP_MGR_EVT_REQ_DATA && event != DSP_MGR_EVT_DATA_IND)
#endif
    {
        AUDIO_PRINT_INFO2("audio_pipe_dsp_session_cb: handle %p, event 0x%x",
                          audio_pipe, event);
    }

    switch (event)
    {
    case DSP_MGR_EVT_PREPARE_READY:
        {
            dsp_mgr_session_run(audio_pipe->dsp_session);

            ///TODO: move to dsp_mgr
            dipc_codec_pipe_create((uint32_t)audio_pipe->dsp_session,
                                   (uint8_t)audio_pipe->mode,
                                   audio_pipe->decoder.coder_id,
                                   audio_pipe->decoder.frame_num,
                                   audio_pipe->decoder.format_size,
                                   (uint8_t *)(&(audio_pipe->decoder.format)),
                                   audio_pipe->encoder.coder_id,
                                   audio_pipe->encoder.frame_num,
                                   audio_pipe->encoder.format_size,
                                   (uint8_t *)(&(audio_pipe->encoder.format)));
        }
        break;

    case DSP_MGR_EVT_CODEC_PIPE_CREATE:
        {
            T_DIPC_CODEC_PIPE_CREATE_CMPL *p_info = (T_DIPC_CODEC_PIPE_CREATE_CMPL *)param;

            if (p_info->status == DIPC_ERROR_SUCCESS)
            {
                audio_pipe->src_transport_address = p_info->src_transport_address;
                audio_pipe->src_transport_size = p_info->src_transport_size;
                audio_pipe->snk_transport_address = p_info->snk_transport_address;
                audio_pipe->snk_transport_size = p_info->snk_transport_size;

                audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_CREATED);
            }
            else
            {
                audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_RELEASED);
            }
        }
        break;

    case DSP_MGR_EVT_CODEC_PIPE_DESTROY:
        {
            audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_RELEASED);
        }
        break;

    case DSP_MGR_EVT_CODEC_PIPE_START:
        {
            audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_STARTED);
        }
        break;

    case DSP_MGR_EVT_CODEC_PIPE_STOP:
        {
            audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_STOPPED);
        }
        break;

    case DSP_MGR_EVT_CODEC_PIPE_MIXED:
        {
            audio_pipe->cback(audio_pipe, AUDIO_PIPE_EVENT_MIXED, 0);
        }
        break;

    case DSP_MGR_EVT_CODEC_PIPE_DEMIXED:
        {
            audio_pipe->cback(audio_pipe, AUDIO_PIPE_EVENT_DEMIXED, 0);
        }
        break;

    case DSP_MGR_EVT_REQ_DATA:
        if (audio_pipe->state == AUDIO_PIPE_STATE_STARTED)
        {
            audio_pipe->credits = 1;
            audio_pipe->cback(audio_pipe, AUDIO_PIPE_EVENT_DATA_FILLED, 0);
        }
        break;

    case DSP_MGR_EVT_DATA_IND:
        if (audio_pipe->state == AUDIO_PIPE_STATE_STARTED)
        {
            audio_pipe->cback(audio_pipe, AUDIO_PIPE_EVENT_DATA_IND, 0);
        }
        break;

    default:
        break;
    }
}

T_AUDIO_PIPE_HANDLE audio_pipe_create(T_AUDIO_STREAM_MODE mode,
                                      T_AUDIO_FORMAT_INFO src_info,
                                      T_AUDIO_FORMAT_INFO snk_info,
                                      uint16_t            gain,
                                      P_AUDIO_PIPE_CBACK  cback)
{
    T_AUDIO_PIPE *audio_pipe;
    T_DSP_SESSION_CFG dsp_cfg;
    int32_t ret = 0;

    if (audio_pipe_db == NULL)
    {
        ret = 1;
        goto fail_check_param;
    }

    if (cback == NULL)
    {
        ret = 2;
        goto fail_check_cback;
    }

    audio_pipe = os_mem_zalloc2(sizeof(T_AUDIO_PIPE));
    if (audio_pipe == NULL)
    {
        ret = 3;
        goto fail_alloc_pipe;
    }

    audio_pipe->mode = mode;
    audio_pipe->cback = cback;
    audio_pipe->state = AUDIO_PIPE_STATE_RELEASED;
    audio_pipe->action = AUDIO_PIPE_ACTION_NONE;
    audio_pipe->gain_left = gain;
    audio_pipe->gain_right = gain;
    audio_pipe->credits = 1;
    audio_pipe->decoder.frame_num = src_info.frame_num;
    audio_pipe->encoder.frame_num = snk_info.frame_num;

    format_init[src_info.type](&audio_pipe->decoder, &src_info.attr);
    format_init[snk_info.type](&audio_pipe->encoder, &snk_info.attr);

    dsp_cfg.context = audio_pipe;
    dsp_cfg.callback = audio_pipe_dsp_session_cb;
    audio_pipe->dsp_session = dsp_mgr_session_create(DSP_SESSION_TYPE_PIPE, dsp_cfg);

    if (audio_pipe->dsp_session == NULL)
    {
        ret = 4;
        goto fail_alloc_dsp_session;
    }

    os_queue_in(&audio_pipe_db->pipes, audio_pipe);

    AUDIO_PRINT_INFO6("audio_pipe_create: pipe_session %p, dsp_session %p, src type 0x%x, snk type 0x%x "
                      "decoder_frame_num %d, encoder_frame_num %d",
                      audio_pipe, audio_pipe->dsp_session, src_info.type, snk_info.type,
                      audio_pipe->decoder.frame_num, audio_pipe->encoder.frame_num);

    audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_CREATING);

    return audio_pipe;

fail_alloc_dsp_session:
    os_mem_free(audio_pipe);
fail_alloc_pipe:
fail_check_cback:
fail_check_param:
    AUDIO_PRINT_ERROR1("audio_pipe_create: failed %d", -ret);
    return NULL;
}

bool audio_pipe_release(T_AUDIO_PIPE_HANDLE handle)
{
    T_AUDIO_PIPE *audio_pipe;
    T_AUDIO_PIPE *item;
    uint8_t       i;
    int32_t       ret = 0;

    audio_pipe = (T_AUDIO_PIPE *)handle;
    if (os_queue_search(&audio_pipe_db->pipes, audio_pipe) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    AUDIO_PRINT_TRACE3("audio_pipe_release: handle %p, state %d, action %d",
                       audio_pipe, audio_pipe->state, audio_pipe->action);

    if (audio_pipe->mixed_pipe_num > 0)
    {
        ret = 2;
        goto fail_check_mixed_pipe;
    }

    item = os_queue_peek(&audio_pipe_db->pipes, 0);
    while (item)
    {
        for (i = 0; i < MAX_MIXED_PIPE_NUM; i++)
        {
            if (item->mixed_pipes[i] == handle)
            {
                item->mixed_pipe_num--;
                item->mixed_pipes[i] = 0;
                break;
            }
        }

        item = item->p_next;
    }

    if (audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_RELEASING) == false)
    {
        ret = 3;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_check_mixed_pipe:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_pipe_release: handle %p, failed %d", audio_pipe, -ret);
    return false;
}

bool audio_pipe_start(T_AUDIO_PIPE_HANDLE handle)
{
    T_AUDIO_PIPE *audio_pipe;
    int32_t       ret = 0;

    audio_pipe = (T_AUDIO_PIPE *)handle;
    if (os_queue_search(&audio_pipe_db->pipes, audio_pipe) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    AUDIO_PRINT_TRACE3("audio_pipe_start: handle %p, state %d, action %d",
                       audio_pipe, audio_pipe->state, audio_pipe->action);

    if (audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_STARTING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_pipe_start: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_pipe_stop(T_AUDIO_PIPE_HANDLE handle)
{
    T_AUDIO_PIPE *audio_pipe;
    int32_t       ret = 0;

    audio_pipe = (T_AUDIO_PIPE *)handle;
    if (os_queue_search(&audio_pipe_db->pipes, audio_pipe) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    AUDIO_PRINT_TRACE3("audio_pipe_stop: handle %p, state %d, action %d",
                       audio_pipe, audio_pipe->state, audio_pipe->action);

    if (audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_STOPPING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_pipe_stop: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_pipe_gain_get(T_AUDIO_PIPE_HANDLE handle, uint16_t *gain_left, uint16_t *gain_right)
{
    T_AUDIO_PIPE *audio_pipe = (T_AUDIO_PIPE *)handle;

    if (os_queue_search(&audio_pipe_db->pipes, audio_pipe) == false)
    {
        return false;
    }

    *gain_left = audio_pipe->gain_left;
    *gain_right = audio_pipe->gain_right;

    return true;
}

bool audio_pipe_gain_set(T_AUDIO_PIPE_HANDLE handle, uint16_t gain_left, uint16_t gain_right)
{
    T_AUDIO_PIPE *audio_pipe = (T_AUDIO_PIPE *)handle;
    int32_t       ret = 0;

    if (os_queue_search(&audio_pipe_db->pipes, audio_pipe) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (audio_pipe->state == AUDIO_PIPE_STATE_STARTED)
    {
        if (dipc_codec_pipe_gain_set((uint32_t)audio_pipe->dsp_session,
                                     gain_left,
                                     gain_right) == false)
        {
            ret = 2;
            goto fail_set_gain;
        }
    }

    audio_pipe->gain_left = gain_left;
    audio_pipe->gain_right = gain_right;
    return true;

fail_set_gain:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_pipe_gain_set: handle %p, failed %d", handle, -ret);
    return false;
}

RAM_TEXT_SECTION
bool audio_pipe_fill_in_isr(T_AUDIO_PIPE_HANDLE    handle,
                            uint32_t               timestamp,
                            uint16_t               seq_num,
                            T_AUDIO_STREAM_STATUS  status,
                            uint8_t                frame_num,
                            void                  *buf,
                            uint16_t               len)
{
    T_AUDIO_PIPE       *audio_pipe = (T_AUDIO_PIPE *)handle;
    T_DIPC_DATA_HEADER *header = (T_DIPC_DATA_HEADER *)audio_pipe_ipc_buf;
    int32_t             ret = 0;

    if (header == NULL)
    {
        ret = 1;
        goto fail_check_buf;
    }

    if (audio_pipe->state != AUDIO_PIPE_STATE_STARTED)
    {
        ret = 2;
        goto fail_invalid_state;
    }

    if (audio_pipe->mode != AUDIO_STREAM_MODE_DIRECT)
    {
        if (audio_pipe->credits == 0)
        {
            ret = 3;
            goto fail_no_credits;
        }
    }

    if (len > (AUDIO_PIPE_IPC_BUF_SIZE - sizeof(T_DIPC_DATA_HEADER)))
    {
        ret = 4;
        goto fail_no_credits;
    }

    header->sync_word = DIPC_SYNC_WORD;
    header->session_id = (uint32_t)audio_pipe->dsp_session;
    header->timestamp = timestamp;
    header->seq_num = seq_num;
    header->frame_count = 0;  ///TODO
    header->frame_num = frame_num;
    header->status = status;
    header->payload_length = len;
    header->tail = DIPC_TAIL;

    if (buf != NULL)
    {
        memcpy((uint8_t *)header + sizeof(T_DIPC_DATA_HEADER), buf, len);
    }

    if (h2d_data_send2((void *)audio_pipe->src_transport_address,
                       0,
                       (uint8_t *)header,
                       len + sizeof(T_DIPC_DATA_HEADER),
                       true,
                       0) == 0)
    {
        ret = 5;
        goto fail_fill_data;
    }

    audio_pipe->credits = 0;

    return true;

fail_fill_data:
fail_no_credits:
fail_invalid_state:
fail_check_buf:
    AUDIO_PRINT_ERROR2("audio_pipe_fill: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_pipe_fill(T_AUDIO_PIPE_HANDLE    handle,
                     uint32_t               timestamp,
                     uint16_t               seq_num,
                     T_AUDIO_STREAM_STATUS  status,
                     uint8_t                frame_num,
                     void                  *buf,
                     uint16_t               len)
{
    T_AUDIO_PIPE       *audio_pipe = (T_AUDIO_PIPE *)handle;
    T_DIPC_DATA_HEADER *header;
    int32_t             ret = 0;

    if (os_queue_search(&audio_pipe_db->pipes, audio_pipe) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (audio_pipe->state != AUDIO_PIPE_STATE_STARTED)
    {
        ret = 2;
        goto fail_invalid_state;
    }

    if (audio_pipe->mode != AUDIO_STREAM_MODE_DIRECT)
    {
        if (audio_pipe->credits == 0)
        {
            ret = 3;
            goto fail_no_credits;
        }
    }

    header = (T_DIPC_DATA_HEADER *)os_mem_alloc2(len + sizeof(T_DIPC_DATA_HEADER));
    if (header == NULL)
    {
        ret = 4;
        goto fail_alloc_mem;
    }

    header->sync_word = DIPC_SYNC_WORD;
    header->session_id = (uint32_t)audio_pipe->dsp_session;
    header->timestamp = timestamp;
    header->seq_num = seq_num;
    header->frame_count = 0;  ///TODO
    header->frame_num = frame_num;
    header->status = status;
    header->payload_length = len;
    header->tail = DIPC_TAIL;

    if (buf != NULL)
    {
        memcpy((uint8_t *)header + sizeof(T_DIPC_DATA_HEADER), buf, len);
    }

    if (h2d_data_send2((void *)audio_pipe->src_transport_address,
                       0,
                       (uint8_t *)header,
                       len + sizeof(T_DIPC_DATA_HEADER),
                       true,
                       0) == 0)
    {
        ret = 5;
        goto fail_fill_data;
    }

#if (AUDIO_CODEC_DEBUG == 1)
    AUDIO_PRINT_INFO3("audio_pipe_fill: pipe_session: %p, seq_num: 0x%x, len: 0x%x",
                      audio_pipe, seq_num, len);
#endif

    audio_pipe->credits = 0;
    os_mem_free(header);

    return true;

fail_fill_data:
    os_mem_free(header);
fail_alloc_mem:
fail_no_credits:
fail_invalid_state:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_pipe_fill: handle %p, failed %d", handle, -ret);
    return false;
}

RAM_TEXT_SECTION
bool audio_pipe_direct_drain(uint32_t snk_transport_address,
                             uint32_t              *timestamp,
                             uint16_t              *seq_num,
                             T_AUDIO_STREAM_STATUS *status,
                             uint8_t               *frame_num,
                             void                  *buf,
                             uint16_t              *len)
{
    T_DIPC_DATA_HEADER  header;
    uint16_t            peek_len;
    uint16_t            recv_len;
    uint8_t             recv_type;
    int32_t             ret = 0;

    if (snk_transport_address == 0 || audio_pipe_peek_buf == NULL)
    {
        ret = 1;
        goto fail_check_buf;
    }

    peek_len = d2h_data_length_peek2((void *)snk_transport_address);

    if (peek_len == 0)
    {
        ret = 2;
        goto fail_drain_data;
    }

    if (peek_len > AUDIO_PIPE_PEEK_BUF_SIZE)
    {
        ret = 3;
        goto fail_drain_data;
    }

    recv_len = d2h_data_recv2((void *)snk_transport_address, &recv_type,
                              audio_pipe_peek_buf, peek_len);
    if (recv_len == 0)
    {
        ret = 4;
        goto fail_drain_data;
    }

    memcpy((void *)&header, audio_pipe_peek_buf, sizeof(T_DIPC_DATA_HEADER));
    recv_len = recv_len - sizeof(T_DIPC_DATA_HEADER);
    *timestamp = header.timestamp;
    *seq_num = header.seq_num;
    *status = (T_AUDIO_STREAM_STATUS)header.status;
    *frame_num = header.frame_num;
    *len = recv_len;
    memcpy(buf, audio_pipe_peek_buf + sizeof(T_DIPC_DATA_HEADER), recv_len);

    return true;

fail_drain_data:
fail_check_buf:
    APP_PRINT_ERROR1("audio_pipe_direct_drain: failed -%d", ret);

    return false;
}

bool audio_pipe_drain(T_AUDIO_PIPE_HANDLE    handle,
                      uint32_t              *timestamp,
                      uint16_t              *seq_num,
                      T_AUDIO_STREAM_STATUS *status,
                      uint8_t               *frame_num,
                      void                  *buf,
                      uint16_t              *len)
{
    T_AUDIO_PIPE       *audio_pipe = (T_AUDIO_PIPE *)handle;
    T_DIPC_DATA_HEADER  header;
    uint8_t            *peek_buf;
    uint16_t            peek_len;
    uint16_t            recv_len;
    uint8_t             recv_type;
    int32_t             ret = 0;

    if (os_queue_search(&audio_pipe_db->pipes, audio_pipe) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (audio_pipe->state != AUDIO_PIPE_STATE_STARTED)
    {
        ret = 2;
        goto fail_invalid_state;
    }

    peek_len = d2h_data_length_peek2((void *)audio_pipe->snk_transport_address);
    peek_buf = os_mem_alloc2(peek_len);
    if (peek_buf == NULL)
    {
        ret = 3;
        goto fail_alloc_mem;
    }

    recv_len = d2h_data_recv2((void *)audio_pipe->snk_transport_address, &recv_type,
                              peek_buf, peek_len);
    if (recv_len == 0)
    {
        ret = 4;
        goto fail_drain_data;
    }

    memcpy((void *)&header, peek_buf, sizeof(T_DIPC_DATA_HEADER));
    recv_len = recv_len - sizeof(T_DIPC_DATA_HEADER);
    *timestamp = header.timestamp;
    *seq_num = header.seq_num;
    *status = (T_AUDIO_STREAM_STATUS)header.status;
    *frame_num = header.frame_num;
    *len = recv_len;
    memcpy(buf, peek_buf + sizeof(T_DIPC_DATA_HEADER), recv_len);
    os_mem_free(peek_buf);

#if (AUDIO_CODEC_DEBUG == 1)
    AUDIO_PRINT_INFO3("audio_pipe_drain: pipe_session: %p, recv_len: 0x%x, seq_num: 0x%x",
                      audio_pipe, recv_len, header.seq_num);
#endif

    return true;

fail_drain_data:
    os_mem_free(peek_buf);
fail_alloc_mem:
fail_invalid_state:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_pipe_drain: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_pipe_pre_mix(T_AUDIO_PIPE_HANDLE prime_handle, T_AUDIO_PIPE_HANDLE auxiliary_handle)
{
    uint8_t       i;
    T_AUDIO_PIPE *prime_pipe = (T_AUDIO_PIPE *)prime_handle;
    T_AUDIO_PIPE *auxiliary_pipe = (T_AUDIO_PIPE *)auxiliary_handle;

    if (os_queue_search(&audio_pipe_db->pipes, prime_pipe) == false)
    {
        return false;
    }

    if (os_queue_search(&audio_pipe_db->pipes, auxiliary_pipe) == false)
    {
        return false;
    }

    if (prime_pipe->mixed_pipe_num < MAX_MIXED_PIPE_NUM)
    {
        for (i = 0; i < MAX_MIXED_PIPE_NUM; i++)
        {
            if (prime_pipe->mixed_pipes[i] == 0)
            {
                prime_pipe->mixed_pipe_num++;
                prime_pipe->mixed_pipes[i] = auxiliary_handle;
                break;
            }
        }

        AUDIO_PRINT_INFO2("audio_pipe_pre_mix: prime %p, auxiliary %p", prime_pipe, auxiliary_pipe);

        return dipc_codec_pipe_pre_mixer_add((uint32_t)prime_pipe->dsp_session,
                                             (uint32_t)auxiliary_pipe->dsp_session);
    }

    return false;
}

bool audio_pipe_post_mix(T_AUDIO_PIPE_HANDLE prime_handle, T_AUDIO_PIPE_HANDLE auxiliary_handle)
{
    uint8_t       i;
    T_AUDIO_PIPE *prime_pipe = (T_AUDIO_PIPE *)prime_handle;
    T_AUDIO_PIPE *auxiliary_pipe = (T_AUDIO_PIPE *)auxiliary_handle;

    if (os_queue_search(&audio_pipe_db->pipes, prime_pipe) == false)
    {
        return false;
    }

    if (os_queue_search(&audio_pipe_db->pipes, auxiliary_pipe) == false)
    {
        return false;
    }

    if (prime_pipe->mixed_pipe_num < MAX_MIXED_PIPE_NUM)
    {
        for (i = 0; i < MAX_MIXED_PIPE_NUM; i++)
        {
            if (prime_pipe->mixed_pipes[i] == 0)
            {
                prime_pipe->mixed_pipe_num++;
                prime_pipe->mixed_pipes[i] = auxiliary_handle;
                break;
            }
        }

        AUDIO_PRINT_INFO2("audio_pipe_post_mix: prime %p, auxiliary %p", prime_pipe, auxiliary_pipe);

        return dipc_codec_pipe_post_mixer_add((uint32_t)prime_pipe->dsp_session,
                                              (uint32_t)auxiliary_pipe->dsp_session);
    }

    return false;
}

bool audio_pipe_demix(T_AUDIO_PIPE_HANDLE prime_handle, T_AUDIO_PIPE_HANDLE auxiliary_handle)
{
    uint8_t       i;
    T_AUDIO_PIPE *prime_pipe = (T_AUDIO_PIPE *)prime_handle;
    T_AUDIO_PIPE *auxiliary_pipe = (T_AUDIO_PIPE *)auxiliary_handle;

    if (os_queue_search(&audio_pipe_db->pipes, prime_pipe) == false)
    {
        return false;
    }

    if (os_queue_search(&audio_pipe_db->pipes, auxiliary_pipe) == false)
    {
        return false;
    }

    for (i = 0; i < MAX_MIXED_PIPE_NUM; i++)
    {
        if (prime_pipe->mixed_pipes[i] == auxiliary_handle)
        {
            prime_pipe->mixed_pipe_num--;
            prime_pipe->mixed_pipes[i] = 0;
            break;
        }
    }

    AUDIO_PRINT_INFO2("audio_pipe_demix: prime %p, auxiliary %p", prime_pipe, auxiliary_pipe);

    return dipc_codec_pipe_mixer_remove((uint32_t)prime_pipe->dsp_session,
                                        (uint32_t)auxiliary_pipe->dsp_session);
}

bool audio_pipe_asrc_set(T_AUDIO_PIPE_HANDLE handle,
                         int32_t             ratio)
{
    T_AUDIO_PIPE *pipe = (T_AUDIO_PIPE *)handle;
    int32_t       ret = 0;

    if (os_queue_search(&audio_pipe_db->pipes, pipe) == false)
    {
        ret = 1;
        goto fail_check_pipe;
    }

    if (pipe->state != AUDIO_PIPE_STATE_STARTED)
    {
        ret = 2;
        goto fail_check_pipe_state;
    }

    if (dipc_pipe_asrc_set((uint32_t)pipe->dsp_session, ratio) == false)
    {
        ret = 3;
        goto fail_set_asrc;
    }

    return true;

fail_set_asrc:
fail_check_pipe_state:
fail_check_pipe:
    AUDIO_PRINT_ERROR1("audio_pipe_asrc_set: failed %d", -ret);
    return false;
}

static bool audio_pipe_state_set(T_AUDIO_PIPE *audio_pipe, T_AUDIO_PIPE_STATE state)
{
    bool ret = false;

    AUDIO_PRINT_INFO4("audio_pipe_state_set: handle %p, curr_state 0x%02x, next_state 0x%02x, action 0x%02x",
                      audio_pipe, audio_pipe->state, state, audio_pipe->action);

    switch (audio_pipe->state)
    {
    case AUDIO_PIPE_STATE_RELEASED:
        if (state == AUDIO_PIPE_STATE_CREATING)
        {
            audio_pipe->state = state;

            if (dsp_mgr_is_stable(audio_pipe->dsp_session) == true)
            {
                if (dsp_mgr_power_on_check() == false)
                {
                    dsp_mgr_power_on();
                }

                dsp_mgr_session_enable(audio_pipe->dsp_session);
            }
            else
            {
                if (dsp_mgr_get_state() == DSP_STATE_LOADER)
                {
                    dsp_mgr_session_enable(audio_pipe->dsp_session);
                }
                else if (dsp_mgr_get_state() == DSP_STATE_WAIT_OFF)
                {
                    //wait for DSP_MGR_EVT_POWER_OFF
                    audio_pipe->state = AUDIO_PIPE_STATE_RELEASED;
                }
            }

            ret = true;
        }
        break;

    case AUDIO_PIPE_STATE_CREATING:
        if (state == AUDIO_PIPE_STATE_CREATED)
        {
            audio_pipe->state = state;
            audio_pipe->cback(audio_pipe, AUDIO_PIPE_EVENT_CREATED, AUDIO_CODEC_SNK_BUF_SIZE);
            audio_pipe->cback(audio_pipe, AUDIO_PIPE_EVENT_CREATED_ADDR, audio_pipe->snk_transport_address);

            if (audio_pipe_db->pipes.count == 1)
            {
                audio_path_lpm_set(false);
            }

            if (audio_pipe->action == AUDIO_PIPE_ACTION_NONE)
            {
                ret = true;
            }
            else if (audio_pipe->action == AUDIO_PIPE_ACTION_START)
            {
                audio_pipe->action = AUDIO_PIPE_ACTION_NONE;
                ret = audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_STARTING);
            }
            else if (audio_pipe->action == AUDIO_PIPE_ACTION_RELEASE)
            {
                audio_pipe->action = AUDIO_PIPE_ACTION_NONE;
                ret = audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_RELEASING);
            }
        }
        else if (state == AUDIO_PIPE_STATE_STARTING)
        {
            audio_pipe->action = AUDIO_PIPE_ACTION_START;
            ret = true;
        }
        else if (state == AUDIO_PIPE_STATE_RELEASING)
        {
            audio_pipe->action = AUDIO_PIPE_ACTION_RELEASE;
            ret = true;
        }
        else if (state == AUDIO_PIPE_STATE_RELEASED)
        {
            /* pipe create fail, release resource. */
            ret = dsp_mgr_session_destroy(audio_pipe->dsp_session);

            os_queue_delete(&audio_pipe_db->pipes, audio_pipe);
            os_mem_free(audio_pipe);
        }
        break;

    case AUDIO_PIPE_STATE_CREATED:
    case AUDIO_PIPE_STATE_STOPPED:
        if (state == AUDIO_PIPE_STATE_STARTING)
        {
            ret = dipc_codec_pipe_start((uint32_t)audio_pipe->dsp_session);
            audio_pipe->state = state;
        }
        else if (state == AUDIO_PIPE_STATE_RELEASING)
        {
            ret = dipc_codec_pipe_destroy((uint32_t)audio_pipe->dsp_session);
            audio_pipe->state = state;
        }
        break;

    case AUDIO_PIPE_STATE_STARTING:
        if (state == AUDIO_PIPE_STATE_STARTED)
        {
            dipc_codec_pipe_gain_set((uint32_t)audio_pipe->dsp_session,
                                     audio_pipe->gain_left,
                                     audio_pipe->gain_right);

            audio_pipe->state = state;
            audio_pipe->cback(audio_pipe, AUDIO_PIPE_EVENT_STARTED, 0);

            if (audio_pipe->action == AUDIO_PIPE_ACTION_NONE)
            {
                ret = true;
            }
            else if (audio_pipe->action == AUDIO_PIPE_ACTION_STOP)
            {
                audio_pipe->action = AUDIO_PIPE_ACTION_NONE;
                ret = audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_STOPPING);
            }
            else if (audio_pipe->action == AUDIO_PIPE_ACTION_RELEASE)
            {
                audio_pipe->action = AUDIO_PIPE_ACTION_NONE;
                ret = audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_RELEASING);
            }
        }
        else if (state == AUDIO_PIPE_STATE_STOPPING)
        {
            audio_pipe->action = AUDIO_PIPE_ACTION_STOP;
            ret = true;
        }
        else if (state == AUDIO_PIPE_STATE_RELEASING)
        {
            audio_pipe->action = AUDIO_PIPE_ACTION_RELEASE;
            ret = true;
        }
        break;

    case AUDIO_PIPE_STATE_STARTED:
        if (state == AUDIO_PIPE_STATE_STOPPING)
        {
            ret = dipc_codec_pipe_stop((uint32_t)audio_pipe->dsp_session);
            audio_pipe->state = state;
        }
        else if (state == AUDIO_PIPE_STATE_RELEASING)
        {
            /* Started audio pipe cannot be released directly, so it should be
             * stopped first before released.
             */
            audio_pipe->state  = AUDIO_PIPE_STATE_STOPPING;
            audio_pipe->action = AUDIO_PIPE_ACTION_RELEASE;

            ret = dipc_codec_pipe_stop((uint32_t)audio_pipe->dsp_session);
            if (ret == false)
            {
                /* Restore the pipe state. */
                audio_pipe->state  = AUDIO_PIPE_STATE_STARTED;
                audio_pipe->action = AUDIO_PIPE_ACTION_NONE;
            }
        }
        break;

    case AUDIO_PIPE_STATE_STOPPING:
        if (state == AUDIO_PIPE_STATE_STOPPED)
        {
            audio_pipe->state = state;
            audio_pipe->cback(audio_pipe, AUDIO_PIPE_EVENT_STOPPED, 0);

            if (audio_pipe->action == AUDIO_PIPE_ACTION_NONE)
            {
                ret = true;
            }
            else if (audio_pipe->action == AUDIO_PIPE_ACTION_START)
            {
                audio_pipe->action = AUDIO_PIPE_ACTION_NONE;
                ret = audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_STARTING);
            }
            else if (audio_pipe->action == AUDIO_PIPE_ACTION_RELEASE)
            {
                audio_pipe->action = AUDIO_PIPE_ACTION_NONE;
                ret = audio_pipe_state_set(audio_pipe, AUDIO_PIPE_STATE_RELEASING);
            }
        }
        else if (state == AUDIO_PIPE_STATE_STARTING)
        {
            audio_pipe->action = AUDIO_PIPE_ACTION_START;
            ret = true;
        }
        else if (state == AUDIO_PIPE_STATE_RELEASING)
        {
            audio_pipe->action = AUDIO_PIPE_ACTION_RELEASE;
            ret = true;
        }
        break;

    case AUDIO_PIPE_STATE_RELEASING:
        if (state == AUDIO_PIPE_STATE_RELEASED)
        {
            audio_pipe->cback(audio_pipe, AUDIO_PIPE_EVENT_RELEASED, 0);

            ret = dsp_mgr_session_destroy(audio_pipe->dsp_session);

            os_queue_delete(&audio_pipe_db->pipes, audio_pipe);
            os_mem_free(audio_pipe);

            if (audio_pipe_db->pipes.count == 0)
            {
                audio_path_lpm_set(true);
            }
        }
        break;

    default:
        break;
    }

    return ret;
}

bool audio_pipe_buf_init(void)
{
    uint8_t ret = 0;

    audio_pipe_ipc_buf = os_mem_alloc2(AUDIO_PIPE_IPC_BUF_SIZE);
    if (audio_pipe_ipc_buf == NULL)
    {
        ret = 1;
        goto fail_alloc_ipc_buf;
    }

    audio_pipe_peek_buf = os_mem_alloc2(AUDIO_PIPE_PEEK_BUF_SIZE);
    if (audio_pipe_peek_buf == NULL)
    {
        ret = 2;
        goto fail_alloc_peek_buf;
    }

    return true;

fail_alloc_peek_buf:
    os_mem_free(audio_pipe_ipc_buf);
fail_alloc_ipc_buf:

    APP_PRINT_ERROR1("audio_pipe_buf_init: failed %d", -ret);

    return false;
}

#else
#include <stdint.h>
#include <stddef.h>
#include "audio_pipe.h"
bool audio_pipe_init(void)
{
    return true;
}

void audio_pipe_deinit(void)
{

}

T_AUDIO_PIPE_HANDLE audio_pipe_create(T_AUDIO_STREAM_MODE mode,
                                      T_AUDIO_FORMAT_INFO src_info,
                                      T_AUDIO_FORMAT_INFO snk_info,
                                      uint16_t            gain,
                                      P_AUDIO_PIPE_CBACK  cback)
{
    return NULL;
}

bool audio_pipe_release(T_AUDIO_PIPE_HANDLE handle)
{
    return false;
}

bool audio_pipe_start(T_AUDIO_PIPE_HANDLE handle)
{
    return false;
}

bool audio_pipe_stop(T_AUDIO_PIPE_HANDLE handle)
{
    return false;
}

bool audio_pipe_gain_get(T_AUDIO_PIPE_HANDLE handle, uint16_t *gain_left, uint16_t *gain_right)
{
    return false;
}

bool audio_pipe_gain_set(T_AUDIO_PIPE_HANDLE handle, uint16_t gain_left, uint16_t gain_right)
{
    return false;
}

bool audio_pipe_fill(T_AUDIO_PIPE_HANDLE    handle,
                     uint32_t               timestamp,
                     uint16_t               seq_num,
                     T_AUDIO_STREAM_STATUS  status,
                     uint8_t                frame_num,
                     void                  *buf,
                     uint16_t               len)
{
    return false;
}

bool audio_pipe_drain(T_AUDIO_PIPE_HANDLE    handle,
                      uint32_t              *timestamp,
                      uint16_t              *seq_num,
                      T_AUDIO_STREAM_STATUS *status,
                      uint8_t               *frame_num,
                      void                  *buf,
                      uint16_t              *len)
{
    return false;
}

bool audio_pipe_pre_mix(T_AUDIO_PIPE_HANDLE prime_handle, T_AUDIO_PIPE_HANDLE auxiliary_handle)
{
    return false;
}

bool audio_pipe_post_mix(T_AUDIO_PIPE_HANDLE prime_handle, T_AUDIO_PIPE_HANDLE auxiliary_handle)
{
    return false;
}

bool audio_pipe_demix(T_AUDIO_PIPE_HANDLE prime_handle, T_AUDIO_PIPE_HANDLE auxiliary_handle)
{
    return false;
}

bool audio_pipe_asrc_set(T_AUDIO_PIPE_HANDLE handle,
                         int32_t             ratio)
{
    return false;
}
#endif
