#if F_APP_HEARABLE_SUPPORT
#include <stdbool.h>
#include <string.h>
#include "stdlib.h"
#include "math.h"
#include "ftl.h"
#include "fmc_api.h"
#include "bt_hfp.h"
#include "audio.h"
#include "audio_probe.h"
#include "audio_passthrough.h"
#include "app_anc.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_timer.h"
#include "app_cmd.h"
#include "app_sensor.h"
#include "app_hearable.h"
#include "app_a2dp.h"
#include "ha_ext_ftl.h"
#include "app_audio_passthrough.h"
#include "app_audio_policy.h"
#include "app_audio_hearing.h"
#include "pm.h"

#define HA_BLOCK                            256
#define HA_PROG_OPTION                      4
#define HA_PROG_MAX_NUM                     16
#define HA_PROG_LIST_HEADER_LEN             36
#define HA_PROG_NUM                         4
#define HA_PROG_MAX_TONE_NUM                10
#define HA_VERSION                          8

#define HA_EFFECT_TO_PROG_OBJ(effect)       (effect + HA_PROG_OBJ_WDRC)
#define HA_PROG_OBJ_TO_EFFECT(obj)          (T_APP_HA_EFFECT)(obj - HA_PROG_OBJ_WDRC)
#define HA_PROG_FEATURE_NUM                 14
#define HA_GLOBAL_FEATURE_NUM               2
#define HA_PARAMS_FEATURE_NUM               3

#define HA_PARAMS_RNS_SIZE                  260
#define HA_PARAMS_SPEECH_ENHANCE_MAX_SIZE   1024
#define HA_PARAMS_MIC_SETTING_MAX_SIZE      1024
#define HA_PARAMS_SPK_COMPENSATION_MAX_SIZE 1024
#define HA_PARAMS_MIC_COMPENSATION_MAX_SIZE 1024
#define HA_PARAMS_MMI_VOLUME_STEP_SIZE      20
#define HA_PARAMS_WDRC_SIZE                 872
#define HA_PARAMS_WDRC_ENABLED_SIZE         4
#define HA_PARAMS_HA_TEST_SIZE              4

#define HA_GLOBAL_OBJ_WDRC_SIZE                 432
#define HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS_SIZE   16

#define HA_DSP_EXT_PARA_OFFSET              0x24
#define HA_DSP_FEATURE_BITS_OFFSET          0x2C
#define HA_DSP_EXT_PARA_SYNC_WORD           0xAA55
#define HA_DSP_FEATURE_HA_PROG              0x01
#define HA_DSP_FEATURE_HA_SPK_RESPONSE      0x02
#define HA_DSP_FEATURE_HA_DSP_CFG_GAIN      0x03
#define HA_DSP_FEATURE_HA_RNS_PARAMS        0x04
#define HA_DSP_FEATURE_HA_SPK_COMPENSATION  0x05
#define HA_DSP_FEATURE_HA_MIC_COMPENSATION  0x06
#define HA_DSP_FEATURE_HA_RESERVED          0x07
#define HA_DSP_FEATURE_HA_SPEECH_ENHANCE    0x08
#define HA_DSP_FEATURE_HA_MMI_VOLUME_STEP   0x09
#define HA_DSP_FEATURE_HA_WDRC_PARAMS       0x0A
#define HA_DSP_FEATURE_HA_WDRC_ENABLED      0x0B
#define HA_DSP_FEATURE_HA_TEST              0x0C
#define HA_DSP_FEATURE_HA_MIC_SETTING       0x0D
#define HA_DSP_FEATURE_DEFAULT              0xFF

#define HA_LISTENING_DELAY_TIME             2000
#define HA_AUDIO_VOLUME_REFRESH_INTERVAL    100

#define HA_INFO_PRIMARY                     0
#define HA_INFO_SECONDARY                   1

#define HA_SCALE_UP_GAIN_DB                 64

#define HA_HEARING_TEST_SAMPLE_RATE             48000
#define HA_HEARING_TEST_SAMPLE_DEPTH            24
#define HA_HEARING_TEST_SAMPLE_DEPTH_IN_BYTES   3
#define HA_HEARING_TEST_SAMPLE_DEPTH_MAX_VAL    8388607 //pow(2,24-1)-1
#define HA_HEARING_TEST_FRAME_LEN               960
#define HA_HEARING_TEST_FRAME_SEND_DELAY_TIME   15
#define HA_HEARING_TEST_TONE_TABLE_MAX_LEN      384 //125Hz, 48*8
#define HA_HEARING_TEST_TONE_PHASE              4 //fade in, duration, fade_out, interval
#define HA_HEARING_TEST_TONE_FADE_IN_OUT_MS     40
#define HA_HEARING_TEST_TONE_DURATION_MS        216
#define HA_HEARING_TEST_TONE_INTERVAL_MS        204
#define HA_HEARING_TEST_TONE_DEFAULT_FREQ       1000
#define HA_HEARING_TEST_TONE_DEFAULT_VOLUME     15
#define HA_HEARING_TEST_TONE_DEFAULT_GAIN       -70
#define HA_HEARING_TEST_TONE_MAX_GAIN           -40

typedef struct t_app_ha_dsp_ext_para
{
    uint32_t spk_channel                    : 1;
    uint32_t mic_channel                    : 1;
    uint32_t i2s_ext                        : 1;
    uint32_t spdif_enb                      : 1;
    uint32_t apt_enb                        : 1;
    uint32_t sound_effect                   : 1;
    uint32_t dac_output_mode                : 1;
    uint32_t sdk                            : 1;
    uint32_t opus_celt_encoder3             : 1;
    uint32_t record_mode_voice_parameter    : 1;
    uint32_t ha_enable                      : 1;
    uint32_t ha_scenario                    : 1;
    uint32_t resvbit                        : 20;
} T_APP_HA_DSP_EXT_PARA;

typedef enum
{
    APP_TIMER_HA_LISTENING_DELAY,
    APP_TIMER_HA_APT_VOL_LEVEL,
    APP_TIMER_HA_HEARING_TEST_TONE,
} T_HA_TIMER;

typedef enum
{
    APP_HA_CMD_MSG_SET_EFFECT_CMD       = 0x00,
    APP_HA_DATA_MSG_SET_PARAMS          = 0x01,
    APP_HA_HEARING_TEST_MSG_SET         = 0x02,
    APP_HA_GLOBAL_OBJ_MSG_SET           = 0X03,

    APP_HA_CMD_MSG_TOTAL,
} T_APP_HA_CMD_MSG;

typedef enum
{
    HA_EFFECT_WDRC          = 0x00,
    HA_EFFECT_NR            = 0x01,
    HA_EFFECT_GRAPHIC_EQ    = 0x02,
    HA_EFFECT_FBC           = 0x03,
    HA_EFFECT_OVP           = 0x04,
    HA_EFFECT_BF            = 0x05,
    HA_EFFECT_WNR           = 0x06,
    HA_EFFECT_INR           = 0x07,
    HA_EFFECT_RNS           = 0x08,
    HA_EFFECT_OUTPUT_DRC    = 0x09,

    HA_EFFECT_TOTAL         = 0x0A,
} T_APP_HA_EFFECT;

typedef enum
{
    HA_EFFECT_WDRC_SIZE         = 432,
    HA_EFFECT_NR_SIZE           = 12,
    HA_EFFECT_GRAPHIC_EQ_SIZE   = 140,
    HA_EFFECT_FBC_SIZE          = 20,
    HA_EFFECT_OVP_SIZE          = 12,
    HA_EFFECT_BF_SIZE           = 12,
    HA_EFFECT_WNR_SIZE          = 12,
    HA_EFFECT_INR_SIZE          = 12,
    HA_EFFECT_RNS_SIZE          = 12,
    HA_EFFECT_OUTPUT_DRC_SIZE   = 24,
} T_APP_HA_EFFECT_SIZE;

typedef enum
{
    HA_PARAMS_RNS               = 0x00,
    HA_PARAMS_SPEECH_ENHANCE    = 0x01,
    HA_PARAMS_MMI_VOLUME_STEP   = 0x02,
    HA_PARAMS_WDRC              = 0x03,
    HA_PARAMS_WDRC_ENABLED      = 0x04,
    HA_PARAMS_HA_TEST           = 0x05,
    HA_PARAMS_MIC_SETTING       = 0X06,
    HA_PARAMS_SPK_COMPENSATION  = 0X07,
    HA_PARAMS_MIC_COMPENSATION  = 0X08,

    HA_PARAMS_TOTAL             = 0x09,
} T_APP_HA_PARAMS;

typedef struct t_app_ha_effect_item
{
    struct t_app_ha_effect_item     *p_next;
    T_APP_HA_EFFECT                 effect;
    void                            *info_buf;
    uint16_t                        info_len;
    bool                            enabled;
} T_APP_HA_EFFECT_ITEM;

typedef enum
{
    APP_REMOTE_MSG_HA_PROGRAM_SET_ID                                = 0x00,
    APP_REMOTE_MSG_HA_PROGRAM_SET_OBJ                               = 0x01,
    APP_REMOTE_MSG_HA_PROGRAM_SET_ALL_OBJ                           = 0x02,
    APP_REMOTE_MSG_HA_PROGRAM_GET_SPK_RESPONSE                      = 0x03,
    APP_REMOTE_MSG_HA_PROGRAM_RELAY_SPK_RESPONSE                    = 0x04,
    APP_REMOTE_MSG_HA_PROGRAM_GET_AUDIO_VOLUME                      = 0x05,
    APP_REMOTE_MSG_HA_PROGRAM_RELAY_AUDIO_VOLUME                    = 0x06,
    APP_REMOTE_MSG_HA_PROGRAM_RESET_OBJ                             = 0x07,
    APP_REMOTE_MSG_HA_PROGRAM_GET_ALL_DIRECTION_OBJ                 = 0x08,
    APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOLUME                         = 0x09,
    APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOLUME_SYNC_STATUS             = 0x0A,
    APP_REMOTE_MSG_HA_PROGRAM_SET_HA_NR                             = 0x0B,
    APP_REMOTE_MSG_HA_PROGRAM_SET_HA_OVP                            = 0x0C,
    APP_REMOTE_MSG_HA_PROGRAM_GET_HA_VOL                            = 0x0D,
    APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOL                            = 0x0E,
    APP_REMOTE_MSG_HA_PROGRAM_GET_HA_BF                             = 0x0F,
    APP_REMOTE_MSG_HA_PROGRAM_SET_HA_BF                             = 0x10,
    APP_REMOTE_MSG_HA_PROGRAM_STOP_A2DP_AUDIO_TRACK                 = 0x11,
    APP_REMOTE_MSG_HA_PROGRAM_DISABLE_AUDIO_EQ_START_HEARING_TEST   = 0X12,
    APP_REMOTE_MSG_HA_PROGRAM_RELAY_HEARING_TEST_STATUS             = 0X13,
    APP_REMOTE_MSG_HA_PROGRAM_RELAY_STOP_HEARING_TEST               = 0X14,
    APP_REMOTE_MSG_HA_PROGRAM_RELAY_HA_VOLUME_STATUS                = 0X15,
    APP_REMOTE_MSG_HA_TOTAL                                         = 0x16,
} T_HA_REMOTE_MSG;

typedef enum
{
    HA_PROG_OBJ_SUSTAIN         = 0x00,
    HA_PROG_OBJ_SELECTABLE      = 0x01,
    HA_PROG_OBJ_NAME            = 0x02,
    HA_PROG_OBJ_TOOL_DATA       = 0x03,
    HA_PROG_OBJ_VOLUME_LEVEL    = 0x04,
    HA_PROG_OBJ_CTRL_TOTAL      = 0x05,

    HA_PROG_OBJ_WDRC            = 0x20,
    HA_PROG_OBJ_NR              = 0x21,
    HA_PROG_OBJ_GRAPHIC_EQ      = 0x22,
    HA_PROG_OBJ_FBC             = 0x23,
    HA_PROG_OBJ_OVP             = 0x24,
    HA_PROG_OBJ_BF              = 0X25,
    HA_PROG_OBJ_WNR             = 0X26,
    HA_PROG_OBJ_INR             = 0X27,
    HA_PROG_OBJ_RNS             = 0X28,
    HA_PROG_OBJ_OUTPUT_DRC      = 0X29,
    HA_PROG_OBJ_EFFECT_TOTAL    = 0x2A,
} T_APP_HA_PROG_OBJ;

typedef enum
{
    HA_PROG_OBJ_SUSTAIN_SIZE        = 0,
    HA_PROG_OBJ_SELECTABLE_SIZE     = 0,
    HA_PROG_OBJ_NAME_SIZE           = 12,
    HA_PROG_OBJ_TOOL_DATA_SIZE      = 128,
    HA_PROG_OBJ_VOLUME_LEVEL_SIZE   = 4,

    HA_PROG_OBJ_WDRC_SIZE        = HA_EFFECT_WDRC_SIZE,
    HA_PROG_OBJ_NR_SIZE          = HA_EFFECT_NR_SIZE,
    HA_PROG_OBJ_GRAPHIC_EQ_SIZE  = HA_EFFECT_GRAPHIC_EQ_SIZE,
    HA_PROG_OBJ_FBC_SIZE         = HA_EFFECT_FBC_SIZE,
    HA_PROG_OBJ_OVP_SIZE         = HA_EFFECT_OVP_SIZE,
    HA_PROG_OBJ_BF_SIZE          = HA_EFFECT_BF_SIZE,
    HA_PROG_OBJ_WNR_SIZE         = HA_EFFECT_WNR_SIZE,
    HA_PROG_OBJ_INR_SIZE         = HA_EFFECT_INR_SIZE,
    HA_PROG_OBJ_RNS_SIZE         = HA_EFFECT_RNS_SIZE,
    HA_PROG_OBJ_OUTPUT_DRC_SIZE  = HA_EFFECT_OUTPUT_DRC_SIZE,
} T_APP_HA_PROG_OBJ_SIZE;

typedef enum
{
    HA_PROG_OPCODE_GET_NUM                  = 0x00,
    HA_PROG_OPCODE_GET_ID                   = 0x01,
    HA_PROG_OPCODE_SET_ID                   = 0x02,
    HA_PROG_OPCODE_GET_OBJ                  = 0x03,
    HA_PROG_OPCODE_SET_OBJ                  = 0x04,
    HA_PROG_OPCODE_GET_ALL_OBJ              = 0x05,
    HA_PROG_OPCODE_SET_ALL_OBJ              = 0x06,
    HA_PROG_OPCODE_RESET_OBJ                = 0x07,
    HA_PROG_OPCODE_GET_ALL_DIRECTION_OBJ    = 0x08,

    HA_PROG_OPCODE_TOTAL                    = 0x09,
} T_APP_HA_PROG_OPCODE;

typedef enum
{
    HA_GLOBAL_OBJ_WDRC                  = 0x00,
    HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS    = 0x01,
    HA_GLOBAL_OBJ_TOTAL                 = 0x02,
} T_APP_HA_GLOBAL_OBJ;

typedef enum
{
    HA_HEARING_TEST_OPCODE_CREATE   = 0X00,
    HA_HEARING_TEST_OPCODE_SET      = 0X01,
    HA_HEARING_TEST_OPCODE_START    = 0X02,
    HA_HEARING_TEST_OPCODE_STOP     = 0X03,
    HA_HEARING_TEST_OPCODE_DESTROY  = 0X04,

    HA_HEARING_TEST_OPCODE_TOTAL    = 0X05,
} T_APP_HA_HEARING_TEST_OPCODE;

typedef void (*APP_HA_PROG_APPLY_CBACK)(void);
typedef void (*APP_HA_EFFECT_APPLY_CBACK)(void);

typedef struct t_ha_effect_db
{
    T_OS_QUEUE *queue;
    APP_HA_EFFECT_APPLY_CBACK apply_cback;

} T_HA_EFFECT_DB;

typedef struct t_app_ha_prog_obj_item
{
    struct t_app_ha_prog_obj_item     *p_next;
    T_APP_HA_PROG_OBJ                 type;
    uint16_t                          len;
} T_APP_HA_PROG_OBJ_ITEM;

typedef struct t_ha_prog_db
{
    T_OS_QUEUE *obj_queue;
    uint8_t prog_num;
    uint16_t prog_size_arr[HA_PROG_MAX_NUM];
    uint16_t prog_offset_arr[HA_PROG_MAX_NUM];
    uint8_t permanent_prog_idx;
    uint8_t selectable_prog_idx;
    uint8_t current_prog_is_sync_status;
    APP_HA_PROG_APPLY_CBACK apply_cback;
} T_HA_PROG_DB;

typedef struct t_app_ha_params_item
{
    struct t_app_ha_params_item *p_next;
    T_APP_HA_PARAMS             type;
    uint16_t                    len;
    uint8_t                     *buf;
} T_APP_HA_PARAMS_ITEM;

typedef struct t_ha_params_db
{
    T_OS_QUEUE obj_queue;
} T_HA_PARAMS_DB;

typedef struct t_ha_hearing_test_db
{
    uint32_t sample_rate;
    uint32_t freq;
    int32_t gain;
    uint32_t fade_in_out_ms;
    uint32_t duration_ms;
    uint32_t interval_ms;
    uint32_t offset_list[1 + HA_HEARING_TEST_TONE_PHASE];
    uint32_t filter_samples;
    double amplitude;
    double samples_per_sine_wave;
    int32_t mapping_table[HA_HEARING_TEST_TONE_TABLE_MAX_LEN];
    uint32_t test_tone_sample_num;
    void (*test_tone_func[HA_HEARING_TEST_TONE_PHASE])(uint32_t seq, uint32_t len, uint32_t buf_idx);
    uint8_t *buf;
    uint32_t buf_len;
    uint32_t buf_sample_num;
    uint32_t frame_len;
} T_HA_HEARING_TEST_DB;

static uint8_t ha_timer_id = 0;
static uint8_t timer_idx_ha_listening_delay = 0;
static uint8_t timer_idx_ha_apt_vol_level = 0;
static uint8_t timer_idx_ha_hearing_test_tone = 0;

static uint8_t current_apt_vol_level_cnt = 0;
static uint8_t low_to_high_gain_level[5] = {7, 9, 11, 13, 15};

static T_HA_PROG_DB *ha_prog_db;
static T_HA_EFFECT_DB *ha_effect_db;

static T_HA_PARAMS_DB ha_params_db;
static uint8_t ha_hearing_test_freq_handle = 0;
static bool is_hearing_test = false;
static bool bud_is_hearing_test = false;
static uint32_t org_actual_mhz = 40;
static T_AUDIO_TRACK_HANDLE last_playback_handle = NULL;

static uint8_t prev_selectable_prog_idx = 0;

static bool check_switch_listening_mode = false;

static uint16_t ha_params_size_table[HA_PARAMS_TOTAL] =
{
    [HA_PARAMS_RNS] = HA_PARAMS_RNS_SIZE,
    [HA_PARAMS_SPEECH_ENHANCE] = HA_PARAMS_SPEECH_ENHANCE_MAX_SIZE,
    [HA_PARAMS_MMI_VOLUME_STEP] = HA_PARAMS_MMI_VOLUME_STEP_SIZE,
    [HA_PARAMS_WDRC] = HA_PARAMS_WDRC_SIZE,
    [HA_PARAMS_WDRC_ENABLED] = HA_PARAMS_WDRC_ENABLED_SIZE,
    [HA_PARAMS_HA_TEST] = HA_PARAMS_HA_TEST_SIZE,
    [HA_PARAMS_MIC_SETTING] = HA_PARAMS_MIC_SETTING_MAX_SIZE,
    [HA_PARAMS_MIC_COMPENSATION] = HA_PARAMS_MIC_COMPENSATION_MAX_SIZE,
    [HA_PARAMS_SPK_COMPENSATION] = HA_PARAMS_SPK_COMPENSATION_MAX_SIZE,
};

static const uint16_t ha_effect_size_table[HA_EFFECT_TOTAL] =
{
    [HA_EFFECT_WDRC]        = HA_EFFECT_WDRC_SIZE,
    [HA_EFFECT_NR]          = HA_EFFECT_NR_SIZE,
    [HA_EFFECT_GRAPHIC_EQ]  = HA_EFFECT_GRAPHIC_EQ_SIZE,
    [HA_EFFECT_FBC]         = HA_EFFECT_FBC_SIZE,
    [HA_EFFECT_OVP]         = HA_EFFECT_OVP_SIZE,
    [HA_EFFECT_BF]          = HA_EFFECT_BF_SIZE,
    [HA_EFFECT_WNR]         = HA_EFFECT_WNR_SIZE,
    [HA_EFFECT_INR]         = HA_EFFECT_INR_SIZE,
    [HA_EFFECT_RNS]         = HA_EFFECT_RNS_SIZE,
    [HA_EFFECT_OUTPUT_DRC]  = HA_EFFECT_OUTPUT_DRC_SIZE,
};

static const uint16_t ha_prog_obj_size_table[HA_PROG_OBJ_EFFECT_TOTAL] =
{
    [HA_PROG_OBJ_SUSTAIN]       = HA_PROG_OBJ_SUSTAIN_SIZE,
    [HA_PROG_OBJ_SELECTABLE]    = HA_PROG_OBJ_SELECTABLE_SIZE,
    [HA_PROG_OBJ_NAME]          = HA_PROG_OBJ_NAME_SIZE,
    [HA_PROG_OBJ_TOOL_DATA]     = HA_PROG_OBJ_TOOL_DATA_SIZE,
    [HA_PROG_OBJ_WDRC]          = HA_PROG_OBJ_WDRC_SIZE,
    [HA_PROG_OBJ_NR]            = HA_PROG_OBJ_NR_SIZE,
    [HA_PROG_OBJ_GRAPHIC_EQ]    = HA_PROG_OBJ_GRAPHIC_EQ_SIZE,
    [HA_PROG_OBJ_FBC]           = HA_PROG_OBJ_FBC_SIZE,
    [HA_PROG_OBJ_OVP]           = HA_PROG_OBJ_OVP_SIZE,
    [HA_PROG_OBJ_BF]            = HA_PROG_OBJ_BF_SIZE,
    [HA_PROG_OBJ_WNR]           = HA_PROG_OBJ_WNR_SIZE,
    [HA_PROG_OBJ_INR]           = HA_PROG_OBJ_INR_SIZE,
    [HA_PROG_OBJ_RNS]           = HA_PROG_OBJ_RNS_SIZE,
    [HA_PROG_OBJ_VOLUME_LEVEL]  = HA_PROG_OBJ_VOLUME_LEVEL_SIZE,
    [HA_PROG_OBJ_OUTPUT_DRC]    = HA_PROG_OBJ_OUTPUT_DRC_SIZE,
};

static const T_APP_HA_PROG_OBJ ha_program_feature_table[HA_PROG_FEATURE_NUM] =
{
    HA_PROG_OBJ_SELECTABLE,
    HA_PROG_OBJ_NAME,
    HA_PROG_OBJ_NR,
    HA_PROG_OBJ_WDRC,
    HA_PROG_OBJ_TOOL_DATA,
    HA_PROG_OBJ_GRAPHIC_EQ,
    HA_PROG_OBJ_FBC,
    HA_PROG_OBJ_OVP,
    HA_PROG_OBJ_BF,
    HA_PROG_OBJ_WNR,
    HA_PROG_OBJ_INR,
    HA_PROG_OBJ_RNS,
    HA_PROG_OBJ_VOLUME_LEVEL,
    HA_PROG_OBJ_OUTPUT_DRC,
};

static const uint16_t ha_global_obj_size_table[HA_GLOBAL_OBJ_TOTAL] =
{
    [HA_GLOBAL_OBJ_WDRC]                = HA_GLOBAL_OBJ_WDRC_SIZE,
    [HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS]  = HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS_SIZE,
};

static const T_APP_HA_GLOBAL_OBJ ha_global_feature_table[HA_GLOBAL_FEATURE_NUM] =
{
    HA_GLOBAL_OBJ_WDRC,
    HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS,
};

static const T_APP_HA_PARAMS ha_params_feature_table[HA_PARAMS_FEATURE_NUM] =
{
    HA_PARAMS_RNS,
    HA_PARAMS_SPEECH_ENHANCE,
    HA_PARAMS_MMI_VOLUME_STEP,
};

static const T_APP_AUDIO_TONE_TYPE ha_prog_to_tone[HA_PROG_MAX_TONE_NUM] =
{
    TONE_HA_PROG_0,
    TONE_HA_PROG_1,
    TONE_HA_PROG_2,
    TONE_HA_PROG_3,
    TONE_HA_PROG_4,
    TONE_HA_PROG_5,
    TONE_HA_PROG_6,
    TONE_HA_PROG_7,
    TONE_HA_PROG_8,
    TONE_HA_PROG_9,
};

static T_HA_HEARING_TEST_DB ha_hearing_test_db;

static void app_ha_effect_apply_cback_register(APP_HA_EFFECT_APPLY_CBACK ha_effect_apply_cb)
{
    ha_effect_db->apply_cback = ha_effect_apply_cb;
}

static void app_ha_effect_create(T_APP_HA_EFFECT effect)
{
    T_APP_HA_EFFECT_ITEM *p_item = NULL;

    if (effect < HA_EFFECT_TOTAL)
    {
        p_item = (T_APP_HA_EFFECT_ITEM *)malloc(sizeof(T_APP_HA_EFFECT_ITEM));

        p_item->p_next = NULL;
        p_item->effect = effect;
        p_item->info_buf = NULL;
        p_item->info_len = ha_effect_size_table[effect];
        p_item->enabled = false;

        os_queue_in(ha_effect_db->queue, p_item);
    }
}

static void app_ha_effect_set(T_APP_HA_EFFECT effect, void *info_buf, uint16_t info_len)
{
    T_APP_HA_EFFECT_ITEM *p_item = NULL;

    if (info_buf == NULL || info_len == 0)
    {
        return;
    }

    p_item = (T_APP_HA_EFFECT_ITEM *)ha_effect_db->queue->p_first;

    while (p_item != NULL)
    {
        if (p_item->effect == effect)
        {
            if (p_item->info_len != info_len || info_buf == NULL)
            {
                return;
            }

            if (p_item->info_buf == NULL)
            {
                p_item->info_buf = malloc(sizeof(uint8_t) * info_len);
            }

            memcpy(p_item->info_buf, info_buf, info_len);
            p_item->enabled = true;

            return;
        }

        p_item = p_item->p_next;
    }
}

static T_APP_HA_EFFECT_ITEM *app_ha_effect_get(T_APP_HA_EFFECT effect)
{
    T_APP_HA_EFFECT_ITEM *p_item = NULL;

    p_item = (T_APP_HA_EFFECT_ITEM *)ha_effect_db->queue->p_first;

    while (p_item != NULL)
    {
        if (p_item->effect == effect)
        {
            return p_item;
        }

        p_item = p_item->p_next;
    }

    return NULL;
}

#if 0
static void app_ha_effect_enable(T_APP_HA_EFFECT effect)
{
    T_APP_HA_EFFECT_ITEM *p_item = app_ha_effect_get(effect);

    if (p_item)
    {
        p_item->enabled = true;
    }
}
#endif

#if 0
static void app_ha_effect_disable(T_APP_HA_EFFECT effect)
{
    T_APP_HA_EFFECT_ITEM *p_item = app_ha_effect_get(effect);

    if (p_item)
    {
        p_item->enabled = false;
    }
}
#endif

static void app_ha_set_anc_apt_on_off(bool set_anc_apt_on)
{
    T_ANC_APT_STATE state = ANC_OFF_NORMAL_APT_ON;
    T_ANC_APT_EVENT event = EVENT_NORMAL_APT_ON;

    if (app_listening_get_is_special_event_ongoing())
    {
        //MMI triggered, change final to current
        *app_db.final_listening_state = app_db.current_listening_state;
    }

    if (set_anc_apt_on)
    {
        state = app_db.last_anc_apt_on_state;
        event = ANC_APT_STATE_TO_EVENT(state);
    }

    app_listening_state_machine(event, true, true);
}

static void app_ha_effect_apply(T_APP_HA_EFFECT effect)
{
    T_APP_HA_EFFECT_ITEM *p_item = app_ha_effect_get(effect);

    if (p_item)
    {
        if (p_item->enabled && app_apt_is_normal_apt_started())
        {
            audio_probe_dsp_send(p_item->info_buf, p_item->info_len);
        }
    }
}

#if 0
static void app_ha_effect_release(T_APP_HA_EFFECT effect)
{
    T_APP_HA_EFFECT_ITEM *p_item = app_ha_effect_get(effect);

    if (p_item->info_buf)
    {
        free(p_item->info_buf);
    }

    os_queue_delete(ha_effect_db->queue, p_item);
}
#endif

static void app_ha_effect_set_scheme()
{
    uint8_t i = 0;

    for (i = 0; i < HA_EFFECT_TOTAL; i++)
    {
        app_ha_effect_create((T_APP_HA_EFFECT)i);
    }
}

static bool app_ha_tone_flush_and_play(T_APP_AUDIO_TONE_TYPE tone_type, bool relay)
{
    bool ret = false;

    app_audio_tone_type_cancel(ha_prog_to_tone[prev_selectable_prog_idx], relay);

    ret = app_audio_tone_type_play(tone_type, false, relay);

    return ret;
}

static void app_ha_prog_set_selectable_idx(uint8_t prog_id)
{
    uint8_t buf[HA_PROG_OPTION];

    prev_selectable_prog_idx = ha_prog_db->selectable_prog_idx;
    ha_prog_db->selectable_prog_idx = prog_id;

    if (ha_ext_ftl_load_data(buf, 0, HA_PROG_OPTION) == 0)
    {
        buf[2] = prog_id;
        ha_ext_ftl_save_data(buf, 0, HA_PROG_OPTION);
    }

    if ((ha_prog_db->selectable_prog_idx < HA_PROG_MAX_TONE_NUM) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
         app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED))
    {
        app_ha_tone_flush_and_play((T_APP_AUDIO_TONE_TYPE)(TONE_HA_PROG_0 +
                                                           ha_prog_db->selectable_prog_idx), true);
    }
}

static void app_ha_prog_add_object(T_APP_HA_PROG_OBJ type)
{
    T_APP_HA_PROG_OBJ_ITEM *p_item = NULL;

    if (type < HA_PROG_OBJ_CTRL_TOTAL ||
        (type >= HA_PROG_OBJ_WDRC && type < HA_PROG_OBJ_EFFECT_TOTAL))
    {

        p_item = (T_APP_HA_PROG_OBJ_ITEM *)malloc(sizeof(T_APP_HA_PROG_OBJ_ITEM));

        p_item->p_next = NULL;
        p_item->type = type;
        p_item->len = ha_prog_obj_size_table[type]; // include tool data
        os_queue_in(ha_prog_db->obj_queue, p_item);
    }
}

static uint16_t app_ha_prog_get_object_size(uint8_t type)
{
    uint16_t size = 0;

    if (type < HA_PROG_OBJ_EFFECT_TOTAL)
    {
        size = ha_prog_obj_size_table[type];
    }

    return size;
}

static void app_ha_prog_set_scheme()
{
    uint8_t i = 0;
    T_APP_HA_PROG_OBJ_ITEM *p_item;

    while (ha_prog_db->obj_queue->count)
    {
        p_item = os_queue_out(ha_prog_db->obj_queue);
        free(p_item);
    }

    for (i = 0; i < HA_PROG_FEATURE_NUM; i++)
    {
        app_ha_prog_add_object(ha_program_feature_table[i]);
    }
}

static void app_ha_prog_add()
{
    uint8_t buf[HA_PROG_LIST_HEADER_LEN];
    uint8_t *tp_buf = NULL;
    uint16_t prev_total_prog_size = 0;
    uint8_t i = 0;
    uint16_t cur_effect_offset = 0;
    uint16_t prog_size = 0;
    T_APP_HA_PROG_OBJ_ITEM *p_item;

    if (ha_ext_ftl_load_data(buf, 0, HA_PROG_LIST_HEADER_LEN))
    {
        return;
    }

    for (i = 0; i < ha_prog_db->prog_num; i++)
    {
        prev_total_prog_size += buf[4 + i * 2] + (buf[4 + i * 2 + 1] << 8);
    }

    //count type(1 byte), size(2 bytes), data(n bytes)
    p_item = os_queue_peek(ha_prog_db->obj_queue, 0);

    while (p_item)
    {
        prog_size += 4;
        prog_size += p_item->len;
        p_item = p_item->p_next;
    }

    if (prog_size % 4 != 0)
    {
        prog_size = prog_size + 4 - (prog_size % 4);
    }

    ha_prog_db->prog_num += 1;
    buf[0] = ha_prog_db->prog_num;
    buf[HA_PROG_OPTION + 2 * (ha_prog_db->prog_num - 1)] = prog_size;
    buf[HA_PROG_OPTION + 2 * (ha_prog_db->prog_num - 1) + 1] = prog_size >> 8;
    ha_ext_ftl_save_data(buf, 0, HA_PROG_LIST_HEADER_LEN);
    ha_prog_db->prog_size_arr[ha_prog_db->prog_num - 1] = prog_size;
    ha_prog_db->prog_offset_arr[ha_prog_db->prog_num - 1] = HA_PROG_LIST_HEADER_LEN +
                                                            prev_total_prog_size;

    tp_buf = calloc(prog_size, sizeof(uint8_t));

    p_item = os_queue_peek(ha_prog_db->obj_queue, 0);

    while (p_item)
    {
        tp_buf[cur_effect_offset + 0] = p_item->type;
        tp_buf[cur_effect_offset + 1] = 0; // is_set
        tp_buf[cur_effect_offset + 2] = p_item->len;
        tp_buf[cur_effect_offset + 3] = p_item->len >> 8;

        if (p_item->type == HA_PROG_OBJ_SELECTABLE || p_item->type == HA_PROG_OBJ_SUSTAIN ||
            p_item->type == HA_PROG_OBJ_TOOL_DATA)
        {
            tp_buf[cur_effect_offset + 1] = 1;
        }

        cur_effect_offset += p_item->len + 4;
        p_item = p_item->p_next;
    }

    ha_ext_ftl_save_data(tp_buf, ha_prog_db->prog_offset_arr[ha_prog_db->prog_num - 1], prog_size);
    free(tp_buf);

    while (ha_prog_db->obj_queue->count)
    {
        p_item = os_queue_out(ha_prog_db->obj_queue);
        free(p_item);
    }
}

static void app_ha_prog_set_object(uint8_t prog_id, uint8_t type, uint8_t *data)
{
    uint8_t *buf = calloc(ha_prog_db->prog_size_arr[prog_id], sizeof(uint8_t));
    uint16_t cur_object_addr = 0;
    uint16_t cur_object_data_size = 0;
    uint16_t cur_object_size = 0;

    if (ha_ext_ftl_load_data(buf, ha_prog_db->prog_offset_arr[prog_id],
                             ha_prog_db->prog_size_arr[prog_id]))
    {
        free(buf);
        return;
    }

    cur_object_addr = 0;

    while (cur_object_addr < ha_prog_db->prog_size_arr[prog_id])
    {
        cur_object_data_size = buf[cur_object_addr + 2] + (buf[cur_object_addr + 3] << 8);
        cur_object_size = 4 + cur_object_data_size;

        if (buf[cur_object_addr] == type)
        {
            buf[cur_object_addr + 1] = 1; //is set
            memcpy(buf + cur_object_addr + 4, data, cur_object_data_size);

            ha_ext_ftl_save_data(buf, ha_prog_db->prog_offset_arr[prog_id], ha_prog_db->prog_size_arr[prog_id]);
            break;
        }

        cur_object_addr += cur_object_size;
    }

    free(buf);
}

static void app_ha_prog_get_object(uint8_t prog_id, uint8_t type, uint8_t *buf, uint16_t *size)
{
    uint8_t *tp_buf = calloc(ha_prog_db->prog_size_arr[prog_id], sizeof(uint8_t));
    uint16_t cur_effect_addr = 0;
    uint16_t cur_effect_size = 0;

    *size = 0;

    if (ha_ext_ftl_load_data(tp_buf, ha_prog_db->prog_offset_arr[prog_id],
                             ha_prog_db->prog_size_arr[prog_id]))
    {
        free(tp_buf);
        return;
    }

    cur_effect_addr = 0;

    while (cur_effect_addr < ha_prog_db->prog_size_arr[prog_id])
    {
        cur_effect_size = 4 + tp_buf[cur_effect_addr + 2] + (tp_buf[cur_effect_addr + 3] << 8);

        if (tp_buf[cur_effect_addr] == type)
        {
            //if (buf[cur_effect_addr + 1] == 1)
            *size = cur_effect_size - 4;
            memcpy(buf, tp_buf + cur_effect_addr + 4, *size);

            break;
        }

        cur_effect_addr += cur_effect_size;
    }

    free(tp_buf);
}

static uint32_t app_ha_dsp_ext_para_offset_get(uint16_t *payload_len, uint16_t type)
{
    uint32_t offset = 0;
    uint32_t type_offset = 0;
    uint32_t end_offset = 0;
    uint8_t buf[4];
    uint16_t sync_word;
    uint16_t ext_para_len;
    uint16_t feature_type = 0;
    uint16_t feature_para_len = 0;

    //get ext_para_offset
    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                       HA_DSP_EXT_PARA_OFFSET,
                       (uint8_t *)&offset, sizeof(uint32_t));

    //get ext_para sync word, ext_para len
    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                       offset,
                       (uint8_t *)&buf, sizeof(uint32_t));
    sync_word = buf[0] + (buf[1] << 8);
    ext_para_len = buf[2] + (buf[3] << 8);
    end_offset = offset + 4 + ext_para_len;

    if (sync_word == HA_DSP_EXT_PARA_SYNC_WORD)
    {
        offset += 4;

        while (offset < end_offset)
        {
            fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                               offset,
                               (uint8_t *)&buf, sizeof(uint32_t));
            feature_type = buf[0] + (buf[1] << 8);
            feature_para_len = buf[2] + (buf[3] << 8);

            if (feature_type == type)
            {
                type_offset = offset + 4;
                *payload_len = feature_para_len;
                break;
            }

            offset += (4 + feature_para_len);
        }
    }

    return type_offset;
}

static uint16_t app_ha_get_rns_params(uint8_t *payload)
{
    uint16_t len = 0;
    uint32_t offset = 0;

    offset = app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_RNS_PARAMS);

    if (offset)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                           offset, payload, len);
    }

    return len;
}

static uint16_t app_ha_get_speech_enhance(uint8_t *payload)
{
    uint16_t len = 0;
    uint32_t offset = 0;

    offset = app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_SPEECH_ENHANCE);

    if (offset)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) + offset, payload, len);
    }

    return len;
}

static uint16_t app_ha_get_wdrc_params(uint8_t *payload)
{
    uint16_t len = 0;
    uint32_t offset = 0;

    offset = app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_WDRC_PARAMS);

    if (offset)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) + offset, payload, len);
    }

    return len;
}

static uint16_t app_ha_get_wdrc_enabled_params(uint8_t *payload)
{
    uint16_t len = 0;
    uint32_t offset = 0;

    offset = app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_WDRC_ENABLED);

    if (offset)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) + offset, payload, len);
    }

    return len;
}

static uint16_t app_ha_get_ha_test_params(uint8_t *payload)
{
    uint16_t len = 0;
    uint32_t offset = 0;

    offset = app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_TEST);

    if (offset)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) + offset, payload, len);
    }

    return len;
}

static uint16_t app_ha_get_spk_compensation(uint8_t *payload)
{
    uint16_t len = 0;
    uint32_t offset = 0;

    offset = app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_SPK_COMPENSATION);

    if (offset)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) + offset, payload, len);
    }

    return len;
}

static uint16_t app_ha_get_mic_compensation(uint8_t *payload)
{
    uint16_t len = 0;
    uint32_t offset = 0;

    offset = app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_MIC_COMPENSATION);

    if (offset)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) + offset, payload, len);
    }

    return len;
}

static uint16_t app_ha_get_mic_setting(uint8_t *payload)
{
    uint16_t len = 0;
    uint32_t offset = 0;

    offset = app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_MIC_SETTING);

    if (offset)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) + offset, payload, len);
    }

    return len;
}

static void app_ha_params_add(T_APP_HA_PARAMS type)
{
    T_APP_HA_PARAMS_ITEM *p_item = NULL;
    uint16_t len = 0;

    if (type >= HA_PARAMS_TOTAL)
    {
        return;
    }

    p_item = (T_APP_HA_PARAMS_ITEM *)malloc(sizeof(T_APP_HA_PARAMS_ITEM));

    if (p_item == NULL)
    {
        return;
    }

    p_item->p_next = NULL;
    p_item->type = type;
    p_item->len = ha_params_size_table[type];
    p_item->buf = NULL;

    switch (type)
    {
    case HA_PARAMS_RNS:
        {
            p_item->len = 12 + HA_PARAMS_RNS_SIZE;
            p_item->buf = calloc(p_item->len, 1);

            if (p_item->buf == NULL)
            {
                free(p_item);
                break;
            }

            len = app_ha_get_rns_params(p_item->buf + 12);

            if (len)
            {
                p_item->buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                p_item->buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                p_item->buf[2] = (p_item->len) / 4;
                p_item->buf[3] = ((p_item->len) / 4) >> 8;
                p_item->buf[4] = 15;       //RNS params type
                p_item->buf[8] = HA_PARAMS_RNS_SIZE & 0xFF;
                p_item->buf[9] = (HA_PARAMS_RNS_SIZE & 0xFF00) >> 8;

                os_queue_in(&ha_params_db.obj_queue, p_item);
            }
            else
            {
                free(p_item->buf);
                p_item->buf = NULL;
                free(p_item);
                p_item = NULL;
            }
        }
        break;

    case HA_PARAMS_SPEECH_ENHANCE:
        {
            app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_SPEECH_ENHANCE);

            if (len > 0)
            {
                p_item->len = 12 + len;
                p_item->buf = calloc(p_item->len, 1);

                if (p_item->buf == NULL)
                {
                    free(p_item);
                    break;
                }

                len = app_ha_get_speech_enhance(p_item->buf + 12);

                p_item->buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                p_item->buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                p_item->buf[2] = (p_item->len) / 4;
                p_item->buf[3] = ((p_item->len) / 4) >> 8;
                p_item->buf[4] = 19; //SPEECH ENHANCE type
                p_item->buf[8] = len & 0xFF;
                p_item->buf[9] = (len & 0xFF00) >> 8;

                os_queue_in(&ha_params_db.obj_queue, p_item);
            }
            else
            {
                free(p_item);
                p_item = NULL;
            }
        }
        break;

    case HA_PARAMS_WDRC:
        {
            app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_WDRC_PARAMS);
            p_item->len = 8 + len;
            p_item->buf = calloc(p_item->len, 1);

            if (p_item->buf == NULL)
            {
                free(p_item);
                break;
            }

            len = app_ha_get_wdrc_params(p_item->buf + 8);

            if (len)
            {
                p_item->buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                p_item->buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                p_item->buf[2] = (p_item->len - 4) / 4;
                p_item->buf[3] = ((p_item->len - 4) / 4) >> 8;
                p_item->buf[4] = 24; //WDRC params type

                os_queue_in(&ha_params_db.obj_queue, p_item);
            }
            else
            {
                free(p_item->buf);
                p_item->buf = NULL;
                free(p_item);
                p_item = NULL;
            }
        }
        break;

    case HA_PARAMS_WDRC_ENABLED:
        {
            p_item->len = 8 + HA_PARAMS_WDRC_ENABLED_SIZE;
            p_item->buf = calloc(p_item->len, 1);

            if (p_item->buf == NULL)
            {
                free(p_item);
                break;
            }

            len = app_ha_get_wdrc_enabled_params(p_item->buf + 8);

            if (len)
            {
                p_item->buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                p_item->buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                p_item->buf[2] = (p_item->len - 4) / 4;
                p_item->buf[3] = ((p_item->len - 4) / 4) >> 8;
                p_item->buf[4] = 25; //WDRC Enabled params type

                os_queue_in(&ha_params_db.obj_queue, p_item);
            }
            else
            {
                free(p_item->buf);
                p_item->buf = NULL;
                free(p_item);
                p_item = NULL;
            }
        }
        break;

    case HA_PARAMS_HA_TEST:
        {
            p_item->len = 8 + HA_PARAMS_HA_TEST_SIZE;
            p_item->buf = calloc(p_item->len, 1);

            if (p_item->buf == NULL)
            {
                free(p_item);
                break;
            }

            len = app_ha_get_ha_test_params(p_item->buf + 8);

            if (len)
            {
                p_item->buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                p_item->buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                p_item->buf[2] = (p_item->len - 4) / 4;
                p_item->buf[3] = ((p_item->len - 4) / 4) >> 8;
                p_item->buf[4] = 8; //HA TEST params type

                os_queue_in(&ha_params_db.obj_queue, p_item);
            }
            else
            {
                free(p_item->buf);
                p_item->buf = NULL;
                free(p_item);
                p_item = NULL;
            }
        }
        break;

    case HA_PARAMS_SPK_COMPENSATION:
        {
            app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_SPK_COMPENSATION);

            p_item->len = 12 + len;
            p_item->buf = calloc(p_item->len, 1);

            if (p_item->buf == NULL)
            {
                free(p_item);
                break;
            }

            len = app_ha_get_spk_compensation(p_item->buf + 12);

            if (len > 0)
            {
                p_item->buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                p_item->buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                p_item->buf[2] = (p_item->len) / 4;
                p_item->buf[3] = ((p_item->len) / 4) >> 8;
                p_item->buf[4] = 18; //SPK COMPENSATION type
                p_item->buf[8] = len & 0xFF;
                p_item->buf[9] = (len & 0xFF00) >> 8;

                os_queue_in(&ha_params_db.obj_queue, p_item);
            }
            else
            {
                free(p_item->buf);
                p_item->buf = NULL;
                free(p_item);
                p_item = NULL;
            }
        }
        break;

    case HA_PARAMS_MIC_COMPENSATION:
        {
            app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_MIC_COMPENSATION);

            p_item->len = 12 + len;
            p_item->buf = calloc(p_item->len, 1);

            if (p_item->buf == NULL)
            {
                free(p_item);
                break;
            }

            len = app_ha_get_mic_compensation(p_item->buf + 12);

            if (len > 0)
            {
                p_item->buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                p_item->buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                p_item->buf[2] = (p_item->len) / 4;
                p_item->buf[3] = ((p_item->len) / 4) >> 8;
                p_item->buf[4] = 17; //MIC COMPENSATION type
                p_item->buf[8] = len & 0xFF;
                p_item->buf[9] = (len & 0xFF00) >> 8;

                os_queue_in(&ha_params_db.obj_queue, p_item);
            }
            else
            {
                free(p_item->buf);
                p_item->buf = NULL;
                free(p_item);
                p_item = NULL;
            }
        }
        break;

    case HA_PARAMS_MIC_SETTING:
        {
            app_ha_dsp_ext_para_offset_get(&len, HA_DSP_FEATURE_HA_MIC_SETTING);

            p_item->len = 12 + len;
            p_item->buf = calloc(p_item->len, 1);

            if (p_item->buf == NULL)
            {
                free(p_item);
                break;
            }

            len = app_ha_get_mic_setting(p_item->buf + 12);

            if (len > 0)
            {
                p_item->buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                p_item->buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                p_item->buf[2] = (p_item->len) / 4;
                p_item->buf[3] = ((p_item->len) / 4) >> 8;
                p_item->buf[4] = 27; //MIC SETTING type
                p_item->buf[8] = (len / 4) & 0xFF;
                p_item->buf[9] = ((len / 4) & 0xFF00) >> 8;

                os_queue_in(&ha_params_db.obj_queue, p_item);
            }
            else
            {
                free(p_item->buf);
                p_item->buf = NULL;
                free(p_item);
                p_item = NULL;
            }
        }
        break;

    default:
        {
            free(p_item);
            p_item = NULL;
        }
        break;
    }

    return;
}

static void app_ha_params_init()
{
    uint8_t i = 0;

    for (i = 0; i < HA_PARAMS_FEATURE_NUM; i++)
    {
        app_ha_params_add((T_APP_HA_PARAMS)ha_params_feature_table[i]);
    }

    return;
}

void app_ha_generate_gain_db(uint8_t volume_level, uint8_t balance_level, uint8_t *buf)
{
    uint8_t payload[4] = {0};
    uint16_t payload_len = 0;
    uint32_t offset = 0;
    int8_t volume_max_gain = 0;
    int8_t volume_min_gain = 0;
    int8_t balance_max_gain = 0;
    int16_t volume_gain = 0;
    int16_t balance_gain = 0;
    int16_t gain_db = 0;

    offset = app_ha_dsp_ext_para_offset_get(&payload_len, HA_DSP_FEATURE_HA_DSP_CFG_GAIN);

    //get first 4 bytes
    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                       offset, payload, 4);

    volume_max_gain = (int8_t)payload[0];
    volume_min_gain = (int8_t)payload[1];
    balance_max_gain = (int8_t)payload[2];

    volume_gain = volume_min_gain * HA_SCALE_UP_GAIN_DB + ((volume_max_gain - volume_min_gain) *
                                                           volume_level * HA_SCALE_UP_GAIN_DB / 100);

    if ((app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT && balance_level < 50) ||
        (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT && balance_level > 50))
    {
        if (balance_level < 50)
        {
            balance_level = 50 - balance_level;
        }
        else
        {
            balance_level = balance_level - 50;
        }

        balance_gain = balance_max_gain * balance_level * HA_SCALE_UP_GAIN_DB / 50;
    }

    gain_db = volume_gain + balance_gain;

    memset(buf, 0, 12);

    buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
    buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
    buf[2] = 3;
    buf[4] = 16; //volume type
    buf[8] = volume_level;
    buf[10] = gain_db & 0x00FF;
    buf[11] = (gain_db & 0xFF00) >> 8;
}

static uint32_t app_ha_dsp_prog_offset_get(uint16_t *payload_len)
{
    static bool is_init = false;
    static uint32_t prog_offset = 0;
    static uint16_t len = 0;

    if (is_init)
    {
        *payload_len = len;

        return prog_offset;
    }

    prog_offset = app_ha_dsp_ext_para_offset_get(payload_len, HA_DSP_FEATURE_HA_PROG);
    len = *payload_len;
    is_init = true;

    return prog_offset;
}

static uint32_t app_ha_get_global_obj_base()
{
    uint16_t payload_len = 0;

    app_ha_dsp_prog_offset_get(&payload_len);

    return (uint32_t)payload_len;
}

bool app_ha_check_switch_listening_mode(uint8_t prog_id)
{
    uint8_t data[HA_PROG_OBJ_RNS_SIZE] = {0};
    uint16_t size = 0;
    bool is_enabled = false;
    bool is_normal_apt = false;
    bool is_anc_apt = false;

    app_ha_prog_get_object(prog_id, HA_PROG_OBJ_RNS, data, &size);

    is_enabled = (bool)data[8];
    is_normal_apt = app_apt_is_normal_apt_on_state(app_db.current_listening_state);
    is_anc_apt = app_listening_is_anc_apt_on_state(app_db.current_listening_state);

    //RNS on need to enter ANC+APT mode, RNS off need to enter APT mode
    if ((is_normal_apt || is_anc_apt) && (is_enabled == is_normal_apt))
    {
        app_ha_set_anc_apt_on_off(is_enabled);

        return true;
    }

    return false;
}

void app_ha_prog_load(uint8_t prog_id)
{
    uint8_t *buf = calloc(ha_prog_db->prog_size_arr[prog_id], sizeof(uint8_t));
    uint16_t cur_effect_addr = 0;
    uint16_t cur_effect_size = 0;
    uint8_t type = 0;
    T_APP_HA_PARAMS_ITEM *p_item = NULL;
    uint32_t base = 0;

    if (ha_ext_ftl_load_data(buf, ha_prog_db->prog_offset_arr[prog_id],
                             ha_prog_db->prog_size_arr[prog_id]))
    {
        free(buf);
        return;
    }

    if (check_switch_listening_mode)
    {
        app_ha_check_switch_listening_mode(prog_id);
        check_switch_listening_mode = false;
    }

    cur_effect_addr = 0;

    while (cur_effect_addr < ha_prog_db->prog_size_arr[prog_id])
    {
        cur_effect_size = 4 + buf[cur_effect_addr + 2] + (buf[cur_effect_addr + 3] << 8);
        type = buf[cur_effect_addr];

        switch (type)
        {
        case HA_PROG_OBJ_SUSTAIN:
        case HA_PROG_OBJ_SELECTABLE:
        case HA_PROG_OBJ_NAME:
            break;

        case HA_PROG_OBJ_VOLUME_LEVEL:
            {
                if (buf[cur_effect_addr + 1] == 1) // is set
                {
                    uint8_t *info_buf = buf + cur_effect_addr + 4;
                    uint8_t data_buf[12] = {0};
                    uint8_t volume_level = info_buf[0];
                    uint8_t balance_level = info_buf[1];

                    app_ha_generate_gain_db(volume_level, balance_level, data_buf);

                    if (app_apt_is_normal_apt_started())
                    {
                        audio_probe_dsp_send(data_buf, 12);
                    }
                }
            }
            break;

        case HA_PROG_OBJ_NR:
        case HA_PROG_OBJ_WDRC:
        case HA_PROG_OBJ_GRAPHIC_EQ:
        case HA_PROG_OBJ_FBC:
        case HA_PROG_OBJ_OVP:
        case HA_PROG_OBJ_BF:
        case HA_PROG_OBJ_WNR:
        case HA_PROG_OBJ_INR:
        case HA_PROG_OBJ_RNS:
        case HA_PROG_OBJ_OUTPUT_DRC:
            {
                if (buf[cur_effect_addr + 1] == 1) // is set
                {
                    app_ha_effect_set(HA_PROG_OBJ_TO_EFFECT(type), (void *)(buf + cur_effect_addr + 4),
                                      cur_effect_size - 4);
                    app_ha_effect_apply(HA_PROG_OBJ_TO_EFFECT(type));
                }
            }
            break;

        default:
            break;
        }

        cur_effect_addr += cur_effect_size;
    }

    free(buf);

    base = app_ha_get_global_obj_base();

    if (HA_GLOBAL_FEATURE_NUM)
    {
        uint8_t data_buf[4];
        uint8_t obj_type = 0;
        uint8_t obj_set = 0;
        uint16_t obj_size = 0;
        uint32_t offset = 0;
        uint8_t i = 0;

        offset = 0;

        for (i = 0; i < HA_GLOBAL_FEATURE_NUM; i++)
        {
            ha_ext_ftl_load_data(data_buf, base + offset, 4);

            obj_type = data_buf[0];
            obj_set = data_buf[1];
            obj_size = data_buf[2] + (data_buf[3] << 8);

            switch (obj_type)
            {
            case HA_GLOBAL_OBJ_WDRC:
                {
                    if (obj_set && app_apt_is_normal_apt_started())
                    {
                        uint8_t *buf = calloc(obj_size, 1);

                        ha_ext_ftl_load_data(buf, base + offset + 4, obj_size);
                        audio_probe_dsp_send(buf, obj_size);

                        free(buf);
                        buf = NULL;
                    }
                }
                break;

            case HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS:
                {
                    if (obj_set)
                    {
                        uint8_t *buf = calloc(obj_size, 1);

                        ha_ext_ftl_load_data(buf, base + offset + 4, obj_size);
                        ha_prog_db->current_prog_is_sync_status = buf[ha_prog_db->selectable_prog_idx];

                        free(buf);
                        buf = NULL;
                    }
                }
                break;

            default:
                {

                }
                break;
            }

            offset += (4 + obj_size);
        }
    }

    p_item = os_queue_peek(&ha_params_db.obj_queue, 0);

    while (p_item)
    {
        switch (p_item->type)
        {
        case HA_PARAMS_RNS:
        case HA_PARAMS_SPEECH_ENHANCE:
        case HA_PARAMS_WDRC:
        case HA_PARAMS_WDRC_ENABLED:
        case HA_PARAMS_HA_TEST:
        case HA_PARAMS_SPK_COMPENSATION:
        case HA_PARAMS_MIC_COMPENSATION:
        case HA_PARAMS_MIC_SETTING:
            {
                if (p_item->len)
                {
                    audio_probe_dsp_send(p_item->buf, p_item->len);
                }
            }
            break;

        default:
            break;
        }

        p_item = p_item->p_next;
    }
}

bool app_ha_dsp_prog_is_set()
{
    T_APP_HA_DSP_EXT_PARA ext_para;

    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                       HA_DSP_FEATURE_BITS_OFFSET,
                       (uint8_t *)&ext_para, sizeof(T_APP_HA_DSP_EXT_PARA));

    return (bool)ext_para.ha_scenario;
}

static void app_ha_dsp_prog_header_get(uint8_t *buf)
{
    uint16_t payload_len;

    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                       app_ha_dsp_prog_offset_get(&payload_len),
                       buf, HA_PROG_LIST_HEADER_LEN);
}

void app_ha_global_obj_set(T_APP_HA_GLOBAL_OBJ type, uint8_t *data_buf)
{
    uint32_t base = app_ha_get_global_obj_base();
    uint8_t buf[4] = {0};
    uint32_t offset = 0;
    uint8_t i = 0;

    if (type >= HA_GLOBAL_OBJ_TOTAL)
    {
        return;
    }

    offset = 0;

    for (i = 0; i < type; i++)
    {
        offset += (4 + ha_global_obj_size_table[i]);
    }

    ha_ext_ftl_load_data(buf, base + offset, 4);
    buf[1] = 1;
    ha_ext_ftl_save_data(buf, base + offset, 4);

    ha_ext_ftl_save_data(data_buf, base + offset + 4, ha_global_obj_size_table[type]);
}

void app_ha_global_obj_get(T_APP_HA_GLOBAL_OBJ type, uint8_t *data_buf)
{
    uint32_t base = app_ha_get_global_obj_base();
    uint32_t offset = 0;
    uint8_t buf[4] = {0};
    uint8_t i = 0;

    if (type >= HA_GLOBAL_OBJ_TOTAL)
    {
        return;
    }

    offset = 0;

    for (i = 0; i < type; i++)
    {
        offset += (4 + ha_global_obj_size_table[i]);
    }

    ha_ext_ftl_load_data(buf, base + offset, 4);

    if (buf[1]) //is_set
    {
        ha_ext_ftl_load_data(data_buf, base + offset + 4, ha_global_obj_size_table[type]);
    }
}

static bool app_ha_prog_reset(uint8_t prog_id)
{
    uint16_t payload_len;
    uint16_t base = 0;
    uint32_t dsp_prog_base = 0;
    uint8_t buf[HA_BLOCK] = {0};
    uint16_t offset = 0;
    uint8_t i = 0;

    if (!app_ha_dsp_prog_is_set())
    {
        return false;
    }

    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                       app_ha_dsp_prog_offset_get(&payload_len),
                       buf, HA_PROG_LIST_HEADER_LEN);

    if (buf[0] == 0) //check prog num
    {
        return false;
    }

    base = 0;
    offset = 0;
    memset(buf, 0, HA_BLOCK);

    dsp_prog_base = app_ha_dsp_prog_offset_get(&payload_len);
    payload_len = ha_prog_db->prog_size_arr[prog_id];
    offset = ha_prog_db->prog_offset_arr[prog_id];

    for (i = 0; i < payload_len / HA_BLOCK; i++)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                           dsp_prog_base + offset,
                           buf, HA_BLOCK);

        ha_ext_ftl_save_data(buf, base + offset, HA_BLOCK);
        offset += HA_BLOCK;
    }

    if (payload_len % HA_BLOCK)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                           dsp_prog_base + offset,
                           buf, payload_len % HA_BLOCK);
        ha_ext_ftl_save_data(buf, base + offset, payload_len % HA_BLOCK);
    }

    app_ha_global_obj_get(HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS, buf);

    // 0 is unsync, 1 is sync
    buf[prog_id] = !app_cfg_const.rws_disallow_sync_apt_volume;

    app_ha_global_obj_set(HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS, buf);

    check_switch_listening_mode = true;

    app_ha_prog_load(prog_id);

    return true;
}

static bool app_ha_dsp_prog_load()
{
    uint16_t payload_len;
    uint16_t base = 0;
    uint32_t dsp_prog_base = 0;
    uint8_t buf[HA_BLOCK] = {0};
    uint16_t offset = 0;
    uint8_t i = 0;

    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                       app_ha_dsp_prog_offset_get(&payload_len),
                       buf, HA_PROG_LIST_HEADER_LEN);

    if (buf[0] == 0) //check prog num
    {
        return false;
    }

    base = 0;
    offset = 0;
    memset(buf, 0, HA_BLOCK);
    dsp_prog_base = app_ha_dsp_prog_offset_get(&payload_len);

    for (i = 0; i < payload_len / HA_BLOCK; i++)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                           dsp_prog_base + offset,
                           buf, HA_BLOCK);

        ha_ext_ftl_save_data(buf, base + offset, HA_BLOCK);
        offset += HA_BLOCK;
    }

    if (payload_len % HA_BLOCK)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                           dsp_prog_base + offset,
                           buf, payload_len % HA_BLOCK);
        ha_ext_ftl_save_data(buf, base + offset, payload_len % HA_BLOCK);
    }

    return true;
}

static uint32_t app_ha_dsp_spk_response_offset_get(uint16_t *payload_len)
{
    static bool is_init = false;
    static uint32_t offset = 0;
    static uint16_t len = 0;

    if (is_init)
    {
        *payload_len = len;

        return offset;
    }

    offset = app_ha_dsp_ext_para_offset_get(payload_len, HA_DSP_FEATURE_HA_SPK_RESPONSE);
    len = *payload_len;
    is_init = true;

    return offset;
}

static bool app_ha_dsp_spk_response_load(uint8_t *payload, uint16_t *payload_len)
{
    uint32_t offset = app_ha_dsp_spk_response_offset_get(payload_len);

    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                       offset, payload, *payload_len);

    return true;
}

static uint8_t app_ha_version_get()
{
    uint8_t buf[HA_PROG_LIST_HEADER_LEN] = {0};

    if (app_ha_dsp_prog_is_set())
    {
        app_ha_dsp_prog_header_get(buf);
        return buf[3];
    }

    return HA_VERSION;
}

static bool app_ha_prog_db_load()
{
    uint8_t buf[HA_BLOCK] = {0};
    uint8_t i = 0;
    uint16_t prog_start_offset = 0;

    ha_ext_ftl_load_data(buf, 0, HA_BLOCK);

    // check prog num
    if (buf[0] > 0)
    {
        ha_prog_db->prog_num = buf[0];

        if (buf[1] != 0xFF)
        {
            ha_prog_db->permanent_prog_idx = buf[1];
        }

        if (buf[2] != 0xFF)
        {
            prev_selectable_prog_idx = buf[2];
            ha_prog_db->selectable_prog_idx = buf[2];
        }

        prog_start_offset = HA_PROG_LIST_HEADER_LEN;

        for (i = 0; i < ha_prog_db->prog_num; i++)
        {
            uint16_t prog_size = buf[4 + i * 2] + (buf[4 + i * 2 + 1] << 8);

            ha_prog_db->prog_offset_arr[i] = prog_start_offset;
            ha_prog_db->prog_size_arr[i] = prog_size;

            //set raw data offset to next prog
            prog_start_offset += prog_size;
        }

        check_switch_listening_mode = true;

        app_ha_prog_load(ha_prog_db->selectable_prog_idx);

        return true;
    }

    return false;
}

void app_ha_clear_param()
{
    uint8_t buf[HA_BLOCK] = {0};
    uint8_t i = 0;
    uint16_t base = 0;

    memset(buf, 0, HA_BLOCK);

    for (i = 0; i < HA_SIZE / HA_BLOCK; i++)
    {
        ha_ext_ftl_save_data(buf, base, HA_BLOCK);
        base += HA_BLOCK;
    }

    if (HA_SIZE % HA_BLOCK > 0)
    {
        ha_ext_ftl_save_data(buf, base, HA_SIZE % HA_BLOCK);
    }
}

bool app_ha_prog_init()
{
    uint8_t buf[HA_BLOCK] = {0};
    uint8_t i = 0;
    uint32_t status = 0;
    bool is_init = false;

    status = ha_ext_ftl_load_data(buf, 0, HA_BLOCK);

    if (status)
    {
        app_ha_clear_param();
    }

    // check HA version
    if (buf[3] != app_ha_version_get())
    {
        //need to init FTL, prog db from scratch or dsp config
        if (app_ha_dsp_prog_is_set())
        {
            app_ha_dsp_prog_load();
            is_init = app_ha_prog_db_load();
        }
        else
        {
            if (ha_ext_ftl_load_data(buf, 0, HA_PROG_OPTION))
            {
                is_init = false;
            }
            else //init FTL, prog db from scratch
            {
                buf[3] = HA_VERSION;
                ha_ext_ftl_save_data(buf, 0, HA_PROG_OPTION);

                for (i = 0; i < HA_PROG_NUM; i++)
                {
                    app_ha_prog_set_scheme();
                    app_ha_prog_add();
                }

                is_init = true;
            }
        }
    }
    else
    {
        is_init = app_ha_prog_db_load();
    }

    return is_init;
}

void app_ha_prog_get_object_status(uint8_t prog_id, uint8_t type, uint8_t *data)
{
    uint8_t obj_data[HA_PROG_OBJ_TOOL_DATA_SIZE] = {0};
    uint16_t obj_size = 0;

    app_ha_prog_get_object(prog_id, HA_PROG_OBJ_TOOL_DATA, obj_data, &obj_size);

    switch (type)
    {
    case HA_PROG_OBJ_NR:
        {
            memcpy(data, obj_data + 112, 4);
        }
        break;

    case HA_PROG_OBJ_OVP:
        {
            memcpy(data, obj_data + 108, 4);
        }
        break;

    case HA_PROG_OBJ_BF:
        {
            memcpy(data, obj_data + 104, 4);
        }
        break;

    default:
        {

        }
        break;
    }
}

void app_ha_prog_set_object_with_tool_data(uint8_t prog_id, uint8_t type, uint8_t *data)
{
    uint8_t obj_data[HA_PROG_OBJ_TOOL_DATA_SIZE] = {0};
    uint16_t obj_size = 0;

    app_ha_prog_get_object(prog_id, HA_PROG_OBJ_TOOL_DATA, obj_data, &obj_size);

    switch (type)
    {
    case HA_PROG_OBJ_NR:
        {
            memcpy(obj_data + 112, data + 8, 4);
            app_ha_prog_set_object(prog_id, HA_PROG_OBJ_TOOL_DATA, obj_data);
            app_ha_prog_set_object(prog_id, HA_PROG_OBJ_NR, data);
        }
        break;

    case HA_PROG_OBJ_OVP:
        {
            memcpy(obj_data + 108, data + 8, 4);
            app_ha_prog_set_object(prog_id, HA_PROG_OBJ_TOOL_DATA, obj_data);
            app_ha_prog_set_object(prog_id, HA_PROG_OBJ_OVP, data);
        }
        break;

    case HA_PROG_OBJ_BF:
        {
            memcpy(obj_data + 104, data + 8, 4);
            app_ha_prog_set_object(prog_id, HA_PROG_OBJ_TOOL_DATA, obj_data);
            app_ha_prog_set_object(prog_id, HA_PROG_OBJ_BF, data);
        }
        break;

    default:
        {

        }
        break;
    }


}

void app_ha_generate_nr_cmd(uint8_t is_enabled, uint8_t level, uint8_t *buf)
{
    buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
    buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
    buf[2] = 2;
    buf[4] = 4; //nr type
    buf[8] = is_enabled;
    buf[9] = level;
}

void app_ha_generate_ovp_cmd(uint8_t is_enabled, uint8_t level, uint8_t *buf)
{
    buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
    buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
    buf[2] = 2;
    buf[4] = 5; //ovp type
    buf[8] = is_enabled;
    buf[9] = level;
}

void app_ha_generate_bf_cmd(uint8_t is_enabled, uint8_t *buf)
{
    buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
    buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
    buf[2] = 2;
    buf[4] = 6; //bf type
    buf[8] = is_enabled;
}

void app_ha_global_obj_init()
{
    uint32_t base = 0;
    uint32_t offset = 0;
    uint8_t obj_type = 0;
    uint8_t obj_set = 0;
    uint16_t obj_size = 0;
    uint8_t i = 0;
    uint8_t buf[4] = {0};
    uint8_t data_buf[HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS_SIZE] = {0};

    base = app_ha_get_global_obj_base();

    if (HA_GLOBAL_FEATURE_NUM == 0)
    {
        return;
    }

    ha_ext_ftl_load_data(buf, base, 4);

    obj_size = buf[2] + (buf[3] >> 8);

    if (obj_size == 0) //need to initilize global obj columns
    {
        offset = 0;
        obj_set = 0;

        for (i = 0; i < HA_GLOBAL_FEATURE_NUM; i++)
        {
            obj_type = ha_global_feature_table[i];
            obj_size = ha_global_obj_size_table[obj_type];

            buf[0] = obj_type;
            buf[1] = obj_set;
            buf[2] = obj_size;
            buf[3] = obj_size >> 8;

            ha_ext_ftl_save_data(buf, base + offset, 4);
            offset += (4 + obj_size);
        }

        memset(data_buf, !app_cfg_const.rws_disallow_sync_apt_volume,
               HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS_SIZE);
        app_ha_global_obj_set(HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS, data_buf);
    }
}

static void app_ha_get_dsp_cfg_gain(void)
{
    uint8_t payload[132] = {0};
    uint16_t payload_len = 0;
    uint32_t offset = 0;

    offset = app_ha_dsp_ext_para_offset_get(&payload_len, HA_DSP_FEATURE_HA_DSP_CFG_GAIN);
    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) +
                       offset, payload, payload_len);
    app_report_event_broadcast(EVENT_HA_DSP_CFG_GAIN_INFO, payload, payload_len);
}

static void app_ha_get_spk_response(bool is_remote)
{
    uint8_t payload[100] = {0};
    uint16_t payload_len = 0;

    app_ha_dsp_spk_response_load(payload, &payload_len);

    if (!is_remote)
    {
        app_report_event_broadcast(EVENT_HA_SPK_RESPONSE_INFO, payload, payload_len);
    }
    else
    {
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                            APP_REMOTE_MSG_HA_PROGRAM_RELAY_SPK_RESPONSE,
                                            payload,
                                            payload_len);
    }

    return;
}

static uint8_t app_ha_get_volume_level(bool is_vol_up, uint8_t current_level)
{
    static uint8_t data[20] = {0};
    static bool is_init = false;
    static uint16_t data_len = 0;
    uint32_t offset = 0;
    uint8_t level = 0xFF; // 0xFF: no need to change level
    uint8_t i = 0;

    if (is_init == false)
    {
        offset = app_ha_dsp_ext_para_offset_get(&data_len, HA_DSP_FEATURE_HA_MMI_VOLUME_STEP);
        is_init = true;

        if (data_len)
        {
            fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_DSPCONFIG) + offset, data,
                               data_len);
        }
    }

    if (data_len)
    {
        if (is_vol_up)
        {
            for (i = 4; i < 4 + data[0]; i++)
            {
                if (data[i] > current_level)
                {
                    level = data[i];
                    break;
                }
            }
        }
        else
        {
            for (i = 4 + data[0] - 1; i >= 4; i--)
            {
                if (data[i] < current_level)
                {
                    level = data[i];
                    break;
                }
            }
        }
    }

    return level;
}

uint8_t app_ha_get_volume_sync()
{
    return ha_prog_db->current_prog_is_sync_status;
}

static void app_ha_sync_volume_sync()
{
    uint8_t buf[HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS_SIZE] = {0};
    uint16_t size = 0;
    bool is_sync = false;

    if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_db.remote_session_state != REMOTE_SESSION_STATE_CONNECTED))
    {
        return;
    }

    app_ha_global_obj_get(HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS, buf);

    // 0 is unsync, 1 is sync
    is_sync = (bool)buf[ha_prog_db->selectable_prog_idx];

    if (is_sync == false)
    {
        return;
    }

    uint8_t data[HA_PROG_OBJ_VOLUME_LEVEL_SIZE] = {0};

    app_ha_prog_get_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, data, &size);

    //send current earbud's volume
    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                        APP_REMOTE_MSG_HA_PROGRAM_RELAY_HA_VOLUME_STATUS,
                                        data, 1);
}

void app_ha_get_all_direction_obj(uint8_t prog_id, uint8_t type)
{
    uint16_t obj_size = app_ha_prog_get_object_size(type);
    uint16_t event_len = 5 + 2 * obj_size;
    uint16_t offset = 5;
    uint8_t *event_data = calloc(event_len, sizeof(uint8_t));

    event_data[0] = prog_id;
    event_data[1] = HA_PROG_OPCODE_GET_ALL_DIRECTION_OBJ;
    event_data[2] = type;
    event_data[3] = obj_size;
    event_data[4] = obj_size >> 8;

    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)
    {
        offset += obj_size;
    }

    if (((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) &&
         (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)) ||
        (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED))
    {
        app_ha_prog_get_object(prog_id, type, event_data + offset, &obj_size);
    }

    if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                            APP_REMOTE_MSG_HA_PROGRAM_GET_ALL_DIRECTION_OBJ,
                                            event_data, event_len);
    }
    else
    {
        app_report_event_broadcast(EVENT_HA_PROGRAM_INFO, event_data + 1, event_len - 1);
    }

    free(event_data);
}

void app_ha_get_all_direction_obj_status(uint8_t prog_id, uint8_t type)
{
    switch (type)
    {
    case HA_PROG_OBJ_VOLUME_LEVEL:
        {
            uint16_t event_len = 5; //prog_id, buds' status, L level, R level, balance
            uint16_t offset = 2;
            uint8_t *event_data = calloc(event_len, sizeof(uint8_t));
            uint16_t tool_data_size = app_ha_prog_get_object_size(HA_PROG_OBJ_TOOL_DATA);
            uint8_t *tool_data = calloc(tool_data_size, 1);

            app_ha_prog_get_object(prog_id, HA_PROG_OBJ_TOOL_DATA, tool_data, &tool_data_size);

            event_data[0] = prog_id;

            memcpy(event_data + offset, tool_data + 2, 2); //set curr bud's volume

            offset = 4 + tool_data[1] * 2;
            memcpy(event_data + 4, tool_data + offset, 1); //set balance

            free(tool_data);
            tool_data = NULL;

            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
                (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
            {
                event_data[1] = 2;
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                    APP_REMOTE_MSG_HA_PROGRAM_GET_HA_VOL,
                                                    event_data, event_len);
            }
            else
            {
                event_data[1] = app_cfg_const.bud_side;
                app_report_event_broadcast(EVENT_HA_PROGRAM_VOL_STATUS_INFO, event_data + 1, event_len - 1);
            }

            free(event_data);
            event_data = NULL;
        }
        break;

    default:
        {

        }
        break;
    }
}

void app_ha_adjust_volume_level(bool is_vol_up)
{
    if (app_apt_is_normal_apt_started())
    {
        uint8_t data[4] = {0};
        uint8_t cmd_buf[12] = {0};
        uint16_t size = 0;
        uint8_t volume_level = 0;
        uint8_t balance_level = 0;

        app_ha_prog_get_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, data, &size);
        volume_level = data[0];
        balance_level = data[1];

        volume_level = app_ha_get_volume_level(is_vol_up, volume_level);

        if (volume_level <= 100)
        {
            app_ha_generate_gain_db(volume_level, balance_level, cmd_buf);
            audio_probe_dsp_send(cmd_buf, 12);

            data[0] = volume_level;
            app_ha_prog_set_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, data);

            app_ha_get_all_direction_obj(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL);

        }
    }
}

static void *app_ha_get_audio_track()
{
    if (app_a2dp_is_streaming())
    {
        T_APP_BR_LINK *p_link = NULL;

        p_link = &app_db.br_link[app_a2dp_get_active_idx()];
        return p_link->a2dp_track_handle;
    }

    return NULL;
}

static void app_ha_get_audio_volume()
{
    void *a2dp_track_handle = app_ha_get_audio_track();
    T_AUDIO_TRACK_STATE state = AUDIO_TRACK_STATE_RELEASED;

    app_audio_hearing_state_get(&state);

    if (a2dp_track_handle)
    {
        audio_track_signal_out_monitoring_start(a2dp_track_handle, HA_AUDIO_VOLUME_REFRESH_INTERVAL);
    }
    else if (state == AUDIO_TRACK_STATE_STARTED)
    {
        audio_track_signal_out_monitoring_start(app_audio_hearing_track_handle_get(),
                                                HA_AUDIO_VOLUME_REFRESH_INTERVAL);
    }

    return;
}

bool app_ha_get_bud_is_hearing_test()
{
    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        return bud_is_hearing_test;
    }

    return false;
}

bool app_ha_get_is_hearing_test()
{
    return is_hearing_test;
}

void app_ha_set_is_hearing_test(bool status)
{
    is_hearing_test = status;

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                            APP_REMOTE_MSG_HA_PROGRAM_RELAY_HEARING_TEST_STATUS,
                                            (uint8_t *)&is_hearing_test,
                                            1);
    }
}

static void app_ha_audio_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    switch (event_type)
    {
    case AUDIO_EVENT_PASSTHROUGH_ENABLED:
        {
            T_AUDIO_EVENT_PARAM_PASSTHROUGH_STARTED *param = (T_AUDIO_EVENT_PARAM_PASSTHROUGH_STARTED *)
                                                             event_buf;
            if (param->mode == AUDIO_PASSTHROUGH_MODE_NORMAL)
            {
                ha_effect_db->apply_cback();
            }
        }
        break;

    case AUDIO_EVENT_TRACK_SIGNAL_OUT_REFRESHED:
        {
            T_AUDIO_EVENT_PARAM *payload = event_buf;
            uint8_t buf[5];
            void *a2dp_track_handle = app_ha_get_audio_track();
            T_AUDIO_TRACK_STATE state = AUDIO_TRACK_STATE_RELEASED;

            app_audio_hearing_state_get(&state);

            if (a2dp_track_handle)
            {
                audio_track_signal_out_monitoring_stop(a2dp_track_handle);
            }
            else if (state == AUDIO_TRACK_STATE_STARTED)
            {
                audio_track_signal_out_monitoring_stop(app_audio_hearing_track_handle_get());
            }

            buf[0] = app_cfg_const.bud_side;

            //left_gain, right_gain are uint16_t type which respectively receive signed value
            //posted by dsp
            //To get original audio gain measured by dsp, mcu needs to do type cast to recover
            //signed dsp value and do bit operation
            buf[1] = (uint8_t)((int16_t)(payload->track_signal_out_refreshed.left_gain) >> 7);
            buf[2] = (uint8_t)((int16_t)(payload->track_signal_out_refreshed.left_gain) >> 15);
            buf[3] = (uint8_t)((int16_t)(payload->track_signal_out_refreshed.right_gain) >> 7);
            buf[4] = (uint8_t)((int16_t)(payload->track_signal_out_refreshed.right_gain) >> 15);

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_report_event_broadcast(EVENT_HA_AUDIO_VOLUME_INFO, buf, 5);
            }
            else
            {
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                    APP_REMOTE_MSG_HA_PROGRAM_RELAY_AUDIO_VOLUME,
                                                    buf,
                                                    5);
            }
        }
        break;

    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            T_AUDIO_EVENT_PARAM *param = event_buf;
            uint8_t i = 0;
            uint8_t app_idx = app_a2dp_get_active_idx();
            T_AUDIO_STREAM_TYPE mode = (T_AUDIO_STREAM_TYPE)0xFF;

            if (param->track_state_changed.handle == app_audio_hearing_track_handle_get() &&
                param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED &&
                ha_hearing_test_db.buf)
            {
                for (i = 0; i < 7; i++)
                {
                    app_ha_hearing_test_tone_generator();
                    app_audio_hearing_write(1, ha_hearing_test_db.buf, ha_hearing_test_db.buf_len);
                }
            }

            if (param->track_state_changed.handle)
            {
                audio_track_stream_type_get(param->track_state_changed.handle, &mode);

                if (mode == AUDIO_STREAM_TYPE_PLAYBACK)
                {
                    last_playback_handle = param->track_state_changed.handle;
                }
            }

            if (app_ha_get_is_hearing_test() == true &&
                param->track_state_changed.handle == last_playback_handle &&
                param->track_state_changed.state == AUDIO_TRACK_STATE_RELEASED &&
                ha_hearing_test_db.buf)
            {
                app_ha_hearing_test_start();
            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_LOW:
        {
            T_AUDIO_EVENT_PARAM_TRACK_BUFFER_LOW *payload = event_buf;
            uint8_t i = 0;

            if (payload->handle == app_audio_hearing_track_handle_get() &&
                ha_hearing_test_db.buf)
            {
                for (i = 0; i < 3; i++)
                {
                    app_ha_hearing_test_tone_generator();
                    app_audio_hearing_write(1, ha_hearing_test_db.buf, ha_hearing_test_db.buf_len);
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_ha_effect_apply_cback(void)
{
    check_switch_listening_mode = false;

    app_ha_prog_load(ha_prog_db->selectable_prog_idx);
}

#if F_APP_LISTENING_MODE_SUPPORT
void app_ha_listening_delay_start()
{
    app_start_timer(&timer_idx_ha_listening_delay, "ha_listening_delay",
                    ha_timer_id, APP_TIMER_HA_LISTENING_DELAY, 0, false,
                    app_cfg_const.time_delay_to_open_apt_when_in_ear * 1000);
}
#endif

void app_ha_switch_hearable_prog()
{
    app_ha_prog_set_selectable_idx((ha_prog_db->selectable_prog_idx + 1) % HA_PROG_NUM);

    check_switch_listening_mode = true;

    app_ha_prog_load(ha_prog_db->selectable_prog_idx);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
        app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        uint8_t event_data[2];

        event_data[0] = HA_PROG_OPCODE_SET_ID;
        event_data[1] = ha_prog_db->selectable_prog_idx;
        app_report_event_broadcast(EVENT_HA_PROGRAM_INFO, event_data, sizeof(uint8_t) * 2);
    }
}

static void app_ha_hearing_test_tone_fade_in_sample_generator(uint32_t seq, uint32_t len,
                                                              uint32_t buf_idx)
{
    uint32_t i = 0;
    int32_t val = 0;

    for (i = seq; i < seq + len; i++)
    {
        val = ((double)i / ha_hearing_test_db.filter_samples) * ha_hearing_test_db.mapping_table[i %
                  HA_HEARING_TEST_TONE_TABLE_MAX_LEN];

        ha_hearing_test_db.buf[buf_idx++] = val & 0x000000FF;
        ha_hearing_test_db.buf[buf_idx++] = (val & 0x0000FF00) >> 8;
        ha_hearing_test_db.buf[buf_idx++] = (val & 0x00FF0000) >> 16;
    }
}

static void app_ha_hearing_test_tone_duration_sample_generator(uint32_t seq, uint32_t len,
                                                               uint32_t buf_idx)
{
    uint32_t i = 0;
    int32_t val = 0;

    for (i = seq; i < seq + len; i++)
    {
        val = ha_hearing_test_db.mapping_table[i % HA_HEARING_TEST_TONE_TABLE_MAX_LEN];

        ha_hearing_test_db.buf[buf_idx++] = val & 0x000000FF;
        ha_hearing_test_db.buf[buf_idx++] = (val & 0x0000FF00) >> 8;
        ha_hearing_test_db.buf[buf_idx++] = (val & 0x00FF0000) >> 16;
    }
}

static void app_ha_hearing_test_tone_fade_out_sample_generator(uint32_t seq, uint32_t len,
                                                               uint32_t buf_idx)
{
    uint32_t i = 0;
    int32_t val = 0;

    for (i = seq; i < seq + len; i++)
    {
        val = (1 - ((double)i / ha_hearing_test_db.filter_samples)) * ha_hearing_test_db.mapping_table[i %
                  HA_HEARING_TEST_TONE_TABLE_MAX_LEN];

        ha_hearing_test_db.buf[buf_idx++] = val & 0x000000FF;
        ha_hearing_test_db.buf[buf_idx++] = (val & 0x0000FF00) >> 8;
        ha_hearing_test_db.buf[buf_idx++] = (val & 0x00FF0000) >> 16;
    }
}

static void app_ha_hearing_test_tone_interval_sample_generator(uint32_t seq, uint32_t len,
                                                               uint32_t buf_idx)
{
    uint32_t i = 0;

    for (i = seq; i < seq + len; i++)
    {
        ha_hearing_test_db.buf[buf_idx++] = 0;
        ha_hearing_test_db.buf[buf_idx++] = 0;
        ha_hearing_test_db.buf[buf_idx++] = 0;
    }
}

static void app_ha_hearing_test_tone_spefify_window_sample_generator(uint32_t test_tone_seq,
                                                                     uint32_t test_tone_num, uint32_t buf_idx)
{
    int8_t i = 0;
    uint32_t tp_seq = 0;
    uint32_t tp_num = 0;
    uint32_t cou = 0;
    uint8_t generator_idx = 0;

    for (i = HA_HEARING_TEST_TONE_PHASE; i >= 0; i--)
    {
        if (test_tone_seq >= ha_hearing_test_db.offset_list[i])
        {
            generator_idx = i;
            break;
        }
    }

    tp_seq = test_tone_seq;
    tp_num = test_tone_num;
    cou = 0;

    while (tp_num > 0)
    {
        if (tp_seq < ha_hearing_test_db.offset_list[generator_idx])
        {
            generator_idx += 1;
            continue;
        }

        if (tp_seq + tp_num < ha_hearing_test_db.offset_list[generator_idx + 1])
        {
            cou = tp_num;
        }
        else
        {
            cou = ha_hearing_test_db.offset_list[generator_idx + 1] - tp_seq;
        }

        ha_hearing_test_db.test_tone_func[generator_idx](tp_seq -
                                                         ha_hearing_test_db.offset_list[generator_idx], cou, buf_idx);

        tp_seq += cou;
        tp_num -= cou;
        buf_idx += (cou * HA_HEARING_TEST_SAMPLE_DEPTH_IN_BYTES);
        generator_idx += 1;

        if (generator_idx + 1 >= 1 + HA_HEARING_TEST_TONE_PHASE)
        {
            break;
        }
    }
}

static void app_ha_hearing_test_update_mapping_table()
{
    uint16_t i = 0;

    for (i = 0; i < HA_HEARING_TEST_TONE_TABLE_MAX_LEN; i++)
    {
        ha_hearing_test_db.mapping_table[i] = ha_hearing_test_db.amplitude * round(sin(
                                                                                       i * 2 * 3.14159265358979323846 / ha_hearing_test_db.samples_per_sine_wave) *
                                                                                   HA_HEARING_TEST_SAMPLE_DEPTH_MAX_VAL);
    }
}

static void app_ha_hearing_test_tone_set(uint32_t freq, int32_t gain, uint16_t timeout_ms)
{
    uint16_t i = 0;
    uint32_t fade_in_num  = round(ha_hearing_test_db.sample_rate * (double)
                                  ha_hearing_test_db.fade_in_out_ms / 1000);
    uint32_t duration_num = round(ha_hearing_test_db.sample_rate * (double)
                                  ha_hearing_test_db.duration_ms / 1000);
    uint32_t fade_out_num = round(ha_hearing_test_db.sample_rate * (double)
                                  ha_hearing_test_db.fade_in_out_ms / 1000);
    uint32_t interval_num = round(ha_hearing_test_db.sample_rate * (double)
                                  ha_hearing_test_db.interval_ms / 1000);

    ha_hearing_test_db.offset_list[i++] = 0;
    ha_hearing_test_db.offset_list[i] = ha_hearing_test_db.offset_list[i - 1] + fade_in_num;
    i++;
    ha_hearing_test_db.offset_list[i] = ha_hearing_test_db.offset_list[i - 1] + duration_num;
    i++;
    ha_hearing_test_db.offset_list[i] = ha_hearing_test_db.offset_list[i - 1] + fade_out_num;
    i++;
    ha_hearing_test_db.offset_list[i] = ha_hearing_test_db.offset_list[i - 1] + interval_num;

    ha_hearing_test_db.test_tone_sample_num = fade_in_num + duration_num + fade_out_num + interval_num;

    ha_hearing_test_db.filter_samples = ha_hearing_test_db.fade_in_out_ms *
                                        ha_hearing_test_db.sample_rate / 1000;

    ha_hearing_test_db.freq = freq;
    ha_hearing_test_db.gain = gain;

    ha_hearing_test_db.amplitude = pow(10.0, gain / 20.0);
    ha_hearing_test_db.samples_per_sine_wave = round(ha_hearing_test_db.sample_rate * 100 / freq) /
                                               100; //step

    app_stop_timer(&timer_idx_ha_hearing_test_tone);

    if (timeout_ms)
    {
        app_start_timer(&timer_idx_ha_hearing_test_tone, "ha_hearing_test",
                        ha_timer_id, APP_TIMER_HA_HEARING_TEST_TONE, 0, false,
                        timeout_ms);
    }
}

static void app_ha_hearing_test_tone_init(uint32_t sample_rate, uint32_t freq,
                                          uint32_t fade_in_out_ms, uint32_t duration_ms,
                                          uint32_t interval_ms, int32_t gain,
                                          uint32_t buf_len)
{
    uint16_t i = 0;
    uint32_t fade_in_num  = round(sample_rate * (double)fade_in_out_ms / 1000);
    uint32_t duration_num = round(sample_rate * (double)duration_ms / 1000);
    uint32_t fade_out_num = round(sample_rate * (double)fade_in_out_ms / 1000);
    uint32_t interval_num = round(sample_rate * (double)interval_ms / 1000);

    ha_hearing_test_db.sample_rate = sample_rate;
    ha_hearing_test_db.freq = freq;
    ha_hearing_test_db.fade_in_out_ms = fade_in_out_ms;
    ha_hearing_test_db.duration_ms = duration_ms;
    ha_hearing_test_db.interval_ms = interval_ms;
    ha_hearing_test_db.gain = gain;

    ha_hearing_test_db.offset_list[i++] = 0;
    ha_hearing_test_db.offset_list[i] = ha_hearing_test_db.offset_list[i - 1] + fade_in_num;
    i++;
    ha_hearing_test_db.offset_list[i] = ha_hearing_test_db.offset_list[i - 1] + duration_num;
    i++;
    ha_hearing_test_db.offset_list[i] = ha_hearing_test_db.offset_list[i - 1] + fade_out_num;
    i++;
    ha_hearing_test_db.offset_list[i] = ha_hearing_test_db.offset_list[i - 1] + interval_num;

    ha_hearing_test_db.filter_samples = fade_in_out_ms * sample_rate / 1000;
    ha_hearing_test_db.amplitude = pow(10.0, gain / 20.0);
    ha_hearing_test_db.samples_per_sine_wave = round(ha_hearing_test_db.sample_rate * 100 / freq) /
                                               100; //step

    ha_hearing_test_db.test_tone_sample_num = fade_in_num + duration_num + fade_out_num + interval_num;

    i = 0;
    ha_hearing_test_db.test_tone_func[i++] = app_ha_hearing_test_tone_fade_in_sample_generator;
    ha_hearing_test_db.test_tone_func[i++] = app_ha_hearing_test_tone_duration_sample_generator;
    ha_hearing_test_db.test_tone_func[i++] = app_ha_hearing_test_tone_fade_out_sample_generator;
    ha_hearing_test_db.test_tone_func[i++] = app_ha_hearing_test_tone_interval_sample_generator;

    ha_hearing_test_db.buf = calloc(buf_len, sizeof(uint8_t));
    ha_hearing_test_db.buf_len = buf_len;
    ha_hearing_test_db.buf_sample_num = buf_len / HA_HEARING_TEST_SAMPLE_DEPTH_IN_BYTES;
    ha_hearing_test_db.frame_len = HA_HEARING_TEST_FRAME_LEN;

    app_ha_hearing_test_update_mapping_table();
    app_stop_timer(&timer_idx_ha_hearing_test_tone);
}

static void app_ha_hearing_test_tone_generator()
{
    uint32_t buf_left_sample_num = 0;
    uint32_t buf_idx = 0;
    static uint32_t test_tone_seq = 0;
    uint32_t cou = 0;

    buf_left_sample_num = ha_hearing_test_db.buf_sample_num;
    buf_idx = 0;

    while (true)
    {
        if (test_tone_seq + buf_left_sample_num <= ha_hearing_test_db.test_tone_sample_num)
        {
            cou = buf_left_sample_num;
        }
        else
        {
            cou = ha_hearing_test_db.test_tone_sample_num - test_tone_seq;
        }

        app_ha_hearing_test_tone_spefify_window_sample_generator(test_tone_seq, cou, buf_idx);

        test_tone_seq += cou;
        buf_idx += (cou * HA_HEARING_TEST_SAMPLE_DEPTH_IN_BYTES);
        buf_left_sample_num -= cou;

        if (test_tone_seq == ha_hearing_test_db.test_tone_sample_num)
        {
            app_ha_hearing_test_update_mapping_table();
            test_tone_seq = 0;
        }

        if (buf_idx == (ha_hearing_test_db.buf_sample_num * HA_HEARING_TEST_SAMPLE_DEPTH_IN_BYTES))
        {
            break;
        }
    }

    return;
}

static void app_ha_hearing_test_start()
{
    T_AUDIO_FORMAT_INFO format_info_test;
    uint8_t i = 0;
    uint32_t actual_mhz;

    org_actual_mhz = pm_cpu_freq_get();
    pm_cpu_freq_req(&ha_hearing_test_freq_handle, 100, &actual_mhz);

    app_ha_hearing_test_update_mapping_table();

    format_info_test.type = AUDIO_FORMAT_TYPE_PCM;
    format_info_test.attr.pcm.sample_rate = ha_hearing_test_db.sample_rate;
    format_info_test.attr.pcm.frame_length = ha_hearing_test_db.frame_len;
    format_info_test.attr.pcm.chann_num = 1;
    format_info_test.attr.pcm.bit_width = HA_HEARING_TEST_SAMPLE_DEPTH;

    app_audio_hearing_start(format_info_test, HA_HEARING_TEST_TONE_DEFAULT_VOLUME);
}

static void app_ha_hearing_test_stop()
{
    uint32_t actual_mhz = 0;

    app_audio_hearing_stop();
    pm_cpu_freq_req(&ha_hearing_test_freq_handle, org_actual_mhz, &actual_mhz);
}

static void app_ha_hearing_test_init()
{
    app_ha_hearing_test_tone_init(HA_HEARING_TEST_SAMPLE_RATE, HA_HEARING_TEST_TONE_DEFAULT_FREQ,
                                  HA_HEARING_TEST_TONE_FADE_IN_OUT_MS, HA_HEARING_TEST_TONE_DURATION_MS,
                                  HA_HEARING_TEST_TONE_INTERVAL_MS, HA_HEARING_TEST_TONE_DEFAULT_GAIN, HA_HEARING_TEST_FRAME_LEN);
}

static void app_ha_hearing_test_deinit()
{
    app_stop_timer(&timer_idx_ha_hearing_test_tone);

    free(ha_hearing_test_db.buf);
    ha_hearing_test_db.buf = NULL;
}

void app_ha_check_trigger_stop_hearing_test(uint8_t status)
{
    if (app_ha_get_is_hearing_test() == true)
    {
        app_ha_hearing_test_stop();
        app_ha_set_is_hearing_test(false);
        app_ha_hearing_test_deinit();

        app_report_event_broadcast(EVENT_HA_HEARING_TEST_STATUS_INFO, &status, 1);
    }
    else if (app_ha_get_bud_is_hearing_test() == true)
    {
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                            APP_REMOTE_MSG_HA_PROGRAM_RELAY_STOP_HEARING_TEST,
                                            NULL,
                                            0);

        app_report_event_broadcast(EVENT_HA_HEARING_TEST_STATUS_INFO, &status, 1);
    }
}

static void app_ha_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_HA_LISTENING_DELAY:
        {
            app_stop_timer(&timer_idx_ha_listening_delay);

#if F_APP_LISTENING_MODE_SUPPORT
            app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_IN_EAR);

#if F_APP_APT_SUPPORT
            if (app_apt_is_apt_on_state(app_db.current_listening_state))
            {
                current_apt_vol_level_cnt = 0;

                if (low_to_high_gain_level[current_apt_vol_level_cnt] <= app_cfg_nv.apt_volume_out_level)
                {
                    audio_passthrough_volume_out_set(low_to_high_gain_level[current_apt_vol_level_cnt]);
                    app_start_timer(&timer_idx_ha_apt_vol_level, "ha_apt_vol_level",
                                    ha_timer_id, APP_TIMER_HA_APT_VOL_LEVEL, 0, true,
                                    500);
                }
                else
                {
                    audio_passthrough_volume_out_set(app_cfg_nv.apt_volume_out_level);
                }
            }
#endif
#endif
        }
        break;

    case APP_TIMER_HA_APT_VOL_LEVEL:
        {
            current_apt_vol_level_cnt++;

            if (low_to_high_gain_level[current_apt_vol_level_cnt] <= app_cfg_nv.apt_volume_out_level)
            {
                audio_passthrough_volume_out_set(low_to_high_gain_level[current_apt_vol_level_cnt]);

                if (current_apt_vol_level_cnt >= 4)
                {
                    app_stop_timer(&timer_idx_ha_apt_vol_level);
                }
            }
            else
            {
                app_stop_timer(&timer_idx_ha_apt_vol_level);
            }
        }
        break;

    case APP_TIMER_HA_HEARING_TEST_TONE:
        {
            app_ha_hearing_test_stop();
            app_ha_set_is_hearing_test(false);
            app_ha_hearing_test_deinit();
        }
        break;

    default:
        break;
    }
}

#if F_APP_ERWS_SUPPORT
static uint16_t app_ha_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_HA, 0, NULL, true, total);
}

static void app_ha_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                               T_REMOTE_RELAY_STATUS status)
{
    switch (msg_type)
    {
    case APP_REMOTE_MSG_HA_PROGRAM_SET_ID:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_prog_set_selectable_idx(*(uint8_t *)buf);

                check_switch_listening_mode = true;

                app_ha_prog_load(ha_prog_db->selectable_prog_idx);

                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    uint8_t event_data[2];

                    event_data[0] = HA_PROG_OPCODE_SET_ID;
                    event_data[1] = ha_prog_db->selectable_prog_idx;
                    app_report_event_broadcast(EVENT_HA_PROGRAM_INFO, event_data, 2);
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_SET_OBJ:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_prog_set_object(*(uint8_t *)buf, *(uint8_t *)(buf + 1), buf + 2);
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_SET_ALL_OBJ:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t type = *(uint8_t *)buf;
                uint16_t type_len = app_ha_prog_get_object_size(type);
                uint8_t i = 0;

                for (i = 0; i < HA_PROG_NUM; i++)
                {
                    app_ha_prog_set_object(i, type, buf + 1 + (type_len * i));
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_GET_SPK_RESPONSE:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_get_spk_response(true);
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_RELAY_SPK_RESPONSE:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_report_event_broadcast(EVENT_HA_SPK_RESPONSE_INFO, buf, len);
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_GET_AUDIO_VOLUME:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_get_audio_volume();
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_RELAY_AUDIO_VOLUME:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_report_event_broadcast(EVENT_HA_AUDIO_VOLUME_INFO, buf, len);
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_RESET_OBJ:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t event_payload[2] = {0};
                static uint8_t reset_status = 0;

                if (len == 0) //sent from single / primary
                {
                    reset_status = (uint8_t)app_ha_prog_reset(ha_prog_db->selectable_prog_idx);

                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                    {
                        app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_PROGRAM_RESET_OBJ,
                                                           &reset_status, 1,
                                                           REMOTE_TIMER_HIGH_PRECISION, 0,
                                                           false);
                    }
                    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
                    {
                        event_payload[0] = HA_PROG_OPCODE_RESET_OBJ;
                        event_payload[1] = reset_status;
                        app_report_event_broadcast(EVENT_HA_PROGRAM_INFO, event_payload, 2);
                    }
                }
                else //sent from secondary
                {
                    event_payload[0] = HA_PROG_OPCODE_RESET_OBJ;
                    event_payload[1] = reset_status & buf[0];
                    app_report_event_broadcast(EVENT_HA_PROGRAM_INFO, event_payload, 2);
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_GET_ALL_DIRECTION_OBJ:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t prog_id = buf[0];
                uint8_t type = buf[2];
                uint16_t obj_size = app_ha_prog_get_object_size(type);
                uint16_t offset = 5;

                if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)
                {
                    offset += obj_size;
                }

                app_ha_prog_get_object(prog_id, type, buf + offset, &obj_size);

                if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) &&
                    (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
                {
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                        APP_REMOTE_MSG_HA_PROGRAM_GET_ALL_DIRECTION_OBJ,
                                                        buf, len);
                }
                else
                {
                    app_report_event_broadcast(EVENT_HA_PROGRAM_INFO, buf + 1, len - 1);
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOLUME:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint16_t size = 0;
                uint8_t remote_role = buf[2];
                uint8_t remote_volume = buf[0];
                uint8_t local_volume = 0;

                app_ha_prog_get_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, buf, &size);
                local_volume = buf[0];

                if (remote_volume < local_volume)
                {
                    buf[0] = remote_volume;
                    app_ha_prog_set_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, buf);
                }

                if (remote_role == REMOTE_SESSION_ROLE_PRIMARY) // sent from pri
                {
                    buf[2] = app_cfg_nv.bud_role;
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                        APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOLUME,
                                                        buf,
                                                        HA_PROG_OBJ_VOLUME_LEVEL_SIZE);
                }
                else // sent from sec
                {
                    uint8_t data[3] = {0};

                    data[0] = 0; //is_need_to_report
                    data[1] = 1; //is_sync
                    data[2] = 1; //is_need_to_apply
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                       APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOLUME_SYNC_STATUS,
                                                       data, 3,
                                                       REMOTE_TIMER_HIGH_PRECISION, 0,
                                                       true);
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOLUME_SYNC_STATUS:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t data_buf[HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS_SIZE] = {0};
                uint8_t event_buf[4 + 2 * HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS_SIZE] = {0};
                uint8_t status = 1;
                uint8_t is_need_to_report = buf[0];
                uint8_t is_sync = buf[1];
                uint8_t is_need_to_apply = buf[2];
                uint16_t size = 0;
                uint8_t volume_level = 0;
                uint8_t balance_level = 0;

                ha_prog_db->current_prog_is_sync_status = is_sync;
                app_ha_global_obj_get(HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS, data_buf);
                data_buf[ha_prog_db->selectable_prog_idx] = is_sync;
                app_ha_global_obj_set(HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS, data_buf);

                if (is_need_to_apply)
                {
                    app_ha_prog_get_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, data_buf, &size);
                    volume_level = data_buf[0];
                    balance_level = data_buf[1];

                    app_ha_generate_gain_db(volume_level, balance_level, data_buf);

                    if (app_apt_is_normal_apt_started())
                    {
                        audio_probe_dsp_send(data_buf, 12);
                    }
                }

                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    app_report_event_broadcast(EVENT_HA_VOLUME_SYNC_STATUS, &status, 1);

                    if (is_need_to_report)
                    {
                        app_report_event_broadcast(EVENT_HA_VOLUME_SYNC_STATUS_INFO, &is_sync, 1);
                    }

                    if (is_sync)
                    {
                        event_buf[0] = HA_PROG_OPCODE_GET_ALL_DIRECTION_OBJ;
                        event_buf[1] = HA_PROG_OBJ_VOLUME_LEVEL;
                        event_buf[2] = HA_PROG_OBJ_VOLUME_LEVEL_SIZE;
                        event_buf[3] = HA_PROG_OBJ_VOLUME_LEVEL_SIZE >> 8;

                        app_ha_prog_get_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, data_buf, &size);
                        memcpy(event_buf + 4, data_buf, HA_PROG_OBJ_VOLUME_LEVEL_SIZE);
                        memcpy(event_buf + 4 + HA_PROG_OBJ_VOLUME_LEVEL_SIZE, data_buf, HA_PROG_OBJ_VOLUME_LEVEL_SIZE);

                        app_report_event_broadcast(EVENT_HA_PROGRAM_INFO, event_buf,
                                                   4 + 2 * HA_PROG_OBJ_VOLUME_LEVEL_SIZE);
                    }
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_SET_HA_NR:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_prog_set_object_with_tool_data(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_NR, buf);

                audio_probe_dsp_send(buf, 12);
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_SET_HA_OVP:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_prog_set_object_with_tool_data(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_OVP, buf);
                audio_probe_dsp_send(buf, 12);
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_SET_HA_BF:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_prog_set_object_with_tool_data(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_BF, buf);
                audio_probe_dsp_send(buf, 12);
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_GET_HA_VOL:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) &&
                    (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
                {
                    uint8_t prog_id = buf[0];
                    uint16_t tool_data_size = app_ha_prog_get_object_size(HA_PROG_OBJ_TOOL_DATA);
                    uint16_t offset = 2;
                    uint8_t *tool_data = calloc(tool_data_size, 1);

                    app_ha_prog_get_object(prog_id, HA_PROG_OBJ_TOOL_DATA, tool_data, &tool_data_size);

                    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)
                    {
                        offset += 1;
                    }

                    memcpy(buf + offset, tool_data + 2 + app_cfg_const.bud_side, 1); //set curr bud's volume

                    free(tool_data);
                    tool_data = NULL;

                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_PROGRAM_GET_HA_VOL, buf,
                                                        len);
                }
                else
                {
                    app_report_event_broadcast(EVENT_HA_PROGRAM_VOL_STATUS_INFO, buf + 1, len - 1);
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOL:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t volume_level = buf[app_cfg_const.bud_side];
                uint8_t balance_level = buf[2];
                uint8_t cmd_buf[12] = {0};
                uint16_t tool_data_size = app_ha_prog_get_object_size(HA_PROG_OBJ_TOOL_DATA);
                uint16_t offset = 2;
                uint8_t *tool_data = calloc(tool_data_size, 1);

                app_ha_generate_gain_db(volume_level, balance_level, cmd_buf);
                app_ha_prog_set_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, cmd_buf);

                app_ha_prog_get_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_TOOL_DATA, tool_data,
                                       &tool_data_size);
                tool_data[offset] = buf[0]; //L_level
                tool_data[offset + 1] = buf[1]; //R_level

                offset = 4 + tool_data[1] * 2;
                tool_data[offset] = balance_level;

                app_ha_prog_set_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_TOOL_DATA, tool_data);

                free(tool_data);
                tool_data = NULL;

                audio_probe_dsp_send(cmd_buf, 12);
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_STOP_A2DP_AUDIO_TRACK:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t app_idx = app_a2dp_get_active_idx();

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_avrcp_pause(app_db.br_link[app_idx].bd_addr);
                }

                if (audio_track_release(app_db.br_link[app_idx].a2dp_track_handle) == false)
                {
                    if (app_ha_get_is_hearing_test())
                    {
                        app_ha_hearing_test_start();
                    }
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_DISABLE_AUDIO_EQ_START_HEARING_TEST:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (app_ha_get_is_hearing_test())
                {
                    app_ha_hearing_test_start();
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_RELAY_HEARING_TEST_STATUS:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                bud_is_hearing_test = buf[0];
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_RELAY_STOP_HEARING_TEST:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_hearing_test_stop();
                app_ha_set_is_hearing_test(false);
                app_ha_hearing_test_deinit();
            }
        }
        break;

    case APP_REMOTE_MSG_HA_PROGRAM_RELAY_HA_VOLUME_STATUS:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t volume_buf[HA_PROG_OBJ_VOLUME_LEVEL_SIZE] = {0};
                uint16_t size = 0;

                app_ha_prog_get_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, volume_buf,
                                       &size);

                //need to update both earbuds' volume sync status to not sync
                if (buf[0] != volume_buf[0])
                {
                    uint8_t data[3] = {0};

                    data[0] = 1; //is_need_to_report
                    data[1] = 0; //is_sync
                    data[2] = 0; //is_need_to_apply
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                       APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOLUME_SYNC_STATUS,
                                                       data, 3,
                                                       REMOTE_TIMER_HIGH_PRECISION, 0,
                                                       true);
                }
            }
        }
        break;

    default:
        break;
    }
}
#endif

void app_ha_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                       uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_HA_ACCESS_PROGRAM:
        {
            switch (cmd_ptr[2])
            {
            case HA_PROG_OPCODE_GET_NUM:
                {
                    uint8_t event_data[2];

                    event_data[0] = cmd_ptr[2];
                    event_data[1] = ha_prog_db->prog_num;

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_HA_PROGRAM_INFO, app_idx,
                                     event_data, sizeof(uint8_t) * 2);
                }
                break;

            case HA_PROG_OPCODE_GET_ID:
                {
                    uint8_t event_data[2];

                    event_data[0] = cmd_ptr[2];
                    event_data[1] = ha_prog_db->selectable_prog_idx;

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_HA_PROGRAM_INFO, app_idx,
                                     event_data, sizeof(uint8_t) * 2);
                }
                break;

            case HA_PROG_OPCODE_SET_ID:
                {
                    if (cmd_ptr[3] < ha_prog_db->prog_num)
                    {
                        if (cmd_ptr[3] != ha_prog_db->selectable_prog_idx)
                        {
                            if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                            {
                                uint8_t event_data[2];

                                app_ha_prog_set_selectable_idx(cmd_ptr[3]);

                                check_switch_listening_mode = true;

                                app_ha_prog_load(ha_prog_db->selectable_prog_idx);

                                event_data[0] = cmd_ptr[2];
                                event_data[1] = ha_prog_db->selectable_prog_idx;
                                app_report_event_broadcast(EVENT_HA_PROGRAM_INFO, event_data, 2);
                            }
                            else
                            {
                                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_PROGRAM_SET_ID,
                                                                   cmd_ptr + 3, 1,
                                                                   REMOTE_TIMER_HIGH_PRECISION, 0,
                                                                   true);
                            }
                        }
                        else
                        {
                            uint8_t event_data[2];

                            event_data[0] = cmd_ptr[2];
                            event_data[1] = ha_prog_db->selectable_prog_idx;
                            app_report_event_broadcast(EVENT_HA_PROGRAM_INFO, event_data, 2);
                        }

                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                }
                break;

            case HA_PROG_OPCODE_GET_OBJ:
                {
                    uint8_t prog_id = cmd_ptr[3];
                    uint8_t type = cmd_ptr[4];

                    if (prog_id < ha_prog_db->prog_num)
                    {
                        uint16_t len = 3 + app_ha_prog_get_object_size(type);
                        uint8_t *event_data = malloc(sizeof(uint8_t) * len);
                        uint16_t size = 0;

                        memset(event_data, 0, len);

                        event_data[0] = cmd_ptr[2];
                        event_data[1] = len - 3;
                        event_data[2] = (len - 3) >> 8;
                        app_ha_prog_get_object(prog_id, type, event_data + 3, &size);

                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        app_report_event(cmd_path, EVENT_HA_PROGRAM_INFO, app_idx,
                                         event_data, sizeof(uint8_t) * len);
                        free(event_data);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                }
                break;

            case HA_PROG_OPCODE_SET_OBJ:
                {
                    uint8_t prog_id = cmd_ptr[3];
                    uint8_t type = cmd_ptr[4];

                    if (prog_id < ha_prog_db->prog_num)
                    {
                        uint16_t type_len = app_ha_prog_get_object_size(type);

                        if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                        {
                            app_ha_prog_set_object(prog_id, type, cmd_ptr + 5);
                        }
                        else
                        {
                            app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_PROGRAM_SET_OBJ,
                                                               cmd_ptr + 3, type_len + 2,
                                                               REMOTE_TIMER_HIGH_PRECISION, 0,
                                                               true);
                        }

                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                }
                break;

            case HA_PROG_OPCODE_GET_ALL_OBJ:
                {
                    uint8_t type = cmd_ptr[3];
                    uint16_t type_len = app_ha_prog_get_object_size(type);
                    uint16_t event_len = 3 + (type_len * HA_PROG_NUM);
                    uint8_t *event_data = calloc(event_len, sizeof(uint8_t));
                    uint16_t size = 0;
                    uint8_t i = 0;

                    event_data[0] = cmd_ptr[2];
                    event_data[1] = event_len - 3;
                    event_data[2] = (event_len - 3) >> 8;

                    for (i = 0; i < HA_PROG_NUM; i++)
                    {
                        app_ha_prog_get_object(i, type, event_data + 3 + (type_len * i), &size);
                    }

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_HA_PROGRAM_INFO, app_idx, event_data, sizeof(uint8_t) * event_len);
                    free(event_data);
                }
                break;

            case HA_PROG_OPCODE_SET_ALL_OBJ:
                {
                    uint8_t type = cmd_ptr[3];
                    uint16_t type_len = app_ha_prog_get_object_size(type);
                    uint8_t i = 0;

                    if ((cmd_len - 4) == (type_len * HA_PROG_NUM))
                    {
                        if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                        {
                            for (i = 0; i < HA_PROG_NUM; i++)
                            {
                                app_ha_prog_set_object(i, type, cmd_ptr + 4 + (type_len * i));
                            }
                        }
                        else
                        {
                            app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_PROGRAM_SET_ALL_OBJ,
                                                               cmd_ptr + 3, (type_len * HA_PROG_NUM) + 1,
                                                               REMOTE_TIMER_HIGH_PRECISION, 0,
                                                               true);
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    }

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
                break;

            case HA_PROG_OPCODE_RESET_OBJ:
                {
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_PROGRAM_RESET_OBJ,
                                                           NULL, 0,
                                                           REMOTE_TIMER_HIGH_PRECISION, 0,
                                                           true);
                    }
                    else
                    {
                        uint8_t event_data[2] = {0};
                        uint16_t event_len = 2;

                        event_data[0] = cmd_ptr[2];
                        event_data[1] = (uint8_t)app_ha_prog_reset(ha_prog_db->selectable_prog_idx);
                        app_report_event(cmd_path, EVENT_HA_PROGRAM_INFO, app_idx, event_data, event_len);
                    }
                }
                break;

            case HA_PROG_OPCODE_GET_ALL_DIRECTION_OBJ:
                {
                    uint8_t prog_id = cmd_ptr[3];
                    uint8_t type = cmd_ptr[4];

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                    app_ha_get_all_direction_obj(prog_id, type);
                }
                break;
            }
        }
        break;

    case CMD_HA_SPK_RESPONSE:
        {
            if (cmd_len < 3)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t direction = cmd_ptr[2];

                if (direction <= DEVICE_BUD_SIDE_RIGHT)
                {
                    if (direction == app_cfg_const.bud_side)
                    {
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        app_ha_get_spk_response(false);
                    }
                    else if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                            APP_REMOTE_MSG_HA_PROGRAM_GET_SPK_RESPONSE,
                                                            NULL,
                                                            0);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
            }
        }
        break;

    case CMD_HA_AUDIO_VOLUME_GET:
        {
            if (cmd_len < 3)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t direction = cmd_ptr[2];

                if (direction <= DEVICE_BUD_SIDE_RIGHT)
                {
                    if (direction == app_cfg_const.bud_side)
                    {
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        app_ha_get_audio_volume();
                    }
                    else if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                            APP_REMOTE_MSG_HA_PROGRAM_GET_AUDIO_VOLUME,
                                                            NULL,
                                                            0);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
            }
        }
        break;

    case CMD_HA_VOLUME_SYNC_STATUS:
        {
            if (cmd_len < 3)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t buf[HA_PROG_OBJ_VOLUME_LEVEL_SIZE] = {0};
                uint8_t status = 0;
                uint16_t size = 0;

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
                    if (cmd_ptr[2]) // if is_sync is enabled, need to sync minimun HA volume
                    {
                        app_ha_prog_get_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, buf, &size);
                        buf[2] = app_cfg_nv.bud_role;
                        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                            APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOLUME,
                                                            buf,
                                                            HA_PROG_OBJ_VOLUME_LEVEL_SIZE);
                    }
                    else
                    {
                        uint8_t data[3] = {0};

                        data[0] = 0; //is_need_to_report
                        data[1] = cmd_ptr[2]; //is_sync
                        data[2] = 0; //is_need_to_apply
                        app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                           APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOLUME_SYNC_STATUS,
                                                           data, 3,
                                                           REMOTE_TIMER_HIGH_PRECISION, 0,
                                                           true);
                    }
                }
                else
                {
                    status = 0;
                    app_report_event(cmd_path, EVENT_HA_VOLUME_SYNC_STATUS, app_idx, &status, 1);
                }
            }
        }
        break;

    case CMD_HA_VOLUME_SYNC_STATUS_GET:
        {
            if (cmd_len < 3)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t prog_id = cmd_ptr[2];
                uint8_t is_sync = 0;

                if (prog_id >= HA_PROG_NUM)
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
                else
                {
                    uint8_t buf[HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS_SIZE] = {0};

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                    app_ha_global_obj_get(HA_GLOBAL_OBJ_VOLUME_SYNC_STATUS, buf);
                    is_sync = buf[prog_id];

                    app_report_event(cmd_path, EVENT_HA_VOLUME_SYNC_STATUS_INFO, app_idx, &is_sync, 1);
                }
            }
        }
        break;

    case CMD_HA_PROGRAM_NR_GET:
        {
            if (cmd_len < 3)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t prog_id = cmd_ptr[2];
                uint8_t obj_type = HA_PROG_OBJ_NR;
                uint8_t *data = calloc(4, 1);

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_ha_prog_get_object_status(prog_id, obj_type, data);
                app_report_event(cmd_path, EVENT_HA_PROGRAM_NR_STATUS_INFO, app_idx, data, 4);

                free(data);
            }
        }
        break;

    case CMD_HA_PROGRAM_NR_SET:
        {
            if (cmd_len < 4)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t is_enabled = cmd_ptr[2];
                uint8_t level = cmd_ptr[3];
                uint8_t cmd_buf[12] = {0};

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                app_ha_generate_nr_cmd(is_enabled, level, cmd_buf);

                if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE) &&
                    (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
                {
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                       APP_REMOTE_MSG_HA_PROGRAM_SET_HA_NR,
                                                       cmd_buf, 12,
                                                       REMOTE_TIMER_HIGH_PRECISION, 0,
                                                       true);
                }
                else
                {
                    app_ha_prog_set_object_with_tool_data(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_NR, cmd_buf);
                    audio_probe_dsp_send(cmd_buf, 12);
                }
            }
        }
        break;

    case CMD_HA_PROGRAM_OVP_GET:
        {
            if (cmd_len < 3)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t prog_id = cmd_ptr[2];
                uint8_t obj_type = HA_PROG_OBJ_OVP;
                uint8_t *data = calloc(4, 1);

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_ha_prog_get_object_status(prog_id, obj_type, data);
                app_report_event(cmd_path, EVENT_HA_PROGRAM_NR_STATUS_INFO, app_idx, data, 4);

                free(data);
            }
        }
        break;

    case CMD_HA_PROGRAM_OVP_SET:
        {
            if (cmd_len < 4)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t is_enabled = cmd_ptr[2];
                uint8_t level = cmd_ptr[3];
                uint8_t cmd_buf[12] = {0};

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                app_ha_generate_ovp_cmd(is_enabled, level, cmd_buf);

                if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE) &&
                    (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
                {
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                       APP_REMOTE_MSG_HA_PROGRAM_SET_HA_OVP,
                                                       cmd_buf, 12,
                                                       REMOTE_TIMER_HIGH_PRECISION, 0,
                                                       true);
                }
                else
                {
                    app_ha_prog_set_object_with_tool_data(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_OVP, cmd_buf);
                    audio_probe_dsp_send(cmd_buf, 12);
                }
            }
        }
        break;

    case CMD_HA_PROGRAM_BF_GET:
        {
            if (cmd_len < 3)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t prog_id = cmd_ptr[2];
                uint8_t obj_type = HA_PROG_OBJ_BF;
                uint8_t *data = calloc(4, 1);

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_ha_prog_get_object_status(prog_id, obj_type, data);
                app_report_event(cmd_path, EVENT_HA_PROGRAM_BF_STATUS_INFO, app_idx, data, 4);

                free(data);
            }
        }
        break;

    case CMD_HA_PROGRAM_BF_SET:
        {
            if (cmd_len < 3)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t is_enabled = cmd_ptr[2];
                uint8_t cmd_buf[12] = {0};

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                app_ha_generate_bf_cmd(is_enabled, cmd_buf);

                if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE) &&
                    (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
                {
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                       APP_REMOTE_MSG_HA_PROGRAM_SET_HA_BF,
                                                       cmd_buf, 12,
                                                       REMOTE_TIMER_HIGH_PRECISION, 0,
                                                       true);
                }
                else
                {
                    app_ha_prog_set_object_with_tool_data(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_BF, cmd_buf);
                    audio_probe_dsp_send(cmd_buf, 12);
                }
            }
        }
        break;

    case CMD_HA_PROGRAM_VOL_GET:
        {
            if (cmd_len < 3)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t prog_id = cmd_ptr[2];

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                app_ha_get_all_direction_obj_status(prog_id, HA_PROG_OBJ_VOLUME_LEVEL);
            }
        }
        break;

    case CMD_HA_PROGRAM_VOL_SET:
        {
            if (cmd_len < 5)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE) &&
                    (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
                {
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                       APP_REMOTE_MSG_HA_PROGRAM_SET_HA_VOL,
                                                       cmd_ptr + 2, 3,
                                                       REMOTE_TIMER_HIGH_PRECISION, 0,
                                                       true);
                }
                else
                {
                    uint8_t volume_level = cmd_ptr[2 + app_cfg_const.bud_side];
                    uint8_t balance_level = cmd_ptr[4];
                    uint8_t cmd_buf[12] = {0};
                    uint16_t tool_data_size = app_ha_prog_get_object_size(HA_PROG_OBJ_TOOL_DATA);
                    uint16_t offset = 2;
                    uint8_t *tool_data = calloc(tool_data_size, 1);

                    app_ha_generate_gain_db(volume_level, balance_level, cmd_buf);
                    app_ha_prog_set_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_VOLUME_LEVEL, cmd_buf);

                    app_ha_prog_get_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_TOOL_DATA, tool_data,
                                           &tool_data_size);
                    tool_data[offset] = cmd_ptr[2]; //L_level
                    tool_data[offset + 1] = cmd_ptr[3]; //R_level

                    offset = 4 + tool_data[1] * 2;
                    tool_data[offset] = balance_level;

                    app_ha_prog_set_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_TOOL_DATA, tool_data);

                    free(tool_data);
                    tool_data = NULL;

                    audio_probe_dsp_send(cmd_buf, 12);
                }
            }
        }
        break;

    case CMD_HA_GET_DSP_CFG_GAIN:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_ha_get_dsp_cfg_gain();
        }
        break;

    case CMD_MP_HA_BYPASS_ALGORITHM:
        {
            if (app_apt_is_normal_apt_started())
            {
                uint8_t data_buf[12];

                memset(data_buf, 0, 12);
                data_buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                data_buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                data_buf[2] = 2;
                data_buf[4] = 8; //HA test
                data_buf[8] = 1; //HA test
                data_buf[10] = 0; //outputgain L
                data_buf[11] = 0; //outputgain H
                audio_probe_dsp_send(data_buf, 12);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_MP_HA_ENABLE_ALGORITHM:
        {
            if (app_apt_is_normal_apt_started())
            {
                uint8_t data_buf[12];

                memset(data_buf, 0, 12);
                data_buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                data_buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                data_buf[2] = 2;
                data_buf[4] = 8; //HA test
                data_buf[8] = 0; //exit HA test
                data_buf[10] = 0; //outputgain L
                data_buf[11] = 0; //outputgain H
                audio_probe_dsp_send(data_buf, 12);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_MP_HA_SET_GRAPHIC_EQ:
        {
            if (app_apt_is_normal_apt_started())
            {
                uint8_t *data_buf = NULL;
                uint8_t graphic_eq_len = 132;

                if (cmd_len - 2 != graphic_eq_len)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                }
                else
                {
                    uint8_t payload_len = ((graphic_eq_len + 3) / 4) * 4 + 8;
                    data_buf = malloc(payload_len);

                    if (data_buf)
                    {
                        memset(data_buf, 0, payload_len);
                        data_buf[0] = AUDIO_PROBE_HA_PARA & 0xFF;
                        data_buf[1] = (AUDIO_PROBE_HA_PARA & 0xFF00) >> 8;
                        data_buf[2] = (((graphic_eq_len + 4) + 3) / 4) & 0xFF;
                        data_buf[3] = ((((graphic_eq_len + 4) + 3) / 4) & 0xFF00) >> 8;
                        data_buf[4] = 7; //HA test
                        memcpy(&data_buf[8], &cmd_ptr[2], graphic_eq_len);
                        audio_probe_dsp_send(data_buf, payload_len);
                        free(data_buf);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_MP_HA_SET_FULL_ON_GAIN:
        {
            if (app_apt_is_normal_apt_started())
            {
                uint8_t data_buf[12];
                memset(data_buf, 0, 12);
                app_ha_generate_gain_db(100, 50, data_buf);
                audio_probe_dsp_send(data_buf, 12);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    default:
        break;
    }
}

static void app_ha_handle_effect_payload(uint8_t *buf)
{
    uint8_t effect_type = buf[0];
    uint16_t tool_data_len = buf[1] + (buf[2] << 8);
    uint16_t effect_l_len = buf[3] + (buf[4] << 8);
    uint16_t effect_r_len = buf[5] + (buf[6] << 8);
    uint16_t effect_offset = 0;
    uint16_t effect_len = 0;

    switch (effect_type)
    {
    case HA_EFFECT_WDRC:
    case HA_EFFECT_NR:
    case HA_EFFECT_GRAPHIC_EQ:
    case HA_EFFECT_FBC:
    case HA_EFFECT_OVP:
    case HA_EFFECT_BF:
    case HA_EFFECT_WNR:
    case HA_EFFECT_INR:
    case HA_EFFECT_RNS:
    case HA_EFFECT_OUTPUT_DRC:
        {
            //effect tool_data set
            app_ha_prog_set_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_TOOL_DATA, buf + 7);

            if (effect_l_len > 0 && (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT))
            {
                effect_offset = 7 + tool_data_len;
                effect_len = effect_l_len;
            }
            else if (effect_r_len > 0 && (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT))
            {
                effect_offset = 7 + tool_data_len + effect_l_len;
                effect_len = effect_r_len;
            }

            if (effect_len > 0)
            {
                switch (effect_type)
                {
                case HA_EFFECT_WDRC:
                case HA_EFFECT_GRAPHIC_EQ:
                case HA_EFFECT_FBC:
                case HA_EFFECT_OVP:
                case HA_EFFECT_BF:
                case HA_EFFECT_WNR:
                case HA_EFFECT_INR:
                case HA_EFFECT_OUTPUT_DRC:
                    {
                        app_ha_effect_set((T_APP_HA_EFFECT)effect_type, buf + effect_offset, effect_len);
                        app_ha_prog_set_object(ha_prog_db->selectable_prog_idx,
                                               HA_EFFECT_TO_PROG_OBJ(effect_type),
                                               buf + effect_offset);
                        app_ha_effect_apply((T_APP_HA_EFFECT)effect_type);
                    }
                    break;

                case HA_EFFECT_NR:
                    {
                        app_ha_effect_set((T_APP_HA_EFFECT)effect_type, buf + effect_offset, effect_len);
                        app_ha_prog_set_object(ha_prog_db->selectable_prog_idx,
                                               HA_EFFECT_TO_PROG_OBJ(effect_type),
                                               buf + effect_offset);
                        app_ha_effect_apply((T_APP_HA_EFFECT)effect_type);
                        app_ha_effect_apply(HA_EFFECT_WDRC);
                        app_ha_effect_apply(HA_EFFECT_GRAPHIC_EQ);
                    }
                    break;

                case HA_EFFECT_RNS:
                    {
                        app_ha_effect_set((T_APP_HA_EFFECT)effect_type, buf + effect_offset, effect_len);
                        app_ha_prog_set_object(ha_prog_db->selectable_prog_idx,
                                               HA_EFFECT_TO_PROG_OBJ(effect_type),
                                               buf + effect_offset);

                        if (!app_ha_check_switch_listening_mode(ha_prog_db->selectable_prog_idx))
                        {
                            app_ha_effect_apply((T_APP_HA_EFFECT)effect_type);
                        }
                    }
                    break;
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_ha_handle_data_payload(uint8_t *buf)
{
    uint8_t prog_obj_type = buf[0];
    uint16_t tool_data_len  = buf[1] + (buf[2] << 8);
    uint16_t data_l_len   = buf[3] + (buf[4] << 8);
    uint16_t data_r_len   = buf[5] + (buf[6] << 8);
    uint16_t data_offset = 0;
    uint16_t data_len = 0;

    switch (prog_obj_type)
    {
    case HA_PROG_OBJ_VOLUME_LEVEL:
        {
            //effect tool_data set
            app_ha_prog_set_object(ha_prog_db->selectable_prog_idx, HA_PROG_OBJ_TOOL_DATA, buf + 7);

            if (data_l_len > 0 && (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT))
            {
                data_offset = 7 + tool_data_len;
                data_len = data_l_len;
            }
            else if (data_r_len > 0 && (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT))
            {
                data_offset = 7 + tool_data_len + data_l_len;
                data_len = data_r_len;
            }

            if (data_len > 0)
            {
                uint8_t data_buf[12] = {0};
                uint8_t volume_level = buf[data_offset];
                uint8_t balance_level = buf[data_offset + 1];

                app_ha_generate_gain_db(volume_level, balance_level, data_buf);

                if (app_apt_is_normal_apt_started())
                {
                    audio_probe_dsp_send(data_buf, 12);
                }

                app_ha_prog_set_object(ha_prog_db->selectable_prog_idx, prog_obj_type,
                                       buf + data_offset);
            }
        }
        break;

    default:
        break;
    }
}

void app_ha_handle_hearing_test_payload(uint8_t *buf, uint16_t len)
{
    switch (buf[0])
    {
    case HA_HEARING_TEST_OPCODE_CREATE:
        {
            app_ha_hearing_test_init();
        }
        break;

    case HA_HEARING_TEST_OPCODE_START:
        {
            uint8_t app_idx = app_a2dp_get_active_idx();

            app_ha_set_is_hearing_test(false);

            if (app_db.br_link[app_idx].a2dp_track_handle)
            {
                app_ha_set_is_hearing_test(true);

                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    bt_avrcp_pause(app_db.br_link[app_idx].bd_addr);

                    if (audio_track_release(app_db.br_link[app_idx].a2dp_track_handle) == false)
                    {
                        if (app_ha_get_is_hearing_test())
                        {
                            app_ha_hearing_test_start();
                        }
                    }
                }
                else
                {
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                       APP_REMOTE_MSG_HA_PROGRAM_STOP_A2DP_AUDIO_TRACK,
                                                       NULL, 0,
                                                       REMOTE_TIMER_HIGH_PRECISION, 0,
                                                       true);
                }
            }
            else
            {
                app_ha_set_is_hearing_test(true);

                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_ha_hearing_test_start();
                }
                else
                {
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HA,
                                                       APP_REMOTE_MSG_HA_PROGRAM_DISABLE_AUDIO_EQ_START_HEARING_TEST,
                                                       NULL, 0,
                                                       REMOTE_TIMER_HIGH_PRECISION, 0,
                                                       true);
                }
            }
        }
        break;

    case HA_HEARING_TEST_OPCODE_STOP:
        {
            app_ha_hearing_test_stop();
            app_ha_set_is_hearing_test(false);
            app_stop_timer(&timer_idx_ha_hearing_test_tone);
        }
        break;

    case HA_HEARING_TEST_OPCODE_SET:
        {
            uint16_t freq = buf[1] + (buf[2] << 8);
            int16_t gain = buf[3] + (buf[4] << 8);
            uint16_t timeout_ms = 0;

            if (gain > HA_HEARING_TEST_TONE_MAX_GAIN)
            {
                break;
            }

            if (len == 7)
            {
                timeout_ms = buf[5] + (buf[6] << 8);
            }

            app_ha_hearing_test_tone_set((uint32_t)freq, (int32_t)gain, timeout_ms);
        }
        break;

    case HA_HEARING_TEST_OPCODE_DESTROY:
        {
            app_ha_hearing_test_deinit();
        }
        break;
    }
}

static void app_ha_handle_global_payload(uint8_t *data_buf)
{
    uint8_t target_obj_type = data_buf[0];
    uint16_t target_obj_size_l = data_buf[1] + (data_buf[2] << 8);
    uint16_t target_obj_size_r = data_buf[3] + (data_buf[4] << 8);
    uint8_t obj_type = 0;
    uint16_t obj_size = 0;
    uint32_t base = app_ha_get_global_obj_base();
    uint32_t offset = 0;
    uint8_t buf[4] = {0};
    uint8_t i = 0;

    if ((app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT && target_obj_size_l == 0) ||
        (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT && target_obj_size_r == 0))
    {
        return;
    }

    data_buf += 5; //target to data_L

    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)
    {
        data_buf += target_obj_size_l; //target to data_R
    }

    for (i = 0; i < HA_GLOBAL_OBJ_TOTAL; i++)
    {
        obj_type = ha_global_feature_table[i];
        obj_size = ha_global_obj_size_table[obj_type];

        if (obj_type == i)
        {
            break;
        }

        offset += (4 + obj_size);
    }

    ha_ext_ftl_load_data(buf, base + offset, 4);
    buf[1] = 1; //enable is_set column
    ha_ext_ftl_save_data(buf, base + offset, 4);
    ha_ext_ftl_save_data(data_buf, base + offset + 4, obj_size);

    switch (target_obj_type)
    {
    case HA_GLOBAL_OBJ_WDRC:
        {
            if (app_apt_is_normal_apt_started())
            {
                audio_probe_dsp_send(data_buf, ha_global_obj_size_table[obj_type]);
            }
        }
        break;

    default:
        {

        }
        break;
    }
}

void app_ha_cmd_cback(uint8_t msg_type, uint8_t *buf, uint16_t len)
{
    switch (msg_type)
    {
    case APP_HA_CMD_MSG_SET_EFFECT_CMD:
        {
            app_ha_handle_effect_payload(buf);
        }
        break;

    case APP_HA_DATA_MSG_SET_PARAMS:
        {
            app_ha_handle_data_payload(buf);
        }
        break;

    case APP_HA_HEARING_TEST_MSG_SET:
        {
            app_ha_handle_hearing_test_payload(buf, len);
        }
        break;

    case APP_HA_GLOBAL_OBJ_MSG_SET:
        {
            app_ha_handle_global_payload(buf);
        }
        break;

    default:
        break;
    }

    return;
}

static void app_ha_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_HFP_CALL_STATUS:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->hfp_call_status.bd_addr);

            if (p_link != NULL)
            {
                switch (param->hfp_call_status.curr_status)
                {
                case BT_HFP_CALL_INCOMING:
                    {
                        app_ha_check_trigger_stop_hearing_test(1);
                    }
                    break;

                default:
                    {

                    }
                    break;
                }
            }
        }
        break;

    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->remote_conn_cmpl.bd_addr);

            if (p_link != NULL)
            {
                app_ha_sync_volume_sync();
            }
        }
        break;

    default:
        {

        }
        break;
    }
}

void app_ha_init()
{
    ha_effect_db = calloc(1, sizeof(T_HA_EFFECT_DB));
    ha_effect_db->queue = calloc(1, sizeof(T_OS_QUEUE));
    os_queue_init(ha_effect_db->queue);

    ha_prog_db = calloc(1, sizeof(T_HA_PROG_DB));
    ha_prog_db->obj_queue = calloc(1, sizeof(T_OS_QUEUE));
    os_queue_init(ha_prog_db->obj_queue);

    os_queue_init(&ha_params_db.obj_queue);

    app_ha_effect_set_scheme();

    app_ha_params_init();

    ha_ext_ftl_storage_init();

    app_ha_prog_init();

    app_ha_global_obj_init();

    app_timer_reg_cb(app_ha_timeout_cb, &ha_timer_id);
#if F_APP_ERWS_SUPPORT
    app_relay_cback_register(app_ha_relay_cback, app_ha_parse_cback,
                             APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_TOTAL);
#endif
    app_cmd_cback_register(app_ha_cmd_cback, APP_CMD_MODULE_TYPE_HA);
    bt_mgr_cback_register(app_ha_bt_cback);
    audio_mgr_cback_register(app_ha_audio_cback);
    app_ha_effect_apply_cback_register(app_ha_effect_apply_cback);
}

#endif
