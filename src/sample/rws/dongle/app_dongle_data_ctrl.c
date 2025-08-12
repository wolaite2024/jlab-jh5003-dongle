#if F_APP_COMMON_DONGLE_SUPPORT
#include <string.h>
#include <stdlib.h>
#endif
#include "trace.h"
#include "gap_br.h"
#include "bt_spp.h"
#include "app_sdp.h"
#include "app_timer.h"
#include "app_dongle_common.h"
#include "app_dongle_data_ctrl.h"
#include "app_transfer.h"
#include "app_link_util.h"
#include "app_dongle_dual_mode.h"
#include "app_gaming_sync.h"
#include "app_sniff_mode.h"
#include "app_usb_vol_control.h"
#include "app_lea_ascs.h"

#if F_APP_COMMON_DONGLE_SUPPORT
bool app_dongle_send_cmd(T_APP_DONGLE_CMD cmd, uint8_t *data, uint8_t len)
{
    bool ret = false;
    uint8_t cause = 0;
    uint8_t *buf = NULL;
    bool dongle_link_exist = false;

#if TARGET_LE_AUDIO_GAMING
    T_APP_LE_LINK *p_lea_link = app_dongle_get_le_audio_dongle_link();

    if (p_lea_link != NULL)
    {
        dongle_link_exist = true;
    }
#endif

#if TARGET_LEGACY_AUDIO_GAMING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    T_APP_BR_LINK *p_br_link = app_dongle_get_connected_dongle_link();

    if (p_br_link != NULL)
    {
        dongle_link_exist = true;
    }
#endif

    if (dongle_link_exist)
    {
        uint16_t total_len = len + 5;
        buf = malloc(total_len);

        /* cmd + payload len, total 12 bits, buf[1] first 4 bits + buf[2] */
        uint16_t payload_len = len + 1;

        if (payload_len > 255)
        {
            cause = 1;
            goto exit;
        }

        if (buf != NULL)
        {
            buf[0] = DONGLE_FORMAT_START_BIT;
            buf[1] = DONGLE_TYPE_CMD & 0x0F;
            buf[2] = payload_len;
            buf[3] = cmd;
            memcpy(buf + 4, data, len);
            buf[len + 4] = DONGLE_FORMAT_STOP_BIT;

#if TARGET_LE_AUDIO_GAMING || TARGET_LEGACY_AUDIO_GAMING
            if (0)
            {}
#if TARGET_LE_AUDIO_GAMING
            else if (p_lea_link != NULL)
            {
                ret = app_transfer_start_for_le(p_lea_link->id, total_len, buf);
            }
#endif
#if TARGET_LEGACY_AUDIO_GAMING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
            else if (p_br_link != NULL)
            {
                if (app_dongle_send_fix_channel_data(p_br_link->bd_addr, FIX_CHANNEL_CID, buf, total_len,
                                                     false))
                {
                    ret = true;
                }
            }
#endif
#endif
            if (ret == false)
            {
                cause = 2;
                goto exit;
            }
        }
    }

exit:
    if (buf != NULL)
    {
        free(buf);
    }

    if (ret == false)
    {
        APP_PRINT_ERROR1("app_dongle_send_cmd failed: cause %d", cause);
    }

    return ret;
}
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
#if TARGET_LE_AUDIO_GAMING
void app_dongle_handle_le_data(uint8_t *data, uint16_t len)
{
    if ((data[0] != DONGLE_FORMAT_START_BIT) || (data[len - 1] != DONGLE_FORMAT_STOP_BIT))
    {
        APP_PRINT_ERROR0("app_dongle_handle_le_data: Data fromat is not correct!");
    }

    bool handle = true;
    T_GAMING_SYNC_HDR *sync_hdr = (T_GAMING_SYNC_HDR *)data;

    switch (sync_hdr->type)
    {
    case DONGLE_TYPE_CMD:
        {
            if (sync_hdr->cmd == DONGLE_CMD_REQ_OPEN_MIC)
            {
#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
                app_dongle_le_audio_handle_mic(data[4]);
#endif
            }
            else if (sync_hdr->cmd == DONGLE_CMD_SYNC_STATUS)
            {
                memcpy(&dongle_status, &data[4], sizeof(dongle_status));

                app_dongle_streaming_handle(dongle_status.streaming_to_peer);
                app_dongle_lea_handle_dongle_status();
            }
            else if (sync_hdr->cmd == DONGLE_CMD_CTRL_RAW_DATA)
            {
                app_gaming_ctrl_data_rcv(sync_hdr->payload, sync_hdr->pl - 1);
            }
#if F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
            else if (sync_hdr->cmd == DONGLE_CMD_TRANS_SPK_VOL_INFO)
            {
                memcpy(&usb_spk_vol, &data[4], sizeof(T_USB_SPK_INFO));
                app_gaming_handle_usb_spk_vol();
            }
#endif
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle)
    {
        APP_PRINT_TRACE2("app_dongle_handle_le_data: msg_type: 0x%02x, id: 0x%02x", sync_hdr->type,
                         sync_hdr->cmd);
    }
}
#endif

static void app_dongle_fix_chann_cb(T_GAP_BR_FIX_CHANN_MSG msg, void *p_buf)
{
    switch (msg)
    {
    case GAP_BR_FIX_CHANN_DATA_IND:
        {
            T_GAP_BR_FIX_CHANN_DATA_IND *data = p_buf;
            uint8_t addr[6] = {0};

            if (data != NULL)
            {
                app_gaming_sync_disassemble_data(data->bd_addr, data->p_data, data->length);
            }
        }
        break;

    default:
        break;
    }
}

#if TARGET_LEGACY_AUDIO_GAMING
static bool app_dongle_send_fix_chann_data(uint8_t *addr, uint8_t *data, uint16_t len,
                                           bool flushable)
{
    return app_dongle_send_fix_channel_data(addr, FIX_CHANNEL_CID, data, len, flushable);
}
#endif

#if TARGET_LE_AUDIO_GAMING
static bool app_dongle_send_le_transmit_service_data(uint8_t link_id, uint8_t *data, uint16_t len)
{
    return app_transfer_start_for_le(link_id, len, data);
}
#endif

void app_gaming_ctrl_data_rcv(uint8_t *data, uint16_t len)
{
    APP_PRINT_TRACE1("app_gaming_ctrl_data_rcv: len %d", len);
}

bool app_gaming_ctrl_data_send_to_dongle(uint8_t *data, uint16_t size, bool flushable)
{
    return app_gaming_sync_data_send(data, size, flushable);
}

void app_dongle_send_vol_ctrl_cmd(uint8_t vol_cmd)
{
    app_dongle_send_cmd(DONGLE_CMD_SPK_VOL_CTRL, &vol_cmd, 1);
}

void app_gaming_handle_usb_spk_vol(void)
{
    int16_t gain = usb_spk_vol.uac_spk_vol_gain;

#if TARGET_LEGACY_AUDIO_GAMING
    T_APP_BR_LINK *p_link = app_dongle_get_connected_dongle_link();
    if (p_link)
    {
        app_usb_volume_db_set(p_link->a2dp_track_handle, gain, CTRL_FROM_HOST);
    }
#endif

#if TARGET_LE_AUDIO_GAMING
    T_APP_LE_LINK *p_lea_link = app_dongle_get_le_audio_dongle_link();
    T_LEA_ASE_ENTRY *p_ase_entry;
    p_ase_entry = app_lea_ascs_find_ase_entry_by_direction(p_lea_link, DATA_PATH_OUTPUT_FLAG);
    if (p_lea_link)
    {
        app_usb_volume_db_set(p_ase_entry->track_handle, gain, CTRL_FROM_HOST);
    }
#endif
}

void app_dongle_data_ctrl_init(void)
{
#if TARGET_LEGACY_AUDIO_GAMING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    gap_br_reg_fix_chann(FIX_CHANNEL_CID);
    gap_br_reg_fix_chann_cb(app_dongle_fix_chann_cb);
#endif


#if TARGET_LEGACY_AUDIO_GAMING
    app_gaming_sync_legacy_send_register(app_dongle_send_fix_chann_data);
    app_gaming_sync_rcv_register(app_gaming_ctrl_data_rcv);
#endif

#if TARGET_LE_AUDIO_GAMING
    app_gaming_sync_le_send_register(app_dongle_send_le_transmit_service_data);
    app_gaming_sync_rcv_register(app_gaming_ctrl_data_rcv);

    app_gaming_sync_le_cmd_register(app_dongle_handle_le_data);
#endif

}
#endif
