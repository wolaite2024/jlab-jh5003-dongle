#include <string.h>
#include <app_system_call.h>
#include "trace.h"
#include "btm.h"
#include "app_timer.h"
#include "app_link_util.h"
#include "app_usb_dongle_test.h"
#include "app_usb_hid.h"

#define EXT_OP_GROUP_UPPERSTACK                 (0x07000000)    //!< extension opcode group upperstack
#define EXT_OP_GET_ACL_TX_STATUS                (EXT_OP_GROUP_UPPERSTACK | 0x03)//For PATCH_BTDONGLE Project.
#define EXT_OP_SET_ESCO_RECORD_HANDLE           (EXT_OP_GROUP_UPPERSTACK | 0x04)//For PATCH_BTDONGLE Project.
#define EXT_OP_GET_ESCO_RECORD_PARAM            (EXT_OP_GROUP_UPPERSTACK | 0x05)//For PATCH_BTDONGLE Project.

#define HCI_COMMAND_SUCCEEDED                   (0x00)
#define NO_CONNECTION_ERROR                     (0x00)
#define COMMAND_DISALLOWED_ERROR                (0x00)

#define SYSTEM_CALL_PACKET_STATISTICS_TIMEOUT   (1000)

//static uint16_t last_acl_tx_num = 0;
static uint16_t last_acl_tx_acked_num = 0;
static uint16_t last_esco_tx_rx_num = 0;
static uint16_t last_esco_tx_acked_num = 0;
static uint16_t last_esco_tx_no_ack_num = 0;
static uint16_t last_succ_esco_rx_num = 0;
static uint16_t last_fail_esco_rx_num = 0;
static uint8_t system_call_timer_id = 0;
static uint8_t timer_idx_system_call_acl = 0;
static uint8_t timer_idx_system_call_esco = 0;
static uint8_t bd_addr[6] = {0};
extern uint8_t rf_test_mode;

extern uint32_t (*SystemCall)(uint32_t opcode, uint32_t param);

void acl_tx_and_acked_num_get(uint16_t *acl_tx_num, uint16_t *acl_tx_acked_num)
{
    uint32_t ret = 0;

    ret = SystemCall(EXT_OP_GET_ACL_TX_STATUS, 0);

    *acl_tx_num = ret >> 16;
    *acl_tx_acked_num = ret & 0x00000000ffffffff;
}

bool esco_tx_and_rx_record_get(ESCO_RECORD_EVT_PARAM *esco_record, int16_t conn_handle)
{
    bool ret = false;

    if (SystemCall(EXT_OP_SET_ESCO_RECORD_HANDLE, conn_handle) != HCI_COMMAND_SUCCEEDED)
    {
        goto failed;
    }

    if (SystemCall(EXT_OP_GET_ESCO_RECORD_PARAM, (uint32_t)esco_record) != HCI_COMMAND_SUCCEEDED)
    {
        goto failed;
    }

    ret = true;

failed:
    return ret;
}


static void app_system_call_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            if (memcmp(bd_addr, param->acl_conn_success.bd_addr, 6) != 0)
            {
                memcpy(bd_addr, param->acl_conn_success.bd_addr, 6);
                app_start_timer(&timer_idx_system_call_acl, "system_call_acl_timer",
                                system_call_timer_id, APP_TIMER_SYSTEM_CALL_ACL_PACKET_STATISTICS, 0, false,
                                SYSTEM_CALL_PACKET_STATISTICS_TIMEOUT);
            }
        }
        break;
    case BT_EVENT_ACL_CONN_DISCONN:
        {
            app_stop_timer(&timer_idx_system_call_acl);
            last_acl_tx_acked_num = 0;
            memset(bd_addr, 0, 6);
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            app_start_timer(&timer_idx_system_call_esco, "system_call_esco_timer",
                            system_call_timer_id, APP_TIMER_SYSTEM_CALL_ESCO_PACKET_STATISTICS, 0, false,
                            SYSTEM_CALL_PACKET_STATISTICS_TIMEOUT);
        }
        break;

    case BT_EVENT_SCO_DISCONNECTED:
        {
            app_stop_timer(&timer_idx_system_call_esco);
            last_succ_esco_rx_num = 0;
            last_fail_esco_rx_num = 0;
            last_esco_tx_rx_num = 0;
            last_esco_tx_acked_num = 0;
            last_esco_tx_no_ack_num = 0;
        }
        break;
    default:
        break;
    }
}

static void app_system_call_report_statistics(uint8_t *data)
{
    APP_SYSTEM_CALL_REPORT_STATISTICS *res = (APP_SYSTEM_CALL_REPORT_STATISTICS *)data;
    res->commandId = TEST_CMD_ID_ACL_ESCO_STATIS;
    res->payLength = sizeof(APP_SYSTEM_CALL_REPORT_STATISTICS) - 2;
    uint8_t length = sizeof(APP_SYSTEM_CALL_REPORT_STATISTICS);
    if (rf_test_mode)
    {
        app_usb_hid_send_report(HID_IF_TEST, 0, data, length, 0);
    }
}


static void system_call_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO2("system_call_timeout_cb: timer_evt %d, param %d",
                    timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_SYSTEM_CALL_ACL_PACKET_STATISTICS:
        {
            app_stop_timer(&timer_idx_system_call_acl);

            uint16_t acl_tx_num = 0;
            uint16_t acl_tx_acked_num = 0;
            uint16_t acl_tx_acked_diff = 0;
            acl_tx_and_acked_num_get(&acl_tx_num, &acl_tx_acked_num);

            acl_tx_acked_diff = acl_tx_acked_num - last_acl_tx_acked_num;
            last_acl_tx_acked_num = acl_tx_acked_num;

            APP_SYSTEM_CALL_REPORT_STATISTICS data;
            data.type = 0x0200;
            data.total_num = ((acl_tx_acked_diff & 0x00FF) << 8) | ((acl_tx_acked_diff & 0xFF00) >> 8);
            data.acked_num = ((acl_tx_acked_diff & 0x00FF) << 8) | ((acl_tx_acked_diff & 0xFF00) >> 8);
            data.no_ack_num = 0x0000;
            app_system_call_report_statistics((uint8_t *)&data);

            app_start_timer(&timer_idx_system_call_acl, "system_call_acl_timer",
                            system_call_timer_id, APP_TIMER_SYSTEM_CALL_ACL_PACKET_STATISTICS, 0, false,
                            SYSTEM_CALL_PACKET_STATISTICS_TIMEOUT);
        }
        break;

    case APP_TIMER_SYSTEM_CALL_ESCO_PACKET_STATISTICS:
        {
            app_stop_timer(&timer_idx_system_call_esco);

            T_APP_BR_LINK *p_link;
            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                bool ret;
                ESCO_RECORD_EVT_PARAM esco_record;
                ret = esco_tx_and_rx_record_get(&esco_record, p_link->sco_handle);
                if (ret)
                {
                    uint16_t esco_tx_acked_num = esco_record.rcv_ack_tx_time[0] + esco_record.rcv_ack_tx_time[1] +
                                                 esco_record.rcv_ack_tx_time[2] + esco_record.rcv_ack_tx_time[3];
                    uint16_t esco_rx_succ_num = esco_record.rcv_data_rx_time[0] + esco_record.rcv_data_rx_time[1] +
                                                esco_record.rcv_data_rx_time[2] + esco_record.rcv_data_rx_time[3];
                    if ((esco_tx_acked_num + esco_record.no_rcv_ack +  esco_record.tx_poll_or_null) !=
                        esco_record.instant_times)
                    {
                        APP_PRINT_INFO0("eSCO tx packet statistics error!");
                    }
                    else
                    {
                        uint16_t total_tx_num = esco_record.instant_times - last_esco_tx_rx_num;
                        uint16_t acked_tx_num = esco_tx_acked_num - last_esco_tx_acked_num;
                        uint16_t no_ack_tx_num = esco_record.no_rcv_ack - last_esco_tx_no_ack_num;
                        APP_SYSTEM_CALL_REPORT_STATISTICS data;
                        data.type = 0x0000;
                        data.total_num = ((total_tx_num & 0x00FF) << 8) | ((total_tx_num & 0xFF00) >> 8);
                        data.acked_num = ((acked_tx_num & 0x00FF) << 8) | ((acked_tx_num & 0xFF00) >> 8);
                        data.no_ack_num = ((no_ack_tx_num & 0x00FF) << 8) | ((no_ack_tx_num & 0xFF00) >> 8);
                        app_system_call_report_statistics((uint8_t *)&data);

                        uint16_t total_rx_num = esco_record.instant_times - last_esco_tx_rx_num;
                        uint16_t acked_rx_num = esco_rx_succ_num - last_succ_esco_rx_num;
                        uint16_t no_ack_rx_num = esco_record.no_rcv_data - last_fail_esco_rx_num;
                        data.type = 0x0100;
                        data.total_num = ((total_rx_num & 0x00FF) << 8) | ((total_rx_num & 0xFF00) >> 8);
                        data.acked_num = ((acked_rx_num & 0x00FF) << 8) | ((acked_rx_num & 0xFF00) >> 8);
                        data.no_ack_num = ((no_ack_rx_num & 0x00FF) << 8) | ((no_ack_rx_num & 0xFF00) >> 8);
                        app_system_call_report_statistics((uint8_t *)&data);

                        last_esco_tx_rx_num = esco_record.instant_times;
                        last_esco_tx_acked_num = esco_tx_acked_num;
                        last_esco_tx_no_ack_num = esco_record.no_rcv_ack;
                        last_succ_esco_rx_num = esco_rx_succ_num;
                        last_fail_esco_rx_num = esco_record.no_rcv_data;
                    }
                }
            }

            app_start_timer(&timer_idx_system_call_esco, "system_call_esco_timer",
                            system_call_timer_id, APP_TIMER_SYSTEM_CALL_ESCO_PACKET_STATISTICS, 0, false,
                            SYSTEM_CALL_PACKET_STATISTICS_TIMEOUT);
        }
        break;

    default:
        break;
    }
}


void app_system_call_init(void)
{
    bt_mgr_cback_register(app_system_call_bt_cback);
    app_timer_reg_cb(system_call_timeout_cb, &system_call_timer_id);
}


