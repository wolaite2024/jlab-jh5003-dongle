
#include <string.h>
#include "trace.h"

#include "stdlib_corecrt.h"
#include "os_mem.h"
#include "app_flags.h"
#include "app_cfg.h"
#include "dongle_transmit_gatt_client.h"
#include "le_unicast_gaming.h"
#include "app_dongle_transmit_client.h"
#include "app_link_util.h"
#include "app_usb_uac.h"
#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
#include "bt_gatt_client.h"
#endif
#include "le_audio_service.h"
#include "bap.h"
#include "ble_audio_def.h"
#include "app_ctrl_pkt_policy.h"
#include "app_le_audio.h"
#include "connection_mgr.h"

#include "app_downstream_encode.h"
#include "app_gaming_sync.h"
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "app_adapter_service.h"
#include "app_upstream_decode.h"
#endif
#include "app_usb_vol_control.h"

#if F_APP_GAMING_LE_FIX_CHANNEL_SUPPORT
#include "gap_fix_chann_conn.h"

bool app_cmd_send_by_le(uint8_t cmd, uint8_t *data, uint16_t len, T_EARBUD_SIDE side)
{
    T_APP_LE_LINK *p_left_link = NULL;
    T_APP_LE_LINK *p_right_link = NULL;
    uint8_t cause = 0;
    bool left_ret = true;
    bool right_ret = true;
    bool ret = true;
    uint16_t pack_len = 0;
    uint8_t *p_pack = NULL;
    T_GAMING_SYNC_HDR *p_hdr = NULL;

    if (data == NULL || len == 0)
    {
        cause = 1;
        ret = false;
        goto failed;
    }

    pack_len = len + sizeof(T_GAMING_SYNC_HDR) + 1;
    p_pack = calloc(1, pack_len);

    if (p_pack == NULL)
    {
        cause = 2;
        ret = false;
        goto failed;
    }

    p_hdr = (T_GAMING_SYNC_HDR *)p_pack;

    p_hdr->sync = DONGLE_FORMAT_START_BIT;
    p_hdr->type = DONGLE_TYPE_CMD;
    p_hdr->pl = len + 1;
    p_hdr->cmd = cmd;
    memcpy(p_pack + sizeof(T_GAMING_SYNC_HDR), data, len);
    p_pack[pack_len - 1] = DONGLE_FORMAT_STOP_BIT;

    if (side == EARBUD_SIDE_LEFT)
    {
        p_left_link = app_link_find_le_link_by_bud_side(DEVICE_BUD_SIDE_LEFT);
    }
    else if (side == EARBUD_SIDE_RIGHT)
    {
        p_right_link = app_link_find_le_link_by_bud_side(DEVICE_BUD_SIDE_RIGHT);
    }
    else
    {
        p_left_link  = app_link_find_le_link_by_bud_side(DEVICE_BUD_SIDE_LEFT);
        p_right_link = app_link_find_le_link_by_bud_side(DEVICE_BUD_SIDE_RIGHT);
    }

    if (p_left_link != NULL)
    {
        left_ret = le_fixed_chann_data_send(p_left_link->conn_id, LE_FIX_CHANNEL_ID, p_pack, pack_len);
    }

    if (p_right_link != NULL)
    {
        right_ret = le_fixed_chann_data_send(p_right_link->conn_id, LE_FIX_CHANNEL_ID, p_pack,
                                             pack_len);
    }

    if (left_ret == false || right_ret == false)
    {
        ret = false;
    }

    free(p_pack);
failed:
    if (ret == false)
    {
        APP_PRINT_ERROR2("app_cmd_send_by_le failed: cmd %d cause -%d", cmd, cause);
    }

    return ret;
}
#else
bool app_cmd_send_by_le(uint8_t cmd, uint8_t *data, uint16_t len, T_EARBUD_SIDE side)
{
    return false;
}
#endif

#if TARGET_LE_AUDIO_GAMING_DONGLE
static void le_data_handler(uint8_t *buf, uint16_t len)
{
    T_GAMING_SYNC_HDR *hdr = (void *)buf;
    T_APP_DONGLE_DATA_TYPE type = (T_APP_DONGLE_DATA_TYPE)(hdr->type & 0x0F);
    uint16_t payload_len = (hdr->pl | ((hdr->type & 0xF0) << 4));
#if ENABLE_UAC2
    uint8_t *params = buf + sizeof(T_GAMING_SYNC_HDR);
#endif

    APP_PRINT_INFO2("le_data_handler(): 0x%04x, type: 0x%02x", payload_len, type);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if ((type & 0x3) == DONGLE_TYPE_UPSTREAM_VOICE)
    {
        /* only support fix channel upstream data */
        us_handle_rcv_pkt(buf, len);
        return;
    }
#endif

    switch (type)
    {
    case DONGLE_TYPE_CMD: /* cmd */
        {
            T_APP_DONGLE_CMD cmd = (T_APP_DONGLE_CMD)hdr->cmd;

            if (cmd == DONGLE_CMD_SYNC_STATUS)
            {
                memcpy(&headset_status, buf + offsetof(T_GAMING_SYNC_HDR, payload), sizeof(headset_status));
                app_gaming_handle_headset_status(&headset_status);
                ble_audio_handle_conn_update_for_headset_stream_status_change(&headset_status);
            }
            else if (cmd == DONGLE_CMD_SET_VOL_BALANCE)
            {
#if ENABLE_UAC2
                app_usb_ds_set_gain_percentage(params[0], params[1]);
#endif
            }
#if F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
            else if (cmd == DONGLE_CMD_SPK_VOL_CTRL)
            {
                app_usb_audio_volume_hid_ctrl((T_APP_VOL_CTRL)hdr->payload[0]);
            }
#endif
            else if (cmd == DONGLE_CMD_B2B_RELAY_DATA || cmd == DONGLE_CMD_B2B_RELAY_ENGAGE_ADV)
            {
                T_DONGLE_RELAY_B2B_DATA *b2b_relay_data = NULL;
                T_DONGLE_RELAY_B2B_ADV  *b2b_relay_adv  = NULL;
                T_APP_LE_LINK *p_link;
                T_EARBUD_SIDE peer_bud_side;
                T_DEVICE_BUD_SIDE bud_side;
                uint8_t *send_data = NULL;
                uint16_t send_len = 0;

                if (cmd == DONGLE_CMD_B2B_RELAY_DATA)
                {
                    b2b_relay_data = (T_DONGLE_RELAY_B2B_DATA *)(buf + offsetof(T_GAMING_SYNC_HDR, payload));
                    bud_side = b2b_relay_data->bud_side;
                    send_data = (uint8_t *)b2b_relay_data;
                    send_len = sizeof(T_DONGLE_RELAY_B2B_DATA) + b2b_relay_data->len;
                }
                else
                {
                    b2b_relay_adv = (T_DONGLE_RELAY_B2B_ADV *)(buf + offsetof(T_GAMING_SYNC_HDR, payload));
                    bud_side = b2b_relay_adv->bud_side;
                    send_data = (uint8_t *)b2b_relay_adv;
                    send_len = sizeof(T_DONGLE_RELAY_B2B_ADV) + b2b_relay_adv->adv_len;
                }

                if (bud_side == DEVICE_BUD_SIDE_LEFT)
                {
                    p_link = app_link_find_le_link_by_bud_side(DEVICE_BUD_SIDE_RIGHT);
                    peer_bud_side = EARBUD_SIDE_RIGHT;
                }
                else
                {
                    p_link = app_link_find_le_link_by_bud_side(DEVICE_BUD_SIDE_LEFT);
                    peer_bud_side = EARBUD_SIDE_LEFT;
                }

                if (p_link != NULL)
                {
                    app_cmd_send_by_le(cmd, send_data, send_len, peer_bud_side);
                }
            }
        }
        break;

    default:
        {
            APP_PRINT_ERROR0("le_data_handler: invaild type");
        }
        break;
    }
}
#endif

#if DONGLE_TRANSMIT_CLIENT_SUPPORT

extern uint8_t GATT_UUID128_DONGLE_TRANSMIT_SRV[16];

static void app_build_and_send_ack(uint16_t conn_handle, uint8_t status, uint16_t event_id)
{
    uint8_t idx = 0;
    uint8_t tx_event_seqn = 0;
    uint8_t *p_dongle_transmit_ack = NULL;

    tx_event_seqn++;
    p_dongle_transmit_ack = calloc(1, DONGLE_TRANSMIT_ACK_LEN + 4);
    p_dongle_transmit_ack[idx++] = SYNC_WORD;
    p_dongle_transmit_ack[idx++] = tx_event_seqn;
    p_dongle_transmit_ack[idx++] = DONGLE_TRANSMIT_ACK_LEN;
    p_dongle_transmit_ack[idx++] = DONGLE_TRANSMIT_ACK_LEN >> 8;
    p_dongle_transmit_ack[idx++] = CMD_ACK;
    p_dongle_transmit_ack[idx++] = CMD_ACK >> 8;
    p_dongle_transmit_ack[idx++] = event_id;
    p_dongle_transmit_ack[idx++] = event_id >> 8;
    p_dongle_transmit_ack[idx++] = status;

    APP_PRINT_INFO1("app_build_and_send_ack: %s",
                    TRACE_BINARY(DONGLE_TRANSMIT_ACK_LEN + 4, p_dongle_transmit_ack));

    dongle_transmit_data_write(conn_handle, p_dongle_transmit_ack, 4 + DONGLE_TRANSMIT_ACK_LEN);
    free(p_dongle_transmit_ack);
}

static bool app_build_and_send_packet(uint8_t link_id, uint16_t data_len, uint8_t *p_data)
{
    uint8_t idx = 0;
    uint16_t len, opcode;
    uint8_t *p_dongle_transmit_data = NULL;
    T_LE_AUDIO *p_le_link = NULL;
    p_le_link = ble_audio_find_by_link_id(link_id);
    bool ret = false;

    if (p_le_link == NULL)
    {
        APP_PRINT_ERROR1("app_build_and_send_packet: link_id: %d is invaild", link_id);
        return false;
    }

    p_le_link->tx_event_seqn++;
    len = data_len + 8; //spp header
    opcode = CMD_LE_DATA_TRANSFER;
    p_dongle_transmit_data = calloc(1, len + 4);
    p_dongle_transmit_data[idx++] = SYNC_WORD;
    p_dongle_transmit_data[idx++] = p_le_link->tx_event_seqn;
    p_dongle_transmit_data[idx++] = len;
    p_dongle_transmit_data[idx++] = len >> 8;
    p_dongle_transmit_data[idx++] = opcode;
    p_dongle_transmit_data[idx++] = opcode >> 8;
    p_dongle_transmit_data[idx++] = link_id;
    p_dongle_transmit_data[idx++] = DONGLE_PACKET_TYPE_SINGLE;
    p_dongle_transmit_data[idx++] = data_len + 2;
    p_dongle_transmit_data[idx++] = (data_len + 2) >> 8;
    p_dongle_transmit_data[idx++] = data_len;
    p_dongle_transmit_data[idx++] = data_len >> 8;

    for (int i = 0; i < data_len; i++)
    {
        p_dongle_transmit_data[idx++] = p_data[i];
    }

    APP_PRINT_INFO1("app_build_and_send_packet: p_dongle_transmit_data %s",
                    TRACE_BINARY(data_len + DONGLE_TRANSMIT_DATA_MIN_LEN, p_dongle_transmit_data));

    ret = dongle_transmit_data_write(p_le_link->conn_handle, p_dongle_transmit_data,
                                     data_len + DONGLE_TRANSMIT_DATA_MIN_LEN);

    free(p_dongle_transmit_data);

    return ret;
}

bool app_dongle_transmit_client_start(T_EARBUD_SIDE bud_side, uint16_t data_len, uint8_t *p_data)
{
    uint8_t link_num, link_id;
    T_LE_AUDIO *p_le_link = NULL;
    bool ret = false;

    if (p_data == NULL)
    {
        return false;
    }

    if (bud_side == EARBUD_SIDE_ANY)
    {
        link_num = le_get_active_link_num();

        if (link_num == 0)
        {
            APP_PRINT_ERROR0("app_dongle_transmit_start: no le link exsit.");
            return false;
        }

        for (int i = 0; i < MAX_BLE_LINK_NUM; i++)
        {
            link_id = i;
            app_build_and_send_packet(link_id, data_len, p_data);
        }

        ret = true;
    }
    else
    {
        p_le_link = ble_audio_find_by_bud_side(bud_side);

        if (p_le_link)
        {
            ret = app_build_and_send_packet(p_le_link->link_id, data_len, p_data);
        }
    }

    return ret;
}

static T_APP_RESULT app_transmit_client_handle_notify(uint16_t conn_handle, uint8_t *p_data,
                                                      uint16_t size)
{
    uint16_t event_id = 0;
    uint8_t *p_buf = p_data;
    uint16_t len = 0;
    uint8_t status = 0;
    uint8_t need_ack = true;
    uint8_t ret = 0;

    if ((!p_data) || (!size))
    {
        ret = 1;
        status = 1;
        goto send_ack;
    }

    /*first word must be 0xAA*/
    if (*p_buf != SYNC_WORD)
    {
        ret = 2;
        status = 1;
        goto send_ack;
    }

    /* one byte for sync word */
    p_buf++;
    /*one btye for index*/
    p_buf++;
    len = p_buf[0] | (p_buf[1] << 8);
    /*two bytes for total len except sync_word index and total len*/
    p_buf += 2;

    /*two bytes for event_id*/
    event_id = p_buf[0] | (p_buf[1] << 8);
    p_buf += 2;

    switch (event_id)
    {
    case EVENT_LEA_GAMING_MODE_SWITCH:
        {
            uint8_t ltv_remain_len = p_buf[0];
            uint8_t ltv_len;
            uint8_t *p_ltv = NULL;
            p_buf++;

            while (ltv_remain_len)
            {
                p_ltv = p_buf;
                ltv_len = p_ltv[0];
                if (ltv_len > ltv_remain_len - 1)
                {
                    ret = 3;
                    status = 1;
                    goto send_ack;
                }

                switch (p_ltv[1])
                {
                case BT_AUDIO_GAMING_MODE:
                    {
                        app_switch_gaming_mode(p_ltv[2]);
                    }
                    break;

                default:
                    break;
                }

                p_buf += ltv_len + 1;
                ltv_remain_len -= ltv_len + 1;
            }
        }
        break;

    case CMD_LE_DATA_TRANSFER:
        {
            need_ack = false;
            uint8_t para_len = len - 8;
            uint8_t *p_para = p_data + DONGLE_TRANSMIT_DATA_MIN_LEN;

            app_gaming_sync_disassemble_data(NULL, p_para, para_len);
            APP_PRINT_INFO2("para_len 0x%x para %s", para_len, TRACE_BINARY(para_len, p_para));
        }
        break;

    default:
        status = 2;
        break;
    }

send_ack:
    if (need_ack)
    {
        if (status)
        {
            APP_PRINT_WARN5("app_transmit_client_handle_notify: conn_handle %x, len %d, data %b, status %d, ret %d",
                            conn_handle, size, TRACE_BINARY(size, p_data), status, ret);
        }
        app_build_and_send_ack(conn_handle, status, event_id);
    }

    return APP_RESULT_SUCCESS;
}

T_APP_RESULT app_dongle_transmit_client_callback(uint16_t conn_handle, uint8_t *p_srv_uuid,
                                                 void *p_data)
{
    T_DONGLE_TRANSMIT_CLT_CB_DATA *p_cb_data = (T_DONGLE_TRANSMIT_CLT_CB_DATA *)p_data;

    if (!p_data)
    {
        return APP_RESULT_APP_ERR;
    }

    if (memcmp(p_srv_uuid, GATT_UUID128_DONGLE_TRANSMIT_SRV, 16))
    {
        APP_PRINT_ERROR2("transmit_client_callback: handle %x, uuid %b", conn_handle, TRACE_BINARY(16,
                         p_srv_uuid));
        return APP_RESULT_SUCCESS;
    }

    APP_PRINT_INFO2("transmit_client_callback: handle %x event %x", conn_handle, p_cb_data->cb_type);
    switch (p_cb_data->cb_type)
    {
    case DONGLE_TRANSMIT_CLIENT_CB_TYPE_DISC_DONE:
        {
            if (p_cb_data->cb_content.disc_done.is_found)
            {
                uint8_t p_pack[4] = {0xa5, 0xa5, 0x5a, 0x5a};

                if (app_dongle_transmit_client_start(EARBUD_SIDE_ANY, 4, p_pack) == false)
                {
                    APP_PRINT_INFO0("app_cmd_send_by_tranfer_client: failed to send gaming mode flag");
                }

                T_DONGLE_TRANSMIT_DEV_INFO dev_info = {0};
                dev_info.device_type = DONGLE_TRANSMIT_DEVICE_DONGLE;
                memcpy(dev_info.bd_addr, app_cfg_nv.bud_local_addr, 6);
                memcpy(dev_info.device_name, app_cfg_nv.device_name_le, DONGLE_TRANSMIT_DEVICE_NAME_MAX_LENGTH);
                dongle_transmit_dev_info_write(conn_handle, (uint8_t *)&dev_info,
                                               sizeof(T_DONGLE_TRANSMIT_DEV_INFO));

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
                uint8_t open_mic = app_get_usb_us_state();

                app_cmd_send_by_le(DONGLE_CMD_REQ_OPEN_MIC, &open_mic, 1, EARBUD_SIDE_ANY);
#endif

#if F_APP_LEA_DONGLE_BINDING
                app_le_audio_transmit_service_connected();
#endif
            }
        }
        break;

    case DONGLE_TRANSMIT_CLIENT_CB_TYPE_WRITE_RESULT:
        {
        }
        break;

    case DONGLE_TRANSMIT_CLIENT_CB_TYPE_CCCD_CFG:
        {

        }
        break;

    case DONGLE_TRANSMIT_CLIENT_CB_TYPE_NOTIFY_IND:
        {
            app_transmit_client_handle_notify(conn_handle, p_cb_data->cb_content.notify_ind.p_value,
                                              p_cb_data->cb_content.notify_ind.value_size);
        }
        break;
    default:
        break;
    }

    return APP_RESULT_SUCCESS;
}

bool app_lea_conn_mgr_cback(uint8_t event)
{
    bool rtn = true;

    switch (event)
    {
    case CI_EVENT_CIS_DISCONNECT:
    case CI_EVENT_CIS_ESTABLISH:
        {
            if (headset_status.headset_source == HEADSET_SOURCE_BT)
            {
                rtn = false;
            }
        }
        break;

    default:
        break;
    }

    APP_PRINT_TRACE3("app_lea_conn_mgr_cback: 0x%02x, source: %d, rtn: %d", event,
                     headset_status.headset_source,
                     rtn);
    return rtn;
}


void app_dongle_transmit_client_init(void)
{
    dongle_transmit_client_init(app_dongle_transmit_client_callback);

    ble_conn_mgr_register_app_cb(app_lea_conn_mgr_cback);

#if TARGET_LE_AUDIO_GAMING_DONGLE
    app_gaming_sync_rcv_register(app_gaming_ctrl_data_rcv);

    app_gaming_sync_le_cmd_register(le_data_handler);
#endif
}
#endif
