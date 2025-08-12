#include <string.h>
#include "trace.h"
#include "ascs_def.h"
#include "ascs_mgr.h"
#include "pacs_mgr.h"
#include "le_unicast_sink_service.h"
#include "ascs_def.h"
#include "ual_types.h"
#include "ual_list.h"
#include "le_audio_data.h"
#include "ual_adv.h"
#include "ble_isoch_def.h"
#include "gap_cig_mgr.h"
#include "csis_rsi.h"
#include "le_csis_srv_service.h"
#include "app_audio_path.h"
#include "le_service_mgr.h"
#include "le_pacs_service.h"
#include "ual_adapter.h"

#if LE_AUDIO_ASCS_SUPPORT

#define UST_SNK_PRIMARY_ADV_INTERVAL_MIN          32
#define UST_SNK_PRIMARY_ADV_INTERVAL_MAX          160
#define UST_SNK_SECONDARY_ADV_MAX_SKIP            0


typedef struct
{
    bool              streaming;
    uint8_t           adv_handle;
    T_BLE_ADV_STATE   adv_state;
} T_UCT_SNK_CB;

static T_UCT_SNK_CB unicast_sink_cb;

static void unicast_snk_adv_callback(uint8_t cb_type, T_BLE_ADV_CB_DATA *p_cb_data)
{
    if (cb_type == BLE_ADV_STATE_CHANGE)
    {
        if (p_cb_data)
        {
            unicast_sink_cb.adv_state = p_cb_data->p_ble_state_change->state;
        }
    }
    else if (cb_type == BLE_ADV_RPA_CHANGE)
    {
        uint16_t idx = 0;
        uint8_t audio_adv_data[50];

        idx = le_service_get_comm_adv_data(audio_adv_data, 25);

        audio_adv_data[idx] = 0x08;
        idx++;
        audio_adv_data[idx] = GAP_ADTYPE_SERVICE_DATA;
        idx++;
        audio_adv_data[idx] = LO_WORD(GATT_UUID_ASCS);
        idx++;
        audio_adv_data[idx] = HI_WORD(GATT_UUID_ASCS);
        idx++;

        audio_adv_data[idx] = ADV_TARGETED_ANNOUNCEMENT;
        idx++;

#if LE_AUDIO_PACS_SUPPORT
        LE_UINT16_TO_ARRAY(audio_adv_data + idx, dongle_sink_available_contexts);
        idx += 2;
        LE_UINT16_TO_ARRAY(audio_adv_data + idx, dongle_source_available_contexts);
        idx += 2;
#endif

#if LE_AUDIO_CSIS_SUPPORT
        uint8_t psri_data[CSI_RSI_LEN];
        if (le_csis_srv_get_rsi(psri_data))
        {
            audio_adv_data[idx] = CSI_RSI_LEN + 1;
            idx++;
            audio_adv_data[idx] = GAP_ADTYPE_RSI;
            idx++;
            memcpy(audio_adv_data + idx, psri_data, CSI_RSI_LEN);
            idx += CSI_RSI_LEN;
        }
#endif
        ble_set_adv_data(unicast_sink_cb.adv_handle, idx, audio_adv_data);
    }

}

static void unicast_snk_update_ase_state(T_ASCS_ASE_STATE *p_data)
{

    switch (p_data->ase_data.ase_state)
    {
    case ASE_STATE_IDLE:
        break;

    case ASE_STATE_CODEC_CONFIGURED:
        {
            uint32_t presentation_delay_min;
            uint32_t presentation_delay_max;
            uint16_t max_transport_latency;
            LE_ARRAY_TO_UINT24(presentation_delay_min,
                               p_data->ase_data.param.codec_configured.data.presentation_delay_min);
            LE_ARRAY_TO_UINT24(presentation_delay_max,
                               p_data->ase_data.param.codec_configured.data.presentation_delay_max);
            LE_ARRAY_TO_UINT16(max_transport_latency,
                               p_data->ase_data.param.codec_configured.data.max_transport_latency);

            APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_CODEC_CONFIGURED: supported_framing 0x%01x, preferred_phy 0x%01x, preferred_retrans_number 0x%01x",
                            p_data->ase_data.param.codec_configured.data.supported_framing,
                            p_data->ase_data.param.codec_configured.data.preferred_phy,
                            p_data->ase_data.param.codec_configured.data.preferred_retrans_number,
                            p_data->ase_data.param.codec_configured.data.preferred_retrans_number);
            APP_PRINT_INFO3("[BAP][ASCS] ASE_STATE_CODEC_CONFIGURED: max_transport_latency 0x%02x, presentation_delay_min 0x%03x, presentation_delay_max 0x%03x",
                            max_transport_latency,
                            presentation_delay_min, presentation_delay_max);
            APP_PRINT_INFO2("[BAP][ASCS] ASE_STATE_CODEC_CONFIGURED: codec_id %b, codec_spec_cfg_len %d",
                            TRACE_BINARY(5, p_data->ase_data.param.codec_configured.data.codec_id),
                            p_data->ase_data.param.codec_configured.data.codec_spec_cfg_len);
            if (p_data->ase_data.param.codec_configured.data.codec_spec_cfg_len)
            {
                APP_PRINT_INFO1("[BAP][ASCS] codec_spec_cfg %b",
                                TRACE_BINARY(p_data->ase_data.param.codec_configured.data.codec_spec_cfg_len,
                                             p_data->ase_data.param.codec_configured.p_codec_spec_cfg));
            }
        }
        break;

    case ASE_STATE_QOS_CONFIGURED:
        {
            uint16_t max_sdu_size;
            uint16_t max_transport_latency;
            uint32_t sdu_interval;
            uint32_t presentation_delay;
            LE_ARRAY_TO_UINT24(sdu_interval, p_data->ase_data.param.qos_configured.sdu_interval);
            LE_ARRAY_TO_UINT24(presentation_delay, p_data->ase_data.param.qos_configured.presentation_delay);
            LE_ARRAY_TO_UINT16(max_sdu_size, p_data->ase_data.param.qos_configured.max_sdu);
            LE_ARRAY_TO_UINT16(max_transport_latency,
                               p_data->ase_data.param.qos_configured.max_transport_latency);
            APP_PRINT_INFO5("[BAP][ASCS] ASE_STATE_QOS_CONFIGURED: cig_id %d, cis_id %d, sdu_interval 0x%03x, framing 0x%01x, phy 0x%x",
                            p_data->ase_data.param.qos_configured.cig_id,
                            p_data->ase_data.param.qos_configured.cis_id, sdu_interval,
                            p_data->ase_data.param.qos_configured.framing,
                            p_data->ase_data.param.qos_configured.phy);
            APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_QOS_CONFIGURED: max_sdu 0x%02x, retransmission_number %d, max_transport_latency 0x%02x, presentation_delay 0x%03x",
                            max_sdu_size, p_data->ase_data.param.qos_configured.retransmission_number,
                            max_transport_latency, presentation_delay);
        }
        break;

    case ASE_STATE_ENABLING:
        APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_ENABLING: cig_id %d, cis_id %d, metadata[%d] %b",
                        p_data->ase_data.param.enabling.cig_id,
                        p_data->ase_data.param.enabling.cis_id,
                        p_data->ase_data.param.enabling.metadata_length,
                        TRACE_BINARY(p_data->ase_data.param.enabling.metadata_length,
                                     p_data->ase_data.param.enabling.p_metadata));

        break;

    case ASE_STATE_STREAMING:
        {
            APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_STREAMING: cig_id %d, cis_id %d, metadata[%d] %b",
                            p_data->ase_data.param.streaming.cig_id,
                            p_data->ase_data.param.streaming.cis_id,
                            p_data->ase_data.param.streaming.metadata_length,
                            TRACE_BINARY(p_data->ase_data.param.streaming.metadata_length,
                                         p_data->ase_data.param.streaming.p_metadata));
        }
        break;

    case ASE_STATE_DISABLING:
        {
            APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_DISABLING: cig_id %d, cis_id %d, metadata[%d] %b",
                            p_data->ase_data.param.disabling.cig_id,
                            p_data->ase_data.param.disabling.cis_id,
                            p_data->ase_data.param.disabling.metadata_length,
                            TRACE_BINARY(p_data->ase_data.param.disabling.metadata_length,
                                         p_data->ase_data.param.disabling.p_metadata));
        }
        break;
    case ASE_STATE_RELEASING:
        {
            APP_PRINT_INFO2("[BAP][ASCS] ASE_STATE_RELEASING: ase_id %d, direction %d",
                            p_data->ase_data.ase_id, p_data->ase_data.direction);
            if (p_data->ase_data.direction == SERVER_AUDIO_SINK)
            {
                if (unicast_sink_cb.streaming)
                {
                    unicast_sink_cb.streaming = false;
                }
            }
        }
        break;
    default:
        break;
    }
}


void le_unicast_snk_start_adv(void)
{
    uint16_t idx = 0;
    uint8_t  peer_addr[6] = {0, 0, 0, 0, 0, 0};
    uint8_t audio_adv_data[50];

    idx = le_service_get_comm_adv_data(audio_adv_data, 25);

    audio_adv_data[idx] = 0x08;
    idx++;
    audio_adv_data[idx] = GAP_ADTYPE_SERVICE_DATA;
    idx++;
    audio_adv_data[idx] = LO_WORD(GATT_UUID_ASCS);
    idx++;
    audio_adv_data[idx] = HI_WORD(GATT_UUID_ASCS);
    idx++;

    audio_adv_data[idx] = ADV_TARGETED_ANNOUNCEMENT;
    idx++;

#if LE_AUDIO_PACS_SUPPORT
    LE_UINT16_TO_ARRAY(audio_adv_data + idx,
                       dongle_sink_available_contexts);
    idx += 2;
    LE_UINT16_TO_ARRAY(audio_adv_data + idx,
                       dongle_source_available_contexts);
    idx += 2;
#endif

#if LE_AUDIO_CSIS_SUPPORT
    uint8_t psri_data[CSI_RSI_LEN];
    if (le_csis_srv_get_rsi(psri_data))
    {
        audio_adv_data[idx] = CSI_RSI_LEN + 1;
        idx++;
        audio_adv_data[idx] = GAP_ADTYPE_RSI;
        idx++;
        memcpy(audio_adv_data + idx, psri_data, CSI_RSI_LEN);
        idx += CSI_RSI_LEN;
    }
#endif
    T_BLE_BD_TYPE local_bd_type = ual_support_privacy() ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
    T_BLE_BD_TYPE peer_bd_type = ual_support_privacy() ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;

    if (unicast_sink_cb.adv_handle == 0xFF)
    {
        unicast_sink_cb.adv_handle = ble_alloc_adv_instance(unicast_snk_adv_callback,
                                                            EXT_ADV_EXTENDED_ADV_CONN_UNDIRECTED,
                                                            UST_SNK_PRIMARY_ADV_INTERVAL_MIN, UST_SNK_PRIMARY_ADV_INTERVAL_MAX,
                                                            local_bd_type, peer_bd_type, peer_addr,
                                                            idx, audio_adv_data, 0, NULL);
    }

    if (unicast_sink_cb.adv_handle != 0xFF)
    {
        if (unicast_sink_cb.adv_state == BLE_ADV_STATE_ADVERTISING)
        {
            APP_PRINT_INFO1("le_unicast_snk_start_adv: already in advertising adv_handle 0x%x",
                            unicast_sink_cb.adv_handle);
            return;
        }
        ble_enable_adv(unicast_sink_cb.adv_handle, 0);
    }

}

void le_unicast_snk_stop_adv(void)
{
    if (unicast_sink_cb.adv_handle != 0xFF)
    {
        ble_remove_adv(unicast_sink_cb.adv_handle);
    }
    unicast_sink_cb.adv_handle = 0xFF;
}

uint16_t le_unicast_sink_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    uint8_t i = 0;
    APP_PRINT_INFO1("le_unicast_sink_handle_msg: msg %x", msg);

    switch (msg)
    {
    case LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH:
        {
            T_ASCS_SETUP_DATA_PATH *p_data = (T_ASCS_SETUP_DATA_PATH *)buf;
            APP_PRINT_INFO4("LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH: conn_handle 0x%x, ase_id %d, path_direction 0x%x, cis_conn_handle 0x%x",
                            p_data->conn_handle,
                            p_data->ase_id,
                            p_data->path_direction,
                            p_data->cis_conn_handle);
            T_CIS_SETUP_DATA_PATH data;
            T_ASE_QOS_CFG_STATE_DATA cfg_data;
            data.cis_conn_handle = p_data->cis_conn_handle;
            data.codec_parsed_data = p_data->codec_parsed_data;
            data.path_direction = p_data->path_direction;
            //FIX TODO
            if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
            {
                uint32_t presentation_delay;
                if (ascs_get_qos_cfg(p_data->conn_handle, p_data->ase_id, &cfg_data))
                {
                    presentation_delay = cfg_data.presentation_delay[0] | cfg_data.presentation_delay[1] << 8 |
                                         cfg_data.presentation_delay[2] << 16;
                }
                else
                {
                    APP_PRINT_ERROR0("LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH source unkonw presentation_delay");
                    presentation_delay = 40000;
                }

                le_audio_data_start_encode_path(&p_data->codec_parsed_data, presentation_delay);
            }
            else
            {
                T_AUDIO_FORMAT_INFO src;
                src.type = AUDIO_FORMAT_TYPE_LC3;
                switch (p_data->codec_parsed_data.sample_frequency)
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
                    APP_PRINT_ERROR1("LE_AUDIO_MSG_ASCS_SETUP_DATA_PATHsource sample rate %d not supported",
                                     p_data->codec_parsed_data.sample_frequency);
                    return BLE_AUDIO_CB_RESULT_SUCCESS;
                }

                if ((__builtin_popcount(p_data->codec_parsed_data.audio_channel_allocation)) < 2)
                {
                    src.attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO;
                }
                else
                {
                    if (p_data->codec_parsed_data.audio_channel_allocation & AUDIO_CHANNEL_LOCATION_FL)
                    {
                        src.attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_FL;
                    }
                    if (p_data->codec_parsed_data.audio_channel_allocation & AUDIO_CHANNEL_LOCATION_FR)
                    {
                        src.attr.lc3.chann_location |= AUDIO_CHANNEL_LOCATION_FR;
                    }
                }

                if (p_data->codec_parsed_data.frame_duration == FRAME_DURATION_CFG_7_5_MS)
                {
                    src.attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_7_5_MS;
                }
                else
                {
                    src.attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
                }

                src.attr.lc3.frame_length = p_data->codec_parsed_data.octets_per_codec_frame;
                APP_PRINT_INFO4("LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH: sample_rate %d, chann_location 0x%x, frame_length 0x%x, frame_duration 0x%x",
                                src.attr.lc3.sample_rate,
                                src.attr.lc3.chann_location,
                                src.attr.lc3.frame_length,
                                src.attr.lc3.frame_duration);
                if (ascs_get_qos_cfg(p_data->conn_handle, p_data->ase_id, &cfg_data))
                {
                    src.attr.lc3.presentation_delay = cfg_data.presentation_delay[0] | cfg_data.presentation_delay[1] <<
                                                      8 | cfg_data.presentation_delay[2] << 16;
                }
                else
                {
                    APP_PRINT_ERROR0("LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH source unkonw presentation_delay");
                    src.attr.lc3.presentation_delay = 40000;
                }
                //T_AUDIO_PATH_DEVICE path_out = (le_audio_get_audio_path() == LE_AUDIO_AUX_PATH ||
                //                                le_audio_get_audio_path() == LE_AUDIO_AUX_MIC_PATH) ? AUDIO_PATH_OUT_AUX : AUDIO_PATH_OUT_USB;
                //app_audio_set_path(AUDIO_PATH_NULL, path_out, NULL, &src, true, NULL);
            }
            handle_cis_data_path_setup_cmplt_msg(&data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_REMOVE_DATA_PATH:
        {
            T_ASCS_REMOVE_DATA_PATH *p_data = (T_ASCS_REMOVE_DATA_PATH *)buf;
            T_CIS_REMOVE_DATA_PATH data;
            data.cis_conn_handle = p_data->cis_conn_handle;
            data.path_direction = p_data->path_direction;
            handle_cis_data_path_remove_complt_msg(&data);
            if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
            {
                le_audio_data_stop_encode_path();
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_ASE_STATE:
        {
            T_ASCS_ASE_STATE *p_data = (T_ASCS_ASE_STATE *)buf;
            unicast_snk_update_ase_state(p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_CONFIG_CODEC_IND:
        {
            T_ASCS_CP_CONFIG_CODEC_IND *p_data = (T_ASCS_CP_CONFIG_CODEC_IND *)buf;
            for (i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO5("ase param[%d]: ase_id %d, target_latency %d, target_phy %d, codec_id %b",
                                i, p_data->param[i].data.ase_id,
                                p_data->param[i].data.target_latency,
                                p_data->param[i].data.target_phy,
                                TRACE_BINARY(5, p_data->param[i].data.codec_id));

                APP_PRINT_INFO7("ase param[%d]: type_exist 0x%x, frame_duration 0x%x, sample_frequency 0x%x, codec_frame_blocks_per_sdu %d, octets_per_codec_frame %d, audio_channel_allocation 0x%x",
                                i, p_data->param[i].codec_parsed_data.type_exist,
                                p_data->param[i].codec_parsed_data.frame_duration,
                                p_data->param[i].codec_parsed_data.sample_frequency,
                                p_data->param[i].codec_parsed_data.codec_frame_blocks_per_sdu,
                                p_data->param[i].codec_parsed_data.octets_per_codec_frame,
                                p_data->param[i].codec_parsed_data.audio_channel_allocation
                               );
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_CONFIG_QOS_IND:
        {
            T_ASCS_CP_CONFIG_QOS_IND *p_data = (T_ASCS_CP_CONFIG_QOS_IND *)buf;
            for (i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO4("ase param[%d]: ase_id %d,  cig_id %d, cis_id %d",
                                i, p_data->param[i].ase_id,
                                p_data->param[i].cig_id, p_data->param[i].cis_id);
                APP_PRINT_INFO8("ase param[%d]: sdu_interval %x %x %x, framing %d, phy 0x%x, max_sdu  %x %x",
                                i, p_data->param[i].sdu_interval[0], p_data->param[i].sdu_interval[1],
                                p_data->param[i].sdu_interval[2],
                                p_data->param[i].framing, p_data->param[i].phy,
                                p_data->param[i].max_sdu[0], p_data->param[i].max_sdu[1]);

                APP_PRINT_INFO7("ase param[%d]: retransmission_number %d, max_transport_latency %x %x, presentation_delay  %x %x %x",
                                i,
                                p_data->param[i].retransmission_number,
                                p_data->param[i].max_transport_latency[0],
                                p_data->param[i].max_transport_latency[1],
                                p_data->param[i].presentation_delay[0],
                                p_data->param[i].presentation_delay[1],
                                p_data->param[i].presentation_delay[2]);
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_ENABLE_IND:
        {
            T_ASCS_CP_ENABLE_IND *p_data = (T_ASCS_CP_ENABLE_IND *)buf;
            bool sink_stream = false;
            T_ASE_CHAR_DATA ase_data;
            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_ENABLE_IND: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);
            for (i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO4("ase param[%d]: ase_id %d, metadata_length %d, metadata %b",
                                i, p_data->param[i].ase_id,
                                p_data->param[i].metadata_length,
                                TRACE_BINARY(p_data->param[i].metadata_length, p_data->param[i].p_metadata));
                if (!ascs_get_ase_data(p_data->conn_handle, p_data->param[i].ase_id, &ase_data))
                {
                    cb_result = ASE_CP_RESP_UNSPECIFIED_ERROR;
                    break;
                }
                if (ase_data.direction == SERVER_AUDIO_SINK)
                {
                    sink_stream = true;
                }
            }

            if (sink_stream)
            {
                if (unicast_sink_cb.streaming)
                {
                    cb_result = ASE_CP_RESP_INSUFFICIENT_RESOURCE;
                }
                else
                {
                    unicast_sink_cb.streaming = true;
                    cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_DISABLE_IND:
        {
            T_ASCS_CP_DISABLE_IND *p_data = (T_ASCS_CP_DISABLE_IND *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_ASCS_CP_DISABLE_IND: conn_handle 0x%x, number_of_ase %d, ase_ids %b",
                            p_data->conn_handle,
                            p_data->number_of_ase, TRACE_BINARY(p_data->number_of_ase, p_data->ase_id));

            for (i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO2("ase param[%d]: ase_id %d", i, p_data->ase_id[i]);
                T_ASE_CHAR_DATA ase_data;
                if (!ascs_get_ase_data(p_data->conn_handle, p_data->ase_id[i], &ase_data))
                {
                    cb_result = ASE_CP_RESP_UNSPECIFIED_ERROR;
                    break;
                }
                if (ase_data.direction == SERVER_AUDIO_SINK)
                {
                    unicast_sink_cb.streaming = false;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_UPDATE_METADATA_IND:
        {
            T_ASCS_CP_UPDATE_METADATA_IND *p_data = (T_ASCS_CP_UPDATE_METADATA_IND *)buf;
            for (i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO4("ase param[%d]: ase_id %d, metadata_length %d, metadata %b",
                                i, p_data->param[i].ase_id,
                                p_data->param[i].metadata_length,
                                TRACE_BINARY(p_data->param[i].metadata_length, p_data->param[i].p_metadata));
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CIS_REQUEST_IND:
        {
            T_ASCS_CIS_REQUEST_IND *p_data = (T_ASCS_CIS_REQUEST_IND *)buf;
            APP_PRINT_INFO6("LE_AUDIO_MSG_ASCS_CIS_REQUEST_IND: conn_handle 0x%x, cis_conn_handle 0x%x, snk_ase_id %d, snk_ase_state %d, src_ase_id %d, src_ase_state %d",
                            p_data->conn_handle, p_data->cis_conn_handle,
                            p_data->snk_ase_id, p_data->snk_ase_state,
                            p_data->src_ase_id, p_data->src_ase_state);
        }
        break;

    default:
        break;
    }
    return cb_result;
}

void le_unicast_sink_init(void)
{
    memset(&unicast_sink_cb, 0, sizeof(T_UCT_SNK_CB));
    unicast_sink_cb.adv_handle = 0xFF;
}

#endif
