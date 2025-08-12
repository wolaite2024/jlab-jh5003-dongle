/*
 *  Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "trace.h"
#include "app_mmi.h"
#include "app_le_audio.h"
#include "app_led_ctl.h"
#include "le_bst_src_service.h"
#include "app_le_audio.h"
#include "le_audio_wrapper.h"

#if F_APP_LEA_DONGLE_BINDING
#include "app_adapter_service.h"
#endif

#if DONGLE_LE_AUDIO
static uint8_t le_audio_cis_enabled;
static uint8_t le_audio_bis_enabled;
#endif

#if F_APP_LEA_DONGLE_BINDING
void le_audio_power_on_start_scan(void)
{
    uint8_t empty_id[3] = {0, 0, 0};

    APP_PRINT_TRACE1("le_audio_power_on_start_scan: saved_id %b",
                     TRACE_BINARY(3, app_cfg_nv.saved_id));

    if (memcmp(app_cfg_nv.saved_id, empty_id, 3))
    {
        /* Power on start to scan when dongle had bud id in flash */
        app_le_audio_start_auto_pair();
    }
}
#endif

int le_audio_cis_enter(void)
{
#if DONGLE_LE_AUDIO
    le_audio_cis_enabled = true;
    APP_PRINT_INFO0("le_audio_cis_enter");

    app_le_audio_get_stream_state();
    extern void teams_call_control_load_telephony(void);
    teams_call_control_load_telephony();
#endif
    return 0;
}

int le_audio_cis_exit(void)
{
#if DONGLE_LE_AUDIO
    le_audio_cis_enabled = false;
    APP_PRINT_INFO0("le_audio_cis_exit");
    app_le_audio_enable_unicast_audio(false);
#endif
    return 0;
}

int le_audio_bis_enter(void)
{
#if DONGLE_LE_AUDIO
    le_audio_bis_enabled = true;
    APP_PRINT_INFO1("le_audio_bis_enter auto_bis %x",
                    app_cfg_const.dongle_auto_broadcast_support);

    if (app_cfg_const.dongle_auto_broadcast_support)
    {
        le_audio_mmi_bis(MMI_BT_BIS_START, NULL);
    }
#endif
    return 0;
}

int le_audio_bis_exit(void)
{
#if DONGLE_LE_AUDIO
    le_audio_bis_enabled = false;
    APP_PRINT_INFO0("le_audio_bis_exit");
    bs_release_broadcast();
    app_led_set_mode(APP_LED_MODE_BST_IDLE);
#endif
    return 0;
}

bool key_mmi_trigger_scan = false;

int le_audio_mmi_cis(uint8_t mmi_action, void *params)
{
#if DONGLE_LE_AUDIO
    if (!le_audio_cis_enabled)
    {
        return -1;
    }

    switch (mmi_action)
    {
    case MMI_BT_CIS_SCAN:
        {
#if F_APP_LEA_DONGLE_BINDING
            adapter_set_scan_by_mmi(true);
#endif

            if (app_le_audio_scan_is_on())
            {
                key_mmi_trigger_scan = true;

                stop_le_audio_scan();
            }
            else
            {
                app_le_audio_start_auto_pair();
            }
        }
        break;

    case MMI_BT_CIS_RECONNECT:
        {
#if F_APP_LEA_DONGLE_BINDING
            if (adapter_get_scan_by_mmi())
            {
                /* short press cancel pairing */
                adapter_set_scan_by_mmi(false);

                app_led_set_mode(APP_LED_MODE_UNICAST_IDLE);
            }
#else
            if (app_le_audio_is_in_idle())
            {
                app_le_audio_enable_unicast_audio(true);
            }
#endif
        }
        break;
    case MMI_BT_CIS_DISCONNECT:
        {
#if F_APP_LEA_DONGLE_BINDING
            if (adapter_get_scan_by_mmi())
            {
                /* short press cancel pairing */
                adapter_set_scan_by_mmi(false);

                app_led_set_mode(APP_LED_MODE_UNICAST_IDLE);
            }
#endif
            if (!app_le_audio_is_in_idle())
            {
                app_le_audio_enable_unicast_audio(false);
            }
        }
        break;
    default:
        break;
    }
#endif
    return 0;

}

#if DONGLE_LE_AUDIO
uint8_t le_audio_cis_get_status(void)
{
    return app_le_audio_is_in_idle() ? 0 : 1;
}

uint8_t le_audio_bis_get_status(void)
{
    return bs_is_in_idle() ? 0 : 1;
}
#endif

int le_audio_mmi_bis(uint8_t mmi_action, void *params)
{
#if DONGLE_LE_AUDIO
    if (!le_audio_bis_enabled)
    {
        return -1;
    }

    switch (mmi_action)
    {
    case MMI_BT_BIS_START:
        {
            if (bs_is_in_idle())
            {
                uint8_t code[16] = { 0 };

                memset(code, 0, sizeof(code));
                if (app_cfg_const.dongle_broadcast_code_support)
                {
                    memcpy(code, app_cfg_const.dongle_broadcast_code, 16);
                }
                APP_PRINT_INFO1("broadcast_code %b", TRACE_BINARY(16, code));
                bs_start_broadcast(code);
                app_led_set_mode(APP_LED_MODE_BROADCASTING);
            }
        }
        break;

    case MMI_BT_BIS_STOP:
        {
            if (!bs_is_in_idle())
            {
                bs_release_broadcast();
                app_led_set_mode(APP_LED_MODE_BST_IDLE);
            }
        }
        break;

    default:
        break;
    }
#endif
    return 0;
}



int le_audio_init(void *evt_queue, void *msg_queue)
{
    return 0;
}
