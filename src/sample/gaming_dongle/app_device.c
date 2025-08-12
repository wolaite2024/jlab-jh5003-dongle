/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "rtl876x_wdg.h"
#include "os_timer.h"
#include "trace.h"
//#include "dlps.h"
#include "app_timer.h"
#include "charger_api.h"
#include "audio_passthrough.h"
#include "sysm.h"
#include "app_device.h"
//#include "app_ble_device.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_dlps.h"
//#include "app_report.h"
#include "app_key_process.h"
#include "app_charger.h"
#include "app_led_ctl.h"
#include "bt_hfp.h"
#include "bt_avrcp.h"
#include "remote.h"
#include "voice_prompt.h"
#include "app_mmi.h"
//#include "app_sensor.h"
#include "app_dlps.h"
#include "rtl876x_tim.h"
#include "rtl876x_rcc.h"
#include "gap_vendor.h"
#include "app_ctrl_pkt_policy.h"

#if F_APP_LEA_DONGLE_BINDING
#include "app_le_audio.h"
#endif

#if TARGET_RTL8763EAU
#include "os_timer.h"
#include "app_usb.h"
#include "usb_dm.h"
#endif

#if (AUDIO_TEST_ENABLE == 1)
#include "audio_test.h"
#endif

#ifdef ENABLE_SPI0_SLAVE
#include "app_spi.h"
#endif


#define APP_POWERON_DELAY_TIMEOUT   (2000)

#define BIS_AUTO_START          0

#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif

#if (APP_SITRON_INIT == 1)
#include "app_dual_audio_effect.h"
#endif
//#include "app_line_in.h"
#include "pm.h"
#include "app_le_ext_ftl.h"
#include "le_unicast_src_service.h"

#include "app_audio_path.h"
#include "legacy_audio_wrapper.h"
#include "general_audio_wrapper.h"
#include "le_audio_wrapper.h"
#include "gap_br.h"
#include "le_audio_data.h"
#ifdef LEGACY_BT_GAMING
#include "app_bond.h"
#endif
#ifdef LEGACY_BT_GENERAL
#include "app_src_storage_target_dev.h"
#include "app_ctrl_cfg.h"
#endif
#include "gap_bond_le.h"
#include "app_gaming_sync.h"

#if DONGLE_LE_AUDIO
#if F_APP_LEA_DONGLE_BINDING
#include "le_audio_wrapper.h"
#endif
#endif

static uint8_t device_timer_id = 0;

void (*app_power_on_hook)(void) = NULL;
void (*app_power_off_hook)(void) = NULL;

static bool app_device_power_on = false;

static uint8_t timer_idx_wait_led_stop = 0;
static uint8_t timer_idx_device_reboot = 0;
static uint8_t timer_idx_factory_reset_gap = 0;
static uint8_t timer_idx_dsp_init = 0;
static uint8_t timer_idx_app_poweron = 0;

#if TARGET_RTL8763EAU
typedef enum _ACTIVE_CLK_TYPE
{
    CLK_CPU,
    CLK_DSP,
    CLK_SPIC0,
    CLK_SPIC1,
    CLK_SPIC2,
    CLK_SPIC3,
    CLK_PSRAM,
    CLK_40M,
    CLK_MAX
} ACTIVE_CLK_TYPE;

typedef enum _ACTIVE_CLK_SRC
{
    CLK_SRC_XTAL  = 0,
    CLK_SRC_PLL1,
    CLK_SRC_PLL2,
    CLK_SRC_OSC,
    CLK_SRC_DEF_MAX,
} ACTIVE_CLK_SRC_TYPE;
typedef void *TimerHandle_t;
typedef void (*VOID_FUNC)();
extern void (*set_clock_output)(ACTIVE_CLK_SRC_TYPE type, bool is_enable);
extern void (*set_clock_gen)();
extern TimerHandle_t internal_32k_cal_timer;
#endif
typedef enum
{
    APP_IO_TIMER_WAIT_LED_STOP,
    APP_DEVICE_TIMER_REBOOT,
    APP_TIMER_FACTORY_RESET_GAP,
    APP_TIMER_DSP_INIT,
    APP_TIMER_POWER_ON,
} T_DEVICE_TIMER;

#if TARGET_RTL8763EAU
void app_device_set_cpu_to_osc(void)
{
    os_timer_stop(&internal_32k_cal_timer); // 32k sdmk timer need xtal40 as ref clock

    set_clock_output(CLK_SRC_OSC, true);
    set_clock_gen(CLK_40M, CLK_SRC_OSC);
    set_clock_gen(CLK_CPU, CLK_SRC_OSC);
    set_clock_gen(CLK_SPIC0, CLK_SRC_OSC);

    set_clock_output(CLK_SRC_XTAL, false);
}

void app_device_set_cpu_to_xtal(void)
{
    set_clock_output(CLK_SRC_XTAL, true);
    set_clock_gen(CLK_SPIC0, CLK_SRC_XTAL);
    set_clock_gen(CLK_CPU, CLK_SRC_XTAL);
    set_clock_gen(CLK_40M, CLK_SRC_XTAL);

    os_timer_start(&internal_32k_cal_timer); // 32k sdmk timer need xtal40 as ref clock

    set_clock_output(CLK_SRC_OSC, false);
}
#endif

static bool app_set_le_psd_chnl_map_option(bool enable)
{
    uint8_t params[3] = {0x0};

    uint8_t sub_opcode = 11;
    uint8_t mask = 5;

    params[0] = sub_opcode;
    params[1] = mask;
    params[2] = enable ? 1 : 0;

    APP_PRINT_INFO1("app_set_le_psd_chnl_map_option %d", params[2]);
    return gap_vendor_cmd_req(0xFC6E, sizeof(params), params);
}

static void app_device_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_device_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_IO_TIMER_WAIT_LED_STOP:
        {
            app_stop_timer(&timer_idx_wait_led_stop);
        }
        break;
    case APP_DEVICE_TIMER_REBOOT:
        {
            app_stop_timer(&timer_idx_device_reboot);
            chip_reset(RESET_ALL);
        }
        break;
    case APP_TIMER_FACTORY_RESET_GAP:
        {
            app_stop_timer(&timer_idx_factory_reset_gap);
            app_cfg_nv.adp_factory_reset_power_on = 0;
            app_cfg_store();
        }
        break;

    case APP_TIMER_DSP_INIT:
        {
            app_stop_timer(&timer_idx_dsp_init);
        }
        break;

    case APP_TIMER_POWER_ON:
        APP_PRINT_INFO0("app_device_timer_callback: app poweron");
        app_stop_timer(&timer_idx_app_poweron);
        /* DSP image start loading when dsp init timer expired. And the image
         * would be loaded before cpu ran here */

        app_led_set_mode(APP_LED_MODE_POWER_ON);

#ifdef LEGACY_BT_GAMING
        /* Set LED to idle state */
        app_led_set_mode(APP_LED_MODE_LINK1_STANDBY);
        app_led_set_mode(APP_LED_MODE_LINK2_STANDBY);
#endif

        /* Set special vendor cmd to lowstack */
        app_set_le_psd_chnl_map_option(false);
        /* TODO: Enter legacy audio by default. */
        app_bt_mode_init();
#if 0
#ifdef LEGACY_BT_GAMING
        legacy_audio_enter();
#else
        general_audio_enter();
#endif
#endif
#if (AUDIO_TEST_ENABLE == 1)
        audio_test_lc3_track_line_in_init();
        audio_test_lc3_track_spk_out_init();
#endif
#if BIS_AUTO_START
#ifdef LEGACY_BT_GAMING
        legacy_audio_exit();
#else
        general_audio_exit();
#endif
        le_audio_enter(false);

        le_audio_mmi_bis(MMI_DONGLE_LINK1_KEY_SHORT_PRESS, NULL);
#endif
        /* FIXME: Why not call application func directly? */
        if (app_power_on_hook)
        {
            app_power_on_hook();
        }

#if DONGLE_LE_AUDIO
#if F_APP_LEA_DONGLE_BINDING
        le_audio_power_on_start_scan();
#endif
#endif

        if (app_power_on_hook)
        {
            app_power_on_hook();
        }

#ifdef ENABLE_SPI0_SLAVE
        app_spi_init(1000000);
#endif

        app_device_power_on = true;
#if TARGET_RTL8763EAU
        /* After power on, if usb suspend is enabled, dongle goes to suspend.
        This judgment needs to be placed at the end of poweron's timer to
        prevent suspend from taking effect */
        if (app_usb_power_state() == USB_SUSPENDED)
        {
            app_usb_dm_evt_suspend_handle();
        }
#endif
        break;

    default:
        break;
    }
}

bool app_device_is_power_on(void)
{
    return app_device_power_on;
}

void app_device_reboot(uint32_t timeout_ms, T_WDG_MODE wdg_mode)
{
    if (timeout_ms == 0)
    {
        chip_reset(wdg_mode);
    }
    else
    {
        app_start_timer(&timer_idx_device_reboot, "device_reboot",
                        device_timer_id, APP_DEVICE_TIMER_REBOOT, 0, false,
                        timeout_ms);
    }
}

void app_device_factory_reset(void)
{
    APP_PRINT_INFO0("app_device_factory_reset");
    if (!app_db.ignore_led_mode)
    {
    }
    else
    {
        app_db.ignore_led_mode = false;
    }

#ifdef LEGACY_BT_GAMING
    app_src_legacy_clear_all_keys();
#endif

#ifdef LEGACY_BT_GENERAL
    app_src_storage_clear_target_dev(DEV_SLOT_HS);
#endif

    le_bond_clear_all_keys();
    lea_ext_clear_hash();
    le_unicast_src_release_group();

    app_cfg_reset();

    app_cfg_nv.factory_reset_done = 1;

    if (app_cfg_store() != 0)
    {
        APP_PRINT_ERROR0("app_device_dm_cback: save nv cfg data error");
    }

    if (app_cfg_nv.adp_factory_reset_power_on == 1)
    {
        app_device_reboot(1000, RESET_ALL);
    }
}

void app_device_state_change(T_APP_DEVICE_STATE state)
{
    APP_PRINT_INFO2("app_device_state_change: cur_state 0x%02x, next_state 0x%02x",
                    app_db.device_state,
                    state);
    app_db.device_state = state;
    //app_report_event(CMD_PATH_UART, EVENT_DEVICE_STATE, 0, &app_db.device_state,
    //                 sizeof(app_db.device_state));
}

T_APP_DEVICE_STATE app_device_state_get(void)
{
    return app_db.device_state;
}

static void app_device_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            gap_get_param(GAP_PARAM_BD_ADDR, dongle_status.dongle_addr);

            app_device_state_change(APP_DEVICE_STATE_ON);

            /* app_dlps_enable_auto_poweroff_stop_wdg_timer(); */

            app_db.power_off_cause_in_box = false;

            if (!app_db.ignore_led_mode)
            {
                app_led_set_mode(APP_LED_MODE_POWER_ON);
            }
            else
            {
                app_db.ignore_led_mode = false;
            }

            app_key_set_power_on(true);

            //Update battery volume after power on
            if (app_cfg_const.discharger_support)
            {
                uint8_t state_of_charge;

                state_of_charge = charger_api_get_state_of_charge();
                if (state_of_charge > 100)
                {
                    app_db.local_batt_level = app_cfg_nv.local_level;
                }
                else
                {
                    app_db.local_batt_level = state_of_charge;//initial battery here
                }
                //app_handle_state_of_charge(state_of_charge, 1);
            }

            //app_ble_device_handle_power_on();

            APP_PRINT_INFO1("app_device_dm_cback lin_in_supprot: 0x%x", app_cfg_const.line_in_support);
            if (app_cfg_const.line_in_support)
            {
                app_line_in_power_on_check();
                le_audio_set_audio_path(app_cfg_const.dongle_media_device);
            }
            power_mode_set(POWER_DLPS_MODE);

            //Update battery volume after power on
            if (app_cfg_const.discharger_support)
            {
                app_charger_update();
            }

            if (!app_cfg_nv.app_is_power_on)
            {
                app_cfg_nv.app_is_power_on = 1;
                app_cfg_store();
            }

            //app_listening_apply_when_power_on();

            /* NOTE: We moved the app_power_on_hook calling to the timer func.
             * The hook will be called when APP_TIMER_POWER_ON timer expired.
             * */
            app_start_timer(&timer_idx_app_poweron, "app_poweron",
                            device_timer_id, APP_TIMER_POWER_ON, 0, false,
                            APP_POWERON_DELAY_TIMEOUT);

            //app_auto_power_off_enable(AUTO_POWER_OFF_MASK_POWER_ON, app_cfg_const.timer_auto_power_off);
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            app_device_state_change(APP_DEVICE_STATE_OFF_ING);

            //app_ble_device_handle_power_off();

            //app_auto_power_off_enable(~AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF,
            //                          app_cfg_const.timer_auto_power_off);

            app_led_set_mode(APP_LED_MODE_POWER_OFF);
            app_key_set_power_on(false);

            app_device_power_on = false;
            if ((!app_db.power_off_cause_in_box) && (app_cfg_nv.adp_factory_reset_power_on == 0))
            {
                //app_audio_tone_play(app_cfg_const.tone_power_off, false, false);
            }

            if (app_cfg_const.line_in_support)
            {
                app_line_in_power_off_check();
            }

            if (app_cfg_const.discharger_support)
            {
                app_cfg_nv.local_level = app_db.local_batt_level;
                app_cfg_nv.remote_level = app_db.remote_batt_level;
            }

            if (app_power_off_hook)
            {
                app_power_off_hook();
            }

            if (app_cfg_nv.app_is_power_on)
            {
                app_cfg_nv.app_is_power_on = 0;

                app_cfg_store();
            }

            //app_listening_state_machine(EVENT_ALL_OFF, 0);

            //app_bt_policy_shutdown();

#if (SUPPORT_AIRPLANE == 1)
            app_airplane_power_off_handle();
#endif
            //app_bond_clear_sec_diff_link_record();

            app_dlps_power_off();
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_device_dm_cback: event_type 0x%04x", event_type);
    }
}

/* @brief  wheather the test equipment is required device
*
* @param  name and addr
* @return true/false
*/
static bool app_device_is_test_equipment(const char *name, uint8_t *addr)
{
    T_APP_TEST_EQUIPMENT_INFO equipment_info[] =
    {
        {{0x00, 0x30, 0xD3}, "AGILENT TECHNOLOGIES"},
        {{0x8C, 0x14, 0x7D}, "AGILENT TECHNOLOGIES"},
        {{0x00, 0x04, 0x43}, "AGILENT TECHNOLOGIES"},
        {{0xBD, 0xBD, 0xBD}, "AGILENT TECHNOLOGIES"},
        {{0x00, 0x02, 0xB1}, "ANRITSU MT8852"},
        {{0x00, 0x00, 0x91}, "ANRITSU MT8852"},
        {{0x00, 0x00, 0x91}, "ANRITSU MT8850A Bluetooth Test set"},
        {{0x70, 0xB3, 0xD5}, "R&S CMW"},
        {{0x00, 0x90, 0xB8}, "R&S CMW"},
        {{0xDC, 0x44, 0x27}, "R&S CMW"},
        {{0x70, 0xB3, 0xD5}, "R&S CMU"},
        {{0x00, 0x90, 0xB8}, "R&S CMU"},
        {{0xDC, 0x44, 0x27}, "R&S CMU"},
        {{0x70, 0xB3, 0xD5}, "R&S CBT"},
        {{0x00, 0x90, 0xB8}, "R&S CBT"},
        {{0xDC, 0x44, 0x27}, "R&S CBT"},
    };

    bool ret = false;
    uint8_t i = 0;
    uint8_t device_oui[3] = {0};

    device_oui[0] = addr[5];
    device_oui[1] = addr[4];
    device_oui[2] = addr[3];

    for (i = 0; i < sizeof(equipment_info) / sizeof(T_APP_TEST_EQUIPMENT_INFO); i++)
    {
        if (!strncmp(equipment_info[i].name, name, strlen(equipment_info[i].name)) &&
            !memcmp(device_oui, equipment_info[i].oui, sizeof(equipment_info[i].oui)))
        {
            ret = true;
            break;
        }
    }

    APP_PRINT_TRACE3("app_device_is_test_equipment: ret %d name %s oui %b", ret, TRACE_STRING(name),
                     TRACE_BINARY(3, device_oui));

    return ret;
}

static void app_device_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;
    bool power_on_flg = false;

#if LOCAL_PLAYBACK_FEATURE_SUPPORT
    bool usb_need_start = false;
#endif

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            if (app_db.key0_wake_up || app_db.combine_poweron_key_wake_up)
            {
                APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, key0_wake_up, do nothing");
            }
            else if (app_cfg_nv.adp_factory_reset_power_on)
            {
                app_cfg_nv.adp_factory_reset_power_on = 0;
                app_cfg_store();
            }
            else if (app_cfg_nv.app_is_power_on && !app_db.peri_wake_up)
            {
                if (app_device_is_in_the_box())
                {
                    APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, app_is_power_on, in the box, do nothing");
                }
                else
                {
                    APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, app_is_power_on, boot up");

                    power_on_flg = true;
                }
            }
            else
            {
                if (!app_cfg_nv.factory_reset_done)
                {
                    APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, not factory_reset_done, do nothing");
                }
                else
                {
#if 0
                    if (!app_cfg_const.enable_outbox_power_on)
                    {
                        APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, not enable_outbox_power_on, do nothing");
                    }
                    else
#endif
                    {
#if 0
                        if (app_cfg_const.box_detect_method == GPIO_DETECT)
                        {
                        }
                        else if (app_cfg_const.box_detect_method == ADAPTOR_DETECT && app_cfg_const.charger_support)
                        {
                            if (0 /* get_adaptor_plug_state() == ADAPTOR_UNPLUG */)
                            {
                                if (app_cfg_const.enable_rtk_charging_box)
                                {
#ifdef ADAPT_NEW_RWS
                                    if ((adp_get_current_state(ADP_DETECT_5V) != ADP_STATE_OUT) ||
                                        app_cfg_nv.power_off_cause_cmd) //maybe adp detect status is debouncing from reset
                                    {
#if LOCAL_PLAYBACK_FEATURE_SUPPORT
                                        if ((app_cfg_nv.power_off_cause_cmd) && (app_cfg_nv.usb_need_start != 0))
                                        {
                                            usb_need_start = true;
                                        }
#endif
                                        APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, out box ignore");
                                    }
                                    else
                                    {
                                        power_on_flg = true;
                                    }
#endif
                                }
                                else
                                {
                                    power_on_flg = true;
                                }

                                if (power_on_flg)
                                {
                                    APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, ADAPTOR_UNPLUG, power on");
                                }
                            }
                            else
                            {
                                APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, ADAPTOR_PLUG, do nothing");
                            }
                        }
                        else
                        {
                            APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, DISABLE_DETECT, do nothing");
                        }
#endif
                    }
                }
            }
            power_on_flg = true;
            if (power_on_flg)
            {
                if (app_db.device_state == APP_DEVICE_STATE_ON)
                {
                    APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, already power on, do nothing");
                }
                else
                {
                    if ((app_cfg_const.discharger_support) && (charger_api_get_state_of_charge() == BAT_CAPACITY_0))
                    {
                        APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, low battery, do nothing");
                    }
                    else
                    {
                        APP_PRINT_INFO0("app_device_bt_cback: BT_EVENT_BT_READY, power off state, goto power on");

                        //app_audio_tone_play(app_cfg_const.tone_power_on, false, false);
                        app_mmi_handle_action(MMI_DEV_POWER_ON);
                        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                    }
                }
            }

#if LOCAL_PLAYBACK_FEATURE_SUPPORT
            APP_PRINT_INFO3("app_device_bt_cback: power_off_cause_cmd %d usb_need_start_nv %d usb_need_start %d",
                            app_cfg_nv.power_off_cause_cmd, app_cfg_nv.usb_need_start, usb_need_start);

            if (usb_need_start)
            {
                usb_need_start = false;
                app_cfg_nv.usb_need_start = 0;
                app_cfg_store();

                if (!power_on_flg)
                {
                    //app_adp_usb_start_handle();
                }
            }
#endif
            //app_dlps_check_direct_power_on();

            app_db.bt_is_ready = true;

#if F_APP_BB2_EXTERNAL_PA_SUPPORT
            if (app_dlps_is_support_external_pa())
            {
                uint16_t aon_1_0 = btaon_fast_read_safe(0x1568);
                uint16_t aon_1_1 = btaon_fast_read_safe(0x156A);
                uint8_t p_buf[3] = {0x0B, 0x0E, 0x0};

                APP_PRINT_TRACE5("app_device_bt_cback: before init 0x40058030 = 0x%x, 0x400002A8 = 0x%x, aon_1_0 = 0x%x, aon_1_1 = 0x%x, pinmux = 0x%x",
                                 *((volatile uint32_t *)0x40058030), *((volatile uint32_t *)0x400002A8), \
                                 aon_1_0, aon_1_1, PINMUX0->CFG[2]);

                gap_vendor_cmd_req(0xFC6E, sizeof(p_buf), p_buf);

                *((volatile uint32_t *)0x40058030) &= 0xFFFFFC00;
                *((volatile uint32_t *)0x40058030) |= 0xA0;

                *((volatile uint32_t *)0x400002A8) |= BIT(28);
                *((volatile uint32_t *)0x400002A8) &= 0xFFF1FFFF;
                *((volatile uint32_t *)0x400002A8) |= BIT(16);

                Pinmux_Config(P1_0, DIGI_DEBUG);
                Pinmux_Config(P1_1, DIGI_DEBUG);
                Pad_Config(P1_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
                Pad_Config(P1_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);

                aon_1_0 = btaon_fast_read_safe(0x1568);
                aon_1_1 = btaon_fast_read_safe(0x156A);

                APP_PRINT_TRACE5("app_device_bt_cback: after init 0x40058030 = 0x%x, 0x400002A8 = 0x%x, aon_1_0 = 0x%x, aon_1_1 = 0x%x, pinmux = 0x%x",
                                 *((volatile uint32_t *)0x40058030), *((volatile uint32_t *)0x400002A8), \
                                 aon_1_0, aon_1_1, PINMUX0->CFG[2]);
            }
#endif

        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            //we need to get remote name to judge whether enter dut mode or not.
            app_db.force_enter_dut_mode_when_acl_connected = false;
            gap_br_get_remote_name(param->acl_conn_success.bd_addr);
        }
        break;

    case BT_EVENT_REMOTE_NAME_RSP:
        {
            if (app_device_is_test_equipment(param->remote_name_rsp.name,
                                             param->remote_name_rsp.bd_addr))
            {
                app_db.force_enter_dut_mode_when_acl_connected = true;
                bt_sniff_mode_disable(param->remote_name_rsp.bd_addr);
                app_mmi_handle_action(MMI_DUT_TEST_MODE);
                app_db.force_enter_dut_mode_when_acl_connected = false;
            }
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
            switch (param->hfp_call_status.curr_status)
            {
            case BT_HFP_CALL_INCOMING:
#ifdef BUZZER_SUPPORT
                buzzer_set_mode(INCOMING_CALL_BUZZER);
#endif
                break;

            case BT_HFP_CALL_ACTIVE:
                {
                }
                break;

            default:
                break;
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {

        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_device_bt_cback: event_type 0x%04x", event_type);
    }
}

bool app_device_is_in_the_box(void)
{
    bool rtn = false;
#if 0
    if (app_cfg_const.charger_support)
    {
        if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
        {
            if (0 /* get_adaptor_plug_state() == ADAPTOR_PLUG */)
            {
                rtn = true;
            }
        }
    }
    else if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
    }

    APP_PRINT_INFO1("app_device_is_in_the_box %d", rtn);
#endif
    return rtn;
}

void app_device_handle_remote_msg(uint16_t msg, void *buf)
{
    switch (msg)
    {
    default:
        break;
    }
}

void app_device_bt_policy_startup(bool at_once_trigger)
{
    //app_bt_policy_startup(app_device_link_policy_ind, at_once_trigger);
}

bool app_device_save_pairing_id(uint8_t *id, uint8_t len)
{
    if (len != 3)
    {
        return false;
    }

    if (memcmp(app_cfg_nv.saved_id, id, 3))
    {
        memcpy(app_cfg_nv.saved_id, id, 3);

#if F_APP_LEA_DONGLE_BINDING
        /* Note! for lea dongle if we want to pair a new bud pair
         * we need to clear the exist bud record and scan white list
         */

        app_le_audio_clear_bud_record();
        app_le_audio_clear_white_list();
#endif

        app_cfg_store();
    }

    return true;
}

void app_device_init(void)
{
    sys_mgr_cback_register(app_device_dm_cback);
    bt_mgr_cback_register(app_device_bt_cback);
    app_timer_reg_cb(app_device_timeout_cb, &device_timer_id);
    /* TODO: LED pin configuration. */
}
