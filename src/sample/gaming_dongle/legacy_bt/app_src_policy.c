#include <trace.h>
#include <audio.h>
#include <rtl876x_wdg.h>
#include <mem_types.h>
#include <os_mem.h>
#include <gap.h>
#include <gap_br.h>
#include <btm.h>
#include <bt_a2dp.h>
#include <bt_avrcp.h>
#include <bt_rfc.h>
#include <sysm.h>
#include "gap_vendor.h"
#include "app_cfg.h"
#include "app_timer.h"
#include "app_ctrl_pkt_policy.h"
#include "app_sdp.h"
#include "app_link_util.h"
#include "app_usb_audio_wrapper.h"

#include "app_bond.h"
#include "app_msg.h"
#include "app_audio_path.h"
#include "gaming_bt.h"
#include "app_spp_audio.h"

#include "app_line_in.h"

#include "section.h"
#include "app_audio_pipe.h"
#include "app_cyclic_buffer.h"
#include "app_src_gaming.h"
#include "app_src_policy.h"
#include "app_device.h"
#include "app_gaming_sync.h"
#include "gaming_bt.h"
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "app_adapter_service.h"
#endif

#ifdef SPP_SBCDEC_SIMULATOR
static uint8_t *dummy_data;
#endif

#include "app_usb_uac.h"
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
#include "uac_silence_detect.h"
#endif

#include "app_downstream_encode.h"
#include "app_upstream_decode.h"

#if F_APP_GAMING_CONTROLLER_SUPPORT
#include "app_usb_controller.h"
#endif


#ifdef LEGACY_BT_GAMING
#define APP_USB_AUDIO        1
#define INPUT_SAMPLE_RATE    48000
#define OUTPUT_SAMPLE_RATE   16000
#define INPUT2_SAMPLE_RATE   48000

/* If you want to measure the latency while mic is open, set the macro */
/* #define APP_AUX_SPP_AUDIO */

#define CONFIG_FLUSH_SBCENC_DATA
#define CONFIG_SPP_AUDIO_STAT

/* If you want to replace the received spp audio pkt with recorded audio
 * sample, open this macro.
 */
/* #define CONFIG_USE_SPP_AUDIO_SAMPLE */

#ifdef CONFIG_USE_SPP_AUDIO_SAMPLE
#include "16k16bit_sbc_123456.c"
#define SPP_AUDIO_SAMPLE_SBC_LEN        52
uint8_t *spp_audio_sample = (uint8_t *)__16k16bit_sbc_123456_sbc;
static uint8_t *spp_audio_ptr;
extern uint32_t __16k16bit_sbc_123456_sbc_len;
#endif

#define REMOTE_COD_MASK         (0x000400)
#define REMOTE_RSSI_LEVEL_MIN   (-80)

#define FTL_SRC_MODE_OFFSET     (4096)
#define FTL_SRC_MODE_SIZE       (4)

#define SRC_POWERON_DELAY_TIMEOUT   (2000)
#define SRC_PAIRING_MODE_TIMEOUT    (60000)
#define SRC_ACL_EXIST_DELAY_TIMEOUT (2000)
#define SRC_STREAM_IDLE_TIMEOUT     40
#define SRC_STREAM_IDLE_MAX         1

#define SRC_BITPOOL_MAX_START   GAMING_SBC_BITPOOL
#define SRC_BITPOOL_MAX_END     53

#define SRC_NORMAL_TPOLL        20
#define SRC_LOW_LATENCY_TPOLL   6
#define SRC_Multilink_LOW_LATENCY_TPOLL   10
#define HEADSET_IS_LINKBACK_TPOLL         120

#define AUX_STATE_IN    0x01
#define AUX_STATE_OUT   0x02

#define SWITCH_STATE_SINK   0x01
#define SWITCH_STATE_SRC    0x02

#define CONN_ATTEMPT_COUNT_MAX  10

#define SRC_DEFAULT_TPOLL       12

//#define AUDIO_PLAY_LOCAL

//#define SPP_DATA_SPK_OUT

typedef enum app_audio_type
{
    AUDIO_TYPE_NONE,
    AUDIO_TYPE_AUX_IN,
    AUDIO_TYPE_USB_DS,
} T_APP_AUDIO_TYPE;

typedef enum
{
    SRC_TIMER_ID_POWERON_DELAY      = 0x01,
    SRC_TIMER_ID_PAIRING_MODE       = 0x02,
    SRC_TIMER_ID_ACL_EXIST_SINK0    = 0x03,
    SRC_TIMER_ID_ACL_EXIST_SINK1    = 0x04,
    SRC_TIMER_ID_A2DP               = 0x05,
    SRC_TIMER_ID_A2DP_2             = 0x06,
    SRC_TIMER_ID_CONN               = 0x07,
    SRC_TIMER_ID_CONN_2             = 0x08,
    SRC_TIMER_ID_RECONN             = 0x09,
    SRC_TIMER_ID_RECONN_2           = 0x0a,
    SRC_TIMER_ID_A2DP_CFG           = 0x0b,
    SRC_TIMER_ID_A2DP_CFG_2         = 0x0c,
    SRC_TIMER_ID_AUTOPAIR           = 0x0d,
    SRC_TIMER_ID_STREAM_IDLE        = 0x0e,
    SRC_TIMER_ID_STREAM_IDLE_2      = 0x0f,
    SRC_TIMER_ID_SPP_AUDIO_STAT     = 0x10,
    SRC_TIMER_ID_SPP_AUDIO_STAT_2   = 0x11,
    SRC_TIMER_ID_DSP_INIT           = 0x12,
    SRC_TIMER_ID_RSV1               = 0x13,
    SRC_TIMER_ID_RSV2               = 0x14,
    SRC_TIMER_ID_PAIRING_TIMEOUT    = 0x15,
    SRC_TIMER_ID_PAIRING_TIMEOUT_2  = 0x16,
    SRC_TIMER_ID_LINKBACK_TIMEOUT   = 0x17,
    SRC_TIMER_ID_LINKBACK_TIMEOUT_2 = 0x18,
    SRC_TIMER_ID_AUTO_POWEROFF_TIMEOUT      = 0x19,
    SRC_TIMER_ID_AUTO_POWEROFF_TIMEOUT_2    = 0x20,
    SRC_TIMER_ID_AUTO_GAMING_MODE    = 0x1b,
    SRC_TIMER_ID_AUTO_GAMING_MODE_2  = 0x1c
} T_APP_TIMER_ID;

enum src_codec_type
{
    SRC_CODEC_SBCENC,
    SRC_CODEC_SBCDEC,
    SRC_CODEC_SBCENC2,
    SRC_CODEC_LC3ENC,
    SRC_CODEC_LC3DEC,
    SRC_CODEC_LC3ENC2,
    SRC_CODEC_MAX,
};


bool rtp_voice_prequeue_ready = false;
bool rtp_voice_transmitting = false;

#if F_APP_LEGACY_DONGLE_BINDING
#else
static uint16_t reconnect_intervals[] = { 0, 1, 2, 2, 2, 2, 2 };
static uint8_t reconnect_intervals_len =
    (sizeof(reconnect_intervals) /
     sizeof(reconnect_intervals[0]));
#endif

static bool legacy_gaming_ready = false;

static uint8_t connect_attempt_count = CONN_ATTEMPT_COUNT_MAX;
static uint8_t timer_idx_src_conn[SINK_DEV_NUM_MAX] = { 0 };
static uint8_t timer_idx_src_reconn[SINK_DEV_NUM_MAX] = { 0 };
static uint8_t timer_idx_src_pairing_timeout[SINK_DEV_NUM_MAX] = { 0 };
static uint8_t timer_idx_src_linkback_timeout[SINK_DEV_NUM_MAX] = { 0 };
static uint8_t timer_idx_src_auto_power_off_timeout[SINK_DEV_NUM_MAX] = { 0 };
static uint8_t timer_idx_src_gaming_mode[SINK_DEV_NUM_MAX] = { 0 };

T_APP_DEV sink_dev_list[SINK_DEV_NUM_MAX];
uint8_t current_index;

static bool ota_mode = false;
static bool shutdown = true;
/* static bool spp_test_mode = false; */
static bool autopairing = false;

static uint8_t src_policy_timer_id = 0;
static uint8_t timer_idx_src_pairing_mode = 0;
static uint8_t timer_idx_src_poweron_delay = 0;
static uint8_t timer_idx_src_a2dp[SINK_DEV_NUM_MAX] = { 0 };
static uint8_t timer_idx_src_a2dp_cfg[SINK_DEV_NUM_MAX] = { 0 };
static uint8_t timer_idx_src_auto_pair = 0;
static uint8_t timer_idx_src_stream_idle[SINK_DEV_NUM_MAX] = { 0 };

static bool a2dp_reconn_timeout[SINK_DEV_NUM_MAX];
static bool auto_power_off_timeout_flag[SINK_DEV_NUM_MAX];

static uint8_t autopairing_index = 0xff;
static uint8_t switch_state;
#define A2DP_STREAM_MAX_CREDITS     4

uint8_t game_volume_pecent;
uint8_t chat_volume_pecent;
#define VOL_BALANCE_ACK             0x00
#define VOL_BALANCE_INFORM          0x01
#define VOL_BALANCE_UPDATE          0x02

static bool force_suspend_resume_flag = false;

static bool support_a2dp_codec_lc3 = false;

struct src_auto_link_struct
{
    uint8_t single_link;
    uint16_t autopair: 1;
    uint16_t autopair_general: 1;
    uint16_t autopair_timeout: 14;
    int8_t autopair_rssi;
} src_auto_link_cfg;

struct src_general_cfg_struct
{
    uint8_t sbc_frames_per_pkt;
    uint8_t auto_gaming;
    uint8_t uac2;
    uint16_t pairing_timeout;
    uint16_t linkback_timeout;
    uint16_t auto_power_off_timeout;
} src_general_cfg;

#define APP_AVDTP_START_REQ     1
#define APP_AVDTP_SUSPEND_REQ   2

T_APP_AUDIO_TYPE app_audio_type;

#if F_APP_LEGACY_DONGLE_BINDING
const uint32_t src_supported_profiles[] =
{
    SPP_AUDIO_PROFILE_MASK,
    A2DP_PROFILE_MASK,
    AVRCP_PROFILE_MASK,
};
#else
const uint32_t src_supported_profiles[] =
{
    A2DP_PROFILE_MASK,
    AVRCP_PROFILE_MASK,
    SPP_AUDIO_PROFILE_MASK,
};
#endif

const static uint8_t UUID_SPP_AUDIO[16] =
{
    0x12, 0xA2, 0x4D, 0x2E,
    0xFE, 0x14,
    0x48, 0x8e,
    0x93, 0xD2,
    0x17, 0x3C, 0x5A, 0x01, 0x00, 0x00
};

/*****************************************************************************
 * FIXME
 */
typedef struct
{
    uint16_t idx_r;
    uint16_t idx_w;
    uint16_t total_len;
    uint8_t  *buf;
} RINGBUF_T;
/*****************************************************************************/
typedef struct
{
    uint8_t *bd_addr;
    uint16_t   cause;
} T_BT_PARAM;

/****** Gaming structures ******/
enum discovery_state
{
    DISCOVERY_STOPPED = 0,
    DISCOVERY_STARTING,
    DISCOVERY_FINDING,
    DISCOVERY_STOPPING,
};

static gaming_bt_cback_t bt_uapi_cback = NULL;
static uint8_t discov_state;
static uint8_t client_discovering;

void gaming_set_cfg(uint8_t type, uint8_t *value, uint8_t len);
int gaming_bt_reg_cback(gaming_bt_cback_t cback);
int gaming_bt_connect(uint8_t id, uint8_t *bdaddr);
int gaming_bt_disconnect_by_bdaddr(uint8_t *bdaddr);
int gaming_bt_disconnect_by_id(uint8_t id);
void gaming_bt_remove_bond_by_bdaddr(uint8_t *bdaddr);
void gaming_bt_remove_bond_by_id(uint8_t id);
/* It only discovers the headphone that supports gaming mode. */
int gaming_bt_start_discovery(void);
int gaming_bt_stop_discovery(void);
void app_src_dongle_volume_balance_spp_command(uint8_t message_type);
static void src_update_a2dp_stream_info(uint8_t index);

static void src_control_remote_mic(bool active, T_APP_DEV *sink_dev);

/******************************************************************************/
uint8_t low_latency_mode = 0;

//#define A2DP_INTERVAL_PACKET

#ifdef AUDIO_DELAY_TEST_SCO_US
static bool first_sco_us_enc_pkt = true;
#endif

#ifdef AUDIO_DELAY_TEST_SCO_DS
static bool first_sco_ds_enc_pkt = true;
#endif

#ifdef AUDIO_DELAY_TEST_MUSIC
static bool first_music_enc_pkt = true;
static bool first_music_sbc_pkt = true;
static bool first_music_start = true;
#endif

#ifdef CONFIG_SPP_AUDIO_STAT
static uint32_t spp_sbc_stat[SINK_DEV_NUM_MAX];
static uint8_t timer_idx_src_app_audio_stat[SINK_DEV_NUM_MAX] = { 0 };
#endif

extern uint16_t player_id;

static T_AUDIO_FORMAT_INFO audio_codec_info[SRC_CODEC_MAX];

static bool scan_by_mmi = false;

static bool src_start_disconn(uint8_t *bd_addr);
static uint32_t get_profile_mask_from_bond_flag(uint16_t bond_flag);
static uint32_t get_bond_flag_from_profile_mask(uint32_t profile_mask);

static void src_dev_conn(uint8_t index, uint8_t *bd_addr, uint32_t profile_mask);
static void src_dev_profile_conn(uint8_t index);
static bool src_profile_conn(uint8_t *bd_addr, uint32_t profile_mask, T_APP_SRC_SDP_INFO sdp_info);

static void src_profile_conn_finish(uint8_t index);
static void add_conneted_profile(uint8_t *bd_addr, uint32_t profile_mask);
static void del_connected_profile(uint8_t *bd_addr, uint32_t profile_mask);
static void src_profile_disconn(uint8_t *bd_addr, uint32_t profile_mask);

static void src_start_reconn(uint8_t index);
static void src_start_conn(uint8_t index);
static void src_handle_dev_pending_evt(uint8_t index, T_APP_SRC_KEY_EVENT pending_evt);

static bool src_is_aux_in(void);
#ifdef APP_AUX_AUDIO
static bool sbc_audio_read_cb(uint32_t *timestamp, uint16_t *seq_num,
                              uint8_t *frame_num, void *buf,
                              uint16_t required_len, uint16_t *actual_len);
#endif


static uint8_t src_get_stream_dev_num(void);

extern void (*app_power_on_hook)(void);

static bool uac_us_state;
static SRC_POLICY_UAC_CB_F src_policy_uac_cb;

#ifdef AUDIO_PLAY_LOCAL
static bool audio_path_aux_in_uapi_cback(uint8_t id, uint8_t event, void *buf,
                                         uint16_t len, uint16_t frm_num);
static bool audio_path_spk_out_uapi_cback(uint8_t id, uint8_t event, void *buf,
                                          uint16_t len, uint16_t frm_num);
#endif

static spp_cmd_cback_t spp_cmd_recv_cback = NULL;

extern void app_usb_uac_game_chat_balance(uint8_t game_vol,
                                          uint8_t chat_vol);
//static void prepare_audio_path(void);

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
extern void app_ctrl_spp_receive_ack(uint8_t *params, uint8_t *bd_addr);
#endif

void key_short_press_handle(uint8_t index);
void key_long_press_handle(uint8_t index);

#define APP_WEAKLY_DEFINITIONS
#ifdef APP_WEAKLY_DEFINITIONS
/******************************************************************************
 * FIXME
 * Implement weakly defined functions for porting this policy.
 *
 * If the reference to a missing weak function is made from code that compiles
 * to a branch or branch link instruction, then either:
 *   The reference is resolved as branching to the next instruction. This
 *   effectively makes the branch a NOP.
 *   The branch is replaced by a NOP instruction.
 */

uint16_t player_id;
T_AUDIO_FORMAT_INFO sbcenc_info;
T_AUDIO_FORMAT_INFO sbcdec_info;

__weak uint8_t app_aux_switch_state_get(void) { return 2; }

#define AUX_DETECT_IN 1
#define AUX_DETECT_OUT 2
__weak uint8_t app_aux_detect_state_get(void) { return AUX_DETECT_OUT; }

__weak void app_src_sbc_voice_start_capture(T_AUDIO_FORMAT_INFO p_format_info,
                                            P_AUDIO_TRACK_ASYNC_IO async_read) {};
__weak void app_src_sbc_voice_stop_capture(void) {};

__weak bool app_usb_audio_is_us_streaming(void)
{
    return uac_us_state;
}
__weak bool app_usb_audio_is_ds_streaming(void) { return false; }
__weak void app_usb_empty_us_buf(void) {};

__weak void app_usb_uac_game_chat_balance(uint8_t game_vol,
                                          uint8_t chat_vol)
{
}

__attribute__((unused)) static inline void app_src_init_sbcdec(T_AUDIO_FORMAT_INFO *info)
{
    if (!info)
    {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->type = AUDIO_FORMAT_TYPE_SBC;
    info->attr.sbc.sample_rate = OUTPUT_SAMPLE_RATE;
    info->attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
    info->attr.sbc.block_length = 16;
    info->attr.sbc.subband_num = 8;
    info->attr.sbc.allocation_method = 0; /* Loudness */
    info->attr.sbc.bitpool = 22;
}

__attribute__((unused)) static inline void app_src_init_sbcenc(T_AUDIO_FORMAT_INFO *info)
{
    if (!info)
    {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->type = AUDIO_FORMAT_TYPE_SBC;
    info->attr.sbc.sample_rate = INPUT_SAMPLE_RATE;
    info->attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
    info->attr.sbc.block_length = 16;
    info->attr.sbc.subband_num = 8;
    info->attr.sbc.allocation_method = 0; /* Loudness */
    info->attr.sbc.bitpool = GAMING_SBC_BITPOOL;
}

__attribute__((unused)) static inline void app_src_init_sbcenc2(T_AUDIO_FORMAT_INFO *info)
{
    if (!info)
    {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->type = AUDIO_FORMAT_TYPE_SBC;
    info->attr.sbc.sample_rate = INPUT2_SAMPLE_RATE;
    info->attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
    info->attr.sbc.block_length = 16;
    info->attr.sbc.subband_num = 8;
    info->attr.sbc.allocation_method = 0; /* Loudness */
    info->attr.sbc.bitpool = GAMING_SBC_BITPOOL;
}

__attribute__((unused)) static inline void app_src_init_lc3dec(T_AUDIO_FORMAT_INFO *info)
{
    if (!info)
    {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->type = AUDIO_FORMAT_TYPE_LC3;
    info->attr.lc3.sample_rate = OUTPUT_SAMPLE_RATE;
    info->attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO;
    info->attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
    info->attr.lc3.frame_length = 40;
}

__attribute__((unused)) static inline void app_src_init_lc3enc(T_AUDIO_FORMAT_INFO *info)
{
    if (!info)
    {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->type = AUDIO_FORMAT_TYPE_LC3;
    info->attr.lc3.sample_rate = INPUT_SAMPLE_RATE;
    info->attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_FL |
                                    AUDIO_CHANNEL_LOCATION_FR;
    info->attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
    info->attr.lc3.frame_length = 100;
}

__attribute__((unused)) static inline void app_src_init_lc3enc2(T_AUDIO_FORMAT_INFO *info)
{
    if (!info)
    {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->type = AUDIO_FORMAT_TYPE_LC3;
    info->attr.lc3.sample_rate = INPUT2_SAMPLE_RATE;
    info->attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_FL |
                                    AUDIO_CHANNEL_LOCATION_FR;
    info->attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
    info->attr.lc3.frame_length = 100;
}

static void src_set_codec_fmt(uint8_t codec_type, T_AUDIO_FORMAT_INFO *info)
{
    if (!info)
    {
        return;
    }

    if (codec_type >= SRC_CODEC_MAX)
    {
        return;
    }
    T_AUDIO_FORMAT_INFO *p_codec = &audio_codec_info[codec_type];

    memcpy(p_codec, info, sizeof(T_AUDIO_FORMAT_INFO));

}

/*****************************************************************************/
#endif /* APP_WEAKLY_DEFINITIONS */
/*
void app_a2dp_src_key_cfg_init(void)
{
    app_cfg_const.key_table[0][0][KEY1] = MMI_LEGACY_BT_LINK1_RECONNECT;
    app_cfg_const.key_table[1][0][KEY1] = MMI_LEGACY_BT_LINK1_CONNECT;
    app_cfg_const.key_table[0][0][KEY2] = MMI_LEGACY_BT_LINK2_RECONNECT;
    app_cfg_const.key_table[1][0][KEY2] = MMI_LEGACY_BT_LINK2_CONNECT;

    app_cfg_const.hybrid_key_mapping[HY_KEY7][0] = 0x02;    //key1 only
    app_cfg_const.hybrid_key_mapping[HY_KEY7][1] = 0x03;    //multi click 2 times
    app_cfg_const.hybrid_key_table[0][HY_KEY7] = 0xf6;      //event
}
*/

void app_load_role(void)
{
    switch_state = app_aux_switch_state_get();

    APP_PRINT_INFO1("app_load_role: %d", switch_state);
}

bool app_a2dp_role_switch_needed(void)
{
    /* TODO: For dongle projects, we only support a2dp src. */
    return false;
#if 0
    if (app_aux_switch_state_get() != switch_state)
    {
        return true;
    }
    else
    {
        return false;
    }
#endif
}

static bool src_dev_all_idle(void)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].dev_state != STATE_DEV_IDLE)
        {
            break;
        }
    }

    if (i == SINK_DEV_NUM_MAX)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool src_bt_is_busy(void)
{
    int i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].connecting)
        {
            return true;
        }
        if (sink_dev_list[i].bt_state == STATE_BT_INQUIRY)
        {
            return true;
        }
        if (sink_dev_list[i].bt_state == STATE_BT_PAGING)
        {
            return true;
        }
    }

    /* No inquiry or paging or connecting */
    return false;
}

static void src_check_product_role_switch(void)
{
    if (app_a2dp_role_switch_needed() && src_dev_all_idle())
    {
        //need to check shutdown ?
        app_cfg_nv.app_is_power_on = 1;
        app_cfg_store();
        chip_reset(RESET_ALL);
    }
}

bool src_policy_find_dev_index_by_addr(uint8_t *bd_addr, uint8_t *index)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (!memcmp(bd_addr, sink_dev_list[i].remote_info.bd_addr, 6))
        {
            *index = i;
            return true;
        }
    }

    return false;
}

bool src_policy_find_dev_index_by_bond_info(uint8_t *bd_addr, uint8_t *index)
{
    uint8_t i;

    if (app_src_legacy_get_paired_idx(bd_addr, &i))
    {
        *index = i;
        return true;
    }
    else
    {
        return false;
    }
}

static void src_dev_state_set(uint8_t index, uint16_t dev_state)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_dev_state_set: invalid index %d", index);
        return ;
    }

    sink_dev = &sink_dev_list[index];

    if (dev_state >= STATE_DEV_MAX)
    {
        APP_PRINT_WARN1("src_dev_state_set: invalid state 0x%04x", dev_state);
        return ;
    }

    sink_dev->dev_state = dev_state;
}

static void src_bt_state_set(uint8_t index, uint16_t bt_state)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_bt_state_set: invalid index %d", index);
        return ;
    }

    sink_dev = &sink_dev_list[index];

    if (bt_state >= STATE_BT_MAX)
    {
        APP_PRINT_WARN1("src_bt_state_set: invalid state 0x%04x", bt_state);
        return ;
    }

    sink_dev->bt_state = bt_state;
}

static void src_led_set_idle(uint8_t index)
{
    struct gaming_ev_led_state ev;
#ifdef APP_AUX_AUDIO
    uint32_t timeout;
#endif

    APP_PRINT_INFO1("src_led_set_idle: %d", index);

    if (shutdown)
    {
        return;
    }

    ev.id = index;
    ev.led_state = GAMING_LED_BT_IDLE;
    if (bt_uapi_cback)
    {
        bt_uapi_cback(EVENT_GAMING_LED_INDICATOR, (void *)&ev, sizeof(ev));
    }

    /* if (index == 0)
     * {
     *     app_led_set_mode(APP_LED_MODE_LINK1_STANDBY);
     * }
     * else if (index == 1)
     * {
     *     app_led_set_mode(APP_LED_MODE_LINK2_STANDBY);
     * }
     */

#ifdef APP_AUX_AUDIO
    timeout = src_general_cfg.auto_power_off_timeout * 1000;
    if (timeout)
    {
        APP_PRINT_INFO1("src_led_set_idle:  auto poweroff timeout %u", timeout);
        app_start_timer(&timer_idx_src_auto_power_off_timeout[index], "src_auto_poweroff_timeout",
                        src_policy_timer_id, SRC_TIMER_ID_AUTO_POWEROFF_TIMEOUT + index, 0, false,
                        timeout);
    }
#endif
}

static void src_led_set_reconnecting(uint8_t index)
{
    struct gaming_ev_led_state ev;

    APP_PRINT_INFO1("src_led_set_reconnecting: %d", index);

    if (shutdown)
    {
        return;
    }

    ev.id = index;
    ev.led_state = GAMING_LED_BT_RECONNECTING;
    if (bt_uapi_cback)
    {
        bt_uapi_cback(EVENT_GAMING_LED_INDICATOR, (void *)&ev, sizeof(ev));
    }

    /* if (index == 0)
     * {
     *     app_led_set_mode(APP_LED_MODE_LINK1_RECONNECTING);
     * }
     * else if (index == 1)
     * {
     *     app_led_set_mode(APP_LED_MODE_LINK2_RECONNECTING);
     * }
     */

    app_stop_timer(&timer_idx_src_auto_power_off_timeout[index]);
}

static void src_led_set_connecting(uint8_t index)
{
    struct gaming_ev_led_state ev;

    APP_PRINT_INFO1("src_led_set_connecting: %d", index);

    if (shutdown)
    {
        return;
    }

    ev.id = index;
    ev.led_state = GAMING_LED_BT_CONNECTING;
    if (bt_uapi_cback)
    {
        bt_uapi_cback(EVENT_GAMING_LED_INDICATOR, (void *)&ev, sizeof(ev));
    }

    /* if (index == 0)
     * {
     *     app_led_set_mode(APP_LED_MODE_LINK1_PAIRING);
     * }
     * else if (index == 1)
     * {
     *     app_led_set_mode(APP_LED_MODE_LINK2_PAIRING);
     * }
     */

    app_stop_timer(&timer_idx_src_auto_power_off_timeout[index]);
}

static void src_led_set_connected(uint8_t index)
{
    struct gaming_ev_led_state ev;

    APP_PRINT_INFO1("src_led_set_connected: %d", index);

    if (shutdown)
    {
        return;
    }

    ev.id = index;
    ev.led_state = GAMING_LED_BT_CONNECTED;
    if (bt_uapi_cback)
    {
        bt_uapi_cback(EVENT_GAMING_LED_INDICATOR, (void *)&ev, sizeof(ev));
    }

    /* if (index == 0)
     * {
     *     app_led_set_mode(APP_LED_MODE_LINK1_CONNECTED);
     * }
     * else if (index == 1)
     * {
     *     app_led_set_mode(APP_LED_MODE_LINK2_CONNECTED);
     * }
     */

    app_stop_timer(&timer_idx_src_auto_power_off_timeout[index]);
}

static void src_enter_reconnecting_state(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;
    uint32_t target_profile_mask = A2DP_PROFILE_MASK | SPP_AUDIO_PROFILE_MASK | AVRCP_PROFILE_MASK;
    uint32_t timeout;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_enter_reconnecting_state: invalid index %d", index);
        return ;
    }

    sink_dev = &sink_dev_list[index];

    memset(sink_dev, 0, sizeof(T_APP_DEV));

    src_dev_state_set(index, STATE_DEV_RECONNECTING);
    src_bt_state_set(index, STATE_BT_IDLE);

    sink_dev->attempt = 0;

    /* We should set the two masks. There is a scenario that the first link is
     * connecting, the user presses the key, then the second link enters
     * reconnecting state. At this time, the remote may be reconnecting to the
     * second link. We needs the correct masks. So we should set the masks
     * before the second link is really connecting.
     */
    sink_dev->profile_mask_plan = target_profile_mask;
    sink_dev->profile_mask_remain = target_profile_mask;

    //set led here
    src_led_set_reconnecting(index);

    timeout = src_general_cfg.linkback_timeout;
    if (timeout)
    {
        timeout *= 1000;
        APP_PRINT_INFO1("src_enter_reconnecting_state: linkback timer start, timeout %u",
                        timeout);
        app_start_timer(&timer_idx_src_linkback_timeout[index], "src_linkback_timeout",
                        src_policy_timer_id, SRC_TIMER_ID_LINKBACK_TIMEOUT + index, 0, false,
                        timeout);
    }
}

static void src_enter_connecting_state(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;
    uint32_t timeout;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_enter_connecting_state: invalid index %d", index);
        return ;
    }

    sink_dev = &sink_dev_list[index];

    memset(sink_dev, 0, sizeof(T_APP_DEV));

    src_dev_state_set(index, STATE_DEV_CONNECTING);
    src_bt_state_set(index, STATE_BT_IDLE);

    sink_dev->attempt = 0;

    //set led here
    src_led_set_connecting(index);

    /* Autopair and pairing timeout are mutual exclusive. */
    if (!src_auto_link_cfg.autopair)
    {
        timeout = src_general_cfg.pairing_timeout;
        if (timeout)
        {
            timeout *= 1000;
            APP_PRINT_INFO1("src_enter_connecting_state: pairing timer start, timeout %u",
                            timeout);
            app_start_timer(&timer_idx_src_pairing_timeout[index], "src_pairing_timeout",
                            src_policy_timer_id, SRC_TIMER_ID_PAIRING_TIMEOUT + index, 0, false,
                            timeout);
        }
    }
}

static bool src_dev_is_bonded(uint8_t index)
{
    uint8_t bond_flag = 0;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_dev_is_bonded: index %d is invalid", index);
        return false;
    }

    bond_flag = app_src_legacy_get_bond_flag_by_index(index);

    if (bond_flag & APP_SRC_BOND_FLAG_A2DP)
    {
        return true;
    }

    return false;
}

/* static uint8_t src_get_bond_no_conn_num(void)
 * {
 *     uint8_t i = 0;
 *     uint8_t num = 0;
 *
 *     for (i = 0; i < SINK_DEV_NUM_MAX; i++)
 *     {
 *         if (src_dev_is_bonded(i) &&
 *             (sink_dev_list[i].bt_state != STATE_BT_ACL_CONNECTED)
 *             && (sink_dev_list[i].dev_state != STATE_DEV_CONNECTING))
 *         {
 *             num++;
 *         }
 *     }
 *
 *     return num;
 * }
 */

static void src_set_bt_mode(T_BT_DEVICE_MODE mode)
{
    static T_BT_DEVICE_MODE cur_mode = BT_DEVICE_MODE_IDLE;

    if (shutdown)
    {
        mode = BT_DEVICE_MODE_IDLE;
    }
    else if (ota_mode)
    {
        mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
    }

    if (cur_mode != mode)
    {
        cur_mode = mode;

        bt_device_mode_set(cur_mode);
    }
}

static void src_check_set_bt_mode(void)
{
    uint8_t i = 0;
    uint8_t max_dev_num = SINK_DEV_NUM_MAX;

    if (shutdown)
    {
        return;
    }

    /* In single mode, we only use index 0 and we disable page/inquiry scan
     * after one link is connected.
     * */
    if (src_auto_link_cfg.single_link)
    {
        max_dev_num = 1;
    }

    for (i = 0; i < max_dev_num; i++)
    {
        /* At least one device is connecting or reconnecting */
        if (sink_dev_list[i].dev_state == STATE_DEV_CONNECTING ||
            sink_dev_list[i].dev_state == STATE_DEV_RECONNECTING)
        {
            if (app_cfg_const.enable_dongle_dual_mode)
            {
                APP_PRINT_INFO0("src_check_set_bt_mode(): case 1 set idle mode");
                src_set_bt_mode(BT_DEVICE_MODE_CONNECTABLE);
            }
            else
            {
                APP_PRINT_INFO0("src_check_set_bt_mode(): case 1 set discoverable & connectable mode");
                src_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
            }
            return;
        }

        /* Reconnecting was exhausted */
        if (sink_dev_list[i].dev_state == STATE_DEV_IDLE &&
            src_dev_is_bonded(i))
        {
            APP_PRINT_INFO0("src_check_set_bt_mode(): case 2 set discoverable & connectable mode");
            src_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
            return;
        }
    }

    APP_PRINT_INFO0("src_check_set_bt_mode(): set idle mode");
    src_set_bt_mode(BT_DEVICE_MODE_IDLE);
}


void src_enter_pairing_mode(void)
{
    APP_PRINT_INFO0("src_enter_pairing_mode");
    ota_mode = true;
    src_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);

    app_start_timer(&timer_idx_src_pairing_mode, "src_pairing_mode",
                    src_policy_timer_id, SRC_TIMER_ID_PAIRING_MODE, 0, false,
                    SRC_PAIRING_MODE_TIMEOUT);

    /* app_led_set_mode(APP_LED_MODE_OTA_PAIRING); */
}

static void src_dev_schedule_next(void)
{
    uint8_t i, j;
    T_APP_DEV *sink_dev = NULL;
    uint8_t next_index;

    /* src_start_reconn() or src_start_conn() will set cur_active_dev_info.index
     * and cur_active_dev_info.is_busy
     */
#if SINK_DEV_NUM_MAX == 1
    next_index = 0;
#else
    next_index = current_index;
    next_index++;
    next_index = next_index % SINK_DEV_NUM_MAX;
#endif

    APP_PRINT_INFO1("src_dev_schedule_next(): next index %u", next_index);

#if SINK_DEV_NUM_MAX == 1
    for (i = next_index, j = 0;
         j < SINK_DEV_NUM_MAX; i = 0, j++)
#else
    for (i = next_index, j = 0;
         j < SINK_DEV_NUM_MAX;
         i = (i + 1) % SINK_DEV_NUM_MAX, j++)
#endif
    {
        sink_dev = &sink_dev_list[i];

        APP_PRINT_INFO3("src_dev_schedule_next: index %u, dev_state %02x, bt_state %02x",
                        i, sink_dev->dev_state, sink_dev->bt_state);

        if (sink_dev->dev_state == STATE_DEV_RECONNECTING)
        {
#if F_APP_LEGACY_DONGLE_BINDING
            if (1)
#else
            if (sink_dev->attempt < reconnect_intervals_len)
#endif
            {
                if (src_dev_is_bonded(i))
                {
                    app_start_timer(&timer_idx_src_reconn[i], "src_reconn",
                                    src_policy_timer_id, SRC_TIMER_ID_RECONN + i, 0, false,
#if F_APP_LEGACY_DONGLE_BINDING
                                    2000);
#else
                                    1000 * reconnect_intervals[sink_dev->attempt]);
#endif
                    break;
                }
                else
                {
                    APP_PRINT_INFO1("dev %u has no bond info", i);
                    /* No bonded device, exit */
                    memset(sink_dev, 0, sizeof(T_APP_DEV));
                    src_led_set_idle(i);
                }
            }
            else
            {
                /* Attempt exhausts */
                sink_dev->dev_state = STATE_DEV_IDLE;
                src_led_set_idle(i);
            }
        }
        else if (sink_dev->dev_state == STATE_DEV_CONNECTING)
        {
#if F_APP_LEGACY_DONGLE_BINDING
            if (1)
#else
            if (sink_dev->attempt < connect_attempt_count)
#endif
            {
                app_start_timer(&timer_idx_src_conn[i], "src_conn",
                                src_policy_timer_id, SRC_TIMER_ID_CONN + i, 0, false,
                                1000);
                break;
            }
            else
            {
                /* Attempt exhausts */
                sink_dev->dev_state = STATE_DEV_IDLE;
                src_led_set_idle(i);
            }
        }
    }

    if (j == SINK_DEV_NUM_MAX)
    {
        /* No dev to be schduled, enable connectable mode if necessary */
        src_check_set_bt_mode();
        if (!autopairing)
        {
            return;
        }
        for (i = 0; i < SINK_DEV_NUM_MAX; i++)
        {
            if (src_auto_link_cfg.single_link && i > 0)
            {
                continue;
            }
            sink_dev = &sink_dev_list[i];
            /* if connecting attempts exhaust, we restart autopairing again. */
            if (sink_dev->dev_state == STATE_DEV_IDLE)
            {
                APP_PRINT_INFO0("src_dev_schedule_next(): Autopairing");
                autopairing_index = i;
                connect_attempt_count = 0xff;
                /* Similar to long key press */
                src_enter_connecting_state(i);
                if (!src_bt_is_busy())
                {
                    src_start_conn(i);
                }
                break;
            }
        }
    }
}

//autopair/reconn result
static void src_handle_action_result(uint8_t index, bool conn_ok)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    uint32_t bond_flag;

    APP_PRINT_INFO4("src_handle_action_result: index %d, conn_ok %d, dev_state %02x, attempt %d",
                    index, conn_ok, sink_dev->dev_state, sink_dev->attempt);

    app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
    sink_dev_list[index].connecting = false;

    if (conn_ok)
    {
        sink_dev->dev_state = STATE_DEV_CONNECTED;
        sink_dev->attempt = 0;
        sink_dev->inquiry_dev_found = 0;
        memset(sink_dev->remote_info.gaming_bdaddr, 0, 6);

        src_led_set_connected(index);

        bond_flag = app_src_legacy_get_bond_flag_by_index(index);
        APP_PRINT_INFO1("src_handle_action_result: bond_flag %08x", bond_flag);
    }
    else
    {
        sink_dev->attempt++;

#if F_APP_LEGACY_DONGLE_BINDING
        if ((sink_dev->dev_state == STATE_DEV_RECONNECTING) ||
            (sink_dev->dev_state == STATE_DEV_CONNECTING))
#else
        if ((sink_dev->dev_state == STATE_DEV_RECONNECTING &&
             sink_dev->attempt < reconnect_intervals_len) ||
            (sink_dev->dev_state == STATE_DEV_CONNECTING &&
             sink_dev->attempt < connect_attempt_count))
#endif
        {
            T_APP_DEV *dev = NULL;

            /* Retry later, keep dev_state & attempt only */
            APP_PRINT_INFO4("src_handle_action_result1: index %d, conn_ok %d, dev_state %02x, attempt %d",
                            index, conn_ok, sink_dev->dev_state, sink_dev->attempt);

            dev = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof(T_APP_DEV));
            if (!dev)
            {
                APP_PRINT_ERROR1("src_handle_action_result: alloc dev %u err",
                                 index);
                memset(sink_dev, 0, sizeof(*sink_dev));
                src_led_set_idle(index);
                goto sched_next;
            }

            memcpy(dev, sink_dev, sizeof(*sink_dev));

            /* bt state is set to IDLE */
            memset(sink_dev, 0, sizeof(T_APP_DEV));

            sink_dev->dev_state = dev->dev_state;
            sink_dev->attempt = dev->attempt;
            /* Save the mask plan.
             * Remote may reconnect to us before the reconnect timer expired
             */
            sink_dev->profile_mask_plan = dev->profile_mask_plan;
            /* Sometimes, we reconnected failed, and before we reconnect again,
             * remote connects to us.
             */
            sink_dev->profile_mask_remain = dev->profile_mask_plan;
            /* Save the address for the case that remote connects before timer
             * timeout
             */
            memcpy(sink_dev->remote_info.bd_addr, dev->remote_info.bd_addr, 6);
            memcpy(sink_dev->remote_info.gaming_bdaddr,
                   dev->remote_info.gaming_bdaddr, 6);

            os_mem_free(dev);

            APP_PRINT_INFO4("src_handle_action_result2: index %d, conn_ok %d, dev_state %02x, attempt %d",
                            index, conn_ok, sink_dev->dev_state, sink_dev->attempt);
        }
        else
        {
            memset(sink_dev, 0, sizeof(T_APP_DEV));
            src_led_set_idle(index);
        }
    }

sched_next:
    src_dev_schedule_next();
}

static bool src_get_inquiry_dev_index(uint8_t *index)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].bt_state == STATE_BT_INQUIRY)
        {
            break;
        }
    }

    if (i < SINK_DEV_NUM_MAX)
    {
        *index = i;
        return true;
    }
    else
    {
        return false;
    }
}

static void src_handle_inquiry_rsp(uint16_t cause)
{
    uint8_t index = 0;

    if (discov_state != DISCOVERY_STARTING)
    {
        APP_PRINT_ERROR1("src_handle_inquiry_rsp: wrong discov state %u",
                         discov_state);
    }

    if (!cause)
    {
        discov_state = DISCOVERY_FINDING;
        if (bt_uapi_cback)
        {
            bt_uapi_cback(EVENT_GAMING_DISCOVERY_STARTED, NULL, 0);
        }
    }
    else
    {
        discov_state = DISCOVERY_STOPPED;
        if (bt_uapi_cback)
        {
            bt_uapi_cback(EVENT_GAMING_DISCOVERY_STOPPED, NULL, 0);
        }
    }

    if (src_get_inquiry_dev_index(&index))
    {

        APP_PRINT_INFO2("src_handle_inquiry_rsp: cause %d, dev_state 0x%04x",
                        cause, sink_dev_list[index].dev_state);

        if (cause)
        {
            src_handle_action_result(index, false);
        }
    }

    /* The discov_state might be set to DISCOVERYING_STARTING when autopairing
     * was restarted in src_dev_schedule_next() in src_handle_action_result().
     * Don't worry. Correct state will be informed to upper layer at that
     * place.
     * */
}

static bool src_already_connecting(uint8_t index, uint8_t *bdaddr)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i ++)
    {
        if (i == index)
        {
            continue;
        }
        /* TODO: Should check the dev_state? */
        if (!memcmp(sink_dev_list[i].remote_info.bd_addr, bdaddr, 6))
        {
            return true;
        }
    }

    return false;
}

static void src_handle_inquiry_res(T_BT_EVENT_PARAM_INQUIRY_RESULT inquiry_result)
{
    uint8_t index = 0;
    T_APP_DEV *sink_dev;
#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
    uint8_t lock_addr[6];
#endif

    if (src_get_inquiry_dev_index(&index))
    {
        sink_dev = &sink_dev_list[index];

        if ((!sink_dev->inquiry_dev_found) &&
            (sink_dev->dev_state == STATE_DEV_CONNECTING))
        {
            if ((inquiry_result.cod & REMOTE_COD_MASK) &&
                (inquiry_result.rssi >= app_cfg_const.dongle_autopair_rssi))
            {

                if (app_find_br_link(inquiry_result.bd_addr) == NULL)
                {
                    uint8_t pre_state;
                    uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

                    /* This dev belongs to another link, don't touch it */
                    if (src_already_connecting(index, inquiry_result.bd_addr))
                    {
                        goto done;
                    }

                    if (!memcmp(ba_any, inquiry_result.bd_addr, 6))
                    {
                        APP_PRINT_ERROR0("src_handle_inquiry_res: empty dev bdaddr");
                        goto done;
                    }

                    if (memcmp(sink_dev->remote_info.gaming_bdaddr, ba_any, 6))
                    {
                        if (memcmp(sink_dev->remote_info.gaming_bdaddr,
                                   inquiry_result.bd_addr, 6))
                        {
                            goto done;
                        }

                        /* gaming_bdaddr is same as remote dev bdaddr. */
                    }
#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
                    /* This function is for 1V1 lock. If dongle is bound with a headset,
                       only this headset can be connected.*/
                    if (app_src_legacy_get_lock_flag_by_index(0) &&
                        app_src_legacy_get_lock_addr_by_index(0, lock_addr))
                    {
                        if (memcmp(lock_addr, inquiry_result.bd_addr, 6) != 0)
                        {
                            goto done;
                        }
                    }
#endif
                    src_eir_parse(sink_dev, inquiry_result.p_eir,
                                  inquiry_result.eir_len);

                    /* Autopairing, autopairing private headset, and found
                     * headset.
                     */
                    if (autopairing && !src_auto_link_cfg.autopair_general)
                    {
                        if (!sink_dev->headset_info_valid)
                        {
                            sink_dev->headset_info_valid = false;
                            memset(sink_dev->headset_info, 0,
                                   sizeof(sink_dev->headset_info));
                            goto done;
                        }
                        if (inquiry_result.rssi < src_auto_link_cfg.autopair_rssi)
                        {
                            sink_dev->headset_info_valid = false;
                            memset(sink_dev->headset_info, 0,
                                   sizeof(sink_dev->headset_info));
                            goto done;
                        }
                    }

                    memcpy(sink_dev->remote_info.bd_addr, inquiry_result.bd_addr, 6);
                    sink_dev->remote_info.cod = inquiry_result.cod;
                    memcpy(sink_dev->remote_info.name, inquiry_result.name, GAP_DEVICE_NAME_LEN);
                    sink_dev->inquiry_dev_found = true;

                    /* wait for inquiry cancel rsp, then connect to dev */
                    pre_state = discov_state;
                    discov_state = DISCOVERY_STOPPING;
                    if (gap_br_stop_inquiry())
                    {
                        APP_PRINT_ERROR0("src_handle_inquiry_res: Stop inquiry err");
                        discov_state = pre_state;
                    }
                    else
                    {
                        if (bt_uapi_cback)
                        {
                            bt_uapi_cback(EVENT_GAMING_DISCOVERY_STOPPING, NULL, 0);
                        }
                    }
                }
                else
                {
                    APP_PRINT_TRACE1("src_handle_inquiry_res: %s already connected",
                                     TRACE_BDADDR(inquiry_result.bd_addr));
                }
            }
        }
    }

done:
    if ((inquiry_result.cod & REMOTE_COD_MASK) &&
        (inquiry_result.rssi >= REMOTE_RSSI_LEVEL_MIN) &&
        client_discovering && bt_uapi_cback)
    {
        /* TODO: It would be better if there is no cast. */
        struct gaming_ev_device_found *ev = (void *)&inquiry_result;

        bt_uapi_cback(EVENT_GAMING_DEVICE_FOUND, (uint8_t *)ev,
                      sizeof(*ev));
    }

    return;
}

static void src_handle_inquiry_cmpl(uint16_t cause)
{
    uint8_t index = 0;

    discov_state = DISCOVERY_STOPPED;
    if (bt_uapi_cback)
    {
        bt_uapi_cback(EVENT_GAMING_DISCOVERY_STOPPED, NULL, 0);
    }

    if (src_get_inquiry_dev_index(&index))
    {
        APP_PRINT_INFO2("src_handle_inquiry_cmpl: cause 0x%04x, dev_state %d",
                        cause, sink_dev_list[index].dev_state);

        if (sink_dev_list[index].dev_state == STATE_DEV_CONNECTING)
        {
            src_handle_action_result(index, false);
        }
    }

    /* The discov_state might be set to DISCOVERYING_STARTING when autopairing
     * was restarted in src_dev_schedule_next() in src_handle_action_result().
     * */

    if (client_discovering && discov_state == DISCOVERY_STOPPED)
    {
        /* User may want to continue discovering. */
        discov_state = DISCOVERY_STARTING;
        if (gap_br_start_inquiry(false, 10))
        {
            APP_PRINT_ERROR0("src_handle_inquiry_cmpl: Start inquiry err");
            discov_state = DISCOVERY_STOPPED;
        }
        else
        {
            if (bt_uapi_cback)
            {
                bt_uapi_cback(EVENT_GAMING_DISCOVERY_STARTING, NULL, 0);
            }
        }
    }
}

/* FIXME: We assume inquiry cancel cmpl and inquiry cmpl are mutually exclusive.
 * Actually at low level, it is possible that we receive Inquiry Complete event
 * right before we receive Inquiry Cancel Command Complete event, in which case
 * the latter event should have status of Command Disallowed (0x0c). This
 * should not be treated as error.
 * */
static void src_handle_inquiry_cancel_cmpl(uint16_t cause)
{
    uint8_t index = 0;
    T_APP_DEV *sink_dev = NULL;
    T_APP_SRC_KEY_EVENT pending_evt = KEY_NONE;

    discov_state = DISCOVERY_STOPPED;
    if (bt_uapi_cback)
    {
        bt_uapi_cback(EVENT_GAMING_DISCOVERY_STOPPED, NULL, 0);
    }

    if (src_get_inquiry_dev_index(&index))
    {
        sink_dev = &sink_dev_list[index];
        pending_evt = sink_dev->pending_event;

        switch (sink_dev->dev_state)
        {
        case STATE_DEV_CONNECTING:
            {
                /* NOTE: if device is found, we connect to dev, and upper layer
                 * needs to restart if it wants inquiry.
                 *
                 * If inquiry was canceled by upper layer, inquiry_dev_found
                 * might be false. In this case, we schedule next.
                 * */
                if (sink_dev->inquiry_dev_found)
                {
                    src_dev_conn(index, sink_dev->remote_info.bd_addr,
                                 A2DP_PROFILE_MASK |
                                 SPP_AUDIO_PROFILE_MASK |
                                 AVRCP_PROFILE_MASK);
                    return;
                }
                else
                {
                    /* Inquiry will be started later after one second. Or it is
                     * autopairing now, the discov_state might be set to
                     * DISCOVERYING_STARTING when autopairing was restarted in
                     * src_dev_schedule_next() in src_handle_action_result().
                     * */
                    src_handle_action_result(index, false);
                }
            }
            break;

        case STATE_DEV_CLEANUP:
            {
                memset(sink_dev, 0, sizeof(T_APP_DEV));
                src_led_set_idle(index);

                /* For pending event, treat as in IDLE state.
                 *
                 * Inquiry might be started in the below function.
                 * */
                src_handle_dev_pending_evt(index, pending_evt);
            }
            break;

        default:
            break;
        }
    }

    /* The discov_state might be DISCOVERY_STOPPED or DISCOVERY_STARTING. */

    if (client_discovering && discov_state == DISCOVERY_STOPPED)
    {
        discov_state = DISCOVERY_STARTING;
        if (gap_br_start_inquiry(false, 10))
        {
            APP_PRINT_ERROR0("src_handle_inquiry_cancel_cmpl: start inquiry err");
            discov_state = DISCOVERY_STOPPED;
        }
        else
        {
            if (bt_uapi_cback)
            {
                bt_uapi_cback(EVENT_GAMING_DISCOVERY_STARTING, NULL, 0);
            }
        }
    }
}

static void src_handle_acl_conn_req(uint8_t *bd_addr)
{
    uint8_t index;
//    uint8_t i;
    uint8_t link_num = 0;
    uint8_t equiment_addr1[3] = {0x00, 0x00, 0x91};
    uint8_t equiment_addr2[3] = {0x00, 0x02, 0xB1};

    if (shutdown)
    {
        APP_PRINT_INFO0("src_handle_acl_conn_req: shutdown get conn req, reject");
        bt_acl_conn_reject(bd_addr, BT_ACL_REJECT_LIMITED_RESOURCE);
        return;
    }

    for (index = 0; index < SINK_DEV_NUM_MAX; index++)
    {
        APP_PRINT_INFO2("src_handle_acl_conn_req: %s %d",
                        TRACE_BDADDR(sink_dev_list[index].remote_info.bd_addr),
                        sink_dev_list[index].dev_state);
        if (sink_dev_list[index].bt_state == STATE_BT_ACL_CONNECTED)
        {
            link_num++;
        }
    }

    if (link_num >= 2)
    {
        APP_PRINT_ERROR1("src_handle_acl_conn_req: exceed max %u conn, reject",
                         link_num);
        bt_acl_conn_reject(bd_addr, BT_ACL_REJECT_LIMITED_RESOURCE);
        return;
    }

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        if (src_auto_link_cfg.single_link && index > 0)
        {
            APP_PRINT_WARN1("src_handle_acl_conn_req(): single link, 1rej %u",
                            index);
            bt_acl_conn_reject(bd_addr, BT_ACL_REJECT_LIMITED_RESOURCE);
            return;
        }
        /* We are connecting or reconnecting. headset_info[2] bit3 indicates whether
         * the headset support the mix function. if supported, dongle is a slave.
         * */
        if (app_eir_parse_dongle_need_to_be_slave(index))
        {
            bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_SLAVE);
        }
        else
        {
            bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_MASTER);
        }
        current_index = index;
        sink_dev_list[index].connecting = true;
    }
    else if (src_policy_find_dev_index_by_bond_info(bd_addr, &index))
    {
        if (src_auto_link_cfg.single_link && index > 0)
        {
            APP_PRINT_WARN1("src_handle_acl_conn_req(): single link, 2rej %u",
                            index);
            bt_acl_conn_reject(bd_addr, BT_ACL_REJECT_LIMITED_RESOURCE);
            return;
        }
        /* We are in STATE_DEV_IDLE but have bond info */
        if (app_eir_parse_dongle_need_to_be_slave(index))
        {
            bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_SLAVE);
        }
        else
        {
            bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_MASTER);
        }
        current_index = index;
        sink_dev_list[index].connecting = true;
        memcpy(sink_dev_list[index].remote_info.bd_addr, bd_addr, 6);
    }
    else if (ota_mode)
    {
        bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_MASTER);
    }
    else if ((memcmp(bd_addr, equiment_addr1, 3)) || (memcmp(bd_addr, equiment_addr2, 3)))
    {
        /*When the 8852B device initiates a connection , acl connection is allowed*/
        bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_MASTER);
    }
    else
    {
        /* bt_acl_conn_reject(bd_addr, BT_ACL_REJECT_LIMITED_RESOURCE); */
        bt_acl_conn_reject(bd_addr, BT_ACL_REJECT_LIMITED_RESOURCE);
        return;
#if 0
        if (src_auto_link_cfg.single_link)
        {
            for (i = 0; i < SINK_DEV_NUM_MAX; i++)
            {
                if (sink_dev_list[i].bt_state == STATE_BT_ACL_CONNECTED)
                {
                    APP_PRINT_INFO0("src_handle_acl_conn_req(): reject unknown device");
                    bt_acl_conn_reject(bd_addr, BT_ACL_REJECT_LIMITED_RESOURCE);
                    return;
                }
            }
        }
        /* No device connecting, no device bond and not ota mode.
         * Maybe remote wants to connect our spp and let dongle enter mp mode.
         */
        for (i = 0; i < SINK_DEV_NUM_MAX; i++)
        {
            if (sink_dev_list[i].bt_state != STATE_BT_ACL_CONNECTED)
            {
                bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_MASTER);
                current_index = i;
                sink_dev_list[i].connecting = true;
                memcpy(sink_dev_list[i].remote_info.bd_addr, bd_addr, 6);
                return;
            }
        }

        if (i >= SINK_DEV_NUM_MAX)
        {
            APP_PRINT_INFO0("src_handle_acl_conn_req(): no resource");
            bt_acl_conn_reject(bd_addr, BT_ACL_REJECT_LIMITED_RESOURCE);
        }
#endif
    }
}

static void idle_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("idle_handle_acl_status: %s, status %02x, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), event_type, sink_dev->bt_state);

    //TODO: connected by remote

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link == NULL)
            {
                p_link = app_alloc_br_link(para->bd_addr);

            }

            gap_br_cfg_acl_link_flush_tout(para->bd_addr, AVDTP_STREAM_DATA_FLUSH_TIMEOUT);

            sink_dev->bt_state = STATE_BT_ACL_CONNECTED;

            app_start_timer(&sink_dev->timer_idx_acl_exist_delay, "acl_init_from_remote",
                            src_policy_timer_id, SRC_TIMER_ID_ACL_EXIST_SINK0 + index, 0, false,
                            SRC_ACL_EXIST_DELAY_TIMEOUT);
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("idle_handle_acl_status: disconnected, %04x",
                            para->cause);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            sink_dev->bt_state = STATE_BT_IDLE;
            sink_dev->profile_mask_doing = 0;
            sink_dev->codec_configured = false;
            app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            /* bt_link_role_switch(info->bd_addr, true); */
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
        break;

    default:
        break;
    }
}

static void reconnecting_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("reconnecting_handle_acl_status: %s, status %02x, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), event_type, sink_dev->bt_state);

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link == NULL)
            {
                p_link = app_alloc_br_link(para->bd_addr);
            }

            if (sink_dev->bt_state != STATE_BT_PAGING)
            {
                //should be non-active state, connected from remote
                //re-useacl exists timer

                app_start_timer(&sink_dev->timer_idx_acl_exist_delay, "acl_init_from_remote",
                                src_policy_timer_id, SRC_TIMER_ID_ACL_EXIST_SINK0 + index, 0, false,
                                SRC_ACL_EXIST_DELAY_TIMEOUT);
            }

            gap_br_cfg_acl_link_flush_tout(para->bd_addr, AVDTP_STREAM_DATA_FLUSH_TIMEOUT);

            sink_dev->bt_state = STATE_BT_ACL_CONNECTED;

        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {
            APP_PRINT_INFO1("reconnecting_handle_acl_status: conn fail 0x%04x",
                            para->cause);

            if (sink_dev->bt_state == STATE_BT_PAGING)
            {
                if (para->cause == (HCI_ERR | HCI_ERR_ACL_CONN_EXIST))
                {
                    //continue to do sdp did
                    //gap_br_start_did_discov(info->bd_addr);

                    /* Currently local is service discovering. There are two
                     * scenarios.
                     * 1. remote connects to our profile before sdp cmpl
                     * 2. sdp cmpl first
                     * Don't worry about the cases,
                     * src_handle_profile_conn_cmpl() and src_handle_sdp_cmpl()
                     * would handle them properly.
                     * */

                    app_start_timer(&sink_dev->timer_idx_acl_exist_delay, "acl_exist_delay",
                                    src_policy_timer_id, SRC_TIMER_ID_ACL_EXIST_SINK0 + index, 0, false,
                                    SRC_ACL_EXIST_DELAY_TIMEOUT);
                }
                else
                {
                    sink_dev->bt_state = STATE_BT_IDLE;
                    app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
                    src_handle_action_result(index, false);
                }
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("reconnecting_handle_acl_status: disconnected, %04x",
                            para->cause);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);

            sink_dev->bt_state = STATE_BT_IDLE;

            sink_dev->profile_mask_doing = 0;
            sink_dev->codec_configured = false;

            /* Try to reconnect again */
            src_handle_action_result(index, false);
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            /* bt_link_role_switch(info->bd_addr, true); */
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            if ((para->cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)) ||
                (para->cause == (HCI_ERR | HCI_ERR_KEY_MISSING)))
            {
                uint8_t bond_index;

                APP_PRINT_INFO1("reconnecting_handle_acl_status: %u auth fail",
                                index);
                /* We are using obsolete bond info that is held in another
                 * place.
                 */
                if (src_policy_find_dev_index_by_bond_info(para->bd_addr,
                                                           &bond_index))
                {
                    APP_PRINT_INFO1("reconnecting_handle_acl_status(): Remove %u bond",
                                    bond_index);
                    app_src_legacy_delete_bond_by_index(bond_index);
                }
            }
        }
        break;

    default:
        break;
    }
}

static void connecting_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("connecting_handle_acl_status: %s, status %d, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), event_type, sink_dev->bt_state);

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link == NULL)
            {
                p_link = app_alloc_br_link(para->bd_addr);
            }

            gap_br_cfg_acl_link_flush_tout(para->bd_addr, AVDTP_STREAM_DATA_FLUSH_TIMEOUT);

            sink_dev->bt_state = STATE_BT_ACL_CONNECTED;
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            if ((para->cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)) ||
                (para->cause == (HCI_ERR | HCI_ERR_KEY_MISSING)))
            {
                uint8_t bond_index;

                //wait disconnect complete event to cleanup dev
                if (src_policy_find_dev_index_by_bond_info(para->bd_addr,
                                                           &bond_index))
                {
                    APP_PRINT_INFO1("connecting_handle_acl_status(): Remove %u bond",
                                    bond_index);
                    app_src_legacy_delete_bond_by_index(bond_index);
                }

                if (para->cause == (HCI_ERR | HCI_ERR_KEY_MISSING))
                {
                    if (app_cfg_const.enable_dongle_dual_mode && scan_by_mmi)
                    {
                        /* scan bud adv then try to connect to bud but key missing,
                        wait acl disconnect cmpl and start to scan again */
                        gaming_bt_disconnect_by_id(0);
                    }
                }
            }
        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {
            APP_PRINT_INFO1("connecting_handle_acl_status: conn fail 0x%04x",
                            para->cause);

            if (sink_dev->bt_state == STATE_BT_PAGING)
            {
                if (para->cause == (HCI_ERR | HCI_ERR_ACL_CONN_EXIST))
                {
                    //continue to do sdp did
                    //gap_br_start_did_discov(info->bd_addr);
                    app_start_timer(&sink_dev->timer_idx_acl_exist_delay, "acl_exist_delay",
                                    src_policy_timer_id, SRC_TIMER_ID_ACL_EXIST_SINK0 + index, 0, false,
                                    SRC_ACL_EXIST_DELAY_TIMEOUT);
                }
                else
                {
                    sink_dev->bt_state = STATE_BT_IDLE;
                    /* Try to connect again */
                    src_handle_action_result(index, false);
                }
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("connecting_handle_acl_status: disconnected, 0x%04x",
                            para->cause);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            if (sink_dev->bt_state == STATE_BT_ACL_CONNECTED)
            {
                sink_dev->bt_state = STATE_BT_IDLE;
                sink_dev->profile_mask_doing = 0;
                sink_dev->codec_configured = false;
                src_handle_action_result(index, false);
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            /* bt_link_role_switch(info->bd_addr, true); */
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
        break;

    default:
        break;
    }
}

static void connected_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("connected_handle_acl_status: %s, status %d, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), event_type, sink_dev->bt_state);

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("connected_handle_acl_status: disconnected, 0x%04x",
                            para->cause);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            /* The dev state changes from CONNECTED to IDLE
             * codec_configured changes to false
             */
            memset(sink_dev, 0, sizeof(T_APP_DEV));
            a2dp_reconn_timeout[index] = false;

            src_led_set_idle(index);

            if (para->cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
            {
                /* Don't worry. Schedule will also check if bt should be set to
                 * connectable
                 */
                if (!src_bt_is_busy())
                {
                    src_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
                }
                else
                {
                    //wait for reschdule
                }
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            /* bt_link_role_switch(info->bd_addr, true); */
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            if ((para->cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)) ||
                (para->cause == (HCI_ERR | HCI_ERR_KEY_MISSING)))
            {
                uint8_t bond_index;

                //wait disconnect complete event to cleanup dev
                if (src_policy_find_dev_index_by_bond_info(para->bd_addr,
                                                           &bond_index))
                {
                    APP_PRINT_INFO1("connected_handle_acl_status(): Remove %u bond",
                                    bond_index);
                    app_src_legacy_delete_bond_by_index(bond_index);
                }
            }
        }
        break;

    default:
        break;
    }
}

static void cleanup_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_SRC_KEY_EVENT pending_evt = sink_dev->pending_event;

    APP_PRINT_INFO3("cleanup_handle_acl_status: %s, status %d, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), event_type, sink_dev->bt_state);

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            APP_PRINT_INFO0("cleanup_handle_acl_status: connected, disconnect");

            /* For stopping sdp, maybe acl conn first, then disconn.
             *
             * Stack will automatic disconn acl
             */
            if (sink_dev->bt_state == STATE_BT_PAGING)
            {
                APP_PRINT_INFO0("cleanup_handle_acl_status(): paging success");
                gap_br_send_acl_disconn_req(para->bd_addr);
            }
            else
            {
                APP_PRINT_WARN0("cleanup_handle_acl_status(): Remote conn?");
            }
        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {
            APP_PRINT_INFO1("cleanup_handle_acl_status: conn fail, %04x",
                            para->cause);

            app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
            memset(sink_dev, 0, sizeof(T_APP_DEV));
            src_led_set_idle(index);
            src_handle_dev_pending_evt(index, pending_evt);
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("cleanup_handle_acl_status: disconnected, %04x",
                            para->cause);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            if (sink_dev->stream_info.buffer != NULL)
            {
                os_mem_free(sink_dev->stream_info.buffer);
            }

            app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
            memset(sink_dev, 0, sizeof(T_APP_DEV));
            src_led_set_idle(index);
            src_handle_dev_pending_evt(index, pending_evt);

            if (app_cfg_const.enable_dongle_dual_mode && scan_by_mmi)
            {
                /* restart to scan after acl disconnect if pairing fail */
                uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

                gaming_bt_force_connect(0, ba_any);
            }
        }
        break;

    default:
        break;
    }
}

static void src_ota_handle_acl_status(T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;

    APP_PRINT_INFO2("src_ota_handle_acl_status: %s, status %d, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), event_type);

    //TODO: connected by remote

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link == NULL)
            {
                p_link = app_alloc_br_link(para->bd_addr);
            }

        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("src_ota_handle_acl_status: disconnected, %04x",
                            para->cause);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
        break;

    default:
        break;
    }
}

/* static void src_handle_acl_status_spp_mode(T_GAP_ACL_STATUS_INFO *info)
 * {
 *     T_APP_BR_LINK *p_link;
 *
 *     APP_PRINT_INFO2("src_handle_acl_status_spp_mode: %s, status %d, bt_state %d",
 *                     TRACE_BDADDR(info->bd_addr), info->status);
 *
 *     switch (info->status)
 *     {
 *     case GAP_ACL_CONN_SUCCESS:
 *         {
 *             p_link = app_find_br_link(info->bd_addr);
 *             if (p_link == NULL)
 *             {
 *                 p_link = app_alloc_br_link(info->bd_addr);
 *                 p_link->acl_handle = info->p.conn_success.handle;
 *             }
 *
 *         }
 *         break;
 *
 *     case GAP_ACL_CONN_DISCONN:
 *         {
 *             APP_PRINT_INFO1("src_handle_acl_status_spp_mode: disconnected, %04x",
 *                             info->p.conn_fail.cause);
 *
 *             p_link = app_find_br_link(info->bd_addr);
 *             if (p_link != NULL)
 *             {
 *                 app_free_br_link(p_link);
 *             }
 *         }
 *         break;
 *
 *     case GAP_ACL_CONN_READY:
 *         {
 *             bt_acl_pkt_type_set(info->bd_addr, BT_ACL_PKT_TYPE_2M);
 *         }
 *         break;
 *
 *     default:
 *         break;
 *     }
 * }
 */

void src_handle_pkt_type(uint8_t *addr, bool enable_3M)
{
    uint16_t pkt_type = 0;

    if (enable_3M)
    {
        pkt_type = GAP_PKT_TYPE_DM1 | GAP_PKT_TYPE_DH1 | \
                   GAP_PKT_TYPE_DM3 | GAP_PKT_TYPE_DH3 | \
                   GAP_PKT_TYPE_DM5 | GAP_PKT_TYPE_DH5 | \
                   GAP_PKT_TYPE_NO_2DH1 | GAP_PKT_TYPE_NO_2DH3 | GAP_PKT_TYPE_NO_2DH5;
    }
    else
    {
        pkt_type = GAP_PKT_TYPE_DM1 | GAP_PKT_TYPE_DH1 | \
                   GAP_PKT_TYPE_DM3 | GAP_PKT_TYPE_DH3 | \
                   GAP_PKT_TYPE_DM5 | GAP_PKT_TYPE_DH5 | \
                   GAP_PKT_TYPE_NO_3DH1 | GAP_PKT_TYPE_NO_3DH3 | GAP_PKT_TYPE_NO_3DH5;
    }

    APP_PRINT_TRACE2("src_handle_pkt_type: %s enable_3M %d", TRACE_BDADDR(addr), enable_3M);

    gap_br_cfg_acl_pkt_type(addr, pkt_type);
}

static void src_handle_acl_link_status(T_BT_PARAM *para, T_BT_EVENT event_type)
{
    uint8_t index = 0;

    APP_PRINT_INFO2("src_handle_acl_link_status: %s, %02x",
                    TRACE_BDADDR(para->bd_addr), event_type);

    if (event_type == BT_EVENT_ACL_CONN_READY)
    {
        src_handle_pkt_type(para->bd_addr, false);
    }

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (event_type == BT_EVENT_ACL_CONN_FAIL || event_type == BT_EVENT_ACL_CONN_DISCONN)
    {
        adapter_link_handle_headset_conn_status(HEADSET_PHONE_STATUS_UNKOWN);
    }
#endif

    if (src_policy_find_dev_index_by_addr(para->bd_addr, &index) ||
        src_policy_find_dev_index_by_bond_info(para->bd_addr, &index))
    {
        APP_PRINT_INFO1("src_handle_acl_link_status: dev state %04x",
                        sink_dev_list[index].dev_state);

        if (event_type == BT_EVENT_ACL_CONN_SUCCESS)
        {
            if (bt_uapi_cback)
            {
                struct gaming_ev_device_connected ev;

                ev.id = index;
                memcpy(ev.ba, para->bd_addr, 6);
                bt_uapi_cback(EVENT_GAMING_DEVICE_CONNECTED, (void *)&ev,
                              sizeof(ev));
            }
            if (app_eir_parse_dongle_need_to_be_slave(index)) //for mixing
            {
                legacy_gaming_xfer_start(para->bd_addr, LEGACY_GAMING_XFER_1);
            }
        }
        else if (event_type == BT_EVENT_ACL_CONN_FAIL ||
                 event_type == BT_EVENT_ACL_CONN_DISCONN)
        {
            /* spp_test_mode = false; */
            sink_dev_list[index].connecting = false;
            sink_dev_list[index].headset_info_valid = false;
            sink_dev_list[index].stream_info.mtu = 0;
            memset(sink_dev_list[index].headset_info, 0,
                   sizeof(sink_dev_list[index].headset_info));

            if (sink_dev_list[index].bt_state == STATE_BT_ACL_CONNECTED &&
                bt_uapi_cback)
            {
                struct gaming_ev_device_disconnected ev;

                ev.id = index;
                memcpy(ev.ba, para->bd_addr, 6);
                bt_uapi_cback(EVENT_GAMING_DEVICE_DISCONNECTED, (void *)&ev,
                              sizeof(ev));
            }

            /* We don't change the device dev_state, because we want the device
             * to be picked up for next or next next (re)connecting.
             *
             * The dev_state might be set to STATE_DEV_CLEANUP when we
             * terminated the connection when the dev is in connecting or
             * reconnecting or connected state.
             * */
        }
        else if (event_type == BT_EVENT_ACL_CONN_READY)
        {
            if (app_eir_parse_dongle_need_to_be_slave(index))
            {
                bt_link_role_switch(para->bd_addr, false);
            }
        }
        else if (event_type == BT_EVENT_ACL_ROLE_SLAVE)
        {
            if (app_eir_parse_dongle_need_to_be_slave(index)) //for mixing
            {
                legacy_gaming_xfer_stop(para->bd_addr, LEGACY_GAMING_XFER_1);
            }
        }

        switch (sink_dev_list[index].dev_state)
        {
        case STATE_DEV_IDLE:
            {
                idle_handle_acl_status(index, para, event_type);
            }
            break;

        case STATE_DEV_RECONNECTING:
            {
                reconnecting_handle_acl_status(index, para, event_type);
            }
            break;

        case STATE_DEV_CONNECTING:
            {
                connecting_handle_acl_status(index, para, event_type);
            }
            break;

        case STATE_DEV_CONNECTED:
            {
                connected_handle_acl_status(index, para, event_type);
            }
            break;

        case STATE_DEV_CLEANUP:
            {
                cleanup_handle_acl_status(index, para, event_type);
            }
            break;

        default:
            break;
        }
    }
    else
    {
        if (ota_mode)
        {
            src_ota_handle_acl_status(para, event_type);
        }
        /* else if (spp_test_mode)
         * {
         *     src_handle_acl_status_spp_mode(info);
         * }
         */
    }

    src_check_set_bt_mode();
}

static void src_handle_link_key_info(uint8_t *bd_addr, uint8_t *link_key,
                                     T_BT_LINK_KEY_TYPE key_type)
{
    uint8_t index = 0;

    /* if (spp_test_mode)
     * {
     *     APP_PRINT_INFO0("src_handle_link_key_info(): spp test");
     *     return;
     * }
     */

    bool is_save_link_key_info;

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    is_save_link_key_info = !memcmp(bd_addr, app_cfg_nv.public_bud_addr, 6);
#else
    is_save_link_key_info = src_policy_find_dev_index_by_addr(bd_addr, &index) ||
                            src_policy_find_dev_index_by_bond_info(bd_addr, &index);
#endif

    if (is_save_link_key_info)
    {
        if (src_auto_link_cfg.single_link && index > 0)
        {
            APP_PRINT_WARN1("src_handle_link_key_info(): single link, ignore %u",
                            index);
            return;
        }
        app_src_legacy_save_bond(index, bd_addr, link_key, key_type);
        if (sink_dev_list[index].headset_info_valid)
        {
            app_src_legacy_add_hs_info(index, bd_addr,
                                       sink_dev_list[index].headset_info);
        }
    }
    else
    {
        //save in OTA mode ???
    }
}

static void src_handle_link_key_req(uint8_t *bd_addr)
{
    uint8_t index;
    uint8_t link_key[16];
    T_BT_LINK_KEY_TYPE key_type = BT_LINK_KEY_TYPE_COMBINATION;

    /* if (spp_test_mode)
     * {
     *     APP_PRINT_INFO0("src_handle_link_key_req(): spp test");
     *     bt_link_key_cfm(bd_addr, false, key_type, link_key);
     *     return;
     * }
     */

    if ((src_policy_find_dev_index_by_addr(bd_addr, &index) ||
         src_policy_find_dev_index_by_bond_info(bd_addr, &index)) &&
        app_src_legacy_get_bond_by_addr(bd_addr, link_key, &key_type))
    {
        uint32_t bond_flag = app_src_legacy_get_bond_flag_by_addr(bd_addr);
        uint8_t info[4] = { 0, 0, 0, 0 };

        if (src_auto_link_cfg.single_link && index > 0)
        {
            APP_PRINT_WARN1("src_handle_link_key_req(): single link, ignore %u",
                            index);
            bt_link_key_cfm(bd_addr, false, key_type, link_key);
            return;
        }

        if (bond_flag & APP_SRC_BOND_FLAG_HS_INFO)
        {
            if (app_src_legacy_get_hs_info(bd_addr, info))
            {
                if (sink_dev_list[index].headset_info_valid)
                {
                    /* Get headset info by inquiring */
                    if (memcmp(sink_dev_list[index].headset_info, info,
                               sizeof(info)))
                    {
                        uint8_t tmp[4];

                        memcpy(tmp, info, 4);
                        memcpy(info, sink_dev_list[index].headset_info, 4);
                        /* Update hs info of bond info */
                        app_src_legacy_add_hs_info(index, bd_addr, info);
                        APP_PRINT_INFO4("src_handle_link_key_req(): hs %02x %02x -> %02x %02x",
                                        tmp[0], tmp[1], info[0], info[1]);
                    }
                }
                memcpy(sink_dev_list[index].headset_info, info, 4);
                sink_dev_list[index].headset_info_valid = true;
            }
        }
        else
        {
            if (sink_dev_list[index].headset_info_valid)
                app_src_legacy_add_hs_info(index, bd_addr,
                                           sink_dev_list[index].headset_info);
        }

        bt_link_key_cfm(bd_addr, true, key_type, link_key);
    }
    else
    {
        bt_link_key_cfm(bd_addr, false, key_type, link_key);
    }
}



static void src_handle_sdp_attr_info(uint8_t *bd_addr, T_BT_SDP_ATTR_INFO sdp_attr)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index;

    APP_PRINT_INFO1("src_handle_sdp_attr_info: %s", TRACE_BDADDR(bd_addr));

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        APP_PRINT_INFO1("src_handle_sdp_attr_info: doing_profile %08x",
                        sink_dev->profile_mask_doing);

        if ((sink_dev->dev_state == STATE_DEV_RECONNECTING) ||
            (sink_dev->dev_state == STATE_DEV_CONNECTING))
        {
            if (SPP_AUDIO_PROFILE_MASK == sink_dev->profile_mask_doing)
            {
                APP_PRINT_INFO0("src_handle_sdp_attr_info(): spp audio");
                sink_dev->sdp_info.is_found = true;
                sink_dev->sdp_info.server_channel = sdp_attr.server_channel;
            }
            else if (sink_dev->profile_mask_doing == A2DP_PROFILE_MASK)
            {
                APP_PRINT_INFO1("src_handle_sdp_attr_info: a2dp %04x",
                                sdp_attr.protocol_version);
                sink_dev->sdp_info.protocol_version = sdp_attr.protocol_version;
                sink_dev->sdp_info.is_found = true;
            }
            else
            {
                sink_dev->sdp_info.is_found = true;
            }
        }
    }
}

static void src_handle_did_attr_info(T_BT_EVENT_PARAM_DID_ATTR_INFO info)
{
    //T_APP_DEV *sink_dev = NULL;
    uint8_t index;

    if (src_policy_find_dev_index_by_addr(info.bd_addr, &index))
    {

    }
}


static void src_handle_sdp_cmpl(uint8_t *bd_addr, uint16_t cause)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        APP_PRINT_INFO5("src_handle_sdp_cmpl: %s mask %08x, dev_state 0x%04x, found %d, cause 0x%04x",
                        TRACE_BDADDR(sink_dev->remote_info.bd_addr),
                        sink_dev->profile_mask_doing,
                        sink_dev->dev_state,
                        sink_dev->sdp_info.is_found,
                        cause);

        if ((sink_dev->dev_state == STATE_DEV_RECONNECTING) ||
            (sink_dev->dev_state == STATE_DEV_CONNECTING))
        {
            sink_dev->sdp_info.is_busy = false;

            if (cause)
            {
                APP_PRINT_WARN0("src_handle_sdp_cmpl: fail, wait for conn fail");
            }
            else if (sink_dev->sdp_info.is_found)
            {
                /* if the profile is connected by remote initiator, we connect
                 * to next profile.
                 */
                if (profile_is_connected(bd_addr, sink_dev->profile_mask_doing))
                {
                    src_profile_conn_finish(index);
                    src_dev_profile_conn(index);
                    return;
                }

                if (!src_profile_conn(sink_dev->remote_info.bd_addr,
                                      sink_dev->profile_mask_doing,
                                      sink_dev->sdp_info))
                {
                    APP_PRINT_INFO2("src_handle_sdp_cmpl: failed with %s %08x",
                                    TRACE_BDADDR(sink_dev->remote_info.bd_addr),
                                    sink_dev->profile_mask_doing);

                    src_profile_conn_finish(index);
                    src_dev_profile_conn(index);
                }
            }
            else
            {
                src_profile_conn_finish(index);
                src_dev_profile_conn(index);
            }
        }
    }
}

#if 0
static bool src_is_low_latency(T_APP_DEV *sink_dev)
{
    if (!sink_dev->headset_info_valid)
    {
        /* If headset has no low latency info in EIR, we return no low latency.
         * */
        return false;
        /* if (src_general_cfg.sbc_frames_per_pkt)
         * {
         *     return true;
         * }
         * else
         * {
         *     return false;
         * }
         */
    }

    /* Not support low latency */
    if (!(sink_dev->headset_info[0] & 0x02))
    {
        return false;
    }

    /* Frame num in EIR is not zero */
    if (sink_dev->headset_info[1] & 0x0f)
    {
        return true;
    }

    if (src_general_cfg.sbc_frames_per_pkt)
    {
        return true;
    }
    else
    {
        return false;
    }
}
#endif

#ifdef APP_USB_AUDIO
static bool src_is_mic_opened(void)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].mic_open)
        {
            APP_PRINT_INFO1("src_is_mic_opened: %u MIC already open", i);
            return true;
        }
    }

    return false;
}
#endif /* end of APP_USB_AUDIO */

static void src_set_tpoll(uint8_t *bdaddr, uint32_t tpoll)
{
    if (headset_status.headset_linkback && !headset_status.rtp_enable)
    {
        tpoll = HEADSET_IS_LINKBACK_TPOLL;
    }
    else
    {
        tpoll = SRC_LOW_LATENCY_TPOLL;
    }

    APP_PRINT_INFO2("src_set_tpoll(): set tpoll %u for dev %b", tpoll,
                    TRACE_BDADDR(bdaddr));

    /* tpoll = 6 for low latency
     * tpoll = 40 for normal mode
     */

    if (!bt_link_qos_set(bdaddr, BT_QOS_TYPE_GUARANTEED, tpoll))
    {
        APP_PRINT_ERROR2("src_set_tpoll: failed to set tpoll %u for dev %b",
                         tpoll, TRACE_BDADDR(bdaddr));
    }
}

/* static void app_src_balance_stream_credits(uint8_t index)
 * {
 *     uint8_t a2dp_num;
 *     uint8_t i;
 *     uint8_t average_credits;
 *
 *     a2dp_num = app_connected_profile_link_num(A2DP_PROFILE_MASK);
 *     if (a2dp_num <= 1) {
 *             sink_dev_list[index].a2dp_stream_credits = A2DP_STREAM_MAX_CREDITS;
 *             return;
 *     }
 *
 *     average_credits = A2DP_STREAM_MAX_CREDITS / a2dp_num;
 *     if (!average_credits) {
 *         APP_PRINT_ERROR0("app_src_balance_stream_credits: Zero credit");
 *     }
 *
 *     for (i = 0; i < SINK_DEV_NUM_MAX; i++) {
 *         if (profile_is_connected(sink_dev_list[i].remote_info.bd_addr,
 *                                  A2DP_PROFILE_MASK)) {
 *             sink_dev_list[i].a2dp_stream_credits = average_credits;
 *         }
 *     }
 * }
 */

void src_handle_headset_linkback_tpoll(void)
{
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    uint8_t *addr = app_cfg_nv.public_bud_addr;
#else
    uint8_t *addr = sink_dev_list[0].remote_info.bd_addr;
#endif

    if (headset_status.headset_linkback && !headset_status.rtp_enable)
    {
        src_set_tpoll(addr, HEADSET_IS_LINKBACK_TPOLL);
    }
    else
    {
        src_set_tpoll(addr, SRC_LOW_LATENCY_TPOLL);
    }
}

void src_handle_profile_conn_cmpl(uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;
    uint32_t bond_flag = 0;
    T_APP_BR_LINK *p_link;
    uint32_t target_profile_mask = A2DP_PROFILE_MASK | SPP_AUDIO_PROFILE_MASK | AVRCP_PROFILE_MASK;

    APP_PRINT_INFO2("src_handle_profile_conn_cmpl: %s mask %08x",
                    TRACE_BDADDR(bd_addr), profile_mask);

    add_conneted_profile(bd_addr, profile_mask);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (profile_mask == A2DP_PROFILE_MASK)
    {
        if (app_usb_audio_is_us_streaming())
        {
            src_control_remote_mic(true, &sink_dev_list[0]);
        }
    }
#endif

    if (src_policy_find_dev_index_by_addr(bd_addr, &index) ||
        src_policy_find_dev_index_by_bond_info(bd_addr, &index))
    {
        p_link = app_find_br_link(bd_addr);
        if (!p_link)
        {
            APP_PRINT_ERROR1("src_handle_profile_conn_cmpl: index %d, "
                             "no br link", index);
            return;
        }

        sink_dev = &sink_dev_list[index];

        if (bt_uapi_cback)
        {
            struct gaming_ev_profile_connected ev;

            ev.id = index;
            memcpy(ev.ba, bd_addr, 6);
            ev.profile = profile_mask;
            if (profile_mask == SPP_AUDIO_PROFILE_MASK)
            {
                ev.profile_data = RFC_SPP_AUDIO_CHANN_NUM;
            }
            bt_uapi_cback(EVENT_GAMING_PROFILE_CONNECTED, (void *)&ev,
                          sizeof(ev));
        }

        /* FIXME: Indicate that device is connected. This makes good user
         * experience ?  */
        if (profile_mask == A2DP_PROFILE_MASK)
        {
            /*After consulting Alex, prepare path here is because the dual uac
            parameter is fixed before, and it can be unnecessary here.*/
#if 0
            if (app_eir_parse_is_support_lc3_codec(index))
            {
                support_a2dp_codec_lc3 = true;
            }
            prepare_audio_path();
#endif
            src_led_set_connected(index);
            sink_dev->a2dp_opened = 0;
#ifdef A2DP_SRC_TX_FLOW_CONTROL
            sink_dev->a2dp_stream_credits = A2DP_STREAM_MAX_CREDITS;
#endif
            /* app_src_balance_stream_credits(index); */
        }

        if (profile_mask == SPP_AUDIO_PROFILE_MASK)
        {
            app_gaming_sync_dongle_status();

            src_led_set_connected(index);

            if (app_usb_audio_is_us_streaming())
            {
                src_control_remote_mic(true, sink_dev);
            }
#if F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
            app_usb_spk_vol_sync_to_headset();
#endif
        }

        /* There is a scenario that the dongle and earphone were in gaming
         * mode, then the dongle restarted.
         * In this scenario, dongle would send a command to earphone to make it
         * leave the gaming mode. But the command sending would fail if a2dp is
         * opened before spp connection.  So we resend the command to the
         * earphone. It is not harmful if the earphone receives a duplicate
         * command.
         * */
        if (profile_mask == SPP_AUDIO_PROFILE_MASK)
        {
            if (sink_dev->a2dp_opened)
            {
                src_spp_req_leave_ll(bd_addr, 0);
            }
        }

        APP_PRINT_INFO2("src_handle_profile_conn_cmpl: index %d, dev_state %02x",
                        index, sink_dev->dev_state);

        bond_flag = get_bond_flag_from_profile_mask(profile_mask);
        if (bond_flag != 0)
        {
            app_src_legacy_add_bond_flag(bd_addr, bond_flag);
        }

        switch (sink_dev->dev_state)
        {
        case STATE_DEV_IDLE:
            {
                /* wait for acl exist timeout */
#if 0
                if (profile_mask == A2DP_PROFILE_MASK)
                {
                    sink_dev->connecting = false;
                    sink_dev->dev_state = STATE_DEV_CONNECTED;
                    memcpy(sink_dev->remote_info.bd_addr, bd_addr, 6);
                    src_led_set_connected(index);
                    app_start_timer(&timer_idx_src_a2dp_cfg[index], "a2dp_cfg_reverse",
                                    src_policy_timer_id, SRC_TIMER_ID_A2DP_CFG + index, 0, false,
                                    3000);
                    /* To see if there is another sink to connect */
                    src_handle_action_result(index, true);
                    //app_start_timer(&timer_idx_src_a2dp[index], "a2dp_reverse",
                    //                src_policy_timer_id, SRC_TIMER_ID_A2DP + index, 0, false,
                    //                5000);
                }
#endif
                /* In the scenario that we were in idle state and headset reconn dongle
                 * successfully, we will wait until the acl exist timer timeout to switch
                 * to dev connected. if the downstream start before this, send start req
                 * will be blocked in gaming_bt_try_start_stream().
                 * */
                if ((p_link->connected_profile & target_profile_mask) == target_profile_mask)
                {
                    app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
                    src_handle_action_result(index, true);
                }
            }
            break;

        case STATE_DEV_RECONNECTING:
        case STATE_DEV_CONNECTING:
            {
                /* In the scenario that we were in connecting state, but we
                 * didn't find the headset or we were in the state before conn
                 * timer expired. And remote headset was connecting to us at
                 * this time.
                 * In this scenario, the device profile_mask_plan is zero.
                 * This causes src_dev_profile_conn() to detach the acl conn.
                 * */
                sink_dev->profile_mask_plan = target_profile_mask;
                sink_dev->profile_mask_remain = p_link->connected_profile ^ target_profile_mask;

                /* if acl connected from local, profile from remote, in case
                 * this profile is the current one, check sdp's is_busy flag to
                 * avoid starting a new sdp while doing the last one.
                 *
                 * sdp cmpl handler would find that the profile_mask_doing is
                 * connected, and connect to next profile.
                 */
                if (sink_dev->sdp_info.is_busy)
                {
                    APP_PRINT_INFO0("src_handle_profile_conn_cmpl, conn req from local, ignore");
                    return;
                }

                /* Sometimes, dongle (re)connecting failed with acl conn failed
                 * and remote reconnected. In this scenario, profile_mask_doing
                 * is zero
                 */
                if (!sink_dev->profile_mask_doing ||
                    sink_dev->profile_mask_doing == profile_mask)
                {
                    /* Try to connect next profile */
                    if (profile_mask != A2DP_PROFILE_MASK)
                    {
                        src_profile_conn_finish(index);
                        src_dev_profile_conn(index);
                        return;
                    }

                    src_profile_conn_finish(index);
                    src_dev_profile_conn(index);

                    /* Some devices just reconnect, but not send any avdtp
                     * signal cmd. So we start a timer to send avdtp sig cmd.
                     */
                    app_start_timer(&timer_idx_src_a2dp_cfg[index], "a2dp_cfg_reverse",
                                    src_policy_timer_id, SRC_TIMER_ID_A2DP_CFG + index, 0, false,
                                    3000);
                    //app_start_timer(&timer_idx_src_a2dp[index], "a2dp_reverse",
                    //                src_policy_timer_id, SRC_TIMER_ID_A2DP + index, 0, false,
                    //                5000);
                }
            }
            break;

        case STATE_DEV_CLEANUP:
            {
                APP_PRINT_INFO0("src_handle_profile_conn_cmpl: disconnect in cleaup");
                src_profile_disconn(bd_addr, profile_mask);
            }
            break;

        default:
            break;
        }
    }
}

uint8_t src_sbc_frame_size(T_AUDIO_SBC_CHANNEL_MODE chann_mode,
                           uint8_t blocks, uint8_t subbands,
                           uint8_t bitpool)
{
    uint16_t frame_size = 0;
    uint8_t channels;
    uint8_t joint = 0;
    uint16_t tmp;

    switch (chann_mode)
    {
    case AUDIO_SBC_CHANNEL_MODE_MONO:
        channels = 1;
        break;
    case AUDIO_SBC_CHANNEL_MODE_DUAL:
        channels = 2;
        break;
    case AUDIO_SBC_CHANNEL_MODE_STEREO:
        channels = 2;
        break;
    case AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO:
        channels = 2;
        joint = 1;
        break;
    default:
        APP_PRINT_ERROR1("src_sbc_frame_size(): unknown channel mode %u",
                         chann_mode);
        return 0;
    }

    if (chann_mode == AUDIO_SBC_CHANNEL_MODE_MONO ||
        chann_mode == AUDIO_SBC_CHANNEL_MODE_DUAL)
    {
        tmp = blocks * channels * bitpool;

        frame_size = 4 + subbands * channels / 2 + tmp / 8;

        if (tmp % 8 != 0)
        {
            frame_size++;
        }
    }
    else if (chann_mode == AUDIO_SBC_CHANNEL_MODE_STEREO ||
             chann_mode == AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO)
    {
        tmp = joint * subbands + blocks * bitpool;

        frame_size = 4 + subbands * channels / 2 + tmp / 8;

        if (tmp % 8 != 0)
        {
            frame_size++;
        }
    }

    APP_PRINT_INFO1("src_sbc_frame_size(): frame size %u", frame_size);

    return frame_size;

}

static uint16_t get_a2dp_sbc_sample_rate(uint8_t sf_mask)
{
    uint16_t sample_rate = 0;

    switch (sf_mask)
    {
    case 0x80:
        sample_rate = 16000;
        break;

    case 0x40:
        sample_rate = 32000;
        break;

    case 0x20:
        sample_rate = 44100;
        break;

    case 0x10:
        sample_rate = 48000;
        break;

    default:
        APP_PRINT_WARN1("get_sbc_sample_rate: unknown mask %02x", sf_mask);
        break;
    }

    return sample_rate;
}

static T_AUDIO_SBC_CHANNEL_MODE get_a2dp_sbc_channel_mode(uint8_t cm_mask)
{
    T_AUDIO_SBC_CHANNEL_MODE cm = AUDIO_SBC_CHANNEL_MODE_MONO;

    switch (cm_mask)
    {
    case 0x08:
        cm = AUDIO_SBC_CHANNEL_MODE_MONO;
        break;

    case 0x04:
        cm = AUDIO_SBC_CHANNEL_MODE_DUAL;
        break;

    case 0x02:
        cm = AUDIO_SBC_CHANNEL_MODE_STEREO;
        break;

    case 0x01:
        cm = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
        break;

    default:
        APP_PRINT_WARN1("get_a2dp_sbc_channel_mode: unknown mask %02x", cm_mask);
        break;
    }

    return cm;
}

static uint8_t get_a2dp_sbc_block_length(uint8_t bl_mask)
{
    uint8_t bl = 0;

    switch (bl_mask)
    {
    case 0x80:
        bl = 4;
        break;

    case 0x40:
        bl = 8;
        break;

    case 0x20:
        bl = 12;
        break;

    case 0x10:
        bl = 16;
        break;

    default:
        APP_PRINT_WARN1("get_a2dp_sbc_block_length: unknown mask %02x", bl_mask);
        break;
    }

    return bl;
}

static uint8_t get_a2dp_sbc_subbands(uint8_t sbs_mask)
{
    uint8_t sbs = 0;

    switch (sbs_mask)
    {

    case 0x08:
        sbs = 4;
        break;

    case 0x04:
        sbs = 8;
        break;

    default:
        APP_PRINT_WARN1("get_a2dp_sbc_subbands: unknown mask %02x", sbs_mask);
        break;
    }

    return sbs;
}

static uint8_t get_a2dp_sbc_allocation_method(uint8_t am_mask)
{
    uint8_t am = 0;

    switch (am_mask)
    {

    case 0x02:
        am = 1;
        break;

    case 0x01:
        am = 0;
        break;

    default:
        APP_PRINT_WARN1("get_a2dp_sbc_allocation_method: unknown mask %02x", am_mask);
        break;
    }

    return am;
}

#ifndef CONFIG_FIX_BITPOOL
static uint8_t src_sbc_bitpool(T_AUDIO_FORMAT_INFO *info,
                               uint8_t sbc_frame_num,
                               uint16_t *acl_plen)
{
    uint8_t bitpool = 0;
    uint16_t media_offset = SRC_2M_ACL_AVDTP_STREAM_OFFSET;
    uint16_t acl_pkt_size[] =
    {
        SRC_2M_ACL_2DH3_PAYLOAD_SIZE,
        SRC_2M_ACL_2DH5_PAYLOAD_SIZE
    };
    uint32_t i;
    uint16_t frame_size, frame_size2;
    uint16_t tmp1;
    uint8_t channels;
    uint8_t blocks = info->attr.sbc.block_length;
    uint8_t subbands = info->attr.sbc.subband_num;

    APP_PRINT_INFO1("src_sbc_bitpool(): frame num %u", sbc_frame_num);

    if (!sbc_frame_num)
    {
        if (acl_plen)
        {
            *acl_plen = SRC_2M_ACL_2DH5_PAYLOAD_SIZE;
        }
        return SRC_BITPOOL_MAX_START;
    }

    if (info->attr.sbc.chann_mode == AUDIO_SBC_CHANNEL_MODE_MONO)
    {
        channels = 1;
    }
    else
    {
        channels = 2;
    }

    tmp1 = 4 * subbands * channels;
    tmp1 >>= 3;

    for (i = 0; i < sizeof(acl_pkt_size) / sizeof(acl_pkt_size[0]); i++)
    {
        frame_size = (acl_pkt_size[i] - media_offset) / sbc_frame_num;
        if (info->attr.sbc.chann_mode == AUDIO_SBC_CHANNEL_MODE_MONO ||
            info->attr.sbc.chann_mode == AUDIO_SBC_CHANNEL_MODE_DUAL)
        {
            bitpool = (frame_size - 4 - tmp1) * 8 / (blocks * channels);
        }
        else if (info->attr.sbc.chann_mode == AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO)
        {
            bitpool = ((frame_size - 4 - tmp1) * 8 - subbands) / blocks;
        }
        else
        {
            bitpool = ((frame_size - 4 - tmp1) * 8) / blocks;
        }
        frame_size2 = src_sbc_frame_size(info->attr.sbc.chann_mode, blocks,
                                         subbands, bitpool);
        /* TODO: Will this happen ? */
        while (frame_size2 > frame_size)
        {
            bitpool--;
            frame_size2 = src_sbc_frame_size(info->attr.sbc.chann_mode, blocks,
                                             subbands, bitpool);
        }
        APP_PRINT_INFO2("src_sbc_bitpool(): calculated max bitpool %u, acl %u",
                        bitpool, acl_pkt_size[i]);
        if (bitpool >= SRC_BITPOOL_MAX_START)
        {
            if (acl_plen)
            {
                *acl_plen = acl_pkt_size[i];
            }
            if (bitpool <= SRC_BITPOOL_MAX_END)
            {
                return bitpool;
            }
            else
            {
                return SRC_BITPOOL_MAX_END;
            }
        }
    }

    if (acl_plen)
    {
        *acl_plen = SRC_2M_ACL_2DH5_PAYLOAD_SIZE;
    }
    return SRC_BITPOOL_MAX_START;
}
#endif

static void get_audio_format_info_from_a2dp_cfg(uint8_t index,
                                                T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL cfg)
{
    T_AUDIO_FORMAT_INFO *info = &sink_dev_list[index].codec_info;
    uint8_t i;
    uint8_t cur_bitpool = 0;
    bool re_config_flag = false;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].codec_configured && app_cfg_const.dongle_link_num_max)
        {
            cur_bitpool = sink_dev_list[i].codec_info.attr.sbc.bitpool;
            APP_PRINT_INFO2("get_audio_format_info_from_a2dp_cfg(): Other link %u exists, bitpool %u",
                            i, cur_bitpool);
            break;
        }

        if (sink_dev_list[i].codec_configured && (!app_cfg_const.dongle_link_num_max))
        {
            //re-config
            APP_PRINT_INFO2("get_audio_format_info_from_a2dp_cfg(): cfg_codec %d, info_codec %d",
                            cfg.codec_type, info->type);
            if (((cfg.codec_type == BT_A2DP_CODEC_TYPE_SBC) && (info->type == AUDIO_FORMAT_TYPE_LC3)) ||
                ((cfg.codec_type == BT_A2DP_CODEC_TYPE_LC3) && (info->type == AUDIO_FORMAT_TYPE_SBC)))
            {
                re_config_flag = true;
            }
        }
    }

    switch (cfg.codec_type)
    {
    case BT_A2DP_CODEC_TYPE_SBC:
        {
            /* We assume that the device is in normal mode when it is
             * connected.
             */
#ifndef CONFIG_FIX_BITPOOL
            uint8_t frame_num = SRC_MAX_SBC_FRAME_NUM;
#endif
            uint8_t bitpool;

            /* This audio format info will be passed to dsp */
            info->type = AUDIO_FORMAT_TYPE_SBC;

            info->attr.sbc.sample_rate =
                get_a2dp_sbc_sample_rate(cfg.codec_info.sbc.sampling_frequency);

            info->attr.sbc.chann_mode =
                get_a2dp_sbc_channel_mode(cfg.codec_info.sbc.channel_mode);

            info->attr.sbc.block_length =
                get_a2dp_sbc_block_length(cfg.codec_info.sbc.block_length);
            APP_PRINT_INFO2("get_a2dp_sbc_block_length old %x new %x", cfg.codec_info.sbc.block_length,
                            info->attr.sbc.block_length);

            info->attr.sbc.subband_num =
                get_a2dp_sbc_subbands(cfg.codec_info.sbc.subbands);

            info->attr.sbc.allocation_method =
                get_a2dp_sbc_allocation_method(cfg.codec_info.sbc.allocation_method);

            if (cur_bitpool)
            {
                info->attr.sbc.bitpool = cur_bitpool;
                break;
            }

#ifdef CONFIG_FIX_BITPOOL
            bitpool = FIXED_BITPOOL;
#else
            bitpool = src_sbc_bitpool(info, frame_num, NULL);
#endif
            APP_PRINT_INFO1("get_audio_format_info_from_a2dp_cfg(): calculated bitpool %u",
                            bitpool);
#ifndef CONFIG_FIX_BITPOOL
            if (bitpool < cfg.codec_info.sbc.min_bitpool)
            {
                bitpool = cfg.codec_info.sbc.min_bitpool;
            }
            if (bitpool > cfg.codec_info.sbc.max_bitpool)
            {
                bitpool = cfg.codec_info.sbc.max_bitpool;
            }
#endif
            info->attr.sbc.bitpool = bitpool;
        }
        break;

    case BT_A2DP_CODEC_TYPE_LC3:
        {
            info->type = AUDIO_FORMAT_TYPE_LC3;
            switch (cfg.codec_info.lc3.sampling_frequency)
            {
            case BT_A2DP_LC3_SAMPLING_FREQUENCY_8KHZ:
                info->attr.lc3.sample_rate = 8000;
                break;
            case BT_A2DP_LC3_SAMPLING_FREQUENCY_16KHZ:
                info->attr.lc3.sample_rate = 16000;
                break;
            case BT_A2DP_LC3_SAMPLING_FREQUENCY_24KHZ:
                info->attr.lc3.sample_rate = 24000;
                break;
            case BT_A2DP_LC3_SAMPLING_FREQUENCY_32KHZ:
                info->attr.lc3.sample_rate = 32000;
                break;
            case BT_A2DP_LC3_SAMPLING_FREQUENCY_44_1KHZ:
                info->attr.lc3.sample_rate = 44100;
                break;
            case BT_A2DP_LC3_SAMPLING_FREQUENCY_48KHZ:
                info->attr.lc3.sample_rate = 48000;
                break;
            default:
                break;
            }

            switch (cfg.codec_info.lc3.channel_number)
            {
            case BT_A2DP_LC3_CHANNEL_NUM_1:
                info->attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO;
                break;
            case BT_A2DP_LC3_CHANNEL_NUM_2:
                info->attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_FL |
                                                AUDIO_CHANNEL_LOCATION_FR;
                break;
            default:
                break;
            }

            switch (cfg.codec_info.lc3.frame_duration)
            {
            case BT_A2DP_LC3_FRAME_DURATION_7_5MS:
                info->attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_7_5_MS;
                break;
            case BT_A2DP_LC3_FRAME_DURATION_10MS:
                info->attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
                break;
            default:
                break;
            }
            info->attr.lc3.frame_length = cfg.codec_info.lc3.frame_length;
        }
        break;

    default:
        break;
    }

    if (cfg.codec_type == BT_A2DP_CODEC_TYPE_SBC)
    {
        support_a2dp_codec_lc3 = false;
        APP_PRINT_INFO6("get_audio_format_info_from_a2dp_cfg: Sbc sr %u, bl %u, cm %u, "
                        "a %u, s %u, bitpool %u",
                        info->attr.sbc.sample_rate,
                        info->attr.sbc.block_length,
                        info->attr.sbc.chann_mode,
                        info->attr.sbc.allocation_method,
                        info->attr.sbc.subband_num,
                        info->attr.sbc.bitpool);
    }
    else if (cfg.codec_type == BT_A2DP_CODEC_TYPE_LC3)
    {
        support_a2dp_codec_lc3 = true;
        APP_PRINT_INFO4("get_audio_format_info_from_a2dp_cfg(): Lc3 sample %d, location %u, duration %u,frame-len %u",
                        info->attr.lc3.sample_rate,
                        info->attr.lc3.chann_location,
                        info->attr.lc3.frame_duration,
                        info->attr.lc3.frame_length);
    }
    if (re_config_flag)
    {
        src_update_a2dp_stream_info(index);
    }
}

static void src_handle_a2dp_config_cmpl(T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL cfg)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;

    APP_PRINT_INFO3("src_handle_a2dp_config_cmpl: %s cfg.codec_type %d frame_length %d",
                    TRACE_BDADDR(cfg.bd_addr),
                    cfg.codec_type, cfg.codec_info.lc3.frame_length);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT == 0
    if (src_policy_find_dev_index_by_addr(cfg.bd_addr, &index) ||
        src_policy_find_dev_index_by_bond_info(cfg.bd_addr, &index))
#endif
    {
        sink_dev = &sink_dev_list[index];
        get_audio_format_info_from_a2dp_cfg(index, cfg);
        sink_dev->codec_configured = true;

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
        app_usb_ds_legacy_pipe_create(cfg);
#endif
    }
}

#ifdef A2DP_INTERVAL_PACKET
static uint16_t src_cal_time_packet_num(uint32_t sample_rate,
                                        uint8_t interval_ms, uint8_t blocks, uint8_t subbands)
{
    uint64_t temp1 = 0;
    uint32_t temp2 = 0;
    uint16_t pkt_num = 0;

    temp1 = sample_rate * interval_ms;
    temp2 = blocks * subbands * 1000;
    pkt_num = temp1 / temp2;

    if (temp1 % temp2 != 0)
    {
        pkt_num++;
    }

    APP_PRINT_INFO5("src_cal_time_packet_num: %d, %d, %d, %d, %d",
                    sample_rate, interval_ms, blocks, subbands, pkt_num);

    return pkt_num;
}
#endif

#if 0
static void src_adjust_max_sbc_frame_num(uint16_t sbc_frame_size)
{
    uint8_t i;
    uint8_t num = sink_dev_list[0].stream_info.sbc_max_frame_num;
    uint8_t link_num = 0;
    uint8_t n;
    T_APP_DEV *sink_dev;

    if (sink_dev_list[0].codec_configured)
    {
        link_num++;
    }

    /* Select the largest num and its buf size */
    for (i = 1; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];
        if (sink_dev->stream_info.buffer &&
            sink_dev->stream_info.sbc_max_frame_num > num)
        {
            num = sink_dev->stream_info.sbc_max_frame_num;
        }

        if (sink_dev->codec_configured)
        {
            link_num++;
        }
    }

    /* dual links require low latency */
    if (num < 3 && link_num > 1)
    {
        num = 2 * SRC_STEREO_SBC_FRAME_NUM;
        APP_PRINT_INFO2("src_adjust_max_sbc_frame_num: extend num %u, link %u",
                        num, link_num);
    }

    /* Update all num and buf size to largest ones */
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];
        if (!sink_dev->stream_info.buffer)
        {
            continue;
        }
        n = 0;
        if (sink_dev->stream_info.mtu > 0)
            n = (sink_dev->stream_info.mtu - SRC_2M_ACL_AVDTP_STREAM_OFFSET) /
                sbc_frame_size;
        if (n > num)
        {
            n = num;
        }

        sink_dev->stream_info.sbc_max_frame_num = n;
        sink_dev->stream_info.buf_size = n * sbc_frame_size;
    }
}
#endif

static uint8_t src_get_acl_link_num(void)
{
    uint8_t link_num = 0;
    uint8_t i = 0;
    T_APP_DEV *sink_dev;
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];
        if (sink_dev->bt_state != STATE_BT_ACL_CONNECTED)
        {
            continue;
        }
        link_num++;
    }
    return link_num;
}

uint8_t src_get_spp_connected_link_num(void)
{
    uint8_t ret = 0;
    uint8_t i = 0;
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (profile_is_connected(sink_dev_list[i].remote_info.bd_addr,
                                 SPP_AUDIO_PROFILE_MASK))
        {
            ret++;
        }
    }
    return ret;
}

uint8_t src_announce_enter_or_leave_ll(bool ll)
{
    uint8_t i;
    uint8_t sbc_frm_num = src_get_proper_frame_num2();
    uint8_t ret = 0;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        /* if (i == index)
         *     continue;
         */

        if (profile_is_connected(sink_dev_list[i].remote_info.bd_addr,
                                 SPP_AUDIO_PROFILE_MASK))
        {
            uint8_t resp[SRC_SPP_HDR_SIZE + 5];
            T_GAMING_SYNC_HDR *hdr = (T_GAMING_SYNC_HDR *)&resp[0];
            uint8_t *bdaddr = sink_dev_list[i].remote_info.bd_addr;

            hdr->sync = DONGLE_FORMAT_START_BIT;
            hdr->type = DONGLE_TYPE_CMD;
            hdr->pl   = 0x05;
            hdr->cmd  = DONGLE_CMD_SET_GAMING_MOE;
            if (ll && src_get_acl_link_num() == src_get_spp_connected_link_num() &&
                src_low_latency_allowed())
            {
                resp[SRC_SPP_HDR_SIZE] = 0x01;
            }
            else
            {
                resp[SRC_SPP_HDR_SIZE] = 0x00;
            }
            resp[SRC_SPP_HDR_SIZE + 1] = sbc_frm_num;
            resp[SRC_SPP_HDR_SIZE + 2] = 0x00;
            resp[SRC_SPP_HDR_SIZE + 3] = 0x00;
            resp[SRC_SPP_HDR_SIZE + 4] = DONGLE_FORMAT_STOP_BIT;

            APP_PRINT_INFO1("src_announce_enter_or_leave_ll(): %b ++",
                            TRACE_BDADDR(bdaddr));

            if (app_gaming_ctrl_send_fix_channel_data(bdaddr, FIX_CHANNEL_CID, resp, sizeof(resp),
                                                      false) == false)
            {
                APP_PRINT_INFO3("src_announce_enter_or_leave_ll: %u failed to "
                                "send %u, %b", i, sizeof(resp),
                                TRACE_BINARY(sizeof(resp), resp));
            }
            else
            {
                ret++;
                if (ll)
                {
                    if (app_eir_parse_is_support_multilink(i))
                    {
                        src_set_tpoll(sink_dev_list[i].remote_info.bd_addr,
                                      SRC_Multilink_LOW_LATENCY_TPOLL);
                    }
                    else
                    {
                        src_set_tpoll(sink_dev_list[i].remote_info.bd_addr,
                                      SRC_LOW_LATENCY_TPOLL);
                    }
                }
                else
                {
#ifdef APP_USB_AUDIO
                    if (!src_is_mic_opened())
                    {
                        src_set_tpoll(sink_dev_list[i].remote_info.bd_addr,
                                      SRC_NORMAL_TPOLL);
                    }
#endif
                }

            }
        }
    }
    return ret;
}

static void src_init_a2dp_stream_info(uint8_t index, uint16_t mtu)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_SRC_STREAM_INFO *info = &sink_dev->stream_info;
    T_AUDIO_FORMAT_INFO *codec = &sink_dev->codec_info;
    T_APP_BR_LINK *p_link = app_find_br_link(sink_dev->remote_info.bd_addr);

#ifdef A2DP_INTERVAL_PACKET
    uint16_t interval_pkt_num = 0;
#endif

    if (p_link != NULL)
    {
        sink_dev->stream_info.codec_type = p_link->a2dp_codec_type;

        if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_SBC)
        {
            uint16_t max_buf_size;
            uint8_t max_frame_num;
            /* We selected the proper bitpool in src_handle_a2dp_config_cmpl()
             */
            APP_PRINT_INFO4("src_init_a2dp_stream_info(): chann mode %u, bl %u, subbands %u, bitpool %u",
                            codec->attr.sbc.chann_mode,
                            codec->attr.sbc.block_length,
                            codec->attr.sbc.subband_num,
                            codec->attr.sbc.bitpool);
            if (sink_dev->headset_info_valid)
            {
                APP_PRINT_INFO2("src_init_a2dp_stream_info(): headset %02x-%02x",
                                sink_dev->headset_info[0],
                                sink_dev->headset_info[1]);
            }
            info->sbc_frame_size = src_sbc_frame_size(codec->attr.sbc.chann_mode,
                                                      codec->attr.sbc.block_length,
                                                      codec->attr.sbc.subband_num,
                                                      codec->attr.sbc.bitpool);

            if (mtu > SRC_2M_ACL_2DH5_PAYLOAD_SIZE)
            {
                mtu = SRC_2M_ACL_2DH5_PAYLOAD_SIZE;
            }
            info->mtu = mtu;

            if (!info->sbc_frame_size)
            {
                APP_PRINT_ERROR0("src_init_a2dp_stream_info(): sbc frame size is zero");
                return;
            }

#ifdef A2DP_INTERVAL_PACKET
            interval_pkt_num = src_cal_time_packet_num(sink_dev->codec_info.attr.sbc.sample_rate,
                                                       15,
                                                       sink_dev->codec_info.attr.sbc.block_length,
                                                       sink_dev->codec_info.attr.sbc.subband_num);

            if (mtu - SRC_2M_ACL_AVDTP_STREAM_OFFSET >= interval_pkt_num * info->sbc_frame_size)
            {
                info->sbc_max_frame_num = interval_pkt_num;
            }
            else
            {
                info->sbc_max_frame_num = (mtu - SRC_2M_ACL_AVDTP_STREAM_OFFSET) / info->sbc_frame_size;
            }
#else
            max_frame_num = (mtu - SRC_2M_ACL_AVDTP_STREAM_OFFSET) / info->sbc_frame_size;

            /* Choose the right sbc frame count in one avdtp media packet */
            /* sbc_num = src_get_proper_frame_num2();
             * APP_PRINT_INFO2("src_init_a2dp_stream_info: frame num %u %u",
             *                 max_frame_num, sbc_num);
             * if (!sbc_num)
             * {
             *     sbc_num = max_frame_num;
             * }
             * if (sbc_num > max_frame_num)
             * {
             *     sbc_num = max_frame_num;
             * }
             * info->sbc_max_frame_num = sbc_num;
             */
            info->sbc_max_frame_num = max_frame_num;

#endif
            info->sbc_offset = 0;
            info->buf_size = info->sbc_max_frame_num * info->sbc_frame_size;

            /* We allocate memory for maximum sbc frames. We may have two conn,
             * and the second conn is in normal mode. In order to use less
             * bandwidth for the first conn, we extend the sbc buf size to
             * reach this goal.
             */
            max_buf_size = SRC_2M_ACL_2DH5_PAYLOAD_SIZE -
                           SRC_2M_ACL_AVDTP_STREAM_OFFSET;
            info->buffer = os_mem_zalloc(RAM_TYPE_ITCM1, max_buf_size);
            APP_PRINT_INFO4("src_init_a2dp_stream_info: mtu %u, max frame num %d, buf_size %u, max buf size %u",
                            mtu, info->sbc_max_frame_num, info->buf_size,
                            max_buf_size);

            if (info->buffer == NULL)
            {
                APP_PRINT_INFO0("src_calc_a2dp_max_frame: can not alloc buffer !!!");
            }

            uint8_t spp_num = src_get_spp_connected_link_num();

            /* only all link leave low latency, dongle leave latency */
            if (src_announce_enter_or_leave_ll(false) == spp_num)
            {
                APP_PRINT_INFO1("src_init_a2dp_stream_info(): %b ++ leave low latency",
                                TRACE_BDADDR(sink_dev->remote_info.bd_addr));
                src_leave_low_latency(sink_dev->remote_info.bd_addr);
            }

            if (spp_num == 0)
            {
                src_set_tpoll(sink_dev->remote_info.bd_addr, SRC_DEFAULT_TPOLL);
            }

        }
        else if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_LC3)
        {
            uint16_t max_buf_size;
            uint8_t max_frame_num;
            APP_PRINT_INFO4("src_init_a2dp_stream_info(): chann mode %u, fr-du %u, fr-le %u, sample %u",
                            codec->attr.lc3.chann_location,
                            codec->attr.lc3.frame_duration,
                            codec->attr.lc3.frame_length,
                            codec->attr.lc3.sample_rate);
            info->sbc_frame_size = codec->attr.lc3.frame_length;
            if (mtu > SRC_2M_ACL_2DH5_PAYLOAD_SIZE)
            {
                mtu = SRC_2M_ACL_2DH5_PAYLOAD_SIZE;
            }
            info->mtu = mtu;

            max_frame_num = 2;
            info->sbc_max_frame_num = max_frame_num;
            info->sbc_offset = 0;
            info->buf_size = info->sbc_max_frame_num * info->sbc_frame_size;
            max_buf_size = SRC_2M_ACL_2DH5_PAYLOAD_SIZE -
                           SRC_2M_ACL_AVDTP_STREAM_OFFSET;
            info->buffer = os_mem_zalloc(RAM_TYPE_ITCM1, max_buf_size);
            APP_PRINT_INFO4("src_init_a2dp_stream_info: mtu %u, max frame num %d, buf_size %u, max buf size %u",
                            mtu, info->sbc_max_frame_num, info->buf_size,
                            max_buf_size);

            if (info->buffer == NULL)
            {
                APP_PRINT_INFO0("src_calc_a2dp_max_frame: can not alloc buffer !!!");
            }
        }
        else
        {

        }
    }

}

static void src_update_a2dp_stream_info(uint8_t index)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_SRC_STREAM_INFO *info = &sink_dev->stream_info;
    T_AUDIO_FORMAT_INFO *codec = &sink_dev->codec_info;
    T_APP_BR_LINK *p_link = app_find_br_link(sink_dev->remote_info.bd_addr);

    if (p_link != NULL)
    {
        sink_dev->stream_info.codec_type = p_link->a2dp_codec_type;

        if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_SBC)
        {
            info->sbc_frame_size = src_sbc_frame_size(codec->attr.sbc.chann_mode,
                                                      codec->attr.sbc.block_length,
                                                      codec->attr.sbc.subband_num,
                                                      codec->attr.sbc.bitpool);
            if (!info->sbc_frame_size)
            {
                APP_PRINT_ERROR0("src_update_a2dp_stream_info(): sbc frame size is zero");
                return;
            }

            info->sbc_max_frame_num = (672 - SRC_2M_ACL_AVDTP_STREAM_OFFSET) / info->sbc_frame_size;

            info->sbc_offset = 0;
            info->buf_size = info->sbc_max_frame_num * info->sbc_frame_size;
            /*Resume will init a2dp stream info, the frame num will become the max num of 7.
            So, if it is in low latency mode now, need change the frame num of 1 packet.*/
            if (low_latency_mode)
            {
                src_spp_req_enter_ll(sink_dev->remote_info.bd_addr, 0);
            }
        }
        else if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_LC3)
        {
            info->sbc_frame_size = codec->attr.lc3.frame_length;

            info->sbc_max_frame_num = 2;
            info->sbc_offset = 0;
            info->buf_size = info->sbc_max_frame_num * info->sbc_frame_size;
        }
        else
        {

        }
        APP_PRINT_INFO3("src_update_a2dp_stream_info: codec %d, max frame num %d, buf_size %u",
                        p_link->a2dp_codec_type, info->sbc_max_frame_num, info->buf_size);
    }

}

static inline void src_drain_sbc_frames(uint8_t i)
{
    T_APP_DEV *sink_dev;
    T_APP_SRC_STREAM_INFO *stream_info;

    sink_dev = &sink_dev_list[i];
    stream_info = &sink_dev->stream_info;

    if (!stream_info->is_streaming || !stream_info->buffer)
    {
        return;
    }

    if (stream_info->sbc_offset &&
        stream_info->sbc_offset < stream_info->buf_size)
    {
#ifndef CONFIG_FLUSH_SBCENC_DATA
        uint8_t n;
        bool rc;

        n = stream_info->sbc_offset / stream_info->sbc_frame_size;
        APP_PRINT_INFO3("src_drain_sbc_frames(): %u Drain, %u %u", i,
                        stream_info->sbc_offset,
                        stream_info->buf_size);
#ifdef A2DP_SRC_TX_FLOW_CONTROL
        if (sink_dev->a2dp_stream_credits)
        {
            rc = bt_a2dp_stream_data_send(sink_dev->remote_info.bd_addr,
                                          sink_dev->a2dp_ll_seq,
                                          sink_dev->a2dp_timestamp,
                                          n,
                                          stream_info->buffer,
                                          stream_info->sbc_offset);
            if (rc)
            {
                sink_dev->a2dp_stream_credits--;
            }
            else
            {
                APP_PRINT_ERROR1("src_drain_sbc_frames: %u Transmit a2dp failed", i);
            }
        }
        else
        {
            APP_PRINT_WARN1("src_drain_sbc_frames: has no credits for a2dp", i);
        }
        sink_dev->a2dp_ll_seq++;
        sink_dev->a2dp_timestamp += n * sink_dev->codec_info.attr.sbc.block_length *
                                    sink_dev->codec_info.attr.sbc.subband_num;
#else
        rc = bt_a2dp_stream_data_send(sink_dev->remote_info.bd_addr,
                                      sink_dev->a2dp_ll_seq,
                                      sink_dev->a2dp_timestamp,
                                      n,
                                      stream_info->buffer,
                                      stream_info->sbc_offset);
        if (!rc)
        {
            APP_PRINT_ERROR1("src_drain_sbc_frames: %u Transmit failed", i);
        }
        else
        {
        }
        sink_dev->a2dp_ll_seq++;
        sink_dev->a2dp_timestamp += n * sink_dev->codec_info.attr.sbc.block_length *
                                    sink_dev->codec_info.attr.sbc.subband_num;
#endif
#else
        APP_PRINT_INFO0("src_drain_sbc_frames(): Flush");
#endif
    }
#ifdef A2DP_STREAM_LATENCY_MONITOR
    sink_dev->a2dp_seq_synchronized = 0;
#endif
    stream_info->sbc_offset = 0;
}

static void src_start_dsp(uint8_t bitpool)
{
    if (app_audio_type != AUDIO_TYPE_NONE)
    {
        APP_PRINT_INFO0("src_start_dsp(): Audio has been opened");
        return;
    }

#ifdef APP_AUX_AUDIO
    if (src_is_aux_in())
    {
        app_audio_type = AUDIO_TYPE_AUX_IN;
        app_src_sbc_voice_start_capture(*codec, sbc_audio_read_cb);
    }
#endif

}

static void src_stop_dsp(void)
{
    APP_PRINT_INFO0("src_stop_dsp(): ++");

    if (app_audio_type == AUDIO_TYPE_NONE)
    {
        APP_PRINT_INFO0("src_stop_dsp(): Audio has been closed");
        return;
    }

    if (app_audio_type == AUDIO_TYPE_AUX_IN)
    {
        app_src_sbc_voice_stop_capture();
    }
    else if (app_audio_type == AUDIO_TYPE_USB_DS)
    {
    }

    app_audio_type = AUDIO_TYPE_NONE;
}

void src_adjust_stream_codec(uint8_t bitpool, uint8_t sbc_frame_num,
                             uint8_t sbc_frame_size)
{
    T_APP_DEV *sink_dev;
    T_AUDIO_FORMAT_INFO *codec;
    T_APP_SRC_STREAM_INFO *sinfo;
    uint8_t i;
    bool bitpool_changed = false;
    uint8_t max_frame_num = 0;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];
        sinfo = &sink_dev->stream_info;
        codec = &sink_dev->codec_info;

        /* stream is not open */
        if (!sinfo->buffer)
        {
            continue;
        }

        if (codec->attr.sbc.bitpool != bitpool)
        {
            if (!bitpool_changed)
            {
                bitpool_changed = true;
                src_stop_dsp();
            }
        }

        src_drain_sbc_frames(i);

        if (sinfo->mtu > 0)
        {
            max_frame_num = (sinfo->mtu - SRC_2M_ACL_AVDTP_STREAM_OFFSET) /
                            sbc_frame_size;
            if (sbc_frame_num > max_frame_num)
            {
                APP_PRINT_INFO2("src_adjust_stream_codec: mtu %u limits %u num",
                                sinfo->mtu, max_frame_num);
                sbc_frame_num = max_frame_num;
            }
        }
        else
        {
            APP_PRINT_INFO1("src_adjust_stream_codec(): %u dev mtu is 0", i);
        }
        APP_PRINT_INFO4("src_adjust_stream_codec: num %u -> %u bitpool %u -> %u",
                        sinfo->sbc_max_frame_num, sbc_frame_num,
                        codec->attr.sbc.bitpool, bitpool);
        sinfo->sbc_frame_size = sbc_frame_size;
        sinfo->sbc_max_frame_num = sbc_frame_num;
        sinfo->sbc_offset = 0;
        sinfo->buf_size = sinfo->sbc_max_frame_num * sinfo->sbc_frame_size;
        codec->attr.sbc.bitpool = bitpool;
    }

    if (bitpool_changed)
    {
        src_start_dsp(bitpool);
    }
}

bool app_spp_cmd_send(uint8_t index, uint8_t cmd, uint8_t *data, uint16_t len)
{
    if ((!data) || (!len))
    {
        return false;
    }

    if (index >= SINK_DEV_NUM_MAX)
    {
        return false;
    }

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    uint8_t *bd_addr = app_cfg_nv.public_bud_addr;
#else
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    uint8_t *bd_addr = sink_dev->remote_info.bd_addr;
#endif

    uint16_t spp_pack_len = len + sizeof(T_GAMING_SYNC_HDR) + 1;
    uint8_t *p_pack = os_mem_zalloc(RAM_TYPE_DSPSHARE, spp_pack_len);
    if (!p_pack)
    {
        APP_PRINT_ERROR0("app_spp_cmd_send(): zalloc fail");
        return false;
    }

    T_GAMING_SYNC_HDR *p_hdr = (T_GAMING_SYNC_HDR *)p_pack;
    p_hdr->sync = DONGLE_FORMAT_START_BIT;
    p_hdr->type = DONGLE_TYPE_CMD;
    p_hdr->pl = len + 1;
    p_hdr->cmd = cmd;
    memcpy(p_pack + sizeof(T_GAMING_SYNC_HDR), data, len);
    p_pack[spp_pack_len - 1] = DONGLE_FORMAT_STOP_BIT;

    if (app_gaming_ctrl_send_fix_channel_data(bd_addr, FIX_CHANNEL_CID, p_pack, spp_pack_len,
                                              false) == false)
    {
        APP_PRINT_INFO3("app_spp_cmd_send: %u failed to send %u, %b", TRACE_BDADDR(bd_addr), spp_pack_len,
                        TRACE_BINARY(spp_pack_len, p_pack));
        os_mem_free(p_pack);
        return false;
    }
    os_mem_free(p_pack);
    return true;
}

#ifdef SPP_STREAM_LATENCY_MONITOR
RAM_TEXT_SECTION
static void src_spp_latency_check(T_APP_DEV *sink_dev)
{
    uint32_t seq_gap;
    uint32_t time_gap_ref;
    uint32_t time_gap_real;
    uint32_t time_delta;
    uint32_t now;

    if (!sink_dev)
    {
        return;
    }
#define MAX_SPP_STREAMING_TIME_DRIFT      8000
#define SPP_STREAMING_TIME_DRIFT_BOUND    60000

    now = log_timestamp_get();

    /* Time rolls over. */
    if (now <= sink_dev->spp_anchor_point)
    {
        sink_dev->spp_seq_synchronized = 0;
        return;
    }

    /* FIXME: spp_seq should never roll over.
     * Because the time rolls over before and a2dp_seq will be reset.
     * */
    if (sink_dev->spp_seq > sink_dev->spp_anchor_seq)
    {
        seq_gap = sink_dev->spp_seq - sink_dev->spp_anchor_seq;
    }
    else
    {
        seq_gap = 0xFFFFFFFF - sink_dev->spp_anchor_seq + sink_dev->spp_seq + 1;
    }

    time_gap_ref = 8000 * seq_gap;

    time_gap_real = now - sink_dev->spp_anchor_point;

    if (time_gap_real >= time_gap_ref)
    {
        time_delta = time_gap_real - time_gap_ref;
        if (time_delta > MAX_SPP_STREAMING_TIME_DRIFT)
        {
            APP_PRINT_ERROR4("src_spp_latency_check: LAG, spp seq %u, "
                             "anchor seq %u, anchor point %u, "
                             "time_delta %u",
                             sink_dev->spp_seq, sink_dev->spp_anchor_seq,
                             sink_dev->spp_anchor_point,
                             time_delta);
            if (time_delta > SPP_STREAMING_TIME_DRIFT_BOUND)
            {
                APP_PRINT_ERROR1("src_spp_latency_check: LAG exceeds max %u",
                                 SPP_STREAMING_TIME_DRIFT_BOUND);
                sink_dev->spp_seq = 0;
                sink_dev->spp_seq_synchronized = 0;
            }
        }
    }
    else
    {
        time_delta = time_gap_ref - time_gap_real;
        if (time_delta > MAX_SPP_STREAMING_TIME_DRIFT)
        {
            APP_PRINT_ERROR4("src_spp_latency_check: AHEAD, spp seq %u, "
                             "anchor seq %u, anchor point %u, "
                             "time_delta %u",
                             sink_dev->spp_seq, sink_dev->spp_anchor_seq,
                             sink_dev->spp_anchor_point,
                             time_delta);
            if (time_delta > SPP_STREAMING_TIME_DRIFT_BOUND)
            {
                APP_PRINT_ERROR1("src_spp_latency_check: AHEAD, exceeds max %u",
                                 SPP_STREAMING_TIME_DRIFT_BOUND);
                sink_dev->spp_seq = 0;
                sink_dev->spp_seq_synchronized = 0;
            }
        }
    }
}
#endif

void spp_cmd_register_recv_cback(spp_cmd_cback_t cback)
{
    spp_cmd_recv_cback = cback;
}

void src_spp_audio_packet_received(uint8_t *bdaddr, uint8_t *buf, uint16_t len)
{
    T_GAMING_SYNC_HDR *hdr = (void *)buf;
    uint8_t *params = buf + SRC_SPP_HDR_SIZE;
    uint8_t type;

    /* 'R', Type 0/1, PL, cmd_id, p0, p1, p2, p3, 'T' */
    type = (hdr->type & 0x03);

    switch (type)
    {
    case DONGLE_TYPE_CMD:
        APP_PRINT_INFO1("src_spp_audio_packet_received(): %b",
                        TRACE_BINARY(len, buf));

        /* Remote requests enter/exit low latency */
        if (hdr->cmd == DONGLE_CMD_REQUEST_GAMING_MOE)
        {
            if (params[0] == 0x01)
            {
                src_spp_req_enter_ll(bdaddr, params[1]);
            }
            else
            {
                src_spp_req_leave_ll(bdaddr, params[1]);
            }
        }
#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
        else if (hdr->cmd == 0x04)
        {
            app_ctrl_spp_receive_ack(params, bdaddr);
        }
#endif
        else if (hdr->cmd == DONGLE_CMD_SET_VOL_BALANCE)
        {
            game_volume_pecent = params[0];
            chat_volume_pecent = params[1];

#if ENABLE_UAC2
            app_usb_ds_set_gain_percentage(game_volume_pecent, chat_volume_pecent);
#endif
        }
        else if (hdr->cmd == DONGLE_CMD_FORCE_SUSPEND)
        {
#if TARGET_LEGACY_GAMING_DONGLE
            app_usb_ds_handle_a2dp_suspend(params[0]);
#endif
        }
        else
        {
            legacy_gaming_proc_data(buf + offsetof(T_GAMING_SYNC_HDR, cmd), hdr->pl, bdaddr);

            if (spp_cmd_recv_cback)
            {
                uint8_t *cmd = (uint8_t *)(&(hdr->cmd));
                spp_cmd_recv_cback(bdaddr, cmd, hdr->pl + ((hdr->type) & 0xF0) << 8);
            }
        }
        break;

#if F_APP_GAMING_CONTROLLER_SUPPORT
    case DONGLE_TYPE_CONTROLLER_HID_DATA:
        {
            app_usb_handle_controller_data_from_headset(buf + 3);
        }
        break;
#endif

#if TARGET_LEGACY_GAMING_DONGLE
    case DONGLE_TYPE_UPSTREAM_VOICE:
        {
            /* only support fix channel upstream data */
            us_handle_rcv_pkt(buf, len);
        }
        break;
#endif
    }
}

static void src_dev_handle_voice_data(uint8_t index, void *buf, uint16_t len)
{
    //bool ret = false;
    static uint8_t header_index = 0;

    T_APP_DEV *sink_dev = &sink_dev_list[index];

    //APP_PRINT_INFO2("src_dev_handle_voice_data: %d %d", index, len);

    if ((len == 57) && (sink_dev->sco_connected))
    {
#ifdef AUDIO_DELAY_TEST_SCO_DS
        if (first_sco_ds_enc_pkt)
        {
            Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
            first_sco_ds_enc_pkt = false;
        }
#endif
        bt_sco_data_send(sink_dev->remote_info.bd_addr, header_index,
                         buf, len);

        header_index++;
        header_index %= 4;
    }
}

#ifdef A2DP_STREAM_LATENCY_MONITOR
RAM_TEXT_SECTION
static void src_a2dp_latency_check(T_APP_DEV *sink_dev)
{
    uint32_t seq_gap;
    uint32_t time_gap_ref;
    uint32_t time_gap_real;
    uint32_t time_delta;
    uint32_t now;

    if (!sink_dev)
    {
        return;
    }

    if (!low_latency_mode)
    {
        return;
    }

#define MAX_A2DP_STREAMING_TIME_DRIFT      4000
#define A2DP_STREAMING_TIME_DRIFT_BOUND    60000

    now = log_timestamp_get();

    /* Time rolls over. */
    if (now <= sink_dev->a2dp_anchor_point)
    {
        sink_dev->a2dp_seq_synchronized = 0;
        return;
    }

    /* FIXME: a2dp_seq should never roll over.
     * Because the time rolls over before and a2dp_seq will be reset.
     * */
    if (sink_dev->a2dp_seq > sink_dev->a2dp_anchor_seq)
    {
        seq_gap = sink_dev->a2dp_seq - sink_dev->a2dp_anchor_seq;
    }
    else
    {
        seq_gap = 0xFFFFFFFF - sink_dev->a2dp_anchor_seq + sink_dev->a2dp_seq + 1;
    }

    if (!seq_gap)
    {
        return;
    }

    time_gap_ref = 5333 * seq_gap;

    /* Supplement the time
     * 0.3333333... us drift per a2dp media packet (two sbc frames)
     * 5.333333333333333 ms between two a2dp media packets
     * */
    if (seq_gap < 1024)
    {
        time_gap_ref += (seq_gap >> 2);

    }
    else if (seq_gap >= 1024 && seq_gap < 1024 * 8)
    {
        /* 0.33333333... * 1024 = 341 */
        time_gap_ref += 341 * (seq_gap >> 10);
    }
    else if (seq_gap >= 1024 * 8 && seq_gap < 1024 * 8 * 8)
    {
        /* 0.333333... * 1024 * 8 = 2731 */
        time_gap_ref += 2731 * (seq_gap >> 13);
    }
    else if (seq_gap >= 1024 * 8 * 8 && seq_gap < 1024 * 8 * 8 * 8)
    {
        /* 0.333333... * 1024 * 8 * 8 = 2731 */
        time_gap_ref += 21845 * (seq_gap >> 16);
    }
    else
    {
        /* FIXME: Stop working */
        return;
    }

    time_gap_real = now - sink_dev->a2dp_anchor_point;

    if (time_gap_real >= time_gap_ref)
    {
        time_delta = time_gap_real - time_gap_ref;
        if (time_delta > MAX_A2DP_STREAMING_TIME_DRIFT)
        {
            APP_PRINT_ERROR5("src_a2dp_latency_check: LAG, a2dp seq %u, "
                             "anchor seq %u, ll seq %u, anchor point %u, "
                             "time_delta %u",
                             sink_dev->a2dp_seq, sink_dev->a2dp_anchor_seq,
                             sink_dev->a2dp_ll_seq,
                             sink_dev->a2dp_anchor_point,
                             time_delta);
            if (time_delta > A2DP_STREAMING_TIME_DRIFT_BOUND)
            {
                APP_PRINT_ERROR1("src_a2dp_latency_check: LAG exceeds max %u",
                                 A2DP_STREAMING_TIME_DRIFT_BOUND);
                sink_dev->a2dp_seq = sink_dev->a2dp_ll_seq;
                sink_dev->a2dp_seq_synchronized = 0;
            }
        }
    }
    else
    {
        time_delta = time_gap_ref - time_gap_real;
        if (time_delta > MAX_A2DP_STREAMING_TIME_DRIFT)
        {
            APP_PRINT_ERROR5("src_a2dp_latency_check: AHEAD, a2dp seq %u, "
                             "anchor seq %u, ll seq %u, anchor point %u, "
                             "time_delta %u",
                             sink_dev->a2dp_seq, sink_dev->a2dp_anchor_seq,
                             sink_dev->a2dp_ll_seq,
                             sink_dev->a2dp_anchor_point,
                             time_delta);
            sink_dev->a2dp_seq_synchronized = 0;
            if (time_delta > A2DP_STREAMING_TIME_DRIFT_BOUND)
            {
                APP_PRINT_ERROR1("src_a2dp_latency_check: AHEAD, exceeds max %u",
                                 A2DP_STREAMING_TIME_DRIFT_BOUND);
                sink_dev->a2dp_seq = sink_dev->a2dp_ll_seq;
                sink_dev->a2dp_seq_synchronized = 0;
            }
        }
    }
}
#endif

static void src_dev_handle_read_data(uint8_t index,
                                     uint16_t seq_num, uint8_t frame_num, void *buf, uint16_t len)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_SRC_STREAM_INFO *stream_info = &sink_dev->stream_info;
    uint8_t *buf_in = (uint8_t *)buf;
    uint16_t buf_offset = 0;

    //APP_PRINT_INFO4("src_dev_handle_read_data: index %d, seq %d, frame_num %d, len %d",
    //                index, seq_num, frame_num, len);

    if (stream_info->is_streaming && (stream_info->buffer != NULL))
    {
        if (stream_info->codec_type == BT_A2DP_CODEC_TYPE_SBC ||
            stream_info->codec_type == BT_A2DP_CODEC_TYPE_LC3)
        {
            sink_dev->sbcenc_packets++;
            if (support_a2dp_codec_lc3)
            {
                stream_info->sbc_frame_size = 200;
                stream_info->sbc_max_frame_num = 1;
            }

            while (len >= stream_info->sbc_frame_size)
            {
#ifdef AUDIO_DELAY_TEST_MUSIC
                if (first_music_sbc_pkt)
                {
                    Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                    Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                    first_music_sbc_pkt = false;
                }
#endif

#if 1
                memcpy(stream_info->buffer + stream_info->sbc_offset,
                       buf_in + buf_offset, stream_info->sbc_frame_size);

                stream_info->sbc_offset += stream_info->sbc_frame_size;
                buf_offset += stream_info->sbc_frame_size;
                len -= stream_info->sbc_frame_size;
#if 0
                APP_PRINT_INFO3("src_dev_handle_read_data buf_size %d, sbc_offset %d, sbcenc_packets %d",
                                stream_info->buf_size, stream_info->sbc_offset, sink_dev->sbcenc_packets);
#endif
#else
                if (first_sbc_pkt_flag)
                {
                    for (uint8_t i = 0; i < 29; i++)
                    {
                        //dump_sbc[i] = i;
                        //dump_sbc[29+i] = i;
                        //dump_sbc[58+i] = i;
                    }
                    first_sbc_pkt_flag = 0;
                }
                memcpy(stream_info->buffer + stream_info->sbc_offset,
                       dump_sbc, stream_info->sbc_frame_size);

                stream_info->sbc_offset += stream_info->sbc_frame_size;
                buf_offset += stream_info->sbc_frame_size;
                len -= stream_info->sbc_frame_size;

#endif
                if (stream_info->sbc_offset == stream_info->buf_size)
                {
                    bool rc;

#ifdef AUDIO_DELAY_TEST_MUSIC
                    if (first_music_enc_pkt)
                    {
                        Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                        Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                        first_music_enc_pkt = false;
                    }
#endif
#ifdef A2DP_SRC_TX_FLOW_CONTROL
#ifdef A2DP_STREAM_LATENCY_MONITOR
                    if (!sink_dev->a2dp_seq_synchronized)
                    {
                        sink_dev->a2dp_seq_synchronized = 1;
                        sink_dev->a2dp_seq = sink_dev->a2dp_ll_seq;
                        sink_dev->a2dp_anchor_seq = sink_dev->a2dp_seq;
                        sink_dev->a2dp_anchor_point = log_timestamp_get();
                    }
                    else
                    {
                        src_a2dp_latency_check(sink_dev);
                    }
                    sink_dev->a2dp_seq++;
#endif
                    if (sink_dev->a2dp_stream_credits)
                    {
                        uint16_t len = stream_info->buf_size;

                        if (gaming_bt_low_latency_streaming())
                        {
                            ctrl_pkt_append_to_a2dp(stream_info->buffer + stream_info->sbc_offset, &len);
                        }

                        APP_PRINT_INFO2("bt_a2dp_stream_data_send seq %u len %d", sink_dev->a2dp_ll_seq, len);

                        rc = bt_a2dp_stream_data_send(sink_dev->remote_info.bd_addr,
                                                      sink_dev->a2dp_ll_seq,
                                                      sink_dev->a2dp_timestamp,
                                                      stream_info->sbc_max_frame_num,
                                                      stream_info->buffer,
                                                      len);
                        if (rc)
                        {
                            sink_dev->a2dp_stream_credits--;
                        }
                        else
                        {
                            APP_PRINT_ERROR1("%b Transmit a2dp media pkt failed",
                                             TRACE_BDADDR(sink_dev->remote_info.bd_addr));
                        }
                    }
                    else
                    {
                        APP_PRINT_WARN2("%b has no credits for transmitting a2dp, seq %u",
                                        TRACE_BDADDR(sink_dev->remote_info.bd_addr),
                                        sink_dev->a2dp_ll_seq);
#ifdef A2DP_STREAM_LATENCY_MONITOR
                        sink_dev->a2dp_seq_synchronized = 0;
#endif
                    }

#else /* A2DP_SRC_TX_FLOW_CONTROL */
                    rc = bt_a2dp_stream_data_send(sink_dev->remote_info.bd_addr,
                                                  sink_dev->a2dp_ll_seq,
                                                  sink_dev->a2dp_timestamp,
                                                  stream_info->sbc_max_frame_num,
                                                  stream_info->buffer,
                                                  stream_info->buf_size);
                    if (!rc)
                    {
                        APP_PRINT_ERROR1("dev %u transmit media pkt failed",
                                         index);
                    }
                    else
                    {
                    }
#endif
                    sink_dev->a2dp_ll_seq++;
                    if (support_a2dp_codec_lc3 == false)
                    {
                        sink_dev->a2dp_timestamp += stream_info->sbc_max_frame_num *
                                                    sink_dev->codec_info.attr.sbc.block_length *
                                                    sink_dev->codec_info.attr.sbc.subband_num;
                    }
                    else
                    {
                        uint32_t sample_rate = sink_dev->codec_info.attr.lc3.sample_rate;
                        uint16_t frame_duration;

                        if (sample_rate == 44100)
                        {
                            sample_rate = 48000;
                        }

                        if (sink_dev->codec_info.attr.lc3.frame_duration == AUDIO_LC3_FRAME_DURATION_7_5_MS)
                        {
                            frame_duration = 75;  /* 7.5ms*10 */
                        }
                        else
                        {
                            frame_duration = 100; /* 10ms*10 */
                        }
                        sink_dev->a2dp_timestamp += stream_info->sbc_max_frame_num * sample_rate *
                                                    frame_duration / 10000;
                    }
                    stream_info->sbc_offset = 0;
                }
            }
        }
        else
        {

        }
    }
}


bool sbc_audio_read_cb(uint32_t *timestamp, uint16_t *seq_num,
                       uint8_t *frame_num, void *buf,
                       uint16_t required_len, uint16_t *actual_len)
{
    APP_PRINT_INFO2("sbc_audio_read_cb: len %u, frame_num %u", required_len, *frame_num);

    //app_audio_fill(buf, len);
    *actual_len = required_len;
    uint8_t i;
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        src_dev_handle_read_data(i, 0, *frame_num, buf, required_len);
    }

    return true;
}

#ifdef AUDIO_PLAY_LOCAL
static bool audio_path_aux_in_uapi_cback(uint8_t id, uint8_t event, void *buf,
                                         uint16_t len, uint16_t frm_num)
{
    if (event == EVENT_AUDIO_PATH_DATA_IND)
    {

        APP_PRINT_INFO3("audio_path_aux_in_upai_cback: id %u, data recv, len %u, frmn %u",
                        id, len, frm_num);
        app_audio_path_fill(IT_RELAY_CODE1, buf, len, 0, 0);

        return true;
    }

    switch (event)
    {
    case EVENT_AUDIO_PATH_STREAM_STARTED:
        APP_PRINT_INFO1("audio_path_aux_in_upai_cback: id %u, audio stream started", id);
        break;
    case EVENT_AUDIO_PATH_STREAM_STOPPED:
        APP_PRINT_INFO1("audio_path_aux_in_upai_cback: id %u, audio stream stopped", id);
        break;
    case EVENT_AUDIO_PATH_READY:
        APP_PRINT_INFO1("audio_path_aux_in_upai_cback: id %u, audio path ready", id);
        break;
    case EVENT_AUDIO_PATH_RELEASED:
        APP_PRINT_INFO1("audio_path_aux_in_upai_cback: id %u, audio path released", id);
        break;
    }

    return true;
}

static bool audio_path_spk_out_uapi_cback(uint8_t id, uint8_t event, void *buf,
                                          uint16_t len, uint16_t frm_num)
{
    if (event == EVENT_AUDIO_PATH_DATA_IND)
    {

        //APP_PRINT_INFO3("audio_path_spk_out_upai_cback: id %u, data recv, len %u, frmn %u",
        //                id, len, frm_num);

        return true;
    }

    switch (event)
    {
    case EVENT_AUDIO_PATH_STREAM_STARTED:
        APP_PRINT_INFO1("audio_path_spk_out_upai_cback: id %u, audio stream started", id);
        break;
    case EVENT_AUDIO_PATH_STREAM_STOPPED:
        APP_PRINT_INFO1("audio_path_spk_out_upai_cback: id %u, audio stream stopped", id);
        break;
    case EVENT_AUDIO_PATH_READY:
        APP_PRINT_INFO1("audio_path_spk_out_upai_cback: id %u, audio path ready", id);
        break;
    case EVENT_AUDIO_PATH_RELEASED:
        APP_PRINT_INFO1("audio_path_spk_out_upai_cback: id %u, audio path released", id);
        break;
    }

    return true;
}
#endif

bool usb_audio_data_cb(uint8_t *buf, uint16_t len, uint16_t frame_num)
{
    uint8_t i;

    APP_PRINT_INFO2("usb_audio_data_cb: len %u, frame_num %u", len, frame_num);
    if ((buf != NULL) && (len > 0) && (frame_num > 0))
    {
        if (len == 57)
        {
            for (i = 0; i < SINK_DEV_NUM_MAX; i++)
            {
                src_dev_handle_voice_data(i, buf, 57);
            }

            //app_usb_audio_msbc_fill_us(buf, 57);
        }
        else
        {
            for (i = 0; i < SINK_DEV_NUM_MAX; i++)
            {
                src_dev_handle_read_data(i, 0, frame_num, buf, len);
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

#ifdef APP_AUX_AUDIO
static bool sbc_audio_read_cb(uint32_t *timestamp, uint16_t *seq_num,
                              uint8_t *frame_num, void *buf,
                              uint16_t required_len, uint16_t *actual_len)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        src_dev_handle_read_data(i, *seq_num, *frame_num, buf, required_len);
    }

    *actual_len = required_len;
    return true;
}
#endif

static void src_handle_a2dp_open(uint8_t *bd_addr, uint16_t remote_mtu)
{
    uint8_t index = 0;
    uint8_t bond_flag = 0;

    APP_PRINT_INFO3("src_handle_a2dp_open: %s mtu %d aux %d",
                    TRACE_BDADDR(bd_addr), remote_mtu, src_is_aux_in());

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        /* the below a2dp stream init may use a2dp_opened */
        sink_dev_list[index].a2dp_opened = 1;

        sink_dev_list[index].a2dp_ll_seq = 0;
        sink_dev_list[index].a2dp_timestamp = 0;
#ifdef A2DP_STREAM_LATENCY_MONITOR
        sink_dev_list[index].a2dp_seq = 0;
        sink_dev_list[index].a2dp_anchor_seq = 0;
        sink_dev_list[index].a2dp_seq_synchronized = 0;
        sink_dev_list[index].a2dp_anchor_point = 0;
#endif

        /* TODO: framework reports the real space which only includes SBC
         * frames. We assume that the media packet contains Content protection
         * header.
         * */
        src_init_a2dp_stream_info(index, remote_mtu + SRC_2M_ACL_AVDTP_STREAM_OFFSET);

        bond_flag = app_src_legacy_get_bond_flag_by_index(index);
        APP_PRINT_INFO1("src_handle_a2dp_open: bond_flag %08x", bond_flag);

        /* If the audio is not from aux in, we assume that it is usb ds
         * In order to reduce latency, we send avdtp start here instead of
         * the time when usb audio is active
         */
#ifdef APP_AUX_AUDIO
        if (src_is_aux_in())
        {
            if (sink_dev_list[index].pending_req == 0)
            {
                APP_PRINT_INFO0("a2dp_open: Send stream start req for aux in");
                if (bt_a2dp_stream_start_req(bd_addr))
                {
                    sink_dev_list[index].pending_req = APP_AVDTP_START_REQ;
                }
            }
        }
#endif

        APP_PRINT_INFO2("a2dp_open: auto_gaming %u, low_latency_mode %u",
                        src_general_cfg.auto_gaming, low_latency_mode);
        if (src_general_cfg.auto_gaming && (!low_latency_mode))
        {
            app_start_timer(&timer_idx_src_gaming_mode[index], "dongle_auto_gaming",
                            src_policy_timer_id, SRC_TIMER_ID_AUTO_GAMING_MODE + index, 0, false,
                            1000);
        }

        /*Sometimes the A2DP has not been opened after the spp connection is successful,
          will not send eixt gaming mode.*/
        if (profile_is_connected(sink_dev_list[index].remote_info.bd_addr, SPP_AUDIO_PROFILE_MASK))
        {
            src_spp_req_leave_ll(bd_addr, 0);
        }
    }
}

static uint8_t src_get_stream_dev_num(void)
{
    uint8_t num = 0;
    int i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].stream_info.is_streaming)
        {
            num++;
        }
    }

    return num;
}

bool src_is_streaming(uint8_t *bdaddr)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (!memcmp(sink_dev_list[i].remote_info.bd_addr, bdaddr, 6) &&
            sink_dev_list[i].stream_info.is_streaming)
        {
            return true;
        }
    }

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (adapter_a2dp_is_connected() && !memcmp(app_cfg_nv.public_bud_addr, bdaddr, 6))
    {
        return app_usb_ds_is_a2dp_stream();
    }
#endif

    return false;
}

#if 0
static void app_audio_policy_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;
    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            if ((param->track_state_changed).state == AUDIO_TRACK_STATE_STARTED)
            {
#ifdef AUDIO_DELAY_TEST_MUSIC
                if (first_music_start)
                {
                    first_music_start = false;
                    APP_PRINT_INFO0("Audio track started");
                    Pad_Config(P1_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                    Pad_Config(P1_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                }
#endif
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_audio_policy_cback: event_type 0x%04x", event_type);
    }
}

#endif

static bool __attribute__((unused)) is_avdtp_req_pending(void)
{
    int i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].pending_req == APP_AVDTP_START_REQ ||
            sink_dev_list[i].pending_req == APP_AVDTP_SUSPEND_REQ)
        {
            return true;
        }
    }

    return false;
}

#if 0
static inline void app_src_init_test_sbcdec(T_AUDIO_FORMAT_INFO *info)
{
    info->type = AUDIO_FORMAT_TYPE_SBC;
    info->attr.sbc.sample_rate = 16000; /* ICODEC_SR_16K */;
    info->attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
    info->attr.sbc.block_length = 16; /* 16 */
    info->attr.sbc.subband_num = 8; /* 8 */
    info->attr.sbc.allocation_method = 0; /* Loudness */
    info->attr.sbc.bitpool = 22;
}
#endif

static void src_handle_a2dp_stream_start(uint8_t *bd_addr)
{
    uint8_t index = 0;

    APP_PRINT_INFO0("src_handle_a2dp_stream_start");

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (adapter_a2dp_is_connected())
    {
        bt_avrcp_play_status_change_register_rsp(app_cfg_nv.public_bud_addr, BT_AVRCP_PLAY_STATUS_PLAYING);
    }
#endif

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        bt_avrcp_play_status_change_register_rsp(bd_addr, BT_AVRCP_PLAY_STATUS_PLAYING);
        sink_dev_list[index].pending_req = 0;
        sink_dev_list[index].sbcenc_packets = 0;
        sink_dev_list[index].stream_idle_count = 0;
#ifdef A2DP_STREAM_LATENCY_MONITOR
        sink_dev_list[index].a2dp_anchor_seq = 0;
        sink_dev_list[index].a2dp_seq_synchronized = 0;
        sink_dev_list[index].a2dp_anchor_point = 0;
#endif

#ifdef APP_AUX_AUDIO
        if (app_audio_type != AUDIO_TYPE_NONE)
        {
            /* Maybe there is another a2dp stream prior to this stream. */
            APP_PRINT_INFO0("a2dp stream start: Audio has been opened");
            sink_dev_list[index].stream_info.is_streaming = true;
            return;
        }

        if (src_is_aux_in())
        {
            app_audio_type = AUDIO_TYPE_AUX_IN;
            app_src_sbc_voice_start_capture(sink_dev_list[index].codec_info,
                                            sbc_audio_read_cb);
        }
#endif
#ifdef AUDIO_DELAY_TEST_MUSIC
        first_music_enc_pkt = true;
        first_music_sbc_pkt = true;
        first_music_start = true;
        Pad_Config(P1_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
        Pad_Config(P1_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
        Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
        Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
        Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
        Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
        Pad_Config(P2_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
        Pad_Config(P2_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
#endif
        sink_dev_list[index].stream_info.is_streaming = true;

        /* enable always poll when a2dp stream start and disable when a2dp
           stream stop to save power consumption and bandwidth */
        if (src_legacy_gaming_is_ready())
        {
            legacy_gaming_xfer_start(bd_addr, LEGACY_GAMING_XFER_TYPE);
        }

    }
}

/* This func can be invoked in interrupt context. */
RAM_TEXT_SECTION
bool gaming_bt_a2dp_is_streaming(void)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].stream_info.is_streaming)
        {
            return true;
        }
    }

    return false;
}

static void src_handle_a2dp_stream_stop(uint8_t *bd_addr)
{
    uint8_t index = 0;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev_list[index].stream_info.is_streaming = false;
        sink_dev_list[index].stream_info.sbc_offset = 0;
        sink_dev_list[index].pending_req = 0;

        app_stop_timer(&timer_idx_src_stream_idle[index]);
        sink_dev_list[index].sbcenc_packets = 0;
        sink_dev_list[index].stream_idle_count = 0;
        sink_dev_list[index].a2dp_ll_seq = 0;
        sink_dev_list[index].a2dp_timestamp = 0;
#ifdef A2DP_STREAM_LATENCY_MONITOR
        sink_dev_list[index].a2dp_anchor_seq = 0;
        sink_dev_list[index].a2dp_seq_synchronized = 0;
        sink_dev_list[index].a2dp_anchor_point = 0;
#endif
    }

    APP_PRINT_INFO1("src_handle_a2dp_stream_stop(): index %u", index);

    if ((!app_cfg_const.dongle_link_num_max))
    {
        /* enable always poll when a2dp stream start and disable when a2dp
           stream stop to save power consumption and bandwidth */
        legacy_gaming_xfer_stop(bd_addr, LEGACY_GAMING_XFER_TYPE);
    }

    //stop stream if no streaming device exist
    if (src_get_stream_dev_num() == 0)
    {
#ifdef APP_AUX_AUDIO
        if (app_audio_type == AUDIO_TYPE_AUX_IN)
        {
            app_src_sbc_voice_stop_capture();
        }
#endif
#if APP_USB_AUDIO
        if (app_audio_type == AUDIO_TYPE_USB_DS)
        {
            /* NOTE: audio path is released in a2dp stream close handler. */
            bt_avrcp_play_status_change_register_rsp(bd_addr, BT_AVRCP_PLAY_STATUS_PAUSED);

            return;
        }
#endif
        if (app_audio_type == AUDIO_TYPE_NONE)
        {
            APP_PRINT_ERROR0("a2dp stream stop: Audio has been already closed");
        }
//        app_audio_type = AUDIO_TYPE_NONE;
        bt_avrcp_play_status_change_register_rsp(bd_addr, BT_AVRCP_PLAY_STATUS_PAUSED);
    }
}

static void src_handle_a2dp_stream_close(uint8_t *bd_addr)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;


    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];
        sink_dev->pending_req = 0;
        sink_dev->a2dp_opened = 0;

        sink_dev->stream_info.is_streaming = false;

        if (sink_dev->stream_info.buffer != NULL)
        {
            os_mem_free(sink_dev->stream_info.buffer);
            sink_dev->stream_info.buffer = NULL;
        }

        app_stop_timer(&timer_idx_src_stream_idle[index]);
        sink_dev_list[index].sbcenc_packets = 0;
        sink_dev_list[index].stream_idle_count = 0;
        sink_dev_list[index].a2dp_ll_seq = 0;
        sink_dev_list[index].a2dp_timestamp = 0;
#ifdef A2DP_STREAM_LATENCY_MONITOR
        sink_dev_list[index].a2dp_seq = 0;
        sink_dev_list[index].a2dp_anchor_seq = 0;
        sink_dev_list[index].a2dp_seq_synchronized = 0;
        sink_dev_list[index].a2dp_anchor_point = 0;
#endif
    }

    //stop stream if no streaming device exist
    if (src_get_stream_dev_num() == 0)
    {
        if (app_audio_type == AUDIO_TYPE_NONE)
        {
            APP_PRINT_ERROR0("a2dp stream close: Audio has been already closed");
        }
        app_audio_type = AUDIO_TYPE_NONE;

        bt_avrcp_play_status_change_register_rsp(bd_addr, BT_AVRCP_PLAY_STATUS_STOPPED);
    }

}

static void src_a2dp_reconn(uint8_t index, uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_DEV *sink_dev = NULL;

    APP_PRINT_INFO2("src_a2dp_reconn: address %s, profile %08x",
                    TRACE_BDADDR(bd_addr), profile_mask);

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_a2dp_reconn: invalid index %u", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    if (sink_dev->bt_state != STATE_BT_ACL_CONNECTED)
    {
        APP_PRINT_INFO2("src_a2dp_reconn: index %d, addr %s, acl disconnected",
                        index, TRACE_BDADDR(bd_addr));
        return;
    }

    if (sink_dev->dev_state != STATE_DEV_CONNECTED)
    {
        APP_PRINT_INFO2("src_a2dp_reconn: index %d, addr %s, device disconnected",
                        index, TRACE_BDADDR(bd_addr));
        return;
    }

    //sink_dev->dev_state = STATE_BT_PAGING;
    sink_dev->dev_state = STATE_DEV_RECONNECTING;
    memcpy(sink_dev->remote_info.bd_addr, bd_addr, 6);
    sink_dev->profile_mask_plan = profile_mask;
    sink_dev->profile_mask_remain = profile_mask;

    /* TODO */
    gap_br_send_acl_disconn_req(sink_dev_list[0].remote_info.bd_addr);
    //sink_dev->connecting = true;
    //src_dev_profile_conn(index);
}

void src_handle_profile_disconn_cmpl(uint8_t *bd_addr, uint16_t cause, uint32_t profile_mask)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);

    if (!p_link)
    {
        APP_PRINT_WARN0("src_handle_profile_disconn_cmpl(): No link exists");
        return;
    }

    del_connected_profile(bd_addr, profile_mask);

    APP_PRINT_INFO3("src_handle_profile_disconn_cmpl: %s, profile_mask %04x, cause %02x",
                    TRACE_BDADDR(bd_addr), profile_mask, cause);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (profile_mask == A2DP_PROFILE_MASK)
    {
        src_control_remote_mic(false, &sink_dev_list[0]);
    }
#endif

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        APP_PRINT_INFO2("src_handle_profile_disconn_cmpl: connected_mask %04x, dev_state %02x",
                        p_link->connected_profile, sink_dev->dev_state);

        if (bt_uapi_cback)
        {
            struct gaming_ev_profile_disconnected ev;

            ev.id = index;
            memcpy(ev.ba, bd_addr, 6);
            ev.profile = profile_mask;
            bt_uapi_cback(EVENT_GAMING_PROFILE_DISCONNECTED, (void *)&ev,
                          sizeof(ev));
        }

        if (profile_mask == SPP_AUDIO_PROFILE_MASK)
        {
            src_control_remote_mic(false, sink_dev);
        }

        if (profile_mask == A2DP_PROFILE_MASK)
        {
            uint8_t bdaddr[6];

            memcpy(bdaddr, bd_addr, 6);
            sink_dev->pending_req = 0;
            sink_dev->stream_info.is_streaming = false;
            sink_dev->a2dp_opened = 0;
            app_stop_timer(&timer_idx_src_a2dp_cfg[index]);
            app_stop_timer(&timer_idx_src_a2dp[index]);

            if (sink_dev->stream_info.buffer != NULL)
            {
                os_mem_free(sink_dev->stream_info.buffer);
            }

            memset(&sink_dev->stream_info, 0, sizeof(T_APP_SRC_STREAM_INFO));

            if (src_get_stream_dev_num() == 0)
            {
                app_audio_type = AUDIO_TYPE_NONE;
                bt_avrcp_play_status_change_register_rsp(bd_addr, BT_AVRCP_PLAY_STATUS_STOPPED);
            }
            if (a2dp_reconn_timeout[index])
            {
                a2dp_reconn_timeout[index] = false;
                src_a2dp_reconn(index, bdaddr, A2DP_PROFILE_MASK);
            }
        }

        switch (sink_dev->dev_state)
        {
        case STATE_DEV_RECONNECTING:
        case STATE_DEV_CONNECTING:
            {
                if ((cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                    (cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)) ||
                    (cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)))
                {
                    APP_PRINT_INFO0("src_handle_profile_disconn_cmpl: wait for disconnected");
                }
                else if (cause == (L2C_ERR | L2C_ERR_SECURITY_BLOCK))
                {
                    gap_br_send_acl_disconn_req(bd_addr);
                }
                else if (sink_dev->profile_mask_doing == profile_mask)
                {
                    /* if the current profile connection was not established,
                     * we try to connect to next profile.
                     * */
                    src_profile_conn_finish(index);
                    src_dev_profile_conn(index);
                }
            }
            break;

        case STATE_DEV_CLEANUP:
            {
                if ((cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                    (cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
                {
                    APP_PRINT_INFO0("src_handle_profile_disconn_cmpl: wait for disconnected");
                }
                else if (cause == (L2C_ERR | L2C_ERR_SECURITY_BLOCK))
                {
                    gap_br_send_acl_disconn_req(bd_addr);
                }
                else
                {
                    if ((p_link != NULL) && (p_link->connected_profile == 0))
                    {
                        gap_br_send_acl_disconn_req(bd_addr);
                    }
                }
            }
            break;

        default:
            break;
        }
    }
}

static uint32_t get_profile_mask_from_bond_flag(uint16_t bond_flag)
{
    uint32_t profile_mask = 0;

    if (bond_flag & APP_SRC_BOND_FLAG_A2DP)
    {
        profile_mask |= A2DP_PROFILE_MASK;
        profile_mask |= SPP_AUDIO_PROFILE_MASK;
    }

    return profile_mask;
}

static uint32_t get_bond_flag_from_profile_mask(uint32_t profile_mask)
{
    uint32_t bond_flag = 0;

    if (profile_mask & A2DP_PROFILE_MASK)
    {
        bond_flag |= APP_SRC_BOND_FLAG_A2DP;
    }

    return bond_flag;
}

static uint32_t load_doing_profile(uint32_t remain_profile_mask)
{
    int i;

    for (i = 0; i < sizeof(src_supported_profiles) / sizeof(src_supported_profiles[0]); i++)
    {
        if (src_supported_profiles[i] & remain_profile_mask)
        {
            return src_supported_profiles[i];
        }
    }

    return 0;
}

static bool get_profile_uuid_from_mask(uint32_t profile_mask,
                                       T_GAP_UUID_DATA *uuid,
                                       T_GAP_UUID_TYPE *type)
{
    if (!uuid || !type)
    {
        APP_PRINT_ERROR0("get_profile_uuid_from_mask(): Invalid paramter");
        return false;
    }

    switch (profile_mask)
    {
    case A2DP_PROFILE_MASK:
        {
            uuid->uuid_16 = UUID_AUDIO_SINK;
            *type = GAP_UUID16;
        }
        break;

    case SPP_AUDIO_PROFILE_MASK:
        {
            memcpy(uuid->uuid_128, UUID_SPP_AUDIO, 16);
            *type = GAP_UUID128;
        }
        break;
    case AVRCP_PROFILE_MASK:
        {
            uuid->uuid_16 = UUID_AV_REMOTE_CONTROL;
            *type = GAP_UUID16;
        }
        break;
    default:
        return false;
    }

    switch (*type)
    {
    case GAP_UUID16:
        APP_PRINT_INFO1("profile uuid: 0x%04x", uuid->uuid_16);
        break;
    case GAP_UUID32:
        APP_PRINT_INFO1("profile uuid: 0x%08x", uuid->uuid_32);
        break;
    case GAP_UUID128:
        APP_PRINT_INFO1("profile uuid: %b", TRACE_BINARY(16, uuid->uuid_128));
        break;
    }

    return true;
}

static bool get_connected_profile(uint8_t *bd_addr, uint32_t *profile_masks)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (profile_masks != NULL)
        {
            *profile_masks = p_link->connected_profile;
        }
        return true;
    }

    return false;
}

bool profile_is_connected(uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & profile_mask)
        {
            return true;
        }
    }

    return false;
}

static void add_conneted_profile(uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile |= profile_mask;
    }
}

static void del_connected_profile(uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile &= ~profile_mask;
    }
}

static void src_profile_conn_finish(uint8_t index)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    sink_dev->profile_mask_remain &= ~sink_dev->profile_mask_doing;
    sink_dev->profile_mask_doing = 0;
    sink_dev->sdp_info.is_found = false;
}

static void src_profile_conn_exit(uint8_t index)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO1("src_profile_conn_exit(): index %u", index);

    p_link = app_find_br_link(sink_dev->remote_info.bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & SPP_AUDIO_PROFILE_MASK)
        {
            /* After the spp profile connection is completed, the dongle
             * actively enters the low latency mode
             * */
            APP_PRINT_INFO2("spp_profile_conn_exit: auto_gaming %u,"
                            " low_latency_mode %u",
                            src_general_cfg.auto_gaming,
                            low_latency_mode);
            if (src_general_cfg.auto_gaming && (!low_latency_mode))
            {
                app_start_timer(&timer_idx_src_gaming_mode[index], "dongle_auto_gaming",
                                src_policy_timer_id, SRC_TIMER_ID_AUTO_GAMING_MODE + index, 0, false,
                                1000);
            }
        }
        if ((p_link->connected_profile & sink_dev->profile_mask_plan) != 0)
        {
            src_handle_action_result(index, true);
        }
        else
        {
            APP_PRINT_ERROR0("src_profile_conn_exit: send acl disc req");
            //cleanup when disconnected
            gap_br_send_acl_disconn_req(p_link->bd_addr);
        }
    }
}

static bool src_profile_search_start(uint8_t index)
{
    bool ret = false;
    T_GAP_UUID_DATA uuid;
    T_GAP_UUID_TYPE uuid_type = GAP_UUID16;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    ret = get_profile_uuid_from_mask(sink_dev->profile_mask_doing, &uuid,
                                     &uuid_type);
    if (ret)
    {
        if (gap_br_start_sdp_discov(sink_dev->remote_info.bd_addr, uuid_type,
                                    uuid) == GAP_CAUSE_SUCCESS)
        {
            sink_dev->sdp_info.is_busy = true;
            ret = true;
        }
    }

    return ret;
}

static void src_dev_profile_conn(uint8_t index)
{
    uint32_t connected_profiles;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO2("src_profile_conn: mask %08x %08x",
                    sink_dev->profile_mask_remain,
                    sink_dev->profile_mask_plan);

RETRY:
    if (sink_dev->profile_mask_remain == 0)
    {
        src_profile_conn_exit(index);
        return;
    }

    sink_dev->profile_mask_doing = load_doing_profile(sink_dev->profile_mask_remain);

    if (get_connected_profile(sink_dev->remote_info.bd_addr, &connected_profiles))
    {
        if (connected_profiles & sink_dev->profile_mask_doing)
        {
            src_profile_conn_finish(index);
            goto RETRY;
        }
    }

    if (!src_profile_search_start(index))
    {
        src_profile_conn_finish(index);
        goto RETRY;
    }
}

static bool src_profile_conn(uint8_t *bd_addr, uint32_t profile_mask, T_APP_SRC_SDP_INFO sdp_info)
{
    bool ret = true;

    switch (profile_mask)
    {
    case A2DP_PROFILE_MASK:
        ret = bt_a2dp_connect_req(bd_addr, sdp_info.protocol_version, BT_A2DP_ROLE_SNK, 0);
        break;
    case SPP_AUDIO_PROFILE_MASK:
        ret = bt_rfc_conn_req(bd_addr, RFC_SPP_AUDIO_CHANN_NUM,
                              sdp_info.server_channel, 1012, 100);
        break;
    case AVRCP_PROFILE_MASK:
        ret = bt_avrcp_connect_req(bd_addr);
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

static void src_profile_disconn(uint8_t *bd_addr, uint32_t profile_mask)
{
    APP_PRINT_INFO2("src_profile_disconn: address %s, profile 0x%08x",
                    TRACE_BDADDR(bd_addr), profile_mask);

    if (profile_mask & A2DP_PROFILE_MASK)
    {
        bt_a2dp_disconnect_req(bd_addr);
    }

    if (profile_mask & AVRCP_PROFILE_MASK)
    {
        bt_avrcp_disconnect_req(bd_addr);
    }

    if (profile_mask & SPP_AUDIO_PROFILE_MASK)
    {
        bt_rfc_disconn_req(bd_addr, RFC_SPP_AUDIO_CHANN_NUM);
    }
}

void src_dev_conn(uint8_t index, uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_DEV *sink_dev = NULL;

    APP_PRINT_INFO2("src_dev_conn: address %s, profile %08x",
                    TRACE_BDADDR(bd_addr), profile_mask);

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_dev_conn: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    if (sink_dev->bt_state == STATE_BT_ACL_CONNECTED)
    {
        APP_PRINT_INFO2("src_dev_conn: index %d, addr %s, acl already conn, wait timeout",
                        index, TRACE_BDADDR(bd_addr));
        return;
    }

    memcpy(sink_dev->remote_info.bd_addr, bd_addr, 6);

    sink_dev->bt_state = STATE_BT_PAGING;
    sink_dev->profile_mask_plan = profile_mask;
    sink_dev->profile_mask_remain = profile_mask;

    if (discov_state == DISCOVERY_STARTING || discov_state == DISCOVERY_FINDING)
    {
        uint8_t pre_state;

        pre_state = discov_state;
        if (!gap_br_stop_inquiry())
        {
            discov_state = DISCOVERY_STOPPING;
            if (bt_uapi_cback)
            {
                bt_uapi_cback(EVENT_GAMING_DISCOVERY_STOPPING, NULL, 0);
            }
        }
        else
        {
            discov_state = pre_state;
            APP_PRINT_ERROR0("src_dev_conn: stop inquiry error");
        }
    }
    if (gap_br_start_did_discov(bd_addr))
    {
        sink_dev->bt_state = STATE_BT_IDLE;
    }
    else
    {
        sink_dev->connecting = true;
        current_index = index;
    }
}

static void src_start_reconn(uint8_t index)
{
    uint8_t bd_addr[6];
    uint32_t bond_flag = 0;
    uint32_t profile_mask = 0;

    app_src_legacy_get_bond_addr_by_index(index, bd_addr);
    bond_flag = app_src_legacy_get_bond_flag_by_index(index);
    profile_mask = get_profile_mask_from_bond_flag(bond_flag);

    src_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);

    APP_PRINT_INFO1("src_start_reconn(): mask %08x", profile_mask);

    src_dev_conn(index, bd_addr, profile_mask);
}

static void src_start_conn(uint8_t index)
{
    uint8_t inquiry_len = 10;

    APP_PRINT_INFO0("src_start_conn(): ++");
    src_check_set_bt_mode();
    sink_dev_list[index].bt_state = STATE_BT_INQUIRY;

    if (discov_state == DISCOVERY_STOPPED)
    {
        discov_state = DISCOVERY_STARTING;
        /* Make headset reconnecting to us quickly. */

        if (app_cfg_const.enable_dongle_dual_mode)
        {
            /* Use le scan to discover device */
        }
        else
        {
            if (autopairing)
            {
                inquiry_len = 5;
            }
            if (gap_br_start_inquiry(false, inquiry_len))
            {
                APP_PRINT_ERROR1("src_start_conn(): Dev %u failed to start inquiry",
                                 index);
                sink_dev_list[index].bt_state = STATE_BT_IDLE;
                discov_state = DISCOVERY_STOPPED;

                return;
            }
        }
        if (bt_uapi_cback)
        {
            bt_uapi_cback(EVENT_GAMING_DISCOVERY_STARTING, NULL, 0);
        }
    }

    /* discov_state might be STARTING, FINDING, STOPPING.
     * If it is stopping, we will schedule next dev in
     * src_handle_inquiry_cancel_cmpl(). So do not worry.
     *
     * Previously the bt_state of dev has been set to STATE_BT_INQUIRY and
     * dev_state has been set to CONNECTING.
     * */
    sink_dev_list[index].connecting = true;
    memset(sink_dev_list[index].headset_info, 0,
           sizeof(sink_dev_list[index].headset_info));
    sink_dev_list[index].headset_info_valid = false;
    current_index = index;
}

static bool src_start_disconn(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);

    if (p_link != NULL)
    {
#if 0
        if (p_link->connected_profile != 0)
        {
            src_profile_disconn(p_link->bd_addr, p_link->connected_profile);
            //start timer here
        }
        else
#endif
        {
            gap_br_send_acl_disconn_req(p_link->bd_addr);
        }

        return true;
    }

    return false;
}

static void src_handle_dev_pending_evt(uint8_t index, T_APP_SRC_KEY_EVENT pending_evt)
{
    uint8_t i = 0;

    if (shutdown)
    {
        for (i = 0; i < SINK_DEV_NUM_MAX; i++)
        {
            //can not change bt_mode while bt_state not in BT_IDLE
            if (sink_dev_list[i].bt_state != STATE_BT_IDLE)
            {
                return;
            }
        }
        src_set_bt_mode(BT_DEVICE_MODE_IDLE);
        return;
    }

    if (pending_evt > KEY_NONE)
    {
        memset(&sink_dev_list[index], 0, sizeof(T_APP_DEV));

        if (pending_evt == KEY_SHORT)
        {
            key_short_press_handle(index);
        }
        else if (pending_evt == KEY_LONG)
        {
            key_long_press_handle(index);
        }
    }


    /* Schedule when it is the end of cleanup */
    if (sink_dev_list[current_index].dev_state != STATE_DEV_CONNECTING &&
        sink_dev_list[current_index].dev_state != STATE_DEV_RECONNECTING)
    {
        src_dev_schedule_next();
    }
}

static void src_dev_cleanup(uint8_t index, T_APP_SRC_KEY_EVENT evt)
{
    T_APP_DEV *sink_dev;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_dev_cleanup: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];
    sink_dev->pending_event = evt;

    a2dp_reconn_timeout[index] = false;
    app_stop_timer(&timer_idx_src_a2dp[index]);
    app_stop_timer(&timer_idx_src_stream_idle[index]);

    APP_PRINT_INFO3("src_dev_cleanup: index %d, dev_state %04x, bt_state %04x",
                    index, sink_dev->dev_state, sink_dev->bt_state);

    switch (sink_dev->dev_state)
    {
    case STATE_DEV_IDLE:
        {
            if (shutdown)
            {
                /* The dev state changes to STATE_DEV_IDLE */
                memset(sink_dev, 0, sizeof(T_APP_DEV));
                src_led_set_idle(index);

                /* app will start reconnecting if the evt is short press
                    */
                src_handle_dev_pending_evt(index, evt);
            }
        }
        break;

    case STATE_DEV_CONNECTING:
        {
            app_stop_timer(&timer_idx_src_conn[index]);
            /* It's almost the same as above. */
            if (sink_dev->bt_state == STATE_BT_INQUIRY)
            {
                if (app_cfg_const.enable_dongle_dual_mode)
                {
                    discov_state = DISCOVERY_STOPPED;

                    memset(sink_dev, 0, sizeof(T_APP_DEV));
                    src_led_set_idle(index);

                    if (bt_uapi_cback)
                    {
                        bt_uapi_cback(EVENT_GAMING_DISCOVERY_STOPPED, NULL, 0);
                    }
                }
                else
                {
                    uint8_t pre_state = discov_state;

                    /* No matter the device is inquiring or not, the stack always
                     * return BT_EVENT_INQUIRY_CANCEL_RSP.
                     */
                    discov_state = DISCOVERY_STOPPING;
                    if (gap_br_stop_inquiry())
                    {
                        discov_state = pre_state;
                    }
                    else
                    {
                        if (bt_uapi_cback)
                        {
                            bt_uapi_cback(EVENT_GAMING_DISCOVERY_STOPPING, NULL, 0);
                        }
                    }
                    sink_dev->dev_state = STATE_DEV_CLEANUP;
                }
                return;
            }
        }
    /* fall through */

    case STATE_DEV_RECONNECTING:
        {
            app_stop_timer(&timer_idx_src_reconn[index]);
            if (sink_dev->bt_state == STATE_BT_PAGING)
            {
                /* If hci create conn cmd is not sent or acl haven't been set
                 * up, stack will wait until l2cap chan is established, then
                 * stack disconnects it.
                 *
                 * So app would receive acl disconn event and clean the device
                 */
                gap_br_stop_sdp_discov(sink_dev->remote_info.bd_addr);
                sink_dev->dev_state = STATE_DEV_CLEANUP;
            }
            else if (sink_dev->bt_state == STATE_BT_ACL_CONNECTED)
            {
                /* Wait for acl conn to disconnect */
                gap_br_send_acl_disconn_req(sink_dev->remote_info.bd_addr);
                sink_dev->dev_state = STATE_DEV_CLEANUP;
            }
            else
            {
                /* The dev state changes to STATE_DEV_IDLE */
                memset(sink_dev, 0, sizeof(T_APP_DEV));
                src_led_set_idle(index);

                /* app will start reconnecting if the evt is short press
                 */
                src_handle_dev_pending_evt(index, evt);
            }
        }
        break;

    case STATE_DEV_CONNECTED:
        {
            /* Wait for acl conn to be disconn. */
            if (src_start_disconn(sink_dev->remote_info.bd_addr))
            {
                sink_dev->dev_state = STATE_DEV_CLEANUP;
            }
            else
            {
                APP_PRINT_ERROR0("src_dev_cleanup: No device found");
            }
        }
        break;

    case STATE_DEV_CLEANUP:
        {
            APP_PRINT_WARN0("src_dev_cleanup: Already cleanup");
        }
        break;

    default:
        break;
    }
}

bool src_legacy_gaming_is_ready(void)
{
    return legacy_gaming_ready;
}

void src_legacy_gaming_event_cback(T_LEGACY_GAMING_EVENT event, uint8_t *addr)
{
    APP_PRINT_TRACE1("src_legacy_gaming_event_cback: event %d", event);

    switch (event)
    {
    case LEGACY_GAMING_READY:
        {
            legacy_gaming_ready = true;

            if (src_is_streaming(addr))
            {
                legacy_gaming_xfer_start(addr, LEGACY_GAMING_XFER_TYPE);
            }
        }
        break;

    case LEGACY_GAMING_STOP:
        {
            legacy_gaming_ready = false;
        }
        break;

    default:
        break;
    }
}

void key_short_press_handle(uint8_t index)
{
    T_APP_DEV *sink_dev;

    if (src_auto_link_cfg.single_link && index > 0)
    {
        APP_PRINT_WARN1("key_short_press_handle(): single link, ignore %u",
                        index);
        return;
    }

    if (shutdown)
    {
        APP_PRINT_WARN1("key_short_press_handle: index %d not handled in shutdown", index);
        return;
    }

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_INFO1("key_short_press_handle: invalid %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("key_short_press_handle: index %d, dev_state %04x, bt_state %04x",
                    index, sink_dev->dev_state, sink_dev->bt_state);

    switch (sink_dev->dev_state)
    {
    case STATE_DEV_RECONNECTING:
        {
            APP_PRINT_INFO0("key_short_press_handle: stop reconnecting");
            /* Stop reconnecting. */
            src_dev_cleanup(index, KEY_NONE);
        }
        break;

    case STATE_DEV_IDLE:
        {
            if (src_dev_is_bonded(index))
            {
                src_enter_reconnecting_state(index);
                /* Maybe remote is connecting to us */
                if (!src_bt_is_busy())
                {
                    src_start_reconn(index);
                }
            }
        }
        break;

    case STATE_DEV_CONNECTING:
        {
            if (src_dev_is_bonded(index))
            {
                /* Pending event will processed after acl disc or inquiry
                 * cancelled.
                 *
                 * Or maybe the key event will be handled in this cleanup func
                 * Because the device bt state is IDLE
                 * */
                src_dev_cleanup(index, KEY_SHORT);
            }
            else
            {
                APP_PRINT_INFO0("key_short_press_handle: CONNECTING, no dev bonded");
            }
        }
        break;

    case STATE_DEV_CONNECTED:
        {
            APP_PRINT_WARN0("key_short_press_handle: disconnecting");
            src_dev_cleanup(index, KEY_NONE);
        }
        break;

    case STATE_DEV_CLEANUP:
        {
            APP_PRINT_INFO1("key_short_press_handle: CLEANUP pending event %d",
                            sink_dev->pending_event);

            /* sink_dev->pending_event = KEY_SHORT; */
        }
        break;

    default:
        break;
    }
}

void key_long_press_handle(uint8_t index)
{
    T_APP_DEV *sink_dev;

    if (src_auto_link_cfg.single_link && index > 0)
    {
        APP_PRINT_WARN1("key_long_press_handle(): single link, ignore %u",
                        index);
        return;
    }

    if (shutdown)
    {
        APP_PRINT_WARN1("key_long_press_handle: index %d not handled in shutdown", index);
        return;
    }

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_INFO1("key_long_press_handle: invalid %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("key_long_press_handle: index %d, dev_state %04x, bt_state %04x",
                    index, sink_dev->dev_state, sink_dev->bt_state);

    switch (sink_dev->dev_state)
    {
    case STATE_DEV_IDLE:
        {
            src_enter_connecting_state(index);
            if (!src_bt_is_busy())
            {
                src_start_conn(index);
            }
        }
        break;

    case STATE_DEV_RECONNECTING:
        {
            src_dev_cleanup(index, KEY_LONG);
        }
        break;

    case STATE_DEV_CONNECTING:
        {
            APP_PRINT_WARN0("key_long_press_handle: already connecting");
        }
        break;

    case STATE_DEV_CONNECTED:
        {
            APP_PRINT_WARN0("key_long_press_handle: disconnecting");
            src_dev_cleanup(index, KEY_LONG);
        }
        break;

    case STATE_DEV_CLEANUP:
        {
            APP_PRINT_INFO1("key_long_press_handle: CLEANUP pending event %d",
                            sink_dev->pending_event);

            /* sink_dev->pending_event = KEY_LONG; */
        }
        break;

    default:
        break;
    }
}

void gaming_bt_try_start_stream(void)
{
    uint8_t i = 0;
    T_APP_DEV *sink_dev = NULL;
    T_APP_BR_LINK *p_link = NULL;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];
        p_link = app_find_br_link(sink_dev->remote_info.bd_addr);
        if (!p_link)
        {
            continue;
        }
        if (sink_dev->dev_state == STATE_DEV_CONNECTED &&
            (p_link->connected_profile & A2DP_PROFILE_MASK) &&
            !sink_dev->stream_info.is_streaming &&
            !sink_dev->pending_req &&
            sink_dev->a2dp_opened &&
            !force_suspend_resume_flag
            && (dongle_status.usb_is_streaming || app_line_in_plug_in() ||
                app_cfg_const.dongle_media_device == 1)
           )
        {
            APP_PRINT_INFO0("gaming_bt_try_start_stream: Send start req");
            if (bt_a2dp_stream_start_req(sink_dev->remote_info.bd_addr))
            {
                sink_dev->pending_req = APP_AVDTP_START_REQ;
            }
            else
            {
                APP_PRINT_WARN0("gaming_bt_try_start_stream: failed to send start req");
            }
        }
    }
}

void src_handle_poweron(void)
{
    shutdown = false;

    /* dm_power_off_disable(0xff); */

    //src_check_set_bt_mode();
    /* FIXME */
    /* src_auto_link_cfg.single_link = app_cfg_const.single_link;
     * src_auto_link_cfg.autopair = app_cfg_const.autopair;
     * src_auto_link_cfg.autopair_timeout = app_cfg_const.autopair_timeout;
     * src_auto_link_cfg.autopair_general = app_cfg_const.autopair_general;
     * src_auto_link_cfg.autopair_rssi = app_cfg_const.autopair_rssi;
     * src_general_cfg.sbc_frames_per_pkt = app_cfg_const.app_dongle_sbc_frames_per_pkt;
     */
    src_auto_link_cfg.single_link = 0;
    src_auto_link_cfg.autopair = 0;
    src_auto_link_cfg.autopair_timeout = 0;
    src_auto_link_cfg.autopair_general = 1;
    src_auto_link_cfg.autopair_rssi = -60;
    src_general_cfg.sbc_frames_per_pkt = 0;

    app_start_timer(&timer_idx_src_poweron_delay, "src_poweron_delay",
                    src_policy_timer_id, SRC_TIMER_ID_POWERON_DELAY, 0, false,
                    SRC_POWERON_DELAY_TIMEOUT);
}

void src_poweroff_cleanup(void)
{
    uint8_t i;

    shutdown = true;

    //src_set_bt_mode(BT_DEVICE_MODE_IDLE);

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        src_dev_cleanup(i, KEY_NONE);
    }

#ifdef APP_AUX_AUDIO
    app_src_sbc_voice_stop_capture();
#endif
    app_audio_type = AUDIO_TYPE_NONE;

    //see if need to reset to switch a2dp role
    src_check_product_role_switch();
}

static bool src_is_aux_in(void)
{
    if (app_aux_detect_state_get() == AUX_DETECT_IN)
    {
        return true;
    }

    return false;
}

void src_handle_aux_in(void)
{
#ifdef APP_AUX_AUDIO
    uint8_t i;

    /* If there is at least one stream and aux audio is not open, we start aux
     * audio */
    if (src_get_stream_dev_num())
    {
        if (app_audio_type != AUDIO_TYPE_AUX_IN)
        {
            for (i = 0; i < SINK_DEV_NUM_MAX; i++)
            {
                if (sink_dev_list[i].stream_info.is_streaming)
                {
                    app_src_sbc_voice_start_capture(sink_dev_list[i].codec_info,
                                                    sbc_audio_read_cb);
                    app_audio_type = AUDIO_TYPE_AUX_IN;
                    break;
                }
            }
        }
    }

    APP_PRINT_INFO0("AUX IN detected");
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (!sink_dev_list[i].stream_info.is_streaming &&
            sink_dev_list[i].pending_req == 0)
        {
            if (bt_a2dp_stream_start_req(sink_dev_list[i].remote_info.bd_addr))
            {
                sink_dev_list[i].pending_req = APP_AVDTP_START_REQ;
            }
        }
    }
#endif
}

void src_handle_aux_out(void)
{
#ifdef APP_AUX_AUDIO
    uint8_t i;

    APP_PRINT_INFO0("AUX OUT detected");
    if (app_audio_type == AUDIO_TYPE_AUX_IN)
    {
        /* TODO: Although the handler of avdtp suspend will close audio, we
         * close audio here to avoid missing avdtp suspend resp from remote
         */
        app_src_sbc_voice_stop_capture();
        app_audio_type = AUDIO_TYPE_NONE;

#if APP_USB_AUDIO
        /* If usb audio downstream is streaming and there is at least one a2dp
         * streaming, we open usb audio pipe and retain a2dp streaming
         */
        if (app_usb_audio_is_ds_streaming() && src_get_stream_dev_num())
        {
            app_audio_type = AUDIO_TYPE_USB_DS;
            for (i = 0; i < SINK_DEV_NUM_MAX; i++)
            {
                if (sink_dev_list[i].stream_info.is_streaming)
                {
                    struct path_iovec iv[2];
                    uint8_t ids[2];
                    uint8_t ivn;
                    T_AUDIO_FORMAT_INFO iofmt;

                    memset(iv, 0, sizeof(iv));
                    iv[0].it = IT_AUX;
                    iv[0].ot = OT_SBC;
                    iv[0].ident = "it_aux_ot_a2dp";
                    iofmt.attr.pcm.sample_rate = INPUT_SAMPLE_RATE;
                    iofmt.type = AUDIO_FORMAT_TYPE_PCM;
                    iv[0].ifmt = &iofmt;
                    iv[0].ofmt = src_get_codec_fmt(SRC_CODEC_SBCENC);
                    iv[0].uapi_cback = audio_path_enc_uapi_cback;
                    iv[0].priority = 0;
                    ivn = 1;
                    app_audio_path_createv(iv, ivn ids);
                    break;
                }

            }
            return;
        }
#endif

        for (i = 0; i < SINK_DEV_NUM_MAX; i++)
        {
            if (sink_dev_list[i].stream_info.is_streaming &&
                sink_dev_list[i].pending_req == 0)
            {
                if (bt_a2dp_stream_suspend_req(sink_dev_list[i].remote_info.bd_addr))
                {
                    sink_dev_list[i].pending_req = APP_AVDTP_START_REQ;
                }
            }
        }
    }
#endif
}

static void src_control_remote_mic(bool active, T_APP_DEV *sink_dev)
{
    uint8_t data[SRC_SPP_HDR_SIZE + 2];
    T_GAMING_SYNC_HDR *hdr = (void *)data;

    hdr->sync = DONGLE_FORMAT_START_BIT;
    hdr->type = DONGLE_TYPE_CMD;
    hdr->pl   = 0x02;
    hdr->cmd  = DONGLE_CMD_REQ_OPEN_MIC;
    if (active)
    {
        data[SRC_SPP_HDR_SIZE] = 0x01;
    }
    else
    {
        data[SRC_SPP_HDR_SIZE] = 0x00;
    }
    data[SRC_SPP_HDR_SIZE + 1] = DONGLE_FORMAT_STOP_BIT;

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (active)
    {
        bt_sniff_mode_disable(app_cfg_nv.public_bud_addr);

        gap_br_send_fix_chann_data(app_cfg_nv.public_bud_addr, FIX_CHANNEL_CID, data, sizeof(data), false);
        src_set_tpoll(app_cfg_nv.public_bud_addr, SRC_LOW_LATENCY_TPOLL);
    }
    else
    {
        bt_sniff_mode_enable(app_cfg_nv.public_bud_addr, 784, 816, 0, 0);

        gap_br_send_fix_chann_data(app_cfg_nv.public_bud_addr, FIX_CHANNEL_CID, data, sizeof(data), false);
        src_set_tpoll(app_cfg_nv.public_bud_addr, SRC_NORMAL_TPOLL);
    }

#else
    if (active)
    {
        if (sink_dev->mic_open == false)
        {
            sink_dev->mic_open = true;
            bt_sniff_mode_disable(sink_dev->remote_info.bd_addr);

            app_gaming_ctrl_send_fix_channel_data(sink_dev->remote_info.bd_addr, FIX_CHANNEL_CID, data,
                                                  sizeof(data),
                                                  false);
            src_set_tpoll(sink_dev->remote_info.bd_addr, SRC_LOW_LATENCY_TPOLL);
        }
    }
    else
    {
        if (sink_dev->mic_open)
        {
            sink_dev->mic_open = false;
            bt_sniff_mode_enable(sink_dev->remote_info.bd_addr, 784, 816, 0, 0);

            app_gaming_ctrl_send_fix_channel_data(sink_dev->remote_info.bd_addr, FIX_CHANNEL_CID, data,
                                                  sizeof(data),
                                                  false);
            src_set_tpoll(sink_dev->remote_info.bd_addr, SRC_NORMAL_TPOLL);
        }
    }
#endif
}

void __app_src_uac_us_status(bool active)
{
    uint8_t i;
    T_APP_DEV *sink_dev = NULL;
    bool headset_connected = false;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
        headset_connected = adapter_acl_is_connected();
#else
        headset_connected = profile_is_connected(sink_dev->remote_info.bd_addr, SPP_AUDIO_PROFILE_MASK);
#endif

        APP_PRINT_INFO2("app_src_uac_us_status: active %d headset_connected %d", active, headset_connected);

        if (headset_connected)
        {
            src_control_remote_mic(active, sink_dev);
        }
    }
}

void gaming_bt_audio_capture_switch(bool active)
{
    if (active)
    {
        uac_us_state = true;
        __app_src_uac_us_status(true);
    }
    else
    {
        uac_us_state = false;
        __app_src_uac_us_status(false);
    }
}

void gaming_bt_set_volume(uint32_t vol, uint8_t mute)
{
    uint8_t i;

    APP_PRINT_INFO1("gaming_bt_set_volume: volume 0x%02x", vol);
    if (mute)
    {
        vol = 0;
    }

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (profile_is_connected(sink_dev_list[i].remote_info.bd_addr,
                                 AVRCP_PROFILE_MASK))
        {
            if (bt_avrcp_absolute_volume_set(sink_dev_list[i].remote_info.bd_addr, vol))
            {
                APP_PRINT_INFO2("gaming_bt_set_volume: %b set vol 0x%02x succeeded",
                                TRACE_BDADDR(sink_dev_list[i].remote_info.bd_addr),
                                vol);
            }
            else
            {
                APP_PRINT_INFO2("gaming_bt_set_volume: %b set vol 0x%02x failed",
                                TRACE_BDADDR(sink_dev_list[i].remote_info.bd_addr),
                                vol);
            }
        }
    }
}

void app_src_enable_auto_linking(void)
{
    autopairing = true;
    connect_attempt_count = 0xff;
    APP_PRINT_INFO0("app_src_enable_auto_linking(): Autopairing");
    if (src_auto_link_cfg.autopair_timeout)
    {
        app_start_timer(&timer_idx_src_auto_pair, "src_autopair",
                        src_policy_timer_id, SRC_TIMER_ID_AUTOPAIR, 0, false,
                        src_auto_link_cfg.autopair_timeout * 1000);
    }
    key_long_press_handle(0);
}

static void src_handle_poweron_delay_timeout(void)
{
    uint8_t i;
    T_APP_DEV *sink_dev;
    uint32_t timeout;
    bool connecting = false;

    /* a2dp_stream_credits = A2DP_STREAM_MAX_CREDITS; */

#ifdef APP_AUX_AUDIO
    app_src_sbc_voice_stop_capture();
#endif
    app_audio_type = AUDIO_TYPE_NONE;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev_list[i].a2dp_stream_credits = A2DP_STREAM_MAX_CREDITS;
        src_led_set_idle(i);
        if (src_auto_link_cfg.autopair)
        {
            continue;
        }
        if (src_auto_link_cfg.single_link && i > 0)
        {
            continue;
        }
        key_short_press_handle(i);
        if (sink_dev_list[i].dev_state == STATE_DEV_CONNECTING ||
            sink_dev_list[i].dev_state == STATE_DEV_RECONNECTING)
        {
            connecting = true;
        }
    }

    /* If there is no connecting or reconnecting, enable disc and connectable */
    if (!connecting)
    {
        //src_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
    }

    if (!src_auto_link_cfg.autopair)
    {
        return;
    }

    APP_PRINT_INFO0("src_handle_poweron_delay_timeout() Autopair is enabled");
#if F_APP_LEGACY_DONGLE_BINDING
#else
    /* TODO: if autopair is enabled, how many reconnecting times would be
     * suitable.
     */
    reconnect_intervals_len = 2;
#endif

    autopairing_index = 0xff;
    autopairing = true;
    timeout = src_auto_link_cfg.autopair_timeout;
    if (timeout)
    {
        timeout *= 1000;
        app_start_timer(&timer_idx_src_auto_pair, "src_autopair",
                        src_policy_timer_id, SRC_TIMER_ID_AUTOPAIR, 0, false,
                        timeout);
    }
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (src_auto_link_cfg.single_link && i > 0)
        {
            continue;
        }

        sink_dev = &sink_dev_list[i];
        if (sink_dev->dev_state == STATE_DEV_IDLE)
        {
            APP_PRINT_INFO0("src_handle_poweron_delay_timeout(): Autopairing");
            autopairing_index = i;
            connect_attempt_count = 0xff;
            /* Similar to long key press */
            src_enter_connecting_state(i);
            if (!src_bt_is_busy())
            {
                src_start_conn(i);
            }
            break;
        }
    }
}

static void src_handle_acl_exist_timeout(uint8_t index)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = NULL;
    uint32_t target_profile_mask = A2DP_PROFILE_MASK | SPP_AUDIO_PROFILE_MASK | AVRCP_PROFILE_MASK;

    APP_PRINT_INFO1("src_handle_acl_exist_timeout(): index %u", index);

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_handle_acl_exist_timeout: invalid index %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    p_link = app_find_br_link(sink_dev->remote_info.bd_addr);
    if (!p_link)
    {
        src_handle_action_result(index, false);
        return;
    }

    if (sink_dev->dev_state == STATE_DEV_IDLE ||
        sink_dev->dev_state == STATE_DEV_CONNECTING ||
        sink_dev->dev_state == STATE_DEV_RECONNECTING ||
        sink_dev->dev_state == STATE_DEV_CONNECTED)
    {
        if (p_link->connected_profile != target_profile_mask)
        {
            sink_dev->profile_mask_plan = target_profile_mask;
            sink_dev->profile_mask_remain = p_link->connected_profile ^ target_profile_mask;

            /* signal channel connected, no discover cmd received
             * start avdtp discover by open api
             */

            if (!sink_dev->codec_configured &&
                (p_link->connected_profile & A2DP_PROFILE_MASK))
            {
                bt_a2dp_stream_open_req(sink_dev->remote_info.bd_addr, BT_A2DP_ROLE_SNK);
            }

            if (sink_dev->dev_state == STATE_DEV_IDLE)
            {
                src_dev_state_set(index, STATE_DEV_RECONNECTING);
#if F_APP_LEGACY_DONGLE_BINDING
#else
                /* Avoid too many times of reconnecting if this reconnecting
                 * failed
                 */
                sink_dev->attempt = reconnect_intervals_len;
#endif
            }
            src_dev_profile_conn(index);
        }
        else
        {
            src_handle_action_result(index, true);
        }
    }
}

static void src_a2dp_cfg_reverse(uint8_t index)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(sink_dev_list[index].remote_info.bd_addr);
    if (!sink_dev_list[index].codec_configured && p_link &&
        (p_link->connected_profile & A2DP_PROFILE_MASK))
    {
        APP_PRINT_INFO0("src_a2dp_cfg_reverse():  Send a2dp open req");
        bt_a2dp_stream_open_req(sink_dev_list[index].remote_info.bd_addr, BT_A2DP_ROLE_SNK);
        app_start_timer(&timer_idx_src_a2dp[index], "a2dp_reverse",
                        src_policy_timer_id, SRC_TIMER_ID_A2DP + index, 0, false,
                        3000);
    }
}

static void src_connecting_timeout(uint8_t index)
{

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_INFO1("src_connecting_timeout(): Invalid index %u", index);
        return;
    }

    /* If BT is inquiring or paging, just return. Later the timer will start
     * again when schedule runs again.
     *
     * If the corresponding acl is connected, do the same thing above.
     *
     * Leave the attempt untouched, because the next sched will not increase
     * the attempt
     */
    if (src_bt_is_busy() ||
        sink_dev_list[index].bt_state == STATE_BT_ACL_CONNECTED)
    {
        return;
    }

    if (sink_dev_list[index].dev_state == STATE_DEV_CONNECTING)
    {
        src_start_conn(index);
    }
}

static void src_reconnecting_timeout(uint8_t index)
{
    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_INFO1("src_reconnecting_timeout(): Invalid index %u", index);
        return;
    }

    /* If BT is inquiring or paging, just return. Later the timer will
     * start again when schedule runs again.
     *
     * If the corresponding acl is connected, do the same thing above.
     *
     * Leave the attempt untouched, because the next sched will not increase
     * the attempt
     */
    if (src_bt_is_busy() ||
        sink_dev_list[index].bt_state == STATE_BT_ACL_CONNECTED)
    {
        return;
    }

    if (src_dev_is_bonded(index) &&
        sink_dev_list[index].dev_state == STATE_DEV_RECONNECTING)
    {
        src_start_reconn(index);
    }
}

static void src_handle_stream_idle_timeout(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_handle_stream_idle_timeout: invalid index %d",
                        index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    /* APP_PRINT_INFO2("src_handle_stream_idle_timeout: index %d, dev state %d",
     *                 index, sink_dev->dev_state);
     */

    if (sink_dev->stream_info.is_streaming)
    {
        if (sink_dev->sbcenc_packets > 0)
        {
            sink_dev->stream_idle_count = 0;
        }
        else
        {
            sink_dev->stream_idle_count++;
        }

        sink_dev->sbcenc_packets = 0;

        if (sink_dev->stream_idle_count >= SRC_STREAM_IDLE_MAX)
        {
            if (bt_a2dp_stream_suspend_req(sink_dev->remote_info.bd_addr))
            {
                sink_dev->pending_req = APP_AVDTP_SUSPEND_REQ;
            }
            else
            {
                APP_PRINT_WARN0("src_handle_stream_idle_timeout: failed to "
                                "send suspend req");
            }
        }
        else
        {
            app_start_timer(&timer_idx_src_stream_idle[index], "src_stream_idle",
                            src_policy_timer_id, SRC_TIMER_ID_STREAM_IDLE + index, 0, false,
                            SRC_STREAM_IDLE_TIMEOUT);
        }
    }
}
#if 0
static void prepare_audio_path(void)
{
#if APP_USB_AUDIO
    struct path_iovec iv[3];
    uint8_t i = 0;
    uint8_t ids[3];
    T_AUDIO_FORMAT_INFO fmt[3];

    APP_PRINT_INFO0("prepare_audio_path");

    if (support_a2dp_codec_lc3)
    {
        app_src_init_lc3enc(&fmt[0]);
        src_set_codec_fmt(SRC_CODEC_LC3ENC,  &fmt[0]);
        app_src_init_lc3dec(&fmt[1]);
        src_set_codec_fmt(SRC_CODEC_LC3DEC, &fmt[1]);
        if (src_general_cfg.uac2)
        {
            app_src_init_lc3enc2(&fmt[2]);
            src_set_codec_fmt(SRC_CODEC_LC3ENC2, &fmt[2]);
        }
    }
    else
    {
        app_src_init_sbcenc(&fmt[0]);
        src_set_codec_fmt(SRC_CODEC_SBCENC,  &fmt[0]);
        app_src_init_sbcdec(&fmt[1]);
        src_set_codec_fmt(SRC_CODEC_SBCDEC, &fmt[1]);
        if (src_general_cfg.uac2)
        {
            app_src_init_sbcenc2(&fmt[2]);
            src_set_codec_fmt(SRC_CODEC_SBCENC2, &fmt[2]);
        }
    }

    memset(iv, 0, sizeof(iv));
    memset(fmt, 0, sizeof(fmt));

    if (src_general_cfg.uac2)
    {
        iv[i].it = IT_UDEV_IN2;
        if (support_a2dp_codec_lc3)
        {
            iv[i].ot = OT_LC3FRM2;
            iv[i].ident = "it_uac2_ot_lc3frm2";
        }
        else
        {
            iv[i].ot = OT_SBC2;
            iv[i].ident = "it_uac2_ot_sbc2";
        }
        fmt[i].attr.pcm.sample_rate = INPUT2_SAMPLE_RATE;
        fmt[i].type = AUDIO_FORMAT_TYPE_PCM;
        iv[i].ifmt = &fmt[i];
        if (support_a2dp_codec_lc3)
        {
            iv[i].ofmt = src_get_codec_fmt(SRC_CODEC_LC3ENC2);
        }
        else
        {
            iv[i].ofmt = src_get_codec_fmt(SRC_CODEC_SBCENC2);
        }
        iv[i].uapi_cback = audio_path_enc2_uapi_cback;
        iv[i].priority = 0;
        iv[i].mix = 1;
        i++;
    }

    if (app_cfg_const.dongle_media_device == 1)
    {
        iv[i].it = IT_AUX;
        app_audio_type = AUDIO_TYPE_AUX_IN;
    }
    else if (app_cfg_const.dongle_media_device == 2)
    {
        iv[i].it = IT_MIC;
        app_audio_type = AUDIO_TYPE_AUX_IN;
    }
    else
    {
        iv[i].it = IT_UDEV_IN1;
        app_audio_type = AUDIO_TYPE_USB_DS;
    }
    if (support_a2dp_codec_lc3)
    {
        iv[i].ot = OT_LC3FRM;
        iv[i].ident = "it_uac1_ot_lcfrm";
    }
    else
    {
        iv[i].ot = OT_SBC;
        iv[i].ident = "it_uac1_ot_sbc";
    }
    fmt[i].attr.pcm.sample_rate = INPUT_SAMPLE_RATE;
    fmt[i].type = AUDIO_FORMAT_TYPE_PCM;
    iv[i].ifmt = &fmt[i];
    if (support_a2dp_codec_lc3)
    {
        iv[i].ofmt = src_get_codec_fmt(SRC_CODEC_LC3ENC);
    }
    else
    {
        iv[i].ofmt = src_get_codec_fmt(SRC_CODEC_SBCENC);
    }
    iv[i].uapi_cback = audio_path_enc_uapi_cback;
    iv[i].priority = 0;
    if (src_general_cfg.uac2)
    {
        iv[i].mix = 1;
    }
    i++;

    if (support_a2dp_codec_lc3)
    {
        iv[i].it = IT_LC3FRM;
    }
    else
    {
        iv[i].it = IT_SBC;
    }
    if (app_cfg_const.dongle_media_device)
    {
        iv[i].ot = OT_SPK;
    }
    else
    {
        iv[i].ot = OT_UDEV_OUT1;
    }

    iv[i].ident = "it_spp_ot_uac1";
    if (support_a2dp_codec_lc3)
    {
        iv[i].ifmt = src_get_codec_fmt(SRC_CODEC_LC3DEC);
    }
    else
    {
        iv[i].ifmt = src_get_codec_fmt(SRC_CODEC_SBCDEC);
    }
    fmt[i].attr.pcm.sample_rate = (iv[i].ifmt)->attr.sbc.sample_rate;
    fmt[i].type = AUDIO_FORMAT_TYPE_PCM;
    iv[i].ofmt = &fmt[i];
    iv[i].uapi_cback = audio_path_dec_uapi_cback;
    iv[i].priority = 0;
    i++;

    /*FIXME: dongle spk-out not support yet, do not create spk-out track*/
    if (!app_cfg_const.dongle_media_device)
    {
        app_audio_path_createv(iv, i, ids);
    }
    else
    {
        app_audio_path_createv(iv, i - 1, ids);
    }

#endif /* APP_USB_AUDIO */
}
#endif

static void src_policy_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    /* Avoid to output too much log. */
    if (timer_evt != SRC_TIMER_ID_STREAM_IDLE &&
        timer_evt != SRC_TIMER_ID_STREAM_IDLE_2)
        APP_PRINT_INFO2("src_policy_timeout_cb: timer_evt %d, param %d",
                        timer_evt, param);

    switch (timer_evt)
    {
    case SRC_TIMER_ID_POWERON_DELAY:
        {
            app_stop_timer(&timer_idx_src_poweron_delay);
            src_handle_poweron_delay_timeout();
        }
        break;

#ifdef DONGLE_BIS_CES_DEMO
    case 0x15:
        {
            uint8_t broadcast_code[16] = {0};
            bs_start_broadcast(broadcast_code);
        }
        break;
#endif
    case SRC_TIMER_ID_PAIRING_MODE:
        {
            ota_mode = false;
            app_stop_timer(&timer_idx_src_pairing_mode);

            /* app_led_set_mode(APP_LED_MODE_POWER_ON); */

            src_check_set_bt_mode();
        }
        break;

    case SRC_TIMER_ID_ACL_EXIST_SINK0:
        {
            app_stop_timer(&sink_dev_list[0].timer_idx_acl_exist_delay);
            src_handle_acl_exist_timeout(0);
        }
        break;

    case SRC_TIMER_ID_A2DP_CFG:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: a2dp 0 cfg");
            app_stop_timer(&timer_idx_src_a2dp_cfg[0]);
            src_a2dp_cfg_reverse(0);
        }
        break;

    case SRC_TIMER_ID_A2DP:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: a2dp 0 timeout");
            app_stop_timer(&timer_idx_src_a2dp[0]);

            if (sink_dev_list[0].codec_configured)
            {
                return;
            }

            /* Disconnect l2cap chan.
             * In src_handle_profile_disconn_cmpl(), app starts profile conn
             * again
             *
             * TODO: But some devices didn't reply l2cap disconn rsp. Instead
             * they entered sniff mode.
             */
            if (sink_dev_list[0].bt_state == STATE_BT_ACL_CONNECTED)
            {
                APP_PRINT_INFO0("src_policy_timeout_cb: Disc 0 l2cap chan of a2dp");
                a2dp_reconn_timeout[0] = true;
                src_profile_disconn(sink_dev_list[0].remote_info.bd_addr,
                                    A2DP_PROFILE_MASK);
            }
        }
        break;

    case SRC_TIMER_ID_CONN:
        {
            APP_PRINT_INFO1("src_policy_timeout_cb: Connecting timer 0 expired, attempt %u",
                            sink_dev_list[0].attempt);
            app_stop_timer(&timer_idx_src_conn[0]);
            src_connecting_timeout(0);
        }
        break;

    case SRC_TIMER_ID_RECONN:
        {
            APP_PRINT_INFO1("src_policy_timeout_cb: Reconnecting timer 0 expired, attempt %u",
                            sink_dev_list[0].attempt);
            app_stop_timer(&timer_idx_src_reconn[0]);
            src_reconnecting_timeout(0);
        }
        break;

    case SRC_TIMER_ID_STREAM_IDLE:
        {
            app_stop_timer(&timer_idx_src_stream_idle[0]);
            src_handle_stream_idle_timeout(0);
        }
        break;

#if SINK_DEV_NUM_MAX > 1
    case SRC_TIMER_ID_ACL_EXIST_SINK1:
        {
            app_stop_timer(&sink_dev_list[1].timer_idx_acl_exist_delay);
            src_handle_acl_exist_timeout(1);
        }
        break;

    case SRC_TIMER_ID_A2DP_CFG + 1:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: a2dp 1 cfg");
            app_stop_timer(&timer_idx_src_a2dp_cfg[1]);
            src_a2dp_cfg_reverse(1);
        }
        break;

    case SRC_TIMER_ID_A2DP + 1:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: a2dp 1 timeout");
            app_stop_timer(&timer_idx_src_a2dp[1]);

            if (sink_dev_list[0].bt_state == STATE_BT_ACL_CONNECTED)
            {
                APP_PRINT_INFO0("src_policy_timeout_cb: Disc 1 l2cap chan of a2dp");
                a2dp_reconn_timeout[1] = true;
                src_profile_disconn(sink_dev_list[1].remote_info.bd_addr,
                                    A2DP_PROFILE_MASK);
            }
        }
        break;

    case SRC_TIMER_ID_CONN + 1:
        {
            APP_PRINT_INFO1("src_policy_timeout_cb: Connecting timer 1 expired, attempt %u",
                            sink_dev_list[1].attempt);
            app_stop_timer(&timer_idx_src_conn[1]);
            src_connecting_timeout(1);
        }
        break;

    case SRC_TIMER_ID_RECONN + 1:
        {
            APP_PRINT_INFO1("src_policy_timeout_cb: Reconnecting timer 1 expired, attempt %u",
                            sink_dev_list[1].attempt);
            app_stop_timer(&timer_idx_src_reconn[1]);
            src_reconnecting_timeout(1);
        }
        break;

    case SRC_TIMER_ID_STREAM_IDLE + 1:
        {
            app_stop_timer(&timer_idx_src_stream_idle[1]);
            src_handle_stream_idle_timeout(1);
        }
        break;
#endif

    case SRC_TIMER_ID_AUTOPAIR:
        {
            uint8_t i = autopairing_index;

            APP_PRINT_INFO0("src_policy_timeout_cb: Autopairing timeout");
            autopairing_index = 0xff;
            app_stop_timer(&timer_idx_src_auto_pair);
            autopairing = false;
            connect_attempt_count = CONN_ATTEMPT_COUNT_MAX;
            /* Let the scheduler stop the device connecting */
            if (i < SINK_DEV_NUM_MAX &&
                sink_dev_list[i].dev_state == STATE_DEV_CONNECTING)
            {
                APP_PRINT_INFO1("src_policy_timeout_cb: %u pairing last time",
                                i);
                sink_dev_list[i].attempt = connect_attempt_count - 1;
            }
        }
        break;

#ifdef CONFIG_SPP_AUDIO_STAT
    case SRC_TIMER_ID_SPP_AUDIO_STAT:
        APP_PRINT_INFO1("src_policy_timeout_cb: link 0 spp audio count %u",
                        spp_sbc_stat[0]);
        spp_sbc_stat[0] = 0;
        app_start_timer(&timer_idx_src_app_audio_stat[0], "spp_audio_stat",
                        src_policy_timer_id, SRC_TIMER_ID_SPP_AUDIO_STAT, 0, false,
                        1000);
        break;

    case SRC_TIMER_ID_SPP_AUDIO_STAT + 1:
        APP_PRINT_INFO1("src_policy_timeout_cb: link 1 spp audio count %u",
                        spp_sbc_stat[1]);
        spp_sbc_stat[1] = 0;
        app_start_timer(&timer_idx_src_app_audio_stat[1], "spp_audio_stat",
                        src_policy_timer_id, SRC_TIMER_ID_SPP_AUDIO_STAT + 1, 0, false,
                        1000);
        break;
#endif

    case SRC_TIMER_ID_PAIRING_TIMEOUT:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: Pairing timer 0 timeout");
            app_stop_timer(&timer_idx_src_pairing_timeout[0]);

            connect_attempt_count = CONN_ATTEMPT_COUNT_MAX;
            if (sink_dev_list[0].dev_state == STATE_DEV_CONNECTING)
            {
                APP_PRINT_INFO0("src_policy_timeout_cb: link 0 pairing last time");
                /* Make scheduler not select this dev. */
                sink_dev_list[0].attempt = connect_attempt_count - 1;
                gap_br_stop_inquiry();
                if (sink_dev_list[0].bt_state == STATE_BT_INQUIRY)
                {
                    src_handle_action_result(0, false);
                }
            }
        }
        break;

    case SRC_TIMER_ID_PAIRING_TIMEOUT + 1:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: Pairing timer 1 timeout");
            app_stop_timer(&timer_idx_src_pairing_timeout[1]);

            connect_attempt_count = CONN_ATTEMPT_COUNT_MAX;
            if (sink_dev_list[1].dev_state == STATE_DEV_CONNECTING)
            {
                APP_PRINT_INFO0("src_policy_timeout_cb: link 1 pairing last time");
                /* Make scheduler not select this dev. */
                sink_dev_list[1].attempt = connect_attempt_count - 1;
                gap_br_stop_inquiry();
                if (sink_dev_list[1].bt_state == STATE_BT_INQUIRY)
                {
                    src_handle_action_result(1, false);
                }
            }
        }
        break;

    case SRC_TIMER_ID_LINKBACK_TIMEOUT:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: LinkBack timer 0 timeout");
            app_stop_timer(&timer_idx_src_linkback_timeout[0]);

            if (sink_dev_list[0].dev_state == STATE_DEV_RECONNECTING)
            {
                src_dev_cleanup(0, KEY_NONE);
            }

        }
        break;

    case SRC_TIMER_ID_LINKBACK_TIMEOUT + 1:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: LinkBack timer 1 timeout");
            app_stop_timer(&timer_idx_src_linkback_timeout[1]);

            if (sink_dev_list[1].dev_state == STATE_DEV_RECONNECTING)
            {
                src_dev_cleanup(1, KEY_NONE);
            }
        }
        break;

    case SRC_TIMER_ID_AUTO_POWEROFF_TIMEOUT:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: auto_power_off 0 timeout");
            app_stop_timer(&timer_idx_src_auto_power_off_timeout[0]);
            auto_power_off_timeout_flag[0] = true;
            if (auto_power_off_timeout_flag[0] && auto_power_off_timeout_flag[1])
            {
                auto_power_off_timeout_flag[0] = false;
                auto_power_off_timeout_flag[1] = false;
                /* TODO: don't do power off in this file. */
                /* app_mmi_handle_action(MMI_DEV_POWER_OFF); */
            }
        }
        break;

    case SRC_TIMER_ID_AUTO_POWEROFF_TIMEOUT + 1:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: auto_power_off 1 timeout");
            app_stop_timer(&timer_idx_src_auto_power_off_timeout[1]);
            auto_power_off_timeout_flag[1] = true;
            if (auto_power_off_timeout_flag[0] && auto_power_off_timeout_flag[1])
            {
                auto_power_off_timeout_flag[0] = false;
                auto_power_off_timeout_flag[1] = false;
                /* TODO: don't do power off in this file. */
                /* app_mmi_handle_action(MMI_DEV_POWER_OFF); */
            }
        }
        break;

    case SRC_TIMER_ID_AUTO_GAMING_MODE:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: auto_gaming_mode 0 timeout");
            if (!low_latency_mode)
            {
                src_spp_req_enter_ll(sink_dev_list[0].remote_info.bd_addr, 0);
            }
        }
        break;
    case SRC_TIMER_ID_AUTO_GAMING_MODE + 1:
        {
            APP_PRINT_INFO0("src_policy_timeout_cb: auto_gaming_mode 1 timeout");
            if (!low_latency_mode)
            {
                src_spp_req_enter_ll(sink_dev_list[1].remote_info.bd_addr, 0);
            }
        }
        break;

    default:
        break;
    }
}

/*
* subcommand : 0x23
* bd address(6 byte)
* afh policy mode (1 byte): BT = 0, 2.4G = 1
* afh policy priority (1 byte): remote first = 0, local first = 1
*/
static void src_send_link_info_to_lowerstack(uint8_t *addr)
{
    uint8_t params[9] = {0};

    params[0] = 0x23;

    memcpy(params + 1, addr, 6);
    params[7] = 1; /* 2.4g */
    params[8] = 0; /* remote first; due to dongle is src */

    APP_PRINT_TRACE1("src_send_link_info_to_lowerstack: %b", TRACE_BINARY(sizeof(params), params));

    gap_vendor_cmd_req(0xfd81, sizeof(params), params);
}

static void src_bt_mgr_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;
    T_BT_PARAM bt_param;
    switch (event_type)
    {

    case BT_EVENT_READY:
        {
            //check for bonded device to re-connect
            //app_mmi_handle_action(MMI_DEV_POWER_ON);
        }
        break;

    case BT_EVENT_INQUIRY_RESULT:
        {
            src_handle_inquiry_res(param->inquiry_result);
        }
        break;

    case BT_EVENT_INQUIRY_RSP:
        {
            src_handle_inquiry_rsp(param->inquiry_rsp.cause);
        }
        break;

    case BT_EVENT_INQUIRY_CMPL:
        {
            src_handle_inquiry_cmpl(param->inquiry_cmpl.cause);
        }
        break;

    case BT_EVENT_INQUIRY_CANCEL_RSP:
        {
            src_handle_inquiry_cancel_cmpl(param->inquiry_cancel_rsp.cause);
        }
        break;

    case BT_EVENT_ACL_CONN_IND:
        {
            src_handle_acl_conn_req(param->acl_conn_ind.bd_addr);
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            src_send_link_info_to_lowerstack(param->acl_conn_success.bd_addr);

            app_usb_uac_clear_headset_status();

#if F_APP_ASK_HEADSET_MAX_TX_POWER
            bt_link_rssi_golden_range_set(param->acl_conn_success.bd_addr, 10, 0);
#endif

            bt_param.bd_addr = param->acl_conn_success.bd_addr;
            bt_param.cause = 0;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {
            bt_param.bd_addr = param->acl_conn_fail.bd_addr;
            bt_param.cause = param->acl_conn_fail.cause;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            bt_param.bd_addr = param->acl_conn_disconn.bd_addr;
            bt_param.cause = param->acl_conn_disconn.cause;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            bt_param.bd_addr = param->acl_role_slave.bd_addr;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            bt_param.bd_addr = param->acl_conn_active.bd_addr;
            bt_param.cause = HCI_SUCCESS;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            bt_param.bd_addr = param->acl_authen_fail.bd_addr;
            bt_param.cause = param->acl_authen_fail.cause;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_LINK_KEY_INFO:
        {
            src_handle_link_key_info(param->link_key_info.bd_addr,
                                     param->link_key_info.link_key,
                                     param->link_key_info.key_type);
        }
        break;

    case BT_EVENT_LINK_KEY_REQ:
        {
            src_handle_link_key_req(param->link_key_req.bd_addr);
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            src_handle_sdp_attr_info(param->sdp_attr_info.bd_addr,
                                     param->sdp_attr_info.info);
        }
        break;

    case BT_EVENT_DID_ATTR_INFO:
        {
            src_handle_did_attr_info(param->did_attr_info);
        }
        break;

    case BT_EVENT_SDP_DISCOV_CMPL:
        {
            src_handle_sdp_cmpl(param->sdp_discov_cmpl.bd_addr,
                                param->sdp_discov_cmpl.cause);
        }
        break;

#ifdef A2DP_SRC_TX_FLOW_CONTROL
    case BT_EVENT_A2DP_STREAM_DATA_RSP:
        {
            uint8_t index = 0;

            if (src_policy_find_dev_index_by_addr(param->a2dp_stream_data_rsp.bd_addr,
                                                  &index))
            {
                sink_dev_list[index].a2dp_stream_credits++;
                if (sink_dev_list[index].a2dp_stream_credits > A2DP_STREAM_MAX_CREDITS)
                {
                    APP_PRINT_ERROR0("a2dp stream credits exceed");
                    sink_dev_list[index].a2dp_stream_credits = A2DP_STREAM_MAX_CREDITS;
                }
                /* APP_PRINT_INFO2("BT_EVENT_A2DP_STREAM_DATA_RSP: %b credits %u",
                 *                 TRACE_BDADDR(param->a2dp_stream_data_rsp.bd_addr),
                 *                 sink_dev_list[index].a2dp_stream_credits);
                 */
                handle = false; /* FIXME: Avoid too much log */
            }
            else
            {

                APP_PRINT_ERROR1("%b has been disconnected",
                                 TRACE_BDADDR(param->a2dp_stream_data_rsp.bd_addr));
            }
        }
        break;
#endif

    case BT_EVENT_A2DP_CONN_CMPL:
        {
            src_handle_profile_conn_cmpl(param->a2dp_conn_cmpl.bd_addr,
                                         A2DP_PROFILE_MASK);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
            adapter_dual_mode_link_mgr(ADAPTER_EVENT_A2DP_CONNECTED, 0);
#endif
        }
        break;

    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
            uint8_t index = 0;

            if (src_policy_find_dev_index_by_addr(param->a2dp_stream_open.bd_addr,
                                                  &index))
            {
                app_stop_timer(&timer_idx_src_a2dp_cfg[index]);
                app_stop_timer(&timer_idx_src_a2dp[index]);
            }
            src_handle_a2dp_config_cmpl(param->a2dp_config_cmpl);
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            /* FIXME */
            /* src_handle_a2dp_open(param->a2dp_stream_open.bd_addr,
             *                      param->a2dp_stream_open.remote_mtu);
             */
            src_handle_a2dp_open(param->a2dp_stream_open.bd_addr, 672);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            /* stream start cfm */
            src_handle_a2dp_stream_start(param->a2dp_stream_start_ind.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            src_handle_a2dp_stream_start(param->a2dp_stream_start_rsp.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            src_handle_a2dp_stream_stop(param->a2dp_stream_stop.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            src_handle_a2dp_stream_close(param->a2dp_stream_close.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            src_handle_profile_disconn_cmpl(param->a2dp_disconn_cmpl.bd_addr,
                                            param->a2dp_disconn_cmpl.cause,
                                            A2DP_PROFILE_MASK);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
            adapter_dual_mode_link_mgr(ADAPTER_EVENT_A2DP_DISCONNECTED, 0);
#endif

            app_usb_ds_pipe_release();
        }
        break;

    case BT_EVENT_AVRCP_CONN_CMPL:
        src_handle_profile_conn_cmpl(param->avrcp_conn_cmpl.bd_addr,
                                     AVRCP_PROFILE_MASK);
        break;
    case BT_EVENT_AVRCP_DISCONN_CMPL:
        src_handle_profile_disconn_cmpl(param->avrcp_disconn_cmpl.bd_addr,
                                        param->avrcp_disconn_cmpl.cause,
                                        AVRCP_PROFILE_MASK);
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED_REG_REQ:
        if (!src_is_streaming(param->avrcp_reg_play_status_changed.bd_addr))
            bt_avrcp_play_status_change_register_rsp(param->avrcp_reg_play_status_changed.bd_addr,
                                                     BT_AVRCP_PLAY_STATUS_PAUSED);
        else
            bt_avrcp_play_status_change_register_rsp(param->avrcp_reg_play_status_changed.bd_addr,
                                                     BT_AVRCP_PLAY_STATUS_PLAYING);

        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("src_bt_mgr_cback: event_type 0x%04x", event_type);
    }
}

void app_src_dongle_volume_balance_spp_command(uint8_t message_type)
{
    uint8_t i;
    T_APP_DEV *sink_dev = NULL;
    uint8_t data[SRC_SPP_HDR_SIZE + 4];
    T_GAMING_SYNC_HDR *hdr = (void *)data;

    APP_PRINT_INFO1("app_src_dongle_volume_balance: message_type %x ", message_type);

    hdr->sync = DONGLE_FORMAT_START_BIT;
    hdr->type = DONGLE_TYPE_CMD;
    hdr->pl   = 0x03;
    hdr->cmd  = DONGLE_CMD_SET_VOL_BALANCE;
    data[SRC_SPP_HDR_SIZE + 3] = DONGLE_FORMAT_STOP_BIT;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];

        if (sink_dev->dev_state != STATE_DEV_CONNECTED)
        {
            continue;
        }

        if (!profile_is_connected(sink_dev->remote_info.bd_addr,
                                  SPP_AUDIO_PROFILE_MASK))
        {
            continue;
        }

        switch (message_type)
        {
        case VOL_BALANCE_ACK:
            {
                APP_PRINT_INFO0("app_src_dongle_volume_balance: dongle ack ");
                data[SRC_SPP_HDR_SIZE] = VOL_BALANCE_ACK;
                data[SRC_SPP_HDR_SIZE + 1] = game_volume_pecent;
                data[SRC_SPP_HDR_SIZE + 2] = chat_volume_pecent;

                if (app_gaming_ctrl_send_fix_channel_data(sink_dev->remote_info.bd_addr, FIX_CHANNEL_CID, data,
                                                          sizeof(data),
                                                          false) == false)
                {
                    APP_PRINT_WARN0("app_src_dongle_volume_balance(): dongle ack error!");
                }
            }
            break;

        case VOL_BALANCE_INFORM:
            {
                APP_PRINT_INFO0("app_src_dongle_volume_balance: dongle inform "); //no need
            }
            break;
        }
    }
}

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
void app_src_dongle_lock_headset(bool lock)
{
    uint8_t i;
    T_APP_DEV *sink_dev = NULL;
    uint8_t data[SRC_SPP_HDR_SIZE + 2];
    T_GAMING_SYNC_HDR *hdr = (void *)data;
    APP_PRINT_INFO1("app_src_dongle_lock_headset: lock_cmd %x ", lock);
    hdr->sync = 'R';
    hdr->type = 0x01;
    hdr->pl   = 0x02;
    hdr->cmd  = 0x04;
    data[SRC_SPP_HDR_SIZE + 1] = 'T';
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];
        if (sink_dev->dev_state != STATE_DEV_CONNECTED)
        {
            continue;
        }
        if (!profile_is_connected(sink_dev->remote_info.bd_addr,
                                  SPP_AUDIO_PROFILE_MASK))
        {
            continue;
        }
        if (lock)
        {
            APP_PRINT_INFO0("app_src_dongle_lock_headset: send lock command ");
            data[SRC_SPP_HDR_SIZE] = 0x01;

            if (app_gaming_ctrl_send_fix_channel_data(sink_dev->remote_info.bd_addr, FIX_CHANNEL_CID, data,
                                                      sizeof(data),
                                                      false) == false)
            {
                APP_PRINT_WARN0("app_src_dongle_lock_headset: send lock cmd error!");
            }
        }
        else
        {
            APP_PRINT_INFO0("app_src_dongle_lock_headset: send unlock command ");
            data[SRC_SPP_HDR_SIZE] = 0x00;
            if (app_gaming_ctrl_send_fix_channel_data(sink_dev->remote_info.bd_addr, FIX_CHANNEL_CID, data,
                                                      sizeof(data),
                                                      false) == false)
            {
                APP_PRINT_WARN0("app_src_dongle_lock_headset(): send unclock cmd error!");
            }
        }
    }
}
#endif

static void src_device_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("src_device_dm_cback: event_type 0x%04x", event_type);
    }
}

RAM_TEXT_SECTION
bool gaming_a2dp_is_force_suspend(void)
{
    if (force_suspend_resume_flag)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool gaming_a2dp_codec_is_lc3(void)
{
    if (support_a2dp_codec_lc3)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void gaming_set_cfg(uint8_t type, uint8_t *value, uint8_t len)
{
    if (!value || !len)
    {
        return;
    }

    switch (type)
    {
    case GAMING_CFG_LOW_LATENCY:
        break;
    case GAMING_CFG_AUTOPAIR_ENABLE:
        break;
    case GAMING_CFG_AUTOPAIR_GENERAL:
        break;
    case GAMING_CFG_AUTOPAIR_TIMEOUT:
        break;
    case GAMING_CFG_AUTOPAIR_RSSI:
        break;
    case GAMING_CFG_UAC2:
        {
            T_AUDIO_FORMAT_INFO iofmt;

            src_general_cfg.uac2 = *value;
            app_src_init_sbcenc2(&iofmt);
            src_set_codec_fmt(SRC_CODEC_SBCENC2, &iofmt);
        }
        break;
    default:
        APP_PRINT_ERROR1("gaming_set_cfg: Unrecognized config type %u", type);
        break;
    }
}

int gaming_bt_reg_cback(gaming_bt_cback_t cback)
{
    if (bt_uapi_cback)
    {
        APP_PRINT_ERROR0("gaming_bt_reg_cback: Callback already existed");
        return -1;
    }
    bt_uapi_cback = cback;
    return 0;
}

int gaming_bt_get_bond_bdaddr(uint8_t id, uint8_t *bdaddr)
{
    if (id >= SINK_DEV_NUM_MAX)
    {
        return -1;
    }

    if (app_src_legacy_get_bond_addr_by_index(id, bdaddr))
    {
        return 0;
    }
    else
    {
        return -2;
    }
}

int gaming_bt_connect(uint8_t id, uint8_t *bdaddr)
{
    T_APP_DEV *dev;
    uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

    if (shutdown)
    {
        APP_PRINT_ERROR2("gaming_bt_connect: host is down, dev %u (%b)", id,
                         TRACE_BDADDR(bdaddr));
        return -1;
    }

    if (id >= SINK_DEV_NUM_MAX)
    {
        return -2;
    }

    if (!bdaddr)
    {
        return -3;
    }

    dev = &sink_dev_list[id];

    APP_PRINT_INFO4("gaming_bt_connect: dev %u (%b), dev_state %04x, bt_state %04x",
                    id, TRACE_BDADDR(bdaddr), dev->dev_state, dev->bt_state);

    switch (dev->dev_state)
    {
    case STATE_DEV_IDLE:
        if (!memcmp(bdaddr, ba_any, 6))
        {
            /* Connect to any found device. */
            src_enter_connecting_state(id);
            if (!src_bt_is_busy())
            {
                src_start_conn(id);
            }
            return 0;
        }

        if (src_dev_is_bonded(id))
        {
            uint8_t ba[6] = { 0, 0, 0, 0, 0, 0 };

            /* The result is definitely true because of the protection from the
             * above condition.
             * */
            app_src_legacy_get_bond_addr_by_index(id, ba);

            if (!memcmp(bdaddr, ba, 6))
            {
                src_enter_reconnecting_state(id);
                /* Maybe remote is connecting to us */
                if (!src_bt_is_busy())
                {
                    src_start_reconn(id);
                }
                return 0;
            }

            if (!app_src_legacy_delete_bond_by_index(id))
            {
                APP_PRINT_ERROR1("gaming_bt_connect: rm bond for dev %u err", id);
            }
        }

        src_enter_connecting_state(id);
        memcpy(dev->remote_info.gaming_bdaddr, bdaddr, 6);
        if (!src_bt_is_busy())
        {
            src_start_conn(id);
        }
        break;

    case STATE_DEV_RECONNECTING:
        APP_PRINT_WARN0("gaming_bt_connect: reconnecting in progress");
        break;

    case STATE_DEV_CONNECTING:
        APP_PRINT_WARN0("gaming_bt_connect: connecting in progress");
        break;

    case STATE_DEV_CLEANUP:
        APP_PRINT_WARN1("gaming_bt_connect: cleaning up in progress, next event %u",
                        dev->pending_event);
        break;

    case STATE_DEV_CONNECTED:
        APP_PRINT_WARN2("gaming_bt_connect: dev %u (%b) is already connected",
                        id, TRACE_BDADDR(dev->remote_info.bd_addr));
        break;

    default:
        break;
    }

    return 0;
}

int gaming_bt_disconnect_by_bdaddr(uint8_t *bdaddr)
{
    uint8_t i;

    if (!bdaddr)
    {
        return -1;
    }

    if (shutdown)
    {
        APP_PRINT_ERROR1("gaming_bt_disconnect_by_bdaddr: host is down, dev %b",
                         TRACE_BDADDR(bdaddr));
        return -2;
    }

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (!memcmp(sink_dev_list[i].remote_info.bd_addr, bdaddr, 6))
        {
            break;
        }
    }

    if (i >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("gaming_bt_disconnect_by_bdaddr: No id for dev %b found",
                         TRACE_BDADDR(bdaddr));
        return -3;
    }

    src_dev_cleanup(i, KEY_NONE);

    return 0;
}

int gaming_bt_disconnect_by_id(uint8_t id)
{
    if (shutdown)
    {
        APP_PRINT_ERROR1("gaming_bt_disconnect_by_id: host is down dev %u", id);
        return -1;
    }

    if (id >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("gaming_bt_disconnect_by_id: Invaild id %u", id);
        return -2;
    }

    src_dev_cleanup(id, KEY_NONE);

    return 0;
}
void gaming_bt_disconnect_all_link(void)
{
		uint8_t i;
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        src_dev_cleanup(i, KEY_NONE);
    }	
}

void gaming_bt_remove_bond_by_bdaddr(uint8_t *bdaddr)
{
    if (shutdown)
    {
        APP_PRINT_ERROR1("gaming_bt_remove_bond_by_bdaddr: host is down, dev %b",
                         TRACE_BDADDR(bdaddr));
        return;
    }

    if (!app_src_legacy_delete_bond_by_addr(bdaddr))
        APP_PRINT_ERROR1("gaming_bt_remove_bond_by_bdaddr: rm bond err, dev %b",
                         TRACE_BDADDR(bdaddr));

    return;
}

void gaming_bt_remove_bond_by_id(uint8_t id)
{
    if (shutdown)
    {
        APP_PRINT_ERROR1("gaming_bt_remove_bond_by_id: host is down, id %u", id);
        return;
    }

    if (id >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("gaming_bt_remove_bond_by_id: Invalid id %u", id);
        return;
    }

    if (!app_src_legacy_delete_bond_by_index(id))
        APP_PRINT_ERROR1("gaming_bt_remove_bond_by_id: rm bond for dev %u err",
                         id);

    return;
}

int gaming_bt_start_discovery(void)
{
    uint8_t i;

    if (shutdown)
    {
        APP_PRINT_ERROR0("gaming_bt_start_discovery: host is down");
        return -1;
    }

    if (client_discovering)
    {
        APP_PRINT_ERROR0("gaming_bt_start_discovery: It is discovering");
        return -2;
    }

    switch (discov_state)
    {
    case DISCOVERY_STARTING:
        client_discovering = 1;
        /* src_handle_inquiry_rsp() will send STARTED event to upper layer. */
        return 0;
    case DISCOVERY_FINDING:
        client_discovering = 1;
        return 0;
    case DISCOVERY_STOPPING:
        /* If it is stopping, src_handle_inquiry_cancel_cmpl() will start
         * inquiry again. And src_handle_inquiry_rsp() will send STARTED event
         * to upper layer.
         * */
        client_discovering = 1;
        return 0;
    case DISCOVERY_STOPPED:
        break;
    default:
        APP_PRINT_ERROR1("gaming_bt_start_discovery: Invalid discov state %u",
                         discov_state);
        return -3;
    }

    /* Here, discov_state is definitely DISCOVERY_STOPPED. */

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].bt_state == STATE_BT_PAGING)
        {
            APP_PRINT_ERROR2("gaming_bt_start_discovery: Connecting to %u (%b)",
                             i, TRACE_BDADDR(sink_dev_list[i].remote_info.bd_addr));
            client_discovering = 0;
            return -4;
        }
        else if (sink_dev_list[i].bt_state == STATE_BT_INQUIRY)
        {
            /* FIXME: Is it possible that local dev is inquiring? */
            client_discovering = 1;
            APP_PRINT_ERROR0("gaming_bt_start_discovery: It is inquiring");
            /* Later, inquiry rsp is received, and this state will be set to
             * DISCOVERY_FINDING.
             * */
            return 0;
        }
    }

    discov_state = DISCOVERY_STARTING;

    if (gap_br_start_inquiry(false, 10))
    {
        APP_PRINT_ERROR0("gaming_bt_start_discovery: Start inquiry err");
        discov_state = DISCOVERY_STOPPED;
        client_discovering = 0;
        return -5;
    }

    client_discovering = 1;

    if (bt_uapi_cback)
    {
        bt_uapi_cback(EVENT_GAMING_DISCOVERY_STARTING, NULL, 0);
    }

    return 0;
}

int gaming_bt_stop_discovery(void)
{
    uint8_t i;
    uint8_t pre_state;
    uint8_t pre_client_state;

    if (shutdown)
    {
        APP_PRINT_ERROR0("gaming_bt_stop_discovery: host is down");
        return -1;
    }

    if (!client_discovering)
    {
        APP_PRINT_ERROR0("gaming_bt_stop_discovery: Discovery already stopped");
        return -2;
    }

    pre_client_state = client_discovering;
    client_discovering = 0;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].bt_state == STATE_BT_INQUIRY)
        {
            APP_PRINT_INFO1("gaming_bt_stop_discovery: inquiring for conn dev %u", i);
            /* Discovery will be stopped because of false client_discovering
             * */
            return 0;
        }
    }

    switch (discov_state)
    {
    case DISCOVERY_STARTING:
    case DISCOVERY_FINDING:
        pre_state = discov_state;
        discov_state = DISCOVERY_STOPPING;

        if (gap_br_stop_inquiry())
        {
            APP_PRINT_ERROR0("gaming_bt_stop_discovery: Stop inquiry err");
            discov_state = pre_state;
            client_discovering = pre_client_state;
            return -4;
        }
        if (bt_uapi_cback)
        {
            bt_uapi_cback(EVENT_GAMING_DISCOVERY_STOPPING, NULL, 0);
        }
        return 0;

    case DISCOVERY_STOPPING:
        return 0;
    case DISCOVERY_STOPPED:
        return 0;
    default:
        APP_PRINT_ERROR1("gaming_bt_stop_discovery: Unknown discov state %u",
                         discov_state);
        client_discovering = pre_client_state;
        return -3;
    }
}

bool gaming_bt_is_connected(void)
{
    uint8_t i = 0;
    bool connected = false;
    T_APP_DEV *sink_dev = NULL;
    T_APP_BR_LINK *p_link = NULL;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];
        p_link = app_find_br_link(sink_dev->remote_info.bd_addr);
        if (!p_link)
        {
            continue;
        }
        if (sink_dev->dev_state == STATE_DEV_CONNECTED &&
            (p_link->connected_profile & A2DP_PROFILE_MASK) &&
            sink_dev->a2dp_opened)
        {
            connected = true;
            break;
        }
    }

    return connected;
}

bool gaming_bt_is_idle(uint8_t id)
{
    if (id >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("gaming_bt_is_idle: Incorrect id %u", id);
        return true;
    }

    switch (sink_dev_list[id].dev_state)
    {
    case STATE_DEV_CONNECTED:
    case STATE_DEV_CONNECTING:
    case STATE_DEV_RECONNECTING:
    case STATE_DEV_CLEANUP:
        return false;
    default:
        return true;
    }
}

void gaming_bt_profile_conn_cmpl(uint8_t *bd_addr, uint32_t profile_mask)
{
    src_handle_profile_conn_cmpl(bd_addr, profile_mask);
}

void gaming_bt_profile_disconn_cmpl(uint8_t *bd_addr, uint16_t cause,
                                    uint32_t profile_mask)
{
    src_handle_profile_disconn_cmpl(bd_addr, cause, profile_mask);
}

int gaming_bt_enable(void)
{
    shutdown = false;

    src_auto_link_cfg.single_link = app_cfg_const.dongle_link_num_max;
    src_auto_link_cfg.autopair = app_cfg_const.dongle_autopair_support;
    src_auto_link_cfg.autopair_timeout = app_cfg_const.dongle_autopair_timeout;
    src_auto_link_cfg.autopair_general = app_cfg_const.dongle_autopair_general_support;
    src_auto_link_cfg.autopair_rssi = app_cfg_const.dongle_autopair_rssi;
    src_general_cfg.sbc_frames_per_pkt = 0;
    src_general_cfg.pairing_timeout = 0;
    src_general_cfg.linkback_timeout = 0;
    src_general_cfg.auto_power_off_timeout = 0;
    src_general_cfg.auto_gaming = 1;

    /* TODO: Handle autopair and single link configs. */
    if (src_auto_link_cfg.autopair && (!app_cfg_const.enable_dongle_dual_mode))
    {
        app_src_enable_auto_linking();
    }

    return 0;
}

void gaming_bt_disable(void)
{
    uint8_t i = 0;

    /* Set correct LED state first. */
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        src_led_set_idle(i);
    }

    src_poweroff_cleanup();
}

void gaming_set_bt_mode(T_BT_DEVICE_MODE mode)
{
    APP_PRINT_INFO1("gaming_set_bt_mode: mode %d ", mode);

    src_set_bt_mode(mode);
}

int gaming_bt_force_connect(uint8_t id, uint8_t *bdaddr)
{
    uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

    if (id >= SINK_DEV_NUM_MAX)
    {
        return -2;
    }

    if (!bdaddr)
    {
        return -3;
    }

    APP_PRINT_INFO2("gaming_bt_force_connect: dev %u (%b)", id, TRACE_BDADDR(bdaddr));

    if (!memcmp(bdaddr, ba_any, 6))
    {
        /* Connect to any found device. */
        src_enter_connecting_state(id);
        if (!src_bt_is_busy())
        {
            src_start_conn(id);
        }
    }

    return 0;
}

bool gaming_bt_have_saved_id_bond_record(void)
{
    uint8_t bdaddr[6];

    for (uint8_t i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (gaming_bt_get_bond_bdaddr(i, bdaddr) == 0
            && memcmp(app_cfg_nv.saved_id, bdaddr, 3) == 0)
        {
            /* Dongle has a bud link key record and last 3 byte of bud bdaddr
               are the same as the saved id */
            return true;
        }
    }
    return false;
}

void gaming_bt_set_scan_by_mmi(bool flag)
{
    scan_by_mmi = flag;

    APP_PRINT_TRACE1("gaming_bt_set_scan_by_mmi: scan_by_mmi %d", scan_by_mmi);
}

bool gaming_bt_get_scan_by_mmi(void)
{
    return scan_by_mmi;
}

void gaming_handle_ext_adv_report_info(uint8_t cb_type, void *result)
{
    T_LE_EXT_ADV_REPORT_INFO *ext_adv_report_info = (T_LE_EXT_ADV_REPORT_INFO *)result;
    uint8_t index = 0;
    T_APP_DEV *sink_dev;
    uint8_t ret = 0;

    if (src_get_inquiry_dev_index(&index))
    {
        sink_dev = &sink_dev_list[index];

        if ((!sink_dev->inquiry_dev_found) &&
            (sink_dev->dev_state == STATE_DEV_CONNECTING) &&
            ext_adv_report_info->data_len == 31)
        {
            T_LEGACY_DONGLE_BLE_DATA dongle_ble_data;
            uint8_t dongle_pairing_uuid_1 = (app_cfg_const.dongle_pairing_uuid >> 8) & 0xFF;
            uint8_t dongle_pairing_uuid_2 = app_cfg_const.dongle_pairing_uuid & 0xFF;

            memcpy(&dongle_ble_data, ext_adv_report_info->p_data, sizeof(T_LEGACY_DONGLE_BLE_DATA));

            bool same_id = !memcmp(app_cfg_nv.saved_id, ext_adv_report_info->bd_addr, 3);
            bool same_pairing_id = (dongle_ble_data.eir_data.pairing_id_1 == dongle_pairing_uuid_1
                                    && dongle_ble_data.eir_data.pairing_id_2 == dongle_pairing_uuid_2);
#if F_APP_LEGACY_DONGLE_BINDING
            bool pair_new_bud = (scan_by_mmi && dongle_ble_data.pairing_bit);
            bool pair_exist_bud = (!scan_by_mmi && !dongle_ble_data.pairing_bit && same_id);

            if (same_pairing_id && (ext_adv_report_info->rssi >= app_cfg_const.dongle_autopair_rssi) &&
                (pair_new_bud || pair_exist_bud))
#else
            if (((scan_by_mmi && dongle_ble_data.pairing_bit) || (!scan_by_mmi && same_id))
                && same_pairing_id
                && ext_adv_report_info->rssi >= app_cfg_const.dongle_autopair_rssi)
#endif
            {
                if (app_find_br_link(ext_adv_report_info->bd_addr) == NULL)
                {
                    //uint8_t pre_state;
                    uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

                    /* This dev belongs to another link, don't touch it */
                    if (src_already_connecting(index, ext_adv_report_info->bd_addr))
                    {
                        ret = 1;
                        goto done;
                    }

                    if (!memcmp(ba_any, ext_adv_report_info->bd_addr, 6))
                    {
                        APP_PRINT_ERROR0("gaming_handle_ext_adv_report_info: empty dev bdaddr");
                        ret = 2;
                        goto done;
                    }

                    if (memcmp(sink_dev->remote_info.gaming_bdaddr, ba_any, 6))
                    {
                        if (memcmp(sink_dev->remote_info.gaming_bdaddr,
                                   ext_adv_report_info->bd_addr, 6))
                        {
                            ret = 3;
                            goto done;
                        }

                        /* gaming_bdaddr is same as remote dev bdaddr. */
                    }

                    src_eir_parse(sink_dev, (uint8_t *)&dongle_ble_data.eir_data,
                                  dongle_ble_data.eir_data.length + 1);

                    /* Autopairing, autopairing private headset, and found
                     * headset.
                     */
                    if (autopairing && !src_auto_link_cfg.autopair_general)
                    {
                        if (!sink_dev->headset_info_valid)
                        {
                            sink_dev->headset_info_valid = false;
                            memset(sink_dev->headset_info, 0,
                                   sizeof(sink_dev->headset_info));
                            ret = 4;
                            goto done;
                        }
                        if (ext_adv_report_info->rssi < src_auto_link_cfg.autopair_rssi)
                        {
                            sink_dev->headset_info_valid = false;
                            memset(sink_dev->headset_info, 0,
                                   sizeof(sink_dev->headset_info));
                            ret = 5;
                            goto done;
                        }
                    }

                    memcpy(sink_dev->remote_info.bd_addr, ext_adv_report_info->bd_addr, 6);
                    sink_dev->inquiry_dev_found = true;

                    discov_state = DISCOVERY_STOPPED;
                    if (bt_uapi_cback)
                    {
                        bt_uapi_cback(EVENT_GAMING_DISCOVERY_STOPPED, NULL, 0);
                    }

                    if (!same_id)
                    {
                        app_device_save_pairing_id(ext_adv_report_info->bd_addr, 3);
                    }

                    src_dev_conn(index, sink_dev->remote_info.bd_addr,
                                 A2DP_PROFILE_MASK |
                                 SPP_AUDIO_PROFILE_MASK |
                                 AVRCP_PROFILE_MASK);
                }
                else
                {
                    APP_PRINT_TRACE1("gaming_handle_ext_adv_report_info: %s already connected",
                                     TRACE_BDADDR(ext_adv_report_info->bd_addr));
                }
            }
            else
            {
                if (same_id || same_pairing_id)
                {
                    APP_PRINT_TRACE6("gaming_handle_ext_adv_report_info: saved_id %b, scan_id %b, dongle_pairing_uuid 0x%04x, remote_pairing_uuid 0x%04x, pairing_bit %d, scan_by_mmi %d",
                                     TRACE_BINARY(3, app_cfg_nv.saved_id),
                                     TRACE_BINARY(3, ext_adv_report_info->bd_addr),
                                     app_cfg_const.dongle_pairing_uuid,
                                     dongle_ble_data.eir_data.pairing_id_1 << 8 | dongle_ble_data.eir_data.pairing_id_2,
                                     dongle_ble_data.pairing_bit,
                                     scan_by_mmi);
                }
            }
        }
    }

done:
    if (ret)
    {
        APP_PRINT_TRACE1("gaming_handle_ext_adv_report_info: ret %d", ret);
    }
    return;
}

bool gaming_bt_low_latency_streaming(void)
{
    bool ret = false;

    if (low_latency_mode && (src_get_stream_dev_num() > 0))
    {
        ret = true;
    }

    return ret;
}

uint16_t gaming_sync_app_seq_number(uint8_t index)
{
    return sink_dev_list[index].a2dp_ll_seq;
}

RAM_TEXT_SECTION
bool gaming_current_codec_is_lc3(void)
{
    if (support_a2dp_codec_lc3)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void app_src_policy_init(void)
{
    T_AUDIO_FORMAT_INFO iofmt;

    APP_PRINT_INFO0("app_src_policy_init(): ++");

    shutdown = true;

    app_src_init_sbcenc(&iofmt);
    src_set_codec_fmt(SRC_CODEC_SBCENC,  &iofmt);
    app_src_init_sbcdec(&iofmt);
    src_set_codec_fmt(SRC_CODEC_SBCDEC, &iofmt);
    if (src_general_cfg.uac2)
    {
        app_src_init_sbcenc2(&iofmt);
        src_set_codec_fmt(SRC_CODEC_SBCENC2, &iofmt);
    }

    /* FIXME: We have moved poweron related code to app_device.c */

    /* app_power_on_hook = src_handle_poweron; */
    memset(sink_dev_list, 0, sizeof(sink_dev_list));
    current_index = 0;
    bt_mgr_cback_register(src_bt_mgr_cback);
    sys_mgr_cback_register(src_device_dm_cback);

    app_timer_reg_cb(src_policy_timeout_cb, &src_policy_timer_id);
}

void app_src_policy_uac_register(SRC_POLICY_UAC_CB_F *p_func)
{
    if (p_func)
    {
        src_policy_uac_cb.up_stream_data_write = p_func->up_stream_data_write;
        src_policy_uac_cb.up_stream_get_data_len = p_func->up_stream_get_data_len;
    }
}

#if TARGET_LEGACY_GAMING_DONGLE
static bool app_src_sync_pkt_send_cb(uint8_t *addr, uint8_t *data, uint16_t len, bool flushable)
{
    return app_gaming_ctrl_send_fix_channel_data(addr, FIX_CHANNEL_CID, data, len, flushable);
}
#endif

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
static void app_src_fix_chann_cb(T_GAP_BR_FIX_CHANN_MSG msg, void *p_buf)
{
    switch (msg)
    {
    case GAP_BR_FIX_CHANN_DATA_IND:
        {
            T_GAP_BR_FIX_CHANN_DATA_IND *tmp = p_buf;

            if (tmp != NULL)
            {
                app_gaming_sync_disassemble_data(tmp->bd_addr, tmp->p_data, tmp->length);
            }
        }
        break;

    default:
        break;
    }

}
#endif

void app_src_policy_uac_unregister(void)
{
    memset(&src_policy_uac_cb, 0, sizeof(src_policy_uac_cb));
}

int gaming_bt_init(void)
{
    app_spp_audio_init();
#ifdef SPP_SBCDEC_SIMULATOR
    dummy_data = os_mem_zalloc(RAM_TYPE_DSPSHARE, 1024);
    if (!dummy_data)
    {
        APP_PRINT_ERROR0("gaming_bt_init: Alloc dummy data err");
    }
#endif
    app_src_policy_init();

#if TARGET_LEGACY_GAMING_DONGLE
    app_gaming_sync_legacy_send_register(app_src_sync_pkt_send_cb);
    app_gaming_sync_rcv_register(app_gaming_ctrl_data_rcv);
    app_gaming_sync_legacy_cmd_register(src_spp_audio_packet_received);
#endif

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    gap_br_reg_fix_chann(FIX_CHANNEL_CID);
    gap_br_reg_fix_chann_cb(app_src_fix_chann_cb);
#endif

    us_process_init();

    return 0;
}
#endif
