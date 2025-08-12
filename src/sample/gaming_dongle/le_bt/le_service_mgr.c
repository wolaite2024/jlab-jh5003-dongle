#include <string.h>
#include "trace.h"
#include "le_audio_service.h"
#include "le_service_mgr.h"
#include "gatt_builtin_services.h"
#include "ble_audio.h"
#include "ual_api_types.h"
#include "bt_gatt_svc.h"
#include "dev_mgr.h"

#define BLE_DEVICE_NAME     "BLE_DONGLE_PRI"
#define BLE_APPEARANCE      GAP_GATT_APPEARANCE_GENERIC_MEDIA_PLAYER

static T_APP_RESULT le_profiles_reg_callback(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    if (service_id == SERVICE_PROFILE_GENERAL_ID)
    {
        T_SERVER_APP_CB_DATA *p_param = (T_SERVER_APP_CB_DATA *)p_data;
        switch (p_param->eventId)
        {
        case PROFILE_EVT_SRV_REG_COMPLETE:// srv register result event.
            APP_PRINT_INFO1("PROFILE_EVT_SRV_REG_COMPLETE: result %d",
                            p_param->event_data.service_reg_result);
            break;

        case PROFILE_EVT_SEND_DATA_COMPLETE:
            APP_PRINT_INFO5("PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d",
                            p_param->event_data.send_data_result.conn_id,
                            p_param->event_data.send_data_result.cause,
                            p_param->event_data.send_data_result.service_id,
                            p_param->event_data.send_data_result.attrib_idx,
                            p_param->event_data.send_data_result.credits);
            if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
            {
                APP_PRINT_INFO0("PROFILE_EVT_SEND_DATA_COMPLETE success");
            }
            else
            {
                APP_PRINT_ERROR0("PROFILE_EVT_SEND_DATA_COMPLETE failed");
            }

            {
                uint16_t conn_handle = le_get_conn_handle(p_param->event_data.send_data_result.conn_id);
                if (!gatt_svc_handle_profile_data_cmpl(conn_handle, L2C_FIXED_CID_ATT,
                                                       p_param->event_data.send_data_result.service_id,
                                                       p_param->event_data.send_data_result.attrib_idx,
                                                       p_param->event_data.send_data_result.credits,
                                                       p_param->event_data.send_data_result.cause))
                {
                    APP_PRINT_ERROR0("gatt_svc_handle_profile_data_cmpl failed");
                }
            }
            break;

        default:
            break;
        }
    }

    return app_result;
}

void le_service_gap_params_reset(void)
{
    uint16_t appearance = BLE_APPEARANCE;
#if  (LE_AUDIO_CONNECT_FASTER == 1)
    //set ci as 7.5 to speed up the connection
    uint16_t conn_interval_min = BLE_CONN_FAST_CI_DEF;
    uint16_t conn_interval_max = BLE_CONN_FAST_CI_DEF;
#else
    uint16_t conn_interval_min = BLE_CONN_NORMAL_CI_DEF;
    uint16_t conn_interval_max = BLE_CONN_NORMAL_CI_DEF;
#endif
    uint16_t slave_latency = BLE_CONN_SLAVE_LATENCY_DEF;
    uint16_t supervision_timeout = BLE_CONN_TIMEOUT_DEF;

    /* Set device name and device appearance */
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, strlen(BLE_DEVICE_NAME), BLE_DEVICE_NAME);
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(uint16_t), &appearance);
    gaps_set_peripheral_preferred_conn_param(conn_interval_min, conn_interval_max,
                                             slave_latency, supervision_timeout);
}

uint8_t le_service_get_comm_adv_data(uint8_t *p_data, uint8_t buff_len)
{
    uint8_t idx = 0;
    if (buff_len > 2)
    {
        p_data[idx] = 0x02;
        idx++;
        p_data[idx] = GAP_ADTYPE_FLAGS;
        idx++;
        p_data[idx] = GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
        idx++;
    }

    if (buff_len > 6)
    {
        p_data[idx] = 0x03;
        idx++;
        p_data[idx] = GAP_ADTYPE_APPEARANCE;
        idx++;
        p_data[idx] = BLE_APPEARANCE & 0x00FF;
        idx++;
        p_data[idx] = (BLE_APPEARANCE >> 8) & 0x00FF;
        idx++;
    }

    if (buff_len > 9)
    {
        uint8_t length = (buff_len - 9) > strlen(BLE_DEVICE_NAME) ? strlen(BLE_DEVICE_NAME) :
                         (buff_len - 9);
        p_data[idx] = length + 1;
        idx++;
        p_data[idx] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
        idx++;
        memcpy(p_data + idx, BLE_DEVICE_NAME, length);
        idx += length;
    }
    return idx;
}

void ble_service_mgr_init(void *evt_queue, void *io_queue)
{
    ble_audio_mgr_init(evt_queue, io_queue);
    server_register_app_cb(le_profiles_reg_callback);
}

