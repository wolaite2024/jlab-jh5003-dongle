#include <stdio.h>
#include <stdlib.h>

#include "trace.h"
#include "le_audio_service.h"
#include "vector.h"
#include "csis_client.h"
#include "le_audio_data.h"
#include "app_usb_layer.h"
#include "le_csis_client_service.h"
#include "ual_adapter.h"
#include "le_unicast_src_service.h"
#include "mcp_mgr.h"
#include "ccp_mgr.h"
#include "app_audio_path.h"
#include "app_cfg.h"
#include "cap.h"
#include "csis_def.h"
#include "dev_mgr.h"
#include "le_unicast_gaming.h"
#if (LE_EXT_FTL_SUPPORT == 1)
#include "app_le_ext_ftl.h"
#endif

#if UAL_CONSOLE_PRINT
#include "console.h"
#endif
#include "mics_def.h"
#include "app_usb_uac.h"
#include "le_media_player.h"
#include "teams_call_control.h"
#include "ual_api_types.h"

#define MAX_DEV_SETTINGS     2

#define AUDIO_CFG_1_SERVER   0x6ABF
#define AUDIO_CFG_2_SERVER   0x9540

#define AUDIO_CFG_1_ASE_SNK  0x019D
#define AUDIO_CFG_2_ASE_SNK  0xC660
#define AUDIO_CFG_0_ASE_SNK  0x3802

#define AUDIO_CFG_1_ASE_SRC  0x2796         //unicast LC3 Audio Configuration
#define AUDIO_CFG_2_ASE_SRC  0xD800
#define AUDIO_CFG_0_ASE_SRC  0x0069

#define AUDIO_CFG_1_CHL_SNK  0xC7E5
#define AUDIO_CFG_2_CHL_SNK  0x0018
#define AUDIO_CFG_0_CHL_SNK  0x3802

#define AUDIO_CFG_1_CHL_SRC  0xDF96
#define AUDIO_CFG_2_CHL_SRC  0x2000
#define AUDIO_CFG_0_CHL_SRC  0x0069


#define FL_CSIS_EXIST       (1 << 0)
#define FL_CSIS_SIRK        (1 << 1)

#define METADATA_CCID_LTV_LEN   (3)

typedef struct
{
    T_UALIST_HEAD             active_devs_list;
    uint8_t                   play_mode;
    uint8_t                   qos;
    T_SERVER_ID               mcs_id;
    T_SERVER_ID               ccp_id;
    uint16_t                  media_audio_context;
    uint16_t                  conversation_audio_context;
    uint8_t                   media_snk_prefer_idx;
    uint8_t                   conversation_src_prefer_idx;
    uint8_t                   conversation_snk_prefer_idx;

    T_BLE_AUDIO_GROUP_HANDLE  group_handle;
    T_AUDIO_STREAM_SESSION_HANDLE media_session_handle;
    T_AUDIO_STREAM_SESSION_HANDLE conversation_session_handle;
    T_UNICAST_AUDIO_CFG_TYPE  media_cfg_type;
    T_UNICAST_AUDIO_CFG_TYPE  conversation_cfg_type;
    T_AUDIO_STREAM_STATE      media_state;
    T_AUDIO_STREAM_STATE      conversation_state;
    uint8_t                   output_chnl_mask;

    uint32_t                  service_flags;
    T_UALIST_HEAD             user_cfg_list;
    bool                      stream_starting;
} T_UNICAST_SRC_DB;


typedef struct t_unicast_dev_settings
{
    T_UALIST_HEAD           list;
    T_BLE_AUDIO_DEV_HANDLE  dev_handle;
    uint8_t                 bd_addr[BD_ADDR_LEN];
    uint8_t                 ase_num;
    uint8_t                 media_cfg_ase_num;
    uint8_t                 conversation_cfg_ase_num;
    bool                    src_set;

    T_UNICAST_ASE_CB        *media_ase_cb;
    T_UNICAST_ASE_CB        *conversation_ase_cb;
} T_UNICAST_DEV_SETTINGS;

#define GAMING_CE_LEN   (2 * BLE_CONN_GAMING_CI_DEF - 2)
#define FAST_CE_LEN     (2 * BLE_CONN_FAST_CI_DEF   - 2)

typedef enum
{
    CONN_UPDATE_STATE_CIS_DISCONNECTED,
    CONN_UPDATE_STATE_GATT_DISCOV,
    CONN_UPDATE_STATE_DISCOV_DONE,
    CONN_UPDATE_STATE_CIS_CONNECTED,
} T_LE_CONN_UPDATE_STATE;

typedef struct
{
    uint16_t conn_interval;
    uint16_t ce_length;
    uint16_t conn_latency;
    uint16_t supervision_timeout;
} T_CONN_UPDATE_INFO;

typedef struct conn_update_mgr
{
    struct conn_update_mgr  *p_next;
    bool                    mtu_updated;
    bool                    auth_cmpl;
    bool                    discov_done;
    uint8_t                 conn_id;
    T_LE_CONN_UPDATE_STATE  state;
    T_CONN_UPDATE_INFO      curr_active_update_info;
    T_CONN_UPDATE_INFO      update_info;
} T_CONN_UPDATE_MGR;

static const T_CONN_UPDATE_INFO conn_update_info[] =
{
    [CONN_UPDATE_STATE_CIS_DISCONNECTED] =
    {
        .conn_interval       = BLE_CONN_GAMING_CI_DEF,
        .ce_length           = GAMING_CE_LEN, /* 2 * 32 - 2 */
        .conn_latency        = BLE_CONN_SLAVE_LATENCY_DEF,
        .supervision_timeout = BLE_CONN_TIMEOUT_DEF,
    },

    [CONN_UPDATE_STATE_GATT_DISCOV] =
    {
        .conn_interval       = BLE_CONN_FAST_CI_DEF,
        .ce_length           = FAST_CE_LEN, /* 2 * 6 - 2 */
        .conn_latency        = BLE_CONN_SLAVE_LATENCY_DEF,
        .supervision_timeout = BLE_CONN_TIMEOUT_DEF,
    },

    [CONN_UPDATE_STATE_DISCOV_DONE] =
    {
        .conn_interval       = BLE_CONN_GAMING_CI_DEF,
        .ce_length           = GAMING_CE_LEN,
        .conn_latency        = BLE_CONN_SLAVE_LATENCY_DEF,
        .supervision_timeout = BLE_CONN_TIMEOUT_DEF,
    },

    [CONN_UPDATE_STATE_CIS_CONNECTED] =
    {
        .conn_interval       = BLE_CONN_GAMING_CI_DEF,
        .ce_length           = GAMING_CE_LEN,
        .conn_latency        = BLE_CONN_SLAVE_LATENCY_DEF,
        .supervision_timeout = BLE_CONN_TIMEOUT_DEF,
    },
};

static uint8_t unicast_metadata[] =
{
    0x03,
    METADATA_TYPE_STREAMING_AUDIO_CONTEXTS,
    (uint8_t)(AUDIO_CONTEXT_MEDIA),
    (uint8_t)(AUDIO_CONTEXT_MEDIA >> 8),
    0x2,
    METADATA_TYPE_CCID_LIST,
    0
};

static bool disable_conn_update = false;
static T_OS_QUEUE conn_update_mgr_queue;
static T_UNICAST_SRC_DB unicast_src_db;
static bool start_media_stream(uint8_t qos, bool gaming);
static bool start_conversation_stream(uint8_t qos, bool gaming);
extern T_TBS_CALL_STATE teams_get_call_state(void);

uint8_t le_unicast_src_get_stream_starting_state(void)
{
    return unicast_src_db.stream_starting;
}

void le_unicast_src_set_stream_starting_state(bool state)
{
    unicast_src_db.stream_starting = state;
}

uint8_t le_unicast_src_get_stream_state(void)
{
    APP_PRINT_INFO3("le_unicast_src_get_stream_state stream_starting %x media_state %x conversation_state %x",
                    unicast_src_db.stream_starting,
                    unicast_src_db.media_state,
                    unicast_src_db.conversation_state);
    if (le_unicast_src_get_stream_starting_state())
    {
        return true;
    }

    if ((unicast_src_db.media_state != AUDIO_STREAM_STATE_IDLE_CONFIGURED &&
         unicast_src_db.media_state != AUDIO_STREAM_STATE_IDLE) ||
        (unicast_src_db.conversation_state != AUDIO_STREAM_STATE_IDLE_CONFIGURED &&
         unicast_src_db.conversation_state != AUDIO_STREAM_STATE_IDLE))
    {
        return true;
    }

    return false;
}

static T_UNICAST_DEV_SETTINGS *le_audio_find_device_by_dev_handle(T_BLE_AUDIO_DEV_HANDLE dev_handle)
{
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        APP_PRINT_ERROR0("le_audio_find_device_by_dev_handle list is empty");
        return NULL;
    }

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        if (dev_set->dev_handle == dev_handle)
        {
            return dev_set;
        }
    }

    APP_PRINT_ERROR0("le_audio_find_device_by_dev_handle list can't find device");
    return NULL;
}

static bool le_audio_get_ase_param(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                   T_BLE_AUDIO_DEV_HANDLE dev_handle,
                                   uint8_t ase_id, T_BAP_UNICAST_ASE_INFO *ase_info)
{
    T_BAP_UNICAST_SESSION_INFO session_info;

    if (bap_unicast_audio_get_session_info(handle, &session_info))
    {
        for (uint8_t i = 0; i < session_info.dev_num; i++)
        {
            if (session_info.dev_info[i].dev_handle == dev_handle)
            {
                for (uint8_t j = 0; j < session_info.dev_info[i].ase_num; j++)
                {
                    if (session_info.dev_info[i].ase_info[j].ase_id == ase_id)
                    {
                        memcpy(ase_info, &session_info.dev_info[i].ase_info, sizeof(T_BAP_UNICAST_ASE_INFO));
                        return true;
                    }
                }
            }
        }
    }

    APP_PRINT_ERROR0("le_audio_get_ase_param fail!");
    return false;

}

uint8_t le_audio_get_unicast_play_mode(void)
{
    return unicast_src_db.play_mode;
}

#if (LE_AUDIO_CONNECT_FASTER == 1)
static bool le_audio_get_device_stream(void)
{
    return (app_cfg_const.dongle_media_device || app_get_usb_ds_state() || app_get_usb_us_state());
}
#endif

static T_UNICAST_DEV_SETTINGS *le_audio_find_added_device(uint8_t *bd_addr)
{
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        APP_PRINT_ERROR0("le_audio_find_added_device list is empty");
        return NULL;
    }

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        if (!memcmp(dev_set->bd_addr, bd_addr, BD_ADDR_LEN))
        {
            return dev_set;
        }
    }

    APP_PRINT_ERROR0("le_audio_find_added_device list can't find device");
    return NULL;
}

static T_UNICAST_DEV_SETTINGS *unicast_src_add_reserved_setting(T_UNICAST_DEV_SETTINGS *sibling)
{
    T_UNICAST_DEV_SETTINGS *settings = NULL;
    uint8_t i;
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;

    APP_PRINT_INFO1("unicast_src_add_reserved_setting: add set from %b",
                    TRACE_BDADDR(sibling->bd_addr));

    /* Fake a dev setting for reserved addr */
    settings = calloc(1, sizeof(T_UNICAST_DEV_SETTINGS) +
                      2 * sibling->ase_num * sizeof(T_UNICAST_ASE_CB));
    if (!settings)
    {
        APP_PRINT_ERROR0("unicast_src_add_reserved_setting: alloc err");
        return NULL;
    }
    init_ualist_head(&settings->list);
    settings->ase_num = sibling->ase_num;
    settings->media_cfg_ase_num = sibling->media_cfg_ase_num;
    settings->conversation_cfg_ase_num = sibling->conversation_cfg_ase_num;
    settings->media_ase_cb = (T_UNICAST_ASE_CB *)(settings + 1);
    settings->conversation_ase_cb = ((T_UNICAST_ASE_CB *)(settings + 1)) + settings->ase_num;
    memcpy(settings->bd_addr, bdaddr_reserved, BD_ADDR_LEN);
    /* We ignore the state in T_UNICAST_ASE_CB */
    for (i = 0; i < sibling->media_cfg_ase_num; i++)
    {
        /* ase_id
         * real_ase_id
         * direction
         * sample_freq
         * chnl_loc
         * prefer_codec_index
         * ase_id_associated
         * */
        memcpy(&settings->media_ase_cb[i].param, &sibling->media_ase_cb[i].param,
               sizeof(T_UNICAST_ASE_PARAM));
        settings->media_ase_cb[i].param.ase_id = 0;
        settings->media_ase_cb[i].param.real_ase_id = 0;
        settings->media_ase_cb[i].param.ase_id_associated = 0;

        /* We should set the opposite channel location. */
        if (sibling->media_ase_cb[i].param.chnl_loc == AUDIO_LOCATION_MONO)
        {
            settings->media_ase_cb[i].param.chnl_loc = AUDIO_LOCATION_MONO;
        }
        else if (sibling->media_ase_cb[i].param.chnl_loc & AUDIO_LOCATION_FL)
        {
            settings->media_ase_cb[i].param.chnl_loc = AUDIO_LOCATION_FR;
        }
        else
        {
            settings->media_ase_cb[i].param.chnl_loc = AUDIO_LOCATION_FL;
        }
    }
    for (i = 0; i < sibling->conversation_cfg_ase_num; i++)
    {
        /* ase_id
         * real_ase_id
         * direction
         * sample_freq
         * chnl_loc
         * prefer_codec_index
         * ase_id_associated
         * */
        memcpy(&settings->conversation_ase_cb[i].param,
               &sibling->conversation_ase_cb[i].param,
               sizeof(T_UNICAST_ASE_PARAM));
        settings->conversation_ase_cb[i].param.ase_id = 0;
        settings->conversation_ase_cb[i].param.real_ase_id = 0;
        settings->conversation_ase_cb[i].param.ase_id_associated = 0;
        /* We should set the opposite channel location no matter sibling is
         * source or sink.
         * */
        if (sibling->conversation_ase_cb[i].param.chnl_loc == AUDIO_LOCATION_MONO)
        {
            settings->conversation_ase_cb[i].param.chnl_loc = AUDIO_LOCATION_MONO;
        }
        else if (sibling->conversation_ase_cb[i].param.chnl_loc & AUDIO_LOCATION_FL)
        {
            settings->conversation_ase_cb[i].param.chnl_loc = AUDIO_LOCATION_FR;
        }
        else
        {
            settings->conversation_ase_cb[i].param.chnl_loc = AUDIO_LOCATION_FL;
        }
    }
    ualist_add_tail(&settings->list, &unicast_src_db.active_devs_list);

    return settings;
}

bool le_audio_update_unicast_addr(uint8_t *pseudo_bdaddr, uint8_t bdaddr_type)
{
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;

    if (!pseudo_bdaddr)
    {
        return false;
    }

    APP_PRINT_INFO1("le_audio_update_unicast_addr: new bdaddr %b",
                    TRACE_BDADDR(pseudo_bdaddr));

    dev_set = le_audio_find_added_device(pseudo_bdaddr);
    if (dev_set)
    {
        APP_PRINT_INFO0("le_audio_update_unicast_addr: Already updated addr");
        return true;
    }

    dev_set = le_audio_find_added_device(bdaddr_reserved);
    if (!dev_set)
    {
        APP_PRINT_ERROR1("le_audio_update_unicast_addr: No reserved slot for %b",
                         TRACE_BDADDR(pseudo_bdaddr));
        return false;
    }

    memcpy(dev_set->bd_addr, pseudo_bdaddr, 6);

    return true;
}

static void unicast_remove_all_dev_settings(void)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        ualist_del(&dev_set->list);
        free(dev_set);
    }
}

static void unicast_remove_all_dev_handles(void)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);

        APP_PRINT_INFO2("unicast_remove_all_dev_handles: Remove dev %b handle %p",
                        TRACE_BDADDR(dev_set->bd_addr),
                        dev_set->dev_handle);
        if (!ble_audio_group_del_dev(unicast_src_db.group_handle,
                                     &dev_set->dev_handle))
        {
            APP_PRINT_WARN1("unicast_remove_all_dev_handles: conversation Remove dev %b handle err",
                            TRACE_BDADDR(dev_set->bd_addr));
        }
    }
}

static bool update_unicast_dev_media_codec_cfg(T_BLE_AUDIO_DEV_HANDLE dev_handle, uint8_t ase_num,
                                               T_BAP_UNICAST_ASE_INFO *p_ase_info)
{
    T_UNICAST_DEV_SETTINGS *p_dev_set = le_audio_find_device_by_dev_handle(dev_handle);
    bool match_dev = false;
    T_AUDIO_ASE_CODEC_CFG prefer_codec_cfg;
    uint32_t channel_location = AUDIO_LOCATION_MONO;
    if (p_dev_set == NULL)
    {
        APP_PRINT_ERROR0("update_unicast_dev_media_codec_cfg list can't find device");
        return false;
    }
    for (uint8_t i = 0; i < ase_num; i++)
    {
        match_dev = false;
        for (uint8_t j = 0; j < p_dev_set->media_cfg_ase_num; j++)
        {
            if (!p_dev_set->media_ase_cb[j].param.ase_id_associated)
            {
                //FIX TODO channel num compare?
                if (p_ase_info[i].direction == p_dev_set->media_ase_cb[j].param.direction)
                {
                    /* FIXME: before bap_unicast_audio_start(), the ase_id is
                     * 0. So the real_ase_id is always set to 0.
                     * */
                    p_dev_set->media_ase_cb[j].param.real_ase_id = p_ase_info[i].ase_id;
                    p_dev_set->media_ase_cb[j].state = p_ase_info[i].ase_state;
                    channel_location = p_dev_set->media_ase_cb[j].param.chnl_loc;
                    p_dev_set->media_ase_cb[j].param.ase_id_associated = 1;
                    match_dev = true;
                    break;
                }
            }
        }

        memset(&prefer_codec_cfg, 0, sizeof(T_AUDIO_ASE_CODEC_CFG));
        prefer_codec_cfg.codec_id[0] = LC3_CODEC_ID;
        prefer_codec_cfg.target_latency = ASE_TARGET_LOWER_LATENCY;
        prefer_codec_cfg.target_phy = ASE_TARGET_PHY_2M;

        codec_preferred_cfg_get((T_CODEC_CFG_ITEM)unicast_src_db.media_snk_prefer_idx,
                                &prefer_codec_cfg.codec_cfg);

        prefer_codec_cfg.target_latency = (T_ASE_TARGET_LATENCY)unicast_src_db.qos;
        prefer_codec_cfg.target_phy = ASE_TARGET_PHY_2M;

        if (!match_dev)
        {
            APP_PRINT_ERROR2("update_unicast_dev_media_codec_cfg ase mis-match 0x%x, direction %d",
                             p_ase_info[i].ase_id, p_ase_info[i].direction);
        }

        prefer_codec_cfg.codec_cfg.type_exist |= CODEC_CFG_AUDIO_CHANNEL_ALLOCATION_EXIST;
        prefer_codec_cfg.codec_cfg.audio_channel_allocation = channel_location;

        if (!bap_unicast_audio_cfg_ase_codec(unicast_src_db.media_session_handle, dev_handle,
                                             p_ase_info[i].cfg_idx,
                                             &prefer_codec_cfg))
        {
            APP_PRINT_ERROR1("update_unicast_dev_media_codec_cfg config ase_id %d fail", p_ase_info[i].ase_id);
            return false;
        }

    }
    return true;
}

static bool update_unicast_dev_conversation_codec_cfg(T_BLE_AUDIO_DEV_HANDLE dev_handle,
                                                      uint8_t ase_num,
                                                      T_BAP_UNICAST_ASE_INFO *p_ase_info)
{
    T_UNICAST_DEV_SETTINGS *p_dev_set = le_audio_find_device_by_dev_handle(dev_handle);
    bool match_dev = false;
    T_AUDIO_ASE_CODEC_CFG prefer_codec_cfg;
    uint32_t channel_location = AUDIO_LOCATION_MONO;
    if (p_dev_set == NULL)
    {
        APP_PRINT_ERROR0("update_unicast_dev_conversation_codec_cfg list can't find device");
        return false;
    }
    for (uint8_t i = 0; i < ase_num; i++)
    {
        match_dev = false;
        for (uint8_t j = 0; j < p_dev_set->conversation_cfg_ase_num; j++)
        {
            if (!p_dev_set->conversation_ase_cb[j].param.ase_id_associated)
            {
                //FIX TODO channel num compare?
                if (p_ase_info[i].direction == p_dev_set->conversation_ase_cb[j].param.direction)
                {
                    /* FIXME: before bap_unicast_audio_start(), the ase_id is
                     * 0. So the real_ase_id is always set to 0.
                     * */
                    p_dev_set->conversation_ase_cb[j].param.real_ase_id = p_ase_info[i].ase_id;
                    p_dev_set->conversation_ase_cb[j].state = p_ase_info[i].ase_state;
                    channel_location = p_dev_set->conversation_ase_cb[j].param.chnl_loc;
                    p_dev_set->conversation_ase_cb[j].param.ase_id_associated = 1;
                    match_dev = true;
                    break;
                }
            }
        }

        memset(&prefer_codec_cfg, 0, sizeof(T_AUDIO_ASE_CODEC_CFG));
        prefer_codec_cfg.codec_id[0] = LC3_CODEC_ID;
        prefer_codec_cfg.target_latency = ASE_TARGET_LOWER_LATENCY;
        prefer_codec_cfg.target_phy = ASE_TARGET_PHY_2M;

        if (p_ase_info[i].direction == SERVER_AUDIO_SOURCE)
        {
            codec_preferred_cfg_get((T_CODEC_CFG_ITEM)unicast_src_db.conversation_src_prefer_idx,
                                    &prefer_codec_cfg.codec_cfg);
            /*we always create two channel lc3 decode track*/
            unicast_src_db.output_chnl_mask = AUDIO_LEFT_OUTPUT_MASK | AUDIO_RIGHT_OUTPUT_MASK;
        }
        else
        {
            codec_preferred_cfg_get((T_CODEC_CFG_ITEM)unicast_src_db.conversation_snk_prefer_idx,
                                    &prefer_codec_cfg.codec_cfg);
        }

        prefer_codec_cfg.target_latency = (T_ASE_TARGET_LATENCY)unicast_src_db.qos;
        prefer_codec_cfg.target_phy = ASE_TARGET_PHY_2M;

        if (!match_dev)
        {
            APP_PRINT_ERROR2("update_unicast_dev_conversation_codec_cfg ase mis-match 0x%x, direction %d",
                             p_ase_info[i].ase_id, p_ase_info[i].direction);
        }

        prefer_codec_cfg.codec_cfg.type_exist |= CODEC_CFG_AUDIO_CHANNEL_ALLOCATION_EXIST;
        prefer_codec_cfg.codec_cfg.audio_channel_allocation = channel_location;

        if (!bap_unicast_audio_cfg_ase_codec(unicast_src_db.conversation_session_handle, dev_handle,
                                             p_ase_info[i].cfg_idx,
                                             &prefer_codec_cfg))
        {
            APP_PRINT_ERROR1("update_unicast_dev_conversation_codec_cfg config ase_id %d fail",
                             p_ase_info[i].ase_id);
            return false;
        }
    }
    return true;
}


static void update_unicast_devs_state(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                      T_AUDIO_STREAM_STATE state)
{
    T_BAP_UNICAST_SESSION_INFO session_info;
    APP_PRINT_INFO1("update_unicast_devs_state stream state %d", state);
    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        APP_PRINT_ERROR0("update_unicast_devs_state list is empty");
        return;
    }

    if (bap_unicast_audio_get_session_info(handle, &session_info))
    {
        for (uint8_t i = 0; i < session_info.dev_num; i++)
        {
            T_UNICAST_DEV_SETTINGS *p_dev_set = le_audio_find_device_by_dev_handle(
                                                    session_info.dev_info[i].dev_handle);
            if (p_dev_set)
            {
                T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(p_dev_set->bd_addr);
                if (p_dev_rec)
                {
                    T_LE_AUDIO *p_audio_link = ble_audio_find_by_conn_id(p_dev_rec->le_conn_id);
                    if (p_audio_link)
                    {
                        if (state >= AUDIO_STREAM_STATE_CONFIGURED)
                        {
                            p_audio_link->audio_state |= CONFIGURED_STATE;
                        }
                        else
                        {
                            p_audio_link->audio_state &= (~CONFIGURED_STATE);
                        }

                        if (state == AUDIO_STREAM_STATE_STREAMING ||
                            state == AUDIO_STREAM_STATE_PARTIAL_STREAMING)
                        {
                            p_audio_link->audio_state |= STREAM_STATE;
                        }
                        else
                        {
                            p_audio_link->audio_state &= (~STREAM_STATE);
                        }

                        if (state == AUDIO_STREAM_STATE_RELEASING)
                        {
                            p_audio_link->audio_state |= RELEASING_STATE;
                        }
                        else
                        {
                            p_audio_link->audio_state &= ~RELEASING_STATE;
                        }

                        ble_usb_audio_state_change(p_dev_rec->pseudo_addr, p_dev_rec->bd_type, p_audio_link->state,
                                                   p_audio_link->audio_state, 0);
                    }
                }
            }
        }
    }
}

static void update_unicast_audio_state(uint8_t mode,
                                       T_AUDIO_STREAM_STATE state)
{
    uint8_t audio_state = LE_AUDIO_IDLE_STATE;

    if (mode != UNICAST_MEDIA_MODE && mode != UNICAST_CONVERSATION_MODE)
    {
        APP_PRINT_ERROR1("update_unicast_audio_state: invalid mode %u", mode);
        return;
    }

    APP_PRINT_INFO1("update_unicast_audio_state stream state %d", state);

    /* FIXME:
     * Sometimes, the bap state is emitted after device disconnection.
     * The dev settings have been removed after all device disconnection,
     * so the below list will be empty.
     * Let's emit the audio state changed event to upper layer no matter the
     * list is empty or not.
     * */
    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        APP_PRINT_ERROR0("update_unicast_audio_state list is empty");
    }

    if (state > AUDIO_STREAM_STATE_IDLE_CONFIGURED &&
        state < AUDIO_STREAM_STATE_STREAMING)
    {
        audio_state = LE_AUDIO_STREAM_STARTING_STATE;
    }
    else if (state == AUDIO_STREAM_STATE_STREAMING ||
             state == AUDIO_STREAM_STATE_PARTIAL_STREAMING)
    {
        audio_state = LE_AUDIO_STREAMING_STATE;
    }
    else if (state == AUDIO_STREAM_STATE_STOPPING ||
             state == AUDIO_STREAM_STATE_RELEASING)
    {
        audio_state = LE_AUDIO_RELEASING_STATE;
    }
    le_audio_state_play_callback(mode, audio_state);
}


static void handle_audio_session_removed(T_AUDIO_STREAM_SESSION_HANDLE handle)
{
    APP_PRINT_INFO4("handle_audio_session_removed: play_mode %d, handle 0x%x, 0x%x, 0x%x",
                    unicast_src_db.play_mode,
                    unicast_src_db.media_session_handle, unicast_src_db.conversation_session_handle, handle);

    if (unicast_src_db.media_session_handle == handle)
    {
        unicast_src_db.media_session_handle = NULL;
    }
    else if (unicast_src_db.conversation_session_handle == handle)
    {
        unicast_src_db.conversation_session_handle = NULL;
    }

}

static void handle_audio_session_released(T_AUDIO_STREAM_SESSION_HANDLE handle)
{
    APP_PRINT_INFO4("handle_audio_session_released: play_mode %d, handle 0x%x, 0x%x, 0x%x",
                    unicast_src_db.play_mode,
                    unicast_src_db.media_session_handle, unicast_src_db.conversation_session_handle, handle);

    /* lower layer will transfer to IDLE and emit bap message to app task. */
    bap_unicast_audio_remove_cfg(handle);
    bap_unicast_audio_remove_session(handle);
}

static void unicast_media_handle_bap_state(T_AUDIO_GROUP_BAP_STATE *p_data)
{
    if (!p_data)
    {
        APP_PRINT_ERROR0("unicast_media_handle_bap_state: invalid parameter");
        return;
    }

    if (unicast_src_db.media_state == p_data->state)
    {
        APP_PRINT_INFO1("unicast_media_handle_bap_state: same state %d",
                        p_data->state);
        return;
    }

    APP_PRINT_INFO3("unicast_media_handle_bap_state: new state %d, old state %d, media_device %d",
                    p_data->state, unicast_src_db.media_state, app_cfg_const.dongle_media_device);

    update_unicast_devs_state(p_data->handle, p_data->state);

    switch (p_data->state)
    {
    case AUDIO_STREAM_STATE_STOPPING:
    case AUDIO_STREAM_STATE_RELEASING:
        /* Release audio data path */
        break;
    case AUDIO_STREAM_STATE_IDLE:
        unicast_remove_all_dev_settings();
        //unicast_remove_dev_setting(bdaddr_reserved);
        unicast_remove_all_dev_handles();
        /* We keep the dev settings because next time we may start stream
         * without adding dev settings again
         * */
        break;
    case AUDIO_STREAM_STATE_IDLE_CONFIGURED:
        /* Generally the state changed from RELEASING to
         * IDLE_CONFIGURED. */
        if (unicast_src_db.media_state != AUDIO_STREAM_STATE_IDLE)
        {
            handle_audio_session_released(p_data->handle);
        }
        break;
    case AUDIO_STREAM_STATE_CONFIGURED:
        /* If err result occurred, release audio session. */
        if (p_data->result)
        {
            APP_PRINT_ERROR1("unicast_media_handle_bap_state: result %d",
                             p_data->result);
            bap_unicast_audio_release(unicast_src_db.media_session_handle);
            break;
        }
        /* ASE in QoS configured state. */
        break;
    case AUDIO_STREAM_STATE_PARTIAL_STREAMING:
        break;
    case AUDIO_STREAM_STATE_STREAMING:
        break;
    default:
        break;
    }

    unicast_src_db.media_state = p_data->state;
    update_unicast_audio_state(UNICAST_MEDIA_MODE, p_data->state);
}

static void unicast_conversation_handle_bap_state(T_AUDIO_GROUP_BAP_STATE *p_data)
{
    if (unicast_src_db.conversation_state == p_data->state)
    {
        APP_PRINT_INFO1("unicast_conversation_handle_bap_state: same state %d",
                        p_data->state);
        return;
    }

    APP_PRINT_INFO2("unicast_conversation_handle_bap_state: new state %d, old state %d",
                    p_data->state, unicast_src_db.conversation_state);

    update_unicast_devs_state(p_data->handle, p_data->state);

    switch (p_data->state)
    {
    case AUDIO_STREAM_STATE_STOPPING:
    case AUDIO_STREAM_STATE_RELEASING:
        break;
    case AUDIO_STREAM_STATE_IDLE:
        //unicast_remove_dev_setting(bdaddr_reserved);
        unicast_remove_all_dev_settings();
        unicast_remove_all_dev_handles();
        /* We keep the dev settings because next time we may start stream
         * without adding dev settings again
         * */
        break;
    case AUDIO_STREAM_STATE_IDLE_CONFIGURED:
        if (unicast_src_db.conversation_state != AUDIO_STREAM_STATE_IDLE)
        {
            handle_audio_session_released(p_data->handle);
        }
        break;
    case AUDIO_STREAM_STATE_CONFIGURED:
        /* If err result occurred, release audio session. */
        if (p_data->result)
        {
            APP_PRINT_ERROR1("unicast_conversation_handle_bap_state: result %d",
                             p_data->result);
            bap_unicast_audio_release(unicast_src_db.conversation_session_handle);
            break;
        }
        break;
    case AUDIO_STREAM_STATE_PARTIAL_STREAMING:
        break;
    default:
        break;
    }

    unicast_src_db.conversation_state = p_data->state;
    update_unicast_audio_state(UNICAST_CONVERSATION_MODE, p_data->state);
}

#if (LEA_GAMING_SPECIAL == 1)
static bool unicast_src_check_avail_metadata(uint8_t dir, uint16_t mask)
{
    T_BT_DEVICE *dev_rec = NULL;
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;
    T_LE_AUDIO *p_audio_link = NULL;

    APP_PRINT_INFO2("unicast_src_check_avail_metadata, dir %x, mask %d", dir, mask);

    /*FIXME: we asume all connected links have same snk avail context*/
    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);

        /* If dev setting is reserved, we skip it. */
        if (!memcmp(dev_set->bd_addr, bdaddr_reserved, 6))
        {
            continue;
        }

        dev_rec = ual_find_device_by_addr(dev_set->bd_addr);
        if (!dev_rec)
        {
            APP_PRINT_ERROR0("unicast_src_check_avail_metadata can't find device record");
            return false;
        }

        p_audio_link = ble_audio_find_by_conn_id(dev_rec->le_conn_id);
        if (!p_audio_link || p_audio_link->state != BT_CONN_STATE_CONNECTED)
        {
            APP_PRINT_ERROR0("unicast_src_check_avail_metadata: link is disconnected");
            continue;
        }

        APP_PRINT_INFO4("unicast_src_check_avail_metadata bd_addr %b src_avail %x snk_avail %x update %x",
                        TRACE_BDADDR(dev_set->bd_addr),
                        p_audio_link->src_avail_context,
                        p_audio_link->snk_avail_context,
                        mask);
        if (dir == SERVER_AUDIO_SOURCE)
        {
            if (p_audio_link->src_avail_context & mask)
            {
                return true;
            }
        }
        else
        {
            if (p_audio_link->snk_avail_context & mask)
            {
                return true;
            }
        }
    }
    return false;
}
#endif

bool unicast_src_special_game_update_metadata(uint16_t src_metadata, uint16_t snk_metadata)
{
    uint8_t update_support = 0;
    uint8_t metadata_len = 0;
    T_BAP_UNICAST_SESSION_INFO session_info;
    uint8_t dev_num = 0;
    T_BAP_UNICAST_DEV_INFO *p_dev_info = NULL;
    T_BAP_UNICAST_ASE_INFO *p_ase_info = NULL;

    APP_PRINT_INFO4("game_update_metadata, src_metadata %x snk_metadata %x mode %x conversation handle %x",
                    src_metadata, snk_metadata, unicast_src_db.play_mode, unicast_src_db.conversation_session_handle);
    /* special gaming mode always work in conversation mode */
    if ((unicast_src_db.play_mode != UNICAST_CONVERSATION_PLAY_MODE)
        || (!unicast_src_db.conversation_session_handle))
    {
        return false;
    }

    if (!unicast_src_check_avail_metadata(SERVER_AUDIO_SOURCE, src_metadata) ||
        !unicast_src_check_avail_metadata(SERVER_AUDIO_SINK, snk_metadata))
    {
        return false;
    }

    if (!bap_unicast_audio_get_session_info(unicast_src_db.conversation_session_handle, &session_info))
    {
        APP_PRINT_INFO0("unicast_src_special_game_update_metadata: get session info fail");
        return false;
    }

    dev_num = session_info.dev_num;
    for (uint8_t i = 0; i < dev_num; i++)
    {
        p_dev_info = (T_BAP_UNICAST_DEV_INFO *) & (session_info.dev_info[i]);

        if (p_dev_info->conn_state != GAP_CONN_STATE_CONNECTED)
        {
            continue;
        }

        for (uint8_t j = 0; j < p_dev_info->ase_num; j++)
        {
            p_ase_info = (T_BAP_UNICAST_ASE_INFO *) & (p_dev_info->ase_info[j]);
            metadata_len = sizeof(unicast_metadata);
            if (!p_ase_info)
            {
                continue;
            }

            if (p_ase_info->direction == SERVER_AUDIO_SOURCE)
            {
                if (src_metadata == AUDIO_CONTEXT_CONVERSATIONAL)
                {
                    if (le_tbs_get_enabled_cccd())
                    {
                        unicast_metadata[6] = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GTBS;
                    }
                    else
                    {
                        metadata_len -= METADATA_CCID_LTV_LEN;
                    }
                }
                else
                {
                    if (le_mcs_get_enabled_cccd())
                    {
                        unicast_metadata[6] = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GMCS;
                    }
                    else
                    {
                        metadata_len -= METADATA_CCID_LTV_LEN;
                    }
                }
                unicast_metadata[2] = (uint8_t)(src_metadata);
                unicast_metadata[3] = (uint8_t)(src_metadata >> 8);
                update_support++;
                bap_unicast_audio_cfg_ase_metadata(unicast_src_db.conversation_session_handle,
                                                   p_dev_info->dev_handle, p_ase_info->ase_id,
                                                   metadata_len, unicast_metadata);
            }
            else
            {
                if (snk_metadata == AUDIO_CONTEXT_CONVERSATIONAL)
                {
                    if (le_tbs_get_enabled_cccd())
                    {
                        unicast_metadata[6] = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GTBS;
                    }
                    else
                    {
                        metadata_len -= METADATA_CCID_LTV_LEN;
                    }
                }
                else
                {
                    if (le_mcs_get_enabled_cccd())
                    {
                        unicast_metadata[6] = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GMCS;
                    }
                    else
                    {
                        metadata_len -= METADATA_CCID_LTV_LEN;
                    }
                }
                unicast_metadata[2] = (uint8_t)(snk_metadata);
                unicast_metadata[3] = (uint8_t)(snk_metadata >> 8);
                update_support++;
                bap_unicast_audio_cfg_ase_metadata(unicast_src_db.conversation_session_handle,
                                                   p_dev_info->dev_handle, p_ase_info->ase_id,
                                                   metadata_len, unicast_metadata);
            }
        }
    }

    if (update_support)
    {
        return bap_unicast_audio_update(unicast_src_db.conversation_session_handle);
    }

    APP_PRINT_INFO2("game_update_metadata  src_context %x snk_context %x don't support update",
                    src_metadata, snk_metadata);
    return false;
}

static void unicast_src_update_qos_cfg(T_AUDIO_STREAM_SESSION_HANDLE handle, uint8_t prefer_idx,
                                       void *buf)
{
    uint8_t ret = 0;
    T_AUDIO_SESSION_QOS_CFG default_session_cfg = {0x0};
    T_AUDIO_SESSION_QOS_CFG session_cfg = {0x0};
    T_BAP_UNICAST_SESSION_INFO session_info;
    T_BAP_UNICAST_DEV_INFO *p_dev = NULL;
    T_AUDIO_ASE_QOS_CFG default_ase_qos = {0x0};
    T_AUDIO_ASE_QOS_CFG ase_qos = {0x0};
    T_AUDIO_GROUP_BAP_START_QOS_CFG *p_data = (T_AUDIO_GROUP_BAP_START_QOS_CFG *)buf;

#if (LE_AUDIO_GAMING_SUPPORT == 1)
    T_CODEC_CFG codec_cfg;
#endif

    if ((!handle) || (!buf))
    {
        ret = 1;
        goto fail;
    }

    if (!bap_unicast_audio_get_session_info(handle, &session_info))
    {
        ret = 2;
        goto fail;
    }

    if (!bap_unicast_audio_get_session_qos(handle, &default_session_cfg))
    {
        ret = 2;
        goto fail;
    }

    memset(&session_cfg, 0, sizeof(T_AUDIO_SESSION_QOS_CFG));
    memcpy(&session_cfg, &default_session_cfg, sizeof(T_AUDIO_SESSION_QOS_CFG));


    session_cfg.latency_s_m = default_session_cfg.latency_s_m;
    session_cfg.latency_m_s = default_session_cfg.latency_m_s;
    session_cfg.sink_presentation_delay = default_session_cfg.sink_presentation_delay;
    session_cfg.source_presentation_delay = default_session_cfg.source_presentation_delay;

#if (LE_AUDIO_CIG_INTERLEAVED == 1)
    session_cfg.packing = 1;    //interleaved
#else
    session_cfg.packing = 0;
#endif

#if (LE_AUDIO_GAMING_SUPPORT == 1)
    if (le_unicast_is_gaming_mode())
    {
        codec_preferred_cfg_get((T_CODEC_CFG_ITEM)prefer_idx, &codec_cfg);
        if (codec_cfg.frame_duration == FRAME_DURATION_CFG_7_5_MS)
        {
            session_cfg.latency_m_s = GAMING_DURATION_7_5_MS_QOS_LATENCY;
            session_cfg.latency_s_m = GAMING_DURATION_7_5_MS_QOS_LATENCY;
        }
        else
        {
            session_cfg.latency_m_s = GAMING_DURATION_10_MS_QOS_LATENCY;
            session_cfg.latency_s_m = GAMING_DURATION_10_MS_QOS_LATENCY;
        }
    }
#endif

    if (session_cfg.latency_m_s > p_data->sink_transport_latency_max)
    {
        session_cfg.latency_m_s = p_data->sink_transport_latency_max;
    }
    if (session_cfg.latency_s_m > p_data->source_transport_latency_max)
    {
        session_cfg.latency_s_m = p_data->source_transport_latency_max;
    }

    if ((session_cfg.sink_presentation_delay < p_data->sink_presentation_delay_min) ||
        (session_cfg.sink_presentation_delay > p_data->sink_presentation_delay_max))
    {
        session_cfg.sink_presentation_delay = p_data->sink_presentation_delay_max;
    }

    if ((session_cfg.source_presentation_delay < p_data->source_presentation_delay_min) ||
        (session_cfg.source_presentation_delay > p_data->source_presentation_delay_max))
    {
        session_cfg.source_presentation_delay = p_data->source_presentation_delay_max;
    }

    APP_PRINT_INFO4("unicast_src_update_qos_cfg: m_s %x s_m %x snk_pd %x src_pd %x",
                    session_cfg.latency_m_s, session_cfg.latency_s_m,
                    session_cfg.sink_presentation_delay,
                    session_cfg.source_presentation_delay);

    if (!bap_unicast_audio_cfg_session_qos(handle, &session_cfg))
    {
        ret = 3;
        goto fail;
    }

    for (uint8_t i = 0; i < session_info.dev_num; i++)
    {
        p_dev = &session_info.dev_info[i];
        if (p_dev->conn_state != GAP_CONN_STATE_CONNECTED)
        {
            continue;
        }
        for (uint8_t j = 0; j < p_dev->ase_num; j++)
        {
            if (!bap_unicast_audio_get_ase_qos(handle, p_dev->dev_handle, p_dev->ase_info[j].ase_id,
                                               &default_ase_qos))
            {
                ret = 4;
                goto fail;
            }

            memset(&ase_qos, 0, sizeof(T_AUDIO_ASE_QOS_CFG));
            memcpy(&ase_qos, &default_ase_qos, sizeof(T_AUDIO_ASE_QOS_CFG));
            ase_qos.retransmission_number = default_ase_qos.retransmission_number;
#if (LE_AUDIO_GAMING_SUPPORT == 1)
            if (le_unicast_is_gaming_mode())
            {
                ase_qos.retransmission_number = GAMING_QOS_RTN;
            }
#endif
            if (!bap_unicast_audio_cfg_ase_qos(handle, p_dev->dev_handle, p_dev->ase_info[j].ase_id, &ase_qos))
            {
                ret = 5;
                goto fail;
            }
        }
    }
    return;

fail:
    APP_PRINT_INFO2("unicast_src_update_qos_cfg: fail! ret %d, handle %x", -ret, handle);

}

void le_audio_unicast_src_group_cb(T_AUDIO_GROUP_MSG msg,
                                   T_BLE_AUDIO_GROUP_HANDLE handle,
                                   void *buf)
{
    switch (msg)
    {
    case AUDIO_GROUP_MSG_DEV_CONN:
        {
            T_AUDIO_GROUP_MSG_DEV_CONN *p_data = (T_AUDIO_GROUP_MSG_DEV_CONN *)buf;
            APP_PRINT_INFO2("AUDIO_GROUP_MSG_DEV_CONN: group handle 0x%x, dev handle 0x%x",
                            handle, p_data->dev_handle);
        }
        break;

    case AUDIO_GROUP_MSG_DEV_DISCONN:
        {
            T_AUDIO_GROUP_MSG_DEV_DISCONN *p_data = (T_AUDIO_GROUP_MSG_DEV_DISCONN *)buf;
            APP_PRINT_INFO3("AUDIO_GROUP_MSG_DEV_DISCONN: group handle 0x%x, dev handle 0x%x, cause 0x%x",
                            handle, p_data->dev_handle, p_data->cause);
        }
        break;
    case AUDIO_GROUP_MSG_DEV_BOND_REMOVE:
        {
            T_AUDIO_GROUP_MSG_DEV_BOND_REMOVE *p_data = (T_AUDIO_GROUP_MSG_DEV_BOND_REMOVE *)buf;
            APP_PRINT_INFO2("AUDIO_GROUP_MSG_DEV_BOND_REMOVE: group handle 0x%x, dev handle 0x%x",
                            handle, p_data->dev_handle);
        }
        break;
    case AUDIO_GROUP_MSG_DEV_BOND_CLEAR:
        {
            APP_PRINT_INFO1("AUDIO_GROUP_MSG_DEV_BOND_CLEAR: group handle 0x%x", handle);
        }
        break;
    case AUDIO_GROUP_MSG_DEV_EMPTY:
        {
            APP_PRINT_INFO1("AUDIO_GROUP_MSG_DEV_EMPTY: group handle 0x%x", handle);
        }
        break;

    case AUDIO_GROUP_MSG_BAP_STATE:
        {
            T_AUDIO_GROUP_BAP_STATE *p_data = (T_AUDIO_GROUP_BAP_STATE *)buf;

            APP_PRINT_INFO6("AUDIO_GROUP_MSG_BAP_STATE: group handle 0x%x, session handle 0x%x, curr_action %d, state %d, result %d, cause 0x%x",
                            handle, p_data->handle, p_data->curr_action,
                            p_data->state, p_data->result, p_data->cause);
            if (unicast_src_db.media_session_handle == p_data->handle)
            {
                unicast_media_handle_bap_state(p_data);
            }
            else if (unicast_src_db.conversation_session_handle == p_data->handle)
            {
                unicast_conversation_handle_bap_state(p_data);
            }
            if (le_unicast_src_get_stream_starting_state())
            {
                le_unicast_src_set_stream_starting_state(false);
            }
        }
        break;

    case AUDIO_GROUP_MSG_BAP_START_QOS_CFG:
        {
            T_AUDIO_GROUP_BAP_START_QOS_CFG *p_data = (T_AUDIO_GROUP_BAP_START_QOS_CFG *)buf;
            APP_PRINT_INFO4("AUDIO_GROUP_MSG_BAP_START_QOS_CFG: group handle 0x%x, session handle 0x%x, sink latency 0x%x, source latency 0x%x",
                            handle, p_data->handle,
                            p_data->sink_transport_latency_max, p_data->source_transport_latency_max);
            APP_PRINT_INFO8("AUDIO_GROUP_MSG_BAP_START_QOS_CFG: sink preferred presentation delay(0x%x-0x%x), sink presentation delay(0x%x-0x%x), source preferred presentation delay(0x%x-0x%x), source presentation delay(0x%x-0x%x)",
                            p_data->sink_preferred_presentation_delay_min, p_data->sink_preferred_presentation_delay_max,
                            p_data->sink_presentation_delay_min, p_data->sink_presentation_delay_max,
                            p_data->source_preferred_presentation_delay_min, p_data->source_preferred_presentation_delay_max,
                            p_data->source_presentation_delay_min, p_data->source_presentation_delay_max);
            if (unicast_src_db.play_mode == UNICAST_MEDIA_PLAY_MODE)
            {
                unicast_src_update_qos_cfg(p_data->handle, unicast_src_db.media_snk_prefer_idx, buf);
            }
            else
            {
                unicast_src_update_qos_cfg(p_data->handle, unicast_src_db.conversation_snk_prefer_idx, buf);
            }
        }
        break;
    case AUDIO_GROUP_MSG_BAP_CREATE_CIS:
        {
            T_AUDIO_GROUP_BAP_CREATE_CIS *p_data = (T_AUDIO_GROUP_BAP_CREATE_CIS *)buf;
            APP_PRINT_INFO3("AUDIO_GROUP_MSG_BAP_CREATE_CIS: group handle 0x%x, session handle 0x%x, dev_num 0x%x",
                            handle, p_data->handle, p_data->dev_num);
            if (unicast_src_db.media_session_handle == p_data->handle ||
                unicast_src_db.conversation_session_handle == p_data->handle)
            {
                for (uint8_t i = 0; i < p_data->dev_num; i++)
                {
                    T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(p_data->conn_handle_tbl[i]);
                    if (p_dev_rec)
                    {
                        bt_dev_disable_conn_update(p_dev_rec->pseudo_addr);
                    }
                }
            }
        }
        break;
    case AUDIO_GROUP_MSG_BAP_CIS_DISCONN:
        {
            T_AUDIO_GROUP_BAP_CIS_DISCONN *p_data = (T_AUDIO_GROUP_BAP_CIS_DISCONN *)buf;
            APP_PRINT_INFO4("AUDIO_GROUP_MSG_BAP_CIS_DISCONN: group handle 0x%x, session handle 0x%x, conn_handle 0x%x, reason 0x%x",
                            handle, p_data->handle, p_data->conn_handle, p_data->cause);
            if (unicast_src_db.media_session_handle == p_data->handle ||
                unicast_src_db.conversation_session_handle == p_data->handle)
            {
                T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(p_data->conn_handle);
                if (p_dev_rec)
                {
                    bt_dev_handle_conn_update_event(p_dev_rec->pseudo_addr, CI_EVENT_CIS_DISCONNECT);
                    bt_dev_enable_conn_update(p_dev_rec->pseudo_addr);
                }
            }

            if (unicast_src_db.media_session_handle == p_data->handle ||
                unicast_src_db.conversation_session_handle == p_data->handle)
            {
                T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(p_data->conn_handle);
                if (p_dev_rec)
                {
                    le_unicast_conn_update_mgr(CONN_UPDATE_EVENT_CIS_DISCONNECT, p_dev_rec->le_conn_id);
                }
            }
        }
        break;
    case AUDIO_GROUP_MSG_BAP_START_METADATA_CFG:
        {
            T_AUDIO_GROUP_BAP_START_METADATA_CFG *p_data = (T_AUDIO_GROUP_BAP_START_METADATA_CFG *)buf;
            T_AUDIO_DEV_INFO dev_info;
            T_AUDIO_SESSION_QOS_CFG qos_cfg;
            T_BAP_UNICAST_ASE_INFO ase_info;
            uint8_t metadata_len = 0;

            APP_PRINT_INFO4("AUDIO_GROUP_MSG_BAP_START_METADATA_CFG: group handle 0x%x, session handle 0x%x, ase_id %d, direction %x",
                            handle, p_data->handle, p_data->ase_id, p_data->direction);
            if (unicast_src_db.media_session_handle == p_data->handle)
            {
                metadata_len = sizeof(unicast_metadata);
                if (le_mcs_get_enabled_cccd())
                {
#if LE_AUDIO_MCP_SERVER_SUPPORT
                    unicast_metadata[6] = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GMCS;
#endif
                }
                else
                {
                    metadata_len -= METADATA_CCID_LTV_LEN;
                }
                unicast_metadata[2] = (uint8_t)(unicast_src_db.media_audio_context);
                unicast_metadata[3] = (uint8_t)(unicast_src_db.media_audio_context >> 8);
            }
            else
            {

                metadata_len = sizeof(unicast_metadata);
                if (le_tbs_get_enabled_cccd())
                {
#if LE_AUDIO_CCP_SERVER_SUPPORT
                    unicast_metadata[6] = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GTBS;
#endif
                }
                else
                {
                    metadata_len -= METADATA_CCID_LTV_LEN;
                }

                unicast_metadata[2] = (uint8_t)(unicast_src_db.conversation_audio_context);
                unicast_metadata[3] = (uint8_t)(unicast_src_db.conversation_audio_context >> 8);
#if (LEA_GAMING_SPECIAL == 1)
                APP_PRINT_INFO3("AUDIO_GROUP_MSG_BAP_START_METADATA_CFG specail_gaming %x us_state %x dir %x",
                                le_unicast_is_gaming_mode(), app_get_usb_us_state(), p_data->direction);
                if (le_unicast_is_gaming_mode())
                {
                    if (!app_get_usb_us_state())
                    {
                        if (p_data->direction == SERVER_AUDIO_SOURCE)
                        {
                            if (unicast_src_check_avail_metadata(p_data->direction, AUDIO_CONTEXT_UNSPECIFIED))
                            {
                                unicast_metadata[2] = (uint8_t)(AUDIO_CONTEXT_UNSPECIFIED);
                                unicast_metadata[3] = (uint8_t)(AUDIO_CONTEXT_UNSPECIFIED >> 8);
                            }
                        }
                        else
                        {
                            if (unicast_src_check_avail_metadata(p_data->direction, AUDIO_CONTEXT_MEDIA))
                            {
                                unicast_metadata[2] = (uint8_t)(AUDIO_CONTEXT_MEDIA);
                                unicast_metadata[3] = (uint8_t)(AUDIO_CONTEXT_MEDIA >> 8);
                            }
                        }
                        gap_register_direct_cb(NULL);
                    }
                    else
                    {
                        if (unicast_src_check_avail_metadata(p_data->direction, AUDIO_CONTEXT_CONVERSATIONAL))
                        {
                            unicast_metadata[2] = (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL);
                            unicast_metadata[3] = (uint8_t)(AUDIO_CONTEXT_CONVERSATIONAL >> 8);
                        }
                        gap_register_direct_cb(le_audio_data_direct_callback);
                    }
                }
#endif
            }
            bap_unicast_audio_cfg_ase_metadata(p_data->handle, p_data->dev_handle, p_data->ase_id,
                                               metadata_len, (uint8_t *)unicast_metadata);

            if (le_audio_get_ase_param(p_data->handle, p_data->dev_handle, p_data->ase_id, &ase_info))
            {
                if (ase_info.direction == SERVER_AUDIO_SINK)
                {
                    if (bap_unicast_audio_get_session_qos(p_data->handle, &qos_cfg) &&
                        ble_audio_group_get_dev_info(handle, p_data->dev_handle, &dev_info))
                    {
                        bt_dev_handle_conn_update_event(dev_info.bd_addr, CI_EVENT_CIS_ESTABLISH);
                    }
                }
            }

            if (le_audio_get_ase_param(p_data->handle, p_data->dev_handle, p_data->ase_id, &ase_info))
            {
                if (ase_info.direction == SERVER_AUDIO_SINK)
                {
                    if (bap_unicast_audio_get_session_qos(p_data->handle, &qos_cfg) &&
                        ble_audio_group_get_dev_info(handle, p_data->dev_handle, &dev_info))
                    {
                        T_BT_DEVICE *dev_rec = ual_find_device_by_addr(dev_info.bd_addr);

                        if (dev_rec != NULL)
                        {
                            le_unicast_conn_update_mgr(CONN_UPDATE_EVENT_CREATE_CIS, dev_rec->le_conn_id);
                        }
                    }
                }
            }
        }
        break;

    case AUDIO_GROUP_MSG_BAP_SESSION_REMOVE:
        {
            T_AUDIO_GROUP_BAP_SESSION_REMOVE *p_data = (T_AUDIO_GROUP_BAP_SESSION_REMOVE *)buf;
            APP_PRINT_INFO2("AUDIO_GROUP_MSG_BAP_SESSION_REMOVE: group handle 0x%x, session handle 0x%x",
                            handle, p_data->handle);
            handle_audio_session_removed(p_data->handle);
        }
        break;

    case AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH:
        {
            T_AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH *p_data = (T_AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH *)buf;
            APP_PRINT_INFO2("AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH: group handle 0x%x, session handle 0x%x",
                            handle, p_data->handle);
            T_CIS_SETUP_DATA_PATH data;
            data.cis_conn_handle = p_data->cis_conn_handle;
            data.codec_parsed_data = p_data->codec_parsed_data;
            data.path_direction = p_data->path_direction;
            handle_cis_data_path_setup_cmplt_msg(&data);
        }
        break;

    case AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH:
        {
            T_AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH *p_data = (T_AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH *)buf;
            APP_PRINT_INFO2("AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH: group handle 0x%x, session handle 0x%x",
                            handle, p_data->handle);

            T_CIS_REMOVE_DATA_PATH data;
            data.cis_conn_handle = p_data->cis_conn_handle;
            data.path_direction = p_data->path_direction;
            handle_cis_data_path_remove_complt_msg(&data);
        }
        break;

    case AUDIO_GROUP_MSG_BAP_METADATA_UPDATE:
        {
            T_AUDIO_GROUP_MSG_BAP_METADATA_UPDATE *p_data = (T_AUDIO_GROUP_MSG_BAP_METADATA_UPDATE *)buf;
            APP_PRINT_INFO6("AUDIO_GROUP_MSG_BAP_METADATA_UPDATE: group handle 0x%x, session handle 0x%x, dev_handle 0x%x, ase_id 0x%x, metadata_length %d, p_metadata %b",
                            handle, p_data->handle,
                            p_data->dev_handle,
                            p_data->ase_id,
                            p_data->metadata_length,
                            TRACE_BINARY(p_data->metadata_length, p_data->p_metadata));
        }
        break;
    default:
        break;
    }
    return;
}

bool unicast_src_update_ringtone_metadata(void)
{
    T_BT_DEVICE *dev_rec = NULL;
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;
    T_LE_AUDIO *p_audio_link = NULL;
    uint8_t metadata_len = 0;

    APP_PRINT_INFO1("unicast_src_update_ringtone_metadata, current mode %d", unicast_src_db.play_mode);

    if (unicast_src_db.play_mode == UNICAST_CONVERSATION_PLAY_MODE)
    {
        return true;
    }
    /* check ase available context */

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);

        /* If dev setting is reserved, we skip it. */
        if (!memcmp(dev_set->bd_addr, bdaddr_reserved, 6))
        {
            continue;
        }

        dev_rec = ual_find_device_by_addr(dev_set->bd_addr);
        if (!dev_rec)
        {
            APP_PRINT_ERROR0("unicast_src_update_ringtone_metadata can't find device record");
            return false;
        }

        p_audio_link = ble_audio_find_by_conn_id(dev_rec->le_conn_id);
        if (!p_audio_link || p_audio_link->state != BT_CONN_STATE_CONNECTED)
        {
            APP_PRINT_ERROR0("unicast_src_update_ringtone_metadata: link is disconnected");
            /*
             * If there are two devices that are or will be connected.
             * Only audio channel location is different between the two
             * devices.
             * So we can determine the audio channel locations when any one
             * device is connected.
             * */
            continue;
        }

        /* Check stream context */
        if (!(p_audio_link->snk_avail_context & AUDIO_CONTEXT_RINGTONE))
        {
            APP_PRINT_ERROR2("unicast_src_update_ringtone_metadata bd_addr %b context %x not support RINGTONE",
                             TRACE_BDADDR(dev_set->bd_addr), p_audio_link->snk_avail_context);
        }
    }
    /* FIXME: we assume ringtone media data come before hid telephony*/
    if (unicast_src_db.play_mode == UNICAST_MEDIA_PLAY_MODE)
    {
        if (teams_get_call_state() == TBS_CALL_STATE_INCOMING)
        {
            metadata_len = sizeof(unicast_metadata);
            if (le_mcs_get_enabled_cccd())
            {
                unicast_metadata[6] = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GMCS;
            }
            else
            {
                metadata_len -= METADATA_CCID_LTV_LEN;
            }
            unicast_metadata[2] = (uint8_t)(unicast_src_db.media_audio_context | AUDIO_CONTEXT_RINGTONE);
            unicast_metadata[3] = (uint8_t)((unicast_src_db.media_audio_context | AUDIO_CONTEXT_RINGTONE) >> 8);
        }
        else
        {
            if (unicast_src_db.media_audio_context & ~AUDIO_CONTEXT_RINGTONE)
            {
                unicast_src_db.media_audio_context &= ~AUDIO_CONTEXT_RINGTONE;
            }
            else
            {
                return false;
            }
        }
        APP_PRINT_INFO1("unicast_src_update_ringtone_metadata: context %x",
                        unicast_src_db.media_audio_context);
        if (bap_unicast_audio_cfg_ase_metadata(unicast_src_db.media_session_handle, NULL, 0,
                                               metadata_len, unicast_metadata))
        {
            return bap_unicast_audio_update(unicast_src_db.media_session_handle);
        }
    }

    return false;
}

static bool unicast_src_check_and_alloc_group(void)
{
    T_BT_DEVICE *dev_rec = NULL;
    T_BT_DEVICE *conn_rec = NULL;
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_LE_AUDIO *link = NULL;
    T_LE_AUDIO *audio_link = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;

    if (unicast_src_db.group_handle)
    {
        APP_PRINT_INFO1("unicast_src_check_and_alloc_group: Group %08x already exists",
                        unicast_src_db.group_handle);
        return true;
    }

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        dev_rec = ual_find_device_by_addr(dev_set->bd_addr);
        if (!dev_rec)
        {
            continue;
        }
        link = ble_audio_find_by_conn_handle(dev_rec->le_conn_handle);
        if (!link || link->state != BT_CONN_STATE_CONNECTED)
        {
            continue;
        }
        conn_rec = dev_rec;
        audio_link = link;
        if (audio_link->supported_services & LE_AUDIO_CSIS_CLIENT_FLAG)
        {
            break;
        }
    }

    if (!audio_link || !conn_rec)
    {
        APP_PRINT_ERROR0("unicast_src_check_and_alloc_group: No link");
        return false;
    }

    if (unicast_src_db.service_flags & FL_CSIS_EXIST)
    {
        T_BLE_AUDIO_GROUP_HANDLE group_handle;

        group_handle = set_coordinator_find_by_addr(conn_rec->pseudo_addr,
                                                    conn_rec->bd_type,
                                                    GATT_UUID_CAS, NULL);
        if (group_handle)
        {
            /* FIXME: How to get the device SIRK. */
            unicast_src_db.group_handle = group_handle;
        }
        else
        {
            APP_PRINT_ERROR1("unicast_src_check_and_alloc_group: No group for %b",
                             TRACE_BDADDR(conn_rec->pseudo_addr));
            return false;
        }
    }

    return !!unicast_src_db.group_handle;
}

static uint32_t le_audio_freq_trans_to_freq_mask(uint8_t freq)
{
    uint32_t freq_mask = 0;
    switch (freq)
    {
    case SAMPLING_FREQUENCY_CFG_8K:
        {
            freq_mask = SAMPLE_FREQ_8K_TABLE_MASK;
        }
        break;

    case SAMPLING_FREQUENCY_CFG_16K:
        {
            freq_mask = SAMPLE_FREQ_16K_TABLE_MASK;
        }
        break;

    case SAMPLING_FREQUENCY_CFG_24K:
        {
            freq_mask = SAMPLE_FREQ_24K_TABLE_MASK;
        }
        break;

    case SAMPLING_FREQUENCY_CFG_32K:
        {
            freq_mask = SAMPLE_FREQ_32K_TABLE_MASK;
        }
        break;

    case SAMPLING_FREQUENCY_CFG_44_1K:
        {
            freq_mask = SAMPLE_FREQ_441K_TABLE_MASK;
        }
        break;

    case SAMPLING_FREQUENCY_CFG_48K:
        {
            freq_mask = SAMPLE_FREQ_48K_TABLE_MASK;
        }
        break;

    default:
        break;
    }
    return freq_mask;
}

static bool unicast_find_prefer_l3c_cfg_idx(uint16_t sup_table_mask, uint8_t duration,
                                            uint16_t min_frame_size, uint8_t *p_index,
                                            uint16_t prefer_codec_idx)
{
    uint8_t index = 0;
    T_CODEC_CFG cfg;

    if (p_index == NULL)
    {
        return false;
    }
    if (duration == FRAME_DURATION_CFG_10_MS)
    {
        sup_table_mask &= FRAME_DURATION_10M_TABLE_MASK;
    }
    else
    {
        sup_table_mask &= FRAME_DURATION_7_5M_TABLE_MASK;
    }
    *p_index = 0xFF;

    if (sup_table_mask == 0)
    {
        return false;
    }

    sup_table_mask = (sup_table_mask & prefer_codec_idx) ? (sup_table_mask & prefer_codec_idx)
                     : sup_table_mask;

    while (sup_table_mask)
    {
        if (sup_table_mask & 0x01)
        {
            if (codec_preferred_cfg_get((T_CODEC_CFG_ITEM)index, &cfg))
            {
                if (min_frame_size <= cfg.octets_per_codec_frame)
                {
                    *p_index = index;
                    return true;
                }
            }
        }
        sup_table_mask >>= 1;
        index++;
    }

    return false;
}

static uint8_t le_audio_get_server_num(uint8_t mode)
{
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;

    uint8_t server_num = 0;
    uint8_t cfg_ase_num = 0;

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        APP_PRINT_ERROR0("le_audio_get_server_num list is empty");
        return false;
    }

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        if (mode == UNICAST_MEDIA_PLAY_MODE)
        {
            cfg_ase_num = dev_set->media_cfg_ase_num;
        }
        else
        {
            cfg_ase_num = dev_set->conversation_cfg_ase_num;
        }
        if (cfg_ase_num > 0)
        {
            server_num++;
        }
    }
    APP_PRINT_INFO2("le_audio_get_server_num mode %x num %x", mode, server_num);
    return server_num;
}

static bool le_audio_get_cfg_ase_info(uint8_t mode, uint8_t dir, uint8_t *p_ase_num,
                                      uint8_t *p_chnl_num, uint32_t *p_freq, uint16_t *p_codec)
{
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    uint8_t cfg_ase_num = 0;

    uint8_t ase_num = 0;
    uint8_t chnl_num = 0;
    uint16_t prefer_codec = 0;
    uint8_t sample_freq = 0;

    uint8_t temp_chnl_num = 0;
    T_UNICAST_ASE_CB *ase_cb = NULL;

    if ((!p_ase_num) || (!p_chnl_num) || (!p_freq) || (!p_codec))
    {
        APP_PRINT_ERROR0("le_audio_get_cfg_ase_info param error");
        return false;
    }

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        APP_PRINT_ERROR0("le_audio_get_cfg_ase_info list is empty");
        return false;
    }

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        if (mode == UNICAST_MEDIA_PLAY_MODE)
        {
            cfg_ase_num = dev_set->media_cfg_ase_num;
            ase_cb = dev_set->media_ase_cb;
        }
        else
        {
            cfg_ase_num = dev_set->conversation_cfg_ase_num;
            ase_cb = dev_set->conversation_ase_cb;
        }

        if ((cfg_ase_num == 0) || (ase_cb == NULL))
        {
            return false;
        }
        for (uint8_t i = 0; i < cfg_ase_num; i++)
        {
            if (ase_cb[i].param.direction != dir)
            {
                continue;
            }
            if (ase_cb[i].param.prefer_codec_index != 0xFF)
            {
                prefer_codec = (1 << ase_cb[i].param.prefer_codec_index);
            }

            if (sample_freq == 0)
            {
                sample_freq = ase_cb[i].param.sample_freq;
            }
            else if (sample_freq != ase_cb[i].param.sample_freq)
            {
                APP_PRINT_ERROR2("le_audio_get_cfg_ase_info sample mis-match 0x%x : 0x%x",
                                 sample_freq,
                                 ase_cb[i].param.sample_freq);
                return false;
            }
            if (ase_cb[i].param.chnl_loc == AUDIO_LOCATION_MONO)
            {
                temp_chnl_num = 1;
            }
            else
            {
                temp_chnl_num = __builtin_popcount(ase_cb[i].param.chnl_loc);
            }
            if (chnl_num == 0)
            {
                chnl_num = temp_chnl_num;
            }
            else if (chnl_num != temp_chnl_num)
            {
                APP_PRINT_ERROR2("le_audio_get_cfg_ase_info chnl mis-match channels :%d, chnl_cnt: %d",
                                 chnl_num, temp_chnl_num);
                return false;
            }
            ase_num++;
        }
    }
    *p_ase_num = ase_num;
    *p_chnl_num = chnl_num;
    *p_freq = le_audio_freq_trans_to_freq_mask(sample_freq);;
    *p_codec = prefer_codec;

    return true;
}

static uint32_t le_audio_get_lc3_sample_mask(uint16_t handle, uint8_t dir, uint8_t duration,
                                             uint16_t context, uint8_t chnl)
{
    uint32_t freq_mask = 0;
    uint32_t duration_mask = 0;

    if (duration & FRAME_DURATION_10_MS_BIT)
    {
        duration_mask |= FRAME_DURATION_10M_TABLE_MASK;
    }

    if (duration & FRAME_DURATION_7_5_MS_BIT)
    {
        duration_mask |= FRAME_DURATION_7_5M_TABLE_MASK;
    }

    if (dir == SERVER_AUDIO_SINK)
    {
        freq_mask = bap_pacs_get_lc3_snk_table_msk(handle, context, chnl, 1);
        if ((freq_mask & duration_mask) == 0)
        {
            freq_mask = bap_pacs_get_lc3_snk_table_msk(handle, AUDIO_CONTEXT_UNSPECIFIED, chnl, 1);
        }
    }
    else
    {
        freq_mask = bap_pacs_get_lc3_src_table_msk(handle, context, chnl, 1);
        if ((freq_mask & duration_mask) == 0)
        {
            freq_mask = bap_pacs_get_lc3_src_table_msk(handle, AUDIO_CONTEXT_UNSPECIFIED, chnl, 1);
        }
    }
    return (freq_mask & duration_mask);
}

static bool le_audio_get_lc3_cfg_index(uint8_t mode, uint8_t dir, uint32_t sup_freq_mask,
                                       uint8_t duration,
                                       uint16_t prefer_codec, uint8_t *p_lc3_idx)
{
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_BT_DEVICE *dev_rec = NULL;

    uint8_t cfg_ase_num = 0;
    uint8_t chnl_cnt = 0;
    uint32_t temp_freq_mask = 0;
    uint16_t session_context = 0;
    T_UNICAST_ASE_CB *ase_cb = NULL;
    T_LE_AUDIO *p_audio_link = NULL;
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;
    uint8_t lc3_index = 0;
    bool ret = false;

    if (!p_lc3_idx)
    {
        return false;
    }

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        return false;
    }

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);

        /* If dev setting is reserved, we skip it. */
        if (!memcmp(dev_set->bd_addr, bdaddr_reserved, 6))
        {
            continue;
        }

        dev_rec = ual_find_device_by_addr(dev_set->bd_addr);
        if (!dev_rec)
        {
            continue;
        }

        p_audio_link = ble_audio_find_by_conn_id(dev_rec->le_conn_id);
        if (!p_audio_link || p_audio_link->state != BT_CONN_STATE_CONNECTED)
        {
            APP_PRINT_ERROR0("le_audio_get_lc3_cfg_index: link is disconnected");
            /*
             * If there are two devices that are or will be connected.
             * Only audio channel location is different between the two
             * devices.
             * So we can determine the audio channel locations when any one
             * device is connected.
             * */
            continue;
        }
        if (mode == UNICAST_MEDIA_PLAY_MODE)
        {
            cfg_ase_num = dev_set->media_cfg_ase_num;
            ase_cb = dev_set->media_ase_cb;
            session_context = AUDIO_CONTEXT_MEDIA;
        }
        else
        {
            cfg_ase_num = dev_set->conversation_cfg_ase_num;
            ase_cb = dev_set->conversation_ase_cb;
            session_context = AUDIO_CONTEXT_CONVERSATIONAL;
        }
        for (uint8_t i = 0; i < cfg_ase_num; i++)
        {
            temp_freq_mask = 0;
            if (ase_cb[i].param.direction != dir)
            {
                continue;
            }
            if (ase_cb[i].param.chnl_loc == AUDIO_LOCATION_MONO)
            {
                chnl_cnt = 1;
            }
            else
            {
                chnl_cnt = __builtin_popcount(ase_cb[i].param.chnl_loc);
            }

            temp_freq_mask = le_audio_get_lc3_sample_mask(p_audio_link->conn_handle,
                                                          dir,
                                                          duration,
                                                          session_context,
                                                          chnl_cnt);
            if ((temp_freq_mask & sup_freq_mask) == 0)
            {
                temp_freq_mask = le_audio_get_lc3_sample_mask(p_audio_link->conn_handle,
                                                              dir,
                                                              duration,
                                                              AUDIO_CONTEXT_UNSPECIFIED,
                                                              chnl_cnt);
            }
            sup_freq_mask &= temp_freq_mask;
        }
    }

    if ((!sup_freq_mask) || (sup_freq_mask == 0xFFFF))
    {
        APP_PRINT_ERROR1("le_audio_get_lc3_cfg_index sup_freq_mask %x", sup_freq_mask);
        return false;
    }

    /* frame_duration 10 is high priority*/
    if (duration & FRAME_DURATION_10_MS_BIT)
    {
        ret = unicast_find_prefer_l3c_cfg_idx(sup_freq_mask, FRAME_DURATION_CFG_10_MS,
                                              0, &lc3_index, prefer_codec);
    }

    if (ret == false)
    {
        if (duration & FRAME_DURATION_7_5_MS_BIT)
        {
            ret = unicast_find_prefer_l3c_cfg_idx(sup_freq_mask, FRAME_DURATION_CFG_7_5_MS,
                                                  0, &lc3_index, prefer_codec);
        }
    }

    if (ret == false)
    {
        APP_PRINT_ERROR0("le_audio_get_lc3_cfg_index can't find prefer idx");
        return false;
    }


    APP_PRINT_INFO3("le_audio_get_lc3_cfg_index mode %x dir %x lc3_index %x", mode, dir, lc3_index);
    *p_lc3_idx = lc3_index;
    return true;
}

static bool le_audio_get_avail_context(uint8_t mode, uint16_t *p_context)
{
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_BT_DEVICE *dev_rec = NULL;
    T_LE_AUDIO *p_link = NULL;
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;
    uint16_t group_context = 0;
    uint16_t link_context = 0;

    if (!p_context)
    {
        return false;
    }

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        APP_PRINT_ERROR0("le_audio_get_avail_context list is empty");
        return false;
    }
    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);

        /* If dev setting is reserved, we skip it. */
        if (!memcmp(dev_set->bd_addr, bdaddr_reserved, 6))
        {
            continue;
        }

        dev_rec = ual_find_device_by_addr(dev_set->bd_addr);
        if (!dev_rec)
        {
            APP_PRINT_ERROR0("le_audio_get_avail_context can't find device record");
            return false;
        }

        p_link = ble_audio_find_by_conn_id(dev_rec->le_conn_id);
        if (!p_link || p_link->state != BT_CONN_STATE_CONNECTED)
        {
            APP_PRINT_ERROR0("le_audio_get_avail_context: link is disconnected");
            /*
             * If there are two devices that are or will be connected.
             * Only audio channel location is different between the two
             * devices.
             * So we can determine the audio channel locations when any one
             * device is connected.
             * */
            continue;
        }
        if (mode == UNICAST_MEDIA_PLAY_MODE)
        {
            link_context = p_link->snk_avail_context;
        }
        else
        {
            link_context = p_link->snk_avail_context & p_link->src_avail_context;;
        }
        if (group_context == 0)
        {
            group_context = link_context;
        }
        else
        {
            group_context &= link_context;
        }
    }
    if (!group_context)
    {
        APP_PRINT_INFO1("le_audio_get_avail_context mode %x no avail context", mode);
        return false;
    }
    *p_context = group_context;
    return true;
}

static bool le_audio_select_media_prefer_codec(uint8_t qos, bool gaming_mode)
{
    uint16_t cfg_mask = 0xFFFF;
    uint32_t sup_freq_mask = 0xFFFFFFFF;
    uint8_t snk_ase_num = 0;
    uint8_t chnl_num = 0;
    uint32_t sample_freq = 0;
    uint16_t ase_codec_mask = 0;
    uint8_t server_num = 0;
    uint16_t prefer_codec_msk = 0;
    uint8_t frame_snk_duration = 0;
    uint16_t avail_context = 0;
    uint8_t idx = 0;
    uint8_t ret = 0;

    if (qos == ASE_TARGET_HIGHER_RELIABILITY)
    {
        prefer_codec_msk = CODEC_CFG_ITEM_48_5_BIT | CODEC_CFG_ITEM_48_6_BIT;
        frame_snk_duration = FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_10_MS_BIT;
    }
    else
    {
        prefer_codec_msk = CODEC_CFG_ITEM_48_1_BIT | CODEC_CFG_ITEM_48_2_BIT;
        frame_snk_duration = FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_10_MS_BIT;
    }

    if (gaming_mode)
    {
        frame_snk_duration = FRAME_DURATION_7_5_MS_BIT;
        prefer_codec_msk = CODEC_CFG_ITEM_48_1_BIT;
    }

    server_num = le_audio_get_server_num(UNICAST_MEDIA_PLAY_MODE);
    if ((server_num != 1) && (server_num != 2))
    {
        ret = 1;
        goto CHECK_FAIL;
    }

    if (!le_audio_get_cfg_ase_info(UNICAST_MEDIA_PLAY_MODE, SERVER_AUDIO_SINK, &snk_ase_num,
                                   &chnl_num, &sample_freq, &ase_codec_mask))
    {
        ret = 2;
        goto CHECK_FAIL;
    }

    /* FIXME: By now we only support bap lc3 audio configuration topology */
    /* remove conversational topology */
    cfg_mask &= (~(AUDIO_CFG_1_ASE_SRC | AUDIO_CFG_2_ASE_SRC));
    cfg_mask &= (~(AUDIO_CFG_1_CHL_SRC | AUDIO_CFG_2_CHL_SRC));

    if (server_num == 2)
    {
        cfg_mask &= (~AUDIO_CFG_1_SERVER);
    }
    else if (server_num == 1)
    {
        cfg_mask &= (~AUDIO_CFG_2_SERVER);
    }

    if (snk_ase_num == 1)
    {
        cfg_mask &= (~AUDIO_CFG_2_ASE_SNK);
        cfg_mask &= (~AUDIO_CFG_0_ASE_SNK);
    }
    else if (snk_ase_num == 2)
    {
        cfg_mask &= (~AUDIO_CFG_1_ASE_SNK);
        cfg_mask &= (~AUDIO_CFG_0_ASE_SNK);
    }
    else if (snk_ase_num == 0)
    {
        cfg_mask &= (~AUDIO_CFG_1_ASE_SNK);
        cfg_mask &= (~AUDIO_CFG_2_ASE_SNK);
    }
    else
    {
        ret = 3;
        goto CHECK_FAIL;
    }

    if (chnl_num == 1)
    {
        cfg_mask &= (~AUDIO_CFG_2_CHL_SNK);
        cfg_mask &= (~AUDIO_CFG_0_CHL_SNK);
    }
    else if (chnl_num == 2)
    {
        cfg_mask &= (~AUDIO_CFG_1_CHL_SNK);
        cfg_mask &= (~AUDIO_CFG_0_CHL_SNK);
    }
    else if (chnl_num == 0)
    {
        cfg_mask &= (~AUDIO_CFG_1_CHL_SNK);
        cfg_mask &= (~AUDIO_CFG_2_CHL_SNK);
    }
    else
    {
        ret = 4;
        goto CHECK_FAIL;
    }

    unicast_src_db.media_cfg_type = UNICAST_AUDIO_CFG_UNKNOWN;
    while (cfg_mask)
    {
        idx++;
        if (cfg_mask & 0x01)
        {
            unicast_src_db.media_cfg_type = (T_UNICAST_AUDIO_CFG_TYPE)idx;
            break;
        }
        cfg_mask >>= 1;
    }

    sup_freq_mask &= sample_freq;
    prefer_codec_msk |= ase_codec_mask;

    if (!le_audio_get_lc3_cfg_index(UNICAST_MEDIA_PLAY_MODE, SERVER_AUDIO_SINK, sup_freq_mask,
                                    frame_snk_duration, prefer_codec_msk, &unicast_src_db.media_snk_prefer_idx))
    {
        unicast_src_db.media_cfg_type =  UNICAST_AUDIO_CFG_UNKNOWN;
        ret = 5;
        goto CHECK_FAIL;
    }

    if (!le_audio_get_avail_context(UNICAST_MEDIA_PLAY_MODE, &avail_context))
    {
        unicast_src_db.media_cfg_type =  UNICAST_AUDIO_CFG_UNKNOWN;
        ret = 6;
        goto CHECK_FAIL;
    }

    if ((teams_get_call_state() == TBS_CALL_STATE_INCOMING) &&
        (avail_context & AUDIO_CONTEXT_RINGTONE))
    {
        unicast_src_db.media_audio_context = AUDIO_CONTEXT_RINGTONE;
    }
    else if (avail_context & AUDIO_CONTEXT_MEDIA)
    {
        unicast_src_db.media_audio_context = AUDIO_CONTEXT_MEDIA;
    }
    else if (avail_context & AUDIO_CONTEXT_UNSPECIFIED)
    {
        unicast_src_db.media_audio_context = AUDIO_CONTEXT_UNSPECIFIED;
    }
    else
    {
        unicast_src_db.media_cfg_type =  UNICAST_AUDIO_CFG_UNKNOWN;
        ret = 7;
        goto CHECK_FAIL;
    }
    unicast_src_db.media_audio_context = AUDIO_CONTEXT_MEDIA;
    APP_PRINT_INFO3("media_prefer_codec cfg_type %x snk_lc3_idx %x context %x",
                    unicast_src_db.media_cfg_type, unicast_src_db.media_snk_prefer_idx,
                    unicast_src_db.media_audio_context);
    return true;
CHECK_FAIL:
    APP_PRINT_ERROR1("le_audio_select_media_prefer_codec ret %d", ret);
    return false;
}

static bool le_audio_select_conversation_prefer_codec(uint8_t qos, bool gaming)
{
    uint32_t snk_sup_freq = 0xFFFFFFFF;
    uint32_t src_sup_freq = 0xFFFFFFFF;
    uint16_t snk_prefer_codec = CODEC_CFG_ITEM_16_2_BIT;
    uint16_t src_prefer_codec = CODEC_CFG_ITEM_16_2_BIT;
    uint8_t snk_duration = FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_10_MS_BIT;
    uint8_t src_duration = FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_10_MS_BIT;
#if UAL_LATENCY_TEST
    snk_prefer_codec = CODEC_CFG_ITEM_48_3_BIT;
    src_prefer_codec = CODEC_CFG_ITEM_16_1_BIT;
    snk_duration = FRAME_DURATION_7_5_MS_BIT;
    src_duration = FRAME_DURATION_7_5_MS_BIT;
#endif

    if (qos == ASE_TARGET_HIGHER_RELIABILITY)
    {
        snk_prefer_codec = CODEC_CFG_ITEM_48_5_BIT | CODEC_CFG_ITEM_48_6_BIT;
        src_prefer_codec = CODEC_CFG_ITEM_16_1_BIT | CODEC_CFG_ITEM_16_2_BIT;
        snk_duration = FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_10_MS_BIT;
        src_duration = FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_10_MS_BIT;
    }
    else
    {
        snk_prefer_codec = CODEC_CFG_ITEM_48_1_BIT | CODEC_CFG_ITEM_48_2_BIT;
        src_prefer_codec = CODEC_CFG_ITEM_16_1_BIT | CODEC_CFG_ITEM_16_2_BIT;
        snk_duration = FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_10_MS_BIT;
        src_duration = FRAME_DURATION_7_5_MS_BIT | FRAME_DURATION_10_MS_BIT;
    }

    if (gaming)
    {
#if TARGET_LE_AUDIO_GAMING_DONGLE
        //le audio gaming dongle use frame duration 10ms
        snk_prefer_codec = CODEC_CFG_ITEM_48_4_BIT;
        src_prefer_codec = CODEC_CFG_ITEM_16_2_BIT;
        snk_duration = FRAME_DURATION_10_MS_BIT;
        src_duration = FRAME_DURATION_10_MS_BIT;
#else
        snk_prefer_codec = CODEC_CFG_ITEM_48_1_BIT;
        src_prefer_codec = CODEC_CFG_ITEM_16_1_BIT;
        snk_duration = FRAME_DURATION_7_5_MS_BIT;
        src_duration = FRAME_DURATION_7_5_MS_BIT;
#endif
    }
    uint8_t src_index = 0;
    uint8_t snk_index = 0;
    uint8_t server_num = 0;
    uint16_t avail_context = 0;
    uint16_t cfg_mask = 0xFFFF;

    uint8_t snk_ase_num = 0;
    uint8_t snk_chnl_num = 0;
    uint16_t snk_ase_mask = 0;
    uint32_t snk_sample_freq = 0;

    uint8_t src_ase_num = 0;
    uint8_t src_chnl_num = 0;
    uint16_t src_ase_mask = 0;
    uint32_t src_sample_freq = 0;
    uint8_t idx = 0;
    uint8_t ret = 0;

    unicast_src_db.conversation_audio_context = AUDIO_CONTEXT_UNSPECIFIED;

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        ret = 1;
        goto CHECK_FAIL;
    }

    server_num = le_audio_get_server_num(UNICAST_CONVERSATION_PLAY_MODE);
    if ((server_num != 1) && (server_num != 2))
    {
        ret = 2;
        goto CHECK_FAIL;
    }

    if (!le_audio_get_cfg_ase_info(UNICAST_CONVERSATION_PLAY_MODE, SERVER_AUDIO_SINK, &snk_ase_num,
                                   &snk_chnl_num, &snk_sample_freq, &snk_ase_mask))
    {
        ret = 3;
        goto CHECK_FAIL;
    }

    if (!le_audio_get_cfg_ase_info(UNICAST_CONVERSATION_PLAY_MODE, SERVER_AUDIO_SOURCE, &src_ase_num,
                                   &src_chnl_num, &src_sample_freq, &src_ase_mask))
    {
        ret = 4;
        goto CHECK_FAIL;
    }

    /* FIXME: By now we only support bap lc3 audio configuration topology */
    if (server_num == 2)
    {
        cfg_mask &= (~AUDIO_CFG_1_SERVER);
    }
    else if (server_num == 1)
    {
        cfg_mask &= (~AUDIO_CFG_2_SERVER);
    }

    if (snk_ase_num == 1)
    {
        cfg_mask &= (~AUDIO_CFG_2_ASE_SNK);
        cfg_mask &= (~AUDIO_CFG_0_ASE_SNK);
    }
    else if (snk_ase_num == 2)
    {
        cfg_mask &= (~AUDIO_CFG_1_ASE_SNK);
        cfg_mask &= (~AUDIO_CFG_0_ASE_SNK);
    }
    else if (snk_ase_num == 0)
    {
        cfg_mask &= (~AUDIO_CFG_1_ASE_SNK);
        cfg_mask &= (~AUDIO_CFG_2_ASE_SNK);
    }
    else
    {
        ret = 4;
        goto CHECK_FAIL;
    }

    if (src_ase_num == 1)
    {
        cfg_mask &= (~AUDIO_CFG_2_ASE_SRC);
        cfg_mask &= (~AUDIO_CFG_0_ASE_SRC);
    }
    else if (src_ase_num == 2)
    {
        cfg_mask &= (~AUDIO_CFG_1_ASE_SRC);
        cfg_mask &= (~AUDIO_CFG_0_ASE_SRC);
    }
    else if (src_ase_num == 0)
    {
        cfg_mask &= (~AUDIO_CFG_1_ASE_SRC);
        cfg_mask &= (~AUDIO_CFG_2_ASE_SRC);
    }
    else
    {
        ret = 5;
        goto CHECK_FAIL;
    }

    APP_PRINT_INFO1("conversation_prefer_codec cfg_mask 0x%x",
                    cfg_mask);

    if (snk_chnl_num == 1)
    {
        cfg_mask &= (~AUDIO_CFG_2_CHL_SNK);
        cfg_mask &= (~AUDIO_CFG_0_CHL_SNK);
    }
    else if (snk_chnl_num == 2)
    {
        cfg_mask &= (~AUDIO_CFG_1_CHL_SNK);
        cfg_mask &= (~AUDIO_CFG_0_CHL_SNK);
    }
    else if (snk_chnl_num == 0)
    {
        cfg_mask &= (~AUDIO_CFG_1_CHL_SNK);
        cfg_mask &= (~AUDIO_CFG_2_CHL_SNK);
    }
    else
    {
        ret = 6;
        goto CHECK_FAIL;
    }

    if (src_chnl_num == 1)
    {
        cfg_mask &= (~AUDIO_CFG_2_CHL_SRC);
        cfg_mask &= (~AUDIO_CFG_0_CHL_SRC);
    }
    else if (src_chnl_num == 2)
    {
        cfg_mask &= (~AUDIO_CFG_1_CHL_SRC);
        cfg_mask &= (~AUDIO_CFG_0_CHL_SRC);
    }
    else if (src_chnl_num == 0)
    {
        cfg_mask &= (~AUDIO_CFG_1_CHL_SRC);
        cfg_mask &= (~AUDIO_CFG_2_CHL_SRC);
    }
    else
    {
        ret = 7;
        goto CHECK_FAIL;
    }

    unicast_src_db.conversation_cfg_type = UNICAST_AUDIO_CFG_UNKNOWN;
    while (cfg_mask)
    {
        idx++;
        if (cfg_mask & 0x01)
        {
            unicast_src_db.conversation_cfg_type = (T_UNICAST_AUDIO_CFG_TYPE)idx;
            break;
        }
        cfg_mask >>= 1;
    }

    APP_PRINT_INFO1("conversation_prefer_codec cfg_type idx 0x%x",
                    unicast_src_db.conversation_cfg_type);
    snk_sup_freq &= snk_sample_freq;
    snk_prefer_codec |= snk_ase_mask;

    if (!le_audio_get_lc3_cfg_index(UNICAST_CONVERSATION_PLAY_MODE, SERVER_AUDIO_SINK, snk_sup_freq,
                                    snk_duration, snk_prefer_codec, &snk_index))
    {
        unicast_src_db.conversation_cfg_type =  UNICAST_AUDIO_CFG_UNKNOWN;
        ret = 8;
        goto CHECK_FAIL;
    }

    src_sup_freq &= src_sample_freq;
    src_prefer_codec |= src_ase_mask;

    if (!le_audio_get_lc3_cfg_index(UNICAST_CONVERSATION_PLAY_MODE, SERVER_AUDIO_SOURCE, src_sup_freq,
                                    src_duration, src_prefer_codec, &src_index))
    {
        unicast_src_db.conversation_cfg_type =  UNICAST_AUDIO_CFG_UNKNOWN;
        ret = 9;
        goto CHECK_FAIL;
    }

    if (!le_audio_get_avail_context(UNICAST_CONVERSATION_PLAY_MODE, &avail_context))
    {
        unicast_src_db.conversation_cfg_type =  UNICAST_AUDIO_CFG_UNKNOWN;
        ret = 10;
        goto CHECK_FAIL;
    }

    if (avail_context & AUDIO_CONTEXT_CONVERSATIONAL)
    {
        unicast_src_db.conversation_audio_context = AUDIO_CONTEXT_CONVERSATIONAL;
    }
    else if (avail_context & AUDIO_CONTEXT_UNSPECIFIED)
    {
        unicast_src_db.conversation_audio_context = AUDIO_CONTEXT_UNSPECIFIED;
    }
    else
    {
        unicast_src_db.conversation_cfg_type =  UNICAST_AUDIO_CFG_UNKNOWN;
        ret = 11;
        goto CHECK_FAIL;
    }
    unicast_src_db.conversation_src_prefer_idx = src_index;
    unicast_src_db.conversation_snk_prefer_idx = snk_index;

    APP_PRINT_INFO4("conversation_prefer_codec cfg_type 0x%x src_lc3_idx %x snk_lc3_idx %x context %x",
                    unicast_src_db.conversation_cfg_type,
                    unicast_src_db.conversation_src_prefer_idx,
                    unicast_src_db.conversation_snk_prefer_idx,
                    unicast_src_db.conversation_audio_context);
    return true;

CHECK_FAIL:
    APP_PRINT_ERROR1("le_audio_select_conversation_prefer_codec ret %d", ret);
    return false;
}

static void le_unicast_src_check_audio(uint8_t *pseudo_addr, uint8_t bd_type)
{
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;
    T_UNICAST_DEV_SETTINGS *settings = NULL;
    T_BLE_AUDIO_DEV_HANDLE dev_handle = NULL;
    T_BT_DEVICE *p_dev = NULL;
    T_LE_AUDIO *p_link = NULL;


    if (ualist_len(&unicast_src_db.active_devs_list) > MAX_DEV_SETTINGS)
    {
        APP_PRINT_ERROR2("le_unicast_src_check_audio: dev set num %u exceeds %u",
                         ualist_len(&unicast_src_db.active_devs_list),
                         MAX_DEV_SETTINGS);
        return;
    }

    p_dev = ual_find_device_by_addr(pseudo_addr);
    if (p_dev == NULL)
    {
        return;
    }

    p_link = ble_audio_find_by_conn_id(p_dev->le_conn_id);
    if (p_link == NULL)
    {
        return;
    }

    if (p_link->csis_disc_done == false)
    {
        return;
    }

    if (!unicast_src_db.group_handle)
    {
        APP_PRINT_ERROR0("le_unicast_src_check_audio: group_handle NULL");
        return;
    }

    dev_handle = ble_audio_group_find_dev(unicast_src_db.group_handle, pseudo_addr, bd_type);
    if (dev_handle == NULL)
    {
        APP_PRINT_WARN2("le_unicast_src_check_audio: group_handle %x, bd_addr %s not find",
                        unicast_src_db.group_handle, TRACE_BDADDR(pseudo_addr));
        bt_dev_le_gatt_disconnect(pseudo_addr, DEV_MGR_APP_ID);
        return;
    }

    /* Check if the dev setting already exists. */
    settings = le_audio_find_added_device(pseudo_addr);

    APP_PRINT_INFO2("le_unicast_src_check_audio: bdaddr %b, settings %p",
                    TRACE_BDADDR(pseudo_addr), settings);

    if (!settings)
    {
        /* TODO: Check if it is in the same coordinated set group with the
         * connected device before. */

        settings = le_audio_find_added_device(bdaddr_reserved);
        if (settings)
        {
            memcpy(settings->bd_addr, pseudo_addr, 6);
        }
    }
}

void le_audio_unicast_handle_pacs_discover(T_BAP_DIS_ALL_DONE *p_data)
{
    T_LE_AUDIO *p_audio_link =  ble_audio_find_by_conn_handle(p_data->conn_handle);
    T_BT_DEVICE *p_dev_rec = NULL;
    if (p_audio_link != NULL)
    {
        p_dev_rec = ual_find_device_by_conn_id(p_audio_link->conn_id);
        if (p_dev_rec &&
            (p_audio_link->remote_serv_sup & LE_AUDIO_ASCS_CLIENT_FLAG) &&
            (p_audio_link->remote_serv_sup & LE_AUDIO_PACS_CLIENT_FLAG) &&
            (p_audio_link->csis_disc_done))
        {
            le_unicast_src_check_audio(p_dev_rec->pseudo_addr,
                                       p_dev_rec->bd_type);
        }

#if (LE_AUDIO_CONNECT_FASTER == 1)
        if (!le_audio_get_device_stream())
        {
            /* set sdu interval as 0 because we cannot determine the value according to pacs records */
            bt_dev_handle_conn_update_event(p_dev_rec->pseudo_addr, CI_EVENT_DISCV_ALL_DONE);
        }
#endif
    }
}

void le_audio_get_ase_info(T_BAP_DIS_ALL_DONE *p_data)
{
    T_LE_AUDIO *p_audio_link = ble_audio_find_by_conn_handle(p_data->conn_handle);
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_handle(p_data->conn_handle);
    if (p_audio_link)
    {
        p_audio_link->remote_serv_sup |= LE_AUDIO_ASCS_CLIENT_FLAG;
        p_audio_link->snk_ase_num = p_data->sink_ase_num;
        p_audio_link->src_ase_num = p_data->source_ase_num;
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

    if (p_dev_rec && p_audio_link &&
        (p_audio_link->remote_serv_sup & LE_AUDIO_ASCS_CLIENT_FLAG) &&
        (p_audio_link->remote_serv_sup & LE_AUDIO_PACS_CLIENT_FLAG) &&
        (p_audio_link->csis_disc_done))
    {

        le_unicast_src_check_audio(p_dev_rec->pseudo_addr,
                                   p_dev_rec->bd_type);
    }
}

static bool le_audio_copy_unicast_usr_cfg(uint8_t *bdaddr, uint8_t mode,
                                          uint8_t num, T_UNICAST_ASE_PARAM *params)
{
    T_UNICAST_DEV_USER_CFG *p_cfg = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_UALIST_HEAD *le_conn_list = NULL;
    T_LE_AUDIO *link = NULL;
    T_BT_DEVICE *rec = NULL;
    T_UNICAST_ASE_PARAM *p_ase = NULL;

    if (!bdaddr || !num || !params)
    {
        APP_PRINT_ERROR3("le_audio_copy_unicast_usr_cfg: invalid params %p %u %p",
                         bdaddr, num, params);
        return false;
    }

    /* copy setted params to connected device */
    le_conn_list = ble_audio_get_conn_list();
    if (!le_conn_list)
    {
        APP_PRINT_ERROR2("le_audio_copy_unicast_usr_cfg: No conns, bdaddr %b mode %u",
                         TRACE_BDADDR(bdaddr), mode);
        return false;
    }

    ualist_for_each_safe(pos, n, le_conn_list)
    {
        link = ualist_entry(pos, T_LE_AUDIO, list);
        if (link->state != BT_CONN_STATE_CONNECTED)
        {
            continue;
        }
        rec = ual_find_device_by_conn_handle(link->conn_handle);
        if (!rec)
        {
            continue;
        }
        if (!memcmp(rec->pseudo_addr, bdaddr, BD_ADDR_LEN))
        {
            continue;
        }
        APP_PRINT_INFO3("le_audio_copy_unicast_usr_cfg copy %b cfg to %b mode %x",
                        TRACE_BDADDR(bdaddr), TRACE_BDADDR(rec->pseudo_addr), mode);
        p_cfg = calloc(1, sizeof(T_UNICAST_DEV_USER_CFG) + num * sizeof(T_UNICAST_ASE_PARAM));
        if (!p_cfg)
        {
            APP_PRINT_ERROR1("le_audio_copy_unicast_usr_cfg: Alloc mem for %b err",
                             TRACE_BDADDR(bdaddr));
            return false;
        }
        init_ualist_head(&p_cfg->list);
        memcpy(p_cfg->bdaddr, rec->pseudo_addr, BD_ADDR_LEN);
        p_cfg->mode = mode;
        p_cfg->params_count = num;
        p_cfg->params = (T_UNICAST_ASE_PARAM *)(p_cfg + 1);
        memcpy(p_cfg->params, params, num * sizeof(T_UNICAST_ASE_PARAM));
        for (uint8_t i = 0; i < num; i++)
        {
            p_ase = &(p_cfg->params[i]);
            if (p_ase->chnl_loc == AUDIO_LOCATION_FL)
            {
                p_ase->chnl_loc = AUDIO_LOCATION_FR;
            }
            else if (p_ase->chnl_loc == AUDIO_LOCATION_FR)
            {
                p_ase->chnl_loc = AUDIO_LOCATION_FL;
            }
        }
        ualist_add_tail(&p_cfg->list, &unicast_src_db.user_cfg_list);
    }
    return true;
}

bool le_audio_add_unicast_user_cfg(uint8_t *bdaddr, uint8_t mode,
                                   uint8_t num, T_UNICAST_ASE_PARAM *params)
{
    T_UNICAST_DEV_USER_CFG *cfg = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;

    if (!bdaddr || !num || !params)
    {
        APP_PRINT_ERROR3("le_audio_add_unicast_user_cfg: invalid params %p %u %p",
                         bdaddr, num, params);
        return false;
    }

    ualist_for_each_safe(pos, n, &unicast_src_db.user_cfg_list)
    {
        cfg = ualist_entry(pos, T_UNICAST_DEV_USER_CFG, list);
        if ((!memcmp(cfg->bdaddr, bdaddr, BD_ADDR_LEN)) && (cfg->mode == mode))
        {
            APP_PRINT_INFO1("le_audio_add_unicast_user_cfg: Same bdaddr %b",
                            TRACE_BDADDR(cfg->bdaddr));
            ualist_del(&cfg->list);
            free(cfg);
        }
        cfg = NULL;
    }

    cfg = calloc(1, sizeof(*cfg) + num * sizeof(*params));
    if (!cfg)
    {
        APP_PRINT_ERROR1("le_audio_add_unicast_user_cfg: Alloc mem for %b err",
                         TRACE_BDADDR(bdaddr));
        return false;
    }
    init_ualist_head(&cfg->list);
    memcpy(cfg->bdaddr, bdaddr, BD_ADDR_LEN);
    cfg->mode = mode;
    cfg->params_count = num;
    cfg->params = (T_UNICAST_ASE_PARAM *)(cfg + 1);
    memcpy(cfg->params, params, num * sizeof(*params));
    ualist_add_tail(&cfg->list, &unicast_src_db.user_cfg_list);

    le_audio_copy_unicast_usr_cfg(bdaddr, mode, num, params);
    return true;
}

bool le_audio_add_unicast_cfg(uint8_t *bd_addr, uint8_t mode, uint8_t param_num,
                              T_UNICAST_ASE_PARAM *param)
{
    T_BT_DEVICE *p_dev_rec = NULL;
    T_LE_AUDIO *p_audio_link;
    T_UNICAST_DEV_SETTINGS *dev_set;
    uint8_t num_set = 0;
    uint8_t i;
    uint8_t ret = 0;

    if (!bd_addr || !param || !param_num)
    {
        ret = 1;
        goto FAIL_NULL;
    }

    if ((unicast_src_db.media_state != AUDIO_STREAM_STATE_IDLE_CONFIGURED &&
         unicast_src_db.media_state != AUDIO_STREAM_STATE_IDLE) ||
        (unicast_src_db.conversation_state != AUDIO_STREAM_STATE_IDLE_CONFIGURED &&
         unicast_src_db.conversation_state != AUDIO_STREAM_STATE_IDLE))
    {
        ret = 2;
        goto FAIL_NULL;
    }

    num_set = ualist_len(&unicast_src_db.active_devs_list);

    p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (!p_dev_rec)
    {
        ret = 3;
        goto FAIL_NULL;
    }

    p_audio_link = ble_audio_find_by_conn_id(p_dev_rec->le_conn_id);
    if (!p_audio_link || p_audio_link->state != BT_CONN_STATE_CONNECTED)
    {
        ret = 4;
        goto FAIL_NULL;
    }

    if (!(p_audio_link->remote_serv_sup & LE_AUDIO_ASCS_CLIENT_FLAG))
    {
        ret = 5;
        goto FAIL_CHECK;
    }

    if (!(p_audio_link->remote_serv_sup & LE_AUDIO_PACS_CLIENT_FLAG))
    {
        ret = 6;
        goto FAIL_CHECK;
    }

    APP_PRINT_INFO3("le_audio_add_unicast_cfg: dev %b, snk_ase_num %u, "
                    "src_ase_num %u", TRACE_BDADDR(bd_addr),
                    p_audio_link->snk_ase_num, p_audio_link->src_ase_num);

    /* Be careful. if the snk_ase_num and src_ase_num are zero, the following
     * parameter assignment would cause buffer overflow.
     * */
    if (param_num > p_audio_link->snk_ase_num + p_audio_link->src_ase_num)
    {
        ret = 7;
        goto FAIL_CHECK;
    }

    dev_set = le_audio_find_added_device(p_dev_rec->pseudo_addr);
    if (!dev_set)
    {
        /* We can not add more dev set. */
        if (num_set >= MAX_DEV_SETTINGS)
        {
            ret = 7;
            goto FAIL_CHECK;
        }

        dev_set = calloc(1, sizeof(T_UNICAST_DEV_SETTINGS) +
                         2 * (p_audio_link->snk_ase_num + p_audio_link->src_ase_num) *
                         sizeof(T_UNICAST_ASE_CB));

        if (!dev_set)
        {
            ret = 8;
            goto FAIL_CHECK;
        }
        dev_set->ase_num = p_audio_link->snk_ase_num + p_audio_link->src_ase_num;
        dev_set->media_ase_cb = (T_UNICAST_ASE_CB *)(dev_set + 1);
        dev_set->conversation_ase_cb = ((T_UNICAST_ASE_CB *)(dev_set + 1)) + dev_set->ase_num;
        /* We always use the connected device address for the first time for
         * dev setting.
         * */
        memcpy(dev_set->bd_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
        ualist_add_tail(&dev_set->list, &unicast_src_db.active_devs_list);
    }
    else
    {
        if (mode == UNICAST_MEDIA_MODE)
        {
            dev_set->media_cfg_ase_num = 0;
            memset(dev_set->media_ase_cb, 0, dev_set->ase_num * sizeof(T_UNICAST_ASE_CB));
        }
        else
        {
            dev_set->conversation_cfg_ase_num = 0;
            dev_set->src_set = false;
            memset(dev_set->conversation_ase_cb, 0, dev_set->ase_num * sizeof(T_UNICAST_ASE_CB));
        }
    }

    for (i = 0; i < param_num; i++)
    {
        param[i].real_ase_id = 0;
        param[i].ase_id_associated = 0;

        if (param[i].direction == SERVER_AUDIO_SOURCE)
        {
            dev_set->src_set = true;
        }

        if (param[i].sample_freq == 0 && param[i].prefer_codec_index != 0xFF)
        {
            switch (param[i].prefer_codec_index)
            {
            case CODEC_CFG_ITEM_8_1:
            case CODEC_CFG_ITEM_8_2:
                {
                    param[i].sample_freq = SAMPLING_FREQUENCY_CFG_8K;
                }
                break;
            case CODEC_CFG_ITEM_16_1:
            case CODEC_CFG_ITEM_16_2:
                {
                    param[i].sample_freq = SAMPLING_FREQUENCY_CFG_16K;
                }
                break;
            case CODEC_CFG_ITEM_24_1:
            case CODEC_CFG_ITEM_24_2:
                {
                    param[i].sample_freq = SAMPLING_FREQUENCY_CFG_24K;
                }
                break;
            case CODEC_CFG_ITEM_32_1:
            case CODEC_CFG_ITEM_32_2:
                {
                    param[i].sample_freq = SAMPLING_FREQUENCY_CFG_32K;
                }
                break;
            case CODEC_CFG_ITEM_441_1:
            case CODEC_CFG_ITEM_441_2:
                {
                    param[i].sample_freq = SAMPLING_FREQUENCY_CFG_44_1K;
                }
                break;
            case CODEC_CFG_ITEM_48_1:
            case CODEC_CFG_ITEM_48_2:
            case CODEC_CFG_ITEM_48_3:
            case CODEC_CFG_ITEM_48_4:
            case CODEC_CFG_ITEM_48_5:
            case CODEC_CFG_ITEM_48_6:
                {
                    param[i].sample_freq = SAMPLING_FREQUENCY_CFG_48K;
                }
                break;
            default:
                break;
            }
        }
        if (mode == UNICAST_MEDIA_MODE)
        {
            dev_set->media_ase_cb[dev_set->media_cfg_ase_num++].param = param[i];
        }
        else
        {
            dev_set->conversation_ase_cb[dev_set->conversation_cfg_ase_num++].param = param[i];
        }
    }
    /* FIXME: It is only allowed to add cfg when the device is connected. */
    /* bt_dev_le_gatt_connect(p_dev_rec->pseudo_addr, p_dev_rec->le_addr_type,
     *                        LE_AUDIO_APP_ID, true);
     */

    return true;
FAIL_NULL:
    APP_PRINT_ERROR7("le_audio_add_unicast_cfg: fail %d, params %p %p %u, states %d %d, device %b", ret,
                     bd_addr, param, param_num,
                     unicast_src_db.media_state, unicast_src_db.conversation_state, TRACE_BDADDR(bd_addr));
    return false;
FAIL_CHECK:
    APP_PRINT_ERROR7("le_audio_add_unicast_cfg: check fail %d, addr %b, snk_ase %u src_ase %u, param num %u, num_set %u, max_num %u",
                     ret, TRACE_BDADDR(bd_addr), p_audio_link->snk_ase_num, p_audio_link->src_ase_num, param_num,
                     num_set, MAX_DEV_SETTINGS);
    return false;

}

bool le_audio_clear_unicast_device_cfg(uint8_t num, uint8_t *p_addrs)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    uint8_t i = 0;
    uint8_t *addr = NULL;

    APP_PRINT_INFO0("le_audio_clear_unicast_device_cfg");

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        return true;
    }

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        if (!dev_set)
        {
            return false;
        }
        if (num)
        {
            for (i = 0; i < num; i++)
            {
                addr = p_addrs + i * BD_ADDR_LEN;
                if (!memcmp(dev_set->bd_addr, addr, BD_ADDR_LEN))
                {
                    APP_PRINT_INFO1("le_audio_clear_unicast_user_cfg: Remove dev %b set",
                                    TRACE_BDADDR(dev_set->bd_addr));
                    ualist_del(&dev_set->list);
                    free(dev_set);
                    break;
                }
            }
        }
        else
        {
            APP_PRINT_INFO1("le_audio_clear_unicast_user_cfg: Remove dev %b set",
                            TRACE_BDADDR(dev_set->bd_addr));
            ualist_del(&dev_set->list);
            free(dev_set);
        }

    }
    return true;
}

bool le_audio_clear_unicast_user_cfg(uint8_t num, uint8_t *p_addrs)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_UNICAST_DEV_USER_CFG *user_cfg = NULL;
    uint8_t *addr = NULL;
    bool rc = false;

    if (!num)
    {
        APP_PRINT_INFO0("le_audio_clear_unicast_user_cfg: Remove all user cfgs");
        ualist_for_each_safe(pos, n, &unicast_src_db.user_cfg_list)
        {
            user_cfg = ualist_entry(pos, T_UNICAST_DEV_USER_CFG, list);
            ualist_del(&user_cfg->list);
            free(user_cfg);
        }
        return le_audio_clear_unicast_device_cfg(0, NULL);
    }

    ualist_for_each_safe(pos, n, &unicast_src_db.user_cfg_list)
    {
        user_cfg = ualist_entry(pos, T_UNICAST_DEV_USER_CFG, list);
        for (uint8_t i = 0; i < num; i++)
        {
            addr = p_addrs + i * BD_ADDR_LEN;
            if (!memcmp(user_cfg->bdaddr, addr, BD_ADDR_LEN))
            {
                APP_PRINT_INFO1("le_audio_clear_unicast_user_cfg: Remove user "
                                "cfg for %b", TRACE_BDADDR(addr));
                rc = le_audio_clear_unicast_device_cfg(1, addr);
                if (!rc)
                {
                    APP_PRINT_ERROR1("le_audio_clear_unicast_user_cfg: Remove "
                                     "user cfg for dev %b err",
                                     TRACE_BDADDR(addr));
                }
                ualist_del(&user_cfg->list);
                free(user_cfg);
                break;
            }
        }
    }
    return true;
}


static void unicast_check_csis_for_reserved_setting(T_BT_DEVICE *dev_rec,
                                                    T_UNICAST_DEV_SETTINGS *settings)
{
    uint8_t num;
    uint8_t i;
    T_AUDIO_DEV_INFO *devs;
    T_BT_DEVICE *rec;
    uint8_t ret = 0;
    uint8_t bdaddr_empty[BD_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    if (!dev_rec || !settings)
    {
        ret = 1;
        goto fail;
    }

    num = ble_audio_group_get_dev_num(unicast_src_db.group_handle);
    if (!num)
    {
        ret = 2;
        goto fail;
    }

    devs = calloc(1, num * sizeof(*devs));
    if (!devs)
    {
        ret = 3;
        goto fail;
    }

    if (!ble_audio_group_get_info(unicast_src_db.group_handle, &num, devs))
    {
        ret = 4;
        goto done;
    }

    APP_PRINT_INFO1("unicast_check_csis_for_reserved_setting: num %u", num);
    for (i = 0; i < num; i++)
    {
        APP_PRINT_INFO4("unicast_check_csis_for_reserved_setting: i %u, addr %b,"
                        " conn_state %x, dev handle %p",
                        i, TRACE_BDADDR(devs[i].bd_addr),
                        devs[i].conn_state, devs[i].dev_handle);
        if (devs[i].conn_state == GAP_CONN_STATE_CONNECTED)
        {
            continue;
        }

        if (!memcmp(devs[i].bd_addr, bdaddr_empty, BD_ADDR_LEN))
        {
            continue;
        }

        /* FIXME: We assume that there are only two devices in the group.
         * We choose the other device that is not connected now and use its
         * address to update the reserved dev settiing address.
         * The address will be used to retrieve the dev handle that will be
         * used to config ase codec.
         * */
        rec = ual_find_device_by_addr(devs[i].bd_addr);
        if (rec && rec != dev_rec)
        {
            memcpy(settings->bd_addr, rec->pseudo_addr, BD_ADDR_LEN);
            ret = 5;
            break;
        }

    }

done:
    APP_PRINT_ERROR2("unicast_check_csis_for_reserved_setting: ret %d, addr %b", ret,
                     TRACE_BDADDR(settings->bd_addr));
    free(devs);
    return;
fail:
    APP_PRINT_ERROR3("unicast_check_csis_for_reserved_setting: fail %d, %p %p %p", ret, dev_rec,
                     settings);
    return;
}

static bool unicast_check_and_alloc_reserved_setting(void)
{
    T_UNICAST_DEV_SETTINGS *first_conn_dev = NULL;
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_BT_DEVICE *dev_rec = NULL;
    T_BT_DEVICE *rec = NULL;
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;
    T_LE_AUDIO *audio_link = NULL;
    T_LE_AUDIO *link = NULL;
    uint8_t srv_num = 0;
    uint8_t dev_cfg_num = 0;

    /* Get the connected devices who has dev setting. */
    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        if (memcmp(dev_set->bd_addr, bdaddr_reserved, 6))
        {
            dev_cfg_num++;
        }

        dev_rec = ual_find_device_by_addr(dev_set->bd_addr);
        if (!dev_rec)
        {
            continue;
        }
        audio_link = ble_audio_find_by_conn_id(dev_rec->le_conn_id);
        if (!audio_link)
        {
            continue;
        }
        else
        {
            if (audio_link->state != BT_CONN_STATE_CONNECTED)
            {
                continue;
            }
            if (!(audio_link->supported_services & LE_AUDIO_CSIS_CLIENT_FLAG))
            {
                continue;
            }
            srv_num++;
            if (!first_conn_dev)
            {
                first_conn_dev = dev_set;
                link = audio_link;
                rec = dev_rec;
            }
        }
    }

    if (dev_cfg_num == 2)
    {
        APP_PRINT_INFO1("unicast_check_and_alloc_reserved_setting: valid dev cfg num %u",
                        dev_cfg_num);
        return true;
    }

    APP_PRINT_INFO1("unicast_check_and_alloc_reserved_setting: srv_num %u",
                    srv_num);

    /* the dev has csis */
    if (srv_num == 1)
    {
        if (link != NULL)
        {
            if (link->csis_size == 1)
            {
                return false;
            }
        }
        T_UNICAST_DEV_SETTINGS *settings = NULL;

        dev_set = first_conn_dev;
        settings = le_audio_find_added_device(bdaddr_reserved);
        if (settings)
        {
            APP_PRINT_INFO0("unicast_check_and_alloc_reserved_setting: Reserved dev already exists");
            unicast_check_csis_for_reserved_setting(rec, settings);
            return true;
        }

        settings = unicast_src_add_reserved_setting(dev_set);
        if (settings)
        {
            APP_PRINT_INFO0("unicast_check_and_alloc_reserved_setting: Reserved dev setting added");
            unicast_check_csis_for_reserved_setting(rec, settings);
        }
        else
        {
            APP_PRINT_ERROR0("unicast_check_and_alloc_reserved_setting: Add reserved setting err");
        }
    }

    return true;
}

T_BLE_AUDIO_DEV_HANDLE unicast_src_get_reserved_dev_handle(void)
{
    uint8_t num;
    T_AUDIO_DEV_INFO *devs;
    uint8_t i;
    T_BLE_AUDIO_DEV_HANDLE handle = NULL;

    num = ble_audio_group_get_dev_num(unicast_src_db.group_handle);

    if (!num)
    {
        APP_PRINT_ERROR0("unicast_src_get_reserved_dev_handle: No handles");
        return NULL;
    }

    devs = calloc(1, num * sizeof(*devs));
    if (!devs)
    {
        APP_PRINT_ERROR0("unicast_src_get_reserved_dev_handle: Alloc devs err");
        return NULL;
    }

    if (!ble_audio_group_get_info(unicast_src_db.group_handle, &num, devs))
    {
        APP_PRINT_ERROR0("unicast_src_get_reserved_dev_handle: Get info err");
        goto err;
    }

    for (i = 0; i < num; i++)
    {
        if (devs[i].conn_state != GAP_CONN_STATE_CONNECTED)
        {
            handle = devs[i].dev_handle;
            free(devs);
            return handle;
        }
    }

err:
    free(devs);

    return NULL;
}

static bool start_media_stream(uint8_t qos, bool gaming)
{
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_BLE_AUDIO_DEV_HANDLE dev_handle = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_BLE_AUDIO_DEV_HANDLE dev_tbl[] = {NULL, NULL};
    uint8_t srv_num = 0;
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;
    uint8_t bdaddr[6];
    uint8_t bdaddr_type;
    uint8_t ret = 0;

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        ret = 1;
        goto FAIL;
    }

    if (ualist_len(&unicast_src_db.active_devs_list) > MAX_DEV_SETTINGS)
    {
        ret = 2;
        goto FAIL;
    }

    /* FIXME: We should find group first */
    if (!unicast_src_check_and_alloc_group())
    {
        ret = 3;
        goto FAIL;
    }

    unicast_check_and_alloc_reserved_setting();

    if (!le_audio_select_media_prefer_codec(qos, gaming))
    {
        ret = 5;
        goto FAIL;
    }

    srv_num = 0;
    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(dev_set->bd_addr);
        if (p_dev_rec == NULL)
        {
            APP_PRINT_ERROR0("start_media_stream device record is empty");
            /* For reserved addr, we proceed. */
            if (memcmp(dev_set->bd_addr, bdaddr_reserved, 6))
            {
                return false;
            }
            memcpy(bdaddr, bdaddr_reserved, 6);
            bdaddr_type = BLE_ADDR_PUBLIC;
        }
        else
        {
            memcpy(bdaddr, p_dev_rec->pseudo_addr, 6);
            bdaddr_type = p_dev_rec->bd_type;
        }
        APP_PRINT_TRACE1("start_media_stream address %s",
                         TRACE_BDADDR(bdaddr));
        /* FIX TODO Check user has set this device to media stream */
        dev_handle = ble_audio_group_find_dev(unicast_src_db.group_handle,
                                              bdaddr, bdaddr_type);
        if (!dev_handle)
        {
            if (unicast_src_db.service_flags & FL_CSIS_EXIST)
            {
                if (memcmp(bdaddr, bdaddr_reserved, 6))
                {
                    APP_PRINT_INFO1("start_media_stream: No dev handle for %b",
                                    TRACE_BDADDR(bdaddr));
                    return false;
                }
                else
                {
                    dev_handle = unicast_src_get_reserved_dev_handle();
                    APP_PRINT_INFO1("start_media_stream: Reserved dev handle %p",
                                    dev_handle);
                }
            }
            else
            {
                dev_handle = ble_audio_group_add_dev(unicast_src_db.group_handle, bdaddr, bdaddr_type);
                APP_PRINT_INFO3("start_media_stream service_flags %x add %b handle %p",
                                unicast_src_db.service_flags, TRACE_BDADDR(bdaddr), dev_handle);
            }
        }

        dev_set->dev_handle = dev_handle;

        if (dev_set->media_cfg_ase_num > 0)
        {
            srv_num++;
        }
        else
        {
            APP_PRINT_WARN1("start_media_stream: device not config media mode, srv_num %d", srv_num);
            continue;
        }

        for (uint8_t i = 0; i < dev_set->media_cfg_ase_num; i++)
        {
            dev_set->media_ase_cb[i].param.real_ase_id = 0;
            dev_set->media_ase_cb[i].param.ase_id_associated = 0;
        }

        if (dev_tbl[0] == NULL)
        {
            dev_tbl[0] = dev_set->dev_handle;
        }
        else
        {
            dev_tbl[1] = dev_set->dev_handle;
        }
    }
    APP_PRINT_INFO2("start_media_stream config type %d, srv_num %d",
                    unicast_src_db.media_cfg_type, srv_num);

    if (unicast_src_db.media_session_handle == NULL)
    {
        unicast_src_db.media_session_handle = audio_stream_session_allocate(unicast_src_db.group_handle);
        if (unicast_src_db.media_session_handle == NULL)
        {
            ret = 7;
            goto FAIL;
        }
    }

    APP_PRINT_INFO1("start_media_stream unicast_src_db.media_state 0x%x", unicast_src_db.media_state);
    if (unicast_src_db.media_state == AUDIO_STREAM_STATE_IDLE)
    {
        if (!bap_unicast_audio_cfg(unicast_src_db.media_session_handle,
                                   unicast_src_db.media_cfg_type,
                                   srv_num, dev_tbl))
        {
            ret = 8;
            goto FAIL;
        }

    }

    T_BAP_UNICAST_SESSION_INFO session_info;
    if (bap_unicast_audio_get_session_info(unicast_src_db.media_session_handle, &session_info))
    {
        APP_PRINT_INFO4("start_media_stream session info: state %d, cfg_type %d, conn_dev_num %d, dev_num %d",
                        session_info.state, session_info.cfg_type,
                        session_info.conn_dev_num, session_info.dev_num);

        if (session_info.state != AUDIO_STREAM_STATE_IDLE)
        {
            if (session_info.cfg_type != unicast_src_db.media_cfg_type)
            {
                bap_unicast_audio_remove_cfg(unicast_src_db.media_session_handle);
                ret = 9;
                goto FAIL;
            }
        }

        for (uint8_t i = 0; i < session_info.dev_num; i++)
        {
            APP_PRINT_INFO4("start_media_stream dev_info[%d]: dev_handle %p, conn_state %d, ase_num %d",
                            i, session_info.dev_info[i].dev_handle,
                            session_info.dev_info[i].conn_state,
                            session_info.dev_info[i].ase_num);

            if (!update_unicast_dev_media_codec_cfg(session_info.dev_info[i].dev_handle,
                                                    session_info.dev_info[i].ase_num,
                                                    session_info.dev_info[i].ase_info))
            {
                ret = 10;
                goto FAIL;
            }
        }
    }

    if (!bap_unicast_audio_start(unicast_src_db.media_session_handle))
    {
        ret = 11;
        goto FAIL;
    }

    le_unicast_src_set_stream_starting_state(true);

    unicast_src_db.play_mode = UNICAST_MEDIA_PLAY_MODE;

    return true;
FAIL:
    APP_PRINT_ERROR4("start_media_stream fail %d, num %u, max_num %u, addr %b",
                     ret, ualist_len(&unicast_src_db.active_devs_list), MAX_DEV_SETTINGS, TRACE_BDADDR(bdaddr));
    return false;
}

bool lea_gaming_update_upstream(bool enable)
{
    T_CODEC_CFG codec_cfg;

    if (enable)
    {
        if (unicast_src_special_game_update_metadata(AUDIO_CONTEXT_CONVERSATIONAL,
                                                     AUDIO_CONTEXT_CONVERSATIONAL))
        {
            APP_PRINT_INFO1("lea_gaming_update_upstream: update %x state success", enable);
        }
        codec_preferred_cfg_get((T_CODEC_CFG_ITEM)unicast_src_db.conversation_src_prefer_idx,
                                &codec_cfg);
        gap_register_direct_cb(le_audio_data_direct_callback);
        return true;
    }
    else
    {
        if (unicast_src_special_game_update_metadata(AUDIO_CONTEXT_UNSPECIFIED, AUDIO_CONTEXT_MEDIA))
        {
            APP_PRINT_INFO1("lea_gaming_update_upstream: update %x state success", enable);
        }
        gap_register_direct_cb(NULL);
        return true;
    }
}


static bool start_conversation_stream(uint8_t qos, bool gaming)
{
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_BLE_AUDIO_DEV_HANDLE dev_handle = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_BLE_AUDIO_DEV_HANDLE dev_tbl[] = {NULL, NULL};
    uint8_t srv_num = 0;
    unicast_src_db.output_chnl_mask = 0;
    uint8_t bdaddr_reserved[6] = BDADDR_NONE;
    uint8_t bdaddr[6];
    uint8_t bdaddr_type;
    uint8_t ret = 0;

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        ret = 1;
        goto FAIL;
    }

    /* FIXME: We should find group first. */
    if (!unicast_src_check_and_alloc_group())
    {
        ret = 2;
        goto FAIL;
    }

    unicast_check_and_alloc_reserved_setting();

    if (!le_audio_select_conversation_prefer_codec(qos, gaming))
    {
        ret = 3;
        goto FAIL;
    }

    srv_num = 0;
    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(dev_set->bd_addr);
        if (p_dev_rec == NULL)
        {
            APP_PRINT_ERROR0("start_conversation_stream device record is empty");
            /* For reserved addr, we proceed. */
            if (memcmp(dev_set->bd_addr, bdaddr_reserved, 6))
            {
                ret = 4;
                goto FAIL;
            }
            memcpy(bdaddr, bdaddr_reserved, 6);
            bdaddr_type = BLE_ADDR_PUBLIC;
        }
        else
        {
            memcpy(bdaddr, p_dev_rec->pseudo_addr, 6);
            bdaddr_type = p_dev_rec->bd_type;
        }

        /* FIX TODO Check user has set this device to media stream */
        dev_handle = ble_audio_group_find_dev(unicast_src_db.group_handle,
                                              bdaddr, bdaddr_type);
        if (!dev_handle)
        {
            if (unicast_src_db.service_flags & FL_CSIS_EXIST)
            {
                if (memcmp(bdaddr, bdaddr_reserved, 6))
                {
                    APP_PRINT_INFO1("start_conversation_stream: No dev handle for %b",
                                    TRACE_BDADDR(bdaddr));
                    return false;
                }
                else
                {
                    dev_handle = unicast_src_get_reserved_dev_handle();
                    APP_PRINT_INFO1("start_conversation_stream: Reserved dev handle %p",
                                    dev_handle);
                }
            }
            else
            {
                dev_handle = ble_audio_group_add_dev(unicast_src_db.group_handle, bdaddr, bdaddr_type);
                APP_PRINT_INFO3("start_conversation_stream service_flags %x add %b handle %p",
                                unicast_src_db.service_flags, TRACE_BDADDR(bdaddr), dev_handle);
            }
        }

        dev_set->dev_handle = dev_handle;

        if (dev_set->conversation_cfg_ase_num > 0)
        {
            srv_num++;
        }
        else
        {
            APP_PRINT_WARN1("start_conversation_stream list device not config media mode, srv_num %d", srv_num);
            continue;
        }

        for (uint8_t i = 0; i < dev_set->conversation_cfg_ase_num; i++)
        {
            dev_set->conversation_ase_cb[i].param.real_ase_id = 0;
            dev_set->conversation_ase_cb[i].param.ase_id_associated = 0;
        }

        /* The device order in table should follow the order in bap topology */
        if ((dev_set->src_set && dev_tbl[1] == NULL) || dev_tbl[0] != NULL)
        {
            dev_tbl[1] = dev_set->dev_handle;
        }
        else
        {
            dev_tbl[0] = dev_set->dev_handle;
        }
    }

    APP_PRINT_INFO2("start_conversation_stream config type %d, srv_num %d",
                    unicast_src_db.conversation_cfg_type, srv_num);

    if (srv_num == 1)
    {
        if (dev_tbl[0] == NULL)
        {
            dev_tbl[0] = dev_tbl[1];
            dev_tbl[1] = NULL;
        }
    }

    if (unicast_src_db.conversation_session_handle == NULL)
    {
        unicast_src_db.conversation_session_handle = audio_stream_session_allocate(
                                                         unicast_src_db.group_handle);
        if (unicast_src_db.conversation_session_handle == NULL)
        {
            ret = 6;
            goto FAIL;
        }
    }

    if (unicast_src_db.conversation_state == AUDIO_STREAM_STATE_IDLE)
    {
        if (!bap_unicast_audio_cfg(unicast_src_db.conversation_session_handle,
                                   unicast_src_db.conversation_cfg_type, srv_num, dev_tbl))
        {
            ret = 7;
            goto FAIL;
        }

    }

    T_BAP_UNICAST_SESSION_INFO session_info;
    if (bap_unicast_audio_get_session_info(unicast_src_db.conversation_session_handle, &session_info))
    {
        if (session_info.state != AUDIO_STREAM_STATE_IDLE)
        {
            if (session_info.cfg_type != unicast_src_db.conversation_cfg_type)
            {
                bap_unicast_audio_remove_cfg(unicast_src_db.conversation_session_handle);
                ret = 8;
                goto FAIL;
            }
        }

        for (uint8_t i = 0; i < session_info.dev_num; i++)
        {
            if (!update_unicast_dev_conversation_codec_cfg(session_info.dev_info[i].dev_handle,
                                                           session_info.dev_info[i].ase_num,
                                                           session_info.dev_info[i].ase_info))
            {
                ret = 9;
                goto FAIL;
            }
        }
    }

    if (!bap_unicast_audio_start(unicast_src_db.conversation_session_handle))
    {
        ret = 10;
        goto FAIL;
    }

    if (unicast_src_db.conversation_src_prefer_idx)
    {
#if (LEA_GAMING_SPECIAL == 1)
        APP_PRINT_INFO2("start_conversation_stream mode %x upstream %x",
                        le_unicast_is_gaming_mode(), app_get_usb_us_state());
        if (gaming)
        {
            if (app_get_usb_us_state())
            {
                gap_register_direct_cb(le_audio_data_direct_callback);
            }
            else
            {
                le_media_player_set_state(MCS_MEDIA_STATE_PLAYING);
                gap_register_direct_cb(NULL);
            }
        }
        else
#endif
        {
            gap_register_direct_cb(le_audio_data_direct_callback);
        }

    }
    le_unicast_src_set_stream_starting_state(true);

    unicast_src_db.play_mode = UNICAST_CONVERSATION_PLAY_MODE;
    return true;
FAIL:
    APP_PRINT_ERROR2("start_conversation_stream: fail %d, addr %b", ret, TRACE_BDADDR(bdaddr));
    return false;
}

void le_audio_disconnect_unicast_device(uint8_t *bdaddr)
{
    if (le_audio_find_added_device(bdaddr) != NULL)
    {
        if ((unicast_src_db.media_state != AUDIO_STREAM_STATE_STARTING &&
             unicast_src_db.media_state != AUDIO_STREAM_STATE_STREAMING &&
             unicast_src_db.media_state != AUDIO_STREAM_STATE_PARTIAL_STREAMING) &&
            (unicast_src_db.conversation_state != AUDIO_STREAM_STATE_STARTING &&
             unicast_src_db.conversation_state != AUDIO_STREAM_STATE_STREAMING &&
             unicast_src_db.conversation_state != AUDIO_STREAM_STATE_PARTIAL_STREAMING))
        {
            bt_dev_le_gatt_disconnect(bdaddr, DEV_MGR_APP_ID);
        }
    }
}

bool le_audio_start_unicast_stream(uint8_t mode, uint8_t qos, bool gaming)
{
    APP_PRINT_INFO4("le_audio_start_unicast_stream play mode %d, media_state %d, conversation_state %d new mode %d",
                    unicast_src_db.play_mode, unicast_src_db.media_state, unicast_src_db.conversation_state, mode);

    if ((mode != UNICAST_MEDIA_PLAY_MODE && mode != UNICAST_CONVERSATION_PLAY_MODE) ||
        (qos == 0 || qos > ASE_TARGET_HIGHER_RELIABILITY))
    {
        return false;
    }

    if (headset_status.headset_source == HEADSET_SOURCE_BT)
    {
        APP_PRINT_TRACE0("le_audio_start_unicast_stream: disallow cis req when headset source in BT mode");
        return false;
    }

#if (LEA_GAMING_SPECIAL == 1)
    if (gaming)
    {
        APP_PRINT_INFO0("le_audio_start_unicast_stream special mode, set to conversation mode");
        mode = UNICAST_CONVERSATION_PLAY_MODE;
    }
#endif
    if (mode == UNICAST_MEDIA_PLAY_MODE)
    {
        if (unicast_src_db.play_mode == UNICAST_CONVERSATION_PLAY_MODE &&
            (unicast_src_db.conversation_state != AUDIO_STREAM_STATE_IDLE_CONFIGURED &&
             unicast_src_db.conversation_state != AUDIO_STREAM_STATE_IDLE))
        {
            if (!le_unicast_stop_stream())
            {
                return false;
            }
            unicast_src_db.play_mode = UNICAST_MEDIA_PLAY_MODE;
            unicast_src_db.qos = qos;
            return true;
        }

        unicast_src_db.qos = qos;
        if (start_media_stream(qos, gaming))
        {
            le_media_player_set_state(MCS_MEDIA_STATE_PLAYING);
            return true;
        }
        return false;
    }
    else
    {
        if (unicast_src_db.play_mode == UNICAST_MEDIA_PLAY_MODE &&
            (unicast_src_db.media_state != AUDIO_STREAM_STATE_IDLE_CONFIGURED &&
             unicast_src_db.media_state != AUDIO_STREAM_STATE_IDLE))
        {
            if (!le_unicast_stop_stream())
            {
                return false;
            }
            unicast_src_db.play_mode = UNICAST_CONVERSATION_PLAY_MODE;
            unicast_src_db.qos = qos;
            return true;
        }
        unicast_src_db.qos = qos;
        return start_conversation_stream(qos, gaming);
    }
}

bool le_audio_user_start_unicast_stream(uint8_t mode, uint8_t qos, bool gaming)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_LE_AUDIO *link = NULL;
    T_UALIST_HEAD *pos2 = NULL;
    T_UALIST_HEAD *n2 = NULL;
    T_UALIST_HEAD *le_conn_list = NULL;
    T_UNICAST_DEV_USER_CFG *cfg = NULL;
    T_BT_DEVICE *rec = NULL;
    bool rc = false;

    le_conn_list = ble_audio_get_conn_list();
    if (!le_conn_list)
    {
        APP_PRINT_ERROR2("le_audio_user_start_unicast_stream: No conns, mode %u, qos %u",
                         mode, qos);
        return false;
    }

    ualist_for_each_safe(pos, n, le_conn_list)
    {
        link = ualist_entry(pos, T_LE_AUDIO, list);
        if (link->state != BT_CONN_STATE_CONNECTED)
        {
            continue;
        }
        rec = ual_find_device_by_conn_handle(link->conn_handle);
        if (!rec)
        {
            continue;
        }
        ualist_for_each_safe(pos2, n2, &unicast_src_db.user_cfg_list)
        {
            cfg = ualist_entry(pos2, T_UNICAST_DEV_USER_CFG, list);
            /* FIXME: We should check if the dev addr is RPA. */
            if ((!memcmp(cfg->bdaddr, rec->pseudo_addr, BD_ADDR_LEN)) && (cfg->mode == mode + 1))
            {
                rc = le_audio_add_unicast_cfg(cfg->bdaddr, cfg->mode,
                                              cfg->params_count, cfg->params);
                if (!rc)
                {
                    le_audio_clear_unicast_device_cfg(0, NULL);
                    return false;
                }
            }
        }
    }

    if (ualist_empty(&unicast_src_db.active_devs_list))
    {
        APP_PRINT_ERROR2("le_audio_user_start_unicast_stream: Add cfg fails, "
                         "mode %u, qos %u", mode, qos);
        return false;
    }

    return le_audio_start_unicast_stream(mode, qos, gaming);
}

bool le_unicast_suspend_stream(void)
{
    bool ret = false;
    APP_PRINT_INFO3("le_unicast_suspend_stream play mode %d, media_state %d, conversation_state %d",
                    unicast_src_db.play_mode, unicast_src_db.media_state, unicast_src_db.conversation_state);
    if (unicast_src_db.play_mode == UNICAST_MEDIA_PLAY_MODE)
    {
        if (unicast_src_db.media_state < AUDIO_STREAM_STATE_STARTING ||
            unicast_src_db.media_state > AUDIO_STREAM_STATE_PARTIAL_STREAMING)
        {
            return true;
        }

        ret = bap_unicast_audio_stop(unicast_src_db.media_session_handle, 10000);
    }
    else if (unicast_src_db.play_mode == UNICAST_CONVERSATION_PLAY_MODE)
    {
        if (unicast_src_db.conversation_state < AUDIO_STREAM_STATE_STARTING ||
            unicast_src_db.conversation_state > AUDIO_STREAM_STATE_PARTIAL_STREAMING)
        {
            return true;
        }
        ret = bap_unicast_audio_stop(unicast_src_db.conversation_session_handle, 10000);
    }
    else
    {
        return false;
    }
    return ret;
}

bool le_unicast_stop_stream(void)
{
    APP_PRINT_INFO3("le_unicast_stop_stream play mode %d, media_state %d, conversation_state %d",
                    unicast_src_db.play_mode, unicast_src_db.media_state, unicast_src_db.conversation_state);
    if (unicast_src_db.play_mode == UNICAST_MEDIA_PLAY_MODE)
    {
        if (unicast_src_db.media_state == AUDIO_STREAM_STATE_IDLE ||
            unicast_src_db.media_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED ||
            unicast_src_db.media_state == AUDIO_STREAM_STATE_RELEASING)
        {
            le_media_player_set_state(MCS_MEDIA_STATE_PAUSED);
            return true;
        }

        if (bap_unicast_audio_release(unicast_src_db.media_session_handle))
        {
            le_media_player_set_state(MCS_MEDIA_STATE_PAUSED);
            return true;
        }
        return false;
    }
    else if (unicast_src_db.play_mode == UNICAST_CONVERSATION_PLAY_MODE)
    {
        if (unicast_src_db.conversation_state == AUDIO_STREAM_STATE_IDLE ||
            unicast_src_db.conversation_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED ||
            unicast_src_db.conversation_state == AUDIO_STREAM_STATE_RELEASING)
        {
            return true;
        }

        return bap_unicast_audio_release(unicast_src_db.conversation_session_handle);
    }
    else
    {
        return false;
    }
}

uint8_t le_unicast_src_play_info(uint8_t *p_qos_type, uint8_t *p_play_mode)
{
    if (p_qos_type)
    {
        *p_qos_type = unicast_src_db.qos;
    }
    if (p_play_mode)
    {
        *p_play_mode = unicast_src_db.play_mode;
    }

    return ((unicast_src_db.media_state > AUDIO_STREAM_STATE_CONFIGURED &&
             unicast_src_db.media_state < AUDIO_STREAM_STATE_STOPPING) ||
            (unicast_src_db.conversation_state > AUDIO_STREAM_STATE_CONFIGURED &&
             unicast_src_db.conversation_state < AUDIO_STREAM_STATE_STOPPING)) ? 0x01 : 0x00;
}

uint8_t le_unicast_src_conn_num(void)
{
    uint8_t dev_num = 0;
    T_UNICAST_DEV_SETTINGS *dev_set = NULL;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_BT_DEVICE *p_dev_rec = NULL;

    ualist_for_each_safe(pos, n, &unicast_src_db.active_devs_list)
    {
        dev_set = ualist_entry(pos, T_UNICAST_DEV_SETTINGS, list);
        p_dev_rec = ual_find_device_by_addr(dev_set->bd_addr);
        if (!p_dev_rec)
        {
            continue;
        }
        APP_PRINT_INFO2("le_unicast_src_conn_num bd_addr %b state %x", TRACE_BDADDR(dev_set->bd_addr),
                        p_dev_rec->ble_state);
        if (p_dev_rec->ble_state == GAP_CONN_STATE_CONNECTED)
        {
            dev_num++;
        }
    }

    APP_PRINT_INFO2("le_unicast_src_conn_num: devs %u, conn_num %u",
                    ualist_len(&unicast_src_db.active_devs_list),
                    dev_num);

    return dev_num;
}

void le_unicast_src_csis_disc_done(T_CSIS_CLIENT_DIS_DONE *dis_done)
{
    uint8_t bd_addr[6];
    uint8_t bd_type;
    T_BT_DEVICE *dev_rec = NULL;
    T_LE_AUDIO *audio_link = NULL;
    uint8_t ret = 0;

    APP_PRINT_INFO1("le_unicast_src_csis_disc_done: dis_done %p", dis_done);

    if (!dis_done)
    {
        return;
    }

    if (!gap_chann_get_addr(dis_done->conn_handle, bd_addr, &bd_type))
    {
        ret = 1;
        goto CHECK_FAIL;
    }

    dev_rec = ual_find_device_by_addr(bd_addr);
    if (!dev_rec)
    {
        ret = 2;
        goto CHECK_FAIL;
    }

    audio_link = ble_audio_find_by_conn_id(dev_rec->le_conn_id);
    if (!audio_link)
    {
        ret = 3;
        goto CHECK_FAIL;

    }

    if (audio_link->state != BT_CONN_STATE_CONNECTED)
    {
        ret = 4;
        goto CHECK_FAIL;
    }

    APP_PRINT_INFO2("le_unicast_src_csis_disc_done: %b csis found %d",
                    TRACE_BDADDR(dev_rec->pseudo_addr), dis_done->is_found);
    if (dis_done->is_found == 0)
    {
        audio_link->csis_disc_done = true;
        if (unicast_src_db.group_handle == 0)
        {
            unicast_src_db.group_handle = ble_audio_group_allocate();
            ble_audio_group_add_dev(unicast_src_db.group_handle, dev_rec->pseudo_addr, bd_type);
            ble_audio_group_reg_cb(unicast_src_db.group_handle, le_audio_unicast_src_group_cb);

#if (LE_EXT_FTL_SUPPORT == 1)
            T_LEA_FTL_DEV ftl_dev;
            memset(&ftl_dev, 0, sizeof(T_LEA_FTL_DEV));
            memcpy(ftl_dev.bd_addr, bd_addr, 0);
            ftl_dev.bd_type = bd_type;
            ftl_dev.csis_support = false;
            lea_ext_save_le_dev(&ftl_dev);
#endif
        }
        else
        {
            APP_PRINT_WARN2("le_unicast_src_csis_disc_done: group_handle %x disconnect %s",
                            unicast_src_db.group_handle, dev_rec->pseudo_addr);
            bt_dev_le_gatt_disconnect(dev_rec->pseudo_addr, DEV_MGR_APP_ID);
            return;
        }
    }
    else
    {
        audio_link->supported_services |= LE_AUDIO_CSIS_CLIENT_FLAG;
        unicast_src_db.service_flags |= FL_CSIS_EXIST;
    }

    if ((audio_link->remote_serv_sup & LE_AUDIO_ASCS_CLIENT_FLAG) &&
        (audio_link->remote_serv_sup & LE_AUDIO_PACS_CLIENT_FLAG) &&
        (audio_link->csis_disc_done))
    {
        le_unicast_src_check_audio(dev_rec->pseudo_addr, dev_rec->bd_type);
    }
    return;

CHECK_FAIL:
    APP_PRINT_ERROR3("le_unicast_src_csis_disc_done: fail %d, conn handle %04x, addr %b",
                     ret, dis_done->conn_handle, TRACE_BDADDR(bd_addr));
    return;
}

bool le_unicast_src_update_group_handle(T_CSIS_CLIENT_SET_MEM_FOUND *p_mem)
{
    if (!p_mem)
    {
        return false;
    }

    T_CSIS_GROUP_INFO csis_info;
    T_BT_DEVICE *p_dev = NULL;
    T_LE_AUDIO *p_link = NULL;
    uint8_t hash[3];

    p_dev = ual_find_device_by_addr(p_mem->bd_addr);
    if (!p_dev)
    {
        return false;
    }

    p_link =  ble_audio_find_by_conn_handle(p_dev->le_conn_handle);
    if (!p_link)
    {
        return false;
    }

    unicast_src_db.service_flags |= FL_CSIS_SIRK;
    p_link->csis_disc_done = true;

    if (unicast_src_db.group_handle)
    {
        if (p_mem->group_handle != unicast_src_db.group_handle)
        {
            le_csis_client_remove_group(p_mem->group_handle);
            ble_audio_group_release(&(p_mem->group_handle));
            bt_dev_le_gatt_disconnect(p_mem->bd_addr, DEV_MGR_APP_ID);
            if (set_coordinator_get_group_info(unicast_src_db.group_handle, &csis_info))
            {
                le_csis_client_gen_local_hash(hash, csis_info.sirk);
                le_csis_client_start_scan(hash);
            }
            return false;
        }
    }

    unicast_src_db.group_handle = p_mem->group_handle;
    if ((p_link->remote_serv_sup & LE_AUDIO_ASCS_CLIENT_FLAG) &&
        (p_link->remote_serv_sup & LE_AUDIO_PACS_CLIENT_FLAG) &&
        (p_link->csis_disc_done))
    {
        le_unicast_src_check_audio(p_dev->pseudo_addr, p_dev->bd_type);
    }
    return true;
}


/*
 * Conditions:
 * The ble link is freed before entering this func. See adapter_msg_cback()
 * The device record might be removed because of bond removing.
 * */
void le_unicast_src_disconnected(uint8_t *pseudo_bdaddr, uint8_t bdaddr_type)
{
    T_LE_AUDIO *link;
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_UALIST_HEAD *le_conn_list;
    uint8_t conn_count = 0;
    uint8_t empty_bdaddr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    APP_PRINT_INFO1("le_unicast_src_disconnected: bdaddr %b",
                    TRACE_BDADDR(pseudo_bdaddr));

    /* Ext Create Conn -> Create Conn Cancel */
    if (!memcmp(empty_bdaddr, pseudo_bdaddr, 6))
    {
        return;
    }

    le_conn_list = ble_audio_get_conn_list();
    if (!le_conn_list)
    {
        return;
    }

    /* Keep the dev setting for reconnection. */

    ualist_for_each_safe(pos, n, le_conn_list)
    {
        link = ualist_entry(pos, T_LE_AUDIO, list);
        if (link->state == BT_CONN_STATE_CONNECTED)
        {
            conn_count++;
        }
    }

    if (conn_count)
    {
        APP_PRINT_INFO1("le_unicast_src_disconnected conn_count %x", conn_count);
        return;
    }

    if (unicast_src_db.group_handle)
    {
        APP_PRINT_INFO2("le_unicast_src_disconnected: Release group %08x, flags %08x",
                        unicast_src_db.group_handle,
                        unicast_src_db.service_flags);
        unicast_src_db.group_handle = NULL;
    }
}

void le_unicast_src_release_group(void)
{
    if (unicast_src_db.group_handle == NULL)
    {
        return;
    }
    if (unicast_src_db.service_flags & FL_CSIS_EXIST)
    {
        le_csis_client_remove_group(unicast_src_db.group_handle);
        unicast_src_db.service_flags &= ~FL_CSIS_EXIST;
    }

    ble_audio_group_release(&(unicast_src_db.group_handle));
    unicast_src_db.group_handle = NULL;
}

void le_unicast_disable_conn_update(bool disable)
{
    disable_conn_update = disable;
}

void le_unicast_conn_update_mgr(T_LE_CONN_UPDATE_EVT evt, uint8_t conn_id)
{
    T_CONN_UPDATE_MGR *elem = NULL;
    uint8_t i = 0;

    if (disable_conn_update)
    {
        return;
    }

    for (i = 0; i < conn_update_mgr_queue.count; i++)
    {
        elem = os_queue_peek(&conn_update_mgr_queue, i);

        if (elem && elem->conn_id == conn_id)
        {
            break;
        }
    }

    if (elem != NULL)
    {
        T_LE_CONN_UPDATE_STATE pre_state = elem->state;
        T_LE_CONN_UPDATE_STATE new_state = elem->state;

        if (evt == CONN_UPDATE_EVENT_UPDATE_MTU)
        {
            elem->mtu_updated = true;
        }
        else if (evt == CONN_UPDATE_EVENT_AUTHEN_COMPLETE)
        {
            elem->auth_cmpl = true;
        }
        else if (evt == CONN_UPDATE_EVENT_DISCV_ALL_DONE)
        {
            elem->discov_done = true;
        }

        if (pre_state == CONN_UPDATE_STATE_CIS_DISCONNECTED)
        {
            if (elem->discov_done == false)
            {
                if (elem->mtu_updated && elem->auth_cmpl)
                {
                    new_state = CONN_UPDATE_STATE_GATT_DISCOV;
                }
            }
            else
            {
                if (evt == CONN_UPDATE_EVENT_CREATE_CIS)
                {
                    new_state = CONN_UPDATE_STATE_CIS_CONNECTED;
                }
            }
        }
        else if (pre_state == CONN_UPDATE_STATE_GATT_DISCOV)
        {
            if (evt == CONN_UPDATE_EVENT_DISCV_ALL_DONE)
            {
                new_state = CONN_UPDATE_STATE_DISCOV_DONE;
            }
        }
        else if (pre_state == CONN_UPDATE_STATE_DISCOV_DONE)
        {
            if (evt == CONN_UPDATE_EVENT_CREATE_CIS)
            {
                new_state = CONN_UPDATE_STATE_CIS_CONNECTED;
            }
        }
        else if (pre_state == CONN_UPDATE_STATE_CIS_CONNECTED)
        {
            if (evt == CONN_UPDATE_EVENT_CIS_DISCONNECT)
            {
                new_state = CONN_UPDATE_STATE_CIS_DISCONNECTED;
            }
        }

        elem->state = new_state;

        if (headset_status.phone_status != PHONE_STREAM_IDLE)
        {
            APP_PRINT_TRACE1("le_unicast_conn_update_mgr: disallow conn update when bt streaming for conn_id",
                             conn_id);
        }
        else
        {
            if (memcmp(&elem->curr_active_update_info, &conn_update_info[new_state],
                       sizeof(T_CONN_UPDATE_INFO)))
            {
                APP_PRINT_TRACE8("le_unicast_conn_update_mgr: conn_id %d evt %d state (%d->%d) ci %d ce %d sv_timeout %d conn_latency %d",
                                 conn_id, evt, pre_state, new_state,
                                 conn_update_info[new_state].conn_interval,
                                 conn_update_info[new_state].ce_length, conn_update_info[new_state].supervision_timeout,
                                 conn_update_info[new_state].conn_latency);

                le_update_conn_param(elem->conn_id,
                                     conn_update_info[new_state].conn_interval,
                                     conn_update_info[new_state].conn_interval,
                                     conn_update_info[new_state].conn_latency,
                                     conn_update_info[new_state].supervision_timeout,
                                     conn_update_info[new_state].ce_length,
                                     conn_update_info[new_state].ce_length);

                memcpy(&elem->curr_active_update_info, &conn_update_info[new_state], sizeof(T_CONN_UPDATE_INFO));
            }
            else
            {
                APP_PRINT_TRACE2("le_unicast_conn_update_mgr: same update info state (%d->%d)", pre_state,
                                 new_state);
            }
        }
    }
}

static void le_unicast_set_phy_2M(uint8_t conn_id)
{
    uint8_t all_phys = GAP_PHYS_PREFER_ALL;
    uint8_t tx_phys = GAP_PHYS_PREFER_2M_BIT;
    uint8_t rx_phys = GAP_PHYS_PREFER_2M_BIT;
    T_GAP_PHYS_OPTIONS phy_options = GAP_PHYS_OPTIONS_CODED_PREFER_S8;

    le_set_phy(conn_id, all_phys, tx_phys, rx_phys, phy_options);
}

void le_unicast_handle_gap_msg(void *p_msg)
{
    T_IO_MSG *p_gap_msg = (T_IO_MSG *)p_msg;
    T_LE_GAP_MSG gap_msg;

    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            T_GAP_CONN_STATE new_state = (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state;
            uint8_t conn_id = gap_msg.msg_data.gap_conn_state_change.conn_id;
            T_CONN_UPDATE_MGR *elem = NULL;

            APP_PRINT_TRACE2("le_unicast_handle_gap_msg: GAP_MSG_LE_CONN_STATE_CHANGE new_state %d conn_id %d",
                             new_state, conn_id);

            if (new_state == GAP_CONN_STATE_CONNECTED)
            {
                elem = calloc(1, sizeof(T_CONN_UPDATE_MGR));

                if (elem != NULL)
                {
                    elem->conn_id = conn_id;
                    elem->state = CONN_UPDATE_STATE_CIS_DISCONNECTED;

                    os_queue_in(&conn_update_mgr_queue, elem);
                }

                le_unicast_set_phy_2M(conn_id);
            }
            else if (new_state == GAP_CONN_STATE_DISCONNECTED)
            {
                uint8_t i = 0;

                for (i = 0; i < conn_update_mgr_queue.count; i++)
                {
                    elem = os_queue_peek(&conn_update_mgr_queue, i);

                    if (elem && elem->conn_id == conn_id)
                    {
                        os_queue_delete(&conn_update_mgr_queue, elem);
                        free(elem);
                    }
                }
            }
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            uint8_t conn_id = gap_msg.msg_data.gap_conn_mtu_info.conn_id;

            le_unicast_conn_update_mgr(CONN_UPDATE_EVENT_UPDATE_MTU, conn_id);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            uint8_t conn_id = gap_msg.msg_data.gap_authen_state.conn_id;
            uint16_t cause = gap_msg.msg_data.gap_authen_state.status;

            if (cause == GAP_SUCCESS)
            {
                le_unicast_conn_update_mgr(CONN_UPDATE_EVENT_AUTHEN_COMPLETE, conn_id);
            }
        }
        break;

    default:
        break;
    }

}

void le_unicast_src_service_init(T_SERVER_ID mcs_id, T_SERVER_ID ccp_id)
{
    memset(&unicast_src_db, 0, sizeof(T_UNICAST_SRC_DB));
    init_ualist_head(&unicast_src_db.active_devs_list);
    init_ualist_head(&unicast_src_db.user_cfg_list);
    unicast_src_db.mcs_id = mcs_id;
    unicast_src_db.ccp_id = ccp_id;
#if (LE_AUDIO_GAMING_SUPPORT == 1)
    lea_gaming_init();

    le_unicast_disable_conn_update(true);
#endif

    ble_conn_update_by_app(true);

    os_queue_init(&conn_update_mgr_queue);
}
