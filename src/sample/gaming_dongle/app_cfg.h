/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <os_mem.h>
#include "trace.h"

#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

//latency measure config
//#define APP_LATENCY_MEASURE_PIN_REUSE           //config pin reuse
#ifdef APP_LATENCY_MEASURE_PIN_REUSE

#endif

#define RTL8763EAU_VER_MAJOR          0
#define RTL8763EAU_VER_MINOR          20
#define RTL8763EAU_DBG_TITLE        ""


#define DEVICE_CUSTOMER_NAME_AUDIO_ONLY     "Microsoft Surface USB Link" //MAX 30 + 1 for \0
#define DEVICE_CUSTOMER_NAME_AUDIO_ONLY_BT  "U S B Link" //MAX 30 + 1 for \0
#define DEVICE_CUSTOMER_NAME_AUDIO_HID      "Microsoft Surface USB Link" //MAX 30 + 1 for \0

#define DONGLE_AUDIO_VOLUME_SYNC 1

//#define SBC_SW_DECODER_DEBUG_ENABLE
#ifdef SBC_SW_DECODER_DEBUG_ENABLE
#define SBC_NO_LINK_ENABLE      1
#define SBC_NO_USB_ENABLE      1
#define SBC_NO_AUX_ENABLE       1
#define SBC_SW_DECODE_ENABLE    1
#endif

#define F_APP_A2DP_CODEC_LC3_SUPPORT  1

#define APP_LATENCY_MEASURE_CIS_DS              //measure cis ds latency
#ifdef APP_LATENCY_MEASURE_CIS_DS
extern uint8_t first_track_start;
extern uint8_t first_enc_data_ind;
extern uint8_t first_iso_send;
extern uint8_t first_gap_send;
extern uint8_t first_end_to_air;

#define TRACK_START_PIN     P1_7
#define ENC_DATA_IND_PIN    P1_7
#define ISO_SEND_PIN        P1_7
#define GAP_SEND_PIN        P1_7
#define SEND_TO_AIR_PIN     P1_7
#endif

//App config data
#define APP_DATA_SYNC_WORD      0xAA55
#define APP_DATA_SYNC_WORD_LEN  2
#define APP_DATA_TONE_NUM_LEN   2

#define DATA_SYNC_WORD          0xAA55AA55
#define DATA_SYNC_WORD_LEN      4

#define APP_CONFIG_OFFSET       (0)
#define APP_CONFIG_SIZE         1024

#define APP_LED_OFFSET          (APP_CONFIG_OFFSET + APP_CONFIG_SIZE)
#define APP_LED_SIZE            512

#define SYS_CONFIG_OFFSET       (APP_LED_OFFSET + APP_LED_SIZE)
#define SYS_CONFIG_SIZE         1024

#define EXTEND_APP_CONFIG_OFFSET  (SYS_CONFIG_OFFSET + SYS_CONFIG_SIZE)
#define EXTEND_APP_CONFIG_SIZE    128

#define TONE_DATA_OFFSET        (4096)//Rsv 4K for APP parameter for better flash control
#define TONE_DATA_SIZE          3072
#define VOICE_PROMPT_OFFSET     (TONE_DATA_OFFSET + TONE_DATA_SIZE)
#define VOICE_PROMPT_SIZE       137 * 1024

//FTL
#define APP_RW_DATA_ADDR        3072
#define APP_RW_DATA_SIZE        360

#define FACTORY_RESET_OFFSET    140

#define APP_CUSTOMIZED_ADP_ADV_COUNT_FLASH_OFFSET   (APP_RW_DATA_ADDR + APP_RW_DATA_SIZE)
#define APP_CUSTOMIZED_ADP_ADV_COUNT_SIZE           4

#define APP_RW_PX318J_ADDR                  (APP_CUSTOMIZED_ADP_ADV_COUNT_FLASH_OFFSET + APP_CUSTOMIZED_ADP_ADV_COUNT_SIZE)
#define APP_RW_PX318J_SIZE                  12

#define APP_RW_JSA_ADDR                     (APP_RW_PX318J_ADDR + APP_RW_PX318J_SIZE)
#define APP_RW_JSA_SIZE                     12

#define APP_RW_HA_OPTION_ADDR               (APP_RW_JSA_ADDR + APP_RW_JSA_SIZE)
#define APP_RW_HA_OPTION_SIZE               4

#define APP_RW_HA_1_ADDR                    (APP_RW_HA_OPTION_ADDR + APP_RW_HA_OPTION_SIZE)
#define APP_RW_HA_1_SIZE                    612

#define APP_RW_HA_2_ADDR                    (APP_RW_HA_1_ADDR + APP_RW_HA_1_SIZE)
#define APP_RW_HA_2_SIZE                    612

#define APP_RW_AVP_ID_DATA_ADDR             (APP_RW_HA_2_ADDR + APP_RW_HA_2_SIZE)
#define APP_RW_AVP_ID_DATA_SIZE             36

#define TILE_APPDATA_BANK_A_ADDR            (APP_RW_AVP_ID_DATA_ADDR + APP_RW_AVP_ID_DATA_SIZE)
#define TILE_STORAGE_SIZE                   28

#define TILE_LINK_BUD_ADDR                  (TILE_APPDATA_BANK_A_ADDR + TILE_STORAGE_SIZE)
#define TILE_ADDR_SIZE                      8

#define TILE_LINK_BUD_LE_ADDR               (TILE_LINK_BUD_ADDR + TILE_ADDR_SIZE)
#define TILE_LE_ADDR_SIZE                   8

#define APP_RW_KEY_REMAP_INFO_ADDR          (TILE_LINK_BUD_LE_ADDR + TILE_LE_ADDR_SIZE)
#define APP_RW_KEY_REMAP_INFO_SIZE          112 /* note: this size must be sync with sizeof(T_CUSTOMER_KEY_INFO) */

#define UNPLUG_POWERON_RESET_ENABLE     0

#define APP_GAMING_MODE_SUPPORT         0

#define TRANSPORT_BIT_LE                0x01
#define TRANSPORT_BIT_SPP               0x02
#define TRANSPORT_BIT_IAP               0x04

#define VOICE_PROMPT_INDEX              0x80
#define TONE_INVALID_INDEX              0xFF

#define A2DP_FLUSH_TIME                     0x110 // 0xD0 130 0xF0 150 0x110 170
#define A2DP_INTERVAL                       0x3a // default 58
#define A2DP_CTRL_RSVD_SLOT                 0x0A // default 0a
#define A2DP_FLUSH_TIME_LOW_LATENCY         0x32
#define A2DP_INTERVAL_LOW_LATENCY           0x20
#define A2DP_CTRL_RSVD_SLOT_LOW_LATENCY     0x0a

#define AAC_LOW_LATENCY_MS                  60
#define SBC_LOW_LATENCY_MS                  60
#if C_APP_TILE_FEATURE_SUPPORT
#define A2DP_LATENCY_MS                     100
#else
//#define A2DP_LATENCY_MS                     280
#define A2DP_LATENCY_MS                     100
#endif

#if C_APP_TILE_FEATURE_SUPPORT
#define PLAYBACK_POOL_SIZE                  (6*1024)
#define NORMAL_MODE_DYNAMIC_LATENCY_ENABLE  false
#else
#define PLAYBACK_POOL_SIZE                  (6*1024)
#define NORMAL_MODE_DYNAMIC_LATENCY_ENABLE  true
#endif
#define VOICE_POOL_SIZE                     (512)
#define RECORD_POOL_SIZE                    (1*1024)
#define NOTIFICATION_POOL_SIZE              (1*1024)

/* Test Param for Idle slot */
/* for some LE application the a2dp idle slot can not be 0 */
#if (XM_XIAOAI_FEATURE_SUPPORT == 1) || (C_QCY_APP_FEATURE_SUPPORT == 1)
#define A2DP_IDLE_SLOT_NO_LE                0x06
#define A2DP_IDLE_SKIP_PERIOD_NO_LE         0x02
#elif (C_APP_TILE_FEATURE_SUPPORT == 1)
#define A2DP_IDLE_SLOT_NO_LE                0x04
#define A2DP_IDLE_SKIP_PERIOD_NO_LE         0x00
#else
#define A2DP_IDLE_SLOT_NO_LE                0x00
#define A2DP_IDLE_SKIP_PERIOD_NO_LE         0x00
#endif

#define A2DP_IDLE_SLOT_MULTILINK           0x06
#define A2DP_IDLE_SKIP_PERIOD_MULTILINK    0x02

// LE Idle
#if (C_APP_TILE_FEATURE_SUPPORT == 1)
#define A2DP_IDLE_SLOT_LE_IDLE          0x04
#define A2DP_IDLE_SKIP_PERIOD_LE_IDLE   0x00
#else
#define A2DP_IDLE_SLOT_LE_IDLE          0x0C
#define A2DP_IDLE_SKIP_PERIOD_LE_IDLE   0x03
#endif

// LE Active
#if (C_APP_TILE_FEATURE_SUPPORT == 1)
#define A2DP_IDLE_SLOT_LE_ACTIVE          0x04
#define A2DP_IDLE_SKIP_PERIOD_LE_ACTIVE   0x00
#else
#define A2DP_IDLE_SLOT_LE_ACTIVE          0x0C
#define A2DP_IDLE_SKIP_PERIOD_LE_ACTIVE   0x00
#endif
/* End */

enum
{
    LOW_LATENCY_LEVEL1  = 0,
    LOW_LATENCY_LEVEL2  = 1,
    LOW_LATENCY_LEVEL3  = 2,
    LOW_LATENCY_LEVEL_MAX  = 3,
};

enum
{
    SHORT = 0,
    LONG  = 1,
};

enum
{
    KEY0 = 0,
    KEY1 = 1,
    KEY2 = 2,
    KEY3 = 3,
    KEY4 = 4,
    KEY5 = 5,
    KEY6 = 6,
    KEY7 = 7
};

enum
{
    HY_KEY0 = 0,
    HY_KEY1 = 1,
    HY_KEY2 = 2,
    HY_KEY3 = 3,
    HY_KEY4 = 4,
    HY_KEY5 = 5,
    HY_KEY6 = 6,
    HY_KEY7 = 7
};

typedef enum
{
    DSP_OUTPUT_LOG_NONE = 0x0,          //!< no DSP log.
    DSP_OUTPUT_LOG_BY_UART = 0x1,       //!< DSP log by uart directly.
    DSP_OUTPUT_LOG_BY_MCU = 0x2,        //!< DSP log by MCU.
} T_DSP_OUTPUT_LOG;

typedef enum
{
    CHANNEL_L = 0x01,
    CHANNEL_R = 0x02,
    CHANNEL_LR_HELF = 0x03,
} T_CHANNEL_TYPE;

typedef enum
{
    DEVICE_BUD_SIDE_LEFT      = 0,
    DEVICE_BUD_SIDE_RIGHT     = 1,
} T_DEVICE_BUD_SIDE;

/** @defgroup APP_CFG App Cfg
  * @brief App Cfg
  * @{
  */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_CFG_Exported_Types App Cfg Types
    * @{
    */

/** @brief  Read only configurations for app  configurations, max size:512 bytes*/
typedef struct
{
    uint32_t sync_word;//0xAA55AA55

    uint8_t dsp_gpio_num;
    uint8_t dsp_gpio_pinmux[7];
    uint8_t dsp_jtag_enable : 1;
    uint8_t dsp2_jtag_enable : 1;
    uint8_t dsp2_log_output_select : 2;
    uint8_t dsp_rsv0 : 4;
    uint8_t dsp_jtck_pinmux;
    uint8_t dsp_jtdi_pinmux;
    uint8_t dsp_jtdo_pinmux;
    uint8_t dsp_jtms_pinmux;
    uint8_t dsp_jtrst_pinmux;

    uint8_t dsp2_jtck_pinmux;
    uint8_t dsp2_jtdi_pinmux;
    uint8_t dsp2_jtdo_pinmux;
    uint8_t dsp2_jtms_pinmux;
    uint8_t dsp2_jtrst_pinmux;
} T_APP_CFG2_CONST;

/** @brief  Read/Write configurations for inbox audio application  */
typedef struct
{
    uint32_t sync_word;
    uint32_t length;
} T_APP_CFG_NV_HDR;

/*T_APP_CFG_NV usage:
The offsets of existing items MUST NOT be changed:
1. If deleting existing items, positions of these items shall be reserved
2. If adding new items, the new items shall be appended in the end
*/
typedef struct
{
    T_APP_CFG_NV_HDR hdr;

    //offset: 0x08
    uint8_t device_name_legacy[40];
    uint8_t device_name_le[40];

    //offset: 0x58
    uint8_t le_random_addr[6];
    uint8_t rsv;
    uint8_t bud_role;
    uint8_t rsv0[16];

    //offset: 0x70
    uint8_t bud_local_addr[6];
    uint8_t bud_peer_addr[6];
    uint8_t bud_peer_factory_addr[6];
    uint16_t peer_valid_magic;

    //offset: 0x84
    uint32_t single_tone_timeout_val;

    //offset: 0x88
    uint8_t single_tone_tx_gain;
    uint8_t single_tone_channel_num;
    uint8_t rsv_1[2];

    //offset: 0x8c
    uint8_t eq_idx_anc_mode_record;
    uint8_t rsv_2[2];

    uint8_t app_is_power_on : 1;
    uint8_t is_bud_stereo : 1;
    uint8_t factory_reset_done : 1;
    uint8_t adp_factory_reset_power_on : 1;
    uint8_t rsv_4 : 4;

    uint8_t rsv_5[8];

    uint8_t case_battery;
    uint8_t local_level;
    uint8_t remote_level;
    uint8_t local_loc;

    //offset: 0x9c
    uint8_t rsv_6;
    uint8_t power_off_cause_cmd;
    uint8_t left_bud_addr[6];
    uint8_t right_bud_addr[6];
    uint8_t rsv_7[4];

    uint8_t voice_prompt_language;

    uint8_t public_bud_addr[6];
    uint8_t rsv_8[6];
    uint8_t usb_need_start;

    //offset: 188
    uint8_t rsv_9[14];

    uint32_t version; //use nv_version to monitor any NV item update

    uint8_t saved_id[3];

    //not to remove cfg nv rsv, use it and reduce bytes as needed
    uint8_t cfg_nv_rsv[1];
#if F_APP_USB_GIP_SUPPORT
    uint8_t gip_speaker_volume;
    uint8_t gip_game_chat_balance;
#endif

    uint8_t led_brightness_percentage : 7;
    uint8_t rsv_9_10 : 1;
} T_APP_CFG_NV;

/** @brief  Read only configurations for inbox audio application */
typedef struct
{
    uint32_t sync_word;

    uint32_t version;

    //Timer: 28 bytes(offset 8)
    uint8_t timer_rsv[28];

    //Key: 32 bytes (offset 36)
    uint8_t key_multi_click_interval;
    uint8_t key_long_press_interval;
    uint8_t key_long_press_repeat_interval;
    uint8_t key_power_on_interval;
    uint8_t key_power_off_interval;
    uint8_t key_enter_pairing_interval;
    uint8_t key_factory_reset_interval;
    uint8_t key_rsv0;
    uint8_t key_pinmux[8];
    uint8_t key_enable_mask;
    uint8_t key_long_press_repeat_mask;
    uint8_t key_short_press_tone_mask;
    uint8_t key_long_press_tone_mask;
    uint8_t hybrid_key_press_tone_mask;
    uint8_t key_rsv1;
    uint8_t key_high_active_mask;
    uint8_t key_rsv2[3];
    uint8_t key_disable_power_on_off;
    uint8_t key_trigger_long_press;
    uint8_t key_rsv3: 1;
    uint8_t mfb_replace_key0 : 1;
    uint8_t reserved_key_rsv1 : 6;
    uint8_t key_factory_reset_rws_interval;
    uint8_t key_rsv4[2];

    //Key table: 232 bytes (offset 68)
    uint8_t key_table[2][9][8]; //Table 0: key short click, Table 1: key long press
    uint8_t hybrid_key_table[9][8];
    uint8_t hybrid_key_mapping[8][2]; //Mapping[0]: Key Index. Mapping[1]: Hybrid Key click type

    //Tone: 64 bytes (offset 300)  -->64 byte
    uint8_t tone_rsv[64];

    //Peripheral: 40 bytes (offset 364)
    uint8_t data_uart_tx_pinmux;
    uint8_t data_uart_rx_pinmux;
    uint8_t tx_wake_up_pinmux;
    uint8_t rx_wake_up_pinmux;
    uint8_t i2c_0_dat_pinmux;
    uint8_t i2c_0_clk_pinmux;
    uint8_t buck_enable_pinmux;
    uint8_t line_in_pinmux;
    uint8_t pwm_pinmux;
    uint8_t led_0_pinmux;
    uint8_t led_1_pinmux;
    uint8_t led_2_pinmux;           //offset 12

    uint8_t pinmux_rsv0[5];

    uint8_t output_indication1_pinmux;
    uint8_t pinmux_rsv2[6];
    uint8_t external_mcu_input_pinmux;
    uint8_t pinmux_rsv3;
    uint8_t slide_switch_0_pinmux;
    uint8_t pinmux_rsv4[9];

    uint8_t data_uart_baud_rate;
    uint8_t peripheral_rsv[3];                     //new_add

    //App option: 8 bytes (offset 404)
    //app_option0
    uint8_t enable_rsv1 : 1;
    uint8_t enable_power_off_to_dlps_mode : 1;
    uint8_t enable_rsv2 : 6;
    //app_option1
    uint8_t enable_rsv3 : 7;
    uint8_t enable_rtk_vendor_cmd : 1;
    //app_option2
    uint8_t enable_rsv6 : 1;
    uint8_t enable_data_uart : 1;
    uint8_t enable_rsv7 : 1;
    uint8_t enable_tx_wake_up : 1;
    uint8_t enable_rx_wake_up : 1;
    uint8_t enable_rsv8 : 2;
    uint8_t enable_dlps : 1;
    //app_option3
    uint8_t led_support : 1;
    uint8_t key_gpio_support : 1;
    uint8_t llapt_support : 1;
    uint8_t line_in_support : 1;
    uint8_t support_rsv1 : 2;
    uint8_t pwm_support : 1;
    uint8_t charger_support : 1;
    //app_option4
    uint8_t normal_apt_support : 1;
    uint8_t enable_rsv9 : 7;
    //app_option5 (Options that can only be used for "support")
    uint8_t discharger_support : 1;
    uint8_t support_rsv2 : 7;

    //app_option6
    uint8_t enable_led0_low_active : 1;
    uint8_t enable_led1_low_active : 1;
    uint8_t enable_led2_low_active : 1;
    uint8_t enable_rsv10 : 5;
    //app_option7;
    uint8_t enable_rsv11 : 2;
    uint8_t enable_factory_reset_when_in_the_box : 1;
    uint8_t enable_rsv12 : 5;

    //Miscellaneous: 132 bytes (offset 412)
    uint8_t device_name_legacy_default[40];
    uint8_t device_name_le_default[40];        //80
    uint8_t class_of_device[3];
    uint8_t misc_rsv_1[5];
    uint8_t battery_warning_percent;
    uint8_t misc_rsv_2[3];
    uint32_t supported_profile_mask;
    uint8_t misc_rsv1;
    uint8_t pin_code_size;
    uint8_t pin_code[8];
    uint8_t misc_rsv2[3];
    uint8_t voice_prompt_language;              //110
    uint8_t company_id[2];
    uint8_t rsv[13];

    //dsp_option0
    uint8_t dsp_opt0_rsv;

    //dsp_option1
    uint8_t dsp_log_pin : 6;
    uint8_t dsp_log_output_select : 2;
    uint8_t dsp_option1_rsv[5];                       //130

    //App option_2: 8 bytes (offset 544)
    uint8_t app_opt2_rsv1 : 6;
    uint8_t enable_not_reset_device_name : 1;
    uint8_t app_opt2_rsv2 : 1;

    //app_option9
    uint8_t reserved_app_option9_rsv5 : 6;
    uint8_t enable_dongle_dual_mode : 1;
    uint8_t reserved_app_option9_rsv6 : 1;

    //app_option10
    uint8_t app_otp10_rsv;

    //app_option11 (Options that can only be used for "support")
    uint8_t app_opt11_rsv : 1;
    uint8_t sw_led_support  : 1;
    uint8_t app_opt11_rsv1 : 6;

    //app_option12
    uint8_t app_tpt12_rsv;

    //app_option13
    uint8_t app_tpt13_rsv1 : 1;
    uint8_t enable_new_low_bat_time_unit : 1; //0:sec, 1:min
    uint8_t app_tpt13_rsv2 : 6;

    //app_option14
    uint8_t app_tpt14_rsv;

    //app_option15
    uint8_t app_tpt15_rsv;

    //Profile A2DP codec settings + profile link number: Total 36 bytes(offset = 680)
    //SBC settings(8 bytes) (offset 552)
    uint8_t codec_rsv[28];

    //Profile link(8 bytes)(offset 580)
    uint8_t a2dp_link_number;
    uint8_t hfp_link_number;
    uint8_t spp_link_number;
    uint8_t profile_rsv;
    uint8_t avrcp_link_number;
    uint8_t profile_rsv1[3];

    //Audio 64 bytes (offset = 588)
    uint8_t audio_rsv[64];

    //App option_3: 10 bytes (offset 652)
    uint8_t app_otp_rsv1: 4;
    uint8_t charger_control_by_mcu : 1;
    uint8_t low_bat_warning_count : 2;
    uint8_t app_otp_rsv2: 1;

    uint8_t app_opt16_rsv[4];                   //new_add

    uint8_t app_opt21_rsv_0: 1;
    uint8_t ext_buck_support: 1;
    uint8_t ext_buck_vendor: 3;
    uint8_t app_opt21_rsv_1: 3;

    uint8_t app_opt17_rsv[6];

    //10 bytes app dongle option: (offset 664)
    uint8_t dongle_opt_rsv[2];                          // used for peer to recognize dongle type
    uint16_t dongle_pairing_uuid;                 //after paired  rws pairing ID
    uint8_t dongle_audio_src_prio;           //Aux_in: 0; USB: 1; First_Inject: 2
    uint8_t dongle_usb_record_sample_rate;      //16K:0 48K:1, upstream
    uint8_t dongle_usb_playback_sample_rate;    //16K:0; 48K:1, downstream
    uint8_t dongle_bt_src_prio;                 //legacy: 0; Broadcast:1; unicast:2;
    uint8_t dongle_legacy_mode_support: 1;          //support
    uint8_t dongle_cis_mode_support: 1;
    uint8_t dongle_bis_mode_support: 1;
    uint8_t dongle_audio_func_rsv: 5;
    uint8_t dongle_opt_rsv_1;

    // usb page 96 bytes(offset 674)
    uint8_t dongle_usb_support: 1;                 //
    uint8_t dongle_usb_hid_support: 1;
    uint8_t dongle_usb_audio_support: 1;
    uint8_t dongle_usb_fun_rsv: 5;
    uint8_t dongle_usb_speed : 2;
    uint8_t dongle_usb_rsv : 6;

    uint16_t dongle_usb_vid;                        //usb vid
    uint16_t dongle_usb_pid;                        //usb pid
    uint8_t dongle_usb_manuf_string[20];
    uint8_t dongle_usb_product_string[70];          //todo , move to structure tail

    // latency page 40 bytes(offset 770)
    uint8_t dongle_lea_rsv0;                    //normal:0; low_latency:1
    uint8_t dongle_stereo_sbcs_per_pkt;
    uint8_t dongle_tws_sbcs_per_pkt;

    // link page
    uint8_t dongle_link_num_max;
    uint16_t dongle_autopair_support: 1;
    uint16_t dongle_autopair_general_support: 1;        //autopair and connect with general headset
    uint16_t dongle_autopair_timeout: 14;
    int8_t dongle_autopair_rssi;
    uint8_t dongle_lea_rsv1[33];      //filter_name+manufactory+others VTL


    //broadcast audio page 20 bytes(offset 810)
    uint8_t dongle_broadcast_code_support: 1;           //broadcast code
    uint8_t dongle_broadcast_codec_support: 1;          // broadcast codec set support
    uint8_t dongle_auto_broadcast_support: 1;
    uint8_t dongle_broadcast_flag_rsv: 5;
    uint8_t dongle_broadcast_code[16];
    /*
    8-1:0; 8-2:1; 16-1:2; 16-2:3; 24-1:4; 24-2:5; 32-1:6; 32-2:7;
    441-1:8; 441-2:9; 48-1:a; 48-2:b; 48-3:c; 48-4:d; 48-5:e; 48-6:f;
    default: 48_4 */
    uint8_t dongle_broadcast_codec_index;
    uint8_t dongle_broadcast_codec_rsv;
    uint8_t dongle_broadcast_codec_latency;            //low_latency: 0; high_reliablity: 1  default: high_reliability

    //unicast audio page

    //config priority page

    //csip page 2 bytes(offset 830)
    uint8_t dongle_lea_rsv_1[4];
    uint8_t dongle_media_device;                  // usb: 0(default), line-in:1, mix:2
    uint8_t dongle_bis_num;                         //0/1/2

    uint8_t dongle_lea_rsv_2;
    uint8_t dongle_broadcast_id[3];
    uint8_t dongle_adv_sid;
    uint8_t dongle_ea_int_min[3];
    uint8_t dongle_ea_int_max[3];
    uint8_t dongle_pa_int_min[3];
    uint8_t dongle_pa_int_max[3];
    uint8_t dongle_pd_delay[2];
    uint8_t dongle_peer_addr[2][6];
} T_APP_CFG_CONST;

extern T_APP_CFG_CONST app_cfg_const;
extern T_APP_CFG2_CONST app_cfg2_const;

extern T_APP_CFG_NV app_cfg_nv;
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_CFG_Exported_Functions App Cfg Functions
    * @{
    */
/**
    * @brief  store config parameters to ftl
    * @param  void
    * @return ftl result
    */
uint32_t app_cfg_store(void);

/**
    * @brief  reset config parameters
    * @param  void
    * @return ftl result
    */
uint32_t app_cfg_reset(void);

/**
    * @brief  load config parameters from ftl
    * @param  void
    * @return void
    */
void app_cfg_load(void);

/**
    * @brief  load extend config parameters from ftl
    * @param  void
    * @return void
    */
void app_extend_cfg_load(void);

/**
    * @brief  config module init
    * @param  void
    * @return void
    */
void app_cfg_init(void);

/**
  * @brief  load led table from config.
  * @param  p_data  led table buffer
  * @param  mode    led mode
  * @param  led_table_size led table size for each led mode table
  * @return true  load cfg success.
  * @return false  load cfg fail.
  */
bool app_cfg_load_led_table(void *p_data, uint8_t mode, uint16_t led_table_size);

#if F_APP_LOCAL_PLAYBACK_SUPPORT
/**
    * @brief  usb config init
    * @param  void
    * @return void
    */
void app_cfg_usb_device_descriptor_content(void);
#endif


/** @} */ /* End of group APP_CFG_Exported_Functions */

/** End of APP_CFG
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif /*_AU_CFG_H_*/
