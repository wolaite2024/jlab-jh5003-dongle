/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <string.h>
#include <stdlib.h>
#include "gap.h"
#include "btm.h"
#include "trace.h"
#include "gap_vendor.h"
#include "cfg_item_api.h"
#include "app_vendor.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_timer.h"
#include "app_cfg.h"
#include "app_lea_ascs.h"
#include "app_lea_broadcast_audio.h"
#include "app_link_util.h"
#include "app_sniff_mode.h"
#include "app_a2dp.h"
#include "app_roleswap_control.h"
#include "app_link_util.h"
#include "gap_conn_le.h"
#include "app_lea_unicast_audio.h"

#if F_APP_B2S_HTPOLL_SUPPORT
#include "app_hfp.h"
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
#include "app_dongle_common.h"
#include "app_dongle_dual_mode.h"
#include "app_ble_common_adv.h"
#if F_APP_B2B_ENGAGE_REDUCE_NSE
#include "transmit_svc_dongle.h"
#endif
#endif

#define HCI_VENDOR_SET_ADV_EXT_MISC 0xFC98
#define HCI_VENDOR_SET_ADV_EXT_MISC_SUBCODE 0x00

#define HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY 0xFD80
#define HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY_SUBCODE 0x1C

#define HCI_BR_VENDOR_EXTENSION_FEATURE         0xFD81
#define HCI_EXT_SUB_ENABLE_SEND_TX_RIGHT_AWATY  0x22
#define HCI_EXT_SUB_SET_REPORT_NEW_CONN_DEVICE  0x27
#define HCI_EXT_SUB_DYNAMIC_SET_CIS_VND_CMD     0x28

#define HCI_VENDOR_REPORT_NEW_CONN_DEVICE_SUBEVENT   0x79

typedef enum
{
    APP_TIMER_RESEND_HTPOLL,
    APP_TIMER_LEA_SET_PREFER_QOS,
} T_APP_VENDOR_TIMER;

static uint8_t app_vendor_timer_id = 0;

#if F_APP_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
typedef struct
{
    bool     is_b2b_htpoll;
    T_BT_TRAFFIC_QOS_TYPE qos_type;
    uint16_t interval;
    uint8_t  rsv_slot;
    uint16_t idle_zone_mask;
} T_HTPOLL_PARAM;

typedef struct
{
    uint8_t addr[6];
    T_HTPOLL_PARAM param;
} T_HTPOLL_INFO;

typedef struct htpoll_tgt
{
    struct htpoll_list *next;
    bool is_b2b_link;
    T_HTPOLL_INFO info;
} T_HTPOLL_TGT;

typedef struct htpoll_work
{
    struct htpoll_work      *next;
    bool                    set;
    bool                    applying;
    uint8_t                 addr[6];
    T_HTPOLL_PARAM          param;
} T_HTPOLL_WORK;

#if F_APP_B2B_HTPOLL_CONTROL
#define B2S_HTPOLL_RSV_SLOT_FOR_B2B_HTPOLL  4

typedef enum
{
    B2B_HTPOLL_NONE,
    B2B_HTPOLL_CIS_B2B_COEXIST,
    B2B_HTPOLL_BIS_B2B_COEXIST,
} T_B2B_HTPOLL_TYPE;

static const T_HTPOLL_PARAM b2b_htpoll_db[] =
{
    [B2B_HTPOLL_CIS_B2B_COEXIST] = {true, BT_NEGOTIATE_PERIODIC_TRAFFIC_QOS, 64, 2, TRAFFIC_MASK_ALL_DISABLE},
    [B2B_HTPOLL_BIS_B2B_COEXIST] = {true, BT_NEGOTIATE_PERIODIC_TRAFFIC_QOS, 64, 2, TRAFFIC_MASK_ALL_DISABLE},
};

static bool b2b_connected = false;
#endif

static T_OS_QUEUE htpoll_work_queue;
static T_OS_QUEUE htpoll_tgt_queue;
static uint8_t timer_idx_htpoll_resend = 0;

static bool app_vendor_htpoll_exist(uint8_t *addr, T_HTPOLL_PARAM param);
static bool app_vendor_htpoll_set(bool set, uint8_t *addr, T_HTPOLL_PARAM param);

#if F_APP_B2S_HTPOLL_SUPPORT
typedef struct
{
    T_HTPOLL_PARAM param;
    uint8_t  gaming_latency_plus;
} T_GAMING_B2S_HTPOLL_DB;

typedef enum
{
    B2S_HTPOLL_NONE,
    B2S_HTPOLL_STREAMING_WITH_SINGLE_SRC,
    B2S_HTPOLL_STREAMING_WITH_MULTI_SRC,
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    B2S_HTPOLL_GAMING_LINKBACK,
    B2S_HTPOLL_GAMING_BLE_ADV,
    B2S_HTPOLL_GAMING_LEA_SET_PREFER_QOS,
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
    GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX,
    GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX_WITH_SCO,
#endif
#endif
} T_GAMING_B2S_HTPOLL_TYPE;

static const T_GAMING_B2S_HTPOLL_DB gaming_b2s_htpoll_db[] =
{
    [B2S_HTPOLL_STREAMING_WITH_SINGLE_SRC]               = {{false, BT_NON_NEGOTIATE_PERIODIC_TRAFFIC_QOS,   0,    0, TRAFFIC_MASK_ALL_DISABLE},   0},
    [B2S_HTPOLL_STREAMING_WITH_MULTI_SRC]                = {{false, BT_NON_NEGOTIATE_PERIODIC_TRAFFIC_QOS,  58,   50, TRAFFIC_MASK_ALL_DISABLE},   5},
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    [B2S_HTPOLL_GAMING_LINKBACK]                         = {{false, BT_NON_NEGOTIATE_PERIODIC_TRAFFIC_QOS,  30,   22, TRAFFIC_MASK_ALL_DISABLE},   20},
    [B2S_HTPOLL_GAMING_BLE_ADV]                          = {{false, BT_NON_NEGOTIATE_PERIODIC_TRAFFIC_QOS,  58,   50, TRAFFIC_MASK_ALL_DISABLE},   5},
    [B2S_HTPOLL_GAMING_LEA_SET_PREFER_QOS]               = {{false, BT_NON_NEGOTIATE_PERIODIC_TRAFFIC_QOS,  30,   22, TRAFFIC_MASK_ALL_DISABLE},   20},
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
    [GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX]          = {{false, BT_NEGOTIATE_PERIODIC_TRAFFIC_QOS,      16,    8, TRAFFIC_MASK_ALL_DISABLE},   22},
    [GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX_WITH_SCO] = {{false, BT_PERIODIC_SYNC_TRAFFIC_QOS,           16,    6, TRAFFIC_MASK_ALL_DISABLE},   22},
#endif
#endif
};

static uint8_t timer_idx_lea_set_prefer_qos = 0;

uint8_t app_vendor_get_htpoll_latency_plus(void)
{
    uint8_t ret = 0;
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;
    bool is_dongle_link_htpoll = false;

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL && htpoll_tgt->info.param.rsv_slot != 0)
        {
            if (app_link_check_dongle_link(htpoll_tgt->info.addr))
            {
                is_dongle_link_htpoll = true;
                break;
            }
        }
    }

    if (is_dongle_link_htpoll)
    {
        for (i = 0; i < sizeof(gaming_b2s_htpoll_db) / sizeof(T_GAMING_B2S_HTPOLL_DB); i++)
        {
            if (gaming_b2s_htpoll_db[i].param.interval == htpoll_tgt->info.param.interval &&
                gaming_b2s_htpoll_db[i].param.rsv_slot == htpoll_tgt->info.param.rsv_slot)
            {
                ret = gaming_b2s_htpoll_db[i].gaming_latency_plus;
                break;
            }
        }
    }

    APP_PRINT_TRACE2("app_vendor_get_htpoll_latency_plus: is_dongle_link_htpoll %d ret %d",
                     is_dongle_link_htpoll, ret);

    return ret;
}

static bool app_vendor_b2s_htpoll_info_check(T_HTPOLL_INFO *info)
{
    uint8_t connected_b2s_num = app_link_get_b2s_link_num();
    uint8_t connected_le_num = app_link_get_le_link_num();
    uint8_t connected_device_num = 0;
    T_GAMING_B2S_HTPOLL_TYPE htpoll_type = B2S_HTPOLL_NONE;
    bool ret = false;
    bool is_a2dp_streaming = app_a2dp_is_streaming();
    bool audio_mixing = false;
    uint8_t cause = 0;
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    T_APP_BR_LINK *p_dongle_link = app_dongle_get_connected_dongle_link();
#endif

    connected_device_num = connected_b2s_num + connected_le_num;

    if (is_a2dp_streaming && app_hfp_sco_is_connected() == false)
    {
        if (app_a2dp_get_streaming_num() == 1)
        {
            uint8_t idx;

            if (app_a2dp_get_streaming_idx(&idx))
            {
                memcpy(info->addr, app_db.br_link[idx].bd_addr, 6);
            }
        }
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT && F_APP_GAMING_CHAT_MIXING_SUPPORT
        else if (p_dongle_link && p_dongle_link->streaming_fg)
        {
            audio_mixing = true;
            memcpy(info->addr, p_dongle_link->bd_addr, 6);
        }
#endif

        if (0)
        {}
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
#if F_APP_SUPPORT_LINKBACK_WHEN_DONGLE_STREAMING
        else if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G_BT &&
                 app_dongle_get_state() == DONGLE_STATE_LINKBACK)
        {
            htpoll_type = B2S_HTPOLL_GAMING_LINKBACK;
            cause = 1;
        }
#endif
        else if (timer_idx_lea_set_prefer_qos != 0)
        {
            htpoll_type = B2S_HTPOLL_GAMING_LEA_SET_PREFER_QOS;
            cause = 2;
        }
        else if (app_cfg_nv.legacy_gaming_support_common_adv)
        {
            if (app_ble_common_adv_get_state() == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                htpoll_type = B2S_HTPOLL_GAMING_BLE_ADV;
                cause = 3;
            }
        }
#endif
        else if (connected_device_num == 1)
        {
            htpoll_type = B2S_HTPOLL_STREAMING_WITH_SINGLE_SRC;
            cause = 4;
        }
        else if (connected_device_num > 1)
        {
            htpoll_type = B2S_HTPOLL_STREAMING_WITH_MULTI_SRC;
            cause = 5;

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT && F_APP_GAMING_CHAT_MIXING_SUPPORT
            if (p_dongle_link)
            {
                if (audio_mixing)
                {
                    memcpy(info->addr, p_dongle_link->bd_addr, 6);
                    htpoll_type = GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX;
                    cause = 6;
                }
                /* phone down streaming and dongle mic upstreaming */
                else if (!p_dongle_link->streaming_fg && app_db.dongle_is_enable_mic)
                {
                    memcpy(info->addr, p_dongle_link->bd_addr, 6);
                    htpoll_type = GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX;
                    cause = 7;
                }
                else if (p_dongle_link->streaming_fg)
                {
                    /* set negotiate htpoll */
                    htpoll_type = GAMING_B2S_HTPOLL_MULTI_SRC_DONGLE_STREAMING;
                    cause = 8;
                }
            }
#endif
        }
    }
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT && F_APP_GAMING_CHAT_MIXING_SUPPORT
    else if (p_dongle_link && p_dongle_link->streaming_fg && app_hfp_sco_is_connected())
    {
        memcpy(info->addr, p_dongle_link->bd_addr, 6);
        htpoll_type = GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX_WITH_SCO;
        cause = 9;
    }
#endif

    if (gaming_b2s_htpoll_db[htpoll_type].param.rsv_slot != 0)
    {
        memcpy(&info->param, &gaming_b2s_htpoll_db[htpoll_type].param, sizeof(T_HTPOLL_PARAM));
        ret = true;

        APP_PRINT_TRACE6("app_vendor_gaming_htpoll_info_check: addr %s htpoll_type %d qos_type %d htpoll (%d:%d) cause %d",
                         TRACE_BDADDR(info->addr), htpoll_type,
                         info->param.qos_type,
                         info->param.interval,
                         info->param.rsv_slot, cause);
    }
    else
    {
        APP_PRINT_TRACE0("app_vendor_gaming_htpoll_info_check: clear htpoll");
    }

    return ret;
}
#endif

#if F_APP_B2B_HTPOLL_CONTROL
bool app_vendor_htpoll_execing(void)
{
    bool ret = false;

    if (htpoll_work_queue.count > 0)
    {
        ret = true;
    }

    return ret;
}

static bool app_vendor_b2b_htpoll_info_check(T_HTPOLL_INFO *info)
{
    uint8_t i = 0;
#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT
    bool local_cis_connected = false;
    uint16_t conn_interval = 0;
    T_APP_LE_LINK *p_lea_active_link = app_lea_uca_get_stream_link();

    if (p_lea_active_link != NULL)
    {
        local_cis_connected = app_lea_ascs_cis_exist();
        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, p_lea_active_link->conn_id);
    }

    bool cis_connected = local_cis_connected || app_db.remote_cis_connected;
#endif
#if F_APP_TMAP_BMR_SUPPORT
    bool bis_connected = app_lea_bca_bis_exist();
#endif
    bool ret = false;
    T_B2B_HTPOLL_TYPE htpoll_type = B2B_HTPOLL_NONE;

    if (0)
    {}
#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT
    else if (b2b_connected && cis_connected)
    {
        htpoll_type = B2B_HTPOLL_CIS_B2B_COEXIST;
    }
#endif
#if F_APP_TMAP_BMR_SUPPORT
    else if (b2b_connected && bis_connected)
    {
        htpoll_type = B2B_HTPOLL_BIS_B2B_COEXIST;
    }
#endif

    if (b2b_htpoll_db[htpoll_type].rsv_slot != 0)
    {
        memcpy(info->addr, app_cfg_nv.bud_peer_addr, 6);

        memcpy(&info->param, &b2b_htpoll_db[htpoll_type], sizeof(T_HTPOLL_PARAM));

        if (local_cis_connected)
        {
            info->param.interval = conn_interval * 2; // conn_interval: slot pair
        }
        else if (app_db.remote_cis_connected)
        {
            info->param.interval = app_db.remote_le_acl_interval * 2;
        }

        ret = true;

        APP_PRINT_TRACE3("app_vendor_b2b_htpoll_info_check: type %d param (%d:%d)",
                         htpoll_type, info->param.interval, info->param.rsv_slot);
    }

    return ret;
}

/* set all b2s link with random traffic htpoll */
static void app_vendor_htpoll_set_b2s_random_htpoll()
{
    bool local_cis_connected = false;
    uint16_t conn_interval = 0;
    T_APP_LE_LINK *p_lea_active_link = app_lea_uca_get_stream_link();

    if (p_lea_active_link != NULL)
    {
        local_cis_connected = app_lea_ascs_cis_exist();
        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, p_lea_active_link->conn_id);
    }

    /* for b2b htpoll we also to set b2s htpoll */
    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_link_check_b2s_link_by_id(i))
        {
            T_HTPOLL_PARAM param = {0};

            if (local_cis_connected)
            {
                param.interval = conn_interval * 2;
            }
            else if (app_db.remote_cis_connected)
            {
                param.interval = app_db.remote_le_acl_interval * 2;
            }
            else
            {
                //should not be here.
                break;
            }

            param.rsv_slot = B2S_HTPOLL_RSV_SLOT_FOR_B2B_HTPOLL;

            if (app_vendor_htpoll_exist(app_db.br_link[i].bd_addr, param) == false)
            {
                app_vendor_htpoll_set(true, app_db.br_link[i].bd_addr, param);

                /* maximum one b2s htpoll allowable */
                break;
            }
        }
    }
}
#endif

static void app_vendor_htpoll_print(void)
{
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL)
        {
            APP_PRINT_TRACE6("app_vendor_htpoll_print: addr %s type %d is_b2b_link %d is_b2b_htpoll %d (%d:%d)",
                             TRACE_BDADDR(htpoll_tgt->info.addr),
                             htpoll_tgt->info.param.qos_type,
                             htpoll_tgt->is_b2b_link,
                             htpoll_tgt->info.param.is_b2b_htpoll,
                             htpoll_tgt->info.param.interval, htpoll_tgt->info.param.rsv_slot);
        }
    }
}

static void app_vendor_htpoll_tgt_add(uint8_t *addr)
{
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;
    bool exist = false;

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL && !memcmp(htpoll_tgt->info.addr, addr, 6))
        {
            exist = true;
            break;
        }
    }

    if (exist == false)
    {
        htpoll_tgt = calloc(1, sizeof(T_HTPOLL_TGT));

        if (htpoll_tgt != NULL)
        {
            if (app_link_check_b2b_link(addr))
            {
                htpoll_tgt->is_b2b_link = true;
            }

            memcpy(htpoll_tgt->info.addr, addr, 6);
            os_queue_in(&htpoll_tgt_queue, htpoll_tgt);
        }
    }
}

static void app_vendor_htpoll_tgt_remove(uint8_t *addr)
{
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL && !memcmp(htpoll_tgt->info.addr, addr, 6))
        {
            os_queue_delete(&htpoll_tgt_queue, htpoll_tgt);
            free(htpoll_tgt);
        }
    }
}

static void app_vendor_htpoll_tgt_replace_b2b_addr(uint8_t *addr)
{
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL && htpoll_tgt->is_b2b_link)
        {
            memcpy(htpoll_tgt->info.addr, addr, 6);
        }
    }
}

static T_HTPOLL_TGT *app_vendor_htpoll_tgt_get(uint8_t *addr)
{
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;
    T_HTPOLL_TGT *ret = NULL;
    bool get_htpoll = false;

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL && !memcmp(htpoll_tgt->info.addr, addr, 6))
        {
            get_htpoll = true;
            break;
        }
    }

    if (get_htpoll)
    {
        ret = htpoll_tgt;
    }

    return ret;
}

static void app_vendor_htpoll_tgt_update(uint8_t *addr, T_HTPOLL_PARAM param)
{
    T_HTPOLL_TGT *htpoll_tgt = NULL;

    for (uint8_t i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL && !memcmp(htpoll_tgt->info.addr, addr, 6))
        {
            memcpy(&htpoll_tgt->info.param, &param, sizeof(T_HTPOLL_PARAM));
            break;
        }
    }
}

static bool app_vendor_htpoll_exist_sniff(uint8_t *addr)
{
    T_APP_BR_LINK *p_link = NULL;
    bool ret = false;

    p_link = app_link_find_br_link(addr);

    if (p_link != NULL && p_link->acl_link_in_sniffmode_flg)
    {
        if (app_link_check_b2s_link(addr))
        {
            app_sniff_mode_b2s_disable(addr, SNIFF_DISABLE_MASK_GAMINGMODE_DONGLE);
        }
        else
        {
            app_sniff_mode_b2b_disable(addr, SNIFF_DISABLE_MASK_GAMINGMODE_DONGLE);
        }

        ret = true;

        APP_PRINT_TRACE1("app_vendor_htpoll_exist_sniff: addr %s", TRACE_BDADDR(addr));
    }

    return ret;
}

static void app_vendor_htpoll_sco_work_remove(void)
{
    T_HTPOLL_WORK *htpoll_work = NULL;

    APP_PRINT_TRACE0("app_vendor_htpoll_sco_work_remove");

    for (uint8_t i = 0; i < htpoll_work_queue.count; i++)
    {
        htpoll_work = os_queue_peek(&htpoll_work_queue, i);

        if (htpoll_work != NULL &&
            htpoll_work->param.qos_type == BT_PERIODIC_SYNC_TRAFFIC_QOS)
        {
            os_queue_delete(&htpoll_work_queue, htpoll_work);
            free(htpoll_work);
        }
    }
}

static void app_vendor_htpoll_work_remove(uint8_t *addr)
{
    T_HTPOLL_WORK *htpoll_work = NULL;

    for (uint8_t i = 0; i < htpoll_work_queue.count; i++)
    {
        htpoll_work = os_queue_peek(&htpoll_work_queue, i);

        if (htpoll_work != NULL && !memcmp(htpoll_work->addr, addr, 6))
        {
            os_queue_delete(&htpoll_work_queue, htpoll_work);
            free(htpoll_work);
        }
    }
}

static void app_vendor_htpoll_exec(void)
{
    T_HTPOLL_WORK *working = os_queue_peek(&htpoll_work_queue, 0);
    bool ret = false;
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
    T_APP_BR_LINK *p_sco_link = app_dongle_get_connected_phone_link();
#endif

    while (working != NULL && working->applying == false &&
           !app_vendor_htpoll_exist_sniff(working->addr) && timer_idx_htpoll_resend == 0)
    {
        if (working->applying == false)
        {
            APP_PRINT_TRACE5("app_vendor_htpoll_exec: set %d qos_type %d addr %s (%d:%d)",
                             working->set, working->param.qos_type, TRACE_BDADDR(working->addr),
                             working->param.interval, working->param.rsv_slot);

            if (working->set)
            {
                T_BT_TRAFFIC_QOS_PARAM param;

                if (working->param.qos_type == BT_NEGOTIATE_PERIODIC_TRAFFIC_QOS)
                {
                    param.u.negotiate_periodic_param.interval = working->param.interval;
                    param.u.negotiate_periodic_param.rsvd_slots = working->param.rsv_slot;
                }
                else if (working->param.qos_type == BT_NON_NEGOTIATE_PERIODIC_TRAFFIC_QOS)
                {
                    param.u.none_negotiate_periodic_param.interval = working->param.interval;
                    param.u.none_negotiate_periodic_param.rsvd_slots = working->param.rsv_slot;
                    param.u.none_negotiate_periodic_param.traffic_mask = working->param.idle_zone_mask;
                }
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
                else if (working->param.qos_type == BT_PERIODIC_SYNC_TRAFFIC_QOS)
                {
                    if (p_sco_link && bt_link_periodic_traffic_set(p_sco_link->bd_addr))
                    {
                        param.u.periodic_sync_param.sync_weighting = working->param.interval;
                        param.u.periodic_sync_param.rsvd_slots = working->param.rsv_slot;
                        param.u.periodic_sync_param.traffic_mask = working->param.idle_zone_mask;
                    }
                }
#endif
                else
                {
                    param.u.random_param.rsvd_slots = working->param.rsv_slot;
                    param.u.random_param.traffic_mask = working->param.idle_zone_mask;
                }

                ret = bt_link_traffic_qos_set(working->addr, working->param.qos_type, param);

#if F_APP_GAMING_CHAT_MIXING_SUPPORT
                if (working->param.qos_type == BT_PERIODIC_SYNC_TRAFFIC_QOS && ret)
                {
                    ret = app_dongle_mixing_set_gaming_hfp_coex();
                }
#endif
            }
            else
            {
                ret = bt_link_traffic_qos_clear(working->addr, working->param.qos_type);
            }

            if (ret == true)
            {
                working->applying = true;
            }
            else
            {
                APP_PRINT_ERROR1("app_vendor_htpoll_exec: failed to set %d", working->set);

                os_queue_out(&htpoll_work_queue);
                free(working);

                working = os_queue_peek(&htpoll_work_queue, 0);
            }
        }
    }
}

static void app_vendor_htpoll_handle_qos_cmpl(T_BT_EVENT_PARAM_TRAFFIC_QOS_CMPL *qos_cmpl)
{
    T_HTPOLL_WORK *working = os_queue_peek(&htpoll_work_queue, 0);

    APP_PRINT_TRACE4("app_vendor_htpoll_handle_qos_cmpl: addr %s enable %d type %d cause %d",
                     TRACE_BDADDR(qos_cmpl->bd_addr), qos_cmpl->enable, qos_cmpl->type, qos_cmpl->cause);

    if (working != NULL)
    {
        if (qos_cmpl->type != working->param.qos_type ||
            memcmp(qos_cmpl->bd_addr, working->addr, 6))
        {
            return;
        }

        if ((qos_cmpl->enable && qos_cmpl->cause == 0) ||
            (qos_cmpl->enable == 0) /* for htpoll disable case we dont need to check the cause*/)
        {
            if (working->set == false)
            {
                if (app_link_check_b2s_link(working->addr))
                {
                    app_sniff_mode_b2s_enable(working->addr, SNIFF_DISABLE_MASK_GAMINGMODE_DONGLE);
                }
                else
                {
                    app_sniff_mode_b2b_enable(working->addr, SNIFF_DISABLE_MASK_GAMINGMODE_DONGLE);
                }
            }

            os_queue_out(&htpoll_work_queue);
            free(working);

#if F_APP_B2B_HTPOLL_CONTROL
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_HTPOLL_EXEC_STOP);
#endif

            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_QOS_CMPL);

            app_vendor_htpoll_exec();
        }
    }
}

static bool app_vendor_exist_b2s_htpoll(T_HTPOLL_INFO *exist_htpoll_info)
{
    bool ret = false;
    T_HTPOLL_TGT *htpoll_tgt = NULL;

    for (uint8_t i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL && app_link_check_b2s_link(htpoll_tgt->info.addr) &&
            htpoll_tgt->info.param.rsv_slot != 0)
        {
            ret = true;
            memcpy(exist_htpoll_info, &htpoll_tgt->info, sizeof(T_HTPOLL_INFO));
            break;
        }
    }

    return ret;
}

static bool app_vendor_htpoll_exist(uint8_t *addr, T_HTPOLL_PARAM param)
{
    bool ret = false;
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL && !memcmp(htpoll_tgt->info.addr, addr, 6))
        {
            if (!memcmp(&htpoll_tgt->info.param, &param, sizeof(T_HTPOLL_PARAM)))
            {
                ret = true;
                break;
            }
        }
    }

    return ret;
}

static bool app_vendor_htpoll_set(bool set, uint8_t *addr, T_HTPOLL_PARAM param)
{
    bool ret = true;
    uint8_t cause = 0;

    APP_PRINT_TRACE4("app_vendor_htpoll_set: set %d addr %s param (%d:%d)", set, TRACE_BDADDR(addr),
                     param.interval, param.rsv_slot);

    if (set == true)
    {
        if (param.interval == 0 || param.rsv_slot == 0)
        {
            APP_PRINT_ERROR0("app_vendor_htpoll_set failed: invalid parameter");
            return false;
        }
    }

    T_HTPOLL_WORK *new_htpoll = calloc(1, sizeof(T_HTPOLL_WORK));

    if (new_htpoll != NULL)
    {
        new_htpoll->set = set;
        new_htpoll->applying = false;
        memcpy(new_htpoll->addr, addr, 6);
        memcpy(&new_htpoll->param, &param, sizeof(T_HTPOLL_PARAM));

        if (set == false)
        {
            new_htpoll->param.interval = 0;
            new_htpoll->param.rsv_slot = 0;
        }

        os_queue_in(&htpoll_work_queue, new_htpoll);

#if F_APP_ERWS_SUPPORT
        app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_HTPOLL_EXEC_START);
#endif

        app_vendor_htpoll_exec();

        app_vendor_htpoll_tgt_update(addr, new_htpoll->param);

        ret = true;
    }

    return ret;
}

static void app_venodr_htpoll_clear_exist_htpoll()
{
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL && htpoll_tgt->info.param.rsv_slot != 0)
        {
            T_HTPOLL_PARAM param = {0};

            param.qos_type = htpoll_tgt->info.param.qos_type;

            app_vendor_htpoll_set(false, htpoll_tgt->info.addr, param);
        }
    }
}

static void app_vendor_htpoll_sco_tgt_clear(void)
{
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;
    T_HTPOLL_PARAM param = {0};

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL &&
            htpoll_tgt->info.param.qos_type == BT_PERIODIC_SYNC_TRAFFIC_QOS)
        {
            memset(&htpoll_tgt->info.param, 0, sizeof(T_HTPOLL_PARAM));
        }
    }
}

static void app_vendor_htpoll_tgt_clear(void)
{
    uint8_t i = 0;
    T_HTPOLL_TGT *htpoll_tgt = NULL;
    T_HTPOLL_PARAM param = {0};

    APP_PRINT_TRACE0("app_vendor_htpoll_tgt_clear");

    for (i = 0; i < htpoll_tgt_queue.count; i++)
    {
        htpoll_tgt = os_queue_peek(&htpoll_tgt_queue, i);

        if (htpoll_tgt != NULL)
        {
            memset(&htpoll_tgt->info.param, 0, sizeof(T_HTPOLL_PARAM));
        }
    }
}

void app_vendor_htpoll_control(T_APP_HTPOLL_EVENT event)
{
    uint8_t target_addr[6] = {0};
    uint8_t null_addr[6] = {0};
    T_HTPOLL_INFO htpoll_info = {0};
    bool ret = false;
    bool set_htpoll = false;

#if F_APP_B2B_HTPOLL_CONTROL
    if (event == B2B_HTPOLL_EVENT_B2B_CONNECTED)
    {
        b2b_connected = true;
    }
    else if (event == B2B_HTPOLL_EVENT_B2B_DISCONNECTED)
    {
        b2b_connected = false;
    }
#endif

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        /* for secondary we will clean the htpoll list due to only primary handle the status
        *  ex: roleswap happens: b2s disc (htpoll list remove) and b2b roleswap (new primary will set b2b htpoll in htpoll list)
        */
        app_vendor_htpoll_tgt_clear();
        return;
    }

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    if (event == B2S_HTPOLL_EVENT_LEA_SET_PREFER_QOS)
    {
        app_start_timer(&timer_idx_lea_set_prefer_qos, "lea_set_prefer_qos",
                        app_vendor_timer_id, APP_TIMER_LEA_SET_PREFER_QOS, 0, false,
                        5000);
    }
#endif

    if (0)
    {}
#if F_APP_B2S_HTPOLL_SUPPORT
    else if (app_vendor_b2s_htpoll_info_check(&htpoll_info))
    {
        set_htpoll = true;
    }
#endif
#if F_APP_B2B_HTPOLL_CONTROL
    else if (app_vendor_b2b_htpoll_info_check(&htpoll_info))
    {
        set_htpoll = true;
    }
#endif

    if (set_htpoll)
    {
        if (app_vendor_htpoll_exist(htpoll_info.addr, htpoll_info.param) == false)
        {
            T_HTPOLL_INFO exist_htpoll_info = {0};
            bool clear_htpoll = false;

            if (app_vendor_exist_b2s_htpoll(&exist_htpoll_info) && app_link_check_b2s_link(htpoll_info.addr))
            {
                if (!memcmp(exist_htpoll_info.addr, htpoll_info.addr, 6))
                {
                    if (exist_htpoll_info.param.qos_type != htpoll_info.param.qos_type)
                    {
                        clear_htpoll = true;
                    }
                }
                else
                {
                    clear_htpoll = true;
                }
            }

            /* clear exist b2s htpoll before set new b2s htpoll */
            if (clear_htpoll)
            {
                T_HTPOLL_PARAM param = {0};

                param.qos_type = exist_htpoll_info.param.qos_type;

                app_vendor_htpoll_set(false, exist_htpoll_info.addr, param);
            }

            app_vendor_htpoll_set(true, htpoll_info.addr, htpoll_info.param);
        }

#if F_APP_B2B_HTPOLL_CONTROL
        if (htpoll_info.param.is_b2b_htpoll)
        {
            app_vendor_htpoll_set_b2s_random_htpoll();
        }
#endif
    }
    else
    {
        app_venodr_htpoll_clear_exist_htpoll();
    }

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    /* adjust gaming dongle latency */
    app_dongle_adjust_gaming_latency();
#endif

    APP_PRINT_TRACE1("app_vendor_htpoll_control: event 0x%02x", event);
}
#endif

static void app_vendor_timer_cback(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
#if F_APP_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
    case APP_TIMER_RESEND_HTPOLL:
        {
            app_stop_timer(&timer_idx_htpoll_resend);

            app_vendor_htpoll_exec();
        }
        break;

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    case APP_TIMER_LEA_SET_PREFER_QOS:
        {
            app_stop_timer(&timer_idx_lea_set_prefer_qos);

            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_LEA_SET_QOS_TIMEOUT);
        }
        break;
#endif
#endif
    }
}

static void app_vendor_bt_cback(T_BT_EVENT event, void *buf, uint16_t buf_len)
{
    bool handle = false;
    T_BT_EVENT_PARAM *param = buf;

    switch (event)
    {
#if F_APP_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
    case BT_EVENT_ACL_CONN_ACTIVE:
        {
            app_vendor_htpoll_exec();
        }
        break;

    case BT_EVENT_TRAFFIC_QOS_RSP:
        {
            T_BT_EVENT_PARAM_TRAFFIC_QOS_RSP *qos_rsp = &param->traffic_qos_rsp;

            if (qos_rsp->cause != HCI_SUCCESS)
            {
                T_HTPOLL_WORK *working = os_queue_peek(&htpoll_work_queue, 0);

                if (working != NULL)
                {
                    APP_PRINT_TRACE1("htpoll set failed casue 0x%04x; retry htpoll!!", qos_rsp->cause);

                    working->applying = false;

                    /* start a timer resend htpoll to prevent controller busy issue */
                    app_start_timer(&timer_idx_htpoll_resend, "resend_htpoll",
                                    app_vendor_timer_id, APP_TIMER_RESEND_HTPOLL, 0, false,
                                    300);
                }
            }

            handle = true;
        }
        break;

    case BT_EVENT_TRAFFIC_QOS_CMPL:
        {
            T_BT_EVENT_PARAM_TRAFFIC_QOS_CMPL *qos_cmpl = &param->traffic_qos_cmpl;

            app_vendor_htpoll_handle_qos_cmpl(qos_cmpl);

            handle = true;
        }
        break;

#if F_APP_B2B_HTPOLL_CONTROL
    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            T_BT_EVENT_PARAM_REMOTE_ROLESWAP_STATUS *roleswap = &param->remote_roleswap_status;

            if (roleswap->status == BT_ROLESWAP_STATUS_SUCCESS)
            {
                app_vendor_htpoll_control(B2B_HTPOLL_EVENT_ROLESWAP_CMPL);
            }
            else if (roleswap->status == BT_ROLESWAP_STATUS_ACL_INFO)
            {
                app_vendor_htpoll_tgt_add(roleswap->u.acl.bd_addr);
            }
            else if (roleswap->status == BT_ROLESWAP_STATUS_BDADDR)
            {
                app_vendor_htpoll_tgt_replace_b2b_addr(roleswap->u.bdaddr.curr_bd_addr);
            }

            handle = true;
        }
        break;
#endif

#if F_APP_B2B_ENGAGE_REDUCE_NSE
    case BT_EVENT_ACL_CONN_FAIL:
        {
            uint16_t dongle_cis_conn_handle = app_dongle_get_dongle_cis_conn_handle();
            uint8_t nse = app_dongle_get_dongle_cis_nse();

            if (app_link_check_b2b_link(param->acl_conn_fail.bd_addr) &&
                dongle_cis_conn_handle != 0 && nse != 0)
            {
                app_vendor_set_cis_nse(dongle_cis_conn_handle, nse);
            }

            handle = true;
        }
        break;
#endif

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            app_vendor_htpoll_tgt_add(param->acl_conn_success.bd_addr);

#if F_APP_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_ACL_CONN);
#endif

#if F_APP_B2B_ENGAGE_REDUCE_NSE
            uint16_t dongle_cis_conn_handle = app_dongle_get_dongle_cis_conn_handle();
            uint8_t nse = app_dongle_get_dongle_cis_nse();

            if (app_link_check_b2b_link(param->acl_conn_success.bd_addr) &&
                dongle_cis_conn_handle != 0 && nse != 0)
            {
                app_vendor_set_cis_nse(dongle_cis_conn_handle, nse);
            }
#endif

            handle = true;
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            app_vendor_htpoll_tgt_remove(param->acl_conn_disconn.bd_addr);
            app_vendor_htpoll_work_remove(param->acl_conn_disconn.bd_addr);

#if F_APP_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_ACL_DISC);
#endif

            handle = true;
        }
        break;

#if F_APP_B2S_HTPOLL_SUPPORT
    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_A2DP_START);
            handle = true;
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_A2DP_STOP);
            handle = true;
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_A2DP_DISC);
            handle = true;
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            if (param->sco_conn_cmpl.cause == 0)
            {
                app_vendor_htpoll_control(B2S_HTPOLL_EVENT_SCO_CONNECTED);
            }
            handle = true;
        }
        break;

    case BT_EVENT_SCO_DISCONNECTED:
        {
            app_vendor_htpoll_sco_work_remove();
            app_vendor_htpoll_sco_tgt_clear();

            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_SCO_DISCONNECTED);
            handle = true;
        }
        break;
#endif
#endif

    default:
        break;
    }

    if (handle)
    {
        APP_PRINT_TRACE1("app_vendor_bt_cback: event 0x%04x", event);
    }
}

#if F_APP_B2B_ENGAGE_REDUCE_NSE
bool app_vendor_set_cis_nse(uint16_t conn_handle, uint8_t max_nse)
{
    uint8_t params[4];

    params[0] = HCI_EXT_SUB_DYNAMIC_SET_CIS_VND_CMD;
    params[1] = conn_handle & 0xff;
    params[2] = conn_handle >> 8;
    params[3] = max_nse;

    APP_PRINT_TRACE2("app_vendor_set_cis_nse: conn_handle 0x%04x max_nse %d", conn_handle, max_nse);

    return gap_vendor_cmd_req(HCI_BR_VENDOR_EXTENSION_FEATURE, sizeof(params), params);
}

bool app_vendor_set_report_conn_event(bool enable)
{
    uint8_t params[2];

    params[0] = HCI_EXT_SUB_SET_REPORT_NEW_CONN_DEVICE;
    params[1] = enable;

    return gap_vendor_cmd_req(HCI_BR_VENDOR_EXTENSION_FEATURE, sizeof(params), params);
}

void app_vendor_ble_gap_handle(uint8_t cb_type, T_GAP_CB_DATA *cb_data)
{
    switch (cb_type)
    {
    case GAP_MSG_VENDOR_EVT_INFO:
        {
            if (cb_data->p_gap_vendor_evt_info->param[0] == HCI_VENDOR_REPORT_NEW_CONN_DEVICE_SUBEVENT)
            {
                uint8_t *addr = &cb_data->p_gap_vendor_evt_info->param[1];
                uint16_t cis_conn_handle = app_dongle_get_dongle_cis_conn_handle();

                APP_PRINT_TRACE1("HCI_VENDOR_REPORT_NEW_CONN_DEVICE_SUBEVENT addr %s", TRACE_BDADDR(addr));

                if (app_link_check_b2b_link(addr) == false)
                {
                    break;
                }

                if (cis_conn_handle != 0)
                {
                    app_vendor_set_cis_nse(cis_conn_handle, 1);
                }
            }
        }
        break;
    }
}
#endif

/**
 * @brief Set Advertising Extended Misc Command
 *
 * @param fix_channel  Aux ADV Fix Channel:
 * 0 Extended Advertising auxiliary packet channel unfixed.
 * 1 Extended Advertising auxiliary packet channel fixed to 1 (2406MHz).
 * @param offset  Aux ADV offset
 * 0~0xFF (Unit: slot)
 * The minimum value of the offset from a Extended Advertising packet to its auxiliary packet
 * The actual offset value might larger than Aux ADV offset because of collision with other protocol.
 * @return true  success
 * @return false fail
 */
bool app_vendor_set_adv_extend_misc(uint8_t fix_channel, uint8_t offset)
{
    uint16_t opcode = HCI_VENDOR_SET_ADV_EXT_MISC;
    uint8_t params[3];
    uint8_t params_len = 3;

    params[0] = HCI_VENDOR_SET_ADV_EXT_MISC_SUBCODE;
    params[1] = fix_channel;
    params[2] = offset;

    return gap_vendor_cmd_req(opcode, params_len, params);
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
bool app_vendor_send_psd_policy(uint16_t lea_conn_handle)
{
    uint16_t opcode = HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY;
    uint8_t params[4];
    uint8_t params_len = 4;

    params[0] = HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY_SUBCODE;
    memcpy(&params[1], &lea_conn_handle, 2);
    params[3] = 1; /* local first; due to earbud is rx */

    return gap_vendor_cmd_req(opcode, params_len, params);
}

void app_vendor_rf_xtak_k(uint8_t channel, uint8_t upperbound, uint8_t lowerbound,
                          uint8_t offset)
{
    uint8_t params[6];

    params[0] = 0x06; //Module ID, MODULE_XTAL = 0x06
    params[1] = 0x00; //Subcmd, AUTO_K = 0x00

    /* cmd params */
    params[2] = channel;
    params[3] = upperbound;
    params[4] = lowerbound;
    params[5] = offset;

    gap_vendor_cmd_req(MP_CMD_HCI_OPCODE, sizeof(params), params);
}

void app_vendor_get_xtak_k_result(void)
{
    uint8_t params[4];

    params[0] = 0x06; //Module ID, MODULE_XTAL = 0x06
    params[1] = 0x01; //Subcmd, XTAL_VALUE = 0x01

    /* cmd params */
    params[2] = 0x0;  //Type, VALUE_GET = 0x0
    params[3] = 0x0;  //Value, this param would be ignored when type is get.

    gap_vendor_cmd_req(MP_CMD_HCI_OPCODE, sizeof(params), params);
}

void app_vendor_write_xtak_k_result(uint8_t xtal_val)
{
#if TARGET_RTL8773DO || TARGET_RTL8773DFL
    /*Write to sys cfg*/
    uint8_t params[17];

    //Only For BBPRO3
    params[0] = 0x00; //Module ID, PLATFORM
    params[1] = 0x02; //Subcmd, CFG_REVISE

    /* cmd params */
    params[2] = 0x02; //config number

    /*XTAL_SC_XI*/
    params[3] = 0x01; //Moudle ID L
    params[4] = 0x00; //Moudle ID H
    params[5] = 0x25; //offset L
    params[6] = 0x00; //offset H
    params[7] = 0x01; //len
    params[8] = xtal_val;//data
    params[9] = 0xFF; //mask

    /*XTAL_SC_XO*/
    params[10] = 0x01; //Moudle ID L
    params[11] = 0x00; //Moudle ID H
    params[12] = 0x26; //offset L
    params[13] = 0x00; //offset H
    params[14] = 0x01; //len
    params[15] = xtal_val;//data
    params[16] = 0xFF; //mask

    gap_vendor_cmd_req(MP_CMD_HCI_OPCODE, sizeof(params), params);
#else
    cfg_update_xtal(xtal_val);
#endif
}

bool app_vendor_enable_send_tx_right_away(void)
{
    uint8_t params[2];

    params[0] = HCI_EXT_SUB_ENABLE_SEND_TX_RIGHT_AWATY;
    params[1] = 1;

    return gap_vendor_cmd_req(HCI_BR_VENDOR_EXTENSION_FEATURE, sizeof(params), params);
}

void app_vendor_init()
{
#if F_APP_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
    os_queue_init(&htpoll_tgt_queue);
    os_queue_init(&htpoll_work_queue);
#endif

    app_timer_reg_cb(app_vendor_timer_cback, &app_vendor_timer_id);

    bt_mgr_cback_register(app_vendor_bt_cback);

#if F_APP_B2B_ENGAGE_REDUCE_NSE
    app_vendor_set_report_conn_event(true);
#endif
}

