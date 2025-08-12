#include <string.h>
#include <stdlib.h>
#include <bt_types.h>
#include "trace.h"
#include "os_queue.h"
#include "os_timer.h"
#include "rtl876x.h"
#include "stdlib.h"
#include "app_usb_hid.h"
#include "app_cfu_passthrough.h"
#include "app_spp_cmd.h"
#include "app_timer.h"

#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)

P_CFU_PT_CB cfu_passthrough_cb;

#define CFU_PAYLOAD_LEN_MAX         (60)
#define CFU_PKG_LEN_MAX             (CFU_PAYLOAD_LEN_MAX+1)
#define CFU_PT_SEND_TIMEOUT         (1000)    //100ms
/* pt_send_timer used for wait ack, when timeout resend */
static uint8_t timer_idx_cfu_pt_send = 0;
static uint8_t cfu_pt_timer_id = 0;

static void cfu_pt_send_timer_start(void);
static void cfu_pt_send_timer_stop(void);

typedef enum
{
    CFU_PT_TIMER_ID_SEND = 0x01,
} T_CFU_PT_TIMER_ID;

static uint8_t transfer_report_id(uint8_t id)
{
    uint8_t transfer_id = 0;
    switch (id)
    {
    case REPORT_ID_CFU_FEATURE:
        transfer_id = REPORT_ID_PASSTHROUGH_CFU_FEATURE;
        break;
    case REPORT_ID_CFU_FEATURE_EX:
        transfer_id = REPORT_ID_PASSTHROUGH_CFU_FEATURE_EX;
        break;
    case REPORT_ID_CFU_OFFER_INPUT:
        transfer_id = REPORT_ID_PASSTHROUGH_CFU_OFFER_INPUT;
        break;
    case REPORT_ID_CFU_PAYLOAD_INPUT:
        transfer_id = REPORT_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT;
        break;
    case REPORT_ID_PASSTHROUGH_CFU_FEATURE:
        transfer_id = REPORT_ID_CFU_FEATURE;
        break;
    case REPORT_ID_PASSTHROUGH_CFU_FEATURE_EX:
        transfer_id = REPORT_ID_CFU_FEATURE_EX;
        break;
    case REPORT_ID_PASSTHROUGH_CFU_OFFER_INPUT:
        transfer_id = REPORT_ID_CFU_OFFER_INPUT;
        break;
    case REPORT_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT:
        transfer_id = REPORT_ID_CFU_PAYLOAD_INPUT;
        break;
    case REPORT_ID_PASSTHROUGH_TEST_FEATURE:
        transfer_id = REPORT_ID_CTRL_DATA_OUT_REQUEST;
        break;
    case REPORT_ID_PASSTHROUGH_TEST_INPUT:
        transfer_id = REPORT_ID_CTRL_DATA_IN_REQUEST;
        break;
    case REPORT_ID_CTRL_DATA_OUT_REQUEST:
        transfer_id = REPORT_ID_PASSTHROUGH_TEST_FEATURE;
        break;
    case REPORT_ID_CTRL_DATA_IN_REQUEST:
        transfer_id = REPORT_ID_PASSTHROUGH_TEST_INPUT;
        break;
    default:
        break;
    }
    return transfer_id;
}

void app_usb_hid_handle_cfu_passthrough_set_report(uint8_t *data, uint16_t length)
{
    if ((!data) || (!length))
    {
        return;
    }
    data[0] = transfer_report_id(data[0]);
    if (cfu_passthrough_cb)
    {
        cfu_passthrough_cb(data, length);
        //cfu_pt_send_timer_start();
    }
}

uint8_t app_usb_hid_handle_cfu_passthrough_get_report(uint8_t *data, uint16_t *length)
{
    if (!data)
    {
        return 0;
    }

    uint8_t spp_data[3] = {1, 0, 0};
    spp_data[2] = transfer_report_id(REPORT_ID_PASSTHROUGH_CFU_FEATURE);
    APP_PRINT_INFO1("app_usb_hid_handle_cfu_passthrough_get_report %x", cfu_passthrough_cb);
    if (cfu_passthrough_cb)
    {
        cfu_passthrough_cb(spp_data, sizeof(spp_data));
        cfu_pt_send_timer_start();
    }
    return 2;
}

/* FIXME: alway send USB_PT_PKG_LEN_MAX in pkg no mater length is less than USB_PT_PKG_LEN_MAX */
bool app_usb_hid_send_cfu_passthrough(uint8_t *data, uint16_t length)
{
    if ((!data) || (!length))
    {
        return false;
    }

    if (length > CFU_PKG_LEN_MAX)
    {
        return false;
    }

    data[0] = transfer_report_id(data[0]);
    app_usb_hid_interrupt_in(data, length);
    return true;
}

bool app_cfu_pt_handle_spp_received(uint8_t *data, uint16_t len)
{
    APP_PRINT_INFO2("app_cfu_pt_handle_spp_received data %b len %d", TRACE_BINARY(6, data), len);
    uint16_t payload_len = data[3] + (data[4] << 8);

    if (data[5] == REPORT_ID_CFU_FEATURE)
    {
        cfu_pt_send_timer_stop();
        data[5] = transfer_report_id(data[5]);
        app_usb_hid_get_report_data_is_ready(data + 5, payload_len);
    }
    else
    {
        app_usb_hid_send_cfu_passthrough(data + 5, payload_len);
    }
    return true;
}


static void cfu_pt_send_timer_start(void)
{
    app_start_timer(&timer_idx_cfu_pt_send, "cfu_pt_send",
                    cfu_pt_timer_id, CFU_PT_TIMER_ID_SEND, 0, false,
                    CFU_PT_SEND_TIMEOUT);
}

static void cfu_pt_send_timer_stop(void)
{
    app_stop_timer(&timer_idx_cfu_pt_send);
}


static void cfu_pt_handle_send_timeout(void)
{
    uint8_t send[CFU_PKG_LEN_MAX] = {REPORT_ID_PASSTHROUGH_CFU_FEATURE, 0};
    app_usb_hid_get_report_data_is_ready(send, sizeof(send));
}

static void cfu_pt_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case CFU_PT_TIMER_ID_SEND:
        cfu_pt_send_timer_stop();
        cfu_pt_handle_send_timeout();
        break;

    default:
        break;
    }
}

bool cfu_pt_cb(uint8_t *data, uint16_t len)
{
    uint8_t *p_pkg = calloc(1, len + 5);
    if (p_pkg == NULL)
    {
        APP_PRINT_ERROR1("cfu_pt_cb zalloc len %d fail", len + 5);
        return false;
    }
    if (data[0] == 1 && data[1] == 0 && data[2] == REPORT_ID_CFU_FEATURE)
    {
        p_pkg[0] = 'R';
        p_pkg[1] = 'T';
        p_pkg[2] = 'K';
        memcpy(p_pkg + 3, data, len);
        app_spp_cfu_send(p_pkg, len + 3);
    }
    else
    {
        p_pkg[0] = 'R';
        p_pkg[1] = 'T';
        p_pkg[2] = 'K';
        p_pkg[3] = (uint8_t)len;
        p_pkg[4] = (uint8_t)len << 8;
        memcpy(p_pkg + 5, data, len);

        app_spp_cfu_send(p_pkg, len + 5);
    }

    free(p_pkg);
    return true;
}

void cfu_pt_register_cb(P_CFU_PT_CB cb)
{
    cfu_passthrough_cb = cb;
}

void app_usb_hid_cfu_pt_init(void)
{
    app_timer_reg_cb(cfu_pt_timeout_cb, &cfu_pt_timer_id);
    cfu_pt_register_cb(cfu_pt_cb);
}

#endif

