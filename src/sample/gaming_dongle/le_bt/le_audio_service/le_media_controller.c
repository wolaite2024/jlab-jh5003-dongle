#if LE_AUDIO_MCP_CLIENT_SUPPORT
#include <string.h>
#include <stdio.h>
#include "trace.h"
#include "mcp_client.h"
#include "le_media_controller.h"
#include "app_usb_layer.h"

#include "ual_dev_mgr.h"
#include "dev_mgr.h"
#if UAL_CONSOLE_PRINT
#include "console.h"
#endif

static void le_mcp_controll_cb(uint8_t opcode, uint8_t result)
{
#if UAL_CONSOLE_PRINT
    uint8_t event_buff[60];
    uint16_t buf_len;
    buf_len =  sprintf((char *)event_buff, "mcp control cb: opcode 0x%x, 0x%x \r\n", opcode, result);

    console_write(event_buff, buf_len);
#endif
}

uint16_t le_mcp_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_BT_DEVICE *p_dev_rec = NULL;

    APP_PRINT_INFO1("le_mcp_handle_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_MCP_CLIENT_DIS_DONE:
        {
            T_MCP_CLIENT_DIS_DONE *p_dis_done = (T_MCP_CLIENT_DIS_DONE *)buf;

            p_dev_rec = ual_find_device_by_conn_handle(p_dis_done->conn_handle);
            if (p_dev_rec == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_dis_done->is_found)
            {
                if (p_dis_done->gmcs)
                {
                    mcp_client_cfg_cccd(p_dis_done->conn_handle,
                                        MCP_CLIENT_CFG_CCCD_FLAG_MEDIA_CONTROL_POINT | MCP_CLIENT_CFG_CCCD_FLAG_TRACK_CHANGED |
                                        MCP_CLIENT_CFG_CCCD_FLAG_MEDIA_STATE,
                                        true, 0, true);
                    mcp_client_read_char_value(p_dis_done->conn_handle, 0, MCS_UUID_CHAR_MEDIA_STATE, true);
                    mcp_client_read_char_value(p_dis_done->conn_handle, 0, MCS_UUID_CHAR_MEDIA_PLAYER_NAME, true);
                }
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_READ_RESULT:
        {
            T_MCP_CLIENT_READ_RESULT *p_read_result = (T_MCP_CLIENT_READ_RESULT *)buf;

            p_dev_rec = ual_find_device_by_conn_handle(p_read_result->conn_handle);
            if (p_dev_rec == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_read_result->gmcs)
            {
                if (p_read_result->char_uuid == MCS_UUID_CHAR_MEDIA_STATE)
                {
#if UAL_CONSOLE_PRINT
                    uint8_t event_buff[60];
                    uint16_t buf_len;
                    buf_len =  sprintf((char *)event_buff, "media state: 0x%x \r\n", p_read_result->data.media_state);

                    console_write(event_buff, buf_len);
#endif
                    APP_PRINT_INFO1("le_mcp_handle_msg: media_state %x", p_read_result->data.media_state);
                }
                else if (p_read_result->char_uuid == MCS_UUID_CHAR_MEDIA_PLAYER_NAME)
                {
                    if (p_read_result->data.media_player_name.media_player_name_len != 0)
                    {
                        uint8_t *p_name = p_read_result->data.media_player_name.p_media_player_name;
                        APP_PRINT_INFO1("le_mcp_handle_msg: player_name %s", TRACE_STRING(p_name));
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_NOTIFY:
        {
            T_MCP_CLIENT_NOTIFY *p_notify_result = (T_MCP_CLIENT_NOTIFY *)buf;

            p_dev_rec = ual_find_device_by_conn_handle(p_notify_result->conn_handle);
            if (p_dev_rec == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            switch (p_notify_result->char_uuid)
            {
            case MCS_UUID_CHAR_MEDIA_STATE:
                {
                    // Because dongle set MCP states is pause forever, not change state.
                    // here is workaround, when headset is streaming, local state will be change.
                    if (p_notify_result->gmcs)
                    {
                        if (p_notify_result->data.media_state != MCS_MEDIA_STATE_INACTIVE)
                        {
#if UAL_CONSOLE_PRINT
                            uint8_t event_buff[60];
                            uint16_t buf_len;
                            buf_len =  sprintf((char *)event_buff, "media state: 0x%x \r\n", p_notify_result->data.media_state);

                            console_write(event_buff, buf_len);
#endif
                        }
                    }
                }
                break;

            case MCS_UUID_CHAR_TRACK_CHANGED:
                {
                    APP_PRINT_INFO0("LE_AUDIO_MSG_MCP_CLIENT_NOTIFY: track change");
                }
                break;

            default:
                break;
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_MEDIA_CP_NOTIFY:
        {
            T_MCP_CLIENT_MEDIA_CP_NOTIFY *p_media_cp_notify = (T_MCP_CLIENT_MEDIA_CP_NOTIFY *)buf;

            le_mcp_controll_cb(p_media_cp_notify->requested_opcode, p_media_cp_notify->result_code);
        }
        break;

    default:
        break;
    }
    return cb_result;
}

void le_media_send_control_key(uint8_t conn_id, uint8_t opcode)
{
    T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM param;
    memset(&param, 0, sizeof(T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM));
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_id(conn_id);
    if (p_dev_rec == NULL)
    {
        return;
    }

    param.opcode = opcode;
    mcp_client_write_media_cp(p_dev_rec->le_conn_handle, 0, true, &param, true);
}

#endif
