/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_IAP_SUPPORT == 1)
#include <string.h>
#include "trace.h"
#include "iap.h"
#include "iap_cp.h"
#include "bt_iap.h"
#include "bt_iap_int.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

uint8_t *bt_iap2_prep_param(uint8_t  *p_param,
                            uint16_t  param_id,
                            void     *p_data,
                            uint16_t  data_len)
{
    return iap2_prep_param(p_param, param_id, p_data, data_len);
}

bool bt_iap_ident_info_send(uint8_t   bd_addr[6],
                            uint8_t  *p_ident,
                            uint16_t  ident_len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_send_ident_info(bd_addr, p_ident, ident_len);
        }
    }

    return false;
}

void bt_iap_cback(uint8_t    bd_addr[6],
                  T_IAP_MSG  msg_type,
                  void      *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    PROFILE_PRINT_INFO1("bt_iap_cback: msg_type %d", msg_type);

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        memcpy(payload.bd_addr, bd_addr, 6);
        payload.msg_buf = msg_buf;

        switch (msg_type)
        {
        case IAP_MSG_CONN_IND:
            {
                bt_mgr_dispatch(BT_MSG_IAP_CONN_IND, &payload);
            }
            break;

        case IAP_MSG_CONNECTED:
            {
                T_IAP_CONN_CMPL_INFO *p_info = (T_IAP_CONN_CMPL_INFO *)msg_buf;

                p_link->iap_data.max_data_len = p_info->max_data_len;
                p_link->connected_profile |= IAP_PROFILE_MASK;
                bt_mgr_dispatch(BT_MSG_IAP_CONN_CMPL, &payload);
            }
            break;

        case IAP_MSG_CONN_FAIL:
            {
                bt_mgr_dispatch(BT_MSG_IAP_CONN_FAIL, &payload);
            }
            break;

        case IAP_MSG_DISCONNTED:
            {
                if (p_link->connected_profile & IAP_PROFILE_MASK)
                {
                    p_link->connected_profile &= ~IAP_PROFILE_MASK;
                    bt_mgr_dispatch(BT_MSG_IAP_DISCONN_CMPL, &payload);
                }
            }
            break;

        case IAP_MSG_AUTHEN_FAIL:
            {
                bt_mgr_dispatch(BT_MSG_IAP_AUTHEN_FAIL, &payload);
            }
            break;

        case IAP_MSG_AUTHEN_SUCCESS:
            {
                bt_mgr_dispatch(BT_MSG_IAP_AUTHEN_SUCCESS, &payload);
            }
            break;

        case IAP_MSG_START_IDENT_REQ:
            {
                bt_mgr_dispatch(BT_MSG_IAP_START_IDENT_REQ, &payload);
            }
            break;

        case IAP_MSG_DATA_IND:
            {
                bt_mgr_dispatch(BT_MSG_IAP_DATA_IND, &payload);
            }
            break;

        case IAP_MSG_START_EAP_SESSION:
            {
                T_IAP_START_EAP_SESSION *p_iap_start_eap_session = (T_IAP_START_EAP_SESSION *)msg_buf;

                p_link->iap_data.eap_id = p_iap_start_eap_session->eap_id;
                p_link->iap_data.eap_session_id = p_iap_start_eap_session->eap_session_id;
                bt_mgr_dispatch(BT_MSG_IAP_START_EAP_SESSION, &payload);
            }
            break;

        case IAP_MSG_STOP_EAP_SESSION:
            {
                bt_mgr_dispatch(BT_MSG_IAP_STOP_EAP_SESSION, &payload);
            }
            break;

        case IAP_MSG_EAP_SESSION_STATUS:
            {
                bt_mgr_dispatch(BT_MSG_IAP_EAP_SESSION_STATUS, &payload);
            }
            break;

        case IAP_MSG_DATA_TRANSMITTED:
            {
                bt_mgr_dispatch(BT_MSG_IAP_DATA_TRANSMITTED, &payload);
            }
            break;

        case IAP_MSG_RESET:
            {
                bt_mgr_dispatch(BT_MSG_IAP_RESET, &payload);
            }
            break;

        case IAP_MSG_CTRL_MSG_IND:
            {
                bt_mgr_dispatch(BT_MSG_IAP_CTRL_MSG_IND, &payload);
            }
            break;

        default:
            break;
        }
    }
    else
    {
        iap_disconnect_req(bd_addr);
    }
}

bool bt_iap_data_send(uint8_t   bd_addr[6],
                      uint16_t  session_id,
                      uint8_t  *p_data,
                      uint16_t  data_len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_send_data(bd_addr, session_id, p_data, data_len);
        }
    }

    return false;
}

bool bt_iap_eap_session_status_send(uint8_t                     bd_addr[6],
                                    uint16_t                    session_id,
                                    T_BT_IAP_EAP_SESSION_STATUS status)
{
    bool result = false;
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            switch (status)
            {
            case BT_IAP_EAP_SESSION_STATUS_OK:
                result = iap_eap_session_status_send(bd_addr, session_id, IAP_EAP_SESSION_STATUS_OK);
                break;

            case BT_IAP_EAP_SESSION_STATUS_CLOSED:
                result = iap_eap_session_status_send(bd_addr, session_id, IAP_EAP_SESSION_STATUS_CLOSED);
                break;

            default:
                break;
            }
        }
    }

    return result;
}

bool bt_iap_app_launch(uint8_t                     bd_addr[6],
                       char                       *boundle_id,
                       uint8_t                     len_boundle_id,
                       T_BT_IAP_APP_LAUNCH_METHOD  method)
{
    bool result = false;
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            switch (method)
            {
            case BT_IAP_APP_LAUNCH_WITH_USER_ALERT:
                result = iap_launch_app(bd_addr, boundle_id, len_boundle_id, IAP_APP_LAUNCH_WITH_USER_ALERT);
                break;

            case BT_IAP_APP_LAUNCH_WITHOUT_USER_ALERT:
                result = iap_launch_app(bd_addr, boundle_id, len_boundle_id, IAP_APP_LAUNCH_WITHOUT_USER_ALERT);
                break;

            default:
                break;
            }
        }
    }

    return result;
}

bool bt_iap_connect_req(uint8_t  bd_addr[6],
                        uint8_t  server_chann,
                        uint16_t frame_size,
                        uint8_t  init_credits)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        return iap_connect_req(bd_addr, server_chann, frame_size, init_credits);
    }
    else
    {
        BTM_PRINT_ERROR0("bt_iap_connect_req: error!!");
    }

    return false;
}

bool bt_iap_connect_cfm(uint8_t  bd_addr[6],
                        bool     accept,
                        uint16_t frame_size,
                        uint8_t  init_credits)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return iap_connect_cfm(bd_addr, accept, frame_size, init_credits);
    }
    else
    {
        BTM_PRINT_ERROR0("bt_iap_connect_cfm: error!!");
    }

    return false;
}

bool bt_iap_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return iap_disconnect_req(bd_addr);
    }

    return false;
}

bool bt_iap_param_set(T_BT_IAP_PARAM_TYPE  type,
                      uint8_t              len,
                      void                *p_value)
{
    return iap_set_param((T_IAP_PARAM_TYPE)type, len, p_value);
}

bool bt_iap_ack_send(uint8_t bd_addr[6],
                     uint8_t ack_seq)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_send_ack(bd_addr, ack_seq);
        }
    }

    return false;
}

bool bt_iap_hid_start(uint8_t   bd_addr[6],
                      uint16_t  hid_component_id,
                      uint16_t  vid,
                      uint16_t  pid,
                      uint8_t  *hid_report_desc,
                      uint16_t  hid_report_desc_len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_hid_start(bd_addr, hid_component_id, vid, pid, hid_report_desc, hid_report_desc_len);
        }
    }

    return false;
}

bool bt_iap_hid_report_send(uint8_t   bd_addr[6],
                            uint16_t  hid_component_id,
                            uint8_t  *hid_report,
                            uint16_t  hid_report_len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_hid_send_report(bd_addr, hid_component_id, hid_report, hid_report_len);
        }
    }

    return false;
}

bool bt_iap_hid_stop(uint8_t  bd_addr[6],
                     uint16_t hid_component_id)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_hid_stop(bd_addr, hid_component_id);
        }
    }

    return false;
}

bool bt_iap_comp_info_send(uint8_t  bd_addr[6],
                           uint16_t comp_id,
                           bool     enable)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_send_bt_comp_info(bd_addr, comp_id, enable);
        }
    }

    return false;
}

bool bt_iap_conn_update_start(uint8_t  bd_addr[6],
                              uint16_t comp_id)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_send_start_bt_conn_update(bd_addr, comp_id);
        }
    }

    return false;
}

bool bt_iap_comm_update_start(uint8_t  bd_addr[6],
                              uint16_t param_id)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_send_start_comm_update(bd_addr, param_id);
        }
    }

    return false;
}

bool bt_iap_mute_status_update(uint8_t bd_addr[6],
                               bool    status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_send_mute_status_update(bd_addr, status);
        }
    }

    return false;
}

bool bt_iap_send_cmd(uint8_t   bd_addr[6],
                     uint16_t  msg_type,
                     uint8_t  *p_param,
                     uint16_t  param_len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & IAP_PROFILE_MASK)
        {
            return iap_send_cmd(bd_addr, msg_type, p_param, param_len);
        }
    }

    return false;
}

bool bt_iap_init(uint8_t         link_num,
                 uint8_t         rfc_iap_chann_num,
                 P_CP_WRITE_FUNC p_cp_write,
                 P_CP_READ_FUNC  p_cp_read)
{
    cp_init(p_cp_write, p_cp_read);
    return iap_init(link_num, rfc_iap_chann_num, bt_iap_cback);
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bt_iap.h"

bool bt_iap_init(uint8_t         link_num,
                 uint8_t         rfc_iap_chann_num,
                 P_CP_WRITE_FUNC p_cp_write,
                 P_CP_READ_FUNC  p_cp_read)
{
    return false;
}

bool bt_iap_data_send(uint8_t   bd_addr[6],
                      uint16_t  session_id,
                      uint8_t  *p_data,
                      uint16_t  data_len)
{
    return false;
}

bool bt_iap_eap_session_status_send(uint8_t                     bd_addr[6],
                                    uint16_t                    session_id,
                                    T_BT_IAP_EAP_SESSION_STATUS status)
{
    return false;
}

bool bt_iap_app_launch(uint8_t                     bd_addr[6],
                       char                       *boundle_id,
                       uint8_t                     len_boundle_id,
                       T_BT_IAP_APP_LAUNCH_METHOD  method)
{
    return false;
}

bool bt_iap_connect_req(uint8_t  bd_addr[6],
                        uint8_t  server_chann,
                        uint16_t frame_size,
                        uint8_t  init_credits)
{
    return false;
}

bool bt_iap_connect_cfm(uint8_t  bd_addr[6],
                        bool     accept,
                        uint16_t frame_size,
                        uint8_t  init_credits)
{
    return false;
}

bool bt_iap_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

uint8_t *bt_iap2_prep_param(uint8_t  *p_param,
                            uint16_t  param_id,
                            void     *p_data,
                            uint16_t  data_len)
{
    return NULL;
}

bool bt_iap_ident_info_send(uint8_t   bd_addr[6],
                            uint8_t  *p_ident,
                            uint16_t  ident_len)
{
    return false;
}

bool bt_iap_param_set(T_BT_IAP_PARAM_TYPE  type,
                      uint8_t              len,
                      void                *p_value)
{
    return false;
}

bool bt_iap_ack_send(uint8_t bd_addr[6],
                     uint8_t ack_seq)
{
    return false;
}

bool bt_iap_comp_info_send(uint8_t  bd_addr[6],
                           uint16_t comp_id,
                           bool     enable)
{
    return false;
}

bool bt_iap_conn_update_start(uint8_t  bd_addr[6],
                              uint16_t comp_id)
{
    return false;
}

bool bt_iap_comm_update_start(uint8_t  bd_addr[6],
                              uint16_t param_id)
{
    return false;
}

bool bt_iap_mute_status_update(uint8_t bd_addr[6],
                               bool    status)
{
    return false;
}

bool bt_iap_send_cmd(uint8_t   bd_addr[6],
                     uint16_t  msg_type,
                     uint8_t  *p_param,
                     uint16_t  param_len)
{
    return false;
}

bool bt_iap_hid_start(uint8_t   bd_addr[6],
                      uint16_t  hid_component_id,
                      uint16_t  vid,
                      uint16_t  pid,
                      uint8_t  *hid_report_desc,
                      uint16_t  hid_report_desc_len)
{
    return false;
}

bool bt_iap_hid_report_send(uint8_t   bd_addr[6],
                            uint16_t  hid_component_id,
                            uint8_t  *hid_report,
                            uint16_t  hid_report_len)
{
    return false;
}

bool bt_iap_hid_stop(uint8_t  bd_addr[6],
                     uint16_t hid_component_id)
{
    return false;
}
#endif
