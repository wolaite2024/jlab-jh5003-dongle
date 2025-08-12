/*
 *  Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 */
#include "trace.h"
#include "general_audio_wrapper.h"
#include "general_bt.h"
#include "app_link_util.h"
#include "app_mmi.h"
#include "app_led_ctl.h"

#ifdef LEGACY_BT_GENERAL
static uint8_t general_enabled;

int general_audio_enter(void)
{
    general_enabled = 1;

    general_bt_enable();

    return 0;
}

void general_audio_exit(void)
{
    general_enabled = 0;

    general_bt_disable();
}

int general_audio_init(void *evt_queue, void *msg_queue)
{
    /* TODO avoid compiling warnings. */
    (void)evt_queue;
    (void)msg_queue;

    general_bt_init();

    return 0;
}

uint8_t general_get_status(uint8_t index)
{
    return general_bt_is_idle(index) ? 0 : 1;
}

int general_audio_mmi(uint8_t mmi_action, void *params)
{
    general_enabled = 1;
    if (!general_enabled)
    {
        return -1;
    }

    APP_PRINT_INFO1("general_audio_mmi: action %02x", mmi_action);

    switch (mmi_action)
    {
    case MMI_BT_LEGACY_LINK1_SCAN:
        {
            uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };
            general_bt_connect(0, ba_any);
        }
        break;

    case MMI_BT_LEGACY_LINK1_RECONNECT:
        {
            uint8_t bdaddr[6];
            if (general_bt_is_idle(0))
            {
                if (general_bt_get_bond_bdaddr(0, bdaddr) < 0)
                {
                    APP_PRINT_WARN0("general_audio_mmi: No bond for dev 0");
                    return -2;
                }
                general_bt_connect(0, bdaddr);
            }
        }
        break;

    case MMI_BT_LEGACY_LINK1_DISCONNECT:
        {
            uint8_t bdaddr[6];
            if (!general_bt_is_idle(0))
            {
                if (general_bt_get_bond_bdaddr(0, bdaddr) == 0)
                {
                    general_bt_disconnect_by_bdaddr(bdaddr);
                }
            }
        }
        break;
    default:
        APP_PRINT_ERROR1("general_audio_mmi: unknown action %u", mmi_action);
        return -2;
    }

    return 0;
}
#endif
