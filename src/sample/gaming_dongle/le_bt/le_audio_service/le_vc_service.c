//#include <string.h>
#include <stdio.h>
#include "trace.h"
#include "le_vc_service.h"
#include "vocs_mgr.h"
#include "codec_def.h"
#include "vcs_def.h"
#include "app_usb_layer.h"

#include "cap.h"

#include "ual_dev_mgr.h"
#include "dev_mgr.h"

bool le_audio_handle_remote_volume_change(uint16_t conn_handle, uint8_t volume_setting,
                                          uint8_t mute)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(conn_handle);
    APP_PRINT_INFO2("le_audio_handle_remote_volume_change vol %x mute %x", volume_setting, mute);
    if (p_dev_rec)
    {
        volume_setting_report_callback(p_dev_rec->pseudo_addr, volume_setting, mute);
    }
    return true;
}

bool le_audio_handle_remote_volume_flag_change(uint16_t conn_handle, uint8_t volume_flag)
{
    extern uint8_t get_dongle_host_vol(void);
    uint8_t host_vol = get_dongle_host_vol() * 2;
    T_VCS_VOLUME_CP_PARAM param;

    /* vcs volume min 0 max 255, pc vol min 0 max 127*/
    param.volume_setting = host_vol;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(conn_handle);
    if (!p_dev_rec)
    {
        return false;
    }
    APP_PRINT_INFO2("volume_flag_change flag %d, host vol %d", volume_flag, host_vol);
    /*Reset Volume Setting, should set absolue volume to Server*/
    if (!volume_flag)
    {
        T_BLE_AUDIO_GROUP_HANDLE group_handle;

        group_handle = set_coordinator_find_by_addr(p_dev_rec->pseudo_addr,
                                                    p_dev_rec->bd_type,
                                                    GATT_UUID_CAS, NULL);
        if (group_handle)
        {
            if (cap_vcs_change_volume(group_handle, host_vol) == LE_AUDIO_CAUSE_SUCCESS)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        return vcs_write_cp(p_dev_rec->le_conn_handle,
                            VCS_CP_SET_ABSOLUTE_VOLUME, &param);
    }
    else
    {

    }
    return true;
}

bool le_audio_handle_remote_volume_offset_change(uint16_t conn_handle, T_VOCS_SRV_DATA *p_vocs_data)
{

    T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(conn_handle);
    if (p_dev_rec)
    {
        volume_offset_val_report_callback(p_dev_rec->pseudo_addr, p_vocs_data->srv_instance_id,
                                          p_vocs_data->volume_offset.volume_offset, p_vocs_data->audio_location);
    }

    return true;
}

bool le_audio_set_remote_volume(uint8_t *bd_addr, uint8_t volume_setting)
{
    T_VCS_VOLUME_CP_PARAM param;
    param.volume_setting = volume_setting;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    T_BLE_AUDIO_GROUP_HANDLE group_handle;

    APP_PRINT_INFO2("le_audio_set_remote_volume bd %b vol %x", TRACE_BDADDR(bd_addr), volume_setting);
    if (p_dev_rec)
    {
        group_handle = set_coordinator_find_by_addr(p_dev_rec->pseudo_addr,
                                                    p_dev_rec->bd_type,
                                                    GATT_UUID_CAS, NULL);
        if (group_handle)
        {
            if (cap_vcs_change_volume(group_handle, volume_setting) == LE_AUDIO_CAUSE_SUCCESS)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        return vcs_write_cp(p_dev_rec->le_conn_handle,
                            VCS_CP_SET_ABSOLUTE_VOLUME, &param);
    }
    return false;
}

bool le_audio_set_remote_mute(uint8_t *bd_addr, uint8_t mute)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    T_BLE_AUDIO_GROUP_HANDLE group_handle;

    APP_PRINT_INFO2("le_audio_set_remote_mute bd %b mute %x", TRACE_BDADDR(bd_addr), mute);
    if (p_dev_rec)
    {
        group_handle = set_coordinator_find_by_addr(p_dev_rec->pseudo_addr,
                                                    p_dev_rec->bd_type,
                                                    GATT_UUID_CAS, NULL);
        if (group_handle)
        {
            if (cap_vcs_change_mute(group_handle, mute) == LE_AUDIO_CAUSE_SUCCESS)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        return vcs_write_cp(p_dev_rec->le_conn_handle,
                            mute == VCS_MUTED ? VCS_CP_MUTE : VCS_CP_UNMUTE,
                            NULL);
    }
    return false;
}



bool le_audio_set_device_volume_offset(uint8_t *bd_addr, uint8_t idx, int16_t offset)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (p_dev_rec)
    {
        T_VOCS_CP_PARAM param = {offset};
        return vocs_write_cp(p_dev_rec->le_conn_handle, idx, VOCS_CP_SET_VOLUME_OFFSET, &param);
    }
    return false;
}


uint16_t le_audio_aics_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_AICS_CLIENT_DIS_DONE:
        {
            T_AICS_CLIENT_DIS_DONE *p_dis_done = (T_AICS_CLIENT_DIS_DONE *)buf;
            if (p_dis_done->is_found)
            {
                T_LE_AUDIO *p_audio_link = ble_audio_find_by_conn_handle(p_dis_done->conn_handle);
                if (p_audio_link)
                {
                    p_audio_link->remote_serv_sup |= LE_AUDIO_AICS_CLIENT_FLAG;
                    p_audio_link->aics_num = p_dis_done->srv_num;
                }
            }
        }
        break;
    case LE_AUDIO_MSG_AICS_CLIENT_READ_RESULT:
        {
            T_AICS_CLIENT_READ_RESULT *read_result = (T_AICS_CLIENT_READ_RESULT *)buf;
            T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(read_result->conn_handle);
            if (read_result->cause != GAP_SUCCESS)
            {
                APP_PRINT_ERROR1("LE_AUDIO_MSG_AICS_CLIENT_READ_RESULT: read fail error: 0x%x",
                                 read_result->cause);
                break;
            }
            if (p_dev_rec != NULL)
            {
                if (read_result->type == AICS_CHAR_INPUT_STATUS)
                {
                    T_AICS_SRV_DATA aics_data;
                    if (aics_get_srv_data(read_result->conn_handle, read_result->srv_instance_id, &aics_data))
                    {
                        audio_input_info_report_callback(p_dev_rec->pseudo_addr, read_result->srv_instance_id, &aics_data);
                    }
                }
            }
        }
        break;
    case LE_AUDIO_MSG_AICS_CLIENT_NOTIFY:
        {
            T_AICS_CLIENT_NOTIFY *notify_data = (T_AICS_CLIENT_NOTIFY *)buf;
            T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(notify_data->conn_handle);
            if (notify_data->type == AICS_CHAR_INPUT_STATE ||
                notify_data->type == AICS_CHAR_INPUT_STATUS)
            {
                T_AICS_SRV_DATA aics_data;
                if (aics_get_srv_data(notify_data->conn_handle, notify_data->srv_instance_id, &aics_data))
                {
                    audio_input_info_report_callback(p_dev_rec->pseudo_addr, notify_data->srv_instance_id, &aics_data);
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


bool le_audio_set_device_input_gain(uint8_t *bd_addr, uint8_t idx, int8_t gai_setting)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (p_dev_rec)
    {
        T_AICS_CP_PARAM param = {gai_setting};
        return aics_write_cp(p_dev_rec->le_conn_handle, idx, AICS_CP_SET_GAIN_SETTING, &param);
    }
    return false;
}

bool le_audio_set_device_input_mute(uint8_t *bd_addr, uint8_t idx, uint8_t mute)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (p_dev_rec)
    {
        T_AICS_CP_PARAM param = {0};
        return aics_write_cp(p_dev_rec->le_conn_handle, idx,
                             mute == AICS_NOT_MUTED ? AICS_CP_UNMUTE : AICS_CP_MUTE, &param);
    }
    return false;
}

bool le_audio_set_device_input_gain_mode(uint8_t *bd_addr, uint8_t idx, uint8_t gain_mode)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (p_dev_rec)
    {
        T_AICS_CP_PARAM param = {0};
        return aics_write_cp(p_dev_rec->le_conn_handle, idx,
                             gain_mode == AICS_GAIN_MODE_MANUAL ? AICS_CP_SET_MANUAL_GAIN_MODE : AICS_CP_SET_AUTOMATIC_GAIN_MODE,
                             &param);
    }
    return false;
}


uint16_t le_audio_vocs_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_LE_AUDIO *p_link = NULL;
    switch (msg)
    {
    case LE_AUDIO_MSG_VOCS_CLIENT_DIS_DONE:
        {
            T_VOCS_CLIENT_DIS_DONE *dis_done = (T_VOCS_CLIENT_DIS_DONE *)buf;
            int i;
            if (dis_done && dis_done->is_found)
            {
                p_link =  ble_audio_find_by_conn_handle(dis_done->conn_handle);
                if (p_link != NULL)
                {
                    p_link->remote_serv_sup |= LE_AUDIO_VOCS_CLIENT_FLAG;
                    p_link->srv_total_num = dis_done->srv_num;
                    for (i = 0; i < dis_done->srv_num; i++)
                    {
                        vocs_read_char_value(dis_done->conn_handle, i, VOCS_CHAR_AUDIO_LOCATION);
                        vocs_cfg_cccd(p_link->conn_handle, i,
                                      VOCS_VOLUME_OFFSET_STATE_FLAG | VOCS_AUDIO_LOCATION_FLAG | VOCS_AUDIO_OUTPUT_DES_FLAG,
                                      true);
                    }
                }
            }
        }
        break;
    case LE_AUDIO_MSG_VOCS_CLIENT_READ_RESULT:
        {
            T_VOCS_CLIENT_READ_RESULT *read_result = (T_VOCS_CLIENT_READ_RESULT *)buf;
            p_link =  ble_audio_find_by_conn_handle(read_result->conn_handle);
            if (read_result->cause != GAP_SUCCESS)
            {
                APP_PRINT_ERROR1("LE_AUDIO_MSG_VOCS_CLIENT_READ_RESULT: read fail error: 0x%x",
                                 read_result->cause);
                break;
            }
            if (p_link != NULL)
            {
                if (read_result->type == VOCS_CHAR_AUDIO_LOCATION)
                {
                    vocs_read_char_value(read_result->conn_handle, read_result->srv_instance_id,
                                         VOCS_CHAR_OFFSET_STATE);
                }
                else if (read_result->type == VOCS_CHAR_OFFSET_STATE)
                {
                    T_VOCS_SRV_DATA srv_data;
                    if (vocs_get_srv_data(read_result->conn_handle, read_result->srv_instance_id, &srv_data))
                    {
                        le_audio_handle_remote_volume_offset_change(read_result->conn_handle, &srv_data);
                    }
                }
            }
        }
        break;
    case LE_AUDIO_MSG_VOCS_CLIENT_NOTIFY:
        {
            T_VOCS_CLIENT_NOTIFY *notify_data = (T_VOCS_CLIENT_NOTIFY *)buf;
            if (notify_data->type == VOCS_CHAR_OFFSET_STATE)
            {
                T_VOCS_SRV_DATA srv_data;
                vocs_get_srv_data(notify_data->conn_handle, notify_data->srv_instance_id, &srv_data);
                le_audio_handle_remote_volume_offset_change(notify_data->conn_handle, &srv_data);
            }
        }

        break;
    case LE_AUDIO_MSG_VOCS_CLIENT_CP_RESULT:
        {
            T_VOCS_CLIENT_CP_RESULT *p_cp_result = (T_VOCS_CLIENT_CP_RESULT *)buf;
            if (p_cp_result->cause != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR3("LE_AUDIO_MSG_VOCS_CLIENT_CP_RESULT: conn_handle 0x%x, cause 0x%x, idx %d",
                                 p_cp_result->conn_handle, p_cp_result->cause, p_cp_result->srv_instance_id);
                vocs_read_char_value(p_cp_result->conn_handle, p_cp_result->cause, VOCS_CHAR_OFFSET_STATE);
            }
        }
        break;
    }

    return cb_result;
}


uint16_t le_audio_vcs_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_LE_AUDIO *p_link = NULL;
    //uint8_t ret;
    switch (msg)
    {
    case LE_AUDIO_MSG_VCS_CLIENT_DIS_DONE:
        {
            T_VCS_CLIENT_DIS_DONE *dis_done = (T_VCS_CLIENT_DIS_DONE *)buf;
            if (dis_done && dis_done->is_found)
            {
                p_link = ble_audio_find_by_conn_handle(dis_done->conn_handle);
                if (p_link != NULL)
                {
                    p_link->remote_serv_sup |= LE_AUDIO_VCS_CLIENT_FLAG;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_VCS_CLIENT_VOLUME_STATE_DATA:
        {
            T_VCS_CLIENT_VOLUME_STATE_DATA *notify_data = (T_VCS_CLIENT_VOLUME_STATE_DATA *)buf;
            p_link =  ble_audio_find_by_conn_handle(notify_data->conn_handle);
            if (p_link == NULL)
            {
                break;
            }
            if (notify_data->type == VCS_CHAR_VOLUME_STATE)
            {
                APP_PRINT_INFO3("LE_AUDIO_MSG_VCS_CLIENT_NOTIFY vol %x mute %x counter %x",
                                notify_data->data.volume_state.volume_setting,
                                notify_data->data.volume_state.mute,
                                notify_data->data.volume_state.change_counter);
                le_audio_handle_remote_volume_change(notify_data->conn_handle,
                                                     notify_data->data.volume_state.volume_setting,
                                                     notify_data->data.volume_state.mute);
            }
            if (notify_data->type == VCS_CHAR_VOLUME_FLAGS)
            {
                APP_PRINT_INFO1("LE_AUDIO_MSG_VCS_CLIENT_NOTIFY vol_flags %x ", notify_data->data.volume_flags);
            }
        }
        break;
    case LE_AUDIO_MSG_VCS_CLIENT_CP_RESULT:
        {
            T_VCS_CLIENT_CP_RESULT *p_cp_result = (T_VCS_CLIENT_CP_RESULT *)buf;
            p_link =  ble_audio_find_by_conn_handle(p_cp_result->conn_handle);
            if (p_link == NULL)
            {
                break;
            }
            if (p_cp_result->cause != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR3("LE_AUDIO_MSG_VCS_CLIENT_CP_RESULT: conn_handle 0x%x, cause 0x%x, op 0x%x",
                                 p_cp_result->conn_handle, p_cp_result->cause, p_cp_result->cp_op);
            }
        }
        break;
    }

    return cb_result;
}

