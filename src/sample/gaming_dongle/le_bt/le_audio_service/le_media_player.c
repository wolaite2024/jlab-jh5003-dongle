#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "le_audio_service.h"
#include "ble_audio.h"
#include "codec_def.h"
#include "le_media_player.h"
#include "mcp_mgr.h"
#include "app_usb_hid.h"
#include "app_timer.h"
//#include "cis_policy.h"
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
#include "uac_silence_detect.h"
#endif

#if LE_AUDIO_MCP_SERVER_SUPPORT
static T_MEDIA_PLAYER_CB media_player_cb;

static uint8_t mcp_timer_queue_id;

#define MCP_FAST_OPCODE_TIMEOUT        (500)
typedef enum
{
    MCP_TIMER_ID_FAST_OPCODE      = 0x01,
} T_MCP_TIMER_ID;


T_SERVER_ID get_media_player_id(void)
{
    return media_player_cb.mcs_id;
}

void le_media_player_set_state(uint8_t state)
{
    T_MCP_SERVER_SET_PARAM set_param = {0};

    APP_PRINT_INFO1("le_media_player_set_state state %x", state);
    if (state > MCS_MEDIA_STATE_SEEKING)
    {
        return;
    }
    media_player_cb.media_state = state;

    set_param.char_uuid = MCS_UUID_CHAR_MEDIA_STATE;
    set_param.param.media_state = state;

    mcp_server_set_param(media_player_cb.mcs_id, &set_param);
}


static void mcp_handle_fast_opcode_timeout(void)
{
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
    app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
}

static void teams_mcp_timeout_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_INFO2("teams_mcp_timeout_cback id %x, chnl %x", timer_id, timer_chann);

    switch (timer_id)
    {
    case MCP_TIMER_ID_FAST_OPCODE:
        {
            app_stop_timer(&media_player_cb.mcp_fast_opcode_timer);
            mcp_handle_fast_opcode_timeout();
        }
        break;
    default:
        break;
    }
}

uint16_t le_media_player_handle_mcs_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_MCP_SERVER_WRITE_MEDIA_CP_IND:
        {
            T_MCP_SERVER_WRITE_MEDIA_CP_IND *p_mcp_req = (T_MCP_SERVER_WRITE_MEDIA_CP_IND *)buf;
            if (!le_media_player_handle_operation(p_mcp_req->opcode))
            {
                cb_result = BLE_AUDIO_CB_RESULT_REJECT;
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_SERVER_READ_IND:
        {
            T_MCP_SERVER_READ_IND *p_read_ind = (T_MCP_SERVER_READ_IND *)buf;

            if (p_read_ind)
            {
                T_MCP_SERVER_READ_CFM read_cfm = {0};

                read_cfm.cause = BLE_AUDIO_CB_RESULT_SUCCESS;
                read_cfm.conn_handle = p_read_ind->conn_handle;
                read_cfm.cid = p_read_ind->cid;
                read_cfm.service_id = p_read_ind->service_id;
                read_cfm.char_uuid = p_read_ind->char_uuid;
                read_cfm.offset = p_read_ind->offset;

                switch (p_read_ind->char_uuid)
                {
                case MCS_UUID_CHAR_MEDIA_PLAYER_NAME:
                    {
                        read_cfm.param.media_player_name.p_media_player_name = "RTK dongle player";
                        read_cfm.param.media_player_name.media_player_name_len = strlen("RTK dongle player");

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                case MCS_UUID_CHAR_TRACK_TITLE:
                    {
                        read_cfm.param.track_title.p_track_title = "RTK media";
                        read_cfm.param.track_title.track_title_len = strlen("RTK media");

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                case MCS_UUID_CHAR_TRACK_DURATION:
                    {
                        read_cfm.param.track_duration = MCS_TRACK_DURATION_CHAR_VALUE_UNKNOWN;

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                case MCS_UUID_CHAR_TRACK_POSITION:
                    {
                        read_cfm.param.track_position = MCS_TRACK_POSITION_CHAR_VALUE_UNAVAILABLE;

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                case MCS_UUID_CHAR_CONTENT_CONTROL_ID:
                    {
                        read_cfm.param.content_control_id = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GMCS;

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                default:
                    break;
                }
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

bool le_media_player_handle_operation(uint8_t opcode)
{
    APP_PRINT_INFO2("le_media_player_handle_operation %d media_state %x", opcode,
                    media_player_cb.media_state);
    if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PLAY)
    {
        if (media_player_cb.media_state == MCS_MEDIA_STATE_SEEKING)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY_PAUSE);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
            app_stop_timer(&media_player_cb.mcp_fast_opcode_timer);
        }
        else if (media_player_cb.media_state != MCS_MEDIA_STATE_PLAYING)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY_PAUSE);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
            le_media_player_set_state(MCS_MEDIA_STATE_PLAYING);
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
            silence_detect_set_hid_state(false);
            slience_set_stream_suspend(false);
#endif
        }
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PAUSE)
    {
        if (media_player_cb.media_state == MCS_MEDIA_STATE_SEEKING)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY_PAUSE);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
            app_stop_timer(&media_player_cb.mcp_fast_opcode_timer);
            le_media_player_set_state(MCS_MEDIA_STATE_PAUSED);
        }
        else if (media_player_cb.media_state != MCS_MEDIA_STATE_PAUSED)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY_PAUSE);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
            le_media_player_set_state(MCS_MEDIA_STATE_PAUSED);
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
            silence_detect_set_hid_state(true);
            slience_set_stream_suspend(true);
#endif
        }
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_STOP)
    {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
        app_usb_hid_send_consumer_ctrl_key_down(KEY_STOP);
        app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
        le_media_player_set_state(MCS_MEDIA_STATE_INACTIVE);
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_FAST_FORWARD)
    {
        if (!media_player_cb.mcp_fast_opcode_timer)
        {
            if (media_player_cb.media_state == MCS_MEDIA_STATE_PLAYING)
            {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
                app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY_PAUSE);
                app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
            }
            app_usb_hid_send_consumer_ctrl_key_down(KEY_FORWARD);
            le_media_player_set_state(MCS_MEDIA_STATE_SEEKING);
        }
        app_start_timer(&media_player_cb.mcp_fast_opcode_timer, "fast_opcode",
                        mcp_timer_queue_id, MCP_TIMER_ID_FAST_OPCODE, 0, false, MCP_FAST_OPCODE_TIMEOUT);
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_FAST_REWIND)
    {

        if (!media_player_cb.mcp_fast_opcode_timer)
        {
            if (media_player_cb.media_state == MCS_MEDIA_STATE_PLAYING)
            {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
                app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY_PAUSE);
                app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
            }
            app_usb_hid_send_consumer_ctrl_key_down(KEY_REWIND);
            le_media_player_set_state(MCS_MEDIA_STATE_SEEKING);
        }
        app_start_timer(&media_player_cb.mcp_fast_opcode_timer, "fast_opcode",
                        mcp_timer_queue_id, MCP_TIMER_ID_FAST_OPCODE, 0, false, MCP_FAST_OPCODE_TIMEOUT);
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PREVIOUS_TRACK ||
             opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_NEXT_TRACK)
    {
        T_MCP_SERVER_SEND_DATA_PARAM send_data_param = {0};

        send_data_param.char_uuid = MCS_UUID_CHAR_TRACK_CHANGED;

        mcp_server_send_data(media_player_cb.mcs_id, &send_data_param);

        if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PREVIOUS_TRACK)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PREV_TK);
#endif
        }
        else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_NEXT_TRACK)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_NEXT_TK);
#endif
        }
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
        app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
    }
    else
    {
        APP_PRINT_ERROR1("le_media_player_handle_operation unspported opcode 0x%x", opcode);
        return false;
    }
    return true;
}

uint16_t le_mcs_get_enabled_cccd(void)
{
    return media_player_cb.mcs_enabled_cccd;
}

bool le_mcs_handle_server_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_SERVER_ATTR_CCCD_INFO *p_cccd = NULL;
    if (LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO == msg)
    {
        if (!buf)
        {
            return false;
        }

        p_cccd = (T_SERVER_ATTR_CCCD_INFO *)buf;
        if (p_cccd->service_id != media_player_cb.mcs_id)
        {
            APP_PRINT_ERROR2("le_mcs_handle_server_msg mcs_id %x, service id %x",
                             media_player_cb.mcs_id, p_cccd->service_id);
            return false;
        }
        APP_PRINT_INFO1("le_mcs_handle_server_msg cccd_flag %x", p_cccd->ccc_bits);
        media_player_cb.mcs_enabled_cccd = p_cccd->ccc_bits;
        return true;
    }
    return false;
}

void le_media_player_init(T_SERVER_ID mcs_id)
{
    T_MCP_SERVER_SET_PARAM set_param = {0};

    media_player_cb.mcs_id = mcs_id;

    set_param.char_uuid = MCS_UUID_CHAR_PLAYING_ORDERS_SUPPORTED;
    set_param.param.playing_orders_supported = 0;

    mcp_server_set_param(mcs_id, &set_param);

    set_param.char_uuid = MCS_UUID_CHAR_MEDIA_STATE;
    set_param.param.media_state = MCS_MEDIA_STATE_PAUSED;

    mcp_server_set_param(mcs_id, &set_param);

    media_player_cb.media_state = MCS_MEDIA_STATE_INACTIVE;

    app_timer_reg_cb(teams_mcp_timeout_cback, &mcp_timer_queue_id);
}
#endif

