#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "le_audio_service.h"
#include "le_service_mgr.h"
#include "ual_api_types.h"
#include "app_timer.h"
#include "app_le_audio.h"
#include "app_usb_layer.h"
#include "app_io_msg.h"
#include "ascs_def.h"
#include "ual_dev_mgr.h"
#include "dev_mgr.h"
#include "ble_audio_def.h"
#include "codec_def.h"
#include "codec_qos.h"
#include "le_unicast_src_service.h"
#include "le_vcs_service.h"
#include "app_led_ctl.h"
#include "cap.h"
#include "app_cfg.h"
#include "app_usb_uac.h"
#include "app_usb_audio_wrapper.h"
#include "le_vc_service.h"
#include "app_dongle_vol.h"
#include "app_line_in.h"
#include "teams_call_control.h"
#include "cis_topo_cfg.h"
#include "le_unicast_gaming.h"
#include "le_mic_service.h"
#include "app_usb.h"
#include "app_link_util.h"
#include "ual_api_types.h"

#if DONGLE_LE_AUDIO
#include "app_le_audio.h"
#endif

#if F_APP_LEA_DONGLE_BINDING
#include "app_le_audio.h"
#include "app_adapter_service.h"
#include "app_dongle_transmit_client.h"
#include "app_ctrl_pkt_policy.h"
#endif

#include "le_unicast_gaming.h"
#include "le_audio_data.h"
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
#include "uac_silence_detect.h"
#endif
#if (LE_EXT_FTL_SUPPORT == 1)
#include "app_le_ext_ftl.h"
#include "gap_storage_le.h"
#endif

#if F_APP_USB_GIP_SUPPORT
#include "app_gip.h"
#endif

#if DONGLE_LE_AUDIO

#define APP_CSIS_MASK      0x0001
#define APP_ASCS_MASK      0x0002
#define APP_PACS_MASK      0x0004

#define APP_LCHNL_MASK           0x00000001
#define APP_RCHNL_MASK          0x00000002
#define APP_OTHER_CHNL_MASK          (~(APP_LCHNL_MASK | APP_RCHNL_MASK))

#define UP_STREAM_ACTIVE            0x01
#define DOWN_STREAM_ACTIVE          0x02
#define AUDIO_STREAM_ACTIVE_MASK    0x03

typedef enum
{
    TIMER_ID_LE_AUDIO_PAIRING  = 0x01,
    TIMER_ID_LE_AUDIO_TRY_STREAM = 0x02,
    TIMER_ID_LE_AUDIO_CONN_TIMEOUT = 0x03,
    TIMER_ID_LE_AUDIO_RESTART_STREAM = 0x04,
} T_LE_AUDIO_TIMER_ID;

#define LE_AUDIO_PAIRING_TIMEOUT        10000   /* 10s */
#define LE_AUDIO_TRY_STREAM_TIMEOUT     (200)

#define LE_LOCAL_NAME_LEN_MAX   (36)

typedef enum
{
    LEA_STATE_IDLE          = 0,
    LEA_STATE_PAIRING       = 1,
    LEA_STATE_CONFIG        = 2,
    LEA_STATE_WAIT_STREAM   = 3,
    LEA_STATE_STREAMING     = 4,
    LEA_STATE_RELEASING     = 5
} T_APP_LE_AUDIO_STATE;

typedef enum
{
    UAC_DIR_DOWNSTREAM,
    UAC_DIR_UPSTREAM,
} T_UAC_DIR;

typedef enum
{
    UAC_STATE_DEACTIVE,
    UAC_STATE_ACTIVE,
} T_UAC_STATE;

typedef enum
{
    UNICAST_MODE_DISABLE,
    UNICAST_MODE_ENABLE,
} T_UNICAST_MODE_STATE;

typedef enum
{
    LEA_PAIR_DISABLE,
    LEA_PAIR_ENABLE,
} T_LEA_PAIR_STATE;

typedef enum
{
    LEA_LINK_DISCONNECTED,
    LEA_LINK_CONNECTED,             //ASCS and PACS read done, set to LEA_LINK_CONNECTED
} T_LEA_LINK_STATE;

typedef enum
{
    LEA_BAP_STATE_IDLE,
    LEA_BAP_STATE_STARTING,
    LEA_BAP_STATE_STREAMING,
    LEA_BAP_STATE_RELEASING,
} T_LEA_BAP_STATE;

typedef enum
{
    LEA_EVT_USB_CMD                 = 1,            //cmd come from usb-layer
    LEA_EVT_STATE_TRANS             = 2,            //app-le-audio state machine
    LEA_EVT_MODE_CHG                = 3,
    LEA_EVT_PAIR_STATE_CHG          = 4,
    LEA_EVT_LINK_STATE_CHG          = 5,            //audio state and acl link state
    LEA_EVT_BAP_STATE_CHG           = 6,
    LEA_EVT_STREAM_STATE_CHG        = 7,
} T_APP_LEA_EVENT;

typedef struct
{
    T_BT_DISC_STATE     scan_state;
    uint8_t             play_mode;   //UNICAST_CONVERSATION_MODE or UNICAST_MEDIA_MODE
    uint8_t             bap_state;
    uint8_t             lea_state;  /* T_APP_LE_AUDIO_STATE */
    T_BT_BOND_STATE     bond_state;
    uint8_t             stream_active;
    uint8_t             csis_hash[3];
    uint8_t             csis_total_size;
    T_UALIST_HEAD       dev_list;
} T_APP_LE_AUDIO;

typedef struct
{
    bool                rssi_enable;
    int8_t              rssi;
    bool                bd_addr_enable;
    uint8_t             bd_type[2];
    uint8_t             bd_addr[2][BD_ADDR_LEN];
    bool                name_enable;
    uint8_t             name_len;
    uint8_t             name[LE_LOCAL_NAME_LEN_MAX];
} T_APP_LE_FILTER;

typedef struct
{
    T_UALIST_HEAD       list;
    uint8_t             bd_addr[BD_ADDR_LEN];
    T_BLE_BD_TYPE       bd_type;
    uint16_t            srv_val_msk;
    uint32_t            snk_audio_loc;
    uint32_t            src_audio_loc;
    uint8_t             vcs_setting;
} T_DEV_DB;

typedef struct
{
    uint16_t event;
    uint16_t subevent;
    uint32_t param;
} T_APP_LE_DATA;


bool app_le_audio_try_stream(void);
static void process_lea_event_in_idle(uint16_t event, uint16_t subevent, uint32_t param);
static void process_lea_event_in_pairing(uint16_t event, uint16_t subevent, uint32_t param);
static void process_lea_event_in_configure(uint16_t event, uint16_t subevent, uint32_t param);
static void process_lea_event_in_wait_stream(uint16_t event, uint16_t subevent, uint32_t param);
static void process_lea_event_in_streaming(uint16_t event, uint16_t subevent, uint32_t param);
static void process_lea_event_in_releasing(uint16_t event, uint16_t subevent, uint32_t param);

#if F_APP_LEA_DONGLE_BINDING
static void app_le_audio_handle_conn_num_change(void);

typedef struct t_app_le_audio_white_list_elem
{
    struct t_app_le_audio_white_list_elem *p_next;
    uint8_t bd_addr[6];
    T_GAP_REMOTE_ADDR_TYPE bd_type;
    bool loaded;
} T_APP_LE_AUDIO_WHITE_LIST_ELEM;

static T_OS_QUEUE le_audio_white_list_queue;
#endif

static bool le_audio_scan_start = false;
static T_APP_LE_AUDIO *app_lea_db;
static uint8_t le_audio_timer_id = 0;
static uint8_t timer_idx_pairing_timeout = 0;
static uint8_t timer_idx_try_stream = 0;
static uint8_t timer_idx_restart_lea_stream = 0;
#if F_APP_LEA_DONGLE_BINDING
static uint8_t timer_idx_check_ble_conn_timeout = 0;
#endif
static T_APP_LE_FILTER app_le_filter;

#if (LE_EXT_FTL_SUPPORT == 1)
void app_le_audio_load_dev(void);
#endif

#if (F_APP_LEA_DONGLE_BINDING == 0)
static bool app_le_check_scan_info_according_filter(uint8_t *data, uint8_t len);
#endif

static void app_le_audio_msg_send(uint32_t param)
{
    T_IO_MSG le_audio_msg;

    le_audio_msg.type = IO_MSG_TYPE_DONGLE_APP;
    le_audio_msg.subtype = APP_LE_AUDIO_MSG_SEND_EVENT;
    le_audio_msg.u.param = param;
    app_io_msg_send(&le_audio_msg);
}

static T_DEV_DB *find_device_by_addr(uint8_t *bd_addr)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    if (ualist_empty(&app_lea_db->dev_list))
    {
        APP_PRINT_ERROR0("find_device_by_addr: list is empty");
        return NULL;
    }

    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);
        if (!memcmp(p_dev->bd_addr, bd_addr, BD_ADDR_LEN))
        {
            return p_dev;
        }
    }

    APP_PRINT_ERROR1("find_device_by_addr: can't find device %b", TRACE_BDADDR(bd_addr));
    return NULL;
}

/*
* device cnt 1 without csis
* device cnt 1 with csis
* device cnt 2
*/

static T_LE_AUDIO *lea_get_device_link(T_DEV_DB *p_dev)
{
    if (!p_dev)
    {
        return NULL;
    }
    if (!(p_dev->srv_val_msk & APP_PACS_MASK) ||
        !(p_dev->srv_val_msk & APP_ASCS_MASK))
    {
        APP_PRINT_INFO1("lea_get_device_link: PACS or ASCS unknown %s",
                        TRACE_BDADDR(p_dev->bd_addr));
        return NULL;
    }
    T_BT_DEVICE *p_sec_dev = ual_find_device_by_addr(p_dev->bd_addr);
    if (p_sec_dev == NULL)
    {
        APP_PRINT_ERROR1("lea_get_device_link: can't find p_sec_dev %b", TRACE_BDADDR(p_dev->bd_addr));
        return NULL;
    }
    return ble_audio_find_by_conn_handle(p_sec_dev->le_conn_handle);
}

static bool app_lea_check_link_pacs(T_LE_AUDIO *p_link, uint8_t mode, uint8_t dir,
                                    uint8_t ase_num, uint8_t chnl_cnt, uint16_t sample,
                                    bool use_pref_context)
{
    uint8_t link_chnl_cnt = 0;
    if (!p_link)
    {
        return false;
    }

    uint8_t link_ase_num = 0;
    uint16_t link_sample = 0;
    uint32_t link_loc = 0;
    uint16_t target_context = 0;

    if (dir == SERVER_AUDIO_SINK)
    {
        link_ase_num = p_link->snk_ase_num;
        link_loc = p_link->snk_audio_loc;
        target_context = AUDIO_CONTEXT_CONVERSATIONAL | AUDIO_CONTEXT_CONVERSATIONAL | AUDIO_CONTEXT_MEDIA;
        if ((p_link->snk_avail_context & target_context) == 0)
        {
            APP_PRINT_ERROR3("app_lea_check_link_pacs: link id %d snk_avail_context %x target_context %d",
                             p_link->conn_id, p_link->snk_avail_context, target_context);
            return false;
        }
        if (mode == UNICAST_MEDIA_MODE)
        {
            link_sample = bap_pacs_get_lc3_snk_table_msk(p_link->conn_handle,
                                                         AUDIO_CONTEXT_MEDIA,
                                                         chnl_cnt, 1);

        }
        else
        {
            link_sample = bap_pacs_get_lc3_snk_table_msk(p_link->conn_handle,
                                                         AUDIO_CONTEXT_CONVERSATIONAL,
                                                         chnl_cnt, 1);
        }
        if (!use_pref_context && !(link_sample & sample))
        {
            link_sample = bap_pacs_get_lc3_snk_table_msk(p_link->conn_handle,
                                                         AUDIO_CONTEXT_UNSPECIFIED,
                                                         chnl_cnt, 1);
        }
    }
    else
    {
        link_ase_num = p_link->src_ase_num;
        link_loc = p_link->src_audio_loc;
        target_context = AUDIO_CONTEXT_CONVERSATIONAL | AUDIO_CONTEXT_UNSPECIFIED;
        if ((p_link->src_avail_context & target_context) == 0)
        {
            APP_PRINT_ERROR3("app_lea_check_link_pacs: link id %d src_avail_context %x target_context %d",
                             p_link->conn_id, p_link->src_avail_context, target_context);
            return false;
        }
        link_sample = bap_pacs_get_lc3_src_table_msk(p_link->conn_handle,
                                                     AUDIO_CONTEXT_CONVERSATIONAL,
                                                     chnl_cnt, 1);
        if (!use_pref_context && !(link_sample & sample))
        {
            link_sample = bap_pacs_get_lc3_src_table_msk(p_link->conn_handle,
                                                         AUDIO_CONTEXT_UNSPECIFIED,
                                                         chnl_cnt, 1);
        }
    }
    if (link_ase_num < ase_num)
    {
        return false;
    }

    if (!(link_sample & sample))
    {
        return false;
    }

    if (link_loc == AUDIO_LOCATION_MONO)
    {
        link_chnl_cnt = 1;
    }
    else
    {
        link_chnl_cnt = __builtin_popcount(link_loc);
    }

    if (link_chnl_cnt < chnl_cnt)
    {
        return false;
    }

    return true;
}

static bool app_lea_check_audio_loc(T_LE_AUDIO *p_link, uint8_t dir, uint8_t chnl_cnt)
{
    if (!p_link)
    {
        return false;
    }

    uint32_t audio_loc = (dir == SERVER_AUDIO_SINK) ?
                         (p_link->snk_audio_loc) : (p_link->src_audio_loc);

    if (chnl_cnt == 2)
    {
        if (audio_loc & (APP_LCHNL_MASK | APP_RCHNL_MASK))
        {
            return true;
        }
    }
    else
    {
        if ((audio_loc & APP_LCHNL_MASK) || (audio_loc & APP_RCHNL_MASK))
        {
            return true;
        }
    }
    return false;
}

static T_AUDIO_SET_CONF *find_unicast_media_conf(T_LE_AUDIO **p_link, uint8_t num,
                                                 T_AUDIO_SET_CONF *p_cfg, uint8_t len,
                                                 bool use_pref_context)
{
    if ((!p_link) || (!p_cfg))
    {
        return NULL;
    }

    uint8_t snk_ase_num = 0;
    uint8_t snk_chnl_cnt = 0;
    uint16_t snk_sample = 0;
    uint8_t retval = false;

    for (uint8_t i = 0; i < len; i++)
    {
        snk_ase_num = p_cfg[i].ase_cnt / p_cfg[i].device_cnt;
        snk_chnl_cnt = p_cfg[i].chnl_cnt;
        snk_sample = p_cfg[i].sample_rate_table;

        for (uint8_t j = 0; j < num; j++)
        {
            retval = app_lea_check_link_pacs(p_link[j], UNICAST_MEDIA_MODE, SERVER_AUDIO_SINK,
                                             snk_ase_num, snk_chnl_cnt, snk_sample, use_pref_context);
            if (!retval)
            {
                break;
            }

            retval = app_lea_check_audio_loc(p_link[j], SERVER_AUDIO_SINK, snk_sample);
            if (!retval)
            {
                break;
            }
        }

        if (!retval)
        {
            continue;
        }

        APP_PRINT_INFO2("find_unicast_media_conf: use_pref_context %d idx %x", use_pref_context, i);
        return (T_AUDIO_SET_CONF *)&p_cfg[i];
    }
    return NULL;
}

static T_AUDIO_SET_CONF *find_unicast_conversation_conf(T_LE_AUDIO **p_link, uint8_t num,
                                                        T_AUDIO_SET_CONF p_cfg[][2], uint8_t len,
                                                        bool use_pref_context)
{
    if ((!p_link) || (!p_cfg))
    {
        return NULL;
    }

    T_AUDIO_SET_CONF *p_snk_cfg = NULL;
    T_AUDIO_SET_CONF *p_src_cfg = NULL;
    uint8_t snk_ase_num = 0;
    uint8_t snk_chnl_cnt = 0;
    uint16_t snk_sample = 0;
    uint8_t src_ase_num = 0;
    uint8_t src_chnl_cnt = 0;
    uint16_t src_sample = 0;

    bool snk_pacs_ret = false;
    bool snk_loc_ret = false;
    bool src_pacs_ret = false;
    bool src_loc_ret = false;
    uint8_t snk_valid_cnt = 0;
    uint8_t src_valid_cnt = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        snk_valid_cnt = 0;
        src_valid_cnt = 0;
        p_snk_cfg = (T_AUDIO_SET_CONF *)&p_cfg[i][0];
        p_src_cfg = (T_AUDIO_SET_CONF *)&p_cfg[i][1];
        snk_ase_num = p_snk_cfg->ase_cnt / p_snk_cfg->device_cnt;
        snk_chnl_cnt = p_snk_cfg->chnl_cnt;
        snk_sample = p_snk_cfg->sample_rate_table;

        src_ase_num = p_src_cfg->ase_cnt / p_src_cfg->device_cnt;
        src_chnl_cnt = p_src_cfg->chnl_cnt;
        src_sample = p_src_cfg->sample_rate_table;
        for (uint8_t j = 0; j < num; j++)
        {
            snk_pacs_ret = app_lea_check_link_pacs(p_link[j], UNICAST_CONVERSATION_MODE, SERVER_AUDIO_SINK,
                                                   snk_ase_num, snk_chnl_cnt, snk_sample, use_pref_context);
            snk_loc_ret = app_lea_check_audio_loc(p_link[j], SERVER_AUDIO_SINK, snk_chnl_cnt);
            if (snk_pacs_ret && snk_loc_ret)
            {
                snk_valid_cnt++;
            }
            src_pacs_ret = app_lea_check_link_pacs(p_link[j], UNICAST_CONVERSATION_MODE, SERVER_AUDIO_SOURCE,
                                                   src_ase_num, src_chnl_cnt, src_sample, use_pref_context);
            src_loc_ret = app_lea_check_audio_loc(p_link[j], SERVER_AUDIO_SOURCE, src_chnl_cnt);
            if (src_pacs_ret && src_loc_ret)
            {
                src_valid_cnt++;
            }
        }

        if ((snk_valid_cnt >= num)
            && (src_valid_cnt >= num))
        {
            APP_PRINT_INFO2("find_unicast_conversation_conf: use_pref_context %x idx %x", use_pref_context, i);
            return (T_AUDIO_SET_CONF *)&p_cfg[i][0];
        }
    }

    return NULL;
}

static uint8_t app_lea_get_freq_from_mask(uint16_t mask)
{
    uint8_t freq = 0;
    switch (mask)
    {
    case SAMPLE_FREQ_48K_TABLE_MASK:
        freq = SAMPLING_FREQUENCY_CFG_48K;
        break;
    case SAMPLE_FREQ_441K_TABLE_MASK:
        freq = SAMPLING_FREQUENCY_CFG_44_1K;
        break;
    case SAMPLE_FREQ_32K_TABLE_MASK:
        freq = SAMPLING_FREQUENCY_CFG_32K;
        break;
    case SAMPLE_FREQ_24K_TABLE_MASK:
        freq = SAMPLING_FREQUENCY_CFG_24K;
        break;
    case SAMPLE_FREQ_16K_TABLE_MASK:
        freq = SAMPLING_FREQUENCY_CFG_16K;
        break;
    case SAMPLE_FREQ_8K_TABLE_MASK:
        freq = SAMPLING_FREQUENCY_CFG_8K;
        break;
    default:
        break;
    }
    return freq;
}

static T_LE_AUDIO *app_lea_get_connected_dev_link(uint8_t idx)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;
    T_LE_AUDIO *p_link = NULL;
    uint8_t device_cnt = 0;

    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);
        p_link = lea_get_device_link(p_dev);
        if (p_link &&
            (p_link->state == BT_CONN_STATE_CONNECTED) &&
            p_link->csis_disc_done)
        {
            if (idx == device_cnt)
            {
                return p_link;
            }
            device_cnt++;
        }
    }
    return NULL;
}

static uint32_t app_lea_get_chnl_loc(uint8_t chnl_cnt, uint32_t sup_loc, uint8_t device_idx,
                                     uint8_t ase_idx)
{
    uint32_t chnl_loc = 0;

    if (chnl_cnt == 2)
    {
        if ((sup_loc & (AUDIO_LOCATION_FL | AUDIO_LOCATION_FR)) == (AUDIO_LOCATION_FL | AUDIO_LOCATION_FR))
        {
            chnl_loc = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;
        }
    }
    else if (chnl_cnt == 1)
    {
        if ((sup_loc & (AUDIO_LOCATION_FL | AUDIO_LOCATION_FR)) == (AUDIO_LOCATION_FL | AUDIO_LOCATION_FR))
        {
            if (device_idx == 0)
            {
                chnl_loc = (ase_idx == 0) ? AUDIO_LOCATION_FL : AUDIO_LOCATION_FR;
            }
            else
            {
                chnl_loc = (ase_idx == 0) ? AUDIO_LOCATION_FR : AUDIO_LOCATION_FL;
            }
        }
        else if (sup_loc & AUDIO_LOCATION_FL)
        {
            chnl_loc = AUDIO_LOCATION_FL;
        }
        else if (sup_loc & AUDIO_LOCATION_FR)
        {
            chnl_loc = AUDIO_LOCATION_FR;
        }
    }
    return chnl_loc;
}


static bool app_lea_media_cfg_ase(T_LE_AUDIO **p_link, uint8_t num, T_AUDIO_SET_CONF *p_cfg)
{
    if ((!p_link) || (num > 2) || (num == 0))
    {
        return false;
    }

    T_BT_DEVICE *p_dev = NULL;
    T_UNICAST_ASE_PARAM param[2] = {0x0};
    uint8_t ase_num = 0;
    uint32_t sup_loc = 0;

    ase_num = p_cfg->ase_cnt / p_cfg->device_cnt;

    for (uint8_t j = 0; j < num; j++)
    {
        p_dev = ual_find_device_by_conn_id(p_link[j]->conn_id);
        if (!p_dev)
        {
            return false;
        }
        sup_loc = p_link[j]->snk_audio_loc;
        for (uint8_t i = 0; i < ase_num; i++)
        {
            param[i].ase_id = i + 1;
            param[i].real_ase_id = 0;
            param[i].direction = SERVER_AUDIO_SINK;
            param[i].sample_freq = app_lea_get_freq_from_mask(p_cfg->sample_rate_table);
            param[i].prefer_codec_index = 0xFF;
            /*FIXME: what to do if ase not support FL or FR*/
            param[i].chnl_loc = app_lea_get_chnl_loc(p_cfg->chnl_cnt, sup_loc, j, i);
        }

        if (!le_audio_add_unicast_cfg(p_dev->bd_addr, UNICAST_MEDIA_MODE, ase_num, param))
        {
            le_audio_clear_unicast_device_cfg(0, NULL);
            return false;
        }
    }
    return true;
}

static bool app_lea_conversation_cfg_ase(T_LE_AUDIO **p_link, uint8_t num, T_AUDIO_SET_CONF *p_cfg)
{
    if ((!p_link) || (num > 2) || (num == 0))
    {
        return false;
    }

    T_AUDIO_SET_CONF *p_snk_cfg = (T_AUDIO_SET_CONF *)&p_cfg[0];
    T_AUDIO_SET_CONF *p_src_cfg = (T_AUDIO_SET_CONF *)&p_cfg[1];
    T_BT_DEVICE *p_dev = NULL;
    uint8_t snk_ase_num = 0;
    uint8_t src_ase_num = 0;
    uint32_t snk_sup_loc = 0;
    uint32_t src_sup_loc = 0;
    uint8_t idx = 0;

    T_UNICAST_ASE_PARAM param[4] = {0x0};

    snk_ase_num = p_snk_cfg->ase_cnt / p_snk_cfg->device_cnt;
    src_ase_num = p_src_cfg->ase_cnt / p_src_cfg->device_cnt;

    for (uint8_t j = 0; j < num; j++)
    {
        p_dev = ual_find_device_by_conn_id(p_link[j]->conn_id);
        if (!p_dev)
        {
            return false;
        }
        snk_sup_loc = p_link[j]->snk_audio_loc;
        src_sup_loc = p_link[j]->src_audio_loc;
        idx = 0;
        for (uint8_t i = 0; i < snk_ase_num; i++)
        {
            param[idx].ase_id = idx + 1;
            param[idx].real_ase_id = 0;
            param[idx].direction = SERVER_AUDIO_SINK;
            param[idx].sample_freq = app_lea_get_freq_from_mask(p_snk_cfg->sample_rate_table);
            param[idx].prefer_codec_index = 0xFF;
            /*FIXME: what to do if ase not support FL or FR*/
            param[idx].chnl_loc = app_lea_get_chnl_loc(p_snk_cfg->chnl_cnt, snk_sup_loc, j, i);
            idx ++;
        }

        for (uint8_t i = 0; i < src_ase_num; i++)
        {
            idx = i + snk_ase_num;
            param[idx].ase_id = idx + 1;
            param[idx].real_ase_id = 0;
            param[idx].direction = SERVER_AUDIO_SOURCE;
            param[idx].sample_freq = app_lea_get_freq_from_mask(p_src_cfg->sample_rate_table);
            param[idx].prefer_codec_index = 0xFF;
            /*FIXME: what to do if ase not support FL or FR*/
            param[idx].chnl_loc = app_lea_get_chnl_loc(p_src_cfg->chnl_cnt, src_sup_loc, j, i);
            idx++;
        }

        if (!le_audio_add_unicast_cfg(p_dev->bd_addr, UNICAST_CONVERSATION_MODE,
                                      snk_ase_num + src_ase_num, param))
        {
            le_audio_clear_unicast_device_cfg(0, NULL);
            return false;
        }
    }
    return true;
}

static bool find_first_supported_media_conf(void)
{
    T_LE_AUDIO *p_link[2] = {NULL, NULL};
    uint8_t num = 0;
    uint8_t cfg_table_size = 0;
    T_AUDIO_SET_CONF *p_cfg_table = NULL;
    T_AUDIO_SET_CONF *p_cfg = NULL;

    p_link[0] = app_lea_get_connected_dev_link(0);
    p_link[1] = app_lea_get_connected_dev_link(1);

    if (!p_link[0])
    {
        return false;
    }

    if (p_link[0]->csis_size <= 1)
    {
        num = 1;
        APP_PRINT_INFO1("find_media_conf: headset, gaming %x", le_unicast_is_gaming_mode());
        if (le_unicast_is_gaming_mode())
        {
            cfg_table_size = sizeof(lea_headset_gaming_media_conf) / sizeof(T_AUDIO_SET_CONF);
            p_cfg_table = (T_AUDIO_SET_CONF *)lea_headset_gaming_media_conf;
        }
        else
        {
            cfg_table_size = sizeof(lea_headset_media_conf) / sizeof(T_AUDIO_SET_CONF);
            p_cfg_table = (T_AUDIO_SET_CONF *)lea_headset_media_conf;
        }
    }
    else
    {
        num = (p_link[1] == NULL) ? 1 : 2;
        APP_PRINT_INFO2("find_media_conf: rws num %x gaming %x", num, le_unicast_is_gaming_mode());
        if (le_unicast_is_gaming_mode())
        {
            cfg_table_size = sizeof(lea_rws_gaming_media_conf) / sizeof(T_AUDIO_SET_CONF);
            p_cfg_table = (T_AUDIO_SET_CONF *)lea_rws_gaming_media_conf;
        }
        else
        {
            cfg_table_size = sizeof(lea_rws_media_conf) / sizeof(T_AUDIO_SET_CONF);
            p_cfg_table = (T_AUDIO_SET_CONF *)lea_rws_media_conf;
        }
    }

    p_cfg = find_unicast_media_conf(p_link, num, p_cfg_table, cfg_table_size, true);
    if (!p_cfg)
    {
        p_cfg = find_unicast_media_conf(p_link, num, p_cfg_table, cfg_table_size, false);
        if (!p_cfg)
        {
            return NULL;
        }
    }
    return app_lea_media_cfg_ase(p_link, num, p_cfg);
}


static bool find_first_supported_conversation_conf(void)
{
    T_LE_AUDIO *p_link[2] = {NULL, NULL};
    uint8_t num = 0;
    uint8_t cfg_table_size = 0;
    T_AUDIO_SET_CONF(*p_cfg_table)[2] = NULL;
    T_AUDIO_SET_CONF *p_cfg = NULL;

    p_link[0] = app_lea_get_connected_dev_link(0);
    p_link[1] = app_lea_get_connected_dev_link(1);

    if (!p_link[0])
    {
        return false;
    }

    if (p_link[0]->csis_size <= 1)
    {
        num = 1;
        APP_PRINT_INFO1("find_conversation_conf: headset gaming %x", le_unicast_is_gaming_mode());
        if (le_unicast_is_gaming_mode())
        {
            cfg_table_size = sizeof(lea_headset_gaming_conversation_conf) / (sizeof(T_AUDIO_SET_CONF) * 2);
            p_cfg_table = (T_AUDIO_SET_CONF(*)[2])lea_headset_gaming_conversation_conf;
        }
        else
        {
            cfg_table_size = sizeof(lea_headset_conversation_conf) / (sizeof(T_AUDIO_SET_CONF) * 2);
            p_cfg_table = (T_AUDIO_SET_CONF(*)[2])lea_headset_conversation_conf;
        }
    }
    else
    {
        num = (p_link[1] == NULL) ? 1 : 2;
        APP_PRINT_INFO2("find_conversation_conf: rws num %x gaming %x", num, le_unicast_is_gaming_mode());
        if (le_unicast_is_gaming_mode())
        {
            cfg_table_size = sizeof(lea_rws_gaming_conversation_conf) / (sizeof(T_AUDIO_SET_CONF) * 2);
            p_cfg_table = (T_AUDIO_SET_CONF(*)[2])lea_rws_gaming_conversation_conf;
        }
        else
        {
            cfg_table_size = sizeof(lea_rws_conversation_conf) / (sizeof(T_AUDIO_SET_CONF) * 2);
            p_cfg_table = (T_AUDIO_SET_CONF(*)[2])lea_rws_conversation_conf;
        }
    }

    p_cfg = find_unicast_conversation_conf(p_link, num, p_cfg_table, cfg_table_size, true);
    if (!p_cfg)
    {
        p_cfg = find_unicast_conversation_conf(p_link, num, p_cfg_table, cfg_table_size, false);
        if (!p_cfg)
        {
            return NULL;
        }
    }

    return app_lea_conversation_cfg_ase(p_link, num, p_cfg);
}

#if F_APP_LEA_DONGLE_BINDING
bool app_le_audio_add_white_list(uint8_t *bd_addr, T_GAP_REMOTE_ADDR_TYPE bd_type)
{
    uint8_t i = 0;
    T_APP_LE_AUDIO_WHITE_LIST_ELEM *elem = NULL;
    bool found = false;
    bool ret = false;

    for (i = 0; i < le_audio_white_list_queue.count; i++)
    {
        elem = os_queue_peek(&le_audio_white_list_queue, i);

        if (elem != NULL && !memcmp(elem->bd_addr, bd_addr, 6))
        {
            found = true;
            ret = true;
            break;
        }
    }

    if (found == false)
    {
        T_APP_LE_AUDIO_WHITE_LIST_ELEM *op = calloc(1, sizeof(T_APP_LE_AUDIO_WHITE_LIST_ELEM));

        if (op != NULL)
        {
            op->bd_type = bd_type;
            op->loaded = false;
            memcpy(op->bd_addr, bd_addr, 6);
            os_queue_in(&le_audio_white_list_queue, op);
            ret = true;
        }
    }

    APP_PRINT_TRACE2("app_le_audio_add_white_list: ret %d addr %s", ret, TRACE_BDADDR(bd_addr));

    return ret;
}

void app_le_audio_clear_white_list(void)
{
    T_APP_LE_AUDIO_WHITE_LIST_ELEM *p_elem = NULL;

    while (le_audio_white_list_queue.count)
    {
        p_elem = os_queue_out(&le_audio_white_list_queue);
        free(p_elem);
    }
}

static bool load_white_list_before_scan(void)
{
    T_APP_LE_AUDIO_WHITE_LIST_ELEM *p_elem = NULL;
    uint8_t i = 0;
    bool loading_white_list = false;

    for (i = 0; i < le_audio_white_list_queue.count; i++)
    {
        p_elem = os_queue_peek(&le_audio_white_list_queue, i);

        if (p_elem && p_elem->loaded == false)
        {
            le_modify_white_list(GAP_WHITE_LIST_OP_ADD, p_elem->bd_addr, p_elem->bd_type);
            p_elem->loaded = true;

            loading_white_list = true;
            break;
        }
    }

    return loading_white_list;
}
#endif

static void handle_le_audio_scan(void)
{
    uint8_t data[6];
    uint8_t *pp = data;
    uint8_t cause = 0;
    bool has_white_list = false;

#if F_APP_LEA_DONGLE_BINDING
    has_white_list = le_audio_white_list_queue.count ? true : false;
    T_APP_LE_AUDIO_WHITE_LIST_ELEM *p_elem = NULL;
    uint8_t i = 0;

    if (le_audio_white_list_queue.count)
    {
        if (load_white_list_before_scan())
        {
            /* white list is loading */
            cause = 1;
            goto exit;
        }

        /* clear white list loaded flag when ready to scan */
        for (i = 0; i < le_audio_white_list_queue.count; i++)
        {
            p_elem = os_queue_peek(&le_audio_white_list_queue, i);

            if (p_elem)
            {
                p_elem->loaded = false;
            }
        }
    }
#endif

    if (app_lea_db->scan_state == LE_SCAN_ON)
    {
        cause = 2;
        goto exit;
    }

    LE_UINT16_TO_STREAM(pp, SCAN_START_OPCODE);
    LE_UINT16_TO_STREAM(pp, GATT_UUID_ASCS);
    LE_UINT8_TO_STREAM(pp, 1);                  //use active scan
    LE_UINT8_TO_STREAM(pp, has_white_list);

    usb_parse_host_cmd(data, 5);

exit:
    APP_PRINT_TRACE1("handle_le_audio_scan: cause %d", cause);
}

void start_le_audio_scan(void)
{
    if (app_usb_is_suspend() == false)
    {
        le_audio_scan_start = true;

        APP_PRINT_TRACE0("start_le_audio_scan");

        handle_le_audio_scan();
    }
}

void stop_le_audio_scan(void)
{
    uint8_t data[2];
    uint8_t *pp = data;

    le_audio_scan_start = false;

    APP_PRINT_INFO1("stop_le_audio_scan: scan_state %d", app_lea_db->scan_state);

    if (app_lea_db->scan_state == LE_SCAN_OFF)
    {
        return;
    }

    LE_UINT16_TO_STREAM(pp, SCAN_STOP_OPCODE);
    usb_parse_host_cmd(data, 2);
}

static void auto_pair_create_bond(uint8_t *bd_addr, uint8_t bd_type)
{
    uint8_t data[10];
    uint8_t *pp = data;

    APP_PRINT_TRACE2("auto_pair_create_bond: %s, bond_state %d", TRACE_BDADDR(bd_addr),
                     app_lea_db->bond_state);

    if (app_lea_db->bond_state != BT_BOND_STATE_NONE)
    {
#if F_APP_LEA_DONGLE_BINDING
        /* don't care bond state when lea dongle binding */
#else
        return;
#endif
    }

    UINT16_TO_STREAM(pp, CREATE_BOND_OPCODE);
    UINT8_TO_STREAM(pp, BONDED_TYPE);
    UINT8_TO_STREAM(pp, bd_type);
    memcpy(pp, bd_addr, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;

    T_DEV_DB *p_dev = find_device_by_addr(bd_addr);
    if (p_dev == NULL)
    {
        p_dev = calloc(1, sizeof(T_DEV_DB));
        if (!p_dev)
        {
            APP_PRINT_ERROR1("auto_pair_create_bond: alloc device fail %s", TRACE_BDADDR(bd_addr));
            return;
        }

        memcpy(p_dev->bd_addr, bd_addr, BD_ADDR_LEN);
        p_dev->bd_type = bd_type;
        ualist_add_tail(&p_dev->list, &app_lea_db->dev_list);
    }
    if (usb_parse_host_cmd(data, 10) != COMMAND_COMPLETE_SUCCESS)
    {
        start_le_audio_scan();
    }
}

void app_le_audio_handle_msg(uint32_t param)
{
    T_APP_LE_DATA *p_data = (T_APP_LE_DATA *)param;
    if (p_data == NULL)
    {
        return;
    }

    APP_PRINT_INFO4("app_le_audio_handle_msg: state %x, event %x, subevent %x, param %x",
                    app_lea_db->lea_state, p_data->event, p_data->subevent, p_data->param);
    switch (app_lea_db->lea_state)
    {
    case LEA_STATE_IDLE:
        {
            process_lea_event_in_idle(p_data->event, p_data->subevent, p_data->param);
        }
        break;
    case LEA_STATE_PAIRING:
        {
            process_lea_event_in_pairing(p_data->event, p_data->subevent, p_data->param);
        }
        break;
    case LEA_STATE_CONFIG:
        {
            process_lea_event_in_configure(p_data->event, p_data->subevent, p_data->param);
        }
        break;
    case LEA_STATE_WAIT_STREAM:
        {
            process_lea_event_in_wait_stream(p_data->event, p_data->subevent, p_data->param);
        }
        break;
    case LEA_STATE_STREAMING:
        {
            process_lea_event_in_streaming(p_data->event, p_data->subevent, p_data->param);
        }
        break;
    case LEA_STATE_RELEASING:
        {
            process_lea_event_in_releasing(p_data->event, p_data->subevent, p_data->param);
        }
        break;
    default :
        break;
    }

    free(p_data);
}

static void le_audio_start_auto_pairing(void)
{
    memset(app_lea_db->csis_hash, 0, 3);
    app_lea_db->csis_total_size = 0;

    le_csis_client_stop_scan();
    //FIX TODO we may need start a timer for auto pair
    start_le_audio_scan();
}

static void le_audio_handle_pairing_timeout(void)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);

        /* The event handler will delete the dev from list and free it. */
        bt_dev_le_gatt_disconnect(p_dev->bd_addr, DEV_MGR_APP_ID);
        //bt_dev_remove_bond(p_dev->bd_addr);
    }

    if (app_lea_db->lea_state == LEA_STATE_PAIRING)
    {
        /* Start le scan. */
        le_audio_start_auto_pairing();
    }
    else
    {
        APP_PRINT_ERROR0("le_audio_handle_pairing_timeout: host is not in pairing state");
    }
}

static void le_audio_handle_try_stream_timeout(void)
{
    if (!app_le_audio_try_stream())
    {
        app_start_timer(&timer_idx_try_stream,
                        "le_audio_try_stream_timeout",
                        le_audio_timer_id,
                        TIMER_ID_LE_AUDIO_TRY_STREAM,
                        0,
                        false,
                        LE_AUDIO_TRY_STREAM_TIMEOUT);
    }
}

static void le_audio_timeout_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_INFO2("le_audio_timeout_cback: id %x chann %x", timer_id, timer_chann);
    switch (timer_id)
    {
    case TIMER_ID_LE_AUDIO_PAIRING:
        {
            app_stop_timer(&timer_idx_pairing_timeout);
            le_audio_handle_pairing_timeout();
        }
        break;

    case TIMER_ID_LE_AUDIO_TRY_STREAM:
        {
            app_stop_timer(&timer_idx_try_stream);
            le_audio_handle_try_stream_timeout();
        }
        break;

    case TIMER_ID_LE_AUDIO_RESTART_STREAM:
        {
            app_stop_timer(&timer_idx_restart_lea_stream);

            if (app_get_usb_stream_ready_to_tx())
            {
                app_le_audio_set_downstream_active(true);
            }

            if (app_get_usb_us_state())
            {
                app_le_audio_set_upstream_active(true);
            }
        }
        break;

#if F_APP_LEA_DONGLE_BINDING
    case TIMER_ID_LE_AUDIO_CONN_TIMEOUT:
        {
            app_stop_timer(&timer_idx_check_ble_conn_timeout);

            app_le_audio_handle_conn_num_change();
        }
        break;
#endif

    default:
        break;
    }
}

static void app_le_audio_dev_state_check(void)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;
    uint8_t dev_in_list = 0;
    uint8_t dev_cnt = 0;

    /* Host is out of unicast mode. */
    if (app_lea_db->lea_state == LEA_STATE_IDLE)
    {
        app_led_set_mode(APP_LED_MODE_UNICAST_IDLE);
        return;
    }
    if (ualist_len(&app_lea_db->dev_list) != 0)
    {
        ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
        {
            dev_in_list++;
            p_dev = ualist_entry(pos, T_DEV_DB, list);
            if ((p_dev->srv_val_msk & APP_PACS_MASK) &&
                (p_dev->srv_val_msk & APP_ASCS_MASK))
            {
                dev_cnt++;
            }
        }
        if (dev_cnt)
        {
#if F_APP_LEA_DONGLE_BINDING
            if (adapter_get_scan_by_mmi())
            {
                app_led_set_mode(APP_LED_MODE_UNICAST_PAIRING);
            }
            else
#endif
            {
                app_led_set_mode(APP_LED_MODE_UNICAST_CONNECTED);
            }
        }
        else if (app_lea_db->lea_state == LEA_STATE_PAIRING)
        {
#if F_APP_LEA_DONGLE_BINDING
            if (adapter_get_scan_by_mmi())
            {
                app_led_set_mode(APP_LED_MODE_UNICAST_PAIRING);
            }
            else
            {
                app_led_set_mode(APP_LED_MODE_UNICAST_IDLE);
            }
#else
            app_led_set_mode(APP_LED_MODE_UNICAST_PAIRING);
#endif
        }
        else
        {
            app_led_set_mode(APP_LED_MODE_UNICAST_BG_SCAN);
        }
    }
    else
    {
        /* autopairing.
         * The led will set in transfer_to_next_state()
         * */
    }
}

static uint8_t app_le_audio_src_conn_num(void)
{
    uint8_t conn_dev_num = 0;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_BT_DEVICE *p_dev_rec = NULL;
    T_DEV_DB *p_dev;

    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);
        p_dev_rec = ual_find_device_by_addr(p_dev->bd_addr);
        if (p_dev_rec && p_dev_rec->ble_state == GAP_CONN_STATE_CONNECTED && p_dev_rec->auth_cmpl)
        {
            conn_dev_num++;
        }
    }

    APP_PRINT_TRACE1("app_le_audio_src_conn_num: conn_dev_num %d", conn_dev_num);

    return conn_dev_num;
}

#if F_APP_LEA_DONGLE_BINDING
static void app_le_audio_handle_conn_num_change(void)
{
    uint8_t curr_ble_conn_num = app_le_audio_src_conn_num();

    dongle_status.src_conn_num = curr_ble_conn_num;

    if (app_usb_is_suspend() == false)
    {
        if (curr_ble_conn_num == 2 || (curr_ble_conn_num == 1 && app_cfg_nv.is_bud_stereo))
        {
            app_stop_timer(&timer_idx_check_ble_conn_timeout);

            stop_le_audio_scan();
        }
        else
        {
            start_le_audio_scan();

#if 0
            /* start a timer to check ble connection to ensure ble scan is enable */
            app_start_timer(&timer_idx_check_ble_conn_timeout,
                            "check_ble_conn_timeout",
                            le_audio_timer_id,
                            TIMER_ID_LE_AUDIO_CONN_TIMEOUT,
                            0,
                            false,
                            20000);
#endif
        }
    }

    APP_PRINT_TRACE1("app_le_audio_handle_conn_num_change: curr_ble_conn_num %d", curr_ble_conn_num);
}


void app_le_audio_handle_auth_complete(uint8_t *p_addr)
{
    APP_PRINT_TRACE1("app_le_audio_handle_auth_complete: addr %s", TRACE_BDADDR(p_addr));

    uint8_t null_addr[6] = {0, 0, 0, 0, 0, 0};

    if (memcmp(p_addr, null_addr, 6) == 0)
    {
        return;
    }

    app_le_audio_handle_conn_num_change();

    adapter_set_scan_by_mmi(false);

#if F_APP_USB_GIP_SUPPORT
    if (app_gip_get_switch_mode() && app_le_audio_src_conn_num() < 2)
    {
        app_gip_handle_bt_evt(GIP_BT_EVT_CONN);
    }
#endif
#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
    uint8_t open_mic = app_get_usb_us_state();

    app_cmd_send_by_le(DONGLE_CMD_REQ_OPEN_MIC, &open_mic, 1, EARBUD_SIDE_ANY);
#endif

#if (F_APP_USB_GIP_SUPPORT == 0)
    extern void gaming_app_cis_pair_status_report_event(uint8_t connect_status);
    gaming_app_cis_pair_status_report_event(0);
#endif
}

void app_le_audio_handle_conn_state_change(uint8_t *p_addr, uint8_t addr_type, uint8_t conn_state)
{
    APP_PRINT_TRACE2("app_le_audio_handle_conn_state_change: addr %s, conn_state %d",
                     TRACE_BDADDR(p_addr),
                     conn_state);

    uint8_t null_addr[6] = {0, 0, 0, 0, 0, 0};

    if (memcmp(p_addr, null_addr, 6) == 0)
    {
        return;
    }

    if (conn_state == BT_CONN_STATE_DISCONNECTED)
    {
        app_le_audio_handle_conn_num_change();

#if F_APP_USB_GIP_SUPPORT
        if (app_gip_get_switch_mode() && app_le_audio_src_conn_num() == 0)
        {
            app_gip_handle_bt_evt(GIP_BT_EVT_DISCONN);
        }
#endif
    }
}

void app_le_audio_transmit_service_connected(void)
{
    app_gaming_sync_dongle_status();
#if F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
    app_usb_spk_vol_sync_to_headset();
#endif
}

void app_le_usb_cmd_handle_stop_scan(void)
{
    app_le_audio_enable_unicast_audio(false);
    app_stop_timer(&timer_idx_check_ble_conn_timeout);
}
#endif

static void app_le_audio_set_stream_active(uint8_t state)
{
    APP_PRINT_TRACE1("app_le_audio_set_stream_active: %d", state);

    app_lea_db->stream_active = state;
}

static bool app_le_audio_handle_pacs_update(uint8_t *p_data, uint8_t len)
{
    if ((!p_data) || (len < 32))
    {
        APP_PRINT_ERROR2("app_le_audio_handle_pacs_update: p_data %x len %x", p_data, len);
        return false;
    }

    uint8_t *p_addr = &p_data[3];
    //uint8_t addr_type = p_data[2];
    uint16_t snk_sup_context = p_data[12] | (p_data[13] << 8);
    uint16_t src_sup_context = p_data[14] | (p_data[15] << 8);
    uint16_t snk_avail_context = p_data[18] | (p_data[19] << 8);
    uint16_t src_avail_context = p_data[20] | (p_data[21] << 8);
    uint32_t snk_audio_loc = p_data[24] | (p_data[25] << 8) | (p_data[26] << 16) | (p_data[27] << 24);
    uint32_t src_audio_loc = p_data[28] | (p_data[29] << 8) | (p_data[30] << 16) | (p_data[31] << 24);
    T_BT_DEVICE *p_dev = NULL;
    T_LE_AUDIO *p_link = NULL;

    uint16_t ci_min = BLE_CONN_NORMAL_CI_DEF;
    uint16_t ci_max = BLE_CONN_NORMAL_CI_DEF;
    uint16_t ce_min = 2 * (ci_min - 1);
    uint16_t ce_max = 2 * (ci_max - 1);
    uint16_t conn_latency = BLE_CONN_SLAVE_LATENCY_DEF;
    uint16_t super_timeout = BLE_CONN_TIMEOUT_DEF;



    APP_PRINT_INFO7("app_le_audio_handle_pacs_update: p_addr %b, snk_sup %x src_sup %x snk_avail %x src_avail %x snk_loc %x src_loc %x",
                    TRACE_BDADDR(p_addr), snk_sup_context, src_sup_context, snk_avail_context, src_avail_context,
                    snk_audio_loc, src_audio_loc);
    p_dev = ual_find_device_by_addr(p_addr);
    if (!p_dev)
    {
        /* This should never happen. */
        APP_PRINT_ERROR1("app_le_audio_handle_pacs_update: No dev %b record",
                         TRACE_BDADDR(p_addr));
        return false;
    }
    p_link = ble_audio_find_by_conn_handle(p_dev->le_conn_handle);
    if (!p_link)
    {
        /* This should never happen. */
        APP_PRINT_ERROR1("app_le_audio_handle_pacs_update: No link %b",
                         p_addr);
        return false;
    }

    if (snk_avail_context & AUDIO_CONTEXT_MEDIA)
    {
        app_le_audio_set_stream_active(0);

        app_le_audio_get_stream_state();
    }
    else
    {
        bt_dev_update_conn_params(p_addr, ci_min, ci_max, conn_latency, super_timeout, ce_min, ce_max);
    }

#if F_APP_B2B_ENGAGE_IMPROVE_BY_LEA_DONGLE
    dongle_status.snk_audio_loc = snk_audio_loc;

    app_gaming_sync_dongle_status();
#endif

    return true;
}

#if F_APP_LEA_DONGLE_BINDING
#else
static bool app_lea_check_reconnect_bd_addr(uint8_t *bd_addr, uint8_t bd_type)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    if (ualist_empty(&app_lea_db->dev_list))
    {
        return true;
    }

    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);
        if (!memcmp(p_dev->bd_addr, bd_addr, BD_ADDR_LEN) &&
            (p_dev->bd_type == bd_type))
        {
            return true;
        }
    }
    return false;
}
#endif

static void app_lea_handle_scan_state_change(uint8_t *p_data, uint8_t len)
{
    if (!p_data || !len)
    {
        return;
    }

    app_lea_db->scan_state = (T_BT_DISC_STATE)p_data[0];

    extern bool key_mmi_trigger_scan;

    if (key_mmi_trigger_scan)
    {
        key_mmi_trigger_scan = false;

        if (app_lea_db->scan_state == LE_SCAN_OFF)
        {
            app_le_audio_start_auto_pair();
        }
    }
}

static void app_lea_handle_scan_result_info(uint8_t *p_data, uint8_t len)
{
    if (!p_data || !len)
    {
        return;
    }

    uint8_t bd_addr[BD_ADDR_LEN];
    T_BLE_BD_TYPE  bd_type = p_data[0];
    uint8_t gaming_mode = (uint8_t)p_data[len - 1];
    bool allow_connect = false;

    memcpy(bd_addr, p_data + 1, BD_ADDR_LEN);

#if F_APP_LEA_DONGLE_BINDING
    int8_t rssi = (int8_t)p_data[len - 2];
    T_BT_DEVICE *p_dev_rec = ual_find_alloc_device_by_addr(bd_addr, bd_type);

    if (rssi >= app_cfg_const.dongle_autopair_rssi
        && p_dev_rec != NULL
        && p_dev_rec->ble_state != GAP_CONN_STATE_CONNECTING
        && p_dev_rec->ble_state != GAP_CONN_STATE_CONNECTED)
    {
        allow_connect = true;
    }
#else
    if (app_le_check_scan_info_according_filter(p_data, len))
    {
        allow_connect = app_lea_check_reconnect_bd_addr(bd_addr, bd_type);
    }
#endif

    if (allow_connect)
    {
        le_unicast_set_gaming_mode(gaming_mode);
        stop_le_audio_scan();

#if F_APP_LEA_DONGLE_BINDING
        app_stop_timer(&timer_idx_check_ble_conn_timeout);
#if 0
        app_start_timer(&timer_idx_check_ble_conn_timeout,
                        "check_ble_conn_timeout",
                        le_audio_timer_id,
                        TIMER_ID_LE_AUDIO_CONN_TIMEOUT,
                        0,
                        false,
                        20000);
#endif
#endif

        /* Don't care the result, if creating bond failed, we re-start
        * scanning.
        * NOTE: dev is still in dev_list.
        * auto_pair_create_bond() would take care of this case.
        * */
        ble_set_default_conn_interval(BLE_CONN_NORMAL_CI_DEF);

        if (le_audio_check_cis_exist())
        {
            T_APP_LE_LINK *p_le_link = app_link_get_connected_le_link();
            uint16_t conn_interval;

            if (p_le_link != NULL)
            {
                le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, p_le_link->conn_id);

                ble_set_default_conn_interval(conn_interval);
            }
        }

        auto_pair_create_bond(bd_addr, bd_type);
#if (F_APP_LEA_DONGLE_BINDING == 0)
        if (app_lea_db->lea_state == LEA_STATE_PAIRING)
        {
            app_start_timer(&timer_idx_pairing_timeout, "le_audio_pairing_timeout",
                            le_audio_timer_id, TIMER_ID_LE_AUDIO_PAIRING, 0, false,
                            LE_AUDIO_PAIRING_TIMEOUT);
        }
#endif
    }
}


static void app_lea_handle_bond_state_change(uint8_t *p_data, uint8_t len)
{
    if (!p_data || !len)
    {
        return;
    }
    uint8_t *pp = p_data;
    uint8_t bd_addr[BD_ADDR_LEN];
    /*
    * after uapi creating bond,
    * ltk is removed,
    * new ltk received/created,
    * */
    uint8_t bond_state;
    memcpy(bd_addr, pp + 1, BD_ADDR_LEN);
    pp += 7;
    LE_STREAM_TO_UINT8(bond_state, pp);
    //FIX TODO we may need to restart scan
    app_lea_db->bond_state = (T_BT_BOND_STATE)bond_state;
    if (app_lea_db->bond_state != BT_BOND_STATE_BONDING)
    {
        if (app_lea_db->bond_state == BT_BOND_STATE_NONE)
        {
            T_DEV_DB *p_dev = find_device_by_addr(bd_addr);
            if (p_dev)
            {
                APP_PRINT_INFO1("app_lea_handle_bond_state_change: del %b", TRACE_BDADDR(bd_addr));
                ualist_del(&p_dev->list);
                free(p_dev);
            }
        }
        app_lea_db->bond_state = BT_BOND_STATE_NONE;
    }
    app_le_audio_dev_state_check();
}

static void app_lea_handle_device_state_change(uint8_t *p_data, uint8_t len)
{
    if (!p_data || !len)
    {
        return;
    }
    uint8_t *pp = p_data;
    uint8_t bd_addr[BD_ADDR_LEN];
    T_BLE_BD_TYPE  bd_type;
    T_APP_LE_DATA *p_le_data = NULL;
    T_DEV_DB *p_dev = NULL;
    uint8_t state;
    uint16_t disc_cause;
    /*
        * acl link connecting,
        * acl link connected,
        * acl link disconnected,
        * */
    //If disconnect, clear PACS and ASCS MASK

    LE_STREAM_TO_UINT8(bd_type, pp);
    memcpy(bd_addr, pp, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    LE_STREAM_TO_UINT8(state, pp);
    LE_STREAM_TO_UINT16(disc_cause, pp);

    p_dev = find_device_by_addr(bd_addr);
    if (state == 0)
    {
        if (p_dev == NULL)
        {
            APP_PRINT_WARN1("app_lea_handle_device_state_change: %b not in list disconnected",
                            TRACE_BDADDR(bd_addr));
        }
        else
        {
            p_dev->srv_val_msk &= (~(APP_PACS_MASK | APP_ASCS_MASK));
            p_dev->vcs_setting = false;
        }

        p_le_data = calloc(1, sizeof(T_APP_LE_DATA));
        if (!p_le_data)
        {
            APP_PRINT_ERROR1("app_lea_handle_device_state_change: alloc %x fail", sizeof(T_APP_LE_DATA));
            return;
        }

        p_le_data->event = LEA_EVT_LINK_STATE_CHG;
        p_le_data->subevent = LEA_LINK_DISCONNECTED;
        p_le_data->param = disc_cause;
        app_le_audio_msg_send((uint32_t) p_le_data);
    }
    else
    {
        if (p_dev == NULL)
        {
            /* just add to dev list, when ascs and pacs read done,change state */
            APP_PRINT_INFO1("app_lea_handle_device_state_change: new device %b added", TRACE_BDADDR(bd_addr));
            p_dev = calloc(1, sizeof(T_DEV_DB));
            if (!p_dev)
            {
                APP_PRINT_ERROR1("app_lea_handle_device_state_change: alloc %x fail", sizeof(T_DEV_DB));
                return;
            }
            memcpy(p_dev->bd_addr, bd_addr, BD_ADDR_LEN);
            p_dev->bd_type = bd_type;
            ualist_add_tail(&p_dev->list, &app_lea_db->dev_list);
        }
    }

    app_le_audio_dev_state_check();
}

static void app_lea_handle_device_props_update(uint8_t *p_data, uint8_t len)
{
    if (!p_data || !len)
    {
        return;
    }
    uint8_t *pp = p_data;
    uint8_t bd_addr[BD_ADDR_LEN];
    T_APP_LE_DATA *p_le_data;
    /* PACS read done
        * */
    pp += 1;
    memcpy(bd_addr, pp, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    T_DEV_DB *p_dev = find_device_by_addr(bd_addr);
    if (!p_dev)
    {
        return;
    }

    app_le_audio_handle_pacs_update(p_data, len);
    if (p_dev->srv_val_msk & APP_PACS_MASK)
    {
        return;
    }
    p_dev->srv_val_msk |= APP_PACS_MASK;
    if ((p_dev->srv_val_msk & APP_ASCS_MASK) == 0)
    {
        return;
    }

    p_le_data = calloc(1, sizeof(T_APP_LE_DATA));
    if (!p_le_data)
    {
        APP_PRINT_ERROR1("app_lea_handle_device_props_update: alloc %x fail", sizeof(T_APP_LE_DATA));
        return;
    }

    p_le_data->event = LEA_EVT_LINK_STATE_CHG;
    p_le_data->subevent = LEA_LINK_CONNECTED;
    p_le_data->param = 0;
    app_le_audio_msg_send((uint32_t) p_le_data);
    app_le_audio_dev_state_check();
}

static void app_lea_handle_config_info(uint8_t *p_data, uint8_t len)
{
    if (!p_data || !len)
    {
        return;
    }
    uint8_t *pp = p_data;
    uint8_t bd_addr[BD_ADDR_LEN];
    T_APP_LE_DATA *p_le_data;
    /* ASCS read done
        * */
    //guess ascs chars discover is done
    pp += 1;
    memcpy(bd_addr, pp, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    T_DEV_DB *p_dev = find_device_by_addr(bd_addr);
    if (!p_dev)
    {
        return;
    }
    if (p_dev->srv_val_msk & APP_ASCS_MASK)
    {
        return;
    }
    p_dev->srv_val_msk |= APP_ASCS_MASK;
    if ((p_dev->srv_val_msk & APP_PACS_MASK) == 0)
    {
        return;
    }

    p_le_data = calloc(1, sizeof(T_APP_LE_DATA));
    if (!p_le_data)
    {
        APP_PRINT_ERROR1("app_lea_handle_config_info: alloc %x fail", sizeof(T_APP_LE_DATA));
        return;
    }

    p_le_data->event = LEA_EVT_LINK_STATE_CHG;
    p_le_data->subevent = LEA_LINK_CONNECTED;
    p_le_data->param = (uint32_t)p_dev;
    app_le_audio_msg_send((uint32_t) p_le_data);
    app_le_audio_dev_state_check();
}

#if (F_APP_LEA_DONGLE_BINDING == 0)
static void app_lea_handle_csis_report(uint8_t *p_data, uint8_t len)
{
    if (!p_data || !len)
    {
        return;
    }
    uint8_t *pp = p_data;
    uint8_t bd_addr[BD_ADDR_LEN];
    /* CSIS read done */
    uint16_t        srv_uuid;
    uint8_t         hash[3];
    uint8_t         hash_zero[3] = {0};
    uint8_t         csis_size;
    T_DEV_DB *p_dev = NULL;
    T_BT_DEVICE *p_sec_dev = NULL;
    T_LE_AUDIO *p_link = NULL;
    uint8_t  conn_num = app_le_audio_src_conn_num();

    pp += 1;
    memcpy(bd_addr, pp, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    pp += 3;
    memcpy(hash, pp, 3);
    pp += 3;
    LE_STREAM_TO_UINT16(srv_uuid, pp);
    pp++;
    LE_STREAM_TO_UINT8(csis_size, pp);
    if (srv_uuid != GATT_UUID_CAS)
    {
        return;
    }

    p_sec_dev = ual_find_device_by_addr(bd_addr);
    if (!p_sec_dev)
    {
        return;
    }

    if (memcmp(app_lea_db->csis_hash, hash_zero, 3) &&
        memcmp(app_lea_db->csis_hash, hash, 3))
    {
        APP_PRINT_ERROR3("app_lea_handle_csis_report: hash %b, bd_addr %b hash %b",
                         TRACE_BINARY(3, app_lea_db->csis_hash),
                         TRACE_BDADDR(bd_addr), TRACE_BINARY(3, hash));
        return;
    }

    memcpy(app_lea_db->csis_hash, hash, 3);
    app_lea_db->csis_total_size = csis_size;

    p_dev = find_device_by_addr(p_sec_dev->pseudo_addr);
    if (p_dev == NULL)
    {
        p_dev = calloc(1, sizeof(T_DEV_DB));
        if (!p_dev)
        {
            APP_PRINT_INFO1("app_lea_handle_csis_report: alloc %d fail", sizeof(T_DEV_DB));
            return;
        }
        memcpy(p_dev->bd_addr, p_sec_dev->pseudo_addr, BD_ADDR_LEN);
        p_dev->bd_type = p_sec_dev->bd_type;
        p_link = ble_audio_find_by_conn_handle(p_sec_dev->le_conn_handle);
        if (p_link)
        {
            if (p_link->remote_serv_sup & LE_AUDIO_ASCS_CLIENT_FLAG)
            {
                p_dev->srv_val_msk |= APP_ASCS_MASK;
            }
            if (p_link->remote_serv_sup & LE_AUDIO_PACS_CLIENT_FLAG)
            {
                p_dev->srv_val_msk |= APP_PACS_MASK;
            }
        }
        ualist_add_tail(&p_dev->list, &app_lea_db->dev_list);
    }

    p_dev->srv_val_msk |= APP_CSIS_MASK;

    if (p_sec_dev->ble_state != BT_CONN_STATE_CONNECTED)
    {
        bt_dev_create_bond(p_dev->bd_addr, p_dev->bd_type);
        conn_num ++;
    }
    APP_PRINT_INFO2("app_lea_handle_csis_report: csis_size %x dev_cnt %x", csis_size,
                    ualist_len(&app_lea_db->dev_list));

    if (app_lea_db->csis_total_size != 0 &&
        conn_num < app_lea_db->csis_total_size)
    {
        le_csis_client_start_scan(app_lea_db->csis_hash);
    }
}
#endif

static void app_lea_handle_audio_play(uint8_t *p_data, uint8_t len)
{
    if (!p_data || !len)
    {
        return;
    }
    uint8_t *pp = p_data;
    T_APP_LE_DATA *p_le_data;
    /* unicast audio stream starting, streaming, releasing, idle.
        * ASE subset.
        * */
    uint8_t mode, audio_state, bap_state;
    LE_STREAM_TO_UINT8(mode, pp);
    LE_STREAM_TO_UINT8(audio_state, pp);
    if (audio_state == LE_AUDIO_IDLE_STATE)
    {
        bap_state = LEA_BAP_STATE_IDLE;
    }
    else if (audio_state == LE_AUDIO_STREAM_STARTING_STATE)
    {
        bap_state = LEA_BAP_STATE_STARTING;
    }
    else if (audio_state == LE_AUDIO_STREAMING_STATE)
    {
        bap_state = LEA_BAP_STATE_STREAMING;
    }
    else
    {
        bap_state = LEA_BAP_STATE_RELEASING;
    }

    if (mode == app_lea_db->play_mode &&
        bap_state == app_lea_db->bap_state)
    {
        return;
    }

    p_le_data = calloc(1, sizeof(T_APP_LE_DATA));
    if (p_le_data == NULL)
    {
        APP_PRINT_ERROR1("app_lea_handle_audio_play: alloc %x fail", sizeof(T_APP_LE_DATA));
        return;
    }

    p_le_data->event = LEA_EVT_BAP_STATE_CHG;
    p_le_data->subevent = 0;
    p_le_data->param = (mode << 8) | bap_state;
    app_le_audio_msg_send((uint32_t) p_le_data);
}

static void app_lea_handle_mic_mute_state(uint8_t *p_data, uint8_t len)
{
    if (!p_data || !len)
    {
        return;
    }
    uint8_t *pp = p_data;
    uint8_t bd_addr[BD_ADDR_LEN];
    uint8_t mute;

    memcpy(bd_addr, pp, BD_ADDR_LEN);
    pp += BD_ADDR_LEN;
    LE_STREAM_TO_UINT8(mute, pp);
    teams_handle_mics_msg(bd_addr, mute);
}

void app_le_audio_parse_event(uint8_t *p_data, uint8_t len)
{
    uint8_t *pp = p_data;

    if ((!p_data) || (len < 2))
    {
        APP_PRINT_ERROR1("app_le_audio_parse_event: get error, len %d", len);
        return;
    }

    uint16_t event;
    LE_STREAM_TO_UINT16(event, pp);

    APP_PRINT_INFO2("app_le_audio_parse_event: event 0x%x state %x", event, app_lea_db->lea_state);
    switch (event)
    {
    case SCAN_STATE_CHANGE_EVENT:
        {
            app_lea_handle_scan_state_change(pp, len - 2);
        }
        break;
    case SCAN_RESULT_INFO_EVENT:
        {
            app_lea_handle_scan_result_info(pp, len - 2);
        }
        break;
    case BOND_STATE_CHANGE_EVENT:
        {
            app_lea_handle_bond_state_change(pp, len - 2);
        }
        break;
    case DEVICE_STATE_CHANGE_EVENT:
        {
            app_lea_handle_device_state_change(pp, len - 2);
        }
        break;
    case DEVICE_PROPS_UPDATE_EVENT:
        {
            app_lea_handle_device_props_update(pp, len - 2);
        }
        break;
    case DEVICE_CONFIG_INFO_EVENT:
        {
            app_lea_handle_config_info(pp, len - 2);
        }
        break;

    case COORDINATE_SET_REPORT_EVENT:
        {
#if (F_APP_LEA_DONGLE_BINDING == 0)
            bool handle_csis_report = true;
            if (handle_csis_report)
            {
                app_lea_handle_csis_report(pp, len - 2);
            }
#endif
        }
        break;
    case AUDIO_PLAY_STATE_EVENT:
        {
            app_lea_handle_audio_play(pp, len - 2);
        }
        break;
    case MIC_MUTE_STATE_REPORT_EVENT:
        {
            app_lea_handle_mic_mute_state(pp, len - 2);
        }
        break;
    default:
        break;
    }
}

static bool le_audio_start_stream(uint8_t play_mode, uint8_t qos)
{
    extern bool le_unicast_is_gaming_mode();

    APP_PRINT_TRACE2("le_audio_start_stream: play_mode %d qos %d", play_mode, qos);

#if (LEA_GAMING_SPECIAL == 1)
    if (le_unicast_is_gaming_mode())
    {
        play_mode = UNICAST_CONVERSATION_PLAY_MODE;
    }
#endif

    if (play_mode == UNICAST_MEDIA_PLAY_MODE)
    {
        if (!find_first_supported_media_conf())
        {
            return false;
        }
    }
    else
    {
        if (!find_first_supported_conversation_conf())
        {
            return false;
        }
    }

    if (!le_audio_start_unicast_stream(play_mode, qos, le_unicast_is_gaming_mode()))
    {
        le_audio_clear_unicast_device_cfg(0, NULL);
        return false;
    }

    return true;

}

static bool le_audio_stop_stream()
{
    if (le_unicast_stop_stream())
    {
        ble_audio_set_unicast_mode(false, AUDIO_ROLE_SOURCE);
        le_vcs_set_bluetooth_out_state(false);
        APP_PRINT_INFO0("le_audio_stop_stream: stop succcess");
        return true;
    }

    return false;
}

static void le_audio_handle_unicast_audio_enable(bool enable, bool conn_via_scan)
{
    APP_PRINT_TRACE2("le_audio_handle_unicast_audio_enable: enable %d conn_via_scan %d", enable,
                     conn_via_scan);

    T_APP_LE_DATA *p_le_data = calloc(1, sizeof(T_APP_LE_DATA));
    if (!p_le_data)
    {
        APP_PRINT_ERROR1("app_le_audio_enable_unicast_audio: alloc %x fail", sizeof(T_APP_LE_DATA));
        return;
    }

    p_le_data->event = LEA_EVT_MODE_CHG;
    p_le_data->subevent = enable ? UNICAST_MODE_ENABLE : UNICAST_MODE_DISABLE;
    p_le_data->param = 0;

    if (enable)
    {
        p_le_data->param = conn_via_scan;
    }
    app_le_audio_msg_send((uint32_t) p_le_data);
}

void app_le_audio_start_auto_pair()
{
    APP_PRINT_INFO0("app_le_audio_start_auto_pair");
    T_APP_LE_DATA *p_le_data = calloc(1, sizeof(T_APP_LE_DATA));
    if (!p_le_data)
    {
        APP_PRINT_ERROR1("app_le_audio_start_auto_pair: alloc %x fail", sizeof(T_APP_LE_DATA));
        return;
    }

    p_le_data->event = LEA_EVT_PAIR_STATE_CHG;
    p_le_data->subevent = LEA_PAIR_ENABLE;
    p_le_data->param = 0;
    app_le_audio_msg_send((uint32_t) p_le_data);
}

void app_le_audio_enable_unicast_audio(bool enable)
{
    le_audio_handle_unicast_audio_enable(enable, true);
}

void app_le_audio_direct_enable_unicast_audio(void)
{
    le_audio_handle_unicast_audio_enable(true, false);
}

void app_le_audio_set_upstream_active(bool state)
{
    T_APP_LE_DATA *p_le_data = NULL;
    APP_PRINT_TRACE1("app_le_audio_set_upstream_active: state %d", state);

    /*FIXME: may be we should move this to event process*/
    le_tbs_handle_usb_upstream(state);
    p_le_data = calloc(1, sizeof(T_APP_LE_DATA));
    if (p_le_data == NULL)
    {
        APP_PRINT_ERROR1("app_le_audio_set_upstream_active: alloc %x fail", sizeof(T_APP_LE_DATA));
        return;
    }

    p_le_data->event = LEA_EVT_STREAM_STATE_CHG;
    p_le_data->subevent = UAC_DIR_UPSTREAM;
    p_le_data->param = state;
    app_le_audio_msg_send((uint32_t) p_le_data);
}

void app_le_audio_set_downstream_active(bool state)
{
    T_APP_LE_DATA *p_le_data = NULL;
    APP_PRINT_TRACE1("app_le_audio_set_downstream_active: state %d", state);

    p_le_data = calloc(1, sizeof(T_APP_LE_DATA));
    if (p_le_data == NULL)
    {
        APP_PRINT_ERROR1("app_le_audio_set_downstream_active: alloc %x fail", sizeof(T_APP_LE_DATA));
        return;
    }

    p_le_data->event = LEA_EVT_STREAM_STATE_CHG;
    p_le_data->subevent = UAC_DIR_DOWNSTREAM;
    p_le_data->param = state;
    app_le_audio_msg_send((uint32_t) p_le_data);
}

void app_le_audio_get_stream_state(void)
{
    bool state = false;

    APP_PRINT_TRACE0("app_le_audio_get_stream_state");

    if ((app_cfg_const.dongle_media_device) || app_line_in_plug_in())
    {
        app_le_audio_set_downstream_active(true);
    }
    else
    {
        state = app_get_usb_stream_ready_to_tx() ? true : false;
        app_le_audio_set_downstream_active(state);

        state = app_get_usb_us_state() ? true : false;
        app_le_audio_set_upstream_active(state);
    }
}

bool le_audio_ready_to_start(void)
{
    uint8_t ret = 0;
    uint8_t le_acl_cnt = 0;
    uint8_t bap_disc_all_done_cnt = 0;
    uint8_t csis_ready_result_cnt = 0;
    uint16_t snk_context = AUDIO_CONTEXT_CONVERSATIONAL | AUDIO_CONTEXT_MEDIA |
                           AUDIO_CONTEXT_UNSPECIFIED;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev = NULL;
    T_BT_DEVICE *p_bt_dev = NULL;
    T_LE_AUDIO *p_link = NULL;

    if (ualist_empty(&app_lea_db->dev_list))
    {
        ret = 1;
        goto FAIL_EMPTY;
    }

    /* Get codec cfg */
    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);
        p_bt_dev = ual_find_device_by_addr(p_dev->bd_addr);
        if (!p_bt_dev)
        {
            continue;
        }

        p_link = ble_audio_find_by_conn_id(p_bt_dev->le_conn_id);
        if (!p_link || p_link->state != BT_CONN_STATE_CONNECTED)
        {
            APP_PRINT_WARN1("le_audio_ready_to_start: addr %b link not ready", TRACE_BDADDR(p_dev->bd_addr));
            continue;
        }

        if (!p_link->csis_disc_done)
        {
            APP_PRINT_WARN0("le_audio_ready_to_start: csis not done");
            continue;
        }

        if (!(p_dev->srv_val_msk & APP_PACS_MASK) ||
            !(p_dev->srv_val_msk & APP_ASCS_MASK))
        {
            APP_PRINT_WARN2("le_audio_ready_to_start: %b srv_val_msk %x not done",
                            TRACE_BDADDR(p_dev->bd_addr), p_dev->srv_val_msk);
            continue;
        }

        /* FIXME: we only check snk-context from now */
        if ((p_link->snk_avail_context & snk_context) == 0)
        {
            ret = 3;
            goto CHECK_FAIL;
        }

        /* the csis size shall not equal to 0 if device support csis*/
        if ((p_link->supported_services & LE_AUDIO_CSIS_CLIENT_FLAG) && !p_link->csis_size)
        {
            ret = 4;
            goto CHECK_FAIL;
        }

        if (p_bt_dev->ble_conn_param.conn_mask & BLE_UPDATE_PEND)
        {
            APP_PRINT_INFO2("le_audio_ready_to_start: %b conn_mask %x",
                            TRACE_BDADDR(p_dev->bd_addr), p_bt_dev->ble_conn_param.conn_mask);
            return false;
        }

    }

    pos = NULL;
    n = NULL;

    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);
        if (p_dev == NULL)
        {
            continue;
        }

        p_bt_dev = ual_find_device_by_addr(p_dev->bd_addr);
        if (p_bt_dev == NULL)
        {
            continue;
        }

        p_link = ble_audio_find_by_conn_id(p_bt_dev->le_conn_id);
        if (p_link == NULL)
        {
            continue;
        }

        if (p_link->state == BT_CONN_STATE_CONNECTED)
        {
            le_acl_cnt++;
        }

        if (p_link->bap_disc_all_done)
        {
            bap_disc_all_done_cnt++;
        }

        if (p_link->csis_ready_result)
        {
            csis_ready_result_cnt++;
        }
    }

    APP_PRINT_INFO3("le_audio_ready_to_start: le_acl_cnt %d, bap_disc_all_done_cnt %d, csis_ready_result_cnt %d",
                    le_acl_cnt, bap_disc_all_done_cnt, csis_ready_result_cnt);

    if ((le_acl_cnt != 0 && bap_disc_all_done_cnt != 0 && csis_ready_result_cnt != 0) &&
        (le_acl_cnt == bap_disc_all_done_cnt && bap_disc_all_done_cnt == csis_ready_result_cnt))
    {
        return true;
    }
    else
    {
        return false;
    }

FAIL_EMPTY:
    APP_PRINT_ERROR1("le_audio_ready_to_start NULL device, ret %d", ret);
    return false;
CHECK_FAIL:
    APP_PRINT_ERROR7("le_audio_ready_to_start ret %d, addr %b, snk_context %x, src_context %x, conn_mask %x, services %x size %d",
                     ret, TRACE_BDADDR(p_dev->bd_addr), p_link->snk_avail_context, p_link->src_avail_context,
                     p_bt_dev->ble_conn_param.conn_mask, p_link->supported_services, p_link->csis_size);
    return false;
}

bool app_le_audio_try_stream(void)
{
    if (!le_audio_ready_to_start())
    {
        APP_PRINT_INFO0("le_audio_start_stream: not ready to start");
        return false;
    }

#if (UAC_SILENCE_DETECT_SUPPORT == 1)
    if (uac_stream_is_stream_suspend())
    {
        APP_PRINT_WARN0("app_le_audio_try_stream: suspend");
        return false;
    }
#endif

    if (app_lea_db->stream_active & UP_STREAM_ACTIVE)
    {
        if (!le_audio_start_stream(UNICAST_CONVERSATION_PLAY_MODE, ASE_TARGET_LOWER_LATENCY))
        {
            APP_PRINT_ERROR0("app_le_audio_try_stream: start conversation fail");
            return false;
        }
    }
    else if (app_lea_db->stream_active & DOWN_STREAM_ACTIVE)
    {
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
        /* get current stream state */
        if ((le_audio_get_audio_path() == LE_AUDIO_USB_PATH)
            && !uac_get_silence_state()
           )
        {
            APP_PRINT_WARN1("app_le_audio_try_stream: silence path %x", le_audio_get_audio_path());

            return false;
        }
#endif

        if (!le_audio_start_stream(UNICAST_MEDIA_PLAY_MODE, ASE_TARGET_HIGHER_RELIABILITY))
        {
            APP_PRINT_ERROR0("app_le_audio_try_stream: start media fail");
            return false;
        }
    }
    else
    {
        return false;
    }
    return true;
}

bool apply_le_audio_strategy(void)
{
    APP_PRINT_TRACE1("apply_le_audio_strategy: start stream_active %d",
                     app_lea_db->stream_active);
    if (ualist_len(&app_lea_db->dev_list) == 0)
    {
        APP_PRINT_ERROR0("apply_le_audio_strategy: start no devices bonded, start pairing");
        app_le_audio_start_auto_pair();
        return false;
    }

    extern uint8_t le_unicast_src_get_stream_state(void);
    if (le_unicast_src_get_stream_state())
    {
        APP_PRINT_INFO0("apply_le_audio_strategy: already stream");
        return false;
    }

    app_stop_timer(&timer_idx_try_stream);
    if (!app_le_audio_try_stream())
    {
        app_start_timer(&timer_idx_try_stream,
                        "le_audio_try_stream_timeout",
                        le_audio_timer_id,
                        TIMER_ID_LE_AUDIO_TRY_STREAM,
                        0,
                        false,
                        LE_AUDIO_TRY_STREAM_TIMEOUT);

        return false;
    }

    APP_PRINT_TRACE0("apply_le_audio_strategy success");

    return true;
}

static bool app_lea_check_uac_state(uint8_t dir, uint8_t state, uint8_t *stream_active)
{
    uint8_t stream_chnl = *stream_active;

    if (dir == UAC_DIR_DOWNSTREAM)
    {
        if (state == UAC_STATE_ACTIVE)
        {
            stream_chnl |= DOWN_STREAM_ACTIVE;
        }
        else
        {
            stream_chnl &= (~DOWN_STREAM_ACTIVE);
        }
    }
    else
    {
        if (state == UAC_STATE_ACTIVE)
        {
            stream_chnl |= UP_STREAM_ACTIVE;
        }
        else
        {
            stream_chnl &= (~UP_STREAM_ACTIVE);
        }
    }

    if (stream_chnl == *stream_active)
    {
        APP_PRINT_INFO3("lea_idle_handle_stream_state_change: event %x param %x stream_active %x",
                        dir, state, *stream_active);
        return false;
    }

    *stream_active =  stream_chnl;
    return true;
}

static void transfer_to_next_state(T_APP_LE_AUDIO_STATE next_state)
{
    T_APP_LE_DATA *p_le_data = NULL;
    APP_PRINT_TRACE2("transfer_to_next_state: cur_state %d, next_state %d", app_lea_db->lea_state,
                     next_state);

    if (app_lea_db->lea_state == next_state)
    {
        return;
    }

    if (app_lea_db->lea_state == LEA_STATE_PAIRING)
    {
        app_stop_timer(&timer_idx_pairing_timeout);
    }

    p_le_data = calloc(1, sizeof(T_APP_LE_DATA));
    if (p_le_data == NULL)
    {
        APP_PRINT_ERROR1("transfer_to_next_state: alloc %x fail", sizeof(T_APP_LE_DATA));
        return;
    }

    app_lea_db->lea_state = next_state;
    p_le_data->event = LEA_EVT_STATE_TRANS;
    p_le_data->subevent = 0;
    p_le_data->param = 0;
    app_le_audio_msg_send((uint32_t) p_le_data);
}

static void lea_handle_create_conn(bool conn_by_scan)
{
    if (conn_by_scan)
    {
        uint8_t hash_zero[3] = {0};
        if (memcmp(app_lea_db->csis_hash, hash_zero, 3) == 0)
        {
            start_le_audio_scan();
        }
        else
        {
            le_csis_client_start_scan(app_lea_db->csis_hash);
        }
    }
    else
    {
        T_UALIST_HEAD *pos = NULL;
        T_UALIST_HEAD *n = NULL;
        T_DEV_DB *p_dev;
        ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
        {
            p_dev = ualist_entry(pos, T_DEV_DB, list);
            bt_dev_create_bond(p_dev->bd_addr, p_dev->bd_type);
        }
    }
}

static void lea_idle_handle_state_trans(uint16_t event, uint32_t param)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        le_audio_clear_unicast_device_cfg(0, NULL);
    }
    else if (app_lea_db->bap_state != LEA_BAP_STATE_RELEASING)
    {
        le_audio_stop_stream();
    }

    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);
        bt_dev_le_gatt_disconnect(p_dev->bd_addr, DEV_MGR_APP_ID);
    }
    /* stop current scan */
    le_csis_client_stop_scan();

    app_le_audio_dev_state_check();
}

static void lea_idle_handle_mode_change(uint16_t event, uint32_t param)
{
    if (event == UNICAST_MODE_ENABLE)
    {
#if (LE_EXT_FTL_SUPPORT == 1)
        app_le_audio_load_dev();
#endif
        if (ualist_len(&app_lea_db->dev_list) != 0)
        {
            lea_handle_create_conn(param);

            APP_PRINT_TRACE2("lea_idle_handle_mode_change: mode %d, state %d",
                             app_lea_db->play_mode, app_lea_db->bap_state);
            if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
            {
                transfer_to_next_state(LEA_STATE_CONFIG);
            }
            else if (app_lea_db->bap_state == LEA_BAP_STATE_STARTING)
            {
                transfer_to_next_state(LEA_STATE_WAIT_STREAM);
            }
            else if (app_lea_db->bap_state == LEA_BAP_STATE_STREAMING)
            {
                transfer_to_next_state(LEA_STATE_STREAMING);
            }
            else if (app_lea_db->bap_state == LEA_BAP_STATE_RELEASING)
            {
                transfer_to_next_state(LEA_STATE_RELEASING);
            }
        }
        else
        {
            transfer_to_next_state(LEA_STATE_PAIRING);
            le_audio_start_auto_pairing();
        }

        app_le_audio_dev_state_check();
    }
    else
    {
        if (app_lea_db->bap_state != LEA_BAP_STATE_RELEASING &&
            app_lea_db->bap_state != LEA_BAP_STATE_IDLE)
        {
            le_audio_stop_stream();
        }
        else if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
        {
            le_audio_clear_unicast_device_cfg(0, NULL);
        }
    }
}
static void lea_idle_handle_pair_state_change(uint16_t event, uint32_t param)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    if (event == LEA_PAIR_ENABLE)
    {
        ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
        {
            p_dev = ualist_entry(pos, T_DEV_DB, list);
            if (p_dev)
            {
                bt_dev_le_gatt_disconnect(p_dev->bd_addr, DEV_MGR_APP_ID);
                //bt_dev_remove_bond(p_dev->bd_addr);
            }
        }
        le_csis_client_remove_all_groups();
        //le_bond_clear_all_keys();
        transfer_to_next_state(LEA_STATE_PAIRING);
        le_audio_start_auto_pairing();
    }
}

static void lea_idle_handle_link_state_change(uint16_t event, uint32_t param)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    /* In idle state, links should in disconnected state */
    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);
        bt_dev_le_gatt_disconnect(p_dev->bd_addr, DEV_MGR_APP_ID);
    }
}

static void lea_idle_handle_bap_state_change(uint16_t event, uint32_t param)
{
    app_lea_db->bap_state = param & 0xFF;
    app_lea_db->play_mode = (param >> 8) & 0xFF;

    if (app_lea_db->bap_state != LEA_BAP_STATE_RELEASING &&
        app_lea_db->bap_state != LEA_BAP_STATE_IDLE)
    {
        le_audio_stop_stream();
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        le_audio_clear_unicast_device_cfg(0, NULL);
    }

}

static void lea_idle_handle_stream_state_change(uint16_t event, uint32_t param)
{
    uint8_t stream_active = app_lea_db->stream_active;
    if (!app_lea_check_uac_state(event, param, &stream_active))
    {
        return;
    }

    app_le_audio_set_stream_active(stream_active);

    if (app_lea_db->bap_state != LEA_BAP_STATE_RELEASING &&
        app_lea_db->bap_state != LEA_BAP_STATE_IDLE)
    {
        le_audio_stop_stream();
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        le_audio_clear_unicast_device_cfg(0, NULL);
    }
}

static void lea_pairing_handle_state_trans(uint16_t event, uint32_t param)
{
#if F_APP_LEA_DONGLE_BINDING
    if (adapter_get_scan_by_mmi())
    {
        app_led_set_mode(APP_LED_MODE_UNICAST_PAIRING);
    }
    else
    {
        app_led_set_mode(APP_LED_MODE_UNICAST_IDLE);
    }
#else
    app_led_set_mode(APP_LED_MODE_UNICAST_PAIRING);
#endif

    app_le_audio_dev_state_check();
}

static void lea_pairing_handle_mode_change(uint16_t event, uint32_t param)
{
    if (event == UNICAST_MODE_DISABLE)
    {
        stop_le_audio_scan();
        transfer_to_next_state(LEA_STATE_IDLE);
    }
}
static void lea_pairing_handle_pair_state_change(uint16_t event, uint32_t param)
{
    if (event == LEA_PAIR_DISABLE)
    {
        stop_le_audio_scan();
        transfer_to_next_state(LEA_STATE_IDLE);
#if F_APP_LEA_DONGLE_BINDING
        if (adapter_get_scan_by_mmi())
        {
            app_led_set_mode(APP_LED_MODE_UNICAST_IDLE);
        }
#endif
    }
#if F_APP_LEA_DONGLE_BINDING
    else
    {
        start_le_audio_scan();
        if (adapter_get_scan_by_mmi())
        {
            app_led_set_mode(APP_LED_MODE_UNICAST_PAIRING);
        }
    }
#endif
}

static void lea_pairing_handle_link_state_change(uint16_t event, uint32_t param)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    /*FIXME: we should check disconect cause*/
    if (event == LEA_LINK_DISCONNECTED)
    {
        ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
        {
            p_dev = ualist_entry(pos, T_DEV_DB, list);
            if (p_dev)
            {
                bt_dev_le_gatt_disconnect(p_dev->bd_addr, DEV_MGR_APP_ID);
                //bt_dev_remove_bond(p_dev->bd_addr);
            }
        }
        le_csis_client_remove_all_groups();
        //le_bond_clear_all_keys();
        transfer_to_next_state(LEA_STATE_PAIRING);
        le_audio_start_auto_pairing();
    }
    else
    {
        if (!ualist_empty(&app_lea_db->dev_list))
        {
            transfer_to_next_state(LEA_STATE_CONFIG);
        }
    }
}

static void lea_pairing_handle_bap_state_change(uint16_t event, uint32_t param)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    app_lea_db->bap_state = param & 0xFF;
    app_lea_db->play_mode = (param >> 8) & 0xFF;

    if (app_lea_db->bap_state != LEA_BAP_STATE_RELEASING &&
        app_lea_db->bap_state != LEA_BAP_STATE_IDLE)
    {
        le_audio_stop_stream();
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        le_audio_clear_unicast_device_cfg(0, NULL);
        if (ualist_len(&app_lea_db->dev_list) != 0)
        {
            ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
            {
                p_dev = ualist_entry(pos, T_DEV_DB, list);
                bt_dev_le_gatt_disconnect(p_dev->bd_addr, DEV_MGR_APP_ID);
                //bt_dev_remove_bond(p_dev->bd_addr);
            }
            le_audio_start_auto_pairing();
        }

#if F_APP_LEA_DONGLE_BINDING
        app_le_audio_dev_state_check();
#endif
    }
}

static void lea_pairing_handle_stream_state_change(uint16_t event, uint32_t param)
{
    uint8_t stream_active = app_lea_db->stream_active;
    if (!app_lea_check_uac_state(event, param, &stream_active))
    {
        return;
    }

    app_le_audio_set_stream_active(stream_active);
}

static void lea_config_handle_state_trans(uint16_t event, uint32_t param)
{
    if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        if (apply_le_audio_strategy())
        {
            transfer_to_next_state(LEA_STATE_WAIT_STREAM);
        }
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STARTING)
    {
        transfer_to_next_state(LEA_STATE_WAIT_STREAM);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STREAMING)
    {
        transfer_to_next_state(LEA_STATE_STREAMING);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_RELEASING)
    {
        transfer_to_next_state(LEA_STATE_RELEASING);
    }
}

static void lea_config_handle_mode_change(uint16_t event, uint32_t param)
{
    if (event == UNICAST_MODE_DISABLE)
    {
        if (app_usb_is_suspend())
        {
            stop_le_audio_scan();
        }
        transfer_to_next_state(LEA_STATE_IDLE);
    }
}
static void lea_config_handle_pair_state_change(uint16_t event, uint32_t param)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    if (event == LEA_PAIR_ENABLE)
    {
        ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
        {
            p_dev = ualist_entry(pos, T_DEV_DB, list);
            if (p_dev)
            {
                bt_dev_le_gatt_disconnect(p_dev->bd_addr, DEV_MGR_APP_ID);
                //bt_dev_remove_bond(p_dev->bd_addr);
            }
        }
        le_csis_client_remove_all_groups();
        //le_bond_clear_all_keys();
        transfer_to_next_state(LEA_STATE_PAIRING);
        if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
        {
            le_audio_clear_unicast_device_cfg(0, NULL);
            le_audio_start_auto_pairing();
        }
    }

}

static void lea_config_handle_link_state_change(uint16_t event, uint32_t param)
{
    uint16_t disc_cause = (uint16_t)param;

    if (event == LEA_LINK_DISCONNECTED)
    {
        if (disc_cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
        {
            if (app_le_audio_src_conn_num() == 0)
            {
                transfer_to_next_state(LEA_STATE_IDLE);
            }
        }
        else
        {
            lea_handle_create_conn(param);
        }
    }
    else
    {
        if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
        {
            if (apply_le_audio_strategy())
            {
                transfer_to_next_state(LEA_STATE_WAIT_STREAM);
            }
        }
        else if (app_lea_db->bap_state == LEA_BAP_STATE_STARTING)
        {
            transfer_to_next_state(LEA_STATE_WAIT_STREAM);
        }
        else if (app_lea_db->bap_state == LEA_BAP_STATE_STREAMING)
        {
            transfer_to_next_state(LEA_STATE_STREAMING);
        }
        else if (app_lea_db->bap_state == LEA_BAP_STATE_RELEASING)
        {
            transfer_to_next_state(LEA_STATE_RELEASING);
        }
    }
}

static void lea_config_handle_bap_state_change(uint16_t event, uint32_t param)
{
    app_lea_db->bap_state = param & 0xFF;
    app_lea_db->play_mode = (param >> 8) & 0xFF;
    if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        if (apply_le_audio_strategy())
        {
            transfer_to_next_state(LEA_STATE_WAIT_STREAM);
        }
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STARTING)
    {
        transfer_to_next_state(LEA_STATE_WAIT_STREAM);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STREAMING)
    {
        transfer_to_next_state(LEA_STATE_STREAMING);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_RELEASING)
    {
        transfer_to_next_state(LEA_STATE_RELEASING);
    }
}

static void lea_config_handle_stream_state_change(uint16_t event, uint32_t param)
{
    uint8_t stream_active = app_lea_db->stream_active;
    if (!app_lea_check_uac_state(event, param, &(stream_active)))
    {
        return;
    }

    app_le_audio_set_stream_active(stream_active);

    if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        if (apply_le_audio_strategy())
        {
            transfer_to_next_state(LEA_STATE_WAIT_STREAM);
        }
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STARTING)
    {
        transfer_to_next_state(LEA_STATE_WAIT_STREAM);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STREAMING)
    {
        transfer_to_next_state(LEA_STATE_STREAMING);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_RELEASING)
    {
        transfer_to_next_state(LEA_STATE_RELEASING);
    }
}

static void lea_wait_stream_handle_state_trans(uint16_t event, uint32_t param)
{
    APP_PRINT_TRACE2("lea_wait_stream_handle_state_trans: stream_active 0x%x mode %d",
                     app_lea_db->stream_active, le_audio_get_unicast_play_mode());
    if ((app_lea_db->stream_active & AUDIO_STREAM_ACTIVE_MASK) == 0)
    {
        le_audio_stop_stream();
    }
    else if (app_lea_db->stream_active & UP_STREAM_ACTIVE)
    {
        if (le_audio_get_unicast_play_mode() != UNICAST_CONVERSATION_PLAY_MODE)
        {
            le_audio_stop_stream();
        }
    }
}

static void lea_wait_stream_handle_mode_change(uint16_t event, uint32_t param)
{
    if (event == UNICAST_MODE_DISABLE)
    {
        transfer_to_next_state(LEA_STATE_IDLE);
    }
}
static void lea_wait_stream_handle_pair_state_change(uint16_t event, uint32_t param)
{
    if (event == LEA_PAIR_ENABLE)
    {
        transfer_to_next_state(LEA_STATE_PAIRING);
        if (app_lea_db->bap_state != LEA_BAP_STATE_RELEASING)
        {
            le_audio_stop_stream();
        }
    }
}

static void lea_wait_stream_handle_link_state_change(uint16_t event, uint32_t param)
{
    uint16_t disc_cause = (uint16_t)param;

    if (event == LEA_LINK_DISCONNECTED)
    {
        if (disc_cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
        {
            if (app_le_audio_src_conn_num() == 0)
            {
                transfer_to_next_state(LEA_STATE_IDLE);
            }
        }
        else
        {
            lea_handle_create_conn(param);
        }
    }
}

static void lea_wait_stream_handle_bap_state_change(uint16_t event, uint32_t param)
{
    app_lea_db->bap_state = param & 0xFF;
    app_lea_db->play_mode = (param >> 8) & 0xFF;
    if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        transfer_to_next_state(LEA_STATE_CONFIG);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STREAMING)
    {
        transfer_to_next_state(LEA_STATE_STREAMING);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_RELEASING)
    {
        transfer_to_next_state(LEA_STATE_RELEASING);
    }
    else
    {
        APP_PRINT_TRACE2("lea_wait_stream_handle_bap_state_change: stream_active 0x%x mode %d",
                         app_lea_db->stream_active, le_audio_get_unicast_play_mode());
        if ((app_lea_db->stream_active & AUDIO_STREAM_ACTIVE_MASK) == 0)
        {
            le_audio_stop_stream();
        }
        else if (app_lea_db->stream_active & UP_STREAM_ACTIVE)
        {
            if (le_audio_get_unicast_play_mode() != UNICAST_CONVERSATION_PLAY_MODE)
            {
                le_audio_stop_stream();
            }
        }
    }
}

static void lea_wait_stream_handle_stream_state_change(uint16_t event, uint32_t param)
{
    uint8_t stream_active = app_lea_db->stream_active;
    if (!app_lea_check_uac_state(event, param, &(stream_active)))
    {
        return;
    }

    app_le_audio_set_stream_active(stream_active);

    if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        transfer_to_next_state(LEA_STATE_CONFIG);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STREAMING)
    {
#if (LEA_GAMING_SPECIAL == 1)
        if (le_unicast_is_gaming_mode())
        {
            if (app_lea_db->stream_active & UP_STREAM_ACTIVE)
            {
                lea_gaming_update_upstream(true);
            }
        }
#endif
        transfer_to_next_state(LEA_STATE_STREAMING);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_RELEASING)
    {
        transfer_to_next_state(LEA_STATE_RELEASING);
    }
    else
    {
        APP_PRINT_TRACE2("lea_wait_stream_handle_stream_state_change: stream_active 0x%x mode %d",
                         app_lea_db->stream_active, le_audio_get_unicast_play_mode());
        if ((app_lea_db->stream_active & AUDIO_STREAM_ACTIVE_MASK) == 0)
        {
            le_audio_stop_stream();
        }
        else if (app_lea_db->stream_active & UP_STREAM_ACTIVE)
        {
#if (LEA_GAMING_SPECIAL == 1)
            if (le_unicast_is_gaming_mode())
            {
                lea_gaming_update_upstream(true);
                return;
            }
#endif
            if (le_audio_get_unicast_play_mode() != UNICAST_CONVERSATION_PLAY_MODE)
            {
                le_audio_stop_stream();
            }
        }
        else if ((app_lea_db->stream_active & AUDIO_STREAM_ACTIVE_MASK) == DOWN_STREAM_ACTIVE)
        {
#if (LEA_GAMING_SPECIAL == 1)
            if (le_unicast_is_gaming_mode())
            {
                lea_gaming_update_upstream(false);
                return;
            }
#endif
            if (le_audio_get_unicast_play_mode() != UNICAST_MEDIA_PLAY_MODE)
            {
                le_audio_stop_stream();
            }
        }
    }
}

static void lea_streaming_handle_state_trans(uint16_t event, uint32_t param)
{
    APP_PRINT_TRACE2("lea_streaming_handle_state_trans: stream_active 0x%x mode %d",
                     app_lea_db->stream_active, le_audio_get_unicast_play_mode());
    if ((app_lea_db->stream_active & AUDIO_STREAM_ACTIVE_MASK) == 0)
    {
        le_audio_stop_stream();
    }
    else if (app_lea_db->stream_active & UP_STREAM_ACTIVE)
    {
        if (le_audio_get_unicast_play_mode() != UNICAST_CONVERSATION_PLAY_MODE)
        {
            le_audio_stop_stream();
        }
    }
}

static void lea_streaming_handle_mode_change(uint16_t event, uint32_t param)
{
    if (event == UNICAST_MODE_DISABLE)
    {
        transfer_to_next_state(LEA_STATE_IDLE);
    }
}
static void lea_streaming_handle_pair_state_change(uint16_t event, uint32_t param)
{
    if (event == LEA_PAIR_ENABLE)
    {
        transfer_to_next_state(LEA_STATE_PAIRING);
        if (app_lea_db->bap_state != LEA_BAP_STATE_RELEASING)
        {
            le_audio_stop_stream();
        }
    }
}

static void lea_streaming_handle_link_state_change(uint16_t event, uint32_t param)
{
    uint16_t disc_cause = (uint16_t)param;

    if (event == LEA_LINK_DISCONNECTED)
    {
        if (disc_cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
        {
            if (app_le_audio_src_conn_num() == 0)
            {
                transfer_to_next_state(LEA_STATE_IDLE);
            }
        }
        else
        {
            lea_handle_create_conn(param);
        }
    }
}

static void lea_streaming_handle_bap_state_change(uint16_t event, uint32_t param)
{
    app_lea_db->bap_state = param & 0xFF;
    app_lea_db->play_mode = (param >> 8) & 0xFF;
    if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        transfer_to_next_state(LEA_STATE_CONFIG);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STARTING)
    {
        transfer_to_next_state(LEA_STATE_WAIT_STREAM);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_RELEASING)
    {
        transfer_to_next_state(LEA_STATE_RELEASING);
    }
}

static void lea_streaming_handle_stream_state_change(uint16_t event, uint32_t param)
{
    uint8_t stream_active = app_lea_db->stream_active;
    if (!app_lea_check_uac_state(event, param, &(stream_active)))
    {
        return;
    }

    app_le_audio_set_stream_active(stream_active);

    if ((app_lea_db->stream_active & AUDIO_STREAM_ACTIVE_MASK) == 0)
    {
        le_audio_stop_stream();
    }
    else if (app_lea_db->stream_active & UP_STREAM_ACTIVE)
    {
#if (LEA_GAMING_SPECIAL == 1)
        if (le_unicast_is_gaming_mode())
        {
            lea_gaming_update_upstream(true);
            return;
        }
#endif
        if (le_audio_get_unicast_play_mode() != UNICAST_CONVERSATION_PLAY_MODE)
        {
            le_audio_stop_stream();
        }
    }
    else if ((app_lea_db->stream_active & AUDIO_STREAM_ACTIVE_MASK) == DOWN_STREAM_ACTIVE)
    {
#if (LEA_GAMING_SPECIAL == 1)
        if (le_unicast_is_gaming_mode())
        {
            lea_gaming_update_upstream(false);
            return;
        }
#endif
        if (le_audio_get_unicast_play_mode() != UNICAST_MEDIA_PLAY_MODE)
        {
            le_audio_stop_stream();
        }
    }
}

static void lea_releasing_handle_state_trans(uint16_t event, uint32_t param)
{
    APP_PRINT_INFO2("lea_releasing_handle_state_trans: curr state %x, new state %x",
                    app_lea_db->lea_state, param);
}

static void lea_releasing_handle_mode_change(uint16_t event, uint32_t param)
{
    if (event == UNICAST_MODE_DISABLE)
    {
        transfer_to_next_state(LEA_STATE_IDLE);
    }
}
static void lea_releasing_handle_pair_state_change(uint16_t event, uint32_t param)
{
    if (event == LEA_PAIR_ENABLE)
    {
        transfer_to_next_state(LEA_STATE_PAIRING);
        if (app_lea_db->bap_state != LEA_BAP_STATE_RELEASING)
        {
            le_audio_stop_stream();
        }
    }
}

static void lea_releasing_handle_link_state_change(uint16_t event, uint32_t param)
{
    uint16_t disc_cause = (uint16_t)param;

    if (event == LEA_LINK_DISCONNECTED)
    {
        if (disc_cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
        {
            if (app_le_audio_src_conn_num() == 0)
            {
                transfer_to_next_state(LEA_STATE_IDLE);
            }
        }
        else
        {
            lea_handle_create_conn(param);
        }
    }
}

static void lea_releasing_handle_bap_state_change(uint16_t event, uint32_t param)
{
    app_lea_db->bap_state = param & 0xFF;
    app_lea_db->play_mode = (param >> 8) & 0xFF;
    if (app_lea_db->bap_state == LEA_BAP_STATE_IDLE)
    {
        transfer_to_next_state(LEA_STATE_CONFIG);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STARTING)
    {
        transfer_to_next_state(LEA_STATE_WAIT_STREAM);
    }
    else if (app_lea_db->bap_state == LEA_BAP_STATE_STREAMING)
    {
        transfer_to_next_state(LEA_STATE_STREAMING);
    }
}

static void lea_releasing_handle_stream_state_change(uint16_t event, uint32_t param)
{
    uint8_t stream_active = app_lea_db->stream_active;
    if (!app_lea_check_uac_state(event, param, &(stream_active)))
    {
        return;
    }

    app_le_audio_set_stream_active(stream_active);

}

static void process_lea_event_in_idle(uint16_t event, uint16_t subevent, uint32_t param)
{
    switch (event)
    {
    case LEA_EVT_STATE_TRANS:
        {
            lea_idle_handle_state_trans(subevent, param);
        }
        break;

    case LEA_EVT_MODE_CHG:
        {
            lea_idle_handle_mode_change(subevent, param);
        }
        break;

    case LEA_EVT_PAIR_STATE_CHG:
        {
            lea_idle_handle_pair_state_change(subevent, param);
        }
        break;

    case LEA_EVT_LINK_STATE_CHG:
        {
            lea_idle_handle_link_state_change(subevent, param);
        }
        break;

    case LEA_EVT_BAP_STATE_CHG:
        {
            lea_idle_handle_bap_state_change(subevent, param);
        }
        break;

    case LEA_EVT_STREAM_STATE_CHG:
        {
            lea_idle_handle_stream_state_change(subevent, param);
        }
        break;

    default:
        break;
    }
}

static void process_lea_event_in_pairing(uint16_t event, uint16_t subevent, uint32_t param)
{
    switch (event)
    {
    case LEA_EVT_STATE_TRANS:
        {
            lea_pairing_handle_state_trans(subevent, param);
        }
        break;
    case LEA_EVT_MODE_CHG:
        {
            lea_pairing_handle_mode_change(subevent, param);
        }
        break;

    case LEA_EVT_PAIR_STATE_CHG:
        {
            lea_pairing_handle_pair_state_change(subevent, param);
        }
        break;

    case LEA_EVT_LINK_STATE_CHG:
        {
            lea_pairing_handle_link_state_change(subevent, param);
        }
        break;

    case LEA_EVT_BAP_STATE_CHG:
        {
            lea_pairing_handle_bap_state_change(subevent, param);
        }
        break;

    case LEA_EVT_STREAM_STATE_CHG:
        {
            lea_pairing_handle_stream_state_change(subevent, param);
        }
        break;

    default:
        break;
    }
}

static void process_lea_event_in_configure(uint16_t event, uint16_t subevent, uint32_t param)
{
    switch (event)
    {
    case LEA_EVT_STATE_TRANS:
        {
            lea_config_handle_state_trans(subevent, param);
        }
        break;

    case LEA_EVT_MODE_CHG:
        {
            lea_config_handle_mode_change(subevent, param);
        }
        break;

    case LEA_EVT_PAIR_STATE_CHG:
        {
            lea_config_handle_pair_state_change(subevent, param);
        }
        break;

    case LEA_EVT_LINK_STATE_CHG:
        {
            lea_config_handle_link_state_change(subevent, param);
        }
        break;

    case LEA_EVT_BAP_STATE_CHG:
        {
            lea_config_handle_bap_state_change(subevent, param);
        }
        break;

    case LEA_EVT_STREAM_STATE_CHG:
        {
            lea_config_handle_stream_state_change(subevent, param);
        }
        break;
    default:
        break;
    }
}

static void process_lea_event_in_wait_stream(uint16_t event, uint16_t subevent, uint32_t param)
{
    switch (event)
    {
    case LEA_EVT_STATE_TRANS:
        {
            lea_wait_stream_handle_state_trans(subevent, param);
        }
        break;

    case LEA_EVT_MODE_CHG:
        {
            lea_wait_stream_handle_mode_change(subevent, param);
        }
        break;

    case LEA_EVT_PAIR_STATE_CHG:
        {
            lea_wait_stream_handle_pair_state_change(subevent, param);
        }
        break;

    case LEA_EVT_LINK_STATE_CHG:
        {
            lea_wait_stream_handle_link_state_change(subevent, param);
        }
        break;

    case LEA_EVT_BAP_STATE_CHG:
        {
            lea_wait_stream_handle_bap_state_change(subevent, param);
        }
        break;

    case LEA_EVT_STREAM_STATE_CHG:
        {
            lea_wait_stream_handle_stream_state_change(subevent, param);
        }
        break;
    default:
        break;
    }
}

static void process_lea_event_in_streaming(uint16_t event, uint16_t subevent, uint32_t param)
{
    switch (event)
    {
    case LEA_EVT_STATE_TRANS:
        {
            lea_streaming_handle_state_trans(subevent, param);
        }
        break;

    case LEA_EVT_MODE_CHG:
        {
            lea_streaming_handle_mode_change(subevent, param);
        }
        break;

    case LEA_EVT_PAIR_STATE_CHG:
        {
            lea_streaming_handle_pair_state_change(subevent, param);
        }
        break;

    case LEA_EVT_LINK_STATE_CHG:
        {
            lea_streaming_handle_link_state_change(subevent, param);
        }
        break;

    case LEA_EVT_BAP_STATE_CHG:
        {
            lea_streaming_handle_bap_state_change(subevent, param);
        }
        break;

    case LEA_EVT_STREAM_STATE_CHG:
        {
            lea_streaming_handle_stream_state_change(subevent, param);
        }
        break;

    default:
        break;
    }
}

static void process_lea_event_in_releasing(uint16_t event, uint16_t subevent, uint32_t param)
{
    switch (event)
    {
    case LEA_EVT_STATE_TRANS:
        {
            lea_releasing_handle_state_trans(subevent, param);
        }
        break;

    case LEA_EVT_MODE_CHG:
        {
            lea_releasing_handle_mode_change(subevent, param);
        }
        break;

    case LEA_EVT_PAIR_STATE_CHG:
        {
            lea_releasing_handle_pair_state_change(subevent, param);
        }
        break;

    case LEA_EVT_LINK_STATE_CHG:
        {
            lea_releasing_handle_link_state_change(subevent, param);
        }
        break;

    case LEA_EVT_BAP_STATE_CHG:
        {
            lea_releasing_handle_bap_state_change(subevent, param);
        }
        break;

    case LEA_EVT_STREAM_STATE_CHG:
        {
            lea_releasing_handle_stream_state_change(subevent, param);
        }
        break;
    default:
        break;
    }

}

bool app_le_audio_is_in_idle(void)
{
    if (app_lea_db->lea_state == LEA_STATE_IDLE)
    {
        return true;
    }
    else
    {
        return false;
    }
}


#if F_APP_LEA_DONGLE_BINDING
bool app_le_audio_is_in_pairing(void)
{
    if (app_lea_db->lea_state == LEA_STATE_PAIRING)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void app_le_audio_cancel_pairing(void)
{
    APP_PRINT_INFO0("app_le_audio_cancel_pairing");

    T_APP_LE_DATA *p_data = malloc(sizeof(T_APP_LE_DATA));
    if (!p_data)
    {
        APP_PRINT_ERROR1("app_le_audio_cancel_pairing: alloc %x fail", sizeof(T_APP_LE_DATA));
        return;
    }

    p_data->event = LEA_EVT_PAIR_STATE_CHG;
    p_data->subevent = LEA_PAIR_DISABLE;
    p_data->param = 0;
    app_le_audio_msg_send((uint32_t) p_data);
}
#endif

static bool app_le_audio_set_cis_vol(uint8_t *p_addr, uint8_t volume_setting, uint8_t mute)
{
    if (p_addr == NULL)
    {
        return false;
    }

    T_VOLUME_STATE data = {0x0};
    T_BT_DEVICE *p_bt_dev = NULL;
    T_LE_AUDIO *p_link = NULL;
    T_DEV_DB *p_dev = NULL;

    p_dev = find_device_by_addr(p_addr);
    if (!p_dev)
    {
        APP_PRINT_ERROR1("app_le_audio_set_cis_vol: find %b failed", TRACE_BDADDR(p_addr));
        return false;
    }

    p_bt_dev = ual_find_device_by_addr(p_addr);
    if (p_bt_dev == NULL)
    {
        APP_PRINT_ERROR1("app_le_audio_set_cis_vol: %b can't find p_dev", TRACE_BDADDR(p_addr));
        return false;
    }
    p_link = ble_audio_find_by_conn_handle(p_bt_dev->le_conn_handle);
    if (p_link == NULL)
    {
        APP_PRINT_ERROR1("app_le_audio_set_cis_vol: %b can't find p_link", TRACE_BDADDR(p_addr));
        return false;
    }

    if (!vcs_get_volume_state(p_link->conn_handle, &data))
    {
        APP_PRINT_ERROR1("app_le_audio_set_cis_vol: %b can't find vcs data", TRACE_BDADDR(p_addr));
        return false;
    }

    bool mute_need_sync = data.mute == mute ? false : true;
    bool vol_need_sync = data.volume_setting == volume_setting ? false : true;
    APP_PRINT_INFO5("app_le_audio_set_cis_vol: bd_addr %b vol %x mute %x remote_mute %x remote_vol %x",
                    TRACE_BDADDR(p_dev->bd_addr), volume_setting, mute,
                    data.mute,
                    data.volume_setting);
    if (p_dev->vcs_setting)
    {
        APP_PRINT_WARN0("app_le_audio_set_cis_vol: already setting");
        return true;
    }

    if (mute_need_sync)
    {
        if (!le_audio_set_remote_mute(p_dev->bd_addr, mute))
        {
            APP_PRINT_WARN2("app_le_audio_set_cis_vol: set bd_addr %b mute %x failed",
                            TRACE_BDADDR(p_dev->bd_addr), mute);
            return false;
        }
        p_dev->vcs_setting = true;
    }
    else if (vol_need_sync)
    {
        if (!le_audio_set_remote_volume(p_dev->bd_addr, volume_setting))
        {
            APP_PRINT_WARN2("app_le_audio_set_cis_vol: set bd_addr %b vol %x failed",
                            TRACE_BDADDR(p_dev->bd_addr), volume_setting);
            return false;
        }
        p_dev->vcs_setting = true;
    }
    return true;
}

bool app_le_audio_sync_host_vol_to_remote(uint16_t conn_handle)
{
#if F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME
    /* don't send vol to headset */
    return false;
#else
    extern uint8_t get_dongle_host_vol(void);
    extern uint8_t get_dongle_host_mute(void);

    T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(conn_handle);
    if (!p_dev_rec)
    {
        return false;
    }

    /* vcs volume min 0 max 255, pc vol min 0 max 127*/
    uint8_t vol = get_dongle_host_vol() * 2;
    uint8_t mute = get_dongle_host_mute();

    return app_le_audio_set_cis_vol(p_dev_rec->pseudo_addr, vol, mute);
#endif
}

void app_le_audio_handle_vol_notify(uint8_t *p_addr, uint8_t volume_setting, uint8_t mute)
{
    T_DEV_DB *p_dev = NULL;
    if (app_cfg_const.dongle_media_device)
    {
        return;
    }

    p_dev = find_device_by_addr(p_addr);
    if (!p_dev)
    {
        APP_PRINT_ERROR1("app_le_audio_handle_vol_notify: find %b failed", TRACE_BDADDR(p_addr));
        return;
    }
    bool mute_flag = mute ? true : false;

    APP_PRINT_INFO3("app_le_audio_handle_vol_notify: bd_addr %b vol %x mute %x",
                    TRACE_BDADDR(p_addr), volume_setting, mute);
    if (p_dev->vcs_setting == false)
    {
        app_dongle_handle_a2u_set_vol(volume_setting >> 1, mute_flag);
    }
    else
    {
        p_dev->vcs_setting = false;
        app_le_audio_set_cis_vol(p_addr, get_dongle_host_vol() * 2, get_dongle_host_mute());
    }
}

void app_le_audio_set_mic_mute(uint8_t mic_mute)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    if (ualist_empty(&app_lea_db->dev_list))
    {
        APP_PRINT_ERROR0("app_le_audio_set_mic_mute: list is empty");
        return;
    }

    APP_PRINT_INFO1("app_le_audio_set_mic_mute: mute %x", mic_mute);
    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);

        if (p_dev == NULL)
        {
            APP_PRINT_WARN0("app_le_audio_set_mic_mute: device null");
            continue;
        }
        le_mic_set_remote_mute(p_dev->bd_addr, mic_mute);
    }
}

void app_le_audio_set_remote_vol(uint8_t vol, uint8_t mute)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_dev;

    extern uint8_t app_get_cur_bt_mode(void);
    if (app_get_cur_bt_mode() == 1
#if F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME
        || app_get_cur_bt_mode() == 2 //CIS
#endif
       )
    {
        APP_PRINT_INFO2("app_le_audio_set_remote_vol: bis set vol %x mute %x", vol, mute);
        if (mute)
        {
            le_vcs_usb_set_out_volume(mute);
        }
        else
        {
            le_vcs_usb_set_out_volume(vol + 2);
        }
        return;
    }

    if (ualist_empty(&app_lea_db->dev_list))
    {
        APP_PRINT_ERROR0("app_le_audio_set_remote_vol: list is empty");
        return;
    }

    APP_PRINT_INFO2("app_le_audio_set_remote_vol: vol %x mute %x", vol, mute);
    ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
    {
        p_dev = ualist_entry(pos, T_DEV_DB, list);

        if (p_dev == NULL)
        {
            APP_PRINT_WARN0("app_le_audio_set_remote_vol: device null");
            continue;
        }
        app_le_audio_set_cis_vol(p_dev->bd_addr, vol * 2, mute);
    }
}

#if (LE_EXT_FTL_SUPPORT == 1)
void app_le_audio_load_dev(void)
{
    /* get dev from bond info */
    T_LE_KEY_ENTRY *p_entry = NULL;
    T_LEA_FTL_DEV   ftl_dev;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_DEV_DB *p_new_dev = NULL;
    T_DEV_DB *p_dev = NULL;
    uint8_t csis_hash[3] = {0x0};
    uint8_t csis_size = 0;
    uint8_t ret = 0;
    uint32_t offset = 0;

    /* clear dev_list loaded before */
    if (ualist_len(&app_lea_db->dev_list) != 0)
    {
        ualist_for_each_safe(pos, n, &app_lea_db->dev_list)
        {
            p_dev = ualist_entry(pos, T_DEV_DB, list);
            if (p_dev != NULL)
            {
                ualist_del(&p_dev->list);
                free(p_dev);
                p_dev = NULL;
            }
        }
    }

    p_entry = le_get_high_priority_bond();
    if (!p_entry)
    {
        APP_PRINT_WARN0("app_le_audio_load_dev: no bond info");
        return;
    }

    ret = lea_ext_find_dev_by_addr(p_entry->remote_bd.addr, p_entry->remote_bd.remote_bd_type,
                                   &ftl_dev, &offset);
    if (!ret)
    {
        APP_PRINT_WARN1("app_le_audio_load_dev: bd_addr %b no found",
                        TRACE_BDADDR(p_entry->remote_bd.addr));
        return;
    }

    if (!ftl_dev.csis_support)
    {
        APP_PRINT_INFO1("app_le_audio_load_dev: bd_addr %b no csis, add to list",
                        TRACE_BDADDR(ftl_dev.bd_addr));
        p_dev = find_device_by_addr(ftl_dev.bd_addr);
        if (p_dev)
        {
            APP_PRINT_INFO1("app_le_audio_load_dev: bd_addr %b already in list", TRACE_BDADDR(ftl_dev.bd_addr));
            return;
        }

        p_new_dev = calloc(1, sizeof(T_DEV_DB));
        if (!p_new_dev)
        {
            APP_PRINT_ERROR0("app_le_audio_load_dev: alloc fail");
            return;
        }

        memcpy(p_new_dev->bd_addr, ftl_dev.bd_addr, 6);
        p_new_dev->bd_type = ftl_dev.bd_type;
        ualist_add_tail(&p_new_dev->list, &app_lea_db->dev_list);
        return;
    }

    memcpy(csis_hash, ftl_dev.csis_hash, 3);
    csis_size = ftl_dev.csis_size;

    for (uint8_t i = 0; i < csis_size; i++)
    {
        ret = lea_ext_find_dev_by_hash(csis_hash, i, &ftl_dev);
        if (!ret)
        {
            APP_PRINT_WARN2("app_le_audio_load_dev: hash %b not found %i", TRACE_BINARY(3, csis_hash), i);
            return;
        }
        p_dev = find_device_by_addr(ftl_dev.bd_addr);
        if (p_dev)
        {
            APP_PRINT_INFO1("app_le_audio_load_dev: bd_addr %b alread in list", TRACE_BDADDR(ftl_dev.bd_addr));
            continue;
        }
        p_new_dev = calloc(1, sizeof(T_DEV_DB));
        if (!p_new_dev)
        {
            APP_PRINT_ERROR0("app_le_audio_load_dev: alloc fail");
            return;
        }
        memcpy(p_new_dev->bd_addr, ftl_dev.bd_addr, 6);
        p_new_dev->bd_type = ftl_dev.bd_type;
        ualist_add_tail(&p_new_dev->list, &app_lea_db->dev_list);
    }
}
#endif

static void app_lea_set_peer_addr(uint8_t *bd_addr, uint8_t bd_type)
{
    if (!bd_addr)
    {
        return;
    }

    uint8_t bd_addr_invalid[BD_ADDR_LEN] = {0x0};
    uint8_t i = 0;

    if (!memcmp(app_le_filter.bd_addr[0], bd_addr_invalid, BD_ADDR_LEN))
    {
        app_le_filter.bd_type[0] = bd_type;
        for (i = 0; i < 6; i++)
        {
            app_le_filter.bd_addr[0][5 - i] = bd_addr[i];
        }
    }
    else if (!memcmp(app_le_filter.bd_addr[1], bd_addr_invalid, BD_ADDR_LEN))
    {
        app_le_filter.bd_type[1] = bd_type;
        for (i = 0; i < 6; i++)
        {
            app_le_filter.bd_addr[1][5 - i] = bd_addr[i];
        }
    }
    else
    {
        app_le_filter.bd_type[0] = app_le_filter.bd_type[1];
        memcpy(app_le_filter.bd_addr[0],  app_le_filter.bd_addr[1], BD_ADDR_LEN);
        app_le_filter.bd_type[1] = bd_type;
        for (i = 0; i < 6; i++)
        {
            app_le_filter.bd_addr[1][5 - i] = bd_addr[i];
        }
    }

    for (i = 0; i < 2; i++)
    {
        APP_PRINT_INFO3("app_lea_filter: %i type %x, bd_addr %b", i,
                        app_le_filter.bd_type[i], TRACE_BDADDR(app_le_filter.bd_addr[i]));
    }
}


bool app_le_audio_set_pair_filter(uint8_t enable, uint8_t filter_type, uint8_t *data, uint8_t len)
{
    if (!data)
    {
        return false;
    }

    APP_PRINT_INFO3("app_le_audio_set_pair_filter: enable %x, type %x, %b", enable, filter_type,
                    TRACE_BINARY(len, data));

    if (enable == false)
    {
        if (filter_type == 0)
        {
            app_le_filter.bd_addr_enable = false;
            memset(app_le_filter.bd_addr[0], 0, BD_ADDR_LEN);
            app_le_filter.bd_type[0] = 0;
            memset(app_le_filter.bd_addr[1], 0, BD_ADDR_LEN);
            app_le_filter.bd_type[1] = 0;
        }
        else
        {
            app_le_filter.name_enable = false;
            memset(app_le_filter.name, 0, LE_LOCAL_NAME_LEN_MAX);
        }
        return true;
    }

    if (filter_type == 0)
    {
        app_le_filter.bd_addr_enable = true;
        app_lea_set_peer_addr(data + 1, data[0]);
    }
    else
    {
        app_le_filter.name_enable = true;
        app_le_filter.name_len = (strlen((char *)data) < LE_LOCAL_NAME_LEN_MAX) ? strlen((
                                     char *)data) : LE_LOCAL_NAME_LEN_MAX;
        memcpy(app_le_filter.name, data, app_le_filter.name_len);
    }
    return true;
}

#if (F_APP_LEA_DONGLE_BINDING == 0)
static bool app_le_check_scan_info_according_filter(uint8_t *data, uint8_t len)
{
    uint8_t *pp = data;
    uint8_t *bd_addr = NULL;
    int8_t   rssi;
    uint8_t *name = NULL;
    uint8_t  name_len;

    pp++;
    bd_addr = pp;
    pp += BD_ADDR_LEN;
    pp ++;  //connect_mode
    pp ++;  //adv_sid
    LE_STREAM_TO_UINT8(name_len, pp);
    name = pp;
    pp += name_len;
    rssi = (int8_t)pp[0];

    if (app_le_filter.bd_addr_enable)
    {
        if (memcmp(bd_addr, app_le_filter.bd_addr[0], BD_ADDR_LEN) &&
            memcmp(bd_addr, app_le_filter.bd_addr[1], BD_ADDR_LEN))
        {
            APP_PRINT_INFO3("according_filter: return false, filter_bd_addr %b %b, adv bd_addr %b",
                            TRACE_BDADDR(app_le_filter.bd_addr[0]),
                            TRACE_BDADDR(app_le_filter.bd_addr[1]),
                            TRACE_BDADDR(bd_addr));
            return false;
        }
    }

    if (app_le_filter.name_enable)
    {
        if (memcmp(name, app_le_filter.name, app_le_filter.name_len))
        {
            APP_PRINT_INFO2("according_filter: return false filter_name %b, adv_name %b",
                            TRACE_BINARY(app_le_filter.name_len, app_le_filter.name),
                            TRACE_BINARY(app_le_filter.name_len, name));
            return false;
        }
    }

    /*when bd_addr filter or name filter enable, disenable rssi filter*/
    if ((app_le_filter.rssi_enable) && (!app_le_filter.bd_addr_enable) && (!app_le_filter.name_enable))
    {
        if (rssi < app_le_filter.rssi)
        {
            APP_PRINT_INFO3("according_filter: return false, filter rssi %x adv_rssi %x %b",
                            app_le_filter.rssi, rssi, TRACE_BDADDR(bd_addr));
            return false;
        }
    }

    APP_PRINT_INFO1("according_filter: return true, connect %b", TRACE_BDADDR(bd_addr));
    return true;
}
#endif

void app_le_filter_init(void)
{
    uint8_t bd_addr_invalid[BD_ADDR_LEN] = {0x0};
    app_le_filter.rssi_enable = true;
    app_le_filter.rssi = app_cfg_const.dongle_autopair_rssi;
    app_le_filter.name_enable = false;
    memset(app_le_filter.name, 0, LE_LOCAL_NAME_LEN_MAX);

    app_le_filter.bd_addr_enable = false;
    for (uint8_t i = 0; i < 2; i++)
    {
        memset(app_le_filter.bd_addr[i], 0, BD_ADDR_LEN);
    }

    if (memcmp(app_cfg_const.dongle_peer_addr[0], bd_addr_invalid, BD_ADDR_LEN))
    {
        app_le_filter.bd_addr_enable = true;
        memcpy(app_le_filter.bd_addr[0], app_cfg_const.dongle_peer_addr[0], BD_ADDR_LEN);
    }

    if (memcmp(app_cfg_const.dongle_peer_addr[1], bd_addr_invalid, BD_ADDR_LEN))
    {
        app_le_filter.bd_addr_enable = true;
        memcpy(app_le_filter.bd_addr[1], app_cfg_const.dongle_peer_addr[1], BD_ADDR_LEN);
    }
}

bool app_le_audio_scan_is_on(void)
{
    return (app_lea_db->scan_state == LE_SCAN_ON) ? true : false;
}

void app_le_audio_gap_cback(uint8_t cb_type, void *p_cb_data)
{
    APP_PRINT_TRACE1("app_le_audio_gap_cback: 0x%02x", cb_type);

    switch (cb_type)
    {
    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        {
            APP_PRINT_TRACE0("app_le_audio_gap_cback: GAP_MSG_LE_MODIFY_WHITE_LIST");

            if (le_audio_scan_start)
            {
                handle_le_audio_scan();
            }
        }
        break;

    case GAP_MSG_LE_CONN_UPDATE_IND:
        {
            APP_PRINT_TRACE0("app_le_audio_gap_cback: LEA gaming dongle would not handle GAP_MSG_LE_CONN_UPDATE_IND!");
        }
        break;

#if F_APP_GAMING_LE_FIX_CHANNEL_SUPPORT
    case GAP_MSG_LE_FIXED_CHANN_DATA_IND:
        {
            T_LE_CB_DATA *p_le_cb_data = p_cb_data;
            T_LE_FIXED_CHANN_DATA_IND *data_ind = p_le_cb_data->p_le_fixed_chann_data_ind;

            app_gaming_sync_disassemble_data(NULL, data_ind->p_data, data_ind->value_len);
        }
        break;
#endif

    default:
        break;
    }
}

bool app_le_audio_restarting(void)
{
    return timer_idx_restart_lea_stream ? true : false;
}

void app_le_audio_restart_stream(void)
{
    bool restart_stream = false;

    if (app_lea_db->stream_active & DOWN_STREAM_ACTIVE)
    {
        app_le_audio_set_downstream_active(false);
        restart_stream = true;
    }

    if (app_lea_db->stream_active & UP_STREAM_ACTIVE)
    {
        app_le_audio_set_upstream_active(false);
        restart_stream = true;
    }

    if (restart_stream)
    {
        app_start_timer(&timer_idx_restart_lea_stream, "restart_lea_stream",
                        le_audio_timer_id, TIMER_ID_LE_AUDIO_RESTART_STREAM,
                        0, false, USB_SEND_SILENCE_TIME + 500);
    }
}

#if F_APP_LEA_DONGLE_BINDING
void app_le_audio_load_white_list(void)
{
    uint8_t null_addr1[6] = {0};
    uint8_t null_addr2[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    APP_PRINT_TRACE2("app_le_audio_load_white_list: left %s right %s",
                     TRACE_BDADDR(app_cfg_nv.left_bud_addr), TRACE_BDADDR(app_cfg_nv.right_bud_addr));

    if ((memcmp(app_cfg_nv.left_bud_addr, null_addr1, 6) &&
         memcmp(app_cfg_nv.left_bud_addr, null_addr2, 6)) &&
        (memcmp(app_cfg_nv.right_bud_addr, null_addr1, 6) &&
         memcmp(app_cfg_nv.right_bud_addr, null_addr2, 6)))
    {
        /* both left & right addr are valid */
        app_le_audio_add_white_list(app_cfg_nv.left_bud_addr, GAP_REMOTE_ADDR_LE_RANDOM);
        app_le_audio_add_white_list(app_cfg_nv.right_bud_addr, GAP_REMOTE_ADDR_LE_RANDOM);
    }
}

void app_le_audio_clear_bud_record(void)
{
    memset(app_cfg_nv.left_bud_addr,  0, 6);
    memset(app_cfg_nv.right_bud_addr, 0, 6);

    app_cfg_store();
}
#endif

void app_le_audio_init()
{
    app_lea_db = calloc(1, sizeof(T_APP_LE_AUDIO));
    if (app_lea_db == NULL)
    {
        APP_PRINT_ERROR0("app_le_audio_init: alloc fail");
        return;
    }

#if TARGET_RTL8773DO
    APP_PRINT_INFO1("app_cfg_const.dongle_media_device 0x%x", app_cfg_const.dongle_media_device);
    if (app_cfg_const.dongle_media_device)
    {
        app_le_audio_set_downstream_active(true);
    }
#endif
    init_ualist_head(&app_lea_db->dev_list);

    app_timer_reg_cb(le_audio_timeout_cback, &le_audio_timer_id);
#if (LE_EXT_FTL_SUPPORT == 1)
    lea_ext_ftl_init();
#endif
    app_le_filter_init();

#if F_APP_LEA_DONGLE_BINDING
    os_queue_init(&le_audio_white_list_queue);

    bt_dev_set_support_dongle_dual_mode();

    app_le_audio_load_white_list();

    ual_le_register_app_cb2(app_le_audio_gap_cback);
#endif
}
#endif
