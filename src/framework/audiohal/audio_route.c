/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "os_mem.h"
#include "trace.h"
#include "sys_cfg.h"
#include "audio_route.h"

/* TODO Remove Start */
#define AUDIO_ROUTE_ANC_FF_MIC              (0x01)
/* TODO Remove End*/


typedef struct t_lvl_gain_cback
{
    P_AUDIO_ROUTE_ADC_GAIN_CBACK cback_adc_get;
    P_AUDIO_ROUTE_DAC_GAIN_CBACK cback_dac_get;
} T_LVL_GAIN_CBACK;

typedef union
{
    uint16_t offset;
    struct
    {
        uint16_t audio_running        : 1;
        uint16_t voice_running        : 1;
        uint16_t record_running       : 1;
        uint16_t notification_running : 1;
        uint16_t apt_running          : 1;
        uint16_t anc_running          : 1;
        uint16_t vad_running          : 1;
        uint16_t analog_running       : 1;
        uint16_t reserved             : 8;
    } ;
} T_AUDIO_ROUTE_CATEGORY_BITMASK;

typedef struct t_audio_route_db
{
    T_LVL_GAIN_CBACK *lvl_gain_cback;
    P_AUDIO_ROUTE_SIDETONE_CBACK lvl_sidetone_cback;
    uint32_t record_dev;
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP register_group[AUDIO_CATEGORY_NUMBER];
    T_AUDIO_ROUTE_CATEGORY_BITMASK category_bitmask;
    /* TODO: Remove */
    uint8_t flag;
    /* end */
} T_AUDIO_ROUTE_DB;

static T_AUDIO_ROUTE_DB *audio_route_db = NULL;

static const int16_t voice_dac_dig_gain_table[] =
{
    0xe980, 0xeb00, 0xec80, 0xee00, 0xef80, 0xf100, 0xf280, 0xf400,
    0xf580, 0xf700, 0xf880, 0xfa00, 0xfb80, 0xfd00, 0xfe80, 0x0000
};

static const int16_t audio_dac_dig_gain_table[] =
{
    0x8001, 0xeb00, 0xec80, 0xee00, 0xef80, 0xf100, 0xf280, 0xf400,
    0xf580, 0xf700, 0xf880, 0xfa00, 0xfb80, 0xfd00, 0xfe80, 0x0000
};

static const int16_t voice_audio_adc_gain_table[] =
{
    0x0000, 0x002f, 0x0037, 0x003f, 0x0047, 0x012f, 0x0137, 0x013f,
    0x0147, 0x022f, 0x0237, 0x023f, 0x0247, 0x032f, 0x0337, 0x033f
};

static const uint32_t sample_rate_map[] =
{
    [AUDIO_ROUTE_SPORT_SR_8KHZ]       = 8000,
    [AUDIO_ROUTE_SPORT_SR_16KHZ]      = 16000,
    [AUDIO_ROUTE_SPORT_SR_32KHZ]      = 32000,
    [AUDIO_ROUTE_SPORT_SR_44P1KHZ]    = 44100,
    [AUDIO_ROUTE_SPORT_SR_48KHZ]      = 48000,
    [AUDIO_ROUTE_SPORT_SR_88P2KHZ]    = 88200,
    [AUDIO_ROUTE_SPORT_SR_96KHZ]      = 96000,
    [AUDIO_ROUTE_SPORT_SR_192KHZ]     = 192000,
    [AUDIO_ROUTE_SPORT_SR_12KHZ]      = 12000,
    [AUDIO_ROUTE_SPORT_SR_24KHZ]      = 24000,
    [AUDIO_ROUTE_SPORT_SR_11P025KHZ]  = 11025,
    [AUDIO_ROUTE_SPORT_SR_22P05KHZ]   = 22050,
    [AUDIO_ROUTE_SPORT_SR_64KHZ]      = 64000,
    [AUDIO_ROUTE_SPORT_SR_DYNAMIC]    = 0,
};

static T_AUDIO_ROUTE_MIC_CLASS audio_route_mic_class_get(uint8_t mic_sel)
{
    T_AUDIO_ROUTE_MIC_CLASS mic_power_class = AUDIO_ROUTE_MIC_NORMAL;

    switch (mic_sel)
    {
    case AUDIO_ROUTE_AMIC1:
        {
            mic_power_class = (T_AUDIO_ROUTE_MIC_CLASS)sys_cfg_const.amic1_class;
        }
        break;
    case AUDIO_ROUTE_AMIC2:
        {
            mic_power_class = (T_AUDIO_ROUTE_MIC_CLASS)sys_cfg_const.amic2_class;
        }
        break;
    case AUDIO_ROUTE_AMIC3:
        {
            mic_power_class = (T_AUDIO_ROUTE_MIC_CLASS)sys_cfg_const.amic3_class;
        }
        break;
    case AUDIO_ROUTE_AMIC4:
        {
            mic_power_class = (T_AUDIO_ROUTE_MIC_CLASS)sys_cfg_const.amic4_class;
        }
        break;
    default:
        {
            mic_power_class = AUDIO_ROUTE_MIC_NORMAL;
        }
        break;
    }
    return mic_power_class;
}

static uint8_t audio_route_audio_physical_path_num_get(void)
{
    uint8_t path_num = 0;

    path_num += sys_cfg_const.audio_primary_spk_enable;
    path_num += sys_cfg_const.audio_secondary_spk_enable;
    path_num += sys_cfg_const.audio_primary_ref_spk_enable;
    path_num += sys_cfg_const.audio_secondary_ref_spk_enable;

    return path_num;
}

static void audio_route_audio_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.audio_primary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.audio_primary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.audio_primary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.audio_primary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.audio_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.audio_primary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_0_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.audio_primary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.audio_primary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.audio_primary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.audio_secondary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.audio_secondary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.audio_secondary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.audio_secondary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.audio_secondary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.audio_secondary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_1_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.audio_secondary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.audio_secondary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.audio_secondary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.audio_primary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.audio_primary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.audio_primary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.audio_primary_ref_spk_dac_ch;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.audio_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.audio_primary_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.audio_primary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.audio_primary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.audio_primary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.audio_primary_ref_spk_polarity;
    }
}

static uint8_t audio_route_voice_physical_path_num_get(void)
{
    uint8_t path_num;

    path_num = 0;

    path_num += sys_cfg_const.voice_primary_spk_enable;
    path_num += sys_cfg_const.voice_secondary_spk_enable;
    path_num += sys_cfg_const.voice_primary_ref_spk_enable;
    path_num += sys_cfg_const.voice_secondary_ref_spk_enable;
    path_num += sys_cfg_const.voice_primary_mic_enable;
    path_num += sys_cfg_const.voice_secondary_mic_enable;
    path_num += sys_cfg_const.voice_fusion_mic_enable;
    path_num += sys_cfg_const.voice_primary_ref_mic_enable;
    path_num += sys_cfg_const.voice_secondary_ref_mic_enable;
    path_num += sys_cfg_const.voice_bone_mic_enable;

    return path_num;
}

static void audio_route_voice_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.voice_primary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.voice_primary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.voice_primary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.voice_primary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.voice_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.voice_primary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_0_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_primary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_primary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_primary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_secondary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.voice_secondary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.voice_secondary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.voice_secondary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.voice_secondary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.voice_secondary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_1_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_secondary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_secondary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_secondary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_primary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.voice_primary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.voice_primary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.voice_primary_ref_spk_dac_ch;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.voice_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.voice_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.voice_primary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_primary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_primary_ref_spk_sport_ch;
        physical_path++;
    }

    if (sys_cfg_const.voice_secondary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.voice_secondary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.voice_secondary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.voice_secondary_ref_spk_dac_ch;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.voice_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.voice_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.voice_secondary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_secondary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_secondary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_secondary_ref_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_primary_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.voice_primary_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.voice_primary_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.voice_primary_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.voice_primary_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.voice_primary_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.voice_primary_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_primary_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_primary_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_primary_mic_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_secondary_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.voice_secondary_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.voice_secondary_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.voice_secondary_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.voice_secondary_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.voice_secondary_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.voice_secondary_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.voice_secondary_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_secondary_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_secondary_mic_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_fusion_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_FUSION_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.voice_fusion_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.voice_fusion_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)sys_cfg_const.voice_fusion_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.voice_fusion_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.voice_fusion_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.voice_fusion_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_fusion_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_fusion_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_fusion_mic_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_primary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.voice_primary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.voice_primary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.voice_primary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.voice_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.voice_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain =
            sys_cfg_const.voice_primary_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.voice_primary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_primary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_primary_ref_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_primary_ref_mic_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_secondary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.voice_secondary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.voice_secondary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.voice_secondary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.voice_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.voice_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain =
            sys_cfg_const.voice_primary_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.voice_secondary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_secondary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_secondary_ref_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_secondary_ref_mic_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_bone_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_BONE_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.voice_bone_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.voice_bone_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)sys_cfg_const.voice_bone_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.voice_bone_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.voice_bone_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.voice_bone_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_bone_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_bone_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_bone_mic_polarity;
    }
}

static uint8_t audio_route_record_physical_path_num_get(void)
{
    uint8_t path_num = 0;
    uint32_t record_dev;

    record_dev = audio_route_record_dev_get();

    if (record_dev == AUDIO_DEVICE_IN_MIC)
    {
        path_num += sys_cfg_const.record_primary_mic_enable;
        path_num += sys_cfg_const.record_secondary_mic_enable;
        path_num += sys_cfg_const.record_primary_ref_mic_enable;
        path_num += sys_cfg_const.record_secondary_ref_mic_enable;
    }
    else if (record_dev == AUDIO_DEVICE_IN_AUX)
    {
        path_num += sys_cfg_const.record_aux_left_enable;
        path_num += sys_cfg_const.record_aux_right_enable;
        path_num += sys_cfg_const.record_primary_ref_mic_enable;
        path_num += sys_cfg_const.record_secondary_ref_mic_enable;
    }

    return path_num;
}

static void audio_route_record_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    uint32_t record_dev;

    record_dev = audio_route_record_dev_get();

    if (record_dev == AUDIO_DEVICE_IN_MIC)
    {
        if (sys_cfg_const.record_primary_mic_enable)
        {
            physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_MIC;
            physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
            physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                  sys_cfg_const.record_primary_mic_adc_ch;
            physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                   sys_cfg_const.record_primary_mic_sel;
            physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                    sys_cfg_const.record_primary_mic_type;
            physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                         physical_path->attr.mic_attr.mic_sel);
            physical_path->attr.mic_attr.equalizer_en = true;
            physical_path->attr.mic_attr.ana_gain = sys_cfg_const.record_primary_mic_analog_gain;
            physical_path->attr.mic_attr.dig_gain = sys_cfg_const.record_primary_mic_digital_gain;
            physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.record_primary_mic_digital_boost_gain;
            physical_path->attr.mic_attr.mic_external = false;
            physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.record_primary_mic_sport_idx;
            physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
            physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.record_primary_mic_sport_ch;
            physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.record_primary_mic_polarity;
            physical_path++;
        }

        if (sys_cfg_const.record_secondary_mic_enable)
        {
            physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_MIC;
            physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
            physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                  sys_cfg_const.record_secondary_mic_adc_ch;
            physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                   sys_cfg_const.record_secondary_mic_sel;
            physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                    sys_cfg_const.record_secondary_mic_type;
            physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                         physical_path->attr.mic_attr.mic_sel);
            physical_path->attr.mic_attr.equalizer_en = true;
            physical_path->attr.mic_attr.ana_gain = sys_cfg_const.record_secondary_mic_analog_gain;
            physical_path->attr.mic_attr.dig_gain = sys_cfg_const.record_secondary_mic_digital_gain;
            physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.record_secondary_mic_digital_boost_gain;
            physical_path->attr.mic_attr.mic_external = false;
            physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                       sys_cfg_const.record_secondary_mic_sport_idx;
            physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
            physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.record_secondary_mic_sport_ch;
            physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.record_secondary_mic_polarity;
            physical_path++;
        }

        if (sys_cfg_const.record_primary_ref_mic_enable)
        {
            physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC;
            physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
            physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                  sys_cfg_const.record_primary_ref_mic_adc_ch;
            physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                   sys_cfg_const.record_primary_ref_mic_sel;
            physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                    sys_cfg_const.record_primary_ref_mic_type;
            physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                         physical_path->attr.mic_attr.mic_sel);
            physical_path->attr.mic_attr.equalizer_en = true;
            physical_path->attr.mic_attr.ana_gain = sys_cfg_const.record_primary_mic_analog_gain;
            physical_path->attr.mic_attr.dig_gain = sys_cfg_const.record_primary_mic_digital_gain;
            physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.record_primary_ref_mic_external;
            physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                       sys_cfg_const.record_primary_ref_mic_sport_idx;
            physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
            physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.record_primary_ref_mic_sport_ch;
            physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.record_primary_ref_mic_polarity;
            physical_path++;
        }

        if (sys_cfg_const.record_secondary_ref_mic_enable)
        {
            physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC;
            physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
            physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                  sys_cfg_const.record_secondary_ref_mic_adc_ch;
            physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                   sys_cfg_const.record_secondary_ref_mic_sel;
            physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                    sys_cfg_const.record_secondary_ref_mic_type;
            physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                         physical_path->attr.mic_attr.mic_sel);
            physical_path->attr.mic_attr.equalizer_en = true;
            physical_path->attr.mic_attr.ana_gain = sys_cfg_const.record_primary_mic_analog_gain;
            physical_path->attr.mic_attr.dig_gain = sys_cfg_const.record_primary_mic_digital_gain;
            physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.record_secondary_ref_mic_external;
            physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                       sys_cfg_const.record_primary_ref_mic_sport_idx;
            physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
            physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.record_secondary_ref_mic_sport_ch;
        }
    }
    else if (record_dev == AUDIO_DEVICE_IN_AUX)
    {
        if (sys_cfg_const.record_aux_left_enable)
        {
            physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_AUX_IN;
            physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_AUX_IN;
            physical_path->attr.aux_in_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                     sys_cfg_const.record_aux_left_adc_ch;
            physical_path->attr.aux_in_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                      sys_cfg_const.record_aux_left_sel;
            physical_path->attr.aux_in_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                       sys_cfg_const.record_aux_left_type;
            physical_path->attr.aux_in_attr.mic_class = audio_route_mic_class_get(
                                                            physical_path->attr.aux_in_attr.mic_sel);
            physical_path->attr.aux_in_attr.equalizer_en = true;
            physical_path->attr.aux_in_attr.ana_gain = sys_cfg_const.record_aux_left_analog_gain;
            physical_path->attr.aux_in_attr.dig_gain = sys_cfg_const.record_aux_left_digital_gain;
            physical_path->attr.aux_in_attr.dig_boost_gain = sys_cfg_const.record_aux_left_digital_boost_gain;
            physical_path->attr.aux_in_attr.aux_in_external = false;
            physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.record_aux_left_sport_idx;
            physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
            physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.record_aux_left_sport_ch;
            physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.record_aux_left_polarity;
            physical_path++;
        }

        if (sys_cfg_const.record_aux_right_enable)
        {
            physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_AUX_IN;
            physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_AUX_IN;
            physical_path->attr.aux_in_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                     sys_cfg_const.record_aux_right_adc_ch;
            physical_path->attr.aux_in_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                      sys_cfg_const.record_aux_right_sel;
            physical_path->attr.aux_in_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                       sys_cfg_const.record_aux_right_type;
            physical_path->attr.aux_in_attr.mic_class = audio_route_mic_class_get(
                                                            physical_path->attr.aux_in_attr.mic_sel);
            physical_path->attr.aux_in_attr.equalizer_en = true;
            physical_path->attr.aux_in_attr.ana_gain = sys_cfg_const.record_aux_right_analog_gain;
            physical_path->attr.aux_in_attr.dig_gain = sys_cfg_const.record_aux_right_digital_gain;
            physical_path->attr.aux_in_attr.dig_boost_gain = sys_cfg_const.record_aux_right_digital_boost_gain;
            physical_path->attr.aux_in_attr.aux_in_external = false;
            physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                       sys_cfg_const.record_aux_right_sport_idx;
            physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
            physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.record_aux_right_sport_ch;
            physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.record_aux_right_polarity;
            physical_path++;
        }

        if (sys_cfg_const.record_primary_ref_mic_enable)
        {
            physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC;
            physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
            physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                  sys_cfg_const.record_primary_ref_mic_adc_ch;
            physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                   sys_cfg_const.record_primary_ref_mic_sel;
            physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                    sys_cfg_const.record_primary_ref_mic_type;
            physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                         physical_path->attr.mic_attr.mic_sel);
            physical_path->attr.mic_attr.equalizer_en = true;
            physical_path->attr.mic_attr.ana_gain = sys_cfg_const.record_primary_mic_analog_gain;
            physical_path->attr.mic_attr.dig_gain = sys_cfg_const.record_primary_mic_digital_gain;
            physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.record_primary_ref_mic_external;
            physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                       sys_cfg_const.record_primary_ref_mic_sport_idx;
            physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
            physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.record_primary_ref_mic_sport_ch;
            physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.record_primary_ref_mic_polarity;
            physical_path++;
        }

        if (sys_cfg_const.record_secondary_ref_mic_enable)
        {
            physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC;
            physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
            physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                  sys_cfg_const.record_secondary_ref_mic_adc_ch;
            physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                   sys_cfg_const.record_secondary_ref_mic_sel;
            physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                    sys_cfg_const.record_secondary_ref_mic_type;
            physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                         physical_path->attr.mic_attr.mic_sel);
            physical_path->attr.mic_attr.equalizer_en = true;
            physical_path->attr.mic_attr.ana_gain = sys_cfg_const.record_primary_mic_analog_gain;
            physical_path->attr.mic_attr.dig_gain = sys_cfg_const.record_primary_mic_digital_gain;
            physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.record_secondary_ref_mic_external;
            physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                       sys_cfg_const.record_primary_ref_mic_sport_idx;
            physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
            physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.record_secondary_ref_mic_sport_ch;
            physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)
                                      sys_cfg_const.record_secondary_ref_mic_polarity;
        }
    }
}

static uint8_t audio_route_ringtone_physical_path_num_get(void)
{
    uint8_t path_num = 0;

    path_num += sys_cfg_const.ringtone_primary_spk_enable;
    path_num += sys_cfg_const.ringtone_secondary_spk_enable;
    path_num += sys_cfg_const.ringtone_primary_ref_spk_enable;
    path_num += sys_cfg_const.ringtone_secondary_ref_spk_enable;

    return path_num;
}

static void audio_route_ringtone_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.ringtone_primary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;

        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.ringtone_primary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.ringtone_primary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.ringtone_primary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.ringtone_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.ringtone_primary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_0_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.ringtone_primary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.ringtone_primary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.ringtone_primary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.ringtone_secondary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.ringtone_secondary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.ringtone_secondary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.ringtone_secondary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.ringtone_secondary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.ringtone_secondary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_1_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.ringtone_secondary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.ringtone_secondary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.ringtone_secondary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.ringtone_primary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.ringtone_primary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.ringtone_primary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.ringtone_primary_ref_spk_dac_ch;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.ringtone_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.ringtone_primary_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.ringtone_primary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.ringtone_primary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.ringtone_primary_ref_spk_sport_ch;
        physical_path++;
    }

    if (sys_cfg_const.ringtone_secondary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.ringtone_secondary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.ringtone_secondary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.ringtone_secondary_ref_spk_dac_ch;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.ringtone_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.ringtone_primary_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.ringtone_secondary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.ringtone_secondary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.ringtone_secondary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)
                                  sys_cfg_const.ringtone_secondary_ref_spk_polarity;
    }
}

static uint8_t audio_route_vp_physical_path_num_get(void)
{
    uint8_t path_num = 0;

    path_num += sys_cfg_const.vp_primary_spk_enable;
    path_num += sys_cfg_const.vp_secondary_spk_enable;
    path_num += sys_cfg_const.vp_primary_ref_spk_enable;
    path_num += sys_cfg_const.vp_secondary_ref_spk_enable;

    return path_num;
}

static void audio_route_vp_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.vp_primary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.vp_primary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.vp_primary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.vp_primary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.vp_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.vp_primary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)sys_cfg_const.dac_0_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.vp_primary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.vp_primary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.vp_primary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.vp_secondary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.vp_secondary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.vp_secondary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.vp_secondary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.vp_secondary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.vp_secondary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)sys_cfg_const.dac_1_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.vp_secondary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.vp_secondary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.vp_secondary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.vp_primary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.vp_primary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.vp_primary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.vp_primary_ref_spk_dac_ch;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.vp_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.vp_primary_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.vp_primary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.vp_primary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.vp_primary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.vp_primary_ref_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.vp_secondary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.vp_secondary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.vp_secondary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.vp_secondary_ref_spk_dac_ch;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.vp_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.vp_primary_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.vp_secondary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.vp_secondary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.vp_secondary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.vp_secondary_ref_spk_polarity;
    }
}

static uint8_t audio_route_apt_physical_path_num_get(void)
{
    uint8_t path_num = 0;

    path_num += sys_cfg_const.apt_primary_spk_enable;
    path_num += sys_cfg_const.apt_secondary_spk_enable;
    path_num += sys_cfg_const.apt_primary_ref_spk_enable;
    path_num += sys_cfg_const.apt_secondary_ref_spk_enable;
    path_num += sys_cfg_const.apt_primary_mic_left_enable;
    path_num += sys_cfg_const.apt_primary_mic_right_enable;
    path_num += sys_cfg_const.apt_secondary_mic_left_enable;
    path_num += sys_cfg_const.apt_secondary_mic_right_enable;
    path_num += sys_cfg_const.apt_primary_ref_mic_enable;
    path_num += sys_cfg_const.apt_secondary_ref_mic_enable;

    return path_num;
}

static void audio_route_apt_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.apt_primary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.apt_primary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.apt_primary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.apt_primary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.apt_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.apt_primary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_0_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_primary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_primary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.apt_primary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.apt_secondary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.apt_secondary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.apt_secondary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.apt_secondary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.apt_secondary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.apt_secondary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_1_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_secondary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_secondary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.apt_secondary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.apt_primary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.apt_primary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.apt_primary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.apt_primary_ref_spk_dac_ch;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.apt_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.apt_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.apt_primary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_primary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_primary_ref_spk_sport_ch;
        physical_path++;
    }

    if (sys_cfg_const.apt_secondary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.apt_secondary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.apt_secondary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.apt_secondary_ref_spk_dac_ch;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.apt_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.apt_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.apt_secondary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_secondary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_secondary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.apt_primary_ref_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.apt_primary_mic_left_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.apt_primary_mic_left_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.apt_primary_mic_left_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.apt_primary_mic_left_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.apt_primary_mic_left_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.apt_primary_mic_left_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.apt_primary_mic_left_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_primary_mic_left_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_primary_mic_left_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.apt_primary_mic_left_polarity;
        physical_path++;
    }

    if (sys_cfg_const.apt_primary_mic_right_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.apt_primary_mic_right_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.apt_primary_mic_right_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.apt_primary_mic_right_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.apt_primary_mic_right_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.apt_primary_mic_right_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain =
            sys_cfg_const.apt_primary_mic_right_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_primary_mic_right_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_primary_mic_right_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.apt_primary_mic_right_polarity;
        physical_path++;
    }

    if (sys_cfg_const.apt_secondary_mic_left_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.apt_secondary_mic_left_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.apt_secondary_mic_left_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.apt_secondary_mic_left_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.apt_secondary_mic_left_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.apt_secondary_mic_left_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain =
            sys_cfg_const.apt_secondary_mic_left_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_secondary_mic_left_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_secondary_mic_left_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.apt_secondary_mic_left_polarity;
        physical_path++;
    }

    if (sys_cfg_const.apt_secondary_mic_right_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.apt_secondary_mic_right_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.apt_secondary_mic_right_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.apt_secondary_mic_right_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.apt_secondary_mic_right_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.apt_secondary_mic_right_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain =
            sys_cfg_const.apt_secondary_mic_right_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_secondary_mic_right_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_secondary_mic_right_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.apt_secondary_mic_right_polarity;
        physical_path++;
    }

    if (sys_cfg_const.apt_primary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.apt_primary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)sys_cfg_const.apt_primary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.apt_primary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.apt_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.apt_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.apt_primary_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.apt_primary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_primary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_primary_ref_mic_sport_ch;
        physical_path++;
    }

    if (sys_cfg_const.apt_secondary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.apt_secondary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.apt_secondary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.apt_secondary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.apt_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.apt_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.apt_primary_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.apt_secondary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.apt_secondary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.apt_secondary_ref_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.apt_secondary_ref_mic_polarity;
    }
}
#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)
static uint8_t audio_route_anc_physical_path_num_get(void)
{
    uint8_t path_num = 0;

    path_num += sys_cfg_const.anc_primary_spk_enable;
    path_num += sys_cfg_const.anc_secondary_spk_enable;
    path_num += sys_cfg_const.anc_primary_ref_spk_enable;
    path_num += sys_cfg_const.anc_secondary_ref_spk_enable;
    path_num += sys_cfg_const.anc_fb_mic_left_enable;
    path_num += sys_cfg_const.anc_fb_mic_right_enable;
    path_num += sys_cfg_const.anc_ff_mic_left_enable;
    path_num += sys_cfg_const.anc_ff_mic_right_enable;
    path_num += sys_cfg_const.anc_primary_ref_mic_enable;
    path_num += sys_cfg_const.anc_secondary_ref_mic_enable;

    return path_num;
}

static void audio_route_anc_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.anc_primary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.anc_primary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.anc_primary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.anc_primary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.anc_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.anc_primary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_0_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_primary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_primary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_primary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.anc_secondary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.anc_secondary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.anc_secondary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.anc_secondary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.anc_secondary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.anc_secondary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_1_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_secondary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_secondary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_secondary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.anc_primary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.anc_primary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.anc_primary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.anc_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.anc_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.anc_primary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_primary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_primary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_primary_ref_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.anc_secondary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.anc_secondary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.anc_secondary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.anc_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.anc_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.anc_secondary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_secondary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_secondary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_secondary_ref_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.anc_fb_mic_left_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_INTERNAL_MIC_LEFT;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.anc_fb_mic_left_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.anc_fb_mic_left_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)sys_cfg_const.anc_fb_mic_left_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.anc_fb_mic_left_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.anc_fb_mic_left_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.anc_fb_mic_left_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_fb_mic_left_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_fb_mic_left_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_fb_mic_left_polarity;
        physical_path++;
    }

    if (sys_cfg_const.anc_fb_mic_right_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_INTERNAL_MIC_RIGHT;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.anc_fb_mic_right_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.anc_fb_mic_right_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)sys_cfg_const.anc_fb_mic_right_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.anc_fb_mic_right_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.anc_fb_mic_right_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.anc_fb_mic_right_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_fb_mic_right_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_fb_mic_right_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_fb_mic_right_polarity;
        physical_path++;
    }

    if ((sys_cfg_const.anc_ff_mic_left_enable) &&
        ((audio_route_db->flag & AUDIO_ROUTE_ANC_FF_MIC) == AUDIO_ROUTE_ANC_FF_MIC))
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_LEFT;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.anc_ff_mic_left_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.anc_ff_mic_left_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)sys_cfg_const.anc_ff_mic_left_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.anc_ff_mic_left_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.anc_ff_mic_left_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.anc_ff_mic_left_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_ff_mic_left_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_ff_mic_left_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_ff_mic_left_polarity;
        physical_path++;
    }

    if ((sys_cfg_const.anc_ff_mic_right_enable) &&
        ((audio_route_db->flag & AUDIO_ROUTE_ANC_FF_MIC) == AUDIO_ROUTE_ANC_FF_MIC))
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_RIGHT;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.anc_ff_mic_right_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.anc_ff_mic_right_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)sys_cfg_const.anc_ff_mic_right_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.anc_ff_mic_right_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.anc_ff_mic_right_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.anc_ff_mic_right_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_ff_mic_right_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_ff_mic_right_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_ff_mic_right_polarity;
        physical_path++;
    }

    if (sys_cfg_const.anc_primary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.anc_primary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.anc_primary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.anc_primary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.anc_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.anc_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.anc_primary_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.anc_primary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_primary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_primary_ref_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_primary_ref_mic_polarity;
        physical_path++;
    }

    if (sys_cfg_const.anc_secondary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.anc_secondary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.anc_secondary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.anc_secondary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.anc_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.anc_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.anc_primary_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.anc_secondary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.anc_secondary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.anc_secondary_ref_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.anc_secondary_ref_mic_polarity;
    }
}
#endif
#if 0
static uint8_t audio_route_vad_physical_path_num_get(void)
{
    uint8_t path_num = 0;

    path_num += sys_cfg_const.vad_primary_mic_enable;
    path_num += sys_cfg_const.vad_primary_ref_mic_enable;
    path_num += sys_cfg_const.vad_secondary_ref_mic_enable;

    return path_num;
}

static void audio_route_vad_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.vad_primary_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.vad_primary_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.vad_primary_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)sys_cfg_const.vad_primary_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.vad_primary_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.vad_primary_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.vad_primary_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.vad_primary_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.vad_primary_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.vad_primary_mic_polarity;
        physical_path++;
    }

    if (sys_cfg_const.vad_primary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.vad_primary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)sys_cfg_const.vad_primary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.vad_primary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.vad_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.vad_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.vad_primary_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.vad_primary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.vad_primary_ref_mic_sport_ch;
        physical_path++;
    }

    if (sys_cfg_const.vad_primary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.vad_secondary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.vad_secondary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.vad_secondary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.vad_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.vad_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain =
            sys_cfg_const.vadvad_primary_ref_mic_digital_boost_gain_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.vad_secondary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.vad_secondary_ref_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.vad_secondary_ref_mic_polarity;
    }
}
#endif
static uint8_t audio_route_analog_physical_path_num_get(void)
{
    uint8_t path_num = 0;

    path_num += sys_cfg_const.analog_primary_spk_enable;
    path_num += sys_cfg_const.analog_secondary_spk_enable;
    path_num += sys_cfg_const.analog_primary_ref_spk_enable;
    path_num += sys_cfg_const.analog_secondary_ref_spk_enable;
    path_num += sys_cfg_const.analog_aux_left_enable;
    path_num += sys_cfg_const.analog_aux_right_enable;
    path_num += sys_cfg_const.analog_primary_ref_mic_enable;
    path_num += sys_cfg_const.analog_secondary_ref_mic_enable;

    return path_num;
}

static void audio_route_analog_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.analog_primary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;

        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.analog_primary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.analog_primary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.analog_primary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.analog_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.analog_primary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_0_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.analog_primary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.analog_primary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.analog_primary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.analog_secondary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;

        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.analog_secondary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.analog_secondary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.analog_secondary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.analog_secondary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.analog_secondary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_1_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.analog_secondary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.analog_secondary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.analog_secondary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.analog_primary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.analog_primary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.analog_primary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.analog_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.analog_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.analog_primary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.analog_primary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.analog_primary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.analog_primary_ref_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.analog_secondary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.analog_secondary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.analog_secondary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.analog_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.analog_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.analog_secondary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.analog_secondary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.analog_secondary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)
                                  sys_cfg_const.analog_secondary_ref_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.analog_aux_left_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_AUX_IN;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_AUX_IN;
        physical_path->attr.aux_in_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                 sys_cfg_const.analog_aux_left_adc_ch;
        physical_path->attr.aux_in_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                  sys_cfg_const.analog_aux_left_sel;
        physical_path->attr.aux_in_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                   sys_cfg_const.analog_aux_left_type;
        physical_path->attr.aux_in_attr.mic_class = audio_route_mic_class_get(
                                                        physical_path->attr.aux_in_attr.mic_sel);
        physical_path->attr.aux_in_attr.equalizer_en = true;
        physical_path->attr.aux_in_attr.ana_gain = sys_cfg_const.analog_aux_left_analog_gain;
        physical_path->attr.aux_in_attr.dig_gain = sys_cfg_const.analog_aux_left_digital_gain;
        physical_path->attr.aux_in_attr.dig_boost_gain = sys_cfg_const.analog_aux_left_digital_boost_gain;
        physical_path->attr.aux_in_attr.aux_in_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.analog_aux_left_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.analog_aux_left_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.analog_aux_left_polarity;
        physical_path++;
    }

    if (sys_cfg_const.analog_aux_right_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_AUX_IN;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_AUX_IN;
        physical_path->attr.aux_in_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                 sys_cfg_const.analog_aux_right_adc_ch;
        physical_path->attr.aux_in_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                  sys_cfg_const.analog_aux_right_sel;
        physical_path->attr.aux_in_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                   sys_cfg_const.analog_aux_right_type;
        physical_path->attr.aux_in_attr.mic_class = audio_route_mic_class_get(
                                                        physical_path->attr.aux_in_attr.mic_sel);
        physical_path->attr.aux_in_attr.equalizer_en = true;
        physical_path->attr.aux_in_attr.ana_gain = sys_cfg_const.analog_aux_right_analog_gain;
        physical_path->attr.aux_in_attr.dig_gain = sys_cfg_const.analog_aux_right_digital_gain;
        physical_path->attr.aux_in_attr.dig_boost_gain = sys_cfg_const.analog_aux_right_digital_boost_gain;
        physical_path->attr.aux_in_attr.aux_in_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.analog_aux_right_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.analog_aux_right_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.analog_aux_right_polarity;
        physical_path++;
    }

    if (sys_cfg_const.analog_primary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_AUX_IN;
        physical_path->attr.aux_in_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                 sys_cfg_const.analog_primary_ref_mic_adc_ch;
        physical_path->attr.aux_in_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                  sys_cfg_const.analog_primary_ref_mic_sel;
        physical_path->attr.aux_in_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                   sys_cfg_const.analog_primary_ref_mic_type;
        physical_path->attr.aux_in_attr.mic_class = audio_route_mic_class_get(
                                                        physical_path->attr.aux_in_attr.mic_sel);
        physical_path->attr.aux_in_attr.equalizer_en = true;
        physical_path->attr.aux_in_attr.ana_gain = sys_cfg_const.analog_primary_ref_mic_analog_gain;
        physical_path->attr.aux_in_attr.dig_gain = sys_cfg_const.analog_primary_ref_mic_digital_gain;
        physical_path->attr.aux_in_attr.dig_boost_gain =
            sys_cfg_const.analog_primary_ref_mic_digital_boost_gain;
        physical_path->attr.aux_in_attr.aux_in_external = (bool)
                                                          sys_cfg_const.analog_primary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.analog_primary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.analog_primary_ref_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.analog_primary_ref_mic_polarity;
        physical_path++;
    }

    if (sys_cfg_const.analog_secondary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_AUX_IN;
        physical_path->attr.aux_in_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                                 sys_cfg_const.analog_secondary_ref_mic_adc_ch;
        physical_path->attr.aux_in_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                                  sys_cfg_const.analog_secondary_ref_mic_sel;
        physical_path->attr.aux_in_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                   sys_cfg_const.analog_secondary_ref_mic_type;
        physical_path->attr.aux_in_attr.mic_class = audio_route_mic_class_get(
                                                        physical_path->attr.aux_in_attr.mic_sel);
        physical_path->attr.aux_in_attr.equalizer_en = true;
        physical_path->attr.aux_in_attr.ana_gain = sys_cfg_const.analog_primary_ref_mic_analog_gain;
        physical_path->attr.aux_in_attr.dig_gain = sys_cfg_const.analog_primary_ref_mic_digital_gain;
        physical_path->attr.aux_in_attr.dig_boost_gain =
            sys_cfg_const.analog_primary_ref_mic_digital_boost_gain;
        physical_path->attr.aux_in_attr.aux_in_external = (bool)
                                                          sys_cfg_const.analog_secondary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.analog_secondary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.analog_secondary_ref_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)
                                  sys_cfg_const.analog_secondary_ref_mic_polarity;
    }
}

static uint8_t audio_route_llapt_physical_path_num_get(void)
{
    uint8_t path_num = 0;

    path_num += sys_cfg_const.llapt_primary_spk_enable;
    path_num += sys_cfg_const.llapt_secondary_spk_enable;
    path_num += sys_cfg_const.llapt_primary_ref_spk_enable;
    path_num += sys_cfg_const.llapt_secondary_ref_spk_enable;
    path_num += sys_cfg_const.llapt_mic_left_enable;
    path_num += sys_cfg_const.llapt_mic_right_enable;
    path_num += sys_cfg_const.llapt_primary_ref_mic_enable;
    path_num += sys_cfg_const.llapt_secondary_ref_mic_enable;

    return path_num;
}

static void audio_route_llapt_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.llapt_primary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;

        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.llapt_primary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.llapt_primary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.llapt_primary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.llapt_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.llapt_primary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_0_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.llapt_primary_spk_sport_idx;;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.llapt_primary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.llapt_primary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.llapt_secondary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.llapt_secondary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.llapt_secondary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.llapt_secondary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.llapt_secondary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.llapt_secondary_spk_digital_gain;

        if (physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK1 ||
            physical_path->attr.spk_attr.spk_sel == AUDIO_ROUTE_SPK2)
        {
            physical_path->attr.spk_attr.spk_external = false;
        }
        else
        {
            physical_path->attr.spk_attr.spk_external = true;
        }

        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_1_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.llapt_secondary_spk_sport_idx;;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.llapt_secondary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.llapt_secondary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.llapt_primary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.llapt_primary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.llapt_primary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.llapt_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.llapt_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.llapt_primary_ref_spk_enable;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.llapt_primary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.llapt_primary_ref_spk_sport_ch;
        physical_path++;
    }

    if (sys_cfg_const.llapt_secondary_ref_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.llapt_secondary_ref_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.llapt_secondary_ref_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.llapt_primary_ref_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.llapt_primary_ref_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = (bool)sys_cfg_const.llapt_secondary_ref_spk_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)
                                   sys_cfg_const.llapt_secondary_ref_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)
                                  sys_cfg_const.llapt_secondary_ref_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.llapt_primary_ref_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.llapt_mic_left_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_LEFT;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.llapt_mic_left_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.llapt_mic_left_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)sys_cfg_const.llapt_mic_left_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.llapt_mic_left_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.llapt_mic_left_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.llapt_mic_left_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.llapt_mic_left_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.llapt_mic_left_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.llapt_mic_left_polarity;
        physical_path++;
    }

    if (sys_cfg_const.llapt_mic_right_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_RIGHT;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.llapt_mic_right_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.llapt_mic_right_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)sys_cfg_const.llapt_mic_right_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.llapt_mic_right_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.llapt_mic_right_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.llapt_mic_right_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.llapt_mic_right_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.llapt_mic_right_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.llapt_mic_right_polarity;
        physical_path++;
    }

    if (sys_cfg_const.llapt_primary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.llapt_primary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.llapt_primary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.llapt_primary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.llapt_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.llapt_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain =
            sys_cfg_const.llapt_primary_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.llapt_primary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.llapt_primary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.llapt_primary_ref_mic_sport_ch;
        physical_path++;
    }

    if (sys_cfg_const.llapt_secondary_ref_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_REF_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.llapt_secondary_ref_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.llapt_secondary_ref_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.llapt_secondary_ref_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = false;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.llapt_primary_ref_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.llapt_primary_ref_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain =
            sys_cfg_const.llapt_primary_ref_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = (bool)sys_cfg_const.llapt_secondary_ref_mic_external;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.llapt_secondary_ref_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.llapt_secondary_ref_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.llapt_primary_ref_mic_polarity;
    }
}

static uint8_t audio_route_sidetone_physical_path_num_get(void)
{
    uint8_t path_num;

    path_num = 0;

    path_num += sys_cfg_const.voice_primary_spk_enable;
    path_num += sys_cfg_const.voice_secondary_spk_enable;
    path_num += sys_cfg_const.voice_primary_mic_enable;

    return path_num;
}

static void audio_route_sidetone_physical_path_get(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    if (sys_cfg_const.voice_primary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.voice_primary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.voice_primary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.voice_primary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk1_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.voice_primary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.voice_primary_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = false;
        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_0_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_primary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_primary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_primary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_secondary_spk_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_SECONDARY_SPEAKER;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_SPEAKER;
        physical_path->attr.spk_attr.dac_ch = (T_AUDIO_ROUTE_DAC_CHANNEL)
                                              sys_cfg_const.voice_secondary_spk_dac_ch;
        physical_path->attr.spk_attr.spk_sel = (T_AUDIO_ROUTE_SPK_SEL)
                                               sys_cfg_const.voice_secondary_spk_sel;
        physical_path->attr.spk_attr.spk_type = (T_AUDIO_ROUTE_SPK_TYPE)
                                                sys_cfg_const.voice_secondary_spk_type;
        physical_path->attr.spk_attr.spk_class = (T_AUDIO_ROUTE_SPK_CLASS)sys_cfg_const.spk2_class;
        physical_path->attr.spk_attr.equalizer_en = true;
        physical_path->attr.spk_attr.ana_gain = sys_cfg_const.voice_secondary_spk_analog_gain;
        physical_path->attr.spk_attr.dig_gain = sys_cfg_const.voice_secondary_spk_digital_gain;
        physical_path->attr.spk_attr.spk_external = false;
        physical_path->attr.spk_attr.mix_en = (T_AUDIO_ROUTE_DAC_MIX_POINT)
                                              sys_cfg_const.dac_1_mix_point;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_secondary_spk_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_TX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_secondary_spk_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_secondary_spk_polarity;
        physical_path++;
    }

    if (sys_cfg_const.voice_primary_mic_enable)
    {
        physical_path->logic_io_type = AUDIO_ROUTE_LOGIC_PRIMARY_MIC;
        physical_path->physical_io_type = AUDIO_ROUTE_PHYSICAL_IO_MIC;
        physical_path->attr.mic_attr.adc_ch = (T_AUDIO_ROUTE_ADC_CHANNEL)
                                              sys_cfg_const.voice_primary_mic_adc_ch;
        physical_path->attr.mic_attr.mic_sel = (T_AUDIO_ROUTE_MIC_SEL)
                                               sys_cfg_const.voice_primary_mic_sel;
        physical_path->attr.mic_attr.mic_type = (T_AUDIO_ROUTE_MIC_TYPE)
                                                sys_cfg_const.voice_primary_mic_type;
        physical_path->attr.mic_attr.mic_class = audio_route_mic_class_get(
                                                     physical_path->attr.mic_attr.mic_sel);
        physical_path->attr.mic_attr.equalizer_en = true;
        physical_path->attr.mic_attr.ana_gain = sys_cfg_const.voice_primary_mic_analog_gain;
        physical_path->attr.mic_attr.dig_gain = sys_cfg_const.voice_primary_mic_digital_gain;
        physical_path->attr.mic_attr.dig_boost_gain = sys_cfg_const.voice_primary_mic_digital_boost_gain;
        physical_path->attr.mic_attr.mic_external = false;
        physical_path->sport_idx = (T_AUDIO_ROUTE_SPORT_IDX)sys_cfg_const.voice_primary_mic_sport_idx;
        physical_path->sport_ch_dir = AUDIO_ROUTE_SPORT_CH_DIR_RX;
        physical_path->sport_ch = (T_AUDIO_ROUTE_SPORT_CH)sys_cfg_const.voice_primary_mic_sport_ch;
        physical_path->polarity = (T_AUDIO_ROUTE_IO_POLARITY)sys_cfg_const.voice_primary_mic_polarity;
    }
}

uint8_t audio_route_physical_path_num_get(T_AUDIO_CATEGORY category)
{
    uint8_t path_num = 0;

    switch (category)
    {
    case AUDIO_CATEGORY_AUDIO:
        {
            path_num = audio_route_audio_physical_path_num_get();
        }
        break;

    case AUDIO_CATEGORY_VOICE:
        {
            path_num = audio_route_voice_physical_path_num_get();
        }
        break;

    case AUDIO_CATEGORY_RECORD:
        {
            path_num = audio_route_record_physical_path_num_get();
        }
        break;

    case AUDIO_CATEGORY_TONE:
        {
            path_num = audio_route_ringtone_physical_path_num_get();
        }
        break;

    case AUDIO_CATEGORY_VP:
        {
            path_num = audio_route_vp_physical_path_num_get();
        }
        break;

    case AUDIO_CATEGORY_APT:
        {
            path_num = audio_route_apt_physical_path_num_get();
        }
        break;
#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)
    case AUDIO_CATEGORY_ANC:
        {
            path_num = audio_route_anc_physical_path_num_get();
        }
        break;
#endif
#if 0
    case AUDIO_CATEGORY_VAD:
        {
            path_num = audio_route_vad_physical_path_num_get();
        }
        break;
#endif

    case AUDIO_CATEGORY_ANALOG:
        {
            path_num = audio_route_analog_physical_path_num_get();
        }
        break;

    case AUDIO_CATEGORY_LLAPT:
        {
            path_num = audio_route_llapt_physical_path_num_get();
        }
        break;

    case AUDIO_CATEGORY_SIDETONE:
        {
            path_num = audio_route_sidetone_physical_path_num_get();
        }
        break;

    default:
        break;
    }

    return path_num;
}

void audio_route_physical_path_get(T_AUDIO_CATEGORY category,
                                   T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    switch (category)
    {
    case AUDIO_CATEGORY_AUDIO:
        {
            audio_route_audio_physical_path_get(physical_path);
        }
        break;

    case AUDIO_CATEGORY_VOICE:
        {
            audio_route_voice_physical_path_get(physical_path);
        }
        break;

    case AUDIO_CATEGORY_RECORD:
        {
            audio_route_record_physical_path_get(physical_path);
        }
        break;

    case AUDIO_CATEGORY_TONE:
        {
            audio_route_ringtone_physical_path_get(physical_path);
        }
        break;

    case AUDIO_CATEGORY_VP:
        {
            audio_route_vp_physical_path_get(physical_path);
        }
        break;

    case AUDIO_CATEGORY_APT:
        {
            audio_route_apt_physical_path_get(physical_path);
        }
        break;
#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)
    case AUDIO_CATEGORY_ANC:
        {
            audio_route_anc_physical_path_get(physical_path);
        }
        break;
#endif
#if 0
    case AUDIO_CATEGORY_VAD:
        {
            audio_route_vad_physical_path_get(physical_path);
        }
        break;
#endif
    case AUDIO_CATEGORY_ANALOG:
        {
            audio_route_analog_physical_path_get(physical_path);
        }
        break;

    case AUDIO_CATEGORY_LLAPT:
        {
            audio_route_llapt_physical_path_get(physical_path);
        }
        break;

    case AUDIO_CATEGORY_SIDETONE:
        {
            audio_route_sidetone_physical_path_get(physical_path);
        }
        break;

    default:
        break;
    }
}

T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP audio_route_physical_path_take(T_AUDIO_CATEGORY category)
{
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP physical_path_group =
    {
        .physical_path = NULL,
        .physical_path_num = 0,
    };

    if (audio_route_db->register_group[category].physical_path != NULL)
    {
        physical_path_group.physical_path_num = audio_route_db->register_group[category].physical_path_num;
        physical_path_group.physical_path = audio_route_db->register_group[category].physical_path;
    }
    else
    {
        physical_path_group.physical_path_num = audio_route_physical_path_num_get(category);
        if (physical_path_group.physical_path_num != 0)
        {
            physical_path_group.physical_path = os_mem_zalloc2(sizeof(T_AUDIO_ROUTE_PHYSICAL_PATH) *
                                                               physical_path_group.physical_path_num);
            if (physical_path_group.physical_path != NULL)
            {
                audio_route_physical_path_get(category, physical_path_group.physical_path);
            }
        }
    }

    return physical_path_group;
}

static bool audio_route_is_registered_group(T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP *physical_path_group)
{
    for (int i = 0; i < AUDIO_CATEGORY_NUMBER; i++)
    {
        if (physical_path_group->physical_path == (audio_route_db->register_group[i]).physical_path)
        {
            return true;
        }
    }

    return false;
}

void audio_route_physical_path_give(T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP *physical_path_group)
{
    if (!audio_route_is_registered_group(physical_path_group))
    {
        if ((physical_path_group->physical_path_num != 0) &&
            (physical_path_group->physical_path != NULL))
        {
            os_mem_free(physical_path_group->physical_path);
        }
    }
}

bool audio_route_init(void)
{
    int32_t ret = 0;

    audio_route_db = os_mem_zalloc2(sizeof(T_AUDIO_ROUTE_DB));
    if (audio_route_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    audio_route_db->lvl_gain_cback = os_mem_zalloc2(AUDIO_CATEGORY_NUMBER * sizeof(T_LVL_GAIN_CBACK));
    if (audio_route_db->lvl_gain_cback == NULL)
    {
        ret = 2;
        goto fail_init_db;
    }

    sys_cfg_load();

    audio_route_db->record_dev = AUDIO_DEVICE_IN_MIC;
    audio_route_db->flag |= AUDIO_ROUTE_ANC_FF_MIC;
    return true;

fail_init_db:
    os_mem_free(audio_route_db);
    audio_route_db = NULL;
fail_alloc_db:
    AUDIO_PRINT_ERROR1("audio_route_init: failed %d", -ret);
    return false;
}

void audio_route_deinit(void)
{
    if (audio_route_db->lvl_gain_cback != NULL)
    {
        os_mem_free(audio_route_db->lvl_gain_cback);
    }

    if (audio_route_db != NULL)
    {
        os_mem_free(audio_route_db);
    }
}

bool audio_route_gain_register(T_AUDIO_CATEGORY category, P_AUDIO_ROUTE_DAC_GAIN_CBACK dac_cback,
                               P_AUDIO_ROUTE_ADC_GAIN_CBACK adc_cback)
{
    AUDIO_PRINT_TRACE3("audio_route_register_lvl_gain_cback: category 0x%2x, cback_adc_get %p, cback_dac_get %p",
                       category, adc_cback, dac_cback);

    if ((adc_cback == NULL) && (dac_cback == NULL))
    {
        return false;
    }
    else
    {
        if (adc_cback != NULL)
        {
            audio_route_db->lvl_gain_cback[category].cback_adc_get = adc_cback;
        }

        if (dac_cback != NULL)
        {
            audio_route_db->lvl_gain_cback[category].cback_dac_get = dac_cback;
        }
    }

    return true;
}

void audio_route_gain_unregister(T_AUDIO_CATEGORY category)
{
    audio_route_db->lvl_gain_cback[category].cback_adc_get = NULL;
    audio_route_db->lvl_gain_cback[category].cback_dac_get = NULL;
}

bool audio_route_sidetone_register(P_AUDIO_ROUTE_SIDETONE_CBACK cback)
{
    AUDIO_PRINT_TRACE1("audio_route_register_lvl_sidetone_cback: cback_sidetone_get %p",
                       cback);

    if (cback == NULL)
    {
        return false;
    }
    else
    {
        audio_route_db->lvl_sidetone_cback = cback;
    }

    return true;
}

void audio_route_sidetone_unregister(void)
{
    audio_route_db->lvl_sidetone_cback = NULL;
}

void audio_route_record_dev_set(uint32_t dev)
{
    audio_route_db->record_dev = dev;
}

uint32_t audio_route_record_dev_get(void)
{
    return audio_route_db->record_dev;
}

bool audio_route_adc_gain_get(T_AUDIO_CATEGORY category, uint32_t level,
                              T_AUDIO_ROUTE_ADC_GAIN *gain)
{
    if (audio_route_db->lvl_gain_cback[category].cback_adc_get != NULL)
    {
        return audio_route_db->lvl_gain_cback[category].cback_adc_get(category, level, gain);
    }
    else
    {
        gain->adc_gain = voice_audio_adc_gain_table[level];
    }
    return true;
}

bool audio_route_dac_gain_get(T_AUDIO_CATEGORY category, uint32_t level,
                              T_AUDIO_ROUTE_DAC_GAIN *gain)
{
    if (audio_route_db->lvl_gain_cback[category].cback_dac_get != NULL)
    {
        return audio_route_db->lvl_gain_cback[category].cback_dac_get(category, level, gain);
    }
    else if (category == AUDIO_CATEGORY_AUDIO)
    {
        gain->dac_gain = audio_dac_dig_gain_table[level];
    }
    else
    {
        gain->dac_gain = voice_dac_dig_gain_table[level];
    }

    return true;
}

bool audio_route_sidetone_info_get(T_AUDIO_ROUTE_SIDETONE_INFO *info)
{
    if (audio_route_db->lvl_sidetone_cback != NULL)
    {
        return audio_route_db->lvl_sidetone_cback(info);
    }

    return false;
}

bool audio_route_category_path_register(T_AUDIO_CATEGORY category,
                                        T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path,
                                        uint8_t physical_path_num)
{
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP *register_group;
    int32_t                            ret = 0;

    if (audio_route_db == NULL)
    {
        ret = 1;
        goto invaid_audio_route_db;
    }

    if (category >= AUDIO_CATEGORY_NUMBER)
    {
        ret = 2;
        goto invaid_category_num;
    }

    if (physical_path == NULL)
    {
        ret = 3;
        goto invaid_physical_path;
    }

    if (physical_path_num == 0)
    {
        ret = 4;
        goto invaid_physical_path_num;
    }

    register_group = &audio_route_db->register_group[category];
    if (register_group->physical_path != NULL)
    {
        ret = 5;
        goto fail_registered_path;
    }

    register_group->physical_path = os_mem_zalloc2(sizeof(T_AUDIO_ROUTE_PHYSICAL_PATH) *
                                                   physical_path_num);
    if (register_group->physical_path == NULL)
    {
        ret = 6;
        goto fail_alloc_path;
    }

    register_group->physical_path_num = physical_path_num;
    memcpy(register_group->physical_path, physical_path,
           sizeof(T_AUDIO_ROUTE_PHYSICAL_PATH) * physical_path_num);

    return true;

fail_alloc_path:
fail_registered_path:
invaid_physical_path_num:
invaid_physical_path:
invaid_category_num:
invaid_audio_route_db:
    AUDIO_PRINT_ERROR1("audio_route_category_path_register: failed %d", -ret);
    return false;
}

bool audio_route_category_path_unregister(T_AUDIO_CATEGORY category)
{
    int32_t    ret = 0;

    if (audio_route_db == NULL)
    {
        ret = 1;
        goto invaid_audio_route_db;
    }

    if (category >= AUDIO_CATEGORY_NUMBER)
    {
        ret = 2;
        goto invaid_category_num;
    }

    if (audio_route_db->register_group[category].physical_path == NULL)
    {
        ret = 3;
        goto fail_check_path;
    }

    os_mem_free(audio_route_db->register_group[category].physical_path);

    audio_route_db->register_group[category].physical_path_num = 0;
    audio_route_db->register_group[category].physical_path = NULL;

    return true;

fail_check_path:
invaid_category_num:
invaid_audio_route_db:
    AUDIO_PRINT_ERROR1("audio_route_category_path_unregister: failed %d", -ret);
    return false;
}

T_AUDIO_ROUTE_SPORT_CFG audio_route_sport_cfg_get(T_AUDIO_ROUTE_SPORT_IDX sport_id,
                                                  T_AUDIO_ROUTE_SPORT_CH_DIR direction)
{
    T_AUDIO_ROUTE_SPORT_CFG rtx_cfg;

    switch (sport_id)
    {
    case AUDIO_ROUTE_SPORT_0:
        {
            rtx_cfg.role = (T_AUDIO_ROUTE_SPORT_ROLE)sys_cfg_const.sport0_role;

            if (direction == AUDIO_ROUTE_SPORT_CH_DIR_TX)
            {
                rtx_cfg.bridge = (T_AUDIO_ROUTE_SPORT_BRIDGE)sys_cfg_const.sport0_tx_bridge;
                rtx_cfg.mode = (T_AUDIO_ROUTE_SPORT_MODE)sys_cfg_const.sport0_tx_mode;
                rtx_cfg.format = (T_AUDIO_ROUTE_SPORT_FORMAT)sys_cfg_const.sport0_tx_format;
                rtx_cfg.data_len = (T_AUDIO_ROUTE_SPORT_DATA_LEN)sys_cfg_const.sport0_tx_data_len;
                rtx_cfg.chann_len = (T_AUDIO_ROUTE_SPORT_CH_LEN)sys_cfg_const.sport0_tx_chann_len;
                rtx_cfg.sample_rate = sample_rate_map[sys_cfg_const.sport0_tx_sample_rate];
            }
            else
            {
                rtx_cfg.bridge = (T_AUDIO_ROUTE_SPORT_BRIDGE)sys_cfg_const.sport0_rx_bridge;
                rtx_cfg.mode = (T_AUDIO_ROUTE_SPORT_MODE)sys_cfg_const.sport0_rx_mode;
                rtx_cfg.format = (T_AUDIO_ROUTE_SPORT_FORMAT)sys_cfg_const.sport0_rx_format;
                rtx_cfg.data_len = (T_AUDIO_ROUTE_SPORT_DATA_LEN)sys_cfg_const.sport0_rx_data_len;
                rtx_cfg.chann_len = (T_AUDIO_ROUTE_SPORT_CH_LEN)sys_cfg_const.sport0_rx_chann_len;
                rtx_cfg.sample_rate = sample_rate_map[sys_cfg_const.sport0_rx_sample_rate];
            }
        }
        break;

    case AUDIO_ROUTE_SPORT_1:
        {
            rtx_cfg.role = (T_AUDIO_ROUTE_SPORT_ROLE)sys_cfg_const.sport1_role;

            if (direction == AUDIO_ROUTE_SPORT_CH_DIR_TX)
            {
                rtx_cfg.bridge = (T_AUDIO_ROUTE_SPORT_BRIDGE)sys_cfg_const.sport1_tx_bridge;
                rtx_cfg.mode = (T_AUDIO_ROUTE_SPORT_MODE)sys_cfg_const.sport1_tx_mode;
                rtx_cfg.format = (T_AUDIO_ROUTE_SPORT_FORMAT)sys_cfg_const.sport1_tx_format;
                rtx_cfg.data_len = (T_AUDIO_ROUTE_SPORT_DATA_LEN)sys_cfg_const.sport1_tx_data_len;
                rtx_cfg.chann_len = (T_AUDIO_ROUTE_SPORT_CH_LEN)sys_cfg_const.sport1_tx_chann_len;
                rtx_cfg.sample_rate = sample_rate_map[sys_cfg_const.sport1_tx_sample_rate];
            }
            else
            {
                rtx_cfg.bridge = (T_AUDIO_ROUTE_SPORT_BRIDGE)sys_cfg_const.sport1_rx_bridge;
                rtx_cfg.mode = (T_AUDIO_ROUTE_SPORT_MODE)sys_cfg_const.sport1_rx_mode;
                rtx_cfg.format = (T_AUDIO_ROUTE_SPORT_FORMAT)sys_cfg_const.sport1_rx_format;
                rtx_cfg.data_len = (T_AUDIO_ROUTE_SPORT_DATA_LEN)sys_cfg_const.sport1_rx_data_len;
                rtx_cfg.chann_len = (T_AUDIO_ROUTE_SPORT_CH_LEN)sys_cfg_const.sport1_rx_chann_len;
                rtx_cfg.sample_rate = sample_rate_map[sys_cfg_const.sport1_rx_sample_rate];
            }
        }
        break;

    case AUDIO_ROUTE_SPORT_2:
        {
            rtx_cfg.role = (T_AUDIO_ROUTE_SPORT_ROLE)sys_cfg_const.sport2_role;

            if (direction == AUDIO_ROUTE_SPORT_CH_DIR_TX)
            {
                rtx_cfg.bridge = (T_AUDIO_ROUTE_SPORT_BRIDGE)sys_cfg_const.sport2_tx_bridge;
                rtx_cfg.mode = (T_AUDIO_ROUTE_SPORT_MODE)sys_cfg_const.sport2_tx_mode;
                rtx_cfg.format = (T_AUDIO_ROUTE_SPORT_FORMAT)sys_cfg_const.sport2_tx_format;
                rtx_cfg.data_len = (T_AUDIO_ROUTE_SPORT_DATA_LEN)sys_cfg_const.sport2_tx_data_len;
                rtx_cfg.chann_len = (T_AUDIO_ROUTE_SPORT_CH_LEN)sys_cfg_const.sport2_tx_chann_len;
                rtx_cfg.sample_rate = sample_rate_map[sys_cfg_const.sport2_tx_sample_rate];
            }
            else
            {
                rtx_cfg.bridge = (T_AUDIO_ROUTE_SPORT_BRIDGE)sys_cfg_const.sport2_rx_bridge;
                rtx_cfg.mode = (T_AUDIO_ROUTE_SPORT_MODE)sys_cfg_const.sport2_rx_mode;
                rtx_cfg.format = (T_AUDIO_ROUTE_SPORT_FORMAT)sys_cfg_const.sport2_rx_format;
                rtx_cfg.data_len = (T_AUDIO_ROUTE_SPORT_DATA_LEN)sys_cfg_const.sport2_rx_data_len;
                rtx_cfg.chann_len = (T_AUDIO_ROUTE_SPORT_CH_LEN)sys_cfg_const.sport2_rx_chann_len;
                rtx_cfg.sample_rate = sample_rate_map[sys_cfg_const.sport2_rx_sample_rate];
            }
        }
        break;

    case AUDIO_ROUTE_SPORT_3:
        {
            rtx_cfg.role = (T_AUDIO_ROUTE_SPORT_ROLE)sys_cfg_const.sport3_role;

            if (direction == AUDIO_ROUTE_SPORT_CH_DIR_TX)
            {
                rtx_cfg.bridge = (T_AUDIO_ROUTE_SPORT_BRIDGE)sys_cfg_const.sport3_tx_bridge;
                rtx_cfg.mode = (T_AUDIO_ROUTE_SPORT_MODE)sys_cfg_const.sport3_tx_mode;
                rtx_cfg.format = (T_AUDIO_ROUTE_SPORT_FORMAT)sys_cfg_const.sport3_tx_format;
                rtx_cfg.data_len = (T_AUDIO_ROUTE_SPORT_DATA_LEN)sys_cfg_const.sport3_tx_data_len;
                rtx_cfg.chann_len = (T_AUDIO_ROUTE_SPORT_CH_LEN)sys_cfg_const.sport3_tx_chann_len;
                rtx_cfg.sample_rate = sample_rate_map[sys_cfg_const.sport3_tx_sample_rate];
            }
            else
            {
                rtx_cfg.bridge = (T_AUDIO_ROUTE_SPORT_BRIDGE)sys_cfg_const.sport3_rx_bridge;
                rtx_cfg.mode = (T_AUDIO_ROUTE_SPORT_MODE)sys_cfg_const.sport3_rx_mode;
                rtx_cfg.format = (T_AUDIO_ROUTE_SPORT_FORMAT)sys_cfg_const.sport3_rx_format;
                rtx_cfg.data_len = (T_AUDIO_ROUTE_SPORT_DATA_LEN)sys_cfg_const.sport3_rx_data_len;
                rtx_cfg.chann_len = (T_AUDIO_ROUTE_SPORT_CH_LEN)sys_cfg_const.sport3_rx_chann_len;
                rtx_cfg.sample_rate = sample_rate_map[sys_cfg_const.sport3_rx_sample_rate];
            }
        }
        break;

    default:
        {
            memset(&rtx_cfg, 0, sizeof(T_AUDIO_ROUTE_SPORT_CFG));
        }
        break;
    }
    return rtx_cfg;
}

int16_t audio_route_ramp_gain_get(T_AUDIO_CATEGORY category)
{
    int16_t ramp_gain;

    switch (category)
    {
    case AUDIO_CATEGORY_AUDIO:
    case AUDIO_CATEGORY_TONE:
    case AUDIO_CATEGORY_VP:
        {
            ramp_gain = (int16_t)sys_cfg_const.audio_gain_step_left;
        }
        break;

    case AUDIO_CATEGORY_VOICE:
        {
            ramp_gain = (int16_t)sys_cfg_const.voice_gain_step_left;
        }
        break;

    case AUDIO_CATEGORY_ANALOG:
        {
            ramp_gain = (int16_t)sys_cfg_const.analog_gain_step_left;
        }
        break;

    case AUDIO_CATEGORY_APT:
        {
            ramp_gain = (int16_t)sys_cfg_const.apt_gain_step_left;
        }
        break;

    default:
        {
            ramp_gain = (int16_t)sys_cfg_const.audio_gain_step_left;
        }
        break;
    }

    return ramp_gain;
}

bool audio_route_logic_io_enable(T_AUDIO_CATEGORY category, T_AUDIO_ROUTE_LOGIC_IO_TYPE io)
{
    bool ret;

    ret = false;

    if ((category == AUDIO_CATEGORY_ANC) &&
        (io == AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_LEFT))
    {
        audio_route_db->flag |= AUDIO_ROUTE_ANC_FF_MIC;
        ret = true;
    }

    return ret;
}

bool audio_route_logic_io_disable(T_AUDIO_CATEGORY category, T_AUDIO_ROUTE_LOGIC_IO_TYPE io)
{
    bool ret;

    ret = false;

    if ((category == AUDIO_CATEGORY_ANC) &&
        (io == AUDIO_ROUTE_LOGIC_EXTERNAL_MIC_LEFT))
    {
        audio_route_db->flag &= (~AUDIO_ROUTE_ANC_FF_MIC);
        ret = true;
    }

    return ret;
}

#if F_APP_EXT_MIC_SWITCH_SUPPORT
bool flash_dsp_get_voice_sec_mic_cfg(void)
{
    return sys_cfg_const.voice_secondary_mic_enable;
}

void flash_dsp_set_voice_sec_mic_cfg(bool enable)
{
    sys_cfg_const.voice_secondary_mic_enable = enable;
}

void flash_dsp_set_record_sec_mic_cfg(bool enable)
{
    sys_cfg_const.record_secondary_mic_enable = enable;
}
#endif
