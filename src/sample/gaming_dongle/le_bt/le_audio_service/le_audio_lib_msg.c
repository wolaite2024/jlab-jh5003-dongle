#include "trace.h"
#include "le_audio_lib_msg.h"
#include "bass_client.h"
#include "le_audio_service.h"
#include "os_mem.h"
#include "vector.h"
#include "pacs_client.h"
#include "vcs_mgr.h"
#include "vocs_client.h"
#include "vcs_client.h"
#include "csis_client.h"

#include "aics_client.h"
#include "codec_def.h"
#include "vcs_def.h"
#include "ble_audio_def.h"
#include "bap.h"
#include "mcp_mgr.h"
#include "cap.h"
#include "ccp_mgr.h"

#include "dev_mgr.h"
#include "le_bst_src_service.h"
#include "le_ba_service.h"
#include "le_bass_service.h"

#include "app_usb_layer.h"
#include "le_media_player.h"
#include "teams_call_control.h"
#include "le_pacs_service.h"
#include "le_unicast_sink_service.h"
#include "le_unicast_src_service.h"
#include "le_vc_service.h"
#include "le_vcs_service.h"
#include "le_csis_srv_service.h"
#include "le_media_controller.h"
#include "le_broadcast_mgr.h"
#include "le_mic_service.h"

uint16_t ble_audio_msg_cb(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    uint16_t msg_group;
    T_LE_AUDIO *p_link = NULL;

    APP_PRINT_TRACE1("ble_audio_msg_cb: msg 0x%04x", msg);

    msg_group = msg & 0xff00;
    switch (msg_group)
    {
    case LE_AUDIO_MSG_GROUP_BAP:
        {
            if (msg == LE_AUDIO_MSG_BAP_DIS_ALL_DONE)
            {
                T_BAP_DIS_ALL_DONE *p_dis_done = (T_BAP_DIS_ALL_DONE *)buf;
                if (p_dis_done)
                {
                    p_link = ble_audio_find_by_conn_handle(p_dis_done->conn_handle);
                    if (p_link != NULL)
                    {
                        p_link->bap_disc_all_done = true;

                        p_link->bass_brs_num = p_dis_done->brs_char_num > BASS_BRS_NUM_MAX ?
                                               BASS_BRS_NUM_MAX : p_dis_done->brs_char_num;

                        le_unicast_conn_update_mgr(CONN_UPDATE_EVENT_DISCV_ALL_DONE, p_link->conn_id);
                    }
                }
                ble_audio_get_pacs_info((T_BAP_DIS_ALL_DONE *)buf);
                le_audio_get_ase_info((T_BAP_DIS_ALL_DONE *)buf);
                le_audio_unicast_handle_pacs_discover((T_BAP_DIS_ALL_DONE *)buf);
            }
            else if (msg == LE_AUDIO_MSG_BAP_PACS_NOTIFY)
            {
                ble_audio_update_pacs_info((T_BAP_PACS_NOTIFY *)buf);
            }
        }
        break;

#if LE_AUDIO_PACS_SUPPORT
    case LE_AUDIO_MSG_GROUP_PACS:
        cb_result = le_pacs_handle_msg(msg, buf);
        break;
#endif

#if LE_AUDIO_ASCS_SUPPORT
    case LE_AUDIO_MSG_GROUP_ASCS:
        cb_result = le_unicast_sink_handle_msg(msg, buf);
        break;
#endif

#if LE_AUDIO_BASS_SUPPORT
    case LE_AUDIO_MSG_GROUP_BASS:
        {
            cb_result = le_audio_bass_handle_msg(msg, buf);
        }
        break;
#endif

#if LE_AUDIO_BASS_CLIENT_SUPPORT
    case LE_AUDIO_MSG_GROUP_BASS_CLIENT:
        {
            if (msg == LE_AUDIO_MSG_BASS_CLIENT_BRS_DATA)
            {
                T_BASS_CLIENT_BRS_DATA *p_brs_data = (T_BASS_CLIENT_BRS_DATA *)buf;
                if (p_brs_data)
                {
                    p_link = ble_audio_find_by_conn_handle(p_brs_data->conn_handle);
                    if (p_link != NULL)
                    {
                        bs_update_brs_state(p_link, p_brs_data);
                        ble_update_brs_state(p_link, p_brs_data);
                        ba_handle_brs_state(p_link, p_brs_data);
                        bap_ba_handle_bass_brs_state(p_brs_data);
                    }
                }
            }
            //FIX TODO APP
            else if (msg == LE_AUDIO_MSG_BASS_CLIENT_SYNC_INFO_REQ)
            {
                T_BASS_CLIENT_SYNC_INFO_REQ *sync_req_data = (T_BASS_CLIENT_SYNC_INFO_REQ *)buf;
                if (sync_req_data)
                {
                    p_link =  ble_audio_find_by_conn_handle(sync_req_data->conn_handle);
                    if (p_link != NULL)
                    {
                        bap_ba_handle_bass_sync_request(sync_req_data);
                    }
                }

            }
        }
        break;
#endif

#if LE_AUDIO_VCS_CLIENT_SUPPORT
    case LE_AUDIO_MSG_GROUP_VCS_CLIENT:
        {
            cb_result = le_audio_vcs_handle_msg(msg, buf);
        }
        break;

#if LE_AUDIO_VOCS_CLIENT_SUPPORT
    case LE_AUDIO_MSG_GROUP_VOCS_CLIENT:
        {
            cb_result = le_audio_vocs_handle_msg(msg, buf);
        }
        break;
#endif

#if LE_AUDIO_AICS_CLIENT_SUPPORT
    case LE_AUDIO_MSG_GROUP_AICS_CLIENT:
        {
            cb_result = le_audio_aics_handle_msg(msg, buf);
        }
        break;
#endif
#endif

#if LE_AUDIO_MICS_CLIENT_SUPPORT
    case LE_AUDIO_MSG_GROUP_MICS_CLIENT:
        {
            le_mic_handle_msg(msg, buf);
        }
        break;
#endif



#if LE_AUDIO_CSIS_CLIENT_SUPPORT
    case LE_AUDIO_MSG_GROUP_CSIS_CLIENT:
        {
            cb_result = le_csis_client_handle_msg(msg, buf);
        }
        break;

#endif

#if LE_AUDIO_MCP_SERVER_SUPPORT
    case LE_AUDIO_MSG_GROUP_MCP_SERVER:
        cb_result = le_media_player_handle_mcs_msg(msg, buf);
        break;
#endif

#if LE_AUDIO_CCP_SERVER_SUPPORT
    case LE_AUDIO_MSG_GROUP_CCP_SERVER:
        cb_result = teams_call_handle_ccp_msg(msg, buf);
        break;
#endif

#if LE_AUDIO_CSIS_SUPPORT
    case LE_AUDIO_MSG_GROUP_CSIS:
        cb_result = le_csis_handle_msg(msg, buf);
        break;
#endif

#if LE_AUDIO_VCS_SUPPORT
    case LE_AUDIO_MSG_GROUP_VCS:
        cb_result = le_vcs_handle_msg(msg, buf);
        break;

#if LE_AUDIO_VOCS_SUPPORT
    case LE_AUDIO_MSG_GROUP_VOCS:
        cb_result = le_vocs_handle_msg(msg, buf);
        break;
#endif

#if LE_AUDIO_AICS_SUPPORT
    case LE_AUDIO_MSG_GROUP_AICS:
        cb_result = le_aics_handle_msg(msg, buf);
        break;
#endif

#if LE_AUDIO_MICS_SUPPORT
    case LE_AUDIO_MSG_GROUP_MICS:
        cb_result = le_mics_handle_msg(msg, buf);
        break;
#endif
#endif

#if LE_AUDIO_MCP_CLIENT_SUPPORT
    case LE_AUDIO_MSG_GROUP_MCP_CLIENT:
        cb_result = le_mcp_handle_msg(msg, buf);
        break;
#endif
    case LE_AUDIO_MSG_GROUP_SERVER:
#if LE_AUDIO_MCP_SERVER_SUPPORT
        le_mcs_handle_server_msg(msg, buf);
#endif
#if LE_AUDIO_CCP_SERVER_SUPPORT
        le_tbs_handle_server_msg(msg, buf);
#endif
        break;

    case LE_AUDIO_MSG_GROUP_CAP:
        cb_result = le_audio_handle_cap_msg(msg, buf);
        break;

    default:
        break;
    }

    return cb_result;
}
