#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "audio_pipe.h"
#include "app_audio_pipe_mgr.h"

#define MIXED_PIPE_NUM     2

typedef struct
{
    bool                   new_codec;
    bool                   pipe_no_auto_start;
    bool                   handle_data_in_isr;
    bool                   asrc_toggle;
    bool                   is_mixed_pipe;
    T_GAMING_CODEC         src_type;
    T_GAMING_CODEC         snk_type;
    T_APP_PIPE_STATE       state;
    T_PIPE_FORMAT_INFO     format;
    T_AUDIO_PIPE_HANDLE    handle;
    T_AUDIO_STREAM_MODE    mode;
    uint16_t               gain;
    uint16_t               seq;
    uint16_t               src_frame_size;
    uint16_t               snk_frame_size;
    uint32_t               snk_transport_address;
    int32_t                asrc_ratio;
    P_AUDIO_PIPE_CBACK     pipe_cback;
    T_APP_PIPE_PARAM       param;
    T_APP_PIPE_PARAM       next_param;
} T_AUDIO_PIPE_INFO;

static bool downstream_pipe_cb(T_AUDIO_PIPE_HANDLE handle,
                               T_AUDIO_PIPE_EVENT  event,
                               uint32_t            param);

static bool upstream_pipe_cb(T_AUDIO_PIPE_HANDLE handle,
                             T_AUDIO_PIPE_EVENT  event,
                             uint32_t            param);

static const T_APP_PIPE_EVENT pipe_event_map[] =
{
    [AUDIO_PIPE_EVENT_RELEASED] = PIPE_EVENT_RELEASED,
    [AUDIO_PIPE_EVENT_CREATED]  = PIPE_EVENT_CREATED,
    [AUDIO_PIPE_EVENT_STARTED]  = PIPE_EVENT_STARTED,
    [AUDIO_PIPE_EVENT_DATA_FILLED] = PIPE_EVENT_FILLED,
};

static const T_AUDIO_FORMAT_INFO codec_info[] =
{
    [PCM_16K_16BIT_MONO_10MS] = {
        .type         = AUDIO_FORMAT_TYPE_PCM,
        .frame_num    = 1,
        .attr.pcm.sample_rate  = 16000,
        .attr.pcm.frame_length = PCM_16K_16BIT_MONO_10MS_FRAME_SIZE,
        .attr.pcm.chann_num    = 1,
        .attr.pcm.bit_width    = 16,
    },

    [PCM_48K_16BIT_MONO_10MS] = {
        .type         = AUDIO_FORMAT_TYPE_PCM,
        .frame_num    = 1,
        .attr.pcm.sample_rate = 48000,
        .attr.pcm.frame_length = PCM_48K_16BIT_MONO_10MS_FRAME_SIZE,
        .attr.pcm.chann_num   = 1,
        .attr.pcm.bit_width   = 16,
    },

    [PCM_48K_16BIT_STEREO_0_5MS] = {
        .type         = AUDIO_FORMAT_TYPE_PCM,
        .frame_num    = 1,
        .attr.pcm.sample_rate = 48000,
        .attr.pcm.frame_length = PCM_48K_16BIT_MONO_0_5MS_FRAME_SIZE,
        .attr.pcm.chann_num   = 2,
        .attr.pcm.bit_width   = 16,
    },

    [PCM_48K_16BIT_STEREO_1MS] = {
        .type         = AUDIO_FORMAT_TYPE_PCM,
        .frame_num    = 1,
        .attr.pcm.sample_rate = 48000,
        .attr.pcm.frame_length = PCM_48K_16BIT_MONO_1MS_FRAME_SIZE,
        .attr.pcm.chann_num   = 2,
        .attr.pcm.bit_width   = 16,
    },

    [PCM_48K_16BIT_STEREO_2MS] = {
        .type         = AUDIO_FORMAT_TYPE_PCM,
        .frame_num    = 1,
        .attr.pcm.sample_rate = 48000,
        .attr.pcm.frame_length = PCM_48K_16BIT_MONO_2MS_FRAME_SIZE,
        .attr.pcm.chann_num   = 2,
        .attr.pcm.bit_width   = 16,
    },

    [PCM_48K_24BIT_STEREO_0_5MS] = {
        .type         = AUDIO_FORMAT_TYPE_PCM,
        .frame_num    = 1,
        .attr.pcm.sample_rate = 48000,
        .attr.pcm.frame_length = PCM_48K_24BIT_MONO_0_5MS_FRAME_SIZE,
        .attr.pcm.chann_num   = 2,
        .attr.pcm.bit_width   = 24,
    },

    [PCM_48K_24BIT_STEREO_1MS] = {
        .type         = AUDIO_FORMAT_TYPE_PCM,
        .frame_num    = 1,
        .attr.pcm.sample_rate = 48000,
        .attr.pcm.frame_length = PCM_48K_24BIT_MONO_1MS_FRAME_SIZE,
        .attr.pcm.chann_num   = 2,
        .attr.pcm.bit_width   = 24,
    },

    [PCM_48K_24BIT_STEREO_2MS] = {
        .type         = AUDIO_FORMAT_TYPE_PCM,
        .frame_num    = 1,
        .attr.pcm.sample_rate = 48000,
        .attr.pcm.frame_length = PCM_48K_24BIT_MONO_2MS_FRAME_SIZE,
        .attr.pcm.chann_num   = 2,
        .attr.pcm.bit_width   = 24,
    },

    [PCM_96K_24BIT_STEREO_0_5MS] = {
        .type         = AUDIO_FORMAT_TYPE_PCM,
        .frame_num    = 1,
        .attr.pcm.sample_rate = 96000,
        .attr.pcm.frame_length = PCM_96K_24BIT_MONO_0_5MS_FRAME_SIZE,
        .attr.pcm.chann_num   = 2,
        .attr.pcm.bit_width   = 24,
    },

    [SBC_48K_16BIT_STEREO] = {
        .type = AUDIO_FORMAT_TYPE_SBC,
        .frame_num = 1,
        .attr.sbc.sample_rate = 48000,
        .attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO,
        .attr.sbc.block_length = 16,
        .attr.sbc.subband_num = 8,
        .attr.sbc.bitpool = GAMING_SBC_BITPOOL,
        .attr.sbc.allocation_method = 0,
    },

    [LC3_16K_16BIT_MONO_7_5MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 16000,
        .attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO,
        .attr.lc3.frame_length = 30,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_7_5_MS,
        .attr.lc3.presentation_delay = 0,
    },

    [LC3_32K_16BIT_MONO_7_5MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 32000,
        .attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO,
        .attr.lc3.frame_length = 60,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_7_5_MS,
        .attr.lc3.presentation_delay = 0,
    },

    [LC3_16K_16BIT_MONO_10MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 16000,
        .attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO,
        .attr.lc3.frame_length = 40,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS,
        .attr.lc3.presentation_delay = 0,
    },

    [LC3_32K_16BIT_MONO_10MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 32000,
        .attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO,
        .attr.lc3.frame_length = 80,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS,
        .attr.lc3.presentation_delay = 0,
    },

    [LC3_48K_16BIT_MONO_10MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 48000,
        .attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO,
        .attr.lc3.frame_length = 120,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS,
        .attr.lc3.presentation_delay = 0,
    },

    [LC3_16K_16BIT_STEREO_7_5MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 16000,
        .attr.lc3.chann_location = (AUDIO_CHANNEL_LOCATION_FL | AUDIO_CHANNEL_LOCATION_FR),
        .attr.lc3.frame_length = 30,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_7_5_MS,
        .attr.lc3.presentation_delay = 0,
    },

    [LC3_32K_16BIT_STEREO_7_5MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 32000,
        .attr.lc3.chann_location = (AUDIO_CHANNEL_LOCATION_FL | AUDIO_CHANNEL_LOCATION_FR),
        .attr.lc3.frame_length = 60,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_7_5_MS,
        .attr.lc3.presentation_delay = 0,
    },

    [LC3_16K_16BIT_STEREO_10MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 16000,
        .attr.lc3.chann_location = (AUDIO_CHANNEL_LOCATION_FL | AUDIO_CHANNEL_LOCATION_FR),
        .attr.lc3.frame_length = 40,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS,
        .attr.lc3.presentation_delay = 0,
    },

    [LC3_32K_16BIT_STEREO_10MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 32000,
        .attr.lc3.chann_location = (AUDIO_CHANNEL_LOCATION_FL | AUDIO_CHANNEL_LOCATION_FR),
        .attr.lc3.frame_length = 80,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS,
        .attr.lc3.presentation_delay = 0,
    },

    [LC3_48K_16BIT_STEREO_10MS] = {
        .type = AUDIO_FORMAT_TYPE_LC3,
        .frame_num = 1,
        .attr.lc3.sample_rate = 48000,
        .attr.lc3.chann_location = (AUDIO_CHANNEL_LOCATION_FL | AUDIO_CHANNEL_LOCATION_FR),
        .attr.lc3.frame_length = 120,
        .attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS,
        .attr.lc3.presentation_delay = 0,
    },

};

static T_AUDIO_PIPE_INFO pipe_info[AUDIO_PIPE_MAX_NUM] =
{
    [AUDIO_PIPE_DOWNSTREAM_UAC1] = {
        .state = PIPE_STATE_RELEASED,
        .mode  = AUDIO_STREAM_MODE_DIRECT,
        .pipe_cback = downstream_pipe_cb,
        .gain  = 0,
    },

    [AUDIO_PIPE_DOWNSTREAM_UAC2] = {
        .state = PIPE_STATE_RELEASED,
        .mode  = AUDIO_STREAM_MODE_DIRECT,
        .pipe_cback = downstream_pipe_cb,
        .gain  = 0,
    },

    [AUDIO_PIPE_UPSTREAM] = {
        .state = PIPE_STATE_RELEASED,
        .mode  = AUDIO_STREAM_MODE_DIRECT,
        .pipe_cback = upstream_pipe_cb,
        .gain  = 0,
    },
};

static uint8_t *pipe_tmp_buf;
#if F_APP_HANDLE_DS_PIPE_IN_ISR
static uint8_t *pipe_data_buf_in_isr;
#endif

static T_APP_AUDIO_PIPE_TYPE get_pipe_type(T_AUDIO_PIPE_HANDLE handle)
{
    uint8_t i = 0;
    T_APP_AUDIO_PIPE_TYPE pipe_type = AUDIO_PIPE_MAX_NUM;

    for (i = 0; i < AUDIO_PIPE_MAX_NUM; i++)
    {
        if (pipe_info[i].handle == handle)
        {
            pipe_type = (T_APP_AUDIO_PIPE_TYPE)i;
            break;
        }
    }

    return pipe_type;
}

static void pipe_state_machine(T_APP_PIPE_EVENT event, T_AUDIO_PIPE_INFO *pipe,
                               T_APP_PIPE_PARAM *param)
{
    T_APP_PIPE_STATE pre_state = pipe->state;
    static T_AUDIO_PIPE_INFO *mixed_pipe[MIXED_PIPE_NUM] = {NULL};

    if (event == PIPE_EVENT_CREATE)
    {
        pipe->handle_data_in_isr = param->handle_data_in_isr;
        pipe->asrc_toggle = param->asrc_toggle;
        pipe->mode = param->mode;
        pipe->is_mixed_pipe = param->is_mixed_pipe;

        if (pre_state != PIPE_STATE_RELEASED)
        {
            bool change_codec = false;

            if (pipe->new_codec == false)
            {
                if (param->src_codec != pipe->param.src_codec ||
                    param->snk_codec != pipe->param.snk_codec)
                {
                    change_codec = true;
                }
            }
            else
            {
                if (param->src_codec != pipe->next_param.src_codec ||
                    param->snk_codec != pipe->next_param.snk_codec)
                {
                    change_codec = true;
                }
            }

            if (change_codec)
            {
                APP_PRINT_TRACE0("pipe_state_machine: set new codec");

                pipe->new_codec = true;
                pipe->seq = 0;
                memcpy(&pipe->next_param, param, sizeof(T_APP_PIPE_PARAM));
            }
        }
    }

    switch (pre_state)
    {
    case PIPE_STATE_MIXING:
        {
            if (event == PIPE_EVENT_MIXED)
            {
                if (audio_pipe_start(mixed_pipe[0]->handle))
                {
                    mixed_pipe[0]->state = PIPE_STATE_STARTING;
                }

                if (audio_pipe_start(mixed_pipe[1]->handle))
                {
                    mixed_pipe[1]->state = PIPE_STATE_STARTING;
                }
            }
        }
        break;

    case PIPE_STATE_DEMIXING:
        {
            if (event == PIPE_EVENT_DEMIXED)
            {
                mixed_pipe[0]->state = PIPE_STATE_RELEASING;
                mixed_pipe[1]->state = PIPE_STATE_RELEASING;

                /* set state releasing first to prevent fill data in
                   isr while releasing
                */
                if (audio_pipe_release(mixed_pipe[0]->handle) == false)
                {
                    mixed_pipe[0]->state = PIPE_STATE_DEMIXING;
                }

                if (audio_pipe_release(mixed_pipe[1]->handle) == false)
                {
                    mixed_pipe[1]->state = PIPE_STATE_DEMIXING;
                }
            }
        }
        break;

    case PIPE_STATE_RELEASED:
        {
            if (event == PIPE_EVENT_CREATE)
            {
                pipe->handle = audio_pipe_create(pipe->mode, codec_info[param->src_codec],
                                                 codec_info[param->snk_codec], pipe->gain, pipe->pipe_cback);

                if (pipe->handle != NULL)
                {
                    pipe->state = PIPE_STATE_CREATING;
                    pipe->src_type = param->src_codec;
                    pipe->snk_type = param->snk_codec;
                    memcpy(&pipe->param, param, sizeof(T_APP_PIPE_PARAM));
                }
            }
        }
        break;

    case PIPE_STATE_CREATING:
        {
            if (event == PIPE_EVENT_CREATED)
            {
                pipe->state = PIPE_STATE_CREATED;
                pipe->asrc_ratio = 0;

                if (pipe->is_mixed_pipe == false)
                {
                    if (audio_pipe_start(pipe->handle))
                    {
                        pipe->state = PIPE_STATE_STARTING;
                    }
                }
                else
                {
                    /* start pipe while the all mixed pipe created */
                    for (uint8_t i = 0; i < MIXED_PIPE_NUM; i++)
                    {
                        if (mixed_pipe[i] == NULL)
                        {
                            mixed_pipe[i] = pipe;
                            break;
                        }
                    }

                    if (mixed_pipe[0] && mixed_pipe[1])
                    {
                        if (audio_pipe_pre_mix(mixed_pipe[0]->handle, mixed_pipe[1]->handle))
                        {
                            mixed_pipe[0]->state = PIPE_STATE_MIXING;
                            mixed_pipe[1]->state = PIPE_STATE_MIXING;
                        }
                    }
                }
            }
            else if (event == PIPE_EVENT_RELEASE || pipe->new_codec)
            {
                /* set state releasing first to prevent fill data in
                   isr while releasing
                */
                pipe->state = PIPE_STATE_RELEASING;

                if (audio_pipe_release(pipe->handle) == false)
                {
                    pipe->state = PIPE_STATE_CREATING;
                }
            }
        }
        break;

    case PIPE_STATE_STARTING:
        {
            if (event == PIPE_EVENT_STARTED)
            {
                pipe->state = PIPE_STATE_STARTED;
                pipe->asrc_ratio = 0;

                audio_pipe_gain_set(pipe->handle, pipe->gain, pipe->gain);
            }
            else if (event == PIPE_EVENT_RELEASE || pipe->new_codec)
            {
                /* set state releasing first to prevent fill data in
                   isr while releasing
                */
                pipe->state = PIPE_STATE_RELEASING;

                if (audio_pipe_release(pipe->handle) == false)
                {
                    pipe->state = PIPE_STATE_STARTING;
                }
            }
        }
        break;

    case PIPE_STATE_STARTED:
        {
            if (event == PIPE_EVENT_RELEASE || pipe->new_codec)
            {
                if (pipe->is_mixed_pipe)
                {
                    mixed_pipe[0]->state = PIPE_STATE_DEMIXING;
                    mixed_pipe[1]->state = PIPE_STATE_DEMIXING;

                    if (audio_pipe_demix(mixed_pipe[0]->handle, mixed_pipe[1]->handle) == false)
                    {
                        mixed_pipe[0]->state = PIPE_STATE_STARTED;
                        mixed_pipe[1]->state = PIPE_STATE_STARTED;
                    }
                }
                else
                {
                    /* set state releasing first to prevent fill data in
                       isr while releasing
                    */
                    pipe->state = PIPE_STATE_RELEASING;

                    if (audio_pipe_release(pipe->handle) == false)
                    {
                        pipe->state = PIPE_STATE_STARTED;
                    }
                }
            }
        }
        break;

    case PIPE_STATE_RELEASING:
        {
            if (event == PIPE_EVENT_RELEASED)
            {
                pipe->handle = NULL;
                pipe->state = PIPE_STATE_RELEASED;
                pipe->src_type = GAMING_CODEC_NONE;
                pipe->snk_type = GAMING_CODEC_NONE;
                pipe->snk_transport_address = 0;
                pipe->asrc_toggle = false;
                pipe->is_mixed_pipe = false;

                if (pipe->new_codec)
                {
                    pipe->handle = audio_pipe_create(pipe->mode, codec_info[pipe->next_param.src_codec],
                                                     codec_info[pipe->next_param.snk_codec], pipe->gain, pipe->pipe_cback);

                    if (pipe->handle != NULL)
                    {
                        pipe->state = PIPE_STATE_CREATING;
                        pipe->src_type = pipe->next_param.src_codec;
                        pipe->snk_type = pipe->next_param.snk_codec;
                        memcpy(&pipe->param, &pipe->next_param, sizeof(T_APP_PIPE_PARAM));
                    }

                    pipe->new_codec = false;
                    memset(&pipe->next_param, 0, sizeof(T_APP_PIPE_PARAM));
                }
            }
        }
        break;
    }

    APP_PRINT_TRACE4("pipe_state_machine: event %d handle %p state (%d->%d)", event, pipe->handle,
                     pre_state, pipe->state);

}

static T_APP_PIPE_EVENT pipe_event_mapping(T_AUDIO_PIPE_EVENT event)
{
    T_APP_PIPE_EVENT ret = PIPE_EVENT_NONE;
    uint8_t event_num = sizeof(pipe_event_map) / sizeof(T_AUDIO_PIPE_EVENT);

    if (event <= (event_num - 1))
    {
        ret = pipe_event_map[event];
    }

    return ret;
}

static void pipe_event_handle(T_AUDIO_PIPE_EVENT event, T_AUDIO_PIPE_HANDLE handle, uint32_t param)
{
    T_APP_AUDIO_PIPE_TYPE type = get_pipe_type(handle);

#if F_APP_HANDLE_DS_PIPE_IN_ISR
    if (event == AUDIO_PIPE_EVENT_DATA_IND && pipe_info[type].handle_data_in_isr)
    {
        return;
    }
#endif

    if (type == AUDIO_PIPE_MAX_NUM)
    {
        APP_PRINT_ERROR0("pipe_event_handle: failed to find pipe");
        return;
    }

    if (event != AUDIO_PIPE_EVENT_DATA_IND)
    {
        APP_PRINT_TRACE2("pipe_event_handle: event %d handle %p", event, handle);
    }

    if (event != AUDIO_PIPE_EVENT_DATA_IND)
    {
        T_APP_PIPE_EVENT evt = pipe_event_mapping(event);

        if (pipe_info[type].param.event_cback &&
            evt != PIPE_EVENT_NONE)
        {
            pipe_info[type].param.event_cback(evt);
        }
    }

    switch (event)
    {
    case AUDIO_PIPE_EVENT_CREATED:
        {
            pipe_state_machine(PIPE_EVENT_CREATED, &pipe_info[type], NULL);
        }
        break;

    case AUDIO_PIPE_EVENT_STARTED:
        {
            pipe_state_machine(PIPE_EVENT_STARTED, &pipe_info[type], NULL);
        }
        break;

    case AUDIO_PIPE_EVENT_RELEASED:
        {
            pipe_state_machine(PIPE_EVENT_RELEASED, &pipe_info[type], NULL);
        }
        break;

#if F_APP_HANDLE_DS_PIPE_IN_ISR
    case AUDIO_PIPE_EVENT_CREATED_ADDR:
        {
            APP_PRINT_TRACE1("AUDIO_PIPE_EVENT_CREATED_ADDR: %p", param);

            if (pipe_info[type].handle_data_in_isr)
            {
                pipe_info[type].snk_transport_address = param;
            }
        }
        break;
#endif

    case AUDIO_PIPE_EVENT_MIXED:
        {
            pipe_state_machine(PIPE_EVENT_MIXED, &pipe_info[type], NULL);
        }
        break;

    case AUDIO_PIPE_EVENT_DEMIXED:
        {
            pipe_state_machine(PIPE_EVENT_DEMIXED, &pipe_info[type], NULL);
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_IND:
        {
            uint32_t timestamp = 0;
            uint16_t dsp_seq = 0;
            T_AUDIO_STREAM_STATUS status;
            uint8_t frame_num = 0;
            uint16_t len = 0;

            if (audio_pipe_drain(pipe_info[type].handle, &timestamp, &dsp_seq,
                                 &status, &frame_num, pipe_tmp_buf, &len))
            {
                if (pipe_info[type].param.data_cback)
                {
                    pipe_info[type].param.data_cback(pipe_tmp_buf, len);
                }
            }
        }
        break;
    }
}

#if F_APP_HANDLE_DS_PIPE_IN_ISR
static bool pipe_data_cb_in_isr(void)
{
    uint8_t i = 0;
    uint32_t timestamp;
    uint16_t seq;
    T_AUDIO_STREAM_STATUS status;
    uint8_t frame_num;
    uint16_t len;

    for (i = 0; i < AUDIO_PIPE_MAX_NUM; i++)
    {
        T_AUDIO_PIPE_INFO *pipe = &pipe_info[i];

        if (pipe->handle_data_in_isr &&
            pipe->state == PIPE_STATE_STARTED)
        {
            if (audio_pipe_direct_drain(pipe->snk_transport_address, &timestamp, &seq,
                                        &status, &frame_num, pipe_data_buf_in_isr, &len))
            {
                pipe->param.data_cback(pipe_data_buf_in_isr, len);
            }
        }
    }

    return true;
}
#endif

static bool downstream_pipe_cb(T_AUDIO_PIPE_HANDLE handle,
                               T_AUDIO_PIPE_EVENT  event,
                               uint32_t            param)
{
    pipe_event_handle(event, handle, param);

    return true;
}

static bool upstream_pipe_cb(T_AUDIO_PIPE_HANDLE handle,
                             T_AUDIO_PIPE_EVENT  event,
                             uint32_t            param)
{
    pipe_event_handle(event, handle, param);

    return true;
}

uint16_t app_pipe_get_codec_frame_size(T_GAMING_CODEC codec_type)
{
    uint16_t len = 0;
    const T_AUDIO_FORMAT_INFO *codec = &codec_info[codec_type];

    if (codec->type == AUDIO_FORMAT_TYPE_PCM)
    {
        len = codec->attr.pcm.frame_length;

        if (codec->attr.pcm.chann_num == 2)
        {
            len = len * 2;
        }
    }
    else if (codec->type == AUDIO_FORMAT_TYPE_LC3)
    {
        len = codec->attr.lc3.frame_length;

        if (codec->attr.lc3.chann_location != AUDIO_CHANNEL_LOCATION_MONO)
        {
            len = len * 2;
        }
    }

    return len;
}

T_APP_PIPE_STATE app_pipe_get_state(T_APP_AUDIO_PIPE_TYPE type)
{
    return pipe_info[type].state;
}

int32_t app_pipe_get_asrc_ratio(T_APP_AUDIO_PIPE_TYPE type)
{
    return pipe_info[type].asrc_ratio;
}

bool app_pipe_asrc_set(T_APP_AUDIO_PIPE_TYPE type, int32_t ratio)
{
    T_AUDIO_PIPE_INFO *pipe = &pipe_info[type];
    bool ret = false;

    if (pipe->state == PIPE_STATE_STARTED)
    {
        ret = audio_pipe_asrc_set(pipe->handle, ratio);

        if (ret)
        {
            pipe->asrc_ratio = ratio;
        }
    }

    APP_PRINT_TRACE3("app_pipe_asrc_set: type %d ratio %d ret %d", type, ratio, ret);

    return ret;
}

bool app_pipe_gain_set(T_APP_AUDIO_PIPE_TYPE type, uint16_t gain)
{
    T_AUDIO_PIPE_INFO *pipe = &pipe_info[type];
    bool ret = false;

    pipe->gain = gain;

    if (pipe->state == PIPE_STATE_STARTED)
    {
        ret = audio_pipe_gain_set(pipe->handle, pipe->gain, pipe->gain);
    }

    APP_PRINT_TRACE3("app_pipe_gain_set: type %d gain 0x%04x ret %d", type, gain, ret);

    return ret;
}

bool app_pipe_is_ready(T_APP_AUDIO_PIPE_TYPE type)
{
    T_AUDIO_PIPE_INFO *pipe = &pipe_info[type];
    bool ret = false;

    if (pipe->state == PIPE_STATE_STARTED)
    {
        ret = true;
    }

    return ret;
}

#if F_APP_HANDLE_DS_PIPE_IN_ISR
bool app_pipe_fill_in_isr(T_APP_AUDIO_PIPE_TYPE type, void *buf, uint16_t len)
{
    bool ret = false;
    T_AUDIO_PIPE_INFO *pipe = &pipe_info[type];

    if (pipe->state == PIPE_STATE_STARTED)
    {
        ret = audio_pipe_fill_in_isr(pipe->handle, 0, pipe->seq, AUDIO_STREAM_STATUS_CORRECT, 1, buf, len);

        if (ret)
        {
            pipe->seq++;
        }
    }

    if (ret == false)
    {
        APP_PRINT_ERROR1("app_pipe_fill failed: state %d", pipe->state);
    }

    return ret;
}
#endif

bool app_pipe_fill(T_APP_AUDIO_PIPE_TYPE type, void *buf, uint16_t len)
{
    bool ret = false;
    T_AUDIO_PIPE_INFO *pipe = &pipe_info[type];
    uint16_t seq = pipe->seq;

    if (pipe->state == PIPE_STATE_STARTED)
    {
        ret = audio_pipe_fill(pipe->handle, 0, seq, AUDIO_STREAM_STATUS_CORRECT, 1, buf, len);
    }

    if (ret == false)
    {
        APP_PRINT_ERROR1("app_pipe_fill failed: state %d", pipe->state);
    }
    else
    {
        pipe->seq++;
    }

    return ret;
}

void app_pipe_create(T_APP_AUDIO_PIPE_TYPE type, T_APP_PIPE_PARAM *param)
{
    T_AUDIO_PIPE_INFO *pipe = &pipe_info[type];

    APP_PRINT_TRACE3("app_pipe_create: type %d src %d snk %d", type, param->src_codec,
                     param->snk_codec);

    pipe_state_machine(PIPE_EVENT_CREATE, pipe, param);
}

void app_pipe_release(T_APP_AUDIO_PIPE_TYPE type)
{
    T_AUDIO_PIPE_INFO *pipe = &pipe_info[type];

    APP_PRINT_TRACE1("app_pipe_release: type %d", type);

    pipe_state_machine(PIPE_EVENT_RELEASE, pipe, NULL);
}

void app_pipe_init(void)
{
    pipe_tmp_buf = calloc(1, 4096);

#if F_APP_HANDLE_DS_PIPE_IN_ISR
    void dsp_hal_register_pipe_data_cback(bool (*)(void));

    pipe_data_buf_in_isr = calloc(1, 1024);

    dsp_hal_register_pipe_data_cback(pipe_data_cb_in_isr);
#endif
}

