/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "ftl.h"
#include "patch_header_check.h"
#include "rtl876x.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_mmi.h"
#include "app_key_process.h"
#include "app_dsp_cfg.h"
#include "gap_le.h"
#include "fmc_api.h"
#if F_APP_LOCAL_PLAYBACK_SUPPORT
#include "usb_lib_ext.h"
#endif

#ifdef APP_LATENCY_MEASURE_CIS_DS
uint8_t first_track_start = true;
uint8_t first_enc_data_ind = true;
uint8_t first_iso_send = true;
uint8_t first_gap_send = true;
uint8_t first_end_to_air = true;
#endif

static const T_APP_CFG_CONST app_cfg_default =
{
    .sync_word = DATA_SYNC_WORD,

    //Timer

    //Key
    .key_multi_click_interval = 0,  //unit 100ms
    .key_long_press_interval = 8,
    .key_long_press_repeat_interval = 4,
    .key_power_on_interval = 25,
    .key_power_off_interval = 30,
    .key_enter_pairing_interval = 50,
    .key_factory_reset_interval = 100,
    .key_pinmux = {P1_2, P1_3, P1_4, P1_5, P1_6, P1_7, 0xFF, 0xFF},
    .key_enable_mask = 0x3F,
    .key_long_press_repeat_mask = 0x3C, //0x3C for repeat vol+/-,fastforward/rewind
    .key_short_press_tone_mask = 0x01,
    .key_long_press_tone_mask = 0x01,
    .hybrid_key_press_tone_mask = 0x0F,
    .key_high_active_mask = 0,
    .key_disable_power_on_off = 0,
    .key_trigger_long_press = 0,
    .mfb_replace_key0 = 0,
    .key_factory_reset_rws_interval = 100,

    //key_table[SHORT/LONG][BT_HFP_XXX][KEY0/1/...] = MMI_XXX
    //hybrid_key_table[BT_HFP_XXX][HY_KEY0/1/...]   = MMI_XXX
    //If the key is not used, set its value to MMI_NULL

    .hybrid_key_mapping[HY_KEY0][0]  = 0x03, //(key0+key1) combine-key
    .hybrid_key_mapping[HY_KEY0][1]  = 0x01, // long

    .hybrid_key_mapping[HY_KEY1][0]  = 0x0C, //(key2+key3) combine-key
    .hybrid_key_mapping[HY_KEY1][1]  = 0x01, // long

    .hybrid_key_mapping[HY_KEY2][0]  = 0x30, //(key4+key5) combine-key
    .hybrid_key_mapping[HY_KEY2][1]  = 0x01, // long

    .hybrid_key_mapping[HY_KEY3][0]  = 0x00,
    .hybrid_key_mapping[HY_KEY3][1]  = 0xFF, // Set invalid value if no key assigned to hybrid key

    .hybrid_key_mapping[HY_KEY4][0]  = 0x00,
    .hybrid_key_mapping[HY_KEY4][1]  = 0xFF, // Set invalid value if no key assigned to hybrid key

    .hybrid_key_mapping[HY_KEY5][0]  = 0x00,
    .hybrid_key_mapping[HY_KEY5][1]  = 0xFF, // Set invalid value if no key assigned to hybrid key

    .hybrid_key_mapping[HY_KEY6][0]  = 0x00,
    .hybrid_key_mapping[HY_KEY6][1]  = 0xFF, // Set invalid value if no key assigned to hybrid key

    .hybrid_key_mapping[HY_KEY7][0]  = 0x00,
    .hybrid_key_mapping[HY_KEY7][1]  = 0xFF, // Set invalid value if no key assigned to hybrid key

    //Tone

    //Peripheral
    .data_uart_tx_pinmux = P1_6,
    .data_uart_rx_pinmux = P1_7,
    .tx_wake_up_pinmux = 0xFF,
    .rx_wake_up_pinmux = 0xFF,
    .i2c_0_dat_pinmux = ADC_2,
    .i2c_0_clk_pinmux = ADC_3,
    .line_in_pinmux = 0xFF,
    .pwm_pinmux = P3_0,
    .led_0_pinmux = P1_0,
    .led_1_pinmux = P2_1,
    .led_2_pinmux = P2_2,

    .output_indication1_pinmux = 0xFF,
    .external_mcu_input_pinmux = 0xFF,
    .slide_switch_0_pinmux = 0xFF,
    .data_uart_baud_rate = BAUD_RATE_115200,


    //App option
    //app_option0
    .enable_power_off_to_dlps_mode = 0,
    //app_option1

    //app_option2
    .enable_data_uart = 0,
    .enable_tx_wake_up = 0,
    .enable_rx_wake_up = 0,
    .enable_dlps = 1,
    //app_option3
    .led_support = 1,
    .key_gpio_support = 1,
    .line_in_support = 0,
    .pwm_support = 0,
    .charger_support = 0,
    //app_option4

    //app_option5
    .discharger_support = 0,

    //app_option6
    .enable_led0_low_active = 0,
    .enable_led1_low_active = 0,
    .enable_led2_low_active = 0,
    //app_option7
    .enable_factory_reset_when_in_the_box = 0,

    //Miscellaneous
    .device_name_legacy_default = {"BBPro2 AUDIO"},
    .device_name_le_default = {"BBPro2 LE"},
    .class_of_device = {0x18, 0x04, 0x24},
    .battery_warning_percent = 0,
    .supported_profile_mask = 0x0F,
    .voice_prompt_language = 0,

    //DSP-related
    //dsp_option0

    //dsp_option1
    .dsp_log_pin = 0,
    .dsp_log_output_select = 0,

    //App option_2
    .enable_not_reset_device_name = 0,

    //app_option9

    //app_option10

    //app_option11
    .sw_led_support = 0,

    //app_option12

    //app_option13

    //app_option14

    //app_option15

    //Profile A2DP codec settings + profile link number
    //SBC settings

    //AAC settings

    //LDAC settings

    //Profile link
    .a2dp_link_number = 1,
    .hfp_link_number = 1,
    .spp_link_number = 1,
    .avrcp_link_number = 1,

    //app_option16
    .charger_control_by_mcu = 0,

    // usb page
    .dongle_usb_speed = 0,
};

/*
RTL8763BFP Module Registration Information.
[big endian]
Model ID: 0x4C6A38
Public Anti-spoofing Key: rqA1ifd7+U9M/5y6NXkAUQdSelN9uZ9izOT/KDh7Qs950pcS2Ruk1bvPqQ3aHo/g/U7NjCOqKJGvLcvjexR+4w==
Private Anti-spoofing Key: 5cYFDqabtvR3wqdKhyhYyscMsZDNcDKPdSnVIHhHlWg=
*/

static const T_APP_CFG_NV app_cfg_rw_default =
{
    .hdr.sync_word = DATA_SYNC_WORD,
    .hdr.length = sizeof(T_APP_CFG_NV),
    .single_tone_timeout_val = 20 * 1000, //20s
    .single_tone_tx_gain = 0,
    .single_tone_channel_num = 20,
};

T_APP_CFG_CONST app_cfg_const;
T_APP_CFG2_CONST app_cfg2_const;
T_APP_CFG_NV app_cfg_nv;

uint32_t app_cfg_reset(void)
{
    uint16_t temp_magic;
    uint8_t temp_bd_addr[6];
    uint8_t temp_device_name_legacy[40];
    uint8_t temp_device_name_le[40];
    uint8_t temp_reset_power_on;
    uint8_t temp_reset_done;
    uint32_t temp_sync_word;
    uint8_t temp_power_off_cause_cmd = 0;

    //Keep for restore
    temp_magic = app_cfg_nv.peer_valid_magic;
    if (app_key_is_enter_factory_reset())
    {
        app_cfg_nv.adp_factory_reset_power_on = 0;
    }

    temp_reset_power_on = app_cfg_nv.adp_factory_reset_power_on;
    if (app_cfg_nv.adp_factory_reset_power_on && app_cfg_nv.power_off_cause_cmd)
    {
        temp_power_off_cause_cmd = app_cfg_nv.power_off_cause_cmd;
    }

    temp_reset_done = app_cfg_nv.factory_reset_done;
    temp_sync_word = app_cfg_nv.hdr.sync_word;

    memcpy(temp_bd_addr, app_cfg_nv.bud_peer_factory_addr, 6);
    memcpy(temp_device_name_legacy, app_cfg_nv.device_name_legacy, 40);
    memcpy(temp_device_name_le, app_cfg_nv.device_name_le, 40);
    memcpy(&app_cfg_nv, &app_cfg_rw_default, sizeof(T_APP_CFG_NV));

    app_cfg_nv.adp_factory_reset_power_on = temp_reset_power_on;
    if (app_cfg_nv.adp_factory_reset_power_on && temp_power_off_cause_cmd)
    {
        app_cfg_nv.power_off_cause_cmd = temp_power_off_cause_cmd;
    }

    app_cfg_nv.peer_valid_magic = temp_magic;
    app_cfg_nv.factory_reset_done = temp_reset_done;
    memcpy(app_cfg_nv.bud_peer_factory_addr, temp_bd_addr, 6);
    memcpy(app_cfg_nv.device_name_legacy, temp_device_name_legacy, 40);
    memcpy(app_cfg_nv.device_name_le, temp_device_name_le, 40);

    if ((app_cfg_const.enable_not_reset_device_name == 0) ||
        (temp_sync_word != DATA_SYNC_WORD))  //First init
    {
        memcpy(&app_cfg_nv.device_name_legacy[0], &app_cfg_const.device_name_legacy_default[0], 40);
        memcpy(&app_cfg_nv.device_name_le[0], &app_cfg_const.device_name_le_default[0], 40);

        // update device name when factory reset
        bt_local_name_set(&app_cfg_nv.device_name_legacy[0], GAP_DEVICE_NAME_LEN);
        le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, &app_cfg_nv.device_name_le[0]);
    }

    //app_cfg_nv.bud_role = app_cfg_const.bud_role;
    // memcpy(app_cfg_nv.bud_peer_addr, app_cfg_const.bud_peer_addr, 6);
    memcpy(app_cfg_nv.bud_local_addr, app_db.factory_addr, 6);
    app_cfg_nv.app_is_power_on = 0;

    /* if (app_cfg_const.charger_support)
     * {
     *     if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
     *     {
     *         app_cfg_nv.adaptor_changed = 1;

     *         if (get_adaptor_plug_state() == ADAPTOR_PLUG)
     *         {
     *             app_cfg_nv.adaptor_is_plugged = 1;
     *         }
     *         else
     *         {
     *             app_cfg_nv.adaptor_is_plugged = 0;
     *         }
     *     }
     * }
     */

    app_cfg_nv.voice_prompt_language = app_cfg_const.voice_prompt_language;

    /*     if (app_cfg_const.enable_rtk_charging_box)
     *     {
     *         app_adp_case_battery_check(&app_db.case_battery, &app_cfg_nv.case_battery);
     *     }
     */

#if F_APP_USB_GIP_SUPPORT
    app_cfg_nv.gip_speaker_volume = 0x32;
    app_cfg_nv.gip_game_chat_balance = 0x32;
#endif

    app_cfg_nv.led_brightness_percentage = 100;

    return ftl_save_to_storage(&app_cfg_nv, APP_RW_DATA_ADDR, sizeof(T_APP_CFG_NV));
}

void app_cfg_load(void)
{
    uint32_t sync_word = 0;

    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) +
                        APP_CONFIG_OFFSET),
                       (uint8_t *)&sync_word, DATA_SYNC_WORD_LEN);
    if (sync_word == DATA_SYNC_WORD)
    {
        fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) +
                            APP_CONFIG_OFFSET),
                           (uint8_t *)&app_cfg_const, sizeof(T_APP_CFG_CONST));
    }
    else
    {
        memcpy(&app_cfg_const, &app_cfg_default, sizeof(T_APP_CFG_CONST));
    }
    app_dsp_cfg_init(app_cfg_const.normal_apt_support);

    //read-write data
    ftl_load_from_storage(&app_cfg_nv.hdr, APP_RW_DATA_ADDR, sizeof(app_cfg_nv.hdr));

    if (app_cfg_nv.hdr.sync_word != DATA_SYNC_WORD)
    {
        //Load factory reset bit first when mppgtool factory reset
        if (app_cfg_nv.hdr.length == 0)
        {
            ftl_load_from_storage(&app_cfg_nv.eq_idx_anc_mode_record, APP_RW_DATA_ADDR + FACTORY_RESET_OFFSET,
                                  4);
        }

        app_cfg_reset();
    }
    else
    {
        uint32_t load_len = app_cfg_nv.hdr.length;

        if (load_len > sizeof(T_APP_CFG_NV))
        {
            APP_PRINT_ERROR0("app_cfg_load, error");
        }
        else
        {
            uint32_t res = ftl_load_from_storage(&app_cfg_nv, APP_RW_DATA_ADDR, load_len);

            if (res == 0)
            {

                if (load_len < sizeof(T_APP_CFG_NV))
                {
                    uint8_t *p_dst = ((uint8_t *)&app_cfg_nv) + load_len;
                    uint8_t *p_src = ((uint8_t *)&app_cfg_rw_default) + load_len;
                    memcpy(p_dst, p_src, sizeof(T_APP_CFG_NV) - load_len);
                }
                app_cfg_nv.hdr.length = sizeof(T_APP_CFG_NV);
            }
            else
            {
                app_cfg_reset();
            }

        }

    }

    /* donot open le-audio auto sch till all mode done */
    //app_cfg_const.dongle_windows_tool_support = true;
}

bool app_cfg_load_led_table(void *p_data, uint8_t mode, uint16_t led_table_size)
{
    uint16_t sync_word = 0;
    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) +
                        APP_LED_OFFSET),
                       (uint8_t *)&sync_word, APP_DATA_SYNC_WORD_LEN);
    if (sync_word == APP_DATA_SYNC_WORD)
    {
        fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) +
                            APP_LED_OFFSET + APP_DATA_SYNC_WORD_LEN + (mode * led_table_size)),
                           (uint8_t *)p_data, led_table_size);
        return true;
    }
    else
    {
        return false;
    }
}

uint32_t app_cfg_store(void)
{
    return ftl_save_to_storage(&app_cfg_nv, APP_RW_DATA_ADDR, sizeof(T_APP_CFG_NV));
}

void app_cfg_init(void)
{
    app_cfg_load();
}
