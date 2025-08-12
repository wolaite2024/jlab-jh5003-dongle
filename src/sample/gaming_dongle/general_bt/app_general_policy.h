#ifndef _APP_GENERAL_POLICY_H_
#define _APP_GENERAL_POLICY_H_

#ifdef LEGACY_BT_GENERAL
#include <stdint.h>
#include <stdbool.h>
#include <gap.h>

#include "audio_type.h"
#include "app_ctrl_cfg.h"

#define HWID_LEN_MAX       20

typedef struct
{
    uint8_t index;
    bool    is_busy;
} T_APP_SRC_CUR_ACTIVE_INFO;

typedef struct
{
    uint8_t bd_addr[6];
    uint16_t vid;
    uint16_t pid;
    uint32_t cod;
    uint8_t rtk_feature[8];
    uint8_t name[GAP_DEVICE_NAME_LEN];
    uint8_t hwid[HWID_LEN_MAX];
} T_APP_SRC_REMOTE_INFO;

typedef struct
{
    uint32_t sdp_mask;
    bool is_found;
    bool is_busy;
    uint16_t protocol_version;
    uint8_t server_channel;
    uint8_t local_server_channel;
    uint8_t feature;
} T_APP_SRC_SDP_INFO;


typedef enum
{
    A2DP_STREAM_STATE_IDLE,
    A2DP_STREAM_STATE_STARTING,
    A2DP_STREAM_STATE_STARTED,
    A2DP_STREAM_STATE_STOPPING,
} T_APP_A2DP_STREAM_STATE;

typedef enum
{
    SCO_SETUP_STATE_IDLE,
    SCO_SETUP_STATE_CONNECTING,
    SCO_SETUP_STATE_CONNECTED,
    SCO_SETUP_STATE_DISCONNECTING,
} T_APP_SCO_STATE;

#define APP_SRC_STREAM_BUFFER_MAX_SIZE  (1021)

typedef struct
{
    uint8_t stream_state;
    uint8_t codec_type;
    uint8_t *buffer;
    uint16_t buf_size;
    uint16_t sbc_frame_size;
    uint16_t sbc_max_frame_num;
    uint16_t sbc_offset;
    uint16_t credits;
    uint16_t a2dp_seq_num;
    uint32_t a2dp_timestamp;
} T_APP_SRC_STREAM_INFO;

typedef struct
{
    uint8_t msbc_data[60];
    uint8_t h2_header_offset;
} T_APP_SRC_VOICE_DATA_RECOMPOSE;


typedef enum
{
    STATE_DEV_IDLE              = 0x00,
    STATE_DEV_RECONNECTING      = 0x01,
    STATE_DEV_CONNECTING        = 0x02,
    STATE_DEV_CLEANUP           = 0x03,
    STATE_DEV_CONNECTED         = 0x04,
    STATE_DEV_SHUTDOWN          = 0x05,
    STATE_DEV_OTA               = 0x06,
    STATE_DEV_MAX               = 0x07,
} T_APP_DEV_STATE;

typedef enum
{
    STATE_BT_IDLE               = 0x00,
    STATE_BT_INQUIRY            = 0x01,
    STATE_BT_PAGING             = 0x02,
    STATE_BT_ACL_CONNECTED      = 0x03,
    STATE_BT_MAX                = 0x04,
} T_APP_BT_STATE;

typedef enum
{
    KEY_NON         = 0x00,
    KEY_SHORT       = 0x01,
    KEY_LONG        = 0x02,
    KEY_NEW_TARGET  = 0x03,
} T_APP_SRC_KEY_EVENT;

typedef enum
{
    SRC_TIMER_ID_POWERON_DELAY = 0x01,
    SRC_TIMER_ID_PAIRING_MODE,
    SRC_TIMER_ID_ACL_EXIST_SINK0,
    SRC_TIMER_ID_ACL_EXIST_SINK1,
    SRC_TIMER_ID_STREAM_IDLE_SINK0,
    SRC_TIMER_ID_STREAM_IDLE_SINK1,
    SRC_TIMER_ID_PAIR_SUC_REPORT_DELAY_SINK0,
    SRC_TIMER_ID_PAIR_SUC_REPORT_DELAY_SINK1,
    SRC_TIMER_ID_A2DP_CONN_SETUP_DELAY_SINK0,
    SRC_TIMER_ID_A2DP_CONN_SETUP_DELAY_SINK1,
    SRC_TIMER_ID_SILENT_DETECT,
    SRC_TIMER_ID_SCO_CONN_SETUP_DELAY,
    SRC_TIMER_ID_USB_STREAM_RESTART,
#ifdef BTDONGLE_BQB_MODE_ENABLE
    SRC_TIMER_ID_BQB_USB_SET_REPORT,
#endif
} T_APP_TIMER_ID;

typedef struct
{
    uint8_t remote_bd[6];
    uint32_t cod;
} T_APP_PENDING_INFO;

typedef struct
{
    bool in_use;
    uint16_t dev_state;
    uint16_t bt_state;

    bool inquiry_dev_found;

    bool hfp_codec_selected;
    uint8_t sco_state;

    bool conn_ready;

    bool a2dp_conn_setup_delay_once;

    T_APP_SRC_REMOTE_INFO remote_info;

    uint32_t profile_mask_plan;
    uint32_t profile_mask_remain;
    uint32_t profile_mask_doing;

    uint8_t profiles_retry_count;

    T_APP_SRC_SDP_INFO  sdp_info;

    bool a2dp_connected_from_remote;
    bool a2dp_codec_configured;
    T_AUDIO_FORMAT_INFO codec_info;

    T_APP_SRC_STREAM_INFO stream_info;

    T_APP_SRC_VOICE_DATA_RECOMPOSE voice_data;

    T_APP_SRC_KEY_EVENT pending_event;

    T_APP_PENDING_INFO pending_info;

    uint16_t stream_frame_count;
    uint16_t stream_idle_count;

    uint8_t timer_idx_acl_exist_delay;

    uint8_t timer_idx_a2dp_conn_setup_delay;

    uint8_t timer_idx_stream_idle;

    uint8_t timer_idx_pair_suc_report_delay;

    uint8_t retry_count;
} T_APP_DEV;

void app_load_role(void);
bool app_a2dp_is_src(void);
bool app_a2dp_role_switch_needed(void);


void app_a2dp_src_key_cfg_init(void);

void src_handle_a2dp_stream_suspend(void);

void src_enter_pairing_mode(void);


void key_short_press_handle(uint8_t index);
void key_long_press_handle(uint8_t index);

bool usb_audio_data_cb(uint8_t *buf, uint16_t len, uint16_t frame_no);
void app_src_handle_usb_audio_in(void);
void app_src_handle_usb_audio_out(void);

void app_handle_music_data_notify(void);

void app_notify_policy_us_audio_status(bool active);

void src_handle_poweron(void);
void src_poweroff_cleanup(void);

void src_handle_aux_in(void);

void src_handle_aux_out(void);

void src_stop_poweron_reconn(void);

uint8_t src_get_audio_device_pair_state(void);
bool src_get_audio_connected_dev_addr(uint8_t *bd_addr);
bool src_connect_audio_device(uint8_t *bd_addr, uint32_t cod);
bool src_remove_audio_device(void);

//typedef void (*APP_SRC_AUDIO_CONN_RESULT_CB)(uint8_t *bd_addr, T_APP_CTRL_PAIR_STATE pair_state,
//                                             uint8_t *pin_code);
//void src_register_audio_pair_state_nofity(APP_SRC_AUDIO_CONN_RESULT_CB pair_state_cb);
typedef void (*APP_SRC_PAIR_RESULT_CB)(uint8_t *bd_addr, T_APP_CTRL_PAIR_STATE pair_state);
void src_register_pair_state_nofity(APP_SRC_PAIR_RESULT_CB pair_state_cb);

void src_usb_reset_reconn(void);

//only to reconn/disconn current device
bool src_connect(void);
bool src_disconnect(void);

uint8_t src_get_disconn_reason(void);

uint8_t src_get_audio_device_conn_state(void);
uint8_t src_get_audio_device_profile_state(void);

//void app_usb_handle_uac_set_volume(uint32_t vol, uint8_t mute);

void app_src_policy_init(void);
void src_handle_profile_conn_cmpl(uint8_t *bd_addr, uint32_t profile_mask);
void src_handle_profile_disconn_cmpl(uint8_t *bd_addr, uint16_t cause,
                                     uint32_t profile_mask);
void app_src_uac_us_status(bool active);
void src_detect_silent_data_start_timer(void);
void src_detect_silent_data_stop_timer(void);
#endif
#endif
