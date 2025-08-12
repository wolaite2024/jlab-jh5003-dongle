#include "trace.h"
#include "ble_audio.h"
#include "mics_mgr.h"
#include "app_cfg.h"
#include "app_lea_ccp.h"
#include "app_lea_mics.h"
#include "app_lea_unicast_audio.h"
#include "app_link_util.h"
#include "app_mmi.h"

#if F_APP_MICS_SUPPORT
static uint16_t app_lea_mics_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    bool handle = true;

    switch (msg)
    {
    case LE_AUDIO_MSG_MICS_WRITE_MUTE_IND:
        {
            uint8_t action = MMI_NULL;
            uint8_t mute_state = *(uint8_t *)buf;
            T_APP_LE_LINK *p_link = NULL;

            p_link = app_link_find_le_link_by_conn_handle(app_lea_ccp_get_active_conn_handle());
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (mute_state == MICS_NOT_MUTE)
            {
                action = MMI_DEV_MIC_UNMUTE;
            }
            else if (mute_state == MICS_MUTED)
            {
                action = MMI_DEV_MIC_MUTE;
            }
            app_lea_uca_set_mic_mute(p_link, action, true);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_lea_mics_ble_audio_cback: msg %x", msg);
    }

    return cb_result;
}

void app_lea_mics_init(void)
{
    T_MICS_PARAM mics_param;

    mics_param.mic_mute = MICS_NOT_MUTE;
    mics_set_param(&mics_param);
    ble_audio_cback_register(app_lea_mics_ble_audio_cback);
}
#endif
