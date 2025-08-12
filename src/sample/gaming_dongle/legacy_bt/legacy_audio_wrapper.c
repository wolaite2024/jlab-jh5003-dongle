/*
 *  Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include "trace.h"
#include "legacy_audio_wrapper.h"
#include "gaming_bt.h"
#include "app_link_util.h"
#include "app_mmi.h"
#include "app_led_ctl.h"
#include "ual_adapter.h"
#include "app_cfg.h"
#include "app_gaming_ctrl_cfg.h"
#include "app_usb_audio_wrapper.h"

#if F_APP_USB_GIP_SUPPORT
#include "app_gip.h"
#include "app_gip_security.h"
#endif

#ifdef LEGACY_BT_GAMING
#include "app_usb.h"

static uint8_t gaming_enabled;

int scan_id = 0;

bool force_enter_pairing = false;

void src_start_le_scan(void)
{
    scan_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, GAP_SCAN_FILTER_ANY,
                                      gaming_handle_ext_adv_report_info);

    APP_PRINT_INFO1("src_start_le_scan: scan_id %d", scan_id);
}

void src_stop_le_scan(void)
{
    int ret;
    ret = bt_adap_stop_discovery(scan_id);

    APP_PRINT_INFO1("src_stop_le_scan: ret %d", ret);
}

static void gaming_bt_handle_pair_id_action()
{
#if F_APP_LEGACY_DONGLE_BINDING
    uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

    gaming_bt_connect(0, ba_any);
#else
    uint8_t saved_id_empty[3] = {0, 0, 0};
    bool is_saved_id_exist = memcmp(app_cfg_nv.saved_id, saved_id_empty, 3);
    bool is_bond_record_exist = gaming_bt_have_saved_id_bond_record();

    APP_PRINT_TRACE3("gaming_bt_handle_pair_id_action: saved_id %b, is_saved_id_exist %d, is_bond_record_exist %d",
                     TRACE_BINARY(3, app_cfg_nv.saved_id),
                     is_saved_id_exist,
                     is_bond_record_exist);

    if (is_saved_id_exist && is_bond_record_exist == false)
    {
        uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

        gaming_bt_connect(0, ba_any);
    }
#endif
}

static void gaming_led_indicator(uint8_t *data, uint16_t len)
{
    struct gaming_ev_led_state *ev;

    if (sizeof(*ev) != len)
    {
        APP_PRINT_ERROR2("gaming_led_indicator: len mismatch, %u, %u",
                         sizeof(*ev), len);
        return;
    }

    ev = (struct gaming_ev_led_state *)data;

    APP_PRINT_INFO1("gaming_led_indicator: led state %u", ev->led_state);

    switch (ev->led_state)
    {
    case GAMING_LED_BT_IDLE:
        if (ev->id == 0)
        {
            app_led_set_mode(APP_LED_MODE_LINK1_STANDBY);
        }
        else
        {
            app_led_set_mode(APP_LED_MODE_LINK2_STANDBY);
        }
        break;
    case GAMING_LED_BT_CONNECTING:
        if (ev->id == 0)
        {
            app_led_set_mode(APP_LED_MODE_LINK1_PAIRING);
        }
        else
        {
            app_led_set_mode(APP_LED_MODE_LINK2_PAIRING);
        }
        break;
    case GAMING_LED_BT_RECONNECTING:
        if (ev->id == 0)
        {
            app_led_set_mode(APP_LED_MODE_LINK1_RECONNECTING);
        }
        else
        {
            app_led_set_mode(APP_LED_MODE_LINK2_RECONNECTING);
        }
        break;
    case GAMING_LED_BT_CONNECTED:
        if (ev->id == 0)
        {
            app_led_set_mode(APP_LED_MODE_LINK1_CONNECTED);
        }
        else
        {
            app_led_set_mode(APP_LED_MODE_LINK2_CONNECTED);
        }
        break;
    default:
        break;
    }
}

bool gaming_bt_cback(uint8_t evt, uint8_t *data, uint16_t len)
{
    gaming_usb_ctrl_cmd_msg(evt, data, len);

    APP_PRINT_INFO3("gaming_bt_cback: evt %u, pdata %p, len %u",
                    evt, data, len);
    switch (evt)
    {
    case EVENT_GAMING_DEVICE_CONNECTED:
        {
            struct gaming_ev_device_connected *ev = (void *)data;

            if (!ev)
            {
                return false;
            }
            APP_PRINT_INFO2("gaming_bt_cback: device %u, %b connected", ev->id,
                            TRACE_BDADDR(ev->ba));
        }
        break;

    case EVENT_GAMING_DEVICE_DISCONNECTED:
        {
            struct gaming_ev_device_disconnected *ev = (void *)data;

            if (!ev)
            {
                return false;
            }
            APP_PRINT_INFO2("gaming_bt_cback: device %u, %b disconnected",
                            ev->id, TRACE_BDADDR(ev->ba));

            if (app_cfg_const.enable_dongle_dual_mode && !app_usb_is_suspend())
            {
#if F_APP_LEGACY_DONGLE_BINDING
                uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

                gaming_bt_force_connect(0, ba_any);
#else
                if (force_enter_pairing)
                {
                    uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

                    gaming_bt_force_connect(0, ba_any);

                    force_enter_pairing = false;
                }
#endif
            }

#if F_APP_USB_GIP_SUPPORT
            if (app_gip_get_switch_mode())
            {
                app_gip_handle_bt_evt(GIP_BT_EVT_DISCONN);
            }
#endif
        }
        break;

    case EVENT_GAMING_PROFILE_CONNECTED:
        {
            struct gaming_ev_profile_connected *ev = (void *)data;

            if (!ev)
            {
                return false;
            }
            if (ev->profile == SPP_AUDIO_PROFILE_MASK)
                APP_PRINT_INFO4("gaming_bt_cback: dev %u, %b, spp audio profile"
                                " %08x (ch %u) connected", ev->id,
                                TRACE_BDADDR(ev->ba), ev->profile,
                                ev->profile_data);
            else
                APP_PRINT_INFO3("gaming_bt_cback: dev %u, %b, profile %08x "
                                "connected", ev->id, TRACE_BDADDR(ev->ba),
                                ev->profile);


            if (app_cfg_const.enable_dongle_dual_mode)
            {
                gaming_bt_set_scan_by_mmi(false);
            }

#if F_APP_USB_GIP_SUPPORT
            if (app_gip_get_switch_mode())
            {
                app_gip_handle_bt_evt(GIP_BT_EVT_CONN);
            }
#endif
        }
        break;

    case EVENT_GAMING_PROFILE_DISCONNECTED:
        {
            struct gaming_ev_profile_disconnected *ev = (void *)data;

            if (!ev)
            {
                return false;
            }
            APP_PRINT_INFO3("gaming_bt_cback: dev %u, %b, profile %08x disc",
                            ev->id, TRACE_BDADDR(ev->ba), ev->profile);
        }
        break;

    case EVENT_GAMING_DEVICE_FOUND:
        {
            struct gaming_ev_device_found *ev = (void *)data;

            if (!ev)
            {
                return false;
            }
            APP_PRINT_INFO1("gaming_bt_cback: device %b found",
                            TRACE_BDADDR(ev->result.bd_addr));
        }
        break;

    case EVENT_GAMING_DISCOVERY_STOPPED:
        if (app_cfg_const.enable_dongle_dual_mode)
        {
            src_stop_le_scan();
        }
        break;

    case EVENT_GAMING_DISCOVERY_STARTING:
        if (app_cfg_const.enable_dongle_dual_mode)
        {
            src_start_le_scan();

            if (gaming_bt_get_scan_by_mmi() == true)
            {
                /* pairing */
                app_led_set_mode(APP_LED_MODE_LINK1_PAIRING);
            }
            else
            {
                /* linkback */
                app_led_set_mode(APP_LED_MODE_LINK1_STANDBY);
            }
        }
        break;

    case EVENT_GAMING_DISCOVERY_STARTED:
        break;

    case EVENT_GAMING_DISCOVERY_STOPPING:
        break;

    case EVENT_GAMING_LED_INDICATOR:
        gaming_led_indicator(data, len);
        break;

    default:
        break;
    }

    return true;
}

int legacy_audio_enter(void)
{
    gaming_enabled = 1;

    gaming_bt_enable();

    if (app_cfg_const.enable_dongle_dual_mode)
    {
        gaming_set_bt_mode(BT_DEVICE_MODE_CONNECTABLE);
        gaming_bt_handle_pair_id_action();
    }
    else
    {
        /* force enter dut mode after connected by 8852b. need open inquiry sacn.*/
        gaming_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
    }

    return 0;
}

void legacy_audio_exit(void)
{
    gaming_enabled = 0;

    gaming_bt_disable();
}

uint8_t gaming_get_status(uint8_t index)
{
    return gaming_bt_is_idle(index) ? 0 : 1;
}

/* TODO: Add app task evt and msg queue as func parameters? */
int gaming_init(void)
{
    gaming_bt_init();
    gaming_bt_reg_cback(gaming_bt_cback);

    return 0;
}

/* TODO: Add app task evt and msg queue as func parameters? */
int general_init(void)
{
    return 0;
}

int legacy_audio_init(void *evt_queue, void *msg_queue)
{
#if defined(LEGACY_BT_GAMING)
#ifdef ENABLE_UAC2
    uint8_t uac2 = 1;
#endif
#endif
    /* TODO avoid compiling warnings. */
    (void)evt_queue;
    (void)msg_queue;

#if defined(LEGACY_BT_GAMING)
#ifdef ENABLE_UAC2
    gaming_set_cfg(GAMING_CFG_UAC2, &uac2, sizeof(uac2));
#endif
    gaming_init();
#elif defined(LEGACY_BT_GENERAL)
    general_init();
#else
#error "Neither legacy bt gaming nor general is defined."
#endif
    return 0;
}

int legacy_audio_mmi(uint8_t mmi_action, void *params)
{
    if (!gaming_enabled)
    {
        return -1;
    }

    APP_PRINT_INFO1("legacy_audio_mmi: action %02x", mmi_action);

    switch (mmi_action)
    {
    case MMI_BT_LEGACY_LINK1_SCAN:
        {
            uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

            if (app_cfg_const.enable_dongle_dual_mode)
            {
                gaming_bt_set_scan_by_mmi(true);

                if (gaming_bt_is_connected())
                {
                    gaming_bt_disconnect_by_id(0);
#if F_APP_LEGACY_DONGLE_BINDING
#else
                    /* force disconnect and reconnect after dev disconnect */
                    force_enter_pairing = true;
#endif
                }
                else
                {
                    gaming_bt_connect(0, ba_any);
                    app_led_set_mode(APP_LED_MODE_LINK1_PAIRING);
                }
            }
            else
            {
                gaming_bt_connect(0, ba_any);
            }
        }
        break;

    case MMI_BT_LEGACY_LINK1_RECONNECT:
        {
            if (app_cfg_const.enable_dongle_dual_mode)
            {
                if (gaming_bt_get_scan_by_mmi() == true)
                {
                    gaming_bt_set_scan_by_mmi(false);

                    app_led_set_mode(APP_LED_MODE_LINK1_STANDBY);
                }
            }
            else
            {
                uint8_t bdaddr[6];
                if (gaming_bt_is_idle(0))
                {
                    if (gaming_bt_get_bond_bdaddr(0, bdaddr) < 0)
                    {
                        APP_PRINT_WARN0("legacy_audio_mmi: No bond for dev 0");
                        return -2;
                    }
                    gaming_bt_connect(0, bdaddr);
                }
            }
        }
        break;
    case MMI_BT_LEGACY_LINK1_DISCONNECT:
        {
            if (app_cfg_const.enable_dongle_dual_mode)
            {
                if (gaming_bt_get_scan_by_mmi() == true)
                {
                    gaming_bt_set_scan_by_mmi(false);

                    app_led_set_mode(APP_LED_MODE_LINK1_STANDBY);
                }
            }
            else
            {
                if (!gaming_bt_is_idle(0))
                {
                    gaming_bt_disconnect_by_id(0);
                    return 0;
                }
            }
        }
        break;
    case MMI_BT_LEGACY_LINK2_SCAN:
        {
            uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

            gaming_bt_connect(1, ba_any);
        }
        break;
    case MMI_BT_LEGACY_LINK2_RECONNECT:
        {
            uint8_t bdaddr[6];
            if (gaming_bt_is_idle(1))
            {
                if (gaming_bt_get_bond_bdaddr(1, bdaddr) < 0)
                {
                    APP_PRINT_WARN0("legacy_audio_mmi: No bond for dev 1");
                    return -2;
                }
                gaming_bt_connect(1, bdaddr);
            }
        }
        break;
    case MMI_BT_LEGACY_LINK2_DISCONNECT:
        {
            if (!gaming_bt_is_idle(1))
            {
                gaming_bt_disconnect_by_id(1);
                return 0;
            }
        }
        break;
    default:
        return -2;
    }

    return 0;
}
#endif
