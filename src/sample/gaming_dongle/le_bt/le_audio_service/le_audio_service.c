#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "gap_vendor.h"
#include "le_audio_service.h"
#include "vector.h"
#include "le_audio_lib_msg.h"
#include "mcp_mgr.h"
#include "csis_client.h"
#include "ccp_mgr.h"
#include "le_audio_data.h"
#include "le_csis_client_service.h"
#include "le_bst_src_service.h"
#include "le_media_player.h"
#include "app_usb_layer.h"
#include "dev_mgr.h"
#include "le_vcs_service.h"
#include "le_pacs_service.h"
#include "le_unicast_sink_service.h"
#include "le_unicast_src_service.h"
#include "le_bass_service.h"
#include "le_csis_srv_service.h"
#include "teams_call_control.h"
#include "le_media_controller.h"
#include "le_broadcast_mgr.h"
#include "le_bsk_service.h"
#include "tmas_mgr.h"
#include "mics_def.h"
#include "bt_gatt_svc.h"
#include "bt_gatt_client.h"
#include "app_usb_uac.h"
#include "app_ctrl_pkt_policy.h"

#define LE_AUDIO_BROADCAST_MODE   0x01
#define LE_AUDIO_UNICAST_MODE     0x02
#define LE_AUDIO_UNKNOWN_MODE     0

#define HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY 0xFD80
#define HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY_SUBCODE 0x1C

typedef struct
{
    T_LE_AUDIO        le_link[MAX_BLE_LINK_NUM];    //FIX TODO we should use queue
    T_AUDIO_ROLE      audio_role;
    uint8_t           active_mode;
    uint16_t          service_support;
    uint16_t          client_support;

    uint8_t           active_sink;
    uint8_t           sink_mask;

    T_SERVER_ID       mcs_id;
    T_SERVER_ID       ccp_id;
} T_LE_AUDIO_DB;

static T_LE_AUDIO_DB *p_le_audio_mgr = NULL;
static T_UALIST_HEAD le_audio_link_list =
{
    &le_audio_link_list, &le_audio_link_list
};

T_LE_AUDIO *ble_audio_find_by_conn_id(uint8_t conn_id)
{
    T_LE_AUDIO    *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (p_le_audio_mgr->le_link[i].used == true &&
            p_le_audio_mgr->le_link[i].conn_id == conn_id)
        {
            p_link = &p_le_audio_mgr->le_link[i];
            break;
        }
    }

    return p_link;
}

T_LE_AUDIO *ble_audio_find_by_conn_handle(uint16_t conn_handle)
{
    T_LE_AUDIO    *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (p_le_audio_mgr->le_link[i].used == true &&
            p_le_audio_mgr->le_link[i].conn_handle == conn_handle)
        {
            p_link = &p_le_audio_mgr->le_link[i];
            break;
        }
    }

    return p_link;
}

#if DONGLE_TRANSMIT_CLIENT_SUPPORT
T_LE_AUDIO *ble_audio_find_by_link_id(uint8_t link_id)
{
    T_LE_AUDIO    *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (p_le_audio_mgr->le_link[i].used == true &&
            p_le_audio_mgr->le_link[i].link_id == link_id)
        {
            p_link = &p_le_audio_mgr->le_link[i];
            break;
        }
    }

    return p_link;
}

T_LE_AUDIO *ble_audio_find_by_bud_side(uint8_t bud_side)
{
    T_LE_AUDIO    *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (p_le_audio_mgr->le_link[i].used == true &&
            p_le_audio_mgr->le_link[i].bud_side == bud_side)
        {
            p_link = &p_le_audio_mgr->le_link[i];
            break;
        }
    }

    return p_link;
}
#endif

T_LE_AUDIO *ble_audio_alloc_by_conn_id(uint8_t conn_id)
{
    T_LE_AUDIO *p_link = NULL;
    uint8_t        i;

    p_link = ble_audio_find_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        return p_link;
    }

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (p_le_audio_mgr->le_link[i].used == false)
        {
            p_link = &p_le_audio_mgr->le_link[i];
            p_link->used    = true;
            p_link->conn_id = conn_id;
#if DONGLE_TRANSMIT_CLIENT_SUPPORT
            p_link->link_id = i;
#endif
            break;
        }
    }

    return p_link;
}

bool ble_audio_free(T_LE_AUDIO *p_link)
{
    if (p_link != NULL)
    {
        if (p_link->used == true)
        {
            memset(p_link, 0, sizeof(T_LE_AUDIO));
            p_link->conn_id = 0xFF;
            p_link->conn_handle = 0xFFFF;
            return true;
        }
    }

    return false;
}

/**
 * @brief Set Afh Policy Priority Command
 *
 * @param lea_conn_handle(2 byte)  CIS Connect Handle
 * @param afh policy priority (1 byte)
 * remote first = 0, local first = 1
 * @return true  success
 * @return false fail
 */
static bool ble_audio_send_psd_policy(uint16_t lea_conn_handle)
{
    uint16_t opcode = HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY;
    uint8_t params[4];
    uint8_t params_len = 4;

    params[0] = HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY_SUBCODE;
    memcpy(&params[1], &lea_conn_handle, 2);
    params[3] = 0; /* remote first; due to dongle is src */

    return gap_vendor_cmd_req(opcode, params_len, params);
}

void ble_audio_conn_state_change(uint8_t conn_id, T_BT_CONN_STATE new_state)
{
    T_LE_AUDIO *p_link;

    APP_PRINT_INFO2("ble_audio_conn_state_change: conn_id %d, conn_state %d",
                    conn_id, new_state);

    p_link = ble_audio_find_by_conn_id(conn_id);
    switch (new_state)
    {
    case BT_CONN_STATE_DISCONNECTING:
        if (p_link != NULL)
        {
            p_link->state = BT_CONN_STATE_DISCONNECTING;
        }
        break;

    case BT_CONN_STATE_DISCONNECTED:
        if (p_link != NULL)
        {
            ualist_del(&p_link->list);
            init_ualist_head(&p_link->list);
            p_link->state = BT_CONN_STATE_DISCONNECTED;
            ble_audio_free(p_link);
        }
        break;

    case BT_CONN_STATE_CONNECTING:
        if (p_link == NULL)
        {
            p_link = ble_audio_alloc_by_conn_id(conn_id);
            if (p_link != NULL)
            {
                init_ualist_head(&p_link->list);
                p_link->state = BT_CONN_STATE_CONNECTING;
            }

        }
        break;

    case BT_CONN_STATE_CONNECTED:
        if (p_link != NULL)
        {
            p_link->state = BT_CONN_STATE_CONNECTED;
            p_link->conn_handle = le_get_conn_handle(conn_id);
            ualist_add_tail(&p_link->list, &le_audio_link_list);

            //lea connected psd vendor command
            ble_audio_send_psd_policy(p_link->conn_handle);

            if (ualist_len(&le_audio_link_list) == 1)
            {
                /* clear when first device connected */
                app_usb_uac_clear_headset_status();
            }

#if DONGLE_TRANSMIT_CLIENT_SUPPORT
            p_link->tx_event_seqn = 0;
#endif
        }
        break;

    default:
        break;

    }
}

T_UALIST_HEAD *ble_audio_get_conn_list(void)
{
    return &le_audio_link_list;
}

void ble_update_brs_state(T_LE_AUDIO *p_ble_audio, T_BASS_CLIENT_BRS_DATA *brs_data)
{
    T_BT_DEVICE *p_dev_rec;
    if (!p_ble_audio || !brs_data)
    {
        return;
    }

    if (brs_data->p_brs_data->pa_sync_state == PA_SYNC_STATE_SYNC)
    {
        p_ble_audio->audio_state |= SYNC_STATE;
    }
    else if (brs_data->p_brs_data->pa_sync_state == PA_SYNC_STATE_NOT_SYNC ||
             brs_data->p_brs_data->pa_sync_state == PA_SYNC_STATE_FAILED)
    {
        p_ble_audio->audio_state &= (~SYNC_STATE);
    }

    if (brs_data->p_brs_data->bis_sync_state != 0 &&
        brs_data->p_brs_data->bis_sync_state != 0xFFFFFFFF)
    {
        p_ble_audio->audio_state |= STREAM_STATE;
    }
    else
    {
        p_ble_audio->audio_state &= (~STREAM_STATE);
    }

    if (brs_data->p_brs_data->brs_is_used == false)
    {
        p_ble_audio->audio_state = CONNECT_STATE;
    }
    p_dev_rec = ual_find_device_by_conn_id(p_ble_audio->conn_id);
    if (p_dev_rec)
    {
        ble_usb_audio_state_change(p_dev_rec->pseudo_addr, p_dev_rec->bd_type, p_ble_audio->state,
                                   p_ble_audio->audio_state, 0);
        APP_PRINT_INFO3("ble_update_brs_state: conn_id %d, conn_state %d, audio_state %d",
                        p_ble_audio->conn_id, p_ble_audio->state, p_ble_audio->audio_state);

    }
}

void ble_audio_update_pacs_info(T_BAP_PACS_NOTIFY *p_data)
{
    T_BAP_PACS_INFO pacs_info;
    T_BAP_PAC_RECORD *p_pac_tbl;
    if (p_data == NULL)
    {
        return;
    }
    APP_PRINT_INFO2("ble_audio_update_pacs_info: conn_handle 0x%x, handle 0x%x", p_data->conn_handle,
                    p_data->pac_handle);

    T_LE_AUDIO *p_link =  ble_audio_find_by_conn_handle(p_data->conn_handle);
    T_BT_DEVICE *p_dev_rec = NULL;
    if (p_link != NULL)
    {
        if (!bap_pacs_get_info(p_data->conn_handle, &pacs_info))
        {
            APP_PRINT_ERROR1("ble_audio_updata_pacs_info: get pacs info fail conn_handle 0x%x",
                             p_data->conn_handle);
            return;
        }

        p_dev_rec = ual_find_device_by_conn_id(p_link->conn_id);
        APP_PRINT_INFO2("ble_audio_update_pacs_info: handle %x, pacs value %x", p_data->conn_handle,
                        pacs_info.value_exist);
        if (pacs_info.value_exist & PACS_AUDIO_AVAILABLE_CONTEXTS_EXIST)
        {
            p_link->snk_avail_context = pacs_info.snk_avail_context;
            p_link->src_avail_context = pacs_info.src_avail_context;
        }
        if (pacs_info.value_exist & PACS_AUDIO_SUPPORTED_CONTEXTS_EXIST)
        {
            p_link->snk_sup_context = pacs_info.snk_sup_context;
            p_link->src_sup_context = pacs_info.src_sup_context;
        }
        /* we only care AUDIO_LOCATION_FL|AUDIO_LOCATION_FR,
        * cause AUDIO_LOCATION_FL|AUDIO_LOCATION_FR is mandatory
        * and we only config AUDIO_LOCATION_FL|AUDIO_LOCATION_FR
        * in case some pacs may config serval LEFT or RIGHT location
        */
        if (pacs_info.value_exist & PACS_SINK_AUDIO_LOC_EXIST)
        {
            p_link->snk_audio_loc = pacs_info.snk_audio_loc & (AUDIO_LOCATION_FL | AUDIO_LOCATION_FR);
        }

        if (pacs_info.value_exist & PACS_SOURCE_AUDIO_LOC_EXIST)
        {
            p_link->src_audio_loc = pacs_info.src_audio_loc & (AUDIO_LOCATION_FL | AUDIO_LOCATION_FR);
        }

        if (p_dev_rec != NULL)
        {
            device_properities_update_callback(p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                                               p_link->snk_sup_context,
                                               p_link->src_sup_context,
                                               p_link->snk_avail_context, p_link->src_avail_context,
                                               p_link->snk_audio_loc, p_link->src_audio_loc);
        }

        if (pacs_info.value_exist & PACS_SINK_PAC_EXIST)
        {
            p_pac_tbl = calloc(1, p_link->snk_pac_num * sizeof(T_BAP_PAC_RECORD));

            if (p_pac_tbl == NULL)
            {
                goto failed;
            }

            bap_pacs_get_pac_record(p_data->conn_handle, SERVER_AUDIO_SINK, &p_link->snk_pac_num, p_pac_tbl);

            if (p_dev_rec != NULL)
            {
                for (int i = 0; i < p_link->snk_pac_num; i++)
                {
                    uint16_t sup_freq = 0;
                    uint8_t chnl_cnts = 0;
                    if (p_pac_tbl[i].codec_cap.type_exist & CODEC_CFG_SAMPLING_FREQUENCY_EXIST)
                    {
                        sup_freq = p_pac_tbl[i].codec_cap.supported_sampling_frequencies;
                    }
                    if (p_pac_tbl[i].codec_cap.type_exist & CODEC_CFG_SAMPLING_FREQUENCY_EXIST)
                    {
                        chnl_cnts = p_pac_tbl[i].codec_cap.audio_channel_counts;
                    }
                    device_pac_info_update_callback(p_dev_rec->pseudo_addr, p_dev_rec->bd_type, SERVER_AUDIO_SINK,
                                                    sup_freq, chnl_cnts, p_pac_tbl[i].pref_audio_contexts,
                                                    p_pac_tbl[i].metadata_length, p_pac_tbl[i].p_metadata);
                }
            }
            free(p_pac_tbl);

        }

        if (pacs_info.value_exist & PACS_SOURCE_PAC_EXIST)
        {
            p_pac_tbl = calloc(1, p_link->src_pac_num * sizeof(T_BAP_PAC_RECORD));

            if (p_pac_tbl == NULL)
            {
                goto failed;
            }

            bap_pacs_get_pac_record(p_data->conn_handle, SERVER_AUDIO_SOURCE, &p_link->src_pac_num, p_pac_tbl);

            if (p_dev_rec != NULL)
            {
                for (int i = 0; i < p_link->src_pac_num; i++)
                {
                    uint16_t sup_freq = 0;
                    uint8_t chnl_cnts = 0;
                    if (p_pac_tbl[i].codec_cap.type_exist & CODEC_CFG_SAMPLING_FREQUENCY_EXIST)
                    {
                        sup_freq = p_pac_tbl[i].codec_cap.supported_sampling_frequencies;
                    }
                    if (p_pac_tbl[i].codec_cap.type_exist & CODEC_CFG_SAMPLING_FREQUENCY_EXIST)
                    {
                        chnl_cnts = p_pac_tbl[i].codec_cap.audio_channel_counts;
                    }
                    device_pac_info_update_callback(p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                                                    SERVER_AUDIO_SOURCE, sup_freq, chnl_cnts, p_pac_tbl[i].pref_audio_contexts,
                                                    p_pac_tbl[i].metadata_length, p_pac_tbl[i].p_metadata);
                }
            }
            free(p_pac_tbl);
        }
    }
failed:
    return;
}


void ble_audio_get_pacs_info(T_BAP_DIS_ALL_DONE *p_data)
{
    T_BAP_PACS_INFO pacs_info;
    uint8_t pac_num;
    T_BAP_PAC_RECORD *p_pac_tbl;
    if (p_data == NULL || !p_data->pacs_is_found)
    {
        return;
    }
    APP_PRINT_INFO4("LE_AUDIO_MSG_BAP_DIS_ALL_DONE: conn_handle 0x%x, pacs_is_found %d, sink_pac_num %d, source_pac_num %d",
                    p_data->conn_handle,
                    p_data->pacs_is_found,
                    p_data->sink_pac_num,
                    p_data->source_pac_num);


    T_LE_AUDIO *p_link =  ble_audio_find_by_conn_handle(p_data->conn_handle);
    T_BT_DEVICE *p_dev_rec = NULL;
    if (p_link != NULL)
    {
        p_link->remote_serv_sup |= LE_AUDIO_PACS_CLIENT_FLAG;
        if (!bap_pacs_get_info(p_data->conn_handle, &pacs_info))
        {
            APP_PRINT_ERROR1("ble_audio_get_pacs_info: get pacs info fail conn_handle 0x%x",
                             p_data->conn_handle);
            return;
        }

        p_dev_rec = ual_find_device_by_conn_id(p_link->conn_id);
        APP_PRINT_INFO2("ble_audio_get_pacs_info: handle %x, pacs value %x", p_data->conn_handle,
                        pacs_info.value_exist);
        if (pacs_info.value_exist & PACS_AUDIO_AVAILABLE_CONTEXTS_EXIST)
        {
            p_link->snk_avail_context = pacs_info.snk_avail_context;
            p_link->src_avail_context = pacs_info.src_avail_context;
        }
        if (pacs_info.value_exist & PACS_AUDIO_SUPPORTED_CONTEXTS_EXIST)
        {
            p_link->snk_sup_context = pacs_info.snk_sup_context;
            p_link->src_sup_context = pacs_info.src_sup_context;
        }

        if (pacs_info.value_exist & PACS_SINK_AUDIO_LOC_EXIST)
        {
            p_link->snk_audio_loc = pacs_info.snk_audio_loc;
        }

#if DONGLE_TRANSMIT_CLIENT_SUPPORT
        if (p_link->snk_audio_loc & AUDIO_LOCATION_FL)
        {
            p_link->bud_side = EARBUD_SIDE_LEFT;
        }
        else if (p_link->snk_audio_loc & AUDIO_LOCATION_FR)
        {
            p_link->bud_side = EARBUD_SIDE_RIGHT;
        }
        else
        {
            p_link->bud_side = EARBUD_SIDE_ANY;
        }
        APP_PRINT_INFO2("ble_audio_get_pacs_info: p_link->snk_audio_loc 0x%x bud_side 0x%x",
                        p_link->snk_audio_loc, p_link->bud_side);
#endif

        if (pacs_info.value_exist & PACS_SOURCE_AUDIO_LOC_EXIST)
        {
            p_link->src_audio_loc = pacs_info.src_audio_loc;
        }

        if (p_dev_rec != NULL)
        {
            device_properities_update_callback(p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                                               p_link->snk_sup_context,
                                               p_link->src_sup_context,
                                               p_link->snk_avail_context, p_link->src_avail_context,
                                               p_link->snk_audio_loc, p_link->src_audio_loc);
        }

        if (pacs_info.value_exist & PACS_SINK_PAC_EXIST)
        {
            p_link->snk_pac_num = pacs_info.sink_pac_num;
            pac_num = p_data->sink_pac_num;
            if (pac_num == 0)
            {
                goto failed;
            }

            p_pac_tbl = calloc(1, pac_num * sizeof(T_BAP_PAC_RECORD));
            if (p_pac_tbl == NULL)
            {
                goto failed;
            }

            bap_pacs_get_pac_record(p_data->conn_handle, SERVER_AUDIO_SINK, &pac_num, p_pac_tbl);

            if (p_dev_rec != NULL)
            {
                for (int i = 0; i < pac_num; i++)
                {
                    uint16_t sup_freq = 0;
                    uint8_t chnl_cnts = 0;
                    if (p_pac_tbl[i].codec_cap.type_exist & CODEC_CFG_SAMPLING_FREQUENCY_EXIST)
                    {
                        sup_freq = p_pac_tbl[i].codec_cap.supported_sampling_frequencies;
                    }
                    if (p_pac_tbl[i].codec_cap.type_exist & CODEC_CFG_SAMPLING_FREQUENCY_EXIST)
                    {
                        chnl_cnts = p_pac_tbl[i].codec_cap.audio_channel_counts;
                    }
                    device_pac_info_update_callback(p_dev_rec->pseudo_addr, p_dev_rec->bd_type, SERVER_AUDIO_SINK,
                                                    sup_freq, chnl_cnts, p_pac_tbl[i].pref_audio_contexts,
                                                    p_pac_tbl[i].metadata_length, p_pac_tbl[i].p_metadata);
                }
            }
            free(p_pac_tbl);

        }

        if (pacs_info.value_exist & PACS_SOURCE_PAC_EXIST)
        {
            p_link->src_pac_num = pacs_info.source_pac_num;
            pac_num = p_data->source_pac_num;
            if (pac_num == 0)
            {
                goto failed;
            }

            p_pac_tbl = calloc(1, pac_num * sizeof(T_BAP_PAC_RECORD));
            if (p_pac_tbl == NULL)
            {
                goto failed;
            }

            bap_pacs_get_pac_record(p_data->conn_handle, SERVER_AUDIO_SOURCE, &pac_num, p_pac_tbl);

            if (p_dev_rec != NULL)
            {
                for (int i = 0; i < pac_num; i++)
                {
                    uint16_t sup_freq = 0;
                    uint8_t chnl_cnts = 0;
                    if (p_pac_tbl[i].codec_cap.type_exist & CODEC_CFG_SAMPLING_FREQUENCY_EXIST)
                    {
                        sup_freq = p_pac_tbl[i].codec_cap.supported_sampling_frequencies;
                    }
                    if (p_pac_tbl[i].codec_cap.type_exist & CODEC_CFG_SAMPLING_FREQUENCY_EXIST)
                    {
                        chnl_cnts = p_pac_tbl[i].codec_cap.audio_channel_counts;
                    }
                    device_pac_info_update_callback(p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                                                    SERVER_AUDIO_SOURCE, sup_freq, chnl_cnts, p_pac_tbl[i].pref_audio_contexts,
                                                    p_pac_tbl[i].metadata_length, p_pac_tbl[i].p_metadata);
                }
            }
            free(p_pac_tbl);
        }
    }
failed:
    return;
}

void ble_audio_set_broadcast_mode(bool enable, T_AUDIO_ROLE role)
{
    if (enable)
    {
        p_le_audio_mgr->active_mode |= LE_AUDIO_BROADCAST_MODE;
        p_le_audio_mgr->audio_role = role;
    }
    else
    {
        p_le_audio_mgr->active_mode &= (~LE_AUDIO_BROADCAST_MODE);
    }
}

void ble_audio_set_unicast_mode(bool enable, T_AUDIO_ROLE role)
{
    if (enable)
    {
        p_le_audio_mgr->active_mode |= LE_AUDIO_UNICAST_MODE;
        p_le_audio_mgr->audio_role = role;
    }
    else
    {
        p_le_audio_mgr->active_mode &= (~LE_AUDIO_UNICAST_MODE);
    }
}

uint8_t ble_audio_get_mode(void)
{
    return p_le_audio_mgr->active_mode;
}

void ble_audio_route_sink_dev(uint8_t sink_from, bool enable)
{
    if (enable)
    {
        p_le_audio_mgr->sink_mask |= (1 << (sink_from - 1));
        if (p_le_audio_mgr->active_sink == LE_AUDIO_SINK_NONE)
        {
            if (sink_from == LE_AUDIO_SINK_FROM_LOCAL)
            {
                le_bsnk_setup_sink_audio();
            }
            else
            {
#if LE_AUDIO_BASS_SUPPORT
                le_bass_setup_sink_audio();
#endif
            }
            p_le_audio_mgr->active_sink = sink_from;
        }
    }
    else
    {
        p_le_audio_mgr->sink_mask &= (~(1 << (sink_from - 1)));
        if (p_le_audio_mgr->active_sink == sink_from)
        {
            if (sink_from == LE_AUDIO_SINK_FROM_LOCAL)
            {
                le_bsnk_remove_sink_audio();
            }
            else
            {
#if LE_AUDIO_BASS_SUPPORT
                le_bass_remove_sink_audio();
#endif
            }
            p_le_audio_mgr->active_sink = LE_AUDIO_SINK_NONE;
        }

        if (p_le_audio_mgr->active_sink != 0)
        {
            if (sink_from == LE_AUDIO_SINK_FROM_LOCAL)
            {
#if LE_AUDIO_BASS_SUPPORT
                le_bass_setup_sink_audio();
                p_le_audio_mgr->active_sink = LE_AUDIO_SINK_FROM_BASS;
#endif
            }
            else
            {
                le_bsnk_setup_sink_audio();
                p_le_audio_mgr->active_sink = LE_AUDIO_SINK_FROM_LOCAL;
            }
        }
    }
}

uint16_t le_audio_handle_cap_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_CAP_DIS_DONE:
        {
            T_LE_AUDIO *p_link = NULL;
            T_CAP_DIS_DONE *p_dis_done = (T_CAP_DIS_DONE *)buf;
            if (p_dis_done)
            {
                T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(p_dis_done->conn_handle);
                if (p_dev_rec)
                {
                    p_link = ble_audio_find_by_conn_handle(p_dev_rec->le_conn_handle);
                    if (p_link)
                    {
                        p_link->cap_disc_done = true;
#if DONGLE_LE_AUDIO
                        if (p_link->csis_ready_result)
                        {
                            extern bool app_le_audio_sync_host_vol_to_remote(uint16_t conn_handle);
                            app_le_audio_sync_host_vol_to_remote(p_dev_rec->le_conn_handle);
                        }
#endif
                    }
                }
            }
        }
        break;
    default:
        break;
    }

    return cb_result;
}

void ble_audio_handle_conn_update_for_headset_stream_status_change(T_HEADSET_STATUS *headset_status)
{
    uint8_t i = 0;
    bool headset_is_bt_streaming = false;

    if (headset_status->phone_status != PHONE_STREAM_IDLE)
    {
        headset_is_bt_streaming = true;
    }

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (p_le_audio_mgr->le_link[i].used == true)
        {
            if (headset_is_bt_streaming)
            {
                le_unicast_conn_update_mgr(CONN_UPDATE_EVENT_HEADSET_BT_STREAMING,
                                           p_le_audio_mgr->le_link[i].conn_id);
            }
            else
            {
                le_unicast_conn_update_mgr(CONN_UPDATE_EVENT_HEADSET_BT_IDLE, p_le_audio_mgr->le_link[i].conn_id);
            }
        }
    }
}

void ble_audio_mgr_init(void *evt_queue, void *io_queue)
{
    init_ualist_head(&le_audio_link_list);
    p_le_audio_mgr = calloc(1, sizeof(T_LE_AUDIO_DB));
    if (p_le_audio_mgr == NULL)
    {
        return;
    }
    T_BLE_AUDIO_PARAMS ble_audio_param;
    ble_audio_param.evt_queue_handle = evt_queue;
    ble_audio_param.io_queue_handle = io_queue;
    ble_audio_param.bt_gatt_client_init = GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT |
                                          GATT_CLIENT_DISCOV_MODE_CCCD_STORAGE_BIT |
                                          GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT;
    ble_audio_param.acl_link_num = le_get_max_link_num();
    ble_audio_param.io_event_type = IO_MSG_TYPE_LE_AUDIO;
    gatt_svc_init(GATT_SVC_USE_NORMAL_SERVER, 0);
    ble_audio_init(&ble_audio_param);
    ble_audio_cback_register(ble_audio_msg_cb);

    p_le_audio_mgr->audio_role = AUDIO_ROLE_UNKNOWN;
    p_le_audio_mgr->service_support = 0x480;
    p_le_audio_mgr->client_support = 0x27F;
    T_BAP_ROLE_INFO role_info;
    memset(&role_info, 0, sizeof(role_info));;
    role_info.role_mask = (BAP_UNICAST_CLT_SRC_ROLE | BAP_UNICAST_CLT_SNK_ROLE |
                           BAP_BROADCAST_SOURCE_ROLE |
                           BAP_BROADCAST_ASSISTANT_ROLE | BAP_SCAN_DELEGATOR_ROLE);
    role_info.brs_num = BASS_BRS_NUM_MAX;      //one for ba, one for local sink
#if LE_AUDIO_ASCS_SUPPORT
    role_info.snk_ase_num = 1;
    role_info.src_ase_num = 1;
#endif
    role_info.init_gap = false;


    le_audio_data_init();
    le_csis_client_init();

    bap_role_init(&role_info);
    le_broadcat_mgr_init((BAP_BROADCAST_SOURCE_ROLE |
                          BAP_BROADCAST_ASSISTANT_ROLE | BAP_SCAN_DELEGATOR_ROLE));
    T_CAP_INIT_PARAMS cap_init_param;
    memset(&cap_init_param, 0, sizeof(T_CAP_INIT_PARAMS));
    cap_init_param.cap_role = (CAP_INITIATOR_ROLE | CAP_COMMANDER_ROLE);
#if LE_AUDIO_MCP_CLIENT_SUPPORT
    cap_init_param.mcp_media_control_client = true;
#endif
#if LE_AUDIO_MCP_SERVER_SUPPORT
    if (p_le_audio_mgr->service_support & LE_AUDIO_MCS_SERV_FLAG)
    {
        cap_init_param.mcs.mcp_media_control_server = true;
        cap_init_param.mcs.mcs_num = 1;
        if (p_le_audio_mgr->service_support & LE_AUDIO_OTS_SERV_FLAG)
        {
            cap_init_param.mcs.ots_num = 1;
        }
    }
#endif
#if LE_AUDIO_CCP_SERVER_SUPPORT
    if (p_le_audio_mgr->service_support & LE_AUDIO_TBS_FLAG)
    {
        cap_init_param.tbs.ccp_call_control_server = true;
        cap_init_param.tbs.tbs_num = 1;
    }
#endif
#if LE_AUDIO_CSIS_SUPPORT
    cap_init_param.csip_set_coordinator = true;
    le_csis_srv_init(&cap_init_param);
#endif
    cap_init_param.cas_client = true;
    cap_init_param.vcp_micp.vcp_vcs_client = true;
    cap_init_param.vcp_micp.vcp_aics_client = true;
    cap_init_param.vcp_micp.vcp_aics_cfg = AUDIO_DESCRIPTION_STORAGE;
    cap_init_param.vcp_micp.vcp_vocs_client = true;
    cap_init_param.vcp_micp.vcp_vocs_cfg = AUDIO_DESCRIPTION_STORAGE;
    cap_init_param.vcp_micp.micp_mic_controller = true;
    cap_init(&cap_init_param);

#if LE_AUDIO_MCP_SERVER_SUPPORT
    if (p_le_audio_mgr->service_support & LE_AUDIO_MCS_SERV_FLAG)
    {
        T_MCP_SERVER_REG_SRV_PARAM reg_srv_param = {0};

        reg_srv_param.gmcs = true;
        reg_srv_param.char_media_control_point.support = true;

        p_le_audio_mgr->mcs_id = mcp_server_reg_srv(&reg_srv_param);
        if (p_le_audio_mgr->mcs_id != 0xFF)
        {
            T_MCP_SERVER_SET_PARAM set_param = {0};

            set_param.char_uuid = MCS_UUID_CHAR_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED;
            set_param.param.media_control_point_opcodes_supported = 0x181F;

            mcp_server_set_param(p_le_audio_mgr->mcs_id, &set_param);
            le_media_player_init(p_le_audio_mgr->mcs_id);
        }
    }
#endif

#if LE_AUDIO_CCP_SERVER_SUPPORT
    if (p_le_audio_mgr->service_support & LE_AUDIO_TBS_FLAG)
    {
        T_CCP_SERVER_REG_SRV_PARAM p_gtbs_param = {0};

        p_gtbs_param.gtbs = true;
        p_gtbs_param.char_bearer_uri_schemes_supported_list.optional_property_notify = true;
        p_gtbs_param.char_bearer_signal_strength.support = false;
        p_gtbs_param.char_incoming_call_target_bearer_uri.support = false;
        p_gtbs_param.char_call_friendly_name.support = false;

        p_le_audio_mgr->ccp_id = ccp_server_reg_srv(&p_gtbs_param);
        teams_call_control_init(p_le_audio_mgr->ccp_id);
    }
#endif

#if LE_AUDIO_PACS_SUPPORT
    le_pacs_init();
#endif
#if LE_AUDIO_ASCS_SUPPORT
    le_unicast_sink_init();
#endif
    le_unicast_src_service_init(p_le_audio_mgr->mcs_id, p_le_audio_mgr->ccp_id);
#if LE_AUDIO_BASS_SUPPORT
    le_audio_bass_init();
#endif

#if LE_AUDIO_VCS_SUPPORT
    le_vcs_init();
#endif

    tmas_init(TMAS_CG_ROLE | TMAS_CT_ROLE | TMAS_UMS_ROLE |
              TMAS_UMR_ROLE | TMAS_BMS_ROLE | TMAS_BMR_ROLE);

    bs_init();

    extern bool gap_vendor_le_set_host_feature(uint16_t bit_number, uint8_t bit_value);
    gap_vendor_le_set_host_feature(0, 1);
}
