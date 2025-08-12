#include <string.h>
#include <stdlib.h>

#include "trace.h"
#if UAL_CONSOLE_PRINT
#include "console.h"
#endif
#include "gap_conn_le.h"

#include "ual_adapter.h"
#include "ual_bluetooth.h"
#include "ual_dev_mgr.h"
#include "app_adapter_service.h"
#include "ual_api_types.h"
#include "os_queue.h"
#include "app_usb_layer.h"
#include "le_audio_service.h"
#include "ascs_def.h"
#include "le_csis_client_service.h"
#include "le_unicast_src_service.h"
#include "app_io_msg.h"
#include "le_unicast_gaming.h"
#include "gap_privacy.h"
#include "app_link_util.h"
#include "bt_a2dp.h"
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "gap_br.h"
#endif

#if F_APP_LEA_DONGLE_BINDING
#include "app_cfg.h"
#include "app_le_audio.h"
#include "app_device.h"
#endif

#define APP_MAX_DEVICE_INFO 10

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
typedef enum
{
    ADAPTER_A2DP_DISCONNECTED,
    ADAPTER_A2DP_CONNECTING,
    ADAPTER_A2DP_CONNECTED,
    ADAPTER_A2DP_DISCONNECTING,
} T_ADAPTER_A2DP_CONN_STATUS;

typedef enum
{
    ADAPTER_BLE_DISCONNECTED,
    ADAPTER_BLE_CONNECTING,
    ADAPTER_BLE_CONNECTED,
    ADAPTER_BLE_DISCONNECTING,
} T_ADAPTER_BLE_CONN_STATUS;

typedef enum
{
    ADAPTER_HEADSET_PHONE_UNKNOWN,
    ADAPTER_HEADSET_PHONE_DISCONNECTED,
    ADAPTER_HEADSET_PHONE_CONNECTED,
} T_ADAPTER_HEADSET_CONN_STATUS;

typedef enum
{
    ADAPTER_LINK_STATE_IDLE                 = 0x00,
    ADAPTER_LINK_STATE_A2DP_CONNECTING      = 0x01,
    ADAPTER_LINK_STATE_A2DP_DISCONNECTING   = 0x02,
    ADAPTER_LINK_STATE_A2DP_CONNECTED       = 0x03,
    ADAPTER_LINK_STATE_BLE_CONNECTING       = 0x04,
    ADAPTER_LINK_STATE_BLE_DISCONNECTING    = 0x05,
    ADAPTER_LINK_STATE_BLE_CONNECTED        = 0x06,
} T_ADAPTER_LINK_STATE;

typedef struct
{
    T_ADAPTER_LINK_STATE link_state;
    uint8_t  legacy_conn_addr[6];
    uint16_t ble_conn_id;
} T_ADAPTER_LINK_INFO;

static T_ADAPTER_LINK_INFO adapter_link_info = {.link_state = ADAPTER_LINK_STATE_IDLE};
#endif

typedef struct
{
    uint16_t      filter_uuid;
    int           client_id;
    T_OS_QUEUE    devs_queue;
} tDISC_CB;

typedef struct
{
    uint8_t         *bd_addr;
    uint8_t          bd_type;
    uint8_t          adv_sid;
    bool             uuid_match;
    uint8_t          connect_mode;
    uint8_t          gaming_mode;
    uint8_t         *p_name;
    uint16_t         name_len;
    int8_t           rssi;
    uint8_t         *p_rsi;
#if F_APP_LEA_DONGLE_BINDING
    bool             allow_connect;
#endif
} T_DISC_RESULT;

static tDISC_CB disc_mgr_cb;

#if F_APP_LEA_DONGLE_BINDING
static bool scan_by_mmi = false;
#endif

T_DEV_RES_INFO *disc_mgr_find_device_by_idx(uint8_t idx)
{
    T_DEV_RES_INFO *dev = (T_DEV_RES_INFO *)os_queue_peek(&disc_mgr_cb.devs_queue, idx);

    return dev;
}

static T_DEV_RES_INFO *disc_mgr_find_device(uint8_t *bd_addr, uint8_t bd_type)
{
    uint8_t i;
    T_DEV_RES_INFO *dev;
    for (i = 0; i < disc_mgr_cb.devs_queue.count; i++)
    {
        dev = (T_DEV_RES_INFO *)os_queue_peek(&disc_mgr_cb.devs_queue, i);
        if (memcmp(bd_addr, dev->bd_addr, BD_ADDR_LEN) == 0 &&
            dev->bd_type == bd_type)
        {
            return dev;
        }
    }
    return NULL;
}

static T_DEV_RES_INFO *disc_mgr_add_device(uint8_t *bd_addr, uint8_t bd_type)
{
    T_DEV_RES_INFO *dev;


    /* If result count not at max */
    if (disc_mgr_cb.devs_queue.count < APP_MAX_DEVICE_INFO)
    {
        dev = calloc(1, sizeof(T_DEV_RES_INFO));
        if (dev == NULL)
        {
            APP_PRINT_ERROR0("scan_mgr_add_device alloc device fail!");
            return NULL;
        }
        memcpy(dev->bd_addr, bd_addr, BD_ADDR_LEN);
        dev->bd_type = bd_type;

        os_queue_in(&disc_mgr_cb.devs_queue, dev);

#if UAL_CONSOLE_PRINT
        uint8_t event_buff[60];
        uint16_t buf_len;
        buf_len = sprintf((char *)event_buff,
                          "scan RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d\r\n",
                          dev->bd_addr[5], dev->bd_addr[4],
                          dev->bd_addr[3], dev->bd_addr[2],
                          dev->bd_addr[1], dev->bd_addr[0],
                          dev->bd_type);

        console_write(event_buff, buf_len);
#endif
        return dev;
    }
    else
    {
        return NULL;
    }
}

static void disc_mgr_clear_device_list(void)
{
    T_DEV_RES_INFO *dev_info = NULL;
    disc_mgr_cb.filter_uuid = 0;
    while ((dev_info = os_queue_out(&disc_mgr_cb.devs_queue)) != NULL)
    {
        free(dev_info);
    }
}

static void app_adater_service_init()
{
    memset(&disc_mgr_cb, 0, sizeof(tDISC_CB));
    os_queue_init(&disc_mgr_cb.devs_queue);
}

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
bool adapter_ble_is_connected(void)
{
    bool ret = false;

    if (adapter_link_info.link_state == LINK_STATE_BLE_CONNECTED)
    {
        ret = true;
    }

    return ret;
}

bool adapter_acl_is_connected(void)
{
    bool ret = false;

    if (adapter_link_info.link_state == LINK_STATE_A2DP_CONNECTING_WITH_ACL_CONNECTED ||
        adapter_link_info.link_state == LINK_STATE_A2DP_CONNECTED ||
        adapter_link_info.link_state == LINK_STATE_ACL_DISCONNECTING_WITH_A2DP_CONNECTED ||
        adapter_link_info.link_state == LINK_STATE_ACL_DISCONNECTING_WITH_A2DP_DISCONNECTED)
    {
        ret = true;
    }

    return ret;
}

bool adapter_a2dp_is_connected(void)
{
    bool ret = false;

    if (adapter_link_info.link_state == ADAPTER_LINK_STATE_A2DP_CONNECTED)
    {
        ret = true;
    }

    return ret;
}

void adapter_dual_mode_link_mgr(T_ADAPTER_LINK_EVENT event, uint8_t ble_conn_id)
{
    static uint8_t connected_ble_conn_id = 0xff;
    T_ADAPTER_LINK_STATE pre_link_state = adapter_link_info.link_state;
    T_ADAPTER_LINK_STATE new_link_state = adapter_link_info.link_state;
    static T_HEADSET_CONN_STATUS headset_conn_status = HEADSET_PHONE_STATUS_UNKOWN;

    if (event == ADAPTER_EVENT_HEADSET_PHONE_CONNECTED)
    {
        headset_conn_status = HEADSET_PHONE_CONNECTED;
    }
    else if (event == ADAPTER_EVENT_HEADSET_PHONE_DISCONNECTED)
    {
        headset_conn_status = HEADSET_PHONE_DISCONNECTED;
    }
    else if (event == ADAPTER_EVENT_HEADSET_PHONE_UNKNOWN)
    {
        headset_conn_status = HEADSET_PHONE_STATUS_UNKOWN;
    }

    /* transition state handle */
    if (pre_link_state == ADAPTER_LINK_STATE_IDLE)
    {
        if (event == ADAPTER_EVENT_BLE_CONNECTED)
        {
            connected_ble_conn_id = ble_conn_id;
            new_link_state = ADAPTER_LINK_STATE_BLE_CONNECTED;
        }
        else if (event == ADAPTER_EVENT_A2DP_CONNECTED)
        {
            new_link_state = ADAPTER_LINK_STATE_A2DP_CONNECTED;
        }
    }
    if (pre_link_state == ADAPTER_LINK_STATE_A2DP_CONNECTING)
    {
        if (headset_conn_status == HEADSET_PHONE_CONNECTED)
        {
            if (event == ADAPTER_EVENT_A2DP_CONNECTED)
            {
                new_link_state = ADAPTER_LINK_STATE_A2DP_CONNECTED;
            }
        }
        else
        {
            new_link_state = ADAPTER_LINK_STATE_IDLE;
        }
    }
    else if (pre_link_state == ADAPTER_LINK_STATE_A2DP_CONNECTED)
    {
        if (event == ADAPTER_EVENT_A2DP_DISCONNECTED)
        {
            new_link_state = ADAPTER_LINK_STATE_IDLE;
        }
    }
    else if (pre_link_state == ADAPTER_LINK_STATE_A2DP_DISCONNECTING)
    {
        if (event == ADAPTER_EVENT_A2DP_DISCONNECTED)
        {
            new_link_state = ADAPTER_LINK_STATE_IDLE;
        }
    }
    else if (pre_link_state == ADAPTER_LINK_STATE_BLE_CONNECTED)
    {
        if (event == ADAPTER_EVENT_BLE_DISCONNECTED)
        {
            new_link_state = ADAPTER_LINK_STATE_IDLE;
        }
    }
    else if (pre_link_state == ADAPTER_LINK_STATE_BLE_CONNECTING)
    {
        if (event == ADAPTER_EVENT_BLE_CONNECTED)
        {
            connected_ble_conn_id = ble_conn_id;
            new_link_state = ADAPTER_LINK_STATE_BLE_CONNECTED;
        }
    }
    else if (pre_link_state == ADAPTER_LINK_STATE_BLE_DISCONNECTING)
    {
        if (event == ADAPTER_EVENT_BLE_DISCONNECTED)
        {
            connected_ble_conn_id = ble_conn_id;
            new_link_state = ADAPTER_LINK_STATE_IDLE;
        }
    }

    /* stable state handle */
    if (new_link_state == ADAPTER_LINK_STATE_IDLE)
    {
        if (headset_conn_status == HEADSET_PHONE_CONNECTED)
        {
            if (bt_a2dp_connect_req(app_cfg_nv.public_bud_addr, 0, BT_A2DP_ROLE_SNK))
            {
                new_link_state = ADAPTER_LINK_STATE_A2DP_CONNECTING;
            }
        }
        else if (headset_conn_status == HEADSET_PHONE_DISCONNECTED)
        {
            app_le_audio_direct_enable_unicast_audio();
            new_link_state = ADAPTER_LINK_STATE_BLE_CONNECTING;
        }
    }
    else if (new_link_state == ADAPTER_LINK_STATE_A2DP_CONNECTED)
    {
        if (headset_conn_status == HEADSET_PHONE_DISCONNECTED)
        {
            if (gap_br_send_acl_disconn_req(app_cfg_nv.public_bud_addr) == GAP_CAUSE_SUCCESS)
            {
                new_link_state = ADAPTER_LINK_STATE_A2DP_DISCONNECTING;
            }
        }
    }
    else if (new_link_state == ADAPTER_LINK_STATE_BLE_CONNECTED)
    {
        if (headset_conn_status == HEADSET_PHONE_CONNECTED)
        {
            if (le_disconnect(connected_ble_conn_id) == GAP_CAUSE_SUCCESS)
            {
                new_link_state = ADAPTER_LINK_STATE_BLE_DISCONNECTING;
            }
        }
    }

    APP_PRINT_TRACE4("adapter_dual_mode_link_mgr: event %d state (%d->%d) headset_status %d",
                     event, pre_link_state, new_link_state, headset_conn_status);

    adapter_link_info.link_state = new_link_state;

    /* start le scan when link idle and no headset connected */
    if (adapter_link_info.link_state == ADAPTER_LINK_STATE_IDLE &&
        headset_conn_status == HEADSET_PHONE_STATUS_UNKOWN)
    {
        app_le_audio_start_auto_pair();
    }
}

void adapter_link_handle_headset_conn_status(T_HEADSET_CONN_STATUS headset_conn_status)
{
    static T_HEADSET_CONN_STATUS pre_conn_status = HEADSET_PHONE_STATUS_UNKOWN;

    if (headset_conn_status != pre_conn_status)
    {
        if (headset_conn_status == HEADSET_PHONE_CONNECTED)
        {
            adapter_dual_mode_link_mgr(ADAPTER_EVENT_HEADSET_PHONE_CONNECTED, 0);
        }
        else if (headset_conn_status == HEADSET_PHONE_DISCONNECTED)
        {
            adapter_dual_mode_link_mgr(ADAPTER_EVENT_HEADSET_PHONE_DISCONNECTED, 0);
        }
        else
        {
            adapter_dual_mode_link_mgr(ADAPTER_EVENT_HEADSET_PHONE_UNKNOWN, 0);
        }

        pre_conn_status = headset_conn_status;
    }
}
#endif

void adapter_msg_cback(uint16_t msg, uint8_t *msg_data, uint32_t msg_dlen)
{
#if UAL_CONSOLE_PRINT
    uint8_t event_buff[50];
    uint16_t buf_len;
#endif
    switch (msg)
    {
    case UAL_ADP_STATE_CHANGE:
        {
            if (*msg_data == BT_STATE_ON)
            {
                app_adater_service_init();
#if UAL_CONSOLE_PRINT
                buf_len = strlen("adapter state on\r\n");
                memcpy(event_buff, "adapter state on\r\n", buf_len);
                console_write(event_buff, buf_len);
#endif
            }
        }
        break;
    case UAL_ADP_LE_SCAN_STATE_CHANGE:
        {
            scan_state_changed_signal(*msg_data);
        }
        break;
    case UAL_DEV_BOND_STATE_CHANGE:
        {
            T_BT_BOND_INFO *p = (T_BT_BOND_INFO *)msg_data;
            bond_state_change(p);
        }
        break;

    case UAL_ADP_BOND_PASSKEY_DISPLAY:
        {

        }
        break;

    case UAL_ADP_BOND_USER_CONFIRMATION:
        {
        }
        break;

    case UAL_ADP_BOND_PASSKEY_INPUT:
        {
        }
        break;

    case UAL_ADP_DEV_AUTH_CMPLT:
        {
            T_IO_MSG bond_dev_msg;
            T_BT_AUTH_INFO *p_msg = (T_BT_AUTH_INFO *)msg_data;
            if (p_msg->status == BT_STATUS_AUTH_KEY_MISSING)
            {
                T_RMV_BOND_DEV *p_rmv_dev = (T_RMV_BOND_DEV *)calloc(1, sizeof(T_RMV_BOND_DEV));
                if (p_rmv_dev)
                {
                    memcpy(p_rmv_dev->bd_addr, p_msg->bd_addr, BD_ADDR_LEN);
                    p_rmv_dev->bd_type = p_msg->bd_type;

                    bond_dev_msg.type = IO_MSG_TYPE_DONGLE_APP;
                    bond_dev_msg.subtype = REMOVE_BOND_EVENT;
                    bond_dev_msg.u.param = (uint32_t)p_rmv_dev;
                    app_io_msg_send(&bond_dev_msg);
                }
            }
#if F_APP_LEA_DONGLE_BINDING
            else if (p_msg->status == BT_STATUS_AUTH_COMPLETE)
            {
                app_le_audio_handle_auth_complete(p_msg->bd_addr);
            }
#endif
        }
        break;

    case UAL_DEV_CONN_STATE_CHANGE:
        {
            T_BT_CONN_INFO *p_conn_info = (T_BT_CONN_INFO *)msg_data;
            if (p_conn_info->type == TRANSPORT_TYPE_LE)
            {
                T_LE_AUDIO *p_ble_link = ble_audio_find_by_conn_id(p_conn_info->conn_id);
                if (p_ble_link)
                {
                    if (p_conn_info->state == BT_CONN_STATE_DISCONNECTED)
                    {
                        p_ble_link->audio_state = 0;
                    }

                    ble_usb_audio_state_change(p_conn_info->bd_addr, p_conn_info->bd_type, p_conn_info->state,
                                               p_ble_link->audio_state, p_conn_info->disc_cause);
                }

                ble_audio_conn_state_change(p_conn_info->conn_id, p_conn_info->state);

#if F_APP_LEA_DONGLE_BINDING
                app_le_audio_handle_conn_state_change(p_conn_info->bd_addr, p_conn_info->bd_type,
                                                      p_conn_info->state);
#endif

                if (p_conn_info->state == BT_CONN_STATE_DISCONNECTED)
                {
                    le_unicast_src_disconnected(p_conn_info->bd_addr, p_conn_info->bd_type);
                }
            }
        }
        break;
    default:
        break;
    }
}

#if F_APP_LEA_DONGLE_BINDING
void adapter_set_scan_by_mmi(bool flag)
{
    scan_by_mmi = flag;

    APP_PRINT_TRACE1("adapter_set_scan_by_mmi: scan_by_mmi %d", scan_by_mmi);

    if (scan_by_mmi)
    {
        app_le_audio_clear_white_list();
        app_le_audio_clear_bud_record();
    }
}

bool adapter_get_scan_by_mmi(void)
{
    return scan_by_mmi;
}

#if F_APP_LEA_DONGLE_BINDING
static void save_bud_addr_to_nv(uint8_t *addr, uint8_t bud_side)
{
    uint8_t *bud_addr;

    if (bud_side == DEVICE_BUD_SIDE_LEFT)
    {
        bud_addr = app_cfg_nv.left_bud_addr;
    }
    else
    {
        bud_addr = app_cfg_nv.right_bud_addr;
    }

    if (memcmp(addr, bud_addr, 6))
    {
        memcpy(bud_addr, addr, 6);
        app_cfg_store();
        app_le_audio_load_white_list();

        APP_PRINT_TRACE2("save_bud_addr_to_nv: addr %s bud_side %d", TRACE_BDADDR(addr), bud_side);
    }
}

static void save_bud_public_addr_to_nv(uint8_t *addr, uint8_t is_stereo)
{
    if ((is_stereo != app_cfg_nv.is_bud_stereo) || memcmp(addr, app_cfg_nv.public_bud_addr, 6))
    {
        app_cfg_nv.is_bud_stereo = is_stereo;
        memcpy(app_cfg_nv.public_bud_addr, addr, 6);
        app_cfg_store();
    }
}
#endif

static void adapter_dongle_scan_callback(uint8_t cb_type, void *result)
{
    T_LE_ADV_INFO *p_report = (T_LE_ADV_INFO *)result;
    T_DONGLE_LEA_ADV_DATA *lea_adv_data = NULL;
    uint8_t *p_sirk = NULL;
    uint8_t *p_bud_id = NULL;
    uint8_t pairing_bit = 0;
    uint16_t pairing_id = 0x0000;
    bool allow_connect = false;
    bool same_pairing_id = false;
    bool same_bud_id = false;
    bool pair_new_bud = false;
    bool pair_exist_bud = false;
    bool gaming_mode = false;

    if (cb_type == SCAN_RESULT_LE && p_report->data_len == 31 && p_report->bd_type == BLE_ADDR_RANDOM)
    {
        /* Parse adv data */
        lea_adv_data = (T_DONGLE_LEA_ADV_DATA *)p_report->p_data;

        pairing_bit = lea_adv_data->pairing_bit;
        gaming_mode = lea_adv_data->low_latency;
        pairing_id = ((uint16_t)lea_adv_data->pairing_id_HI << 8) +
                     ((uint16_t)lea_adv_data->pairing_id_LO);
        p_bud_id = lea_adv_data->pri_bud_mac;
        p_sirk = lea_adv_data->psri_data;

        /* Check condition */
        same_pairing_id = (pairing_id == app_cfg_const.dongle_pairing_uuid);
        same_bud_id = !memcmp(app_cfg_nv.saved_id, p_bud_id, 3);
        pair_new_bud = (scan_by_mmi == true && pairing_bit == true);
        pair_exist_bud = (scan_by_mmi == false && pairing_bit == false && same_bud_id);

        if (same_pairing_id && (pair_new_bud || pair_exist_bud))
        {
            allow_connect = true;

            app_device_save_pairing_id(p_bud_id, 3);
        }

        if (same_pairing_id)
        {
            APP_PRINT_TRACE7("adapter_dongle_scan_callback: allow_connect %d, pairing_id 0x%x, pairing_bit %d, scan_by_mmi %d, bud_id %b, saved_id %b, rssi %d",
                             allow_connect, pairing_id, pairing_bit, scan_by_mmi,
                             TRACE_BINARY(3, p_bud_id),
                             TRACE_BINARY(3, app_cfg_nv.saved_id),
                             p_report->rssi);
        }

        if (allow_connect)
        {
#if F_APP_LEA_DONGLE_BINDING
            save_bud_addr_to_nv(p_report->bd_addr, lea_adv_data->bud_side);
            save_bud_public_addr_to_nv(lea_adv_data->public_addr, lea_adv_data->is_stereo);
#endif

            if (disc_mgr_find_device(p_report->bd_addr, p_report->bd_type) == NULL)
            {
                disc_mgr_add_device(p_report->bd_addr, p_report->bd_type);
            }

            scan_results_callback(p_report->bd_addr, p_report->bd_type, 1,
                                  p_report->adv_sid, NULL, 0, p_report->rssi, gaming_mode);
            if (p_sirk)
            {
                le_csis_client_handle_mem_rsi(p_sirk, p_report->bd_type, p_report->bd_addr);
            }
        }
    }
}
#else
static bool apdater_parse_adv(T_LE_ADV_INFO *p_report, T_DISC_RESULT *p_result)
{
    if ((p_report == NULL) || (p_result == NULL))
    {
        return false;
    }

    uint16_t len = 0;
    uint8_t type = 0;
    uint8_t *value = NULL;
    uint8_t *p_data = p_report->p_data;
    uint8_t *pp = NULL;
    uint16_t remain_len = p_report->data_len;
    uint8_t vendor_type = 0;

    uint16_t uuid = 0;

    memset(p_result, 0, sizeof(T_DISC_RESULT));
    p_result->bd_addr = p_report->bd_addr;
    p_result->bd_type = p_report->bd_type;
    p_result->rssi = p_report->rssi;
    p_result->adv_sid = p_report->adv_sid;

    while (remain_len)
    {
        len = *p_data;
        if ((len == 0) || (len > remain_len - 1))
        {
            break;
        }

        p_data ++;
        type = *p_data;
        value = p_data + 1;
        pp = value;
        p_data += len;
        remain_len -= len + 1;


        if (len == 1)
        {
            APP_PRINT_WARN2("apdater_parse_adv: %b type %x len 1 invalid", TRACE_BDADDR(p_report->bd_addr),
                            type);
            continue;
        }

        switch (type)
        {
        case GAP_ADTYPE_16BIT_MORE:
            {
                if (disc_mgr_cb.filter_uuid == 0)
                {
                    continue;
                }

                len -= 1;
                while (len > 1)
                {
                    LE_STREAM_TO_UINT16(uuid, pp);
                    if (uuid == disc_mgr_cb.filter_uuid)
                    {
                        p_result->uuid_match = true;
                        break;
                    }
                    len -= 2;
                }
            }
            break;
        case GAP_ADTYPE_16BIT_COMPLETE:
        case GAP_ADTYPE_SERVICE_DATA:
            {
                LE_STREAM_TO_UINT16(uuid, pp);
                if (disc_mgr_cb.filter_uuid)
                {
                    if (uuid == disc_mgr_cb.filter_uuid)
                    {
                        p_result->uuid_match = true;
                    }
                }

                if ((uuid == GATT_UUID_ASCS) && (len >= 9))
                {
                    LE_STREAM_TO_UINT8(p_result->connect_mode, pp);
                }
            }
            break;

        case GAP_ADTYPE_RSI:
            {
                p_result->p_rsi = value;
            }
            break;

        case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
            {
                p_result->p_name = value;
                p_result->name_len = len - 1;
            }
            break;

        case GAP_ADTYPE_MANUFACTURER_SPECIFIC:
            {
                LE_STREAM_TO_UINT16(uuid, pp);
                if (uuid == 0x005D) //TODO RTK_COMPANY_ID
                {
                    if (pp[0] > 0x01) //length
                    {
                        pp++;
                        LE_STREAM_TO_UINT8(vendor_type, pp);
                        if (vendor_type == VENDOR_DATA_TYPE_GAMING_MODE)
                        {
                            LE_STREAM_TO_UINT8(p_result->gaming_mode, pp);
                            APP_PRINT_ERROR1("VENDOR_DATA_TYPE_GAMING_MODE %d", p_result->gaming_mode);
                        }
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    return true;
}

static void adapter_scan_callback(uint8_t cb_type, void *result)
{
    T_DISC_RESULT scan_result;
    T_LE_ADV_INFO *p_report = (T_LE_ADV_INFO *)result;

    if (cb_type != SCAN_RESULT_LE)
    {
        return;
    }

    if (apdater_parse_adv(p_report, &scan_result) == false)
    {
        return;
    }

    if ((disc_mgr_cb.filter_uuid) && (scan_result.uuid_match == false))
    {
        return;
    }

    if (disc_mgr_find_device(p_report->bd_addr, p_report->bd_type))
    {
        return;
    }

    disc_mgr_add_device(p_report->bd_addr, p_report->bd_type);

    scan_result.bd_type = le_privacy_convert_addr_type((T_GAP_REMOTE_ADDR_TYPE)p_report->bd_type);
    scan_results_callback(scan_result.bd_addr, scan_result.bd_type,
                          scan_result.connect_mode, scan_result.adv_sid,
                          scan_result.p_name, scan_result.name_len,
                          scan_result.rssi, scan_result.gaming_mode);
    if (scan_result.p_rsi)
    {
        le_csis_client_handle_mem_rsi(scan_result.p_rsi, scan_result.bd_type, p_report->bd_addr);
    }
}
#endif

uint32_t adapter_start_le_scan(uint16_t uuid, bool has_white_list)
{
    disc_mgr_clear_device_list();
    disc_mgr_cb.filter_uuid = uuid;
    uint8_t filter_policy = GAP_SCAN_FILTER_ANY;

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    if (adapter_link_info.link_state == ADAPTER_LINK_STATE_A2DP_CONNECTED  ||
        adapter_link_info.link_state == ADAPTER_LINK_STATE_A2DP_CONNECTING ||
        adapter_link_info.link_state == ADAPTER_LINK_STATE_BLE_DISCONNECTING)
    {
        APP_PRINT_ERROR1("adapter_start_le_scan: no need to start scan when a2dp not idle %d",
                         adapter_link_info.link_state);
        return 0;
    }
#endif

    if (has_white_list)
    {
        filter_policy = GAP_SCAN_FILTER_WHITE_LIST;
    }

#if F_APP_LEA_DONGLE_BINDING
    disc_mgr_cb.client_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, filter_policy,
                                                    adapter_dongle_scan_callback);
#else
    disc_mgr_cb.client_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, filter_policy,
                                                    adapter_scan_callback);
#endif

#if UAL_CONSOLE_PRINT
    uint8_t event_buff[50];
    uint16_t buf_len;

    if (disc_mgr_cb.client_id > 0)
    {
        buf_len = sprintf((char *)event_buff, "scan success client id %d\r\n", disc_mgr_cb.client_id);
    }
    else
    {
        buf_len = sprintf((char *)event_buff, "scan fail %d\r\n", disc_mgr_cb.client_id);
    }

    console_write(event_buff, buf_len);
#endif
    if (disc_mgr_cb.client_id < 0)
    {
        return 0;
    }
    return disc_mgr_cb.client_id;
}

void adapter_stop_le_scan(void)
{
    bt_adap_stop_discovery(disc_mgr_cb.client_id);
}
