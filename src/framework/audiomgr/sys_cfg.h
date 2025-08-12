/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _SYS_CFG_H_
#define _SYS_CFG_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_DATA_SYNC_WORD      0xAA55
#define APP_DATA_SYNC_WORD_LEN  2
#define APP_DATA_TONE_NUM_LEN   2

#define DATA_SYNC_WORD          0xAA55AA55
#define DATA_SYNC_WORD_LEN      4

#define DSP_PARAM_OFFSET        (0)
#define DSP_PARAM_SIZE          2304
#define DSP_INIT_OFFSET         DSP_PARAM_OFFSET + DSP_PARAM_SIZE
#define DSP_INIT_SIZE           256

#define APP_CONFIG_OFFSET       (0)
#define APP_CONFIG_SIZE         1024
#define APP_LED_OFFSET          (APP_CONFIG_OFFSET + APP_CONFIG_SIZE)
#define APP_LED_SIZE            512

#define SYS_CONFIG_OFFSET       (APP_LED_OFFSET + APP_LED_SIZE)
#define SYS_CONFIG_SIZE         512

#define TONE_DATA_OFFSET        (4096)
#define TONE_DATA_SIZE          3072
#define VOICE_PROMPT_OFFSET     (TONE_DATA_OFFSET + TONE_DATA_SIZE)
#define VOICE_PROMPT_SIZE       137 * 1024

/** @brief  Read only configurations for inbox audio application. */
typedef struct
{
    //Header: 4 bytes
    uint32_t sync_word;

    uint16_t audio_secondary_ref_spk_enable: 1;
    uint16_t audio_secondary_ref_spk_external: 1;
    uint16_t audio_secondary_ref_spk_sel: 2;
    uint16_t audio_secondary_ref_spk_type: 2;
    uint16_t audio_secondary_ref_spk_dac_ch: 3;
    uint16_t audio_secondary_ref_spk_sport_idx: 3;
    uint16_t audio_secondary_ref_spk_sport_ch: 3;
    uint16_t audio_secondary_ref_spk_sport_dir: 1;

    uint16_t voice_secondary_ref_spk_enable: 1;
    uint16_t voice_secondary_ref_spk_external: 1;
    uint16_t voice_secondary_ref_spk_sel: 2;
    uint16_t voice_secondary_ref_spk_type: 2;
    uint16_t voice_secondary_ref_spk_dac_ch: 3;
    uint16_t voice_secondary_ref_spk_sport_idx: 3;
    uint16_t voice_secondary_ref_spk_sport_ch: 3;
    uint16_t voice_secondary_ref_spk_sport_dir: 1;

    uint16_t analog_secondary_ref_spk_enable: 1;
    uint16_t analog_secondary_ref_spk_external: 1;
    uint16_t analog_secondary_ref_spk_sel: 2;
    uint16_t analog_secondary_ref_spk_type: 2;
    uint16_t analog_secondary_ref_spk_dac_ch: 3;
    uint16_t analog_secondary_ref_spk_sport_idx: 3;
    uint16_t analog_secondary_ref_spk_sport_ch: 3;
    uint16_t analog_secondary_ref_spk_sport_dir: 1;

    uint16_t ringtone_secondary_ref_spk_enable: 1;
    uint16_t ringtone_secondary_ref_spk_external: 1;
    uint16_t ringtone_secondary_ref_spk_sel: 2;
    uint16_t ringtone_secondary_ref_spk_type: 2;
    uint16_t ringtone_secondary_ref_spk_dac_ch: 3;
    uint16_t ringtone_secondary_ref_spk_sport_idx: 3;
    uint16_t ringtone_secondary_ref_spk_sport_ch: 3;
    uint16_t ringtone_secondary_ref_spk_sport_dir: 1;

    uint16_t vp_secondary_ref_spk_enable: 1;
    uint16_t vp_secondary_ref_spk_external: 1;
    uint16_t vp_secondary_ref_spk_sel: 2;
    uint16_t vp_secondary_ref_spk_type: 2;
    uint16_t vp_secondary_ref_spk_dac_ch: 3;
    uint16_t vp_secondary_ref_spk_sport_idx: 3;
    uint16_t vp_secondary_ref_spk_sport_ch: 3;
    uint16_t vp_secondary_ref_spk_sport_dir: 1;

    uint16_t apt_secondary_ref_spk_enable: 1;
    uint16_t apt_secondary_ref_spk_external: 1;
    uint16_t apt_secondary_ref_spk_sel: 2;
    uint16_t apt_secondary_ref_spk_type: 2;
    uint16_t apt_secondary_ref_spk_dac_ch: 3;
    uint16_t apt_secondary_ref_spk_sport_idx: 3;
    uint16_t apt_secondary_ref_spk_sport_ch: 3;
    uint16_t apt_secondary_ref_spk_sport_dir: 1;

    uint16_t llapt_secondary_ref_spk_enable: 1;
    uint16_t llapt_secondary_ref_spk_external: 1;
    uint16_t llapt_secondary_ref_spk_sel: 2;
    uint16_t llapt_secondary_ref_spk_type: 2;
    uint16_t llapt_secondary_ref_spk_dac_ch: 3;
    uint16_t llapt_secondary_ref_spk_sport_idx: 3;
    uint16_t llapt_secondary_ref_spk_sport_ch: 3;
    uint16_t llapt_secondary_ref_spk_sport_dir: 1;

    uint16_t anc_secondary_ref_spk_enable: 1;
    uint16_t anc_secondary_ref_spk_external: 1;
    uint16_t anc_secondary_ref_spk_sel: 2;
    uint16_t anc_secondary_ref_spk_type: 2;
    uint16_t anc_secondary_ref_spk_dac_ch: 3;
    uint16_t anc_secondary_ref_spk_sport_idx: 3;
    uint16_t anc_secondary_ref_spk_sport_ch: 3;
    uint16_t anc_secondary_ref_spk_sport_dir: 1;

    uint16_t voice_secondary_ref_mic_enable: 1;
    uint16_t voice_secondary_ref_mic_external: 1;
    uint16_t voice_secondary_ref_mic_sel: 2;
    uint16_t voice_secondary_ref_mic_type: 2;
    uint16_t voice_secondary_ref_mic_adc_ch: 3;
    uint16_t voice_secondary_ref_mic_sport_idx: 3;
    uint16_t voice_secondary_ref_mic_sport_ch: 3;
    uint16_t voice_secondary_ref_mic_sport_dir: 1;

    uint16_t record_secondary_ref_mic_enable: 1;
    uint16_t record_secondary_ref_mic_external: 1;
    uint16_t record_secondary_ref_mic_sel: 2;
    uint16_t record_secondary_ref_mic_type: 2;
    uint16_t record_secondary_ref_mic_adc_ch: 3;
    uint16_t record_secondary_ref_mic_sport_idx: 3;
    uint16_t record_secondary_ref_mic_sport_ch: 3;
    uint16_t record_secondary_ref_mic_sport_dir: 1;

    uint16_t analog_secondary_ref_mic_enable: 1;
    uint16_t analog_secondary_ref_mic_external: 1;
    uint16_t analog_secondary_ref_mic_sel: 2;
    uint16_t analog_secondary_ref_mic_type: 2;
    uint16_t analog_secondary_ref_mic_adc_ch: 3;
    uint16_t analog_secondary_ref_mic_sport_idx: 3;
    uint16_t analog_secondary_ref_mic_sport_ch: 3;
    uint16_t analog_secondary_ref_mic_sport_dir: 1;

    uint16_t apt_secondary_ref_mic_enable: 1;
    uint16_t apt_secondary_ref_mic_external: 1;
    uint16_t apt_secondary_ref_mic_sel: 2;
    uint16_t apt_secondary_ref_mic_type: 2;
    uint16_t apt_secondary_ref_mic_adc_ch: 3;
    uint16_t apt_secondary_ref_mic_sport_idx: 3;
    uint16_t apt_secondary_ref_mic_sport_ch: 3;
    uint16_t apt_secondary_ref_mic_sport_dir: 1;

    uint16_t llapt_secondary_ref_mic_enable: 1;
    uint16_t llapt_secondary_ref_mic_external: 1;
    uint16_t llapt_secondary_ref_mic_sel: 2;
    uint16_t llapt_secondary_ref_mic_type: 2;
    uint16_t llapt_secondary_ref_mic_adc_ch: 3;
    uint16_t llapt_secondary_ref_mic_sport_idx: 3;
    uint16_t llapt_secondary_ref_mic_sport_ch: 3;
    uint16_t llapt_secondary_ref_mic_sport_dir: 1;

    uint16_t anc_secondary_ref_mic_enable: 1;
    uint16_t anc_secondary_ref_mic_external: 1;
    uint16_t anc_secondary_ref_mic_sel: 2;
    uint16_t anc_secondary_ref_mic_type: 2;
    uint16_t anc_secondary_ref_mic_adc_ch: 3;
    uint16_t anc_secondary_ref_mic_sport_idx: 3;
    uint16_t anc_secondary_ref_mic_sport_ch: 3;
    uint16_t anc_secondary_ref_mic_sport_dir: 1;

    uint8_t rsvd1[4];

    //Audio HAL option1
    uint8_t dmic1_clk_pinmux;
    uint8_t dmic1_dat_pinmux;
    uint8_t dmic2_clk_pinmux;
    uint8_t dmic2_dat_pinmux;
    uint8_t dmic3_clk_pinmux;
    uint8_t dmic3_dat_pinmux;
    uint8_t micbias_pinmux;
    uint8_t micbias_gpio_pinmux;
    uint8_t pinmux_rsvd2;
    uint8_t sport0_tx_bridge   : 1;
    uint8_t sport0_rx_bridge   : 1;
    uint8_t sport1_tx_bridge   : 1;
    uint8_t sport1_rx_bridge   : 1;
    uint8_t sport2_tx_bridge   : 1;
    uint8_t sport2_rx_bridge   : 1;
    uint8_t sport3_tx_bridge   : 1;
    uint8_t sport3_rx_bridge   : 1;

    /*DAC_0 && DAC_1 mix point attribute*/
    uint8_t dac_0_mix_point    : 2;
    uint8_t dac_1_mix_point    : 2;
    uint8_t dac_2_mix_point    : 2;
    uint8_t dac_3_mix_point    : 2;

    //Audio HAL option3
    uint8_t rsvd_spk_type : 2; //(0: single-end, 1: differential, 2: Capless)
    uint8_t rsvd_aux_in_channel : 1; //(0: Mono, 1: Stereo)
    uint8_t dsp_log_output_select : 2;
    uint8_t mic2_aux_co_pad  : 1;
    uint8_t dre_control_sel: 1;
    uint8_t rsvd_audio_hal_opt3: 1;

    //Audio SPK class type and Power Mode
    uint8_t spk1_class: 1;// 0: class D 1: class AB
    uint8_t spk2_class: 1;// 0: class D 1: class AB
    uint8_t amic1_class: 2;// 0: normal mode 1: low power mode, for AMIC1,2
    uint8_t amic2_class: 2;// 0: normal mode 1: low power mode, for AMIC1,2
    uint8_t amic3_class: 2;//0: normal mode 1: HSNR mode, for AMIC3,4
    uint8_t amic4_class: 2;//0: normal mode 1: HSNR mode, for AMIC3,4
    uint8_t rsvd3:       6;
    uint8_t rsvd4[2];

    //Audio HAL option5
    uint32_t dmic1_clock: 3;
    uint32_t dmic2_clock: 3;
    uint32_t dmic3_clock: 3;
    uint32_t rsvd_amb_primary_mic_sel: 3;
    uint32_t rsvd_amb_primary_mic_type: 2;
    uint32_t rsvd_amb_secondary_mic_sel: 3;
    uint32_t rsvd_amb_secondary_mic_type: 2;
    uint32_t micbias_voltage_sel: 3;
    uint32_t micbias_ldo_ctr: 1;
    uint32_t avcc_drv_voltage_sel: 2;
    uint32_t rsvd_audio_hal_opt5: 7;

    uint8_t voice_prompt_index_num;   // Maximum voice prompt number for a single language
    uint8_t voice_prompt_language_num; // Number of configured supported voice prompt languages
    uint8_t rsvd5;
    uint8_t voice_prompt_support_language; //Bit0: English, Bit1: Chinese, Bit2: Franch, Bit3: Portuguese

    uint8_t audio_primary_spk_polarity: 1;
    uint8_t audio_secondary_spk_polarity: 1;
    uint8_t audio_primary_ref_spk_polarity: 1;
    uint8_t audio_secondary_ref_spk_polarity: 1;
    uint8_t voice_primary_spk_polarity: 1;
    uint8_t voice_secondary_spk_polarity: 1;
    uint8_t voice_primary_ref_spk_polarity: 1;
    uint8_t voice_secondary_ref_spk_polarity: 1;
    uint8_t voice_primary_ref_mic_polarity: 1;
    uint8_t voice_secondary_ref_mic_polarity: 1;
    uint8_t voice_primary_mic_polarity: 1;
    uint8_t voice_secondary_mic_polarity: 1;
    uint8_t voice_fusion_mic_polarity: 1;
    uint8_t voice_bone_mic_polarity: 1;
    uint8_t record_primary_ref_mic_polarity: 1;
    uint8_t record_secondary_ref_mic_polarity: 1;
    uint8_t record_primary_mic_polarity: 1;
    uint8_t record_secondary_mic_polarity: 1;
    uint8_t record_fusion_mic_polarity: 1;
    uint8_t record_bone_mic_polarity: 1;
    uint8_t record_aux_left_polarity: 1;
    uint8_t record_aux_right_polarity: 1;
    uint8_t analog_primary_spk_polarity: 1;
    uint8_t analog_secondary_spk_polarity: 1;
    uint8_t analog_primary_ref_spk_polarity: 1;
    uint8_t analog_secondary_ref_spk_polarity: 1;
    uint8_t analog_primary_ref_mic_polarity: 1;
    uint8_t analog_secondary_ref_mic_polarity: 1;
    uint8_t analog_aux_left_polarity: 1;
    uint8_t analog_aux_right_polarity: 1;
    uint8_t ringtone_primary_spk_polarity: 1;
    uint8_t ringtone_secondary_spk_polarity: 1;
    uint8_t ringtone_primary_ref_spk_polarity: 1;
    uint8_t ringtone_secondary_ref_spk_polarity: 1;
    uint8_t vp_primary_spk_polarity: 1;
    uint8_t vp_secondary_spk_polarity: 1;
    uint8_t vp_primary_ref_spk_polarity: 1;
    uint8_t vp_secondary_ref_spk_polarity: 1;
    uint8_t apt_primary_spk_polarity: 1;
    uint8_t apt_secondary_spk_polarity: 1;
    uint8_t apt_primary_ref_spk_polarity: 1;
    uint8_t apt_secondary_ref_spk_polarity: 1;
    uint8_t apt_primary_ref_mic_polarity: 1;
    uint8_t apt_secondary_ref_mic_polarity: 1;
    uint8_t apt_primary_mic_left_polarity: 1;
    uint8_t apt_primary_mic_right_polarity: 1;
    uint8_t apt_secondary_mic_left_polarity: 1;
    uint8_t apt_secondary_mic_right_polarity: 1;
    uint8_t llapt_primary_spk_polarity: 1;
    uint8_t llapt_secondary_spk_polarity: 1;
    uint8_t llapt_primary_ref_spk_polarity: 1;
    uint8_t llapt_secondary_ref_spk_polarity: 1;
    uint8_t llapt_primary_ref_mic_polarity: 1;
    uint8_t llapt_secondary_ref_mic_polarity: 1;
    uint8_t llapt_mic_left_polarity: 1;
    uint8_t llapt_mic_right_polarity: 1;
    uint8_t anc_primary_spk_polarity: 1;
    uint8_t anc_secondary_spk_polarity: 1;
    uint8_t anc_primary_ref_spk_polarity: 1;
    uint8_t anc_secondary_ref_spk_polarity: 1;
    uint8_t anc_primary_ref_mic_polarity: 1;
    uint8_t anc_secondary_ref_mic_polarity: 1;
    uint8_t anc_ff_mic_left_polarity: 1;
    uint8_t anc_ff_mic_right_polarity: 1;
    uint8_t anc_fb_mic_left_polarity: 1;
    uint8_t anc_fb_mic_right_polarity: 1;
    uint8_t vad_primary_ref_mic_polarity: 1;
    uint8_t vad_secondary_ref_mic_polarity: 1;
    uint8_t vad_primary_mic_polarity: 1;
    uint8_t vad_secondary_mic_polarity: 1;
    uint8_t polarity_rsvd1: 2;
    uint8_t polarity_rsvd2: 8;

    // category - audio
    uint16_t audio_primary_spk_enable: 1;
    uint16_t audio_primary_spk_sel: 3;
    uint16_t audio_primary_spk_type: 2;
    uint16_t audio_primary_spk_dac_ch: 3;
    uint16_t audio_primary_spk_sport_idx: 3;
    uint16_t audio_primary_spk_sport_ch: 3;
    uint16_t audio_primary_spk_sport_dir: 1;

    uint16_t audio_secondary_spk_enable: 1;
    uint16_t audio_secondary_spk_sel: 3;
    uint16_t audio_secondary_spk_type: 2;
    uint16_t audio_secondary_spk_dac_ch: 3;
    uint16_t audio_secondary_spk_sport_idx: 3;
    uint16_t audio_secondary_spk_sport_ch: 3;
    uint16_t audio_secondary_spk_sport_dir: 1;

    // category - voice
    uint16_t voice_primary_spk_enable: 1;
    uint16_t voice_primary_spk_sel: 3;
    uint16_t voice_primary_spk_type: 2;
    uint16_t voice_primary_spk_dac_ch: 3;
    uint16_t voice_primary_spk_sport_idx: 3;
    uint16_t voice_primary_spk_sport_ch: 3;
    uint16_t voice_primary_spk_sport_dir: 1;

    uint16_t voice_secondary_spk_enable: 1;
    uint16_t voice_secondary_spk_sel: 3;
    uint16_t voice_secondary_spk_type: 2;
    uint16_t voice_secondary_spk_dac_ch: 3;
    uint16_t voice_secondary_spk_sport_idx: 3;
    uint16_t voice_secondary_spk_sport_ch: 3;
    uint16_t voice_secondary_spk_sport_dir: 1;

    uint16_t voice_primary_ref_spk_enable: 1;
    uint16_t voice_primary_ref_spk_external: 1;
    uint16_t voice_primary_ref_spk_sel: 2;
    uint16_t voice_primary_ref_spk_type: 2;
    uint16_t voice_primary_ref_spk_dac_ch: 3;
    uint16_t voice_primary_ref_spk_sport_idx: 3;
    uint16_t voice_primary_ref_spk_sport_ch: 3;
    uint16_t voice_primary_ref_spk_sport_dir: 1;

    uint16_t voice_primary_ref_mic_enable: 1;
    uint16_t voice_primary_ref_mic_external: 1;
    uint16_t voice_primary_ref_mic_sel: 2;
    uint16_t voice_primary_ref_mic_type: 2;
    uint16_t voice_primary_ref_mic_adc_ch: 3;
    uint16_t voice_primary_ref_mic_sport_idx: 3;
    uint16_t voice_primary_ref_mic_sport_ch: 3;
    uint16_t voice_primary_ref_mic_sport_dir: 1;

    uint16_t voice_primary_mic_enable: 1;
    uint16_t voice_primary_mic_sel: 3;
    uint16_t voice_primary_mic_type: 2;
    uint16_t voice_primary_mic_adc_ch: 3;
    uint16_t voice_primary_mic_sport_idx: 3;
    uint16_t voice_primary_mic_sport_ch: 3;
    uint16_t voice_primary_mic_sport_dir: 1;

    uint16_t voice_secondary_mic_enable: 1;
    uint16_t voice_secondary_mic_sel: 3;
    uint16_t voice_secondary_mic_type: 2;
    uint16_t voice_secondary_mic_adc_ch: 3;
    uint16_t voice_secondary_mic_sport_idx: 3;
    uint16_t voice_secondary_mic_sport_ch: 3;
    uint16_t voice_secondary_mic_sport_dir: 1;

    uint16_t voice_fusion_mic_enable: 1;
    uint16_t voice_fusion_mic_sel: 3;
    uint16_t voice_fusion_mic_type: 2;
    uint16_t voice_fusion_mic_adc_ch: 3;
    uint16_t voice_fusion_mic_sport_idx: 3;
    uint16_t voice_fusion_mic_sport_ch: 3;
    uint16_t voice_fusion_mic_sport_dir: 1;

    uint16_t voice_bone_mic_enable: 1;
    uint16_t voice_bone_mic_sel: 3;
    uint16_t voice_bone_mic_type: 2;
    uint16_t voice_bone_mic_adc_ch: 3;
    uint16_t voice_bone_mic_sport_idx: 3;
    uint16_t voice_bone_mic_sport_ch: 3;
    uint16_t voice_bone_mic_sport_dir: 1;

    // category - record
    uint16_t record_primary_mic_enable: 1;
    uint16_t record_primary_mic_sel: 3;
    uint16_t record_primary_mic_type: 2;
    uint16_t record_primary_mic_adc_ch: 3;
    uint16_t record_primary_mic_sport_idx: 3;
    uint16_t record_primary_mic_sport_ch: 3;
    uint16_t record_primary_mic_sport_dir: 1;

    uint16_t record_secondary_mic_enable: 1;
    uint16_t record_secondary_mic_sel: 3;
    uint16_t record_secondary_mic_type: 2;
    uint16_t record_secondary_mic_adc_ch: 3;
    uint16_t record_secondary_mic_sport_idx: 3;
    uint16_t record_secondary_mic_sport_ch: 3;
    uint16_t record_secondary_mic_sport_dir: 1;

    uint16_t record_aux_left_enable: 1;
    uint16_t record_aux_left_sel: 3;
    uint16_t record_aux_left_type: 2;
    uint16_t record_aux_left_adc_ch: 3;
    uint16_t record_aux_left_sport_idx: 3;
    uint16_t record_aux_left_sport_ch: 3;
    uint16_t record_aux_left_sport_dir: 1;

    uint16_t record_aux_right_enable: 1;
    uint16_t record_aux_right_sel: 3;
    uint16_t record_aux_right_type: 2;
    uint16_t record_aux_right_adc_ch: 3;
    uint16_t record_aux_right_sport_idx: 3;
    uint16_t record_aux_right_sport_ch: 3;
    uint16_t record_aux_right_sport_dir: 1;

    // category - analog
    uint16_t analog_primary_spk_enable: 1;
    uint16_t analog_primary_spk_sel: 3;
    uint16_t analog_primary_spk_type: 2;
    uint16_t analog_primary_spk_dac_ch: 3;
    uint16_t analog_primary_spk_sport_idx: 3;
    uint16_t analog_primary_spk_sport_ch: 3;
    uint16_t analog_primary_spk_sport_dir: 1;

    uint16_t analog_secondary_spk_enable: 1;
    uint16_t analog_secondary_spk_sel: 3;
    uint16_t analog_secondary_spk_type: 2;
    uint16_t analog_secondary_spk_dac_ch: 3;
    uint16_t analog_secondary_spk_sport_idx: 3;
    uint16_t analog_secondary_spk_sport_ch: 3;
    uint16_t analog_secondary_spk_sport_dir: 1;

    uint16_t analog_primary_ref_spk_enable: 1;
    uint16_t analog_primary_ref_spk_external: 1;
    uint16_t analog_primary_ref_spk_sel: 2;
    uint16_t analog_primary_ref_spk_type: 2;
    uint16_t analog_primary_ref_spk_dac_ch: 3;
    uint16_t analog_primary_ref_spk_sport_idx: 3;
    uint16_t analog_primary_ref_spk_sport_ch: 3;
    uint16_t analog_primary_ref_spk_sport_dir: 1;

    uint16_t analog_primary_ref_mic_enable: 1;
    uint16_t analog_primary_ref_mic_external: 1;
    uint16_t analog_primary_ref_mic_sel: 2;
    uint16_t analog_primary_ref_mic_type: 2;
    uint16_t analog_primary_ref_mic_adc_ch: 3;
    uint16_t analog_primary_ref_mic_sport_idx: 3;
    uint16_t analog_primary_ref_mic_sport_ch: 3;
    uint16_t analog_primary_ref_mic_sport_dir: 1;

    uint16_t analog_aux_left_enable: 1;
    uint16_t analog_aux_left_sel: 3;
    uint16_t analog_aux_left_type: 2;
    uint16_t analog_aux_left_adc_ch: 3;
    uint16_t analog_aux_left_sport_idx: 3;
    uint16_t analog_aux_left_sport_ch: 3;
    uint16_t analog_aux_left_sport_dir: 1;

    uint16_t analog_aux_right_enable: 1;
    uint16_t analog_aux_right_sel: 3;
    uint16_t analog_aux_right_type: 2;
    uint16_t analog_aux_right_adc_ch: 3;
    uint16_t analog_aux_right_sport_idx: 3;
    uint16_t analog_aux_right_sport_ch: 3;
    uint16_t analog_aux_right_sport_dir: 1;

    // category - ringtone
    uint16_t ringtone_primary_spk_enable: 1;
    uint16_t ringtone_primary_spk_sel: 3;
    uint16_t ringtone_primary_spk_type: 2;
    uint16_t ringtone_primary_spk_dac_ch: 3;
    uint16_t ringtone_primary_spk_sport_idx: 3;
    uint16_t ringtone_primary_spk_sport_ch: 3;
    uint16_t ringtone_primary_spk_sport_dir: 1;

    uint16_t ringtone_secondary_spk_enable: 1;
    uint16_t ringtone_secondary_spk_sel: 3;
    uint16_t ringtone_secondary_spk_type: 2;
    uint16_t ringtone_secondary_spk_dac_ch: 3;
    uint16_t ringtone_secondary_spk_sport_idx: 3;
    uint16_t ringtone_secondary_spk_sport_ch: 3;
    uint16_t ringtone_secondary_spk_sport_dir: 1;

    // category - voice prompt
    uint16_t vp_primary_spk_enable: 1;
    uint16_t vp_primary_spk_sel: 3;
    uint16_t vp_primary_spk_type: 2;
    uint16_t vp_primary_spk_dac_ch: 3;
    uint16_t vp_primary_spk_sport_idx: 3;
    uint16_t vp_primary_spk_sport_ch: 3;
    uint16_t vp_primary_spk_sport_dir: 1;

    uint16_t vp_secondary_spk_enable: 1;
    uint16_t vp_secondary_spk_sel: 3;
    uint16_t vp_secondary_spk_type: 2;
    uint16_t vp_secondary_spk_dac_ch: 3;
    uint16_t vp_secondary_spk_sport_idx: 3;
    uint16_t vp_secondary_spk_sport_ch: 3;
    uint16_t vp_secondary_spk_sport_dir: 1;

    // category - apt
    uint16_t apt_primary_spk_enable: 1;
    uint16_t apt_primary_spk_sel: 3;
    uint16_t apt_primary_spk_type: 2;
    uint16_t apt_primary_spk_dac_ch: 3;
    uint16_t apt_primary_spk_sport_idx: 3;
    uint16_t apt_primary_spk_sport_ch: 3;
    uint16_t apt_primary_spk_sport_dir: 1;

    uint16_t apt_secondary_spk_enable: 1;
    uint16_t apt_secondary_spk_sel: 3;
    uint16_t apt_secondary_spk_type: 2;
    uint16_t apt_secondary_spk_dac_ch: 3;
    uint16_t apt_secondary_spk_sport_idx: 3;
    uint16_t apt_secondary_spk_sport_ch: 3;
    uint16_t apt_secondary_spk_sport_dir: 1;

    uint16_t apt_primary_ref_spk_enable: 1;
    uint16_t apt_primary_ref_spk_external: 1;
    uint16_t apt_primary_ref_spk_sel: 2;
    uint16_t apt_primary_ref_spk_type: 2;
    uint16_t apt_primary_ref_spk_dac_ch: 3;
    uint16_t apt_primary_ref_spk_sport_idx: 3;
    uint16_t apt_primary_ref_spk_sport_ch: 3;
    uint16_t apt_primary_ref_spk_sport_dir: 1;

    uint16_t apt_primary_ref_mic_enable: 1;
    uint16_t apt_primary_ref_mic_external: 1;
    uint16_t apt_primary_ref_mic_sel: 2;
    uint16_t apt_primary_ref_mic_type: 2;
    uint16_t apt_primary_ref_mic_adc_ch: 3;
    uint16_t apt_primary_ref_mic_sport_idx: 3;
    uint16_t apt_primary_ref_mic_sport_ch: 3;
    uint16_t apt_primary_ref_mic_sport_dir: 1;

    uint16_t apt_primary_mic_left_enable: 1;
    uint16_t apt_primary_mic_left_sel: 3;
    uint16_t apt_primary_mic_left_type: 2;
    uint16_t apt_primary_mic_left_adc_ch: 3;
    uint16_t apt_primary_mic_left_sport_idx: 3;
    uint16_t apt_primary_mic_left_sport_ch: 3;
    uint16_t apt_primary_mic_left_sport_dir: 1;

    uint16_t apt_primary_mic_right_enable: 1;
    uint16_t apt_primary_mic_right_sel: 3;
    uint16_t apt_primary_mic_right_type: 2;
    uint16_t apt_primary_mic_right_adc_ch: 3;
    uint16_t apt_primary_mic_right_sport_idx: 3;
    uint16_t apt_primary_mic_right_sport_ch: 3;
    uint16_t apt_primary_mic_right_sport_dir: 1;

    uint16_t apt_secondary_mic_left_enable: 1;
    uint16_t apt_secondary_mic_left_sel: 3;
    uint16_t apt_secondary_mic_left_type: 2;
    uint16_t apt_secondary_mic_left_adc_ch: 3;
    uint16_t apt_secondary_mic_left_sport_idx: 3;
    uint16_t apt_secondary_mic_left_sport_ch: 3;
    uint16_t apt_secondary_mic_left_sport_dir: 1;

    uint16_t apt_secondary_mic_right_enable: 1;
    uint16_t apt_secondary_mic_right_sel: 3;
    uint16_t apt_secondary_mic_right_type: 2;
    uint16_t apt_secondary_mic_right_adc_ch: 3;
    uint16_t apt_secondary_mic_right_sport_idx: 3;
    uint16_t apt_secondary_mic_right_sport_ch: 3;
    uint16_t apt_secondary_mic_right_sport_dir: 1;

    // category - llapt
    uint16_t llapt_primary_spk_enable: 1;
    uint16_t llapt_primary_spk_sel: 3;
    uint16_t llapt_primary_spk_type: 2;
    uint16_t llapt_primary_spk_dac_ch: 3;
    uint16_t llapt_primary_spk_sport_idx: 3;
    uint16_t llapt_primary_spk_sport_ch: 3;
    uint16_t llapt_primary_spk_sport_dir: 1;

    uint16_t llapt_secondary_spk_enable: 1;
    uint16_t llapt_secondary_spk_sel: 3;
    uint16_t llapt_secondary_spk_type: 2;
    uint16_t llapt_secondary_spk_dac_ch: 3;
    uint16_t llapt_secondary_spk_sport_idx: 3;
    uint16_t llapt_secondary_spk_sport_ch: 3;
    uint16_t llapt_secondary_spk_sport_dir: 1;

    uint16_t llapt_primary_ref_spk_enable: 1;
    uint16_t llapt_primary_ref_spk_external: 1;
    uint16_t llapt_primary_ref_spk_sel: 2;
    uint16_t llapt_primary_ref_spk_type: 2;
    uint16_t llapt_primary_ref_spk_dac_ch: 3;
    uint16_t llapt_primary_ref_spk_sport_idx: 3;
    uint16_t llapt_primary_ref_spk_sport_ch: 3;
    uint16_t llapt_primary_ref_spk_sport_dir: 1;

    uint16_t llapt_primary_ref_mic_enable: 1;
    uint16_t llapt_primary_ref_mic_external: 1;
    uint16_t llapt_primary_ref_mic_sel: 2;
    uint16_t llapt_primary_ref_mic_type: 2;
    uint16_t llapt_primary_ref_mic_adc_ch: 3;
    uint16_t llapt_primary_ref_mic_sport_idx: 3;
    uint16_t llapt_primary_ref_mic_sport_ch: 3;
    uint16_t llapt_primary_ref_mic_sport_dir: 1;

    uint16_t llapt_mic_left_enable: 1;
    uint16_t llapt_mic_left_sel: 3;
    uint16_t llapt_mic_left_type: 2;
    uint16_t llapt_mic_left_adc_ch: 3;
    uint16_t llapt_mic_left_sport_idx: 3;
    uint16_t llapt_mic_left_sport_ch: 3;
    uint16_t llapt_mic_left_sport_dir: 1;

    uint16_t llapt_mic_right_enable: 1;
    uint16_t llapt_mic_right_sel: 3;
    uint16_t llapt_mic_right_type: 2;
    uint16_t llapt_mic_right_adc_ch: 3;
    uint16_t llapt_mic_right_sport_idx: 3;
    uint16_t llapt_mic_right_sport_ch: 3;
    uint16_t llapt_mic_right_sport_dir: 1;

    // category - anc
    uint16_t anc_primary_spk_enable: 1;
    uint16_t anc_primary_spk_sel: 3;
    uint16_t anc_primary_spk_type: 2;
    uint16_t anc_primary_spk_dac_ch: 3;
    uint16_t anc_primary_spk_sport_idx: 3;
    uint16_t anc_primary_spk_sport_ch: 3;
    uint16_t anc_primary_spk_sport_dir: 1;

    uint16_t anc_secondary_spk_enable: 1;
    uint16_t anc_secondary_spk_sel: 3;
    uint16_t anc_secondary_spk_type: 2;
    uint16_t anc_secondary_spk_dac_ch: 3;
    uint16_t anc_secondary_spk_sport_idx: 3;
    uint16_t anc_secondary_spk_sport_ch: 3;
    uint16_t anc_secondary_spk_sport_dir: 1;

    uint16_t anc_primary_ref_spk_enable: 1;
    uint16_t anc_primary_ref_spk_external: 1;
    uint16_t anc_primary_ref_spk_sel: 2;
    uint16_t anc_primary_ref_spk_type: 2;
    uint16_t anc_primary_ref_spk_dac_ch: 3;
    uint16_t anc_primary_ref_spk_sport_idx: 3;
    uint16_t anc_primary_ref_spk_sport_ch: 3;
    uint16_t anc_primary_ref_spk_sport_dir: 1;

    uint16_t anc_primary_ref_mic_enable: 1;
    uint16_t anc_primary_ref_mic_external: 1;
    uint16_t anc_primary_ref_mic_sel: 2;
    uint16_t anc_primary_ref_mic_type: 2;
    uint16_t anc_primary_ref_mic_adc_ch: 3;
    uint16_t anc_primary_ref_mic_sport_idx: 3;
    uint16_t anc_primary_ref_mic_sport_ch: 3;
    uint16_t anc_primary_ref_mic_sport_dir: 1;

    uint16_t anc_ff_mic_left_enable: 1;
    uint16_t anc_ff_mic_left_sel: 3;
    uint16_t anc_ff_mic_left_type: 2;
    uint16_t anc_ff_mic_left_adc_ch: 3;
    uint16_t anc_ff_mic_left_sport_idx: 3;
    uint16_t anc_ff_mic_left_sport_ch: 3;
    uint16_t anc_ff_mic_left_sport_dir: 1;

    uint16_t anc_ff_mic_right_enable: 1;
    uint16_t anc_ff_mic_right_sel: 3;
    uint16_t anc_ff_mic_right_type: 2;

    uint16_t anc_ff_mic_right_adc_ch: 3;
    uint16_t anc_ff_mic_right_sport_idx: 3;
    uint16_t anc_ff_mic_right_sport_ch: 3;
    uint16_t anc_ff_mic_right_sport_dir: 1;

    uint16_t anc_fb_mic_left_enable: 1;
    uint16_t anc_fb_mic_left_sel: 3;
    uint16_t anc_fb_mic_left_type: 2;
    uint16_t anc_fb_mic_left_adc_ch: 3;
    uint16_t anc_fb_mic_left_sport_idx: 3;
    uint16_t anc_fb_mic_left_sport_ch: 3;
    uint16_t anc_fb_mic_left_sport_dir: 1;

    uint16_t anc_fb_mic_right_enable: 1;
    uint16_t anc_fb_mic_right_sel: 3;
    uint16_t anc_fb_mic_right_type: 2;
    uint16_t anc_fb_mic_right_adc_ch: 3;
    uint16_t anc_fb_mic_right_sport_idx: 3;
    uint16_t anc_fb_mic_right_sport_ch: 3;
    uint16_t anc_fb_mic_right_sport_dir: 1;

    // category - vad
    uint16_t vad_primary_ref_mic_enable: 1;
    uint16_t vad_primary_ref_mic_external: 1;
    uint16_t vad_primary_ref_mic_sel: 2;
    uint16_t vad_primary_ref_mic_type: 2;
    uint16_t vad_primary_ref_mic_adc_ch: 3;
    uint16_t vad_primary_ref_mic_sport_idx: 3;
    uint16_t vad_primary_ref_mic_sport_ch: 3;
    uint16_t vad_primary_ref_mic_sport_dir: 1;

    uint16_t vad_secondary_ref_mic_enable: 1;
    uint16_t vad_secondary_ref_mic_external: 1;
    uint16_t vad_secondary_ref_mic_sel: 2;
    uint16_t vad_secondary_ref_mic_type: 2;
    uint16_t vad_secondary_ref_mic_adc_ch: 3;
    uint16_t vad_secondary_ref_mic_sport_idx: 3;
    uint16_t vad_secondary_ref_mic_sport_ch: 3;
    uint16_t vad_secondary_ref_mic_sport_dir: 1;

    uint16_t vad_primary_mic_enable: 1;
    uint16_t vad_primary_mic_sel: 3;
    uint16_t vad_primary_mic_type: 2;
    uint16_t vad_primary_mic_adc_ch: 3;
    uint16_t vad_primary_mic_sport_idx: 3;
    uint16_t vad_primary_mic_sport_ch: 3;
    uint16_t vad_primary_mic_sport_dir: 1;

    uint16_t vad_secondary_mic_enable: 1;
    uint16_t vad_secondary_mic_sel: 3;
    uint16_t vad_secondary_mic_type: 2;
    uint16_t vad_secondary_mic_adc_ch: 3;
    uint16_t vad_secondary_mic_sport_idx: 3;
    uint16_t vad_secondary_mic_sport_ch: 3;
    uint16_t vad_secondary_mic_sport_dir: 1;

    uint16_t audio_primary_ref_spk_enable: 1;
    uint16_t audio_primary_ref_spk_external: 1;
    uint16_t audio_primary_ref_spk_sel: 2;
    uint16_t audio_primary_ref_spk_type: 2;
    uint16_t audio_primary_ref_spk_dac_ch: 3;
    uint16_t audio_primary_ref_spk_sport_idx: 3;
    uint16_t audio_primary_ref_spk_sport_ch: 3;
    uint16_t audio_primary_ref_spk_sport_dir: 1;

    uint16_t record_primary_ref_mic_enable: 1;
    uint16_t record_primary_ref_mic_external: 1;
    uint16_t record_primary_ref_mic_sel: 2;
    uint16_t record_primary_ref_mic_type: 2;
    uint16_t record_primary_ref_mic_adc_ch: 3;
    uint16_t record_primary_ref_mic_sport_idx: 3;
    uint16_t record_primary_ref_mic_sport_ch: 3;
    uint16_t record_primary_ref_mic_sport_dir: 1;

    uint16_t ringtone_primary_ref_spk_enable: 1;
    uint16_t ringtone_primary_ref_spk_external: 1;
    uint16_t ringtone_primary_ref_spk_sel: 2;
    uint16_t ringtone_primary_ref_spk_type: 2;
    uint16_t ringtone_primary_ref_spk_dac_ch: 3;
    uint16_t ringtone_primary_ref_spk_sport_idx: 3;
    uint16_t ringtone_primary_ref_spk_sport_ch: 3;
    uint16_t ringtone_primary_ref_spk_sport_dir: 1;

    uint16_t vp_primary_ref_spk_enable: 1;
    uint16_t vp_primary_ref_spk_external: 1;
    uint16_t vp_primary_ref_spk_sel: 2;
    uint16_t vp_primary_ref_spk_type: 2;
    uint16_t vp_primary_ref_spk_dac_ch: 3;
    uint16_t vp_primary_ref_spk_sport_idx: 3;
    uint16_t vp_primary_ref_spk_sport_ch: 3;
    uint16_t vp_primary_ref_spk_sport_dir: 1;

    uint8_t rsvd7[2];

    uint32_t sport0_enable                      : 1;
    uint32_t sport0_role                        : 1;
    uint32_t sport0_rsvd0                       : 1;
    uint32_t sport0_rx_mode                     : 2;
    uint32_t sport0_tx_mode                     : 2;
    uint32_t sport0_rx_format                   : 2;
    uint32_t sport0_tx_format                   : 2;
    uint32_t sport0_rx_data_len                 : 3;
    uint32_t sport0_tx_data_len                 : 3;
    uint32_t sport0_rx_chann_len                : 3;
    uint32_t sport0_tx_chann_len                : 3;
    uint32_t sport0_rx_sample_rate              : 4;
    uint32_t sport0_tx_sample_rate              : 4;
    uint32_t sport0_rsvd1                       : 1;

    uint32_t sport0_lrc_pinmux                  : 8;
    uint32_t sport0_bclk_pinmux                 : 8;
    uint32_t sport0_adc_dat_pinmux              : 8;
    uint32_t sport0_dac_dat_pinmux              : 8;

    uint32_t sport1_enable                      : 1;
    uint32_t sport1_role                        : 1;
    uint32_t sport1_rsvd0                       : 1;
    uint32_t sport1_rx_mode                     : 2;
    uint32_t sport1_tx_mode                     : 2;
    uint32_t sport1_rx_format                   : 2;
    uint32_t sport1_tx_format                   : 2;
    uint32_t sport1_rx_data_len                 : 3;
    uint32_t sport1_tx_data_len                 : 3;
    uint32_t sport1_rx_chann_len                : 3;
    uint32_t sport1_tx_chann_len                : 3;
    uint32_t sport1_rx_sample_rate              : 4;
    uint32_t sport1_tx_sample_rate              : 4;
    uint32_t sport1_rsvd1                       : 1;

    uint32_t sport1_lrc_pinmux                  : 8;
    uint32_t sport1_bclk_pinmux                 : 8;
    uint32_t sport1_adc_dat_pinmux              : 8;
    uint32_t sport1_dac_dat_pinmux              : 8;

    uint32_t sport2_enable                      : 1;
    uint32_t sport2_role                        : 1;
    uint32_t sport2_rsvd0                       : 1;
    uint32_t sport2_rx_mode                     : 2;
    uint32_t sport2_tx_mode                     : 2;
    uint32_t sport2_rx_format                   : 2;
    uint32_t sport2_tx_format                   : 2;
    uint32_t sport2_rx_data_len                 : 3;
    uint32_t sport2_tx_data_len                 : 3;
    uint32_t sport2_rx_chann_len                : 3;
    uint32_t sport2_tx_chann_len                : 3;
    uint32_t sport2_rx_sample_rate              : 4;
    uint32_t sport2_tx_sample_rate              : 4;
    uint32_t sport2_rsvd1                       : 1;

    uint32_t sport2_lrc_pinmux                  : 8;
    uint32_t sport2_bclk_pinmux                 : 8;
    uint32_t sport2_adc_dat_pinmux              : 8;
    uint32_t sport2_dac_dat_pinmux              : 8;

    uint32_t sport3_enable                      : 1;
    uint32_t sport3_role                        : 1;
    uint32_t sport3_rsvd0                       : 1;
    uint32_t sport3_rx_mode                     : 2;
    uint32_t sport3_tx_mode                     : 2;
    uint32_t sport3_rx_format                   : 2;
    uint32_t sport3_tx_format                   : 2;
    uint32_t sport3_rx_data_len                 : 3;
    uint32_t sport3_tx_data_len                 : 3;
    uint32_t sport3_rx_chann_len                : 3;
    uint32_t sport3_tx_chann_len                : 3;
    uint32_t sport3_rx_sample_rate              : 4;
    uint32_t sport3_tx_sample_rate              : 4;
    uint32_t sport3_rsvd1                       : 1;

    uint32_t sport3_lrc_pinmux                  : 8;
    uint32_t sport3_bclk_pinmux                 : 8;
    uint32_t sport3_adc_dat_pinmux              : 8;
    uint32_t sport3_dac_dat_pinmux              : 8;

    uint32_t sport_ext_mclk_enable              : 1;
    uint32_t sport_ext_mclk_rate                : 4;
    uint32_t sport_ext_mclk_pinmux              : 8;
    uint32_t sport_ext_rsvd                     : 19;

    // category - audio
    uint32_t audio_ramp_gain_en                 : 1;
    uint32_t audio_ramp_gain_rsvd               : 15;
    uint32_t audio_ramp_gain_duration           : 16;

    uint32_t audio_gain_step_left               : 16;
    uint32_t audio_gain_step_right              : 16;

    // category - voice
    uint32_t voice_ramp_gain_en                 : 1;
    uint32_t voice_ramp_gain_rsvd               : 15;
    uint32_t voice_ramp_gain_duration           : 16;

    uint32_t voice_gain_step_left               : 16;
    uint32_t voice_gain_step_right              : 16;

    // category - analog
    uint32_t analog_ramp_gain_en                : 1;
    uint32_t analog_ramp_gain_rsvd              : 15;
    uint32_t analog_ramp_gain_duration          : 16;

    uint32_t analog_gain_step_left              : 16;
    uint32_t analog_gain_step_right             : 16;

    // category - apt
    uint32_t apt_ramp_gain_en                   : 1;
    uint32_t apt_ramp_gain_rsvd                 : 15;
    uint32_t apt_ramp_gain_duration             : 16;

    uint32_t apt_gain_step_left                 : 16;
    uint32_t apt_gain_step_right                : 16;

    // category - audio
    uint8_t audio_primary_spk_analog_gain;
    uint8_t audio_primary_spk_digital_gain;
    uint8_t audio_secondary_spk_analog_gain;
    uint8_t audio_secondary_spk_digital_gain;

    // category - voice
    uint8_t voice_primary_spk_analog_gain;
    uint8_t voice_primary_spk_digital_gain;
    uint8_t voice_secondary_spk_analog_gain;
    uint8_t voice_secondary_spk_digital_gain;
    uint8_t voice_primary_ref_spk_analog_gain;
    uint8_t voice_primary_ref_spk_digital_gain;
    uint8_t voice_primary_ref_mic_analog_gain;
    uint8_t voice_primary_ref_mic_digital_gain;
    uint8_t voice_primary_ref_mic_digital_boost_gain;
    uint8_t voice_primary_mic_analog_gain;
    uint8_t voice_primary_mic_digital_gain;
    uint8_t voice_primary_mic_digital_boost_gain;
    uint8_t voice_secondary_mic_analog_gain;
    uint8_t voice_secondary_mic_digital_gain;
    uint8_t voice_secondary_mic_digital_boost_gain;
    uint8_t voice_fusion_mic_analog_gain;
    uint8_t voice_fusion_mic_digital_gain;
    uint8_t voice_fusion_mic_digital_boost_gain;
    uint8_t voice_bone_mic_analog_gain;
    uint8_t voice_bone_mic_digital_gain;
    uint8_t voice_bone_mic_digital_boost_gain;

    // category - record
    uint8_t record_primary_mic_analog_gain;
    uint8_t record_primary_mic_digital_gain;
    uint8_t record_primary_mic_digital_boost_gain;
    uint8_t record_secondary_mic_analog_gain;
    uint8_t record_secondary_mic_digital_gain;
    uint8_t record_secondary_mic_digital_boost_gain;
    uint8_t record_aux_left_analog_gain;
    uint8_t record_aux_left_digital_gain;
    uint8_t record_aux_left_digital_boost_gain;
    uint8_t record_aux_right_analog_gain;
    uint8_t record_aux_right_digital_gain;
    uint8_t record_aux_right_digital_boost_gain;

    // category - analog
    uint8_t analog_primary_spk_analog_gain;
    uint8_t analog_primary_spk_digital_gain;
    uint8_t analog_secondary_spk_analog_gain;
    uint8_t analog_secondary_spk_digital_gain;
    uint8_t analog_primary_ref_spk_analog_gain;
    uint8_t analog_primary_ref_spk_digital_gain;
    uint8_t analog_primary_ref_mic_analog_gain;
    uint8_t analog_primary_ref_mic_digital_gain;
    uint8_t analog_primary_ref_mic_digital_boost_gain;
    uint8_t analog_aux_left_analog_gain;
    uint8_t analog_aux_left_digital_gain;
    uint8_t analog_aux_left_digital_boost_gain;
    uint8_t analog_aux_right_analog_gain;
    uint8_t analog_aux_right_digital_gain;
    uint8_t analog_aux_right_digital_boost_gain;

    // category - ringtone
    uint8_t ringtone_primary_spk_analog_gain;
    uint8_t ringtone_primary_spk_digital_gain;
    uint8_t ringtone_secondary_spk_analog_gain;
    uint8_t ringtone_secondary_spk_digital_gain;

    // category - voice prompt
    uint8_t vp_primary_spk_analog_gain;
    uint8_t vp_primary_spk_digital_gain;
    uint8_t vp_secondary_spk_analog_gain;
    uint8_t vp_secondary_spk_digital_gain;

    // category - apt
    uint8_t apt_primary_spk_analog_gain;
    uint8_t apt_primary_spk_digital_gain;
    uint8_t apt_secondary_spk_analog_gain;
    uint8_t apt_secondary_spk_digital_gain;
    uint8_t apt_primary_ref_spk_analog_gain;
    uint8_t apt_primary_ref_spk_digital_gain;
    uint8_t apt_primary_ref_mic_analog_gain;
    uint8_t apt_primary_ref_mic_digital_gain;
    uint8_t apt_primary_ref_mic_digital_boost_gain;
    uint8_t apt_primary_mic_left_analog_gain;
    uint8_t apt_primary_mic_left_digital_gain;
    uint8_t apt_primary_mic_left_digital_boost_gain;
    uint8_t apt_primary_mic_right_analog_gain;
    uint8_t apt_primary_mic_right_digital_gain;
    uint8_t apt_primary_mic_right_digital_boost_gain;
    uint8_t apt_secondary_mic_left_analog_gain;
    uint8_t apt_secondary_mic_left_digital_gain;
    uint8_t apt_secondary_mic_left_digital_boost_gain;
    uint8_t apt_secondary_mic_right_analog_gain;
    uint8_t apt_secondary_mic_right_digital_gain;
    uint8_t apt_secondary_mic_right_digital_boost_gain;

    // category - llapt
    uint8_t llapt_primary_spk_analog_gain;
    uint8_t llapt_primary_spk_digital_gain;
    uint8_t llapt_secondary_spk_analog_gain;
    uint8_t llapt_secondary_spk_digital_gain;
    uint8_t llapt_primary_ref_spk_analog_gain;
    uint8_t llapt_primary_ref_spk_digital_gain;
    uint8_t llapt_primary_ref_mic_analog_gain;
    uint8_t llapt_primary_ref_mic_digital_gain;
    uint8_t llapt_primary_ref_mic_digital_boost_gain;
    uint8_t llapt_mic_left_analog_gain;
    uint8_t llapt_mic_left_digital_gain;
    uint8_t llapt_mic_left_digital_boost_gain;
    uint8_t llapt_mic_right_analog_gain;
    uint8_t llapt_mic_right_digital_gain;
    uint8_t llapt_mic_right_digital_boost_gain;

    // category - anc
    uint8_t anc_primary_spk_analog_gain;
    uint8_t anc_primary_spk_digital_gain;
    uint8_t anc_secondary_spk_analog_gain;
    uint8_t anc_secondary_spk_digital_gain;
    uint8_t anc_primary_ref_spk_analog_gain;
    uint8_t anc_primary_ref_spk_digital_gain;
    uint8_t anc_primary_ref_mic_analog_gain;
    uint8_t anc_primary_ref_mic_digital_gain;
    uint8_t anc_primary_ref_mic_digital_boost_gain;
    uint8_t anc_ff_mic_left_analog_gain;
    uint8_t anc_ff_mic_left_digital_gain;
    uint8_t anc_ff_mic_left_digital_boost_gain;
    uint8_t anc_ff_mic_right_analog_gain;
    uint8_t anc_ff_mic_right_digital_gain;
    uint8_t anc_ff_mic_right_digital_boost_gain;
    uint8_t anc_fb_mic_left_analog_gain;
    uint8_t anc_fb_mic_left_digital_gain;
    uint8_t anc_fb_mic_left_digital_boost_gain;
    uint8_t anc_fb_mic_right_analog_gain;
    uint8_t anc_fb_mic_right_digital_gain;
    uint8_t anc_fb_mic_right_digital_boost_gain;

    // category - vad
    uint8_t rsvd8[2];
    uint8_t vad_primary_ref_mic_analog_gain;
    uint8_t vad_primary_ref_mic_digital_gain;
    uint8_t vad_primary_ref_mic_digital_boost_gain;
    uint8_t vad_primary_mic_analog_gain;
    uint8_t vad_primary_mic_digital_gain;
    uint8_t vad_primary_mic_digital_boost_gain;
    uint8_t vad_secondary_mic_analog_gain;
    uint8_t vad_secondary_mic_digital_gain;
    uint8_t vad_secondary_mic_digital_boost_gain;
} T_SYS_CFG_CONST;

extern T_SYS_CFG_CONST sys_cfg_const;

void sys_cfg_load(void);

#ifdef __cplusplus
}
#endif

#endif /*_SYS_CFG_H_*/
