/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      ancs_sample.c
   * @brief     This file handles ANCS Client routines.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#if F_APP_BT_ANCS_CLIENT_SUPPORT
#include <trace.h>
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "ancs_sample.h"
#include "ancs_gatt_client.h"

/** @defgroup  PERIPH_ANCS Peripheral ANCS
    * @brief Apple ANCS service modulization
    * @{
    */
/*============================================================================*
 *                         Types
 *============================================================================*/
typedef struct t_app_ancs_link
{
    struct t_app_ancs_link *p_next;
    uint16_t               conn_handle;
    uint8_t                m_parse_state;
    uint8_t                app_type;
    uint16_t               current_len;
    uint8_t               *ptr;
    T_DS_NOTIFICATION_ATTR notification_attr;
#if F_BT_ANCS_GET_APP_ATTR
    T_DS_APP_ATTR          app_attr;
#endif
} T_APP_ANCS_LINK;

#define HI_WORD(x)  ((uint8_t)((x & 0xFF00) >> 8))
#define LO_WORD(x)  ((uint8_t)(x))


/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup PERIPH_ANCS_Exported_Variables ANCS Exported Variables
   * @brief app register ANCS client to bluetooth host, and return ANCS client id
   * @{
   */
T_OS_QUEUE ancs_link_queue;

/** End of PERIPH_ANCS_Exported_Variables
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup PERIPH_ANCS_Exported_Functions ANCS Exported Functions
   * @{
   */
T_APP_ANCS_LINK *app_ancs_link_find_by_conn_handle(uint16_t conn_handle)
{
    for (uint8_t i = 0; i < ancs_link_queue.count; i++)
    {
        T_APP_ANCS_LINK *p_link = (T_APP_ANCS_LINK *)os_queue_peek(&ancs_link_queue, i);
        if (p_link->conn_handle == conn_handle)
        {
            return p_link;
        }
    }

    return NULL;
}

T_APP_ANCS_LINK *app_ancs_link_alloc(uint16_t conn_handle)
{
    T_APP_ANCS_LINK *p_link = NULL;

    p_link = app_ancs_link_find_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        return p_link;
    }

    p_link = (T_APP_ANCS_LINK *)calloc(1, sizeof(T_APP_ANCS_LINK));

    if (p_link == NULL)
    {
        APP_PRINT_ERROR1("app_ancs_link_alloc: alloc failed, conn_handle 0x%x", conn_handle);
        return NULL;
    }
    p_link->conn_handle = conn_handle;
    os_queue_in(&ancs_link_queue, (void *)p_link);
    return p_link;
}

bool app_ancs_link_free(uint16_t conn_handle)
{
    T_APP_ANCS_LINK *p_link = app_ancs_link_find_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        if (os_queue_delete(&ancs_link_queue, (void *)p_link))
        {
            free((void *) p_link);
            return true;
        }
    }
    APP_PRINT_ERROR1("app_ancs_link_free: failed, conn_handle 0x%x", conn_handle);
    return false;
}

void app_handle_notification_attribute_data(T_APP_ANCS_LINK *p_ancs_link)
{
    if (p_ancs_link->notification_attr.attribute_id != DS_NOTIFICATION_ATTR_ID_NEGATIVE_ACTION_LABEL)
    {
        p_ancs_link->m_parse_state = DS_PARSE_ATTRIBUTE_ID;
        uint8_t *p_value = p_ancs_link->notification_attr.data;

#if F_BT_ANCS_APP_FILTER
        //filter QQ , wechat , short message and incomming call
        if (p_ancs_link->notification_attr.attribute_id == DS_NOTIFICATION_ATTR_ID_APP_IDENTIFIER)
        {
#if F_BT_ANCS_CLIENT_DEBUG
            APP_PRINT_INFO1("parse notify attr: app identifier %s", TRACE_STRING(p_value));
#endif
            //wechat
            if (0 == memcmp(p_value, "com.tencent.xin", 16))
            {
                p_ancs_link->app_type = 1;
            }
            else if (0 == memcmp(p_value, "com.apple.MobileSMS", 20))
            {
                p_ancs_link->app_type = 2;
            }
            else if (0 == memcmp(p_value, "com.apple.mobilephone", 22))
            {
                p_ancs_link->app_type = 3;
            }
            else if (0 == memcmp(p_value, "com.tencent.mqq", 16))
            {
                p_ancs_link->app_type = 4;
            }
            else if (0 == memcmp(p_value, "com.tencent.qq", 15))
            {
                p_ancs_link->app_type = 5;
            }
        }
        else if (p_ancs_link->notification_attr.attribute_id == DS_NOTIFICATION_ATTR_ID_MESSAGE)
        {
            if (p_ancs_link->app_type == 2)
            {
                APP_PRINT_INFO1("MobileSMS: message %s", TRACE_STRING(p_value));
            }
            else if (p_ancs_link->app_type == 5 ||
                     p_ancs_link->app_type == 4)
            {
                APP_PRINT_INFO1("QQ: message %s", TRACE_STRING(p_value));
            }
        }
        else if (p_ancs_link->notification_attr.attribute_id == DS_NOTIFICATION_ATTR_ID_DATE)
        {
#if F_BT_ANCS_CLIENT_DEBUG
            APP_PRINT_INFO1("parse notify attr: date %s", TRACE_STRING(p_value));
#endif
        }
#endif
    }
    else/* All attributes has been parsed*/
    {
        if (p_ancs_link->notification_attr.attribute_id == DS_NOTIFICATION_ATTR_ID_NEGATIVE_ACTION_LABEL)
        {
            if (p_ancs_link->notification_attr.attribute_len)
            {
#if F_BT_ANCS_CLIENT_DEBUG
                APP_PRINT_INFO1("parse notify attr: negative action %s",
                                TRACE_STRING(p_ancs_link->notification_attr.data));
#endif
                /*Sample code for ancs_client_cp_perform_notification_action*/
                //uint32_t notification_uid;
                //LE_ARRAY_TO_UINT32(notification_uid, p_ancs_link->notification_attr.notification_uid);
                //ancs_client_cp_perform_notification_action(p_ancs_link->conn_handle, notification_uid, CP_ACTION_ID_NEGATIVE);
            }
        }

#if F_BT_ANCS_CLIENT_DEBUG
        APP_PRINT_INFO0("parse notify attr: parse done");
#endif
        p_ancs_link->m_parse_state = DS_PARSE_NOT_START;
        memset(&p_ancs_link->notification_attr, 0, sizeof(T_DS_NOTIFICATION_ATTR));
    }
}

void app_parse_notification_attribute(T_APP_ANCS_LINK *p_ancs_link, uint8_t *p_data, uint8_t len)
{
    int i;

    for (i = 0; i < len; i++)
    {
        switch (p_ancs_link->m_parse_state)
        {
        case DS_PARSE_GET_NOTIFICATION_COMMAND_ID:
            p_ancs_link->notification_attr.command_id = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_UID1;
            break;

        case DS_PARSE_UID1:
            p_ancs_link->notification_attr.notification_uid[0] = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_UID2;
            break;

        case DS_PARSE_UID2:
            p_ancs_link->notification_attr.notification_uid[1] = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_UID3;
            break;

        case DS_PARSE_UID3:
            p_ancs_link->notification_attr.notification_uid[2] = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_UID4;
            break;

        case DS_PARSE_UID4:
            p_ancs_link->notification_attr.notification_uid[3] = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_ATTRIBUTE_ID;
            break;

        case DS_PARSE_ATTRIBUTE_ID:
            p_ancs_link->notification_attr.attribute_id = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_ATTRIBUTE_LEN1;
            break;

        case DS_PARSE_ATTRIBUTE_LEN1:
            p_ancs_link->notification_attr.attribute_len = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_ATTRIBUTE_LEN2;
            break;

        case DS_PARSE_ATTRIBUTE_LEN2:
            p_ancs_link->notification_attr.attribute_len |= (p_data[i] << 8);
            p_ancs_link->m_parse_state = DS_PARSE_ATTRIBUTE_READY;
            p_ancs_link->ptr = p_ancs_link->notification_attr.data;
            p_ancs_link->current_len = 0;
#if F_BT_ANCS_CLIENT_DEBUG
            APP_PRINT_INFO2("parse notify attr: attribute_id %d, attribute_len %d",
                            p_ancs_link->notification_attr.attribute_id,
                            p_ancs_link->notification_attr.attribute_len
                           );
#endif
            if (p_ancs_link->notification_attr.attribute_len == 0)
            {
                p_ancs_link->m_parse_state = DS_PARSE_ATTRIBUTE_ID;
            }
            if (p_ancs_link->notification_attr.attribute_len > ANCS_MAX_ATTR_LEN)
            {
#if F_BT_ANCS_CLIENT_DEBUG
                APP_PRINT_ERROR2("parse notify attr: error, attribute_len %d > max length %d",
                                 p_ancs_link->notification_attr.attribute_len,
                                 ANCS_MAX_ATTR_LEN
                                );
#endif
                p_ancs_link->m_parse_state = DS_PARSE_NOT_START;
                memset(&p_ancs_link->notification_attr, 0, sizeof(T_DS_NOTIFICATION_ATTR));
            }
            break;

        case DS_PARSE_ATTRIBUTE_READY:
            *p_ancs_link->ptr++ = p_data[i];
            p_ancs_link->current_len++;

            if (p_ancs_link->current_len == p_ancs_link->notification_attr.attribute_len)
            {
                /*An attribute is always a string whose length in bytes is provided in the tuple but that is not NULL-terminated.*/
                *p_ancs_link->ptr++ = 0;
#if F_BT_ANCS_CLIENT_DEBUG
                APP_PRINT_INFO1("parse notify attr: data %b",
                                TRACE_BINARY(p_ancs_link->notification_attr.attribute_len,
                                             p_ancs_link->notification_attr.data));
#endif
                app_handle_notification_attribute_data(p_ancs_link);
            }
            break;
        }
    }
}

#if F_BT_ANCS_GET_APP_ATTR
void app_parse_app_attribute(T_APP_ANCS_LINK *p_ancs_link, uint8_t *p_data, uint8_t len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        switch (p_ancs_link->m_parse_state)
        {
        case DS_PARSE_GET_APP_COMMAND_ID:
            p_ancs_link->app_attr.command_id = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_APP_IDENTIFIER_START;
            break;

        case DS_PARSE_APP_IDENTIFIER_START:
            if (p_data[i] == 0x00)
            {
                p_ancs_link->m_parse_state = DS_PARSE_APP_IDENTIFIER_END;

                if (i + 1 == len)
                {
                    p_ancs_link->m_parse_state = DS_PARSE_NOT_START;
                }
                else
                {
                    p_ancs_link->m_parse_state = DS_PARSE_APP_ATTRIBUTE_ID;
                }

            }
            break;

        case DS_PARSE_APP_ATTRIBUTE_ID:
            p_ancs_link->app_attr.attribute_id = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_APP_ATTRIBUTE_LEN1;
            break;

        case DS_PARSE_APP_ATTRIBUTE_LEN1:
            p_ancs_link->app_attr.attribute_len = p_data[i];
            p_ancs_link->m_parse_state = DS_PARSE_APP_ATTRIBUTE_LEN2;
            break;

        case DS_PARSE_APP_ATTRIBUTE_LEN2:
            p_ancs_link->app_attr.attribute_len |= (p_data[i] << 8);
            p_ancs_link->m_parse_state = DS_PARSE_APP_ATTRIBUTE_READY;
            p_ancs_link->ptr = p_ancs_link->app_attr.data;
            p_ancs_link->current_len = 0;
#if F_BT_ANCS_CLIENT_DEBUG
            APP_PRINT_INFO2("parse app attr: attribute_id %d, attribute_len %d",
                            p_ancs_link->app_attr.attribute_id,
                            p_ancs_link->app_attr.attribute_len
                           );
#endif
            if (p_ancs_link->app_attr.attribute_len == 0)
            {
                p_ancs_link->m_parse_state = DS_PARSE_APP_ATTRIBUTE_ID;

            }
            if (p_ancs_link->app_attr.attribute_len > ANCS_MAX_ATTR_LEN)
            {
#if F_BT_ANCS_CLIENT_DEBUG
                APP_PRINT_ERROR2("parse app attr: error, attribute_len %d > max length %d",
                                 p_ancs_link->app_attr.attribute_len,
                                 ANCS_MAX_ATTR_LEN
                                );
#endif
                p_ancs_link->m_parse_state = DS_PARSE_NOT_START;
            }
            break;

        case DS_PARSE_APP_ATTRIBUTE_READY:
            *p_ancs_link->ptr++ = p_data[i];
            p_ancs_link->current_len++;

            if (p_ancs_link->current_len == p_ancs_link->app_attr.attribute_len)
            {
                *p_ancs_link->ptr++ = 0;
#if F_BT_ANCS_CLIENT_DEBUG
                APP_PRINT_INFO4("parse app attr: command_id 0x%x, attribute_id 0x%x, attribute_len %d, data %s",
                                p_ancs_link->app_attr.command_id,
                                p_ancs_link->app_attr.attribute_id,
                                p_ancs_link->app_attr.attribute_len,
                                TRACE_STRING(p_ancs_link->app_attr.data));
#endif
                p_ancs_link->m_parse_state = DS_PARSE_NOT_START;

            }
            break;
        }
    }
}
#endif

/**
 * @brief  Parse ANCS data source notification
 * @param[in]  conn_handle Connection handle
 * @param[in]  p_data      Point to data buffer
 * @param[in]  len         Data length
 * @return void
 */
void app_parse_data_source_data(uint16_t conn_handle, uint8_t *p_data, uint8_t len)
{
    APP_PRINT_INFO2("ANCS_DATA_SOURCE: conn_handle 0x%x, len %d", conn_handle, len);
#if F_BT_ANCS_CLIENT_DEBUG
    APP_PRINT_INFO1("data = %b", TRACE_BINARY(len, p_data));
#endif
    T_APP_ANCS_LINK *p_ancs_link = app_ancs_link_alloc(conn_handle);

    if (p_ancs_link == NULL)
    {
        return;
    }

#if F_BT_ANCS_CLIENT_DEBUG
    APP_PRINT_INFO1("m_parse_state %d", p_ancs_link->m_parse_state);
#endif
    if (p_ancs_link->m_parse_state == DS_PARSE_NOT_START)
    {
        if (len >= 1 && p_data[0] == CP_CMD_ID_GET_NOTIFICATION_ATTR)
        {
            p_ancs_link->m_parse_state = DS_PARSE_GET_NOTIFICATION_COMMAND_ID;
        }
#if F_BT_ANCS_GET_APP_ATTR
        else if (len >= 1 && p_data[0] == CP_CMD_ID_GET_APP_ATTR)
        {
            p_ancs_link->m_parse_state = DS_PARSE_GET_APP_COMMAND_ID;
        }
#endif
    }

    if (p_ancs_link->m_parse_state < DS_PARSE_GET_APP_COMMAND_ID)
    {
        app_parse_notification_attribute(p_ancs_link, p_data, len);
    }
#if F_BT_ANCS_GET_APP_ATTR
    else
    {
        app_parse_app_attribute(p_ancs_link, p_data, len);
    }
#endif
}

void app_parse_notification_source_data(uint16_t conn_handle,
                                        T_ANCS_CLIENT_NOTIFICATION_SOURCE *p_notify_src)
{
#if F_BT_ANCS_CLIENT_DEBUG
    APP_PRINT_INFO5("app_parse_notification_source_data: event_id %d, event_flags 0x%02x, category_id %d, category_count %d, notification_uid 0x%08x",
                    p_notify_src->event_id,
                    p_notify_src->event_flags,
                    p_notify_src->category_id,
                    p_notify_src->category_count,
                    p_notify_src->notification_uid
                   );
    APP_PRINT_INFO5("event_flags: silent %d, important %d, pre existing %d, positive action %d, negative action %d ",
                    p_notify_src->event_flags & NS_EVENT_FLAG_SILENT,
                    p_notify_src->event_flags & NS_EVENT_FLAG_IMPORTANT,
                    p_notify_src->event_flags & NS_EVENT_FLAG_PRE_EXISTING,
                    p_notify_src->event_flags & NS_EVENT_FLAG_POSITIVE_ACTION,
                    p_notify_src->event_flags & NS_EVENT_FLAG_NEGATIVE_ACTION
                   );
#endif
    //User can filter by category_id here, for demo purpose, we didn't filter any CategoryID here.
#if F_BT_ANCS_APP_FILTER
    //Filter social and other category & phone category & email category
    if (p_notify_src->category_id == NS_CATEGORY_ID_SOCIAL ||
        p_notify_src->category_id == NS_CATEGORY_ID_OTHER ||
        p_notify_src->category_id == NS_CATEGORY_ID_INCOMING_CALL ||
        p_notify_src->category_id == NS_CATEGORY_ID_EMAIL)
    {
#endif
        if (p_notify_src->event_id != NS_EVENT_ID_NOTIFICATION_REMOVED)
        {
            uint8_t attr_id_list[14];
            uint8_t cur_index = 0;

            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_APP_IDENTIFIER;
            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_TITLE;
            attr_id_list[cur_index++] = LO_WORD(ANCS_MAX_ATTR_LEN);
            attr_id_list[cur_index++] = HI_WORD(ANCS_MAX_ATTR_LEN);

            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_SUB_TITLE;
            attr_id_list[cur_index++] = LO_WORD(ANCS_MAX_ATTR_LEN);
            attr_id_list[cur_index++] = HI_WORD(ANCS_MAX_ATTR_LEN);

            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_MESSAGE;
            attr_id_list[cur_index++] = LO_WORD(ANCS_MAX_ATTR_LEN);
            attr_id_list[cur_index++] = HI_WORD(ANCS_MAX_ATTR_LEN);

            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_MESSAGE_SIZE;
            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_DATE;
            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_POSITIVE_ACTION_LABEL;
            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_NEGATIVE_ACTION_LABEL;

            if (ancs_client_cp_get_notification_attr(conn_handle, p_notify_src->notification_uid, attr_id_list,
                                                     cur_index) == true)
            {
                return;
            }
        }
#if F_BT_ANCS_APP_FILTER
    }
#endif
}

T_APP_RESULT ancs_client_cb(uint16_t conn_handle, uint8_t type, void *p_data)
{
    T_APP_RESULT  app_result = APP_RESULT_SUCCESS;

    switch (type)
    {
    case GATT_MSG_ANCS_CLIENT_DIS_DONE:
        {
            T_ANCS_CLIENT_DIS_DONE *p_dis_done = (T_ANCS_CLIENT_DIS_DONE *)p_data;
            if (p_dis_done->is_found)
            {
                APP_PRINT_INFO0("ANCS BLE Client CB: discover procedure done.");
                ancs_client_cfg_cccd(conn_handle, ANCS_DATA_SOURCE, true);
                ancs_client_cfg_cccd(conn_handle, ANCS_NOTIFICATION_SOURCE, true);
            }
            else
            {
                APP_PRINT_ERROR0("ANCS BLE Client CB: discover request failed.");
            }
        }
        break;

    case GATT_MSG_ANCS_CLIENT_WRITE_CP_RESULT:
        {
#if F_BT_ANCS_CLIENT_DEBUG
            T_ANCS_CLIENT_WRITE_CP_RESULT *p_write_result = (T_ANCS_CLIENT_WRITE_CP_RESULT *)p_data;
            APP_PRINT_INFO0("GATT_MSG_ANCS_CLIENT_WRITE_CP_RESULT");

            if (p_write_result->cause == 0x4A0)
            {
                APP_PRINT_INFO0("The commandID was not recognized by the NP.");
            }
            else if (p_write_result->cause == 0x4A1)
            {
                APP_PRINT_INFO0("The command was improperly formatted.");
            }
            else if (p_write_result->cause == 0x4A2)
            {
                APP_PRINT_INFO0("One of the parameters (for example, the NotificationUID) does not refer to an existing object on the NP.");
            }
            else if (p_write_result->cause == 0x4A3)
            {
                APP_PRINT_INFO0("The action was not performed.");
            }
#endif
        }
        break;

    case GATT_MSG_ANCS_CLIENT_DATA_SOURCE:
        {
            T_ANCS_CLIENT_DATA_SOURCE *p_notify = (T_ANCS_CLIENT_DATA_SOURCE *)p_data;
            app_parse_data_source_data(conn_handle, p_notify->p_value,
                                       p_notify->value_size);
        }
        break;

    case GATT_MSG_ANCS_CLIENT_NOTIFICATION_SOURCE:
        {
            T_ANCS_CLIENT_NOTIFICATION_SOURCE *p_notify = (T_ANCS_CLIENT_NOTIFICATION_SOURCE *)p_data;
            app_parse_notification_source_data(conn_handle, p_notify);
        }
        break;

    case GATT_MSG_ANCS_CLIENT_CCCD_CFG_RESULT:
        {
            T_ANCS_CLIENT_CCCD_CFG_RESULT *p_cfg_result = (T_ANCS_CLIENT_CCCD_CFG_RESULT *)p_data;
            APP_PRINT_INFO3("GATT_MSG_ANCS_CLIENT_CCCD_CFG_RESULT: conn_handle 0x%x, type %d, cause 0x%x",
                            conn_handle, p_cfg_result->type, p_cfg_result->cause);
            if (p_cfg_result->cause == GAP_SUCCESS)
            {
                if (p_cfg_result->type == ANCS_DATA_SOURCE)
                {
                    if (p_cfg_result->enable)
                    {
                        APP_PRINT_INFO0("ANCS_DATA_SOURCE: CCCD enabled");
                    }
                    else
                    {
                        APP_PRINT_INFO0("ANCS_DATA_SOURCE: CCCD disabled");
                    }
                }
                else if (p_cfg_result->type == ANCS_NOTIFICATION_SOURCE)
                {
                    if (p_cfg_result->enable)
                    {
                        APP_PRINT_INFO0("ANCS_NOTIFICATION_SOURCE: CCCD enabled");
                    }
                    else
                    {
                        APP_PRINT_INFO0("ANCS_NOTIFICATION_SOURCE: CCCD disabled");
                    }
                }
            }
        }
        break;

    case GATT_MSG_ANCS_CLIENT_CONN_DEL:
        {
            T_ANCS_CLIENT_CONN_DEL *p_conn_del = (T_ANCS_CLIENT_CONN_DEL *)p_data;
            APP_PRINT_INFO2("GATT_MSG_ANCS_CLIENT_CONN_DEL: conn_handle 0x%x, transport_le %d",
                            conn_handle, p_conn_del->transport_le);
            app_ancs_link_free(conn_handle);
        }
        break;

    default:
        break;
    }
    return app_result;
}

/**
 * @brief  App register ANCS client to bluetooth host.
 *
 * @return void
 */
void app_ancs_client_init(void)
{
    os_queue_init(&ancs_link_queue);
    ancs_client_init(ancs_client_cb);
}
/** @} */ /* End of group PERIPH_ANCS_Exported_Functions */
/** @} */ /* End of group PERIPH_ANCS */
#endif
