/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>

#include "trace.h"
#include "btm.h"
#include "app_cfg.h"
#include "app_main.h"
//#include "hids_client.h"
#include "app_hid_convert.h"
#include "app_cfu.h"
//#include "app_ble_gap.h"




typedef struct
{
    uint8_t input_id;
    uint8_t output_id;
    uint8_t feature_id;
    uint8_t rsv;
} hids_id_t;




enum
{
    HIDS_USAGE_MOUSE        = 0x0,
    HIDS_USAGE_KEYBOARD     = 0x1,
    HIDS_USAGE_MAX
};

hids_report_t usb_hids_id[HIDS_USAGE_MAX] =
{
    {
        .report_id  = REPORT_ID_MOUSE,
        .size       = 9,
    },
};

//void report_convert

bool output_convert_from_usb_to_hogp(uint8_t usb_usage, T_HIDS_CLIENT_REPORT *p_client,
                                     hids_report_t *p_hogp)
{
    if ((usb_usage >= HIDS_USAGE_MAX) || (p_hogp == NULL))
    {
        return false;
    }

    if (usb_usage != p_client->usage)
    {
        return false;
    }

    hids_report_t *p_usb = (hids_report_t *) & (usb_hids_id[usb_usage]);
    memset(p_hogp, 0, sizeof(hids_report_t));

    p_hogp->report_id = p_usb->report_id;
    p_hogp->size = p_usb->size;
    memcpy(p_hogp->data, p_usb->data, p_hogp->size);

    return true;
}

bool input_convert_from_hogp_to_usb(hids_report_t *p_hogp, hids_report_t *p_usb)
{
    if ((p_usb == NULL) || (p_hogp == NULL))
    {
        return false;
    }

    //get report_id related usage in client
#if 0
    uint8_t usage = p_client->usage;
    if (usage >= HIDS_USAGE_MAX)
    {
        return false;
    }
#endif
    //
    memset(p_usb, 0, sizeof(hids_report_t));
    p_usb->size = p_hogp->size;
    memcpy(p_usb->data, p_hogp->data, p_usb->size);

    return true;
}

bool hogp_write_proc(uint8_t *data, uint8_t size)
{
    if ((data == NULL) || (size < 2))
    {
        return false;
    }

    uint8_t report_id = data[0];
    uint8_t hogp_report_id;

    T_APP_LE_LINK *p_link = NULL;

    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        p_link = (T_APP_LE_LINK *) & (app_db.le_link[i]);
        if (p_link)
        {
            if ((p_link->used != true)
                || (p_link->state != LE_LINK_STATE_CONNECTED))
            {
                continue;
            }
            if (report_id == 0x11)
            {
                APP_PRINT_TRACE0("get keyboard output data");
                hogp_report_id = 1;
            }
            hids_client_output_proc(p_link->conn_id, hogp_report_id, data + 1, size - 1);
        }
    }

    return true;
}
