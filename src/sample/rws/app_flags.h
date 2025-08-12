
#ifndef _APP_FLAGS_H_
#define _APP_FLAGS_H_

/** @defgroup APP_FLAGS App Flags
  * @brief App Flags
  * @{
  */

//Init value of default features are defined here
//----- [Device related] -----
#define F_APP_POWER_TEST                            0 //Disable RSSI and Lowerbattery roleswap
#define F_APP_AUTO_POWER_TEST_LOG                   0
#define F_APP_TEST_SUPPORT                          1
#define F_APP_DATA_CAPTURE_SUPPORT                  0
#define F_APP_SAIYAN_MODE                           0
#define F_APP_SAIYAN_EQ_FITTING                     0
#define F_APP_CONSOLE_SUPPORT                       1
#define F_APP_CLI_STRING_MP_SUPPORT                 1
#define F_APP_CLI_BINARY_MP_SUPPORT                 1
#define F_APP_CLI_CFG_SUPPORT                       1
#define F_APP_BQB_CLI_SUPPORT                       1
#define F_APP_BQB_CLI_HFP_AG_SUPPORT                0
#define F_APP_BQB_CLI_MAP_SUPPORT                   0

#define F_APP_MULTI_LINK_ENABLE                     1
#define F_APP_RWS_MULTI_SPK_SUPPORT                 0
#define F_APP_SINGLE_MUTLILINK_SCENERIO_1           0 //this macro only enable in single mode, and if it enable, F_APP_MUTILINK_VA_PREEMPTIVE must be disable
#define F_APP_MUTILINK_VA_PREEMPTIVE                1 //ERWS_MULTILINK_SUPPORT
#define F_APP_MUTILINK_TRIGGER_HIGH_PRIORITY        0
#define F_APP_MUTLILINK_SOURCE_PRIORITY_UI          0
#define F_APP_MUTILINK_ALLOW_TWO_SCO                0 //considering bandwidth issue, only can be enabled for single mode application
#define F_APP_CALL_HOLD_SUPPORT                     0
#define F_APP_VOICE_SPK_EQ_SUPPORT                  1
#define F_APP_VOICE_MIC_EQ_SUPPORT                  1
#define F_APP_AUDIO_VOCIE_SPK_EQ_INDEPENDENT_CFG    0
#define F_APP_AUDIO_VOCIE_SPK_EQ_COMPENSATION_CFG   0

#define F_APP_DUT_MODE_AUTO_POWER_OFF               1
#define F_APP_VOICE_NREC_SUPPORT                    1
#define F_APP_TTS_SUPPORT                           0
#define F_APP_LOCAL_PLAYBACK_SUPPORT                0
#define F_APP_USB_AUDIO_SUPPORT                     0
#define F_APP_USB_HID_SUPPORT                       0
#define F_APP_USB_SUSPEND_SUPPORT                   0
#define F_APP_LINEIN_SUPPORT                        0
#define F_APP_ERWS_SUPPORT                          1
#define F_APP_VAD_SUPPORT                           0
#define F_APP_CFU_SUPPORT                           0
#define F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT     0
#define F_APP_SMOOTH_BAT_REPORT                     1
#define F_APP_GOLDEN_RANGE                          1
#define F_APP_CFU_FEATURE_SUPPORT                   0
#define F_APP_CFU_SPP_SUPPORT                       0
#define F_APP_CFU_BLE_CHANNEL_SUPPORT               0
#define F_APP_CHARGER_CASE_SUPPORT                  0
#define F_APP_FIND_EAR_BUD_SUPPORT                  0
#define F_APP_ADP_CMD_SUPPORT                       1
#define F_APP_EXT_MIC_SWITCH_SUPPORT                0

//Dongle related
#define F_APP_COMMON_DONGLE_SUPPORT                 0
#define F_APP_GAMING_DONGLE_SUPPORT                 0
#define F_APP_CONFERENCE_DONGLE_SUPPORT             0
#define F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT      0
#define F_APP_LEA_ALWAYS_CONVERSATION               0
#define F_APP_DONGLE_MULTI_PAIRING                  0
/* this flag depends on F_APP_COMMON_DONGLE_SUPPORT */
#define F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY   0
/* this flag depends on F_APP_COMMON_DONGLE_SUPPORT */
#define F_APP_KEEP_LEA_BOND                         0
#define F_APP_DISALLOW_AUTO_PAIRING                 0
#define F_APP_LOW_RSSI_DISCONNECT_DONGLE            0
#define F_APP_LEGACY_DONGLE_BINDING                 0
#define F_APP_LEA_DONGLE_BINDING                    0
#define F_APP_CUSTOMIZED_SBC_VP_FLOW_SUPPORT        0
#define F_APP_ALLOW_LEGACY_GAMING_TX_3M             0
#define F_APP_RTP_USE_LC3                           0
#define F_APP_INCREASE_RTK_APP_LINK_PRIORITY_TEMPORARILY    0

#define F_APP_OTA_TOOLING_SUPPORT                   1

#define F_APP_HFP_CMD_SUPPORT                       0
#define F_APP_DEVICE_CMD_SUPPORT                    0
#define F_APP_AVRCP_CMD_SUPPORT                     0
#define F_APP_PBAP_CMD_SUPPORT                      0

//----- [Legacy related] -----
#define F_APP_A2DP_CODEC_LDAC_SUPPORT               0
#define F_APP_A2DP_CODEC_LC3_SUPPORT                0
#define F_APP_A2DP_CODEC_LHDC_SUPPORT               0
#define F_APP_SPECIFIC_UUID_SUPPORT                 1
#define F_APP_HFP_AG_SUPPORT                        0

#define F_APP_IAP_RTK_SUPPORT                       0
#define F_APP_IAP_SUPPORT                           0

#define F_APP_HID_SUPPORT                           0
#define F_APP_HID_MOUSE_SUPPORT                     0
#define F_APP_HID_KEYBOARD_SUPPORT                  0
#define F_APP_CFU_HID_SUPPORT                       0 //conflict with F_APP_TEAMS_HID_SUPPORT

#define F_APP_BT_PROFILE_PBAP_SUPPORT               0
#define F_APP_BT_PROFILE_MAP_SUPPORT                0

#define F_APP_HANDLE_TOO_SMALL_ABS_VOL              0

//----- [LE related] -----
#define F_APP_GATT_SERVER_EXT_API_SUPPORT           1
#define F_APP_BLE_BOND_SYNC_SUPPORT                 1
#define F_APP_SC_KEY_DERIVE_SUPPORT                 1
#define F_APP_BLE_ANCS_CLIENT_SUPPORT               0
#define F_APP_RWS_BLE_USE_RPA_SUPPORT               0
#define F_APP_GATT_CACHING_SUPPORT                  0
#define F_APP_SLAVE_LATENCY_UPDATE_SUPPORT          1

//----- [Peripheral related] -----
#define F_APP_CAP_TOUCH_SUPPORT                     0

#define F_APP_SENSOR_SUPPORT                        1
#define F_APP_SENSOR_IQS773_873_SUPPORT             1 //Light sensor, P sensor
#define F_APP_SENSOR_JSA1225_SUPPORT                1 //Light sensor
#define F_APP_SENSOR_JSA1227_SUPPORT                1 //Light sensor
#define F_APP_SENSOR_PX318J_SUPPORT                 1 //Light sensor
#define F_APP_SENSOR_HX3001_SUPPORT                 1 //Light sensor
#define F_APP_SENSOR_SL7A20_SUPPORT                 1 //G seneor
#define F_APP_SENSOR_SC7A20_AS_LS_SUPPORT           1 //SC7A20 as light sensor
#define F_APP_SENSOR_CAP_TOUCH_SUPPORT              0

#define F_APP_PWM_SUPPORT                           0
#if F_APP_PWM_SUPPORT
#define F_APP_BUZZER_SUPPORT                        1
#endif

#define F_APP_NFC_SUPPORT                           0
#define F_APP_ADC_SUPPORT                           0
#define F_APP_DISCHARGER_NTC_DETECT_PROTECT         0
#define F_APP_GPIO_ONOFF_SUPPORT                    1
#define F_APP_IO_OUTPUT_SUPPORT                     1
#define F_APP_AMP_SUPPORT                           0
#define F_APP_GPIO_MICBIAS_SUPPORT                  1
#define F_APP_KEY_EXTEND_FEATURE                    1
#define F_APP_RWS_KEY_SUPPORT                       1
#define F_APP_EXT_FLASH_SUPPORT                     0

#define F_APP_SLIDE_SWITCH_SUPPORT                  1
#define F_APP_SLIDE_SWITCH_POWER_ON_OFF             1
#define F_APP_SLIDE_SWITCH_LISTENING_MODE           0
#define F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE          0

#if (F_APP_SLIDE_SWITCH_SUPPORT == 0)
#undef F_APP_SLIDE_SWITCH_POWER_ON_OFF
#define F_APP_SLIDE_SWITCH_POWER_ON_OFF             0
#undef F_APP_SLIDE_SWITCH_LISTENING_MODE
#define F_APP_SLIDE_SWITCH_LISTENING_MODE           0
#undef F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE
#define F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE          0
#endif

#define F_APP_ONE_WIRE_UART_SUPPORT                 1
#if F_APP_ONE_WIRE_UART_SUPPORT
#undef F_APP_CONSOLE_SUPPORT
#define F_APP_CONSOLE_SUPPORT                       1
#undef F_APP_CLI_BINARY_MP_SUPPORT
#define F_APP_CLI_BINARY_MP_SUPPORT                 1
#endif

//----- [Listening related] -----
#define F_APP_HEARABLE_SUPPORT                      0
#define F_APP_ANC_SUPPORT                           1
#define F_APP_APT_SUPPORT                           1
#define F_APP_SIDETONE_SUPPORT                      1
#define F_APP_SIDETONE_PATH_SUPPORT                 0
#define F_APP_AIRPLANE_SUPPORT                      0
#define F_APP_LISTENING_MODE_SUPPORT                1
#define F_APP_SUPPORT_ANC_APT_COEXIST               0
#define F_APP_ANC_DEFAULT_ACTIVATE_ALL_GROUP        0
#define F_APP_SUPPORT_ANC_APT_APPLY_BURN            0

//RHE related
#define F_APP_BRIGHTNESS_SUPPORT                    0
#define F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT      0
#define F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT        0
#define F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT         0
#define F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT    0
#define F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT       0

#if (F_APP_LISTENING_MODE_SUPPORT == 0)
#undef F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           0
#undef F_APP_APT_SUPPORT
#define F_APP_APT_SUPPORT                           0
#endif

#if ((F_APP_APT_SUPPORT == 0) || (F_APP_ANC_SUPPORT == 0))
#undef F_APP_BRIGHTNESS_SUPPORT
#define F_APP_BRIGHTNESS_SUPPORT                    0
#endif

//----- [3rd party related] -----
#define AMA_FEATURE_SUPPORT                         0
#define BISTO_FEATURE_SUPPORT                       0
#define CONFIG_REALTEK_GFPS_FEATURE_SUPPORT         0
#define CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT       (0 && CONFIG_REALTEK_GFPS_FEATURE_SUPPORT)
#define CONFIG_REALTEK_GFPS_SASS_SUPPORT            (0 && CONFIG_REALTEK_GFPS_FEATURE_SUPPORT)
#define CONFIG_REALTEK_GFPS_FINDER_SUPPORT          (0 && CONFIG_REALTEK_GFPS_FEATURE_SUPPORT)
#define F_APP_TUYA_SUPPORT                          0
#define F_APP_BLE_SWIFT_PAIR_SUPPORT                0
#define CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT    0
#define XM_XIAOAI_FEATURE_SUPPORT                   0
#define F_APP_XIAOWEI_FEATURE_SUPPORT               0
#define F_APP_FINDMY_FEATURE_SUPPORT                0

#define F_APP_DUAL_AUDIO_EFFECT                     0
#define F_APP_DUAL_AUDIO_TWS_SPATIAL_AUDIO          0

#define F_APP_SAMPLE_CONFIG_TEAMS_RWS_SUPPORT       0
#define F_APP_SAMPLE_CONFIG_TEAMS_SINGLE_SUPPORT    0

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#undef F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                0
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#undef F_APP_RWS_BLE_USE_RPA_SUPPORT
#define F_APP_RWS_BLE_USE_RPA_SUPPORT               1
#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
#undef F_APP_GATT_CACHING_SUPPORT
#define F_APP_GATT_CACHING_SUPPORT                  1
#endif
#endif

#if F_APP_SAMPLE_CONFIG_TEAMS_RWS_SUPPORT
#undef CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
#define CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT    1
#undef F_APP_HID_SUPPORT
#define F_APP_HID_SUPPORT                           1
#define F_APP_TEAMS_HID_SUPPORT                     1    //classic hid module
#define F_APP_TEAMS_CUSTOMIZED_CMD_SUPPORT          0    //app cmd
#define CONFIG_REALTEK_APP_TEAMS_VP_UPDATE_SUPPORT  0    //vp update module(include vp update/vp play)
#define F_APP_TEAMS_BT_POLICY                       1    //bt policy(include pairing/reconnect/dongle detect)
#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT && F_APP_SINGLE_MUTLILINK_SCENERIO_1
#define F_APP_TEAMS_GLOBAL_MUTE_SUPPORT             0    //only can be enabled when teams and single multilink scenerio 1 supported
#endif
#endif /* F_APP_SAMPLE_CONFIG_TEAMS_RWS_SUPPORT */

#if F_APP_SAMPLE_CONFIG_TEAMS_SINGLE_SUPPORT
#undef F_APP_USB_AUDIO_SUPPORT
#define F_APP_USB_AUDIO_SUPPORT                     0
#undef F_APP_CFU_SUPPORT
#define F_APP_CFU_SUPPORT                           0
#undef F_APP_SINGLE_MUTLILINK_SCENERIO_1
#define F_APP_SINGLE_MUTLILINK_SCENERIO_1           1
#undef F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                0
#undef CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
#define CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT    1
#define F_APP_TEAMS_HID_SUPPORT                     1    //classic hid module
#define F_APP_TEAMS_CUSTOMIZED_CMD_SUPPORT          0    //app cmd
#define CONFIG_REALTEK_APP_TEAMS_VP_UPDATE_SUPPORT  0    //vp update module(include vp update/vp play)
#define F_APP_TEAMS_BT_POLICY                       1    //bt policy(include pairing/reconnect/dongle detect)
#define F_APP_TEAMS_MULTILINK_NOTIFICATION_FLAG     1    //teams multilink notification enable
#endif /* F_APP_SAMPLE_CONFIG_TEAMS_SINGLE_SUPPORT */

//----- [Add more target here] -----
#if TARGET_SINGLE
#undef F_APP_ERWS_SUPPORT
#define F_APP_ERWS_SUPPORT                          0
#undef F_APP_BLE_BOND_SYNC_SUPPORT
#define F_APP_BLE_BOND_SYNC_SUPPORT                 0
#undef F_APP_BOND_MGR_BLE_SYNC
#define F_APP_BOND_MGR_BLE_SYNC                     0
#endif /* TARGET_SINGLE */

#if TARGET_LOCAL_PLAYBACK
#undef F_APP_LOCAL_PLAYBACK_SUPPORT
#define F_APP_LOCAL_PLAYBACK_SUPPORT                1
#undef F_APP_BQB_CLI_SUPPORT
#define F_APP_BQB_CLI_SUPPORT                       0
#undef F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           0
#undef F_APP_AIRPLANE_SUPPORT
#define F_APP_AIRPLANE_SUPPORT                      0
#undef F_APP_MULTI_LINK_ENABLE
#define F_APP_MULTI_LINK_ENABLE                     0
#undef F_APP_TTS_SUPPORT
#define F_APP_TTS_SUPPORT                           0
#endif /* TARGET_LOCAL_PLAYBACK */

#if TARGET_RTL8763EWM
#define IC_NAME                                     "RTL8763EWM"

#undef  F_APP_LEA_SUPPORT
#define F_APP_LEA_SUPPORT                           1
#undef  F_APP_ERWS_SUPPORT
#define F_APP_ERWS_SUPPORT                          0
//Dongle related
#undef F_APP_COMMON_DONGLE_SUPPORT
#define F_APP_COMMON_DONGLE_SUPPORT                 1
#undef F_APP_GAMING_DONGLE_SUPPORT
#define F_APP_GAMING_DONGLE_SUPPORT                 1
#undef F_APP_CONFERENCE_DONGLE_SUPPORT
#define F_APP_CONFERENCE_DONGLE_SUPPORT             0
#undef F_APP_QDECODE_SUPPORT
#define F_APP_QDECODE_SUPPORT                       1

#undef F_APP_SINGLE_MUTLILINK_SCENERIO_1
#define F_APP_SINGLE_MUTLILINK_SCENERIO_1           0 //this macro only enable in single mode, and if it enable, F_APP_MUTILINK_VA_PREEMPTIVE must be disable
#undef F_APP_MUTLILINK_SOURCE_PRIORITY_UI
#define F_APP_MUTLILINK_SOURCE_PRIORITY_UI          1
#undef F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                0
#undef F_APP_USB_AUDIO_SUPPORT
#define F_APP_USB_AUDIO_SUPPORT                     1
#undef F_APP_USB_HID_SUPPORT
#define F_APP_USB_HID_SUPPORT                       1
#undef F_APP_USB_SUSPEND_SUPPORT
#define F_APP_USB_SUSPEND_SUPPORT                   1
#undef F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE
#define F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE          1

#undef F_APP_BLE_SWIFT_PAIR_SUPPORT
#define F_APP_BLE_SWIFT_PAIR_SUPPORT                1

//Following settings are same in 73EFE & 63EWM
#undef F_APP_AIRPLANE_SUPPORT
#define F_APP_AIRPLANE_SUPPORT                      1
#undef F_APP_USER_EQ_SUPPORT
#define F_APP_USER_EQ_SUPPORT                       1
#undef F_APP_AMP_SUPPORT
#define F_APP_AMP_SUPPORT                           1

#if F_APP_LISTENING_MODE_SUPPORT
#undef F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           1
#if F_APP_ANC_SUPPORT && F_APP_APT_SUPPORT
#undef F_APP_SUPPORT_ANC_APT_COEXIST
#define F_APP_SUPPORT_ANC_APT_COEXIST               1
#endif

//RHE related
#if F_APP_APT_SUPPORT
#undef F_APP_BRIGHTNESS_SUPPORT
#define F_APP_BRIGHTNESS_SUPPORT                    1
#undef F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT
#define F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT      1
#undef F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT        1
#undef F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT    1
#if F_APP_ANC_SUPPORT
#undef F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
#define F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT         1
#endif
#endif

#undef F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT
#define F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT       1
#endif
#endif

#if TARGET_RTL8763ESE || TARGET_RTL8763EHA

/*--- Feature macro  ---*/
#undef F_APP_SENSOR_SUPPORT
#define F_APP_SENSOR_SUPPORT                        1
#undef F_APP_USER_EQ_SUPPORT
#define F_APP_USER_EQ_SUPPORT                       1
#undef F_APP_USB_SUSPEND_SUPPORT
#define F_APP_USB_SUSPEND_SUPPORT                   1
#undef F_APP_AIRPLANE_SUPPORT
#define F_APP_AIRPLANE_SUPPORT                      1

// Dongle related
#undef F_APP_COMMON_DONGLE_SUPPORT
#define F_APP_COMMON_DONGLE_SUPPORT                 1
#undef F_APP_GAMING_DONGLE_SUPPORT
#define F_APP_GAMING_DONGLE_SUPPORT                 0
#undef F_APP_CONFERENCE_DONGLE_SUPPORT
#define F_APP_CONFERENCE_DONGLE_SUPPORT             1
#undef  F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY
#define F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY   0
#undef F_APP_AMP_SUPPORT
#define F_APP_AMP_SUPPORT                           1

// Listening mode related
#undef  F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           0
#undef  F_APP_APT_SUPPORT
#define F_APP_APT_SUPPORT                           0
#undef  F_APP_LISTENING_MODE_SUPPORT
#define F_APP_LISTENING_MODE_SUPPORT                0

#if TARGET_RTL8763ESE   // ++++  #if TARGET_RTL8763ESE
#define IC_NAME                                     "RTL8763ESE"

/*--- Product macro  ---*/
#define RTL8763ESE_PRODUCT_NORMAL_RWS               0  // DSP: normal_rws
#define RTL8763ESE_PRODUCT_3D_RWS                   0  // DSP: 3d_rws
/*--- Product macro  ---*/

#if RTL8763ESE_PRODUCT_STEREO
#undef F_APP_ERWS_SUPPORT
#define F_APP_ERWS_SUPPORT                          0
#undef F_APP_BLE_BOND_SYNC_SUPPORT
#define F_APP_BLE_BOND_SYNC_SUPPORT                 0
#undef F_APP_BOND_MGR_BLE_SYNC
#define F_APP_BOND_MGR_BLE_SYNC                     0
#undef F_APP_USB_AUDIO_SUPPORT
#define F_APP_USB_AUDIO_SUPPORT                     1
#undef F_APP_USB_HID_SUPPORT
#define F_APP_USB_HID_SUPPORT                       1
#undef F_APP_MUTLILINK_SOURCE_PRIORITY_UI
#define F_APP_MUTLILINK_SOURCE_PRIORITY_UI          1
#undef F_APP_LINEIN_SUPPORT
#define F_APP_LINEIN_SUPPORT                        1
#endif

#if RTL8763ESE_PRODUCT_NORMAL_RWS
#undef  F_APP_LISTENING_MODE_SUPPORT
#define F_APP_LISTENING_MODE_SUPPORT                1
#undef F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           1
#undef F_APP_APT_SUPPORT
#define F_APP_APT_SUPPORT                           1
#endif

#if RTL8763ESE_PRODUCT_3D_RWS
#undef F_APP_SUPPORT_SPATIAL_AUDIO
#define F_APP_SUPPORT_SPATIAL_AUDIO                 1
#undef F_APP_USER_EQ_SUPPORT
#define F_APP_USER_EQ_SUPPORT                       0
#endif

#if RTL8763ESE_PRODUCT_SPEAKER
#undef F_APP_SENSOR_IQS773_873_SUPPORT
#define F_APP_SENSOR_IQS773_873_SUPPORT             1
#undef F_APP_SENSOR_JSA1225_SUPPORT
#define F_APP_SENSOR_JSA1225_SUPPORT                1
#undef F_APP_SENSOR_JSA1227_SUPPORT
#define F_APP_SENSOR_JSA1227_SUPPORT                1
#undef F_APP_SENSOR_PX318J_SUPPORT
#define F_APP_SENSOR_PX318J_SUPPORT                 0
#undef F_APP_SENSOR_HX3001_SUPPORT
#define F_APP_SENSOR_HX3001_SUPPORT                 0
#undef F_APP_SENSOR_SL7A20_SUPPORT
#define F_APP_SENSOR_SL7A20_SUPPORT                 0
#undef F_APP_SENSOR_SC7A20_AS_LS_SUPPORT
#define F_APP_SENSOR_SC7A20_AS_LS_SUPPORT           0
#undef F_APP_LINEIN_SUPPORT
#define F_APP_LINEIN_SUPPORT                        1
#undef F_APP_DUAL_AUDIO_EFFECT
#define F_APP_DUAL_AUDIO_EFFECT                     F_APP_ESE_MALLEUS_SUPPORT
#undef F_APP_USB_AUDIO_SUPPORT
#define F_APP_USB_AUDIO_SUPPORT                     1
#undef F_APP_USB_HID_SUPPORT
#define F_APP_USB_HID_SUPPORT                       1
#undef F_APP_SINGLE_MUTLILINK_SCENERIO_1
#define F_APP_SINGLE_MUTLILINK_SCENERIO_1           0
#undef F_APP_MUTLILINK_SOURCE_PRIORITY_UI
#define F_APP_MUTLILINK_SOURCE_PRIORITY_UI          1
#undef F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                0
#undef  F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE
#define F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE          1
#undef F_APP_COMMON_DONGLE_SUPPORT
#define F_APP_COMMON_DONGLE_SUPPORT                 1
#undef F_APP_GAMING_DONGLE_SUPPORT
#define F_APP_GAMING_DONGLE_SUPPORT                 0
#undef F_APP_CONFERENCE_DONGLE_SUPPORT
#define F_APP_CONFERENCE_DONGLE_SUPPORT             0
#undef F_APP_QDECODE_SUPPORT
#define F_APP_QDECODE_SUPPORT                       1
#endif

#else  // === #if TARGET_RTL8763ESE
#define IC_NAME                                     "RTL8763EHA"

#undef F_APP_HEARABLE_SUPPORT
#define F_APP_HEARABLE_SUPPORT                      1
#undef  F_APP_LISTENING_MODE_SUPPORT
#define F_APP_LISTENING_MODE_SUPPORT                1
#undef  F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           1
#undef F_APP_APT_SUPPORT
#define F_APP_APT_SUPPORT                           1

#endif

#if F_APP_ANC_SUPPORT && F_APP_APT_SUPPORT
#undef F_APP_SUPPORT_ANC_APT_COEXIST
#define F_APP_SUPPORT_ANC_APT_COEXIST               1
#endif

//RHE related
#if F_APP_APT_SUPPORT
#undef F_APP_BRIGHTNESS_SUPPORT
#define F_APP_BRIGHTNESS_SUPPORT                    1
#undef F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT
#define F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT      1
#undef F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT        1
#undef F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT    1
#undef F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT
#define F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT       1

#if F_APP_ANC_SUPPORT
#undef F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
#define F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT         1
#endif

#endif  // --- #if TARGET_RTL8763ESE

#if F_APP_SUPPORT_SPATIAL_AUDIO
#undef F_APP_DUAL_AUDIO_EFFECT
#define F_APP_DUAL_AUDIO_EFFECT                     1
#undef F_APP_DUAL_AUDIO_TWS_SPATIAL_AUDIO
#define F_APP_DUAL_AUDIO_TWS_SPATIAL_AUDIO          0
#undef F_APP_SENSOR_MEMS_SUPPORT
#define F_APP_SENSOR_MEMS_SUPPORT                   0
#undef F_APP_SENSOR_INV42607_SUPPORT
#define F_APP_SENSOR_INV42607_SUPPORT               0
#endif

/*--- Feature macro  ---*/

#endif

#if TARGET_RTL8763EFL || TARGET_RTL8773ESL || TARGET_RTL8773EWE
#undef F_APP_LEA_SUPPORT
#define F_APP_LEA_SUPPORT                           1
#undef CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
#define CONFIG_REALTEK_APP_BOND_MGR_SUPPORT         1
#undef F_APP_USER_EQ_SUPPORT
#define F_APP_USER_EQ_SUPPORT                       1

#if TARGET_RTL8773ESL
#define IC_NAME                                     "RTL8773ESL"

#if F_APP_LISTENING_MODE_SUPPORT
#undef F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           1
#undef F_APP_AIRPLANE_SUPPORT
#define F_APP_AIRPLANE_SUPPORT                      1
#if F_APP_ANC_SUPPORT && F_APP_APT_SUPPORT
#undef F_APP_SUPPORT_ANC_APT_COEXIST
#define F_APP_SUPPORT_ANC_APT_COEXIST               1
#endif
#undef F_APP_HEARABLE_SUPPORT
#define F_APP_HEARABLE_SUPPORT                      1

//RHE related
#if F_APP_APT_SUPPORT
#undef F_APP_BRIGHTNESS_SUPPORT
#define F_APP_BRIGHTNESS_SUPPORT                    1
#undef F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT
#define F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT      1
#undef F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT        1
#undef F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT    1
#if F_APP_ANC_SUPPORT
#undef F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
#define F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT         1
#endif
#endif
#undef F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT
#define F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT       1
#endif
#else
#if TARGET_RTL8773ESL
#define IC_NAME                                     "RTL8763EFL"
#else
#define IC_NAME                                     "RTL8773EWE"
#endif
#undef F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           0
#endif

#if RTL87x3EFL_PRODUCT_STEREO || TARGET_RTL8773EWE
#undef F_APP_ERWS_SUPPORT
#define F_APP_ERWS_SUPPORT                          0
#undef F_APP_QDECODE_SUPPORT
#define F_APP_QDECODE_SUPPORT                       0
#undef F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                0
#undef F_APP_MUTLILINK_SOURCE_PRIORITY_UI
#define F_APP_MUTLILINK_SOURCE_PRIORITY_UI          0
#undef F_APP_USB_AUDIO_SUPPORT
#define F_APP_USB_AUDIO_SUPPORT                     0
#undef F_APP_USB_HID_SUPPORT
#define F_APP_USB_HID_SUPPORT                       0
#undef F_APP_AMP_SUPPORT
#define F_APP_AMP_SUPPORT                           1
#endif

#if RTL87x3EFL_PRODUCT_STEREO
#undef F_APP_LINEIN_SUPPORT
#define F_APP_LINEIN_SUPPORT                        1
#endif

#endif

#if TARGET_RTL8773DO
#define IC_NAME                                     "RTL8773DO"
#undef F_APP_APT_SUPPORT
#define F_APP_APT_SUPPORT                           1
#undef F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           1
#undef F_APP_AIRPLANE_SUPPORT
#define F_APP_AIRPLANE_SUPPORT                      1
#undef CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#define CONFIG_REALTEK_GFPS_FEATURE_SUPPORT         0
#undef F_APP_SS_SUPPORT
#define F_APP_SS_SUPPORT                            0
#undef F_APP_BLE_SWIFT_PAIR_SUPPORT
#define F_APP_BLE_SWIFT_PAIR_SUPPORT                0
#undef F_APP_EXT_FLASH_SUPPORT
#define F_APP_EXT_FLASH_SUPPORT                     0
#undef F_APP_USER_EQ_SUPPORT
#define F_APP_USER_EQ_SUPPORT                       1

#undef F_APP_SAIYAN_EQ_FITTING
#define F_APP_SAIYAN_EQ_FITTING                     0

#undef F_APP_LEA_SUPPORT
#define F_APP_LEA_SUPPORT                           1
#undef  F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE
#define F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE          1
#if F_APP_ANC_SUPPORT && F_APP_APT_SUPPORT
#undef F_APP_SUPPORT_ANC_APT_COEXIST
#define F_APP_SUPPORT_ANC_APT_COEXIST               1
#undef F_APP_HEARABLE_SUPPORT
#define F_APP_HEARABLE_SUPPORT                      1
#endif

//RHE related
#if F_APP_APT_SUPPORT
#undef F_APP_BRIGHTNESS_SUPPORT
#define F_APP_BRIGHTNESS_SUPPORT                    1
#undef F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT
#define F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT      1
#undef F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT        1
#undef F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT    1
#if F_APP_ANC_SUPPORT
#undef F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
#define F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT         1
#endif
#endif
#undef F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT
#define F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT       1

#undef F_APP_HIFI4_SUPPORT
#define F_APP_HIFI4_SUPPORT                         1

#undef F_APP_ALC1017_SUPPORT
#define F_APP_ALC1017_SUPPORT                       0 //this macro only enable when external codec is ALC1017 and enable

#undef F_APP_SINGLE_MUTLILINK_SCENERIO_1
#define F_APP_SINGLE_MUTLILINK_SCENERIO_1           0 //this macro only enable in single mode, and if it enable, F_APP_MUTILINK_VA_PREEMPTIVE must be disable

#undef F_APP_TEAMS_MULTILINK_NOTIFICATION_FLAG
#define F_APP_TEAMS_MULTILINK_NOTIFICATION_FLAG     0 //this macro relay to F_APP_SINGLE_MUTLILINK_SCENERIO_1, so disable this macro

#undef F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                1

#undef F_APP_TTS_SUPPORT
#define F_APP_TTS_SUPPORT                           1

#undef F_APP_COMMON_DONGLE_SUPPORT
#define F_APP_COMMON_DONGLE_SUPPORT                 1
#undef F_APP_GAMING_DONGLE_SUPPORT
#define F_APP_GAMING_DONGLE_SUPPORT                 0
#undef F_APP_CONFERENCE_DONGLE_SUPPORT
#define F_APP_CONFERENCE_DONGLE_SUPPORT             1

#if F_APP_IS_STEREO  //current the stereo just is gaming headset
#undef F_APP_COMMON_DONGLE_SUPPORT
#define F_APP_COMMON_DONGLE_SUPPORT                 1
#undef F_APP_GAMING_DONGLE_SUPPORT
#define F_APP_GAMING_DONGLE_SUPPORT                 1
#undef F_APP_CONFERENCE_DONGLE_SUPPORT
#define F_APP_CONFERENCE_DONGLE_SUPPORT             0
#undef F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT
#define F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT      1
#undef  F_APP_DONGLE_MULTI_PAIRING
#define F_APP_DONGLE_MULTI_PAIRING                  1
#undef F_APP_QDECODE_SUPPORT
#define F_APP_QDECODE_SUPPORT                       1
#undef F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                0
#undef F_APP_MUTLILINK_SOURCE_PRIORITY_UI
#define F_APP_MUTLILINK_SOURCE_PRIORITY_UI          1
#undef F_APP_USB_AUDIO_SUPPORT
#define F_APP_USB_AUDIO_SUPPORT                     1
#undef F_APP_USB_HID_SUPPORT
#define F_APP_USB_HID_SUPPORT                       1
#undef F_APP_AMP_SUPPORT
#define F_APP_AMP_SUPPORT                           1
#endif
#endif

#if TARGET_RTL8773DFL
#define IC_NAME                                     "RTL8773DFL"
#undef F_APP_APT_SUPPORT
#define F_APP_APT_SUPPORT                           1
#undef F_APP_ANC_SUPPORT
#define F_APP_ANC_SUPPORT                           1
#undef F_APP_AIRPLANE_SUPPORT
#define F_APP_AIRPLANE_SUPPORT                      1
#undef CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#define CONFIG_REALTEK_GFPS_FEATURE_SUPPORT         0
#undef F_APP_SS_SUPPORT
#define F_APP_SS_SUPPORT                            0
#undef F_APP_BLE_SWIFT_PAIR_SUPPORT
#define F_APP_BLE_SWIFT_PAIR_SUPPORT                0
#undef F_APP_EXT_FLASH_SUPPORT
#define F_APP_EXT_FLASH_SUPPORT                     0
#undef F_APP_USER_EQ_SUPPORT
#define F_APP_USER_EQ_SUPPORT                       1

#undef F_APP_SAIYAN_EQ_FITTING
#define F_APP_SAIYAN_EQ_FITTING                     0

#undef F_APP_LEA_SUPPORT
#define F_APP_LEA_SUPPORT                           1
#undef  F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE
#define F_APP_SLIDE_SWITCH_MIC_MUTE_TOGGLE          1
#if F_APP_ANC_SUPPORT && F_APP_APT_SUPPORT
#undef F_APP_SUPPORT_ANC_APT_COEXIST
#define F_APP_SUPPORT_ANC_APT_COEXIST               1
#undef F_APP_HEARABLE_SUPPORT
#define F_APP_HEARABLE_SUPPORT                      1
#endif

//RHE related
#if F_APP_APT_SUPPORT
#undef F_APP_BRIGHTNESS_SUPPORT
#define F_APP_BRIGHTNESS_SUPPORT                    1
#undef F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT
#define F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT      1
#undef F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT        1
#undef F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
#define F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT    1
#if F_APP_ANC_SUPPORT
#undef F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
#define F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT         1
#endif
#endif
#undef F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT
#define F_APP_ADJUST_NOTIFICATION_VOL_SUPPORT       1

#undef F_APP_HIFI4_SUPPORT
#define F_APP_HIFI4_SUPPORT                         0

#undef F_APP_SINGLE_MUTLILINK_SCENERIO_1
#define F_APP_SINGLE_MUTLILINK_SCENERIO_1           0 //this macro only enable in single mode, and if it enable, F_APP_MUTILINK_VA_PREEMPTIVE must be disable

#undef F_APP_TEAMS_MULTILINK_NOTIFICATION_FLAG
#define F_APP_TEAMS_MULTILINK_NOTIFICATION_FLAG     0 //this macro relay to F_APP_SINGLE_MUTLILINK_SCENERIO_1, so disable this macro

#undef F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                1

#undef F_APP_TTS_SUPPORT
#define F_APP_TTS_SUPPORT                           1

#undef F_APP_COMMON_DONGLE_SUPPORT
#define F_APP_COMMON_DONGLE_SUPPORT                 1
#undef F_APP_GAMING_DONGLE_SUPPORT
#define F_APP_GAMING_DONGLE_SUPPORT                 0
#undef F_APP_CONFERENCE_DONGLE_SUPPORT
#define F_APP_CONFERENCE_DONGLE_SUPPORT             1

#if F_APP_IS_STEREO  //current the stereo just is gaming headset
#undef F_APP_COMMON_DONGLE_SUPPORT
#define F_APP_COMMON_DONGLE_SUPPORT                 1
#undef F_APP_GAMING_DONGLE_SUPPORT
#define F_APP_GAMING_DONGLE_SUPPORT                 1
#undef F_APP_CONFERENCE_DONGLE_SUPPORT
#define F_APP_CONFERENCE_DONGLE_SUPPORT             0
#undef F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT
#define F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT      1
#undef  F_APP_DONGLE_MULTI_PAIRING
#define F_APP_DONGLE_MULTI_PAIRING                  1
#undef F_APP_QDECODE_SUPPORT
#define F_APP_QDECODE_SUPPORT                       1
#undef F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                0
#undef F_APP_MUTLILINK_SOURCE_PRIORITY_UI
#define F_APP_MUTLILINK_SOURCE_PRIORITY_UI          1
#undef F_APP_USB_AUDIO_SUPPORT
#define F_APP_USB_AUDIO_SUPPORT                     1
#undef F_APP_USB_HID_SUPPORT
#define F_APP_USB_HID_SUPPORT                       1
#undef F_APP_AMP_SUPPORT
#define F_APP_AMP_SUPPORT                           1
#endif
#endif

//----- [Target related] -----
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
#define F_APP_BOND_MGR_DEBUG                        1
#define F_APP_BOND_MGR_BLE_SYNC                     1
#undef F_APP_BLE_BOND_SYNC_SUPPORT
#define F_APP_BLE_BOND_SYNC_SUPPORT                 0
#endif

#if F_APP_ERWS_SUPPORT
#undef F_APP_QOL_MONITOR_SUPPORT
#define F_APP_QOL_MONITOR_SUPPORT                   1
#undef F_APP_DYNAMIC_ADJUST_B2B_TX_POWER
#define F_APP_DYNAMIC_ADJUST_B2B_TX_POWER           1
#else
#undef F_APP_QOL_MONITOR_SUPPORT
#define F_APP_QOL_MONITOR_SUPPORT                   0
#undef F_APP_RWS_MULTI_SPK_SUPPORT
#define F_APP_RWS_MULTI_SPK_SUPPORT                 0
#undef F_APP_DYNAMIC_ADJUST_B2B_TX_POWER
#define F_APP_DYNAMIC_ADJUST_B2B_TX_POWER           0
#endif

#if F_APP_SENSOR_MEMS_SUPPORT
#if F_APP_SENSOR_INV42607_SUPPORT
#undef F_APP_CWM_ALGO_SUPPORT
#define F_APP_CWM_ALGO_SUPPORT                      1
#endif

#if F_APP_SENSOR_QMI8658_SUPPORT
#undef F_APP_CWM_ALGO_SUPPORT
#define F_APP_CWM_ALGO_SUPPORT                      0
#endif
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
/** @brief  Config ANCS Client: 0-Not built in, 1-Open ANCS client function */
#define F_BT_ANCS_CLIENT_SUPPORT                    0
#define F_BT_ANCS_APP_FILTER                        F_BT_ANCS_CLIENT_SUPPORT
#define F_BT_ANCS_GET_APP_ATTR                      0
/** @brief  Config ANCS Client debug log: 0-close, 1-open  */
#define F_BT_ANCS_CLIENT_DEBUG                      0
#define F_APP_FINDMY_USE_UARP                       0 //this macro enable firmware update function
#endif

#if (F_APP_USER_EQ_SUPPORT == 1)
#undef F_APP_AUDIO_VOCIE_SPK_EQ_INDEPENDENT_CFG
#define F_APP_AUDIO_VOCIE_SPK_EQ_INDEPENDENT_CFG    1
#undef F_APP_AUDIO_VOCIE_SPK_EQ_COMPENSATION_CFG
#define F_APP_AUDIO_VOCIE_SPK_EQ_COMPENSATION_CFG   1
#endif

#if F_APP_ERWS_SUPPORT && F_APP_LEA_SUPPORT
#undef  F_APP_B2B_HTPOLL_CONTROL
#define F_APP_B2B_HTPOLL_CONTROL                    1
#endif

/* Gaming headset related
*  TARGET_LE_AUDIO_GAMING: TWS
*  TARGET_LEGACY_AUDIO_GAMING: Stereo
*/
#if (TARGET_LE_AUDIO_GAMING == 1) || (TARGET_LEGACY_AUDIO_GAMING == 1)

#undef  F_APP_LEA_SUPPORT
#if (TARGET_DISABLE_LEA_SUPPORT == 1)
#define F_APP_LEA_SUPPORT                           0
#else
#define F_APP_LEA_SUPPORT                           1
#endif

#undef  F_APP_LEA_DISABLE_BIS
#define F_APP_LEA_DISABLE_BIS                       1
#undef  CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
#define CONFIG_REALTEK_APP_BOND_MGR_SUPPORT         1
#undef  F_APP_COMMON_DONGLE_SUPPORT
#define F_APP_COMMON_DONGLE_SUPPORT                 1
#undef  F_APP_GAMING_DONGLE_SUPPORT
#define F_APP_GAMING_DONGLE_SUPPORT                 1
#undef  F_APP_DONGLE_MULTI_PAIRING
#define F_APP_DONGLE_MULTI_PAIRING                  1
#undef  F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT
#define F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT      1
#undef  F_APP_CFU_FEATURE_SUPPORT
#define F_APP_CFU_FEATURE_SUPPORT                   1
#undef  F_APP_CFU_SPP_SUPPORT
#define F_APP_CFU_SPP_SUPPORT                       1
#undef  F_APP_CUSTOMIZED_SBC_VP_FLOW_SUPPORT
#define F_APP_CUSTOMIZED_SBC_VP_FLOW_SUPPORT        1
#undef  F_APP_HANDLE_TOO_SMALL_ABS_VOL
#define F_APP_HANDLE_TOO_SMALL_ABS_VOL              1
#undef  F_APP_CHANGE_PLAY_PAUSE_TO_SWITCH_PAIRING_IN_PAIRING_MODE
#define F_APP_CHANGE_PLAY_PAUSE_TO_SWITCH_PAIRING_IN_PAIRING_MODE   1
#undef  F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY
#define F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY   1

#if F_APP_LEA_SUPPORT
#undef  F_APP_DUPLICATE_ISO_FOR_DEJITTER
#define F_APP_DUPLICATE_ISO_FOR_DEJITTER            1
#endif

#if F_APP_LINEIN_SUPPORT || F_APP_USB_AUDIO_SUPPORT
#undef  F_APP_GAMING_WIRED_MODE_HANDLE
#define F_APP_GAMING_WIRED_MODE_HANDLE              1
#endif

#if (TARGET_LE_AUDIO_GAMING == 1)
#undef  F_APP_LEA_DONGLE_BINDING
#define F_APP_LEA_DONGLE_BINDING                    1
#undef  F_APP_LEA_ALWAYS_CONVERSATION
#define F_APP_LEA_ALWAYS_CONVERSATION               1
#undef  F_APP_SINGLE_MUTLILINK_SCENERIO_1
#define F_APP_SINGLE_MUTLILINK_SCENERIO_1           0
#undef  F_APP_MUTILINK_VA_PREEMPTIVE
#define F_APP_MUTILINK_VA_PREEMPTIVE                1
#undef  F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
#define F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING    1
#undef  F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
#define F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST      1
#undef  F_APP_DISABLE_LEA_PHONE_SUPPORT
#define F_APP_DISABLE_LEA_PHONE_SUPPORT             1
#undef  F_APP_INCREASE_RTK_APP_LINK_PRIORITY_TEMPORARILY
#define F_APP_INCREASE_RTK_APP_LINK_PRIORITY_TEMPORARILY    1

#if F_APP_ERWS_SUPPORT && F_APP_LEA_SUPPORT
#undef  F_APP_B2B_ENGAGE_IMPROVE_BY_LEA_DONGLE
#define F_APP_B2B_ENGAGE_IMPROVE_BY_LEA_DONGLE      1
#undef  F_APP_B2B_HTPOLL_CONTROL
#define F_APP_B2B_HTPOLL_CONTROL                    1
#endif

#if (TARGET_RTL8773DO || TARGET_RTL8773DFL)
#undef  F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#define F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT        1
#endif

#else
#undef  F_APP_LEGACY_DONGLE_BINDING
#define F_APP_LEGACY_DONGLE_BINDING                 1
#undef  F_APP_SUPPORT_LINKBACK_WHEN_DONGLE_STREAMING
#define F_APP_SUPPORT_LINKBACK_WHEN_DONGLE_STREAMING    1
#undef  F_APP_ALLOW_LEGACY_GAMING_TX_3M
#define F_APP_ALLOW_LEGACY_GAMING_TX_3M             1
#undef  F_APP_RTP_USE_LC3
#define F_APP_RTP_USE_LC3                           1

#if (F_APP_GAMING_CONTROLLER_SUPPORT == 1)
#undef  F_APP_A2DP_CODEC_LC3_SUPPORT
#define F_APP_A2DP_CODEC_LC3_SUPPORT                1
#endif
#endif

/* external mic support start*/
#undef  F_APP_EXT_MIC_SWITCH_SUPPORT
#define F_APP_EXT_MIC_SWITCH_SUPPORT                0

#if F_APP_EXT_MIC_SWITCH_SUPPORT
//if gpio detect define is 0, it will apply adc detect
//gpio detect
//default pin is BOOM_IN_DETECT_PIN
#define F_APP_EXT_MIC_PLUG_IN_GPIO_DETECT           1

#if (F_APP_EXT_MIC_PLUG_IN_GPIO_DETECT == 0)
//adc detect
//default pin is BOOM_IN_ADC_DETECT_PIN
#define F_APP_EXT_MIC_PLUG_IN_ADC_DETECT            1
#endif

//switch mic1 / mic2
#define F_APP_EXT_MIC_SWITCH_PHYSICAL_MIC_SUPPORT   1

#if (F_APP_EXT_MIC_SWITCH_PHYSICAL_MIC_SUPPORT == 0)
//mic1 plug hw switch ic to switch mic
//default pin is MIC_SEL_PIN, DIO_MUTE_PIN
#define F_APP_EXT_MIC_SWITCH_IC_SUPPORT             1
#endif

#endif
/* external mic support end*/

#if ((TARGET_LEGACY_AUDIO_GAMING == 1) && (F_APP_USB_AUDIO_SUPPORT == 1))
#undef  F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
#define F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET    1
#undef  F_APP_ANDROID_VOL_SPECIAL_HANDLE
#define F_APP_ANDROID_VOL_SPECIAL_HANDLE                0
#endif

#if TARGET_LEGACY_AUDIO_GAMING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#undef  F_APP_GAMING_B2S_HTPOLL_SUPPORT
#define F_APP_GAMING_B2S_HTPOLL_SUPPORT             1
#endif

#if TARGET_RTL8763EWM && F_APP_USB_AUDIO_SUPPORT
#undef  F_APP_REDUCE_HEAP_USAGE
#define F_APP_REDUCE_HEAP_USAGE                     1
#endif

#if F_APP_ERWS_SUPPORT == 0
//linein for stereo
#undef  F_APP_LINEIN_SUPPORT
#define F_APP_LINEIN_SUPPORT                1

// reduce code size
#undef  F_APP_ADP_CMD_SUPPORT
#define F_APP_ADP_CMD_SUPPORT               0
#undef  F_APP_ONE_WIRE_UART_SUPPORT
#define F_APP_ONE_WIRE_UART_SUPPORT         0
#undef  F_APP_OTA_TOOLING_SUPPORT
#define F_APP_OTA_TOOLING_SUPPORT           0
#undef  F_APP_SENSOR_SUPPORT
#define F_APP_SENSOR_SUPPORT                0
#undef  F_APP_SENSOR_IQS773_873_SUPPORT
#define F_APP_SENSOR_IQS773_873_SUPPORT     0 //Light sensor, P sensor
#undef  F_APP_SENSOR_JSA1225_SUPPORT
#define F_APP_SENSOR_JSA1225_SUPPORT        0 //Light sensor
#undef  F_APP_SENSOR_JSA1227_SUPPORT
#define F_APP_SENSOR_JSA1227_SUPPORT        0 //Light sensor
#undef  F_APP_SENSOR_PX318J_SUPPORT
#define F_APP_SENSOR_PX318J_SUPPORT         0 //Light sensor
#undef  F_APP_SENSOR_HX3001_SUPPORT
#define F_APP_SENSOR_HX3001_SUPPORT         0 //Light sensor
#undef  F_APP_SENSOR_SL7A20_SUPPORT
#define F_APP_SENSOR_SL7A20_SUPPORT         0 //G seneor
#undef  F_APP_SENSOR_SC7A20_AS_LS_SUPPORT
#define F_APP_SENSOR_SC7A20_AS_LS_SUPPORT   0 //SC7A20 as light sensor

#undef  F_APP_SAIYAN_EQ_FITTING
#define F_APP_SAIYAN_EQ_FITTING             0
#undef  F_APP_DATA_CAPTURE_SUPPORT
#define F_APP_DATA_CAPTURE_SUPPORT          0
#undef  F_APP_SAIYAN_MODE
#define F_APP_SAIYAN_MODE                   0
#endif

#endif

/** End of APP_CHARGER
* @}
*/

#endif

