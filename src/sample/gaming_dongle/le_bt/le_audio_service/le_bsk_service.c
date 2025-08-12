#include <string.h>

#include "trace.h"
#include "os_mem.h"
#include "le_bsk_service.h"
#include "le_audio_data.h"

#include "gap_big_mgr.h"
#include "broadcast_source_sm.h"
#include "metadata_def.h"
#include "codec_def.h"
#include "os_queue.h"
#include "ble_isoch_def.h"
#include "codec_qos.h"
#include "app_usb_layer.h"
#include "bap.h"
#include "le_broadcast_mgr.h"
#include "os_queue.h"

#include "app_audio_path.h"


typedef struct
{
    uint8_t             bd_addr[BD_ADDR_LEN];
    uint8_t             bd_type;
    uint8_t             adv_sid;
    uint8_t             broadcast_id[3];
    uint8_t             num_bis;
    uint8_t             bis[4];
} tBSNK_SYNC_SRC_INFO;

static tBSNK_SYNC_SRC_INFO   bsnk_src_info;
void bsnk_handle_big_sync_state(void *p_buf)
{
    T_BAP_BC_SRC_INFO *p_bc_source = (T_BAP_BC_SRC_INFO *)p_buf;
    if (p_bc_source->big_sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
    {
        ble_audio_route_sink_dev(LE_AUDIO_SINK_FROM_LOCAL, true);
    }
    else if (p_bc_source->big_sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED)
    {
        bap_bsnk_remove_data_path(p_bc_source->adv_addr, p_bc_source->adv_addr_type,
                                  p_bc_source->advertiser_sid, p_bc_source->broadcast_id,
                                  bsnk_src_info.num_bis, bsnk_src_info.bis);
        ble_audio_route_sink_dev(LE_AUDIO_SINK_FROM_LOCAL, false);
    }
}

void bsnk_handle_setup_data_path_cmplt(void *p_buf)
{
    T_BAP_BC_SRC_SETUP_PATH_CMPLT *p_setup_path = (T_BAP_BC_SRC_SETUP_PATH_CMPLT *)p_buf;
    T_BIG_SETUP_DATA_PATH data;

    if (p_setup_path->cause == GAP_SUCCESS)
    {
        if (!base_data_get_bis_codec_cfg(p_setup_path->p_bc_source->p_mapping, p_setup_path->bis_idx,
                                         &data.codec_parsed_data))
        {
            APP_PRINT_ERROR1("bsnk_handle_setup_data_path_cmplt can't find bis_idx %d", p_setup_path->bis_idx);
            return;
        }
        data.path_direction = DATA_PATH_OUTPUT_FLAG;
        data.bis_conn_handle = p_setup_path->bis_conn_handle;
        handle_bis_data_path_setup_complt_msg(&data);
    }
}

void bsnk_handle_rmv_data_path_cmplt(void *p_buf)
{
    T_BAP_BC_SRC_RMV_PATH_CMPLT *p_rmv_path = (T_BAP_BC_SRC_RMV_PATH_CMPLT *)p_buf;
    T_BIG_REMOVE_DATA_PATH data;

    if (p_rmv_path->cause == GAP_SUCCESS)
    {
        data.path_direction = DATA_PATH_OUTPUT_FLAG;
        data.bis_conn_handle = p_rmv_path->bis_conn_handle;
        handle_bis_data_path_remove_complt_msg(&data);
    }
}

void bsnk_select_broadcast_src(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                               uint8_t broadcast_id[3], uint8_t *code)
{
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    if (p_bc_source)
    {
        memcpy(p_bc_source->broadcast_code, code, BROADCAST_CODE_LEN);
        p_bc_source->pa_sync_mask |= BAP_BSNK_PA_SYNC_BSRC_MSK;
        if (p_bc_source->pa_sync_state == GAP_PA_SYNC_STATE_TERMINATED)
        {
            bap_bsnk_pa_sync(bd_addr, bd_type, adv_sid, broadcast_id, 5);
        }

    }
}

bool bsnk_big_establish(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3],
                        uint8_t num_bis, uint8_t *p_bis)
{
    T_CODEC_CFG codec_parsed_data[2];
    uint8_t ret = 0;

    if (num_bis > 2)
    {
        APP_PRINT_WARN0("bsnk_big_establish Dongle can only sync 2 bis");
        num_bis = 2;
    }
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);

    if (p_bc_source == NULL)
    {
        ret = 1;
        goto fail;
    }
    for (int i = 0; i < num_bis; i++)
    {
        if (!base_data_get_bis_codec_cfg(p_bc_source->p_mapping, p_bis[i],
                                         &codec_parsed_data[i]))
        {
            ret = 2;
            goto fail;
        }
    }

    //FIX TODO may be we should check type_exist
    if (num_bis == 2)
    {
        if (codec_parsed_data[0].frame_duration != codec_parsed_data[1].frame_duration ||
            codec_parsed_data[0].sample_frequency != codec_parsed_data[1].sample_frequency ||
            codec_parsed_data[0].codec_frame_blocks_per_sdu != codec_parsed_data[1].codec_frame_blocks_per_sdu
            ||
            codec_parsed_data[0].octets_per_codec_frame != codec_parsed_data[1].octets_per_codec_frame)
        {
            ret = 3;
            goto fail;
        }
        uint8_t chnl_cnt1 = __builtin_popcount(codec_parsed_data[0].audio_channel_allocation) == 0 ?
                            1 : __builtin_popcount(codec_parsed_data[0].audio_channel_allocation);
        uint8_t chnl_cnt2 = __builtin_popcount(codec_parsed_data[1].audio_channel_allocation) == 0 ?
                            1 : __builtin_popcount(codec_parsed_data[1].audio_channel_allocation);
        if (chnl_cnt1 != 1 || chnl_cnt2 != 1)
        {
            ret = 4;
            goto fail;
        }
    }
    else
    {
        uint8_t chnl_cnt1 = __builtin_popcount(codec_parsed_data[0].audio_channel_allocation) == 0 ?
                            1 : __builtin_popcount(codec_parsed_data[0].audio_channel_allocation);
        if (chnl_cnt1 > 2)
        {
            ret = 5;
            goto fail;
        }
    }

    memcpy(bsnk_src_info.bd_addr, bd_addr, BD_ADDR_LEN);
    bsnk_src_info.bd_type = bd_type;
    bsnk_src_info.adv_sid = adv_sid;
    memcpy(bsnk_src_info.broadcast_id, broadcast_id, 3);
    bsnk_src_info.num_bis = num_bis;
    memcpy(bsnk_src_info.bis, p_bis, num_bis);

    p_bc_source->big_sync_mask |= BAP_BSNK_BIG_SYNC_BSRC_MSK;
    return bap_bsnk_sync_big_establish(bd_addr, bd_type, adv_sid, broadcast_id, num_bis, p_bis);

fail:
    APP_PRINT_ERROR1("bsnk_big_establish failed, ret 0x%x", ret);
    return false;
}

bool bsnk_big_terminate(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3])
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    if (p_bc_source == NULL)
    {
        return false;
    }
    p_bc_source->pa_sync_mask &= (~BAP_BSNK_PA_SYNC_BSRC_MSK);
    p_bc_source->big_sync_mask &= (~BAP_BSNK_BIG_SYNC_BSRC_MSK);
    return bap_bsnk_sync_big_terminate(bd_addr, bd_type, adv_sid, broadcast_id);
}

bool bsnk_big_release(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3])
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    if (p_bc_source == NULL)
    {
        return false;
    }

    p_bc_source->pa_sync_mask &= (~BAP_BSNK_PA_SYNC_BSRC_MSK);
    p_bc_source->big_sync_mask &= (~BAP_BSNK_BIG_SYNC_BSRC_MSK);
    return bap_bsnk_pa_sync_release(bd_addr, bd_type, adv_sid, broadcast_id);
}

void le_bsnk_setup_sink_audio(void)
{
    T_CODEC_CFG codec_parsed_data[2];
    uint8_t ret = 0;
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(bsnk_src_info.bd_addr,
                                                              bsnk_src_info.bd_type,
                                                              bsnk_src_info.adv_sid, bsnk_src_info.broadcast_id);

    if (p_bc_source == NULL)
    {
        ret = 1;
        goto fail;
    }
    for (int i = 0; i < bsnk_src_info.num_bis; i++)
    {
        if (!base_data_get_bis_codec_cfg(p_bc_source->p_mapping, bsnk_src_info.bis[i],
                                         &codec_parsed_data[i]))
        {
            APP_PRINT_ERROR1("le_bsnk_setup_sink_audio can't find bis_idx %d", bsnk_src_info.bis[i]);
            ret = 2;
            goto fail;
        }
    }

    //FIX TODO may be we should check type_exist
    if (bsnk_src_info.num_bis == 2)
    {
        if (codec_parsed_data[0].frame_duration != codec_parsed_data[1].frame_duration ||
            codec_parsed_data[0].sample_frequency != codec_parsed_data[1].sample_frequency ||
            codec_parsed_data[0].codec_frame_blocks_per_sdu != codec_parsed_data[1].codec_frame_blocks_per_sdu
            ||
            codec_parsed_data[0].octets_per_codec_frame != codec_parsed_data[1].octets_per_codec_frame)
        {
            ret = 3;
            goto fail;
        }
        uint8_t chnl_cnt1 = __builtin_popcount(codec_parsed_data[0].audio_channel_allocation) == 0 ?
                            1 : __builtin_popcount(codec_parsed_data[0].audio_channel_allocation);
        uint8_t chnl_cnt2 = __builtin_popcount(codec_parsed_data[1].audio_channel_allocation) == 0 ?
                            1 : __builtin_popcount(codec_parsed_data[1].audio_channel_allocation);
        if (chnl_cnt1 != 1 || chnl_cnt2 != 1)
        {
            ret = 4;
            goto fail;
        }
    }
    else
    {
        uint8_t chnl_cnt1 = __builtin_popcount(codec_parsed_data[0].audio_channel_allocation) == 0 ?
                            1 : __builtin_popcount(codec_parsed_data[0].audio_channel_allocation);
        if (chnl_cnt1 > 2)
        {
            ret = 5;
            goto fail;
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
        APP_PRINT_ERROR1("le_bsnk_setup_sink_audio source sample rate %d not supported",
                         codec_parsed_data[0].sample_frequency);
        return;
    }

    if (bsnk_src_info.num_bis == 1)
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

    if (p_bc_source->p_mapping)
    {
        src.attr.lc3.presentation_delay = p_bc_source->p_mapping->presentation_delay;
    }
    else
    {
        APP_PRINT_ERROR0("le_bsnk_setup_sink_audio source unkonw presentation_delay");
        src.attr.lc3.presentation_delay = 40000;
    }

    bap_bsnk_setup_data_path(p_bc_source->adv_addr, p_bc_source->adv_addr_type,
                             p_bc_source->advertiser_sid, p_bc_source->broadcast_id,
                             bsnk_src_info.num_bis, bsnk_src_info.bis);

fail:
    APP_PRINT_ERROR1("le_bsnk_setup_sink_audio failed: ret 0x%x", ret);
    return;
}

void le_bsnk_remove_sink_audio(void)
{
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(bsnk_src_info.bd_addr,
                                                              bsnk_src_info.bd_type,
                                                              bsnk_src_info.adv_sid, bsnk_src_info.broadcast_id);

    if (p_bc_source == NULL)
    {
        APP_PRINT_ERROR0("le_bsnk_remove_sink_audio can't find src");
        return;
    }
    bap_bsnk_remove_data_path(p_bc_source->adv_addr, p_bc_source->adv_addr_type,
                              p_bc_source->advertiser_sid, p_bc_source->broadcast_id,
                              bsnk_src_info.num_bis, bsnk_src_info.bis);

}


