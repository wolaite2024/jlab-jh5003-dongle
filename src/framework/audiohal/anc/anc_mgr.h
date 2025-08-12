/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _ANC_MGR_H_
#define _ANC_MGR_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_anc_mgr_state
{
    ANC_MGR_STATE_ENABLED,
    ANC_MGR_STATE_ENABLING,
    ANC_MGR_STATE_DISABLED,
    ANC_MGR_STATE_DISABLING,
    ANC_MGR_STATE_UNINITIALIZED,
} T_ANC_MGR_STATE;

typedef enum t_anc_mgr_event
{
    ANC_MGR_EVENT_ENABLED,
    ANC_MGR_EVENT_DISABLED,
} T_ANC_MGR_EVENT;

typedef enum
{
    ANC_IMAGE_SUB_TYPE_ANC_COEF,
    ANC_IMAGE_SUB_TYPE_APT_IDLE_COEF,
    ANC_IMAGE_SUB_TYPE_APT_A2DP_COEF,
    ANC_IMAGE_SUB_TYPE_APT_SCO_COEF,
    ANC_IMAGE_SUB_TYPE_MAX,
} T_ANC_IMAGE_SUB_TYPE;

typedef bool (*P_ANC_MGR_CBACK)(T_ANC_MGR_EVENT event);

bool anc_mgr_init(P_ANC_MGR_CBACK cback);

void anc_mgr_deinit(void);

void anc_mgr_enable(void);

void anc_mgr_disable(void);

void anc_mgr_gain_set(float l_gain, float r_gain);

void anc_mgr_eq_set(float strength);

void anc_mgr_adaptive_filter_start(void);

void anc_mgr_adaptive_filter_pause(void);

void anc_mgr_adaptive_filter_stop(void);

void anc_mgr_load_cfg_set(uint8_t sub_type, uint8_t scenario_id);

void anc_mgr_load_configuration(void);

void anc_mgr_load_scenario(void);

void anc_mgr_load_filter_info(void);

void anc_mgr_tool_enable(void);

void anc_mgr_tool_disable(void);

void anc_mgr_tool_set_feature_map(uint32_t feature_map);

uint32_t anc_mgr_tool_get_feature_map(void);

uint8_t anc_mgr_tool_set_para(void *anc_cmd_ptr);

uint32_t anc_mgr_tool_read_reg(uint32_t reg_addr);

bool anc_mgr_tool_response_measure_enable(uint8_t enable, uint8_t ch_sel, uint32_t *tx_freq,
                                          uint8_t freq_num, uint8_t amp_ratio);

bool anc_mgr_tool_config_data_log(uint8_t src0_sel, uint8_t src1_sel, uint16_t log_len);

bool anc_mgr_tool_load_data_log(void);

void anc_mgr_convert_data_log_addr(uint32_t *log_dest_addr);

uint32_t anc_mgr_tool_get_data_log_length(void);

uint8_t anc_mgr_tool_check_resp_meas_mode(void);

void anc_mgr_tool_set_resp_meas_mode(uint8_t resp_meas_mode);

bool anc_mgr_tool_read_mp_ext_data(uint32_t *mp_ext_data);

void anc_mgr_tool_set_gain_mismatch(uint8_t gain_src, uint32_t l_gain, uint32_t r_gain);

bool anc_mgr_tool_read_gain_mismatch(uint8_t gain_src, uint8_t read_flash,
                                     uint32_t *l_gain, uint32_t *r_gain);

bool anc_mgr_tool_burn_gain_mismatch(uint32_t mp_ext_data);

uint8_t anc_mgr_tool_get_scenario_info(uint8_t *scenario_mode, uint8_t sub_type,
                                       uint8_t *scenario_apt_effect);

bool anc_mgr_tool_read_llapt_ext_data(uint32_t *llapt_ext_data);

void anc_mgr_tool_set_llapt_gain_mismatch(uint32_t l_gain, uint32_t r_gain);

bool anc_mgr_tool_read_llapt_gain_mismatch(uint8_t read_flash, uint32_t *l_gain, uint32_t *r_gain);

bool anc_mgr_tool_burn_llapt_gain_mismatch(uint32_t llapt_ext_data);

void anc_mgr_tool_limiter_wns_switch(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ANC_MGR_H_ */
