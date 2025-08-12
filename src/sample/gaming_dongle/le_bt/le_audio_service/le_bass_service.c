#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "le_bass_service.h"
#include "le_audio_data.h"
#include "ble_audio.h"
#include "metadata_def.h"
#include "codec_def.h"
#include "ble_isoch_def.h"
#include "codec_qos.h"
#include "app_usb_layer.h"
#include "bap.h"
#include "ual_adapter.h"
#include "os_queue.h"
#include "bass_mgr.h"
#include "ual_adv.h"
#include "le_csis_srv_service.h"
#include "le_service_mgr.h"
#include "le_broadcast_mgr.h"
#include "app_audio_path.h"

#if UAL_CONSOLE_PRINT
#include "console.h"
#endif

#if LE_AUDIO_BASS_SUPPORT

#define BASS_PRIMARY_ADV_INTERVAL_MIN          32
#define BASS_PRIMARY_ADV_INTERVAL_MAX          160
#define BASS_SECONDARY_ADV_MAX_SKIP            0

#define BASS_SRC_SET_BY_NONE          0x00
#define BASS_SRC_SET_BY_BA            0x01
#define BASS_SRC_SET_BY_LOCAL         0x02

typedef struct
{
    bool                        src_used;

    int                         scan_client_id;
    uint8_t                     source_id;
    uint8_t                     source_from;
    T_CODEC_CFG                 bis_codec_cfg;
    uint8_t                     bis_num;
    uint8_t                     bis_array[2];
    uint8_t                     broadcast_code[BROADCAST_CODE_LEN];
    T_BLE_AUDIO_SYNC_HANDLE     sync_handle;
    T_LE_BIGINFO_ADV_REPORT_INFO big_info;

    uint8_t                     adv_handle;
    T_BLE_ADV_STATE             adv_state;
} T_LE_AUDIO_BASS_CB;

static T_LE_AUDIO_BASS_CB le_audio_bass_cb;

static void bass_adv_callback(uint8_t cb_type, T_BLE_ADV_CB_DATA *p_cb_data)
{
    if (cb_type == BLE_ADV_STATE_CHANGE)
    {
        if (p_cb_data)
        {
            le_audio_bass_cb.adv_state = p_cb_data->p_ble_state_change->state;
        }
    }
    else if (cb_type == BLE_ADV_RPA_CHANGE)
    {
        uint16_t idx = 0;
        uint8_t bass_adv_data[50];
        idx = le_service_get_comm_adv_data(bass_adv_data, 25);

        bass_adv_data[idx++] = 0x03;
        bass_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
        bass_adv_data[idx++] = LO_WORD(GATT_UUID_BASS);
        bass_adv_data[idx++] = HI_WORD(GATT_UUID_BASS);

#if LE_AUDIO_CSIS_SUPPORT
        uint8_t psri_data[CSI_RSI_LEN];
        if (le_csis_srv_get_rsi(psri_data))
        {
            bass_adv_data[idx] = CSI_RSI_LEN + 1;
            idx++;
            bass_adv_data[idx] = GAP_ADTYPE_RSI;
            idx++;
            memcpy(bass_adv_data + idx, psri_data, CSI_RSI_LEN);
            idx += CSI_RSI_LEN;
        }
#endif

        ble_set_adv_data(le_audio_bass_cb.adv_handle, idx, bass_adv_data);
    }
}


static void le_audio_bass_scan_callback(uint8_t cb_type, void *result)
{
    return;
}

static void le_audio_bass_pa_sync_cb(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t cb_type,
                                     void *p_cb_data)
{
    T_BLE_AUDIO_SYNC_CB_DATA *p_sync_cb = (T_BLE_AUDIO_SYNC_CB_DATA *)p_cb_data;
    APP_PRINT_TRACE2("le_audio_bass_pa_sync_cb: handle 0x%x, cb_type 0x%x", handle, cb_type);
    switch (cb_type)
    {
    case MSG_BLE_AUDIO_SYNC_HANDLE_RELEASED:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_SYNC_HANDLE_RELEASED: action_role %d",
                             p_sync_cb->p_sync_handle_released->action_role);
        }
        break;

    case MSG_BLE_AUDIO_ADDR_UPDATE:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_ADDR_UPDATE: advertiser_address %s",
                             TRACE_BDADDR(p_sync_cb->p_addr_update->advertiser_address));
        }
        break;

    case MSG_BLE_AUDIO_PA_SYNC_STATE:
        {
            APP_PRINT_TRACE3("MSG_BLE_AUDIO_PA_SYNC_STATE: sync_state %d, action %d, cause 0x%x",
                             p_sync_cb->p_pa_sync_state->sync_state,
                             p_sync_cb->p_pa_sync_state->action,
                             p_sync_cb->p_pa_sync_state->cause);
            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_WAIT_SCANNING)
            {
                if (le_audio_bass_cb.source_from == BASS_SRC_SET_BY_BA &&
                    le_audio_bass_cb.scan_client_id < 1)
                {
                    le_audio_bass_cb.scan_client_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, GAP_SCAN_FILTER_ANY,
                                                                              le_audio_bass_scan_callback);
                }
            }
            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED ||
                p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)
            {
                if (le_audio_bass_cb.scan_client_id > 0)
                {
                    bt_adap_stop_discovery(le_audio_bass_cb.scan_client_id);
                    le_audio_bass_cb.scan_client_id = 0;
                }

            }

#if UAL_CONSOLE_PRINT
            uint8_t event_buff[50];
            uint16_t buf_len;
            buf_len =  sprintf((char *)event_buff,
                               "bass PA SYNC STATE: 0x%x\r\n", p_sync_cb->p_pa_sync_state->sync_state);

            console_write(event_buff, buf_len);
#endif

        }
        break;

    case MSG_BLE_AUDIO_PA_REPORT_INFO:
        {
        }
        break;

    case MSG_BLE_AUDIO_BASE_DATA_MODIFY_INFO:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_BASE_DATA_MODIFY_INFO: p_base_mapping %p\r\n",
                             p_sync_cb->p_base_data_modify_info->p_base_mapping);
        }
        break;

    case MSG_BLE_AUDIO_PA_BIGINFO:
        {
            APP_PRINT_INFO2("MSG_BLE_AUDIO_PA_BIGINFO: num_bis %d, encryption %d",
                            p_sync_cb->p_le_biginfo_adv_report_info->num_bis,
                            p_sync_cb->p_le_biginfo_adv_report_info->encryption);
            if (memcmp(&le_audio_bass_cb.big_info, p_sync_cb->p_le_biginfo_adv_report_info,
                       sizeof(T_LE_BIGINFO_ADV_REPORT_INFO)) != 0)
            {
                memcpy(&le_audio_bass_cb.big_info, p_sync_cb->p_le_biginfo_adv_report_info,
                       sizeof(T_LE_BIGINFO_ADV_REPORT_INFO));
            }

        }
        break;

    case MSG_BLE_AUDIO_BIG_SYNC_STATE:
        {
            T_BLE_AUDIO_BIG_SYNC_STATE *p_data = p_sync_cb->p_big_sync_state;
            APP_PRINT_INFO4("MSG_BLE_AUDIO_BIG_SYNC_STATE: sync_state %d, action %d, action role %d, cause 0x%x\r\n",
                            p_sync_cb->p_big_sync_state->sync_state,
                            p_sync_cb->p_big_sync_state->action,
                            p_sync_cb->p_big_sync_state->action_role,
                            p_sync_cb->p_big_sync_state->cause);

#if UAL_CONSOLE_PRINT
            uint8_t event_buff[50];
            uint16_t buf_len;
            buf_len =  sprintf((char *)event_buff,
                               "bass BIG SYNC STATE: 0x%x\r\n", p_data->sync_state);

            console_write(event_buff, buf_len);
#endif

            if (p_data->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED)
            {
                ble_audio_route_sink_dev(LE_AUDIO_SINK_FROM_BASS, false);
            }
            else if (p_data->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
            {
                ble_audio_route_sink_dev(LE_AUDIO_SINK_FROM_BASS, true);
            }

        }
        break;

    case MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH:
        {
            APP_PRINT_INFO2("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH: bis_idx %d, cause 0x%x\r\n",
                            p_sync_cb->p_setup_data_path->bis_idx,
                            p_sync_cb->p_setup_data_path->cause);

            if (le_audio_bass_cb.src_used)
            {
                T_BLE_AUDIO_SYNC_INFO sync_info;
                if (!ble_audio_sync_get_info(handle, &sync_info))
                {
                    APP_PRINT_ERROR0("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH, ble_audio_sync_get_info failed");
                    return;
                }
                T_BIG_SETUP_DATA_PATH data;
                data.path_direction = DATA_PATH_OUTPUT_FLAG;
                data.bis_conn_handle = p_sync_cb->p_setup_data_path->bis_conn_handle;
                memset(&data.codec_parsed_data, 0, sizeof(T_CODEC_CFG));

                if (!base_data_get_bis_codec_cfg(sync_info.p_base_mapping, p_sync_cb->p_setup_data_path->bis_idx,
                                                 &data.codec_parsed_data))
                {
                    APP_PRINT_ERROR1("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH can't find bis_idx %d",
                                     p_sync_cb->p_setup_data_path->bis_idx);
                    return;
                }

                handle_bis_data_path_setup_complt_msg(&data);
            }
        }
        break;
    case MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH:
        {
            APP_PRINT_INFO3("MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH: bis_idx %d, bis_conn_handle 0x%x, cause 0x%x",
                            p_sync_cb->p_remove_data_path->bis_idx,
                            p_sync_cb->p_remove_data_path->bis_conn_handle,
                            p_sync_cb->p_remove_data_path->cause);
            T_BIG_REMOVE_DATA_PATH data;
            data.path_direction = DATA_PATH_OUTPUT_FLAG;
            data.bis_conn_handle = p_sync_cb->p_remove_data_path->bis_conn_handle;
            handle_bis_data_path_remove_complt_msg(&data);
        }
        break;

    default:
        break;
    }
    return;
}


uint16_t le_audio_bass_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    APP_PRINT_INFO1("le_audio_bass_handle_msg: msg 0x%x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_BASS_CP_IND:
        {
            T_BASS_CP_IND *p_data = (T_BASS_CP_IND *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_CP_IND: conn_handle 0x%x, cp opcode 0x%x",
                            p_data->conn_handle, p_data->p_cp_data->cp_op);

            switch (p_data->p_cp_data->cp_op)
            {
            case BASS_CP_OP_ADD_SOURCE:
                {
                    T_BASS_CP_ADD_SOURCE *p_add_source = &p_data->p_cp_data->param.add_source;
                    uint8_t bis_num = 0;
                    uint32_t bis_sync = 0;
                    if (le_audio_bass_cb.src_used)
                    {
                        cb_result = BLE_AUDIO_CB_RESULT_REJECT;
                        break;
                    }
                    for (uint8_t i = 0; i < p_add_source->num_subgroups; i++)
                    {
                        if (p_add_source->p_cp_bis_info[i].bis_sync != BASS_CP_BIS_SYNC_NO_PREFER)
                        {
                            bis_num += __builtin_popcount(p_add_source->p_cp_bis_info[i].bis_sync);
                            bis_sync |= p_add_source->p_cp_bis_info[i].bis_sync;
                        }
                    }
                    if (bis_num > 2)
                    {
                        APP_PRINT_ERROR2("BASS_CP_OP_ADD_SOURCE: conn_handle 0x%x, bis num %d limmit",
                                         p_data->conn_handle, bis_num);
                        cb_result = BLE_AUDIO_CB_RESULT_REJECT;
                        break;
                    }
                    le_audio_bass_cb.bis_num = bis_num;
                    bis_num = 0;
                    uint8_t idx = 0;
                    while (bis_sync > 0)
                    {
                        if (bis_sync & 0x01)
                        {
                            le_audio_bass_cb.bis_array[idx++] = bis_num + 1;
                        }
                        bis_num++;
                        bis_sync = bis_sync >> 1;
                    }
                }
                break;

            case BASS_CP_OP_MODIFY_SOURCE:
                {
                    T_BASS_CP_MODIFY_SOURCE *p_modify_source = &p_data->p_cp_data->param.modify_source;
                    uint8_t bis_num = 0;
                    uint32_t bis_sync = 0;
                    if (!le_audio_bass_cb.src_used)
                    {
                        cb_result = BLE_AUDIO_CB_RESULT_REJECT;
                        break;
                    }
                    for (uint8_t i = 0; i < p_modify_source->num_subgroups; i++)
                    {
                        if (p_modify_source->p_cp_bis_info[i].bis_sync != BASS_CP_BIS_SYNC_NO_PREFER)
                        {
                            bis_num += __builtin_popcount(p_modify_source->p_cp_bis_info[i].bis_sync);
                            bis_sync |= p_modify_source->p_cp_bis_info[i].bis_sync;
                        }
                    }
                    if (bis_num > 2)
                    {
                        APP_PRINT_ERROR2("BASS_CP_OP_MODIFY_SOURCE: conn_handle 0x%x, bis num %d limmited",
                                         p_data->conn_handle, bis_num);
                        cb_result = BLE_AUDIO_CB_RESULT_REJECT;
                        break;
                    }
                    le_audio_bass_cb.bis_num = bis_num;
                    bis_num = 0;
                    uint8_t idx = 0;
                    while (bis_sync > 0)
                    {
                        if (bis_sync & 0x01)
                        {
                            le_audio_bass_cb.bis_array[idx++] = bis_num + 1;
                        }
                        bis_num++;
                        bis_sync = bis_sync >> 1;
                    }
                }
                break;

            case BASS_CP_OP_REMOVE_SOURCE:
                {
                    //If local sync sink, do not let remote remove
                    if (!le_audio_bass_cb.src_used ||
                        le_audio_bass_cb.source_id != p_data->p_cp_data->param.remove_source.source_id)
                    {
                        cb_result = BLE_AUDIO_CB_RESULT_REJECT;
                        break;
                    }
                }
                break;
            case BASS_CP_OP_SET_BROADCAST_CODE:
                {
                    T_BASS_CP_SET_BROADCAST_CODE *p_set_code = &p_data->p_cp_data->param.set_broadcast_code;
                    if (le_audio_bass_cb.src_used &&
                        le_audio_bass_cb.source_id == p_set_code->source_id)
                    {
                        memcpy(le_audio_bass_cb.broadcast_code, p_set_code->broadcast_code, BROADCAST_CODE_LEN);
                    }
                    else
                    {
                        T_BASS_BRS_DATA brs_data;
                        T_BLE_AUDIO_SYNC_HANDLE *p_sync_handle = NULL;
                        if (bass_get_brs_char_data(p_set_code->source_id, &brs_data, p_sync_handle))
                        {
                            T_BAP_BC_SRC_INFO *p_src_info = bap_ba_bsnk_find_bst_dev(brs_data.source_address,
                                                                                     brs_data.source_address_type,
                                                                                     brs_data.source_adv_sid, brs_data.broadcast_id);
                            //FIX TODO
                            if (p_src_info)
                            {
                                memcpy(p_src_info->broadcast_code, p_set_code->broadcast_code, BROADCAST_CODE_LEN);
                            }
                        }

                    }
                }
                break;

            default:
                break;
            }
        }
        break;

    //FIX TODO
    case LE_AUDIO_MSG_BASS_BRS_MODIFY:
        {
            T_BASS_BRS_MODIFY *p_data = (T_BASS_BRS_MODIFY *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_BRS_MODIFY: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
            if (le_audio_bass_cb.src_used)
            {
                if (le_audio_bass_cb.source_id == p_data->source_id)
                {
                    if (p_data->p_brs_data->brs_is_used == false)
                    {
                        le_audio_bass_cb.src_used = false;
                        le_audio_bass_cb.source_id = 0;
                        le_audio_bass_cb.source_from = BASS_SRC_SET_BY_NONE;
                        if (le_audio_bass_cb.scan_client_id > 0)
                        {
                            bt_adap_stop_discovery(le_audio_bass_cb.scan_client_id);
                            le_audio_bass_cb.scan_client_id = 0;
                        }
                    }
                }
            }
        }
        break;
    case LE_AUDIO_MSG_BASS_BRS_CHAR_NO_EMPTY:
        {
        }
        break;
    case LE_AUDIO_MSG_BASS_BA_ADD_SOURCE:
        {
            T_BASS_BA_ADD_SOURCE *p_brs_msg = (T_BASS_BA_ADD_SOURCE *)buf;
            le_audio_bass_cb.sync_handle = p_brs_msg->handle;
            le_audio_bass_cb.source_id = p_brs_msg->source_id;
            le_audio_bass_cb.source_from = BASS_SRC_SET_BY_BA;
            le_audio_bass_cb.src_used = true;
            ble_audio_sync_update_cb(p_brs_msg->handle, le_audio_bass_pa_sync_cb);
        }
        break;
    case LE_AUDIO_MSG_BASS_LOCAL_ADD_SOURCE:
        {
            //T_BASS_LOCAL_ADD_SOURCE *p_brs_msg = (T_BASS_LOCAL_ADD_SOURCE *)buf;
        }
        break;
    case LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC:
        {
            T_BASS_SET_PREFER_BIS_SYNC *p_data = (T_BASS_SET_PREFER_BIS_SYNC *)buf;
            T_BLE_AUDIO_SYNC_INFO sync_info;
            APP_PRINT_INFO3("LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC: sync handle %p, source_id %d, num_subgroups %d",
                            p_data->handle, p_data->source_id,
                            p_data->num_subgroups);
            for (uint8_t i = 0; i < p_data->num_subgroups; i++)
            {
                bool found = false;
                if (p_data->p_cp_bis_info[i].bis_sync == BASS_CP_BIS_SYNC_NO_PREFER)
                {
                    if (le_audio_bass_cb.bis_num > 0)
                    {
                        bass_cfg_prefer_bis_sync(p_data->source_id, i, 0);
                        continue;
                    }
                    if (ble_audio_sync_get_info(p_data->handle, &sync_info))
                    {
                        if (sync_info.p_base_mapping)
                        {
                            if (i < sync_info.p_base_mapping->num_subgroups)
                            {
                                //FIX TODO always use lc3 first group first bis index
                                T_BASE_DATA_SUBGROUP_PARAM *p_subgroup = sync_info.p_base_mapping->p_subgroup + i;
                                for (uint8_t j = 0; j < p_subgroup->num_bis; j++)
                                {
                                    T_BASE_DATA_BIS_PARAM *p_bis_param = p_subgroup->p_bis_param + j;
                                    uint8_t codec_id[CODEC_ID_LEN] = {LC3_CODEC_ID, 0, 0, 0, 0};
                                    if (memcmp(p_bis_param->codec_id, codec_id, CODEC_ID_LEN) == 0)
                                    {
                                        if (bass_cfg_prefer_bis_sync(p_data->source_id, p_subgroup->subgroup_idx,
                                                                     (1 << (p_bis_param->bis_index - 1))))
                                        {
                                            le_audio_bass_cb.bis_codec_cfg = p_bis_param->bis_codec_cfg;
                                            le_audio_bass_cb.bis_num = 1;
                                            le_audio_bass_cb.bis_array[0] = p_bis_param->bis_index;
                                            found = true;
                                            break;
                                        }
                                    }
                                }

                                if (!found)
                                {
                                    bass_cfg_prefer_bis_sync(p_data->source_id, i, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
    case LE_AUDIO_MSG_BASS_GET_BROADCAST_CODE:
        {
            T_BASS_GET_BROADCAST_CODE *p_get_info = (T_BASS_GET_BROADCAST_CODE *)buf;
            T_BASS_BRS_DATA brs_data;
            T_BLE_AUDIO_SYNC_HANDLE sync_handle;
            if (bass_get_brs_char_data(p_get_info->source_id, &brs_data, &sync_handle))
            {
                T_BAP_BC_SRC_INFO *p_src_info = bap_ba_bsnk_find_bst_dev(brs_data.source_address,
                                                                         brs_data.source_address_type,
                                                                         brs_data.source_adv_sid, brs_data.broadcast_id);
                //FIX TODO
                if (p_src_info)
                {
                    bass_cfg_broadcast_code(p_get_info->source_id, p_src_info->broadcast_code);
                }
                else
                {
                    bass_send_broadcast_code_required(p_get_info->source_id);
                }
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

void le_bass_setup_sink_audio(void)
{
    uint8_t codec_id[5] = {1, 0, 0, 0, 0};
    codec_id[0] = LC3_CODEC_ID;
    uint32_t controller_delay = 0x1122;
    T_CODEC_CFG codec_parsed_data[2] = {0};
    for (uint8_t i = 0; i < 2; i++)
    {
        memset(&codec_parsed_data[i], 0, sizeof(T_CODEC_CFG));
    }

    T_BLE_AUDIO_SYNC_INFO sync_info;
    APP_PRINT_INFO0("le_bass_setup_sink_audio");
    if ((le_audio_bass_cb.src_used) && (le_audio_bass_cb.bis_num))
    {
        for (uint8_t i = 0; i < le_audio_bass_cb.bis_num; i++)
        {
            ble_audio_bis_setup_data_path(le_audio_bass_cb.sync_handle, le_audio_bass_cb.bis_array[i], codec_id,
                                          controller_delay,
                                          0, NULL);
        }

        if (!ble_audio_sync_get_info(le_audio_bass_cb.sync_handle, &sync_info))
        {
            APP_PRINT_ERROR0("le_bass_setup_sink_audio, ble_audio_sync_get_info failed");
            return;
        }
        for (int i = 0; i < le_audio_bass_cb.bis_num && i < 2; i++)
        {
            if (!base_data_get_bis_codec_cfg(sync_info.p_base_mapping, le_audio_bass_cb.bis_array[i],
                                             &codec_parsed_data[i]))
            {
                APP_PRINT_ERROR1("le_bass_setup_sink_audio can't find bis_idx %d",
                                 le_audio_bass_cb.bis_array[i]);
                return;
            }
        }

        T_AUDIO_FORMAT_INFO src;
        src.type = AUDIO_FORMAT_TYPE_LC3;
        switch (codec_parsed_data[0].sample_frequency)
        {
        case SAMPLING_FREQUENCY_CFG_8K:
            src.attr.lc3.sample_rate = 8000;
            break;
        case SAMPLING_FREQUENCY_CFG_11K:
            src.attr.lc3.sample_rate = 11000;
            break;
        case SAMPLING_FREQUENCY_CFG_16K:
            src.attr.lc3.sample_rate = 16000;
            break;
        case SAMPLING_FREQUENCY_CFG_22K:
            src.attr.lc3.sample_rate = 22000;
            break;
        case SAMPLING_FREQUENCY_CFG_24K:
            src.attr.lc3.sample_rate = 24000;
            break;
        case SAMPLING_FREQUENCY_CFG_32K:
            src.attr.lc3.sample_rate = 32000;
            break;
        case SAMPLING_FREQUENCY_CFG_48K:
            src.attr.lc3.sample_rate = 48000;
            break;
        default:
            APP_PRINT_ERROR1("le_bass_setup_sink_audio source sample rate %d not supported",
                             codec_parsed_data[0].sample_frequency);
            return;
        }

        if (le_audio_bass_cb.bis_num == 1)
        {
            src.attr.lc3.chann_location = __builtin_popcount(codec_parsed_data[0].audio_channel_allocation) < 2
                                          ?
                                          AUDIO_CHANNEL_LOCATION_MONO : (AUDIO_CHANNEL_LOCATION_FL | AUDIO_CHANNEL_LOCATION_FR);
        }
        else
        {
            if (codec_parsed_data[0].audio_channel_allocation & AUDIO_CHANNEL_LOCATION_FL)
            {
                src.attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_FL;
            }
            else
            {
                src.attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_FR;
            }

            if (codec_parsed_data[1].audio_channel_allocation & AUDIO_CHANNEL_LOCATION_FL)
            {
                src.attr.lc3.chann_location |= AUDIO_CHANNEL_LOCATION_FL;
            }
            else
            {
                src.attr.lc3.chann_location |= AUDIO_CHANNEL_LOCATION_FR;
            }
        }

        if (codec_parsed_data[0].frame_duration == FRAME_DURATION_CFG_7_5_MS)
        {
            src.attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_7_5_MS;
        }
        else
        {
            src.attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
        }

        src.attr.lc3.frame_length = codec_parsed_data[0].octets_per_codec_frame;

        if (sync_info.p_base_mapping)
        {
            src.attr.lc3.presentation_delay = sync_info.p_base_mapping->presentation_delay;
        }
        else
        {
            APP_PRINT_ERROR0("le_bass_setup_sink_audio source unkonw presentation_delay");
            src.attr.lc3.presentation_delay = 40000;
        }
        //app_audio_set_path(AUDIO_PATH_NULL, path_out, NULL, &src, true, NULL);
    }
}


void le_bass_remove_sink_audio(void)
{
    if (le_audio_bass_cb.src_used)
    {
        for (uint8_t i = 0; i < le_audio_bass_cb.bis_num; i++)
        {
            ble_audio_bis_remove_data_path(le_audio_bass_cb.sync_handle, le_audio_bass_cb.bis_array[i]);
        }

    }
}


void le_bass_start_adv(void)
{
    uint16_t idx = 0;
    uint8_t  peer_addr[6] = {0, 0, 0, 0, 0, 0};

    uint8_t bass_adv_data[50];
    idx = le_service_get_comm_adv_data(bass_adv_data, 25);

    bass_adv_data[idx++] = 0x03;
    bass_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
    bass_adv_data[idx++] = LO_WORD(GATT_UUID_BASS);
    bass_adv_data[idx++] = HI_WORD(GATT_UUID_BASS);
#if LE_AUDIO_CSIS_SUPPORT
    uint8_t psri_data[CSI_RSI_LEN];
    if (le_csis_srv_get_rsi(psri_data))
    {
        bass_adv_data[idx] = CSI_RSI_LEN + 1;
        idx++;
        bass_adv_data[idx] = GAP_ADTYPE_RSI;
        idx++;
        memcpy(bass_adv_data + idx, psri_data, CSI_RSI_LEN);
        idx += CSI_RSI_LEN;
    }
#endif
    T_BLE_BD_TYPE local_bd_type = ual_support_privacy() ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
    T_BLE_BD_TYPE peer_bd_type = ual_support_privacy() ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;

    if (le_audio_bass_cb.adv_handle == 0xFF)
    {
        le_audio_bass_cb.adv_handle = ble_alloc_adv_instance(bass_adv_callback,
                                                             EXT_ADV_EXTENDED_ADV_CONN_UNDIRECTED,
                                                             BASS_PRIMARY_ADV_INTERVAL_MIN, BASS_PRIMARY_ADV_INTERVAL_MAX,
                                                             local_bd_type, peer_bd_type, peer_addr,
                                                             idx, bass_adv_data, 0, NULL);
    }

    if (le_audio_bass_cb.adv_handle != 0xFF)
    {
        if (le_audio_bass_cb.adv_state == BLE_ADV_STATE_ADVERTISING)
        {
            APP_PRINT_INFO1("le_bass_start_adv: already in advertising adv_handle 0x%x",
                            le_audio_bass_cb.adv_handle);
            return;
        }
        ble_enable_adv(le_audio_bass_cb.adv_handle, 0);
    }

}

void le_bass_stop_adv(void)
{
    if (le_audio_bass_cb.adv_handle != 0xFF)
    {
        ble_remove_adv(le_audio_bass_cb.adv_handle);
    }
    le_audio_bass_cb.adv_handle = 0xFF;
}


void le_audio_bass_init(void)
{
    memset(&le_audio_bass_cb, 0, sizeof(T_LE_AUDIO_BASS_CB));
    le_audio_bass_cb.adv_handle = 0xFF;
}
#endif
