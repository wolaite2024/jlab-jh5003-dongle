/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <string.h>
#include <os_mem.h>
#include <trace.h>
#include "ual_types.h"
#include "ual_list.h"
#include "ual_adapter.h"
//#include "ual_errno.h"
#include "ual_upperstack_cfg.h"

#include "btm.h"
#include "gap_msg.h"
#include "gap_br.h"
#include "inquiry.h"
#include "dev_mgr.h"

void ual_handle_inquiry_rsp(T_BT_ADAPTER *adapter, uint16_t cause)
{
    T_GAP_CAUSE rc;

    APP_PRINT_INFO1("ual_handle_inquiry_rsp: cause %04x", cause);

    if (!adapter)
    {
        return;
    }

    if (cause)
    {
        adapter->inquiring = 0;
        if (adapter->inquiry_refcnt)
        {
            rc = gap_br_start_inquiry(false, 10);
            if (rc != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR0("ual_handle_inquiry_rsp: start inquiry err");
            }
            else
            {
                adapter->inquiring = 1;
            }
        }
    }
}

void ual_handle_inquiry_cancel_cmpl(T_BT_ADAPTER *adapter, uint16_t cause)
{
    T_GAP_CAUSE rc;

    APP_PRINT_INFO1("ual_handle_inquiry_cancel_cmpl: cause %04x", cause);

    if (!adapter)
    {
        return;
    }

    adapter->inquiring = 0;

    if (adapter->inquiry_refcnt)
    {
        rc = gap_br_start_inquiry(false, 10);
        if (rc != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR0("ual_handle_inquiry_cancel_cmpl: start inquiry err");
        }
        else
        {
            adapter->inquiring = 1;
        }

    }
}

void ual_handle_inquiry_cmpl(T_BT_ADAPTER *adapter, uint16_t cause)
{
    T_GAP_CAUSE rc;

    APP_PRINT_INFO1("ual_handle_inquiry_cmpl: cause %04x", cause);

    if (!adapter)
    {
        return;
    }

    if (adapter->inquiry_refcnt)
    {
        rc = gap_br_start_inquiry(false, 10);
        if (rc != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("ual_handle_inquiry_cmpl: rc 0x02%x", rc);
            adapter->inquiring = 0;
        }
        /* Keep adapter->inquiring */
    }
    else
    {
        adapter->inquiring = 0;
    }
}

void ual_handle_inquiry_result(T_BT_ADAPTER *adapter,
                               T_BT_EVENT_PARAM_INQUIRY_RESULT inq_res)
{
    struct bt_scan_client *client;
    T_UALIST_HEAD *pos, *n;

    APP_PRINT_INFO1("ual_handle_inquiry_result: bd_addr %s",
                    TRACE_BDADDR(inq_res.bd_addr));

    if (!adapter)
    {
        return;
    }

    T_DISC_RESULT *p_cur = ual_find_or_alloc_disc_db(inq_res.bd_addr);
    if (p_cur != NULL)
    {
        p_cur->cod = inq_res.cod;
    }

    ualist_for_each_safe(pos, n, &adapter->scan_clients)
    {
        client = ualist_entry(pos, struct bt_scan_client, list);
        if (client->cback && (client->discovery_type & DISCOVERY_TYPE_BREDR))
        {
            client->cback(SCAN_RESULT_BREDR, inq_res.bd_addr);
        }
    }
}

