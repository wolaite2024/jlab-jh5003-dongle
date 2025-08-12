/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "os_msg.h"
#include "trace.h"
#include "anc_mgr.h"
#include "anc_driver.h"
#include "anc_loader.h"
#include "adsp_loader.h"

/* TODO Remove Start */
#include "rtl876x.h"
#include "codec_driver.h"
extern bool dsp_anc_fw_load_code_driver(T_BIN_LOADER_SESSION_HANDLE  session,
                                        uint32_t                     id,
                                        void                        *context);
extern bool prepare_anc_image_load(void);
extern bool postprocess_anc_image_load(void);

void *hANCQueueHandleAu;
/* TODO Remove End */

#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)

#define ANC_MSG_MAX_NUM             (0x10)

typedef enum t_anc_mgr_pending_event
{
    ANC_MGR_PENDING_EVENT_NONE,
    ANC_MGR_PENDING_EVENT_APT_GAIN,
    ANC_MGR_PENDING_EVENT_APT_EQ,
} T_ANC_MGR_PENDING_EVENT;

typedef union t_anc_mgr_pending_record
{
    uint8_t d8[8];
    struct
    {
        float apt_gain_l_record;
        float apt_gain_r_record;
    };
    struct
    {
        float apt_eq_record;
        uint8_t rsvd[4];
    };
} T_ANC_MGR_PENDING_RECORD;

typedef struct t_anc_mgr_db
{
    uint8_t anc_mgr_sub_type_record;
    uint8_t anc_mgr_scenario_id_record;
    T_ANC_MGR_STATE anc_mgr_state;
    P_ANC_MGR_CBACK anc_mgr_cback;
    T_BIN_LOADER_SESSION_HANDLE *anc_load_session;
    uint8_t pending_action;
    T_ANC_MGR_PENDING_RECORD pending_record;
} T_ANC_MGR_DB;

static T_ANC_MGR_DB *anc_mgr_db = NULL;
T_ADSP_ALGORITHM_SCENARIO adsp_pre_scenario_from_bin = ADSP_ALGORITHM_SCENARIO_NONE;

static bool anc_mgr_load_image(T_SHM_SCENARIO image_type);

bool anc_mgr_event_handler(T_ANC_CB_EVENT event)
{
    CODEC_PRINT_TRACE2("anc_mgr_event_handler: event %d, state %d", event, anc_mgr_db->anc_mgr_state);

    switch (event)
    {
    case ANC_CB_EVENT_FADE_IN_COMPLETE:
        {
        }
        break;

    case ANC_CB_EVENT_FADE_OUT_COMPLETE:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_DISABLED;

            anc_drv_disable();

            anc_mgr_db->anc_mgr_cback(ANC_MGR_EVENT_DISABLED);
        }
        break;

    case ANC_CB_EVENT_GAIN_ADJUST_COMPLETE:
    case ANC_CB_EVENT_EQ_ADJUST_COMPLETE:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLED;

            if (anc_mgr_db->pending_action == ANC_MGR_PENDING_EVENT_APT_GAIN)
            {
                anc_mgr_db->pending_action = ANC_MGR_PENDING_EVENT_NONE;

                anc_mgr_gain_set(anc_mgr_db->pending_record.apt_gain_l_record,
                                 anc_mgr_db->pending_record.apt_gain_l_record);
            }
            else if (anc_mgr_db->pending_action == ANC_MGR_PENDING_EVENT_APT_EQ)
            {
                anc_mgr_db->pending_action = ANC_MGR_PENDING_EVENT_NONE;

                anc_mgr_eq_set(anc_mgr_db->pending_record.apt_eq_record);
            }
        }
        break;
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 0)
#else
    case ANC_CB_EVENT_LOAD_ADSP_IMAGE:
        {
            // wait until ADSP image load done
        }
        break;
    case ANC_CB_EVENT_LOAD_ADSP_PARA_COMPLETE:
        {
            anc_drv_turn_on_adsp(1);
            anc_drv_load_adsp_para(p_adsp_para_buf, 0, 1);
        }
        break;
#endif
    case ANC_CB_EVENT_LOAD_CONFIGURATION_COMPLETE:
        {
            if (anc_mgr_db->anc_mgr_state == ANC_MGR_STATE_UNINITIALIZED)
            {
                if (anc_mgr_db->anc_mgr_scenario_id_record == ANC_DRV_DONT_CARE_8)
                {
                    // for tool mode, there's no need to load pending scenario image
                    anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLED;
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 0)
                    anc_drv_enable();
#else
                    if (anc_drv_get_adsp_para_source() == ANC_DRV_ADSP_PARA_SRC_FROM_TOOL)
                    {
                        // anc_drv_enable has been loaded by tool
                    }
                    else
                    {
                        anc_drv_enable();
                    }
#endif
                    anc_mgr_db->anc_mgr_cback(ANC_MGR_EVENT_ENABLED);
                }
                else
                {
                    anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLING;

                    if (anc_drv_get_dynamic_para_idx_status())
                    {
                        anc_mgr_load_filter_info();
                    }
                    else
                    {
                        // load pending scenario image
                        anc_mgr_load_scenario();
                    }
                }
            }
        }
        break;

    case ANC_CB_EVENT_FILTER_INFO_COMPLETE:
        {
            // load pending scenario image
            anc_mgr_load_scenario();
        }
        break;

    case ANC_CB_EVENT_LOAD_SCENARIO_COMPLETE:
        {
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 0)
            anc_drv_enable();

            anc_drv_fade_in();
#else
            if (anc_drv_get_adsp_para_source() == ANC_DRV_ADSP_PARA_SRC_FROM_TOOL)
            {
                // anc_drv_enable has been loaded by tool
            }
            else
            {
                anc_drv_enable();

                anc_drv_fade_in();

                uint8_t image_type;
                uint32_t target_gain = anc_drv_get_active_adsp_scenario();
                if ((target_gain & BIT14) || (target_gain & BIT15) || (target_gain & BIT16))
                {
                    image_type = SHM_SCENARIO_ANC_ADSP_PARA;
                    anc_mgr_load_image((T_SHM_SCENARIO)image_type);
                }
            }
#endif
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLED;

            anc_mgr_db->anc_mgr_cback(ANC_MGR_EVENT_ENABLED);

            // clear record scenario id & sub type
            anc_mgr_db->anc_mgr_sub_type_record = 0;
            anc_mgr_db->anc_mgr_scenario_id_record = 0;
        }
        break;

    default:
        break;
    }

    return true;
}

static bool anc_mgr_load_image(T_SHM_SCENARIO image_type)
{
    bool status = true;

    if (!prepare_anc_image_load())
    {
        CODEC_PRINT_ERROR0("prepare_anc_image_load fail!!");
        status = false;
        return status;
    }

    if (anc_mgr_db->anc_load_session != NULL)
    {
        bin_loader_token_issue(anc_mgr_db->anc_load_session, image_type, NULL);
    }
    else
    {
        CODEC_PRINT_ERROR0("create anc load session fail!!");
    }

    if (!postprocess_anc_image_load())
    {
        CODEC_PRINT_ERROR0("postprocess_anc_image_load fail!!");
        status = false;
        return status;
    }

    return status;
}

bool anc_mgr_load_ear_fit_response(void)
{
    return anc_mgr_load_image((T_SHM_SCENARIO)SHM_SCENARIO_ANC_EAR_FIT);
}

void anc_drv_get_ear_fit_response(uint32_t *buffer)
{
    anc_drv_set_ear_fit_buffer(buffer);
    anc_mgr_load_ear_fit_response();
}

bool anc_mgr_init(P_ANC_MGR_CBACK cback)
{
    int8_t ret = 0;

    anc_mgr_db = os_mem_alloc2(sizeof(T_ANC_MGR_DB));
    if (anc_mgr_db == NULL)
    {
        ret = 1;
        goto fail_alloc_anc_mgr_db;
    }

    anc_mgr_db->anc_load_session = bin_loader_session_create(dsp_anc_fw_load_code_driver,
                                                             anc_loader_dsp_fw_cb_check_finish,
                                                             anc_loader_dsp_fw_cb_finish);

    if (anc_mgr_db->anc_load_session == NULL)
    {
        ret = 2;
        goto fail_alloc_anc_load_session;
    }
    anc_mgr_db->anc_mgr_cback = cback;
    anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_UNINITIALIZED;

    anc_mgr_db->anc_mgr_sub_type_record = 0;
    anc_mgr_db->anc_mgr_scenario_id_record = 0;

    anc_mgr_db->pending_action = ANC_MGR_PENDING_EVENT_NONE;

    for (uint8_t i = 0; i < sizeof(T_ANC_MGR_PENDING_RECORD); i++)
    {
        anc_mgr_db->pending_record.d8[i] = 0;
    }

    anc_drv_param_init(anc_mgr_event_handler);
    anc_loader_init(anc_mgr_event_handler);

    os_msg_queue_create(&hANCQueueHandleAu, "ancQ", ANC_MSG_MAX_NUM, sizeof(T_ANC_SCHED_MSG));
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 0)
#else
    if (adsp_loader_init() != true)
    {
        ret = 3;
        goto fail_init_adsp_load;
    }

    if (adsp_load_initial() != true)
    {
        ret = 4;
        goto fail_initial_adsp_load;
    }
#endif
    return true;
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 0)
#else
fail_initial_adsp_load:
    adsp_load_deinit();
fail_init_adsp_load:
#endif
fail_alloc_anc_load_session:
    os_mem_free(anc_mgr_db);
    anc_mgr_db = NULL;
fail_alloc_anc_mgr_db:
    CODEC_PRINT_ERROR1("anc_mgr_init: fail, ret = %d", -ret);
    return false;
}

void anc_mgr_deinit(void)
{
    if (anc_mgr_db != NULL)
    {
        os_mem_free(anc_mgr_db);
        anc_mgr_db = NULL;
    }
}

void anc_mgr_enable(void)
{
    CODEC_PRINT_TRACE1("anc_mgr_enable: state %d", anc_mgr_db->anc_mgr_state);

    switch (anc_mgr_db->anc_mgr_state)
    {
    case ANC_MGR_STATE_ENABLED:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLED;
        }
        break;

    case ANC_MGR_STATE_ENABLING:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLING;
        }
        break;

    case ANC_MGR_STATE_DISABLED:
        {
            if (anc_mgr_db->anc_mgr_scenario_id_record == ANC_DRV_DONT_CARE_8)
            {
                // for tool mode, there's no need to load scenario image
                anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLED;

                anc_drv_enable();

                anc_mgr_db->anc_mgr_cback(ANC_MGR_EVENT_ENABLED);
            }
            else
            {
                anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLING;

                if (anc_drv_get_dynamic_para_idx_status())
                {
                    anc_mgr_load_filter_info();
                }
                else
                {
                    // load pending scenario image
                    anc_mgr_load_scenario();
                }
            }
        }
        break;

    case ANC_MGR_STATE_DISABLING:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_DISABLING;
        }
        break;

    case ANC_MGR_STATE_UNINITIALIZED:
        {
            // load configuration image first whether in user mode or tool mode
            anc_mgr_load_configuration();
        }
        break;

    default:
        break;
    }
}

void anc_mgr_disable(void)
{
    CODEC_PRINT_TRACE1("anc_mgr_disable: state %d", anc_mgr_db->anc_mgr_state);

    switch (anc_mgr_db->anc_mgr_state)
    {
    case ANC_MGR_STATE_ENABLED:
    case ANC_MGR_STATE_ENABLING:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_DISABLING;

            // disable ANC after fade out complete
            // call anc_drv_set_gain during ANC fade in/out will not receive corresponding event
            anc_drv_fade_out();
        }
        break;

    case ANC_MGR_STATE_DISABLED:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_DISABLED;
        }
        break;

    case ANC_MGR_STATE_DISABLING:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_DISABLING;
        }
        break;

    case ANC_MGR_STATE_UNINITIALIZED:
    default:
        break;
    }
}

void anc_mgr_gain_set(float l_gain, float r_gain)
{
    switch (anc_mgr_db->anc_mgr_state)
    {
    case ANC_MGR_STATE_ENABLED:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLING;

            anc_drv_set_gain(l_gain, r_gain);
        }
        break;

    case ANC_MGR_STATE_ENABLING:
        {
            anc_mgr_db->pending_action = ANC_MGR_PENDING_EVENT_APT_GAIN;
            anc_mgr_db->pending_record.apt_gain_l_record = l_gain;
            anc_mgr_db->pending_record.apt_gain_r_record = r_gain;
        }
        break;

    case ANC_MGR_STATE_DISABLED:
    case ANC_MGR_STATE_DISABLING:
        break;

    default:
        break;
    }
}

void anc_mgr_load_configuration(void)
{
    anc_mgr_load_image(SHM_SCENARIO_ANC_0_CFG);
}

void anc_mgr_load_scenario(void)
{
    uint8_t image_type, en_code_fmt;
    uint8_t sub_type, scenario_id;

    en_code_fmt = 0;

#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 1)
    anc_drv_enable_adaptive_anc(0, 0);
    anc_drv_turn_on_adsp(0);
#endif

    sub_type = anc_mgr_db->anc_mgr_sub_type_record;
    scenario_id = anc_mgr_db->anc_mgr_scenario_id_record;

    anc_drv_get_info(&en_code_fmt, ANC_INFO_EN_CODE_FMT);

    if (en_code_fmt == ANC_DRV_EN_CODE_FMT_ANC_ONLY ||
        en_code_fmt == ANC_DRV_EN_CODE_FMT_OLD_ANC)
    {
        // ANC only or old ANC image
        image_type = SHM_SCENARIO_ANC_1_COEF + scenario_id;
        anc_mgr_load_image((T_SHM_SCENARIO)image_type);
    }
    else if ((sub_type == ANC_IMAGE_SUB_TYPE_ANC_COEF) ||
             (sub_type == ANC_IMAGE_SUB_TYPE_APT_IDLE_COEF) ||
             (sub_type == ANC_IMAGE_SUB_TYPE_APT_A2DP_COEF) ||
             (sub_type == ANC_IMAGE_SUB_TYPE_APT_SCO_COEF))
    {
        anc_loader_set_img_load_param(sub_type, scenario_id);
        image_type = SHM_SCENARIO_ANC_APT_COEF;
        anc_mgr_load_image((T_SHM_SCENARIO)image_type);
    }

    CODEC_PRINT_TRACE4("anc_mgr_load_scenario: state %d, scenario_id %d, sub_type: %d, en_code_fmt %d",
                       anc_mgr_db->anc_mgr_state, scenario_id, sub_type, en_code_fmt);
}

void anc_mgr_load_filter_info(void)
{
    uint8_t image_type;
    uint8_t sub_type, scenario_id;

    sub_type = anc_mgr_db->anc_mgr_sub_type_record;
    scenario_id = anc_mgr_db->anc_mgr_scenario_id_record;

    if ((sub_type == ANC_IMAGE_SUB_TYPE_ANC_COEF) ||
        (sub_type == ANC_IMAGE_SUB_TYPE_APT_IDLE_COEF) ||
        (sub_type == ANC_IMAGE_SUB_TYPE_APT_A2DP_COEF) ||
        (sub_type == ANC_IMAGE_SUB_TYPE_APT_SCO_COEF))
    {
        anc_loader_set_img_load_param(sub_type, scenario_id);
        image_type = SHM_SCENARIO_ANC_APT_FILTER_INFO;
        anc_mgr_load_image((T_SHM_SCENARIO)image_type);
    }

    CODEC_PRINT_TRACE3("anc_mgr_load_filter_info: state %d, scenario_id %d, sub_type: %d",
                       anc_mgr_db->anc_mgr_state, scenario_id, sub_type);
}

void anc_mgr_eq_set(float strength)
{
    switch (anc_mgr_db->anc_mgr_state)
    {
    case ANC_MGR_STATE_ENABLED:
        {
            anc_mgr_db->anc_mgr_state = ANC_MGR_STATE_ENABLING;

            anc_drv_eq_set(strength);
        }
        break;

    case ANC_MGR_STATE_ENABLING:
        {
            anc_mgr_db->pending_action = ANC_MGR_PENDING_EVENT_APT_EQ;
            anc_mgr_db->pending_record.apt_eq_record = strength;
        }
        break;

    case ANC_MGR_STATE_DISABLED:
    case ANC_MGR_STATE_DISABLING:
        break;

    default:
        break;
    }
}

void anc_mgr_adaptive_filter_start(void)
{
    anc_drv_turn_on_adsp(1);
    anc_drv_enable_adaptive_anc(1, 0);
}

void anc_mgr_adaptive_filter_pause(void)
{
    anc_drv_enable_adaptive_anc(3, 0);
}

void anc_mgr_adaptive_filter_stop(void)
{
    anc_drv_enable_adaptive_anc(0, 0);
    anc_drv_turn_on_adsp(0);
}

void anc_mgr_tool_enable(void)
{
    anc_drv_enable();
}

void anc_mgr_tool_disable(void)
{
    anc_drv_disable();
}

void anc_mgr_tool_set_feature_map(uint32_t feature_map)
{
    anc_drv_tool_set_feature_map((T_ANC_DRV_FEATURE_MAP)feature_map);
}

uint32_t anc_mgr_tool_get_feature_map(void)
{
    return anc_drv_tool_get_feature_map();
}

uint8_t anc_mgr_tool_set_para(void *anc_cmd_ptr)
{
    return anc_drv_tool_set_para((T_ANC_CMD_PKT *)anc_cmd_ptr);
}

uint32_t anc_mgr_tool_read_reg(uint32_t reg_addr)
{
    return anc_drv_tool_reg_read(reg_addr);
}

bool anc_mgr_tool_response_measure_enable(uint8_t enable, uint8_t ch_sel, uint32_t *tx_freq,
                                          uint8_t freq_num, uint8_t amp_ratio)
{
    T_CODEC_SPK_CONFIG spk_config;
    T_CODEC_DAC_CONFIG dac_config;

    if (enable)
    {
        // spk setting for response measure
        codec_drv_config_init(CODEC_CONFIG_SEL_SPK, (void *)&spk_config);

        spk_config.power_en = (ch_sel == SPK_CHANNEL_L) ? 1 : 0;
        codec_drv_spk_config_set(SPK_CHANNEL_L, &spk_config, false);
        spk_config.power_en = (ch_sel == DAC_CHANNEL_R) ? 1 : 0;
        codec_drv_spk_config_set(SPK_CHANNEL_R, &spk_config, false);

        // dac setting for response measure
        codec_drv_config_init(CODEC_CONFIG_SEL_DAC, (void *)&dac_config);
        dac_config.anc_mute_en = 1;
        dac_config.sample_rate = 48000;
        dac_config.dig_gain = 0xAF;
        dac_config.i2s_sel = I2S_CHANNEL_0;
        dac_config.downlink_mix = CODEC_DOWNLINK_MIX_NONE;
        dac_config.music_mute_en = (ch_sel == DAC_CHANNEL_L) ? 0 : 1;
        dac_config.ana_power_en = (ch_sel == DAC_CHANNEL_L) ? 1 : 0;
        dac_config.dig_power_en = (ch_sel == DAC_CHANNEL_L) ? 1 : 0;
        codec_drv_dac_config_set(DAC_CHANNEL_L, &dac_config, true);
        dac_config.music_mute_en = (ch_sel == DAC_CHANNEL_R) ? 0 : 1;
        dac_config.ana_power_en = (ch_sel == DAC_CHANNEL_R) ? 1 : 0;
        dac_config.dig_power_en = (ch_sel == DAC_CHANNEL_R) ? 1 : 0;
        codec_drv_dac_config_set(DAC_CHANNEL_R, &dac_config, true);
    }

    return anc_drv_response_measure_enable(enable, tx_freq, freq_num, amp_ratio);
}

bool anc_mgr_tool_config_data_log(uint8_t src0_sel, uint8_t src1_sel, uint16_t log_len)
{
    anc_drv_config_data_log((T_ANC_DRV_LOG_SRC_SEL)src0_sel,
                            (T_ANC_DRV_LOG_SRC_SEL)src1_sel,
                            log_len);
    return true;
}

bool anc_mgr_tool_load_data_log(void)
{
    anc_drv_load_data_log();

    return true;
}

void anc_mgr_convert_data_log_addr(uint32_t *log_dest_addr)
{
    anc_drv_convert_data_log_addr(log_dest_addr);
}

uint32_t anc_mgr_tool_get_data_log_length(void)
{
    return anc_drv_get_data_log_length();
}

uint8_t anc_mgr_tool_check_resp_meas_mode(void)
{
    return anc_drv_check_resp_meas_mode();
}

void anc_mgr_tool_set_resp_meas_mode(uint8_t resp_meas_mode)
{
    anc_drv_set_resp_meas_mode(resp_meas_mode);
}

void anc_mgr_tool_set_gain_mismatch(uint8_t gain_src, uint32_t l_gain, uint32_t r_gain)
{
    anc_drv_set_gain_mismatch(gain_src, l_gain, r_gain);
}

bool anc_mgr_tool_read_gain_mismatch(uint8_t gain_src, uint8_t read_flash,
                                     uint32_t *l_gain, uint32_t *r_gain)
{
    return anc_drv_read_gain_mismatch(gain_src, read_flash, l_gain, r_gain);
}

bool anc_mgr_tool_read_mp_ext_data(uint32_t *mp_ext_data)
{
    return anc_drv_read_mp_ext_data((T_ANC_DRV_MP_EXT_DATA *)mp_ext_data);
}

bool anc_mgr_tool_burn_gain_mismatch(uint32_t mp_ext_data)
{
    return anc_drv_burn_gain_mismatch((T_ANC_DRV_MP_EXT_DATA)mp_ext_data);
}

uint8_t anc_mgr_tool_get_scenario_info(uint8_t *scenario_mode, uint8_t sub_type,
                                       uint8_t *scenario_apt_effect)
{
    return anc_drv_get_scenario_info(scenario_mode, sub_type, scenario_apt_effect);
}

void anc_mgr_tool_set_llapt_gain_mismatch(uint32_t l_gain, uint32_t r_gain)
{
    anc_drv_set_llapt_gain_mismatch(l_gain, r_gain);
}

bool anc_mgr_tool_read_llapt_gain_mismatch(uint8_t read_flash, uint32_t *l_gain, uint32_t *r_gain)
{
    return anc_drv_read_llapt_gain_mismatch(read_flash, l_gain, r_gain);
}

bool anc_mgr_tool_read_llapt_ext_data(uint32_t *llapt_ext_data)
{
    return anc_drv_read_llapt_ext_data((T_ANC_DRV_MP_EXT_DATA *)llapt_ext_data);
}

bool anc_mgr_tool_burn_llapt_gain_mismatch(uint32_t llapt_ext_data)
{
    return anc_drv_burn_llapt_gain_mismatch((T_ANC_DRV_MP_EXT_DATA)llapt_ext_data);
}

void anc_mgr_tool_limiter_wns_switch(void)
{
    anc_drv_limiter_wns_switch();
}

void anc_mgr_load_cfg_set(uint8_t sub_type, uint8_t scenario_id)
{
    CODEC_PRINT_TRACE2("anc_mgr_load_cfg_set: pending scenario_id: %d, sub_type: %d",
                       scenario_id, sub_type);

    anc_mgr_db->anc_mgr_sub_type_record = sub_type;
    anc_mgr_db->anc_mgr_scenario_id_record = scenario_id;
}

#else
bool anc_mgr_init(P_ANC_MGR_CBACK cback)
{
    return true;
}

void anc_mgr_deinit(void)
{
    return;
}

void anc_mgr_enable(void)
{
    return;
}

void anc_mgr_disable(void)
{
    return;
}

void anc_mgr_eq_set(float strength)
{
    return;
}


void anc_mgr_gain_set(float l_gain, float r_gain)
{
    return;
}

void anc_mgr_load_cfg_set(uint8_t sub_type, uint8_t scenario_id)
{
    return;
}
#endif
