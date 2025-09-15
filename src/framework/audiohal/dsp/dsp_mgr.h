/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _DSP_MGR_H_
#define _DSP_MGR_H_

#include <stdint.h>
#include <stdbool.h>

#include "audio_type.h"
#include "sport_mgr.h"
#include "dsp_ipc.h"
#include "sys_ipc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define RWS_ROLE_NONE               0x00
#define RWS_ROLE_SRC                0x01
#define RWS_ROLE_SNK                0x10

#define RWS_RESYNC_V2_OFF       0x00
#define RWS_RESYNC_V2_MASTER    0x01
#define RWS_RESYNC_V2_SLAVE     0x02

typedef enum t_dsp_session_type
{
    DSP_SESSION_TYPE_AUDIO  = AUDIO_CATEGORY_AUDIO,
    DSP_SESSION_TYPE_VOICE  = AUDIO_CATEGORY_VOICE,
    DSP_SESSION_TYPE_RECORD = AUDIO_CATEGORY_RECORD,
    DSP_SESSION_TYPE_ANALOG = AUDIO_CATEGORY_ANALOG,
    DSP_SESSION_TYPE_TONE   = AUDIO_CATEGORY_TONE,
    DSP_SESSION_TYPE_VP     = AUDIO_CATEGORY_VP,
    DSP_SESSION_TYPE_APT    = AUDIO_CATEGORY_APT,
    DSP_SESSION_TYPE_LLAPT  = AUDIO_CATEGORY_LLAPT,
    DSP_SESSION_TYPE_ANC    = AUDIO_CATEGORY_ANC,
    DSP_SESSION_TYPE_VAD    = AUDIO_CATEGORY_VAD,
    DSP_SESSION_TYPE_PIPE,
    DSP_SESSION_TYPE_NUMBER,
} T_DSP_SESSION_TYPE;

typedef enum
{
    DSP_STATE_OFF                           = 0x00,
    DSP_STATE_LOADER                        = 0x01,
    DSP_STATE_IDLE                          = 0x02,
    DSP_STATE_WAIT_OFF                      = 0x06,

    DSP_STATE_FW_READY                      = 0x12,
} T_DSP_STATE;

typedef enum
{
    VAD_SCENARIO_A2DP = 0x0,
    VAD_SCENARIO_SCO = 0x1,
    VAD_SCENARIO_LINE_IN = 0x2,
    VAD_SCENARIO_IDLE = 0x3,
} T_VAD_SCENARIO;

typedef enum t_dsp_mgr_event
{
    DSP_MGR_EVT_INIT_FINISH             = 0x00,
    DSP_MGR_EVT_POWER_OFF               = 0x01,
    DSP_MGR_EVT_POWER_ON                = 0x02,
    DSP_MGR_EVT_DSP_EXCEPTION           = 0x03,
    DSP_MGR_EVT_EFFECT_REQ              = 0x04,
    DSP_MGR_EVT_PREPARE_READY           = 0x05,
    DSP_MGR_EVT_REQUEST_EFFECT          = 0x06,
    DSP_MGR_EVT_FADE_OUT_FINISH         = 0x07,
    DSP_MGR_EVT_SPORT_STOP_FAKE         = 0x0c,
    DSP_MGR_EVT_SPORT_START_FAKE        = 0x0d,
    DSP_MGR_EVT_CODEC_STATE             = 0x0e,
    DSP_MGR_EVT_FW_READY                = 0x0f,
    DSP_MGR_EVT_FW_STOP                 = 0x10,
    DSP_MGR_EVT_MAILBOX_DSP_DATA        = 0x11,
    DSP_MGR_EVT_NOTIFICATION_FINISH     = 0x12,
    DSP_MGR_EVT_PLC_NUM                 = 0x13,
    DSP_MGR_EVT_DECODE_EMPTY            = 0x14,
    DSP_MGR_EVT_DSP_SYNC_V2_SUCC        = 0x15,
    DSP_MGR_EVT_DSP_UNSYNC              = 0x16,
    DSP_MGR_EVT_DSP_SYNC_UNLOCK         = 0x17,
    DSP_MGR_EVT_DSP_SYNC_LOCK           = 0x18,
    DSP_MGR_EVT_SYNC_EMPTY              = 0x19,
    DSP_MGR_EVT_SYNC_LOSE_TIMESTAMP     = 0x1a,
    DSP_MGR_EVT_DSP_JOIN_INFO           = 0x1b,
    DSP_MGR_EVT_LATENCY_REPORT          = 0x1c,
    DSP_MGR_EVT_DSP_LOAD_FINISH         = 0x1d,
    DSP_MGR_EVT_AUDIOPLAY_VOLUME_INFO   = 0x1e,
    DSP_MGR_EVT_REQ_DATA                = 0x1f,
    DSP_MGR_EVT_DATA_IND                = 0x20,
    DSP_MGR_EVT_OPEN_AIR_AVC            = 0x21,
    DSP_MGR_EVT_DECODER_PLC_NOTIFY      = 0x22,

    DSP_MGR_EVT_CODEC_PIPE_CREATE       = 0x30,
    DSP_MGR_EVT_CODEC_PIPE_DESTROY      = 0x31,
    DSP_MGR_EVT_CODEC_PIPE_START        = 0x32,
    DSP_MGR_EVT_CODEC_PIPE_STOP         = 0x33,
    DSP_MGR_EVT_CODEC_PIPE_MIXED        = 0x34,
    DSP_MGR_EVT_CODEC_PIPE_DEMIXED      = 0x35,
} T_DSP_MGR_EVENT;

typedef struct t_dsp_mgr_event_msg
{
    uint8_t algo;
    uint8_t category;
} T_DSP_MGR_EVENT_MSG;

typedef void *T_DSP_MGR_SESSION_HANDLE;

typedef void (*P_DSP_MGR_SESSION_CBACK)(void *handle, T_DSP_MGR_EVENT event, uint32_t param);

typedef struct t_dsp_session_cfg
{
    void                           *context;
    P_DSP_MGR_SESSION_CBACK         callback;
    T_SPORT_MGR_SESSION_HANDLE      sport_handle;
    T_AUDIO_FORMAT_INFO            *format_info;
    uint8_t data_mode;
} T_DSP_SESSION_CFG;

bool dsp_mgr_init(void);

void dsp_mgr_deinit(void);

bool dsp_mgr_power_on(void);

bool dsp_mgr_power_off(void);

T_SYS_IPC_HANDLE dsp_mgr_register_cback(P_SYS_IPC_CBACK cback);

void dsp_mgr_unregister_cback(T_SYS_IPC_HANDLE handle);

T_DSP_MGR_SESSION_HANDLE dsp_mgr_session_create(T_DSP_SESSION_TYPE type, T_DSP_SESSION_CFG config);

bool dsp_mgr_session_destroy(T_DSP_MGR_SESSION_HANDLE handle);

bool dsp_mgr_session_enable(T_DSP_MGR_SESSION_HANDLE handle);

bool dsp_mgr_session_run(T_DSP_MGR_SESSION_HANDLE handle);

bool dsp_mgr_session_disable(T_DSP_MGR_SESSION_HANDLE handle);

bool dsp_mgr_voice_prompt_start(uint32_t cfg, uint32_t cfg_bt_clk_mix);

bool dsp_mgr_voice_prompt_send(uint8_t *p_data, uint32_t len);

void dsp_mgr_voice_prompt_stop(T_DSP_MGR_SESSION_HANDLE handle);

bool dsp_mgr_composite_start(void *p_data, uint32_t len);

void dsp_mgr_composite_stop(T_DSP_MGR_SESSION_HANDLE handle);

void dsp_mgr_signal_proc_start(T_DSP_MGR_SESSION_HANDLE handle, uint32_t timestamp,
                               uint8_t clk_ref, bool synsc_flag);

void dsp_rws_set_role(uint8_t role, uint8_t type);
void dsp_rws_seamless(uint8_t role);
void dsp_remote_init(T_DSP_MGR_SESSION_HANDLE handle, uint8_t clk_ref, uint32_t timestamp,
                     bool sync_flag);

void dsp_mgr_fade_out_start(bool stream_continue,
                            T_DSP_MGR_SESSION_HANDLE dsp_session);

void dsp_mgr_suppress_tx_gain(int16_t gain_step_left, uint8_t time_index);

bool dsp_sport0_da_chann_set(uint8_t chann_mask);
bool dsp_sport0_ad_chann_set(uint8_t chann_mask);

T_DSP_STATE dsp_mgr_get_state(void);

/* unsteady interface */
bool dsp_mgr_apt_action(T_DSP_IPC_ACTION action);
bool dsp_mgr_decoder_action(T_AUDIO_CATEGORY category, T_DSP_IPC_ACTION action);
bool dsp_mgr_encoder_action(T_AUDIO_CATEGORY category, T_DSP_IPC_ACTION action);
bool dsp_mgr_vad_action(T_DSP_IPC_ACTION action);
bool dsp_mgr_line_in_action(T_DSP_IPC_ACTION action);
bool dsp_mgr_session_decoder_effect_control(T_DSP_MGR_SESSION_HANDLE handle,
                                            uint8_t                  action);

bool dsp_mgr_session_encoder_effect_control(T_DSP_MGR_SESSION_HANDLE handle,
                                            uint8_t                  action);

bool dsp_mgr_is_stable(T_DSP_MGR_SESSION_HANDLE handle);
bool dsp_mgr_power_on_check(void);

/* vad */
void dsp_vad_param_load(void);

void dsp_vad_param_set(T_VAD_SCENARIO vad_scenario_state);

/*dsp2*/
uint8_t dsp_mgr_dsp2_ref_get(void);
void dsp_mgr_dsp2_ref_increment(void);
void dsp_mgr_dsp2_ref_decrement(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DSP_MGR_H_ */
