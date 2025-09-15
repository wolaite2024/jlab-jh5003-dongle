
#include <string.h>
#include "trace.h"
#include "aics_def.h"
#include "aics_mgr.h"
#include "gap_conn_le.h"
#include "app_cfg.h"
#include "app_lea_aics.h"
#include "app_lea_ccp.h"
#include "app_lea_profile.h"
#include "app_lea_unicast_audio.h"
#include "app_main.h"

#if F_APP_AICS_SUPPORT
const char input_des_mics[] = "AICS for MICS dec";

static uint16_t app_lea_aics_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    bool handle = true;

    switch (msg)
    {
    case LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO:
        {
            T_SERVER_ATTR_CCCD_INFO *p_cccd = (T_SERVER_ATTR_CCCD_INFO *)buf;

            if (p_cccd->char_uuid >= AICS_UUID_CHAR_INPUT_STATE &&
                p_cccd->char_uuid <= AICS_UUID_CHAR_INPUT_DES)
            {
                uint8_t conn_id;
                T_APP_LE_LINK *p_link;

                le_get_conn_id_by_handle(p_cccd->conn_handle, &conn_id);
                p_link = app_link_find_le_link_by_conn_id(conn_id);
                if (p_link)
                {
                    p_link->lea_device |= APP_LEA_AICS_CCCD_ENABLED;
                }
            }

            APP_PRINT_TRACE5("app_lea_aics_ble_audio_cback: conn_handle 0x%02X, service_id %d, char_uuid 0x%02X, ccc_bits 0x%02X, param %d",
                             p_cccd->conn_handle,
                             p_cccd->service_id,
                             p_cccd->char_uuid,
                             p_cccd->ccc_bits,
                             p_cccd->param);
        }
        break;

    case LE_AUDIO_MSG_AICS_CP_IND:
        {
            T_AICS_CP_IND *p_cp_op = (T_AICS_CP_IND *)buf;

            APP_PRINT_INFO4("app_lea_aics_ble_audio_cback: LE_AUDIO_MSG_AICS_CP_IND, conn_handle 0x%x, srv_instance_id %d, cp_op %d, gain_setting %d",
                            p_cp_op->conn_handle,
                            p_cp_op->srv_instance_id,
                            p_cp_op->cp_op,
                            p_cp_op->gain_setting);

            T_APP_LE_LINK *p_link = NULL;

            p_link = app_link_find_le_link_by_conn_handle(p_cp_op->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            switch (p_cp_op->cp_op)
            {
            case AICS_CP_SET_GAIN_SETTING:
                {
                    if (app_cfg_nv.lea_mic_gain_mode == AICS_GAIN_MODE_MANUAL)
                    {
                        app_cfg_nv.lea_mic_gain_vol = p_cp_op->gain_setting;
                        app_lea_uca_set_mic_gain(p_link);
                    }
                }
                break;

            case AICS_CP_UNMUTE:
                {
                    if ((p_link->lea_device & APP_LEA_MICS_CCCD_ENABLED) == 0)
                    {
                        app_lea_uca_set_mic_mute(p_link, MMI_DEV_MIC_UNMUTE, true, NULL);
                    }
                }
                break;

            case AICS_CP_MUTE:
                {
                    if ((p_link->lea_device & APP_LEA_MICS_CCCD_ENABLED) == 0)
                    {
                        app_lea_uca_set_mic_mute(p_link, MMI_DEV_MIC_MUTE, true, NULL);
                    }
                }
                break;

            case AICS_CP_SET_MANUAL_GAIN_MODE:
                {
                    app_cfg_nv.lea_mic_gain_mode = AICS_GAIN_MODE_MANUAL;
                }
                break;

            case AICS_CP_SET_AUTOMATIC_GAIN_MODE:
                {
                    app_cfg_nv.lea_mic_gain_mode = AICS_GAIN_MODE_AUTOMATIC;
                }
                break;

            default:
                break;
            }

            APP_PRINT_INFO2("app_lea_aics_ble_audio_cback: gain_mode 0x%x, gain_setting %d",
                            app_cfg_nv.lea_mic_gain_mode, app_cfg_nv.lea_mic_gain_vol);
        }
        break;

    case LE_AUDIO_MSG_AICS_WRITE_INPUT_DES_IND:
        {
            T_AICS_WRITE_INPUT_DES_IND *p_write_ind = (T_AICS_WRITE_INPUT_DES_IND *)buf;

            APP_PRINT_INFO4("app_lea_acc_vc_mic_handle_msg: LE_AUDIO_MSG_AICS_WRITE_INPUT_DES_IND, conn_handle 0x%x, srv_instance_id %d, input des[%d] %b",
                            p_write_ind->conn_handle,
                            p_write_ind->srv_instance_id,
                            p_write_ind->input_des.input_des_len,
                            TRACE_BINARY(p_write_ind->input_des.input_des_len, p_write_ind->input_des.p_input_des));
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_lea_aics_ble_audio_cback: msg %x", msg);
    }

    return cb_result;
}

void app_lea_aics_init(void)
{
    T_AICS_INPUT_STATE input_state;
    T_AICS_GAIN_SETTING_PROP gain_prop;
    uint8_t input_type;
    uint8_t input_status;

    input_type = AUDIO_INPUT_MICROPHONE;
    input_status = AICS_INPUT_STATUS_INACTIVE;
    input_state.change_counter = app_cfg_nv.lea_mic_change_cnt;
    input_state.gain_mode = app_cfg_nv.lea_mic_gain_mode;
    input_state.gain_setting = app_cfg_nv.lea_mic_gain_vol;
    input_state.mute = AICS_NOT_MUTED;
    gain_prop.gain_setting_max = 127;
    gain_prop.gain_setting_min = -128;
    gain_prop.gain_setting_units = 16;

    aics_set_param(AICS_MICS_SRV_ID, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                   (uint8_t *)&input_state, true);
    aics_set_param(AICS_MICS_SRV_ID, AICS_PARAM_GAIN_SETTING_PROP,
                   sizeof(T_AICS_GAIN_SETTING_PROP), (uint8_t *)&gain_prop, false);
    aics_set_param(AICS_MICS_SRV_ID, AICS_PARAM_INPUT_TYPE, sizeof(uint8_t), &input_type, false);
    aics_set_param(AICS_MICS_SRV_ID, AICS_PARAM_INPUT_STATUS, sizeof(uint8_t), &input_status, false);
    aics_set_param(AICS_MICS_SRV_ID, AICS_PARAM_INPUT_DES, strlen(input_des_mics),
                   (uint8_t *)input_des_mics, false);

    ble_audio_cback_register(app_lea_aics_ble_audio_cback);
}
#endif
