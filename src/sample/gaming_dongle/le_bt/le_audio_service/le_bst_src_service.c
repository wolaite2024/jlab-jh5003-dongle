#if LE_AUDIO_BROADCAST_SOURCE_ROLE
#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "ble_audio.h"
#include "base_data_generate.h"
#include "metadata_def.h"
#include "le_bst_src_service.h"
#include "codec_def.h"
#include "vector.h"
#include "os_queue.h"
#include "app_usb_layer.h"
#include "ble_isoch_def.h"
#include "bap.h"
#include "le_broadcast_mgr.h"
#include "mcp_mgr.h"
#include "dev_mgr.h"
#include "gap_conn_le.h"
#include "le_audio_data.h"
#include "le_media_player.h"
#include "ual_adapter.h"
#include "app_le_ext_ftl.h"
#include "le_ba_service.h"

typedef struct t_bs_dev_cb
{
    struct t_bs_dev_cb        *p_next;
    uint8_t                   sync_by_bass;
    uint8_t                   instance_id;
    uint8_t                   source_id;
    uint8_t                   addr[BD_ADDR_LEN];
    uint32_t                  chnl_location;
} T_BS_DEV_CB;


typedef struct
{
    T_BROADCAST_SOURCE_HANDLE
    broadcast_handle;   //FIX TODO For now it will not release since it has been created
    uint8_t                   group1_bis_num;
    uint8_t                   group1_idx;
    uint8_t                   group1_subgroup1_idx;
    uint8_t                   group1_bis1_idx;
    uint8_t                   group1_bis2_idx;
    uint8_t                   group1_bis3_idx;
    T_CODEC_CFG_ITEM          stream_cfg_index;
    uint8_t                   channel_num;
    uint16_t                  max_sdu;
    uint32_t                  sdu_interval;
    T_CODEC_CFG               codec_cfg;
    T_QOS_CFG_PREFERRED       prefer_qos;

    T_BLE_BD_TYPE             local_bd_type;
    uint8_t                  rand_addr[GAP_BD_ADDR_LEN];
    T_OS_QUEUE               snk_grp_queue;

    T_BROADCAST_SOURCE_STATE state;
    T_QOS_CFG_TYPE           qos_type;
    uint8_t                  encrypted;
    bool                     need_release;
    uint8_t                  broadcast_code[BROADCAST_CODE_LEN];
    uint8_t                  broadcast_id[BROADCAST_ID_LEN];
    uint8_t                  adv_sid;
    uint32_t                 ea_int_min;
    uint32_t                 ea_int_max;
    uint32_t                 pa_int_min;
    uint32_t                 pa_int_max;
    uint32_t                 pd_delay;
    uint8_t                  phy;
    uint8_t                  packing;
    uint8_t                  framing;
    bool                     test_mode;     /* The big test mode flag, only test_mode is true
                                            the value of iso_interval, nse, max_pdu, bn, irc, pto
                                            are valid*/
    uint16_t                 iso_interval; /**< The time between consecutive BIG anchor points.
                                            Range: 0x0004 to 0x0C80
                                            Time = N * 1.25 ms
                                            Time Range: 5 ms to 4 s. */
    uint8_t                  nse;           /**< The number of subevents in each BIS event in the BIG.
                                            Range: 0x01 to 0x1F */
    uint16_t                 max_pdu;      /**< Maximum size, in octets, of the payload.
                                            Range: 0x0001 to 0x00FB*/
    uint8_t
    bn;            /**< The number of new payloads in each interval for each BIS.
                                            Range: 0x01 to 0x07 */
    uint8_t
    irc;           /**< The number of times the scheduled payloads are transmitted in a given event.
                                            Range: 0x01 to 0x0F */
    uint8_t                  pto;           /**< Offset used for pre-transmissions.
                                            Range: 0x00 to 0x0F */
} T_LE_BS_DB;

static T_LE_BS_DB *le_bs_db;

static uint8_t media_codec_level2[] =
{
    0x02,
    CODEC_CFG_TYPE_SAMPLING_FREQUENCY,
    SAMPLING_FREQUENCY_CFG_48K,
    0x02,
    CODEC_CFG_TYPE_FRAME_DURATION,
    FRAME_DURATION_CFG_10_MS,
    0x03,
    CODEC_CFG_TYPE_OCTET_PER_CODEC_FRAME,
    0x64, 0x00,
    0x02,
    CODEC_CFG_TYPE_BLOCKS_PER_SDU,
    1
};

static const uint8_t media_codec_level3_bisl[] =
{
    0x05,
    CODEC_CFG_TYPE_AUDIO_CHANNEL_ALLOCATION,
    (uint8_t)AUDIO_LOCATION_FL,
    (uint8_t)((AUDIO_LOCATION_FL >> 8) & 0xFF),
    (uint8_t)((AUDIO_LOCATION_FL >> 16) & 0xFF),
    (uint8_t)((AUDIO_LOCATION_FL >> 24) & 0xFF),
};

static const uint8_t media_codec_level3_bisr[] =
{
    0x05,
    CODEC_CFG_TYPE_AUDIO_CHANNEL_ALLOCATION,
    (uint8_t)AUDIO_LOCATION_FR,
    (uint8_t)((AUDIO_LOCATION_FR >> 8) & 0xFF),
    (uint8_t)((AUDIO_LOCATION_FR >> 16) & 0xFF),
    (uint8_t)((AUDIO_LOCATION_FR >> 24) & 0xFF),

};

static const uint8_t media_codec_level3_bislr[] =
{
    0x05,
    CODEC_CFG_TYPE_AUDIO_CHANNEL_ALLOCATION,
    (uint8_t)(AUDIO_LOCATION_FR | AUDIO_LOCATION_FL),
    (uint8_t)(((AUDIO_LOCATION_FR | AUDIO_LOCATION_FL) >> 8) & 0xFF),
    (uint8_t)(((AUDIO_LOCATION_FR | AUDIO_LOCATION_FL) >> 16) & 0xFF),
    (uint8_t)(((AUDIO_LOCATION_FR | AUDIO_LOCATION_FL) >> 24) & 0xFF),
};

#if (LE_AUDIO_PACS_CLIENT_SUPPORT & LE_AUDIO_BASS_CLIENT_SUPPORT)
static void bs_reset_all_snk_sync_by_bass(void);
#endif

static bool find_prefer_lc3_cfg_idx(uint16_t sup_table_mask, uint8_t duration,
                                    uint16_t min_frame_size, uint8_t *p_index)
{
    uint8_t index = 0;
    T_CODEC_CFG cfg;

    if (p_index == NULL)
    {
        return false;
    }
    if (duration == FRAME_DURATION_CFG_10_MS)
    {
        sup_table_mask &= FRAME_DURATION_10M_TABLE_MASK;
    }
    else
    {
        sup_table_mask &= FRAME_DURATION_7_5M_TABLE_MASK;
    }
    *p_index = 0xFF;

    if (sup_table_mask == 0)
    {
        return false;
    }

    while (sup_table_mask)
    {

        if (sup_table_mask & 0x01)
        {
            if (codec_preferred_cfg_get((T_CODEC_CFG_ITEM)index, &cfg))
            {
                if (min_frame_size <= cfg.octets_per_codec_frame)
                {
                    *p_index = index;
                    return true;
                }
            }
        }
        sup_table_mask >>= 1;
        index++;
    }

    return false;
}

static T_BS_DEV_CB *bs_find_added_dev_cb(uint8_t *addr)
{
    int i;
    T_BS_DEV_CB *bs_dev_cb = NULL;
    for (i = 0; i < le_bs_db->snk_grp_queue.count; i++)
    {
        bs_dev_cb = (T_BS_DEV_CB *)os_queue_peek(&le_bs_db->snk_grp_queue, i);
        if (memcmp(bs_dev_cb->addr, addr, BD_ADDR_LEN) == 0)
        {
            return bs_dev_cb;
        }
    }
    return NULL;

}

static void bs_free_basic_data(void)
{
    if (le_bs_db->group1_idx != 0xFF)
    {
        base_data_del_group(le_bs_db->group1_idx);
        le_bs_db->group1_idx = 0xFF;
    }
}

static void bs_gen_basic_data(void)
{
    uint8_t error_idx = 0;
    uint8_t codec_id2[CODEC_ID_LEN] = {LC3_CODEC_ID, 0, 0, 0, 0};
    uint8_t ccid = 0;
#if LE_AUDIO_MCP_SERVER_SUPPORT
    ccid = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GMCS;
#endif
    uint8_t media_metadata[] =
    {
        0x03,
        METADATA_TYPE_STREAMING_AUDIO_CONTEXTS,
        (uint8_t)(AUDIO_CONTEXT_MEDIA),
        (uint8_t)(AUDIO_CONTEXT_MEDIA >> 8),
        0x02,
        METADATA_TYPE_CCID_LIST,
        ccid
    };
    bs_free_basic_data();

    media_codec_level2[2] = le_bs_db->codec_cfg.sample_frequency;
    media_codec_level2[5] = le_bs_db->codec_cfg.frame_duration;
    memcpy(&media_codec_level2[8], &le_bs_db->codec_cfg.octets_per_codec_frame, 2);
    media_codec_level2[12] = le_bs_db->codec_cfg.codec_frame_blocks_per_sdu;
#if UAL_LATENCY_TEST
    APP_PRINT_INFO3("bs_gen_basic_data pd_delay %x, rtn %x tl %x",
                    le_bs_db->prefer_qos.presentation_delay,
                    le_bs_db->prefer_qos.retransmission_number, le_bs_db->prefer_qos.max_transport_latency);
    le_bs_db->prefer_qos.presentation_delay = 5000;
    le_bs_db->prefer_qos.retransmission_number = 2;
    le_bs_db->prefer_qos.max_transport_latency = 8;
    APP_PRINT_INFO3("bs_gen_basic_data pd_delay %x, rtn %x tl %x",
                    le_bs_db->prefer_qos.presentation_delay,
                    le_bs_db->prefer_qos.retransmission_number, le_bs_db->prefer_qos.max_transport_latency);
#endif
    if (base_data_add_group(&le_bs_db->group1_idx, le_bs_db->prefer_qos.presentation_delay) == false)
    {
        error_idx = 1;
        le_bs_db->group1_idx = 0xFF;
        goto error;
    }
    if (base_data_add_subgroup(&le_bs_db->group1_subgroup1_idx, le_bs_db->group1_idx, codec_id2,
                               sizeof(media_codec_level2), media_codec_level2, sizeof(media_metadata),
                               (uint8_t *)media_metadata) == false)
    {
        error_idx = 2;
        le_bs_db->group1_subgroup1_idx = 0xFF;
        goto error;
    }

    if (app_cfg_const.dongle_bis_num == 0)
    {
        if (base_data_add_bis(&le_bs_db->group1_bis1_idx, le_bs_db->group1_idx,
                              le_bs_db->group1_subgroup1_idx,
                              sizeof(media_codec_level3_bislr), (uint8_t *)media_codec_level3_bislr) == false)
        {
            error_idx = 3;
            goto error;
        }
#if (LEA_BIS_DUAL_UAC_SUPPORT == 1)
        if (base_data_add_bis(&le_bs_db->group1_bis1_idx, le_bs_db->group1_idx,
                              le_bs_db->group1_subgroup1_idx,
                              sizeof(media_codec_level3_bislr), (uint8_t *)media_codec_level3_bislr) == false)
        {
            error_idx = 3;
            goto error;
        }
#endif
    }
    else if (app_cfg_const.dongle_bis_num == 1)
    {
        if (base_data_add_bis(&le_bs_db->group1_bis1_idx, le_bs_db->group1_idx,
                              le_bs_db->group1_subgroup1_idx,
                              sizeof(media_codec_level3_bisl), (uint8_t *)media_codec_level3_bisl) == false)
        {
            error_idx = 3;
            goto error;
        }

        if (base_data_add_bis(&le_bs_db->group1_bis2_idx, le_bs_db->group1_idx,
                              le_bs_db->group1_subgroup1_idx,
                              sizeof(media_codec_level3_bisr), (uint8_t *)media_codec_level3_bisr) == false)
        {
            error_idx = 4;
            goto error;
        }
#if (LEA_BIS_DUAL_UAC_SUPPORT == 1)
        if (base_data_add_bis(&le_bs_db->group1_bis1_idx, le_bs_db->group1_idx,
                              le_bs_db->group1_subgroup1_idx,
                              sizeof(media_codec_level3_bisl), (uint8_t *)media_codec_level3_bisl) == false)
        {
            error_idx = 3;
            goto error;
        }

        if (base_data_add_bis(&le_bs_db->group1_bis2_idx, le_bs_db->group1_idx,
                              le_bs_db->group1_subgroup1_idx,
                              sizeof(media_codec_level3_bisr), (uint8_t *)media_codec_level3_bisr) == false)
        {
            error_idx = 4;
            goto error;
        }
#endif
    }
    else if (app_cfg_const.dongle_bis_num == 2)
    {
        if (base_data_add_bis(&le_bs_db->group1_bis1_idx, le_bs_db->group1_idx,
                              le_bs_db->group1_subgroup1_idx,
                              sizeof(media_codec_level3_bisl), (uint8_t *)media_codec_level3_bisl) == false)
        {
            error_idx = 3;
            goto error;
        }

        if (base_data_add_bis(&le_bs_db->group1_bis2_idx, le_bs_db->group1_idx,
                              le_bs_db->group1_subgroup1_idx,
                              sizeof(media_codec_level3_bisr), (uint8_t *)media_codec_level3_bisr) == false)
        {
            error_idx = 4;
            goto error;
        }
        if (base_data_add_bis(&le_bs_db->group1_bis3_idx, le_bs_db->group1_idx,
                              le_bs_db->group1_subgroup1_idx,
                              sizeof(media_codec_level3_bislr), (uint8_t *)media_codec_level3_bislr) == false)
        {
            error_idx = 5;
            goto error;
        }
    }
    base_data_get_bis_num(le_bs_db->group1_idx, &le_bs_db->group1_bis_num);
    return;
error:
    APP_PRINT_ERROR1("bs_gen_basic_data: failed, error_idx %d", error_idx);
}


void bs_handle_setup_data_path(T_BROADCAST_SOURCE_SETUP_DATA_PATH *p_data)
{
    T_BIG_SETUP_DATA_PATH data_path = {0};
    data_path.path_direction = DATA_PATH_INPUT_FLAG;
    data_path.bis_conn_handle = p_data->bis_conn_handle;
    data_path.codec_parsed_data = le_bs_db->codec_cfg;

    if (app_cfg_const.dongle_bis_num == 0)
    {
        if (p_data->bis_idx == BRS_AUDIO_CHAN_1_BIS_NUM)
        {
            data_path.codec_parsed_data.audio_channel_allocation = AUDIO_LOCATION_FR | AUDIO_LOCATION_FL;
        }
#if (LEA_BIS_DUAL_UAC_SUPPORT == 1)
        else if (p_data->bis_idx == BRS_AUDIO_CHAN_2_BIS_NUM)
        {
            data_path.codec_parsed_data.audio_channel_allocation = AUDIO_LOCATION_FR | AUDIO_LOCATION_FL;
        }
#endif
    }
    else if (app_cfg_const.dongle_bis_num == 1)
    {
        if (p_data->bis_idx == BRS_AUDIO_CHAN_1_BIS_NUM)
        {
            data_path.codec_parsed_data.audio_channel_allocation = AUDIO_LOCATION_FL;
        }
        else if (p_data->bis_idx == BRS_AUDIO_CHAN_2_BIS_NUM)
        {
            data_path.codec_parsed_data.audio_channel_allocation = AUDIO_LOCATION_FR;
        }
#if (LEA_BIS_DUAL_UAC_SUPPORT == 1)
        else if (p_data->bis_idx == BRS_AUDIO_CHAN_3_BIS_NUM)
        {
            data_path.codec_parsed_data.audio_channel_allocation = AUDIO_LOCATION_FL;
        }
        else if (p_data->bis_idx == BRS_AUDIO_CHAN_4_BIS_NUM)
        {
            data_path.codec_parsed_data.audio_channel_allocation = AUDIO_LOCATION_FR;
        }
#endif
    }
    else if (app_cfg_const.dongle_bis_num == 2)
    {
        if (p_data->bis_idx == BRS_AUDIO_CHAN_1_BIS_NUM)
        {
            data_path.codec_parsed_data.audio_channel_allocation = AUDIO_LOCATION_FL;
        }
        else if (p_data->bis_idx == BRS_AUDIO_CHAN_2_BIS_NUM)
        {
            data_path.codec_parsed_data.audio_channel_allocation = AUDIO_LOCATION_FR;
        }
        else if (p_data->bis_idx == BRS_AUDIO_CHAN_3_BIS_NUM)
        {
            data_path.codec_parsed_data.audio_channel_allocation = AUDIO_LOCATION_FR | AUDIO_LOCATION_FL;
        }
    }
    handle_bis_data_path_setup_complt_msg(&data_path);
}

void bs_handle_rmv_data_path(T_BROADCAST_SOURCE_REMOVE_DATA_PATH *p_data)
{
    T_BIG_REMOVE_DATA_PATH rmv_data;
    rmv_data.path_direction = DATA_PATH_INPUT_FLAG;
    rmv_data.bis_conn_handle = p_data->bis_conn_handle;
    handle_bis_data_path_remove_complt_msg(&rmv_data);
}

static void bs_handle_state_configured(T_BROADCAST_SOURCE_STATE old_state)
{
    APP_PRINT_INFO2("bs_handle_state_configured: old_state 0x%x, need_release 0x%x", old_state,
                    le_bs_db->need_release);
    if (old_state == BROADCAST_SOURCE_STATE_CONFIGURED_STARTING)
    {
        bs_establish(le_bs_db->test_mode);
    }

    if (le_bs_db->need_release)
    {
        bs_release();
        le_bs_db->need_release = false;
    }
}

void bs_handle_state_change(T_BROADCAST_SOURCE_STATE state)
{
    if (le_bs_db == NULL)
    {
        return;
    }

    APP_PRINT_INFO2("bs_handle_state_change: old_state 0x%x, new_state 0x%x", le_bs_db->state, state);

    switch (state)
    {
    case BROADCAST_SOURCE_STATE_IDLE:
        break;

    case BROADCAST_SOURCE_STATE_CONFIGURED_STARTING:
        break;

    case BROADCAST_SOURCE_STATE_CONFIGURED:
        {
            bs_handle_state_configured(le_bs_db->state);
        }
        break;

    case BROADCAST_SOURCE_STATE_CONFIGURED_STOPPING:
        break;

    case BROADCAST_SOURCE_STATE_STREAMING_STARTING:
        break;

    case BROADCAST_SOURCE_STATE_STREAMING:
        {
            ble_audio_set_broadcast_mode(true, AUDIO_ROLE_SOURCE);
        }
        break;

    case BROADCAST_SOURCE_STATE_STREAMING_STOPPING:
        {
            ble_audio_set_broadcast_mode(false, AUDIO_ROLE_UNKNOWN);
        }
        break;
    default:
        break;
    }

    le_bs_db->state = state;
}

uint8_t bs_get_play_info(uint8_t *p_qos_type)
{
    if (p_qos_type)
    {
        *p_qos_type = le_bs_db->qos_type;
    }
    return (le_bs_db->state == BROADCAST_SOURCE_STATE_STREAMING_STARTING ||
            le_bs_db->state == BROADCAST_SOURCE_STATE_STREAMING) ? 0x01 : 0x00;

}

bool bs_set_track_codec(uint8_t index, uint8_t qos_type)
{
    T_QOS_CFG_TYPE qos = (T_QOS_CFG_TYPE)(qos_type + 1);
    if (le_bs_db == NULL)
    {
        APP_PRINT_ERROR0("bs_set_track_codec alloc fail");
        return false;
    }
    if (le_bs_db->state != BROADCAST_SOURCE_STATE_IDLE)
    {
        APP_PRINT_ERROR1("bs_set_track_codec state wrong %d", le_bs_db->state);
        return false;
    }
    if (index >= CODEC_CFG_ITEM_LC3_MAX ||
        ((qos_type + 1) != QOS_CFG_BIS_LOW_LATENCY && (qos_type + 1) != QOS_CFG_BIS_HIG_RELIABILITY))
    {
        return false;
    }
    bs_free_basic_data();

    le_bs_db->stream_cfg_index = (T_CODEC_CFG_ITEM)index;
    codec_preferred_cfg_get((T_CODEC_CFG_ITEM)index, &le_bs_db->codec_cfg);
    codec_sdu_interval_get(&le_bs_db->codec_cfg, &le_bs_db->sdu_interval);
    codec_max_sdu_len_get(&le_bs_db->codec_cfg, &le_bs_db->max_sdu);

    qos_preferred_cfg_get((T_CODEC_CFG_ITEM)index, qos, &le_bs_db->prefer_qos);
    le_bs_db->qos_type = qos;

    bs_gen_basic_data();
    return true;
}

bool bs_config(T_BLE_BD_TYPE local_addr_type)
{
    APP_PRINT_INFO0("bs_config");
    T_BROADCAST_SOURCE_INFO src_info;
    uint8_t bis_invalid_id[BROADCAST_ID_LEN] = {0x0};
    if (le_bs_db == NULL)
    {
        return false;
    }

    if (le_bs_db->broadcast_handle == NULL)
    {
        le_bs_db->broadcast_handle = bap_bsrc_create_handle();
        if (memcmp(le_bs_db->broadcast_id, bis_invalid_id, BROADCAST_ID_LEN))
        {
            broadcast_source_update_broadcast_id(le_bs_db->broadcast_handle, le_bs_db->broadcast_id);
        }

        if (!broadcast_source_get_info(le_bs_db->broadcast_handle, &src_info))
        {
            APP_PRINT_ERROR0("bs_config, get info fail");
            return false;
        }
        memcpy(le_bs_db->broadcast_id, src_info.broadcast_id, BROADCAST_ID_LEN);
        APP_PRINT_INFO1("bs_config, broadcast_id %b", TRACE_BINARY(3, src_info.broadcast_id));

        lea_ext_save_broadcast_id(le_bs_db->broadcast_id);
    }

    if (bap_bsrc_config(local_addr_type, le_bs_db->adv_sid, le_bs_db->group1_idx,
                        le_bs_db->broadcast_handle,
                        le_bs_db->ea_int_min, le_bs_db->ea_int_max,
                        le_bs_db->pa_int_min, le_bs_db->pa_int_max,
                        le_bs_db->qos_type, le_bs_db->encrypted))
    {
        if (local_addr_type == BLE_ADDR_PUBLIC)
        {
            gap_get_param(GAP_PARAM_BD_ADDR, le_bs_db->rand_addr);
        }
        else
        {
            bap_bsrc_get_random_address(le_bs_db->group1_idx, le_bs_db->rand_addr);
        }
        return true;
    }

    return false;
}

bool bs_reconfig(uint16_t metadata)
{
    uint8_t media_metadata_update[] =
    {
        0x03,
        METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS,
        (uint8_t)(metadata),
        (uint8_t)(metadata >> 8),
    };

    if (le_bs_db == NULL)
    {
        return false;
    }

    return bap_bsrc_reconfig(le_bs_db->group1_idx, le_bs_db->group1_subgroup1_idx,
                             sizeof(media_metadata_update),
                             (uint8_t *)media_metadata_update);
}

bool bs_metaupdate(uint16_t metadata)
{
    uint8_t media_metadata_update[] =
    {
        0x03,
        METADATA_TYPE_STREAMING_AUDIO_CONTEXTS,
        (uint8_t)(metadata),
        (uint8_t)(metadata >> 8),
    };

    if (le_bs_db == NULL)
    {
        return false;
    }
    return bap_bsrc_metaupdate(le_bs_db->group1_idx, le_bs_db->group1_subgroup1_idx,
                               sizeof(media_metadata_update),
                               (uint8_t *)media_metadata_update);
}

bool bs_establish(bool test_mode)
{
    if (le_bs_db == NULL)
    {
        return false;
    }

    uint16_t max_sdu = le_bs_db->max_sdu;
    if ((app_cfg_const.dongle_bis_num == 2) || (app_cfg_const.dongle_bis_num == 0))
    {
        max_sdu = le_bs_db->max_sdu * 2;
    }

    if (test_mode == false)
    {
        return bap_bsrc_establish(le_bs_db->group1_idx, le_bs_db->prefer_qos.retransmission_number,
                                  le_bs_db->sdu_interval, max_sdu, le_bs_db->phy,
                                  le_bs_db->packing, le_bs_db->framing,
                                  le_bs_db->prefer_qos.max_transport_latency, le_bs_db->broadcast_code);
    }
    else
    {
        return bap_bsrc_test_establish(le_bs_db->group1_idx, le_bs_db->prefer_qos.retransmission_number,
                                       le_bs_db->sdu_interval, le_bs_db->iso_interval, le_bs_db->nse, max_sdu,
                                       le_bs_db->max_pdu, le_bs_db->phy, le_bs_db->packing,
                                       le_bs_db->framing, le_bs_db->bn, le_bs_db->irc,
                                       le_bs_db->pto, le_bs_db->broadcast_code);
    }

}

bool bs_disable(void)
{
    if (le_bs_db == NULL)
    {
        return false;
    }
    return bap_bsrc_disable(le_bs_db->group1_idx, HCI_ERR_LOCAL_HOST_TERMINATE);
}


bool bs_start_broadcast(uint8_t *code)
{
    APP_PRINT_INFO0("bs_start_broadcast");
    uint8_t broadcast_invalid_code[BROADCAST_CODE_LEN] = {0x0};

    bs_gen_basic_data();

    le_bs_db->encrypted = false;
    if (code != NULL)
    {
        APP_PRINT_INFO1("bs_start_broadcast code %b", TRACE_BINARY(16, code));

        if (memcmp(code, broadcast_invalid_code, BROADCAST_CODE_LEN))
        {
            le_bs_db->encrypted = true;
            if (memcmp(code, le_bs_db->broadcast_code, BROADCAST_CODE_LEN))
            {
                APP_PRINT_INFO1("bs_start_broadcast: change code %b", TRACE_BINARY(16, code));
                bs_reset_all_snk_sync_by_bass();
            }
        }
        memcpy(le_bs_db->broadcast_code, code, BROADCAST_CODE_LEN);
    }
    else
    {
        memset(le_bs_db->broadcast_code, 0, BROADCAST_CODE_LEN);
    }

    if (le_bs_db)
    {
        APP_PRINT_INFO1("bs_start_broadcast, le_bs_db->state 0x%x", le_bs_db->state);
        if (le_bs_db->state == BROADCAST_SOURCE_STATE_IDLE)
        {
            return bs_config(BLE_ADDR_PUBLIC);
        }
        else if (le_bs_db->state == BROADCAST_SOURCE_STATE_CONFIGURED)
        {
            return bs_establish(le_bs_db->test_mode);
        }
    }
    return false;
}

bool bs_stop_broadcast(void)
{
    APP_PRINT_INFO1("bs_stop_broadcast, le_bs_db->state 0x%x", le_bs_db->state);
    return bs_disable();
}

void bs_release_broadcast(void)
{
    APP_PRINT_INFO1("bs_release_broadcast, le_bs_db->state 0x%x", le_bs_db->state);
    if (le_bs_db->state == BROADCAST_SOURCE_STATE_CONFIGURED)
    {
        bs_release();
    }
    else
    {
        le_bs_db->need_release = true;
        bs_disable();
    }
}


#if (LE_AUDIO_PACS_CLIENT_SUPPORT & LE_AUDIO_BASS_CLIENT_SUPPORT)
void bs_reset_all_snk_sync_by_bass(void)
{
    T_BS_DEV_CB *bs_dev_cb = NULL;
    for (uint8_t i = 0; i < le_bs_db->snk_grp_queue.count; i++)
    {
        bs_dev_cb = (T_BS_DEV_CB *)os_queue_peek(&le_bs_db->snk_grp_queue, i);
        if (!bs_dev_cb)
        {
            continue;
        }
        bs_dev_cb->sync_by_bass = false;
    }
}

bool bs_rmv_snk_from_group(uint8_t *addr, bool remove_src)
{
    int i;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(addr);
    if (p_dev_rec == NULL)
    {
        APP_PRINT_INFO1("bs_rmv_snk_from_group addr %b", TRACE_BDADDR(addr));
        return false;
    }
    T_BS_DEV_CB *bs_dev_cb = NULL;
    bool found = false;
    for (i = 0; i < le_bs_db->snk_grp_queue.count; i++)
    {
        bs_dev_cb = (T_BS_DEV_CB *)os_queue_peek(&le_bs_db->snk_grp_queue, i);
        if (bs_dev_cb && memcmp(bs_dev_cb->addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN) == 0)
        {
            os_queue_delete(&le_bs_db->snk_grp_queue, bs_dev_cb);
            found = true;
            break;
        }
    }
    if (found)
    {
        if (remove_src)
        {
            if (bs_dev_cb->instance_id != 0xFF)
            {
                bap_ba_remove_src(p_dev_rec->le_conn_handle, bs_dev_cb->source_id);
            }
        }
        free(bs_dev_cb);
    }
    return true;
}

bool bs_add_snk_to_group(uint8_t *addr)
{
    T_LE_AUDIO *p_le_audio = NULL;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(addr);
    if (p_dev_rec == NULL)
    {
        return false;
    }
    p_le_audio = ble_audio_find_by_conn_handle(p_dev_rec->le_conn_handle);

    if (p_le_audio == NULL)
    {
        return false;
    }
    if ((p_le_audio->snk_avail_context & AUDIO_CONTEXT_MEDIA) == 0)
    {
        APP_PRINT_ERROR1("app_bs_add_snk_to_group: context mismatch : 0x%x", p_le_audio->snk_avail_context);
        return false;
    }

    T_BS_DEV_CB *bs_dev_cb = bs_find_added_dev_cb(p_dev_rec->pseudo_addr);

    if (bs_dev_cb == NULL)
    {
        bs_dev_cb = calloc(1, sizeof(T_BS_DEV_CB));
        if (bs_dev_cb == NULL)
        {
            APP_PRINT_ERROR0("app_bs_add_snk_to_group: alloc fail");
            return false;
        }
        bs_dev_cb->instance_id = 0xFF;
        memcpy(bs_dev_cb->addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
        os_queue_in(&le_bs_db->snk_grp_queue, (void *) bs_dev_cb);
    }
    return true;
}

bool bs_set_device_conf(uint8_t *addr, uint32_t chnl_location)
{
    T_LE_AUDIO *p_le_audio = NULL;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(addr);

    uint16_t sample_sup_idx;
    uint16_t support_table_mask;
    uint8_t cfg_idx;
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("bs_set_device_conf: device not connected!");
        return false;
    }

    p_le_audio = ble_audio_find_by_conn_handle(p_dev_rec->le_conn_handle);

    if (le_bs_db->codec_cfg.sample_frequency == SAMPLING_FREQUENCY_CFG_8K)
    {
        sample_sup_idx = SAMPLE_FREQ_8K_TABLE_MASK;
    }
    if (le_bs_db->codec_cfg.sample_frequency == SAMPLING_FREQUENCY_CFG_16K)
    {
        sample_sup_idx = SAMPLE_FREQ_16K_TABLE_MASK;
    }
    if (le_bs_db->codec_cfg.sample_frequency == SAMPLING_FREQUENCY_CFG_24K)
    {
        sample_sup_idx = SAMPLE_FREQ_24K_TABLE_MASK;
    }
    if (le_bs_db->codec_cfg.sample_frequency == SAMPLING_FREQUENCY_CFG_32K)
    {
        sample_sup_idx = SAMPLE_FREQ_32K_TABLE_MASK;
    }
    if (le_bs_db->codec_cfg.sample_frequency == SAMPLING_FREQUENCY_CFG_44_1K)
    {
        sample_sup_idx = SAMPLE_FREQ_441K_TABLE_MASK;
    }
    else if (le_bs_db->codec_cfg.sample_frequency == SAMPLING_FREQUENCY_CFG_48K)
    {
        sample_sup_idx = SAMPLE_FREQ_48K_TABLE_MASK;
    }

    if (p_le_audio == NULL)
    {
        APP_PRINT_ERROR0("bs_set_device_conf: p_le_audio is NULL");
        return false;
    }


    if (p_le_audio->snk_avail_context & AUDIO_CONTEXT_MEDIA == 0)
    {
        APP_PRINT_ERROR1("bs_set_device_conf: context mismatch : 0x%x", p_le_audio->snk_avail_context);
        return false;
    }

    T_BS_DEV_CB *bs_dev_cb = bs_find_added_dev_cb(addr);
    if (bs_dev_cb == NULL)
    {
        APP_PRINT_ERROR0("bs_set_device_conf: can not find device");
        return false;
    }
    bs_dev_cb->chnl_location = chnl_location;

    if (p_le_audio->snk_pac_num == 0)
    {
        return false;
    }
    uint8_t chnl_cnt;
    if (chnl_location == AUDIO_LOCATION_MONO)
    {
        chnl_cnt = 1;
    }
    else
    {
        chnl_cnt = __builtin_popcount(chnl_location);
    }
    support_table_mask = bap_pacs_get_lc3_snk_table_msk(p_dev_rec->le_conn_handle, AUDIO_CONTEXT_MEDIA,
                                                        chnl_cnt, 1);
    if (!support_table_mask)
    {
        support_table_mask = bap_pacs_get_lc3_snk_table_msk(p_dev_rec->le_conn_handle,
                                                            AUDIO_CONTEXT_UNSPECIFIED,
                                                            chnl_cnt, 1);
    }
    if ((sample_sup_idx & support_table_mask) == 0)
    {
        return false;
    }

    if (le_bs_db->codec_cfg.frame_duration == FRAME_DURATION_CFG_7_5_MS)
    {
        if ((support_table_mask & FRAME_DURATION_7_5M_TABLE_MASK) == 0)
        {
            return false;
        }
        if (!find_prefer_lc3_cfg_idx((sample_sup_idx & support_table_mask), FRAME_DURATION_CFG_7_5_MS,
                                     le_bs_db->codec_cfg.octets_per_codec_frame, &cfg_idx))
        {
            return false;
        }
    }
    else
    {
        if ((support_table_mask & FRAME_DURATION_10M_TABLE_MASK) == 0)
        {
            return false;
        }
        if (!find_prefer_lc3_cfg_idx((sample_sup_idx & support_table_mask), FRAME_DURATION_CFG_10_MS,
                                     le_bs_db->codec_cfg.octets_per_codec_frame, &cfg_idx))
        {
            return false;
        }
    }
    return true;

}

bool bs_start_sync(uint8_t *addr, uint8_t past, uint8_t prefer)
{
    APP_PRINT_INFO3("bs_start_sync, addr %s, past 0x%x, prefer 0x%x", TRACE_BDADDR(addr), past, prefer);
    T_BS_DEV_CB *bs_dev_cb = NULL;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(addr);
    uint8_t     adv_sid;
    uint8_t     broadcast_id[3];

    T_BASS_PA_SYNC pa_past = BASS_PA_SYNC_PAST;
    uint32_t bis_sync = 0;

    if (past == 0)
    {
        pa_past = BASS_PA_SYNC_NO_PAST;
    }

    if (le_bs_db->state != BROADCAST_SOURCE_STATE_STREAMING)
    {
        return false;
    }

    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("bs_start_sync: can not find device or conn_id");
        return false;
    }

    bs_dev_cb = bs_find_added_dev_cb(p_dev_rec->pseudo_addr);
    if (bs_dev_cb == NULL)
    {
        APP_PRINT_ERROR0("bs_start_sync: can not find device");
        return false;
    }
    bs_dev_cb->sync_by_bass = true;


    if (app_cfg_const.dongle_bis_num == 0)
    {
        if ((bs_dev_cb->chnl_location & AUDIO_LOCATION_RIGHT) &&
            (bs_dev_cb->chnl_location & AUDIO_LOCATION_LEFT))
        {
            bis_sync = 1 << (BRS_AUDIO_CHAN_1_BIS_NUM - 1);
        }
    }
    else if (app_cfg_const.dongle_bis_num == 1)
    {
        if (bs_dev_cb->chnl_location & AUDIO_LOCATION_LEFT)
        {
            bis_sync = BRS_AUDIO_CHAN_1_BIS_NUM;
        }

        if (bs_dev_cb->chnl_location & AUDIO_LOCATION_RIGHT)
        {
            bis_sync = 1 << (BRS_AUDIO_CHAN_2_BIS_NUM - 1);
        }
    }
    else if (app_cfg_const.dongle_bis_num == 2)
    {
        if (bs_dev_cb->chnl_location & AUDIO_LOCATION_LEFT)
        {
            bis_sync = BRS_AUDIO_CHAN_1_BIS_NUM;
        }

        if (bs_dev_cb->chnl_location & AUDIO_LOCATION_RIGHT)
        {
            bis_sync = 1 << (BRS_AUDIO_CHAN_2_BIS_NUM - 1);
        }

        if ((bs_dev_cb->chnl_location & AUDIO_LOCATION_RIGHT) &&
            (bs_dev_cb->chnl_location & AUDIO_LOCATION_LEFT))
        {
            bis_sync = 1 << (BRS_AUDIO_CHAN_3_BIS_NUM - 1);
        }
    }

    if (bis_sync == 0)
    {
        bis_sync = BRS_AUDIO_CHAN_1_BIS_NUM; //default sync
    }

    if (prefer == 0xFF)
    {
        bis_sync = BASS_CP_BIS_SYNC_NO_PREFER;
    }
    APP_PRINT_INFO3("bs_start_sync: bis_sync %x past %d id %x", bis_sync, pa_past,
                    bs_dev_cb->instance_id);

    if (bs_dev_cb->instance_id != 0xFF)
    {
        T_BASS_BRS_DATA *p_brs_data = bass_get_brs_data(p_dev_rec->le_conn_handle, bs_dev_cb->instance_id);
        if (bap_bsrc_get_adv_info(le_bs_db->group1_idx, &adv_sid, broadcast_id))
        {
            if (p_brs_data == NULL ||
                p_brs_data->source_adv_sid != adv_sid ||
                memcmp(broadcast_id, p_brs_data->broadcast_id, 3))
            {
                APP_PRINT_ERROR0("bs_start_sync adv or broadcast_id can't pair");
                bs_dev_cb->instance_id = 0xFF;
            }
            else if (p_brs_data->bis_sync_state != BASS_FAILED_TO_SYNC_TO_BIG &&
                     (bis_sync & p_brs_data->bis_sync_state) == bis_sync)
            {
                APP_PRINT_WARN1("bs_start_sync bis already synced 0x%x", bis_sync);
                return true;
            }
        }

    }

    ba_add_device(le_bs_db->rand_addr, le_bs_db->local_bd_type, le_bs_db->adv_sid,
                  le_bs_db->broadcast_id);
    if (bs_dev_cb->instance_id != 0xFF)
    {
        return bap_ba_modify_local_src(p_dev_rec->le_conn_handle, le_bs_db->group1_idx,
                                       bs_dev_cb->source_id,
                                       pa_past, bis_sync);
    }
    else
    {
        return bap_ba_add_local_src(p_dev_rec->le_conn_handle, le_bs_db->group1_idx, pa_past,
                                    bis_sync);
    }
}

bool bs_stop_sync(uint8_t *addr)
{
    T_BS_DEV_CB *bs_dev_cb = NULL;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(addr);
    uint8_t     adv_sid;
    uint8_t     broadcast_id[3];

    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("bs_stop_sync: can not find device or conn_id");
        return false;
    }

    bs_dev_cb = bs_find_added_dev_cb(p_dev_rec->pseudo_addr);
    if (bs_dev_cb == NULL)
    {
        APP_PRINT_ERROR0("bs_stop_sync: can not find device");
        return false;
    }

    if (bs_dev_cb->instance_id != 0xFF)
    {
        T_BASS_BRS_DATA *p_brs_data = bass_get_brs_data(p_dev_rec->le_conn_handle, bs_dev_cb->instance_id);
        if (bap_bsrc_get_adv_info(le_bs_db->group1_idx, &adv_sid, broadcast_id))
        {
            if (p_brs_data == NULL ||
                p_brs_data->source_adv_sid != adv_sid ||
                memcmp(broadcast_id, p_brs_data->broadcast_id, 3))
            {
                APP_PRINT_ERROR0("bs_stop_sync adv or broadcast_id can't pair");
                bs_dev_cb->instance_id = 0xFF;
                return false;
            }
            else if (p_brs_data->bis_sync_state == 0)
            {
                return true;
            }
        }
        return bap_ba_modify_local_src(p_dev_rec->le_conn_handle, le_bs_db->group1_idx,
                                       bs_dev_cb->source_id,
                                       BASS_PA_NOT_SYNC, 0);
    }
    return false;
}
#endif

#if LE_AUDIO_BASS_CLIENT_SUPPORT
void bs_update_brs_state(T_LE_AUDIO *p_le_audio, T_BASS_CLIENT_BRS_DATA *brs_data)
{
    uint8_t                 addr[BD_ADDR_LEN];
    T_GAP_CONN_INFO         conn_info;
    T_BT_DEVICE              *p_dev_rec;
    uint8_t                 adv_sid;
    uint8_t                 broadcast_id[3];

    T_BS_DEV_CB *bs_dev_cb = NULL;
    if (!p_le_audio || !brs_data || !le_get_conn_info(p_le_audio->conn_id, &conn_info))
    {
        APP_PRINT_ERROR0("bs_update_brs_state fail");
        return;
    }

    p_dev_rec = ual_find_device_by_addr(conn_info.remote_bd);
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("bs_update_brs_state address can't find ");
        return;
    }

    APP_PRINT_INFO1("bs_update_brs_state: addr %b", TRACE_BDADDR(p_dev_rec->pseudo_addr));
    bs_dev_cb = bs_find_added_dev_cb(p_dev_rec->pseudo_addr);
    if (bs_dev_cb == NULL)
    {
        APP_PRINT_ERROR0("bs_update_brs_state address is not in the group");
        return;
    }

    APP_PRINT_INFO3("bs_update_brs_state: brs_is_used %x instance_id %x char_instance_id %x",
                    brs_data->p_brs_data->brs_is_used,
                    bs_dev_cb->instance_id,
                    brs_data->char_instance_id);

    if (brs_data->p_brs_data->brs_is_used == 0 &&
        bs_dev_cb->instance_id == brs_data->char_instance_id)
    {
        bs_dev_cb->instance_id = 0xFF;
        return;
    }

    if (brs_data->p_brs_data->source_address_type == BLE_ADDR_PUBLIC)
    {
        gap_get_param(GAP_PARAM_BD_ADDR, addr);
        if (memcmp(addr, brs_data->p_brs_data->source_address, BD_ADDR_LEN))
        {
            APP_PRINT_ERROR0("bs_update_brs_state public address info can't pair");
            return;
        }
    }
    else
    {
        bap_bsrc_get_random_address(le_bs_db->group1_idx, le_bs_db->rand_addr);
        if (!ble_rpa_matches_local_adapter(brs_data->p_brs_data->source_address))
        {
            APP_PRINT_ERROR0("bs_update_brs_state random address info can't pair");
            return;
        }
    }

    if (bap_bsrc_get_adv_info(le_bs_db->group1_idx, &adv_sid, broadcast_id))
    {
        if (brs_data->p_brs_data->source_adv_sid != adv_sid ||
            memcmp(broadcast_id, brs_data->p_brs_data->broadcast_id, 3))
        {
            APP_PRINT_ERROR0("bs_update_brs_state adv or broadcast_id can't pair");
            return;
        }
    }

    bs_dev_cb->source_id = brs_data->p_brs_data->source_id;
    bs_dev_cb->instance_id = brs_data->char_instance_id;

    if (brs_data->p_brs_data->big_encryption == BIG_BAD_CODE)
    {
        T_BASS_CP_SET_BROADCAST_CODE cp_data;
        if (bs_dev_cb->sync_by_bass)
        {
            cp_data.source_id = brs_data->p_brs_data->source_id;
            memcpy(cp_data.broadcast_code, le_bs_db->broadcast_code, BROADCAST_CODE_LEN);
            bass_cp_set_broadcast_code(brs_data->conn_handle, &cp_data, false);
        }
    }
}
#endif

bool bs_release(void)
{
    if (le_bs_db == NULL)
    {
        return false;
    }

    if (le_bs_db->state == BROADCAST_SOURCE_STATE_IDLE)
    {
        return true;
    }
    return bap_bsrc_release(le_bs_db->group1_idx);
}

uint8_t le_audio_get_brs_state(void)
{
    return le_bs_db->state;
}

bool bs_is_in_idle(void)
{
    if (le_bs_db)
    {
        if (le_bs_db->state == BROADCAST_SOURCE_STATE_STREAMING)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return true;
    }
}

void bis_param_init(void)
{
    uint8_t bis_id_invalid[BROADCAST_ID_LEN] = {0x0};

    uint32_t temp = 0;
    if (app_cfg_const.dongle_broadcast_codec_support)
    {
        le_bs_db->stream_cfg_index = (T_CODEC_CFG_ITEM)app_cfg_const.dongle_broadcast_codec_index;
        if (app_cfg_const.dongle_broadcast_codec_latency)
        {
            le_bs_db->qos_type = QOS_CFG_BIS_LOW_LATENCY;
        }
        else
        {
            le_bs_db->qos_type = QOS_CFG_BIS_HIG_RELIABILITY;
        }
    }
    else
    {
        le_bs_db->stream_cfg_index = CODEC_CFG_ITEM_48_2;
        le_bs_db->qos_type = QOS_CFG_BIS_LOW_LATENCY;
    }

    if (app_cfg_const.dongle_broadcast_code_support)
    {
        memcpy(le_bs_db->broadcast_code, app_cfg_const.dongle_broadcast_code, BROADCAST_CODE_LEN);
    }

    if (memcmp(app_cfg_const.dongle_broadcast_id, bis_id_invalid, BROADCAST_ID_LEN))
    {
        le_bs_db->broadcast_id[0] = app_cfg_const.dongle_broadcast_id[2];
        le_bs_db->broadcast_id[1] = app_cfg_const.dongle_broadcast_id[1];
        le_bs_db->broadcast_id[2] = app_cfg_const.dongle_broadcast_id[0];
    }
    else
    {
        lea_ext_read_broadcast_id(le_bs_db->broadcast_id);
    }
    le_bs_db->adv_sid = app_cfg_const.dongle_adv_sid;

    temp = (app_cfg_const.dongle_ea_int_min[0]) |
           (app_cfg_const.dongle_ea_int_min[1] << 8) |
           (app_cfg_const.dongle_ea_int_min[2] << 16);
    if ((temp >= PRIMARY_ADV_INTERVAL_MIN) &&
        (temp <= PRIMARY_ADV_INTERVAL_MAX))
    {
        le_bs_db->ea_int_min = temp;
    }
    else
    {
        le_bs_db->ea_int_min = DEFAULT_PRIMARY_ADV_INTERVAL_MIN;
    }

    temp = (app_cfg_const.dongle_ea_int_max[0]) |
           (app_cfg_const.dongle_ea_int_max[1] << 8) |
           (app_cfg_const.dongle_ea_int_max[2] << 16);
    if ((temp >= PRIMARY_ADV_INTERVAL_MIN) &&
        (temp <= PRIMARY_ADV_INTERVAL_MAX))
    {
        if (temp < le_bs_db->ea_int_min)
        {
            le_bs_db->ea_int_max = le_bs_db->ea_int_min;
        }
        else
        {
            le_bs_db->ea_int_max = temp;
        }
    }
    else
    {
        le_bs_db->ea_int_max = DEFAULT_PRIMARY_ADV_INTERVAL_MAX;
    }

    temp = (app_cfg_const.dongle_pa_int_min[0]) |
           (app_cfg_const.dongle_pa_int_min[1] << 8) |
           (app_cfg_const.dongle_pa_int_min[2] << 16);
    if ((temp >= PERIODIC_ADV_INTERVAL_MIN) &&
        (temp <= PERIODIC_ADV_INTERVAL_MAX))
    {
        le_bs_db->pa_int_min = temp;
    }
    else
    {
        le_bs_db->pa_int_min = DEFAULT_PERIODIC_ADV_INTERVAL_MIN;
    }

    temp = (app_cfg_const.dongle_pa_int_max[0]) |
           (app_cfg_const.dongle_pa_int_max[1] << 8) |
           (app_cfg_const.dongle_pa_int_max[2] << 16);
    if ((temp >= PERIODIC_ADV_INTERVAL_MIN) &&
        (temp <= PERIODIC_ADV_INTERVAL_MAX))
    {
        if (temp < le_bs_db->pa_int_min)
        {
            le_bs_db->pa_int_max = le_bs_db->pa_int_min;
        }
        else
        {
            le_bs_db->pa_int_max = temp;
        }
    }
    else
    {
        le_bs_db->pa_int_max = DEFAULT_PERIODIC_ADV_INTERVAL_MAX;
    }
    if (app_cfg_const.dongle_adv_sid <= 0x0F)
    {
        le_bs_db->adv_sid = app_cfg_const.dongle_adv_sid;
    }
    else
    {
        le_bs_db->adv_sid = 0;
    }

    temp = app_cfg_const.dongle_pd_delay[0] + (app_cfg_const.dongle_pd_delay[1] << 8);
    le_bs_db->pd_delay = temp * 100;

    le_bs_db->group1_idx = 0xFF;
    le_bs_db->group1_subgroup1_idx = 0xFF;

    codec_preferred_cfg_get((T_CODEC_CFG_ITEM)le_bs_db->stream_cfg_index, &le_bs_db->codec_cfg);
    codec_sdu_interval_get(&le_bs_db->codec_cfg, &le_bs_db->sdu_interval);
    codec_max_sdu_len_get(&le_bs_db->codec_cfg, &le_bs_db->max_sdu);
    qos_preferred_cfg_get((T_CODEC_CFG_ITEM)le_bs_db->stream_cfg_index,
                          (T_QOS_CFG_TYPE)le_bs_db->qos_type, &le_bs_db->prefer_qos);

    if (le_bs_db->pd_delay)
    {
        le_bs_db->prefer_qos.presentation_delay = le_bs_db->pd_delay;
    }

#if UAL_LATENCY_TEST
    qos_preferred_cfg_get((T_CODEC_CFG_ITEM)le_bs_db->stream_cfg_index, QOS_CFG_BIS_LOW_LATENCY,
                          &le_bs_db->prefer_qos);
#endif
#if (LE_AUDIO_BIG_INTERLEAVED == 1)
    le_bs_db->packing = 1;
#else
    le_bs_db->packing = 0;
#endif
    le_bs_db->phy = 2;
    le_bs_db->framing = 0;

    le_bs_db->test_mode = false;
    if (le_bs_db->test_mode)
    {
        le_bs_db->iso_interval = 24;
        le_bs_db->nse = 9;
        le_bs_db->max_pdu = 100;
        le_bs_db->bn = 3;
        le_bs_db->irc = 2;
        le_bs_db->pto = 1;
    }
}

void bs_init(void)
{
    le_bs_db = calloc(1, sizeof(T_LE_BS_DB));
    if (le_bs_db == NULL)
    {
        APP_PRINT_ERROR0("bs_init alloc fail");
        return;
    }

    bis_param_init();

    os_queue_init(&le_bs_db->snk_grp_queue);
    bs_gen_basic_data();
}
#endif

