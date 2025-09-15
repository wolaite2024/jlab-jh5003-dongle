/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_PATH_H_
#define _AUDIO_PATH_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_type.h"
#include "sys_ipc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    AUDIO_PATH_EVT_NONE             = 0x0000,
    AUDIO_PATH_EVT_CREATE           = 0x0001,
    AUDIO_PATH_EVT_IDLE             = 0x0002,
    AUDIO_PATH_EVT_EFFECT_REQ       = 0x0004,
    AUDIO_PATH_EVT_RUNNING          = 0x0005,
    AUDIO_PATH_EVT_REQ_DATA         = 0x0007,
    AUDIO_PATH_EVT_RELEASE          = 0x0009,
    AUDIO_PATH_EVT_DATA_IND         = 0x000A,
    AUDIO_PATH_EVT_SUSPEND          = 0x000B,
    AUDIO_PATH_EVT_DATA_EMPTY       = 0x000C,
    AUDIO_PATH_EVT_DSP_PLC              = 0x000D,
    AUDIO_PATH_EVT_DSP_SYNC_UNLOCK      = 0x000F,
    AUDIO_PATH_EVT_DSP_SYNC_LOCK        = 0x0010,
    AUDIO_PATH_EVT_DSP_SYNC_V2_SUCC     = 0x0011,
    AUDIO_PATH_EVT_DSP_JOIN_INFO        = 0x0012,
    AUDIO_PATH_EVT_DSP_INTER_MSG        = 0x0013,
    AUDIO_PATH_EVT_DSP_LATENCY_RPT      = 0x0014,
    AUDIO_PATH_EVT_DECODER_PLC_NOTIFY   = 0x0015,
    AUDIO_PATH_EVT_SIGNAL_OUT_REFRESH   = 0x0017,
    AUDIO_PATH_EXC_DSP_UNSYNC           = 0x1001,
    AUDIO_PATH_EXC_DSP_SYNC_EMPTY       = 0x1002,
    AUDIO_PATH_EXC_DSP_LOST_TIMESTAMP   = 0x1003,
} T_AUDIO_PATH_EVENT;

typedef enum
{
    AUDIO_PATH_DATA_TYPE_VOICE          = 0x00,
    AUDIO_PATH_DATA_TYPE_AUDIO          = 0x01,
    AUDIO_PATH_DATA_TYPE_COMPOSITE      = 0x02,
    AUDIO_PATH_DATA_TYPE_LOST           = 0x03,
    AUDIO_PATH_DATA_TYPE_VP             = 0x04,
    AUDIO_PATH_DATA_TYPE_RAW_AUDIO      = 0x05,
    AUDIO_PATH_DATA_TYPE_ZERO           = 0x06,
    AUDIO_PATH_DATA_TYPE_DUMMY          = 0xFF,
} T_AUDIO_PATH_DATA_TYPE;

typedef enum
{
    AUDIO_PATH_MSG_TYPE_PLUGIN = 0x00,
    AUDIO_PATH_MSG_TYPE_NUM,
} T_AUDIO_PATH_MSG_TYPE;

typedef void *T_AUDIO_PATH_HANDLE;
typedef bool (*P_AUDIO_PATH_CBACK)(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_EVENT event,
                                   uint32_t param);

typedef struct
{
    uint32_t sync_word;
    uint32_t session_id;
    uint32_t timestamp;
    uint16_t local_seq;
    uint16_t frame_counter;
    uint8_t  frame_number;
    uint8_t  status;
    uint16_t payload_length;
    uint32_t tail;
    uint8_t  payload[0];
} T_H2D_STREAM_HEADER2;

typedef struct t_anc_llapt_cfg
{
    uint8_t sub_type;
    uint8_t scenario_id;
} T_ANC_LLAPT_CFG;

typedef struct
{
    T_AUDIO_PATH_MSG_TYPE type;
    T_AUDIO_PATH_HANDLE path;
    union
    {
        struct
        {
            uint8_t occasion;
        } plugin_msg;
    } data;
} T_AUDIO_PATH_MSG;

typedef struct t_apt_info
{
    uint32_t sample_rate;
} T_APT_INFO;

typedef struct t_analog_info
{
    uint32_t sample_rate;
} T_ANALOG_INFO;

bool audio_path_init(void);

void audio_path_deinit(void);

bool audio_path_cback_register(P_SYS_IPC_CBACK cback);

void audio_path_cback_unregister(void);

T_AUDIO_PATH_HANDLE audio_path_create(T_AUDIO_CATEGORY          category,
                                      uint32_t                  device,
                                      void                     *info,
                                      T_AUDIO_STREAM_MODE       mode,
                                      uint8_t                   dac_level,
                                      uint8_t                   adc_level,
                                      P_AUDIO_PATH_CBACK        cback);

bool audio_path_destory(T_AUDIO_PATH_HANDLE handle);

bool audio_path_start(T_AUDIO_PATH_HANDLE handle);

bool audio_path_stop(T_AUDIO_PATH_HANDLE handle);

bool audio_path_data_send(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_DATA_TYPE type, uint8_t seq,
                          void *p_data, uint32_t len, bool flush);

uint16_t audio_path_data_recv(T_AUDIO_PATH_HANDLE handle,
                              uint8_t *buf,
                              uint16_t len);

uint16_t audio_path_data_peek(T_AUDIO_PATH_HANDLE handle);

bool audio_path_dac_level_set(T_AUDIO_PATH_HANDLE handle, uint8_t level, float scale);

bool audio_path_dac_mute(T_AUDIO_PATH_HANDLE handle);

bool audio_path_adc_level_set(T_AUDIO_PATH_HANDLE handle, uint8_t level, float scale);

bool audio_path_adc_mute(T_AUDIO_PATH_HANDLE handle);

bool audio_path_power_off(void);

bool audio_path_power_on(void);

bool audio_path_is_running(T_AUDIO_PATH_HANDLE handle);

void audio_path_low_latency_set(bool enable_plc, bool upgrade_clk);

bool audio_path_sw_sidetone_enable(int16_t gain, uint8_t level);

bool audio_path_sw_sidetone_disable(void);

void audio_path_latency_rpt_set(bool enable, uint8_t num);

void audio_path_plc_notify_set(uint16_t interval, uint32_t threshold, bool enable);

bool audio_path_decoder_effect_control(T_AUDIO_PATH_HANDLE handle, uint8_t action);

bool audio_path_encoder_effect_control(T_AUDIO_PATH_HANDLE handle, uint8_t action);

bool audio_path_hw_sidetone_enable(int16_t gain, uint8_t level);

bool audio_path_hw_sidetone_disable(void);

/* dsp rws2.0 */
bool audio_path_timestamp_set(T_AUDIO_PATH_HANDLE handle, uint8_t clk_ref, uint32_t timestamp,
                              bool sync_flag);

bool audio_path_synchronization_role_swap(T_AUDIO_PATH_HANDLE handle, uint8_t role, bool start);

bool audio_path_synchronization_data_send(T_AUDIO_PATH_HANDLE handle, uint8_t *buf, uint16_t len);

void audio_path_b2bmsg_interaction_timeout(void);

bool audio_path_synchronization_join_set(T_AUDIO_PATH_HANDLE handle, uint8_t role);

void audio_path_synchronization_join_stop(void);

void audio_path_lpm_set(bool enable);

bool audio_path_sidetone_gain_set(T_AUDIO_PATH_HANDLE handle, int16_t gain);

/* unsteady interface */

bool audio_path_cfg_set(T_AUDIO_PATH_HANDLE handle, void *cfg);

void audio_path_brightness_set(T_AUDIO_PATH_HANDLE handle, float strength);

void audio_path_msg_send(T_AUDIO_PATH_MSG *msg);

bool audio_path_signal_out_monitoring_set(T_AUDIO_PATH_HANDLE handle, bool enable,
                                          uint16_t refresh_interval);

bool audio_path_signal_in_monitoring_set(T_AUDIO_PATH_HANDLE handle, bool enable,
                                         uint16_t refresh_interval);

void audio_path_dac_gain_set(T_AUDIO_CATEGORY category,
                             int16_t          left_gain,
                             int16_t          right_gain);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_PATH_H_ */
