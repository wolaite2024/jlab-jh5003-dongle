#include <string.h>
#include "trace.h"
#include "metadata_def.h"
#include "pacs_mgr.h"
#include "os_mem.h"
#include "codec_qos.h"
#include "app_link_util.h"
#include "app_cfg.h"
#include "app_lea_pacs.h"

#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT
#define SINK_PAC_CNT 2
#define SOURCE_PAC_CNT 1

typedef enum
{
    PAC_MEDIA_SR_44_1K    = 0x01,
    PAC_MEDIA_SR_48_1K    = 0x02,
    PAC_MEDIA_MAX,
} T_LEA_PAC_MEDIA_SAMPLE_RATE;

typedef enum
{
    PAC_CONVO_USR_16K     = 0x01,
    PAC_CONVO_USR_24K     = 0x02,
    PAC_CONVO_USR_32K     = 0x04,
    PAC_CONVO_USR_MAX,
} T_LEA_PAC_CONVO_UPSTREAM_SAMPLE_RATE;

typedef enum
{
    PAC_CONVO_DSR_16K     = 0x01,
    PAC_CONVO_DSR_48K     = 0x02,
    PAC_CONVO_DSR_32K     = 0x04,
    PAC_CONVO_DSR_24K     = 0x08,
    PAC_CONVO_DSR_44_1K   = 0x10,
    PAC_CONVO_DSR_MAX,
} T_LEA_PAC_CONVO_DOWNSTREAM_SAMPLE_RATE;

typedef struct
{
    uint8_t codec_id[5];
    uint8_t capabilities_len;
    uint8_t sample_rate_len;
    uint8_t sample_rate[3];
    uint8_t duration_len;
    uint8_t duration[2];
    uint8_t channel_cnt_len;
    uint8_t channel_cnt[2];
    uint8_t frame_len;
    uint8_t frame[5];
    uint8_t frames_per_sdu_len;
    uint8_t frames_per_sdu[2];
    uint8_t metadata_len;
    uint8_t metadata[4];
} T_LEA_PAC_RECORD;

typedef struct
{
    uint16_t convo_downstream_sample_rate;
    uint16_t convo_upstream_sample_rate;
    uint16_t media_sample_rate;
} T_LEA_PAC_SAMPLE_RATE;

typedef struct
{
    uint16_t frame_len;
    bool update;
} T_LEA_PAC_FRAME_LEN;

typedef struct
{
    uint16_t sample_rate;
    T_LEA_PAC_FRAME_LEN frame_len_min;
    T_LEA_PAC_FRAME_LEN frame_len_max;
} T_LEA_PAC_FRAME_LEN_INFO;

typedef struct
{
    T_LEA_PAC_RECORD *p_pac_record;
    T_LEA_PAC_FRAME_LEN_INFO frame_len_info;
    uint8_t qos;
    uint8_t channel_cnt;
    uint8_t frames_per_sdu;
    uint16_t context;
} T_LEA_PAC_DB;

static const uint16_t app_lea_pac_codec_item_sr_map[CODEC_CFG_ITEM_LC3_MAX] =
{
    SAMPLING_FREQUENCY_8K, SAMPLING_FREQUENCY_8K,
    SAMPLING_FREQUENCY_16K, SAMPLING_FREQUENCY_16K,
    SAMPLING_FREQUENCY_24K, SAMPLING_FREQUENCY_24K,
    SAMPLING_FREQUENCY_32K, SAMPLING_FREQUENCY_32K,
    SAMPLING_FREQUENCY_44_1K, SAMPLING_FREQUENCY_44_1K,
    SAMPLING_FREQUENCY_48K, SAMPLING_FREQUENCY_48K,
    SAMPLING_FREQUENCY_48K, SAMPLING_FREQUENCY_48K,
    SAMPLING_FREQUENCY_48K, SAMPLING_FREQUENCY_48K
};

const static T_CODEC_CFG_ITEM app_lea_pac_media_codec[] =
{
    CODEC_CFG_ITEM_16_2,
    CODEC_CFG_ITEM_24_2,
    CODEC_CFG_ITEM_441_1,
    CODEC_CFG_ITEM_441_2,
    CODEC_CFG_ITEM_48_1,
    CODEC_CFG_ITEM_48_6,
};

const static T_CODEC_CFG_ITEM app_lea_pac_convo_downstream_codec[] =
{
    CODEC_CFG_ITEM_16_1,
    CODEC_CFG_ITEM_16_2,
    CODEC_CFG_ITEM_24_1,
    CODEC_CFG_ITEM_24_2,
    CODEC_CFG_ITEM_32_1,
    CODEC_CFG_ITEM_32_2,
    CODEC_CFG_ITEM_48_1,
    CODEC_CFG_ITEM_48_4,
};

const static T_CODEC_CFG_ITEM app_lea_pac_convo_upstream_codec[] =
{
    CODEC_CFG_ITEM_16_1,
    CODEC_CFG_ITEM_16_2,
    CODEC_CFG_ITEM_24_1,
    CODEC_CFG_ITEM_24_2,
    CODEC_CFG_ITEM_32_1,
    CODEC_CFG_ITEM_32_2,
};

static uint16_t app_lea_pac_sink_available_contexts = AUDIO_CONTEXT_UNSPECIFIED |
                                                      AUDIO_CONTEXT_MEDIA |
                                                      AUDIO_CONTEXT_CONVERSATIONAL |
                                                      AUDIO_CONTEXT_RINGTONE |
                                                      AUDIO_CONTEXT_VOICE_ASSISTANTS;

static uint16_t app_lea_pac_source_available_contexts = AUDIO_CONTEXT_UNSPECIFIED |
                                                        AUDIO_CONTEXT_CONVERSATIONAL |
                                                        AUDIO_CONTEXT_RINGTONE |
                                                        AUDIO_CONTEXT_VOICE_ASSISTANTS;

static uint16_t app_lea_pac_sink_supported_contexts = AUDIO_CONTEXT_UNSPECIFIED |
                                                      AUDIO_CONTEXT_MEDIA |
                                                      AUDIO_CONTEXT_CONVERSATIONAL |
                                                      AUDIO_CONTEXT_RINGTONE |
                                                      AUDIO_CONTEXT_VOICE_ASSISTANTS;

static uint16_t app_lea_pac_source_supported_contexts = AUDIO_CONTEXT_UNSPECIFIED |
                                                        AUDIO_CONTEXT_CONVERSATIONAL |
                                                        AUDIO_CONTEXT_RINGTONE |
                                                        AUDIO_CONTEXT_VOICE_ASSISTANTS;

static uint8_t app_lea_pac_sink_pac_id = 0;
static uint8_t app_lea_pac_source_pac_id = 0;
static uint8_t app_lea_pac_sink_record_size = 0;
static uint8_t app_lea_pac_source_record_size = 0;
static uint8_t *p_app_lea_pac_sink_record = NULL;
static uint8_t *p_app_lea_pac_source_record = NULL;

static uint16_t app_lea_pacs_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    bool handle = true;

    switch (msg)
    {
    case LE_AUDIO_MSG_PACS_READ_AVAILABLE_CONTEXTS_IND:
        {
            T_PACS_READ_AVAILABLE_CONTEXTS_IND *p_data = (T_PACS_READ_AVAILABLE_CONTEXTS_IND *)buf;
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);

            if (p_link)
            {
                pacs_available_contexts_read_cfm(p_data->conn_handle, p_data->cid, p_link->sink_available_contexts,
                                                 p_link->source_available_contexts);
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_lea_pacs_ble_audio_cback: msg 0x%04X", msg);
    }

    return cb_result;
}

static void app_lea_pacs_update_pac_record(T_LEA_PAC_DB pac_db)
{
    pac_db.p_pac_record->codec_id[0] = LC3_CODEC_ID;
    pac_db.p_pac_record->capabilities_len = 19;

    pac_db.p_pac_record->sample_rate_len = 3;
    pac_db.p_pac_record->sample_rate[0] = CODEC_CAP_TYPE_SUPPORTED_SAMPLING_FREQUENCIES;
    pac_db.p_pac_record->sample_rate[1] = pac_db.frame_len_info.sample_rate;
    pac_db.p_pac_record->sample_rate[2] = pac_db.frame_len_info.sample_rate >> 8;

    pac_db.p_pac_record->duration_len = 2;
    pac_db.p_pac_record->duration[0] = CODEC_CAP_TYPE_SUPPORTED_FRAME_DURATIONS;
    pac_db.p_pac_record->duration[1] =  pac_db.qos;

    pac_db.p_pac_record->channel_cnt_len = 2;
    pac_db.p_pac_record->channel_cnt[0] = CODEC_CAP_TYPE_AUDIO_CHANNEL_COUNTS;
    pac_db.p_pac_record->channel_cnt[1] = pac_db.channel_cnt;

    pac_db.p_pac_record->frame_len = 5;
    pac_db.p_pac_record->frame[0] = CODEC_CAP_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME;
    pac_db.p_pac_record->frame[1] = pac_db.frame_len_info.frame_len_min.frame_len;
    pac_db.p_pac_record->frame[2] = (pac_db.frame_len_info.frame_len_min.frame_len >> 8);
    pac_db.p_pac_record->frame[3] = pac_db.frame_len_info.frame_len_max.frame_len;
    pac_db.p_pac_record->frame[4] = (pac_db.frame_len_info.frame_len_max.frame_len >> 8);

    pac_db.p_pac_record->frames_per_sdu_len = 2;
    pac_db.p_pac_record->frames_per_sdu[0] = CODEC_CAP_TYPE_MAX_SUPPORTED_FRAMES_PER_SDU;
    pac_db.p_pac_record->frames_per_sdu[1] = pac_db.frames_per_sdu;

    pac_db.p_pac_record->metadata_len = 4;
    pac_db.p_pac_record->metadata[0] = 3;
    pac_db.p_pac_record->metadata[1] = METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS;
    pac_db.p_pac_record->metadata[2] = pac_db.context;
    pac_db.p_pac_record->metadata[3] = (pac_db.context >> 8);
}

static void app_lea_pacs_update_sample_rate(T_LEA_PAC_SAMPLE_RATE *p_pac_sr)
{
    if (app_cfg_const.media_downstream_sample_rate & PAC_MEDIA_SR_44_1K)
    {
        p_pac_sr->media_sample_rate |= SAMPLING_FREQUENCY_44_1K;
    }

    if (app_cfg_const.media_downstream_sample_rate & PAC_MEDIA_SR_48_1K)
    {
        p_pac_sr->media_sample_rate |= SAMPLING_FREQUENCY_48K;
    }

    if (app_cfg_const.convo_downstream_sample_rate & PAC_CONVO_DSR_16K)
    {
        p_pac_sr->convo_downstream_sample_rate |= SAMPLING_FREQUENCY_16K;
    }

    if (app_cfg_const.convo_downstream_sample_rate & PAC_CONVO_DSR_48K)
    {
        p_pac_sr->convo_downstream_sample_rate |= SAMPLING_FREQUENCY_48K;
    }

    if (app_cfg_const.convo_downstream_sample_rate & PAC_CONVO_DSR_32K)
    {
        p_pac_sr->convo_downstream_sample_rate |= SAMPLING_FREQUENCY_32K;
    }

    if (app_cfg_const.convo_downstream_sample_rate & PAC_CONVO_DSR_24K)
    {
        p_pac_sr->convo_downstream_sample_rate |= SAMPLING_FREQUENCY_24K;
    }

    if (app_cfg_const.convo_upstream_sample_rate & PAC_CONVO_USR_16K)
    {
        p_pac_sr->convo_upstream_sample_rate |= SAMPLING_FREQUENCY_16K;
    }

    if (app_cfg_const.convo_upstream_sample_rate & PAC_CONVO_USR_24K)
    {
        p_pac_sr->convo_upstream_sample_rate |= SAMPLING_FREQUENCY_24K;
    }

    if (app_cfg_const.convo_upstream_sample_rate & PAC_CONVO_USR_32K)
    {
        p_pac_sr->convo_upstream_sample_rate |= SAMPLING_FREQUENCY_32K;
    }
}

static void app_lea_pacs_update_frame_len(T_CODEC_CFG_ITEM item,
                                          T_LEA_PAC_FRAME_LEN_INFO *p_frame_len_info)
{
    T_CODEC_CFG preferred_cfg = {0};

    codec_preferred_cfg_get(item, &preferred_cfg);

    if (preferred_cfg.octets_per_codec_frame >= p_frame_len_info->frame_len_max.frame_len)
    {
        if (p_frame_len_info->frame_len_max.update)
        {
            if (p_frame_len_info->frame_len_min.update)
            {
                if (p_frame_len_info->frame_len_min.frame_len < preferred_cfg.octets_per_codec_frame)
                {
                    p_frame_len_info->frame_len_max.frame_len = preferred_cfg.octets_per_codec_frame;
                }
            }
            else
            {
                p_frame_len_info->frame_len_min.frame_len = p_frame_len_info->frame_len_max.frame_len;
                p_frame_len_info->frame_len_max.frame_len = preferred_cfg.octets_per_codec_frame;
                p_frame_len_info->frame_len_min.update = true;
            }
        }
        else
        {
            p_frame_len_info->frame_len_max.frame_len = preferred_cfg.octets_per_codec_frame;
            p_frame_len_info->frame_len_max.update = true;
        }
    }
    else
    {
        if (p_frame_len_info->frame_len_min.update)
        {
            if (p_frame_len_info->frame_len_max.update)
            {
                if (preferred_cfg.octets_per_codec_frame < p_frame_len_info->frame_len_min.frame_len)
                {
                    p_frame_len_info->frame_len_min.frame_len = preferred_cfg.octets_per_codec_frame;
                }
            }
            else
            {
                p_frame_len_info->frame_len_max.frame_len = p_frame_len_info->frame_len_min.frame_len;
                p_frame_len_info->frame_len_max.update = true;
            }
        }
        else
        {
            p_frame_len_info->frame_len_min.frame_len = preferred_cfg.octets_per_codec_frame;
            p_frame_len_info->frame_len_min.update = true;
        }
    }
}

static bool app_lea_pacs_frame_len_setting(const T_CODEC_CFG_ITEM *p_codec, uint8_t codec_num,
                                           T_LEA_PAC_FRAME_LEN_INFO *p_frame_len_info)
{
    if (p_codec == NULL)
    {
        return false;
    }

    for (uint8_t i = 0; i < codec_num; i++)
    {
        if (app_lea_pac_codec_item_sr_map[p_codec[i]] & p_frame_len_info->sample_rate)
        {
            app_lea_pacs_update_frame_len(p_codec[i], p_frame_len_info);
        }
    }

    if (!p_frame_len_info->frame_len_min.update && !p_frame_len_info->frame_len_max.update)
    {
        APP_PRINT_TRACE0("app_lea_pacs_frame_len_setting: set frame length fail");
        return false;
    }

    if (!p_frame_len_info->frame_len_min.update)
    {
        p_frame_len_info->frame_len_min.frame_len = p_frame_len_info->frame_len_max.frame_len;
    }
    else if (!p_frame_len_info->frame_len_max.update)
    {
        p_frame_len_info->frame_len_max.frame_len = p_frame_len_info->frame_len_min.frame_len;
    }
    return true;
}

static void app_lea_pacs_set_sink_pac(bool low_latency, T_LEA_PAC_SAMPLE_RATE pac_sr)
{
    uint8_t *p_sink_pac_record = p_app_lea_pac_sink_record;
    T_LEA_PAC_FRAME_LEN_INFO media_frame_len_info = {0};
    T_LEA_PAC_FRAME_LEN_INFO convo_ds_frame_len_info = {0};
    T_LEA_PAC_DB pac_db = {0};

    *p_sink_pac_record = SINK_PAC_CNT;//PAC record num for pac_check_record()
    p_sink_pac_record++;

#if TARGET_LE_AUDIO_GAMING
    //le audio gaming dongle use frame duration 10ms
    pac_db.qos = FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_PREFER_10_MS_BIT;
#else
    if (low_latency)
    {
        pac_db.qos = FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT |
                     FRAME_DURATION_PREFER_7_5_MS_BIT;
    }
    else
    {
        pac_db.qos = FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_PREFER_10_MS_BIT;
    }
#endif

    media_frame_len_info.sample_rate = pac_sr.media_sample_rate;
    convo_ds_frame_len_info.sample_rate = pac_sr.convo_downstream_sample_rate;
    app_lea_pacs_frame_len_setting(app_lea_pac_media_codec,
                                   sizeof(app_lea_pac_media_codec),
                                   &media_frame_len_info);
    app_lea_pacs_frame_len_setting(app_lea_pac_convo_downstream_codec,
                                   sizeof(app_lea_pac_convo_downstream_codec),
                                   &convo_ds_frame_len_info);

    pac_db.p_pac_record = (T_LEA_PAC_RECORD *)p_sink_pac_record;
    pac_db.frame_len_info.sample_rate = pac_sr.media_sample_rate;
    pac_db.frame_len_info.frame_len_min.frame_len = media_frame_len_info.frame_len_min.frame_len;
    pac_db.frame_len_info.frame_len_max.frame_len = media_frame_len_info.frame_len_max.frame_len;

    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE && low_latency)
    {
        //not all lea source support this topology
        pac_db.channel_cnt = AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2;
        pac_db.frames_per_sdu = 2;
    }
    else
    {
        pac_db.channel_cnt = AUDIO_CHANNEL_COUNTS_1;
        pac_db.frames_per_sdu = 1;
    }

    pac_db.context = AUDIO_CONTEXT_MEDIA;
    app_lea_pacs_update_pac_record(pac_db);

    pac_db.p_pac_record++;
    pac_db.frame_len_info.sample_rate = pac_sr.convo_downstream_sample_rate;
    pac_db.frame_len_info.frame_len_min.frame_len = convo_ds_frame_len_info.frame_len_min.frame_len;
    pac_db.frame_len_info.frame_len_max.frame_len = convo_ds_frame_len_info.frame_len_max.frame_len;
    pac_db.context = AUDIO_CONTEXT_CONVERSATIONAL;
    app_lea_pacs_update_pac_record(pac_db);
}

static void app_lea_pacs_set_src_pac(bool low_latency, T_LEA_PAC_SAMPLE_RATE pac_sr)
{
    uint8_t *p_source_pac_record = p_app_lea_pac_source_record;
    T_LEA_PAC_FRAME_LEN_INFO convo_us_frame_len_info = {0};
    T_LEA_PAC_DB pac_db = {0};

    *p_source_pac_record = SOURCE_PAC_CNT;//PAC record num for pac_check_record()
    p_source_pac_record++;
    pac_db.p_pac_record = (T_LEA_PAC_RECORD *)p_source_pac_record;

#if TARGET_LE_AUDIO_GAMING
    //le audio gaming dongle use frame duration 10ms
    pac_db.qos = FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_PREFER_10_MS_BIT;
#else
    if (low_latency)
    {
        pac_db.qos = FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT |
                     FRAME_DURATION_PREFER_7_5_MS_BIT;
    }
    else
    {
        pac_db.qos = FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_PREFER_10_MS_BIT;
    }
#endif

    convo_us_frame_len_info.sample_rate = pac_sr.convo_upstream_sample_rate;
    app_lea_pacs_frame_len_setting(app_lea_pac_convo_upstream_codec,
                                   sizeof(app_lea_pac_convo_upstream_codec),
                                   &convo_us_frame_len_info);

    pac_db.frame_len_info.sample_rate = pac_sr.convo_upstream_sample_rate;
    pac_db.frame_len_info.frame_len_min.frame_len = convo_us_frame_len_info.frame_len_min.frame_len;
    pac_db.frame_len_info.frame_len_max.frame_len = convo_us_frame_len_info.frame_len_max.frame_len;

#if (F_APP_GAMING_DONGLE_SUPPORT == 0)
    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE && low_latency)
    {
        //not all lea source support this topology
        pac_db.channel_cnt = AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2;
        pac_db.frames_per_sdu = 2;
    }
    else
#endif
    {
        pac_db.channel_cnt = AUDIO_CHANNEL_COUNTS_1;
        pac_db.frames_per_sdu = 1;
    }

    pac_db.context = AUDIO_CONTEXT_CONVERSATIONAL;
    app_lea_pacs_update_pac_record(pac_db);
}

static void app_lea_pacs_set_pac_record(bool low_latency)
{
    T_LEA_PAC_SAMPLE_RATE pac_sr = {0};

    app_lea_pacs_update_sample_rate(&pac_sr);

    app_lea_pacs_set_sink_pac(low_latency, pac_sr);
    app_lea_pacs_set_src_pac(low_latency, pac_sr);

    APP_PRINT_TRACE1("app_lea_pacs_set_pac_record: low_latency %d", low_latency);
}

bool app_lea_pacs_init_available_context(uint16_t conn_handle)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link)
    {
        p_link->sink_available_contexts = app_lea_pac_sink_available_contexts;
        p_link->source_available_contexts = app_lea_pac_source_available_contexts;
        return true;
    }
    return false;
}

bool app_lea_pacs_set_sink_available_contexts(uint16_t conn_handle, uint16_t sink_contexts,
                                              bool enable)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link)
    {
        uint16_t pre_available_contexts = p_link->sink_available_contexts;

        if (enable)
        {
            p_link->sink_available_contexts |= sink_contexts;
        }
        else
        {
            p_link->sink_available_contexts &= ~sink_contexts;
        }

        if (pre_available_contexts != p_link->sink_available_contexts)
        {
            APP_PRINT_TRACE3("app_lea_pacs_set_sink_available_contexts: avail 0x%02X, set 0x%02X, enable %d",
                             p_link->sink_available_contexts, sink_contexts, enable);

            return pacs_update_available_contexts(conn_handle, p_link->sink_available_contexts,
                                                  p_link->source_available_contexts);
        }
    }
    return false;
}

bool app_lea_pacs_set_source_available_contexts(uint16_t conn_handle, uint16_t source_contexts,
                                                bool enable)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link)
    {
        if (enable)
        {
            p_link->source_available_contexts |= source_contexts;
        }
        else
        {
            p_link->source_available_contexts &= ~source_contexts;
        }

        APP_PRINT_TRACE3("app_lea_pacs_set_source_available_contexts: avail 0x%02X, set 0x%02X, enable %d",
                         p_link->source_available_contexts, source_contexts, enable);

        return pacs_update_available_contexts(conn_handle, p_link->sink_available_contexts,
                                              p_link->source_available_contexts);
    }
    return false;
}

uint16_t app_lea_pacs_get_sink_available_contexts(void)
{
    return app_lea_pac_sink_available_contexts;
}

uint16_t app_lea_pacs_get_source_available_contexts(void)
{
    return app_lea_pac_source_available_contexts;
}

void app_lea_pacs_update_low_latency(bool low_latency)
{
    app_lea_pacs_set_pac_record(low_latency);
    pacs_pac_update(app_lea_pac_sink_pac_id, (const uint8_t *)p_app_lea_pac_sink_record,
                    app_lea_pac_sink_record_size);

    pacs_pac_update(app_lea_pac_source_pac_id, (const uint8_t *)p_app_lea_pac_source_record,
                    app_lea_pac_source_record_size);
}

void app_lea_pacs_init(void)
{
    T_PACS_PARAMS pacs_params = {0};
    uint32_t sink_loc = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;
    uint32_t source_loc = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;

    if (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SINGLE)
    {
        if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
        {
            sink_loc = AUDIO_LOCATION_FL;
            source_loc = AUDIO_LOCATION_FL;
        }
        else
        {
            sink_loc = AUDIO_LOCATION_FR;
            source_loc = AUDIO_LOCATION_FR;
        }
    }

    app_lea_pac_sink_record_size = (SINK_PAC_CNT * sizeof(T_LEA_PAC_RECORD)) + 1;
    app_lea_pac_source_record_size = (SOURCE_PAC_CNT * sizeof(T_LEA_PAC_RECORD))  + 1;
    p_app_lea_pac_sink_record = os_mem_zalloc(RAM_TYPE_DATA_ON, app_lea_pac_sink_record_size);
    p_app_lea_pac_source_record = os_mem_zalloc(RAM_TYPE_DATA_ON, app_lea_pac_source_record_size);

    pacs_params.sink_locations.is_exist = true;
    pacs_params.sink_locations.is_notify = true;
    pacs_params.sink_locations.is_write = false;
    pacs_params.sink_locations.sink_audio_location = sink_loc;

    pacs_params.source_locations.is_exist = true;
    pacs_params.source_locations.is_notify = true;
    pacs_params.source_locations.is_write = false;
    pacs_params.source_locations.source_audio_location = source_loc;

    pacs_params.supported_contexts.is_notify = true;
    pacs_params.supported_contexts.sink_supported_contexts = app_lea_pac_sink_supported_contexts;
    pacs_params.supported_contexts.source_supported_contexts = app_lea_pac_source_supported_contexts;

#if F_APP_GAMING_DONGLE_SUPPORT && F_APP_LEA_SUPPORT
    app_lea_pacs_set_pac_record(true);
#else
    app_lea_pacs_set_pac_record(app_cfg_const.enable_enter_gaming_mode_after_power_on);
#endif

    app_lea_pac_sink_pac_id = pacs_pac_add(SERVER_AUDIO_SINK,
                                           (const uint8_t *)p_app_lea_pac_sink_record,
                                           app_lea_pac_sink_record_size, true);
    app_lea_pac_source_pac_id = pacs_pac_add(SERVER_AUDIO_SOURCE,
                                             (const uint8_t *)p_app_lea_pac_source_record,
                                             app_lea_pac_source_record_size, true);
    pacs_init(&pacs_params);
    ble_audio_cback_register(app_lea_pacs_ble_audio_cback);
}
#endif
