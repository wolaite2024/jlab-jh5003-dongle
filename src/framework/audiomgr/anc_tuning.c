/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdbool.h>
#include <stdint.h>

#include "os_mem.h"
#include "trace.h"
#include "anc_tuning.h"
#include "anc_mgr.h"
#include "anc_driver.h"
#include "adsp_loader.h"
/* TODO Remove Start */
#include "rtl876x.h"

#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)

bool anc_tool_load_scenario(uint8_t image_sub_type, uint8_t scenario_id)
{
    anc_mgr_load_cfg_set(image_sub_type, scenario_id);
    anc_mgr_load_scenario();

    return true;
}

void anc_tool_set_feature_map(uint32_t feature_map)
{
    anc_mgr_tool_set_feature_map(feature_map);
}

uint32_t anc_tool_get_feature_map(void)
{
    return anc_mgr_tool_get_feature_map();
}

void anc_tool_enable(void)
{
    anc_mgr_tool_enable();
}

void anc_tool_disable(void)
{
    anc_mgr_tool_disable();
}

uint8_t anc_tool_set_para(void *anc_cmd_ptr)
{
    return anc_mgr_tool_set_para(anc_cmd_ptr);
}

uint32_t anc_tool_read_reg(uint32_t reg_addr)
{
    return anc_mgr_tool_read_reg(reg_addr);
}

bool anc_tool_response_measure_enable(uint8_t enable, uint8_t ch_sel, uint32_t *tx_freq,
                                      uint8_t freq_num, uint8_t amp_ratio)
{
    return anc_mgr_tool_response_measure_enable(enable, ch_sel, tx_freq, freq_num, amp_ratio);
}

void anc_tool_config_data_log(uint8_t src0_sel, uint8_t src1_sel, uint16_t log_len)
{
    anc_mgr_tool_config_data_log(src0_sel, src1_sel, log_len);
}

uint32_t anc_tool_get_data_log_length(void)
{
    return anc_mgr_tool_get_data_log_length();
}

void anc_tool_load_data_log(void)
{
    anc_mgr_tool_load_data_log();
}

void anc_tool_convert_data_log_addr(uint32_t *log_dest_addr)
{
    anc_mgr_convert_data_log_addr(log_dest_addr);
}

T_ANC_RESP_MEAS_MODE anc_tool_check_resp_meas_mode(void)
{
    return (T_ANC_RESP_MEAS_MODE)anc_mgr_tool_check_resp_meas_mode();
}

void anc_tool_set_resp_meas_mode(T_ANC_RESP_MEAS_MODE resp_meas_mode)
{
    anc_mgr_tool_set_resp_meas_mode(resp_meas_mode);
}

void anc_tool_set_gain_mismatch(uint8_t gain_src, uint32_t l_gain, uint32_t r_gain)
{
    anc_mgr_tool_set_gain_mismatch(gain_src, l_gain, r_gain);
}

bool anc_tool_read_gain_mismatch(uint8_t gain_src, uint8_t read_flash,
                                 uint32_t *l_gain, uint32_t *r_gain)
{
    return anc_mgr_tool_read_gain_mismatch(gain_src, read_flash, l_gain, r_gain);
}

bool anc_tool_read_mp_ext_data(uint32_t *mp_ext_data)
{
    return anc_mgr_tool_read_mp_ext_data(mp_ext_data);
}

bool anc_tool_burn_gain_mismatch(uint32_t mp_ext_data)
{
    return anc_mgr_tool_burn_gain_mismatch(mp_ext_data);
}

uint8_t anc_tool_get_anc_scenario_info(uint8_t *scenario_mode)
{
    return anc_mgr_tool_get_scenario_info(scenario_mode, ANC_IMAGE_SUB_TYPE_ANC_COEF, NULL);
}

uint8_t anc_tool_get_llapt_scenario_info(uint8_t *scenario_mode, uint8_t *scenario_apt_effect)
{
    return anc_mgr_tool_get_scenario_info(scenario_mode, ANC_IMAGE_SUB_TYPE_APT_IDLE_COEF,
                                          scenario_apt_effect);
}

void anc_tool_set_llapt_gain_mismatch(uint32_t l_gain, uint32_t r_gain)
{
    anc_mgr_tool_set_llapt_gain_mismatch(l_gain, r_gain);
}

bool anc_tool_read_llapt_gain_mismatch(uint8_t read_flash, uint32_t *l_gain, uint32_t *r_gain)
{
    return anc_mgr_tool_read_llapt_gain_mismatch(read_flash, l_gain, r_gain);
}

bool anc_tool_read_llapt_ext_data(uint32_t *llapt_ext_data)
{
    return anc_mgr_tool_read_llapt_ext_data(llapt_ext_data);
}

bool anc_tool_burn_llapt_gain_mismatch(uint32_t llapt_ext_data)
{
    return anc_mgr_tool_burn_llapt_gain_mismatch(llapt_ext_data);
}

void anc_tool_limiter_wns_switch(void)
{
    anc_mgr_tool_limiter_wns_switch();
}

void anc_tool_anc_mp_bak_buf_config(uint32_t param1, uint32_t param2)
{
    anc_drv_anc_mp_bak_buf_config(param1, param2);
}

void anc_tool_ramp_data_write(uint32_t wdata)
{
    anc_drv_ramp_data_write(wdata);
}

void anc_tool_get_scenario_addr(uint32_t param)
{
    anc_drv_get_scenario_addr(param);
}

void anc_tool_para_aes_encrypt(uint32_t scenario_img, uint32_t length)
{
    anc_drv_para_aes_encrypt(scenario_img, length);
}

void *anc_tool_get_scenario_img(uint8_t sub_type, uint8_t scenario_id, uint32_t *length)
{
    return anc_drv_get_scenario_img(sub_type, scenario_id, length);
}

void *anc_tool_get_filter_info_img(uint32_t *length)
{
    return anc_drv_get_filter_info_img(length);
}

void anc_tool_set_scenario(void *image_buf)
{
    anc_drv_set_specfic_img(image_buf);
}

uint8_t anc_tool_query_para(uint8_t *adsp_info)
{
    return anc_drv_query_para(adsp_info);
}

uint8_t anc_tool_config_adsp_para(uint16_t crc_check, uint32_t adsp_para_len, uint8_t segment_total)
{
    return anc_drv_config_adsp_para(crc_check, adsp_para_len, segment_total);
}

uint8_t anc_tool_turn_on_adsp(uint8_t enable)
{
    return anc_drv_turn_on_adsp(enable);
}

uint8_t anc_tool_enable_adaptive_anc(uint8_t enable, uint8_t grp_idx)
{
    return anc_drv_enable_adaptive_anc(enable, grp_idx);
}

uint8_t anc_tool_load_adsp_para(uint8_t *para_ptr, uint8_t segment)
{
    return anc_drv_load_adsp_para(para_ptr, segment, 0);
}

T_ADSP_ALGORITHM_SCENARIO adsp_pre_scenario = ADSP_ALGORITHM_SCENARIO_NONE;
uint8_t anc_tool_load_adsp_image(uint32_t encode)
{
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 0)
#else
    extern bool adsp_load_algorithm_code(uint8_t algorithm);
    T_ADSP_ALGORITHM_SCENARIO adsp_curr_scenario = ADSP_ALGORITHM_SCENARIO_NONE;

    if (encode & BIT4)
    {
        adsp_curr_scenario = ADSP_ALGORITHM_SCENARIO_APT;
    }
    else if (encode & BIT5)
    {
        adsp_curr_scenario = ADSP_ALGORITHM_SCENARIO_APT;
    }
    else if (encode & BIT6)
    {
        adsp_curr_scenario = ADSP_ALGORITHM_SCENARIO_ADAPTIVE_ANC;
    }

    if (adsp_curr_scenario != adsp_pre_scenario)
    {
        anc_drv_turn_on_adsp(0);
        adsp_load_algorithm_code(adsp_curr_scenario);
    }

    adsp_pre_scenario = adsp_curr_scenario;
    anc_drv_set_adsp_para_source(ANC_DRV_ADSP_PARA_SRC_FROM_TOOL);
#endif
    return false;
}

uint8_t anc_tool_get_adsp_image_load_done(void)
{
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 0)
    return false;
#else
    return !anc_drv_get_adsp_load_finished();
#endif
}

#else /* CONFIG_REALTEK_AM_ANC_SUPPORT */

bool anc_tool_load_scenario(uint8_t image_sub_type, uint8_t scenario_id)
{
    return false;
}

void anc_tool_set_feature_map(uint32_t feature_map)
{
    return ;
}

uint32_t anc_tool_get_feature_map(void)
{
    return false;
}

void anc_tool_enable(void)
{
    return ;
}

void anc_tool_disable(void)
{
    return ;
}

uint8_t anc_tool_set_para(void *anc_cmd_ptr)
{
    return 0xFF;
}

uint32_t anc_tool_read_reg(uint32_t reg_addr)
{
    return 0xDEADBEEF;
}

bool anc_tool_response_measure_enable(uint8_t enable, uint8_t ch_sel, uint32_t *tx_freq,
                                      uint8_t freq_num, uint8_t amp_ratio)
{
    return false;
}

void anc_tool_config_data_log(uint8_t src0_sel, uint8_t src1_sel, uint16_t log_len)
{
    return ;
}

uint32_t anc_tool_get_data_log_length(void)
{
    return 0xDEADBEEF;
}

void anc_tool_load_data_log(void)
{
    return ;
}

void anc_tool_get_data_log_dest_addr(uint32_t *log_dest_addr)
{
    return ;
}

T_ANC_RESP_MEAS_MODE anc_tool_check_resp_meas_mode(void)
{
    return ANC_RESP_MEAS_MODE_NONE;
}

void anc_tool_trigger_wdg_reset(T_ANC_RESP_MEAS_MODE resp_meas_mode)
{
    return ;
}

void anc_tool_set_gain_mismatch(uint8_t gain_src, uint32_t l_gain, uint32_t r_gain)
{
    return ;
}

bool anc_tool_read_gain_mismatch(uint8_t gain_src, uint8_t read_flash,
                                 uint32_t *l_gain, uint32_t *r_gain)
{
    return false;
}

bool anc_tool_read_mp_ext_data(uint32_t *mp_ext_data)
{
    return false;
}

bool anc_tool_burn_gain_mismatch(uint32_t mp_ext_data)
{
    return false;
}

uint8_t anc_tool_get_anc_scenario_info(uint8_t *scenario_mode)
{
    return 0;
}

uint8_t anc_tool_get_llapt_scenario_info(uint8_t *scenario_mode, uint8_t *scenario_apt_effect)
{
    return 0;
}

void anc_tool_set_llapt_gain_mismatch(uint32_t l_gain, uint32_t r_gain)
{
    return ;
}

bool anc_tool_read_llapt_gain_mismatch(uint8_t read_flash, uint32_t *l_gain, uint32_t *r_gain)
{
    return false;
}

bool anc_tool_read_llapt_ext_data(uint32_t *llapt_ext_data)
{
    return false;
}

bool anc_tool_burn_llapt_gain_mismatch(uint32_t llapt_ext_data)
{
    return false;
}

void anc_tool_limiter_wns_switch(void)
{
    return ;
}

void anc_tool_anc_mp_bak_buf_config(uint32_t param1, uint32_t param2)
{
    return ;
}

void anc_tool_ramp_data_write(uint32_t wdata)
{
    return ;
}

void anc_tool_get_scenario_addr(uint32_t param)
{
    return ;
}

uint8_t anc_tool_query_para(uint8_t *adsp_info)
{
    return false;
}

uint8_t anc_tool_config_adsp_para(uint16_t crc_check, uint32_t adsp_para_len, uint8_t segment_total)
{
    return false;
}

uint8_t anc_tool_turn_on_adsp(uint8_t enable)
{
    return false;
}

uint8_t anc_tool_enable_adaptive_anc(uint8_t enable, uint8_t grp_idx)
{
    return false;
}

uint8_t anc_tool_load_adsp_para(uint8_t *para_ptr, uint8_t segment)
{
    return false;
}

uint8_t anc_tool_load_adsp_image(uint32_t encode)
{
    return false;
}

uint8_t anc_tool_get_adsp_image_load_done(void)
{
    return false;
}

#endif /* CONFIG_REALTEK_AM_ANC_SUPPORT */
