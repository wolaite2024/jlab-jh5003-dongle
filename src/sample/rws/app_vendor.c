/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <string.h>
#include <stdlib.h>
#include "gap.h"
#include "btm.h"
#include "trace.h"
#include "gap_vendor.h"
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

#if F_APP_GAMING_DONGLE_SUPPORT
#include "app_hfp.h"
#include "app_dongle_common.h"
#include "app_dongle_dual_mode.h"
#include "app_ble_common_adv.h"
#endif

#define HCI_VENDOR_SET_ADV_EXT_MISC 0xFC98
#define HCI_VENDOR_SET_ADV_EXT_MISC_SUBCODE 0x00

#define HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY 0xFD80
#define HCI_VENDOR_SET_LE_CH_MAP_REF_POLICY_SUBCODE 0x1C

typedef enum
{
    APP_TIMER_RESEND_HTPOLL,
    APP_TIMER_LEA_SET_PREFER_QOS,
} T_APP_VENDOR_TIMER;

static uint8_t app_vendor_timer_id = 0;

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
typedef struct
{
    bool     is_b2b_htpoll;
    bool     negotiate;
    bool     mixing_with_sco;
    uint16_t interval;
    uint8_t  rsv_slot;
} T_HTPOLL_PARAM;

typedef struct
{
    uint8_t addr[6];
    T_HTPOLL_PARAM param;
} T_HTPOLL_INFO;

typedef struct htpoll_list
{
    struct htpoll_list *next;
    bool is_b2b_link;
    T_HTPOLL_INFO info;
} T_HTPOLL_LIST;

typedef struct htpoll_work
{
    struct htpoll_work      *next;
    bool                    set;
    bool                    applying;
    bool                    is_periodic;
    uint8_t                 addr[6];
    T_HTPOLL_PARAM          param;
} T_HTPOLL_WORK;

#if F_APP_B2B_HTPOLL_CONTROL
#define B2S_HTPOLL_RSV_SLOT_FOR_B2B_HTPOLL  6

typedef enum
{
    B2B_HTPOLL_NONE,
    B2B_HTPOLL_CIS_B2B_COEXIST,
    B2B_HTPOLL_BIS_B2B_COEXIST,
} T_B2B_HTPOLL_TYPE;

static const T_HTPOLL_PARAM b2b_htpoll_db[] =
{
    [B2B_HTPOLL_CIS_B2B_COEXIST] = {true, true, false, 64, 2},
    [B2B_HTPOLL_BIS_B2B_COEXIST] = {true, true, false, 64, 2},
};

static bool b2b_connected = false;
#endif

static T_OS_QUEUE htpoll_work_queue;
static T_OS_QUEUE htpoll_list_queue;
static uint8_t timer_idx_htpoll_resend = 0;

static bool app_vendor_htpoll_exist(uint8_t *addr, T_HTPOLL_PARAM param);
static bool app_vendor_htpoll_set(bool set, bool is_periodic, uint8_t *addr, T_HTPOLL_PARAM param);

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
typedef struct
{
    T_HTPOLL_PARAM param;
    uint8_t  gaming_latency_plus;
} T_GAMING_B2S_HTPOLL_DB;

typedef enum
{
    GAMING_B2S_HTPOLL_NONE,
    GAMING_B2S_HTPOLL_LINKBACK,
    GAMING_B2S_HTPOLL_STREAMING_WITH_SINGLE_SRC,
    GAMING_B2S_HTPOLL_STREAMING_WITH_MULTI_SRC,
    GAMING_B2S_HTPOLL_BLE_ADV,
    GAMING_B2S_HTPOLL_LEA_SET_PREFER_QOS,
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
    GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX,
    GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX_WITH_SCO,
#endif
} T_GAMING_B2S_HTPOLL_TYPE;

static const T_GAMING_B2S_HTPOLL_DB gaming_b2s_htpoll_db[] =
{
    [GAMING_B2S_HTPOLL_LINKBACK]                         = {{false, false, false, 30,   22},   20},
    [GAMING_B2S_HTPOLL_STREAMING_WITH_SINGLE_SRC]        = {{false, false, false,  0,    0},    0},
    [GAMING_B2S_HTPOLL_STREAMING_WITH_MULTI_SRC]         = {{false, false, false, 58,   50},    5},
    [GAMING_B2S_HTPOLL_BLE_ADV]                          = {{false, false, false, 58,   50},    5},
    [GAMING_B2S_HTPOLL_LEA_SET_PREFER_QOS]               = {{false, false, false, 30,   22},   20},
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
    [GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX]          = {{false,  true, false, 16,    8},   22},
    [GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX_WITH_SCO] = {{false, false, true,  16,    6},   22},
#endif
};

static uint8_t timer_idx_lea_set_prefer_qos = 0;

uint8_t app_vendor_get_htpoll_latency_plus(void)
{
    uint8_t ret = 0;
    uint8_t i = 0;
    T_HTPOLL_LIST *htpoll_list = NULL;
    bool is_dongle_link_htpoll = false;

    for (i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll_list = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll_list != NULL && htpoll_list->info.param.rsv_slot != 0)
        {
            if (app_link_check_dongle_link(htpoll_list->info.addr))
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
            if (gaming_b2s_htpoll_db[i].param.interval == htpoll_list->info.param.interval &&
                gaming_b2s_htpoll_db[i].param.rsv_slot == htpoll_list->info.param.rsv_slot)
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

static bool app_vendor_gaming_b2s_htpoll_info_check(T_HTPOLL_INFO *info)
{
    uint8_t connected_b2s_num = app_link_get_b2s_link_num();
    uint8_t connected_le_num = app_link_get_le_link_num();
    uint8_t connected_device_num = 0;
    T_APP_BR_LINK *p_dongle_link = app_dongle_get_connected_dongle_link();
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
    T_APP_BR_LINK *p_phone_link = app_dongle_get_connected_phone_link();
#endif
    T_GAMING_B2S_HTPOLL_TYPE htpoll_type = GAMING_B2S_HTPOLL_NONE;
    bool    ret = false;

    connected_device_num = connected_b2s_num + connected_le_num;

    if (p_dongle_link != NULL)
    {
        uint8_t active_idx = app_a2dp_get_active_idx();
        bool is_streaming = false;

        if (app_db.br_link[active_idx].streaming_fg == true)
        {
            is_streaming = true;
        }

        if (is_streaming && app_hfp_sco_is_connected() == false)
        {
            memcpy(info->addr, app_db.br_link[active_idx].bd_addr, 6);

            if (0)
            {}
#if F_APP_SUPPORT_LINKBACK_WHEN_DONGLE_STREAMING
            else if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G_BT &&
                     app_dongle_get_state() == DONGLE_STATE_LINKBACK)
            {
                htpoll_type = GAMING_B2S_HTPOLL_LINKBACK;
            }
#endif
            else if (timer_idx_lea_set_prefer_qos != 0)
            {
                htpoll_type = GAMING_B2S_HTPOLL_LEA_SET_PREFER_QOS;
            }
            else if (app_cfg_nv.legacy_gaming_support_common_adv)
            {
                if (app_ble_common_adv_get_state() == BLE_EXT_ADV_MGR_ADV_ENABLED)
                {
                    htpoll_type = GAMING_B2S_HTPOLL_BLE_ADV;
                }
            }
            else if (connected_device_num == 1)
            {
                htpoll_type = GAMING_B2S_HTPOLL_STREAMING_WITH_SINGLE_SRC;
            }
            else if (connected_device_num > 1)
            {
                htpoll_type = GAMING_B2S_HTPOLL_STREAMING_WITH_MULTI_SRC;

#if F_APP_GAMING_CHAT_MIXING_SUPPORT
                if ((p_dongle_link->streaming_fg || app_db.dongle_is_enable_mic) &&
                    (p_phone_link && p_phone_link->streaming_fg))
                {
                    memcpy(info->addr, p_dongle_link->bd_addr, 6);
                    htpoll_type = GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX;
                }
#endif
            }
        }
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
        else if (is_streaming && app_hfp_sco_is_connected())
        {
            htpoll_type = GAMING_B2S_HTPOLL_MULTI_SRC_STREAMING_MIX_WITH_SCO;
        }
#endif
    }

    if (gaming_b2s_htpoll_db[htpoll_type].param.rsv_slot != 0)
    {
        memcpy(&info->param, &gaming_b2s_htpoll_db[htpoll_type].param, sizeof(T_HTPOLL_PARAM));
        ret = true;

        APP_PRINT_TRACE5("app_vendor_gaming_htpoll_info_check: addr %s type %d negotiate %d htpoll (%d:%d)",
                         TRACE_BDADDR(info->addr), htpoll_type, info->param.negotiate, info->param.interval,
                         info->param.rsv_slot);
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
    bool local_cis_connected = app_lea_ascs_cis_exist();
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
        ret = true;

        APP_PRINT_TRACE3("app_vendor_b2b_htpoll_info_check: type %d param (%d:%d)",
                         htpoll_type, info->param.interval, info->param.rsv_slot);
    }

    return ret;
}

/* set all b2s link with random traffic htpoll */
static void app_vendor_htpoll_set_b2s_random_htpoll()
{
    /* for b2b htpoll we also to set b2s htpoll with non-periodic */
    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_link_check_b2s_link_by_id(i))
        {
            T_HTPOLL_PARAM param = {0};

            param.rsv_slot = B2S_HTPOLL_RSV_SLOT_FOR_B2B_HTPOLL;

            if (app_vendor_htpoll_exist(app_db.br_link[i].bd_addr, param) == false)
            {
                app_vendor_htpoll_set(true, false, app_db.br_link[i].bd_addr, param);

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
    T_HTPOLL_LIST *htpoll_list = NULL;

    for (i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll_list = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll_list != NULL)
        {
            APP_PRINT_TRACE6("app_vendor_htpoll_print: addr %s is_b2b_link %d is_b2b_htpoll %d negotiate %d (%d:%d)",
                             TRACE_BDADDR(htpoll_list->info.addr),
                             htpoll_list->is_b2b_link,
                             htpoll_list->info.param.is_b2b_htpoll,
                             htpoll_list->info.param.negotiate,
                             htpoll_list->info.param.interval, htpoll_list->info.param.rsv_slot);
        }
    }
}

static void app_vendor_htpoll_list_add(uint8_t *addr)
{
    uint8_t i = 0;
    T_HTPOLL_LIST *htpoll = NULL;
    bool exist = false;

    for (i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll != NULL && !memcmp(htpoll->info.addr, addr, 6))
        {
            exist = true;
            break;
        }
    }

    if (exist == false)
    {
        htpoll = calloc(1, sizeof(T_HTPOLL_LIST));

        if (htpoll != NULL)
        {
            if (app_link_check_b2b_link(addr))
            {
                htpoll->is_b2b_link = true;
            }

            memcpy(htpoll->info.addr, addr, 6);
            os_queue_in(&htpoll_list_queue, htpoll);
        }
    }
}

static void app_vendor_htpoll_list_remove(uint8_t *addr)
{
    uint8_t i = 0;
    T_HTPOLL_LIST *htpoll = NULL;

    for (i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll != NULL && !memcmp(htpoll->info.addr, addr, 6))
        {
            os_queue_delete(&htpoll_list_queue, htpoll);
            free(htpoll);
            break;
        }
    }
}

static void app_vendor_htpoll_list_replace_b2b_addr(uint8_t *addr)
{
    uint8_t i = 0;
    T_HTPOLL_LIST *htpoll = NULL;

    for (i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll != NULL && htpoll->is_b2b_link)
        {
            memcpy(htpoll->info.addr, addr, 6);
        }
    }
}

static T_HTPOLL_LIST *app_vendor_htpoll_list_get(uint8_t *addr)
{
    uint8_t i = 0;
    T_HTPOLL_LIST *htpoll = NULL;
    T_HTPOLL_LIST *ret = NULL;
    bool get_htpoll = false;

    for (i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll != NULL && !memcmp(htpoll->info.addr, addr, 6))
        {
            get_htpoll = true;
            break;
        }
    }

    if (get_htpoll)
    {
        ret = htpoll;
    }

    return ret;
}

static void app_vendor_htpoll_list_update(uint8_t *addr, T_HTPOLL_PARAM param)
{
    T_HTPOLL_LIST *htpoll = NULL;

    for (uint8_t i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll != NULL && !memcmp(htpoll->info.addr, addr, 6))
        {
            memcpy(&htpoll->info.param, &param, sizeof(T_HTPOLL_PARAM));
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
            APP_PRINT_TRACE4("app_vendor_htpoll_exec: set %d addr %s (%d:%d)",
                             working->set, TRACE_BDADDR(working->addr),
                             working->param.interval, working->param.rsv_slot);

            if (working->set)
            {
                if (working->is_periodic)
                {
                    ret = bt_link_periodic_traffic_qos_set(working->addr, working->param.interval,
                                                           working->param.rsv_slot,
                                                           0xffff,
                                                           working->param.negotiate);
                }
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
                else if (working->param.mixing_with_sco)
                {
                    if (p_sco_link)
                    {
                        bt_link_periodic_traffic_set(p_sco_link->bd_addr);
                        ret = bt_link_periodic_sync_traffic_qos_set(working->addr, working->param.interval,
                                                                    working->param.rsv_slot,
                                                                    0xffff);
                    }
                }
#endif
                else
                {
                    ret = bt_link_random_traffic_qos_set(working->addr, working->param.rsv_slot, 0xffff);
                }
            }
            else
            {
                ret = bt_link_traffic_qos_clear(working->addr);
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

static void app_vendor_htpoll_handle_rsp(T_BT_EVENT_PARAM_TRAFFIC_QOS_RSP *qos_rsp)
{
    T_HTPOLL_WORK *working = NULL;

    if (qos_rsp == NULL)
    {
        return;
    }

    working = os_queue_peek(&htpoll_work_queue, 0);

    APP_PRINT_TRACE1("app_vendor_htpoll_handle_rsp: cause %d", qos_rsp->cause);

    if (working != NULL)
    {
        if (qos_rsp->cause == HCI_SUCCESS)
        {
            T_HTPOLL_LIST *htpoll = app_vendor_htpoll_list_get(working->addr);

            if (htpoll != NULL)
            {
                memcpy(htpoll->info.addr,   working->addr, 6);
                memcpy(&htpoll->info.param, &working->param, sizeof(T_HTPOLL_PARAM));

                if (working->param.rsv_slot == 0)
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
            }

            os_queue_out(&htpoll_work_queue);
            free(working);

#if F_APP_B2B_HTPOLL_CONTROL
            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_HTPOLL_EXEC_STOP);
#endif

            app_vendor_htpoll_exec();
        }
        else
        {
            app_start_timer(&timer_idx_htpoll_resend, "resend_htpoll",
                            app_vendor_timer_id, APP_TIMER_RESEND_HTPOLL, 0, false,
                            300);

            working->applying = false;
        }
    }
}

static bool app_vendor_exist_b2s_htpoll(T_HTPOLL_INFO *exist_htpoll_info)
{
    bool ret = false;
    T_HTPOLL_LIST *htpoll = NULL;

    for (uint8_t i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll != NULL && app_link_check_b2s_link(htpoll->info.addr) &&
            htpoll->info.param.rsv_slot != 0)
        {
            ret = true;
            memcpy(exist_htpoll_info, &htpoll->info, sizeof(T_HTPOLL_INFO));
            break;
        }
    }

    return ret;
}

static bool app_vendor_htpoll_exist(uint8_t *addr, T_HTPOLL_PARAM param)
{
    bool ret = false;
    uint8_t i = 0;
    T_HTPOLL_LIST *htpoll = NULL;

    for (i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll != NULL && !memcmp(htpoll->info.addr, addr, 6))
        {
            if (!memcmp(&htpoll->info.param, &param, sizeof(T_HTPOLL_PARAM)))
            {
                ret = true;
                break;
            }
        }
    }

    return ret;
}

static bool app_vendor_htpoll_set(bool set, bool is_periodic, uint8_t *addr, T_HTPOLL_PARAM param)
{
    bool ret = true;
    uint8_t cause = 0;

    APP_PRINT_TRACE5("app_vendor_htpoll_set: set %d is_periodic %d addr %s param (%d:%d)", set,
                     is_periodic, TRACE_BDADDR(addr), param.interval, param.rsv_slot);

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
        new_htpoll->is_periodic = is_periodic;
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

        app_vendor_htpoll_list_update(addr, new_htpoll->param);

        ret = true;
    }

    return ret;
}

static void app_venodr_htpoll_clear_exist_htpoll()
{
    uint8_t i = 0;
    T_HTPOLL_LIST *htpoll = NULL;

    for (i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll != NULL && htpoll->info.param.rsv_slot != 0)
        {
            T_HTPOLL_PARAM param = {0};

            app_vendor_htpoll_set(false, false, htpoll->info.addr, param);
        }
    }
}

static void app_vendor_htpoll_list_clear(void)
{
    uint8_t i = 0;
    T_HTPOLL_LIST *htpoll = NULL;
    T_HTPOLL_PARAM param = {0};

    APP_PRINT_TRACE0("app_vendor_htpoll_list_clear");

    for (i = 0; i < htpoll_list_queue.count; i++)
    {
        htpoll = os_queue_peek(&htpoll_list_queue, i);

        if (htpoll != NULL)
        {
            memset(&htpoll->info.param, 0, sizeof(T_HTPOLL_PARAM));
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
        app_vendor_htpoll_list_clear();
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
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    else if (app_vendor_gaming_b2s_htpoll_info_check(&htpoll_info))
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
            bool is_periodic = true;

            if (htpoll_info.param.mixing_with_sco)
            {
                is_periodic = false;
            }

            T_HTPOLL_INFO exist_htpoll_info = {0};
            bool clear_htpoll = false;

            if (app_vendor_exist_b2s_htpoll(&exist_htpoll_info) && app_link_check_b2s_link(htpoll_info.addr))
            {
                if (!memcmp(exist_htpoll_info.addr, htpoll_info.addr, 6))
                {
                    if (exist_htpoll_info.param.negotiate != htpoll_info.param.negotiate)
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

                param.negotiate = exist_htpoll_info.param.negotiate;

                app_vendor_htpoll_set(false, false, exist_htpoll_info.addr, param);
            }

            app_vendor_htpoll_set(true, is_periodic, htpoll_info.addr, htpoll_info.param);
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
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
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
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
    case BT_EVENT_ACL_CONN_ACTIVE:
        {
            app_vendor_htpoll_exec();
        }
        break;

    case BT_EVENT_TRAFFIC_QOS_RSP:
        {
            T_BT_EVENT_PARAM_TRAFFIC_QOS_RSP *qos_rsp = &param->traffic_qos_rsp;

            app_vendor_htpoll_handle_rsp(qos_rsp);

            if (qos_rsp->cause != HCI_SUCCESS)
            {
                /* start a timer resend htpoll to prevent controller busy issue */
                app_start_timer(&timer_idx_htpoll_resend, "resend_htpoll",
                                app_vendor_timer_id, APP_TIMER_RESEND_HTPOLL, 0, false,
                                300);
            }

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
                app_vendor_htpoll_list_add(roleswap->u.acl.bd_addr);
            }
            else if (roleswap->status == BT_ROLESWAP_STATUS_BDADDR)
            {
                app_vendor_htpoll_list_replace_b2b_addr(roleswap->u.bdaddr.curr_bd_addr);
            }

            handle = true;
        }
        break;
#endif

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            app_vendor_htpoll_list_add(param->acl_conn_success.bd_addr);

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_ACL_CONN);
#endif

            handle = true;
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            app_vendor_htpoll_list_remove(param->acl_conn_disconn.bd_addr);

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
            app_vendor_htpoll_control(B2S_HTPOLL_EVENT_ACL_DISC);
#endif

            handle = true;
        }
        break;

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
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

void app_vendor_init()
{
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
    os_queue_init(&htpoll_list_queue);
    os_queue_init(&htpoll_work_queue);
#endif

    app_timer_reg_cb(app_vendor_timer_cback, &app_vendor_timer_id);

    bt_mgr_cback_register(app_vendor_bt_cback);
}

