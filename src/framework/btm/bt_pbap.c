/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_PBAP_SUPPORT == 1)

#include <string.h>
#include "trace.h"
#include "pbap.h"
#include "bt_pbap_int.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_pbap.h"

void bt_pbap_cback(uint8_t     bd_addr[6],
                   T_PBAP_MSG  msg_type,
                   void       *msg_buf)
{
    T_BT_BR_LINK *p_link;

    BTM_PRINT_INFO1("bt_pbap_cback: msg_type 0x%02x", msg_type);

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, bd_addr, 6);
        payload.msg_buf = msg_buf;

        switch (msg_type)
        {
        case PBAP_MSG_CONNECTED:
            {
                p_link->connected_profile |= PBAP_PROFILE_MASK;
                bt_mgr_dispatch(BT_MSG_PBAP_CONN_CMPL, &payload);
            }
            break;

        case PBAP_MSG_CONNECTION_FAIL:
            {
                bt_mgr_dispatch(BT_MSG_PBAP_CONN_FAIL, &payload);
            }
            break;

        case PBAP_MSG_DISCONNECTED:
            {
                p_link->connected_profile &= ~PBAP_PROFILE_MASK;
                bt_mgr_dispatch(BT_MSG_PBAP_DISCONN_CMPL, &payload);
            }
            break;

        case PBAP_MSG_SET_PATH_DONE:
            {
                T_PBAP_SET_PHONE_BOOK_CMPL *p_msg_data = (T_PBAP_SET_PHONE_BOOK_CMPL *)msg_buf;

                if (p_msg_data->result == true)
                {
                    p_link->pbap_data.path = p_msg_data->path;
                }
                bt_mgr_dispatch(BT_MSG_PBAP_SET_PHONE_BOOK_CMPL, &payload);
            }
            break;

        case PBAP_MSG_GET_PHONE_BOOK_CMPL:
            {
                bt_mgr_dispatch(BT_MSG_PBAP_GET_PHONE_BOOK_CMPL, &payload);
            }
            break;

        case PBAP_MSG_GET_PHONE_BOOK_SIZE_CMPL:
            {
                bt_mgr_dispatch(BT_MSG_PBAP_GET_PHONE_BOOK_SIZE_CMPL, &payload);
            }
            break;

        case PBAP_MSG_GET_VCARD_LISTING_CMPL:
            {
                T_PBAP_GET_VCARD_LISTING_MSG_DATA *p_msg_data = (T_PBAP_GET_VCARD_LISTING_MSG_DATA *)msg_buf;
                uint8_t *p_vcard_list_data = p_msg_data->p_data;
                uint16_t data_len = p_msg_data->data_len;
                T_BT_PBAP_CALLER_ID_NAME caller_id_name;
                uint8_t *temp_ptr = NULL;

                caller_id_name.name_ptr = NULL;
                caller_id_name.name_len = 0;

                if (p_vcard_list_data != NULL)
                {
                    p_vcard_list_data[data_len - 1] = '\0';
                    temp_ptr = (uint8_t *)strstr((char *)p_vcard_list_data, "name");
                    if (temp_ptr != NULL)
                    {
                        uint8_t     step;
                        uint16_t    temp_len;

                        temp_len = data_len - (temp_ptr - p_vcard_list_data);
                        // name="xxx"
                        step = 0;
                        while (temp_len)
                        {
                            if (step == 0)
                            {
                                if (*temp_ptr == 0x22)
                                {
                                    step = 1;
                                    caller_id_name.name_len = 0;
                                    caller_id_name.name_ptr = temp_ptr + 1;
                                }
                            }
                            else if (step == 1)
                            {
                                if (*temp_ptr == 0x22)
                                {
                                    break;
                                }
                                caller_id_name.name_len++;
                            }
                            temp_ptr++;
                            temp_len--;
                        }
                    }

                    payload.msg_buf = &caller_id_name;
                    bt_mgr_dispatch(BT_MSG_PBAP_CALLER_ID_NAME, &payload);
                }
                else
                {
                    if (p_msg_data->data_end == false)
                    {
                        pbap_pull_continue(bd_addr);
                    }
                    else
                    {
                        payload.msg_buf = &caller_id_name;
                        bt_mgr_dispatch(BT_MSG_PBAP_CALLER_ID_NAME, &payload);
                    }
                }
            }
            break;

        case PBAP_MSG_GET_VCARD_ENTRY_CMPL:
            {
                T_PBAP_GET_VCARD_ENTRY_MSG_DATA *p_msg_data = (T_PBAP_GET_VCARD_ENTRY_MSG_DATA *)msg_buf;
                BTM_PRINT_INFO1("bt_pbap_cback: PBAP_MSG_GET_VCARD_ENTRY_CMPL data %s",
                                TRACE_STRING(p_msg_data->p_data));
            }
            break;

        default:
            break;
        }
    }
}

bool bt_pbap_connect_over_rfc_req(uint8_t bd_addr[6],
                                  uint8_t server_chann,
                                  bool    feat_flag)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & PBAP_PROFILE_MASK) == 0)
        {
            if (pbap_conn_over_rfc(bd_addr, server_chann, feat_flag) == true)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_pbap_connect_over_l2c_req(uint8_t  bd_addr[6],
                                  uint16_t l2c_psm,
                                  bool     feat_flag)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & PBAP_PROFILE_MASK) == 0)
        {
            if (pbap_conn_over_l2c(bd_addr, l2c_psm, feat_flag) == true)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_pbap_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return pbap_disconnect_req(bd_addr);
    }

    return false;
}

bool bt_pbap_phone_book_pull(uint8_t              bd_addr[6],
                             T_BT_PBAP_REPOSITORY repos,
                             T_BT_PBAP_PHONE_BOOK phone_book,
                             uint16_t             start_offset,
                             uint16_t             max_list_count,
                             uint64_t             filter)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & PBAP_PROFILE_MASK)
        {
            T_PBAP_PULL_PHONE_BOOK_PARAM param;

            param.filter = filter;
            param.format = PBAP_TAG_ID_FORMAT_VALUE_21;
            param.max_list_count = max_list_count;
            param.start_offset = start_offset;

            bt_sniff_mode_exit(p_link, false);
            return pbap_pull_phone_book(bd_addr, (T_PBAP_REPOSITORY)repos,
                                        (T_PBAP_PHONE_BOOK)phone_book, &param);
        }
    }

    return false;
}

bool bt_pbap_phone_book_size_get(uint8_t              bd_addr[6],
                                 T_BT_PBAP_REPOSITORY repos,
                                 T_BT_PBAP_PHONE_BOOK phone_book)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & PBAP_PROFILE_MASK)
        {
            T_PBAP_PULL_PHONE_BOOK_PARAM param;

            param.filter = 0;
            param.format = PBAP_TAG_ID_FORMAT_VALUE_21;
            param.max_list_count = 0;
            param.start_offset = 0;

            bt_sniff_mode_exit(p_link, false);
            return pbap_pull_phone_book(bd_addr, (T_PBAP_REPOSITORY)repos,
                                        (T_PBAP_PHONE_BOOK)phone_book, &param);
        }
    }

    return false;
}

bool bt_pbap_pull_continue(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & PBAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return pbap_pull_continue(bd_addr);
        }
    }

    return false;
}

bool bt_pbap_pull_abort(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & PBAP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return pbap_pull_abort(bd_addr);
        }
    }

    return false;
}

bool bt_pbap_phone_book_set(uint8_t              bd_addr[6],
                            T_BT_PBAP_REPOSITORY repos,
                            T_BT_PBAP_PATH       path)
{
    T_BT_BR_LINK *p_link;
    bool ret = false;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if ((p_link->connected_profile & PBAP_PROFILE_MASK))
        {
            p_link->pbap_data.repos = repos;

            ret = pbap_set_phone_book(bd_addr, PBAP_PATH_ROOT);
            if (path != BT_PBAP_PATH_ROOT)
            {
                if (repos == BT_PBAP_REPOSITORY_SIM1)
                {
                    ret = (ret && pbap_set_phone_book(bd_addr, PBAP_PATH_SIM1));
                }
                ret = (ret && pbap_set_phone_book(bd_addr, PBAP_PATH_TELECOM));

                if (path != BT_PBAP_PATH_TELECOM)
                {
                    ret = (ret && pbap_set_phone_book(bd_addr, (T_PBAP_PATH)path));
                }
            }
        }
    }

    return ret;
}

bool bt_pbap_vcard_listing_by_number_pull(uint8_t  bd_addr[6],
                                          char    *phone_number)
{
    return pbap_pull_vcard_listing_by_number(bd_addr, phone_number);
}

bool bt_pbap_pull_vcard_entry(uint8_t  bd_addr[6],
                              uint8_t *p_name,
                              uint8_t  name_len,
                              uint64_t filter)
{
    return pbap_pull_vcard_entry(bd_addr, p_name, name_len, filter,
                                 PBAP_TAG_ID_FORMAT_VALUE_21);
}

bool bt_pbap_init(uint8_t link_num)
{
    return pbap_init(link_num, bt_pbap_cback, 0);
}
#else
#include <stdint.h>
#include <stdbool.h>
#include "bt_pbap.h"

bool bt_pbap_init(uint8_t link_num)
{
    return false;
}

bool bt_pbap_connect_over_rfc_req(uint8_t bd_addr[6],
                                  uint8_t server_chann,
                                  bool    feat_flag)
{
    return false;
}

bool bt_pbap_connect_over_l2c_req(uint8_t  bd_addr[6],
                                  uint16_t l2c_psm,
                                  bool     feat_flag)
{
    return false;
}

bool bt_pbap_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_pbap_phone_book_pull(uint8_t              bd_addr[6],
                             T_BT_PBAP_REPOSITORY repos,
                             T_BT_PBAP_PHONE_BOOK phone_book,
                             uint16_t             start_offset,
                             uint16_t             max_list_count,
                             uint64_t             filter)
{
    return false;
}

bool bt_pbap_phone_book_size_get(uint8_t              bd_addr[6],
                                 T_BT_PBAP_REPOSITORY repos,
                                 T_BT_PBAP_PHONE_BOOK phone_book)
{
    return false;
}

bool bt_pbap_pull_continue(uint8_t bd_addr[6])
{
    return false;
}

bool bt_pbap_pull_abort(uint8_t bd_addr[6])
{
    return false;
}

bool bt_pbap_phone_book_set(uint8_t              bd_addr[6],
                            T_BT_PBAP_REPOSITORY repos,
                            T_BT_PBAP_PATH       path)
{
    return false;
}

bool bt_pbap_vcard_listing_by_number_pull(uint8_t  bd_addr[6],
                                          char    *phone_number)
{
    return false;
}
#endif
