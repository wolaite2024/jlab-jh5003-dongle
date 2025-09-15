/**
*****************************************************************************************
*     Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     hids_gatt_client.c
  * @brief
  * @details
  * @author   cen_zhang
  * @date     2023-9-20
  * @version  v1.0
  * *************************************************************************************
  */

#if BLE_HID_CLIENT_SUPPORT
/** Add Includes here **/
#include <string.h>
#include <stdlib.h>
#include "hids_gatt_client.h"
#include "bt_types.h"
#include "trace.h"

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

/** @brief HIDS client report reference*/
typedef struct hids_report
{
    uint8_t report_id;
    T_HIDS_REPORT_TYPE report_type;
} T_HIDS_REPORT;

/**
 * @brief hids service def
 *
 */
typedef struct hids_service
{
    uint8_t srv_instance_id;
    uint8_t hids_report_num;
    T_HIDS_REPORT  *p_report_info;
} T_HIDS_SERVICE;

/**
 * @brief  HIDS client Link control block defination.
 */
typedef struct
{
    bool used;
    uint16_t conn_handle;
    uint8_t hids_service_num;
    T_HIDS_SERVICE *hids_service_info;
} T_HIDS_LINK, *P_HIDS_LINK;

static P_HIDS_LINK hids_table;
static uint8_t hids_client_link_num;

/**<  Callback used to send data to app from HIDS client layer. */
static P_FUN_HIDS_CLIENT_APP_CB hids_client_cb = NULL;

T_HIDS_LINK *hids_client_find_link_by_conn_handle(uint16_t conn_handle)
{
    for (uint8_t i = 0; i < hids_client_link_num; i++)
    {
        if (hids_table[i].used && (hids_table[i].conn_handle == conn_handle))
        {
            return &hids_table[i];
        }
    }
    return NULL;
}

T_HIDS_LINK *hids_client_alloc_link(uint16_t conn_handle)
{
    T_HIDS_LINK *p_hids_link = NULL;
    p_hids_link = hids_client_find_link_by_conn_handle(conn_handle);
    if (p_hids_link)
    {
        return p_hids_link;
    }

    for (uint8_t i = 0; i < hids_client_link_num; i++)
    {
        if (hids_table[i].used == false)
        {
            hids_table[i].used = true;
            hids_table[i].conn_handle = conn_handle;
            return &hids_table[i];
        }
    }

    return NULL;
}

void hids_client_report_init(T_HIDS_SERVICE *p_service_info, uint8_t report_num)
{
    PROTOCOL_PRINT_TRACE1("hids_client_report_init: report_num %d", report_num);

    if (!report_num)
    {
        return;
    }

    p_service_info->p_report_info = (T_HIDS_REPORT *)calloc(report_num, sizeof(T_HIDS_REPORT));
    if (!p_service_info->p_report_info)
    {
        return;
    }
}

void hids_client_service_init(T_HIDS_LINK *p_hids_link, uint8_t service_num)
{
    T_HIDS_SERVICE *p_service_info = NULL;
    PROTOCOL_PRINT_TRACE1("hids_client_service_init: serv num %d", service_num);

    if (!service_num)
    {
        return;
    }

    p_hids_link->hids_service_info = (T_HIDS_SERVICE *)calloc(1, service_num * sizeof(T_HIDS_SERVICE));

    if (!p_hids_link->hids_service_info)
    {
        return;
    }
    p_service_info = p_hids_link->hids_service_info;

    for (uint8_t i = 0; i < service_num; i++)
    {
        p_service_info[i].srv_instance_id = i;
    }
}

T_HIDS_SERVICE  *hids_client_find_service(uint16_t conn_handle, uint8_t srv_instance_id)
{
    T_HIDS_LINK *p_hids_link = NULL;
    T_HIDS_SERVICE *p_hids_service = NULL;
    for (uint8_t i = 0; i < hids_client_link_num; i++)
    {
        if (hids_table[i].used && (hids_table[i].conn_handle == conn_handle))
        {
            p_hids_link = &hids_table[i];
            break;
        }
    }

    if (p_hids_link)
    {
        p_hids_service = p_hids_link->hids_service_info;
        if (p_hids_service)
        {
            return &p_hids_service[srv_instance_id];
        }
    }
    return NULL;
}

T_HIDS_REPORT  *hids_client_find_report(uint16_t conn_handle, uint8_t srv_instance_id,
                                        uint8_t report_instance_id)
{
    T_HIDS_REPORT  *p_hids_report = NULL;
    T_HIDS_SERVICE  *p_hids_service = NULL;

    PROTOCOL_PRINT_TRACE2("hids_client_find_report: srv_id %d, report id %d", srv_instance_id,
                          report_instance_id);

    p_hids_service = hids_client_find_service(conn_handle, srv_instance_id);
    if (p_hids_service)
    {
        p_hids_report = p_hids_service->p_report_info;
        if (report_instance_id >= p_hids_service->hids_report_num || !p_hids_report)
        {
            return NULL;
        }

        return &p_hids_report[report_instance_id];
    }
    else
    {
        return NULL;
    }
}

bool hids_client_read_report(uint8_t conn_handle, uint8_t srv_instance_id, uint8_t report_id,
                             T_HIDS_REPORT_TYPE report_type)
{
    T_HIDS_SERVICE *p_hids_service = NULL;
    T_HIDS_REPORT  *p_hids_report = NULL;
    T_ATTR_UUID srv_uuid = {0};
    T_ATTR_UUID char_uuid = {0};
    T_GAP_CAUSE ret = GAP_CAUSE_ERROR_UNKNOWN;

    PROTOCOL_PRINT_INFO3("[HIDS] hids_client_read_report: service id %d, report id %d, report type %d",
                         srv_instance_id, report_id, report_type);

    p_hids_service = hids_client_find_service(conn_handle, srv_instance_id);
    if (p_hids_service)
    {
        for (uint8_t i = 0; i < p_hids_service->hids_report_num; i++)
        {
            p_hids_report = hids_client_find_report(conn_handle, srv_instance_id, i);
            if (p_hids_report && (p_hids_report->report_id == report_id) &&
                (p_hids_report->report_type == report_type))
            {
                srv_uuid.is_uuid16 = true;
                srv_uuid.instance_id = srv_instance_id;
                srv_uuid.p.uuid16 = GATT_UUID_HIDS;
                char_uuid.is_uuid16 = true;
                char_uuid.instance_id = i;
                char_uuid.p.uuid16 = GATT_UUID_CHAR_REPORT;
                uint16_t handle = 0;

                if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
                {
                    ret = gatt_client_read(conn_handle, handle, NULL);
                }
                else
                {
                    return false;
                }
            }
        }
    }

    if (ret != GAP_CAUSE_SUCCESS)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool hids_client_write_report(uint16_t conn_handle, uint8_t srv_instance_id, uint8_t report_id,
                              T_HIDS_REPORT_TYPE report_type, uint16_t length,
                              uint8_t *p_data)
{
    T_HIDS_SERVICE *p_hids_service = NULL;
    T_HIDS_REPORT  *p_hids_report = NULL;
    uint16_t handle = 0;
    T_ATTR_UUID srv_uuid = {0};
    T_ATTR_UUID char_uuid = {0};
    T_GAP_CAUSE ret = GAP_CAUSE_ERROR_UNKNOWN;

    PROTOCOL_PRINT_INFO4("[HIDS] hids_client_write_report: service id %d, report id %d, report type %d, length %d",
                         srv_instance_id, report_id, report_type, length);

    p_hids_service = hids_client_find_service(conn_handle, srv_instance_id);
    if (p_hids_service)
    {
        for (uint8_t i = 0; i < p_hids_service->hids_report_num; i++)
        {
            p_hids_report = hids_client_find_report(conn_handle, srv_instance_id, i);
            if (p_hids_report && (p_hids_report->report_id == report_id) &&
                (p_hids_report->report_type == report_type))
            {
                srv_uuid.is_uuid16 = true;
                srv_uuid.instance_id = srv_instance_id;
                srv_uuid.p.uuid16 = GATT_UUID_HIDS;
                char_uuid.is_uuid16 = true;
                char_uuid.instance_id = i;
                char_uuid.p.uuid16 = GATT_UUID_CHAR_REPORT;
                if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
                {
                    ret = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle, length, p_data, NULL);
                    break;
                }
                else
                {
                    return false;
                }
            }
        }
    }

    if (ret != GAP_CAUSE_SUCCESS)
    {
        return false;
    }
    else
    {
        return true;
    }
}

T_APP_RESULT hids_client_read_report_reference(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                                               void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;
    T_HIDS_SERVICE *p_hids_service = NULL;
    T_HIDS_REPORT  *p_hids_report = NULL;
    T_HIDS_CLIENT_READ_RESULT read_result = {0};
    read_result.srv_instance_id = p_client_cb_data->read_result.srv_instance_id;
    read_result.cause = p_client_cb_data->read_result.cause;

    PROTOCOL_PRINT_INFO5("[HIDS] hids_client_read_report_reference: conn_handle %d, srv_id %d, char_id %d, value_size %d, data %b",
                         conn_handle,
                         p_client_cb_data->read_result.srv_instance_id,
                         p_client_cb_data->read_result.char_uuid.instance_id,
                         p_client_cb_data->read_result.value_size,
                         TRACE_BINARY(p_client_cb_data->read_result.value_size, p_client_cb_data->read_result.p_value));

    if (p_client_cb_data->read_result.cause == GAP_CAUSE_SUCCESS)
    {
        p_hids_service = hids_client_find_service(conn_handle,
                                                  p_client_cb_data->read_result.srv_instance_id);
        if (p_hids_service)
        {
            p_hids_report = hids_client_find_report(conn_handle, p_client_cb_data->read_result.srv_instance_id,
                                                    p_client_cb_data->read_result.char_uuid.instance_id);
            if (p_hids_report && (p_client_cb_data->read_result.value_size == 2))
            {
                p_hids_report->report_id = *p_client_cb_data->read_result.p_value;
                p_hids_report->report_type = (T_HIDS_REPORT_TYPE) * (p_client_cb_data->read_result.p_value +
                                                                     1);
            }

            if (p_client_cb_data->read_result.char_uuid.instance_id == p_hids_service->hids_report_num - 1)
            {
                read_result.char_uuid = GATT_UUID_CHAR_REPORT_REFERENCE;
            }
        }
    }

    if (hids_client_cb)
    {
        (*hids_client_cb)(conn_handle, GATT_MSG_HIDS_CLIENT_READ_RESULT, &read_result);
    }

    return result;
}

uint8_t hids_client_get_service_num(uint16_t conn_handle)
{
    T_HIDS_LINK *p_hids_link = NULL;
    p_hids_link = hids_client_find_link_by_conn_handle(conn_handle);

    if (p_hids_link == NULL)
    {
        return false;
    }

    return p_hids_link->hids_service_num;
}

bool hids_client_read_protocol_mode(uint16_t conn_handle, uint8_t srv_instance_id)
{
    T_HIDS_LINK *p_hids_link = NULL;
    p_hids_link = hids_client_find_link_by_conn_handle(conn_handle);
    T_ATTR_UUID srv_uuid;
    T_ATTR_UUID char_uuid;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint16_t handle = 0;
    srv_uuid.is_uuid16 = true;
    srv_uuid.instance_id = srv_instance_id;
    srv_uuid.p.uuid16 = GATT_UUID_HIDS;
    char_uuid.is_uuid16 = true;
    char_uuid.instance_id = 0;
    char_uuid.p.uuid16 = GATT_UUID_CHAR_PROTOCOL_MODE;

    if (p_hids_link == NULL)
    {
        return false;
    }

    if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
    {
        cause = gatt_client_read(conn_handle, handle, NULL);
    }
    else
    {
        return false;
    }

    return (cause == GAP_CAUSE_SUCCESS) ? true : false;
}


bool hids_client_read_report_map_value(uint16_t conn_handle, uint8_t srv_instance_id)
{
    T_HIDS_LINK *p_hids_link = NULL;
    p_hids_link = hids_client_find_link_by_conn_handle(conn_handle);
    T_ATTR_UUID srv_uuid;
    T_ATTR_UUID char_uuid;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint16_t handle = 0;
    srv_uuid.is_uuid16 = true;
    srv_uuid.instance_id = srv_instance_id;
    srv_uuid.p.uuid16 = GATT_UUID_HIDS;
    char_uuid.is_uuid16 = true;
    char_uuid.instance_id = 0;
    char_uuid.p.uuid16 = GATT_UUID_CHAR_REPORT_MAP;

    if (p_hids_link == NULL)
    {
        return false;
    }

    if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
    {
        cause = gatt_client_read(conn_handle, handle, NULL);
    }
    else
    {
        return false;
    }

    return (cause == GAP_CAUSE_SUCCESS) ? true : false;
}

/**
 * @brief HIDS BLE Client Callbacks.
*/
T_APP_RESULT hids_client_cbs(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;
    T_HIDS_LINK *p_hids_link = NULL;
    T_ATTR_UUID srv_uuid = {0};
    T_ATTR_UUID char_uuid = {0};
    T_HIDS_SERVICE *p_hids_service = NULL;
    T_HIDS_REPORT  *p_hids_report = NULL;
    uint16_t start_handle;
    uint16_t end_handle;
    uint16_t cccd_handle;
    bool cb_flag = false;

    PROTOCOL_PRINT_INFO1("[HIDS] hids_client_cbs: type %d", type);

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            PROTOCOL_PRINT_INFO2("[HIDS] GATT_CLIENT_EVENT_DIS_DONE: is_found %d, conn_handle %d",
                                 p_client_cb_data->dis_done.is_found, conn_handle);

            T_HIDS_CLIENT_DIS_DONE dis_done = {0};

            dis_done.is_found = p_client_cb_data->dis_done.is_found;
            dis_done.load_from_ftl = p_client_cb_data->dis_done.load_from_ftl;
            dis_done.srv_instance_num = p_client_cb_data->dis_done.srv_instance_num;

            if (p_client_cb_data->dis_done.is_found)
            {
                p_hids_link = hids_client_alloc_link(conn_handle);
                if (p_hids_link)
                {
                    p_hids_link->hids_service_num = p_client_cb_data->dis_done.srv_instance_num;
                    hids_client_service_init(p_hids_link, p_hids_link->hids_service_num);
                }
                else
                {
                    PROTOCOL_PRINT_ERROR0("[HIDS] GATT_CLIENT_EVENT_DIS_DONE: Alloc hids mgr struct failed");
                    break;
                }

                srv_uuid.is_uuid16 = true;
                srv_uuid.p.uuid16 = GATT_UUID_HIDS;
                char_uuid.is_uuid16 = true;
                char_uuid.p.uuid16 = GATT_UUID_CHAR_REPORT;
                for (uint8_t i = 0; i < p_hids_link->hids_service_num; i++)
                {
                    srv_uuid.instance_id = i;
                    p_hids_service = hids_client_find_service(conn_handle, i);
                    if (p_hids_service)
                    {
                        p_hids_service->hids_report_num = gatt_client_get_char_num(conn_handle, &srv_uuid, &char_uuid);
                        hids_client_report_init(p_hids_service, p_hids_service->hids_report_num);

                        for (uint8_t j = 0; j < p_hids_service->hids_report_num; j++)
                        {
                            char_uuid.instance_id = j;
                            if (gatt_client_find_char_descriptor_range(conn_handle, &srv_uuid, &char_uuid, &start_handle,
                                                                       &end_handle, &cccd_handle))
                            {
                                for (uint16_t read_handle = start_handle; read_handle <= end_handle; read_handle++)
                                {
                                    if (read_handle != cccd_handle)
                                    {
                                        gatt_client_read(conn_handle, read_handle, hids_client_read_report_reference);
                                    }
                                }
                            }
                        }

                        gatt_client_enable_srv_cccd(conn_handle, &srv_uuid, GATT_CLIENT_CONFIG_ALL);
                    }
                }
            }

            if (hids_client_cb)
            {
                (*hids_client_cb)(conn_handle, GATT_MSG_HIDS_CLIENT_DIS_DONE, &dis_done);
            }
        }
        break;

    case GATT_CLIENT_EVENT_READ_RESULT:
        {
            PROTOCOL_PRINT_INFO2("[HIDS] GATT_CLIENT_EVENT_READ_RESULT: value_size %d, uuid 0x%x",
                                 p_client_cb_data->read_result.value_size,
                                 p_client_cb_data->read_result.char_uuid.p.uuid16);
            T_HIDS_CLIENT_READ_RESULT read_result = {0};
            read_result.srv_instance_id = p_client_cb_data->read_result.srv_instance_id;
            read_result.char_uuid = p_client_cb_data->read_result.char_uuid.p.uuid16;
            read_result.cause = p_client_cb_data->read_result.cause;

            if (read_result.cause == ATT_SUCCESS)
            {
                switch (read_result.char_uuid)
                {
                case GATT_UUID_CHAR_REPORT_MAP:
                    {
                        read_result.data.hids_report_map.p_hids_report_map =
                            p_client_cb_data->read_result.p_value;
                        read_result.data.hids_report_map.hids_report_map_len =
                            p_client_cb_data->read_result.value_size;
                    }
                    break;

                case GATT_UUID_CHAR_REPORT:
                    {
                        p_hids_report = hids_client_find_report(conn_handle, p_client_cb_data->read_result.srv_instance_id,
                                                                p_client_cb_data->read_result.char_uuid.instance_id);
                        if (p_hids_report)
                        {
                            read_result.data.hids_report.report_id = p_hids_report->report_id;
                            read_result.data.hids_report.hids_report_value_len = p_client_cb_data->read_result.value_size;
                            read_result.data.hids_report.p_hids_report_value = p_client_cb_data->read_result.p_value;
                        }
                    }
                    break;

                case GATT_UUID_CHAR_PROTOCOL_MODE:
                    {
                        read_result.data.protocol_mode = (T_HIDS_PROTOCOL_MODE)p_client_cb_data->read_result.p_value[0];
                    }
                    break;

                default:
                    break;
                }
            }
            if (hids_client_cb)
            {
                (*hids_client_cb)(conn_handle, GATT_MSG_HIDS_CLIENT_READ_RESULT, &read_result);
            }
        }
        break;

    case GATT_CLIENT_EVENT_WRITE_RESULT:
        {
            PROTOCOL_PRINT_INFO3("[HIDS] GATT_CLIENT_EVENT_WRITE_RESULT: conn_handle 0x%x, cause 0x%x, uuid 0x%x",
                                 conn_handle, p_client_cb_data->write_result.cause,
                                 p_client_cb_data->write_result.char_uuid.p.uuid16);
            T_HIDS_CLIENT_WRITE_RESULT write_result = {0};

            write_result.srv_instance_id = p_client_cb_data->write_result.srv_instance_id;
            write_result.char_uuid = p_client_cb_data->write_result.char_uuid.p.uuid16;
            write_result.cause = p_client_cb_data->write_result.cause;

            if (hids_client_cb)
            {
                (*hids_client_cb)(conn_handle, GATT_MSG_HIDS_CLIENT_WRITE_RESULT, &write_result);
            }
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:
        {
            PROTOCOL_PRINT_INFO3("[HIDS] GATT_CLIENT_EVENT_NOTIFY_IND: conn_handle 0x%x, uuid 0x%x, data length %d",
                                 conn_handle, p_client_cb_data->notify_ind.char_uuid.p.uuid16,
                                 p_client_cb_data->notify_ind.value_size);

            T_HIDS_CLIENT_NOTIFY_DATA notify_data = {0};
            if (p_client_cb_data->notify_ind.char_uuid.p.uuid16 == GATT_UUID_CHAR_REPORT)
            {
                p_hids_report = hids_client_find_report(conn_handle, p_client_cb_data->notify_ind.srv_instance_id,
                                                        p_client_cb_data->notify_ind.char_uuid.instance_id);
                if (p_hids_report && p_hids_report->report_type == HID_INPUT_TYPE)
                {
                    cb_flag = true;
                    notify_data.srv_instance_id = p_client_cb_data->notify_ind.srv_instance_id;
                    notify_data.report_id = p_hids_report->report_id;
                    notify_data.data_len = p_client_cb_data->notify_ind.value_size;
                    notify_data.p_data = p_client_cb_data->notify_ind.p_value;
                }
            }
            if (hids_client_cb && cb_flag)
            {
                (*hids_client_cb)(conn_handle, GATT_MSG_HIDS_CLIENT_NOTIFY_IND, &notify_data);
            }
        }
        break;

    case GATT_CLIENT_EVENT_CONN_DEL:
        {
            p_hids_link = hids_client_find_link_by_conn_handle(conn_handle);
            if (p_hids_link)
            {
                for (uint8_t i = 0; i < p_hids_link->hids_service_num; i++)
                {
                    p_hids_service = hids_client_find_service(conn_handle, i);
                    if (p_hids_service)
                    {
                        if (p_hids_service->p_report_info)
                        {
                            free(p_hids_service->p_report_info);
                        }
                        free(p_hids_service);
                    }
                }
                memset(p_hids_link, 0, sizeof(T_HIDS_LINK));
            }
        }
        break;

    default:
        break;
    }

    return result;
}

bool hids_add_client(P_FUN_HIDS_CLIENT_APP_CB app_cb, uint8_t link_num)
{
    T_ATTR_UUID srv_uuid = {0};
    srv_uuid.is_uuid16 = true;
    srv_uuid.p.uuid16 = GATT_UUID_HIDS;

    if (gatt_client_spec_register(&srv_uuid, hids_client_cbs) == GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        hids_client_cb = app_cb;
        hids_client_link_num = link_num;
        hids_table = calloc(1, link_num * sizeof(T_HIDS_LINK));
        return true;
    }

    return false;
}
#endif
