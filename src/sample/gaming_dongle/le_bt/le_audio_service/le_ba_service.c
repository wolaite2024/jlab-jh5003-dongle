#include <string.h>
#include <stdlib.h>

#include "trace.h"
#include "le_ba_service.h"

#include "gap_big_mgr.h"
#include "broadcast_source_sm.h"
#include "metadata_def.h"
#include "codec_def.h"
#include "os_queue.h"
#include "codec_qos.h"
#include "app_usb_layer.h"
#include "bap.h"
#include "le_broadcast_mgr.h"
#include "ual_adapter.h"
#include "os_queue.h"
#include "dev_mgr.h"
#include "pbp_def.h"

#if UAL_CONSOLE_PRINT
#include "console.h"
#endif

#define BASS_USE_PA_INFO_SAVED         0

typedef struct
{
    uint8_t bd_addr[BD_ADDR_LEN];
    uint8_t bd_type;
    uint8_t adv_sid;
    uint8_t broadcast_id[BROADCAST_ID_LEN];
    uint8_t broadcast_code[BROADCAST_CODE_LEN];
} T_BA_SRC;

typedef struct
{
    int                 client_id;
    T_BA_SRC            ba_select_src;
    T_OS_QUEUE          bst_src_queue;
} T_BA_SERVICE_CB;

static T_BA_SERVICE_CB ba_service_cb;



#if UAL_CONSOLE_PRINT
T_BC_SRC_INFO *ba_bsnk_find_bst_dev_by_idx(uint8_t idx)
{
    T_BC_SRC_INFO *dev = (T_BC_SRC_INFO *)os_queue_peek(&ba_service_cb.bst_src_queue, idx);

    return dev;
}
#endif

static T_BRS_INFO *ba_get_bsnk_bis_synced_info(uint8_t *remote_bd_addr, uint8_t *instance_id)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(remote_bd_addr);
    T_LE_AUDIO *p_link = NULL;
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("ba_get_bsnk_bis_synced_info: can't find device record");
        return NULL;
    }

    p_link = ble_audio_find_by_conn_id(p_dev_rec->le_conn_id);
    if (p_link == NULL)
    {
        APP_PRINT_ERROR0("ba_get_bsnk_bis_synced_info: can't find device link");
        return NULL;
    }

    T_BRS_INFO *p_brs = NULL;
    for (uint8_t i = 0; i < BASS_BRS_NUM_MAX; i++)
    {
        p_brs = &(p_link->brs_cb[i]);
        if (p_brs == NULL)
        {
            APP_PRINT_ERROR1("ba_get_bsnk_bis_synced_info: p_brs %d NULL", i);
            continue;
        }

        if (p_brs->bis_sync_state)
        {
            APP_PRINT_INFO4("ba_get_bsnk_bis_synced_info: src %b, id %d,pa_state %d, sync_state %d",
                            TRACE_BDADDR(p_brs->adv_addr), i,
                            p_brs->pa_sync_state, p_brs->bis_sync_state);
            *instance_id = i;
            return p_brs;
        }
    }

    return NULL;
}

static T_BRS_INFO *ba_get_bsnk_added_src_info(uint8_t *remote_bd_addr, uint8_t *src_bd_addr)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(remote_bd_addr);
    T_LE_AUDIO *p_link = NULL;
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("ba_get_bsnk_added_src_info: can't find device record");
        return NULL;
    }

    p_link = ble_audio_find_by_conn_id(p_dev_rec->le_conn_id);
    if (p_link == NULL)
    {
        APP_PRINT_ERROR0("ba_get_bsnk_added_src_info: can't find device link");
        return NULL;
    }

    T_BRS_INFO *p_brs = NULL;
    for (uint8_t i = 0; i < BASS_BRS_NUM_MAX; i++)
    {
        p_brs = &(p_link->brs_cb[i]);
        if (p_brs == NULL)
        {
            APP_PRINT_ERROR1("ba_get_bsnk_added_src_info: p_brs %d NULL", i);
            continue;
        }

        if (!memcmp(p_brs->adv_addr, src_bd_addr, BD_ADDR_LEN))
        {
            APP_PRINT_INFO3("ba_get_bsnk_added_src_info: src %b pa_state %d sync_state %d",
                            TRACE_BDADDR(p_brs->adv_addr),
                            p_brs->pa_sync_state,
                            p_brs->bis_sync_state);
            return p_brs;
        }
    }

    return NULL;
}

static T_BC_SRC_INFO *ba_bsnk_find_bst_dev(uint8_t *bd_addr, uint8_t bd_type,
                                           uint8_t adv_sid, uint8_t broadcast_id[3])
{
    uint8_t i;
    T_BC_SRC_INFO *p_src_info;
    for (i = 0; i < ba_service_cb.bst_src_queue.count; i++)
    {
        p_src_info = (T_BC_SRC_INFO *)os_queue_peek(&ba_service_cb.bst_src_queue, i);
        if (!memcmp(bd_addr, p_src_info->adv_addr, GAP_BD_ADDR_LEN) &&
            p_src_info->adv_addr_type == bd_type &&
            p_src_info->advertiser_sid == adv_sid)
        {
            if (memcmp(p_src_info->broadcast_id, broadcast_id, 3))
            {
                memcpy(p_src_info->broadcast_id, broadcast_id, 3);
                PROTOCOL_PRINT_WARN0("ba_bsnk_find_bst_dev broadcast_id miss-match");
            }
            return p_src_info;
        }
    }
    return NULL;
}

static bool ba_bsk_add_src_dev(uint8_t *bd_addr, uint8_t bd_type,
                               uint8_t adv_sid, uint8_t broadcast_id[3])
{
    T_BC_SRC_INFO *src = NULL;
    if (ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id))
    {
        APP_PRINT_ERROR1("ba_bsk_add_src_dev: %b already added!", TRACE_BDADDR(bd_addr));
        return true;
    }
    src = calloc(1, sizeof(T_BC_SRC_INFO));
    if (src == NULL)
    {
        APP_PRINT_ERROR0("ba_bsk_add_src_dev alloc device fail!");
        return false;
    }
    memcpy(src->adv_addr, bd_addr, BD_ADDR_LEN);
    src->adv_addr_type = bd_type;
    src->advertiser_sid = adv_sid;
    memcpy(src->broadcast_id, broadcast_id, 3);
    os_queue_in(&ba_service_cb.bst_src_queue, src);
#if UAL_CONSOLE_PRINT
    uint8_t event_buff[80];
    uint16_t buf_len;
    buf_len = sprintf((char *)event_buff,
                      "Add BA RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] , type %d, sid %d\r\n",
                      bd_addr[5], bd_addr[4], bd_addr[3], bd_addr[2],
                      bd_addr[1], bd_addr[0], bd_type, adv_sid);

    console_write(event_buff, buf_len);
#endif

    return true;
}

static void ba_bsnk_mgr_clear_src_dev_list(void)
{
    T_BC_SRC_INFO *src = NULL;

    while ((src = os_queue_out(&ba_service_cb.bst_src_queue)) != NULL)
    {
        free(src);
    }
}

static void ba_service_scan_callback(uint8_t cb_type, void *result)
{
    if (cb_type == SCAN_RESULT_LE)
    {
        T_LE_ADV_INFO *p_report = (T_LE_ADV_INFO *)result;
        uint16_t pos = 0;
        uint8_t *pp;
        uint16_t length;
        uint8_t type;
        uint16_t srv_uuid;
        uint8_t *p_name_data = NULL;
        uint16_t name_data_len = 0;
        uint8_t *p_broadcast_id = NULL;

        int8_t rssi = p_report->rssi;
        uint8_t pbp_feature = 0xFF;
        uint8_t pbp_metadata_len = 0;
        uint8_t *pbp_metadata_ltv = NULL;
        uint8_t broadcast_name_len = 0;
        uint8_t *broadcast_name = NULL;

        bool report = false;

        while (pos < p_report->data_len)
        {
            /* Length of the AD structure. */
            length = p_report->p_data[pos++];

            if ((length < 1) || (length > p_report->data_len))
            {
                break;
            }

            if ((length > 0x01) && ((pos + length) <= p_report->data_len))
            {
                /* Copy the AD Data to buffer. */
                pp = p_report->p_data + pos + 1;
                /* AD Type, one octet. */
                type = p_report->p_data[pos];

                switch (type)
                {
                case GAP_ADTYPE_SERVICE_DATA:
                    {
                        LE_STREAM_TO_UINT16(srv_uuid, pp);

                        if (srv_uuid == BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID)
                        {
                            if (length == 6)
                            {
                                p_broadcast_id = pp;
                                if (!ba_bsnk_find_bst_dev(p_report->bd_addr, p_report->bd_type, p_report->adv_sid, pp))
                                {
                                    ba_add_device(p_report->bd_addr, p_report->bd_type,
                                                  p_report->adv_sid, pp);
                                    report = true;
                                }
                            }
                        }
                        else if (srv_uuid == PUBIC_BROADCAST_ANNOUNCEMENT_SRV_UUID)
                        {
                            pbp_feature = pp[0];
                            pbp_metadata_len = pp[1];
                            pbp_metadata_ltv = pp + 2;
                            APP_PRINT_INFO3("ba_service_scan_callback feature %x, len %x, ltv %b", pbp_feature,
                                            pbp_metadata_len, TRACE_BINARY(pbp_metadata_len, pbp_metadata_ltv));
                        }

                    }
                    break;
                case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                    {
                        p_name_data = pp;
                        name_data_len = length - 1;
                    }
                    break;
                case GAP_ADTYPE_BROADCAST_NAME:
                    {
                        broadcast_name_len = length - 1;
                        broadcast_name = pp;
                        APP_PRINT_INFO2("ba_service_scan_callback broadcast len %x name %b", broadcast_name_len,
                                        TRACE_BINARY(broadcast_name_len, broadcast_name));
                    }
                    break;
                default:
                    break;
                }
            }
            pos += length;
        }

        if (report)
        {
            T_BROADCAST_SRC_INFO bc_source;
            memset(&bc_source, 0, sizeof(T_BROADCAST_SRC_INFO));
            T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(p_report->bd_addr);
            if (p_dev_rec)
            {
                bc_source.adv_addr_type = p_dev_rec->bd_type;
                memcpy(bc_source.adv_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
            }
            else
            {
                bc_source.adv_addr_type = p_report->bd_type;
                memcpy(bc_source.adv_addr, p_report->bd_addr, BD_ADDR_LEN);
            }

            bc_source.rssi = rssi;
            bc_source.feature = pbp_feature;
            bc_source.pbp_len = pbp_metadata_len;
            if (pbp_metadata_len)
            {
                bc_source.pbp_ltv = calloc(1, pbp_metadata_len);
                if (bc_source.pbp_ltv == NULL)
                {
                    APP_PRINT_ERROR1("ba_service_scan_callback malloc len %x failed", pbp_metadata_len);
                    return;
                }
                memcpy(bc_source.pbp_ltv, pbp_metadata_ltv, pbp_metadata_len);
            }

            if (broadcast_name_len)
            {
                broadcast_name_len = (broadcast_name_len < BROADCAST_NAME_MAX) ? broadcast_name_len :
                                     BROADCAST_NAME_MAX;
                bc_source.broadcast_name_len = broadcast_name_len;
                if (broadcast_name_len)
                {
                    memcpy(bc_source.broadcast_name, broadcast_name, broadcast_name_len);
                }
            }
            else
            {
                broadcast_name_len = (name_data_len < BROADCAST_NAME_MAX) ? name_data_len :
                                     BROADCAST_NAME_MAX;
                bc_source.broadcast_name_len = broadcast_name_len;
                if (broadcast_name_len)
                {
                    memcpy(bc_source.broadcast_name, p_name_data, broadcast_name_len);
                }
            }

            bc_source.advertiser_sid = p_report->adv_sid;
            memcpy(bc_source.broadcast_id, p_broadcast_id, 3);
            scan_results_callback(bc_source.adv_addr, bc_source.adv_addr_type, 0xFF, p_report->adv_sid,
                                  p_name_data, name_data_len, rssi, 0);
            ba_bst_src_info_callback(&bc_source);
            if (bc_source.pbp_ltv)
            {
                free(bc_source.pbp_ltv);
            }
        }
    }
}


void ba_mgr_clear_device_list(void)
{
    ba_bsnk_mgr_clear_src_dev_list();
    bap_ba_bsnk_mgr_clear_src_dev_list();
}

bool ba_add_device(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                   uint8_t broadcast_id[3])
{
    ba_bsk_add_src_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    return bap_ba_bsnk_add_src_dev(bd_addr, bd_type, adv_sid, broadcast_id, false);
}

void ba_select_broadcast_src(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid,
                             uint8_t broadcast_id[3], uint8_t *code)
{
    T_BAP_BC_SRC_INFO *p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    T_BA_SRC *p_select_src = (T_BA_SRC *) & (ba_service_cb.ba_select_src);

    if (p_bc_source)
    {
        if (p_bc_source->p_mapping)
        {
            /* FIXME: must wait new pa info to add op */
#if (BASS_USE_PA_INFO_SAVED == 1)
            ba_bsnk_handle_bst_src_info((void *)p_bc_source);
#else
            APP_PRINT_INFO1("ba_select_broadcast_src select bd_addr %b, free p_mapping", TRACE_BDADDR(bd_addr));
            free(p_bc_source->p_mapping);
            p_bc_source->p_mapping = NULL;
#endif
        }
    }

    memcpy(p_select_src->bd_addr, bd_addr, BD_ADDR_LEN);
    p_select_src->bd_type = bd_type;
    p_select_src->adv_sid = adv_sid;
    memcpy(p_select_src->broadcast_id, broadcast_id, BROADCAST_ID_LEN);
    memcpy(p_select_src->broadcast_code, code, BROADCAST_CODE_LEN);

    bap_ba_sync_broadcast_src(bd_addr, bd_type, adv_sid, broadcast_id, code);
}

bool ba_start_remote_sync(uint8_t remote_addr_type, uint8_t *remote_addr, uint32_t bis_array)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(remote_addr);
    T_LE_AUDIO *p_link = NULL;
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("ba_start_remote_sync: can't find device record");
        return false;
    }

    p_link = ble_audio_find_by_conn_id(p_dev_rec->le_conn_id);
    if (p_link == NULL)
    {
        APP_PRINT_ERROR0("ba_start_remote_sync: can't find device link");
        return false;
    }

    uint8_t instance_id = 0;
    T_BRS_INFO *p_brs = NULL;
    T_BA_SRC *p_select_src = (T_BA_SRC *) & (ba_service_cb.ba_select_src);
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;

    p_brs = ba_get_bsnk_bis_synced_info(remote_addr, &instance_id);
    if (p_brs)
    {
        /*bsnk already sync to selected bsrc*/
        if (!memcmp(p_brs->adv_addr, p_select_src->bd_addr, BD_ADDR_LEN))
        {
            ba_brs_report_callback(p_dev_rec->pseudo_addr, instance_id, p_brs);
            return true;
        }
        else
        {
            p_bc_source = bap_ba_bsnk_find_bst_dev(p_brs->adv_addr, p_brs->adv_addr_type, p_brs->advertiser_sid,
                                                   p_brs->broadcast_id);
            if (p_bc_source == NULL)
            {
                APP_PRINT_ERROR1("ba_start_remote_sync: src %b p_bc_source NULL", TRACE_BDADDR(p_brs->adv_addr));
                return false;
            }
            /* do not use bap_ba_modify_src, cause this function used selected handle*/
            p_link->brs_pending_sync.pending = true;
            p_link->brs_pending_sync.bd_type = remote_addr_type;
            memcpy(p_link->brs_pending_sync.bd_addr, remote_addr, BD_ADDR_LEN);
            p_link->brs_pending_sync.bis_array = bis_array;

            return bap_ba_modify_src(p_link->conn_handle, p_brs->source_id, BASS_PA_NOT_SYNC, 0);
        }
    }

    p_brs = ba_get_bsnk_added_src_info(remote_addr, p_select_src->bd_addr);
    if (p_brs)
    {
        return bap_ba_modify_src(p_link->conn_handle, p_brs->source_id, BASS_PA_SYNC_PAST, bis_array);
    }
    else
    {
        return bap_ba_add_src(p_link->conn_handle, BASS_PA_SYNC_PAST, bis_array);
    }
}

bool ba_stop_remote_sync(uint8_t *remote_addr, uint8_t instance_id)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(remote_addr);
    T_LE_AUDIO *p_link = NULL;
    T_BRS_INFO *p_brs = NULL;
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("ba_stop_remote_sync can't find device record");
        return false;
    }

    p_link = ble_audio_find_by_conn_id(p_dev_rec->le_conn_id);
    if (p_link == NULL || instance_id >= BASS_BRS_NUM_MAX)
    {
        APP_PRINT_ERROR0("ba_stop_remote_sync can't find device link");
        return false;
    }

    p_brs = (T_BRS_INFO *) & (p_link->brs_cb[instance_id]);

    APP_PRINT_INFO2("ba_stop_remote_sync: pa_sync_state %x bis_sync_state 0x%x",
                    p_brs->pa_sync_state, p_brs->bis_sync_state);

    if ((p_brs->bis_sync_state != 0) || (p_brs->pa_sync_state != 0))
    {
        p_brs->need_remove = true;
        return bap_ba_modify_src(p_link->conn_handle, p_brs->source_id,
                                 BASS_PA_NOT_SYNC, 0);
    }
    return bap_ba_remove_src(p_link->conn_handle, p_brs->source_id);
}


bool ba_modify_remote_sync(uint8_t *remote_addr, uint8_t instance_id, uint32_t bis_array)
{
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(remote_addr);
    T_LE_AUDIO *p_link = NULL;
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("ba_modify_remote_sync can't find device record");
        return false;
    }

    p_link = ble_audio_find_by_conn_id(p_dev_rec->le_conn_id);
    if (p_link == NULL || instance_id >= BASS_BRS_NUM_MAX)
    {
        APP_PRINT_ERROR0("ba_modify_remote_sync can't find device link");
        return false;
    }

    return bap_ba_modify_src(p_link->conn_handle, p_link->brs_cb[instance_id].source_id,
                             BASS_PA_SYNC_PAST, bis_array);
}

bool ba_pa_sync(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3])
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    if (p_bc_source)
    {
        p_bc_source->pa_sync_mask |= BAP_BA_PA_SYNC_BSRC_MSK;
        return bap_bsnk_pa_sync(bd_addr, bd_type, adv_sid, broadcast_id, 10);
    }
    return false;
}

bool ba_pa_terminate(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid, uint8_t broadcast_id[3])
{
    T_BAP_BC_SRC_INFO *p_bc_source = NULL;
    p_bc_source = bap_ba_bsnk_find_bst_dev(bd_addr, bd_type, adv_sid, broadcast_id);
    if (p_bc_source)
    {
        p_bc_source->pa_sync_mask &= ~BAP_BA_PA_SYNC_BSRC_MSK;
        return bap_bsnk_pa_terminate(bd_addr, bd_type, adv_sid, broadcast_id);
    }

    return false;
}

void ba_service_init(void)
{
    memset(&ba_service_cb, 0, sizeof(T_BA_SERVICE_CB));
    os_queue_init(&ba_service_cb.bst_src_queue);
}

uint8_t ba_start_le_scan(void)
{
    if (ba_service_cb.client_id > 0)
    {
        return ba_service_cb.client_id;
    }

    ba_service_cb.client_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, GAP_SCAN_FILTER_ANY,
                                                      ba_service_scan_callback);
    if (ba_service_cb.client_id > 0)
    {
        ba_mgr_clear_device_list();
        return ba_service_cb.client_id;
    }
    ba_service_cb.client_id = 0;
    return 0;
}

void ba_stop_le_scan(void)
{
    if (ba_service_cb.client_id > 0)
    {
        bt_adap_stop_discovery(ba_service_cb.client_id);
    }
    ba_service_cb.client_id = 0;
}

void ba_bsnk_handle_bst_src_info(void *p_source)
{
    uint8_t i, j;
    T_BAP_BC_SRC_INFO *p_bc_source = (T_BAP_BC_SRC_INFO *)p_source;
    uint32_t bis_array = 0;
    T_BROADCAST_SRC_INFO bc_source;
    memset(&bc_source, 0, sizeof(T_BROADCAST_SRC_INFO));
    /*set rssi to max for selected src*/
    bc_source.rssi = 0x7F;

    if (p_bc_source == NULL)
    {
        APP_PRINT_ERROR0("ba_bsnk_handle_bst_src_info source fail");
        return;
    }

    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(p_bc_source->adv_addr);
    if (p_dev_rec)
    {
        bc_source.adv_addr_type = p_dev_rec->bd_type;
        memcpy(bc_source.adv_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
    }
    else
    {
        bc_source.adv_addr_type = p_bc_source->adv_addr_type;
        memcpy(bc_source.adv_addr, p_bc_source->adv_addr, BD_ADDR_LEN);
    }

    bc_source.advertiser_sid = p_bc_source->advertiser_sid;
    memcpy(bc_source.broadcast_id, p_bc_source->broadcast_id, 3);
    bc_source.num_subgroups = p_bc_source->p_mapping ? p_bc_source->p_mapping->num_subgroups : 0;

    for (i = 0; i < bc_source.num_subgroups; i++)
    {
        T_BASE_DATA_SUBGROUP_PARAM *p_subgroup = p_bc_source->p_mapping->p_subgroup + i;
        bc_source.bst_groups[i].subgroup_idx = p_subgroup->subgroup_idx;
        bc_source.bst_groups[i].metadata_len = p_subgroup->metadata_len;

        if (p_subgroup->metadata_len > 0)
        {
            memcpy(bc_source.bst_groups[i].metadata, p_subgroup->p_metadata, p_subgroup->metadata_len);
        }
        bc_source.bst_groups[i].num_bis = p_subgroup->num_bis;

        for (j = 0; j < p_subgroup->num_bis; j++)
        {
            T_BASE_DATA_BIS_PARAM *p_bis_param = p_subgroup->p_bis_param + j;
            bis_array |= p_bis_param->bis_index;
            bc_source.bst_groups[i].bis_param[j].bis_index = p_bis_param->bis_index;
            bc_source.bst_groups[i].bis_param[j].sample_frequency = p_bis_param->bis_codec_cfg.sample_frequency;
            bc_source.bst_groups[i].bis_param[j].chnnl_allocation =
                p_bis_param->bis_codec_cfg.audio_channel_allocation;
        }
    }

    ba_bst_src_info_callback(&bc_source);
}


void ba_handle_brs_state(T_LE_AUDIO *p_ble_audio, T_BASS_CLIENT_BRS_DATA *p_brs_data)
{
    T_BT_DEVICE *p_dev_rec;
    uint8_t instance_id = 0;
    T_BASS_BRS_DATA *p_brs = NULL;
    T_BRS_SYNC_INFO *p_pend_sync = NULL;

    if (!p_ble_audio || !p_brs_data || p_brs_data->p_brs_data == NULL)
    {
        return;
    }

    instance_id = p_brs_data->char_instance_id;
    p_brs = p_brs_data->p_brs_data;

    if (instance_id >= BASS_BRS_NUM_MAX)
    {
        APP_PRINT_ERROR0("ba_handle_brs_state instance_id is too large");
        return;
    }

    APP_PRINT_INFO7("ba_handle_brs_state: source_id %x source_adv_sid %x pa_sync_state %x bis_sync_state %x addr %b id %b is_used %d",
                    p_brs->source_id,
                    p_brs->source_adv_sid,
                    p_brs->pa_sync_state,
                    p_brs->bis_sync_state,
                    TRACE_BDADDR(p_brs->source_address),
                    TRACE_BINARY(3, p_brs->broadcast_id),
                    p_brs->brs_is_used);

    /* brs_is_used false means snk has removed this src*/
    if (p_brs->brs_is_used == false)
    {
        memset(&(p_ble_audio->brs_cb[instance_id]), 0, sizeof(T_BRS_INFO));
        p_pend_sync = &(p_ble_audio->brs_pending_sync);
        if (!p_pend_sync)
        {
            return;
        }

        if (p_pend_sync->pending)
        {
            p_pend_sync->pending = false;
            ba_start_remote_sync(p_pend_sync->bd_type, p_pend_sync->bd_addr, p_pend_sync->bis_array);
        }
        return;
    }

    p_ble_audio->brs_cb[instance_id].source_id = p_brs->source_id;
    p_ble_audio->brs_cb[instance_id].advertiser_sid = p_brs->source_adv_sid;
    p_ble_audio->brs_cb[instance_id].pa_sync_state = p_brs->pa_sync_state;
    p_ble_audio->brs_cb[instance_id].bis_sync_state = p_brs->bis_sync_state;
    p_ble_audio->brs_cb[instance_id].adv_addr_type = p_brs->source_address_type;
    memcpy(p_ble_audio->brs_cb[instance_id].adv_addr, p_brs->source_address, BD_ADDR_LEN);
    memcpy(p_ble_audio->brs_cb[instance_id].broadcast_id, p_brs->broadcast_id, 3);

    p_dev_rec = ual_find_device_by_conn_handle(p_brs_data->conn_handle);

    if (p_dev_rec)
    {
        ba_brs_report_callback(p_dev_rec->pseudo_addr, instance_id, &p_ble_audio->brs_cb[instance_id]);
    }

    if (p_brs->bis_sync_state == 0)
    {
        APP_PRINT_INFO0("ba_handle_brs_state bis_sync_state 0");

        if (p_ble_audio->brs_cb[instance_id].need_remove)
        {
            bap_ba_remove_src(p_brs_data->conn_handle, p_ble_audio->brs_cb[instance_id].source_id);
            p_ble_audio->brs_cb[instance_id].need_remove = false;
        }
    }
    else
    {
        if (ba_bsnk_find_bst_dev(p_brs->source_address, p_brs->source_address_type,
                                 p_brs->source_adv_sid, p_brs->broadcast_id) == NULL)
        {
            ba_add_device(p_brs->source_address, p_brs->source_address_type,
                          p_brs->source_adv_sid, p_brs->broadcast_id);
            ba_select_broadcast_src(p_brs->source_address,
                                    p_brs->source_address_type,
                                    p_brs->source_adv_sid,
                                    p_brs->broadcast_id,
                                    p_brs->bad_code);
        }
    }


}

void ba_bsnk_handle_src_sync_state(void *p_source)
{
    T_BAP_BC_SRC_INFO *p_bc_source = (T_BAP_BC_SRC_INFO *)p_source;
    uint8_t sync_state = BSK_SYNC_SRC_IDLE;
    T_BT_DEVICE *p_dev_rec = NULL;
    if (p_bc_source == NULL)
    {
        APP_PRINT_ERROR0("ba_bsnk_handle_src_sync_state source fail");
        return;
    }

    APP_PRINT_INFO4("src_sync_state bd_addr %b adv_id %x broadcast_id %b state %x",
                    TRACE_BDADDR(p_bc_source->adv_addr),
                    p_bc_source->advertiser_sid,
                    TRACE_BINARY(3, p_bc_source->broadcast_id),
                    p_bc_source->pa_sync_state);
    if (p_bc_source->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_SCAN_IDLE ||
        p_bc_source->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_WAIT_SCANNING ||
        p_bc_source->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING)
    {
        sync_state |= BSK_SYNC_PA_SYNCHRONIZING;
    }
    else if (p_bc_source->pa_sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
    {
        sync_state |= BSK_SYNC_PA_SYNCHRONIZED;
    }

    if (p_bc_source->big_sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZING)
    {
        sync_state |= BSK_SYNC_BIG_SYNCHRONIZING;
    }
    else if (p_bc_source->big_sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
    {
        sync_state |= BSK_SYNC_BIG_SYNCHRONIZED;
    }

    p_dev_rec = ual_find_device_by_addr(p_bc_source->adv_addr);

    if (p_dev_rec)
    {
        bst_src_sync_state_callback(p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                                    p_bc_source->advertiser_sid,
                                    sync_state);
    }
    else
    {
        bst_src_sync_state_callback(p_bc_source->adv_addr, p_bc_source->adv_addr_type,
                                    p_bc_source->advertiser_sid,
                                    sync_state);
    }

}


