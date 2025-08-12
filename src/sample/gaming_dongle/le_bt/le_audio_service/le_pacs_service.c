#include <string.h>
#include "trace.h"
#include "codec_def.h"
#include "metadata_def.h"
#include "pacs_mgr.h"
#include "le_pacs_service.h"

#if LE_AUDIO_PACS_SUPPORT
const uint8_t pac_media_sink_codec[] =
{
    //Number_of_PAC_records
    1,
    //PAC Record
    LC3_CODEC_ID, 0, 0, 0, 0,//Codec_ID
    //Codec_Specific_Capabilities_Length
    16,
    //Codec_Specific_Capabilities
    0x03,
    CODEC_CAP_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
    (uint8_t)(SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_48K),
    (uint8_t)((SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_48K) >> 8),
    0x02,
    CODEC_CAP_TYPE_SUPPORTED_FRAME_DURATIONS,
    FRAME_DURATION_PREFER_10_MS_BIT | FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT,
    0x02,
    CODEC_CAP_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
    0x05,
    CODEC_CAP_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    0x28, 0x00, 0x78, 0x00,
    //Metadata_Length
    0x04,
    //Metadata
    0x03,
    METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS,
    (uint8_t)(AUDIO_CONTEXT_MEDIA),
    (uint8_t)(AUDIO_CONTEXT_MEDIA >> 8),
};

const uint8_t pac_conversation_sink_codec[] =
{
    //Number_of_PAC_records
    1,
    //PAC Record
    LC3_CODEC_ID, 0, 0, 0, 0,//Codec_ID
    //Codec_Specific_Capabilities_Length
    16,
    //Codec_Specific_Capabilities
    0x03,
    CODEC_CAP_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
    (uint8_t)(SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_48K),
    (uint8_t)((SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_48K) >> 8),
    0x02,
    CODEC_CAP_TYPE_SUPPORTED_FRAME_DURATIONS,
    FRAME_DURATION_PREFER_10_MS_BIT | FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT,
    0x02,
    CODEC_CAP_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
    0x05,
    CODEC_CAP_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    0x28, 0x00, 0x78, 0x00,
    //Metadata_Length
    0x04,
    //Metadata
    0x03,
    METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS,
    (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL),
    (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL >> 8),

};

const uint8_t pac_conversation_source_codec[] =
{
    //Number_of_PAC_records
    1,
    //PAC Record
    LC3_CODEC_ID, 0, 0, 0, 0,//Codec_ID
    //Codec_Specific_Capabilities_Length
    16,
    //Codec_Specific_Capabilities
    0x03,
    CODEC_CAP_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
    (uint8_t)(SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_48K),
    (uint8_t)((SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_48K) >> 8),
    0x02,
    CODEC_CAP_TYPE_SUPPORTED_FRAME_DURATIONS,
    FRAME_DURATION_PREFER_10_MS_BIT | FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT,
    0x02,
    CODEC_CAP_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
    0x05,
    CODEC_CAP_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    0x28, 0x00, 0x78, 0x00,
    //Metadata_Length
    0x04,
    //Metadata
    0x03,
    METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS,
    (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL),
    (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL >> 8),
};

uint16_t dongle_sink_available_contexts = AUDIO_CONTEXT_MEDIA | AUDIO_CONTEXT_CONVERSATIONAL;
uint16_t dongle_source_available_contexts = AUDIO_CONTEXT_MEDIA | AUDIO_CONTEXT_CONVERSATIONAL;

uint16_t le_pacs_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_PACS_READ_AVAILABLE_CONTEXTS_IND:
        {
            T_PACS_READ_AVAILABLE_CONTEXTS_IND *p_data = (T_PACS_READ_AVAILABLE_CONTEXTS_IND *)buf;
            APP_PRINT_INFO1("LE_AUDIO_MSG_PACS_READ_AVAILABLE_CONTEXTS_IND: conn_handle 0x%x",
                            p_data->conn_handle);
            pacs_available_contexts_read_cfm(p_data->conn_handle, p_data->cid, dongle_sink_available_contexts,
                                             dongle_source_available_contexts);
        }
        break;

    case LE_AUDIO_MSG_PACS_WRITE_SINK_AUDIO_LOC_IND:
        {
            T_PACS_WRITE_SINK_AUDIO_LOC_IND *p_data = (T_PACS_WRITE_SINK_AUDIO_LOC_IND *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_PACS_WRITE_SINK_AUDIO_LOC_IND: conn_handle 0x%x, sink_audio_locations 0x%x",
                            p_data->conn_handle,
                            p_data->sink_audio_locations);
        }
        break;

    case LE_AUDIO_MSG_PACS_WRITE_SOURCE_AUDIO_LOC_IND:
        {
            T_PACS_WRITE_SOURCE_AUDIO_LOC_IND *p_data = (T_PACS_WRITE_SOURCE_AUDIO_LOC_IND *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_PACS_WRITE_SOURCE_AUDIO_LOC_IND: conn_handle 0x%x, source_audio_locations 0x%x",
                            p_data->conn_handle,
                            p_data->source_audio_locations);
        }
        break;
    default:
        break;
    }
    return cb_result;
}

void le_pacs_init(void)
{
    T_PACS_PARAMS pacs_params;
    memset(&pacs_params, 0, sizeof(T_PACS_PARAMS));
    pacs_params.sink_locations.is_exist = true;
    pacs_params.sink_locations.is_notify = true;
    pacs_params.sink_locations.is_write = true;
    pacs_params.sink_locations.sink_audio_location = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;
    pacs_params.source_locations.is_exist = true;
    pacs_params.source_locations.is_notify = true;
    pacs_params.source_locations.is_write = true;
    pacs_params.source_locations.source_audio_location = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;
    pacs_params.supported_contexts.is_notify = true;
    pacs_params.supported_contexts.sink_supported_contexts = dongle_sink_available_contexts;
    pacs_params.supported_contexts.source_supported_contexts = dongle_source_available_contexts;

    pacs_pac_add(SERVER_AUDIO_SOURCE, pac_conversation_source_codec,
                 sizeof(pac_conversation_source_codec),
                 true);
    pacs_pac_add(SERVER_AUDIO_SINK, pac_media_sink_codec, sizeof(pac_media_sink_codec), true);
    pacs_pac_add(SERVER_AUDIO_SINK, pac_conversation_sink_codec, sizeof(pac_conversation_sink_codec),
                 true);

    pacs_init(&pacs_params);
}
#endif
