#include <string.h>
#include <stdlib.h>
#include <bt_types.h>
#include "trace.h"
//#include "os_sched.h"
#include "os_mem.h"
#include "os_queue.h"
#include "os_timer.h"
#include "os_sync.h"
#include "rtl876x.h"
#include "stdlib.h"
#include "app_timer.h"
#include "app_usb_hid.h"
#include "app_io_msg.h"
#include "app_src_asp.h"
#include "app_dongle_vol.h"
#include "app_usb_passthrough.h"
#include "app_cfu_passthrough.h"
#include "app_usb_hid_wrapper.h"
#include "app_usb_call_control.h"
#include "app_cfg.h"
#include "usb_hid.h"
#include "errno.h"
#include "teams_call_control.h"
#if F_APP_HID_RTK_SUPPORT
#include "app_hid_rtk_ctrl.h"
#endif
#include "app_usb_hid_report.h"


#ifdef F_APP_CFU_RTL8773DO_DONGLE
#include "app_common_cfu.h"
#endif

#if F_APP_GAMING_CONTROLLER_SUPPORT
#include "app_usb_controller.h"
#endif

#if (F_APP_SS_REVISE_HID == 1)
static void *hid_intr_in_handle = NULL;
#endif

#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT

APP_USB_HID_CB_F app_usb_hid_cb_f = {0};
static T_USB_HID_DB usb_hid_db  = {.p_sema = NULL, .p_get_report_data = NULL, .get_report_len = 0};

//////////////consumer ctrl///////////////
bool app_usb_hid_send_consumer_ctrl_key_down(enum CONSUMER_CTRL_KEY_CODE key)
{
    uint8_t send_buf[3] = {0x02, key, 0};
    return usb_hid_report_buffered_send(send_buf, 3);
}

bool app_usb_hid_send_consumer_ctrl_all_key_release()
{
    uint8_t send_buf[3] = {0x02, 0, 0};
    return usb_hid_report_buffered_send(send_buf, 3);
}

//////////////telephony ctrl///////////////
bool app_usb_hid_send_telephony_ctrl_code(uint16_t *code)
{
    uint8_t send_buf[3] = {REPORT_ID_TELEPHONY_INPUT, 0x0, 0x0};
    send_buf[1] = (*code) & 0xFF;
    send_buf[2] = (*code >> 8) & 0xFF;
    return usb_hid_report_buffered_send(send_buf, 3);
}

bool app_usb_hid_send_telephony_mute_ctrl(bool mute)
{
    T_TELEPHONY_HID_INPUT ctrl_code = {0};
    bool ret1 = false;
    bool ret2 = false;

    APP_PRINT_TRACE1("app_usb_hid_send_telephony_mute_ctrl: %d", mute);

    ctrl_code.hook_switch = 1;
    ctrl_code.mute = !mute;

    ret1 = app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);

    ctrl_code.mute = mute;
    ret2 = app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);

    return ret1 && ret2;
}

__weak void app_cfu_handle_set_report(uint8_t *data, uint8_t length) {};
__weak void app_usb_hid_handle_passthrough(uint8_t *data, uint8_t length) {};
void app_usb_hid_handle_set_report_msg(uint8_t *buf, uint16_t length)
{
    uint8_t *data = buf + sizeof(T_HID_DRIVER_REPORT_REQ_VAL);
//    APP_PRINT_TRACE4("app_usb_hid_handle_set_report_msg type 0x%x, ID %x, length 0x%x, %b",
//                     buf[0], buf[1], length, TRACE_BINARY(MIN(length, 5), data));
    extern uint8_t app_get_cur_bt_mode(void);
    switch (data[0])
    {
    case REPORT_ID_CFU_FEATURE_EX:
    case REPORT_ID_CFU_OFFER_INPUT:
    case REPORT_ID_CFU_FEATURE:
    case REPORT_ID_CFU_PAYLOAD_INPUT:

#ifndef F_APP_CFU_RTL8773DO_DONGLE
        app_cfu_handle_set_report(data, length);
#else
        {
            uint8_t report_id = data[0];
            T_APP_CFU_DATA cfu_data;

            cfu_data.usb_data.data_len = length - 1;
            cfu_data.usb_data.p_data = &data[1];
            app_cfu_handle_report(report_id, APP_CFU_REPORT_SOURCE_USB, &cfu_data);
        }
#endif

        break;
    case REPORT_ID_TELEPHONY_INPUT:
        if ((app_usb_hid_cb_f.app_usb_hid_telephony_data_recv_cb)
#if (APP_PID != 2000)
            && (app_get_cur_bt_mode() == 2)
#endif
           )
        {
            uint16_t code = (data[2] << 8) | data[1];
            app_usb_hid_cb_f.app_usb_hid_telephony_data_recv_cb(code);
        }
#ifdef LEGACY_BT_GENERAL
        if (app_get_cur_bt_mode() == 0 && (app_cfg_const.dongle_media_device != 3))
        {
            app_usb_hid_handle_set_Telephony_cmd(data, length);
        }
#endif
        usb_hid_db.telephony_output = (data[2] << 8) | data[1];
        break;
    case REPORT_ID_CTRL_DATA_OUT_REQUEST:
#if (F_APP_HID_RTK_SUPPORT == 1)
        app_hid_rtk_recv_data(data, length);
#endif
        break;
#ifdef LEGACY_BT_GENERAL
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    case REPORT_ID_ASP_FEATURE:
        app_usb_hid_handle_set_ASP_cmd(data, length);
        break;
#endif
#endif
#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
    case REPORT_ID_PASSTHROUGH_CMD_OUTPUT:
        app_usb_hid_handle_passthrough(data, length);
        break;
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)
    case REPORT_ID_PASSTHROUGH_TEST_FEATURE:
    case REPORT_ID_PASSTHROUGH_CFU_FEATURE_EX:
    case REPORT_ID_PASSTHROUGH_CFU_OFFER_INPUT:
    case REPORT_ID_PASSTHROUGH_CFU_FEATURE:
    case REPORT_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT:
        app_usb_hid_handle_cfu_passthrough_set_report(data, length);
        break;
#endif
    default:
        break;
    }
}

uint16_t app_usb_hid_get_telephony_output(void)
{
    return usb_hid_db.telephony_output;
}

// report type HID_REPORT_TYPE_INPUT /OUTPUT/FEATURE
/*|byte0|byte1|byte2|**/
/*|report type|report id| report id + data*/
int8_t app_usb_hid_handle_set_report(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                     uint16_t length)
{
    uint8_t *data_buf = NULL;
    uint16_t data_len = (MIN(length, 0x3f)) + sizeof(T_HID_DRIVER_REPORT_REQ_VAL);
    APP_PRINT_TRACE4("app_usb_hid_handle_set_report id 0x%x, type 0x%x, length %x, %b",
                     req_value.id, req_value.type, length, TRACE_BINARY(MIN(length, 7), data));

    data_buf = malloc(data_len);
    if (data_buf == NULL)
    {
        return -ENOMEM;
    }
    data_buf[0] = req_value.type;
    data_buf[1] = req_value.id;
    memcpy((data_buf + sizeof(T_HID_DRIVER_REPORT_REQ_VAL)), data, MIN(length, 0x3f));

    T_IO_MSG gpio_msg = {0};
    gpio_msg.type = IO_MSG_TYPE_USB_HID;
    gpio_msg.subtype = USB_HID_MSG_TYPE_HID_SET_REPORT | (length << 8);
    gpio_msg.u.buf = data_buf;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        free(data_buf);
    }
    return 0;
}

__weak uint8_t app_cfu_handle_get_report(uint8_t *data, uint16_t *length) {return 0;};
int8_t app_usb_hid_handle_get_report(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                     uint16_t *length)
{
    APP_PRINT_TRACE2("app_usb_hid_handle_get_report, id: 0x%x, len:%d", req_value.id, *length);
    uint8_t result = 0;
    uint8_t reportID = req_value.id;

    switch (reportID)
    {
    case REPORT_ID_CFU_FEATURE:
    case REPORT_ID_CFU_FEATURE_EX:

#ifndef F_APP_CFU_RTL8773DO_DONGLE
        app_cfu_handle_get_report(data, length);
#else
        data[0] = REPORT_ID_CFU_FEATURE;
        memset(&data[1], 0, sizeof(FW_UPDATE_VER_RESPONSE));
        app_cfu_get_version_parse((FW_UPDATE_VER_RESPONSE *)&data[1]);
        *length =  sizeof(FW_UPDATE_VER_RESPONSE) + 1;
#endif

        break;
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    case REPORT_ID_ASP_FEATURE:
        result = app_usb_hid_handle_get_ASP_cmd(data, length);
        break;
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)
    case REPORT_ID_PASSTHROUGH_CFU_FEATURE:
        result = app_usb_hid_handle_cfu_passthrough_get_report(data, length);
        break;
#endif
    default:
        break;
    }

#if ((MICROSOFT_HS_ASP_FEATURE_SUPPORT == 1) || (USB_PASSTHROUGH_CFU_SUPPORT == 1))
    if (result == 2)
    {
        usb_hid_db.p_get_report_data = data;
        usb_hid_db.get_report_len = *length;
        APP_PRINT_WARN0("app_usb_hid_handle_get_report waiting...");
        if (usb_hid_db.p_sema == NULL)
        {
            if (os_sem_create(&usb_hid_db.p_sema, "usb_hid_sema", 0, 1) == false)
            {
                return 0;
            }
        }
        if (os_sem_take(usb_hid_db.p_sema, 0xFFFFFFFFUL))
        {
            usb_hid_db.p_get_report_data = NULL;
            usb_hid_db.get_report_len = 0;
            result = 0;
        }
    }
#endif

    APP_PRINT_TRACE4("app_usb_hid_handle_get_report ID %x, result %d, length 0x%x, %b", reportID,
                     result, *length, TRACE_BINARY(MIN(8, *length), data));
    return result;
}

// when get report need waiting data, the API is used.
bool app_usb_hid_get_report_data_is_ready(uint8_t *data, uint8_t length)
{
    bool ret = true;

    if (usb_hid_db.p_get_report_data)
    {
        if (usb_hid_db.get_report_len != length)
        {
            ret = false;
        }
        memset(usb_hid_db.p_get_report_data, 0, MIN(usb_hid_db.get_report_len, APP_USB_HID_MAX_IN_SIZE));
        usb_hid_db.get_report_len = MIN(length, usb_hid_db.get_report_len);
        memcpy(usb_hid_db.p_get_report_data, data, usb_hid_db.get_report_len);
    }
    else
    {
        ret = false;
    }

    if (ret == false)
    {
        APP_PRINT_ERROR2("app_usb_hid_get_report_data_is_ready fail(%p), len:%d",
                         usb_hid_db.p_get_report_data, length);
    }
    os_sem_give(usb_hid_db.p_sema);
    return ret;
}

void app_usb_hid_send_report(SEND_TYPE type, uint8_t id, uint8_t *data, uint8_t length,
                             uint32_t pnpinfo)
{
    uint8_t app_usb_hid_interrupt_in_buff[APP_USB_HID_MAX_IN_SIZE];
    uint8_t sendcount = MIN(length + 1, APP_USB_HID_MAX_IN_SIZE);
    memset(app_usb_hid_interrupt_in_buff, 0, sizeof(app_usb_hid_interrupt_in_buff));
    memcpy(&app_usb_hid_interrupt_in_buff[1], data, sendcount - 1);
    switch (type)
    {
    case HID_IF_KEYBOARD:
    case HID_IF_MOUSE:
        switch (pnpinfo)
        {
        case 0x0212082f:
            //app_usb_hid_interrupt_in_buff[0] = app_usb_hid_match_id(mouse_0x0212082fid_map, id);
            break;
        default:
            app_usb_hid_interrupt_in_buff[0] = id;
            break;
        }
        break;
    case HID_IF_CONSUMER:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
        sendcount = 3;
        break;
    case HID_IF_HIDSYSCTRL:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_SYS_CTRL;
        sendcount = 3;
        break;
    case HID_IF_TELEPHONY:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_TELEPHONY_INPUT;
        sendcount = 3;
        break;
    case HID_IF_ASP:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_ASP_INPUT;
        sendcount = 3;
        break;
//    case HID_IF_GATT:
//        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_GATT_INPUT;
//        sendcount = 61;
//        break;
    case HID_IF_GAIA:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_GAIA_INPUT;
        sendcount = 61;
        break;
//    case HID_IF_CTRL:
//        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_DONGLE_CTRL_INPUT;
//        sendcount = 61;
//        break;
    case HID_IF_TEST:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_CTRL_DATA_IN_REQUEST;
        sendcount = 61;
        break;
    default:
        break;
    }

    usb_hid_report_buffered_send(app_usb_hid_interrupt_in_buff, sendcount);
}

#if (F_APP_SS_REVISE_HID == 1)
uint32_t app_usb_hid_interrupt_in_complete_result(void *handle, void *buf, uint32_t result,
                                                  int status)
{
    APP_PRINT_TRACE2("app_usb_hid_interrupt_in_complete_result %x, buf  %x", result, buf);
    T_IO_MSG gpio_msg = {0};
    gpio_msg.type = IO_MSG_TYPE_USB_HID;
    gpio_msg.subtype = USB_HID_MSG_TYPE_HID_IN_COMPLETE;
    //gpio_msg.u.buf = buf;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        //free(--buf);
        APP_PRINT_ERROR0("hid_interrupt_in_complete_result: msg send fail");
    }
    return 0;
}

#if USBLIB_LEGACY
bool app_usb_hid_interrupt_in(uint8_t *data, uint8_t data_size)
{
    return true;
}
#else
bool app_usb_hid_interrupt_in(uint8_t *data, uint8_t data_size)
{
    uint8_t sendlength = MIN(data_size, 64);

    APP_PRINT_TRACE2("app_hid_interrupt_in data_size %u %b", data_size,
                     (data_size <= 16 ? TRACE_BINARY(data_size, data) :
                      TRACE_BINARY(16, data)));
    if (hid_intr_in_handle == NULL)
    {
        T_USB_HID_ATTR attr =
        {
            .zlp = 1,
            .high_throughput = 0,
            .rsv = 0,
            .mtu = HID_MAX_TRANSMISSION_UNIT
        };
        hid_intr_in_handle = usb_hid_data_pipe_open(HID_INT_IN_EP_1, attr, HID_MAX_PENDING_REQ_NUM,
                                                    app_usb_hid_interrupt_in_complete_result);
    }
    bool result = usb_hid_data_pipe_send(hid_intr_in_handle, data, sendlength);
    APP_PRINT_TRACE2("app_hid_interrupt_in result %d, buf %p", result, data);
    return result;
}
#endif
#else
void app_usb_hid_interrupt_in_complete_result(int result, uint8_t *buf);
#if USBLIB_LEGACY
bool app_usb_hid_interrupt_in(uint8_t *data, uint8_t data_size)
{
    return true;
}
#else
bool app_usb_hid_interrupt_in(uint8_t *data, uint8_t data_size)
{
    uint8_t sendlength = MIN(data_size, 64);
    uint8_t *buf = os_mem_zalloc(RAM_TYPE_DSPSHARE, sendlength);

    APP_PRINT_TRACE2("app_hid_interrupt_in data_size %u %b", data_size,
                     (data_size <= 16 ? TRACE_BINARY(data_size, data) :
                      TRACE_BINARY(16, data)));

    if (!buf)
    {
        return false;
    }

    memcpy(buf, data, sendlength);
    /* FIXME: The hid driver api is not reentrant code.
     * Make exclusive access to usb hid lower level code.
     *
     * We just use TIM3 to simulate hid reports sometimes.
     * */
    NVIC_DisableIRQ(SPI0_IRQn);
    NVIC_DisableIRQ(TIM3_IRQn);
    bool result = app_usb_hid_interrupt_pipe_send(buf, sendlength);
    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(SPI0_IRQn);
    os_mem_free(buf);
    return result;
}
#endif

void app_usb_hid_interrupt_in_complete_result(int result, uint8_t *buf)
{
    APP_PRINT_TRACE2("app_usb_hid_interrupt_in_complete_result %x, buf  %x", result, buf);
    T_IO_MSG gpio_msg = {0};
    gpio_msg.type = IO_MSG_TYPE_USB_HID;
    gpio_msg.subtype = USB_HID_MSG_TYPE_HID_IN_COMPLETE;
    //gpio_msg.u.buf = buf;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        //free(--buf);
    }
    /* FIXME: The context here might be interrupt context.
     * Releasing mem in interrupt context would cause hardfault.
     * */
}
#endif

__weak void app_usb_hid_handle_io_asp_msg(T_IO_MSG *msg) {return ;};
__weak void app_usb_hid_handle_io_gaia_msg(T_IO_MSG *msg) {return ;};

void app_usb_hid_handle_msg(T_IO_MSG *msg)
{
    uint16_t hid_msg_type = msg->subtype & 0xff;
    switch (hid_msg_type)
    {
    case USB_HID_MSG_TYPE_ASP:
        app_usb_hid_handle_io_asp_msg(msg);
        break;
    case USB_HID_MSG_TYPE_GAIA:
        app_usb_hid_handle_io_gaia_msg(msg);
        break;
    case USB_HID_MSG_TYPE_HID_SET_REPORT:
        {
            app_usb_hid_handle_set_report_msg(msg->u.buf, (msg->subtype) >> 8);
            free(msg->u.buf);
        }
        break;
    case USB_HID_MSG_TYPE_CONSUMER_CRTL:
        {
            app_dongle_handle_a2u_set_vol(msg->u.param, false);
        }
        break;
    case USB_HID_MSG_TYPE_HID_SUSPEND_RESUME:
        {
            T_IO_MSG gpio_msg = {0};
            gpio_msg.type = IO_MSG_TYPE_USB_HID;
            gpio_msg.subtype = USB_HID_MSG_TYPE_ASP;
            gpio_msg.u.param = 0xd0;
            if (app_io_msg_send(&gpio_msg) == false)
            {
            }
            // 1:suspend 2:resume
            APP_PRINT_TRACE1("app_usb_hid_handle_msg 1_suspend 2_resume param:%d", msg->u.param);
        }
        break;
    case USB_HID_MSG_TYPE_CTRL_DATA_REQUEST:
        {
#if (F_APP_HID_RTK_SUPPORT == 1)
            app_hid_rtk_ctrl_handle_send_msg();
#endif
        }
        break;
    case USB_HID_MSG_TYPE_HID_IN_COMPLETE:
        {
            extern void app_dongle_hid_send_cmpl(void);
            app_dongle_hid_send_cmpl();
        }
        break;
    case USB_HID_MSG_TYPE_HID_BUFFERED_REPORT:
        {
            usb_hid_report_handle();
        }
        break;
    default:
        break;
    }
}

#if F_APP_GAMING_CONTROLLER_SUPPORT
void app_usb_hid_game_pad_test_mmi(void)
{
    static uint8_t s_cnt = 0;
    s_cnt++;

    GAME_PAD_HID_INPUT_REPORT gd_data;
    memset(&gd_data, 0, sizeof(GAME_PAD_HID_INPUT_REPORT));
    gd_data.id = REPORT_ID_GAME_PAD_INPUT;
    if (s_cnt == 1)
    {
        gd_data.X = 0x82;
        gd_data.Y = 0x80;
        gd_data.Z = 0x7F;
        gd_data.Rz = 0x7F;
        gd_data.hat_switch = 8;
        gd_data.button_1 = 1;
        app_usb_hid_interrupt_in((uint8_t *)&gd_data, sizeof(gd_data));
    }
    else if (s_cnt == 2)
    {
        gd_data.X = 0x70;
        gd_data.Y = 0x60;
        gd_data.Z = 0x7;
        gd_data.Rz = 0x7F;
        gd_data.Rx = 0x02;
        gd_data.Ry = 0x08;
        gd_data.hat_switch = 6;
        gd_data.button_3 = 1;
        gd_data.button_5 = 1;
        app_usb_hid_interrupt_in((uint8_t *)&gd_data, sizeof(gd_data));
    }
    else
    {
        gd_data.X = 0x0;
        gd_data.Y = 0x0;
        gd_data.Z = 0x88;
        gd_data.Rz = 0x72;
        gd_data.Rx = 0x72;
        gd_data.Ry = 0x78;
        gd_data.hat_switch = 0;
        gd_data.vnd_data = 0x20;
        gd_data.button_9 = 1;
        gd_data.button_15 = 1;
        app_usb_hid_interrupt_in((uint8_t *)&gd_data, sizeof(gd_data));
        s_cnt = 0;
    }

}
#endif

bool app_usb_hid_send_bt_ctrl_data(uint16_t length, uint8_t *buf)
{
#if (F_APP_HID_RTK_SUPPORT == 1)
    return app_hid_rtk_ctrl_send(length, buf);
#else
    return true;
#endif
}

void app_usb_hid_register_cbs(APP_USB_HID_CB_F *functions)
{
    app_usb_hid_cb_f.app_usb_hid_bt_ctrl_data_recv_cb = functions->app_usb_hid_bt_ctrl_data_recv_cb;
    app_usb_hid_cb_f.app_usb_hid_telephony_data_recv_cb = functions->app_usb_hid_telephony_data_recv_cb;
}

void app_usb_hid_init(void)
{
    usb_hid_report_register_cb(app_usb_hid_interrupt_in, 512, IO_MSG_TYPE_USB_HID,
                               USB_HID_MSG_TYPE_HID_BUFFERED_REPORT);

#if (F_APP_SS_REVISE_HID == 1)
    usb_hid_init();
    if (hid_intr_in_handle == NULL)
    {
        T_USB_HID_ATTR attr =
        {
            .zlp = 1,
            .high_throughput = 0,
            .rsv = 0,
            .mtu = HID_MAX_TRANSMISSION_UNIT
        };
        hid_intr_in_handle = usb_hid_data_pipe_open(HID_INT_IN_EP_1, attr, HID_MAX_PENDING_REQ_NUM,
                                                    app_usb_hid_interrupt_in_complete_result);
    }
    APP_PRINT_INFO1("app_usb_hid_init:0x%x", hid_intr_in_handle);
#endif

#if (F_APP_SS_REVISE_HID == 0)
#if USBLIB_LEGACY
#else
    app_usb_hid_wrapper_init();
#endif
#else
    T_HID_CBS cbs = {.set_report = (INT_OUT_FUNC)app_usb_hid_handle_set_report, .get_report = (INT_IN_FUNC)app_usb_hid_handle_get_report};
    usb_hid_ual_register(cbs);
#endif

#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
    app_usb_hid_pt_init();
#endif

#if (F_APP_HID_RTK_SUPPORT == 1)
    app_hid_rtk_init();
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)
    app_usb_hid_cfu_pt_init();
#endif

}
#endif
