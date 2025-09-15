/*
 *  Copyright (C) 2020 Realtek Semiconductor Corporation.
 *
 *  Author: Harvey_Guo <harvey_guo@realsil.com.cn>
 */
#include "trace.h"
#include "bt_rfc.h"
#include "app_spp_vender_passthrough.h"
#include "app_link_util.h"
//#include "app_sdp_a2dp_src.h"
#include "app_usb_hid.h"
#include "app_timer.h"

#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
//const static uint8_t UUID_VENDER_PASSTHROUGH[16] =
//{
//    0x12, 0xA2, 0x4D, 0x2E,
//    0xFE, 0x14,
//    0x48, 0x8e,
//    0x93, 0xD2,
//    0x17, 0x3C, 0x63, 0x01, 0x00, 0x00
//};

extern T_APP_DEV sink_dev_list[];
static uint8_t spp_vender_passthrough_timer_id = 0;
static uint8_t timer_idx_spp_vender_passthrough = 0;

void app_usb_hid_handle_set_vender_passthrough_cmd(uint8_t *data, uint8_t length)
{
    uint8_t ret = 0;

    T_APP_BR_LINK *p_link;
    if (!data || !length || length < 2)
    {
        ret = 1;
        goto failed;
    }
    if (sink_dev_list[0].dev_state != STATE_DEV_CONNECTED ||
        sink_dev_list[0].bt_state != STATE_BT_ACL_CONNECTED)
    {
        ret = 2;
        goto failed;
    }

    p_link = app_find_br_link(sink_dev_list[0].remote_info.bd_addr);
    if (!p_link)
    {
        ret = 3;
        goto failed;
    }

    if (!(p_link->connected_profile & SPP_VENDER_PASSTHROUGH_PROFILE_MASK))
    {
        ret = 4;
        goto failed;
    }

    uint8_t send[69] = {'R', 'T', 'K'};
    send[3] = length;
    memcpy(&send[5], data, length);
    if (send[5] < 0xf0)
    {
        send[5] -= 0x10;
    }
    if (!bt_rfc_data_send(sink_dev_list[0].remote_info.bd_addr, RFC_VENDER_PASSTHROUGH_CHANN_NUM,
                          send, length + 5, false))
    {
        ret = 5;
        goto failed;
    }
    APP_PRINT_TRACE2("app_usb_hid_handle_set_praha_cmd send length %x %b",
                     length + 5, TRACE_BINARY(length + 5, send));
    return;

failed:
    APP_PRINT_ERROR2("app_usb_hid_handle_set_praha_cmd, bd_addr %b error 0x%x",
                     TRACE_BDADDR(sink_dev_list[0].remote_info.bd_addr), ret);
}

uint8_t app_usb_hid_handle_get_vender_passthrough_cmd(uint8_t *data, uint8_t *length)
{
    uint8_t ret = 0;
    T_APP_BR_LINK *p_link;

    if (sink_dev_list[0].dev_state != STATE_DEV_CONNECTED ||
        sink_dev_list[0].bt_state != STATE_BT_ACL_CONNECTED ||
        data[0] == REPORT_ID_PRAHA_CFU_OFFER_INPUT)
    {
        ret = 1;
        goto failed;
    }

    p_link = app_find_br_link(sink_dev_list[0].remote_info.bd_addr);
    if (!p_link)
    {
        ret = 2;
        goto failed;
    }

    if (!(p_link->connected_profile & SPP_VENDER_PASSTHROUGH_PROFILE_MASK))
    {
        ret = 3;
        goto failed;
    }

    uint8_t send[] = {'R', 'T', 'K', 1, 0, REPORT_ID_CFU_FEATURE};
    if (!bt_rfc_data_send(sink_dev_list[0].remote_info.bd_addr, RFC_VENDER_PASSTHROUGH_CHANN_NUM,
                          send, sizeof(send), false))
    {
        ret = 4;
        goto failed;
    }
    APP_PRINT_TRACE2("app_usb_hid_handle_get_vender_passthrough_cmd send length %x %b",
                     sizeof(send), TRACE_BINARY(sizeof(send), send));

    app_start_timer(&timer_idx_spp_vender_passthrough, "hs_cfu_timer",
                    spp_vender_passthrough_timer_id, 0, 0, false,
                    1000);
    return 2;

failed:
    APP_PRINT_ERROR2("app_usb_hid_handle_get_vender_passthrough_cmd: bd_addr %b ret %x",
                     TRACE_BDADDR(sink_dev_list[0].remote_info.bd_addr), ret);
    return 0;
}

static void app_spp_vender_passthrough_cback(uint8_t *bd_addr, BT_RFC_MSG_TYPE msg_type,
                                             void *msg_buf)
{
    T_APP_BR_LINK *p_link = NULL;
    APP_PRINT_INFO1("app_spp_vender_passthrough_cback msg_type 0x%02x, bd_addr %s", msg_type,
                    TRACE_BDADDR(bd_addr));

    p_link = app_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        APP_PRINT_WARN1("app_spp_vender_passthrough_cback: %b no acl link found", TRACE_BDADDR(bd_addr));
    }

    switch (msg_type)
    {
    case BT_RFC_MSG_CONN_IND:
        {
            BT_RFC_CONN_IND *p_ind = (BT_RFC_CONN_IND *)msg_buf;

            if (p_link != NULL)
            {
                bt_rfc_conn_cfm(p_link->bd_addr, p_ind->local_server_chann, true, p_ind->frame_size, 7);
            }
            else
            {
                bt_rfc_conn_cfm(bd_addr, p_ind->local_server_chann, false, p_ind->frame_size, 7);
            }
        }
        break;

    case BT_RFC_MSG_CONN_CMPL:
        {
            /* BT_RFC_CONN_CMPL *p_cmpl = (BT_RFC_CONN_CMPL *)msg_buf; */
            if (p_link != NULL)
            {
                src_handle_profile_conn_cmpl(bd_addr, SPP_VENDER_PASSTHROUGH_PROFILE_MASK);
            }
        }
        break;

    case BT_RFC_MSG_DISCONN_CMPL:
        {
            BT_RFC_DISCONN_CMPL *p_info = (BT_RFC_DISCONN_CMPL *)msg_buf;

            if (p_link != NULL)
            {
                src_handle_profile_disconn_cmpl(bd_addr, p_info->cause,
                                                SPP_VENDER_PASSTHROUGH_PROFILE_MASK);
            }
        }
        break;

    case BT_RFC_MSG_DATA_IND:
        {
            BT_RFC_DATA_IND *p_ind = (BT_RFC_DATA_IND *)msg_buf;
            /*APP_PRINT_INFO2("app_spp_vender_passthrough_cback len %u, data %b",
                            p_ind->length,
                            TRACE_BINARY(p_ind->length, p_ind->buf));*/
            p_link = app_find_br_link(bd_addr);
            if (p_link == NULL)
            {
                return;
            }
            bt_rfc_credits_give(bd_addr, p_ind->local_server_chann, 1);
            switch (p_ind->buf[5])
            {
            case REPORT_ID_CFU_FEATURE:
                {
                    if (timer_idx_spp_vender_passthrough != 0)
                    {
                        app_stop_timer(&timer_idx_spp_vender_passthrough);
                        timer_idx_spp_vender_passthrough = 0;
                    }
                    p_ind->buf[5] += 0x10;
                    app_usb_hid_get_report_data_is_ready(&p_ind->buf[5], 61);
                    break;
                }
            case REPORT_ID_CFU_OFFER_INPUT:
            case REPORT_ID_CFU_PAYLOAD_INPUT:
                {
                    p_ind->buf[5] += 0x10;
                    app_hid_interrupt_in(&p_ind->buf[5], 17);
                    break;
                }
            case REPORT_ID_DONGLE_TEST_INPUT:
                {
                    p_ind->buf[5] += 0x10;
                    app_hid_interrupt_in(&p_ind->buf[5], 61);
                    break;
                }
            case REPORT_ID_PRAHA_VP_INPUT:
                {
                    app_hid_interrupt_in(&p_ind->buf[5], 61);
                    break;
                }
            default:
                {
                    APP_PRINT_WARN1("app_spp_vender_passthrough_cback cannot handle id %x", p_ind->buf[5]);
                }
            }
        }
        break;
    case BT_RFC_MSG_CREDIT_INFO:
        {
            T_BT_RFC_CREDIT_INFO *info = (T_BT_RFC_CREDIT_INFO *)msg_buf;
            APP_PRINT_WARN1("app_spp_vender_passthrough_cback credit remain %x", info->remain_credits);
        }
        break;

    default:
        break;
    }
}

static void app_spp_vender_passthrough_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO2("app_spp_vender_passthrough_timeout_cb: timer_evt %d, param %d",
                    timer_evt, param);
    if (timer_idx_spp_vender_passthrough != 0)
    {
        app_stop_timer(&timer_idx_spp_vender_passthrough);
        timer_idx_spp_vender_passthrough = 0;
    }
    uint8_t send[61] = {0x2a, 0};
    app_usb_hid_get_report_data_is_ready(send, 61);
}

void app_spp_vender_passthrough_init(void)
{
    bt_rfc_service_register(RFC_VENDER_PASSTHROUGH_CHANN_NUM, app_spp_vender_passthrough_cback);
    app_timer_reg_cb(app_spp_vender_passthrough_timeout_cb,
                     &spp_vender_passthrough_timer_id);
}
#endif

