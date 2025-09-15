/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>

#include "trace.h"
#include "app_timer.h"
#include "btm.h"
#include "app_cfg.h"
#include "app_main.h"
//#include "app_report.h"
#include "gap_br.h"
#include "remote.h"
#include "app_hfp_ag.h"
#define STEVEN_HSP_AG_TEST_HANDSFREE

#define APP_HFP_AG_TEST 0

typedef enum
{
    APP_HFP_AG_TEST_API_ID_CONNECT,
    APP_HFP_AG_TEST_API_ID_DISCONNECT,
    APP_HFP_AG_TEST_API_ID_AUDIO_CONNECT,
    APP_HFP_AG_TEST_API_ID_AUDIO_DISCONNECT,
    APP_HFP_AG_TEST_API_ID_CALL_INCOMING,
    APP_HFP_AG_TEST_API_ID_CALL_ANSWER,
    APP_HFP_AG_TEST_API_ID_CALL_REJECT_TERMINATE,
    APP_HFP_AG_TEST_API_ID_CALL_DIAL,
    APP_HFP_AG_TEST_API_ID_CALL_ALERT,
    APP_HFP_AG_TEST_API_ID_NOTIFY_MIC_VOLUMN,
    APP_HFP_AG_TEST_API_ID_NOTIFY_SPEAKER_VOLUMN,
} T_APP_HFP_AG_TEST_API_ID;

static uint8_t hfp_ag_test_timer_id = 0;
static uint8_t timer_idx_hfp_ag_alarm = 0;
static uint8_t test_app_hfp_ag_api_id = APP_HFP_AG_TEST_API_ID_CALL_INCOMING;
static uint8_t target_bd_addr[6] = {0x25, 0x5c, 0xcd, 0x4b, 0x5c, 0x74};//jabra 65t.
//static uint8_t target_bd_addr[6] = {0xa2, 0xb1, 0xbe, 0xa5, 0x22, 0xe4};//PLT.

bool test_app_hfp_ag_hf_sdp(uint8_t *bd_addr)
{
    T_GAP_UUID_DATA uuid;

    uuid.uuid_16 = UUID_HANDSFREE;

    if (gap_br_start_sdp_discov(bd_addr, GAP_UUID16, uuid) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool test_app_hfp_ag_hs_sdp(uint8_t *bd_addr)
{
    T_GAP_UUID_DATA uuid;

    uuid.uuid_16 = UUID_HEADSET;

    if (gap_br_start_sdp_discov(bd_addr, GAP_UUID16, uuid) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

static void test_app_hfp_ag_timer_start(T_APP_BR_LINK *p_link, uint8_t hfp_ag_api_id,
                                        uint16_t period)
{
    test_app_hfp_ag_api_id = hfp_ag_api_id;
    app_start_timer(&timer_idx_hfp_ag_alarm, "app_hfp_ag_test_alarm",
                    hfp_ag_test_timer_id, APP_HFP_AG_TEST, p_link->id, false,
                    period);
}

void test_app_hfp_ag_handle_sdp_discovery_info(uint8_t *bd_addr, T_BT_SDP_ATTR_INFO *sdp_attr)
{
    T_APP_BR_LINK *p_link;
    T_BT_SDP_ATTR_INFO *p_info = (T_BT_SDP_ATTR_INFO *)sdp_attr;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->sdp_active_inquire_profile == HFP_PROFILE_MASK &&
            p_info->srv_class_uuid_data.uuid_16 == UUID_HANDSFREE)
        {
            if ((p_link->connected_profile & HFP_PROFILE_MASK) == 0)
            {
                if (bt_hfp_ag_connect_req(bd_addr, p_info->server_channel, true) == true)
                {
                    p_link->sdp_active_inquire_profile = 0;//Mean SDP successful
                    p_link->sdp_hfp_ag_record_num++;
                }
            }
        }
        else if (p_link->sdp_active_inquire_profile == HSP_PROFILE_MASK &&
                 p_info->srv_class_uuid_data.uuid_16 == UUID_HEADSET)
        {
            if ((p_link->connected_profile & HSP_PROFILE_MASK) == 0)
            {
                if (bt_hfp_ag_connect_req(bd_addr, p_info->server_channel, false) == true)
                {
                    p_link->sdp_active_inquire_profile = 0;//Mean SDP successful
                    p_link->sdp_hfp_ag_record_num++;
                }
            }
        }
    }
}

static void test_app_hfp_ag_handle_sdp_discovery_cmpl(uint8_t *bd_addr, uint16_t cause)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL && cause == 0)
    {
        if (p_link->sdp_active_inquire_profile == HFP_PROFILE_MASK)
        {
            if (p_link->sdp_hfp_ag_record_num != 0)
            {
                //SDP HFP result is not null, just connect HFP profile and skip HS SDP and connect.
                p_link->sdp_hfp_ag_record_num = 0;
                p_link->sdp_active_inquire_profile = 0;
            }
            else
            {
                //HF SDP info is NULL, then try to HS SDP.
                bool ret;
                ret = test_app_hfp_ag_hs_sdp(p_link->bd_addr);

                if (ret == true)
                {
                    p_link->sdp_active_inquire_profile = HSP_PROFILE_MASK;
                }
            }
        }
        else if (p_link->sdp_active_inquire_profile == HSP_PROFILE_MASK)
        {
            //HS SDP info is NULL, Terminate SDP.
            p_link->sdp_hfp_ag_record_num = 0;
            p_link->sdp_active_inquire_profile = 0;
        }
    }
}

static void test_hfp_ag_connect(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);

    if (p_link == NULL)
    {
        p_link = app_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
#ifndef STEVEN_HSP_AG_TEST_HANDSFREE
        //Handsfree profile SDP.
        if ((p_link->connected_profile & HFP_PROFILE_MASK) == 0)
        {
            uint8_t bond_flag;
            bool    ret;

            ret = test_app_hfp_ag_hf_sdp(p_link->bd_addr);

            if (ret == true)
            {
                p_link->sdp_active_inquire_profile = HFP_PROFILE_MASK;
                p_link->sdp_hfp_ag_record_num = 0;
            }
        }
#else
        //Headset profile SDP.
        if ((p_link->connected_profile & HSP_PROFILE_MASK) == 0)
        {
            bool    ret;

            ret = test_app_hfp_ag_hs_sdp(p_link->bd_addr);

            if (ret == true)
            {
                p_link->sdp_active_inquire_profile = HSP_PROFILE_MASK;
                p_link->sdp_hfp_ag_record_num = 0;
            }
        }
#endif
    }
}

static void test_app_hfp_ag_api_handle(T_APP_BR_LINK *p_link, uint8_t hfp_ag_api_id)
{
    APP_PRINT_TRACE1("test_app_hfp_ag_api_handle: hfp_ag_api_id: %d", hfp_ag_api_id);
    switch (hfp_ag_api_id)
    {
    case APP_HFP_AG_TEST_API_ID_DISCONNECT:
        {
            bt_hfp_ag_disconnect_req(p_link->bd_addr);
        }
        break;

    case APP_HFP_AG_TEST_API_ID_AUDIO_CONNECT:
        {
            bt_hfp_ag_audio_connect_req(p_link->bd_addr);
            test_app_hfp_ag_timer_start(p_link, APP_HFP_AG_TEST_API_ID_AUDIO_DISCONNECT, 4000);
        }
        break;

    case APP_HFP_AG_TEST_API_ID_AUDIO_DISCONNECT:
        {
            bt_hfp_ag_audio_disconnect_req(p_link->bd_addr);
        }
        break;

    case APP_HFP_AG_TEST_API_ID_CALL_INCOMING:
        {
            bt_hfp_ag_call_incoming(p_link->bd_addr, "10086", 6, 129);
            test_app_hfp_ag_timer_start(p_link, APP_HFP_AG_TEST_API_ID_CALL_ANSWER, 1000);
        }
        break;

    case APP_HFP_AG_TEST_API_ID_CALL_REJECT_TERMINATE:
        {
            bt_hfp_ag_call_terminate(p_link->bd_addr);
            test_app_hfp_ag_timer_start(p_link, APP_HFP_AG_TEST_API_ID_DISCONNECT, 1000);
        }
        break;

    case APP_HFP_AG_TEST_API_ID_CALL_ANSWER:
        {
            bt_hfp_ag_call_answer(p_link->bd_addr);
            test_app_hfp_ag_timer_start(p_link, APP_HFP_AG_TEST_API_ID_CALL_REJECT_TERMINATE, 4000);
        }
        break;

    case APP_HFP_AG_TEST_API_ID_CALL_DIAL:
        {
            bt_hfp_ag_call_dial(p_link->bd_addr);
            test_app_hfp_ag_timer_start(p_link, APP_HFP_AG_TEST_API_ID_CALL_ALERT, 4000);
        }
        break;

    case APP_HFP_AG_TEST_API_ID_CALL_ALERT:
        {
            bt_hfp_ag_call_alert(p_link->bd_addr);
            test_app_hfp_ag_timer_start(p_link, APP_HFP_AG_TEST_API_ID_CALL_ANSWER, 1000);
        }
        break;

    case APP_HFP_AG_TEST_API_ID_NOTIFY_MIC_VOLUMN:
        {
            bt_hfp_ag_microphone_gain_set(p_link->bd_addr, 3);
            test_app_hfp_ag_timer_start(p_link, APP_HFP_AG_TEST_API_ID_NOTIFY_SPEAKER_VOLUMN, 1000);
        }
        break;

    case APP_HFP_AG_TEST_API_ID_NOTIFY_SPEAKER_VOLUMN:
        {
            bt_hfp_ag_speaker_gain_set(p_link->bd_addr, 5);
            test_app_hfp_ag_timer_start(p_link, APP_HFP_AG_TEST_API_ID_CALL_DIAL, 1000);
        }
        break;

    default:
        break;
    }
}

static void app_hfp_ag_test_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    T_APP_BR_LINK *p_link;
    p_link = &(app_db.br_link[param]);
    app_stop_timer(&timer_idx_hfp_ag_alarm);

    if (p_link == NULL)
    {
        APP_PRINT_ERROR0("app_hfp_ag_test_timeout_cb: no acl link found");
        return;
    }

    switch (timer_evt)
    {
    case APP_HFP_AG_TEST:
        {
            test_app_hfp_ag_api_handle(p_link, test_app_hfp_ag_api_id);
        }
        break;
    }
}

static void test_app_hfp_ag_dm_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    APP_PRINT_ERROR1("app_hfp_ag_dm_cback_test: event_type:%x", event_type);

    switch (event_type)
    {
    case BT_EVENT_BT_READY:
        {
            //Test HFP AG connect.
            APP_PRINT_TRACE0("test_hfp_ag_connect");
            test_hfp_ag_connect(target_bd_addr);
        }
        break;

    case BT_EVENT_ACL_CONN_IND:
        {
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            test_app_hfp_ag_handle_sdp_discovery_info(param->bt_sdp_attr_info.bd_addr,
                                                      &(param->bt_sdp_attr_info.info));
        }
        break;

    case BT_EVENT_SDP_DISCOV_CMPL:
        {
            test_app_hfp_ag_handle_sdp_discovery_cmpl(param->bt_sdp_discov_cmpl.bd_addr,
                                                      param->bt_sdp_discov_cmpl.cause);
        }
        break;

    case BT_EVENT_HFP_AG_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->bt_hfp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                bt_device_mode_set(
                    BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);//Set device mode page scan and inquiry scan enabled.
                //HFP AG Test bt_hfp_ag api.
                test_app_hfp_ag_timer_start(p_link, APP_HFP_AG_TEST_API_ID_NOTIFY_MIC_VOLUMN, 8000);
            }
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            //App send CVSD/MSBC SCO data according to codec type selected.
            //uint8_t sco_buf[60];
            //uint8_t sco_buf_len = 60;
            //bt_sco_data_send(target_bd_addr, sco_buf, sco_buf_len);
        }
        break;

    default:
        break;
    }
}

void test_app_hfp_ag_init(void)
{
    if (app_cfg_const.supported_profile_mask & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
    {
        bt_mgr_cback_register(test_app_hfp_ag_dm_cback);
        app_timer_reg_cb(app_hfp_ag_test_timeout_cb, &hfp_ag_test_timer_id);
    }
}
