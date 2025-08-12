
#ifndef _APP_FLAGS_H_
#define _APP_FLAGS_H_
#include "upperstack_config.h"

#define APP_VERSION_MAJOR       (0)
#define APP_VERSION_MINOR       (10)

//#define APP_PID                 (0x1300)

#define F_APP_TEST_CFG

//Init value of default features are defined here
#define MFB_KEY_SUPPORT                     1
#define F_APP_CLI_CFG_SUPPORT               1
#define F_APP_CONSOLE_SUPPORT                1
#define F_APP_IO_OUTPUT_SUPPORT             1
#define F_APP_CLI_CMD_SUPPORT               1
#define F_APP_USB_SUPPORT                   1
#define F_APP_LINEIN_SUPPORT                1
#define F_APP_SC_KEY_DERIVE_SUPPORT         0

#if F_APP_CFU_RTL8773DO_DONGLE
#define F_APP_CFU_FEATURE_SUPPORT           1
#endif

#if TARGET_RTL8773DO
#define USB_PASSTHROUGH_CMD_SUPPORT         0
#define USB_PASSTHROUGH_CFU_SUPPORT         0
#else
#define USB_PASSTHROUGH_CMD_SUPPORT         1
#define USB_PASSTHROUGH_CFU_SUPPORT         1
#endif

#if TARGET_RTL8763EAU
#define F_APP_BB2_EXTERNAL_PA_SUPPORT       1
#endif

#if F_APP_USB_SUPPORT
#define F_APP_USB_HID_SUPPORT               1
#define F_APP_HID_RTK_SUPPORT               1
#define F_APP_USB_UAC_SUPPORT               1
#define F_APP_USB_AUDIO_SUPPORT             1
#if TARGET_LE_AUDIO_GAMING_DONGLE
#define F_APP_USB_AUDIO_FEEDBACK_SUPPORT    1
#endif
#endif

#define F_APP_CUSTOMIZED_SBC_VP_FLOW_SUPPORT      0
#define F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME      0
#define F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME    0

#if TARGET_LEGACY_GAMING_DONGLE || TARGET_LE_AUDIO_GAMING_DONGLE
#define F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING    1
#define F_APP_HANDLE_DS_PIPE_IN_ISR               1
#endif


#if TARGET_LE_AUDIO_GAMING_DONGLE
#define F_APP_SEND_CTRL_PKT_BY_LE_AUDIO           1
#define F_APP_LEA_DONGLE_BINDING                  1
#if TARGET_LE_AUDIO_GAMING_DONGLE_FOR_STEREO == 0
#define F_APP_SUPPORT_PCM_MIXING_FOR_ONE_CIS      1
#endif

#define F_APP_LE_AUDIO_DONGLE_CIS_ONLY            1

#undef  F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME
#define F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME    1

#undef  F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
#define F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST    1

#undef  F_APP_B2B_ENGAGE_IMPROVE_BY_LEA_DONGLE
#define F_APP_B2B_ENGAGE_IMPROVE_BY_LEA_DONGLE    1

#if TARGET_RTL8773DO
#if F_APP_HANDLE_DS_PIPE_IN_ISR
#define F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT      1

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#undef  F_APP_LE_AUDIO_DONGLE_CIS_ONLY
#define F_APP_LE_AUDIO_DONGLE_CIS_ONLY            0
#endif
#endif
#define F_APP_GATT_SERVER_EXT_API_SUPPORT         1
#define OTA_BY_BLE_ENABLE                         0
#define F_APP_LC3PLUS_CODEC_SUPPORT               1
#define F_APP_USB_HIGH_SPEED_0_5MS                0

#if F_APP_LC3PLUS_CODEC_SUPPORT
#if (F_APP_USB_HIGH_SPEED_0_5MS == 0) && (ENABLE_UAC2 == 0)
#define F_APP_PCM_SPLIT_0_5MS                     1
#endif
#define F_APP_USB_AUDIO_96K_24BIT                 1
#define F_APP_LC3PLUS_MODE_SWITCH_SUPPORT         1
#endif
#endif
#endif

/* legacy gaming dongle related */
#if TARGET_LEGACY_GAMING_DONGLE
#define F_APP_LEGACY_DONGLE_BINDING               1
/* ask headset to use max tx power */
#define F_APP_ASK_HEADSET_MAX_TX_POWER            1

#define F_APP_RTP_PLC_SUPPORT                     1

#undef  F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
#define F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET    0
#endif

#define UART_TRANSFER_DATA  0
#define USB_TRANSFER_DATA  1

#undef F_APP_HIFI4_SUPPORT
#if DSP2_SUPPORT
#define F_APP_HIFI4_SUPPORT                 1
#else
#define F_APP_HIFI4_SUPPORT                 0
#endif
/* Be cautious, it must be set to 1 when the one is set to 1 in bt_ual.
 * And BT_EXT_FTL_ADDR must be defined in flash_map.h
 * */
#if TARGET_RTL8773DO
#define GATTC_TBL_STORAGE_SUPPORT 1
#endif
#if CONFIG_SOC_SERIES_RTL8763E_V2
#define GATTC_TBL_STORAGE_SUPPORT 1
#endif

#define CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT 1

#define APP_MAX_LINKS       2
#define APP_ASE_NUM_MAX     3

#define MAX_BLE_LINK_NUM 2
#define MAX_BLE_SRV_NUM 16
#define MAX_BLE_CLIENT_NUM  16

#define LE_AUDIO_TMAS_SUPPORT 1

#define LE_AUDIO_PACS_SUPPORT 1
#define LE_AUDIO_ASCS_SUPPORT 1
#define LE_AUDIO_BASS_SUPPORT 1


#define LE_AUDIO_VCS_SUPPORT 1
#define LE_AUDIO_VOCS_SUPPORT 1
#define LE_AUDIO_AICS_SUPPORT 1
#define LE_AUDIO_MICS_SUPPORT 1

#define LE_AUDIO_CSIS_SUPPORT 1
#define LE_AUDIO_CAS_SUPPORT 1

#define LE_AUDIO_MCP_CLIENT_SUPPORT  1
#define LE_AUDIO_CCP_CLIENT_SUPPORT  0

//dongle need support
#define LE_AUDIO_MCP_SERVER_SUPPORT  1
#define LE_AUDIO_CCP_SERVER_SUPPORT  1

#define LE_AUDIO_OTS_SERV_SUPPORT    0

#define LE_AUDIO_CSIS_CLIENT_SUPPORT 1
#define LE_AUDIO_CAP_SUPPORT 1
#define LE_AUDIO_PACS_CLIENT_SUPPORT 1
#define LE_AUDIO_ASCS_CLIENT_SUPPORT 1
#define LE_AUDIO_BASS_CLIENT_SUPPORT 1


#define LE_AUDIO_VCS_CLIENT_SUPPORT 1
#define LE_AUDIO_VOCS_CLIENT_SUPPORT 1
#define LE_AUDIO_AICS_CLIENT_SUPPORT 1
#define LE_AUDIO_MICS_CLIENT_SUPPORT 1


#define LE_AUDIO_BROADCAST_SOURCE_ROLE    1
#define LE_AUDIO_BROADCAST_SINK_ROLE      1
#define LE_AUDIO_SCAN_DELEGATOR_ROLE      0
#define LE_AUDIO_BROADCAST_ASSISTANT_ROLE 1
#define LE_AUDIO_BASE_DATA_PARSE          1
#define LE_AUDIO_BASE_DATA_GENERATE (LE_AUDIO_BROADCAST_SOURCE_ROLE)

#define LE_AUDIO_PUBLIC_BROADCAST_SOURCE_ROLE    1
#define LE_AUDIO_PUBLIC_BROADCAST_SINK_ROLE      1
#define LE_AUDIO_PUBLIC_BROADCAST_ASSISTANT_ROLE 1

#define LE_AUDIO_ISO_REF_CLK        1
#define LE_AUDIO_REF_CLK            1

#define AUDIO_TEST_ENABLE       0

#define LE_AUDIO_CIG_INTERLEAVED        1
#define LE_AUDIO_BIG_INTERLEAVED        1

#define MIC_CODEC_DEBUG             0

#define LE_AUDIO_GAMING_SUPPORT     1
#define LE_EXT_FTL_SUPPORT          1

/*
CIS would be always conversation mode.
Real conversation would be notified by METADATA.
Relevant codes ONLY be executed when dongle in gaming mode.
*/

#if TARGET_LE_AUDIO_GAMING_DONGLE
#define LEA_GAMING_SPECIAL          1
#else
#define LEA_GAMING_SPECIAL          1
#endif

#if TARGET_LEGACY_GAMING_DONGLE || TARGET_LE_AUDIO_GAMING_DONGLE
#define F_APP_USB_SILENCE_STREAM_DETECT 1
#else
#define UAC_SILENCE_DETECT_SUPPORT      1
#endif

/*UAC_SILENCE_PROCESS enable will process and pause sending data to
  the headphones when silent data is detected.*/
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
#define UAC_SILENCE_PROCESS   0
#endif

#if ENABLE_UAC2
#define LEA_BIS_DUAL_UAC_SUPPORT     0
#endif

#define DONGLE_TRANSMIT_CLIENT_SUPPORT    1

#define LEGACY_GAMING_LOCK_HEADSET_SUPPORT  1

/* F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME and F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME
   cannot exist at the same time, will cause some issue */
#if F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME
#undef  F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME
#define F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME       0
#endif
#if F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME
#undef  F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME
#define F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME     0
#endif

#if ENABLE_UAC2
#undef  F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME
#define F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME       1
#endif

#define USB_VOL_RANGE_CHANGE_SUPPORT      1
#endif
