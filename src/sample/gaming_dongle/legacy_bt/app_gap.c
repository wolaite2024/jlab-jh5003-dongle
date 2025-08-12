/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "gap.h"
#include "gap_br.h"
#include "app_gap.h"
#include "app_main.h"
#include "app_cfg.h"
#include "remote.h"
#include "btm.h"


const uint8_t null_addr[6] = {0};

static void app_gap_common_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_CB_DATA));
    APP_PRINT_INFO1("app_gap_common_callback: cb_type = %d", cb_type);
    switch (cb_type)
    {
    case GAP_MSG_WRITE_AIRPLAN_MODE:
        break;
    case GAP_MSG_READ_AIRPLAN_MODE:
        break;
    case GAP_MSG_SET_LOCAL_BD_ADDR:
        break;

    default:
        break;
    }
    return;
}

static void app_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            memcpy(app_db.factory_addr, param->ready.bd_addr, 6);
            APP_PRINT_INFO1("app_gap_bt_cback: ready, bd_addr %b",
                            TRACE_BDADDR(param->ready.bd_addr));

            if (!memcmp(app_cfg_nv.bud_local_addr, null_addr, 6))
            {
                memcpy(app_cfg_nv.bud_local_addr, app_db.factory_addr, 6);
                remote_local_addr_set(app_cfg_nv.bud_local_addr);
            }

            if (!memcmp(app_cfg_nv.bud_peer_addr, null_addr, 6))
            {
                //memcpy(app_cfg_nv.bud_peer_addr, app_cfg_const.bud_peer_addr, 6);
                //remote_peer_addr_set(app_cfg_nv.bud_peer_addr);
            }
            //gap_set_bd_addr(app_cfg_nv.bud_local_addr);
        }
        break;

    case BT_EVENT_LINK_USER_CONFIRMATION_REQ:
        {
            gap_br_user_cfm_req_cfm(param->link_user_confirmation_req.bd_addr, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    default:
        break;
    }
}

void app_gap_init(void)
{
    gap_register_app_cb(app_gap_common_callback);

    bt_mgr_cback_register(app_gap_bt_cback);
}
