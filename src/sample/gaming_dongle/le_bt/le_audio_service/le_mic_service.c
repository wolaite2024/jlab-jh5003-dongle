#include "trace.h"
#include "le_mic_service.h"
#include <string.h>
#include "app_usb_layer.h"
#include "cap.h"
#include "ual_dev_mgr.h"
#include "dev_mgr.h"
#include "teams_call_control.h"


uint16_t le_mic_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_MICS_CLIENT_DIS_DONE:
        {
            T_MICS_CLIENT_DIS_DONE *p_dis_done = (T_MICS_CLIENT_DIS_DONE *)buf;
            T_LE_AUDIO *p_audio_link = ble_audio_find_by_conn_handle(p_dis_done->conn_handle);
            if (p_audio_link)
            {
                if (p_dis_done->is_found)
                {
                    p_audio_link->remote_serv_sup |= LE_AUDIO_MICS_CLIENT_FLAG;
                    p_audio_link->mic_mute =  p_dis_done->mic_mute;
                    T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(p_dis_done->conn_handle);
                    if (p_dev_rec)
                    {
                        APP_PRINT_INFO2("le_mic_handle_msg disc %b mic_mute %x",
                                        TRACE_BDADDR(p_dev_rec->bd_addr), p_dis_done->mic_mute);
                        le_mic_set_remote_mute(p_dev_rec->pseudo_addr, le_tbs_get_mute_bit());
#if 0
                        mic_mute_state_report_callback(p_dev_rec->pseudo_addr, p_audio_link->mic_mute);
#endif
                    }

                }
            }
        }
        break;

    case LE_AUDIO_MSG_MICS_CLIENT_NOTIFY:
        {
            T_MICS_CLIENT_NOTIFY *p_notify_data = (T_MICS_CLIENT_NOTIFY *)buf;
            T_LE_AUDIO *p_audio_link = ble_audio_find_by_conn_handle(p_notify_data->conn_handle);
            if (p_audio_link)
            {
                p_audio_link->mic_mute = p_notify_data->mic_mute;
                T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(p_notify_data->conn_handle);
                if (p_dev_rec)
                {
                    APP_PRINT_INFO2("le_mic_handle_msg disc %b mic_mute %x",
                                    TRACE_BDADDR(p_dev_rec->bd_addr), p_notify_data->mic_mute);
                    mic_mute_state_report_callback(p_dev_rec->pseudo_addr, p_audio_link->mic_mute);
                }
            }
        }
        break;

    default:
        cb_result = BLE_AUDIO_CB_RESULT_REJECT;
        break;
    }
    return cb_result;
}

bool le_mic_set_remote_mute(uint8_t *addr, uint8_t mute)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(addr);
    T_BLE_AUDIO_GROUP_HANDLE group_handle;

    if (p_dev_rec)
    {
        group_handle = set_coordinator_find_by_addr(p_dev_rec->pseudo_addr,
                                                    p_dev_rec->bd_type,
                                                    GATT_UUID_CAS, NULL);
        if (group_handle)
        {
            if (cap_mics_change_mute(group_handle, (T_MICS_MUTE)mute) == LE_AUDIO_CAUSE_SUCCESS)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        return mics_write_mute_value(p_dev_rec->le_conn_handle, (T_MICS_MUTE)mute);
    }
    return false;
}
