#include <string.h>
#include <stdlib.h>

#include "trace.h"
#include "os_queue.h"
#include "os_msg.h"
#include "app_main.h"

#if UAL_CONSOLE_PRINT
#include "console.h"
#endif
#include "ual_adapter.h"
#include "app_usb_layer.h"
#include "app_adapter_service.h"
#include "le_bst_src_service.h"
#include "le_ba_service.h"
#include "le_bsk_service.h"
#include "le_csis_client_service.h"
#include "le_unicast_src_service.h"
#include "le_vc_service.h"
#include "app_usb_hid.h"
#include "teams_call_control.h"
#include "dev_mgr.h"
#include "le_mic_service.h"
#include "bap_unicast_client.h"
#include "app_ctrl_cfg.h"
#include "app_le_audio.h"
#include "le_vcs_service.h"
#if DONGLE_LE_AUDIO
#include "app_le_audio.h"
#endif
#if (LE_AUDIO_GAMING_SUPPORT == 1)
#include "le_unicast_gaming.h"
#endif

#if LEGACY_BT_GAMING
#include "app_gaming_ctrl_cfg.h"
#endif

static uint32_t host_start_scan = 0;
static uint32_t host_start_ba_scan = 0;
static bool host_tool_is_enabled = false;

static void cmd_complete_event(uint16_t opcode, uint8_t status)
{
    uint8_t report[5];
    uint8_t *pp = report;
    UINT16_TO_STREAM(pp, COMMAND_COMPLETE_EVENT);
    UINT16_TO_STREAM(pp, opcode);
    UINT8_TO_STREAM(pp, status);

#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(5, report);
    }
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, sizeof(report));
#endif

}

bool le_audio_uac_vol_change(uint8_t vol, uint8_t mute)
{
    uint8_t  evt;
    T_IO_MSG msg;

    evt = EVENT_IO_TO_APP;
    msg.type    = IO_MSG_TYPE_APP_USB_LAYER;
    msg.subtype = UAC_VOL_CHANGE_OPCODE;
    msg.u.param = (vol << 8) | mute;

    if (os_msg_send(audio_io_queue_handle, &msg, 0))
    {
        return  os_msg_send(audio_evt_queue_handle, &evt, 0);
    }

    return false;
}

void bond_state_change(T_BT_BOND_INFO *p_bond_info)
{
    uint8_t report[10];
    uint8_t *pp = report;
    if (p_bond_info->state == BT_BOND_STATE_NONE)
    {
        bs_rmv_snk_from_group(p_bond_info->bd_addr, false);
    }
    le_csis_client_handle_bond_state_change(p_bond_info);
    UINT16_TO_STREAM(pp, BOND_STATE_CHANGE_EVENT);
    UINT8_TO_STREAM(pp, p_bond_info->bd_type);
    memcpy(pp, p_bond_info->bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    UINT8_TO_STREAM(pp, p_bond_info->state);

#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(10, report);
    }
#endif

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[40];
    uint16_t buf_len;
    buf_len = sprintf((char *)event_buff, "bond state change: %d\r\n", p_bond_info->state);

    console_write(event_buff, buf_len);
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, sizeof(report));
#endif
}

void le_audio_state_play_callback(uint8_t mode, uint8_t state)
{
    uint8_t report[4];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, AUDIO_PLAY_STATE_EVENT);
    UINT8_TO_STREAM(pp, mode);
    UINT8_TO_STREAM(pp, state);
#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, 4);
#endif
}


void scan_results_callback(uint8_t *bd_addr, uint8_t bd_type, uint8_t connect_mode,
                           uint8_t adv_sid, uint8_t *name, uint16_t name_len, int8_t rssi, uint8_t gaming_mode)
{
    uint8_t *report = calloc(1, name_len + 14);
    if (report == NULL)
    {
        return;
    }
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, SCAN_RESULT_INFO_EVENT);
    UINT8_TO_STREAM(pp, bd_type);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    UINT8_TO_STREAM(pp, connect_mode);
    UINT8_TO_STREAM(pp, adv_sid);
    UINT8_TO_STREAM(pp, name_len);
    memcpy(pp, name, name_len);
    pp += name_len;
    UINT8_TO_STREAM(pp, rssi);
    UINT8_TO_STREAM(pp, gaming_mode);
#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(name_len + 14, report);
    }
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, name_len + 14);
#endif

    free(report);
}

bool scan_state_changed_signal(uint8_t state)
{
    uint8_t  evt;
    T_IO_MSG msg;

    evt = EVENT_IO_TO_APP;
    msg.type    = IO_MSG_TYPE_APP_USB_LAYER;
    msg.subtype = SCAN_STATE_CHANGE_EVENT;
    msg.u.param = (uint32_t)state;

    if (os_msg_send(audio_io_queue_handle, &msg, 0))
    {
        return  os_msg_send(audio_evt_queue_handle, &evt, 0);
    }

    return false;
}


void scan_state_changed(uint8_t state)
{
    uint8_t report[3];
    uint8_t *pp = report;
    if (host_start_scan > 0 || host_start_ba_scan > 0)
    {
        UINT16_TO_STREAM(pp, SCAN_STATE_CHANGE_EVENT);
        UINT8_TO_STREAM(pp, state);
#if USB_TRANSFER_DATA
        if (host_tool_is_enabled)
        {
            app_usb_hid_send_bt_ctrl_data(3, report);
        }
#endif
        if (state == LE_SCAN_OFF)
        {
            host_start_scan = 0;
            host_start_ba_scan = 0;
        }
#if UAL_CONSOLE_PRINT
        uint8_t event_buff[30];
        uint16_t buf_len;
        buf_len =  sprintf((char *)event_buff, "scan state %d\r\n", state);

        console_write(event_buff, buf_len);
#endif
#if DONGLE_LE_AUDIO
        app_le_audio_parse_event(report, sizeof(report));
#endif
    }
}

void ble_usb_audio_state_change(uint8_t *bd_addr, uint8_t bd_type, uint8_t conn_state,
                                uint8_t audio_state, uint16_t disc_cause)
{
    uint8_t report[12];
    uint8_t state = 0;
    uint8_t *pp = report;
    UINT16_TO_STREAM(pp, DEVICE_STATE_CHANGE_EVENT);
    UINT8_TO_STREAM(pp, bd_type);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    if (conn_state == BT_CONN_STATE_CONNECTED)
    {
        state = CONNECT_STATE;
    }

    state |= audio_state;
    UINT8_TO_STREAM(pp, state);
    UINT16_TO_STREAM(pp, disc_cause);
#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(sizeof(report), report);
    }
#endif

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[100];
    uint16_t buf_len;
    buf_len =  sprintf((char *)event_buff,
                       "addr:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x  connection state 0x%x\r\n",
                       bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5], state);

    console_write(event_buff, buf_len);
#endif
#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, sizeof(report));
#endif
}

void device_pac_info_update_callback(uint8_t *bd_addr, uint8_t bd_type, uint8_t role,
                                     uint16_t sup_freq, uint8_t chnl_cnts, uint16_t prefer_context,
                                     uint8_t metadata_len, uint8_t *p_metadata)
{
    uint8_t *report = calloc(1, 19 + metadata_len);
    if (report == NULL)
    {
        return;
    }
    uint8_t *pp = report;
    uint8_t len = 0;
    UINT16_TO_STREAM(pp, DEVICE_PROPS_UPDATE_EVENT);
    UINT8_TO_STREAM(pp, bd_type);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    pp++;

    //pac record
    len += 8;
    UINT8_TO_STREAM(pp, 7);
    UINT8_TO_STREAM(pp, AUDIO_PAC_RECORD_PROP);
    UINT8_TO_STREAM(pp, role);
    UINT16_TO_STREAM(pp, sup_freq);
    UINT8_TO_STREAM(pp, chnl_cnts);
    UINT16_TO_STREAM(pp, prefer_context);
    UINT8_TO_STREAM(pp, metadata_len);
    for (uint8_t i = 0; i < metadata_len; i++)
    {
        UINT8_TO_STREAM(pp, p_metadata[i]);
    }

    report[9] = len;

# if(LE_AUDIO_GAMING_SUPPORT == 1)
    le_gaming_handle_pac_info_update(bd_addr, bd_type, role, sup_freq, chnl_cnts, prefer_context,
                                     metadata_len, p_metadata);
#endif

#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(19 + metadata_len, report);
    }
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, 19 + metadata_len);
#endif

    free(report);

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[50];
    uint16_t buf_len;
    buf_len =  sprintf((char *)event_buff,
                       "#role : 0x%x, sup_freq: 0x%x, chnl_cnts: 0x%x\r\n", role, sup_freq, chnl_cnts);

    console_write(event_buff, buf_len);
#endif
}

void device_properities_update_callback(uint8_t *bd_addr, uint8_t bd_type,
                                        uint16_t snk_sup_context,
                                        uint16_t src_sup_context,
                                        uint16_t snk_avail_context, uint16_t src_avail_context,
                                        uint32_t snk_audio_loc, uint32_t src_audio_loc)
{
    uint8_t *report = calloc(1, 32);
    if (report == NULL)
    {
        return;
    }
    uint8_t *pp = report;
    uint8_t len = 0;
    UINT16_TO_STREAM(pp, DEVICE_PROPS_UPDATE_EVENT);
    UINT8_TO_STREAM(pp, bd_type);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    pp++;


    //supported context
    len += 6;
    UINT8_TO_STREAM(pp, 5);
    UINT8_TO_STREAM(pp, AUDIO_SUPPORTED_CONTEXTS_PROP);
    UINT16_TO_STREAM(pp, snk_sup_context);
    UINT16_TO_STREAM(pp, src_sup_context);

    //available context
    len += 6;
    UINT8_TO_STREAM(pp, 5);
    UINT8_TO_STREAM(pp, AUDIO_AVAILABLE_CONTEXTS_PROP);
    UINT16_TO_STREAM(pp, snk_avail_context);
    UINT16_TO_STREAM(pp, src_avail_context);

    //audio location
    len += 10;
    UINT8_TO_STREAM(pp, 9);
    UINT8_TO_STREAM(pp, SUPPORT_AUDIO_CHANNEL_ALOC_PROP);
    UINT32_TO_STREAM(pp, snk_audio_loc);
    UINT32_TO_STREAM(pp, src_audio_loc);

    report[9] = len;

#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(32, report);
    }
#endif

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[60];
    uint16_t buf_len;
    buf_len =  sprintf((char *)event_buff,
                       "snk_avail_context: 0x%x, src_avail_context: 0x%x\r\n",
                       snk_avail_context, src_avail_context);

    console_write(event_buff, buf_len);
    buf_len =  sprintf((char *)event_buff,
                       "snk_audio_loc: 0x%x, src_audio_loc: 0x%x \r\n",
                       snk_audio_loc, src_audio_loc);

    console_write(event_buff, buf_len);
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, 32);
#endif

    free(report);

}


void device_cfg_report_callback(uint8_t *bd_addr, uint8_t bd_type, uint8_t *p_data, uint16_t len)
{
    uint16_t report_len = len + 10;
    uint8_t *report = calloc(1, report_len);
    if (report == NULL)
    {
        return;
    }
    uint8_t *pp = report;
    UINT16_TO_STREAM(pp, DEVICE_CONFIG_INFO_EVENT);
    UINT8_TO_STREAM(pp, bd_type);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    UINT8_TO_STREAM(pp, len);
    memcpy(pp, p_data, len);
#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(report_len, report);
    }
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, report_len);
#endif

    free(report);
}


static bool set_device_configurations(uint8_t *p_data, uint16_t len)
{
    uint8_t mode;
    uint8_t length, type;
    uint8_t *pp = p_data;
    STREAM_TO_UINT8(mode, pp);
    len--;
    pp += 7;
    uint8_t ltv_len = *pp++;
    uint8_t index = 0;
    T_UNICAST_ASE_PARAM param[4];
    uint8_t param_num = 0;
    bool ret = false;
    while (index < ltv_len)
    {
        length = *pp++;
        type = *pp++;
        index += (1 + length);

        switch (type)
        {
        case CURRENT_AUDIO_CHANNEL_ALOC_PROP:
            {
                if (mode == BROADCAST_MODE)
                {
                    uint32_t location;
                    location = *(uint32_t *)pp;
                    ret = bs_set_device_conf((p_data + 2), location);
                    return ret;
                }
            }
            break;


        case AUDIO_ASE_ID_CONFIG_PROP:
            {
                if (mode == UNICAST_MEDIA_MODE ||
                    mode == UNICAST_CONVERSATION_MODE)
                {
                    param[param_num].ase_id = *pp;
                    //skip ase state
                    param[param_num].real_ase_id = 0;
                    param[param_num].ase_id_associated = 0;
                    param[param_num].direction = *(pp + 2);
                    param[param_num].sample_freq = *(pp + 3);
                    param[param_num].chnl_loc = *(uint32_t *)(pp + 4);
                    param[param_num].prefer_codec_index = *(pp + 8);
                    param_num++;
                    ret = le_audio_add_unicast_user_cfg(p_data + 2, mode, param_num, param);
                }
                else
                {
                    return false;
                }
            }
            break;
        default:
            return false;
        }
        pp += (length - 1);
    }

    //ret = le_audio_add_unicast_cfg(p_data + 2, mode, param_num, param);
    return ret;
}

void ba_bst_src_info_callback(T_BROADCAST_SRC_INFO *p_bc_source)
{
    uint8_t i, j;
    uint8_t total_len = 0;
    uint8_t *report = NULL;
    uint8_t *pp = NULL;
    uint32_t bis_array = 0;
    uint8_t src_info_len = 0;
    uint16_t report_len = 255;

    if (p_bc_source == NULL)
    {
        APP_PRINT_ERROR0("ba_bst_src_info_callback source fail");
        return;
    }

    report = calloc(1, report_len);
    if (report == NULL)
    {
        APP_PRINT_ERROR0("ba_bst_src_info_callback alloc report fail");
        return;
    }
    pp = report;

    UINT16_TO_STREAM(pp, BROADCAST_SRC_INFO_REPORT_EVENT);
    UINT8_TO_STREAM(pp, p_bc_source->advertiser_sid);
    memcpy(pp, p_bc_source->broadcast_id, 3);
    pp += 3;
    UINT8_TO_STREAM(pp, p_bc_source->adv_addr_type);
    memcpy(pp, p_bc_source->adv_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    total_len += 13;

    //rssi
    UINT8_TO_STREAM(pp, p_bc_source->rssi);
    total_len++;
    //feature
    UINT8_TO_STREAM(pp, p_bc_source->feature);
    total_len++;
    //pbp
    UINT8_TO_STREAM(pp, p_bc_source->pbp_len);
    total_len++;
    if (p_bc_source->pbp_len)
    {
        memcpy(pp, p_bc_source->pbp_ltv, p_bc_source->pbp_len);
        pp += p_bc_source->pbp_len;
        total_len += p_bc_source->pbp_len;
    }
    //broadcast_name_len
    UINT8_TO_STREAM(pp, p_bc_source->broadcast_name_len);
    total_len ++;
    if (p_bc_source->broadcast_name_len)
    {
        memcpy(pp, p_bc_source->broadcast_name, p_bc_source->broadcast_name_len);
        pp += p_bc_source->broadcast_name_len;
        total_len += p_bc_source->broadcast_name_len;
    }

    //skip length
    pp++;
    total_len++;

    //skip LTK length
    pp++;
    total_len++;

    UINT8_TO_STREAM(pp, BROADCAST_SRC_INFO_PROP);
    UINT8_TO_STREAM(pp, p_bc_source->num_subgroups);
    src_info_len += 2;

    for (i = 0; i < p_bc_source->num_subgroups; i++)
    {
        T_BROADCAST_SRC_GRP_PARAMS *p_subgroup = p_bc_source->bst_groups + i;
        UINT8_TO_STREAM(pp, p_subgroup->subgroup_idx);
        UINT8_TO_STREAM(pp, p_subgroup->metadata_len);
        if (p_subgroup->metadata_len > 0)
        {
            memcpy(pp, p_subgroup->metadata, p_subgroup->metadata_len);
        }
        pp += p_subgroup->metadata_len;
        src_info_len += (2 + p_subgroup->metadata_len);
        UINT8_TO_STREAM(pp, p_subgroup->num_bis);
        src_info_len++;
        for (j = 0; j < p_subgroup->num_bis; j++)
        {
            T_BROADCAST_SRC_BIS_PARAMS *p_bis_param = p_subgroup->bis_param + j;
            bis_array |= (1 << (p_bis_param->bis_index - 1));
            UINT8_TO_STREAM(pp, p_bis_param->bis_index);
            UINT8_TO_STREAM(pp, p_bis_param->sample_frequency);
            UINT32_TO_STREAM(pp, p_bis_param->chnnl_allocation);
            src_info_len += 6;
        }
    }

    report[total_len - 2] = src_info_len + 1;
    report[total_len - 1] = src_info_len;

    total_len += src_info_len;
    APP_PRINT_INFO2("ba_bst_src_info_callback len %d data %b", total_len, TRACE_BINARY(total_len,
                    report));
#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(total_len, report);
    }
#endif

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[50];
    uint16_t buf_len;
    buf_len =  sprintf((char *)event_buff,
                       "adv_sid: 0x%x, bis_array: 0x%x\r\n", p_bc_source->advertiser_sid, bis_array);

    console_write(event_buff, buf_len);
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, total_len);
#endif

    free(report);
}

void bst_src_sync_state_callback(uint8_t *adv_addr, uint8_t adv_addr_type, uint8_t advertiser_sid,
                                 uint8_t sync_state)
{
    uint8_t report[11];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, BROADCAST_ASSIST_SYNC_STATE_EVENT);
    UINT8_TO_STREAM(pp, advertiser_sid);
    UINT8_TO_STREAM(pp, adv_addr_type);
    memcpy(pp, adv_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    UINT8_TO_STREAM(pp, sync_state);
#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(11, report);
    }
#endif

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[50];
    uint16_t buf_len;
    buf_len =  sprintf((char *)event_buff,
                       "adv_sid: 0x%x, sync_state: 0x%x\r\n", advertiser_sid, sync_state);

    console_write(event_buff, buf_len);
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, sizeof(report));
#endif


}

void ba_brs_report_callback(uint8_t *addr, uint8_t receiver_id,
                            T_BRS_INFO *p_brs_info)
{
    uint8_t report[21];
    uint8_t *pp = report;
    uint8_t sync_state = 0;

    APP_PRINT_INFO5("ba_brs_report_callback snk_addr %b src_addr %b broadcast_id %b pa_sync_state %x bis_sync_state %x",
                    TRACE_BDADDR(addr),
                    TRACE_BDADDR(p_brs_info->adv_addr),
                    TRACE_BINARY(3, p_brs_info->broadcast_id),
                    p_brs_info->pa_sync_state,
                    p_brs_info->bis_sync_state);
    UINT16_TO_STREAM(pp, BST_RECV_STATE_REPORT_EVENT);
    UINT8_TO_STREAM(pp, receiver_id);
    UINT8_TO_STREAM(pp, p_brs_info->advertiser_sid);
    memcpy(pp, p_brs_info->broadcast_id, 3);
    pp += 3;

    UINT8_TO_STREAM(pp, p_brs_info->adv_addr_type);
    memcpy(pp, p_brs_info->adv_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    if (p_brs_info->pa_sync_state == PA_SYNC_STATE_SYNC)
    {
        sync_state |= BSK_SYNC_PA_SYNCHRONIZED;
    }
    else if (p_brs_info->pa_sync_state == PA_SYNC_STATE_SYNC_INFO_REQ)
    {
        sync_state |= BSK_SYNC_PA_SYNCHRONIZING;
    }

    if (p_brs_info->bis_sync_state != 0 && p_brs_info->bis_sync_state != 0xFFFFFFFF)
    {
        sync_state |= BSK_SYNC_BIG_SYNCHRONIZED;
    }
    UINT8_TO_STREAM(pp, sync_state);

    memcpy(pp, addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;

#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(21, report);
    }
#endif

}


void set_mem_avail_report_callback(T_SET_MEM_AVAIL *set_report)
{
    uint8_t report[19];
    uint8_t *pp = report;

    if (set_report != NULL)
    {
        UINT16_TO_STREAM(pp, COORDINATE_SET_REPORT_EVENT);
        UINT8_TO_STREAM(pp, set_report->bd_type);
        memcpy(pp, set_report->bd_addr, BD_ADDR_LEN);
        pp += BD_ADDR_LEN;
        UINT8_TO_STREAM(pp, 9);
        UINT8_TO_STREAM(pp, 8);
        UINT8_TO_STREAM(pp, COORDINATOR_SET_DEV_ADDED_PROP);
        memcpy(pp, set_report->hash, 3);
        pp += 3;
        UINT16_TO_STREAM(pp, set_report->srv_uuid);
        UINT8_TO_STREAM(pp, set_report->rank);
        UINT8_TO_STREAM(pp, set_report->size);
#if USB_TRANSFER_DATA
        if (host_tool_is_enabled)
        {
            app_usb_hid_send_bt_ctrl_data(19, report);
        }
#endif

#if DONGLE_LE_AUDIO
        app_le_audio_parse_event(report, sizeof(report));
#endif


#if UAL_CONSOLE_PRINT
        uint8_t event_buff[100];
        uint16_t buf_len;
        buf_len = sprintf((char *)event_buff, "hash = [%02x:%02x:%02x] \r\n",
                          set_report->hash[0], set_report->hash[1], set_report->hash[2]);

        console_write(event_buff, buf_len);

        buf_len =  sprintf((char *)event_buff,
                           "coordinate set RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d\r\n",
                           set_report->bd_addr[5], set_report->bd_addr[4],
                           set_report->bd_addr[3], set_report->bd_addr[2],
                           set_report->bd_addr[1], set_report->bd_addr[0],
                           set_report->bd_type);

        console_write(event_buff, buf_len);

#endif


    }
}


void volume_setting_report_callback(uint8_t *bd_addr, uint8_t volume_setting, uint8_t mute)
{
    uint8_t report[10];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, VOLUME_SETTING_REPORT_EVENT);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    UINT8_TO_STREAM(pp, volume_setting);
    UINT8_TO_STREAM(pp, mute);

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[100];
    uint16_t buf_len;
    buf_len = sprintf((char *)event_buff, "addr %x:%x:%x:%x:%x:%x volume change %d, mute: %d\r\n",
                      bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
                      volume_setting, mute);

    console_write(event_buff, buf_len);
#endif
#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(10, report);
    }
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, sizeof(report));
#endif

#if (DONGLE_LE_AUDIO == 1)
#if F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME
    /* Don't handle vol notify */
#else
    app_le_audio_handle_vol_notify(bd_addr, volume_setting, mute);
#endif
#endif
}

void volume_offset_val_report_callback(uint8_t *bd_addr, uint8_t idx,
                                       int16_t volume_offset, uint32_t audio_location)
{
    uint8_t report[15];
    uint8_t *pp = report;
    UINT16_TO_STREAM(pp, VOLUME_OFFSET_REPORT_EVENT);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;

    UINT8_TO_STREAM(pp, idx);
    UINT16_TO_STREAM(pp, volume_offset);
    UINT32_TO_STREAM(pp, audio_location);

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[100];
    uint16_t buf_len;
    buf_len = sprintf((char *)event_buff, "addr %x:%x:%x:%x:%x:%x idx %d, volume_offset: %d\r\n",
                      bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
                      idx, volume_offset);

    console_write(event_buff, buf_len);
#endif

#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(15, report);
    }
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, sizeof(report));
#endif

}

void mic_mute_state_report_callback(uint8_t *bd_addr, uint8_t mute_state)
{
    uint8_t report[9];
    uint8_t *pp = report;
    UINT16_TO_STREAM(pp, MIC_MUTE_STATE_REPORT_EVENT);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;

    UINT8_TO_STREAM(pp, mute_state);

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[50];
    uint16_t buf_len;
    buf_len = sprintf((char *)event_buff, "addr %x:%x:%x:%x:%x:%x, mute: %d\r\n",
                      bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
                      mute_state);

    console_write(event_buff, buf_len);
#endif

#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(9, report);
    }
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, sizeof(report));
#endif

}

void audio_input_info_report_callback(uint8_t *bd_addr, uint8_t instant_id, T_AICS_SRV_DATA *p_data)
{
    uint8_t report[25];
    uint8_t *pp = report;
    uint8_t des_len = 7;
    UINT16_TO_STREAM(pp, AUDIO_INPUT_INFO_REPORT_EVENT);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;

    UINT8_TO_STREAM(pp, instant_id);  // length is 9
    if (p_data->input_des.input_des_len < 7)
    {
        des_len = p_data->input_des.input_des_len;
    }
    UINT8_TO_STREAM(pp, des_len + 8);

    UINT8_TO_STREAM(pp, AUDIO_INPUT_INFO_PROP);
    UINT8_TO_STREAM(pp, p_data->input_status);
    UINT8_TO_STREAM(pp, p_data->input_state.gain_setting);
    UINT8_TO_STREAM(pp, p_data->setting_prop.gain_setting_min);
    UINT8_TO_STREAM(pp, p_data->setting_prop.gain_setting_max);
    UINT8_TO_STREAM(pp, p_data->input_state.mute);
    UINT8_TO_STREAM(pp, p_data->input_state.gain_mode);
    UINT8_TO_STREAM(pp, p_data->input_type);              //length is 18

    memcpy(pp, p_data->input_des.p_input_des, des_len);

#if USB_TRANSFER_DATA
    if (host_tool_is_enabled)
    {
        app_usb_hid_send_bt_ctrl_data(18 + des_len, report);
    }
#endif

#if DONGLE_LE_AUDIO
    app_le_audio_parse_event(report, 18 + des_len);
#endif
}


static void report_ases_info(T_LE_AUDIO *p_audio_link, T_BT_DEVICE *p_dev_rec)
{
    if (p_audio_link && p_dev_rec)
    {
        uint16_t report_len = (p_audio_link->snk_ase_num + p_audio_link->src_ase_num) * 11;
        uint8_t *report = calloc(1, report_len);
        uint8_t *pp = report;
        if (report == NULL)
        {
            APP_PRINT_ERROR0("le_audio_get_ase_info alloc fail");
            return;
        }

        //FIX TODO we may just report ase id only
        for (int i = 0; i < (p_audio_link->snk_ase_num + p_audio_link->src_ase_num); i++)
        {
            T_ASE_CHAR_DATA ase_data;
            uint8_t direction;
            if (i < p_audio_link->snk_ase_num)
            {
                direction = SERVER_AUDIO_SINK;
            }
            else
            {
                direction = SERVER_AUDIO_SOURCE;
            }
            if (ascs_client_get_ase_data(p_audio_link->conn_handle,
                                         i < p_audio_link->snk_ase_num ? i : (i - p_audio_link->snk_ase_num),
                                         &ase_data, direction))
            {
                UINT8_TO_STREAM(pp, 10);
                UINT8_TO_STREAM(pp, AUDIO_ASE_ID_CONFIG_PROP);
                UINT8_TO_STREAM(pp, ase_data.ase_id);
                UINT8_TO_STREAM(pp, ase_data.ase_state);

                UINT8_TO_STREAM(pp, ase_data.direction);
                int j = 0;
                uint8_t len;
                uint8_t type;
                uint32_t cur_chnl_loc = 0;
                uint8_t cur_freq = 0;

                if (ase_data.ase_state == ASE_STATE_CODEC_CONFIGURED)
                {
                    uint8_t *p = ase_data.param.codec_configured.p_codec_spec_cfg;
                    while (j < ase_data.param.codec_configured.data.codec_spec_cfg_len)
                    {
                        len = *p++;
                        type = *p++;
                        j = (len + 1);
                        switch (type)
                        {
                        case CODEC_CFG_TYPE_SAMPLING_FREQUENCY:
                            {
                                cur_freq = *p;
                                p++;
                            }
                            break;
                        case CODEC_CFG_TYPE_FRAME_DURATION:
                            {
                                p++;
                            }
                            break;

                        case CODEC_CFG_TYPE_AUDIO_CHANNEL_ALLOCATION:
                            {
                                STREAM_TO_UINT32(cur_chnl_loc, p);
                            }
                            break;

                        case CODEC_CFG_TYPE_OCTET_PER_CODEC_FRAME:
                            {
                                p += 2;
                            }
                            break;

                        case CODEC_CFG_TYPE_BLOCKS_PER_SDU:
                            {
                                p++;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }

#if UAL_CONSOLE_PRINT
                uint8_t event_buff[60];
                uint16_t buf_len;
                buf_len =  sprintf((char *)event_buff,
                                   "ase_id: 0x%x, diection: 0x%x, ase_state: 0x%x\r\n", ase_data.ase_id, ase_data.direction,
                                   ase_data.ase_state);

                console_write(event_buff, buf_len);
                if (ase_data.ase_state == ASE_STATE_CODEC_CONFIGURED)
                {
                    buf_len =  sprintf((char *)event_buff,
                                       "cur_freq: 0x%x, cur_chnl_loc: 0x%x\r\n", cur_freq, cur_chnl_loc);

                    console_write(event_buff, buf_len);
                }
#endif

                UINT8_TO_STREAM(pp, cur_freq);
                UINT32_TO_STREAM(pp, cur_chnl_loc);
                UINT8_TO_STREAM(pp, 0xFF);
            }
        }
        if (p_dev_rec)
        {
            device_cfg_report_callback(p_dev_rec->pseudo_addr, p_dev_rec->bd_type, report, report_len);
        }
        free(report);
    }
}


static void report_bonded_devs_info()
{
    T_BT_DEVICE *p_dev_sec = ual_get_first_sec_dev();
    while (p_dev_sec)
    {
        if (p_dev_sec->le_bonded)
        {
            T_BT_BOND_INFO bond_info;
            bond_info.state = BT_BOND_STATE_BONDED;
            memcpy(bond_info.bd_addr, p_dev_sec->pseudo_addr, BD_ADDR_LEN);
            bond_info.bd_type = p_dev_sec->bd_type;
            bond_state_change(&bond_info);
        }
        if (p_dev_sec->ble_state == GAP_CONN_STATE_CONNECTED)
        {
            T_LE_AUDIO *p_le_audio = ble_audio_find_by_conn_id(p_dev_sec->le_conn_id);
            if (p_le_audio)
            {
                if ((p_le_audio->remote_serv_sup & LE_AUDIO_PACS_CLIENT_FLAG))
                {
                    T_BAP_DIS_ALL_DONE pacs_dis_info;
                    ble_usb_audio_state_change(p_dev_sec->pseudo_addr, p_dev_sec->bd_type, p_le_audio->state,
                                               p_le_audio->audio_state, 0);
                    pacs_dis_info.conn_handle = p_dev_sec->le_conn_handle;
                    pacs_dis_info.sink_pac_num = p_le_audio->snk_pac_num;
                    pacs_dis_info.source_pac_num = p_le_audio->src_pac_num;
                    pacs_dis_info.pacs_is_found = true;
                    ble_audio_get_pacs_info(&pacs_dis_info);
                }
                if ((p_le_audio->remote_serv_sup & LE_AUDIO_ASCS_CLIENT_FLAG))
                {
                    report_ases_info(p_le_audio, p_dev_sec);
                }
                for (uint8_t i = 0; i < p_le_audio->bass_brs_num; i++)
                {
                    ba_brs_report_callback(p_dev_sec->pseudo_addr, i, &p_le_audio->brs_cb[i]);
                }
                if (p_le_audio->remote_serv_sup & LE_AUDIO_VCS_CLIENT_FLAG)
                {
                    T_VOLUME_STATE vcs_data;
                    if (vcs_get_volume_state(p_le_audio->conn_handle, &vcs_data))
                    {
                        volume_setting_report_callback(p_dev_sec->pseudo_addr,
                                                       vcs_data.volume_setting,
                                                       vcs_data.mute);
                    }
                }
                if (p_le_audio->remote_serv_sup & LE_AUDIO_VOCS_CLIENT_FLAG)
                {
                    for (int i = 0; i < p_le_audio->srv_total_num; i++)
                    {
                        T_VOCS_SRV_DATA vocs_data;
                        if (vocs_get_srv_data(p_le_audio->conn_handle, i, &vocs_data))
                        {
                            volume_offset_val_report_callback(p_dev_sec->pseudo_addr, vocs_data.srv_instance_id,
                                                              vocs_data.volume_offset.volume_offset, vocs_data.audio_location);
                        }
                    }
                }
                if (p_le_audio->remote_serv_sup & LE_AUDIO_AICS_CLIENT_FLAG)
                {
                    for (int i = 0; i < p_le_audio->aics_num; i++)
                    {
                        T_AICS_SRV_DATA aics_data;
                        if (aics_get_srv_data(p_le_audio->conn_handle, i, &aics_data))
                        {
                            audio_input_info_report_callback(p_dev_sec->pseudo_addr, i, &aics_data);
                        }
                    }
                }

                if (p_le_audio->remote_serv_sup & LE_AUDIO_MICS_CLIENT_FLAG)
                {
                    mic_mute_state_report_callback(p_dev_sec->pseudo_addr, p_le_audio->mic_mute);
                }

            }
        }

        le_csis_client_report_mem_info(p_dev_sec->pseudo_addr, p_dev_sec->bd_type);
        p_dev_sec = ual_get_next_sec_dev(p_dev_sec);
    }
    uint8_t report[6];
    uint8_t *pp = report;
    uint8_t play_state;
    uint8_t qos;
    UINT16_TO_STREAM(pp, DONGLE_PLAY_STATE_EVENT);
    play_state = bs_get_play_info(&qos);
    UINT8_TO_STREAM(pp, play_state);
    UINT8_TO_STREAM(pp, qos);
    play_state = le_unicast_src_play_info(&qos, NULL);
    UINT8_TO_STREAM(pp, play_state);
    UINT8_TO_STREAM(pp, qos);

#if USB_TRANSFER_DATA
    app_usb_hid_send_bt_ctrl_data(6, report);
#endif

}

static void le_audio_handle_uac_vol(uint8_t vol, uint8_t mute)
{
#if DONGLE_LE_AUDIO
    app_le_audio_set_remote_vol(vol, mute);
#else
    if (mute)
    {
        le_vcs_usb_set_out_volume(mute);
    }
    else
    {
        le_vcs_usb_set_out_volume(vol + 2);
    }
#endif
}

static bool usb_layer_cmd_signal(uint16_t opcode)
{
    uint8_t  evt;
    T_IO_MSG msg;

    evt = EVENT_IO_TO_APP;
    msg.type    = IO_MSG_TYPE_APP_USB_LAYER;
    msg.subtype = opcode;

    if (os_msg_send(audio_io_queue_handle, &msg, 0))
    {
        return  os_msg_send(audio_evt_queue_handle, &evt, 0) == true;
    }
    return false;
}

void usb_layer_signal_handle(T_IO_MSG *msg)
{
    if (!msg)
    {
        return;
    }

    APP_PRINT_INFO1("usb_layer_signal_handle: subtype 0x%x", msg->subtype);
    uint32_t param = msg->u.param;
    switch (msg->subtype)
    {
    case GET_DEVICES_INFO_OPCODE:
        report_bonded_devs_info();
        break;
    case SCAN_STATE_CHANGE_EVENT:
        scan_state_changed((uint8_t)msg->u.param);
        break;
    case UAC_VOL_CHANGE_OPCODE:
        le_audio_handle_uac_vol((param >> 8) & 0xFF, param & 0xFF);
        break;
    default:
        break;
    }

}


uint8_t usb_parse_host_cmd(uint8_t *p_data, uint16_t len)
{
    uint8_t status = COMMAND_COMPLETE_SUCCESS;
    uint8_t *pp = p_data;
    uint16_t length = len;
    uint16_t opcode;
    STREAM_TO_UINT16(opcode, pp);
    length -= 2;
#if UAL_CONSOLE_PRINT
    uint8_t event_buff[100];
    uint16_t buf_len;
#endif

#ifdef LEGACY_BT_GENERAL
    if ((opcode & 0xff00) == GENERAL_RTK_START_OPCODE)
    {
        status = app_usb_hid_handle_general_cmd(p_data, len);
        return status;
    }
#endif

#ifdef LEGACY_BT_GAMING
    if ((opcode & 0xff00) == GAMING_RTK_START_OPCODE)
    {
        status = app_usb_hid_handle_gaming_cmd(p_data, len);
        return status;
    }
#endif

    switch (opcode)
    {
    case GET_DEVICES_INFO_OPCODE:
        {
            if (!usb_layer_cmd_signal(opcode))
            {
                status = COMMAND_OPERATION_FAIL;
                break;
            }
            host_tool_is_enabled = true;
        }
        break;

    case SCAN_START_OPCODE:
        {
            if (host_start_scan)
            {
                scan_state_changed(LE_SCAN_ON);
                break;
            }
            if (length < 2)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            uint16_t uuid;
            bool user_active_scan = false;
            bool has_white_list = false;

            STREAM_TO_UINT16(uuid, pp);
            STREAM_TO_UINT8(user_active_scan, pp);
            STREAM_TO_UINT8(has_white_list, pp);

            APP_PRINT_TRACE2("SCAN_START_OPCODE: user_active_scan %d has_white_list %d", user_active_scan,
                             has_white_list);

            host_start_scan = adapter_start_le_scan(uuid, has_white_list);
            if (host_start_scan == 0)
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;

    case SCAN_STOP_OPCODE:
        {
            if (host_start_scan)
            {
                adapter_stop_le_scan();
            }
            else
            {
                scan_state_changed(LE_SCAN_OFF);
            }
        }
        break;
    case SET_BROADCAST_TK_CODEC_OPCODE:
        {
            if (length < 2)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }

            if (!bs_set_track_codec(*pp, *(pp + 1)))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case START_BROADCAST_OPCODE:
        {
            if (length < 17)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            uint8_t state;
            STREAM_TO_UINT8(state, pp);
            if (state == BROADCAST_START)
            {
                if (!bs_start_broadcast(pp))
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
            else if (state == BROADCAST_STOP)
            {
                if (!bs_stop_broadcast())
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
            else if (state == BROADCAST_RELEASE)
            {
                bs_release_broadcast();
            }
        }
        break;
    case RMV_DEV_FROM_BSGRP_OPCODE:
        {
            bs_rmv_snk_from_group(pp + 1, true);
        }
        break;
    case ADD_DEV_TO_BSGRP_OPCODE:
        {
            if (length < 7)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if (!bs_add_snk_to_group(pp + 1))
            {
                status = COMMAND_OPERATION_FAIL;;
                pp++;
            }
        }
        break;
    case SET_DEVICE_CONF_OPCODE:
        {
            if (length < 10)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if (!set_device_configurations(pp, length))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;

    case RMV_DEVICES_CONF_OPCODE:
        {
            uint8_t addr_num;
            if (length < 1)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            addr_num = pp[0];
            if (length < (1 + addr_num * BD_ADDR_LEN))
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if (!le_audio_clear_unicast_user_cfg(addr_num, pp + 1))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;

    case ENABLE_BST_SYNC_OPCODE:
        {
            uint8_t enable;
            uint8_t past;
            uint8_t prefer;
            if (length < 10)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            enable = pp[7];
            past = pp[8];
            prefer = pp[9];
            if (enable)
            {
                if (!bs_start_sync(pp + 1, past, prefer))
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
            else
            {
                if (!bs_stop_sync(pp + 1))
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
        }
        break;

    case TRANSFER_OPERATION_OPCODE:
        {
            if (length < 2)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if (*pp == TRANSFER_MEDIA_START || *pp == TRANSFER_CONVERSATION_START)
            {
                extern bool le_unicast_is_gaming_mode();
                if (le_unicast_src_play_info(NULL, NULL))     //already start by media flow
                {
                    status = COMMAND_COMPLETE_SUCCESS;
                }
                else if (!le_audio_user_start_unicast_stream(*pp, *(pp + 1), le_unicast_is_gaming_mode()))
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
            else if (*pp == TRANSFER_STOP)
            {
                if (!le_unicast_stop_stream())
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
            else if (*pp == TRANSFER_SUSPEND)
            {
                if (!le_unicast_suspend_stream())
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
            else
            {
                status = INVALID_COMMAND_PARAM;
            }
        }
        break;

    case CREATE_BOND_OPCODE:
        {
            if (length < 8)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            pp++; //FIX TODO skip bond type
            pp++; //skip bd type
#if UAL_CONSOLE_PRINT
            buf_len = sprintf((char *)event_buff, "bond addr : %x %x %x %x %x %x\r\n", pp[0], pp[1], pp[2],
                              pp[3], pp[4], pp[5]);

            console_write(event_buff, buf_len);
#endif
            if (bt_dev_create_bond(pp, *(pp - 1)) != BT_STATUS_SUCCESS)
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case REMOVE_BOND_OPCODE:
        {
            if (length < 7)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }

            if (bt_dev_remove_bond(pp + 1) != BT_STATUS_SUCCESS)
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case CONNECT_DEVICE_OPCODE:
        {
            if (length < 8)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            pp++;
            if (bt_dev_le_gatt_connect(&pp[1], *pp, DEV_MGR_APP_ID) != BT_STATUS_SUCCESS)
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case DISCONNECT_DEVICE_OPCODE:
        {
            if (length < 8)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            pp++;
            struct bt_device *p_dev_rec = ual_find_device_by_addr(&pp[1]);
            if (!p_dev_rec)
            {
                status = COMMAND_OPERATION_FAIL;
                break;
            }
            T_APP_ID app_id = 0;
            uint32_t le_connect_mask = p_dev_rec->le_connect_mask;
            APP_PRINT_INFO1("DISCONNECT_DEVICE_OPCODE : le_connect_mask 0x%x", le_connect_mask);
            while (le_connect_mask != 0)
            {
                if (le_connect_mask & 0x01)
                {
                    bt_dev_le_gatt_disconnect(p_dev_rec->pseudo_addr, app_id);
                }
                le_connect_mask = le_connect_mask >> 1;
                app_id++;
            }
        }
        break;
    case BROADCAST_ASSIS_DISC_OPCODE:
        {
            uint8_t enable;
            if (length != 1)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            enable = pp[0];

            if (enable == 0)
            {
                if (host_start_ba_scan > 0)
                {
                    ba_stop_le_scan();
                }
            }
            else
            {
                if (host_start_ba_scan)
                {
                    break;
                }
                host_start_ba_scan = ba_start_le_scan();
                if (host_start_ba_scan == 0)
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
        }
        break;
    case BC_AS_SELEC_BC_SRC_OPCODE:
        {
            if (length < 27)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            ba_select_broadcast_src(pp + 5, *(pp + 4), *pp, pp + 1, pp + 11);
        }
        break;
    case BC_AS_SYNC_BC_SRC_OPCODE:
        {
            uint8_t enable;
            if (length < 12)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            enable = pp[11];
            if (enable)
            {
                if (!ba_pa_sync(pp + 5, *(pp + 4), *pp, pp + 1))
                {
                    status = COMMAND_OPERATION_FAIL;
                }
                else
                {
#if UAL_CONSOLE_PRINT
                    buf_len = sprintf((char *)event_buff,
                                      "ba sync start : %d\r\n", 1);

                    console_write(event_buff, buf_len);
#endif
                }
            }
            else
            {
                if (!ba_pa_terminate(pp + 2, *(pp + 1), *pp, pp + 1))
                {
                    status = COMMAND_OPERATION_FAIL;
                }
                else
                {
#if UAL_CONSOLE_PRINT
                    buf_len = sprintf((char *)event_buff,
                                      "ba sync cancel : %d\r\n", 1);

                    console_write(event_buff, buf_len);
#endif
                }
            }
        }
        break;
    case BC_AS_SET_REMOTE_SYNC_OPCODE:
        {
            if (length < 11)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            uint32_t bis_array = *(uint32_t *)(pp + 7);
            if (!ba_start_remote_sync(*pp, pp + 1, bis_array))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;

    case BC_AS_STOP_REMOTE_SYNC_OPCODE:
        {
            if (length < 7)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if (!ba_stop_remote_sync(pp, pp[6]))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;

    case BC_AS_MODIFY_REMOTE_SYNC_OPCODE:
        {
            if (length < 11)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            uint32_t bis_array = *(uint32_t *)(pp + 7);
            if (!ba_modify_remote_sync(pp, pp[6], bis_array))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case BC_SNK_SELEC_BC_SRC_OPCODE:
        {
            if (length < 27)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            bsnk_select_broadcast_src(pp + 5, *(pp + 4), *pp, pp + 1, pp + 11);
        }
        break;

    case BC_SNK_SYNC_BC_SRC_OPCODE:
        {
            uint8_t enable;
            if (length < 13)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            enable = pp[11];
#if UAL_CONSOLE_PRINT
            buf_len = sprintf((char *)event_buff, "big sync enable : %d\r\n", enable);

            console_write(event_buff, buf_len);
#endif

            if (enable == 1)
            {
                if (!bsnk_big_establish(pp + 5, pp[4], pp[0], pp + 1, pp[12], (pp + 13)))
                {
                    status = COMMAND_OPERATION_FAIL;

                }
            }
            else if (enable == 0)
            {
                if (!bsnk_big_terminate(pp + 5, *(pp + 4), *pp, pp + 1))
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
            else
            {
                if (!bsnk_big_release(pp + 5, *(pp + 4), *pp, pp + 1))
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
        }
        break;
    case ENABLE_DISCOVER_SET_MEMBERS:
        {
            if (length < 4)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            uint8_t enable = pp[0];
            if (enable == 0)
            {
                le_csis_client_stop_scan();
            }
            else
            {
                if (le_csis_client_start_scan(pp + 1) < 0)
                {
                    status = COMMAND_OPERATION_FAIL;
                }
            }
        }
        break;

    case CONNECT_ALL_SET_MEMBERS:
        {
            if (length < 3)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if (!le_csis_client_connect_group_all_mems(pp))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;

    case SET_REMOTE_VOL_VAL_OPCODE:
        {
            if (length < 8)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if (!le_audio_set_remote_volume(pp + 1, *(pp + 7)))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case SET_REMOTE_MUTE_OPCODE:
        {
            if (length < 8)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if (!le_audio_set_remote_mute(pp + 1, *(pp + 7)))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case SET_VOLUME_OFFSET_OPCODE:
        {
            if (length < 10)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            pp++;   //skip address type
            int16_t offset = (int16_t)(pp[7] | pp[8] << 8);
            if (!le_audio_set_device_volume_offset(pp, *(pp + 6), offset))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case SET_MIC_MUTE_STATE_OPCODE:
        {
            if (length < 7)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }

            if (!le_mic_set_remote_mute(pp, *(pp + 6)))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case SET_AUDIO_INPUT_GAIN_VALUE_OPCODE:
        {
            if (length < 8)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }

            if (!le_audio_set_device_input_gain(pp, *(pp + 6), (int8_t)(*(pp + 7))))
            {
                status = COMMAND_OPERATION_FAIL;
            }

        }
        break;
    case SET_AUDIO_INPUT_MUTE_OPCODE:
        {
            if (length < 8)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if ((*(pp + 7)) > AICS_MUTE_DISABLED)
            {
                status = INVALID_COMMAND_PARAM;
            }
            else if (!le_audio_set_device_input_gain(pp, *(pp + 6), *(pp + 7)))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case SET_AUDIO_INPUT_GAIN_MODE_OPCODE:
        {
            if (length < 8)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }
            if ((*(pp + 7)) > AICS_GAIN_MODE_AUTOMATIC)
            {
                status = INVALID_COMMAND_PARAM;
            }
            else if (!le_audio_set_device_input_gain_mode(pp, *(pp + 6), *(pp + 7)))
            {
                status = COMMAND_OPERATION_FAIL;
            }
        }
        break;
    case UAC_VOL_CHANGE_OPCODE:
        {
            if (length < 2)
            {
                status = INVALID_COMMAND_LENGTH;
                break;
            }

        }
        break;
    case 0x5DF2:
        {
#if DONGLE_LE_AUDIO
            app_le_audio_set_pair_filter(*pp, *(pp + 1), pp + 2, length - 2);
#endif
        }
        break;
    case MODIFY_LE_WHITE_LIST:
        {
            T_GAP_WHITE_LIST_OP op = (T_GAP_WHITE_LIST_OP)pp[0];
            T_GAP_REMOTE_ADDR_TYPE type = (T_GAP_REMOTE_ADDR_TYPE)pp[1];
            uint8_t addr[6] = {0x0};
            APP_PRINT_INFO3("MODIFY_LE_WHITE_LIST op %x type %x bd %b", op, type, TRACE_BDADDR(pp + 2));
            if (op)
            {
                for (uint8_t i = 0; i < 6; i++)
                {
                    addr[i] = pp[7 - i];
                }
            }
            le_modify_white_list(op, addr, type);
        }
        break;

    default:
        status = UNKNOWN_COMMAND;
        break;

    }
#if UAL_CONSOLE_PRINT
    buf_len = sprintf((char *)event_buff, "---->opcode 0x%x, status 0x%x \r\n", opcode, status);
    console_write(event_buff, buf_len);
#endif
    APP_PRINT_INFO3("usb_parse_host_cmd opcode 0x%x, data %b, status %d", opcode, TRACE_BINARY(len,
                    p_data), status);

    cmd_complete_event(opcode, status);
    return status;
}

static void app_usb_ctrl_data_recv_callback(uint16_t length, uint8_t *p_data)
{
    usb_parse_host_cmd(p_data, length);
}


static APP_USB_HID_CB_F app_usb_func_cb =
{
    app_usb_ctrl_data_recv_callback,
#if LE_AUDIO_CCP_SERVER_SUPPORT
    teams_call_handle_telephony_msg,
#else
    NULL,
#endif
};


void app_usb_layer_init(void)
{
#if F_APP_USB_HID_SUPPORT
    app_usb_hid_register_cbs(&app_usb_func_cb);
#endif
}
