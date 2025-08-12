/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_AVP_SUPPORT == 1)
#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "bt_types.h"
#include "bt_mgr.h"
#include "bt_roleswap.h"
#include "avp.h"
#include "bt_avp.h"

P_BT_AVP_CBACK app_avp_cback = NULL;
uint8_t siri_seq = 0x00;

const uint8_t all_bud_is_compacted[188] = { 0x04, 0x00, 0x04, 0x00, 0x21, 0x00, 0x01, 0x03, 0xB2, 0x00,
                                            0x00, 0x00, 0xD0, 0xE6, 0xD9, 0x41, 0x9B, 0x46, 0xAE, 0x3E,
                                            0xE0, 0x93, 0xC4, 0x3B, 0xFE, 0x9E, 0x2C, 0x3E, 0xF1, 0x87,
                                            0x11, 0x3F, 0xA5, 0x8D, 0x06, 0x3F, 0xA1, 0x28, 0x0F, 0x3F,
                                            0x3C, 0xD3, 0x1B, 0x3F, 0x1D, 0x17, 0xF2, 0x3E, 0xF1, 0x6F,
                                            0x25, 0x3F, 0xEF, 0xB9, 0x0B, 0x3F, 0xDA, 0x9C, 0x13, 0x3F,
                                            0xD5, 0xBD, 0x11, 0x3F, 0x8B, 0xC6, 0x0B, 0x3F, 0x10, 0x43,
                                            0x23, 0x3F, 0x75, 0xBA, 0x01, 0x3F, 0xF3, 0xC3, 0x02, 0x3F,
                                            0x68, 0xF8, 0xFF, 0x3E, 0xBE, 0x3B, 0x50, 0x3F, 0xDD, 0x52,
                                            0xC4, 0x3E, 0xB9, 0xCE, 0x05, 0x3F, 0xC8, 0x3F, 0xBE, 0x3E,
                                            0x8E, 0xCE, 0xB2, 0x41, 0x53, 0x02, 0x85, 0x3E, 0xC0, 0x80,
                                            0x87, 0x3C, 0xD0, 0x36, 0xE9, 0x3E, 0x32, 0x6F, 0x0C, 0x3F,
                                            0xBB, 0x35, 0x1A, 0x3F, 0xDC, 0xF6, 0x14, 0x3F, 0x57, 0xFB,
                                            0x1F, 0x3F, 0xD4, 0x75, 0x64, 0x3E, 0xB3, 0x3B, 0x30, 0x3F,
                                            0x24, 0x5C, 0xED, 0x3E, 0xDA, 0x57, 0xCD, 0x3E, 0x02, 0x34,
                                            0x2F, 0x3F, 0xED, 0x3E, 0xA5, 0x3E, 0x97, 0x18, 0xDC, 0x3E,
                                            0x2F, 0xB7, 0x1D, 0x3F, 0x3B, 0x58, 0x82, 0x3E, 0x59, 0x99,
                                            0xEC, 0x3E, 0x7D, 0xF1, 0x59, 0x3F, 0x71, 0xD5, 0xB7, 0x3E,
                                            0x1F, 0x33, 0x0A, 0x3F, 0xC5, 0xF5, 0x9C, 0x3E
                                          };

const uint8_t right_bud_is_compacted[188] = {0x04, 0x00, 0x04, 0x00, 0x21, 0x00, 0x01, 0x03, 0xB2, 0x00,
                                             0x00, 0x00, 0xC3, 0x45, 0x97, 0x41, 0x7C, 0xE2, 0x5E, 0x3D,
                                             0x40, 0x1E, 0x4F, 0x3C, 0xAB, 0x69, 0x8B, 0x3E, 0x8C, 0x9B,
                                             0x83, 0x3F, 0xB0, 0xC7, 0x94, 0x3F, 0x08, 0x00, 0xA5, 0x3F,
                                             0xE3, 0x5C, 0xA3, 0x3F, 0xFA, 0x2D, 0x91, 0x3F, 0x18, 0x98,
                                             0x8E, 0x3F, 0xA6, 0x8A, 0x39, 0x3F, 0xBB, 0x76, 0x57, 0x3F,
                                             0x75, 0x20, 0x0B, 0x3F, 0x59, 0xF7, 0x4B, 0x3F, 0x56, 0x47,
                                             0x45, 0x3F, 0x86, 0x4D, 0x26, 0x3F, 0x9F, 0x14, 0x32, 0x3F,
                                             0xBE, 0x36, 0x07, 0x3F, 0x25, 0x2D, 0xF2, 0x3E, 0xB0, 0x6D,
                                             0x0D, 0x3F, 0x02, 0x2A, 0x04, 0x3F, 0xE6, 0x78, 0xE0, 0x3E,
                                             0x7A, 0x31, 0x86, 0x41, 0xF7, 0xA6, 0xB8, 0x3D, 0xA0, 0xC2,
                                             0x35, 0x3C, 0xA1, 0x62, 0xDC, 0x3B, 0x04, 0x22, 0x65, 0x3D,
                                             0x60, 0xBF, 0x99, 0x3D, 0x32, 0x4C, 0xE0, 0x3D, 0xAF, 0x56,
                                             0x18, 0x3E, 0x00, 0xE8, 0x3F, 0x3E, 0x49, 0x92, 0x51, 0x3E,
                                             0xB6, 0x34, 0xB5, 0x3E, 0xC2, 0x83, 0xA5, 0x3E, 0xC0, 0xB3,
                                             0x2D, 0x3F, 0x9D, 0xBC, 0x95, 0x3E, 0xA0, 0x15, 0x1B, 0x3F,
                                             0x00, 0x69, 0xB6, 0x3E, 0x10, 0xB7, 0x08, 0x3F, 0x99, 0xCF,
                                             0x1F, 0x3F, 0x21, 0x16, 0x2B, 0x3F, 0x33, 0xC9, 0x90, 0x3F,
                                             0x6D, 0x85, 0xA7, 0x3F, 0x9D, 0x62, 0xD1, 0x3F
                                            };

const uint8_t left_bud_is_compacted[188] = {0x04, 0x00, 0x04, 0x00, 0x21, 0x00, 0x01, 0x03, 0xB2, 0x00,
                                            0x00, 0x00, 0x84, 0x9F, 0x65, 0x41, 0xF8, 0xD3, 0xA0, 0x3D,
                                            0x00, 0x94, 0x3D, 0x3A, 0x37, 0x5B, 0x79, 0x3C, 0xE9, 0x57,
                                            0x0D, 0x3D, 0x0C, 0x64, 0x56, 0x3D, 0x1C, 0xFF, 0x84, 0x3D,
                                            0x1D, 0xAB, 0x82, 0x3D, 0x21, 0x9F, 0xF7, 0x3D, 0xD8, 0xD1,
                                            0xF6, 0x3D, 0x5D, 0x81, 0x37, 0x3E, 0x58, 0x37, 0x1A, 0x3E,
                                            0x0E, 0x1F, 0x8B, 0x3E, 0x7E, 0x87, 0x31, 0x3E, 0x8B, 0x16,
                                            0xB1, 0x3E, 0x3D, 0x61, 0x4D, 0x3E, 0x44, 0x04, 0xBA, 0x3E,
                                            0x4B, 0x3A, 0xB1, 0x3E, 0x78, 0xEF, 0xAB, 0x3E, 0x5A, 0xEF,
                                            0xF1, 0x3E, 0x81, 0x39, 0x27, 0x3F, 0xA8, 0xE1, 0x2F, 0x3F,
                                            0x53, 0x47, 0xA9, 0x41, 0x36, 0x37, 0x9F, 0x3D, 0x00, 0x0D,
                                            0x2B, 0x3B, 0x05, 0x47, 0x88, 0x3E, 0x70, 0x3E, 0x82, 0x3F,
                                            0x7A, 0x46, 0x8A, 0x3F, 0x0E, 0x2F, 0xA3, 0x3F, 0x78, 0xFC,
                                            0xAF, 0x3F, 0x59, 0x95, 0xAC, 0x3F, 0x10, 0x0E, 0xA8, 0x3F,
                                            0xBA, 0xC3, 0x8F, 0x3F, 0xAC, 0x4C, 0x9E, 0x3F, 0xB1, 0x71,
                                            0x55, 0x3F, 0x43, 0x08, 0x74, 0x3F, 0x23, 0xB4, 0x48, 0x3F,
                                            0xB0, 0x3F, 0x5E, 0x3F, 0x49, 0xB7, 0x29, 0x3F, 0x40, 0x2A,
                                            0x55, 0x3F, 0xDA, 0xFA, 0x13, 0x3F, 0xA8, 0x5E, 0x2E, 0x3F,
                                            0xB4, 0xAD, 0x16, 0x3F, 0x1B, 0x18, 0xC5, 0x3E
                                           };

const uint8_t all_bud_is_not_compacted[188] = { 0x04, 0x00, 0x04, 0x00, 0x21, 0x00, 0x01, 0x03, 0xB2, 0x00,
                                                0x00, 0x00, 0x2C, 0xC3, 0xAD, 0x41, 0x04, 0xCC, 0xC1, 0x3D,
                                                0x00, 0x54, 0xBF, 0x3B, 0xDB, 0x6B, 0xE8, 0x3B, 0x49, 0x29,
                                                0x7B, 0x3C, 0x93, 0x99, 0xAF, 0x3C, 0x68, 0x43, 0xF4, 0x3C,
                                                0x53, 0xF6, 0x06, 0x3D, 0x1C, 0x1F, 0x3B, 0x3D, 0x1F, 0x46,
                                                0x5D, 0x3D, 0x99, 0x39, 0x96, 0x3D, 0x1E, 0xEE, 0xAC, 0x3D,
                                                0xF9, 0x5C, 0x07, 0x3E, 0x9E, 0xDB, 0xFC, 0x3D, 0x95, 0x49,
                                                0x2C, 0x3E, 0x6E, 0xEF, 0x35, 0x3E, 0x69, 0x05, 0x3F, 0x3E,
                                                0x54, 0x1F, 0x8F, 0x3E, 0x55, 0xCA, 0xA9, 0x3E, 0x94, 0x51,
                                                0xD3, 0x3E, 0x91, 0x69, 0x08, 0x3F, 0x33, 0xD2, 0x18, 0x3F,
                                                0x4C, 0x57, 0xB5, 0x41, 0x73, 0x44, 0xD9, 0x3D, 0xC0, 0x91,
                                                0xE0, 0x3B, 0x2B, 0xAA, 0x1B, 0x3C, 0xAF, 0x64, 0x3D, 0x3D,
                                                0x7D, 0xCF, 0xB4, 0x3D, 0x60, 0x1B, 0xC6, 0x3D, 0x2A, 0xC6,
                                                0x0B, 0x3E, 0x91, 0x98, 0x17, 0x3E, 0xAD, 0xDD, 0x40, 0x3E,
                                                0x04, 0x02, 0xB7, 0x3E, 0xBA, 0xD2, 0x8E, 0x3E, 0xEE, 0x92,
                                                0x22, 0x3F, 0x1B, 0x45, 0x8B, 0x3E, 0x88, 0x43, 0x07, 0x3F,
                                                0xBA, 0x71, 0xC4, 0x3E, 0x6E, 0xD5, 0xFE, 0x3E, 0x08, 0xBC,
                                                0x3C, 0x3F, 0x56, 0xC9, 0x44, 0x3F, 0xEC, 0xED, 0x8A, 0x3F,
                                                0x75, 0x92, 0xA3, 0x3F, 0x87, 0x05, 0xAB, 0x3F
                                              };


bool bt_avp_control_data_send(uint8_t   bd_addr[6],
                              uint8_t  *p_data,
                              uint16_t  data_len,
                              bool      flushable)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return avp_control_send_data(bd_addr, p_data, data_len, flushable);
    }

    return false;
}

bool bt_avp_audio_connect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return avp_audio_conn_req(bd_addr);
    }

    return false;
}

bool bt_avp_audio_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return avp_audio_disconn_req(bd_addr);
    }

    return false;
}

bool bt_avp_control_connect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return avp_control_conn_req(bd_addr);
    }

    return false;
}

bool bt_avp_control_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return avp_control_disconn_req(bd_addr);
    }

    return false;
}

bool bt_avp_voice_recognition_enable_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t buf[5] = {0x01, 0x00, 0x01, 0x01, 0x02};

        return avp_voice_recognition_enable_req(bd_addr, p_link->acl_handle, buf, sizeof(buf));
    }

    return false;
}

bool bt_avp_voice_recognition_disable_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t buf[4] = {0x01, 0x00, 0x05, 0x00};

        return avp_voice_recognition_disable_req(bd_addr, p_link->acl_handle, buf, sizeof(buf));
    }

    return false;

}

bool bt_avp_voice_recognition_encode_start(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t buf[2] = {0x01, 0x00};

        return avp_voice_recognition_encode_start(bd_addr, p_link->acl_handle, buf, sizeof(buf));
    }

    return false;
}

bool bt_avp_voice_recognition_encode_stop(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t buf[4] = {0x01, 0x00, 0x03, 0x00};

        return avp_voice_recognition_encode_stop(bd_addr, p_link->acl_handle, buf, sizeof(buf));
    }

    return false;
}

bool bt_avp_voice_recognition_data_send(uint8_t   bd_addr[6],
                                        uint8_t  *p_data,
                                        uint16_t  data_len)
{
    T_BT_BR_LINK *p_link;
    bool ret = false;
    uint8_t *p_buf;
    uint8_t *p;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_buf = os_mem_alloc2(data_len + 10);
        if (p_buf)
        {
            p = p_buf;
            LE_UINT8_TO_STREAM(p, 0x01);
            LE_UINT8_TO_STREAM(p, 0x00);
            LE_UINT8_TO_STREAM(p, siri_seq);
            LE_UINT16_TO_STREAM(p, data_len + 5);
            LE_UINT16_TO_STREAM(p, 0x0000);
            LE_UINT8_TO_STREAM(p, siri_seq);
            LE_UINT8_TO_STREAM(p, 0x00);
            LE_UINT8_TO_STREAM(p, (uint8_t)data_len);
            memcpy(p, p_data, data_len);

            ret = avp_voice_recognition_data_send(bd_addr, p_link->acl_handle, p_buf, data_len + 10);

            os_mem_free(p_buf);

            if (siri_seq == 0xff)
            {
                siri_seq = 0;
            }
            else
            {
                siri_seq++;
            }
        }
    }

    return ret;
}

void bt_avp_cback(uint8_t    bd_addr[6],
                  T_AVP_MSG  msg_type,
                  void      *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;
    T_BT_AVP_EVENT_PARAM param;

    p_link = bt_find_br_link(bd_addr);
    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = NULL;

    BTM_PRINT_INFO1("bt_avp_cback: msg_type %d", msg_type);

    switch (msg_type)
    {
    case AVP_MSG_CONTROL_CONNECTED:
        {
            if (p_link != NULL)
            {
                bt_roleswap_handle_bt_avp_control_conn(bd_addr);

                if (app_avp_cback)
                {
                    memcpy(param.control_conn_cmpl.bd_addr, bd_addr, 6);
                    app_avp_cback(BT_AVP_EVENT_CONTROL_CONN_CMPL, &param, sizeof(param));
                }
            }
            else
            {
                avp_control_disconn_req(bd_addr);
            }
        }
        break;

    case AVP_MSG_AUDIO_CONNECTED:
        {
            if (p_link != NULL)
            {
                bt_roleswap_handle_bt_avp_audio_conn(bd_addr);

                if (app_avp_cback)
                {
                    memcpy(param.audio_conn_cmpl.bd_addr, bd_addr, 6);
                    app_avp_cback(BT_AVP_EVENT_AUDIO_CONN_CMPL, &param, sizeof(param));
                }
            }
        }
        break;

    case AVP_MSG_CONTROL_DISCONNECTED:
        {
            if (p_link != NULL)
            {
                uint16_t cause = *(uint16_t *)msg_buf;

                bt_roleswap_handle_bt_avp_control_disconn(bd_addr, cause);

                if (app_avp_cback)
                {
                    if (cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                    {
                        memcpy(param.control_disconn_cmpl.bd_addr, bd_addr, 6);
                        app_avp_cback(BT_AVP_EVENT_CONTROL_DISCONN_CMPL, &param, sizeof(param));
                    }
                }
            }
        }
        break;

    case AVP_MSG_AUDIO_DISCONNECTED:
        {
            if (p_link != NULL)
            {
                uint16_t cause = *(uint16_t *)msg_buf;

                bt_roleswap_handle_bt_avp_audio_disconn(bd_addr, cause);

                if (app_avp_cback)
                {
                    if (cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                    {
                        memcpy(param.audio_disconn_cmpl.bd_addr, bd_addr, 6);
                        app_avp_cback(BT_AVP_EVENT_AUDIO_DISCONN_CMPL, &param, sizeof(param));
                    }
                }
            }
        }
        break;

    case AVP_MSG_CONTROL_DATA_IND:
        {
            if (p_link != NULL)
            {
                T_AVP_DATA_IND *p_msg = (T_AVP_DATA_IND *)msg_buf;

                /*sync packet*/
                if ((p_msg->p_data[0] == 0x00) && (p_msg->p_data[1] == 0x00) &&
                    (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00))
                {
                    /*uint8_t powerbeats[18] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00,
                                              0x0d, 0x00, 0x01, 0x00, 0x07, 0x00, 0x79, 0x8f};*/

                    uint8_t durian_pro[18] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00,
                                              0x02, 0x00, 0x05, 0x00, 0x49, 0x4E, 0x05, 0x00,
                                              0xA5, 0x4F
                                             };

                    bt_avp_control_data_send(bd_addr, durian_pro, sizeof(durian_pro), false);

                    if (app_avp_cback)
                    {
                        memcpy(param.version_sync.bd_addr, bd_addr, 6);
                        app_avp_cback(BT_AVP_EVENT_VERSION_SYNC, &param, sizeof(param));
                    }
                }
                /*iphone set name*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x1A) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x01))
                {
                    if (app_avp_cback)
                    {
                        param.set_name.len = p_msg->p_data[7] + (p_msg->p_data[8] << 8);
                        param.set_name.data = &p_msg->p_data[9];
                        memcpy(param.set_name.bd_addr, bd_addr, 6);
                        app_avp_cback(BT_AVP_EVENT_SET_NAME, &param, sizeof(param));
                    }
                }
                /*pod1&2 click settings*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x0C))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.control_settings.bd_addr, bd_addr, 6);
                        param.control_settings.right_ear_control = (T_BT_AVP_CONTROL)p_msg->p_data[7];
                        param.control_settings.left_ear_control = (T_BT_AVP_CONTROL)p_msg->p_data[8];
                        app_avp_cback(BT_AVP_EVENT_CONTROL_SETTINGS, &param, sizeof(param));
                    }
                }
                /*in_ear detection*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x0A))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.in_ear_detection.bd_addr, bd_addr, 6);

                        if (p_msg->p_data[7] == 0x01)
                        {
                            param.in_ear_detection.open = false;
                        }
                        else if (p_msg->p_data[7] == 0x02)
                        {
                            param.in_ear_detection.open = true;
                        }

                        app_avp_cback(BT_AVP_EVENT_IN_EAR_DETECTION, &param, sizeof(param));
                    }
                }
                /*mic settings*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x01))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.mic_settings.bd_addr, bd_addr, 6);
                        param.mic_settings.setting = (T_BT_AVP_MIC)p_msg->p_data[7];
                        app_avp_cback(BT_AVP_EVENT_MIC_SETTINGS, &param, sizeof(param));
                    }
                }
                /*anc settings*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x0D))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.anc_settings.bd_addr, bd_addr, 6);
                        if (p_msg->p_data[7] == 0x01)
                        {
                            param.anc_settings.setting = BT_AVP_ANC_CLOSE;
                        }
                        else if (p_msg->p_data[7] == 0x02)
                        {
                            param.anc_settings.setting = BT_AVP_ANC_OPEN;
                        }
                        else if (p_msg->p_data[7] == 0x03)
                        {
                            param.anc_settings.setting = BT_AVP_ANC_TRANSPARENCY_MODE;
                        }

                        app_avp_cback(BT_AVP_EVENT_ANC_SETTINGS, &param, sizeof(param));
                    }
                }

                /*pro click settings: anc or voice recg*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x16))
                {
                    if (p_msg->p_data[7] == 0x05)
                    {
                        param.click_settings.right_ear_longpress_control = BT_AVP_CONTROL_ANC;
                    }
                    else if (p_msg->p_data[7] == 0x01)
                    {
                        param.click_settings.right_ear_longpress_control = BT_AVP_CONTROL_VOICE_RECOGNITION;
                    }
                    else if (p_msg->p_data[7] == 0x06)
                    {
                        param.click_settings.right_ear_longpress_control = BT_AVP_CONTROL_VOL_UP;
                    }
                    else if (p_msg->p_data[7] == 0x07)
                    {
                        param.click_settings.right_ear_longpress_control = BT_AVP_CONTROL_VOL_DOWN;
                    }

                    if (p_msg->p_data[8] == 0x05)
                    {
                        param.click_settings.left_ear_longpress_control = BT_AVP_CONTROL_ANC;
                    }
                    else if (p_msg->p_data[8] == 0x01)
                    {
                        param.click_settings.left_ear_longpress_control = BT_AVP_CONTROL_VOICE_RECOGNITION;
                    }
                    else if (p_msg->p_data[8] == 0x06)
                    {
                        param.click_settings.left_ear_longpress_control = BT_AVP_CONTROL_VOL_UP;
                    }
                    else if (p_msg->p_data[8] == 0x07)
                    {
                        param.click_settings.left_ear_longpress_control = BT_AVP_CONTROL_VOL_DOWN;
                    }

                    if (app_avp_cback)
                    {
                        memcpy(param.click_settings.bd_addr, bd_addr, 6);
                        app_avp_cback(BT_AVP_EVENT_CLICK_SETTINGS, &param, sizeof(param));
                    }
                }
                /*pro click settings: anc apt cycle*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x1A))
                {
                    param.cycle_settings.setting = p_msg->p_data[7];

                    if (app_avp_cback)
                    {
                        memcpy(param.cycle_settings.bd_addr, bd_addr, 6);
                        app_avp_cback(BT_AVP_EVENT_ANC_APT_CYCLE, &param, sizeof(param));
                    }
                }
                /*compactness test*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x21) && (p_msg->p_data[5] == 0x00))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.compactness_test.bd_addr, bd_addr, 6);
                        app_avp_cback(BT_AVP_EVENT_COMPACTNESS_TEST, &param, sizeof(param));
                    }
                }
                /*click speed*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x17))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.click_speed_settings.bd_addr, bd_addr, 6);
                        param.click_speed_settings.speed = (T_BT_AVP_CLICK_SPEED)p_msg->p_data[7];
                        app_avp_cback(BT_AVP_EVENT_CLICK_SPEED, &param, sizeof(param));
                    }
                }
                /*long press time*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x18))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.long_press_time_settings.bd_addr, bd_addr, 6);
                        param.long_press_time_settings.time = (T_BT_AVP_LONG_PRESS_TIME)p_msg->p_data[7];
                        app_avp_cback(BT_AVP_EVENT_LONG_RESS_TIME, &param, sizeof(param));
                    }
                }
                /*one bud in ear to open anc*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x1B))
                {
                    memcpy(param.one_bud_anc.bd_addr, bd_addr, 6);

                    if (p_msg->p_data[7] == 0x01)
                    {
                        param.one_bud_anc.is_open = true;
                    }
                    else
                    {
                        param.one_bud_anc.is_open = false;
                    }

                    if (app_avp_cback)
                    {
                        app_avp_cback(BT_AVP_EVENT_ONE_BUD_ANC, &param, sizeof(param));
                    }
                }
                /*air max*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x1C))
                {
                    memcpy(param.digital_crown_settings.bd_addr, bd_addr, 6);

                    if (p_msg->p_data[7] == 0x01)
                    {
                        param.digital_crown_settings.is_clockwise = true;
                    }
                    else if (p_msg->p_data[7] == 0x02)
                    {
                        param.digital_crown_settings.is_clockwise = false;
                    }

                    if (app_avp_cback)
                    {
                        app_avp_cback(BT_AVP_EVENT_DIGITAL_CROWN_SETTINGS, &param, sizeof(param));
                    }
                }
                /*share audio*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x1f) && (p_msg->p_data[5] == 0x00))
                {
                    memcpy(param.audio_sharing.bd_addr, bd_addr, 6);

                    if (p_msg->p_data[6] == p_msg->p_data[7])
                    {
                        param.audio_sharing.is_enable = true;
                    }
                    else
                    {
                        param.audio_sharing.is_enable = false;
                    }

                    if (app_avp_cback)
                    {
                        app_avp_cback(BT_AVP_EVENT_AUDIO_SHARING, &param, sizeof(param));
                    }
                }
                /*volume control setting*/
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x09) && (p_msg->p_data[5] == 0x00) &&
                         (p_msg->p_data[6] == 0x25))
                {
                    memcpy(param.volume_control_settings.bd_addr, bd_addr, 6);

                    if (p_msg->p_data[7] == 0x01)
                    {
                        param.volume_control_settings.is_enable = true;
                    }
                    else if (p_msg->p_data[7] == 0x02)
                    {
                        param.volume_control_settings.is_enable = false;
                    }

                    if (app_avp_cback)
                    {
                        app_avp_cback(BT_AVP_EVENT_VOLUME_CONTROL_SETTINGS, &param, sizeof(param));
                    }
                }
                /*discon event*/
                else if ((p_msg->p_data[0] == 0x02) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x00) && (p_msg->p_data[5] == 0x00))
                {
                    uint8_t disconn_ack[6] = {0x03, 0x00, 0x04, 0x00, 0x00, 0x00};
                    bt_avp_control_data_send(bd_addr, disconn_ack, sizeof(disconn_ack), false);
                }
                else if ((p_msg->p_data[0] == 0x04) && (p_msg->p_data[1] == 0x00) &&
                         (p_msg->p_data[2] == 0x04) && (p_msg->p_data[3] == 0x00) &&
                         (p_msg->p_data[4] == 0x17) && (p_msg->p_data[5] == 0x00))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.spatial_audio.bd_addr, bd_addr, 6);
                        param.spatial_audio.data = &p_msg->p_data[0];
                        param.spatial_audio.len = p_msg->data_length;
                        app_avp_cback(BT_AVP_EVENT_SPATIAL_AUDIO, &param, sizeof(param));
                    }
                }
            }
        }
        break;

    case AVP_MSG_AUDIO_DATA_IND:
        {
            if (p_link != NULL)
            {
                T_AVP_DATA_IND *rx_data = (T_AVP_DATA_IND *)msg_buf;

                /*opcode: ATT_HANDLE_VALUE_IND, handle: 0x0008 */
                /*if ((rx_data->p_data[0] == 0x1d) && (rx_data->p_data[1] == 0x08) &&
                    (rx_data->p_data[2] == 0x00))
                {
                    uint8_t buf[1] = {0x1E};
                    avp_audio_send_data(bd_addr, buf, sizeof(buf));
                    return;
                }*/

                if (rx_data->data_length == 0x05)
                {
                    //voice recognition start
                    if ((rx_data->p_data[0] == 0x01) &&
                        (rx_data->p_data[1] == 0x00) && (rx_data->p_data[2] == 0x04) &&
                        (rx_data->p_data[3] == 0x01) && (rx_data->p_data[4] == 0x07))
                    {
                        if (app_avp_cback)
                        {
                            T_BT_MSG_PAYLOAD payload;
                            memcpy(payload.bd_addr, bd_addr, 6);
                            payload.msg_buf = NULL;

                            bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION, &payload);

                            memcpy(param.voice_recognition_start.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_VOICE_RECOGNITION_START, &param, sizeof(param));
                        }
                    }
                    //voice recognition stop
                    else if ((rx_data->p_data[0] == 0x01) &&
                             (rx_data->p_data[1] == 0x00) && (rx_data->p_data[2] == 0x04) &&
                             (rx_data->p_data[3] == 0x01) && (rx_data->p_data[4] == 0x00))
                    {
                        if (app_avp_cback)
                        {
                            memcpy(param.voice_recognition_stop.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_VOICE_RECOGNITION_STOP, &param, sizeof(param));
                        }
                    }
                }
                else if (rx_data->data_length == 0x02)
                {
                    //encode start
                    if ((rx_data->p_data[0] == 0x01) && (rx_data->p_data[1] == 0x00))
                    {
                        if (app_avp_cback)
                        {
                            T_BT_MSG_PAYLOAD payload;
                            memcpy(payload.bd_addr, bd_addr, 6);
                            payload.msg_buf = NULL;

                            bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION, &payload);

                            memcpy(param.voice_recognition_encode_start.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_VOICE_RECOGNITION_ENCODE_START, &param, sizeof(param));
                        }
                    }
                }
                else if (rx_data->data_length == 0x03)
                {
                    //encode stop
                    if ((rx_data->p_data[0] == 0x01) &&
                        (rx_data->p_data[1] == 0x00) && (rx_data->p_data[2] == 0x00))
                    {
                        siri_seq = 0;

                        if (app_avp_cback)
                        {
                            T_BT_MSG_PAYLOAD payload;
                            memcpy(payload.bd_addr, bd_addr, 6);
                            payload.msg_buf = NULL;

                            memcpy(param.voice_recognition_encode_stop.bd_addr, bd_addr, 6);
                            bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION, &payload);
                            app_avp_cback(BT_AVP_EVENT_VOICE_RECOGNITION_ENCODE_STOP, &param, sizeof(param));
                        }
                    }
                }
                else if (rx_data->data_length == 0x01)
                {
                    //encode stop
                    if (rx_data->p_data[0] == 0)
                    {
                        if (app_avp_cback)
                        {
                            memcpy(param.auto_apt_settings.bd_addr, bd_addr, 6);
                            param.auto_apt_settings.enable = false;
                            app_avp_cback(BT_AVP_EVENT_AUTO_APT_SETTINGS, &param, sizeof(param));
                        }
                    }
                    else if (rx_data->p_data[0] == 0x01)
                    {
                        if (app_avp_cback)
                        {
                            memcpy(param.auto_apt_settings.bd_addr, bd_addr, 6);
                            param.auto_apt_settings.enable = true;
                            app_avp_cback(BT_AVP_EVENT_AUTO_APT_SETTINGS, &param, sizeof(param));
                        }
                    }

                }

                //report start
                /*else if ((rx_data->p_data[0] == 0x01) &&
                         (rx_data->p_data[1] == 0x00) && (rx_data->p_data[2] == 0x04) &&
                         (rx_data->p_data[3] == 0x01) && (rx_data->p_data[4] == 0x03))
                {

                }

                //report stop
                else if ((rx_data->p_data[0] == 0x01) &&
                         (rx_data->p_data[1] == 0x00) && (rx_data->p_data[2] == 0x04) &&
                         (rx_data->p_data[3] == 0x01) && (rx_data->p_data[4] == 0x02))
                {

                }*/

                //error event
                /*else if ((rx_data->p_data[0] == 0x01) &&
                         (rx_data->p_data[1] == 0x00) && (rx_data->p_data[2] == 0x06))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.voice_recognition_error.bd_addr, bd_addr, 6);
                        app_avp_cback(BT_AVP_EVENT_VOICE_RECOGNITION_ERROR, &param, sizeof(param));
                    }
                }*/

                //eq & apt settings
                /*else if ((rx_data->p_data[0] == 0x00) && (rx_data->p_data[1] == 0x00))
                {
                    if (app_avp_cback)
                    {
                        memcpy(param.apt_gain_settings.bd_addr, bd_addr, 6);
                        param.apt_gain_settings.left_ear_gian = rx_data->p_data[36] +
                                                                (rx_data->p_data[37] << 8) +
                                                                (rx_data->p_data[38] << 16) +
                                                                (rx_data->p_data[39] << 24);

                        param.apt_gain_settings.right_ear_gian = rx_data->p_data[84] +
                                                                 (rx_data->p_data[85] << 8) +
                                                                 (rx_data->p_data[86] << 16) +
                                                                 (rx_data->p_data[87] << 24);

                        app_avp_cback(BT_AVP_EVENT_APT_GAIN_SETTINGS, &param, sizeof(param));

                        memcpy(param.apt_tone_settings.bd_addr, bd_addr, 6);
                        param.apt_tone_settings.left_ear_tone = rx_data->p_data[40] +
                                                                (rx_data->p_data[41] << 8) +
                                                                (rx_data->p_data[42] << 16) +
                                                                (rx_data->p_data[43] << 24);

                        param.apt_tone_settings.right_ear_tone = rx_data->p_data[88] +
                                                                 (rx_data->p_data[89] << 8) +
                                                                 (rx_data->p_data[90] << 16) +
                                                                 (rx_data->p_data[91] << 24);

                        app_avp_cback(BT_AVP_EVENT_APT_TONE_SETTINGS, &param, sizeof(param));

                        if ((rx_data->p_data[4] == 0x08) && (rx_data->p_data[5] == 0xac) &&
                            (rx_data->p_data[6] == 0x8e))
                        {
                            memcpy(param.balanced_tone_slight.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_BALANCED_TONE_SLIGHT, &param, sizeof(param));
                        }
                        else if ((rx_data->p_data[4] == 0x62) && (rx_data->p_data[5] == 0x10) &&
                                 (rx_data->p_data[6] == 0xda))
                        {
                            memcpy(param.balanced_tone_moderate.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_BALANCED_TONE_MODERATE, &param, sizeof(param));
                        }
                        else if ((rx_data->p_data[4] == 0xac) && (rx_data->p_data[5] == 0x9c) &&
                                 (rx_data->p_data[6] == 0x3b))
                        {
                            memcpy(param.balanced_tone_strong.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_BALANCED_TONE_STRONG, &param, sizeof(param));
                        }
                        else if ((rx_data->p_data[4] == 0x42) && (rx_data->p_data[5] == 0x60) &&
                                 (rx_data->p_data[6] == 0x13))
                        {
                            memcpy(param.vocal_range_slight.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_VOCAL_RANGE_SLIGHT, &param, sizeof(param));
                        }
                        else if ((rx_data->p_data[4] == 0x25) && (rx_data->p_data[5] == 0x06) &&
                                 (rx_data->p_data[6] == 0x3d))
                        {
                            memcpy(param.vocal_range_moderate.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_VOCAL_RANGE_MODERATE, &param, sizeof(param));
                        }
                        else if ((rx_data->p_data[4] == 0xb8) && (rx_data->p_data[5] == 0x1e) &&
                                 (rx_data->p_data[6] == 0xce))
                        {
                            memcpy(param.vocal_range_strong.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_VOCAL_RANGE_STRONG, &param, sizeof(param));
                        }
                        else if ((rx_data->p_data[4] == 0x39) && (rx_data->p_data[5] == 0xb4) &&
                                 (rx_data->p_data[6] == 0x14))
                        {
                            memcpy(param.brightness_slight.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_BRIGHTNESS_SLIGHT, &param, sizeof(param));
                        }
                        else if ((rx_data->p_data[4] == 0x17) && (rx_data->p_data[5] == 0xd9) &&
                                 (rx_data->p_data[6] == 0xaf))
                        {
                            memcpy(param.brightness_moderate.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_BRIGHTNESS_MODERATE, &param, sizeof(param));
                        }
                        else if ((rx_data->p_data[4] == 0x10) && (rx_data->p_data[5] == 0xd8) &&
                                 (rx_data->p_data[6] == 0x25))
                        {
                            memcpy(param.brightness_strong.bd_addr, bd_addr, 6);
                            app_avp_cback(BT_AVP_EVENT_BRIGHTNESS_STRONG, &param, sizeof(param));
                        }
                        else
                        {
                            BTM_PRINT_INFO0("bt_att_cback: unknown eq settings");
                        }
                    }
                }*/
            }
        }
        break;

    default:
        break;
    }
}

bool bt_avp_couple_battery_level_report(uint8_t bd_addr[6], uint8_t right_ear_level,
                                        uint8_t right_ear_charging,
                                        uint8_t left_ear_level, uint8_t left_ear_charging,
                                        uint8_t case_level, uint8_t case_status)
{
    uint8_t buf[22] = {0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x03, 0x02, 0x01, 0x33,/*batt_level*/
                       0x01, 0x01, 0x04, 0x01, 0x44,/*batt_level*/ 0x02, 0x01, 0x08, 0x01, 0x00,/*case_batt_level*/
                       0x04/*do not display*/, 0x01
                      };

    buf[9] = right_ear_level;
    buf[10] = right_ear_charging;
    buf[14] = left_ear_level;
    buf[15] = left_ear_charging;
    buf[19] = case_level;
    buf[20] = case_status;

    return bt_avp_control_data_send(bd_addr, buf, sizeof(buf), false);
}

bool bt_avp_single_battery_level_report(uint8_t bd_addr[6], uint8_t level, uint8_t charging)
{
    uint8_t buf[12] = {0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01, 0x01, 0x01, 0x33, 0x02, 0x01};

    buf[9] = level;
    buf[10] = charging;

    return bt_avp_control_data_send(bd_addr, buf, sizeof(buf), false);
}

bool bt_avp_bud_location_report(uint8_t bd_addr[6], T_BT_AVP_BUD_LOCATION pri_ear_location,
                                T_BT_AVP_BUD_LOCATION sec_ear_location, bool is_right)
{
    uint8_t buf1[8] = {0x04, 0x00, 0x04, 0x00, 0x06, 0x00, 0x02, 0x02};
    uint8_t buf2[10] = {0x04, 0x00, 0x04, 0x00, 0x08, 0x00, 0x02, 0x02, 0x01, 0x00};

    if (pri_ear_location == BT_AVP_BUD_IN_EAR)
    {
        buf1[6] = 0x00;
    }
    else if (pri_ear_location == BT_AVP_BUD_OUT_OF_CASE)
    {
        buf1[6] = 0x01;
    }
    else if (pri_ear_location == BT_AVP_BUD_IN_CASE)
    {
        buf1[6] = 0x02;
    }
    else if (pri_ear_location == BT_AVP_BUD_UNKNOWN)
    {
        buf1[6] = 0x03;
    }

    if (sec_ear_location == BT_AVP_BUD_IN_EAR)
    {
        buf1[7] = 0x00;
    }
    else if (sec_ear_location == BT_AVP_BUD_OUT_OF_CASE)
    {
        buf1[7] = 0x01;
    }
    else if (sec_ear_location == BT_AVP_BUD_IN_CASE)
    {
        buf1[7] = 0x02;
    }
    else if (sec_ear_location == BT_AVP_BUD_UNKNOWN)
    {
        buf1[7] = 0x03;
    }

    if (is_right)
    {
        buf2[6] = 0x02;
    }
    else
    {
        buf2[6] = 0x01;
    }
    bt_avp_control_data_send(bd_addr, buf1, sizeof(buf1), false);
    bt_avp_control_data_send(bd_addr, buf2, sizeof(buf2), false);
    return true;
}

bool bt_avp_anc_setting_report(uint8_t bd_addr[6], T_BT_AVP_ANC anc_setting)
{
    if (anc_setting == BT_AVP_ANC_CLOSE)
    {
        uint8_t buf[11] = {0x04, 0x00, 0x04, 0x00, 0x09, 0x00, 0x0D, 0x01, 0x00, 0x00, 0x00};
        bt_avp_control_data_send(bd_addr, buf, sizeof(buf), false);
    }
    else if (anc_setting == BT_AVP_ANC_OPEN)
    {
        uint8_t buf[11] = {0x04, 0x00, 0x04, 0x00, 0x09, 0x00, 0x0D, 0x02, 0x00, 0x00, 0x00};
        bt_avp_control_data_send(bd_addr, buf, sizeof(buf), false);
    }
    else if (anc_setting == BT_AVP_ANC_TRANSPARENCY_MODE)
    {
        uint8_t buf[11] = {0x04, 0x00, 0x04, 0x00, 0x09, 0x00, 0x0D, 0x03, 0x00, 0x00, 0x00};
        bt_avp_control_data_send(bd_addr, buf, sizeof(buf), false);
    }

    return true;
}

bool bt_avp_compactness_test_report(uint8_t bd_addr[6], bool right_ear_result, bool left_ear_result)
{
    if (right_ear_result == true)
    {
        if (left_ear_result == true)
        {
            bt_avp_control_data_send(bd_addr, (uint8_t *)all_bud_is_compacted,
                                     sizeof(all_bud_is_compacted), false);
        }
        else
        {
            bt_avp_control_data_send(bd_addr, (uint8_t *)right_bud_is_compacted,
                                     sizeof(right_bud_is_compacted), false);
        }
    }
    else
    {
        if (left_ear_result == true)
        {
            bt_avp_control_data_send(bd_addr, (uint8_t *)left_bud_is_compacted,
                                     sizeof(left_bud_is_compacted), false);
        }
        else
        {
            bt_avp_control_data_send(bd_addr, (uint8_t *)all_bud_is_not_compacted,
                                     sizeof(all_bud_is_not_compacted), false);
        }
    }

    return true;
}

bool bt_avp_init(P_BT_AVP_CBACK cback)
{
    BTM_PRINT_INFO0("bt_avp_init");
    app_avp_cback = cback;
    avp_init(bt_avp_cback);
    return true;
}

#else
#include <stdint.h>
#include <stdbool.h>
#include "bt_avp.h"

bool bt_avp_data_send(uint8_t   bd_addr[6],
                      uint8_t  *p_data,
                      uint16_t  data_len,
                      bool      flush)
{
    return false;
}

bool bt_avp_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avp_init(P_BT_AVP_CBACK cback)
{
    return false;
}
#endif
