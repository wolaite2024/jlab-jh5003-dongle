#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "le_broadcast_mgr.h"
#include "base_data_generate.h"
#include "gap_big_mgr.h"
//#include "gap_past_sender.h"
#include "le_bst_src_service.h"
#include "le_ba_service.h"
#include "le_bsk_service.h"
#include "ual_adv.h"
#include "ual_adapter.h"

typedef enum
{
    LE_AUDIO_MSG_BAP_BS_SRC_STATE_CHANGE,
    LE_AUDIO_MSG_BAP_BS_SRC_SET_DATA_PATH,
    LE_AUDIO_MSG_BAP_BS_SRC_RMV_DATA_PATH,
    LE_AUDIO_MSG_BAP_PA_SYNC_STATE,
    LE_AUDIO_MSG_BAP_PA_INFO_UPDATE_REPORT,
    LE_AUDIO_MSG_BAP_BIGINFO_UPDATE_REPORT,
    LE_AUDIO_MSG_BAP_BIG_SYNC_STATE,
    LE_AUDIO_MSG_BAP_SETUP_DATA_PATH_CMPLT,
    LE_AUDIO_MSG_BAP_REMOVE_DATA_PATH_CMPLT,
} T_LE_BROADCAST_MSG;

#if LE_AUDIO_BROADCAST_SOURCE_ROLE
#include "broadcast_source_sm.h"
#endif

#if LE_AUDIO_BROADCAST_SOURCE_ROLE
#define BS_PERIODIC_ADV_PROP                 0
#define BS_SECONDARY_ADV_MAX_SKIP            0

typedef struct t_bsrc_base
{
    struct t_bsrc_base      *p_next;
    T_BROADCAST_SOURCE_HANDLE handle;
    T_BROADCAST_SOURCE_STATE  state;
    uint8_t                   group1_idx;
    uint8_t                   adv_sid;
    uint8_t                   broadcast_id[3];
    T_BLE_BD_TYPE            local_bd_type;
    uint8_t                   rand_addr[BD_ADDR_LEN];
} T_BSRC_BASE;

typedef struct
{
    T_OS_QUEUE      src_base_queue;
} T_BAP_BSRC_DB;

#endif

#if LE_AUDIO_PUBLIC_BROADCAST_SOURCE_ROLE
#include "pbp_def.h"
#define PUBLIC_BS_ANNOUNCEMENTS_BROADCAST_ID_OFFSET 8
#define PUBLIC_BS_ANNOUNCEMENTS_FEATURES_OFFSET 15

uint8_t broadcast_source_eadv_general_audio(uint8_t *audio_adv_data)
{
    uint8_t idx = 0;
    uint8_t name_len = strlen((const char *)app_cfg_const.device_name_le_default);
    if (name_len > 32)
    {
        name_len = 32; // Broadcast_Name length max is 32
    }

    audio_adv_data[idx++] = 0x03;
    audio_adv_data[idx++] = GAP_ADTYPE_APPEARANCE;
    audio_adv_data[idx++] = LO_WORD(GAP_GATT_APPEARANCE_GENERIC_MEDIA_PLAYER);
    audio_adv_data[idx++] = HI_WORD(GAP_GATT_APPEARANCE_GENERIC_MEDIA_PLAYER);
    audio_adv_data[idx++] = 0x06;
    audio_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
    audio_adv_data[idx++] = (uint8_t)(BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID);
    audio_adv_data[idx++] = (uint8_t)(BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID >> 8);
    audio_adv_data[idx++] = 0x00;
    audio_adv_data[idx++] = 0x00;
    audio_adv_data[idx++] = 0x00; // broadcast id
    audio_adv_data[idx++] = 0x09;
    audio_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
    audio_adv_data[idx++] = (uint8_t)(PUBIC_BROADCAST_ANNOUNCEMENT_SRV_UUID);
    audio_adv_data[idx++] = (uint8_t)(PUBIC_BROADCAST_ANNOUNCEMENT_SRV_UUID >> 8);
    audio_adv_data[idx++] = 0x00; // Public Broadcast Announcement features
    audio_adv_data[idx++] = 0x04; // Metadata length
    audio_adv_data[idx++] = 0x03; // Metadata
    audio_adv_data[idx++] = METADATA_TYPE_PROGRAM_INFO;
    audio_adv_data[idx++] = 'T';
    audio_adv_data[idx++] = 'V';
    audio_adv_data[idx++] = name_len + 1;
    audio_adv_data[idx++] = GAP_ADTYPE_BROADCAST_NAME;
    memcpy(&audio_adv_data[idx], app_cfg_const.device_name_le_default, name_len);
    idx += name_len;
    return idx;
}
#endif

void le_broadcast_mgr_handle_msg(T_LE_BROADCAST_MSG msg, void *buf)
{
    APP_PRINT_TRACE1("le_broadcast_mgr_handle_msg: msg 0x%04x", msg);
#if LE_AUDIO_BROADCAST_SOURCE_ROLE
    if (msg == LE_AUDIO_MSG_BAP_BS_SRC_STATE_CHANGE)
    {
        BAP_BS_SRC_STATE_CHANGE *p_state = (BAP_BS_SRC_STATE_CHANGE *)buf;
        bs_handle_state_change(p_state->new_state);
    }
    else if (msg == LE_AUDIO_MSG_BAP_BS_SRC_SET_DATA_PATH)
    {
        bs_handle_setup_data_path((T_BROADCAST_SOURCE_SETUP_DATA_PATH *)buf);
    }
    else if (msg == LE_AUDIO_MSG_BAP_BS_SRC_RMV_DATA_PATH)
    {
        bs_handle_rmv_data_path((T_BROADCAST_SOURCE_REMOVE_DATA_PATH *)buf);
    }
#endif

    if (msg == LE_AUDIO_MSG_BAP_PA_SYNC_STATE ||
        msg == LE_AUDIO_MSG_BAP_BIG_SYNC_STATE)
    {
#if LE_AUDIO_BROADCAST_SINK_ROLE
        if (msg == LE_AUDIO_MSG_BAP_BIG_SYNC_STATE)
        {
            bsnk_handle_big_sync_state((void *)buf);
        }
#endif
#if LE_AUDIO_BROADCAST_ASSISTANT_ROLE
        ba_bsnk_handle_src_sync_state((void *)buf);
#endif
    }
#if LE_AUDIO_BROADCAST_ASSISTANT_ROLE
    else if (msg == LE_AUDIO_MSG_BAP_PA_INFO_UPDATE_REPORT)
    {
        ba_bsnk_handle_bst_src_info((void *)buf);
    }
    else if (msg == LE_AUDIO_MSG_BAP_SETUP_DATA_PATH_CMPLT)
    {
        bsnk_handle_setup_data_path_cmplt((void *)buf);
    }
    else if (msg == LE_AUDIO_MSG_BAP_REMOVE_DATA_PATH_CMPLT)
    {
        bsnk_handle_rmv_data_path_cmplt((void *)buf);
    }
#endif
}


#if (LE_AUDIO_BROADCAST_ASSISTANT_ROLE | LE_AUDIO_BROADCAST_SINK_ROLE)
typedef struct
{
    T_BAP_BC_SRC_INFO   *bc_src_selected;
    T_OS_QUEUE          bc_src_queue;
} T_BAP_BA_BSK_CB;

static T_BAP_BA_BSK_CB *p_ba_bsnk_db = NULL;
#endif

#if LE_AUDIO_BROADCAST_SOURCE_ROLE
static T_BAP_BSRC_DB *p_bs_src_db = NULL;

static bool brsc_get_eadv_random_address(T_BROADCAST_SOURCE_HANDLE handle,
                                         uint8_t *p_random_addr)
{
    T_BROADCAST_SOURCE_INFO brsc_info;
    if (p_random_addr == NULL)
    {
        return false;
    }
    if (broadcast_source_get_info(handle, &brsc_info))
    {
        return ble_get_ext_adv_random_addr(brsc_info.adv_handle, p_random_addr);
    }
    return false;
}

static T_BSRC_BASE *find_src_base_by_grp_idx(uint8_t grp_idx)
{
    int i;
    T_BSRC_BASE *p_bs_base = NULL;
    for (i = 0; i < p_bs_src_db->src_base_queue.count; i++)
    {
        p_bs_base = (T_BSRC_BASE *)os_queue_peek(&p_bs_src_db->src_base_queue, i);
        if (p_bs_base->group1_idx == grp_idx)
        {
            return p_bs_base;
        }
    }
    return NULL;
}

static T_BSRC_BASE *find_src_base_by_handle(T_BROADCAST_SOURCE_HANDLE handle)
{
    int i;
    T_BSRC_BASE *p_bs_base = NULL;
    for (i = 0; i < p_bs_src_db->src_base_queue.count; i++)
    {
        p_bs_base = (T_BSRC_BASE *)os_queue_peek(&p_bs_src_db->src_base_queue, i);
        if (p_bs_base->handle == handle)
        {
            return p_bs_base;
        }
    }
    return NULL;
}

static T_BSRC_BASE *find_src_base_by_adv_sid(uint8_t adv_sid, uint8_t broadcast_id[3])
{
    int i;
    T_BSRC_BASE *p_bs_base = NULL;
    for (i = 0; i < p_bs_src_db->src_base_queue.count; i++)
    {
        p_bs_base = (T_BSRC_BASE *)os_queue_peek(&p_bs_src_db->src_base_queue, i);
        if (p_bs_base->adv_sid == adv_sid &&
            memcmp(broadcast_id, p_bs_base->broadcast_id, 3) == 0)
        {
            return p_bs_base;
        }
    }
    return NULL;
}

static T_BSRC_BASE *bap_alloc_bsrc_group(T_BROADCAST_SOURCE_HANDLE handle)
{
    if (p_bs_src_db == NULL)
    {
        return NULL;
    }

    T_BSRC_BASE *p_bs_src_base = calloc(1, sizeof(T_BSRC_BASE));
    if (p_bs_src_base == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_alloc_bsrc_group: alloc T_BSRC_BASE fail");
        return NULL;
    }

    p_bs_src_base->handle = handle;
    os_queue_in(&p_bs_src_db->src_base_queue, p_bs_src_base);
    return p_bs_src_base;
}

static bool bap_remove_bsrc_group_by_handle(T_BROADCAST_SOURCE_HANDLE handle)
{
    T_BSRC_BASE *p_bs_src_base;

    if (p_bs_src_db == NULL)
    {
        return false;
    }

    if ((p_bs_src_base = find_src_base_by_handle(handle)) == NULL)
    {
        return true;
    }

#if (LE_AUDIO_BROADCAST_ASSISTANT_ROLE)
    bap_ba_bsnk_rmv_src_dev(p_bs_src_base->rand_addr, p_bs_src_base->local_bd_type,
                            p_bs_src_base->adv_sid, p_bs_src_base->broadcast_id);
#endif

    os_queue_delete(&p_bs_src_db->src_base_queue, p_bs_src_base);
    free(p_bs_src_base);

    return true;
}

static void bap_bsrc_setup_data_path(T_BROADCAST_SOURCE_HANDLE handle, uint8_t bis_idx)
{
    uint8_t codec_id[5] = {0x03, 0, 0, 0, 0}; //transparent codec id
    broadcast_source_setup_data_path(handle, bis_idx,
                                     codec_id, 0, //As controller doesn't consider it , set it to 0
                                     0, NULL);
}

static void bap_bsrc_cb(T_BROADCAST_SOURCE_HANDLE handle, uint8_t cb_type, void *p_cb_data)
{
    T_BROADCAST_SOURCE_SM_CB_DATA *p_sm_data = (T_BROADCAST_SOURCE_SM_CB_DATA *)p_cb_data;
    T_BSRC_BASE *p_bs_src_base = find_src_base_by_handle(handle);
    if (p_bs_src_base->handle != handle)
    {
        PROTOCOL_PRINT_ERROR1("[BAP][BS]bap_bsrc_cb: not find handle %p", handle);
        return;
    }

    switch (cb_type)
    {
    case MSG_BROADCAST_SOURCE_STATE_CHANGE:
        {
            BAP_BS_SRC_STATE_CHANGE state;
            PROTOCOL_PRINT_INFO2("[BAP][BS]MSG_BROADCAST_SOURCE_STATE_CHANGE: state %d, cause 0x%x",
                                 p_sm_data->p_state_change->state,
                                 p_sm_data->p_state_change->cause);
            if (p_sm_data->p_state_change->state == BROADCAST_SOURCE_STATE_STREAMING &&
                p_sm_data->p_state_change->cause == GAP_SUCCESS)
            {
                bap_bsrc_setup_data_path(handle, 1);
            }
            else if (p_sm_data->p_state_change->state == BROADCAST_SOURCE_STATE_CONFIGURED &&
                     p_sm_data->p_state_change->cause == GAP_SUCCESS)
            {
                // NEED app to start
                //app_bs_establish(broadcast_code);
            }
            else if (p_sm_data->p_state_change->state == BROADCAST_SOURCE_STATE_IDLE)
            {
                bap_remove_bsrc_group_by_handle(handle);
            }

            state.old_state = p_bs_src_base->state;
            state.new_state = p_sm_data->p_state_change->state;
            p_bs_src_base->state = p_sm_data->p_state_change->state;
            le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_BS_SRC_STATE_CHANGE,
                                        &state);
        }
        break;

    case MSG_BROADCAST_SOURCE_METADATA_UPDATE:
        {
            PROTOCOL_PRINT_INFO1("[BAP][BS]MSG_BROADCAST_SOURCE_METADATA_UPDATE: cause 0x%x", p_sm_data->cause);
        }
        break;

    case MSG_BROADCAST_SOURCE_RECONFIG:
        {
            PROTOCOL_PRINT_INFO1("[BAP][BS]MSG_BROADCAST_SOURCE_RECONFIG: cause 0x%x", p_sm_data->cause);
        }
        break;

    case MSG_BROADCAST_SOURCE_SETUP_DATA_PATH:
        {
            PROTOCOL_PRINT_INFO2("[BAP][BS]MSG_BROADCAST_SOURCE_SETUP_DATA_PATH: bis_idx %d, cause 0x%x",
                                 p_sm_data->p_setup_data_path->bis_idx,
                                 p_sm_data->p_setup_data_path->cause);
            uint8_t bis_num;
            if (base_data_get_bis_num(p_bs_src_base->group1_idx, &bis_num))
            {
                if (p_sm_data->p_setup_data_path->bis_idx < bis_num)
                {
                    bap_bsrc_setup_data_path(handle, p_sm_data->p_setup_data_path->bis_idx + 1);
                }
            }
            le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_BS_SRC_SET_DATA_PATH,
                                        p_sm_data->p_setup_data_path);
        }
        break;

    case MSG_BROADCAST_SOURCE_REMOVE_DATA_PATH:
        {
            PROTOCOL_PRINT_INFO2("[BAP][BS]MSG_BROADCAST_SOURCE_REMOVE_DATA_PATH: bis_conn_handle %d, cause 0x%x",
                                 p_sm_data->p_remove_data_path->bis_conn_handle,
                                 p_sm_data->p_remove_data_path->cause);
            le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_BS_SRC_RMV_DATA_PATH,
                                        p_sm_data->p_remove_data_path);

        }
        break;
    default:
        break;
    }
}

bool bap_bsrc_get_random_address(uint8_t group_idx, uint8_t *p_random_addr)
{
    T_BSRC_BASE *p_bs_src_base = NULL;
    if (p_bs_src_db == NULL || p_random_addr == NULL)
    {
        return false;
    }
    p_bs_src_base = find_src_base_by_grp_idx(group_idx);

    if (p_bs_src_base == NULL || p_bs_src_base->handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_reconfig: p_bs_src_base is NULL or handle is NULL");
        return false;
    }

    if (p_bs_src_base->local_bd_type == BLE_ADDR_RANDOM)
    {
        if (brsc_get_eadv_random_address(p_bs_src_base->handle, p_random_addr))
        {
            memcpy(p_bs_src_base->rand_addr, p_random_addr, BD_ADDR_LEN);
            return true;
        }
    }

    return false;
}


bool bap_bsrc_get_adv_info(uint8_t group_idx, uint8_t *adv_sid, uint8_t *broadcast_id)
{
    T_BSRC_BASE *p_bs_src_base = NULL;
    if (p_bs_src_db == NULL || adv_sid == NULL || broadcast_id == NULL)
    {
        return false;
    }
    p_bs_src_base = find_src_base_by_grp_idx(group_idx);

    if (p_bs_src_base == NULL || p_bs_src_base->handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_get_adv_info: p_bs_src_base is NULL or handle is NULL");
        return false;
    }
    *adv_sid = p_bs_src_base->adv_sid;
    memcpy(broadcast_id, p_bs_src_base->broadcast_id, 3);

    return true;
}

T_BROADCAST_SOURCE_HANDLE bap_bsrc_create_handle()
{
    return broadcast_source_add(bap_bsrc_cb);
}


bool bap_bsrc_config(T_BLE_BD_TYPE local_addr_type, uint8_t adv_sid, uint8_t group_idx,
                     T_BROADCAST_SOURCE_HANDLE handle,
                     uint32_t ea_min, uint32_t ea_max,
                     uint16_t pa_min, uint16_t pa_max,
                     T_QOS_CFG_TYPE qos, uint8_t encrypted)
{

    PROTOCOL_PRINT_INFO0("[BAP][BS]bap_bsrc_config");
    bool res = true;
    uint8_t ret_val = 0;
    uint16_t pa_data_len = 0;
    uint8_t *pa_data = NULL;
    uint8_t tx_power = 127;
    uint8_t broadcast_audio_announcements[21 + 34];
    T_BROADCAST_SOURCE_INFO src_info;
    uint8_t adv_len = 0;

    T_BSRC_BASE *p_bs_src_base = NULL;
    if (p_bs_src_db == NULL || handle == NULL)
    {
        ret_val = 1;
        res = false;
        goto done;
    }

    if ((p_bs_src_base = find_src_base_by_handle(handle)) == NULL)
    {
        p_bs_src_base = bap_alloc_bsrc_group(handle);
    }

    if (p_bs_src_base == NULL)
    {
        ret_val = 2;
        res = false;
        goto done;
    }

    p_bs_src_base->group1_idx = group_idx;

    if (broadcast_source_get_info(p_bs_src_base->handle, &src_info) == false)
    {
        ret_val = 3;
        res = false;
        goto done;
    }
    p_bs_src_base->adv_sid = src_info.adv_sid;
    memcpy(p_bs_src_base->broadcast_id, src_info.broadcast_id, BROADCAST_ID_LEN);
    //FIX TODO the privacy need to support
    if (local_addr_type == BLE_ADDR_PUBLIC)
    {
        p_bs_src_base->local_bd_type = BLE_ADDR_PUBLIC;
        gap_get_param(GAP_PARAM_BD_ADDR, p_bs_src_base->rand_addr);
    }
    else if (local_addr_type == BLE_ADDR_RANDOM)
    {
        p_bs_src_base->local_bd_type = BLE_ADDR_RANDOM;
    }
#if LE_AUDIO_PUBLIC_BROADCAST_SOURCE_ROLE
    adv_len = broadcast_source_eadv_general_audio(broadcast_audio_announcements);
    memcpy(&broadcast_audio_announcements[PUBLIC_BS_ANNOUNCEMENTS_BROADCAST_ID_OFFSET],
           src_info.broadcast_id, BROADCAST_ID_LEN);

    if (qos == QOS_CFG_BIS_LOW_LATENCY)
    {
        broadcast_audio_announcements[PUBLIC_BS_ANNOUNCEMENTS_FEATURES_OFFSET] =
            PUBIC_BROADCAST_BIT_STANDARD_QUALITY_PRESENT;
    }
    else
    {
        broadcast_audio_announcements[PUBLIC_BS_ANNOUNCEMENTS_FEATURES_OFFSET] =
            PUBIC_BROADCAST_BIT_HIGH_QUALITY_PRESENT;
    }
    if (encrypted)
    {
        broadcast_audio_announcements[PUBLIC_BS_ANNOUNCEMENTS_FEATURES_OFFSET] |=
            PUBIC_BROADCAST_BIT_ENCRYPTED;
    }

#else
    uint8_t idx = 0;
    broadcast_audio_announcements[idx++] = 6;
    broadcast_audio_announcements[idx++] = GAP_ADTYPE_SERVICE_DATA;
    broadcast_audio_announcements[idx++] = (uint8_t)(BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID);
    broadcast_audio_announcements[idx++] = (uint8_t)(BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID >> 8);
    memcpy(&broadcast_audio_announcements[idx], src_info.broadcast_id, BROADCAST_ID_LEN);
    idx += BROADCAST_ID_LEN;
    adv_len = idx;
#endif

    PROTOCOL_PRINT_INFO1("[BAP][BS]bap_bsrc_config: adv_len 0x%x", adv_len);

    if (broadcast_source_set_eadv_param(p_bs_src_base->handle, adv_sid,
                                        ea_min, ea_max,
                                        GAP_ADVCHAN_ALL, (T_GAP_LOCAL_ADDR_TYPE)p_bs_src_base->local_bd_type, p_bs_src_base->rand_addr,
                                        GAP_ADV_FILTER_ANY, tx_power,
                                        GAP_PHYS_PRIM_ADV_1M, BS_SECONDARY_ADV_MAX_SKIP,
                                        GAP_PHYS_1M,
                                        adv_len,
                                        broadcast_audio_announcements) == false)
    {
        ret_val = 4;
        res = false;
        goto done;
    }
    else if (local_addr_type == BLE_ADDR_RANDOM)
    {
        brsc_get_eadv_random_address(p_bs_src_base->handle, p_bs_src_base->rand_addr);
    }

    if (base_data_gen_pa_data(p_bs_src_base->group1_idx, &pa_data_len, &pa_data) == false)
    {
        ret_val = 5;
        res = false;
        goto done;

    }
    if (broadcast_source_set_pa_param(p_bs_src_base->handle, pa_min,
                                      pa_max, BS_PERIODIC_ADV_PROP,
                                      pa_data_len, pa_data) == false)
    {
        ret_val = 6;
        res = false;
        goto done;
    }

    if (broadcast_source_config(p_bs_src_base->handle) == false)
    {
        ret_val = 7;
        res = false;
        goto done;
    }

#if (LE_AUDIO_BROADCAST_ASSISTANT_ROLE)
    if (bap_ba_bsnk_add_src_dev(p_bs_src_base->rand_addr, p_bs_src_base->local_bd_type,
                                p_bs_src_base->adv_sid, p_bs_src_base->broadcast_id,
                                true))
    {
        T_BAP_BC_SRC_INFO *p_src_info = bap_ba_bsnk_find_bst_dev(p_bs_src_base->rand_addr,
                                                                 (uint8_t)p_bs_src_base->local_bd_type,
                                                                 p_bs_src_base->adv_sid,
                                                                 p_bs_src_base->broadcast_id);
        if (p_src_info == NULL)
        {
            ret_val = 8;
            PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_config: p_bs_src_base can't find broadcast src info");
            goto done;
        }
        p_src_info->p_mapping = base_data_parse_data(pa_data_len, pa_data);
    }
#endif

done:
    PROTOCOL_PRINT_INFO1("[BAP][BS]bap_bsrc_config: ret_val 0x%x", ret_val);
    if (pa_data)
    {
        free(pa_data);
    }
    return res;
}

bool bap_bsrc_reconfig(uint8_t group_idx, uint8_t subgroup_idx, uint8_t metadata_len,
                       uint8_t *p_metadata)
{
    uint16_t pa_data_len;
    uint8_t *pa_data = NULL;
    T_BSRC_BASE *p_bs_src_base = NULL;
    if (p_bs_src_db == NULL)
    {
        return false;
    }
    p_bs_src_base = find_src_base_by_grp_idx(group_idx);

    if (p_bs_src_base == NULL || p_bs_src_base->handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_reconfig: p_bs_src_base is NULL or handle is NULL");
        return false;
    }

    if (base_data_update_metadata(group_idx, subgroup_idx,
                                  metadata_len, p_metadata) == false)
    {
        goto error;
    }

    if (base_data_gen_pa_data(group_idx, &pa_data_len, &pa_data) == false)
    {
        goto error;
    }
    if (broadcast_source_reconfig(p_bs_src_base->handle, pa_data_len, pa_data) == false)
    {
        goto error;
    }

    if (p_bs_src_base->local_bd_type == BLE_ADDR_RANDOM)
    {
        brsc_get_eadv_random_address(p_bs_src_base->handle, p_bs_src_base->rand_addr);
    }

    if (pa_data)
    {
        free(pa_data);
    }
    return true;
error:
    if (pa_data)
    {
        free(pa_data);
    }
    return false;
}

bool bap_bsrc_metaupdate(uint8_t group_idx, uint8_t subgroup_idx, uint8_t metadata_len,
                         uint8_t *p_metadata)
{
    uint16_t pa_data_len;
    uint8_t *pa_data = NULL;

    T_BSRC_BASE *p_bs_src_base = NULL;
    if (p_bs_src_db == NULL)
    {
        return false;
    }

    p_bs_src_base = find_src_base_by_grp_idx(group_idx);
    if (p_bs_src_base == NULL || p_bs_src_base->handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_metaupdate: p_bs_src_base is NULL or handle is NULL");
        return false;
    }


    if (base_data_update_metadata(group_idx, subgroup_idx,
                                  metadata_len, p_metadata) == false)
    {
        goto error;
    }

    if (base_data_gen_pa_data(group_idx, &pa_data_len, &pa_data) == false)
    {
        goto error;
    }
    if (broadcast_source_pa_update(p_bs_src_base->handle, pa_data_len, pa_data) == false)
    {
        goto error;
    }

    if (pa_data && pa_data_len)
    {
        free(pa_data);
    }

    return true;
error:
    if (pa_data)
    {
        free(pa_data);
    }
    return false;
}

bool bap_bsrc_establish(uint8_t group_idx, uint8_t  retransmission_number, uint32_t sdu_interval,
                        uint16_t max_sdu,  uint8_t phy, uint8_t packing,
                        uint8_t framing, uint16_t max_transport_latency, uint8_t *broadcast_code)
{
    T_BIG_MGR_ISOC_BROADCASTER_CREATE_BIG_PARAM big_param;
    T_BSRC_BASE *p_bs_src_base = NULL;
    uint8_t bis_num;
    if (p_bs_src_db == NULL || broadcast_code == NULL)
    {
        return false;
    }
    p_bs_src_base = find_src_base_by_grp_idx(group_idx);
    if (p_bs_src_base == NULL || p_bs_src_base->handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_establish: p_bs_src_base is NULL or handle is NULL");
        return false;
    }
    memset(&big_param, 0, sizeof(T_BIG_MGR_ISOC_BROADCASTER_CREATE_BIG_PARAM));

    if (!base_data_get_bis_num(group_idx, &bis_num))
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_establish: can't find bis num");
        return false;
    }

    big_param.rtn = retransmission_number;
    big_param.phy = phy;
    big_param.sdu_interval = sdu_interval;
    big_param.max_sdu = max_sdu;
    big_param.max_transport_latency = max_transport_latency;
    big_param.num_bis = bis_num;
    big_param.framing = framing;
#if (LE_AUDIO_BIG_INTERLEAVED == 1)
    big_param.packing = packing;
#else
    big_param.packing = 0;
#endif
    if (memcmp(big_param.broadcast_code, broadcast_code, 16) == 0)
    {
        big_param.encryption = false;
    }
    else
    {
        memcpy(big_param.broadcast_code, broadcast_code, 16);
        big_param.encryption = true;
    }
#if (LE_AUDIO_BROADCAST_ASSISTANT_ROLE)
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(p_bs_src_base->rand_addr,
                                                              p_bs_src_base->local_bd_type,
                                                              p_bs_src_base->adv_sid, p_bs_src_base->broadcast_id);
    if (p_bc_source && p_bc_source->local_src)
    {
        memcpy(p_bc_source->broadcast_code, broadcast_code, 16);
    }
#endif
    if (broadcast_source_establish(p_bs_src_base->handle, big_param) == false)
    {
        goto error;
    }
    return true;
error:
    PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_establish: establish fail");
    return false;
}


bool bap_bsrc_test_establish(uint8_t group_idx, uint8_t  retransmission_number,
                             uint32_t sdu_interval,
                             uint16_t iso_interval, uint8_t nse, uint16_t max_sdu,
                             uint16_t max_pdu, uint8_t phy, uint8_t packing,
                             uint8_t framing, uint8_t bn, uint8_t irc,
                             uint8_t pto, uint8_t *broadcast_code)
{
    T_BIG_MGR_ISOC_BROADCASTER_CREATE_BIG_TEST_PARAM big_param;
    T_BSRC_BASE *p_bs_src_base = NULL;
    uint8_t bis_num;

    if (p_bs_src_db == NULL || broadcast_code == NULL)
    {
        return false;
    }
    p_bs_src_base = find_src_base_by_grp_idx(group_idx);
    if (p_bs_src_base == NULL || p_bs_src_base->handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_test_establish: p_bs_src_base is NULL or handle is NULL");
        return false;
    }
    memset(&big_param, 0, sizeof(T_BIG_MGR_ISOC_BROADCASTER_CREATE_BIG_TEST_PARAM));

    if (!base_data_get_bis_num(group_idx, &bis_num))
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_test_establish: can't find bis num");
        return false;
    }

    big_param.num_bis = bis_num;
    big_param.sdu_interval = sdu_interval;
    big_param.iso_interval = iso_interval;
    big_param.nse = nse;
    big_param.max_sdu = max_sdu;
    big_param.max_pdu = max_pdu;
    big_param.phy = phy;
    big_param.packing = packing;
    big_param.framing = framing;
    big_param.bn = bn;
    big_param.irc = irc;
    big_param.pto = pto;

    if (memcmp(big_param.broadcast_code, broadcast_code, 16) == 0)
    {
        big_param.encryption = false;
    }
    else
    {
        memcpy(big_param.broadcast_code, broadcast_code, 16);
        big_param.encryption = true;
    }
#if (LE_AUDIO_BROADCAST_ASSISTANT_ROLE)
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(p_bs_src_base->rand_addr,
                                                              p_bs_src_base->local_bd_type,
                                                              p_bs_src_base->adv_sid, p_bs_src_base->broadcast_id);
    if (p_bc_source && p_bc_source->local_src)
    {
        memcpy(p_bc_source->broadcast_code, broadcast_code, 16);
    }
#endif

    if (broadcast_source_establish_test_mode(p_bs_src_base->handle, big_param) == false)
    {
        goto error;
    }
    return true;
error:
    PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_test_establish: establish fail");
    return false;
}

bool bap_bsrc_disable(uint8_t group_idx, uint8_t reason)
{
    T_BSRC_BASE *p_bs_src_base = NULL;
    if (p_bs_src_db == NULL)
    {
        return false;
    }
    p_bs_src_base = find_src_base_by_grp_idx(group_idx);
    if (p_bs_src_base == NULL || p_bs_src_base->handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_disable: p_bs_src_base is NULL or handle is NULL");
        return false;
    }

    return broadcast_source_disable(p_bs_src_base->handle, reason);
}

bool bap_bsrc_release(uint8_t group_idx)
{
    T_BSRC_BASE *p_bs_src_base = NULL;
    if (p_bs_src_db == NULL)
    {
        return false;
    }
    p_bs_src_base = find_src_base_by_grp_idx(group_idx);
    if (p_bs_src_base == NULL || p_bs_src_base->handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BS]bap_bsrc_release: p_bs_src_base is NULL or handle is NULL");
        return false;
    }

    return broadcast_source_release(p_bs_src_base->handle);
}

#endif

#if (LE_AUDIO_BROADCAST_ASSISTANT_ROLE | LE_AUDIO_BROADCAST_SINK_ROLE)
T_BAP_BC_SRC_INFO *bap_ba_bsnk_find_bst_dev(uint8_t *bd_addr, uint8_t bd_type,
                                            uint8_t adv_sid, uint8_t broadcast_id[3])
{
    uint8_t i;
    T_BAP_BC_SRC_INFO *p_src_info;
    for (i = 0; i < p_ba_bsnk_db->bc_src_queue.count; i++)
    {
        p_src_info = (T_BAP_BC_SRC_INFO *)os_queue_peek(&p_ba_bsnk_db->bc_src_queue, i);

        if (p_src_info->advertiser_sid == adv_sid &&
            memcmp(p_src_info->broadcast_id, broadcast_id, 3) == 0)
        {
            if (memcmp(bd_addr, p_src_info->adv_addr, GAP_BD_ADDR_LEN) ||
                p_src_info->adv_addr_type != bd_type)
            {
                PROTOCOL_PRINT_WARN0("[BAP]bap_ba_bsnk_find_bst_dev address or type miss-match");
            }
            return p_src_info;
        }
    }

    PROTOCOL_PRINT_WARN0("[BAP]bap_ba_bsnk_find_bst_dev address can't find broadcast src");
    return NULL;
}

static T_BAP_BC_SRC_INFO *bap_ba_bsnk_find_bst_dev_by_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    uint8_t i;
    if (sync_handle == NULL)
    {
        return NULL;
    }

    T_BAP_BC_SRC_INFO *src;
    for (i = 0; i < p_ba_bsnk_db->bc_src_queue.count; i++)
    {
        src = (T_BAP_BC_SRC_INFO *)os_queue_peek(&p_ba_bsnk_db->bc_src_queue, i);
        if (src->sync_handle == sync_handle)
        {
            return src;
        }
    }

    return NULL;
}

void bap_ba_bsnk_mgr_clear_src_dev_list(void)
{
    T_BAP_BC_SRC_INFO *src = NULL;
    T_BAP_BC_SRC_INFO *selected_src = NULL;
    T_OS_QUEUE temp_bc_src_queue;
    os_queue_init(&temp_bc_src_queue);

    while ((src = os_queue_out(&p_ba_bsnk_db->bc_src_queue)) != NULL)
    {
        if (p_ba_bsnk_db->bc_src_selected == src)
        {
            selected_src = src;
        }
        if (p_ba_bsnk_db->bc_src_selected == src ||
            src->sync_handle != NULL ||
            src->local_src)
        {
            os_queue_in(&temp_bc_src_queue, src);
            continue;
        }

        if (src->p_mapping)
        {
            base_data_free(src->p_mapping);
        }
        src->p_mapping = NULL;
        free(src);
    }

    p_ba_bsnk_db->bc_src_selected = selected_src;
    p_ba_bsnk_db->bc_src_queue = temp_bc_src_queue;
}

bool bap_ba_bsnk_add_src_dev(uint8_t *bd_addr, uint8_t bd_type,
                             uint8_t adv_sid, uint8_t broadcast_id[3],
                             bool local_src)
{
    if (bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id) != NULL)
    {
        return true;
    }
    T_BAP_BC_SRC_INFO *src;
    src = calloc(1, sizeof(T_BAP_BC_SRC_INFO));
    if (src == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP]bap_ba_bsnk_add_src_dev alloc device fail!");
        return false;
    }
    memcpy(src->broadcast_id, broadcast_id, BROADCAST_ID_LEN);
    memcpy(src->adv_addr, bd_addr, GAP_BD_ADDR_LEN);
    src->adv_addr_type = bd_type;
    src->advertiser_sid = adv_sid;
    src->local_src = local_src;
    os_queue_in(&p_ba_bsnk_db->bc_src_queue, src);
    return true;
}

bool bap_ba_bsnk_rmv_src_dev(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                             uint8_t broadcast_id[3])
{
    T_BAP_BC_SRC_INFO *p_src_info;
    if ((p_src_info = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id)) != NULL)
    {
        os_queue_delete(&p_ba_bsnk_db->bc_src_queue, p_src_info);
        if (p_src_info->p_mapping)
        {
            base_data_free(p_src_info->p_mapping);
        }
        free(p_src_info);
        return true;
    }
    return false;
}

static void  bap_ba_bsnk_scan_callback(uint8_t cb_type, void *result)
{
    return;
}

static void bap_ba_bsnk_sync_cb(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t cb_type, void *p_cb_data)
{
    T_BLE_AUDIO_SYNC_CB_DATA *p_sync_cb = (T_BLE_AUDIO_SYNC_CB_DATA *)p_cb_data;
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev_by_handle(handle);
    if (p_bc_source == NULL)
    {
        return;
    }

    switch (cb_type)
    {
    case MSG_BLE_AUDIO_PA_SYNC_STATE:
        {
            PROTOCOL_PRINT_TRACE3("MSG_BLE_AUDIO_PA_SYNC_STATE: sync_state %d, action %d, cause 0x%x\r\n",
                                  p_sync_cb->p_pa_sync_state->sync_state,
                                  p_sync_cb->p_pa_sync_state->action,
                                  p_sync_cb->p_pa_sync_state->cause);
            p_bc_source->pa_sync_state = p_sync_cb->p_pa_sync_state->sync_state;
            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED ||
                p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATING)
            {
                p_bc_source->peri_sync_id = 0;
                p_bc_source->peri_sync_handle = 0;
            }
            else if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_WAIT_SCANNING)
            {
                if (p_bc_source->scan_client_id < 1)
                {
                    p_bc_source->scan_client_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, GAP_SCAN_FILTER_ANY,
                                                                          bap_ba_bsnk_scan_callback);
                }
            }
            else if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED ||
                     p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)
            {
                if (p_bc_source->scan_client_id > 0)
                {
                    bt_adap_stop_discovery(p_bc_source->scan_client_id);
                    p_bc_source->scan_client_id = 0;
                }
            }
            le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_PA_SYNC_STATE, p_bc_source);
            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)
            {
                if (p_bc_source->pa_sync_mask == BAP_BA_BSNK_SYNC_BSRC_NONE &&
                    p_bc_source->big_sync_mask == BAP_BA_BSNK_SYNC_BSRC_NONE)
                {
                    ble_audio_sync_release(&p_bc_source->sync_handle);
                }
            }
        }
        break;

    case MSG_BLE_AUDIO_PA_REPORT_INFO:
        {
            T_BASE_DATA_MAPPING *p_mapping;
            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
            {
                p_bc_source->peri_sync_id = p_sync_cb->p_le_periodic_adv_report_info->sync_id;
                p_bc_source->peri_sync_handle = p_sync_cb->p_le_periodic_adv_report_info->sync_handle;
            }
            if (p_bc_source->p_mapping == NULL)
            {
                p_bc_source->p_mapping = base_data_parse_data(p_sync_cb->p_le_periodic_adv_report_info->data_len,
                                                              p_sync_cb->p_le_periodic_adv_report_info->p_data);
                le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_PA_INFO_UPDATE_REPORT, p_bc_source);
            }
            else
            {
                p_mapping = base_data_parse_data(p_sync_cb->p_le_periodic_adv_report_info->data_len,
                                                 p_sync_cb->p_le_periodic_adv_report_info->p_data);
                if (!base_data_cmp(p_bc_source->p_mapping, p_mapping))
                {
                    base_data_free(p_bc_source->p_mapping);
                    p_bc_source->p_mapping = p_mapping;
                    le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_PA_INFO_UPDATE_REPORT, p_bc_source);
                }
                else
                {
                    base_data_free(p_mapping);
                }


                if (p_ba_bsnk_db->bc_src_selected != NULL &&
                    p_ba_bsnk_db->bc_src_selected != p_bc_source)
                {
                    APP_PRINT_INFO2("MSG_BLE_AUDIO_PA_REPORT_INFO select %b source %b",
                                    TRACE_BDADDR(p_ba_bsnk_db->bc_src_selected->adv_addr),
                                    TRACE_BDADDR(p_bc_source->adv_addr));
                    bap_bsnk_pa_terminate(p_bc_source->adv_addr, p_bc_source->adv_addr_type,
                                          p_bc_source->advertiser_sid, p_bc_source->broadcast_id);
                }
            }
        }
        break;

    case MSG_BLE_AUDIO_PA_BIGINFO:
        {
            PROTOCOL_PRINT_TRACE1("MSG_BLE_AUDIO_PA_BIGINFO: num_bis %d\r\n",
                                  p_sync_cb->p_le_biginfo_adv_report_info->num_bis);
            if (memcmp(&p_bc_source->big_info, p_sync_cb->p_le_biginfo_adv_report_info,
                       sizeof(T_LE_BIGINFO_ADV_REPORT_INFO)) != 0)
            {
                memcpy(&p_bc_source->big_info, p_sync_cb->p_le_biginfo_adv_report_info,
                       sizeof(T_LE_BIGINFO_ADV_REPORT_INFO));
                le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_BIGINFO_UPDATE_REPORT, p_bc_source);
            }
        }
        break;

    case MSG_BLE_AUDIO_BIG_SYNC_STATE:
        {
            PROTOCOL_PRINT_TRACE3("MSG_BLE_AUDIO_BIG_SYNC_STATE: sync_state %d, action %d, cause 0x%x\r\n",
                                  p_sync_cb->p_big_sync_state->sync_state,
                                  p_sync_cb->p_big_sync_state->action,
                                  p_sync_cb->p_big_sync_state->cause);
            p_bc_source->big_sync_state = p_sync_cb->p_big_sync_state->sync_state;
            if (p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED ||
                p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATING)
            {
            }
            else if (p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
            {

            }
            le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_BIG_SYNC_STATE, p_bc_source);
            if (p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED)
            {
                if (p_bc_source->pa_sync_mask == BAP_BA_BSNK_SYNC_BSRC_NONE &&
                    p_bc_source->big_sync_mask == BAP_BA_BSNK_SYNC_BSRC_NONE)
                {
                    ble_audio_sync_release(&p_bc_source->sync_handle);
                }
            }
        }
        break;
#if (LE_AUDIO_BROADCAST_SINK_ROLE)
    case MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH:
        {
            uint8_t codec_id[5] = {TRANSPARENT_CODEC_ID, 0, 0, 0, 0};
            T_BAP_BC_SRC_SETUP_PATH_CMPLT bap_bc_setup_cmplt;
            //FIX TODO the direction shall be input
            PROTOCOL_PRINT_TRACE2("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH: bis_idx %d, cause 0x%x\r\n",
                                  p_sync_cb->p_setup_data_path->bis_idx,
                                  p_sync_cb->p_setup_data_path->cause);
            if (p_sync_cb->p_setup_data_path->cause == GAP_SUCCESS)
            {
                p_bc_source->iso_bis[p_bc_source->iso_num_bis - 1] = 0;
            }
            p_bc_source->iso_num_bis--;
            if (p_bc_source->iso_num_bis > 0)
            {
                ble_audio_bis_setup_data_path(p_bc_source->sync_handle,
                                              p_bc_source->iso_bis[p_bc_source->iso_num_bis - 1],
                                              codec_id, 0, 0, NULL);
            }
            bap_bc_setup_cmplt.p_bc_source = p_bc_source;
            bap_bc_setup_cmplt.bis_idx = p_sync_cb->p_setup_data_path->bis_idx;
            bap_bc_setup_cmplt.bis_conn_handle = p_sync_cb->p_setup_data_path->bis_conn_handle;
            bap_bc_setup_cmplt.cause = p_sync_cb->p_setup_data_path->cause;

            le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_SETUP_DATA_PATH_CMPLT, &bap_bc_setup_cmplt);
        }
        break;
    case MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH:
        {
            PROTOCOL_PRINT_TRACE2("MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH: bis_idx %d, cause 0x%x\r\n",
                                  p_sync_cb->p_remove_data_path->bis_idx,
                                  p_sync_cb->p_remove_data_path->cause);
            T_BAP_BC_SRC_RMV_PATH_CMPLT bap_bc_rmv_cmplt;
            if (p_sync_cb->p_remove_data_path->cause == GAP_SUCCESS)
            {
                p_bc_source->iso_bis[p_bc_source->iso_num_bis - 1] = 0;
            }
            p_bc_source->iso_num_bis--;
            if (p_bc_source->iso_num_bis > 0)
            {
                ble_audio_bis_remove_data_path(p_bc_source->sync_handle,
                                               p_bc_source->iso_bis[p_bc_source->iso_num_bis - 1]);
            }
            bap_bc_rmv_cmplt.p_bc_source = p_bc_source;
            bap_bc_rmv_cmplt.bis_idx = p_sync_cb->p_remove_data_path->bis_idx;
            bap_bc_rmv_cmplt.bis_conn_handle = p_sync_cb->p_remove_data_path->bis_conn_handle;
            bap_bc_rmv_cmplt.cause = p_sync_cb->p_remove_data_path->cause;

            le_broadcast_mgr_handle_msg(LE_AUDIO_MSG_BAP_REMOVE_DATA_PATH_CMPLT, &bap_bc_rmv_cmplt);
        }
        break;
#endif
    default:
        break;
    }
    return;
}

void bap_ba_bsnk_set_broadcast_code(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                                    uint8_t *code, uint8_t broadcast_id[3])
{
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);

    if (p_bc_source != NULL)
    {
        if (code != NULL)
        {
            memcpy(p_bc_source->broadcast_code, code, BROADCAST_CODE_LEN);
        }
    }
}


//Skip should be set to avoid event flush
bool bap_bsnk_pa_sync(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3],
                      uint16_t skip)
{
    uint8_t options = 0;    //For now we just support direct PA sync
    uint8_t sync_cte_type = 0;
    uint16_t sync_timeout = 1000;
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);

    if (p_bc_source == NULL)
    {
        PROTOCOL_PRINT_ERROR0("bap_bsnk_pa_sync can't find broadcast src !");
        return false;
    }
    PROTOCOL_PRINT_INFO1("bap_bsnk_pa_sync pa sync state %d", p_bc_source->pa_sync_state);

    if ((p_bc_source->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
        || (p_bc_source->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING))
    {
        return true;
    }

    if (p_bc_source->sync_handle == NULL)
    {
        p_bc_source->sync_handle = ble_audio_sync_allocate(bap_ba_bsnk_sync_cb,
                                                           p_bc_source->adv_addr_type,
                                                           p_bc_source->adv_addr, p_bc_source->advertiser_sid,
                                                           p_bc_source->broadcast_id);
    }
    if (p_bc_source->sync_handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("bap_bsnk_pa_sync create sync fail !");
        return false;
    }

    if (ble_audio_pa_sync_establish(p_bc_source->sync_handle,
                                    options, sync_cte_type, skip, sync_timeout) == false)
    {
        PROTOCOL_PRINT_ERROR0("bap_bsnk_pa_sync pa sync fail !");
        return false;
    }
    return true;

}

bool bap_bsnk_pa_terminate(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                           uint8_t broadcast_id[3])
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);

    if (p_bc_source == NULL || p_bc_source->sync_handle == NULL)
    {
        PROTOCOL_PRINT_ERROR0("bap_bsnk_pa_terminate pa sync handle is NULL !");
        return false;
    }

    if (p_bc_source->scan_client_id > 0)
    {
        bt_adap_stop_discovery(p_bc_source->scan_client_id);
        p_bc_source->scan_client_id = 0;
    }

    return ble_audio_pa_terminate(p_bc_source->sync_handle);
}

bool bap_bsnk_pa_sync_release(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                              uint8_t broadcast_id[3])
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);

    if (p_bc_source == NULL || p_bc_source->sync_handle == NULL)
    {
        return false;
    }

    if (p_bc_source->scan_client_id > 0)
    {
        bt_adap_stop_discovery(p_bc_source->scan_client_id);
        p_bc_source->scan_client_id = 0;
    }

    if (p_bc_source->pa_sync_mask != BAP_BA_BSNK_SYNC_BSRC_NONE ||
        p_bc_source->big_sync_mask != BAP_BA_BSNK_SYNC_BSRC_NONE)
    {
        PROTOCOL_PRINT_WARN2("[BAP][BSK]bap_bsnk_pa_sync_release someone needs this src pa 0x%x, big 0x%x",
                             p_bc_source->pa_sync_mask, p_bc_source->big_sync_mask);
        return true;
    }

    ble_audio_sync_release(&p_bc_source->sync_handle);
    return true;
}

bool bap_bsnk_sync_big_establish(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                                 uint8_t broadcast_id[3],
                                 uint8_t num_bis, uint8_t *p_bis)
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM sync_param;
    memset(&sync_param, 0, sizeof(T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM));
    if (p_bc_source == NULL || p_bc_source->sync_handle == NULL)
    {
        return false;
    }

    if (p_bc_source->pa_sync_state != GAP_PA_SYNC_STATE_SYNCHRONIZED)
    {
        PROTOCOL_PRINT_ERROR1("[BAP][BSK]bap_bsnk_sync_big_establish pa not synced, state : 0x%x",
                              p_bc_source->pa_sync_state);
        return false;
    }

    if (p_bc_source->big_info.encryption == 0x01 &&
        memcmp(p_bc_source->broadcast_code, sync_param.broadcast_code, BROADCAST_CODE_LEN) == 0)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BSK]bap_bsnk_sync_big_establish encryption, but broadcast code is 0");
        return false;
    }
    sync_param.encryption = p_bc_source->big_info.encryption;
    memcpy(sync_param.broadcast_code, p_bc_source->broadcast_code, BROADCAST_CODE_LEN);
    sync_param.mse = 0;
    //FIX TODO set proper sync timeout
    sync_param.big_sync_timeout = (p_bc_source->big_info.iso_interval * 15 + 100) / 10;
    if (num_bis > GAP_BIG_MGR_MAX_BIS_NUM)
    {
        PROTOCOL_PRINT_WARN1("[BAP][BSK]bap_bsnk_sync_big_establish num_bis is exceed limitation %d",
                             num_bis);
        sync_param.num_bis = GAP_BIG_MGR_MAX_BIS_NUM;
    }
    else
    {
        sync_param.num_bis = num_bis;
    }

    memcpy(sync_param.bis, p_bis, sync_param.num_bis);

    return ble_audio_big_sync_establish(p_bc_source->sync_handle, &sync_param);
}

bool bap_bsnk_sync_big_terminate(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                                 uint8_t broadcast_id[3])
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    if (p_bc_source == NULL || p_bc_source->sync_handle == NULL)
    {
        return false;
    }

    if (p_bc_source->big_sync_state != BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
    {
        PROTOCOL_PRINT_ERROR1("[BAP][BSK]bap_bsnk_sync_big_terminate big not synced, state : 0x%x",
                              p_bc_source->big_sync_state);
        return false;
    }

    return ble_audio_big_terminate(p_bc_source->sync_handle);
}

#if (LE_AUDIO_BROADCAST_SINK_ROLE)
bool bap_bsnk_setup_data_path(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                              uint8_t broadcast_id[3],
                              uint8_t num_bis, uint8_t *p_bis)
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    uint8_t codec_id[5] = {TRANSPARENT_CODEC_ID, 0, 0, 0, 0};
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    if (p_bc_source == NULL || p_bc_source->sync_handle == NULL)
    {
        return false;
    }

    if (p_bc_source->big_sync_state != BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
    {
        PROTOCOL_PRINT_ERROR1("[BAP][BSK]bap_bsnk_setup_data_path big not synced, state : 0x%x",
                              p_bc_source->big_sync_state);
        return false;
    }

    if (p_bc_source->iso_num_bis != 0)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BSK]bap_bsnk_setup_data_path setup or remove is processing");
        return false;
    }

    if (num_bis > GAP_BIG_MGR_MAX_BIS_NUM)
    {
        PROTOCOL_PRINT_WARN1("[BAP][BSK]bap_bsnk_setup_data_path num_bis is exceed limitation %d", num_bis);
        p_bc_source->iso_num_bis = GAP_BIG_MGR_MAX_BIS_NUM;
    }
    else
    {
        p_bc_source->iso_num_bis = num_bis;
    }
    memset(p_bc_source->iso_bis, 0, GAP_BIG_MGR_MAX_BIS_NUM);
    memcpy(p_bc_source->iso_bis, p_bis, p_bc_source->iso_num_bis);

    while (p_bc_source->iso_num_bis > 0 &&
           !ble_audio_bis_setup_data_path(p_bc_source->sync_handle,
                                          p_bc_source->iso_bis[p_bc_source->iso_num_bis - 1],
                                          codec_id, 0, 0, NULL))
    {
        p_bc_source->iso_num_bis--;
    }

    if (p_bc_source->iso_num_bis > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bap_bsnk_remove_data_path(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                               uint8_t broadcast_id[3],
                               uint8_t num_bis, uint8_t *p_bis)
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    if (p_bc_source == NULL || p_bc_source->sync_handle == NULL)
    {
        return false;
    }

    if (p_bc_source->big_sync_state != BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
    {
        PROTOCOL_PRINT_ERROR1("[BAP][BSK]bap_bsnk_remove_data_path big not synced, state : 0x%x",
                              p_bc_source->big_sync_state);
        return false;
    }

    if (p_bc_source->iso_num_bis != 0)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BSK]bap_bsnk_setup_data_path setup or remove is processing");
        return false;
    }

    if (num_bis > GAP_BIG_MGR_MAX_BIS_NUM)
    {
        PROTOCOL_PRINT_WARN1("[BAP][BSK]bap_bsnk_remove_data_path num_bis is exceed limitation %d",
                             num_bis);
        p_bc_source->iso_num_bis = GAP_BIG_MGR_MAX_BIS_NUM;
    }
    else
    {
        p_bc_source->iso_num_bis = num_bis;
    }
    memset(p_bc_source->iso_bis, 0, GAP_BIG_MGR_MAX_BIS_NUM);
    memcpy(p_bc_source->iso_bis, p_bis, p_bc_source->iso_num_bis);

    while (p_bc_source->iso_num_bis > 0 &&
           !ble_audio_bis_remove_data_path(p_bc_source->sync_handle,
                                           p_bc_source->iso_bis[p_bc_source->iso_num_bis - 1]))
    {
        p_bc_source->iso_num_bis--;
    }

    if (p_bc_source->iso_num_bis > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

#endif


#if (LE_AUDIO_BROADCAST_ASSISTANT_ROLE)
void bap_ba_sync_broadcast_src(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                               uint8_t broadcast_id[3], uint8_t *code)
{
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);

    if (p_bc_source == NULL)
    {
        PROTOCOL_PRINT_ERROR1("[BAP][BA]bap_ba_sync_broadcast_src can not find broadcast src adv_sid : %d",
                              adv_sid);
        return;
    }

    p_bc_source->pa_sync_mask |= BAP_BA_PA_SYNC_BSRC_MSK;
    APP_PRINT_INFO2("bap_ba_sync_broadcast_src selected %x source %x", p_ba_bsnk_db->bc_src_selected,
                    p_bc_source);
    if (p_ba_bsnk_db->bc_src_selected != p_bc_source)
    {
        if (p_ba_bsnk_db->bc_src_selected != NULL)
        {
            p_ba_bsnk_db->bc_src_selected->selected = false;
            p_ba_bsnk_db->bc_src_selected->pa_sync_mask &= ~BAP_BA_PA_SYNC_BSRC_MSK;
            APP_PRINT_INFO4("bap_ba_sync_broadcast_src sync_mask %x, sync_state %x, selected %b source %b",
                            p_ba_bsnk_db->bc_src_selected->pa_sync_mask,
                            p_ba_bsnk_db->bc_src_selected->pa_sync_state,
                            TRACE_BDADDR(p_ba_bsnk_db->bc_src_selected->adv_addr),
                            TRACE_BDADDR(p_bc_source->adv_addr));
            if (p_ba_bsnk_db->bc_src_selected->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
            {
                if (p_ba_bsnk_db->bc_src_selected->pa_sync_mask == BAP_BA_BSNK_SYNC_BSRC_NONE)
                {
                    bap_bsnk_pa_terminate(p_ba_bsnk_db->bc_src_selected->adv_addr,
                                          p_ba_bsnk_db->bc_src_selected->adv_addr_type,
                                          p_ba_bsnk_db->bc_src_selected->advertiser_sid,
                                          p_ba_bsnk_db->bc_src_selected->broadcast_id);
                }
            }
        }

        p_ba_bsnk_db->bc_src_selected = p_bc_source;
        p_ba_bsnk_db->bc_src_selected->selected = true;
        if (code != NULL)
        {
            memcpy(p_bc_source->broadcast_code, code, BROADCAST_CODE_LEN);
        }
        bap_bsnk_pa_sync(bd_addr, bd_type, adv_sid, broadcast_id, 5);
    }
    else
    {
        memcpy(p_bc_source->broadcast_code, code, BROADCAST_CODE_LEN);
        if (p_ba_bsnk_db->bc_src_selected->pa_sync_state == GAP_PA_SYNC_STATE_TERMINATED)
        {
            bap_bsnk_pa_sync(bd_addr, bd_type, adv_sid, broadcast_id, 5);
        }
    }
}


#if (LE_AUDIO_BASS_CLIENT_SUPPORT)
bool bap_ba_remote_scanning(uint16_t conn_handle, bool scan_enable)
{
    if (scan_enable)
    {
        return bass_cp_remote_scan_start(conn_handle, true);
    }
    else
    {
        return bass_cp_remote_scan_stop(conn_handle, true);
    }
}

#if LE_AUDIO_BROADCAST_SOURCE_ROLE
bool bap_ba_add_local_src(uint16_t conn_handle, uint8_t group_idx, T_BASS_PA_SYNC pa_sync,
                          uint32_t bis_array)
{

    PROTOCOL_PRINT_INFO2("[BAP][BA]bap_ba_add_local_src pa_sync 0x%x, bis_array 0x%x", pa_sync,
                         bis_array);
    uint8_t err_idx = 0;
    uint32_t bis_array_temp = 0;
    uint16_t meta_data_len = 0;
    uint8_t i;
    T_BASS_CP_BIS_INFO *p_cp_bis_info = NULL;
    T_BASE_DATA_MAPPING *p_mapping;
    uint8_t *p_meta_data = NULL;

    T_BSRC_BASE *p_bs_src_base = NULL;
    if (p_bs_src_db == NULL)
    {
        return false;
    }
    p_bs_src_base = find_src_base_by_grp_idx(group_idx);

    if (!p_bs_src_base)
    {
        return false;
    }

    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(p_bs_src_base->rand_addr,
                                                              p_bs_src_base->local_bd_type,
                                                              p_bs_src_base->adv_sid,
                                                              p_bs_src_base->broadcast_id);
    if (p_bc_source == NULL ||
        p_bc_source->p_mapping == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_add_local_src no selected src device or mapping");

        return false;
    }

    p_mapping = p_bc_source->p_mapping;

    T_BASS_CP_ADD_SOURCE cp_data;
    memset(&cp_data, 0, sizeof(T_BASS_CP_ADD_SOURCE));
    cp_data.advertiser_address_type = p_bc_source->adv_addr_type;
    memcpy(cp_data.advertiser_address, p_bc_source->adv_addr, BD_ADDR_LEN);
    cp_data.advertiser_sid = p_bc_source->advertiser_sid;
    memcpy(cp_data.broadcast_id, p_bc_source->broadcast_id, 3);
    cp_data.num_subgroups = p_mapping->num_subgroups;
    cp_data.pa_sync = pa_sync;
    cp_data.pa_interval = BASS_PA_INTERVAL_UNKNOWN;

    for (i = 0; i < p_mapping->num_subgroups; i++)
    {
        if (p_mapping->p_subgroup[i].bis_array & bis_array)
        {
            bis_array_temp |= (p_mapping->p_subgroup[i].bis_array & bis_array);
        }
        meta_data_len += p_mapping->p_subgroup[i].metadata_len;
    }
    if (bis_array != BASS_CP_BIS_SYNC_NO_PREFER && bis_array != bis_array_temp)
    {
        err_idx = 1;
        goto error;
    }
    cp_data.bis_info_size = p_mapping->num_subgroups * sizeof(T_BASS_CP_BIS_INFO) + meta_data_len;
    p_cp_bis_info = calloc(1, cp_data.bis_info_size);
    if (p_cp_bis_info == NULL)
    {
        err_idx = 2;
        goto error;
    }

    p_meta_data = (uint8_t *)(p_cp_bis_info + p_mapping->num_subgroups);
    for (i = 0; i < p_mapping->num_subgroups; i++)
    {
        if (bis_array == BASS_CP_BIS_SYNC_NO_PREFER)
        {
            p_cp_bis_info[i].bis_sync = BASS_CP_BIS_SYNC_NO_PREFER;
        }
        else
        {
            p_cp_bis_info[i].bis_sync = (p_mapping->p_subgroup[i].bis_array & bis_array);
        }
        p_cp_bis_info[i].metadata_len = p_mapping->p_subgroup[i].metadata_len;
        if (p_cp_bis_info[i].metadata_len > 0)
        {
            memcpy(p_meta_data, p_mapping->p_subgroup[i].p_metadata, p_cp_bis_info[i].metadata_len);
            p_cp_bis_info[i].p_metadata = p_meta_data;
            p_meta_data += p_cp_bis_info->metadata_len;
        }
    }
    cp_data.p_cp_bis_info = p_cp_bis_info;
    cp_data.num_subgroups = p_mapping->num_subgroups;

    if (!bass_cp_add_source(conn_handle, &cp_data, true))
    {
        err_idx = 3;
        goto error;
    }
    return true;
error:
    if (p_cp_bis_info)
    {
        free(p_cp_bis_info);
    }
    PROTOCOL_PRINT_ERROR4("bap_ba_add_local_src: failed, conn_handle 0x%x, pa_sync %d, bis_array 0x%x, err_idx %d",
                          conn_handle, pa_sync, bis_array, err_idx);
    return false;
}

bool bap_ba_modify_local_src(uint16_t conn_handle, uint8_t group_idx, uint8_t source_id,
                             T_BASS_PA_SYNC pa_sync, uint32_t bis_array)
{
    uint8_t err_idx = 0;
    uint32_t bis_array_temp = 0;
    uint16_t meta_data_len = 0;
    uint8_t i;
    T_BASS_CP_BIS_INFO *p_cp_bis_info = NULL;
    T_BASE_DATA_MAPPING *p_mapping;
    T_BSRC_BASE *p_bs_src_base = NULL;
    uint8_t *p_meta_data = NULL;
    if (p_bs_src_db == NULL)
    {
        return false;
    }
    p_bs_src_base = find_src_base_by_grp_idx(group_idx);

    if (!p_bs_src_base)
    {
        return false;
    }

    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(p_bs_src_base->rand_addr,
                                                              p_bs_src_base->local_bd_type,
                                                              p_bs_src_base->adv_sid,
                                                              p_bs_src_base->broadcast_id);
    if (p_bc_source == NULL ||
        p_bc_source->p_mapping == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_modify_local_src no selected src device or mapping");

        return false;
    }

    p_mapping = p_bc_source->p_mapping;

    T_BASS_CP_MODIFY_SOURCE cp_data;
    memset(&cp_data, 0, sizeof(T_BASS_CP_MODIFY_SOURCE));
    cp_data.source_id = source_id;
    cp_data.num_subgroups = p_mapping->num_subgroups;
    cp_data.pa_sync = pa_sync;
    cp_data.pa_interval = BASS_PA_INTERVAL_UNKNOWN;

    for (i = 0; i < p_mapping->num_subgroups; i++)
    {
        if (p_mapping->p_subgroup[i].bis_array & bis_array)
        {
            bis_array_temp |= (p_mapping->p_subgroup[i].bis_array & bis_array);
        }
        meta_data_len += p_mapping->p_subgroup[i].metadata_len;
    }
    if (bis_array != BASS_CP_BIS_SYNC_NO_PREFER && bis_array != bis_array_temp)
    {
        err_idx = 1;
        goto error;
    }
    cp_data.bis_info_size = p_mapping->num_subgroups * sizeof(T_BASS_CP_BIS_INFO) + meta_data_len;
    p_cp_bis_info = calloc(1, cp_data.bis_info_size);
    if (p_cp_bis_info == NULL)
    {
        err_idx = 2;
        goto error;
    }

    p_meta_data = (uint8_t *)(p_cp_bis_info + p_mapping->num_subgroups);
    for (i = 0; i < p_mapping->num_subgroups; i++)
    {
        if (bis_array == BASS_CP_BIS_SYNC_NO_PREFER)
        {
            p_cp_bis_info[i].bis_sync = BASS_CP_BIS_SYNC_NO_PREFER;
        }
        else
        {
            p_cp_bis_info[i].bis_sync = (p_mapping->p_subgroup[i].bis_array & bis_array);
        }
        p_cp_bis_info[i].metadata_len = p_mapping->p_subgroup[i].metadata_len;
        if (p_cp_bis_info[i].metadata_len > 0)
        {
            memcpy(p_meta_data, p_mapping->p_subgroup[i].p_metadata,
                   p_cp_bis_info[i].metadata_len);
            p_cp_bis_info[i].p_metadata = p_meta_data;
            p_meta_data += p_cp_bis_info->metadata_len;
        }
    }
    cp_data.p_cp_bis_info = p_cp_bis_info;
    cp_data.num_subgroups = p_mapping->num_subgroups;

    if (!bass_cp_modify_source(conn_handle, &cp_data, true))
    {
        err_idx = 3;
        PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_modify_local_src fail");
        goto error;
    }
    free(p_cp_bis_info);
    return true;
error:
    if (p_cp_bis_info)
    {
        free(p_cp_bis_info);
    }
    PROTOCOL_PRINT_ERROR4("bap_ba_modify_local_src: failed, conn_handle 0x%x, pa_sync %d, bis_array 0x%x, err_idx %d",
                          conn_handle, pa_sync, bis_array, err_idx);

    return false;
}
#endif

bool bap_ba_add_src(uint16_t conn_handle, T_BASS_PA_SYNC pa_sync, uint32_t bis_array)
{
    if (p_ba_bsnk_db->bc_src_selected == NULL ||
        p_ba_bsnk_db->bc_src_selected->p_mapping == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_add_src no selected src device or mapping");

        return false;
    }

    return bass_cp_add_source_by_sync_info(p_ba_bsnk_db->bc_src_selected->sync_handle, conn_handle,
                                           pa_sync, bis_array, true);
}

bool bap_ba_modify_src(uint16_t conn_handle, uint8_t source_id, T_BASS_PA_SYNC pa_sync,
                       uint32_t bis_array)
{
    if (p_ba_bsnk_db->bc_src_selected == NULL ||
        p_ba_bsnk_db->bc_src_selected->p_mapping == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_modify_src no selected src device or mapping");

        return false;
    }

    return bass_cp_modify_source_by_sync_info(p_ba_bsnk_db->bc_src_selected->sync_handle, conn_handle,
                                              source_id, pa_sync, bis_array, true);
}

bool bap_ba_set_broadcast_code(uint16_t conn_handle, uint8_t *bd_addr, uint8_t bd_type,
                               uint8_t adv_sid,
                               uint8_t broadcast_id[3],
                               uint8_t source_id, uint8_t *code)
{
    T_BASS_CP_SET_BROADCAST_CODE cp_data;
    cp_data.source_id = source_id;
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    if (p_bc_source == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_set_broadcast_code no src device ");

        return false;
    }
    if (code == NULL)
    {
        return false;
    }
    memcpy(p_bc_source->broadcast_code, code, BROADCAST_CODE_LEN);
    memcpy(cp_data.broadcast_code, code, BROADCAST_CODE_LEN);

    return bass_cp_set_broadcast_code(conn_handle, &cp_data, false);
}

bool bap_ba_remove_src(uint16_t conn_handle, uint8_t source_id)
{
    T_BASS_CP_REMOVE_SOURCE cp_data;
    cp_data.source_id = source_id;

    return bass_cp_remove_source(conn_handle, &cp_data, false);
}

void bap_ba_handle_bass_sync_request(T_BASS_CLIENT_SYNC_INFO_REQ *sync_req_data)
{
    T_BSRC_BASE *p_src_base = NULL;
    if (!sync_req_data || !sync_req_data->p_brs_data)
    {
        return;
    }

    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(sync_req_data->p_brs_data->source_address,
                                                              sync_req_data->p_brs_data->source_address_type,
                                                              sync_req_data->p_brs_data->source_adv_sid,
                                                              sync_req_data->p_brs_data->broadcast_id);

    if (p_bc_source == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_handle_bass_sync_request no broadcast src selected !");
        return;
    }

    if (!p_bc_source->local_src)
    {
        if (p_bc_source->pa_sync_state != GAP_PA_SYNC_STATE_SYNCHRONIZED)
        {
            PROTOCOL_PRINT_ERROR1("[BAP][BA]bap_ba_handle_bass_sync_request wrong state : %d",
                                  p_bc_source->pa_sync_state);
            return;
        }
    }
#if LE_AUDIO_BROADCAST_SOURCE_ROLE
    else
    {
        p_src_base = find_src_base_by_adv_sid(sync_req_data->p_brs_data->source_adv_sid,
                                              sync_req_data->p_brs_data->broadcast_id);
        if (p_src_base == NULL)
        {
            PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_handle_bass_sync_request can not find src base");
            return;
        }
        if (p_src_base->state != BROADCAST_SOURCE_STATE_STREAMING)
        {
            PROTOCOL_PRINT_ERROR1("[BAP][BA]bap_ba_handle_bass_sync_request src base state : %d",
                                  p_src_base->state);
            return;
        }
    }
#endif
    if (sync_req_data->p_brs_data->pa_sync_state == PA_SYNC_STATE_SYNC_INFO_REQ)
    {
        if (ble_audio_check_remote_features(sync_req_data->conn_handle,
                                            LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX3,
                                            LE_SUPPORT_FEATURES_PAST_RECIPIENT_MASK_BIT))
        {
            if (le_check_supported_features(LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX3,
                                            LE_SUPPORT_FEATURES_PAST_SENDER_MASK_BIT))
            {
                if (!p_bc_source->local_src)
                {
                    //FIX TODO we may check the adv for the broadcast src
                    T_BASS_PAST_SRV_DATA srv_data;
                    srv_data.adv_a_match_ext_adv = 0;
                    srv_data.adv_a_match_src = 0;
                    srv_data.source_id = sync_req_data->p_brs_data->source_id;
                    if (bass_past_by_remote_src(p_bc_source->sync_handle, sync_req_data->conn_handle,
                                                srv_data))
                    {
                        return;
                    }
                }
#if LE_AUDIO_BROADCAST_SOURCE_ROLE
                else
                {
                    T_BASS_PAST_SRV_DATA srv_data;
                    bool addr_known = true;
                    srv_data.adv_a_match_ext_adv = 1;
                    srv_data.adv_a_match_src = 0;
                    srv_data.source_id = sync_req_data->p_brs_data->source_id;
                    if (p_src_base->local_bd_type == BLE_ADDR_RANDOM)
                    {
                        if (!brsc_get_eadv_random_address(p_src_base->handle, p_src_base->rand_addr))
                        {
                            addr_known = false;
                        }
                    }
                    if (addr_known)
                    {
                        if (memcmp(p_src_base->rand_addr,  sync_req_data->p_brs_data->source_address, BD_ADDR_LEN) == 0 &&
                            p_src_base->local_bd_type == sync_req_data->p_brs_data->source_address_type)
                        {
                            srv_data.adv_a_match_src = 1;
                        }
                    }

                    if (!bass_past_by_local_src(p_src_base->handle, sync_req_data->conn_handle, srv_data))
                    {
                        PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_handle_bass_sync_request transfer local fail");
                        return;
                    }
                }
#endif
            }
        }
    }
}

void bap_ba_handle_bass_brs_state(T_BASS_CLIENT_BRS_DATA *brs_data)
{
    T_BASS_CP_SET_BROADCAST_CODE cp_data;

    if (!brs_data || !brs_data->p_brs_data)
    {
        return;
    }

    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(brs_data->p_brs_data->source_address,
                                                              brs_data->p_brs_data->source_address_type,
                                                              brs_data->p_brs_data->source_adv_sid,
                                                              brs_data->p_brs_data->broadcast_id);

    if (p_bc_source == NULL)
    {
        PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_handle_bass_brs_state no broadcast src selected !");
        return;
    }

    if (!p_bc_source->local_src)
    {
        if (p_bc_source->pa_sync_state != GAP_PA_SYNC_STATE_SYNCHRONIZED)
        {
            PROTOCOL_PRINT_ERROR1("[BAP][BA]bap_ba_handle_bass_brs_state selected src is not synchronized, state : 0x%x",
                                  p_bc_source->pa_sync_state);
            return;
        }
    }
#if LE_AUDIO_BROADCAST_SOURCE_ROLE
    else
    {
        T_BSRC_BASE *p_src_base = find_src_base_by_adv_sid(brs_data->p_brs_data->source_adv_sid,
                                                           brs_data->p_brs_data->broadcast_id);
        if (p_src_base == NULL)
        {
            PROTOCOL_PRINT_ERROR0("[BAP][BA]bap_ba_handle_bass_brs_state can not find src base");
            return;
        }
        if (p_src_base->state != BROADCAST_SOURCE_STATE_STREAMING)
        {
            PROTOCOL_PRINT_ERROR1("[BAP][BA]bap_ba_handle_bass_brs_state src base state : %d",
                                  p_src_base->state);
            return;
        }
    }
#endif

    //FIX TODO source may not known the broadcast code
    if (brs_data->p_brs_data->big_encryption == BIG_BROADCAST_CODE_REQUIRED)
    {
        cp_data.source_id = brs_data->p_brs_data->source_id;
        memcpy(cp_data.broadcast_code, p_bc_source->broadcast_code, BROADCAST_CODE_LEN);
        bass_cp_set_broadcast_code(brs_data->conn_handle, &cp_data, false);
    }
}

#endif
#endif
#endif

#if (LE_AUDIO_SCAN_DELEGATOR_ROLE)
bool bap_scan_delegator_gen_adv_data(uint8_t *p_data, uint8_t buf_len)
{
    uint8_t idx = 0;
    if (buf_len < 4)
    {
        return false;
    }
    memset(p_data, 0, buf_len);
    p_data[idx++] = 0x03;
    p_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
    p_data[idx++] = LO_WORD(GATT_UUID_BASS);
    p_data[idx++] = HI_WORD(GATT_UUID_BASS);
    return true;
}
#endif

bool le_broadcat_mgr_init(uint8_t role_mask)
{
#if LE_AUDIO_BROADCAST_SOURCE_ROLE
    if (role_mask & BAP_BROADCAST_SOURCE_ROLE)
    {
        p_bs_src_db = calloc(1, sizeof(T_BAP_BSRC_DB));
        if (p_bs_src_db)
        {
            os_queue_init(&p_bs_src_db->src_base_queue);
        }
        else
        {
            return false;
        }
    }
#endif

#if (LE_AUDIO_BROADCAST_ASSISTANT_ROLE | LE_AUDIO_BROADCAST_SINK_ROLE)
    if (role_mask & (BAP_BROADCAST_ASSISTANT_ROLE | BAP_BROADCAST_SINK_ROLE))
    {
        p_ba_bsnk_db = calloc(1, sizeof(T_BAP_BA_BSK_CB));
        if (p_ba_bsnk_db)
        {
            p_ba_bsnk_db->bc_src_selected = NULL;
            os_queue_init(&p_ba_bsnk_db->bc_src_queue);
        }
        else
        {
            return false;
        }
    }
#endif
    return true;
}
