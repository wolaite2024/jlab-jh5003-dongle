/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include "audio_type.h"

static uint8_t sbc_chann_num_get(T_AUDIO_SBC_CHANNEL_MODE chann_mode)
{
    uint8_t chann_num;

    if (chann_mode == AUDIO_SBC_CHANNEL_MODE_MONO)
    {
        chann_num = 1;
    }
    else
    {
        chann_num = 2;
    }

    return chann_num;
}

static uint8_t mp3_chann_num_get(T_AUDIO_MP3_CHANNEL_MODE chann_mode)
{
    uint8_t chann_num;

    if (chann_mode == AUDIO_MP3_CHANNEL_MODE_MONO)
    {
        chann_num = 1;
    }
    else
    {
        chann_num = 2;
    }

    return chann_num;
}

static uint8_t lc3_chann_num_get(uint32_t chann_location)
{
    uint8_t chann_num = 0;

    if (chann_location == AUDIO_CHANNEL_LOCATION_MONO)
    {
        return 1;
    }

    while (chann_location != 0)
    {
        chann_num++;
        chann_location &= chann_location - 1;
    }

    return chann_num;
}

static uint8_t ldac_chann_num_get(T_AUDIO_LDAC_CHANNEL_MODE chann_mode)
{
    uint8_t chann_num;

    if (chann_mode == AUDIO_LDAC_CHANNEL_MODE_MONO)
    {
        chann_num = 1;
    }
    else
    {
        chann_num = 2;
    }

    return chann_num;
}

uint8_t audio_codec_chann_num_get(T_AUDIO_FORMAT_TYPE type, void *attr)
{
    uint8_t chann_num;

    switch (type)
    {
    case AUDIO_FORMAT_TYPE_MSBC:
        {
            T_AUDIO_MSBC_ATTR *msbc_attr;

            msbc_attr = (T_AUDIO_MSBC_ATTR *)attr;
            chann_num = sbc_chann_num_get(msbc_attr->chann_mode);
        }
        break;

    case AUDIO_FORMAT_TYPE_SBC:
        {
            T_AUDIO_SBC_ATTR *sbc_attr;

            sbc_attr = (T_AUDIO_SBC_ATTR *)attr;
            chann_num = sbc_chann_num_get(sbc_attr->chann_mode);
        }
        break;

    case AUDIO_FORMAT_TYPE_MP3:
        {
            T_AUDIO_MP3_ATTR *mp3_attr;

            mp3_attr = (T_AUDIO_MP3_ATTR *)attr;
            chann_num = mp3_chann_num_get(mp3_attr->chann_mode);
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            T_AUDIO_LC3_ATTR *lc3_attr;

            lc3_attr = (T_AUDIO_LC3_ATTR *)attr;
            chann_num = lc3_chann_num_get(lc3_attr->chann_location);
        }
        break;

    case AUDIO_FORMAT_TYPE_LDAC:
        {
            T_AUDIO_LDAC_ATTR *ldac_attr;

            ldac_attr = (T_AUDIO_LDAC_ATTR *)attr;
            chann_num = ldac_chann_num_get(ldac_attr->chann_mode);
        }
        break;

    case AUDIO_FORMAT_TYPE_LHDC:
        {
            T_AUDIO_LHDC_ATTR *lhdc_attr;

            lhdc_attr = (T_AUDIO_LHDC_ATTR *)attr;
            chann_num = lhdc_attr->chann_num;
        }
        break;

    default:
        chann_num = 0;
        break;
    }

    return chann_num;
}

static uint16_t pcm_frame_size_get(uint16_t frame_length,
                                   uint8_t  bit_width)
{
    return frame_length * 8 / bit_width;
}

static uint16_t cvsd_frame_size_get(uint32_t                    sample_rate,
                                    T_AUDIO_CVSD_FRAME_DURATION frame_duration)
{
    uint16_t frame_size;

    switch (frame_duration)
    {
    case AUDIO_CVSD_FRAME_DURATION_3_75_MS:
        frame_size = sample_rate * 15 / 4 / 1000;
        break;

    case AUDIO_CVSD_FRAME_DURATION_7_5_MS:
        frame_size = sample_rate * 15 / 2 / 1000;
        break;

    default:
        frame_size = sample_rate * 15 / 2 / 1000;
        break;
    }

    return frame_size;
}

static uint16_t opus_frame_size_get(uint32_t                    sample_rate,
                                    T_AUDIO_OPUS_FRAME_DURATION frame_duration)
{
    uint16_t frame_size;

    switch (frame_duration)
    {
    case AUDIO_OPUS_FRAME_DURATION_2_5_MS:
        frame_size = sample_rate * 5 / 2 / 1000;
        break;
    case AUDIO_OPUS_FRAME_DURATION_5_MS:
        frame_size = sample_rate * 5 / 1000;
        break;
    case AUDIO_OPUS_FRAME_DURATION_10_MS:
        frame_size = sample_rate * 10 / 1000;
        break;
    case AUDIO_OPUS_FRAME_DURATION_20_MS:
        frame_size = sample_rate * 20 / 1000;
        break;
    case AUDIO_OPUS_FRAME_DURATION_40_MS:
        frame_size = sample_rate * 40 / 1000;
        break;
    case AUDIO_OPUS_FRAME_DURATION_60_MS:
        frame_size = sample_rate * 60 / 1000;
        break;
    case AUDIO_OPUS_FRAME_DURATION_80_MS:
        frame_size = sample_rate * 80 / 1000;
        break;
    case AUDIO_OPUS_FRAME_DURATION_100_MS:
        frame_size = sample_rate * 100 / 1000;
        break;
    case AUDIO_OPUS_FRAME_DURATION_120_MS:
        frame_size = sample_rate * 120 / 1000;
        break;
    default:
        frame_size = sample_rate * 20 / 1000;
        break;
    }

    return frame_size;
}

static const uint16_t mp3_frame_samples_table[4][4] =
{
    /* MPEG2.5: 0;  Reserved: 1;    MPEG2: 2;   MPEG1: 3. */
    {0,             0,              0,          0   },      /* Reserved: 0 */
    {576,           0,              576,        1152},      /* Layer 3: 1 */
    {1152,          0,              1152,       1152},      /* Layer 2: 2 */
    {384,           0,              384,        384 },      /* Layer 1: 3 */
};

static uint16_t lc3_frame_size_get(uint32_t                   sample_rate,
                                   T_AUDIO_LC3_FRAME_DURATION frame_duration)
{
    uint16_t frame_size;

    /* For both 44.1kHz and 48kHz, the input frame size is 480 sample for
     * the 10ms frame duration and 360 samples for the 7.5ms frame duration.
     */
    if (sample_rate == 44100)
    {
        sample_rate = 48000;
    }

    switch (frame_duration)
    {
    case AUDIO_LC3_FRAME_DURATION_7_5_MS:
        frame_size = sample_rate * 15 / 2 / 1000;
        break;
    case AUDIO_LC3_FRAME_DURATION_10_MS:
        frame_size = sample_rate * 10 / 1000;
        break;
    default:
        frame_size = sample_rate * 10 / 1000;
        break;
    }

    return frame_size;
}

static uint16_t ldac_frame_size_get(uint32_t sample_rate)
{
    uint16_t frame_size;

    switch (sample_rate)
    {
    case 44100:
    case 48000:
        frame_size = 128;
        break;

    case 88200:
    case 96000:
        frame_size = 256;
        break;

    case 176400:
    case 192000:
        frame_size = 512;
        break;

    default:
        frame_size = 0;
        break;
    }

    return frame_size;
}

static uint16_t lhdc_frame_size_get(uint32_t                    sample_rate,
                                    T_AUDIO_LHDC_FRAME_DURATION frame_duration)
{
    uint16_t frame_size;

    switch (frame_duration)
    {
    case AUDIO_LHDC_FRAME_DURATION_5_MS:
        frame_size = sample_rate * 5 / 1000;
        break;

    default:
        frame_size = 0;
        break;
    }

    return frame_size;
}

uint16_t audio_codec_frame_size_get(T_AUDIO_FORMAT_TYPE type, void *attr)
{
    uint16_t frame_size;

    switch (type)
    {
    case AUDIO_FORMAT_TYPE_PCM:
        {
            T_AUDIO_PCM_ATTR *pcm_attr;

            pcm_attr = (T_AUDIO_PCM_ATTR *)attr;
            frame_size = pcm_frame_size_get(pcm_attr->frame_length, pcm_attr->bit_width);
        }
        break;

    case AUDIO_FORMAT_TYPE_CVSD:
        {
            T_AUDIO_CVSD_ATTR *cvsd_attr;

            cvsd_attr = (T_AUDIO_CVSD_ATTR *)attr;
            frame_size = cvsd_frame_size_get(cvsd_attr->sample_rate, cvsd_attr->frame_duration);
        }
        break;

    case AUDIO_FORMAT_TYPE_OPUS:
        {
            T_AUDIO_OPUS_ATTR *opus_attr;

            opus_attr = (T_AUDIO_OPUS_ATTR *)attr;
            frame_size = opus_frame_size_get(opus_attr->sample_rate, opus_attr->frame_duration);
        }
        break;

    case AUDIO_FORMAT_TYPE_MP3:
        {
            T_AUDIO_MP3_ATTR *mp3_attr;

            mp3_attr = (T_AUDIO_MP3_ATTR *)attr;
            frame_size = mp3_frame_samples_table[mp3_attr->layer][mp3_attr->version];
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            T_AUDIO_LC3_ATTR *lc3_attr;

            lc3_attr = (T_AUDIO_LC3_ATTR *)attr;
            frame_size = lc3_frame_size_get(lc3_attr->sample_rate, lc3_attr->frame_duration);
        }
        break;

    case AUDIO_FORMAT_TYPE_LDAC:
        {
            T_AUDIO_LDAC_ATTR *ldac_attr;

            ldac_attr = (T_AUDIO_LDAC_ATTR *)attr;
            frame_size = ldac_frame_size_get(ldac_attr->sample_rate);
        }
        break;

    case AUDIO_FORMAT_TYPE_LHDC:
        {
            T_AUDIO_LHDC_ATTR *lhdc_attr;

            lhdc_attr = (T_AUDIO_LHDC_ATTR *)attr;
            frame_size = lhdc_frame_size_get(lhdc_attr->sample_rate, lhdc_attr->frame_duration);
        }
        break;

    default:
        frame_size = 0;
        break;
    }

    return frame_size;
}

static uint32_t lc3_frame_duration_get(T_AUDIO_LC3_FRAME_DURATION frame_duration)
{
    uint32_t duration;

    switch (frame_duration)
    {
    case AUDIO_LC3_FRAME_DURATION_7_5_MS:
        duration = 7500;
        break;

    case AUDIO_LC3_FRAME_DURATION_10_MS:
        duration = 10000;
        break;

    default:
        duration = 0;
        break;
    }

    return duration;
}

uint32_t audio_codec_frame_duration_get(T_AUDIO_FORMAT_TYPE type, void *attr)
{
    uint32_t frame_duration;

    switch (type)
    {
    case AUDIO_FORMAT_TYPE_LC3:
        {
            T_AUDIO_LC3_ATTR *lc3_attr;

            lc3_attr = (T_AUDIO_LC3_ATTR *)attr;
            frame_duration = lc3_frame_duration_get(lc3_attr->frame_duration);
        }
        break;

    default:
        frame_duration = 0;
        break;
    }

    return frame_duration;
}
