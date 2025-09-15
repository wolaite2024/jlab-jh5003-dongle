#include "trace.h"
#include "ble_audio.h"
#include "gap_conn_le.h"
#include "mics_mgr.h"
#include "app_cfg.h"
#include "app_lea_ccp.h"
#include "app_lea_mics.h"
#include "app_lea_profile.h"
#include "app_lea_unicast_audio.h"
#include "app_link_util.h"
#include "app_main.h"
#include "app_mmi.h"

#if F_APP_MICS_SUPPORT
T_APP_LE_LINK *app_lea_mics_find_mc_link(void)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if ((app_db.le_link[i].used == true) && (app_db.le_link[i].lea_device & APP_LEA_MICS_CCCD_ENABLED))
        {
            p_link = &app_db.le_link[i];
        }
    }

    return p_link;
}

static uint16_t app_lea_mics_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    bool handle = true;

    switch (msg)
    {
    case LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO:
        {
            T_SERVER_ATTR_CCCD_INFO *p_cccd = (T_SERVER_ATTR_CCCD_INFO *)buf;

            if (p_cccd->char_uuid == MICS_UUID_CHAR_MUTE)
            {
                uint8_t conn_id;
                T_APP_LE_LINK *p_link;

                le_get_conn_id_by_handle(p_cccd->conn_handle, &conn_id);
                p_link = app_link_find_le_link_by_conn_id(conn_id);
                if (p_link)
                {
                    p_link->lea_device |= APP_LEA_MICS_CCCD_ENABLED;
                }
            }

            APP_PRINT_TRACE5("app_lea_mics_ble_audio_cback: conn_handle 0x%02X, service_id %d, char_uuid 0x%02X, ccc_bits 0x%02X, param %d",
                             p_cccd->conn_handle,
                             p_cccd->service_id,
                             p_cccd->char_uuid,
                             p_cccd->ccc_bits,
                             p_cccd->param);
        }
        break;

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
            app_lea_uca_set_mic_mute(p_link, action, true, NULL);
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
