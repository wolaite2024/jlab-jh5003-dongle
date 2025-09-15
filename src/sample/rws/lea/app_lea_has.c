#include "trace.h"
#include "has_mgr.h"
#include "ias_gatt_svc.h"
#include "profile_server_def.h"
#include "remote.h"
#include "app_audio_policy.h"
#include "app_cfg.h"
#include "app_hearable.h"
#include "app_lea_has.h"
#include "app_lea_has_preset_record.h"
#include "app_timer.h"

#if F_APP_HAS_SUPPORT
T_SERVER_ID ias_srv_id = 0xff;
uint8_t ias_alert_level = 0;
static uint8_t ias_timer_id = 0;
static uint8_t timer_idx_ias_alert = 0;

uint16_t app_lea_has_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    bool handle = true;
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_HAS_CP_IND:
        {
            T_HAS_CP_IND *p_write_req = (T_HAS_CP_IND *)buf;
            cb_result = app_lea_has_handle_write_cp_req(p_write_req);
        }
        break;

    case LE_AUDIO_MSG_HAS_PENDING_PRESET_CHANGE:
        {
            T_HAS_PENDING_PRESET_CHANGE *p_cccd_info = (T_HAS_PENDING_PRESET_CHANGE *)buf;

            if (!app_lea_has_handle_cccd_info(p_cccd_info))
            {
                cb_result = BLE_AUDIO_CB_RESULT_APP_ERR;
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_lea_has_ble_audio_cback: msg 0x%04X", msg);
    }

    return cb_result;
}

void app_lea_ias_alert(uint16_t conn_handle)
{
    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        if (ias_alert_level != 0)
        {
            app_audio_tone_type_play(TONE_ALARM, false, true);

            if (app_cfg_const.tone_alarm != 0)
            {
                uint32_t timeout_ms = app_cfg_const.tone_alarm * 1000;

                if (ias_alert_level == APP_IAS_MILD_ALERT)
                {
                    timeout_ms *= 2;
                }
                app_start_timer(&timer_idx_ias_alert, "ias_alert",
                                ias_timer_id, APP_IAS_TIMER_ALERT, conn_handle, true, timeout_ms);
            }
        }
        else
        {
            app_stop_timer(&timer_idx_ias_alert);
        }
    }
    else
    {
        app_stop_timer(&timer_idx_ias_alert);
    }
}

T_APP_RESULT app_lea_ias_server_cback(uint16_t conn_handle, uint16_t cid, uint8_t type,
                                      void *p_data)
{
    T_APP_LE_LINK *p_link;
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    if (p_link == NULL)
    {
        return APP_RESULT_APP_ERR;
    }

    if (type == GATT_MSG_IAS_SERVER_WRITE_ALERT_LEVEL)
    {
        T_IAS_SERVER_WRITE_ALERT_LEVEL *p_ias_data = (T_IAS_SERVER_WRITE_ALERT_LEVEL *)p_data;

        if (p_ias_data->service_id == ias_srv_id)
        {
            ias_alert_level = p_ias_data->alert_level;
            app_lea_ias_alert(conn_handle);
        }
    }

    return app_result;
}

static void app_lea_ias_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_lea_ias_timeout_cb: timer_evt %d, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_IAS_TIMER_ALERT:
        {
            T_APP_LE_LINK *p_link;

            p_link = app_link_find_le_link_by_conn_handle(param);
            if (p_link == NULL)
            {
                ias_alert_level = 0;
            }

            if (ias_alert_level != 0)
            {
                app_audio_tone_type_play(TONE_ALARM, false, true);
            }
            else
            {
                app_stop_timer(&timer_idx_ias_alert);
            }
        }
        break;

    default:
        break;
    }
}

void app_lea_has_handle_authen_evt(void)
{
    has_update_active_preset_idx(app_ha_hearing_get_active_prog_id() + 1);
}

void app_lea_has_init(bool is_ha_cp_notify)
{
    uint8_t idx = 0;

    T_HAS_ATTR_FEATURE attr_feature;
    attr_feature.is_ha_cp_exist = true;
    attr_feature.is_ha_features_notify_support = true;
    attr_feature.is_ha_cp_notify = is_ha_cp_notify;

    T_HAS_HA_FEATURES hearing_aid_features;
    memset(&hearing_aid_features, 0, 1);
    hearing_aid_features.hearing_aid_type = HA_FEATURES_BINAURAL_HA_MASK;
    hearing_aid_features.preset_sync_support = 1;
    hearing_aid_features.independent_preset = 0;
    hearing_aid_features.dynamic_preset = 1;
    hearing_aid_features.writable_preset_support = 1;
    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        hearing_aid_features.hearing_aid_type = HA_FEATURES_MONAURAL_HA_MASK;
        hearing_aid_features.preset_sync_support = 0;
        hearing_aid_features.independent_preset = 0;
    }

    ias_srv_id = ias_reg_srv(app_lea_ias_server_cback);
    has_init(attr_feature, hearing_aid_features);
    ble_audio_cback_register(app_lea_has_ble_audio_cback);
    app_timer_reg_cb(app_lea_ias_timeout_cb, &ias_timer_id);
    app_lea_has_preset_record_init();

    while (idx < app_ha_hearing_get_prog_num())
    {
        T_HA_HAP_INFO has_infor = app_ha_hearing_get_prog_info(idx);
        app_lea_has_modify_preset_record_list(PRESET_CHANGE_ADD, has_infor.index + 1,
                                              HAS_PRESET_PROPERTIES_WRITABLE | HAS_PRESET_PROPERTIES_AVAILABLE,
                                              has_infor.name_len + 1,
                                              has_infor.p_name,
                                              true);
        idx++;
    }
    has_update_active_preset_idx(app_ha_hearing_get_active_prog_id() + 1);
}
#endif
